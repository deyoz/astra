/*
 * tscript - реализация функции >> (сравнение текстовых блоков)
 */

#ifdef XP_TESTING

#include "tscript.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/regex.hpp>
#include "exception.h"
#include "func_placeholders.h"
#include "levenshtein.h"
#include "tscript_diff.h"
#include "checkunit.h"

#define NICKNAME "DMITRYVM"
#include "slogger.h"

#define THROW_MSG(msg)\
    do {\
        std::ostringstream msgStream;\
        msgStream << msg;\
        throw comtech::Exception(STDLOG, __FUNCTION__, maybe_recode_answer(msgStream.str()));\
    } while (0)

namespace xp_testing { namespace tscript {

    static void Compare(const std::string& text, const std::string& expectedText)
    {
        LogTrace(TRACE5) << __FUNCTION__ << ": text =[\n" << text << "]";
        LogTrace(TRACE5) << __FUNCTION__ << ": expectedText =[\n" << expectedText << "]";

        const bool haveDiff = PrintDiff(expectedText, text);
        if (haveDiff)
            THROW_MSG("text mismatch");
    }

    static void CompareRe(const std::string& text, const std::string& re_text, bool mustMatch)
    {
        std::string reStr;
        if (!re_text.empty() && *re_text.rbegin() == '\n') {
            reStr.assign(re_text, 0, re_text.length() - 1);
        } else {
            reStr = re_text;
        }
        const boost::regex re(reStr);
        boost::smatch m;
        const bool match = boost::regex_match(text, m, re);
        
        GetTestContext()->captures.clear();
        for (size_t i = 0; i < m.size(); ++i)
            GetTestContext()->captures[i] = m[i];

        if (match != mustMatch) {
            LogTrace(TRACE1) << __FUNCTION__ << ": text:\n[" << text << "]";
            LogTrace(TRACE1) << __FUNCTION__ << ": re:\n[" << reStr << "]";
            if (mustMatch)
                THROW_MSG("regex mismatch: " << reStr);
            else
                THROW_MSG("regex must not match: " << reStr);
        }
    }

    static void CheckOutqNotEmpty(const std::queue<std::string>& outq, const std::string& expected)
    {
        if (outq.empty())
            THROW_MSG("number of results in FIFO less than expected"
                      "\n at least this one is expected: \n" << expected);
    }

    typedef std::vector<std::pair<int, int> > LineRanges_t;

    /*
     * БНФ:
     *     range ::= N (эквивалентно N:N)
     *     range ::= N:M
     *     range ::= N-M
     *     ranges ::= range[,range[, ...]]
     *
     * N, M - номера строк начала и конца диапазона (включительно).
     * Отрицательное число означает отсчёт с конца.
     * Таким образом, 1 = первая строка, -1 = последняя.
     *
     * Например, "1:3,5:-1" означает объединение двух диапазонов:
     * от 1-ой до 3-ей строки (включительно) и от 5-ой до последней (включительно).
     */
    static LineRanges_t ParseLineRanges(const std::string& text)
    {
        std::vector<std::string> ranges;
        boost::split(ranges, text, boost::is_any_of(","));

        static const boost::regex re1("(-?\\d+)");
        static const boost::regex re2("(-?\\d+)[:-](-?\\d+)");
        
        LineRanges_t result;
        for (const std::string& range:  ranges) {
            LogTrace(TRACE5) << __FUNCTION__ << ": range = \"" << range << "\"";
            boost::smatch m;
            if (boost::regex_match(range, m, re1))
                result.push_back(std::make_pair(std::stoi(m[1]), std::stoi(m[1])));
            else if (boost::regex_match(range, m, re2))
                result.push_back(std::make_pair(std::stoi(m[1]), std::stoi(m[2])));
            else
                THROW_MSG("invalid range: \"" + range + "\"");
        }
        return result;
    }

    static bool IsEqualChars(char a, char b)
    {
        return a == 'x' || a == b;
    }
   
    /* Пытаемся найти самый похожий на block диапазон строк в векторе lines */
    static LineRanges_t AutoLineRanges(const std::vector<std::string>& lines, const std::vector<std::string>& block)
    {
        if (lines.size() <= block.size())
            return LineRanges_t(1, std::make_pair(1, lines.size()));

        unsigned minDistance = std::numeric_limits<unsigned>::max();
        size_t minDistanceOffset = 0;

        for (size_t offset = 0; offset <= lines.size() - block.size(); ++offset) {
            /* Вычисляем расстояние Левенштейна для текущего смещения */
            unsigned distance = 0;
            for (size_t i = 0; i < block.size(); ++i)
                distance += LevenshteinDistance(block.at(i), lines.at(offset + i), IsEqualChars);

            LogTrace(TRACE5) << __FUNCTION__ << ": distance = " << distance << ", offset = " << offset;
            if (distance < minDistance) {
                minDistance = distance;
                minDistanceOffset = offset;
            }
        }

        LogTrace(TRACE5) << __FUNCTION__ << ": min distance = " << minDistance << ", offset = " << minDistanceOffset;
        return LineRanges_t(1, std::make_pair(minDistanceOffset + 1, minDistanceOffset + block.size()));
    }

    static int GetLineNum(int rangeNum, int nlines)
    {
        if (rangeNum < 0)
            rangeNum = nlines + rangeNum + 1; /* Например, -1 соответствует последней строке */
        return std::max(1, std::min(nlines, rangeNum));
    }

    static std::vector<std::string> SelectLines(const std::vector<std::string>& lines, const LineRanges_t& ranges)
    {
        if (lines.empty())
            return lines;

        std::vector<std::string> result;
        for (const LineRanges_t::value_type& range:  ranges) {
            const int from = GetLineNum(range.first, (int)lines.size());
            const int to = GetLineNum(range.second, (int)lines.size());

            LogTrace(TRACE5) << __FUNCTION__ << ": from = " << from << ", to = " << to << ", lines.size() = " << lines.size();
            if (from < to)
                for (int i = from; i <= to; ++i)
                    result.push_back(lines.at(i - 1));
            else
                for (int i = from; i >= to; --i)
                    result.push_back(lines.at(i - 1));
        }
        return result;
    }

    static std::vector<std::string> SplitLines(const std::string& text)
    {
        std::vector<std::string> result;
        std::string line;
        for (std::string::const_iterator c = text.begin(); c != text.end(); ++c) {
            line += *c;
            if (*c == '\n') {
                result.push_back(line);
                line.clear();
            }
        }
        if (!line.empty())
            result.push_back(line);
        return result;
    }

    std::string FP_cmp(const std::vector<tok::Param>& params)
    {
        std::queue<std::string>& outq = GetTestContext()->outq;
        LogTrace(TRACE5) << __FUNCTION__ << ": top: outq.size() = " << outq.size();

        ValidateParams(params, 1, 1, "mode lines");
        const std::string text = PositionalValues(params).at(0);

        CheckOutqNotEmpty(outq, text);

        const std::string mode = tok::GetValue(params, "mode", "text");
        const std::string linesParam = tok::GetValue(params, "lines", "1:-1");

        const std::vector<std::string> splitted = SplitLines(outq.front());
        const LineRanges_t ranges = (linesParam == "auto") ?
            AutoLineRanges(splitted, SplitLines(text)) :
            ParseLineRanges(linesParam);
        const std::string selection = boost::join(SelectLines(splitted, ranges), "");

        if (mode == "text")
            Compare(selection, text);
        else if (mode == "regex")
            CompareRe(selection, text, true /* mustMatch */);
        else if (mode == "!regex")
            CompareRe(selection, text, false /* mustMatch */);
        else
            THROW_MSG("invalid mode: " << mode);

        outq.pop();
        return std::string();
    }

}} /* namespace xp_testing::tscript */

#endif // XP_TESTING
