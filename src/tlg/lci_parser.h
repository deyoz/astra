#ifndef _LCI_PARSER_H
#define _LCI_PARSER_H

#include "tlg_parser.h"
#include <tr1/memory>


namespace TypeB
{

struct TLCIFltInfo {
    TFlightIdentifier flt;
    std::string airp;
    TFltInfo toFltInfo();
    void parse(const char *val, TFlightsForBind &flts);
    void dump();
};

class TLCIHeadingInfo : public THeadingInfo
{
    public:
        TLCIFltInfo flt_info;
        TLCIHeadingInfo(THeadingInfo &info) : THeadingInfo(info) {};
};

TTlgPartInfo ParseLCIHeading(TTlgPartInfo heading, TLCIHeadingInfo &info, TFlightsForBind &flts);

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
    void parse(const std::string &val, const TElemType el);
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

enum TSeatingMethod {
    smFree,
    smClass,
    smZone,
    smRow,
    smSeat,
    smUnknown
};

struct TSM {
    TSeatingMethod value;
    void parse(const char *val);
    void dump();
    TSM(): value(smUnknown) {};
};

struct TSRZones:public std::map<std::string, int> {
    void parse(const std::string &val);
    void dump();
};

struct TSRItems:public std::vector<std::string> {
    void parse(const std::string &val);
    void dump();
};

struct TSRJump {
    int amount;
    std::vector<std::string> seats;
    TSRZones zones;
    TSRJump(): amount(ASTRA::NoExists) {};
    void parse(const char *val);
    void dump();
};

struct TSR {
    TCFG c;
    TSRZones z;
    TSRItems r;
    TSRItems s;
    TSRJump j;
    void parse(const char *val);
    void dump();
};

enum TWMDesignator {
    wmdStandard,
    wmdActual,
    wmdUnknown
};

enum TWMType {
    wmtPax,
    wmtHand,
    wmtBag,
    wmtUnknown
};

enum TWMSubType {
    wmsGender,
    wmsClass,
    wmsClsGender,
    wmsUnknown
};

struct TSubTypeHolder {
    TWMSubType sub_type;    // may be wmsUnknown e.g. WM.A.P.KG or WM.S.B.14.KG

    // depending on sub_type value, data stores in different structs

//    TGenderCount g;        // wmsGender                        WM.S.P.G.76/76/35/0.KG
//    TClsWeight c;           // wmsClass for all except mwtBag   WM.S.P.C.85/76/70.KG or WM.S.H.C.15/10/10.KG
//    TClsGenderWeight cg;    // mwsClsGender                     WM.S.P.CG.è100/50/25/20.Å80/75/50/25.ù75/70/45/20.KG
//    TClsBagWeight cb;       // wmsClass for wmtBag              WM.S.B.C.F45.C40.M35.LB
//
//    int weight;             // wmsUnknown                       WM.S.B.14.KG - sub_type = wmsUnknown, weight = 14
//                            // may be NoExists       WM.A.B.KG or WM.S.B.14.KG
//                            // WM.S.H.C.15/10/10.KG - weights holded in TClsWeight
    virtual void parse(const std::vector<std::string> &val) = 0;
    virtual void dump();

    TMeasur measur;

    TSubTypeHolder():
        sub_type(wmsUnknown),
//        weight(ASTRA::NoExists),
        measur(mUnknown)
    {}
};

struct TGenderCount:public TSubTypeHolder {
    int m, f, c, i;
    void parse(const std::vector<std::string> &val);
    void dump();
    TGenderCount():
        m(ASTRA::NoExists),
        f(ASTRA::NoExists),
        c(ASTRA::NoExists),
        i(ASTRA::NoExists)
    {}
};

struct TClsWeight:public TSubTypeHolder {
    int f, c, y;
    void parse(const std::vector<std::string> &val);
    void dump();
    TClsWeight():
        f(ASTRA::NoExists),
        c(ASTRA::NoExists),
        y(ASTRA::NoExists)
    {}
};

struct TClsGenderWeight:public std::map<std::string, TGenderCount>, public TSubTypeHolder {
    void parse(const std::vector<std::string> &val);
    void dump();
};

struct TClsBagWeight:public std::map<std::string, int>, public TSubTypeHolder {
    void parse(const std::vector<std::string> &val);
    void dump();
};

struct TSimpleWeight:public TSubTypeHolder {
    int weight;
    void parse(const std::vector<std::string> &val);
    void dump();
    TSimpleWeight(): weight(ASTRA::NoExists) {};
};

typedef std::map<TWMType, std::tr1::shared_ptr<TSubTypeHolder> > TWMTypeMap;
typedef std::map<TWMDesignator, TWMTypeMap> TWMMap;

struct TWM:public TWMMap {
    private:
        bool find_item(TWMDesignator desig, TWMType type);
    public:
        void parse(const char *val);
        void dump();
};

enum TPDType {
    pdtClass,
    pdtZone,
    pdtRow,
    pdtJump,
    pdtUnknown
};

struct TPDItem {
    TWeight actual; // actual weight
    TGenderCount standard; // standard weights
    void dump();
};

struct TPDParser:public std::map<std::string, TPDItem> {
    TPDType type;
    TPDParser(): type(pdtUnknown) {};
    void parse(TPDType atype, const std::vector<std::string> &val);
    void dump();
    std::string getKey(const std::string &val, TPDType type);
};

struct TPD:public std::map<TPDType, TPDParser> {
    void parse(const char *val);
    void dump();
};

enum TGender {
    gM,
    gF,
    gC,
    gI,
    gUnknown
};

struct TSPItem {
    int actual;
    TGender gender;
    TSPItem(): actual(ASTRA::NoExists) {};
};

struct TSP:public std::map<std::string, TSPItem> {
    void parse(const char *val);
    void dump();
};

enum TDestInfoKey {
    dkPT,
    dkBT,
    dkH,
    dkUnknown
};

enum TReqType {
    rtSP,
    rtBT,
    rtSR,
    rtWM,
    rtUnknown
};

enum TDestInfoType {
    dtA,
    dtC,
    dtG,
    dtJ,
    dtUnknown
};

struct TClsTotal {
    int f, c, y;
    TClsTotal():
        f(ASTRA::NoExists),
        c(ASTRA::NoExists),
        y(ASTRA::NoExists)
    {}
    void parse(const std::string &val);
    void dump(const std::string &caption);
};

struct TGenderTotal {
    int m, f, c, i;
    TGenderTotal():
        m(ASTRA::NoExists),
        f(ASTRA::NoExists),
        c(ASTRA::NoExists),
        i(ASTRA::NoExists)
    {}
    void parse(const std::string &val);
    void dump();
};

struct TDestInfo
{
    int total, weight, j;
    TMeasur measur;
    TClsTotal cls_total, actual_total;
    TGenderTotal gender_total;
    void dump();
    TDestInfo():
        total(ASTRA::NoExists),
        weight(ASTRA::NoExists),
        j(ASTRA::NoExists),
        measur(mUnknown)
    {};
};

typedef std::map<TDestInfoKey, TDestInfo> TDestInfoMap;

struct TDest:public std::map<std::string, TDestInfoMap> {
    bool find_item(const std::string &airp, TDestInfoKey key);
    void parse(const char *val);
    void dump();
};

struct TLCIReqInfo {
    TReqType req_type;
    TSR sr;             // filled if SR type
    TWMType wm_type;    // filled if WM type;
};

struct TRequest:public std::map<TReqType, TLCIReqInfo> {
    void parse(const char *val);
    void dump();
};

class TLCIContent
{
    public:
        TActionCode action_code;
        TRequest req;
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
