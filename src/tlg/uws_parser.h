#pragma once

#include "tlg_parser.h"

namespace TypeB
{
    namespace UWSParser {

        typedef std::map<bool, int> MailMap;
        typedef std::map<std::string, MailMap> AirpMap;

        class TUWSContent
        {
            public:
                AirpMap data;
                void Clear() {}
        };

        void ParseUWSContent(TTlgPartInfo body, TAHMHeadingInfo& info, TUWSContent& con, TMemoryManager &mem);
        void SaveUWSContent(int tlg_id, TAHMHeadingInfo& info, TUWSContent& con);
    }
}
