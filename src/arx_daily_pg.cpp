//---------------------------------------------------------------------------
#include "arx_daily_pg.h"
#include "oralib.h"
#include "exceptions.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "qrys.h"
#include "stat/stat_main.h"
#include <variant>

#include <memory>
#include <sstream>
#include <optional>
#include "boost/date_time/local_time/local_time.hpp"

#include "pg_session.h"
#include <serverlib/pg_cursctl.h>
#include <serverlib/pg_rip.h>
#include <serverlib/cursctl.h>
#include <serverlib/testmode.h>
#include <serverlib/dates_oci.h>
#include <serverlib/dates_io.h>
#include <serverlib/tcl_utils.h>
#include "serverlib/oci_rowid.h"
#include "dbo.h"
#include "dbostructures.h"


#define NICKNAME "FELIX"
#define NICKTRACE FELIX_TRACE
#include <serverlib/slogger.h>

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

namespace  PG_ARX
{

std::vector<dbo::Points> arx_points(const MoveId_t & vmove_id, const Dates::DateTime_t& part_key);
void arx_move_ref(const MoveId_t & vmove_id, const Dates::DateTime_t& part_key);
void arx_move_ext(const MoveId_t & vmove_id, const Dates::DateTime_t& part_key, int date_range);
void arx_events_by_move_id(const MoveId_t & vmove_id, const Dates::DateTime_t& part_key);
//by point_id
void arx_events_by_point_id(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_mark_trips(const PointId_t& point_id, const Dates::DateTime_t& part_key);
std::vector<dbo::Pax_Grp> arx_pax_grp(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_self_ckin_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_rfisc_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_stat_services(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_stat_rem(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_limited_cap_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_pfs_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_stat_ad(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_stat_ha(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_stat_vo(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_stat_reprint(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_trfer_pax_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_bi_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_agent_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_trfer_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_tlg_out(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_trip_classes(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_trip_delays(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_trip_load(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_trip_sets(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_trip_crs_displace2(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_trip_stages(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_bag_receipts(const PointId_t& point_id, const Dates::DateTime_t& part_key);
void arx_bag_pay_types(const PointId_t& point_id, const Dates::DateTime_t& part_key);
//by grp_id
void arx_annul_bags_tags(const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
void arx_unaccomp_bag_info(const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
void arx_bag2(const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
void arx_bag_prepay(const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
void arx_bag_tags(const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
void arx_paid_bag(const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
std::vector<dbo::PAX> arx_pax(const PointId_t& point_id, const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
void arx_transfer(const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
void arx_tckin_segments(const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
void arx_value_bag(const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
void arx_grp_norms(const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
//by pax_id
void arx_pax_norms(const PaxId_t& pax_id, const Dates::DateTime_t& part_key);
void arx_pax_rem(const PaxId_t& pax_id, const Dates::DateTime_t& part_key);
void arx_transfer_subcls(const PaxId_t& pax_id, const Dates::DateTime_t& part_key);
void arx_pax_doc(const PaxId_t& pax_id, const Dates::DateTime_t& part_key);
void arx_pax_doco(const PaxId_t& pax_id, const Dates::DateTime_t& part_key);
void arx_pax_doca(const PaxId_t& pax_id, const Dates::DateTime_t& part_key);
void deleteByPaxes(const PaxId_t& pax_id);
void deleteByGrpId(const GrpId_t& grp_id);
void deleteAodbBag(const PointId_t& point_id);
void deleteByPointId(const PointId_t& point_id);
void deleteByMoveId(const MoveId_t & move_id);
//step2
int arx_tlgout_noflt(const Dates::DateTime_t& arx_date, int remain_rows);
int arx_events_noflt2(const Dates::DateTime_t& arx_date, int remain_rows);
int arx_events_noflt3(const Dates::DateTime_t& arx_date, int remain_rows);
int arx_stat_zamar(const Dates::DateTime_t& arx_date, int remain_rows);
void move_noflt(const Dates::DateTime_t& arx_date, int max_rows, int time_duration, int& step);
//step3
void arx_tlg_trip(const PointId_t& point_id);
//step 4
void move_typeb_in(int tlg_id);
//step 5
void norms_rates_etc(const Dates::DateTime_t &arx_date, int max_rows, int time_duration, int& step);
int arx_bag_norms(const Dates::DateTime_t &arx_date, int remain_rows);
int arx_bag_rates(const Dates::DateTime_t &arx_date, int remain_rows);
int arx_value_bag_taxes(const Dates::DateTime_t &arx_date, int remain_rows);
int arx_exchange_rates(const Dates::DateTime_t &arx_date, int remain_rows);
int delete_from_mark_trips(const Dates::DateTime_t &arx_date, int remain_rows);
//step 6
void tlgs_files_etc(const Dates::DateTime_t& arx_date, int max_rows, int time_duration, int& step);
int arx_tlgs(const Dates::DateTime_t& arx_date, int remain_rows);
int delete_files(const Dates::DateTime_t& arx_date, int remain_rows);
int delete_kiosk_events(const Dates::DateTime_t& arx_date, int remain_rows);
int delete_rozysk(const Dates::DateTime_t& arx_date, int remain_rows);
int delete_aodb_spp_files(const Dates::DateTime_t& arx_date, int remain_rows);
int delete_eticks_display(const Dates::DateTime_t& arx_date, int remain_rows);
int delete_eticks_display_tlgs(const Dates::DateTime_t& arx_date, int remain_rows);
int delete_emdocs_display(const Dates::DateTime_t& arx_date, int remain_rows);



namespace salons {

std::string get_seat_no(const PaxId_t& pax_id, int seats, int is_jmp, std::string status, const PointId_t& point_id,
                        std::string fmt, int row = 1, int only_lat = 0) {
    char result[41] = {};
    short null = -1, nnull = 0; //-1 - NULL , 0 - value
    auto cur = make_curs(
                "BEGIN \n"
                "   :result := salons.get_seat_no(:pax_id, :seats, :is_jmp, :status, :point_id, :fmt, :rownum, :only_lat); \n"
                "END;");
    cur.bind(":pax_id", pax_id )
            .bind(":seats", seats)
            .bind(":is_jmp", is_jmp)
            .bind(":status", status, status.empty() ? &null : &nnull)
            .bind(":point_id", point_id)
            .bind(":fmt", fmt)
            .bind(":rownum", row)
            .bind(":only_lat", only_lat)
            .bindOutNull(":result", result, "")
            .exec();
    return std::string(result);
}

}

namespace ckin {

int get_excess_pc(const GrpId_t& grp_id, const PaxId_t& pax_id, int include_all_svc = 0)
{
    int excess_pc = 0;
    auto cur = make_curs(
                "BEGIN \n"
                "   :excess := ckin.get_excess_pc(:grp_id, :pax_id, :inc_svc); \n"
                "END;");
    cur.bind(":grp_id", grp_id)
            .bind(":pax_id", pax_id)
            .bind(":inc_svc", include_all_svc)
            .bindOutNull(":excess", excess_pc, ASTRA::NoExists)
            .exec();
    return excess_pc;
}


void delete_typeb_data(const PointId_t& point_id)
{
    auto cur = make_curs(
                "BEGIN \n"
                "   ckin.delete_typeb_data(:point_id, NULL, NULL, FALSE); \n"
                "END;");
    cur.bind(":point_id", point_id)
            .exec();
}

}

bool ARX_PG_ENABLE()
{
    static int always=-1;
    return getVariableStaticBool("ARX_PG_ENABLE", &always, 0);
}

int ARX_DAYS()
{
    static int VAR=NoExists;
    if (VAR==NoExists)
        VAR=getTCLParam("ARX_DAYS",15,NoExists,NoExists);
    return VAR;
}

int ARX_DURATION()
{
    static int VAR=NoExists;
    if (VAR==NoExists)
        VAR=getTCLParam("ARX_DURATION",1,60,15);
    return VAR;
};

int ARX_SLEEP()
{
    static int VAR=NoExists;
    if (VAR==NoExists)
        VAR=getTCLParam("ARX_SLEEP",1,NoExists,60);
    return VAR;
};

int ARX_MAX_ROWS()
{
    static int VAR=NoExists;
    if (VAR==NoExists)
        VAR=getTCLParam("ARX_MAX_ROWS",100,NoExists,1000);
    return VAR;
};

//============================= TArxMove =============================

TArxMove::TArxMove(const Dates::DateTime_t &utc_date)
{
    proc_count=0;
    utcdate = utc_date;
};

TArxMove::~TArxMove()
{
};

//============================= TArxMoveFlt =============================
//STEP 1
TArxMoveFlt::TArxMoveFlt(const Dates::DateTime_t& utc_date):TArxMove(utc_date)
{
    step=0;
    move_ids_count=0;
};

TArxMoveFlt::~TArxMoveFlt()
{
};

bool TArxMoveFlt::GetPartKey(const MoveId_t &move_id, Dates::DateTime_t &part_key, double &date_range)
{
    LogTrace(TRACE5) << __FUNCTION__ << " move_id: " << move_id << " part_key: " << part_key;
    //part_key=NoExists;
    date_range=NoExists;

    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::Points> points = session.query<dbo::Points>().where(" MOVE_ID = :move_id ORDER BY point_num ")
            .setBind({{":move_id", move_id}});

    Dates::DateTime_t first_date;
    Dates::DateTime_t last_date;
    bool deleted=true;

    for(int i = 0; i<points.size(); i++)
    {
        dbo::Points & p = points[i];
        if (p.pr_del!= -1) {
            deleted=false;
        }
        std::vector<Dates::DateTime_t> temp;
        if(i%2 == 0) {
            temp = {first_date,last_date, p.scd_out, p.est_out, p.act_out};
        } else {
            temp = {first_date,last_date, p.scd_in, p.est_in, p.act_in};
        }
        LogTrace(TRACE5) << " temp size: " << temp.size();
        for(const auto & t : temp) LogTrace(TRACE5) << " TEMP: " << t;
        auto fdates = algo::filter(temp, dbo::isNotNull<Dates::DateTime_t>);
        for(const auto & t : fdates) LogTrace(TRACE5) << " FILTER: " << t;
        first_date = (*std::min_element(fdates.begin(), fdates.end()));
        last_date = (*std::max_element(fdates.begin(), fdates.end()));
        LogTrace(TRACE5) << " myfirst_date: " << first_date << " mylast_date: " << last_date;
    }

    if (!deleted)
    {
        if (first_date!=Dates::not_a_date_time && last_date!=Dates::not_a_date_time)
        {
            if (last_date < utcdate - Dates::days(ARX_DAYS()))
            {
                //переместить в архив
                part_key = last_date;
                date_range = BoostToDateTime(last_date) - BoostToDateTime(first_date);
                return true;
            };
        };
    }
    else
    {
        //полностью удаленный рейс
        if (last_date == Dates::not_a_date_time || last_date < utcdate-Dates::days(ARX_DAYS()))
        {
            //удалить
            part_key = Dates::not_a_date_time;
            date_range = NoExists;
            return true;
        };
    };
    return false;
}

void TArxMoveFlt::LockAndCollectStat(const MoveId_t & move_id)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::Points> points = session.query<dbo::Points>()
            .where(" MOVE_ID = :move_id ").setBind({{":move_id",move_id} });
    for(const auto & p : points) {
        bool pr_reg = p.pr_reg != 0;
        if(p.pr_del < 0) continue;
        if (p.pr_del==0 && pr_reg) get_flight_stat(p.point_id, true);
        TReqInfo::Instance()->LocaleToLog("EVT.FLIGHT_MOOVED_TO_ARCHIVE", evtFlt, p.point_id);
    }
};


void TArxMoveFlt::readMoveIds(size_t max_rows)
{
    int move_id;
    Dates::DateTime_t part_key;
    double date_range;
    auto cur = make_curs("SELECT move_id FROM points "
                         "WHERE (time_in > TO_DATE('01.01.0001','DD.MM.YYYY') AND time_in<:arx_date)"
                         " OR (time_out > TO_DATE('01.01.0001','DD.MM.YYYY') AND time_out<:arx_date)"
                         " OR (time_in  = TO_DATE('01.01.0001','DD.MM.YYYY') AND "
                         "      time_out = TO_DATE('01.01.0001','DD.MM.YYYY'))");
    cur
            .def(move_id)
            .bind(":arx_date", utcdate-Dates::days(ARX_DAYS()))
            .exec();

    while(!cur.fen() && (move_ids.size() < max_rows)) {
        if (GetPartKey(MoveId_t(move_id), part_key,date_range))
        {
            move_ids.try_emplace(MoveId_t(move_id), part_key);
        }
    }
}

bool TArxMoveFlt::Next(size_t max_rows, int duration)
{
    readMoveIds(max_rows);
    while (!move_ids.empty())
    {
        //LogTrace(TRACE5) << "MOVE_IDS count: " << move_ids.size();
        MoveId_t move_id = move_ids.begin()->first;
        move_ids.erase(move_ids.begin());
        move_ids_count--;
        Dates::DateTime_t part_key;
        double date_range;

        if (GetPartKey(move_id, part_key, date_range))
        {
            try
            {
                int date_range_int = 0;
                if (date_range != NoExists)
                {
                    if (date_range<0) throw Exception("date_range=%f", date_range);
                    if (date_range<1)
                    {
                        LogTrace(TRACE5) << " date_range < 1 :" << date_range;
                    }
                    else
                    {
                        date_range_int=(int)ceil(date_range);
                        LogTrace(TRACE5) << " date_range_int = " << date_range_int;
                        if (date_range_int>999) throw Exception("date_range_int=%d", date_range_int);
                    };
                } else {
                    LogTrace(TRACE5) << " date_range = " << NoExists;
                }

                LockAndCollectStat(move_id);
                auto points =  arx_points(move_id, part_key);
                if(points.size() > 0) {
                    //LogTrace(TRACE5) << " OUT_DATE_RANGE = " << date_range << " range_int: " << date_range_int;
                    if(date_range >= 1) {
                        arx_move_ext(move_id, part_key, date_range_int);
                    }
                    arx_move_ref(move_id, part_key);
                    arx_events_by_move_id(move_id, part_key);
                    for(const auto &p : points)
                    {
                        PointId_t point_id(p.point_id);
                        if(p.pr_del != -1)
                        {
                            //LogTrace(TRACE5) << "P.PR_DEL = " << p.pr_del << " P.POINT_ID = " << p.point_id << " move_id: " << move_id;
                            arx_events_by_point_id(point_id, part_key);
                            arx_mark_trips(point_id, part_key);
                            auto pax_grps = arx_pax_grp(point_id, part_key);
                            arx_self_ckin_stat(point_id, part_key);
                            arx_rfisc_stat(point_id, part_key);
                            arx_stat_services(point_id, part_key);
                            arx_stat_rem(point_id, part_key);
                            arx_pfs_stat(point_id, part_key);
                            arx_stat_ad(point_id, part_key);
                            arx_stat_ha(point_id, part_key);
                            arx_stat_vo(point_id, part_key);
                            arx_stat_reprint(point_id, part_key);
                            arx_trfer_pax_stat(point_id, part_key);
                            arx_bi_stat(point_id, part_key);
                            arx_agent_stat(point_id, part_key);
                            arx_stat(point_id, part_key);
                            arx_trfer_stat(point_id, part_key);
                            arx_tlg_out(point_id, part_key);

                            arx_trip_classes(point_id,  part_key);
                            arx_trip_delays(point_id,  part_key);
                            arx_trip_load(point_id,  part_key);
                            arx_trip_sets(point_id,  part_key);
                            arx_trip_crs_displace2(point_id,  part_key);
                            arx_trip_stages(point_id, part_key);
                            arx_bag_receipts(point_id, part_key);
                            arx_bag_pay_types(point_id, part_key);
                            for(const auto &grp : pax_grps)
                            {
                                GrpId_t grp_id(grp.m_grp_id);
                                //LogTrace(TRACE5) << " PAX_GRP_ID : " << grp_id;
                                arx_annul_bags_tags(grp_id, part_key);
                                arx_unaccomp_bag_info(grp_id, part_key);
                                arx_bag2(grp_id, part_key);
                                arx_bag_prepay(grp_id, part_key);
                                arx_bag_tags(grp_id, part_key);
                                arx_paid_bag(grp_id, part_key);
                                auto paxes = arx_pax(point_id, grp_id, part_key);
                                arx_transfer(grp_id, part_key);
                                arx_tckin_segments(grp_id, part_key);
                                arx_value_bag(grp_id, part_key);
                                arx_grp_norms(grp_id, part_key);
                                for(const auto & pax : paxes) {
                                    PaxId_t pax_id(pax.pax_id);
                                    //LogTrace(TRACE5) << " PAX_ID : " << pax_id;
                                    arx_pax_norms(pax_id, part_key);
                                    arx_pax_rem(pax_id, part_key);
                                    arx_transfer_subcls(pax_id, part_key);
                                    arx_pax_doc(pax_id, part_key);
                                    arx_pax_doco(pax_id, part_key);
                                    arx_pax_doca(pax_id, part_key);
                                    //deleteByPaxes(pax_id);
                                }
                                //deleteByGrpId(grp_id);
                            }
                            //deleteByPointId(p.point_id);
                        }
                    }
                    //TODO
                    //deleteByMoveId(move_id);
                }
                ASTRA::commit();
                proc_count++;
            }
            catch(...)
            {
                if (part_key != Dates::not_a_date_time)
                    ProgError( STDLOG, "move_id=%d, part_key=%s", move_id.get(), HelpCpp::string_cast(part_key, "dd.mm.yy").c_str() );
                else
                    ProgError( STDLOG, "move_id=%d, part_key=not_a_date_time", move_id.get() );
                throw;
            };
        };
    };
    return false;
};

string TArxMoveFlt::TraceCaption()
{
    return "TArxMoveFlt";
};

void arx_move_ext(const MoveId_t & vmove_id, const Dates::DateTime_t& part_key, int date_range)
{
    LogTrace(TRACE5) << __FUNCTION__ << " move_id: " << vmove_id << " date_range: " << date_range;
    if(date_range > 0) {
        auto & session = dbo::Session::getInstance();
        session.connectPostgres();
        dbo::Move_Arx_Ext ext{date_range, vmove_id.get(), part_key};
        session.insert(ext);
    }
}

void arx_move_ref(const MoveId_t & vmove_id, const Dates::DateTime_t& part_key)
{
    LogTrace(TRACE5) << __FUNCTION__ << " move_id: " << vmove_id;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::Move_Ref> move_refs = session.query<dbo::Move_Ref>()
            .where(" MOVE_ID = :move_id").setBind({{"move_id",vmove_id} });
    session.connectPostgres();
    for(const auto & mr : move_refs) {
        dbo::Arx_Move_Ref amr(mr,part_key);
        session.insert(amr);
    }
}

std::vector<dbo::Points> arx_points(const MoveId_t & vmove_id, const Dates::DateTime_t& part_key)
{
    //LogTrace(TRACE5) << __FUNCTION__ << " vmove_id : "<< vmove_id;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::Points> points = session.query<dbo::Points>()
            .where(" MOVE_ID = :move_id and PR_DEL<>-1 FOR UPDATE").setBind({{"move_id",vmove_id} });

    //    auto cur2 = make_curs("DELETE FROM points WHERE move_id=:vmove_id");
    //    cur2.bind(":vmove_id", vmove_id).exec();

    session.connectPostgres();
    for(const auto &p : points) {
        dbo::Arx_Points ap(p, part_key);
        session.insert(ap);
    }
    return points;
}

void arx_events_by_move_id(const MoveId_t & vmove_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::Lang_Types> langs = session.query<dbo::Lang_Types>();
    for(const auto & lang : langs) {
        session.connectOracle();
        std::vector<dbo::Events_Bilingual> events =  session.query<dbo::Events_Bilingual>()
                .where(" id1 = :move_id and lang = :l and type = :evtDisp FOR UPDATE")
                .setBind({{"move_id",vmove_id}, {"l",lang.code}, {"evtDisp", EncodeEventType(evtDisp)}});
        session.connectPostgres();
        for(const auto & ev : events) {
            dbo::Arx_Events aev(ev,part_key);
            session.insert(aev);
        }
    }
}

void arx_events_by_point_id(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::Lang_Types> langs = session.query<dbo::Lang_Types>();
    for(const auto & lang : langs) {
        session.connectOracle();
        std::vector<dbo::Events_Bilingual> events =  session.query<dbo::Events_Bilingual>()
                .where(" id1 = :point_id and lang = :l and type in "
                       " (:evtFlt, :evtGraph, :evtFltTask, :evtPax, :evtPay, :evtTlg, :evtPrn) FOR UPDATE")
                .setBind({{"l",lang.code}, {"point_id", point_id},
                          {"evtFlt",     EncodeEventType(evtFlt)},
                          {"evtGraph",   EncodeEventType(evtGraph)},
                          {"evtFltTask", EncodeEventType(evtFltTask)},
                          {"evtPax",     EncodeEventType(evtPax)},
                          {"evtPay",     EncodeEventType(evtPay)},
                          {"evtTlg",     EncodeEventType(evtTlg)},
                          {"evtPrn",     EncodeEventType(evtPrn)},
                         });
        session.connectPostgres();
        for(const auto & ev : events) {
            dbo::Arx_Events aev(ev,part_key);
            session.insert(aev);
        }
    }
}

void arx_mark_trips(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<int> pax_grp_id_marks = session.query<int>("SELECT DISTINCT point_id_mark").from("pax_grp")
            .where("point_dep = :point_id").setBind({{"point_id", point_id}});
    std::vector<dbo::Mark_Trips> mark_trips;
    for(const auto &id_mark : pax_grp_id_marks) {
        std::optional<dbo::Mark_Trips> trip = session.query<dbo::Mark_Trips>()
                .where("point_id = :id_mark").setBind({{"id_mark",id_mark}});
        if(trip) {
            mark_trips.push_back(*trip);
        }
    }
    session.connectPostgres();
    for(const auto & mark_trip : mark_trips) {
        dbo::Arx_Mark_Trips amt(mark_trip,part_key);
        session.noThrowError(PgCpp::ConstraintFail).insert(amt);
    }
}

std::vector<dbo::Pax_Grp> arx_pax_grp(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    //LogTrace(TRACE3) << __FUNCTION__ << " point_id: " << point_id;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::Pax_Grp> pax_grps = session.query<dbo::Pax_Grp>()
            .where("point_dep = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &gr : pax_grps) {
        dbo::Arx_Pax_Grp apg(gr,part_key);
        session.insert(apg);
    }
    return pax_grps;
}

void arx_self_ckin_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::Self_Ckin_Stat> ckin_stats = session.query<dbo::Self_Ckin_Stat>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : ckin_stats) {
        dbo::Arx_Self_Ckin_Stat ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_rfisc_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::RFISC_STAT> rfisc_stats = session.query<dbo::RFISC_STAT>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : rfisc_stats) {
        dbo::ARX_RFISC_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_services(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::STAT_SERVICES> stat_services = session.query<dbo::STAT_SERVICES>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stat_services) {
        dbo::ARX_STAT_SERVICES ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_rem(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::STAT_REM> stat_rems = session.query<dbo::STAT_REM>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stat_rems) {
        dbo::ARX_STAT_REM ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_limited_cap_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::LIMITED_CAPABILITY_STAT> stat_lcs = session.query<dbo::LIMITED_CAPABILITY_STAT>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stat_lcs) {
        dbo::ARX_LIMITED_CAPABILITY_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_pfs_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::PFS_STAT> stat_pfs = session.query<dbo::PFS_STAT>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stat_pfs) {
        dbo::ARX_PFS_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_ad(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::STAT_AD> stat_ad = session.query<dbo::STAT_AD>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stat_ad) {
        dbo::ARX_STAT_AD ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_ha(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::STAT_HA> stat_ha = session.query<dbo::STAT_HA>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stat_ha) {
        dbo::ARX_STAT_HA ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_vo(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::STAT_VO> stat_vo = session.query<dbo::STAT_VO>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stat_vo) {
        dbo::ARX_STAT_VO ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_reprint(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::STAT_REPRINT> stat_reprint = session.query<dbo::STAT_REPRINT>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stat_reprint) {
        dbo::ARX_STAT_REPRINT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_trfer_pax_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::TRFER_PAX_STAT> stat_reprint = session.query<dbo::TRFER_PAX_STAT>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stat_reprint) {
        dbo::ARX_TRFER_PAX_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_bi_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::BI_STAT> bi_stats = session.query<dbo::BI_STAT>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : bi_stats) {
        dbo::ARX_BI_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}



void arx_agent_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto agents_stat = dbo::readOraAgentsStat(point_id);

    auto & session = dbo::Session::getInstance();
    //    session.connectOracle();
    //    std::vector<dbo::AGENT_STAT> bi_stats = session.query<dbo::AGENT_STAT>()
    //            .where("point_id = :point_id").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &as : agents_stat) {
        dbo::ARX_AGENT_STAT ascs(as,part_key);
        ascs.part_key = ascs.ondate;
        ascs.point_part_key =part_key;
        session.insert(ascs);
    }
}

void arx_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::STAT> stats = session.query<dbo::STAT>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stats) {
        dbo::ARX_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_trfer_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::TRFER_STAT> stats = session.query<dbo::TRFER_STAT>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stats) {
        dbo::ARX_TRFER_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_tlg_out(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::TLG_OUT> stats = session.query<dbo::TLG_OUT>()
            .where("point_id = :point_id AND type<>'LCI' FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stats) {
        Dates::DateTime_t part_key_nvl;
        if(cs.time_send_act.is_not_a_date_time()) {
            if(cs.time_send_scd.is_not_a_date_time()) {
                part_key_nvl = cs.time_create;
            } else {
                part_key_nvl = cs.time_send_scd;
            }
        } else {
            part_key_nvl = cs.time_send_act;
        }
        //NVL(time_send_act,NVL(time_send_scd,time_create)
        dbo::ARX_TLG_OUT ascs(cs, part_key_nvl);
        session.insert(ascs);
    }
}

void arx_trip_classes(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::TRIP_CLASSES> stats = session.query<dbo::TRIP_CLASSES>()
            .where("point_id = :point_id").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stats) {
        dbo::ARX_TRIP_CLASSES ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_trip_delays(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::TRIP_DELAYS> stats = session.query<dbo::TRIP_DELAYS>()
            .where("point_id = :point_id").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stats) {
        dbo::ARX_TRIP_DELAYS ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_trip_load(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::TRIP_LOAD> stats = session.query<dbo::TRIP_LOAD>()
            .where("point_dep = :point_id").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stats) {
        dbo::ARX_TRIP_LOAD ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_trip_sets(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::TRIP_SETS> trip_sets = session.query<dbo::TRIP_SETS>()
            .where("point_id = :point_id").setBind({{"point_id", point_id}});


    session.connectPostgres();
    for(const auto &ts : trip_sets) {
        dbo::ARX_TRIP_SETS ats;
        ats.point_id = ts.point_id;
        ats.max_commerce = ts.max_commerce;
        ats.comp_id = ts.comp_id;
        ats.pr_etstatus = ts.pr_etstatus;
        ats.pr_stat = ts.pr_stat;
        ats.pr_tranz_reg = ts.pr_tranz_reg;
        ats.f = ts.f;
        ats.c = ts.c;
        ats.y = ts.y;
        ats.part_key =part_key;
        session.insert(ats);
    }
}

void arx_trip_crs_displace2(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::CRS_DISPLACE2> stats = session.query<dbo::CRS_DISPLACE2>()
            .where("point_id_spp = :point_id").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stats) {
        dbo::ARX_CRS_DISPLACE2 ascs(cs,part_key);
        ascs.point_id_tlg = ASTRA::NoExists;
        session.insert(ascs);
    }
}

void arx_trip_stages(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::TRIP_STAGES> stats = session.query<dbo::TRIP_STAGES>()
            .where("point_id = :point_id FOR UPDATE").setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &cs : stats) {
        dbo::ARX_TRIP_STAGES ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_bag_receipts(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    LogTrace1 <<__FUNCTION__ << " point_id: " << point_id;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::BAG_RECEIPTS> bag_receipts = session.query<dbo::BAG_RECEIPTS>()
            .from("BAG_RECEIPTS, BAG_RCPT_KITS")
            .where("bag_receipts.kit_id = bag_rcpt_kits.kit_id(+) AND "
                   "bag_receipts.kit_num = bag_rcpt_kits.kit_num(+) AND "
                   "bag_receipts.point_id = :point_id FOR UPDATE")
            .setBind({{"point_id", point_id}});

    session.connectPostgres();
    for(const auto &br : bag_receipts) {
        dbo::ARX_BAG_RECEIPTS abr(br,part_key);
        session.insert(abr);
    }
}

void arx_bag_pay_types(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    LogTrace1 <<__FUNCTION__ << " point_id: " << point_id;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::BAG_PAY_TYPES> bag_receipts = session.query<dbo::BAG_PAY_TYPES>()
            .from("BAG_RECEIPTS, BAG_RCPT_KITS, BAG_PAY_TYPES")
            .where("bag_receipts.kit_id = bag_rcpt_kits.kit_id(+) AND "
                   "bag_receipts.kit_num = bag_rcpt_kits.kit_num(+) AND "
                   "bag_receipts.point_id = :point_id AND "
                   "bag_receipts.receipt_id = bag_pay_types.receipt_id FOR UPDATE")
            .setBind({{"point_id", point_id}});

    session.connectPostgres();

    for(const auto &bpt : bag_receipts) {
        dbo::ARX_BAG_PAY_TYPES abpt(bpt,part_key);
        session.insert(abpt);
    }
}

void arx_annul_bags_tags(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::ANNUL_BAG> annul_bags = session.query<dbo::ANNUL_BAG>()
            .where("grp_id = :grp_id FOR UPDATE").setBind({{"grp_id", grp_id}});
    std::vector<dbo::ANNUL_TAGS> annul_tags = session.query<dbo::ANNUL_TAGS>()
            .where("id = :grp_id FOR UPDATE").setBind({{"grp_id", grp_id}});

    session.connectPostgres();
    for(const auto &cs : annul_bags) {
        dbo::ARX_ANNUL_BAG ascs(cs,part_key);
        session.insert(ascs);
    }
    for(const auto &cs : annul_tags) {
        dbo::ARX_ANNUL_TAGS ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_unaccomp_bag_info(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::UNACCOMP_BAG_INFO> annul_bags = session.query<dbo::UNACCOMP_BAG_INFO>()
            .where("grp_id = :grp_id FOR UPDATE").setBind({{"grp_id", grp_id}});

    session.connectPostgres();
    for(const auto &cs : annul_bags) {
        dbo::ARX_UNACCOMP_BAG_INFO ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_bag2(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::BAG2> bags2 = session.query<dbo::BAG2>()
            .where("grp_id = :grp_id FOR UPDATE").setBind({{"grp_id", grp_id}});

    session.connectPostgres();
    for(const auto &cs : bags2) {
        dbo::ARX_BAG2 ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_bag_prepay(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::BAG_PREPAY> bags2 = session.query<dbo::BAG_PREPAY>()
            .where("grp_id = :grp_id FOR UPDATE").setBind({{"grp_id", grp_id}});

    session.connectPostgres();
    for(const auto &cs : bags2) {
        dbo::ARX_BAG_PREPAY ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_bag_tags(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::BAG_TAGS> bags2 = session.query<dbo::BAG_TAGS>()
            .where("grp_id = :grp_id FOR UPDATE").setBind({{"grp_id", grp_id}});

    session.connectPostgres();
    for(const auto &cs : bags2) {
        dbo::ARX_BAG_TAGS ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_paid_bag(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::PAID_BAG> bags2 = session.query<dbo::PAID_BAG>()
            .where("grp_id = :grp_id FOR UPDATE").setBind({{"grp_id", grp_id}});

    session.connectPostgres();
    for(const auto &cs : bags2) {
        dbo::ARX_PAID_BAG ascs(cs,part_key);
        session.insert(ascs);
    }
}

std::vector<dbo::PAX> arx_pax(const PointId_t& point_id, const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::PAX> paxes = session.query<dbo::PAX>()
            .where("grp_id = :grp_id FOR UPDATE").setBind({{"grp_id", grp_id}});

    session.connectPostgres();
    for(const auto &cs : paxes) {
        std::string seat_no = salons::get_seat_no(PaxId_t(cs.pax_id), cs.seats, cs.is_jmp, "", point_id, "one" );
        int excess_pc = ckin::get_excess_pc(grp_id, PaxId_t(cs.pax_id));
        dbo::ARX_PAX ascs(cs, part_key, excess_pc, seat_no);
        session.insert(ascs);
    }
    return paxes;
}

void arx_transfer(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::TRANSFER> trfer = session.query<dbo::TRANSFER>()
            .where("grp_id = :grp_id AND transfer_num > 0 FOR UPDATE").setBind({{":grp_id", grp_id}});
    for(const auto& tr : trfer) {
        session.connectOracle();
        std::optional<dbo::TRFER_TRIPS> trip = session.query<dbo::TRFER_TRIPS>().where("point_id = :tp FOR UPDATE")
                .setBind({{":tp", tr.point_id_trfer}});

        if(trip){
            session.connectPostgres();
            dbo::ARX_TRANSFER atr(tr, *trip,part_key);
            session.insert(atr);
            //delete
            //            session.connectOracle();
            //            auto cur = make_curs("DELETE FROM trfer_trips where point_id = :tp");
            //            cur.bind(":tp", tr.point_id_trfer);
            //            cur.exec();
        }
    }
}

void arx_tckin_segments(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    LogTrace(TRACE3) << __FUNCTION__ << " grp_id: " << grp_id;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::TCKIN_SEGMENTS> segs = session.query<dbo::TCKIN_SEGMENTS>()
            .where("grp_id = :grp_id and seg_no > 0 FOR UPDATE").setBind({{":grp_id", grp_id}});
    //LogTrace(TRACE3) << " segs size : " << segs.size();

    for(const auto& s : segs) {
        static int added = 0;
        //LogTrace(TRACE3) << " get transfer trips";
        session.connectOracle();
        std::optional<dbo::TRFER_TRIPS> trip = session.query<dbo::TRFER_TRIPS>().where("point_id = :tp FOR UPDATE")
                .setBind({{":tp", s.point_id_trfer}});

        if(trip) {
            session.connectPostgres();
            dbo::ARX_TCKIN_SEGMENTS atr(s, *trip,part_key);
            session.insert(atr);
            added ++;
            //LogTrace(TRACE3) << " added = " << added;

            //            session.connectOracle();
            //            auto cur = make_curs("DELETE FROM tckin_segments where point_id_trfer = :tp");
            //            cur.bind(":tp", s.point_id_trfer);
            //            cur.exec();
        }
    }
}

void arx_value_bag(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::VALUE_BAG> bags2 = session.query<dbo::VALUE_BAG>()
            .where("grp_id = :grp_id FOR UPDATE").setBind({{"grp_id", grp_id}});

    session.connectPostgres();
    for(const auto &cs : bags2) {
        dbo::ARX_VALUE_BAG ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_grp_norms(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::GRP_NORMS> norms = session.query<dbo::GRP_NORMS>()
            .where("grp_id = :grp_id FOR UPDATE").setBind({{"grp_id", grp_id}});

    session.connectPostgres();
    for(const auto &cs : norms) {
        dbo::ARX_GRP_NORMS ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_pax_norms(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::PAX_NORMS> norms = session.query<dbo::PAX_NORMS>()
            .where("pax_id = :pax_id FOR UPDATE").setBind({{"pax_id", pax_id}});

    session.connectPostgres();
    for(const auto &cs : norms) {
        dbo::ARX_PAX_NORMS ascs(cs,part_key);
        session.insert(ascs);
    }
}


void arx_pax_rem(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::PAX_REM> norms = session.query<dbo::PAX_REM>()
            .where("pax_id = :pax_id FOR UPDATE").setBind({{"pax_id", pax_id}});

    session.connectPostgres();
    for(const auto &cs : norms) {
        dbo::ARX_PAX_REM ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_transfer_subcls(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::TRANSFER_SUBCLS> norms = session.query<dbo::TRANSFER_SUBCLS>()
            .where("pax_id = :pax_id FOR UPDATE").setBind({{"pax_id", pax_id}});

    session.connectPostgres();
    for(const auto &cs : norms) {
        dbo::ARX_TRANSFER_SUBCLS ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_pax_doc(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    LogTrace(TRACE5) << __FUNCTION__ << " pax_id:" << pax_id;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::PAX_DOC> docs = session.query<dbo::PAX_DOC>()
            .where("pax_id = :pax_id FOR UPDATE").setBind({{"pax_id", pax_id}});
    LogTrace(TRACE5) << " docs found : " << docs.size();

    session.connectPostgres();
    for(const auto &d : docs) {
        dbo::ARX_PAX_DOC ad(d,part_key);
        session.insert(ad);
    }
}

void arx_pax_doco(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::PAX_DOCO> norms = session.query<dbo::PAX_DOCO>()
            .where("pax_id = :pax_id FOR UPDATE").setBind({{"pax_id", pax_id}});

    session.connectPostgres();
    for(const auto &cs : norms) {
        dbo::ARX_PAX_DOCO ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_pax_doca(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::PAX_DOCA> norms = session.query<dbo::PAX_DOCA>()
            .where("pax_id = :pax_id FOR UPDATE").setBind({{"pax_id", pax_id}});

    session.connectPostgres();
    for(const auto &cs : norms) {
        dbo::ARX_PAX_DOCA ascs(cs,part_key);
        session.insert(ascs);
    }
}

void deleteByPaxes(const PaxId_t& pax_id)
{
    auto cur = make_curs(
                "BEGIN "
                "DELETE FROM pax_events WHERE pax_id=:pax_id; "
                "DELETE FROM stat_ad WHERE pax_id=:pax_id; "
                "DELETE FROM confirm_print WHERE pax_id=:pax_id; "
                "DELETE FROM pax_fqt WHERE pax_id=:pax_id; "
                "DELETE FROM pax_asvc WHERE pax_id=:pax_id; "
                "DELETE FROM pax_emd WHERE pax_id=:pax_id; "
                "DELETE FROM pax_brands WHERE pax_id=:pax_id; "
                "DELETE FROM pax_rem_origin WHERE pax_id=:pax_id; "
                "DELETE FROM pax_seats WHERE pax_id=:pax_id; "
                "DELETE FROM rozysk WHERE pax_id=:pax_id; "
                "DELETE FROM trip_comp_layers WHERE pax_id=:pax_id; "
                "UPDATE service_payment SET pax_id=NULL WHERE pax_id=:pax_id; "
                "DELETE FROM pax_alarms WHERE pax_id=:pax_id; "
                "DELETE FROM pax_custom_alarms WHERE pax_id=:pax_id; "
                "DELETE FROM pax_service_lists WHERE pax_id=:pax_id; "
                "DELETE FROM pax_services WHERE pax_id=:pax_id; "
                "DELETE FROM pax_services_auto WHERE pax_id=:pax_id; "
                "DELETE FROM paid_rfisc WHERE pax_id=:pax_id; "
                "DELETE FROM pax_norms_text WHERE pax_id=:pax_id; "
                "DELETE FROM sbdo_tags_generated WHERE pax_id=:pax_id; "
                "DELETE FROM pax_calc_data WHERE pax_calc_data_id=:pax_id; "
                "DELETE FROM pax_confirmations WHERE pax_id=:pax_id; END;");
    cur.bind(":pax_id", pax_id);
    cur.exec();
    //todo delete from pax
}

void deleteByGrpId(const GrpId_t& grp_id)
{
    auto cur = make_curs(
                "BEGIN "
                "DELETE FROM paid_bag_emd_props WHERE grp_id=:grp_id; "
                "DELETE FROM service_payment WHERE grp_id=:grp_id; "
                "DELETE FROM tckin_pax_grp WHERE grp_id=:grp_id; "
                "DELETE FROM pnr_addrs_pc WHERE grp_id=:grp_id; "
                "DELETE FROM grp_service_lists WHERE grp_id=:grp_id; "
                "DELETE FROM bag_tags_generated WHERE grp_id=:grp_id; END;");
    cur.bind(":grp_id", grp_id);
    cur.exec();
    //todo delete from pax_grp
}

void deleteAodbBag(const PointId_t& point_id)
{
    auto cur = make_curs(
                "BEGIN "
                "delete from AODB_BAG "
                "where exists (select PAX_ID, POINT_ADDR from AODB_PAX where AODB_PAX.POINT_ID=:point_id and "
                " AODB_BAG.PAX_ID = AODB_pax_id and AODB_BAG.POINT_ADDR = AODB_PAX.POINT_ADDR); END;");
    cur.bind(":point_id", point_id);
    cur.exec();
}

void deleteByPointId(const PointId_t& point_id)
{
    deleteAodbBag(point_id);
    auto cur = make_curs("BEGIN "
                         " DELETE FROM aodb_pax_change WHERE point_id=:point_id;         "
                         " DELETE FROM aodb_unaccomp WHERE point_id=:point_id;           "
                         " DELETE FROM aodb_pax WHERE point_id=:point_id;                "
                         " DELETE FROM aodb_points WHERE point_id=:point_id;             "
                         " DELETE FROM exch_flights WHERE point_id=:point_id;            "
                         " DELETE FROM counters2 WHERE point_dep=:point_id;              "
                         " DELETE FROM crs_counters WHERE point_dep=:point_id;           "
                         " DELETE FROM crs_displace2 WHERE point_id_spp=:point_id;       "
                         " DELETE FROM snapshot_points WHERE point_id=:point_id;         "
                         " UPDATE tag_ranges2 SET point_id=NULL WHERE point_id=:point_id;"
                         " DELETE FROM tlg_binding WHERE point_id_spp=:point_id;         "
                         " DELETE FROM trip_bp WHERE point_id=:point_id;                 "
                         " DELETE FROM trip_hall WHERE point_id=:point_id;               "
                         " DELETE FROM trip_bt WHERE point_id=:point_id;                 "
                         " DELETE FROM trip_ckin_client WHERE point_id=:point_id;        "
                         " DELETE FROM trip_classes WHERE point_id=:point_id;            "
                         " DELETE FROM trip_comp_rem WHERE point_id=:point_id;           "
                         " DELETE FROM trip_comp_rates WHERE point_id=:point_id;         "
                         " DELETE FROM trip_comp_rfisc WHERE point_id=:point_id;         "
                         " DELETE FROM trip_comp_baselayers WHERE point_id=:point_id;    "
                         " DELETE FROM trip_comp_elems WHERE point_id=:point_id;         "
                         " DELETE FROM trip_comp_layers WHERE point_id=:point_id;        "
                         " DELETE FROM trip_crew WHERE point_id=:point_id;               "
                         " DELETE FROM trip_data WHERE point_id=:point_id;               "
                         " DELETE FROM trip_delays WHERE point_id=:point_id;             "
                         " DELETE FROM trip_load WHERE point_dep=:point_id;              "
                         " DELETE FROM trip_sets WHERE point_id=:point_id;               "
                         " DELETE FROM trip_final_stages WHERE point_id=:point_id;       "
                         " DELETE FROM trip_stations WHERE point_id=:point_id;           "
                         " DELETE FROM trip_paid_ckin WHERE point_id=:point_id;          "
                         " DELETE FROM trip_calc_data WHERE point_id=:point_id;          "
                         " DELETE FROM trip_alarms WHERE point_id=:point_id;             "
                         " DELETE FROM trip_pers_weights WHERE point_id=:point_id;       "
                         " DELETE FROM trip_auto_weighing WHERE point_id=:point_id;               "
                         " DELETE FROM trip_rpt_person WHERE point_id=:point_id;                  "
                         " UPDATE trfer_trips SET point_id_spp=NULL WHERE point_id_spp=:point_id; "
                         " DELETE FROM pax_seats WHERE point_id=:point_id;         "
                         " DELETE FROM utg_prl WHERE point_id=:point_id;           "
                         " DELETE FROM trip_tasks WHERE point_id=:point_id;        "
                         " DELETE FROM etickets WHERE point_id=:point_id;          "
                         " DELETE FROM emdocs WHERE point_id=:point_id;            "
                         " DELETE FROM trip_apis_params WHERE point_id=:point_id;  "
                         " DELETE FROM counters_by_subcls WHERE point_id=:point_id;"
                         " DELETE FROM iapi_pax_data WHERE point_id=:point_id;     "
                         " DELETE FROM wb_msg_text where id in(SELECT id FROM wb_msg WHERE point_id = :point_id);"
                         " DELETE FROM wb_msg where point_id = :point_id;"
                         " DELETE FROM trip_vouchers WHERE point_id=:point_id;"
                         " DELETE FROM confirm_print_vo_unreg WHERE point_id = :point_id; "
                         " DELETE FROM hotel_acmd_pax WHERE point_id = :point_id; "
                         " DELETE FROM hotel_acmd_free_pax WHERE point_id = :point_id; "
                         " DELETE FROM hotel_acmd_dates WHERE point_id = :point_id; "
                         "END;");
    cur.bind(":point_id", point_id);
    cur.exec();
}

void deleteByMoveId(const MoveId_t & move_id)
{
    auto cur = make_curs(
                "BEGIN "
                "DELETE FROM points WHERE move_id=:move_id;"
                "DELETE FROM move_ref WHERE move_id=:move_id; END;");
    cur.bind(":move_id", move_id);
    cur.exec();
}

//============================= TArxMoveNoFlt =============================
//STEP 2
int arx_tlgout_noflt(const Dates::DateTime_t& arx_date, int remain_rows)
{
    //LogTrace(TRACE5) << __FUNCTION__ << " arx_date: " << arx_date;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();

    std::vector<dbo::TLG_OUT> tlg_outs = session.query<dbo::TLG_OUT>()
            .where("POINT_ID is null and TIME_CREATE < :arx_date and ROWNUM < :rem_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":rem_rows", remain_rows}});

    session.connectPostgres();

    for(const auto &tg : tlg_outs) {
        Dates::DateTime_t part_key_nvl;
        if(tg.time_send_act.is_not_a_date_time()) {
            if(tg.time_send_scd.is_not_a_date_time()) {
                part_key_nvl = tg.time_create;
            } else {
                part_key_nvl = tg.time_send_scd;
            }
        } else {
            part_key_nvl = tg.time_send_act;
        }
        //NVL(time_send_act,NVL(time_send_scd,time_create)
        dbo::ARX_TLG_OUT ascs(tg, part_key_nvl);
        session.insert(ascs);
    }

    return tlg_outs.size();

    //TODO
    //    session.connectOracle();
    //    for(const auto & tg: tlg_outs) {
    //        auto cur = make_curs("BEGIN"
    //                             "DELETE FROM typeb_out_extra WHERE tlg_id=:id;"
    //                             "DELETE FROM typeb_out_errors WHERE tlg_id=:id;"
    //                             "DELETE FROM tlg_out WHERE id=:id;"
    //                             "END;");
    //        cur.bind(":id", tg.id).exec();
    //    }
}

int arx_events_noflt2(const Dates::DateTime_t& arx_date, int remain_rows)
{
    //LogTrace(TRACE5) << __FUNCTION__ << " arx_date: " << arx_date;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    //Dates::DateTime_t elapsed = arx_date - Dates::days(30);
    std::vector<dbo::Events_Bilingual> events =  session.query<dbo::Events_Bilingual>()
            .where("ID1 is NULL  and TYPE = :evtTlg and TIME >= :elapsed and TIME < :arx_date "
                   "and rownum <= :rem_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date},
                      {":elapsed", arx_date - Dates::days(30)},
                      {":evtTlg", EncodeEventType(evtTlg)},
                      {":rem_rows", remain_rows}});

    session.connectPostgres();
    for(const auto & ev : events) {
        dbo::Arx_Events aev(ev, ev.time);
        session.insert(aev);
    }
    //TODO
    //    session.connectOracle();
    //    auto cur = make_curs("DELETE FROM events_bilingual WHERE ID1 is NULL  and type = :evtTlg and TIME >= :arx_date - 30 and TIME < arx_date");
    //    cur.bind(":evtTlg", EncodeEventType(evtTlg))
    //       .bind(":arx_date", arx_date)
    //       .exec();
    return events.size();
}

int arx_events_noflt3(const Dates::DateTime_t& arx_date, int remain_rows)
{
    //LogTrace(TRACE5) << __FUNCTION__ << " arx_date: " << arx_date;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::Events_Bilingual> events =  session.query<dbo::Events_Bilingual>()
            .where("TIME >= :arx_date - 30 and TIME < :arx_date and rownum <= :rem_rows  and type not in "
                   " (:evtSeason, :evtDisp, :evtFlt, :evtGraph, :evtFltTask, :evtPax, :evtPay, :evtTlg, :evtPrn) FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":rem_rows", remain_rows},
                      {":evtSeason",  EncodeEventType(evtSeason)},
                      {":evtDisp",    EncodeEventType(evtDisp)},
                      {":evtFlt",     EncodeEventType(evtFlt)},
                      {":evtGraph",   EncodeEventType(evtGraph)},
                      {":evtFltTask", EncodeEventType(evtFltTask)},
                      {":evtPax",     EncodeEventType(evtPax)},
                      {":evtPay",     EncodeEventType(evtPay)},
                      {":evtTlg",     EncodeEventType(evtTlg)},
                      {":evtPrn",     EncodeEventType(evtPrn)}});
    session.connectPostgres();
    for(const auto & ev : events) {
        dbo::Arx_Events aev(ev, ev.time);
        session.insert(aev);
    }
    //TODO
    //    session.connectOracle();
    //    auto cur = make_curs("DELETE FROM events_bilingual WHERE ID1 is NULL  and type = :evtTlg and TIME >= :arx_date - 30 and TIME < arx_date");
    //    cur.bind(":evtTlg", EncodeEventType(evtTlg))
    //       .bind(":arx_date", arx_date)
    //       .exec();
    return events.size();
}

int arx_stat_zamar(const Dates::DateTime_t& arx_date, int remain_rows)
{
    //LogTrace(TRACE5) << __FUNCTION__ << " arx_date: " << arx_date;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<dbo::STAT_ZAMAR> stats =  session.query<dbo::STAT_ZAMAR>()
            .where("TIME < :arx_date and rownum <= :rem_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":rem_rows", remain_rows}});
    session.connectPostgres();
    for(const auto & ev : stats) {
        dbo::ARX_STAT_ZAMAR aev(ev, ev.time);
        session.insert(aev);
    }
    //todo
    //    session.connectOracle();
    //    auto cur = make_curs("DELETE FROM stat_zamar WHERE TIME < arx_date");
    //    cur.bind(":arx_date", arx_date).exec();
    return stats.size();
}

void move_noflt(const Dates::DateTime_t& arx_date, int max_rows, int time_duration, int& step)
{
    //LogTrace(TRACE5) << __FUNCTION__ << " arx_date: " << arx_date;
    int remain_rows = max_rows;
    Dates::DateTime_t local_time = Dates::second_clock::local_time();
    Dates::DateTime_t time_finish = local_time + Dates::seconds(time_duration);
    //Dates::DateTime_t time_finish = NowUTC() + time_duration/86400;
    while(step >= 1 && step <=4) {
        int rowsize = 0;
        if(local_time > time_finish) {
            return;
        }
        if(step == 1) {
            rowsize = arx_tlgout_noflt(arx_date, remain_rows);
        } else if(step == 2) {
            rowsize = arx_events_noflt2(arx_date, remain_rows);
        } else if(step == 3) {
            rowsize = arx_events_noflt3(arx_date, remain_rows);
        } else if(step == 4) {
            rowsize = arx_stat_zamar(arx_date, remain_rows);
        }

        remain_rows = remain_rows - rowsize;
        if (remain_rows <= 0) {
            return;
        }
        step += 1;
    }
    step = 0;
}

TArxMoveNoFlt::TArxMoveNoFlt(const Dates::DateTime_t &utc_date):TArxMove(utc_date), step(0)
{
};

TArxMoveNoFlt::~TArxMoveNoFlt()
{
    //
};

bool TArxMoveNoFlt::Next(size_t max_rows, int duration)
{
    if(step <= 0) {
        step = 1;
    }
    Dates::DateTime_t arx_date = utcdate-Dates::days(ARX_DAYS());
    move_noflt(arx_date, max_rows, duration, step);
    ASTRA::commit();
    proc_count++;
    return step>0;
};

string TArxMoveNoFlt::TraceCaption()
{
    return "TArxMoveNoFlt";
};


//============================= TArxTlgTrips =============================
//STEP 3
TArxTlgTrips::TArxTlgTrips(const Dates::DateTime_t& utc_date):TArxMove(utc_date)
{
    step=0;
    point_ids_count=0;
};

std::vector<PointId_t> TArxTlgTrips::getTlgTripPoints(const Dates::DateTime_t& arx_date, size_t max_rows)
{
    std::vector<PointId_t> points;
    int point_id;
    auto cur = make_curs("SELECT point_id FROM tlg_trips,tlg_binding "
                         "WHERE tlg_trips.point_id=tlg_binding.point_id_tlg(+) AND tlg_binding.point_id_tlg IS NULL AND "
                         " tlg_trips.scd < :arx_date");
    cur.def(point_id)
            .bind(":arx_date", arx_date)
            .exec();
    while(!cur.fen()) {
        if(points.size() == max_rows) {
            break;
        }
        points.push_back(PointId_t(point_id));
    }
    return points;
}


void arx_tlg_trip(const PointId_t& point_id)
{
    ckin::delete_typeb_data(point_id);
    auto cur = make_curs(
                "BEGIN "
                "DELETE FROM typeb_data_stat WHERE point_id = :point_id; "
                "DELETE FROM crs_data_stat WHERE point_id = :point_id; "
                "DELETE FROM tlg_comp_layers WHERE point_id = :point_id; "
                "UPDATE crs_displace2 SET point_id_tlg=NULL WHERE point_id_tlg = :point_id;"
                "END;");
    cur.bind(":point_id", point_id).exec();

    auto & session = dbo::Session::getInstance();
    session.connectOracle();
    std::vector<int> pnrids = session.query<int>("SELECT trfer_id").from("tlg_transfer")
            .where("point_id = :point_id").setBind({{":point_id", point_id}});
    std::vector<int> grpids = session.query<int>("SELECT grp_id").from("trfer_grp, tlg_transfer")
            .where("tlg_transfer.trfer_id = trfer_grp.trfer_id AND "
                   "tlg_transfer.point_id_out = :point_id").setBind({{":point_id", point_id}});
    for(const int grp_id : grpids) {
        auto cur = make_curs("BEGIN"
                             "DELETE FROM trfer_pax WHERE grp_id = :grp_id; "
                             "DELETE FROM trfer_tags WHERE grp_id = :grp_id; "
                             "DELETE FROM tlg_trfer_onwards WHERE grp_id = :grp_id; "
                             "DELETE FROM tlg_trfer_excepts WHERE grp_id = :grp_id; "
                             "END;");
        cur.bind(":grp_id", grp_id).exec();
    }
    for(const int trfer_id : pnrids) {
        auto cur = make_curs("DELETE FROM trfer_grp WHERE trfer_id= :trfer_id");
        cur.bind(":trfer_id", trfer_id).exec();
    }

    auto cur2 = make_curs("DELETE FROM tlg_transfer WHERE point_id_out= :point_id;");
    cur2.bind(":point_id", point_id).exec();

    auto cur3 = make_curs("  BEGIN "
                          "SELECT COUNT(*) INTO n FROM tlg_transfer "
                          "WHERE point_id_in = :point_id AND point_id_in<>point_id_out;"
                          "IF n=0 THEN "
                          "DELETE FROM tlg_source WHERE point_id_tlg = :point_id; "
                          "DELETE FROM tlg_trips WHERE point_id = :point_id; "
                          "ELSE "
                          /* удаляем только те ссылки на телеграммы которых нет в tlg_transfer */
                          "DELETE FROM tlg_source "
                          "  WHERE point_id_tlg=vpoint_id AND "
                          "  NOT EXISTS (SELECT * FROM tlg_transfer "
                          "    WHERE tlg_transfer.point_id_in=tlg_source.point_id_tlg AND "
                          "    tlg_transfer.tlg_id=tlg_source.tlg_id AND rownum<2); "
                          "END IF;"
                          "END;");
    cur3.bind(":point_id", point_id).exec();

}


bool TArxTlgTrips::Next(size_t max_rows, int duration)
{
    auto points = getTlgTripPoints(utcdate-Dates::days(ARX_DAYS()), max_rows);

    while (!points.empty())
    {
        PointId_t point_id = points.front();
        points.erase(points.begin());
        try
        {
            arx_tlg_trip(point_id);
            ASTRA::commit();
            proc_count++;
        }
        catch(...)
        {
            ProgError( STDLOG, "tlg_trips.point_id=%d", point_id.get() );
            throw;
        };
    };
    return true;
};

string TArxTlgTrips::TraceCaption()
{
    return "TArxTlgTrips";
};


//============================= TArxTypeBIn =============================
//STEP 4
TArxTypeBIn::TArxTypeBIn(const Dates::DateTime_t &utc_date):TArxMove(utc_date)
{
};

std::map<int, Dates::DateTime_t> getTlgIds(const Dates::DateTime_t& arx_date, size_t max_rows)
{
    std::map<int, Dates::DateTime_t> res;
    int tlg_id;
    Dates::DateTime_t time_receive;
    auto cur = make_curs(
                "SELECT id,time_receive "
                "FROM tlgs_in "
                "WHERE time_receive < :arx_date AND "
                "      NOT EXISTS(SELECT * FROM tlg_source WHERE tlg_source.tlg_id=tlgs_in.id AND rownum<2) AND "
                "      NOT EXISTS(SELECT * FROM tlgs_in a WHERE a.id=tlgs_in.id AND time_receive >= :arx_date AND rownum<2) ");
    cur
            .def(tlg_id)
            .def(time_receive)
            .bind(":arx_date", arx_date)
            .exec();
    while(!cur.fen()) {
        if(res.size() == max_rows) {
            break;
        }
        res.try_emplace(tlg_id, time_receive);
    }
    return res;
}

void move_typeb_in(int tlg_id)
{
    auto cur = make_curs(
                "BEGIN "
                "DELETE FROM typeb_in_body WHERE id=:id;"
                "DELETE FROM typeb_in_errors WHERE tlg_id=:id;"
                "DELETE FROM typeb_in_history WHERE prev_tlg_id=:id;"
                "DELETE FROM typeb_in_history WHERE tlg_id=:id;"
                "DELETE FROM tlgs_in WHERE id=:id;"
                "DELETE FROM typeb_in WHERE id=:id;"
                "END;");
    cur.bind(":id", tlg_id).exec();
}

bool TArxTypeBIn::Next(size_t max_rows, int duration)
{
    std::map<int, Dates::DateTime_t> tlg_ids = getTlgIds(utcdate - Dates::days(ARX_DAYS()), max_rows);
    while (!tlg_ids.empty())
    {
        int tlg_id = tlg_ids.begin()->first;
        tlg_ids.erase(tlg_ids.begin());

        try
        {
            //в архив
            move_typeb_in(tlg_id);
            ASTRA::commit();
            proc_count++;
        }
        catch(...)
        {
            ProgError( STDLOG, "typeb_in.id=%d", tlg_id );
            throw;
        };
    };
    return false;
}

string TArxTypeBIn::TraceCaption()
{
    return "TArxTypeBIn";
};

//============================= TArxNormsRatesEtc =============================
//STEP 5
int arx_bag_norms(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();

    std::vector<dbo::BAG_NORMS> bag_norms = session.query<dbo::BAG_NORMS>()
            .where("last_date < :arx_date AND "
                   "NOT EXISTS (SELECT * FROM pax_norms WHERE pax_norms.norm_id=bag_norms.id AND rownum<2) AND "
                   "NOT EXISTS (SELECT * FROM grp_norms WHERE grp_norms.norm_id=bag_norms.id AND rownum<2) AND "
                   "rownum <= :remain_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});

    session.connectPostgres();
    for(const auto & bn : bag_norms) {
        dbo::ARX_BAG_NORMS abn(bn, bn.last_date);
        session.insert(abn);
    }
    return bag_norms.size();
}

int arx_bag_rates(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();

    std::vector<dbo::BAG_RATES> bag_rates = session.query<dbo::BAG_RATES>()
            .where("last_date < :arx_date AND "
                   "NOT EXISTS (SELECT * FROM paid_bag WHERE paid_bag.rate_id=bag_rates.id AND rownum<2) AND "
                   "rownum <= :remain_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});

    session.connectPostgres();
    for(const auto & bn : bag_rates) {
        dbo::ARX_BAG_RATES abn(bn, bn.last_date);
        session.insert(abn);
    }
    return bag_rates.size();
}

int arx_value_bag_taxes(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();

    std::vector<dbo::VALUE_BAG_TAXES> bag_taxes = session.query<dbo::VALUE_BAG_TAXES>()
            .where("last_date < :arx_date AND "
                   "NOT EXISTS (SELECT * FROM value_bag WHERE value_bag.tax_id=value_bag_taxes.id AND rownum<2) AND "
                   "rownum <= :remain_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});

    session.connectPostgres();
    for(const auto & bn : bag_taxes) {
        dbo::ARX_VALUE_BAG_TAXES abn(bn, bn.last_date);
        session.insert(abn);
    }
    return bag_taxes.size();
}

int arx_exchange_rates(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();

    std::vector<dbo::EXCHANGE_RATES> exc_rates = session.query<dbo::EXCHANGE_RATES>()
            .where("last_date < :arx_date AND rownum <= :remain_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});

    session.connectPostgres();
    for(const auto & bn : exc_rates) {
        dbo::ARX_EXCHANGE_RATES abn(bn, bn.last_date);
        session.insert(abn);
    }
    return exc_rates.size();
}

int delete_from_mark_trips(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();

    std::vector<dbo::Mark_Trips> mark_trips = session.query<dbo::Mark_Trips>()
            .where("scd < :arx_date AND rownum <= :remain_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});

    for(const auto & bn : mark_trips) {
        auto cur = make_curs("delete from MARK_TRIPS where POINT_ID = :point_id");
        cur.bind(":point_id", bn.point_id).exec();
    }
    return mark_trips.size();
}

void norms_rates_etc(const Dates::DateTime_t& arx_date, int max_rows, int time_duration, int& step)
{
    int remain_rows = max_rows;
    Dates::DateTime_t local_time = Dates::second_clock::local_time();
    Dates::DateTime_t time_finish = local_time + Dates::seconds(time_duration);
    while(step >= 1 && step <=5) {
        int rowsize = 0;
        if(local_time > time_finish) {
            return;
        }
        if(step == 1) {
            rowsize = arx_bag_norms(arx_date, remain_rows);
        } else if(step == 2) {
            rowsize = arx_bag_rates(arx_date, remain_rows);
        } else if(step == 3) {
            rowsize = arx_value_bag_taxes(arx_date, remain_rows);
        } else if(step == 4) {
            rowsize = arx_exchange_rates(arx_date, remain_rows);
        } else if(step == 5) {
            rowsize = delete_from_mark_trips(arx_date, remain_rows);
        }

        remain_rows = remain_rows - rowsize;
        if (remain_rows <= 0) {
            return;
        }
        step += 1;
    }
    step = 0;
}

TArxNormsRatesEtc::TArxNormsRatesEtc(const Dates::DateTime_t& utc_date):TArxMove(utc_date), step(0)
{
};

bool TArxNormsRatesEtc::Next(size_t max_rows, int duration)
{
    if(step <= 0) {
        step = 1;
    }
    Dates::DateTime_t arx_date = utcdate-Dates::days(ARX_DAYS());
    norms_rates_etc(arx_date, max_rows, duration, step);
    ASTRA::commit();
    proc_count++;
    return step > 0;
};

string TArxNormsRatesEtc::TraceCaption()
{
    return "TArxNormsRatesEtc";
};


//============================= TArxTlgsFilesEtc===========
// STEP 6
int arx_tlgs(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();

    std::vector<dbo::TLGS> tlgs = session.query<dbo::TLGS>()
            .where("time < :arx_date AND rownum <= :remain_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});

    for(const auto & t : tlgs) {
        std::vector<dbo::TLG_STAT> stats = session.query<dbo::TLG_STAT>()
                .where("QUEUE_TLG_ID = :id and TIME_SEND is not null")
                .setBind({{":id", t.id}});

        session.connectPostgres();
        for(const auto & s : stats) {
            dbo::ARX_TLG_STAT ats(s, s.time_send);
            session.insert(ats);
        }
        session.connectOracle();
        auto cur = make_curs("BEGIN "
                             //"delete from TLGS where POINT_ID = :id" TODO
                             //"delete from TLG_STAT where POINT_ID = :id" TODO
                             "delete from TLG_ERROR where POINT_ID = :id; "
                             "delete from TLG_QUEUE where POINT_ID = :id; "
                             "delete from TLG_TEXT where POINT_ID = :id; END");
        cur.bind(":id", t.id).exec();
    }
    return tlgs.size();
}

int delete_files(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();

    std::vector<dbo::FILES> files = session.query<dbo::FILES>()
            .where("time < :arx_date AND rownum <= :remain_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});

    for(const auto & f : files) {
        auto cur = make_curs("BEGIN "
                             //"delete from FILES where POINT_ID = :id; " TODO
                             "delete from FILE_QUEUE where POINT_ID = :id; "
                             "delete from FILE_PARAMS where POINT_ID = :id; "
                             "delete from FILE_ERROR where POINT_ID = :id; END");
        cur.bind(":id", f.id).exec();
    }
    return files.size();
}

int delete_kiosk_events(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto & session = dbo::Session::getInstance();
    session.connectOracle();

    std::vector<dbo::KIOSK_EVENTS> events = session.query<dbo::KIOSK_EVENTS>()
            .where("TIME < :arx_date and ROWNUM <= :remain_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});

    for(const auto & ev : events) {
        auto cur = make_curs("BEGIN "
                             //"delete from KIOSK_EVENTS where ID = :id; " TODO
                             "delete from KIOSK_EVENT_PARAMS where EVENT_ID = :id; END");
        cur.bind(":id", ev.id).exec();
    }
    return events.size();
}

int delete_rozysk(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto cur = make_curs("delete from ROZYSK where TIME < :arx_date and ROWNUM <= :remain_rows ");
    cur.bind(":arx_date", arx_date).bind(":remain_rows", remain_rows).exec();
    return cur.rowcount();
}

int delete_aodb_spp_files(const Dates::DateTime_t& arx_date, int remain_rows)
{
    int rowsize = 0;
    auto & session = dbo::Session::getInstance();
    session.connectOracle();

    std::vector<dbo::AODB_SPP_FILES> files = session.query<dbo::AODB_SPP_FILES>()
            .where("filename<'SPP'||TO_CHAR(:arx_date, 'YYMMDD')||'.txt' AND rownum <= :remain_rows FOR UPDATE")
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});

    for(const auto & f : files) {
        auto cur = make_curs("BEGIN "
                             "delete from AODB_EVENTS "
                             "where filename= :filename AND point_addr= :point_addr "
                             "AND airline= :airline AND rownum<= :remain_rows; END");
        cur
                .bind(":filename", f.filename)
                .bind(":point_addr", f.point_addr)
                .bind(":airline", f.airline)
                .bind(":remain_rows", remain_rows)
                .exec();
        rowsize += cur.rowcount();

        //todo
        //        auto cur2 = make_curs("BEGIN "
        //                             "delete from AODB_SPP_FILES "
        //                             "where filename= :filename AND point_addr= :point_addr "
        //                             "AND airline= :airline; END");
        //        cur2
        //           .bind(":filename", f.filename)
        //           .bind(":point_addr", f.point_addr)
        //           .bind(":airline", f.airline)
        //           .exec();
        //        rowsize += cur2.rowcount();
    }
    return rowsize;
}

int delete_eticks_display(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto cur = make_curs("delete from ETICKS_DISPLAY where LAST_DISPLAY < :arx_date and ROWNUM <= :remain_rows ");
    cur.bind(":arx_date", arx_date).bind(":remain_rows", remain_rows).exec();
    return cur.rowcount();
}

int delete_eticks_display_tlgs(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto cur = make_curs("delete from ETICKS_DISPLAY_TLGS where LAST_DISPLAY < :arx_date and ROWNUM <= :remain_rows ");
    cur.bind(":arx_date", arx_date).bind(":remain_rows", remain_rows).exec();
    return cur.rowcount();
}

int delete_emdocs_display(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto cur = make_curs("delete from EMDOCS_DISPLAY where LAST_DISPLAY < :arx_date and ROWNUM <= :remain_rows ");
    cur.bind(":arx_date", arx_date).bind(":remain_rows", remain_rows).exec();
    return cur.rowcount();
}

void tlgs_files_etc(const Dates::DateTime_t& arx_date, int max_rows, int time_duration, int& step)
{
    int remain_rows = max_rows;
    Dates::DateTime_t local_time = Dates::second_clock::local_time();
    Dates::DateTime_t time_finish = local_time + Dates::seconds(time_duration);
    while(step >= 1 && step <=8) {
        int rowsize = 0;
        if(local_time > time_finish) {
            return;
        }
        if(step == 1) {
            rowsize = arx_tlgs(arx_date, remain_rows);
        } else if(step == 2) {
            rowsize = delete_files(arx_date, remain_rows);
        } else if(step == 8) {
            rowsize = delete_kiosk_events(arx_date, remain_rows);
        } else if(step == 3) {
            rowsize = delete_rozysk(arx_date, remain_rows);
        } else if(step == 4) {
            rowsize = delete_aodb_spp_files(arx_date, remain_rows);
        } else if(step == 5) {
            rowsize = delete_eticks_display(arx_date, remain_rows);
        } else if(step == 6) {
            rowsize = delete_eticks_display_tlgs(arx_date, remain_rows);
        } else if(step == 7) {
            rowsize = delete_emdocs_display(arx_date, remain_rows);
        }

        remain_rows = remain_rows - rowsize;
        if (remain_rows <= 0) {
            return;
        }
        step += 1;
    }
    step = 0;
}

TArxTlgsFilesEtc::TArxTlgsFilesEtc(const Dates::DateTime_t &utc_date):TArxMove(utc_date), step(0)
{
};

bool TArxTlgsFilesEtc::Next(size_t max_rows, int duration)
{
    if(step <= 0) {
        step = 1;
    }
    Dates::DateTime_t arx_date = utcdate - Dates::days(ARX_DAYS());
    tlgs_files_etc(arx_date, max_rows, duration, step);
    ASTRA::commit();
    proc_count++;
    return step > 0;
};

string TArxTlgsFilesEtc::TraceCaption()
{
    return "TArxTlgsFilesEtc";
};


std::unique_ptr<TArxMove> create_arx_manager(const Dates::DateTime_t& utcdate, int step = 1)
{
    switch (step)
    {
    case 1: return std::make_unique<TArxMoveFlt>(utcdate);
    case 2: return std::make_unique<TArxMoveNoFlt>(utcdate);
    case 3: return std::make_unique<TArxTlgTrips>(utcdate);
    case 4: return std::make_unique<TArxTypeBIn>(utcdate);
    case 5: return std::make_unique<TArxNormsRatesEtc>(utcdate);
    case 6: return std::make_unique<TArxTlgsFilesEtc>(utcdate);
    default: return nullptr;
    };
    return nullptr;
}

bool arx_daily(const Dates::DateTime_t& utcdate)
{
    static Dates::DateTime_t prior_utcdate;
    static  time_t prior_exec = 0;
    static int step = 1;
    static std::unique_ptr<TArxMove> arxMove = nullptr;

    dbo::initStructures();

    time_t time_finish = time(NULL)+ARX_DURATION();

    if (prior_utcdate != utcdate)
    {
        if (arxMove)
        {
            arxMove.reset();
        };
        step=1;
        prior_utcdate = utcdate;
        ProgTrace(TRACE5,"arx_daily START");
    };

    for(;step<7;step++)
    {
        if (!arxMove)
        {
            arxMove = create_arx_manager(utcdate, step);
            ProgTrace(TRACE5,"arx_daily: %s started",arxMove->TraceCaption().c_str());
        }

        arxMove->BeforeProc();
        try
        {
            int duration;
            do {
                duration=time_finish - time(NULL);
                if (duration<=0)
                {
                    ProgTrace(TRACE5,"arx_daily: %d iterations processed", arxMove->Processed());
                    prior_exec = time(NULL);
                    return false;
                };
            } while(arxMove->Next(ARX_MAX_ROWS(),duration));

            ProgTrace(TRACE5,"arx_daily: %d iterations processed", arxMove->Processed());
        }
        catch(...)
        {
            ProgTrace(TRACE5,"arx_daily: %d iterations processed",arxMove->Processed());
            prior_exec = time(NULL);
            throw;
        };
        ProgTrace(TRACE5,"arx_daily: %s finished",arxMove->TraceCaption().c_str());
        arxMove.reset();
    }

    ProgTrace(TRACE5,"arx_daily FINISH");
    prior_exec=time(NULL);
    return true;
}


#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef XP_TESTING
bool test_arx_daily(const Dates::DateTime_t& utcdate, int step)
{
    LogTrace(TRACE5) << __FUNCTION__ << " step: " << step;
    dbo::initStructures();
    auto arxMove = create_arx_manager(utcdate, step);

    ProgTrace(TRACE5,"arx_daily_pg: %s started", arxMove->TraceCaption().c_str());

    arxMove->BeforeProc();
    time_t time_finish = time(NULL)+ARX_DURATION();
    int duration = 0;
    do{
        duration = time_finish - time(NULL);
        if (duration<=0)
        {
            ProgTrace(TRACE5,"arx_daily: %d iterations processed", arxMove->Processed());
            return false;
        };
    }
    while(arxMove->Next(ARX_MAX_ROWS(), duration));

    ProgTrace(TRACE5,"arx_daily_pg: %s finished",arxMove->TraceCaption().c_str());
    return true;
}
#endif //XP_TESTING

} //namespace PG_ARX
