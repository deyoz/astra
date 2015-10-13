#ifndef _MVT_PARSER_H
#define _MVT_PARSER_H

#include "tlg_parser.h"

namespace TypeB
{
    namespace MVTParser {
        BASIC::TDateTime fetch_time(BASIC::TDateTime scd, const std::string &val);
        BASIC::TDateTime nearest_date(BASIC::TDateTime time, int day);

        struct TAD {
            BASIC::TDateTime off_block_time;
            BASIC::TDateTime airborne_time;
            BASIC::TDateTime ea; // Estimated Arrival
            std::string airp_arv;
            void dump();
            void parse(BASIC::TDateTime scd, const std::string &val);
            TAD():
                off_block_time(ASTRA::NoExists),
                airborne_time(ASTRA::NoExists),
                ea(ASTRA::NoExists)
            {}
        };

        struct TAA {
            BASIC::TDateTime touch_down_time;
            BASIC::TDateTime on_block_time;
            BASIC::TDateTime fld_time; // Flight Leg Date - UTC Scheduled Date of Departure for Flight Leg
            void parse(BASIC::TDateTime scd, const std::string &val);
            TAA():
                touch_down_time(ASTRA::NoExists),
                on_block_time(ASTRA::NoExists),
                fld_time(ASTRA::NoExists)
            {}
        };

        enum TMVTMsg {
            msgAD, // departure message
            msgAA, // arrival message
            msgNum
        };

        class TMVTContent
        {
            public:
                TMVTMsg msg_type;
                TAD ad;
                TAA aa;
                void Clear() {};
                TMVTContent(): msg_type(msgNum) {};
        };

        void ParseMVTContent(TTlgPartInfo body, TAHMHeadingInfo& info, TMVTContent& con, TMemoryManager &mem);
        void SaveMVTContent(int tlg_id, TAHMHeadingInfo& info, TMVTContent& con);

    }
}


#endif
