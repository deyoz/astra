#ifndef _MVT_PARSER_H
#define _MVT_PARSER_H

#include "tlg_parser.h"

namespace TypeB
{

    struct TAD {
        BASIC::TDateTime off_block_time;
        BASIC::TDateTime airborne_time;
        BASIC::TDateTime ea; // Estimated Arrival
        std::string airp_arv;
        void dump();
        void parse(const std::string &val);
        TAD():
            off_block_time(ASTRA::NoExists),
            airborne_time(ASTRA::NoExists),
            ea(ASTRA::NoExists)
            {}
    };

    class TMVTContent
    {
        public:
            TAD ad;
            void Clear() {};
    };

    void ParseMVTContent(TTlgPartInfo body, TAHMHeadingInfo& info, TMVTContent& con, TMemoryManager &mem);
    void SaveMVTContent(int tlg_id, TAHMHeadingInfo& info, TMVTContent& con);

}


#endif
