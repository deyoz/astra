#ifdef XP_TESTING
#include "tscript_diff.h"
#include <algorithm>
#include <ostream>
#include <boost/algorithm/string/split.hpp>
#include <boost/regex.hpp>
#include <boost/optional.hpp>
#include <unistd.h>
#include "text_diff.h"
#include "slogger_nonick.h"
#include "logcout.h"
#include "checkunit.h"
#include <iostream>

namespace xp_testing { namespace tscript {

    static const char* RESET_COLOR = "\e[0m";
    /* new */
    static const char* COLOR_NEW = "\e[37m";
    static const char* COLOR_NEW_WS = "\e[44m";
    /* old */
    static const char* COLOR_OLD = "\e[36m";
    static const char* COLOR_OLD_WS = "\e[44m";
    /* del */
    static const char* COLOR_DEL = "\e[1;31m";
    static const char* COLOR_DEL_WS = "\e[44m";
    /* add */
    static const char* COLOR_ADD = "\e[1;32m";
    static const char* COLOR_ADD_WS = "\e[44m";

    static void PrintLine(std::ostream& o, const char* color, const char* colorWs, const std::string& l)
    {
        auto line = maybe_recode_answer(std::string(l)); // да ничего, "это ж тесты", куда спешить...
        static int tty = isatty(1);
        if (!tty) {
            o << line << std::endl;
            return;
        }

        size_t wsLen = 0;
        while (wsLen < line.size() && *(line.rbegin() + wsLen) == ' ') 
            ++wsLen;

        o << color << line.substr(0, line.size() - wsLen);
        if (wsLen > 0)
            o << colorWs << line.substr(line.size() - wsLen);
        o << RESET_COLOR << std::endl;
    }

    static bool IsNewline(char c)
    {
        return c == '\n';
    }

    /* All special characters in default Boost regex syntax */
    static bool IsSpecialRegexChar(char c)
    {
        static const std::string specialChars = ".[{}()\\*+?|^$";
        return specialChars.find(c) != std::string::npos;
    }

    /* Checks for stand-alone ellipsis at given position i */
    static bool CheckEllipsis(const std::string& s, size_t i)
    {
        return i + 2 < s.size()
            && s.at(i + 0) == '.'
            && s.at(i + 1) == '.'
            && s.at(i + 2) == '.'
            && (          i == 0  || s.at(i - 1) != '.')
            && (i + 3 == s.size() || s.at(i + 3) != '.');
    }

    static boost::optional<std::string> MakePatternRegex(const std::string& pattern)
    {
        bool needRegex = false;
        std::string result;
        result.reserve(pattern.size());
        size_t i = 0;
        while (i < pattern.size()) {
            const char c = pattern.at(i);
            ++i;
            /* c == '.' here just for speed optimization, CheckEllipsis is sufficient otherwise */
            if (c == '.' && CheckEllipsis(pattern, i - 1)) {
                /* stand-alone "..." means sequence of any characters */
                result += ".*";
                needRegex = true;
                i += 2;
            } else if (c == 'x') {
                result += '.';
                needRegex = true;
            } else if (IsSpecialRegexChar(c)) {
                result += std::string("\\") + c;
            } else
                result += c;
        }
        return needRegex ? result : boost::optional<std::string>();
    }

    static bool IsEqualLines(const std::string& pattern, const std::string& line)
    {
        const boost::optional<std::string> re = MakePatternRegex(pattern);
        return re ?
            boost::regex_match(line, boost::regex(*re)) :
            pattern == line;
    }

    static void PrintNewLine(std::ostream& o, const std::string& line)
    {
        PrintLine(o, COLOR_NEW, COLOR_NEW_WS, line);
    }

    static void PrintDiffLine(std::ostream& o, const TextDiffLine& diffLine)
    {
        switch (diffLine.type) {
            case TextDiffLine::OLD:
                PrintLine(o, COLOR_OLD, COLOR_OLD_WS, diffLine.line);
                break;
            case TextDiffLine::DEL:
                PrintLine(o, COLOR_DEL, COLOR_DEL_WS, "-" + diffLine.line);
                break;
            case TextDiffLine::ADD:
                PrintLine(o, COLOR_ADD, COLOR_ADD_WS, "+" + diffLine.line);
                break;
        }
    }

    static void PrintDiff_(std::ostream& o, std::vector<std::string>& newLines, const std::list<TextDiffLine>& diff)
    {
        o << "<------------------ new -----------------------" << std::endl;
        for(auto& l : newLines)  PrintNewLine(o,l);
        o << "------------------- diff ----------------------" << std::endl;
        for(auto& l : diff)  PrintDiffLine(o,l);
        o << "----------------- end diff ------------------->" << std::endl;
    }

    bool PrintDiff(const std::string& oldText, const std::string& newText)
    {
        std::vector<std::string> oldLines;
        std::vector<std::string> newLines;
        boost::split(oldLines, oldText, IsNewline);
        boost::split(newLines, newText, IsNewline);

        const std::list<TextDiffLine> diff = TextDiff(oldLines, newLines, IsEqualLines);
        if (!diff.empty())
            PrintDiff_(std::cout, newLines, diff);
        return !diff.empty();
    }

}} /* namespace */

#ifdef DEBUG_TSCRIPT_DIFF
#include <fstream>
#include <iterator>

static std::string ReadFile(const char* file)
{
    std::ifstream in(file);
    std::string data;
    std::copy(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(), std::inserter(data, data.end()));
    return data;
}

int main(int argc, char* argv[])
{
    const bool haveDiff = xp_testing::tscript::PrintDiff(ReadFile(argv[1]), ReadFile(argv[2]));
    LogCout(COUT_INFO) << "haveDiff = " << haveDiff << std::endl;
    return 0;
}
#endif /* #ifdef DEBUG_TSCRIPT_DIFF */
#endif // XP_TESTING
