/* diff using LCS algorithm */

#include "text_diff.h"
#include <cassert>
#include <optional>
#include <regex>
#include <map>
#include <bitset>

struct LcsMatrixElem {
    size_t lcsLen;
    bool eq;
    LcsMatrixElem(): lcsLen(0), eq(false) {}
};

static std::bitset<128> makeSpecialChars(const std::string& chars)
{
    std::bitset<128> result;
    for (char c : chars) {
        result[c] = true;
    }
    return result;
}

/* All special characters in default Boost regex syntax */
static bool IsSpecialRegexChar(char c)
{
    static std::bitset<128> mask = makeSpecialChars(".[]{}()\\*+?|^$");
    return c > 0 && c < 128 && mask[c];
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

static std::optional<std::regex> MakePatternRegex(const std::string& pattern)
{
    bool needRegex = false;
    std::string result;
    result.reserve(pattern.size() + std::count_if(pattern.begin(), pattern.end(), IsSpecialRegexChar));
    size_t i = 0;
    while(i < pattern.size()) {
        const char c = pattern[i];
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
    if (!needRegex) {
        return {};
    }

    static std::map<std::string, std::regex> regex;
    const auto iter = regex.find(result);
    if (iter != regex.cend()) {
        return iter->second;
    }

    const auto re = std::regex(result);
    regex.insert(std::make_pair(result, re));
    return re;
}

using LcsMatrix = std::vector<std::vector<LcsMatrixElem> >;

template<typename F>
static void fillColumn(size_t col, const std::vector<std::string>& newLines, LcsMatrix& m, F match)
{
    for (size_t row = 1; row <= newLines.size(); ++row) {
        const std::string& newLine = newLines[row - 1];

        LcsMatrixElem& elem = m[row][col]; /* current */
        const LcsMatrixElem& u = m[row - 1][col]; /* up */
        const LcsMatrixElem& l = m[row][col - 1]; /* left */

        elem.eq = match(newLine);
        if (elem.eq) {
            elem.lcsLen = 1 + m[row - 1][col - 1].lcsLen;
        } else
            elem.lcsLen = u.lcsLen > l.lcsLen ? u.lcsLen : l.lcsLen;
    }
}

std::list<TextDiffLine> TextDiff(
        const std::vector<std::string>& oldLines,
        const std::vector<std::string>& newLines,
        bool (*isEqual)(const std::string& oldLine, const std::string& newLine))
{
    /* oldLines prefix len = col,
     * newLines prefix len = row
     */
    const bool checkRegex = !isEqual;
    LcsMatrix m(
            newLines.size() + 1,
            std::vector<LcsMatrixElem>(oldLines.size() + 1));

    /* lines numbered from 1 */
    std::optional<std::regex> re;
    for (size_t col = 1; col <= oldLines.size(); ++col) {
        const std::string& oldLine = oldLines[col - 1];

        if (checkRegex) {
            re = MakePatternRegex(oldLine);
        } else {
            re = std::nullopt;
        }

        if (isEqual) {
            fillColumn(col, newLines, m, [&](const std::string& newLine) {
                return isEqual(oldLine, newLine);
            });
        } else if (re) {
            const auto& r = *re;
            fillColumn(col, newLines, m, [&](const std::string& newLine){
                return std::regex_match(newLine, r);
            });
        } else {
            fillColumn(col, newLines, m, [&](const std::string& newLine) {
                return newLine == oldLine;
            });
        }
    }

    /* backtrack with diff generation */
    std::list<TextDiffLine> diff;
    size_t row = newLines.size();
    size_t col = oldLines.size();
    bool haveDiff = false;
    while (row > 0 || col > 0)
        if (m[row][col].eq) {
            assert(row > 0 && col > 0);
            diff.push_front(TextDiffLine(TextDiffLine::OLD, oldLines[col - 1]));
            --row;
            --col;
        } else {
            haveDiff = true;
            if (row > 0 && (col == 0 || m[row - 1][col].lcsLen >= m[row][col - 1].lcsLen)) {
                diff.push_front(TextDiffLine(TextDiffLine::ADD, newLines[row - 1]));
                --row;
            } else {
                assert(col > 0);
                diff.push_front(TextDiffLine(TextDiffLine::DEL, oldLines[col - 1]));
                --col;
            }
        }

    if (!haveDiff)
        diff.clear();
    return diff;
}

