#ifndef _UCM_PARSER_H
#define _UCM_PARSER_H

#include "tlg_parser.h"

namespace TypeB
{

struct TUCMFltInfo {
    std::string airline;
    std::string airp;
    int flt_no;
    std::string suffix;
    TDateTime date;
    TUCMFltInfo():
        airp("òêå"), // !!!
        flt_no(ASTRA::NoExists),
        date(ASTRA::NoExists)
    {}
    TFltInfo toFltInfo();
    void parse(const char *val, TFlightsForBind &flts);
};

class TUCMHeadingInfo : public THeadingInfo
{
    public:
        TUCMFltInfo flt_info;
        TUCMHeadingInfo(THeadingInfo &info) : THeadingInfo(info) {};
};

TTlgPartInfo ParseUCMHeading(TTlgPartInfo heading, TUCMHeadingInfo &info, TFlightsForBind &flts);

}

#endif
