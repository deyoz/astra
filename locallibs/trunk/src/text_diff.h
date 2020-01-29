#ifndef __TEXT_DIFF_H
#define __TEXT_DIFF_H

#include <list>
#include <vector>
#include <string>

struct TextDiffLine {
    enum Type { OLD, DEL, ADD };
    Type type;
    std::string line;
    TextDiffLine(Type type, const std::string& line): type(type), line(line) {}
};

std::list<TextDiffLine> TextDiff(
        const std::vector<std::string>& oldLines,
        const std::vector<std::string>& newLines,
        bool (*isEqual)(const std::string& oldLine, const std::string& newLine));

#endif
