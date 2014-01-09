#ifndef _SSM_PARSER_H
#define _SSM_PARSER_H

#include "tlg_parser.h"

#ifdef ZZZ

namespace TypeB
{
enum TTimeMode{tmLT, tmUTC, tmUnknown};

struct TMsgSeqRef {
    int day, grp, num;
    char month[4], type;
    std::string creator;
    TMsgSeqRef() {
        day = ASTRA::NoExists;
        grp = ASTRA::NoExists;
        num = ASTRA::NoExists;
        *month = 0;
        type = 0;
    }
};

class TSSMHeadingInfo : public THeadingInfo
{
    public:
        TTimeMode time_mode;
        TMsgSeqRef msr;

        void dump();

        TSSMHeadingInfo() : THeadingInfo(), time_mode(tmUnknown) {};
        TSSMHeadingInfo(THeadingInfo &info) : THeadingInfo(info), time_mode(tmUnknown)  {};
};

TTlgPartInfo ParseSSMHeading(TTlgPartInfo heading, TSSMHeadingInfo &info);


enum TActionIdentifier {
    aiNEW,
    aiCNL,
    aiRIN,
    aiRPL,
    aiSKD,
    aiACK,
    aiADM,
    aiCON,
    aiEQT,
    aiFLT,
    aiNAC,
    aiREV,
    aiRSD,
    aiRRT,
    aiTIM,
    aiDEL, // !!! not found in standard
    aiUnknown
};

struct TActionInfo {
    TActionIdentifier id;
    bool xasm;
    void parse(char *val);
    TActionInfo(): id(aiUnknown), xasm(false) {};
};

struct TSSMFltInfo: TFltInfo {
    void parse(const char *val);
};

struct TSSMDate {
    BASIC::TDateTime date;
    void parse(const char *val);
    TSSMDate(): date(ASTRA::NoExists) {};
};

enum TFrequencyRate {frW, frW2}; // Every week, Every two weeks (fortnightly)

struct TPeriodOfOper {
    private:
        void parse(bool pr_from, const char *val);
    public:
        TSSMDate from, to;
        char oper_days[8]; // Day(s) of operation
        TFrequencyRate rate;
        void parse(const char *&ph, TTlgParser &tlg);
        TPeriodOfOper():
            rate(frW)
    {
        *oper_days = 0;
    }
};

struct TDEI {
    int id;
    TDEI(int aid) { id = aid; };
    virtual ~TDEI() {};
    virtual void parse(const char *val) = 0;
    virtual void dump() = 0;
    virtual bool empty() = 0;
};

struct TDEI_1:TDEI {
    std::vector<std::string> airlines;
    void parse(const char *val);
    void dump();
    bool empty() { return airlines.empty(); };
    TDEI_1(): TDEI(1) {};
};

struct TDEI_airline:TDEI {
    char airline[4];
    void parse(const char *val);
    void dump();
    bool empty() { return strlen(airline) == 0; };
    TDEI_airline(int aid): TDEI(aid) { *airline = 0; };
};

struct TDEI_6:TDEI {
    TTlgCategory tlg_type;
    std::string airline;
    char suffix;
    int flt_no;
    int layover; // used in SSM
    BASIC::TDateTime date; // used in ASM
    void parse(const char *val);
    void dump();
    bool empty() { return airline.empty(); };
    TDEI_6(TTlgCategory atlg_type):
        TDEI(6),
        tlg_type(atlg_type),
        suffix(0),
        flt_no(ASTRA::NoExists),
        layover(ASTRA::NoExists),
        date(ASTRA::NoExists)
    {};
};

struct TMealItem {
    std::string cls;
    std::string meal;
    TElemFmt fmt;
};

struct TDEI_7:TDEI {
    private:
        void insert(bool default_meal, std::string &meal);
    public:
        std::vector<TMealItem> meal_service;
        void parse(const char *val);
        void dump();
        bool empty() { return meal_service.empty(); };
        TDEI_7(): TDEI(7) {};
};

struct TDEIHolder {
    TTlgCategory tlg_type;
    TDEI_1 dei1;                // Joint Operation Airline Designators
    TDEI_airline dei2;          // Code Sharing - Commercial duplicate
    TDEI_airline dei3;          // Aircraft Owner
    TDEI_airline dei4;          // Cockpit Crew Employer
    TDEI_airline dei5;          // Cabin Crew Employer
    TDEI_6 dei6;                // Onward Flight
    TDEI_7 dei7;                // Meal Service Note
    TDEI_airline dei9;          // Code Sharing - Shared Airline Designation or
    void parse(TTlgElement e, const char *&ph, TTlgParser &tlg);
    void dump();
    bool empty()
    {
        return
            dei1.empty() and
            dei2.empty() and
            dei3.empty() and
            dei4.empty() and
            dei5.empty() and
            dei6.empty() and
            dei7.empty() and
            dei9.empty();
    }
    TDEIHolder(TTlgCategory atlg_type):
        tlg_type(atlg_type),
        dei2(2),
        dei3(3),
        dei4(4),
        dei5(5),
        dei6(atlg_type),
        dei9(9)
    {}
};

struct TFlightInformation {
    TSSMFltInfo flt;
    TPeriodOfOper oper_period;  // Existing Period of Operation;
    TDEIHolder dei_holder;
    std::string raw_data;
    TFlightInformation(): dei_holder(tcSSM) {};
};

struct TPeriodFrequency {
    TSSMDate effective_date, discontinue_date; //Schedule Validity Effective/Discontinue Dates
    TPeriodOfOper oper_period;
    TDEIHolder dei_holder;
    std::string raw_data;
    TPeriodFrequency(): dei_holder(tcSSM) {};
};

struct TEquipment {
    char service_type;
    char aircraft[4];
    std::string PRBD;   //Passenger Reservations Booking Designator
                        //or Aircraft Configuration/Version
    std::string PRBM;   //Passenger Reservations Booking Modifier
    std::string craft_cfg;
    TDEIHolder dei_holder;
    std::string raw_data;
    void dump();
    void parse(TTlgElement e, const char *val);
    TEquipment(TTlgCategory tlg_type): service_type(0), dei_holder(tlg_type) { *aircraft = 0; }
    bool empty() { return service_type == 0; }
};

struct TRouteStation {
    char airp[4];
    BASIC::TDateTime scd, pax_scd;
    int date_variation;
    void dump();
    void parse(const char *val);
    bool empty() { return *airp == 0; };
    TRouteStation() {
        *airp = 0;
        scd = ASTRA::NoExists;
        pax_scd = ASTRA::NoExists;
        date_variation = 0;
    }
};

struct TLegAirp {
    TElemFmt fmt;
    std::string airp;
};

struct TRouting {
    std::vector<TLegAirp> leg_airps;
    TRouteStation station_dep;
    TRouteStation station_arv;
    TDEIHolder dei_holder;
    void parse_leg_airps(std::string buf);
    void parse(TTlgElement e, TActionIdentifier aid, const char *val);
    TRouting(TTlgCategory tlg_type): dei_holder(tlg_type) {};
};

struct TDEI_8:TDEI {
    char TRC;
    int DEI;
    std::string data;
    void parse(const char *val);
    void dump();
    bool empty() { return TRC == 0; };
    TDEI_8(): TDEI(8), TRC(0), DEI(ASTRA::NoExists) {};
};

struct TOther {
    int DEI;
    std::string data;
    void parse(const char *val);
    TOther():DEI(ASTRA::NoExists) {};
};

struct TSegment {
    std::string airp_dep, airp_arv;
    TElemFmt airp_dep_fmt, airp_arv_fmt;
    TDEI_8 dei8; // Traffic Restriction Note
    TOther other;
    std::string raw_data;
    void parse(const char *val);
};

struct TError {
    int line;
    std::string err;
};

struct TReject {
    std::vector<TError> errors;
    std::vector<std::string> rejected_msg;
    void dump();
};

struct TEqtRouting { // contains both Equipment & Routing info
    TEquipment eqt;
    std::vector<TRouting> routing;
    void dump();
    TEqtRouting(TTlgCategory tlg_type): eqt(tlg_type) {};
};

struct TPeriods {
    std::vector<TPeriodFrequency> period_frequency;
    std::vector<TEqtRouting> eqt_routing; //логика!!! для нескольких периодов только один маршрут. Если их тоже несколько то как сопоставить
    //TEqtRouting = TEquipment(описание) + routing(маршрут)
    //routing = TEquipment(описание) + routing(маршрут) - после описания части маршрута может встречаться опять TEquipment для описания след. части
};

struct TRoute {
    std::vector<TPeriods> periods;
    std::vector<TRouting> routing; //!!! удалить ..или когда перечислены диапазоны и внутри их нет маршрута???   TEqtRouting
    std::vector<TSegment> segments;
    bool route_exists();  //???
};

struct TSchedule {
    std::vector<TFlightInformation> flights;
    std::vector<TRoute> routes; //??? как разбирается на TRoute
};

struct TSchedules:std::vector<TSchedule> {
    void add(const TFlightInformation &c);
    void add(const TPeriodFrequency &c);
    void add(const TSegment &c);
    void add(const TRouting &c);
    void add(const TEquipment &c);
    TSchedule &curr_scd();
    TRoute &curr_route();
    TPeriods &curr_periods();
};

struct TSSMSubMessage {
    TActionInfo ainfo;
    TSchedules scd;
    TSSMFltInfo new_flt; // for FLT message only
    std::vector<std::string> si; // up to 3 items
    TReject reject_info;
    void dump();
};

struct TASMActionInfo {
    TActionIdentifier id;
    std::vector<TActionIdentifier> secondary_ids;
    std::vector<std::string> reasons;
    void parse(const char *val);
    void dump();
    TASMActionInfo(): id(aiUnknown) {};
};

struct TASMFlightInfo {
    TFlightIdentifier flt;
    std::vector<TLegAirp> legs;
    TFlightIdentifier new_flt;
    TDEIHolder dei_holder;
    void dump();
    void parse(TTlgElement e, TActionIdentifier aid, const char *val);
    TASMFlightInfo(): dei_holder(tcASM) {};
};

struct TASMSubMessage {
    TASMActionInfo ainfo;
    std::vector<TASMFlightInfo> flts;
    std::vector<TEqtRouting> eqt_routing;
    std::vector<TSegment> segs;
    std::vector<std::string> si; // up to 3 items
    TReject reject_info;
    void dump();
};

class TASMContent
{
    public:
        std::vector<TASMSubMessage> msgs;
        std::vector<std::string> si; // up to 3 items
        void Clear() {};
        void dump();
};

class TSSMContent
{
    public:
        std::vector<TSSMSubMessage> msgs;
        std::vector<std::string> si; // up to 3 items
        void Clear() {};
        void dump();
};

void ParseSSMContent(TTlgPartInfo body, TSSMHeadingInfo& info, TSSMContent& con, TMemoryManager &mem);
void SaveSSMContent(int tlg_id, TSSMHeadingInfo& info, TSSMContent& con);

void ParseASMContent(TTlgPartInfo body, TSSMHeadingInfo& info, TASMContent& con, TMemoryManager &mem);
void SaveASMContent(int tlg_id, TSSMHeadingInfo& info, TASMContent& con);

// на входе строка формата nn(aaa(nn))
BASIC::TDateTime ParseDate(const std::string &buf);

int ssm(int argc,char **argv);

TTlgPartInfo ParseSSMHeading(TTlgPartInfo heading, TSSMHeadingInfo &info);

}

#endif

#endif
