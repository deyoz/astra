#ifndef DBOSTRUCTURES_H
#define DBOSTRUCTURES_H

#include "astra_dates.h"
#include "astra_consts.h"
#include "dbo.h"
#include "stat/stat_agent.h"


using namespace ASTRA;

namespace dbo
{
class AGENT_STAT;
class ARX_AGENT_STAT;

std::vector<dbo::AGENT_STAT> readOraAgentsStat(PointId_t point_id);
std::vector<dbo::ARX_AGENT_STAT> readOraArxAgentsStat();

void initStructures();


struct Points
{
    int point_id= ASTRA::NoExists;
    int move_id= ASTRA::NoExists;
    int point_num= ASTRA::NoExists;
    std::string airp ;
    short pr_tranzit = false;
    int first_point = ASTRA::NoExists;
    std::string airline;
    int flt_no = ASTRA::NoExists;
    std::string suffix;
    std::string craft;
    std::string bort;
    Dates::DateTime_t scd_in;    //TDateTime
    Dates::DateTime_t est_in;
    Dates::DateTime_t act_in;
    Dates::DateTime_t scd_out;
    Dates::DateTime_t est_out;
    Dates::DateTime_t act_out;
    Dates::DateTime_t time_in;
    Dates::DateTime_t time_out;
    std::string trip_type;
    std::string litera;
    std::string park_in;
    std::string park_out;
    std::string remark;
    short pr_reg = true;
    short  pr_del = -1;
    short airp_fmt;  //= efmtUnknown;    //TElemFmt number(1)
    short airline_fmt = -1; //= efmtUnknown; //TElemFmt
    short craft_fmt = -1; //= efmtUnknown;   //TElemFmt
    short suffix_fmt = -1; //= efmtUnknown;  //TElemFmt
    int  tid = ASTRA::NoExists;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a, point_id, "POINT_ID", dbo::NotNull);
        dbo::field(a, move_id, "MOVE_ID", dbo::NotNull);
        dbo::field(a, point_num, "POINT_NUM", dbo::NotNull);
        dbo::field(a, airp, "AIRP", dbo::NotNull);
        dbo::field(a, pr_tranzit, "PR_TRANZIT", dbo::NotNull);
        dbo::field(a, first_point, "FIRST_POINT");
        dbo::field(a, airline, "AIRLINE");
        dbo::field(a, flt_no, "FLT_NO");
        dbo::field(a, suffix, "SUFFIX");
        dbo::field(a, craft, "CRAFT");
        dbo::field(a, bort, "BORT");
        dbo::field(a, scd_in, "SCD_IN");
        dbo::field(a, est_in, "EST_IN");
        dbo::field(a, act_in, "ACT_IN");
        dbo::field(a, scd_out, "SCD_OUT");
        dbo::field(a, est_out, "EST_OUT");
        dbo::field(a, act_out, "ACT_OUT");
        dbo::field(a, time_in, "TIME_IN");
        dbo::field(a, time_out, "TIME_OUT");
        dbo::field(a, trip_type, "TRIP_TYPE");
        dbo::field(a, litera, "LITERA");
        dbo::field(a, park_in, "PARK_IN");
        dbo::field(a, park_out, "PARK_OUT");
        dbo::field(a, remark, "REMARK");
        dbo::field(a, pr_reg, "PR_REG", dbo::NotNull);
        dbo::field(a, pr_del, "PR_DEL", dbo::NotNull);
        dbo::field(a, airp_fmt, "AIRP_FMT", dbo::NotNull);
        dbo::field(a, airline_fmt, "AIRLINE_FMT");
        dbo::field(a, craft_fmt, "CRAFT_FMT");
        dbo::field(a, suffix_fmt, "SUFFIX_FMT");
        dbo::field(a, tid, "TID", dbo::NotNull);
    }
};

struct Arx_Points : public Points
{
    Arx_Points() = default;
    Arx_Points(const Points & p, const Dates::DateTime_t& part_key_) : Points(p), part_key(part_key_)
    {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a, point_id, "POINT_ID", dbo::NotNull);
        dbo::field(a, move_id, "MOVE_ID", dbo::NotNull);
        dbo::field(a, point_num, "POINT_NUM", dbo::NotNull);
        dbo::field(a, airp, "AIRP", dbo::NotNull);
        dbo::field(a, pr_tranzit, "PR_TRANZIT", dbo::NotNull);
        dbo::field(a, first_point, "FIRST_POINT");
        dbo::field(a, airline, "AIRLINE");
        dbo::field(a, flt_no, "FLT_NO");
        dbo::field(a, suffix, "SUFFIX");
        dbo::field(a, craft, "CRAFT");
        dbo::field(a, bort, "BORT");
        dbo::field(a, scd_in, "SCD_IN");
        dbo::field(a, est_in, "EST_IN");
        dbo::field(a, act_in, "ACT_IN");
        dbo::field(a, scd_out, "SCD_OUT");
        dbo::field(a, est_out, "EST_OUT");
        dbo::field(a, act_out, "ACT_OUT");
        dbo::field(a, trip_type, "TRIP_TYPE");
        dbo::field(a, litera, "LITERA");
        dbo::field(a, park_in, "PARK_IN");
        dbo::field(a, park_out, "PARK_OUT");
        dbo::field(a, remark, "REMARK");
        dbo::field(a, pr_reg, "PR_REG", dbo::NotNull);
        dbo::field(a, pr_del, "PR_DEL", dbo::NotNull);
        dbo::field(a, airp_fmt, "AIRP_FMT", dbo::NotNull);
        dbo::field(a, airline_fmt, "AIRLINE_FMT");
        dbo::field(a, craft_fmt, "CRAFT_FMT");
        dbo::field(a, suffix_fmt, "SUFFIX_FMT");
        dbo::field(a, tid, "TID", dbo::NotNull);
        dbo::field(a, part_key, "PART_KEY", dbo::NotNull);
    }
};

struct Move_Arx_Ext
{
    int date_range = ASTRA::NoExists;
    int move_id = ASTRA::NoExists;
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a, date_range, "DATE_RANGE", dbo::NotNull);
        dbo::field(a, move_id, "MOVE_ID", dbo::NotNull);
        dbo::field(a, part_key, "PART_KEY", dbo::NotNull);
    }
};

struct Move_Ref
{
    int move_id = ASTRA::NoExists;
    std::string reference;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a, move_id, "MOVE_ID", dbo::NotNull);
        dbo::field(a, reference, "REFERENCE");
    }
};

struct Arx_Move_Ref : public Move_Ref
{
    Arx_Move_Ref() = default;
    Arx_Move_Ref(const Move_Ref & mr, const Dates::DateTime_t& part_key_) : Move_Ref(mr), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a, move_id, "MOVE_ID", dbo::NotNull);
        dbo::field(a, part_key, "PART_KEY", dbo::NotNull);
        dbo::field(a, reference, "REFERENCE");
    }
};

struct Lang_Types
{
    std::string code;
    std::string name;
    std::string name_lat;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a, code, "CODE", dbo::NotNull);
        dbo::field(a, name, "NAME", dbo::NotNull);
        dbo::field(a, name_lat, "NAME_LAT");
    }
};

struct Events_Bilingual
{
    int ev_order;// number(9)// not null,
    std::string ev_user;// varchar2(20),
    int id1 = ASTRA::NoExists;// number(9),
    int id2 = ASTRA::NoExists;// number(9),
    int id3 = ASTRA::NoExists;// number(9),
    std::string lang;// varchar2(2) not null,
    std::string msg; // varchar2(250) not null,
    int part_num;// number(2) not null,
    std::string screen;// varchar2(15),
    std::string station;// varchar2(15),
    Dates::DateTime_t time ;//date not null,
    std::string type;// varchar2(3) not null,
    std::string sub_type;// varchar2(100)

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a, ev_order, "EV_ORDER", dbo::NotNull);
        dbo::field(a, ev_user, "EV_USER");
        dbo::field(a, id1, "ID1");
        dbo::field(a, id2, "ID2");
        dbo::field(a, id3, "ID3");
        dbo::field(a, lang, "lang", dbo::NotNull);
        dbo::field(a, msg, "msg", dbo::NotNull);
        dbo::field(a, part_num, "part_num", dbo::NotNull);
        dbo::field(a, screen, "screen");
        dbo::field(a, station, "station");
        dbo::field(a, time, "time", dbo::NotNull);
        dbo::field(a, type, "type", dbo::NotNull);
        dbo::field(a, sub_type, "sub_type");
    }
};

struct Arx_Events : public Events_Bilingual
{
    Arx_Events() = default;
    Arx_Events(const Events_Bilingual & eb, const Dates::DateTime_t& part_key_)
        : Events_Bilingual(eb), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        Events_Bilingual::persist(a);
        dbo::field(a, part_key, "PART_KEY", dbo::NotNull);
    }
};

struct Pax_Grp
{
    std::string m_airp_arv; //varchar2(3) not null,
    std::string m_airp_dep; //varchar2(3) not null,
    int m_bag_refuse; //number(1) not null,
    std::string m_class; //varchar2(1),
    int m_class_grp = ASTRA::NoExists; //number(9),
    std::string m_client_type; //varchar2(5) not null,
    std::string m_desk; // varchar2(6),
    int m_excess_pc;// number(4) not null,
    int m_excess_wt;// number(4) not null,
    int m_grp_id; // number(9) not null,
    int m_hall = ASTRA::NoExists;// number(9),
    int m_inbound_confirm; // number(1) not null,
    int m_piece_concept = ASTRA::NoExists;// number(1),
    int m_point_arv; // number(9) not null,
    int m_point_dep; // number(9) not null,
    int m_point_id_mark;// number(9) not null,
    int m_pr_mark_norms;// number(1) not null,
    int m_rollback_guaranteed = ASTRA::NoExists;// number(1),
    std::string m_status;// varchar2(1) not null,
    int m_tid;// number(9) not null,
    Dates::DateTime_t m_time_create;// date,
    int m_trfer_confirm;// number(1) not null,
    int m_trfer_conflict;// number(1) not null,
    int m_user_id = ASTRA::NoExists;// number(9)

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a, m_airp_arv, "airp_arv", dbo::NotNull);
        dbo::field(a, m_airp_dep, "airp_dep", dbo::NotNull);
        dbo::field(a, m_bag_refuse, "bag_refuse", dbo::NotNull);
        dbo::field(a, m_class, "class");
        dbo::field(a, m_class_grp, "class_grp");
        dbo::field(a, m_client_type, "client_type", dbo::NotNull);
        dbo::field(a, m_desk, "desk");
        dbo::field(a, m_excess_pc, "excess_pc", dbo::NotNull);
        dbo::field(a, m_excess_wt, "excess_wt", dbo::NotNull);
        dbo::field(a, m_grp_id, "grp_id", dbo::NotNull);
        dbo::field(a, m_hall, "hall");
        dbo::field(a, m_inbound_confirm, "inbound_confirm", dbo::NotNull);
        dbo::field(a, m_piece_concept, "piece_concept");
        dbo::field(a, m_point_arv, "point_arv", dbo::NotNull);
        dbo::field(a, m_point_dep, "point_dep", dbo::NotNull);
        dbo::field(a, m_point_id_mark, "point_id_mark", dbo::NotNull);
        dbo::field(a, m_pr_mark_norms, "pr_mark_norms", dbo::NotNull);
        dbo::field(a, m_rollback_guaranteed, "rollback_guaranteed");
        dbo::field(a, m_status, "status", dbo::NotNull);
        dbo::field(a, m_tid, "tid", dbo::NotNull);
        dbo::field(a, m_time_create, "time_create");
        dbo::field(a, m_trfer_confirm, "trfer_confirm", dbo::NotNull);
        dbo::field(a, m_trfer_conflict, "trfer_conflict", dbo::NotNull);
        dbo::field(a, m_user_id, "user_id");
    }
};

struct Arx_Pax_Grp : public Pax_Grp
{
    int m_bag_types_id = ASTRA::NoExists; // number(9),
    int m_excess = ASTRA::NoExists; //number(4),
    Dates::DateTime_t m_part_key ;//date not null,

    Arx_Pax_Grp() = default;
    Arx_Pax_Grp(const Pax_Grp & gr, const Dates::DateTime_t& part_key)
        : Pax_Grp(gr), m_part_key(part_key) {}

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a, m_airp_arv, "airp_arv", dbo::NotNull);
        dbo::field(a, m_airp_dep, "airp_dep", dbo::NotNull);
        dbo::field(a, m_bag_refuse, "bag_refuse", dbo::NotNull);
        dbo::field(a, m_class, "class");
        dbo::field(a, m_class_grp, "class_grp");
        dbo::field(a, m_client_type, "client_type", dbo::NotNull);
        dbo::field(a, m_desk, "desk");
        dbo::field(a, m_excess_pc, "excess_pc");
        dbo::field(a, m_excess_wt, "excess_wt");
        dbo::field(a, m_grp_id, "grp_id", dbo::NotNull);
        dbo::field(a, m_hall, "hall");
        dbo::field(a, m_piece_concept, "piece_concept");
        dbo::field(a, m_point_arv, "point_arv", dbo::NotNull);
        dbo::field(a, m_point_dep, "point_dep", dbo::NotNull);
        dbo::field(a, m_point_id_mark, "point_id_mark", dbo::NotNull);
        dbo::field(a, m_pr_mark_norms, "pr_mark_norms", dbo::NotNull);
        dbo::field(a, m_status, "status", dbo::NotNull);
        dbo::field(a, m_tid, "tid", dbo::NotNull);
        dbo::field(a, m_time_create, "time_create");
        dbo::field(a, m_user_id, "user_id");
        dbo::field(a, m_bag_types_id, "bag_types_id");
        dbo::field(a, m_excess, "excess");
        dbo::field(a, m_part_key, "part_key", dbo::NotNull);
    }
};

struct Mark_Trips
{
    std::string airline ;//varchar2(3) not null,
    std::string airp_dep ;//varchar2(3) not null,
    int flt_no ;//number not null,
    int point_id ;//number(9) not null,
    Dates::DateTime_t scd ;//date not null,
    std::string suffix ;//varchar2(1)

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a, airline, "airline", dbo::NotNull);
        dbo::field(a, airp_dep, "airp_dep", dbo::NotNull);
        dbo::field(a, flt_no, "flt_no", dbo::NotNull);
        dbo::field(a, point_id, "point_id", dbo::NotNull);
        dbo::field(a, scd, "scd", dbo::NotNull);
        dbo::field(a, suffix, "suffix");
    }

};

struct Arx_Mark_Trips : public Mark_Trips
{
    Arx_Mark_Trips() = default;
    Arx_Mark_Trips(const Mark_Trips & m_trips, Dates::DateTime_t part_key)
        : Mark_Trips(m_trips), m_part_key(part_key) {}
    Dates::DateTime_t m_part_key ; //DATE NOT NULL,


    template<typename Action>
    void persist(Action & a) {
        Mark_Trips::persist(a);
        dbo::field(a, m_part_key, "PART_KEY", dbo::NotNull);
    }
};

struct Self_Ckin_Stat
{
    int adult;// number(3) not null,
    int baby;// number(3) not null,
    int child;// number(3) not null,
    std::string client_type;// varchar2(5) not null,
    std::string descr;// varchar2(100) not null,
    std::string desk;// varchar2(6) not null,
    std::string desk_airp;// varchar2(3),
    int point_id;// number(9) not null,
    int tckin;// number(3) not null,
    int term_bag;// number(3),
    int term_bp;// number(3),
    int term_ckin_service;// number(3)

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a, adult, "adult", dbo::NotNull);
        dbo::field(a, baby, "baby", dbo::NotNull);
        dbo::field(a, child, "child", dbo::NotNull);
        dbo::field(a, client_type, "client_type", dbo::NotNull);
        dbo::field(a, descr, "descr", dbo::NotNull);
        dbo::field(a, desk, "desk", dbo::NotNull);
        dbo::field(a, desk_airp, "desk_airp");
        dbo::field(a, point_id, "point_id", dbo::NotNull);
        dbo::field(a, tckin, "tckin", dbo::NotNull);
        dbo::field(a, term_bag, "term_bag");
        dbo::field(a, term_bp, "term_bp");
        dbo::field(a, term_ckin_service, "term_ckin_service");
    }
};

struct Arx_Self_Ckin_Stat : public Self_Ckin_Stat
{
    Arx_Self_Ckin_Stat() = default;
    Arx_Self_Ckin_Stat(const Self_Ckin_Stat & scs, Dates::DateTime_t part_key)
        : Self_Ckin_Stat(scs), m_part_key(part_key) {}
    Dates::DateTime_t m_part_key;

    template<typename Action>
    void persist(Action & a) {
        Self_Ckin_Stat::persist(a);
        dbo::field(a, m_part_key, "PART_KEY", dbo::NotNull);
    }
};

struct RFISC_STAT
{
    std::string airp_arv;
    std::string desk;
    int excess;
    std::string fqt_no;
    int paid;
    int point_id;
    int point_num;
    int pr_trfer;
    std::string rfisc;
    int tag_no;
    Dates::DateTime_t time_create;
    Dates::DateTime_t travel_time;
    std::string trfer_airline;
    std::string trfer_airp_arv;
    int trfer_flt_no;
    Dates::DateTime_t trfer_scd;
    std::string trfer_suffix;
    std::string user_descr;
    std::string user_login;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,desk,"DESK");
        dbo::field(a,excess,"EXCESS");
        dbo::field(a,fqt_no,"FQT_NO");
        dbo::field(a,paid,"PAID");
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,point_num,"POINT_NUM", dbo::NotNull);
        dbo::field(a,pr_trfer,"PR_TRFER", dbo::NotNull);
        dbo::field(a,rfisc,"RFISC", dbo::NotNull);
        dbo::field(a,tag_no,"TAG_NO", dbo::NotNull);
        dbo::field(a,time_create,"TIME_CREATE");
        dbo::field(a,travel_time,"TRAVEL_TIME");
        dbo::field(a,trfer_airline,"TRFER_AIRLINE");
        dbo::field(a,trfer_airp_arv,"TRFER_AIRP_ARV");
        dbo::field(a,trfer_flt_no,"TRFER_FLT_NO");
        dbo::field(a,trfer_scd,"TRFER_SCD");
        dbo::field(a,trfer_suffix,"TRFER_SUFFIX");
        dbo::field(a,user_descr,"USER_DESCR");
        dbo::field(a,user_login,"USER_LOGIN");
    }
};

struct ARX_RFISC_STAT : public RFISC_STAT
{
    ARX_RFISC_STAT() = default;
    ARX_RFISC_STAT(const RFISC_STAT & rs, const Dates::DateTime_t & part_key_)
        :RFISC_STAT(rs), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        RFISC_STAT::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct STAT_SERVICES
{
    std::string airp_arv;
    std::string airp_dep;
    int pax_id;
    int point_id;
    std::string receipt_no;
    std::string rfic;
    std::string rfisc;
    Dates::DateTime_t scd_out;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,airp_dep,"AIRP_DEP", dbo::NotNull);
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,receipt_no,"RECEIPT_NO");
        dbo::field(a,rfic,"RFIC");
        dbo::field(a,rfisc,"RFISC", dbo::NotNull);
        dbo::field(a,scd_out,"SCD_OUT", dbo::NotNull);
    }
};

struct ARX_STAT_SERVICES : public STAT_SERVICES
{
    ARX_STAT_SERVICES() = default;
    ARX_STAT_SERVICES(const STAT_SERVICES & ss, const Dates::DateTime_t & part_key_)
        :STAT_SERVICES(ss), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        STAT_SERVICES::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct STAT_REM
{
    std::string airp_last;
    std::string desk;
    int point_id;
    double rate;
    std::string rate_cur;
    std::string rem_code;
    std::string rfisc;
    std::string ticket_no;
    Dates::DateTime_t travel_time;
    int user_id;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airp_last,"AIRP_LAST", dbo::NotNull);
        dbo::field(a,desk,"DESK", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,rate,"RATE");
        dbo::field(a,rate_cur,"RATE_CUR");
        dbo::field(a,rem_code,"REM_CODE", dbo::NotNull);
        dbo::field(a,rfisc,"RFISC");
        dbo::field(a,ticket_no,"TICKET_NO");
        dbo::field(a,travel_time,"TRAVEL_TIME");
        dbo::field(a,user_id,"USER_ID", dbo::NotNull);
    }
};

struct ARX_STAT_REM : public STAT_REM
{
    ARX_STAT_REM() = default;
    ARX_STAT_REM(const STAT_REM & sr, const Dates::DateTime_t& part_key_) :
        STAT_REM(sr), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        STAT_REM::persist(a);
        dbo::field(a, part_key, "PART_KEY", dbo::NotNull);
    }
};

struct LIMITED_CAPABILITY_STAT
{
    std::string airp_arv;
    int pax_amount;
    int point_id;
    std::string rem_code;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,pax_amount,"PAX_AMOUNT", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,rem_code,"REM_CODE", dbo::NotNull);
    }
};

struct ARX_LIMITED_CAPABILITY_STAT : public LIMITED_CAPABILITY_STAT
{
    ARX_LIMITED_CAPABILITY_STAT() = default;
    ARX_LIMITED_CAPABILITY_STAT(const LIMITED_CAPABILITY_STAT & lcs, const Dates::DateTime_t & part_key_)
        : LIMITED_CAPABILITY_STAT(lcs), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        LIMITED_CAPABILITY_STAT::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct PFS_STAT
{
    std::string airp_arv;
    Dates::DateTime_t birth_date;
    std::string gender;
    std::string name;
    int pax_id;
    std::string pnr;
    int point_id;
    int seats;
    std::string status;
    std::string subcls;
    std::string surname;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,birth_date,"BIRTH_DATE");
        dbo::field(a,gender,"GENDER");
        dbo::field(a,name,"NAME");
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,pnr,"PNR");
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,seats,"SEATS", dbo::NotNull);
        dbo::field(a,status,"STATUS", dbo::NotNull);
        dbo::field(a,subcls,"SUBCLS", dbo::NotNull);
        dbo::field(a,surname,"SURNAME", dbo::NotNull);
    }
};

struct ARX_PFS_STAT : public PFS_STAT
{
    ARX_PFS_STAT() = default;
    ARX_PFS_STAT(const PFS_STAT & ps, const Dates::DateTime_t&  part_key_)
        :PFS_STAT(ps), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        PFS_STAT::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct STAT_AD
{
    int bag_amount;
    int bag_weight;
    std::string m_class;
    std::string client_type;
    std::string desk;
    int pax_id;
    std::string pnr;
    int point_id;
    Dates::DateTime_t scd_out;
    std::string seat_no;
    std::string seat_no_lat;
    std::string station;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,bag_amount,"BAG_AMOUNT");
        dbo::field(a,bag_weight,"BAG_WEIGHT");
        dbo::field(a, m_class,"CLASS", dbo::NotNull);
        dbo::field(a,client_type,"CLIENT_TYPE", dbo::NotNull);
        dbo::field(a,desk,"DESK", dbo::NotNull);
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,pnr,"PNR");
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,scd_out,"SCD_OUT", dbo::NotNull);
        dbo::field(a,seat_no,"SEAT_NO");
        dbo::field(a,seat_no_lat,"SEAT_NO_LAT");
        dbo::field(a,station,"STATION");
    }
};

struct ARX_STAT_AD : public STAT_AD
{
    ARX_STAT_AD() = default;
    ARX_STAT_AD(const STAT_AD & sa, const Dates::DateTime_t & part_key_)
        :STAT_AD(sa), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        STAT_AD::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);

    }
};

struct STAT_HA
{
    int adt;
    int chd;
    int hotel_id;
    int inf;
    int point_id;
    int room_type;
    Dates::DateTime_t scd_out;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,adt,"ADT", dbo::NotNull);
        dbo::field(a,chd,"CHD", dbo::NotNull);
        dbo::field(a,hotel_id,"HOTEL_ID", dbo::NotNull);
        dbo::field(a,inf,"INF", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,room_type,"ROOM_TYPE");
        dbo::field(a,scd_out,"SCD_OUT", dbo::NotNull);
    }
};


struct ARX_STAT_HA : public STAT_HA
{
    ARX_STAT_HA() = default;
    ARX_STAT_HA(const STAT_HA & sh, const Dates::DateTime_t & part_key_)
        :STAT_HA(sh), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        STAT_HA::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct STAT_VO
{
    int amount;
    int point_id;
    Dates::DateTime_t scd_out;
    std::string voucher;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,amount,"AMOUNT", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,scd_out,"SCD_OUT", dbo::NotNull);
        dbo::field(a,voucher,"VOUCHER", dbo::NotNull);
    }
};

struct ARX_STAT_VO : public STAT_VO
{
    ARX_STAT_VO() = default;
    ARX_STAT_VO(const STAT_VO & svo, const  Dates::DateTime_t& part_key_)
        :STAT_VO(svo), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        STAT_VO::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct STAT_REPRINT
{
    int amount;
    std::string ckin_type;
    std::string desk;
    int point_id;
    Dates::DateTime_t scd_out;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,amount,"AMOUNT", dbo::NotNull);
        dbo::field(a,ckin_type,"CKIN_TYPE");
        dbo::field(a,desk,"DESK", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,scd_out,"SCD_OUT", dbo::NotNull);
    }
};

struct ARX_STAT_REPRINT : public STAT_REPRINT
{
    ARX_STAT_REPRINT() = default;
    ARX_STAT_REPRINT(const STAT_REPRINT& sr, const Dates::DateTime_t & part_key_)
        :STAT_REPRINT(sr), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        STAT_REPRINT::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct TRFER_PAX_STAT
{
    int bag_amount;
    int bag_weight;
    int pax_id;
    int point_id;
    int rk_weight;
    Dates::DateTime_t scd_out;
    std::string segments;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,bag_amount,"BAG_AMOUNT");
        dbo::field(a,bag_weight,"BAG_WEIGHT");
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,rk_weight,"RK_WEIGHT");
        dbo::field(a,scd_out,"SCD_OUT", dbo::NotNull);
        dbo::field(a,segments,"SEGMENTS", dbo::NotNull);
    }
};

struct ARX_TRFER_PAX_STAT : public TRFER_PAX_STAT
{
    ARX_TRFER_PAX_STAT() = default;
    ARX_TRFER_PAX_STAT(const TRFER_PAX_STAT& sr, const Dates::DateTime_t & part_key_)
        :TRFER_PAX_STAT(sr), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        TRFER_PAX_STAT::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct BI_STAT
{
    std::string desk;
    int hall;
    std::string op_type;
    int pax_id;
    int point_id;
    std::string print_type;
    int pr_print;
    Dates::DateTime_t scd_out;
    int terminal;
    Dates::DateTime_t time_print;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,desk,"DESK", dbo::NotNull);
        dbo::field(a,hall,"HALL", dbo::NotNull);
        dbo::field(a,op_type,"OP_TYPE", dbo::NotNull);
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,print_type,"PRINT_TYPE", dbo::NotNull);
        dbo::field(a,pr_print,"PR_PRINT", dbo::NotNull);
        dbo::field(a,scd_out,"SCD_OUT", dbo::NotNull);
        dbo::field(a,terminal,"TERMINAL");
        dbo::field(a,time_print,"TIME_PRINT", dbo::NotNull);
    }
};

struct ARX_BI_STAT : public BI_STAT
{
    ARX_BI_STAT() = default;
    ARX_BI_STAT(const BI_STAT& sr, const Dates::DateTime_t & part_key_)
        :BI_STAT(sr), part_key(part_key_) {}

    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        BI_STAT::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

// Нужно добавить поддержку типа agent_stat_t в pg курсор
struct AGENT_STAT
{
    int dbag_amount_inc = 0;
    int dbag_amount_dec = 0;
    int dbag_weight_inc = 0;
    int dbag_weight_dec = 0;

    std::string desk;
    int dpax_amount_inc = 0;
    int dpax_amount_dec = 0;
    int drk_amount_inc = 0;
    int drk_amount_dec = 0;

    int drk_weight_inc = 0;
    int drk_weight_dec = 0;
    int dtckin_amount_inc = 0;
    int dtckin_amount_dec = 0;

    Dates::DateTime_t ondate;
    int pax_amount = 0;
    int pax_time = 0;
    int point_id = 0;
    int user_id = 0;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,dbag_amount_inc,"DBAG_AMOUNT_INC", dbo::NotNull);
        dbo::field(a,dbag_amount_dec,"DBAG_AMOUNT_DEC", dbo::NotNull);
        dbo::field(a,dbag_weight_inc,"DBAG_WEIGHT_INC", dbo::NotNull);
        dbo::field(a,dbag_weight_dec,"DBAG_WEIGHT_DEC", dbo::NotNull);
        dbo::field(a,desk,"DESK", dbo::NotNull);
        dbo::field(a,dpax_amount_inc,"DPAX_AMOUNT_INC", dbo::NotNull);
        dbo::field(a,dpax_amount_dec,"DPAX_AMOUNT_DEC", dbo::NotNull);
        dbo::field(a,drk_amount_inc,"DRK_AMOUNT_INC", dbo::NotNull);
        dbo::field(a,drk_amount_dec,"DRK_AMOUNT_DEC", dbo::NotNull);
        dbo::field(a,drk_weight_inc,"DRK_WEIGHT_INC", dbo::NotNull);
        dbo::field(a,drk_weight_dec,"DRK_WEIGHT_DEC", dbo::NotNull);
        dbo::field(a,dtckin_amount_inc,"DTCKIN_AMOUNT_INC", dbo::NotNull);
        dbo::field(a,dtckin_amount_dec,"DTCKIN_AMOUNT_DEC", dbo::NotNull);
        dbo::field(a,ondate,"ONDATE", dbo::NotNull);
        dbo::field(a,pax_amount,"PAX_AMOUNT", dbo::NotNull);
        dbo::field(a,pax_time,"PAX_TIME", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,user_id,"USER_ID", dbo::NotNull);
    }

    bool operator==(const AGENT_STAT& right) const
    {
        return
            dbag_amount_inc == right.dbag_amount_inc &&
            dbag_amount_dec == right.dbag_amount_dec &&
            dbag_weight_inc == right.dbag_weight_inc &&
            dbag_weight_dec == right.dbag_weight_dec &&

            desk == right.desk &&
            dpax_amount_inc == right.dpax_amount_inc &&
            dpax_amount_dec == right.dpax_amount_dec &&
            drk_amount_inc == right.drk_amount_inc &&
            drk_amount_dec == right.drk_amount_dec &&

            drk_weight_inc == right.drk_weight_inc &&
            drk_weight_dec == right.drk_weight_dec &&
            dtckin_amount_inc == right.dtckin_amount_inc &&
            dtckin_amount_dec == right.dtckin_amount_dec &&

            ondate == right.ondate &&
            pax_amount == right.pax_amount &&
            pax_time == right.pax_time &&
            point_id == right.point_id &&
            user_id == right.user_id;
    }
};


struct ARX_AGENT_STAT : public AGENT_STAT
{
    ARX_AGENT_STAT() = default;
    ARX_AGENT_STAT(const AGENT_STAT& sr, const Dates::DateTime_t & part_key_)
        :AGENT_STAT(sr), part_key(part_key_) {}
    Dates::DateTime_t part_key;
    Dates::DateTime_t point_part_key;

    bool operator==(const ARX_AGENT_STAT& right) const
    {
        return AGENT_STAT::operator ==(right) &&
                part_key == right.part_key && point_part_key == right.point_part_key;
    }
    template<typename Action>
    void persist(Action & a) {
        AGENT_STAT::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
        dbo::field(a,point_part_key,"POINT_PART_KEY", dbo::NotNull);
    }
};

struct STAT
{
    int adult;
    std::string airp_arv;
    int baby;
    int baby_wop;
    int c;
    int child;
    int child_wop;
    std::string client_type;
    int excess_pc;
    int excess_wt;
    int f;
    int hall;
    int pcs;
    int point_id;
    std::string status;
    int term_bag;
    int term_bp;
    int term_ckin_service;
    int unchecked;
    int weight;
    int y;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,adult,"ADULT", dbo::NotNull);
        dbo::field(a,airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,baby,"BABY", dbo::NotNull);
        dbo::field(a,baby_wop,"BABY_WOP", dbo::NotNull);
        dbo::field(a,c,"C", dbo::NotNull);
        dbo::field(a,child,"CHILD", dbo::NotNull);
        dbo::field(a,child_wop,"CHILD_WOP", dbo::NotNull);
        dbo::field(a,client_type,"CLIENT_TYPE", dbo::NotNull);
        dbo::field(a,excess_pc,"EXCESS_PC", dbo::NotNull);
        dbo::field(a,excess_wt,"EXCESS_WT", dbo::NotNull);
        dbo::field(a,f,"F", dbo::NotNull);
        dbo::field(a,hall,"HALL");
        dbo::field(a,pcs,"PCS", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,status,"STATUS", dbo::NotNull);
        dbo::field(a,term_bag,"TERM_BAG");
        dbo::field(a,term_bp,"TERM_BP");
        dbo::field(a,term_ckin_service,"TERM_CKIN_SERVICE");
        dbo::field(a,unchecked,"UNCHECKED", dbo::NotNull);
        dbo::field(a,weight,"WEIGHT", dbo::NotNull);
        dbo::field(a,y,"Y", dbo::NotNull);
    }
};

struct ARX_STAT : public STAT
{
    Dates::DateTime_t part_key;
    ARX_STAT() = default;
    ARX_STAT(const STAT& sr, const Dates::DateTime_t & part_key_)
        :STAT(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        STAT::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct TRFER_STAT
{
    int adult;
    int baby;
    int baby_wop;
    int c;
    int child;
    int child_wop;
    std::string client_type;
    int excess_pc;
    int excess_wt;
    int f;
    int pcs;
    int point_id;
    std::string trfer_route;
    int unchecked;
    int weight;
    int y;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,adult,"ADULT", dbo::NotNull);
        dbo::field(a,baby,"BABY", dbo::NotNull);
        dbo::field(a,baby_wop,"BABY_WOP", dbo::NotNull);
        dbo::field(a,c,"C", dbo::NotNull);
        dbo::field(a,child,"CHILD", dbo::NotNull);
        dbo::field(a,child_wop,"CHILD_WOP", dbo::NotNull);
        dbo::field(a,client_type,"CLIENT_TYPE", dbo::NotNull);
        dbo::field(a,excess_pc,"EXCESS_PC", dbo::NotNull);
        dbo::field(a,excess_wt,"EXCESS_WT", dbo::NotNull);
        dbo::field(a,f,"F", dbo::NotNull);
        dbo::field(a,pcs,"PCS", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,trfer_route,"TRFER_ROUTE", dbo::NotNull);
        dbo::field(a,unchecked,"UNCHECKED", dbo::NotNull);
        dbo::field(a,weight,"WEIGHT", dbo::NotNull);
        dbo::field(a,y,"Y", dbo::NotNull);
    }
};

struct ARX_TRFER_STAT : public TRFER_STAT
{
    ARX_TRFER_STAT() = default;
    ARX_TRFER_STAT(const TRFER_STAT& sr, const Dates::DateTime_t & part_key_)
        :TRFER_STAT(sr), part_key(part_key_) {}
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        TRFER_STAT::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct TLG_OUT
{
    std::string addr;
    std::string airline_mark;
    std::string body;
    int completed;
    std::string ending;
    int has_errors;
    std::string heading;
    int id;
    int manual_creation;
    int num;
    std::string origin;
    int originator_id;
    int point_id = ASTRA::NoExists;
    int pr_lat;
    Dates::DateTime_t time_create;
    Dates::DateTime_t time_send_act;
    Dates::DateTime_t time_send_scd;
    std::string type;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,addr,"ADDR");
        dbo::field(a,airline_mark,"AIRLINE_MARK");
        dbo::field(a,body,"BODY");
        dbo::field(a,completed,"COMPLETED", dbo::NotNull);
        dbo::field(a,ending,"ENDING");
        dbo::field(a,has_errors,"HAS_ERRORS", dbo::NotNull);
        dbo::field(a,heading,"HEADING");
        dbo::field(a,id,"ID", dbo::NotNull);
        dbo::field(a,manual_creation,"MANUAL_CREATION", dbo::NotNull);
        dbo::field(a,num,"NUM", dbo::NotNull);
        dbo::field(a,origin,"ORIGIN");
        dbo::field(a,originator_id,"ORIGINATOR_ID", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID");
        dbo::field(a,pr_lat,"PR_LAT", dbo::NotNull);
        dbo::field(a,time_create,"TIME_CREATE", dbo::NotNull);
        dbo::field(a,time_send_act,"TIME_SEND_ACT");
        dbo::field(a,time_send_scd,"TIME_SEND_SCD");
        dbo::field(a,type,"TYPE", dbo::NotNull);
    }
};

struct ARX_TLG_OUT : public TLG_OUT
{
    std::string extra;
    Dates::DateTime_t part_key;

    ARX_TLG_OUT() = default;
    ARX_TLG_OUT(const TLG_OUT& sr, const Dates::DateTime_t & part_key_)
        :TLG_OUT(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        TLG_OUT::persist(a);
        dbo::field(a,extra,"EXTRA");
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

/////////////////////////////


struct TRIP_CLASSES
{
    int block;
    int cfg;
    std::string m_class;
    int point_id;
    int prot;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,block,"BLOCK", dbo::NotNull);
        dbo::field(a,cfg,"CFG", dbo::NotNull);
        dbo::field(a,m_class,"CLASS", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,prot,"PROT", dbo::NotNull);
    }
};


struct ARX_TRIP_CLASSES : public TRIP_CLASSES
{
    Dates::DateTime_t part_key;
    ARX_TRIP_CLASSES() = default;
    ARX_TRIP_CLASSES(const TRIP_CLASSES& sr, const Dates::DateTime_t & part_key_)
        :TRIP_CLASSES(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        TRIP_CLASSES::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct TRIP_DELAYS
{
    std::string delay_code;
    int delay_num;
    int point_id;
    Dates::DateTime_t time;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,delay_code,"DELAY_CODE", dbo::NotNull);
        dbo::field(a,delay_num,"DELAY_NUM", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,time,"TIME", dbo::NotNull);
    }
};

struct ARX_TRIP_DELAYS : public TRIP_DELAYS
{
    Dates::DateTime_t part_key;
    ARX_TRIP_DELAYS() = default;
    ARX_TRIP_DELAYS(const TRIP_DELAYS& sr, const Dates::DateTime_t & part_key_)
        :TRIP_DELAYS(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        TRIP_DELAYS::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};


struct TRIP_LOAD
{
    std::string airp_arv;
    std::string airp_dep;
    int cargo;
    int mail;
    int point_arv;
    int point_dep;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,airp_dep,"AIRP_DEP", dbo::NotNull);
        dbo::field(a,cargo,"CARGO", dbo::NotNull);
        dbo::field(a,mail,"MAIL", dbo::NotNull);
        dbo::field(a,point_arv,"POINT_ARV", dbo::NotNull);
        dbo::field(a,point_dep,"POINT_DEP", dbo::NotNull);
    }
};

struct ARX_TRIP_LOAD : public TRIP_LOAD
{
    Dates::DateTime_t part_key;
    ARX_TRIP_LOAD() = default;
    ARX_TRIP_LOAD(const TRIP_LOAD& sr, const Dates::DateTime_t & part_key_)
        :TRIP_LOAD(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        TRIP_LOAD::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);

    }
};

struct TRIP_SETS
{
    int apis_control;
    int apis_manual_input;
    int auto_comp_chg;
    int auto_weighing;
    int c= ASTRA::NoExists;
    int comp_id= ASTRA::NoExists;
    int crc_comp;
    int et_final_attempt;
    int f= ASTRA::NoExists;
    int jmp_cfg;
    int lci_pers_weights= ASTRA::NoExists;
    int max_commerce= ASTRA::NoExists;
    int piece_concept;
    int point_id;
    int pr_basel_stat;
    int pr_block_trzt= ASTRA::NoExists;
    int pr_check_load;
    int pr_check_pay;
    int pr_etstatus;
    int pr_exam;
    int pr_exam_check_pay;
    int pr_free_seating;
    int pr_lat_seat= ASTRA::NoExists;
    int pr_overload_reg;
    int pr_reg_without_tkna;
    int pr_reg_with_doc;
    int pr_reg_with_tkn;
    int pr_stat;
    int pr_tranz_reg= ASTRA::NoExists;
    int trzt_bort_changing= ASTRA::NoExists;
    int trzt_brd_with_autoreg= ASTRA::NoExists;
    int use_jmp;
    int y= ASTRA::NoExists;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,apis_control,"APIS_CONTROL", dbo::NotNull);
        dbo::field(a,apis_manual_input,"APIS_MANUAL_INPUT", dbo::NotNull);
        dbo::field(a,auto_comp_chg,"AUTO_COMP_CHG", dbo::NotNull);
        dbo::field(a,auto_weighing,"AUTO_WEIGHING", dbo::NotNull);
        dbo::field(a,c,"C");
        dbo::field(a,comp_id,"COMP_ID");
        dbo::field(a,crc_comp,"CRC_COMP", dbo::NotNull);
        dbo::field(a,et_final_attempt,"ET_FINAL_ATTEMPT", dbo::NotNull);
        dbo::field(a,f,"F");
        dbo::field(a,jmp_cfg,"JMP_CFG", dbo::NotNull);
        dbo::field(a,lci_pers_weights,"LCI_PERS_WEIGHTS");
        dbo::field(a,max_commerce,"MAX_COMMERCE");
        dbo::field(a,piece_concept,"PIECE_CONCEPT", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,pr_basel_stat,"PR_BASEL_STAT", dbo::NotNull);
        dbo::field(a,pr_block_trzt,"PR_BLOCK_TRZT");
        dbo::field(a,pr_check_load,"PR_CHECK_LOAD", dbo::NotNull);
        dbo::field(a,pr_check_pay,"PR_CHECK_PAY", dbo::NotNull);
        dbo::field(a,pr_etstatus,"PR_ETSTATUS", dbo::NotNull);
        dbo::field(a,pr_exam,"PR_EXAM", dbo::NotNull);
        dbo::field(a,pr_exam_check_pay,"PR_EXAM_CHECK_PAY", dbo::NotNull);
        dbo::field(a,pr_free_seating,"PR_FREE_SEATING", dbo::NotNull);
        dbo::field(a,pr_lat_seat,"PR_LAT_SEAT");
        dbo::field(a,pr_overload_reg,"PR_OVERLOAD_REG", dbo::NotNull);
        dbo::field(a,pr_reg_without_tkna,"PR_REG_WITHOUT_TKNA", dbo::NotNull);
        dbo::field(a,pr_reg_with_doc,"PR_REG_WITH_DOC", dbo::NotNull);
        dbo::field(a,pr_reg_with_tkn,"PR_REG_WITH_TKN", dbo::NotNull);
        dbo::field(a,pr_stat,"PR_STAT", dbo::NotNull);
        dbo::field(a,pr_tranz_reg,"PR_TRANZ_REG");
        dbo::field(a,trzt_bort_changing,"TRZT_BORT_CHANGING");
        dbo::field(a,trzt_brd_with_autoreg,"TRZT_BRD_WITH_AUTOREG");
        dbo::field(a,use_jmp,"USE_JMP", dbo::NotNull);
        dbo::field(a,y,"Y");
    }
};

struct ARX_TRIP_SETS
{
    int c = ASTRA::NoExists;
    int comp_id= ASTRA::NoExists;
    int f= ASTRA::NoExists;
    int max_commerce= ASTRA::NoExists;
    Dates::DateTime_t part_key;
    int point_id;
    int pr_airp_seance= ASTRA::NoExists;
    int pr_etstatus;
    int pr_stat;
    int pr_tranz_reg= ASTRA::NoExists;
    int y= ASTRA::NoExists;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,c,"C");
        dbo::field(a,comp_id,"COMP_ID");
        dbo::field(a,f,"F");
        dbo::field(a,max_commerce,"MAX_COMMERCE");
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,pr_airp_seance,"PR_AIRP_SEANCE");
        dbo::field(a,pr_etstatus,"PR_ETSTATUS", dbo::NotNull);
        dbo::field(a,pr_stat,"PR_STAT", dbo::NotNull);
        dbo::field(a,pr_tranz_reg,"PR_TRANZ_REG");
        dbo::field(a,y,"Y");
    }
};


struct CRS_DISPLACE2
{
    std::string airline;
    std::string airp_arv_spp;
    std::string airp_arv_tlg;
    std::string airp_dep;
    std::string class_spp;
    std::string class_tlg;
    int flt_no;
    int point_id_spp;
    int point_id_tlg= ASTRA::NoExists;
    Dates::DateTime_t scd;
    std::string status;
    std::string suffix;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE", dbo::NotNull);
        dbo::field(a,airp_arv_spp,"AIRP_ARV_SPP", dbo::NotNull);
        dbo::field(a,airp_arv_tlg,"AIRP_ARV_TLG", dbo::NotNull);
        dbo::field(a,airp_dep,"AIRP_DEP", dbo::NotNull);
        dbo::field(a,class_spp,"CLASS_SPP", dbo::NotNull);
        dbo::field(a,class_tlg,"CLASS_TLG", dbo::NotNull);
        dbo::field(a,flt_no,"FLT_NO", dbo::NotNull);
        dbo::field(a,point_id_spp,"POINT_ID_SPP", dbo::NotNull);
        dbo::field(a,point_id_tlg,"POINT_ID_TLG");
        dbo::field(a,scd,"SCD", dbo::NotNull);
        dbo::field(a,status,"STATUS", dbo::NotNull);
        dbo::field(a,suffix,"SUFFIX");
    }
};

struct ARX_CRS_DISPLACE2 : public CRS_DISPLACE2
{
    Dates::DateTime_t part_key;
    ARX_CRS_DISPLACE2() = default;
    ARX_CRS_DISPLACE2(const CRS_DISPLACE2& sr, const Dates::DateTime_t & part_key_)
        :CRS_DISPLACE2(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        CRS_DISPLACE2::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct TRIP_STAGES
{
    Dates::DateTime_t act;
    Dates::DateTime_t est;
    int ignore_auto;
    int point_id;
    int pr_auto;
    int pr_manual;
    Dates::DateTime_t scd;
    int stage_id;
    Dates::DateTime_t time_auto_not_act;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,act,"ACT");
        dbo::field(a,est,"EST");
        dbo::field(a,ignore_auto,"IGNORE_AUTO", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,pr_auto,"PR_AUTO", dbo::NotNull);
        dbo::field(a,pr_manual,"PR_MANUAL", dbo::NotNull);
        dbo::field(a,scd,"SCD", dbo::NotNull);
        dbo::field(a,stage_id,"STAGE_ID", dbo::NotNull);
        dbo::field(a,time_auto_not_act,"TIME_AUTO_NOT_ACT");
    }
};

struct ARX_TRIP_STAGES : public TRIP_STAGES
{
    Dates::DateTime_t part_key;
    ARX_TRIP_STAGES() = default;
    ARX_TRIP_STAGES(const TRIP_STAGES& sr, const Dates::DateTime_t & part_key_)
        :TRIP_STAGES(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,act,"ACT");
        dbo::field(a,est,"EST");
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,pr_auto,"PR_AUTO", dbo::NotNull);
        dbo::field(a,pr_manual,"PR_MANUAL", dbo::NotNull);
        dbo::field(a,scd,"SCD", dbo::NotNull);
        dbo::field(a,stage_id,"STAGE_ID", dbo::NotNull);
    }
};

struct BAG_RECEIPTS
{
    std::string aircode;
    std::string airline;
    std::string airp_arv;
    std::string airp_dep;
    Dates::DateTime_t annul_date;
    std::string annul_desk;
    int annul_user_id;
    std::string bag_name;
    int bag_type;
    std::string desk_lang;
    double exch_pay_rate;
    int exch_rate;
    int ex_amount;
    int ex_weight;
    int flt_no;
    std::string form_type;
    int grp_id;
    Dates::DateTime_t issue_date;
    std::string issue_desk;
    std::string issue_place;
    int issue_user_id;
    int is_inter;
    int kit_id;
    int kit_num;
    double nds;
    std::string nds_cur;
    int no;
    std::string pax_doc;
    std::string pax_name;
    std::string pay_rate_cur;
    int point_id;
    std::string prev_no;
    double rate;
    std::string rate_cur;
    int receipt_id;
    std::string remarks;
    Dates::DateTime_t scd_local_date;
    int service_type;
    std::string status;
    std::string suffix;
    std::string tickets;
    double value_tax;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,aircode,"AIRCODE", dbo::NotNull);
        dbo::field(a,airline,"AIRLINE", dbo::NotNull);
        dbo::field(a,airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,airp_dep,"AIRP_DEP", dbo::NotNull);
        dbo::field(a,annul_date,"ANNUL_DATE");
        dbo::field(a,annul_desk,"ANNUL_DESK");
        dbo::field(a,annul_user_id,"ANNUL_USER_ID");
        dbo::field(a,bag_name,"BAG_NAME");
        dbo::field(a,bag_type,"BAG_TYPE");
        dbo::field(a,desk_lang,"DESK_LANG", dbo::NotNull);
        dbo::field(a,exch_pay_rate,"EXCH_PAY_RATE");
        dbo::field(a,exch_rate,"EXCH_RATE");
        dbo::field(a,ex_amount,"EX_AMOUNT");
        dbo::field(a,ex_weight,"EX_WEIGHT");
        dbo::field(a,flt_no,"FLT_NO");
        dbo::field(a,form_type,"FORM_TYPE", dbo::NotNull);
        dbo::field(a,grp_id,"GRP_ID");
        dbo::field(a,issue_date,"ISSUE_DATE", dbo::NotNull);
        dbo::field(a,issue_desk,"ISSUE_DESK", dbo::NotNull);
        dbo::field(a,issue_place,"ISSUE_PLACE", dbo::NotNull);
        dbo::field(a,issue_user_id,"ISSUE_USER_ID", dbo::NotNull);
        dbo::field(a,is_inter,"IS_INTER", dbo::NotNull);
        dbo::field(a,kit_id,"KIT_ID");
        dbo::field(a,kit_num,"KIT_NUM");
        dbo::field(a,nds,"NDS");
        dbo::field(a,nds_cur,"NDS_CUR");
        dbo::field(a,no,"NO", dbo::NotNull);
        dbo::field(a,pax_doc,"PAX_DOC");
        dbo::field(a,pax_name,"PAX_NAME");
        dbo::field(a,pay_rate_cur,"PAY_RATE_CUR", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,prev_no,"PREV_NO");
        dbo::field(a,rate,"RATE", dbo::NotNull);
        dbo::field(a,rate_cur,"RATE_CUR", dbo::NotNull);
        dbo::field(a,receipt_id,"RECEIPT_ID", dbo::NotNull);
        dbo::field(a,remarks,"REMARKS");
        dbo::field(a,scd_local_date,"SCD_LOCAL_DATE");
        dbo::field(a,service_type,"SERVICE_TYPE", dbo::NotNull);
        dbo::field(a,status,"STATUS", dbo::NotNull);
        dbo::field(a,suffix,"SUFFIX");
        dbo::field(a,tickets,"TICKETS", dbo::NotNull);
        dbo::field(a,value_tax,"VALUE_TAX");
    }
};

//Отличие от BAG_RECEIPTS нет полей nds, nds_cur (part_key дополнительно поле)
struct ARX_BAG_RECEIPTS : public BAG_RECEIPTS
{
    ARX_BAG_RECEIPTS() = default;
    ARX_BAG_RECEIPTS(const BAG_RECEIPTS& sr, const Dates::DateTime_t & part_key_)
        :BAG_RECEIPTS(sr), part_key(part_key_) {}

    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,aircode,"AIRCODE", dbo::NotNull);
        dbo::field(a,airline,"AIRLINE", dbo::NotNull);
        dbo::field(a,airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,airp_dep,"AIRP_DEP", dbo::NotNull);
        dbo::field(a,annul_date,"ANNUL_DATE");
        dbo::field(a,annul_desk,"ANNUL_DESK");
        dbo::field(a,annul_user_id,"ANNUL_USER_ID");
        dbo::field(a,bag_name,"BAG_NAME");
        dbo::field(a,bag_type,"BAG_TYPE");
        dbo::field(a,desk_lang,"DESK_LANG", dbo::NotNull);
        dbo::field(a,exch_pay_rate,"EXCH_PAY_RATE");
        dbo::field(a,exch_rate,"EXCH_RATE");
        dbo::field(a,ex_amount,"EX_AMOUNT");
        dbo::field(a,ex_weight,"EX_WEIGHT");
        dbo::field(a,flt_no,"FLT_NO");
        dbo::field(a,form_type,"FORM_TYPE", dbo::NotNull);
        dbo::field(a,grp_id,"GRP_ID");
        dbo::field(a,issue_date,"ISSUE_DATE", dbo::NotNull);
        dbo::field(a,issue_desk,"ISSUE_DESK", dbo::NotNull);
        dbo::field(a,issue_place,"ISSUE_PLACE", dbo::NotNull);
        dbo::field(a,issue_user_id,"ISSUE_USER_ID", dbo::NotNull);
        dbo::field(a,is_inter,"IS_INTER", dbo::NotNull);
        dbo::field(a,kit_id,"KIT_ID");
        dbo::field(a,kit_num,"KIT_NUM");
        dbo::field(a,no,"NO", dbo::NotNull);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
        dbo::field(a,pax_doc,"PAX_DOC");
        dbo::field(a,pax_name,"PAX_NAME");
        dbo::field(a,pay_rate_cur,"PAY_RATE_CUR", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,prev_no,"PREV_NO");
        dbo::field(a,rate,"RATE", dbo::NotNull);
        dbo::field(a,rate_cur,"RATE_CUR", dbo::NotNull);
        dbo::field(a,receipt_id,"RECEIPT_ID", dbo::NotNull);
        dbo::field(a,remarks,"REMARKS");
        dbo::field(a,scd_local_date,"SCD_LOCAL_DATE");
        dbo::field(a,service_type,"SERVICE_TYPE", dbo::NotNull);
        dbo::field(a,status,"STATUS", dbo::NotNull);
        dbo::field(a,suffix,"SUFFIX");
        dbo::field(a,tickets,"TICKETS", dbo::NotNull);
        dbo::field(a,value_tax,"VALUE_TAX");
    }
};


struct BAG_PAY_TYPES
{
    std::string extra;
    int num;
    double pay_rate_sum;
    std::string pay_type;
    int receipt_id;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,extra,"EXTRA");
        dbo::field(a,num,"NUM", dbo::NotNull);
        dbo::field(a,pay_rate_sum,"PAY_RATE_SUM", dbo::NotNull);
        dbo::field(a,pay_type,"PAY_TYPE", dbo::NotNull);
        dbo::field(a,receipt_id,"RECEIPT_ID", dbo::NotNull);
    }
};

struct ARX_BAG_PAY_TYPES : public BAG_PAY_TYPES
{
    Dates::DateTime_t part_key;
    ARX_BAG_PAY_TYPES() = default;
    ARX_BAG_PAY_TYPES(const BAG_PAY_TYPES& sr, const Dates::DateTime_t & part_key_)
        :BAG_PAY_TYPES(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        BAG_PAY_TYPES::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct ANNUL_BAG
{
    int amount;
    int bag_type;
    int grp_id;
    int id;
    int pax_id;
    std::string rfisc;
    Dates::DateTime_t time_annul;
    Dates::DateTime_t time_create;
    int user_id;
    int weight;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,amount,"AMOUNT");
        dbo::field(a,bag_type,"BAG_TYPE");
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,id,"ID", dbo::NotNull);
        dbo::field(a,pax_id,"PAX_ID");
        dbo::field(a,rfisc,"RFISC");
        dbo::field(a,time_annul,"TIME_ANNUL");
        dbo::field(a,time_create,"TIME_CREATE");
        dbo::field(a,user_id,"USER_ID");
        dbo::field(a,weight,"WEIGHT");
    }
};

struct ARX_ANNUL_BAG : public ANNUL_BAG
{
    Dates::DateTime_t part_key;
    ARX_ANNUL_BAG() = default;
    ARX_ANNUL_BAG(const ANNUL_BAG& sr, const Dates::DateTime_t & part_key_)
        :ANNUL_BAG(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        ANNUL_BAG::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct ANNUL_TAGS
{
    int id;
    int no;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,id,"ID", dbo::NotNull);
        dbo::field(a,no,"NO", dbo::NotNull);
    }
};


struct ARX_ANNUL_TAGS : public ANNUL_TAGS
{
    Dates::DateTime_t part_key;
    ARX_ANNUL_TAGS() = default;
    ARX_ANNUL_TAGS(const ANNUL_TAGS& sr, const Dates::DateTime_t & part_key_)
        :ANNUL_TAGS(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        ANNUL_TAGS::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct UNACCOMP_BAG_INFO
{
    std::string airline;
    int flt_no;
    int grp_id;
    std::string name;
    int num;
    std::string original_tag_no;
    Dates::DateTime_t scd;
    std::string suffix;
    std::string surname;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE");
        dbo::field(a,flt_no,"FLT_NO");
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,name,"NAME");
        dbo::field(a,num,"NUM", dbo::NotNull);
        dbo::field(a,original_tag_no,"ORIGINAL_TAG_NO");
        dbo::field(a,scd,"SCD");
        dbo::field(a,suffix,"SUFFIX");
        dbo::field(a,surname,"SURNAME");
    }
};

struct ARX_UNACCOMP_BAG_INFO  : public UNACCOMP_BAG_INFO
{
    Dates::DateTime_t part_key;
    ARX_UNACCOMP_BAG_INFO() = default;
    ARX_UNACCOMP_BAG_INFO(const UNACCOMP_BAG_INFO& sr, const Dates::DateTime_t & part_key_)
        :UNACCOMP_BAG_INFO(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        UNACCOMP_BAG_INFO::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct BAG2
{
    std::string airline;
    int amount;
    int bag_pool_num;
    int bag_type;
    std::string bag_type_str;
    std::string desk;
    int grp_id;
    int hall;
    int handmade;
    int id;
    int is_trfer;
    int list_id;
    int num;
    int pr_cabin;
    int pr_liab_limit;
    std::string rfisc;
    std::string service_type;
    Dates::DateTime_t time_create;
    int to_ramp;
    int user_id;
    int using_scales;
    int value_bag_num;
    int weight;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE", dbo::NotNull);
        dbo::field(a,amount,"AMOUNT", dbo::NotNull);
        dbo::field(a,bag_pool_num,"BAG_POOL_NUM", dbo::NotNull);
        dbo::field(a,bag_type,"BAG_TYPE");
        dbo::field(a,bag_type_str,"BAG_TYPE_STR");
        dbo::field(a,desk,"DESK");
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,hall,"HALL");
        dbo::field(a,handmade,"HANDMADE", dbo::NotNull);
        dbo::field(a,id,"ID", dbo::NotNull);
        dbo::field(a,is_trfer,"IS_TRFER", dbo::NotNull);
        dbo::field(a,list_id,"LIST_ID", dbo::NotNull);
        dbo::field(a,num,"NUM", dbo::NotNull);
        dbo::field(a,pr_cabin,"PR_CABIN", dbo::NotNull);
        dbo::field(a,pr_liab_limit,"PR_LIAB_LIMIT", dbo::NotNull);
        dbo::field(a,rfisc,"RFISC");
        dbo::field(a,service_type,"SERVICE_TYPE");
        dbo::field(a,time_create,"TIME_CREATE");
        dbo::field(a,to_ramp,"TO_RAMP", dbo::NotNull);
        dbo::field(a,user_id,"USER_ID");
        dbo::field(a,using_scales,"USING_SCALES", dbo::NotNull);
        dbo::field(a,value_bag_num,"VALUE_BAG_NUM");
        dbo::field(a,weight,"WEIGHT", dbo::NotNull);
    }
};

struct ARX_BAG2 : public BAG2
{
    Dates::DateTime_t part_key;
    ARX_BAG2() = default;
    ARX_BAG2(const BAG2& sr, const Dates::DateTime_t & part_key_)
        :BAG2(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        BAG2::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct BAG_PREPAY
{
    std::string aircode;
    int bag_type;
    int ex_weight;
    int grp_id;
    std::string no;
    int receipt_id;
    double value;
    std::string value_cur;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,aircode,"AIRCODE", dbo::NotNull);
        dbo::field(a,bag_type,"BAG_TYPE");
        dbo::field(a,ex_weight,"EX_WEIGHT");
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,no,"NO", dbo::NotNull);
        dbo::field(a,receipt_id,"RECEIPT_ID", dbo::NotNull);
        dbo::field(a,value,"VALUE");
        dbo::field(a,value_cur,"VALUE_CUR");
    }
};

struct ARX_BAG_PREPAY : public BAG_PREPAY
{
    Dates::DateTime_t part_key;
    ARX_BAG_PREPAY() = default;
    ARX_BAG_PREPAY(const BAG_PREPAY& sr, const Dates::DateTime_t & part_key_)
        :BAG_PREPAY(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        BAG_PREPAY::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct BAG_TAGS
{
    int bag_num;
    std::string color;
    int grp_id;
    int no;
    int num;
    int pr_print;
    std::string tag_type;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,bag_num,"BAG_NUM");
        dbo::field(a,color,"COLOR");
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,no,"NO", dbo::NotNull);
        dbo::field(a,num,"NUM", dbo::NotNull);
        dbo::field(a,pr_print,"PR_PRINT", dbo::NotNull);
        dbo::field(a,tag_type,"TAG_TYPE", dbo::NotNull);
    }
};

struct ARX_BAG_TAGS : public BAG_TAGS
{
    Dates::DateTime_t part_key;
    ARX_BAG_TAGS() = default;
    ARX_BAG_TAGS(const BAG_TAGS& sr, const Dates::DateTime_t & part_key_)
        :BAG_TAGS(sr), part_key(part_key_) {}
    template<typename Action>
    void persist(Action & a) {
        BAG_TAGS::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct PAID_BAG
{
    std::string airline;
    int bag_type;
    std::string bag_type_str;
    int grp_id;
    int handmade;
    int list_id;
    int rate_id;
    int rate_trfer;
    int weight;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE", dbo::NotNull);
        dbo::field(a,bag_type,"BAG_TYPE");
        dbo::field(a,bag_type_str,"BAG_TYPE_STR");
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,handmade,"HANDMADE");
        dbo::field(a,list_id,"LIST_ID", dbo::NotNull);
        dbo::field(a,rate_id,"RATE_ID");
        dbo::field(a,rate_trfer,"RATE_TRFER");
        dbo::field(a,weight,"WEIGHT", dbo::NotNull);
    }
};

struct ARX_PAID_BAG : public PAID_BAG
{
    Dates::DateTime_t part_key;
    ARX_PAID_BAG() = default;
    ARX_PAID_BAG(const PAID_BAG& sr, const Dates::DateTime_t & part_key_)
        :PAID_BAG(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        PAID_BAG::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct PAX
{
    int bag_pool_num = ASTRA::NoExists;
    std::string cabin_class;
    int cabin_class_grp= ASTRA::NoExists;
    std::string cabin_subclass;
    int coupon_no= ASTRA::NoExists;
    std::string crew_type;
    int doco_confirm;
    int grp_id;
    int is_female= ASTRA::NoExists;
    int is_jmp;
    std::string name;
    int pax_id;
    std::string pers_type;
    int pr_brd= ASTRA::NoExists;
    int pr_exam;
    std::string refuse;
    int reg_no;
    int seats;
    std::string seat_type;
    std::string subclass;
    std::string surname;
    int ticket_confirm;
    std::string ticket_no;
    std::string ticket_rem;
    int tid;
    std::string wl_type;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,bag_pool_num,"BAG_POOL_NUM");
        dbo::field(a,cabin_class,"CABIN_CLASS");
        dbo::field(a,cabin_class_grp,"CABIN_CLASS_GRP");
        dbo::field(a,cabin_subclass,"CABIN_SUBCLASS");
        dbo::field(a,coupon_no,"COUPON_NO");
        dbo::field(a,crew_type,"CREW_TYPE");
        dbo::field(a,doco_confirm,"DOCO_CONFIRM", dbo::NotNull);
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,is_female,"IS_FEMALE");
        dbo::field(a,is_jmp,"IS_JMP", dbo::NotNull);
        dbo::field(a,name,"NAME");
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,pers_type,"PERS_TYPE", dbo::NotNull);
        dbo::field(a,pr_brd,"PR_BRD");
        dbo::field(a,pr_exam,"PR_EXAM", dbo::NotNull);
        dbo::field(a,refuse,"REFUSE");
        dbo::field(a,reg_no,"REG_NO", dbo::NotNull);
        dbo::field(a,seats,"SEATS", dbo::NotNull);
        dbo::field(a,seat_type,"SEAT_TYPE");
        dbo::field(a,subclass,"SUBCLASS");
        dbo::field(a,surname,"SURNAME", dbo::NotNull);
        dbo::field(a,ticket_confirm,"TICKET_CONFIRM", dbo::NotNull);
        dbo::field(a,ticket_no,"TICKET_NO");
        dbo::field(a,ticket_rem,"TICKET_REM");
        dbo::field(a,tid,"TID", dbo::NotNull);
        dbo::field(a,wl_type,"WL_TYPE");
    }
};

//Отличие от Pax нет crew_type
struct ARX_PAX : public PAX
{
    int excess_pc = ASTRA::NoExists;
    Dates::DateTime_t part_key;
    std::string seat_no;

    ARX_PAX() = default;
    ARX_PAX(const PAX& sr, const Dates::DateTime_t & part_key_, int excess_pc_, std::string seat_no_)
        :PAX(sr), excess_pc(excess_pc_), part_key(part_key_),  seat_no(seat_no_) {}

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,bag_pool_num,"BAG_POOL_NUM");
        dbo::field(a,cabin_class,"CABIN_CLASS");
        dbo::field(a,cabin_class_grp,"CABIN_CLASS_GRP");
        dbo::field(a,cabin_subclass,"CABIN_SUBCLASS");
        dbo::field(a,coupon_no,"COUPON_NO");
        dbo::field(a,doco_confirm,"DOCO_CONFIRM");
        dbo::field(a,excess_pc,"EXCESS_PC");
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,is_female,"IS_FEMALE");
        dbo::field(a,is_jmp,"IS_JMP");
        dbo::field(a,name,"NAME");
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,pers_type,"PERS_TYPE", dbo::NotNull);
        dbo::field(a,pr_brd,"PR_BRD");
        dbo::field(a,pr_exam,"PR_EXAM", dbo::NotNull);
        dbo::field(a,refuse,"REFUSE");
        dbo::field(a,reg_no,"REG_NO", dbo::NotNull);
        dbo::field(a,seats,"SEATS", dbo::NotNull);
        dbo::field(a,seat_no,"SEAT_NO");
        dbo::field(a,seat_type,"SEAT_TYPE");
        dbo::field(a,subclass,"SUBCLASS");
        dbo::field(a,surname,"SURNAME", dbo::NotNull);
        dbo::field(a,ticket_confirm,"TICKET_CONFIRM", dbo::NotNull);
        dbo::field(a,ticket_no,"TICKET_NO");
        dbo::field(a,ticket_rem,"TICKET_REM");
        dbo::field(a,tid,"TID", dbo::NotNull);
        dbo::field(a,wl_type,"WL_TYPE");
    }
};


struct TRANSFER
{
    int airline_fmt;
    std::string airp_arv;
    int airp_arv_fmt;
    int airp_dep_fmt;
    int grp_id;
    int piece_concept;
    int point_id_trfer;
    int pr_final;
    int suffix_fmt = ASTRA::NoExists;
    int transfer_num;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline_fmt,"AIRLINE_FMT", dbo::NotNull);
        dbo::field(a,airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,airp_arv_fmt,"AIRP_ARV_FMT", dbo::NotNull);
        dbo::field(a,airp_dep_fmt,"AIRP_DEP_FMT", dbo::NotNull);
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,piece_concept,"PIECE_CONCEPT");
        dbo::field(a,point_id_trfer,"POINT_ID_TRFER", dbo::NotNull);
        dbo::field(a,pr_final,"PR_FINAL", dbo::NotNull);
        dbo::field(a,suffix_fmt,"SUFFIX_FMT");
        dbo::field(a,transfer_num,"TRANSFER_NUM", dbo::NotNull);
    }
};

struct TRFER_TRIPS
{
    std::string airline;
    std::string airp_dep;
    int flt_no;
    int point_id;
    int point_id_spp;
    Dates::DateTime_t scd;
    std::string suffix;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE", dbo::NotNull);
        dbo::field(a,airp_dep,"AIRP_DEP", dbo::NotNull);
        dbo::field(a,flt_no,"FLT_NO", dbo::NotNull);
        dbo::field(a,point_id,"POINT_ID", dbo::NotNull);
        dbo::field(a,point_id_spp,"POINT_ID_SPP");
        dbo::field(a,scd,"SCD", dbo::NotNull);
        dbo::field(a,suffix,"SUFFIX");
    }
};


//Нету TRFER.point_id_transfer и нету TRFER_TRIPS.point_id, point_id_spp
struct ARX_TRANSFER
{
    ARX_TRANSFER() = default;
    ARX_TRANSFER(const TRANSFER& t, const TRFER_TRIPS & tt, const Dates::DateTime_t & part_key_)
        :trfer(t),  trips(tt), part_key(part_key_) {}

    TRANSFER trfer;
    TRFER_TRIPS trips;
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,trips.airline,"AIRLINE", dbo::NotNull);
        dbo::field(a,trfer.airline_fmt,"AIRLINE_FMT", dbo::NotNull);
        dbo::field(a,trfer.airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,trfer.airp_arv_fmt,"AIRP_ARV_FMT", dbo::NotNull);
        dbo::field(a,trips.airp_dep,"AIRP_DEP", dbo::NotNull);
        dbo::field(a,trfer.airp_dep_fmt,"AIRP_DEP_FMT", dbo::NotNull);
        dbo::field(a,trips.flt_no,"FLT_NO", dbo::NotNull);
        dbo::field(a,trfer.grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
        dbo::field(a,trfer.piece_concept,"PIECE_CONCEPT");
        dbo::field(a,trfer.pr_final,"PR_FINAL", dbo::NotNull);
        dbo::field(a,trips.scd,"SCD", dbo::NotNull);
        dbo::field(a,trips.suffix,"SUFFIX");
        dbo::field(a,trfer.suffix_fmt,"SUFFIX_FMT");
        dbo::field(a,trfer.transfer_num,"TRANSFER_NUM", dbo::NotNull);
    }
};

struct TCKIN_SEGMENTS
{
    std::string airp_arv;
    int grp_id;
    int point_id_trfer;
    int pr_final;
    int seg_no;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,point_id_trfer,"POINT_ID_TRFER", dbo::NotNull);
        dbo::field(a,pr_final,"PR_FINAL", dbo::NotNull);
        dbo::field(a,seg_no,"SEG_NO", dbo::NotNull);
    }
};

struct ARX_TCKIN_SEGMENTS
{
    ARX_TCKIN_SEGMENTS() = default;
    ARX_TCKIN_SEGMENTS(const TCKIN_SEGMENTS& ts, const TRFER_TRIPS & tt, const Dates::DateTime_t & part_key_)
        :ckin_segs(ts),  trips(tt), part_key(part_key_) {}

    TCKIN_SEGMENTS ckin_segs;
    TRFER_TRIPS trips;
    Dates::DateTime_t part_key;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,trips.airline,"AIRLINE", dbo::NotNull);
        dbo::field(a,ckin_segs.airp_arv,"AIRP_ARV", dbo::NotNull);
        dbo::field(a,trips.airp_dep,"AIRP_DEP", dbo::NotNull);
        dbo::field(a,trips.flt_no,"FLT_NO", dbo::NotNull);
        dbo::field(a,ckin_segs.grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
        dbo::field(a,ckin_segs.pr_final,"PR_FINAL", dbo::NotNull);
        dbo::field(a,trips.scd,"SCD", dbo::NotNull);
        dbo::field(a,ckin_segs.seg_no,"SEG_NO", dbo::NotNull);
        dbo::field(a,trips.suffix,"SUFFIX");
    }
};

struct VALUE_BAG
{
    int grp_id;
    int num;
    double tax = ASTRA::NoExists;
    int tax_id = ASTRA::NoExists;
    int tax_trfer = ASTRA::NoExists;
    double value;
    std::string value_cur;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,num,"NUM", dbo::NotNull);
        dbo::field(a,tax,"TAX");
        dbo::field(a,tax_id,"TAX_ID");
        dbo::field(a,tax_trfer,"TAX_TRFER");
        dbo::field(a,value,"VALUE", dbo::NotNull);
        dbo::field(a,value_cur,"VALUE_CUR", dbo::NotNull);
    }
};

struct ARX_VALUE_BAG : public VALUE_BAG
{
    Dates::DateTime_t part_key;

    ARX_VALUE_BAG() = default;
    ARX_VALUE_BAG(const VALUE_BAG& sr, const Dates::DateTime_t & part_key_)
        :VALUE_BAG(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        VALUE_BAG::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct GRP_NORMS
{
    std::string airline;
    int bag_type = ASTRA::NoExists;
    std::string bag_type_str;
    int grp_id;
    int handmade = ASTRA::NoExists;
    int list_id = ASTRA::NoExists;
    int norm_id = ASTRA::NoExists;
    int norm_trfer = ASTRA::NoExists;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE");
        dbo::field(a,bag_type,"BAG_TYPE");
        dbo::field(a,bag_type_str,"BAG_TYPE_STR");
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,handmade,"HANDMADE");
        dbo::field(a,list_id,"LIST_ID");
        dbo::field(a,norm_id,"NORM_ID");
        dbo::field(a,norm_trfer,"NORM_TRFER");
    }
};

struct ARX_GRP_NORMS : public GRP_NORMS
{
    Dates::DateTime_t part_key;
    ARX_GRP_NORMS() = default;
    ARX_GRP_NORMS(const GRP_NORMS& sr, const Dates::DateTime_t & part_key_)
        :GRP_NORMS(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        GRP_NORMS::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct PAX_NORMS
{
    std::string airline;
    int bag_type = ASTRA::NoExists;
    std::string bag_type_str;
    int handmade = ASTRA::NoExists;
    int list_id = ASTRA::NoExists;
    int norm_id = ASTRA::NoExists;
    int norm_trfer = ASTRA::NoExists;
    int pax_id;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE");
        dbo::field(a,bag_type,"BAG_TYPE");
        dbo::field(a,bag_type_str,"BAG_TYPE_STR");
        dbo::field(a,handmade,"HANDMADE");
        dbo::field(a,list_id,"LIST_ID");
        dbo::field(a,norm_id,"NORM_ID");
        dbo::field(a,norm_trfer,"NORM_TRFER");
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
    }
};

struct ARX_PAX_NORMS : public PAX_NORMS
{
    Dates::DateTime_t part_key;

    ARX_PAX_NORMS() = default;
    ARX_PAX_NORMS(const PAX_NORMS& sr, const Dates::DateTime_t & part_key_)
        :PAX_NORMS(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        PAX_NORMS::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct PAX_REM
{
    int pax_id;
    std::string rem;
    std::string rem_code;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,rem,"REM", dbo::NotNull);
        dbo::field(a,rem_code,"REM_CODE");
    }
};

struct ARX_PAX_REM : public PAX_REM
{
    Dates::DateTime_t part_key;

    ARX_PAX_REM() = default;
    ARX_PAX_REM(const PAX_REM& sr, const Dates::DateTime_t & part_key_)
        :PAX_REM(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        PAX_REM::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct TRANSFER_SUBCLS
{
    int pax_id;
    std::string subclass;
    int subclass_fmt;
    int transfer_num;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,subclass,"SUBCLASS", dbo::NotNull);
        dbo::field(a,subclass_fmt,"SUBCLASS_FMT", dbo::NotNull);
        dbo::field(a,transfer_num,"TRANSFER_NUM", dbo::NotNull);
    }
};

struct ARX_TRANSFER_SUBCLS : public TRANSFER_SUBCLS
{
    Dates::DateTime_t part_key;
    ARX_TRANSFER_SUBCLS() = default;
    ARX_TRANSFER_SUBCLS(const TRANSFER_SUBCLS& sr, const Dates::DateTime_t & part_key_)
        :TRANSFER_SUBCLS(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        TRANSFER_SUBCLS::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};


struct PAX_DOC
{
    Dates::DateTime_t birth_date;
    Dates::DateTime_t expiry_date;
    std::string first_name;
    std::string gender;
    std::string issue_country;
    std::string nationality;
    std::string no;
    int pax_id;
    int pr_multi;
    int scanned_attrs = ASTRA::NoExists;
    std::string second_name;
    std::string subtype;
    std::string surname;
    std::string type;
    std::string type_rcpt;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,birth_date,"BIRTH_DATE");
        dbo::field(a,expiry_date,"EXPIRY_DATE");
        dbo::field(a,first_name,"FIRST_NAME");
        dbo::field(a,gender,"GENDER");
        dbo::field(a,issue_country,"ISSUE_COUNTRY");
        dbo::field(a,nationality,"NATIONALITY");
        dbo::field(a,no,"NO");
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,pr_multi,"PR_MULTI", dbo::NotNull);
        dbo::field(a,scanned_attrs,"SCANNED_ATTRS");
        dbo::field(a,second_name,"SECOND_NAME");
        dbo::field(a,subtype,"SUBTYPE");
        dbo::field(a,surname,"SURNAME");
        dbo::field(a,type,"TYPE");
        dbo::field(a,type_rcpt,"TYPE_RCPT");
    }
};

struct ARX_PAX_DOC : public PAX_DOC
{
    Dates::DateTime_t part_key;
    ARX_PAX_DOC() = default;
    ARX_PAX_DOC(const PAX_DOC& sr, const Dates::DateTime_t & part_key_)
        :PAX_DOC(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        PAX_DOC::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct PAX_DOCO {
    std::string applic_country;
    std::string birth_place;
    Dates::DateTime_t expiry_date;
    Dates::DateTime_t issue_date;
    std::string issue_place;
    std::string no;
    int pax_id;
    int scanned_attrs = ASTRA::NoExists;
    std::string subtype;
    std::string type;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,applic_country,"APPLIC_COUNTRY");
        dbo::field(a,birth_place,"BIRTH_PLACE");
        dbo::field(a,expiry_date,"EXPIRY_DATE");
        dbo::field(a,issue_date,"ISSUE_DATE");
        dbo::field(a,issue_place,"ISSUE_PLACE");
        dbo::field(a,no,"NO");
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,scanned_attrs,"SCANNED_ATTRS");
        dbo::field(a,subtype,"SUBTYPE");
        dbo::field(a,type,"TYPE");
    }
};


struct ARX_PAX_DOCO : public PAX_DOCO
{
    Dates::DateTime_t part_key;
    ARX_PAX_DOCO() = default;
    ARX_PAX_DOCO(const PAX_DOCO& sr, const Dates::DateTime_t & part_key_)
        :PAX_DOCO(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        PAX_DOCO::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);

    }
};

struct PAX_DOCA
{
    std::string address;
    std::string city;
    std::string country;
    int pax_id;
    std::string postal_code;
    std::string region;
    std::string type;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,address,"ADDRESS");
        dbo::field(a,city,"CITY");
        dbo::field(a,country,"COUNTRY");
        dbo::field(a,pax_id,"PAX_ID", dbo::NotNull);
        dbo::field(a,postal_code,"POSTAL_CODE");
        dbo::field(a,region,"REGION");
        dbo::field(a,type,"TYPE", dbo::NotNull);
    }
};


struct ARX_PAX_DOCA : public PAX_DOCA
{
    Dates::DateTime_t part_key;
    ARX_PAX_DOCA() = default;
    ARX_PAX_DOCA(const PAX_DOCA& sr, const Dates::DateTime_t & part_key_)
        :PAX_DOCA(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        PAX_DOCA::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);

    }
};

struct STAT_ZAMAR
{
    Dates::DateTime_t time;
    std::string airline;
    std::string airp;
    int amount_ok;
    int amount_fault;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,time,"TIME", dbo::NotNull);
        dbo::field(a,airline,"AIRLINE", dbo::NotNull);
        dbo::field(a,airp,"AIRP", dbo::NotNull);
        dbo::field(a,amount_ok,"AMOUNT_OK", dbo::NotNull);
        dbo::field(a,amount_fault,"AMOUNT_FAULT", dbo::NotNull);
    }
};

struct ARX_STAT_ZAMAR : public STAT_ZAMAR
{
    Dates::DateTime_t part_key;
    ARX_STAT_ZAMAR() = default;
    ARX_STAT_ZAMAR(const STAT_ZAMAR& sr, const Dates::DateTime_t & part_key_)
        :STAT_ZAMAR(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        STAT_ZAMAR::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};


struct TLG_TRANSFER
{
    int point_id_in;
    int point_id_out;
    std::string subcl_in;
    std::string subcl_out;
    int tlg_id;
    int trfer_id;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,point_id_in,"POINT_ID_IN", dbo::NotNull);
        dbo::field(a,point_id_out,"POINT_ID_OUT", dbo::NotNull);
        dbo::field(a,subcl_in,"SUBCL_IN");
        dbo::field(a,subcl_out,"SUBCL_OUT");
        dbo::field(a,tlg_id,"TLG_ID", dbo::NotNull);
        dbo::field(a,trfer_id,"TRFER_ID", dbo::NotNull);
    }
};

struct TRFER_GRP
{
    int bag_amount = ASTRA::NoExists;
    int bag_weight = ASTRA::NoExists;
    int grp_id;
    int rk_weight = ASTRA::NoExists;
    int seats = ASTRA::NoExists;
    int trfer_id;
    std::string weight_unit;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,bag_amount,"BAG_AMOUNT");
        dbo::field(a,bag_weight,"BAG_WEIGHT");
        dbo::field(a,grp_id,"GRP_ID", dbo::NotNull);
        dbo::field(a,rk_weight,"RK_WEIGHT");
        dbo::field(a,seats,"SEATS");
        dbo::field(a,trfer_id,"TRFER_ID", dbo::NotNull);
        dbo::field(a,weight_unit,"WEIGHT_UNIT");
    }
};

struct BAG_NORMS
{
    std::string airline;
    int amount = ASTRA::NoExists;
    int bag_type = ASTRA::NoExists;
    std::string city_arv;
    std::string city_dep;
    std::string m_class;
    std::string craft;
    int direct_action;
    std::string extra;
    Dates::DateTime_t first_date;
    int flt_no = ASTRA::NoExists;
    int id;
    Dates::DateTime_t last_date;
    std::string norm_type;
    std::string pax_cat;
    int per_unit = ASTRA::NoExists;
    int pr_del;
    int pr_trfer = ASTRA::NoExists;
    std::string subclass;
    int tid;
    std::string trip_type;
    int weight = ASTRA::NoExists;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE");
        dbo::field(a,amount,"AMOUNT");
        dbo::field(a,bag_type,"BAG_TYPE");
        dbo::field(a,city_arv,"CITY_ARV");
        dbo::field(a,city_dep,"CITY_DEP");
        dbo::field(a,m_class,"CLASS");
        dbo::field(a,craft,"CRAFT");
        dbo::field(a,direct_action,"DIRECT_ACTION", dbo::NotNull);
        dbo::field(a,extra,"EXTRA");
        dbo::field(a,first_date,"FIRST_DATE", dbo::NotNull);
        dbo::field(a,flt_no,"FLT_NO");
        dbo::field(a,id,"ID", dbo::NotNull);
        dbo::field(a,last_date,"LAST_DATE");
        dbo::field(a,norm_type,"NORM_TYPE", dbo::NotNull);
        dbo::field(a,pax_cat,"PAX_CAT");
        dbo::field(a,per_unit,"PER_UNIT");
        dbo::field(a,pr_del,"PR_DEL", dbo::NotNull);
        dbo::field(a,pr_trfer,"PR_TRFER");
        dbo::field(a,subclass,"SUBCLASS");
        dbo::field(a,tid,"TID", dbo::NotNull);
        dbo::field(a,trip_type,"TRIP_TYPE");
        dbo::field(a,weight,"WEIGHT");
    }
};

struct ARX_BAG_NORMS : public BAG_NORMS
{
    Dates::DateTime_t part_key;
    ARX_BAG_NORMS() = default;
    ARX_BAG_NORMS(const BAG_NORMS& sr, const Dates::DateTime_t & part_key_)
        :BAG_NORMS(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        BAG_NORMS::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct BAG_RATES
{
    std::string airline;
    int bag_type = ASTRA::NoExists;
    std::string city_arv;
    std::string city_dep;
    std::string m_class;
    std::string craft;
    std::string extra;
    Dates::DateTime_t first_date;
    int flt_no = ASTRA::NoExists;
    int id;
    Dates::DateTime_t last_date;
    int min_weight = ASTRA::NoExists;
    std::string pax_cat;
    int pr_del;
    int pr_trfer = ASTRA::NoExists;
    double rate;
    std::string rate_cur;
    std::string subclass;
    int tid;
    std::string trip_type;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE");
        dbo::field(a,bag_type,"BAG_TYPE");
        dbo::field(a,city_arv,"CITY_ARV");
        dbo::field(a,city_dep,"CITY_DEP");
        dbo::field(a, m_class,"CLASS");
        dbo::field(a,craft,"CRAFT");
        dbo::field(a,extra,"EXTRA");
        dbo::field(a,first_date,"FIRST_DATE", dbo::NotNull);
        dbo::field(a,flt_no,"FLT_NO");
        dbo::field(a,id,"ID", dbo::NotNull);
        dbo::field(a,last_date,"LAST_DATE");
        dbo::field(a,min_weight,"MIN_WEIGHT");
        dbo::field(a,pax_cat,"PAX_CAT");
        dbo::field(a,pr_del,"PR_DEL", dbo::NotNull);
        dbo::field(a,pr_trfer,"PR_TRFER");
        dbo::field(a,rate,"RATE", dbo::NotNull);
        dbo::field(a,rate_cur,"RATE_CUR", dbo::NotNull);
        dbo::field(a,subclass,"SUBCLASS");
        dbo::field(a,tid,"TID", dbo::NotNull);
        dbo::field(a,trip_type,"TRIP_TYPE");
    }
};

struct ARX_BAG_RATES : public BAG_RATES
{
    Dates::DateTime_t part_key;

    ARX_BAG_RATES() = default;
    ARX_BAG_RATES(const BAG_RATES& sr, const Dates::DateTime_t & part_key_)
        :BAG_RATES(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        BAG_RATES::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct VALUE_BAG_TAXES
{
    std::string airline;
    std::string city_arv;
    std::string city_dep;
    std::string extra;
    Dates::DateTime_t first_date;
    int id;
    Dates::DateTime_t last_date;
    int min_value = ASTRA::NoExists;
    std::string min_value_cur;
    int pr_del;
    int pr_trfer = ASTRA::NoExists;
    float tax;
    int tid;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE");
        dbo::field(a,city_arv,"CITY_ARV");
        dbo::field(a,city_dep,"CITY_DEP");
        dbo::field(a,extra,"EXTRA");
        dbo::field(a,first_date,"FIRST_DATE", dbo::NotNull);
        dbo::field(a,id,"ID", dbo::NotNull);
        dbo::field(a,last_date,"LAST_DATE");
        dbo::field(a,min_value,"MIN_VALUE");
        dbo::field(a,min_value_cur,"MIN_VALUE_CUR");
        dbo::field(a,pr_del,"PR_DEL", dbo::NotNull);
        dbo::field(a,pr_trfer,"PR_TRFER");
        dbo::field(a,tax,"TAX", dbo::NotNull);
        dbo::field(a,tid,"TID", dbo::NotNull);
    }
};

struct ARX_VALUE_BAG_TAXES : public VALUE_BAG_TAXES
{
    Dates::DateTime_t part_key;
    ARX_VALUE_BAG_TAXES() = default;
    ARX_VALUE_BAG_TAXES(const VALUE_BAG_TAXES& sr, const Dates::DateTime_t & part_key_)
        :VALUE_BAG_TAXES(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        VALUE_BAG_TAXES::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct EXCHANGE_RATES
{
    std::string airline;
    std::string cur1;
    std::string cur2;
    std::string extra;
    Dates::DateTime_t first_date;
    int id;
    Dates::DateTime_t last_date;
    int pr_del;
    int rate1;
    double rate2;
    int tid;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE");
        dbo::field(a,cur1,"CUR1", dbo::NotNull);
        dbo::field(a,cur2,"CUR2", dbo::NotNull);
        dbo::field(a,extra,"EXTRA");
        dbo::field(a,first_date,"FIRST_DATE", dbo::NotNull);
        dbo::field(a,id,"ID", dbo::NotNull);
        dbo::field(a,last_date,"LAST_DATE");
        dbo::field(a,pr_del,"PR_DEL", dbo::NotNull);
        dbo::field(a,rate1,"RATE1", dbo::NotNull);
        dbo::field(a,rate2,"RATE2", dbo::NotNull);
        dbo::field(a,tid,"TID", dbo::NotNull);
    }
};

struct ARX_EXCHANGE_RATES : public EXCHANGE_RATES
{
    Dates::DateTime_t part_key;
    ARX_EXCHANGE_RATES() = default;
    ARX_EXCHANGE_RATES(const EXCHANGE_RATES& sr, const Dates::DateTime_t & part_key_)
        :EXCHANGE_RATES(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        EXCHANGE_RATES::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct TLG_STAT
{
    std::string airline;
    std::string airline_mark;
    std::string airp_dep;
    std::string extra;
    int flt_no = ASTRA::NoExists;
    int queue_tlg_id;
    std::string receiver_canon_name;
    std::string receiver_country;
    std::string receiver_descr;
    std::string receiver_sita_addr;
    Dates::DateTime_t scd_local_date;
    std::string sender_canon_name;
    std::string sender_country;
    std::string sender_descr;
    std::string sender_sita_addr;
    std::string suffix;
    Dates::DateTime_t time_create;
    Dates::DateTime_t time_receive;
    Dates::DateTime_t time_send;
    int tlg_len;
    std::string tlg_type;
    int typeb_tlg_id;
    int typeb_tlg_num;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE");
        dbo::field(a,airline_mark,"AIRLINE_MARK");
        dbo::field(a,airp_dep,"AIRP_DEP");
        dbo::field(a,extra,"EXTRA", dbo::NotNull);
        dbo::field(a,flt_no,"FLT_NO");
        dbo::field(a,queue_tlg_id,"QUEUE_TLG_ID", dbo::NotNull);
        dbo::field(a,receiver_canon_name,"RECEIVER_CANON_NAME", dbo::NotNull);
        dbo::field(a,receiver_country,"RECEIVER_COUNTRY");
        dbo::field(a,receiver_descr,"RECEIVER_DESCR", dbo::NotNull);
        dbo::field(a,receiver_sita_addr,"RECEIVER_SITA_ADDR", dbo::NotNull);
        dbo::field(a,scd_local_date,"SCD_LOCAL_DATE");
        dbo::field(a,sender_canon_name,"SENDER_CANON_NAME", dbo::NotNull);
        dbo::field(a,sender_country,"SENDER_COUNTRY");
        dbo::field(a,sender_descr,"SENDER_DESCR", dbo::NotNull);
        dbo::field(a,sender_sita_addr,"SENDER_SITA_ADDR", dbo::NotNull);
        dbo::field(a,suffix,"SUFFIX");
        dbo::field(a,time_create,"TIME_CREATE", dbo::NotNull);
        dbo::field(a,time_receive,"TIME_RECEIVE");
        dbo::field(a,time_send,"TIME_SEND");
        dbo::field(a,tlg_len,"TLG_LEN", dbo::NotNull);
        dbo::field(a,tlg_type,"TLG_TYPE", dbo::NotNull);
        dbo::field(a,typeb_tlg_id,"TYPEB_TLG_ID", dbo::NotNull);
        dbo::field(a,typeb_tlg_num,"TYPEB_TLG_NUM", dbo::NotNull);
    }
};

struct ARX_TLG_STAT : public TLG_STAT
{
    Dates::DateTime_t part_key;
    ARX_TLG_STAT() = default;
    ARX_TLG_STAT(const TLG_STAT& sr, const Dates::DateTime_t & part_key_)
        :TLG_STAT(sr), part_key(part_key_) {}

    template<typename Action>
    void persist(Action & a) {
        TLG_STAT::persist(a);
        dbo::field(a,part_key,"PART_KEY", dbo::NotNull);
    }
};

struct TLGS
{
    std::string error;
    int id;
    int postponed;
    std::string receiver;
    std::string sender;
    Dates::DateTime_t time;
    int tlg_num;
    std::string type;
    int typeb_tlg_id = ASTRA::NoExists;
    int typeb_tlg_num = ASTRA::NoExists;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,error,"ERROR");
        dbo::field(a,id,"ID", dbo::NotNull);
        dbo::field(a,postponed,"POSTPONED");
        dbo::field(a,receiver,"RECEIVER", dbo::NotNull);
        dbo::field(a,sender,"SENDER", dbo::NotNull);
        dbo::field(a,time,"TIME", dbo::NotNull);
        dbo::field(a,tlg_num,"TLG_NUM", dbo::NotNull);
        dbo::field(a,type,"TYPE", dbo::NotNull);
        dbo::field(a,typeb_tlg_id,"TYPEB_TLG_ID");
        dbo::field(a,typeb_tlg_num,"TYPEB_TLG_NUM");
    }
};

struct FILES
{
    std::string error;
    int id;
    std::string receiver;
    std::string sender;
    Dates::DateTime_t time;
    std::string type;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,error,"ERROR");
        dbo::field(a,id,"ID", dbo::NotNull);
        dbo::field(a,receiver,"RECEIVER", dbo::NotNull);
        dbo::field(a,sender,"SENDER", dbo::NotNull);
        dbo::field(a,time,"TIME", dbo::NotNull);
        dbo::field(a,type,"TYPE", dbo::NotNull);
    }
};

struct KIOSK_EVENTS
{
    std::string application;
    int ev_order;
    int id;
    std::string kioskid;
    std::string screen;
    std::string session_id;
    Dates::DateTime_t time;
    std::string type;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,application,"APPLICATION");
        dbo::field(a,ev_order,"EV_ORDER", dbo::NotNull);
        dbo::field(a,id,"ID", dbo::NotNull);
        dbo::field(a,kioskid,"KIOSKID");
        dbo::field(a,screen,"SCREEN");
        dbo::field(a,session_id,"SESSION_ID");
        dbo::field(a,time,"TIME", dbo::NotNull);
        dbo::field(a,type,"TYPE");
    }
};

struct AODB_SPP_FILES
{
    std::string airline;
    std::string filename;
    std::string point_addr;
    int rec_no;

    template<typename Action>
    void persist(Action & a) {
        dbo::field(a,airline,"AIRLINE", dbo::NotNull);
        dbo::field(a,filename,"FILENAME", dbo::NotNull);
        dbo::field(a,point_addr,"POINT_ADDR", dbo::NotNull);
        dbo::field(a,rec_no,"REC_NO", dbo::NotNull);
    }
};

}   //end namespace dbo


#endif // DBOSTRUCTURES_H
