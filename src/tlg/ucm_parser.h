#ifndef _UCM_PARSER_H
#define _UCM_PARSER_H

#include "tlg_parser.h"

namespace TypeB
{

struct TUCMFltInfo {
    std::string src; // whole tlg line with flt as is
    std::string airline;
    std::string airp;
    int flt_no;
    std::string suffix;
    TDateTime date;
    TUCMFltInfo() { clear(); }
    void clear();
    TFltInfo toFltInfo();
    void parse(const char *val, TFlightsForBind &flts, TTlgCategory tlg_cat);
    std::string toString();
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
