#ifndef _MVT_PARSER_H
#define _MVT_PARSER_H

#include "tlg_parser.h"

namespace TypeB
{
    namespace MVTParser {
        using BASIC::date_time::TDateTime;

        struct TAD {
            TDateTime off_block_time;
            TDateTime airborne_time;
            TDateTime ea; // Estimated Arrival
            std::string airp_arv;
            void dump();
            static TDateTime fetch_time(TDateTime scd, const std::string &val);
            static TDateTime nearest_date(TDateTime time, int day);
            void parse(TDateTime scd, const std::string &val);
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
}


#endif
