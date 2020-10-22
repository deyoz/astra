/* diff using LCS algorithm */

#include "text_diff.h"
#include <cassert>

struct LcsMatrixElem {
    size_t lcsLen;
    bool eq;
    LcsMatrixElem(): lcsLen(0), eq(false) {}
};

std::list<TextDiffLine> TextDiff(
        const std::vector<std::string>& oldLines,
        const std::vector<std::string>& newLines,
        bool (*isEqual)(const std::string& oldLine, const std::string& newLine))
{
    /* oldLines prefix len = col,
     * newLines prefix len = row
     */
    std::vector<std::vector<LcsMatrixElem> > m(
            newLines.size() + 1,
            std::vector<LcsMatrixElem>(oldLines.size() + 1));

    /* lines numbered from 1 */
    for (size_t row = 1; row <= newLines.size(); ++row)
    {
        for (size_t col = 1; col <= oldLines.size(); ++col) 
        {
            const std::string& newLine = newLines.at(row - 1);
            const std::string& oldLine = oldLines.at(col - 1);

            LcsMatrixElem& elem = m.at(row).at(col); /* current */
            const LcsMatrixElem& u = m.at(row - 1).at(col); /* up */
            const LcsMatrixElem& l = m.at(row).at(col - 1); /* left */

            elem.eq = isEqual(oldLine, newLine);
            if (elem.eq) {
                elem.lcsLen = 1 + m.at(row - 1).at(col - 1).lcsLen;
            } else
                elem.lcsLen = u.lcsLen > l.lcsLen ? u.lcsLen : l.lcsLen;
        }
    }

    /* backtrack with diff generation */
    std::list<TextDiffLine> diff;
    size_t row = newLines.size();
    size_t col = oldLines.size();
    bool haveDiff = false;
    while (row > 0 || col > 0)
        if (m.at(row).at(col).eq) {
            assert(row > 0 && col > 0);
            diff.push_front(TextDiffLine(TextDiffLine::OLD, oldLines.at(col - 1)));
            --row;
            --col;
        } else {
            haveDiff = true;
            if (row > 0 && (col == 0 || m.at(row - 1).at(col).lcsLen >= m.at(row).at(col - 1).lcsLen)) {
                diff.push_front(TextDiffLine(TextDiffLine::ADD, newLines.at(row - 1)));
                --row;
            } else {
                assert(col > 0);
                diff.push_front(TextDiffLine(TextDiffLine::DEL, oldLines.at(col - 1)));
                --col;
            }
        }

    if (!haveDiff)
        diff.clear();
    return diff;
}

