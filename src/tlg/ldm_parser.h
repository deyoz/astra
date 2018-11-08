#ifndef _LDM_PARSER_H
#define _LDM_PARSER_H

#include "tlg_parser.h"
#include "ucm_parser.h"

namespace TypeB
{
    namespace LDMParser {
        struct TLDMContent {
            std::string bort;
            int cockpit, cabin;
            void clear();
            TLDMContent() { clear(); }
            std::string toString();
        };

        void ParseLDMContent(TTlgPartInfo body, TUCMHeadingInfo& info, TLDMContent& con, TMemoryManager &mem);
        void SaveLDMContent(int tlg_id, TUCMHeadingInfo& info, TLDMContent& con);
    }
}

#endif
