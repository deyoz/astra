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
    bool empty() { return orig == oUnknown and action == aUnknown; }
    void clear();
    void toXML(xmlNodePtr node);
    void fromXML(xmlNodePtr node);
};

struct TCFG:public std::map<std::string, int> {
    void parse(const std::string &val, const TElemType el);
    void dump();
    void toXML(xmlNodePtr node) const;
};

struct TEQT {
    std::string bort, craft;
    TCFG cfg;
    void parse(const char *val);
    void dump();
    void clear();
    bool empty() { return bort.empty() and craft.empty() and cfg.empty(); }
    void toXML(xmlNodePtr node);
};

enum TMeasur {
    mKG,
    mLB,
    // �� WBW ����� �਩� WM.WB.TT.N - N ����砥� �⪫. ����஫� �।��쭮� ������樨
    // � ���� �� �⮬ � ����. ����. ����. �ய��뢠���� 0
    // mN �������� ���� �⮡� ����� �� �����.
    mN,
    mUnknown
};

struct TWeight {
    int amount;
    TMeasur measur;
    TWeight() { clear(); }
    void toXML(xmlNodePtr node, const std::string &tag) const;
    bool empty() const { return amount == ASTRA::NoExists and measur == mUnknown; }
    void clear()
    {
        amount = ASTRA::NoExists;
        measur = mUnknown;
    }
};

struct TWA {
    TWeight payload, underload;
    void parse(const char *val);
    void dump();
    void clear();
    bool empty() const { return payload.empty() and underload.empty(); }
    void toXML(xmlNodePtr node) const;
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
    TSM() { clear(); }
    void toXML(xmlNodePtr node) const;
    void clear()
    {
        value = smUnknown;
    }
};

struct TSRZones:public std::map<std::string, int> {
    void toXML(xmlNodePtr node) const;
    void parse(const std::string &val);
    void dump();
};

struct TSRItems:public std::vector<std::string> {
    void toXML(xmlNodePtr node, const std::string &tag) const;
    void parse(const std::string &val);
    void dump();
};

struct TSRJump {
    int amount;
    std::vector<std::string> seats;
    TSRZones zones;
    TSRJump() { clear(); }
    void parse(const char *val);
    void dump();
    void clear();
    void toXML(xmlNodePtr node) const;
    bool empty() const;
};

struct TSR {

    enum Type {
        srStd,      // ���� SR ᮮ�. �⠭����� AHM
        srWB,       // SR �� ���졥 WB-��࠭⨨
        srUnknown
    };

    Type type;
    char format;
    TCFG c;
    TSRZones z;
    TSRItems r;
    TSRItems s;
    TSRJump j;
    TSR() { clear(); }
    void parse(const char *val);
    void dump();
    void clear();
    bool empty() const;
    void toXML(xmlNodePtr node) const;
};

enum TWMDesignator {
    wmdStandard,
    wmdActual,
    wmdWB,
    wmdUnknown
};

enum TWMType {
    wmtPax,
    wmtHand,
    wmtBag,

    // non standard WBW types
    wmtWBTotalWeight,
    wmtWBTotalPaxWeight,
    wmtWBTotalHandWeight,
    wmtWBTotalBagWeight,
    wmtWBTotalCargoWeight,
    wmtWBTotalMailWeight,

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
//    TClsGenderWeight cg;    // mwsClsGender                     WM.S.P.CG.�100/50/25/20.�80/75/50/25.�75/70/45/20.KG
//    TClsBagWeight cb;       // wmsClass for wmtBag              WM.S.B.C.F45.C40.M35.LB
//
//    int weight;             // wmsUnknown                       WM.S.B.14.KG - sub_type = wmsUnknown, weight = 14
//                            // may be NoExists       WM.A.B.KG or WM.S.B.14.KG
//                            // WM.S.H.C.15/10/10.KG - weights holded in TClsWeight
    virtual void parse(const std::vector<std::string> &val) = 0;
    virtual void dump();
    virtual xmlNodePtr toXML(xmlNodePtr node) const;

    TMeasur measur;

    TSubTypeHolder():
        sub_type(wmsUnknown),
//        weight(ASTRA::NoExists),
        measur(mUnknown)
    {}

    virtual ~TSubTypeHolder() {};
};

struct TGenderCount:public TSubTypeHolder {
    int m, f, c, i;
    void parse(const std::vector<std::string> &val);
    void dump();
    xmlNodePtr toXML(xmlNodePtr node) const;
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
    xmlNodePtr toXML(xmlNodePtr node) const;
    TClsWeight():
        f(ASTRA::NoExists),
        c(ASTRA::NoExists),
        y(ASTRA::NoExists)
    {}
};

struct TClsGenderWeight:public std::map<std::string, TGenderCount>, public TSubTypeHolder {
    xmlNodePtr toXML(xmlNodePtr node) const;
    void parse(const std::vector<std::string> &val);
    void dump();
};

struct TClsBagWeight:public std::map<std::string, int>, public TSubTypeHolder {
    xmlNodePtr toXML(xmlNodePtr node) const;
    void parse(const std::vector<std::string> &val);
    void dump();
};

struct TSimpleWeight:public TSubTypeHolder {
    int weight;
    void parse(const std::vector<std::string> &val);
    void dump();
    xmlNodePtr toXML(xmlNodePtr node) const;
    TSimpleWeight(): weight(ASTRA::NoExists) {};
};

typedef std::map<TWMType, std::tr1::shared_ptr<TSubTypeHolder> > TWMTypeMap;
typedef std::map<TWMDesignator, TWMTypeMap> TWMMap;

struct TWM:public TWMMap {
    private:
        bool find_item(TWMDesignator desig, TWMType type);
    public:
        void parse(const char *val);
        bool parse_wb(const char *val);
        void dump();
        void toXML(xmlNodePtr node) const;
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

// vector<TSPItem> - ᮤ�ন� ���� �� (F ��� M) �, �᫨ ����, �� (I)
// �.�. ���-�� ������⮢ ����� ���� ���ᨬ� 2. ������ 1.
// ��祬, � ��砥 ��।�� ���� ���ᠦ��, ���� �� ��� ��易⥫쭮 ��, ��㣮� ��:
// 5�/M.5�/M.5�/M.5�/I - � �⮬ �-� � TSP �� ����� 5A �㤥� ����� �� 2-� ��-⮢: 5A/M, 5A/I
//
// �� ��।�� 䠪�. ��ᮢ ���ᠦ�஢, ��� ������ ���� �� ����� 2-� ��ᮢ
// (�஢�ઠ ����, �祢����, �� ��������)
// 5�/75.5�/75.5�/75.5�/20 - � �⮬ �-� � TSP �� ����� 5A �㤥� ����� �� 2-� ��-⮢: 5A/75, 5A/20

struct TSP:public std::map<std::string, std::vector<TSPItem> > {
    private:
        int pr_weight; // true - �ᯮ������� 䠪�. ��� ���ᠦ�஢; false - ���;
    public:
        void parse(const char *val);
        void dump();
        TSP() { clear(); }
        void clear()
        {
            pr_weight = ASTRA::NoExists;
            std::map<std::string, std::vector<TSPItem> >::clear();
        }
};

enum TDestInfoKey {
    dkPT,
    dkBT,
    dkH,
    dkSP,
    dkUnknown
};

enum TReqType {
    rtSP,
    rtBT,
    rtSR,
    rtWM,
    rtWB,
    rtUnknown
};

enum TDestInfoType {
    dtA,
    dtC,
    dtG,
    dtJ,
    dtWB,
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
    void toXML(xmlNodePtr node, const std::string &tag) const;
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
    void toXML(xmlNodePtr node) const;
};

struct TDestInfo
{
    int total, weight, j;
    TMeasur measur;
    TClsTotal cls_total, actual_total;
    TGenderTotal gender_total;
    void dump();
    void toXML(xmlNodePtr node) const;
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
    void toXML(xmlNodePtr node) const;
};

enum TSPType {
    spStd,      // ���� SP ᮮ�. �⠭����� AHM
    spWB,       // SP �� ���졥 WB-��࠭⨨
    spUnknown
};

struct TLCIReqInfo {
    TReqType req_type;
    std::string lang;
    int max_commerce;
    TSR sr;             // filled if SR type
    TWMType wm_type;    // filled if WM type;
    TSPType sp_type;
    TLCIReqInfo() { clear(); }
    void clear();
    bool empty() const;
    void toXML(xmlNodePtr node) const;
};

struct TRequest:public std::map<TReqType, TLCIReqInfo> {
    void parse(const char *val);
    void dump();
    void toXML(xmlNodePtr node);
};

typedef const char* TLinePtr;

struct TSI {
    std::vector<std::string> items;
    void parse(TTlgPartInfo &body, TTlgParser &tlg, TLinePtr &line_p);
    void dump();
    void clear()
    {
        items.clear();
    }
};

class TLCIContent
{
    public:
        int point_id_tlg; // �ᯮ������ ��� ��।�� � http ��ࠡ��稪

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
        TSI si; // Non standard feature! Used in WBW exchange only
        void clear();
        void dump();

        std::string toXML();
        void fromXML(const std::string &content);

        int toDB();
        void fromDB(int id);
};

void ParseLCIContent(TTlgPartInfo body, TLCIHeadingInfo& info, TLCIContent& con, TMemoryManager &mem);
void SaveLCIContent(int tlg_id, TDateTime time_receive, TLCIHeadingInfo& info, TLCIContent& con);

int lci(int argc,char **argv);
int lci_data(int argc, char **argv);

}


#endif
