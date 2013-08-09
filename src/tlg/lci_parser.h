#ifndef _LCI_PARSER_H
#define _LCI_PARSER_H

#include "tlg_parser.h"

namespace TypeB
{

struct TLCIFltInfo {
    TFlightIdentifier flt;
    std::string airp;
    void parse(const char *val);
    void dump();
};

class TLCIHeadingInfo : public THeadingInfo
{
    public:
        TLCIFltInfo flt_info;
        TLCIHeadingInfo(THeadingInfo &info) : THeadingInfo(info) {};
};

TTlgPartInfo ParseLCIHeading(TTlgPartInfo heading, TLCIHeadingInfo &info);

enum TOriginator {
    oLoadcontrol,
    oCheckin,
    oUnknown
};

enum TAction {
    aOpen,
    aSuspension,
    aClose,
    aUpdate,
    aFinalised,
    aException,
    aRequest,
    aUnknown
};

struct TActionCode {
    TOriginator orig;
    TAction action;
    void parse(const char *val);
    void dump();
    TActionCode(): orig(oUnknown), action(aUnknown) {};
};

struct TCFG:public std::map<std::string, int> {
    void parse(const std::string &val);
    void dump();
};

struct TEQT {
    std::string bort, craft;
    TCFG cfg;
    void parse(const char *val);
    void dump();
};

enum TMeasur {
    mKG,
    mLB,
    mUnknown
};

struct TWeight {
    int amount;
    TMeasur measur;
    TWeight(): amount(ASTRA::NoExists), measur(mUnknown) {};
};

struct TWA {
    TWeight payload, underload;
    void parse(const char *val);
    void dump();
};

struct TSM {
    void parse(const char *val) {};
};

struct TSR {
    void parse(const char *val) {};
};

struct TWM {
    void parse(const char *val) {};
};

struct TPD {
    void parse(const char *val) {};
};

struct TSP {
    void parse(const char *val) {};
};

struct TDestInfo {
};

struct TDest:public std::map<std::string, TDestInfo> {
    void parse(const char *val);
};

class TLCIContent
{
    public:
        TActionCode action_code;
        TDest dst;
        TEQT eqt;
        TWA wa;
        TSM sm;
        TSR sr;
        TWM wm;
        TPD pd;
        TSP sp;
        void Clear() {};
        void dump();
};

void ParseLCIContent(TTlgPartInfo body, TLCIHeadingInfo& info, TLCIContent& con, TMemoryManager &mem);
void SaveLCIContent(int tlg_id, TLCIHeadingInfo& info, TLCIContent& con);

int lci(int argc,char **argv);

}


#endif
