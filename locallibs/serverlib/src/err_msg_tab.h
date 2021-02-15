#pragma once

#include "helpcpp.h"
#include "lngv.h"

#include <map>
#include <vector>

namespace ErrorMessage {

struct MsgLine {
    unsigned id;         /* Числовой идентификатор сообщения */
    const char * id_str; /* Идентификатор сообщения в виде строки */
    const char * eng;
    const char * rus;
};

template<class T>
class Adapter {

    std::map<unsigned, const MsgLine *> idToLine;
    std::map<const char *, const MsgLine *, HelpCpp::cstring_less> stringIdToLine;
    std::vector<const MsgLine *> sortedLines;

  public:
    Adapter(T& table) {
        for (const auto& line : table) {
            idToLine.emplace(std::make_pair(line.id, &line));
            stringIdToLine.emplace(std::make_pair(line.id_str, &line));
        }
        for (const auto& idLine: idToLine) {
            sortedLines.push_back(idLine.second);
        }
    }

    const char * getStringId(unsigned id) const {
        const auto iterator = idToLine.find(id);
        if (idToLine.end() != iterator) {
            return iterator->second->id_str;
        }
        return nullptr;
    }

    const char * getMessage(unsigned id, Language lang) const {
        const auto iterator = idToLine.find(id);
        if (idToLine.end() != iterator) {
            return RUSSIAN == lang ? iterator->second->rus : iterator->second->eng;
        }
        return nullptr;
    }

    unsigned getId(const char * StringId) const {
        const auto iterator = stringIdToLine.find(StringId);
        if (stringIdToLine.end() != iterator) {
            return iterator->second->id;
        }
        return 0;
    }

    const std::vector<const MsgLine *>& getSortedList() const {
        return sortedLines;
    }
}; // class Adapter

} // namespace ErrorMessage
