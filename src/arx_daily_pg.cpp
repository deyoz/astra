//---------------------------------------------------------------------------
#include "arx_daily_pg.h"
#include "oralib.h"
#include "exceptions.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "qrys.h"
#include "stat/stat_main.h"

#include <memory>
#include <sstream>
#include <optional>
#include "boost/date_time/local_time/local_time.hpp"

#include <serverlib/pg_cursctl.h>
#include <serverlib/pg_rip.h>
#include <serverlib/testmode.h>
#include <serverlib/dates_oci.h>
#include <serverlib/dates_io.h>
#include <serverlib/tcl_utils.h>
#include <serverlib/oci_rowid.h>
#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/timer.h>
#include "PgOraConfig.h"
#include "tlg/typeb_db.h"
#include "pax_db.h"
#include "counters.h"

//#include "serverlib/dump_table.h"
//#include "hooked_session.h"
#include "baggage_ckin.h"
#include "dbostructures.h"
#include "baggage_calc.h"
#include "alarms.h"

#define NICKNAME "FELIX"
#define NICKTRACE FELIX_TRACE
#include <serverlib/slogger.h>

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;


bool deleteEtickets(int point_id);
bool deleteEmdocs(int point_id);

namespace ARX {

bool WRITE_PG()
{
    static bool res = PgOra::Config("SP_PG_GROUP_ARX").writePostgres();
    return res;
}

bool WRITE_ORA()
{
    static bool res = PgOra::Config("SP_PG_GROUP_ARX").writeOracle();
    return res;
}

bool READ_PG()
{
    static bool res = PgOra::Config("SP_PG_GROUP_ARX").readPostgres();
    return res;
}

bool READ_ORA()
{
    static bool res = PgOra::Config("SP_PG_GROUP_ARX").readOracle();
    return res;
}

bool CLEANUP_PG()
{
    // Если читаем из PG, то пусть и PG-архиватор занимается удалением данных
    // из неархивных таблиц
    return READ_PG();
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

int ARX_MAX_DATE_RANGE()
{
    static int VAR=NoExists;
    if (VAR==NoExists)
        VAR=getTCLParam("ARX_MAX_DATE_RANGE",400,NoExists,10000);
    return VAR;
};

}//namespace ARX

/////////////////////////////////////////////////////////////////////////////////////////

bool arx_daily_pg(TDateTime utcdate)
{
    LogTrace(TRACE6) << __func__ << " utcdate: " << utcdate;
    if(utcdate == ASTRA::NoExists) {
        LogTrace(TRACE6) << " utcdate incorrect";
        return false;
    }
    return PG_ARX::arx_daily(DateTimeToBoost(utcdate));
}

/////////////////////////////////////////////////////////////////////////////////////////

namespace  PG_ARX
{
//by move_id
std::vector<dbo::Points> read_points(const MoveId_t & move_id);
std::vector<dbo::Points> arx_points(const std::vector<dbo::Points> &points, const Dates::DateTime_t &part_key);
void arx_move_ref(const MoveId_t & vmove_id, const Dates::DateTime_t & part_key);
void arx_move_ext(const MoveId_t & vmove_id, const Dates::DateTime_t & part_key, double date_range);
void arx_events_by_move_id(const MoveId_t & vmove_id, const Dates::DateTime_t & part_key);
//by point_id
std::vector<dbo::Pax_Grp> read_pax_grp(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_pax_grp(const std::vector<dbo::Pax_Grp> & pax_grps, const Dates::DateTime_t & part_key);
void arx_events_by_point_id(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_mark_trips(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_self_ckin_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_rfisc_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_stat_services(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_stat_rem(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_limited_cap_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_pfs_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_stat_ad(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_stat_ha(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_stat_vo(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_stat_reprint(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_trfer_pax_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_bi_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_agent_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_trfer_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_tlg_out(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_trip_classes(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_trip_delays(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_trip_load(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_trip_sets(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_crs_displace2(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_trip_stages(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_bag_receipts(const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_bag_pay_types(int receipt_id, const Dates::DateTime_t & part_key);
//by grp_id
std::vector<dbo::PAX> read_pax(const GrpId_t& grp_id);
void arx_pax(const std::vector<dbo::PAX>& paxes, const GrpId_t& grp_id,
             const PointId_t& point_id, const Dates::DateTime_t & part_key);
void arx_annul_bags_tags(const GrpId_t& grp_id, const Dates::DateTime_t & part_key);
void arx_unaccomp_bag_info(const GrpId_t& grp_id, const Dates::DateTime_t & part_key);
void arx_bag2(const GrpId_t& grp_id, const Dates::DateTime_t & part_key);
void arx_bag_prepay(const GrpId_t& grp_id, const Dates::DateTime_t & part_key);
void arx_bag_tags(const GrpId_t& grp_id, const Dates::DateTime_t & part_key);
void arx_paid_bag(const GrpId_t& grp_id, const Dates::DateTime_t & part_key);
void arx_pay_services(const GrpId_t& grp_id, const Dates::DateTime_t & part_key);
void arx_transfer(const GrpId_t& grp_id, const Dates::DateTime_t & part_key);
void arx_tckin_segments(const GrpId_t& grp_id, const Dates::DateTime_t & part_key);
void arx_value_bag(const GrpId_t& grp_id, const Dates::DateTime_t & part_key);
void arx_grp_norms(const GrpId_t& grp_id, const Dates::DateTime_t & part_key);
//by pax_id
void arx_pax_norms(const PaxId_t& pax_id, const Dates::DateTime_t & part_key);
void arx_pax_rem(const PaxId_t& pax_id, const Dates::DateTime_t & part_key);
void arx_transfer_subcls(const PaxId_t& pax_id, const Dates::DateTime_t & part_key);
void arx_pax_doc(const PaxId_t& pax_id, const Dates::DateTime_t & part_key);
void arx_pax_doco(const PaxId_t& pax_id, const Dates::DateTime_t & part_key);
void arx_pax_doca(const PaxId_t& pax_id, const Dates::DateTime_t & part_key);

void deleteByPaxes(const PaxId_t& pax_id);
void deleteByGrpId(const GrpId_t& grp_id);
void delete_annul_bags_tags(const GrpId_t& grp_id);
size_t delete_trfer_trips(const GrpId_t& grp_id);
void deleteAodbBag(const PointId_t& point_id);
void deleteTransferPaxStat(const PointId_t& point_id);
void deleteTlgOut(const PointId_t& point_id);
void deleteByPointId(const PointId_t& point_id);
void deleteEventsByPointId(const PointId_t& point_id);
void deleteBagReceipts(const PointId_t& point_id);
void deleteStatServices(const PointId_t& point_id);
void deleteStatAd(const PointId_t& point_id);
void deleteBiStat(const PointId_t& point_id);

void deleteByMoveId(const MoveId_t & move_id);
void deleteEventsByMoveId(const MoveId_t & move_id);

//step2
int arx_tlgout_noflt(const Dates::DateTime_t& arx_date, int remain_rows);
int arx_events_noflt2(const Dates::DateTime_t& arx_date, int remain_rows);
int arx_events_noflt3(const Dates::DateTime_t& arx_date, int remain_rows);
int arx_stat_zamar(const Dates::DateTime_t& arx_date, int remain_rows);
void move_noflt(const Dates::DateTime_t& arx_date, int max_rows, int time_duration, int& step);
//step3
void arx_tlg_trip(const PointIdTlg_t& point_id);
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
                        std::string fmt, int row = 1, int only_lat = 0)
{
    LogTrace(TRACE6) << __func__<< " pax_id: " << pax_id << " seats: " << seats << " is_jmpg: " << is_jmp << " status: "
              << status << " point_id: " << point_id << " fmt: " << fmt;
    char result[50] = {};
#ifdef ENABLE_ORACLE
    short null = -1, nnull = 0; //-1 - NULL , 0 - value
    auto cur = make_curs(
                "BEGIN \n"
                "   :result := salons.get_seat_no(:pax_id, :seats, :is_jmp, :status, :point_id, :fmt, :rownum, :only_lat); \n"
                "END;");
    cur.autoNull()
       .bind(":pax_id", pax_id.get() )
       .bind(":seats", seats)
       .bind(":is_jmp", is_jmp)
       .bind(":status", status, status.empty() ? &null : &nnull)
       .bind(":point_id", point_id.get())
       .bind(":fmt", fmt)
       .bind(":rownum", row)
       .bind(":only_lat", only_lat)
       .bindOutNull(":result", result, "")
       .exec();
#else
    LogError(STDLOG) << "Oracle not enabled. get_seat_no returns empty result.";
#endif // ENABLE_ORACLE
    return std::string(result);
}

}

TArxMove::TArxMove(const Dates::DateTime_t &utc_date)
{
    proc_count=0;
    utcdate = utc_date;
};

//============================= TArxMoveFlt =============================
//STEP 1
TArxMoveFlt::TArxMoveFlt(const Dates::DateTime_t& utc_date):TArxMove(utc_date)
{
};

void TArxMoveFlt::LockAndCollectStat(const MoveId_t & move_id)
{
    LogTrace(TRACE6) << __FUNCTION__ << " move_id: " << move_id;
    dbo::Session session;
    std::vector<dbo::Points> points = session.query<dbo::Points>()
            .where(" MOVE_ID = :move_id ")
            .for_update(true)
            .setBind({{":move_id",move_id.get()} });
    for(const auto & p : points) {
        bool pr_reg = p.pr_reg != 0;
        if(p.pr_del < 0) continue;
        if (p.pr_del==0 && pr_reg) get_flight_stat(p.point_id, true);
        TReqInfo::Instance()->LocaleToLog("EVT.FLIGHT_MOOVED_TO_ARCHIVE", evtFlt, p.point_id);
    }
};


Dates::time_period tripDatePeriod(const std::vector<dbo::Points> & points)
{
    Dates::DateTime_t first_date, last_date;
    for(size_t i = 0; i<points.size(); i++)
    {
        const dbo::Points & p = points[i];
        if(p.pr_del != -1) {
            std::vector<Dates::DateTime_t> temp;
            if(i%2 == 0) {
                temp = {first_date,last_date, p.scd_out, p.est_out, p.act_out};
            } else {
                temp = {first_date,last_date, p.scd_in, p.est_in, p.act_in};
            }
            auto fdates = algo::filter(temp, dbo::isNotNull<Dates::DateTime_t>);
            if(auto minIt = std::min_element(fdates.begin(), fdates.end()); minIt != fdates.end()) {
                first_date = *minIt;
            }
            if(auto maxIt = std::max_element(fdates.begin(), fdates.end()); maxIt != fdates.end()) {
                last_date = *maxIt;
            }
            //LogTrace(TRACE6) << __func__ << " first_date: " << first_date << " last_date: " << last_date;
        }
    }
    return Dates::time_period(first_date, last_date);
}

bool validDatePeriod(const Dates::time_period& date_period, Dates::DateTime_t utcdate)
{
    LogTrace(TRACE6) << __func__ << date_period;
//    if(date_period.is_null()) {
//        LogTrace(TRACE6) << " wrong date_period: " << date_period;
//        return false;
//    }
    Dates::DateTime_t first_date = date_period.begin();
    Dates::DateTime_t last_date = date_period.end();
    if (first_date == Dates::not_a_date_time && last_date == Dates::not_a_date_time) return false;
    return last_date < (utcdate - Dates::days(ARX::ARX_DAYS()));
}

double getDateRange(const Dates::time_period& date_period)
{
    double date_range = BoostToDateTime(date_period.end()) - BoostToDateTime(date_period.begin());
    if (date_range > ARX::ARX_MAX_DATE_RANGE()) {
        return 0;
    }
    return date_range;
}

void TArxMoveFlt::readMoveIds(size_t max_rows)
{
    LogTrace(TRACE6) << __func__;
    int move_id;
    auto cur = make_db_curs("SELECT move_id FROM points "
                            "WHERE (time_in > TO_DATE('01.01.1900','DD.MM.YYYY') AND time_in<:arx_date)"
                            " OR (time_out > TO_DATE('01.01.1900','DD.MM.YYYY') AND time_out<:arx_date)"
                            " OR (time_in  = TO_DATE('01.01.1900','DD.MM.YYYY') AND "
                            "      time_out = TO_DATE('01.01.1900','DD.MM.YYYY'))",
                            PgOra::getROSession("points"));
    cur.stb()
       .def(move_id)
       .bind(":arx_date", utcdate - Dates::days(ARX::ARX_DAYS()))
       .exec();

    while(!cur.fen() && (move_ids.size() < max_rows)) {
        std::vector<dbo::Points> points = read_points(MoveId_t(move_id));
        move_ids.try_emplace(MoveId_t(move_id), tripDatePeriod(points));
    }
}

bool isTripDeleted(const std::vector<dbo::Points>& points)
{
    return std::all_of(begin(points), end(points), [](const dbo::Points & p){return p.pr_del == -1;});
}

bool TArxMoveFlt::Next(size_t max_rows, int duration)
{
    if(move_ids.empty()) {
        HelpCpp::Timer timer;
        readMoveIds(max_rows);
        LogTrace(TRACE6) << " readMoveIds: " << timer.elapsedSeconds();
    }
    HelpCpp::Timer timer;
    while (!move_ids.empty())
    {
        MoveId_t move_id = move_ids.begin()->first;
        Dates::time_period date_period = move_ids.begin()->second;
        LogTrace(TRACE6) << __func__ << " move_id: " << move_id;
        move_ids.erase(move_ids.begin());

        std::vector<dbo::Points> points = read_points(move_id);
        bool isTripDel = isTripDeleted(points);
        bool isValidPeriod = validDatePeriod(date_period, utcdate);
        if(!isValidPeriod && !isTripDel) continue; // если период дат невалидный и маршрут неудаленный то игнорируем

        Dates::DateTime_t part_key = isValidPeriod ? date_period.end() : Dates::not_a_date_time;
        LogTrace(TRACE6) << __func__ << " part_key: " << part_key;
        try
        {
            LockAndCollectStat(move_id);

            if(dbo::isNotNull(part_key) && !isTripDel) {
                arx_move_ext(move_id, part_key, getDateRange(date_period));
                arx_points(points, part_key);
                arx_move_ref(move_id, part_key);
                arx_events_by_move_id(move_id, part_key);
            }
            for(const auto &p : points)
            {
                bool need_arch = p.pr_del != -1;
                PointId_t point_id(p.point_id);
                LogTrace(TRACE6) << __func__ << " POINT_ID = "  << p.point_id;
                auto pax_grps = read_pax_grp(point_id, part_key);
                if(need_arch) {
                    arx_pax_grp(pax_grps, part_key);
                    arx_mark_trips(point_id, part_key);
                    arx_events_by_point_id(point_id, part_key);
                    arx_self_ckin_stat(point_id, part_key);
                    arx_rfisc_stat(point_id, part_key);
                    arx_stat_services(point_id, part_key);
                    arx_stat_rem(point_id, part_key);
                    arx_limited_cap_stat(point_id, part_key);
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
                    arx_trip_classes(point_id,  part_key);
                    arx_trip_delays(point_id,  part_key);
                    arx_trip_load(point_id,  part_key);
                    arx_trip_sets(point_id,  part_key);
                    arx_crs_displace2(point_id,  part_key);
                    arx_trip_stages(point_id, part_key);
                    arx_tlg_out(point_id, part_key);
                    arx_bag_receipts(point_id, part_key);
                }
                //FK to PAX need delete before delete from PAX
                if(ARX::CLEANUP_PG()) {
                    deleteTransferPaxStat(point_id);
                    deleteStatServices(point_id);
                    deleteStatAd(point_id);
                    deleteBiStat(point_id);
                    deleteBagReceipts(point_id); //FK TO PAX_GRP
                }

                for(const auto &grp : pax_grps)
                {
                    GrpId_t grp_id(grp.m_grp_id);
                    LogTrace(TRACE6) << " PAX_GRP_ID : " << grp_id;
                    auto paxes = read_pax(grp_id);
                    if(need_arch) {
                        arx_pax(paxes, grp_id, point_id, part_key);
                        arx_annul_bags_tags(grp_id, part_key);
                        arx_unaccomp_bag_info(grp_id, part_key);
                        arx_bag2(grp_id, part_key);
                        arx_bag_prepay(grp_id, part_key);
                        arx_bag_tags(grp_id, part_key);
                        arx_paid_bag(grp_id, part_key);
                        arx_pay_services(grp_id, part_key);
                        arx_value_bag(grp_id, part_key);
                        arx_grp_norms(grp_id, part_key);
                        arx_transfer(grp_id, part_key);
                        arx_tckin_segments(grp_id, part_key);
                    }
                    if(ARX::CLEANUP_PG()) {
                        delete_annul_bags_tags(grp_id); //FK TO PAX and FK TO PAX_GRP
                    }
                    for(const auto & pax : paxes) {
                        PaxId_t pax_id(pax.pax_id);
                        LogTrace(TRACE6) << " PAX_ID : " << pax_id;
                        if(need_arch) {
                            arx_pax_norms(pax_id, part_key);
                            arx_pax_rem(pax_id, part_key);
                            arx_transfer_subcls(pax_id, part_key);
                            arx_pax_doc(pax_id, part_key);
                            arx_pax_doco(pax_id, part_key);
                            arx_pax_doca(pax_id, part_key);
                        }
                        deleteByPaxes(pax_id);
                    }
                    deleteByGrpId(grp_id);
                }
                deleteByPointId(point_id);
            }
            deleteByMoveId(move_id);

            ASTRA::commitAndCallCommitHooks();
            proc_count++;
        }
        catch(...)
        {
            if (part_key != Dates::not_a_date_time)
                ProgError( STDLOG, "move_id=%d, part_key=%s", move_id.get(),
                           HelpCpp::string_cast(part_key, "%Y%m%d").c_str() );
            else
                ProgError( STDLOG, "move_id=%d, part_key=not_a_date_time", move_id.get() );
            throw;
        };
    };
    LogTrace(TRACE6) << " MoveFlt one iteration time: " << timer.elapsedSeconds();
    return false;
};

string TArxMoveFlt::TraceCaption()
{
    return "TArxMoveFlt";
};

std::vector<dbo::Points> read_points(const MoveId_t & move_id)
{
    LogTrace(TRACE6) << __FUNCTION__ << " move_id : "<< move_id;
    dbo::Session session;
    std::vector<dbo::Points> points = session.query<dbo::Points>()
            .where(" MOVE_ID = :move_id ORDER BY point_num ")
            .for_update(true)
            .setBind({{"move_id",move_id.get()} });
    return points;
}

std::vector<dbo::Points> arx_points(const std::vector<dbo::Points> & points, const Dates::DateTime_t & part_key)
{
    LogTrace(TRACE6) << __func__ << " part_key: " << part_key;
    auto move_points = algo::filter(points, [&](const dbo::Points & point){return point.pr_del!=-1;});
    dbo::Session session;
    for(const auto &p : move_points) {
        dbo::Arx_Points ap(p, part_key);
        session.insert(ap);
    }
    return move_points;
}

void arx_move_ext(const MoveId_t & vmove_id, const Dates::DateTime_t & part_key, double date_range)
{
    int days = (date_range >= 1) ? (int)ceil(date_range) : 0;
    LogTrace(TRACE6) << __FUNCTION__ << " move_id: " << vmove_id << " days: " << days;
    if(days >= 1) {
        dbo::Session session;
        dbo::Move_Arx_Ext ext{days, vmove_id.get(), part_key};
        session.insert(ext);
    }
}

void arx_move_ref(const MoveId_t & vmove_id, const Dates::DateTime_t & part_key)
{
    LogTrace(TRACE6) << __FUNCTION__ << " move_id: " << vmove_id;
    dbo::Session session;
    std::vector<dbo::Move_Ref> move_refs = session.query<dbo::Move_Ref>()
        .where(" MOVE_ID = :move_id")
        .setBind({{"move_id",vmove_id.get()} });
    for(const auto & mr : move_refs) {
        dbo::Arx_Move_Ref amr(mr, part_key);
        session.insert(amr);
    }
}

void arx_events_by_move_id(const MoveId_t & move_id, const Dates::DateTime_t & part_key)
{
    LogTrace(TRACE6) << __FUNCTION__ << " move_id: " << move_id;
    dbo::Session session;
    std::vector<dbo::Lang_Types> langs = session.query<dbo::Lang_Types>();
    for(const auto & lang : langs) {
        std::vector<dbo::Events_Bilingual> events =  session.query<dbo::Events_Bilingual>()
            .where(" id1 = :move_id and lang = :l and type = :evtDisp")
            .for_update(true)
            .setBind({{"move_id",move_id.get()},
                      {"l",lang.code},
                      {"evtDisp", EncodeEventType(evtDisp)}});
        for(const auto & ev : events) {
            dbo::Arx_Events aev(ev, part_key);
            session.insert(aev);
        }
    }
}

void deleteEventsByMoveId(const MoveId_t & move_id)
{
    LogTrace(TRACE6) << __FUNCTION__ << " move_id: " << move_id ;
    dbo::Session session;
    std::vector<dbo::Lang_Types> langs = session.query<dbo::Lang_Types>();
    for(const auto & lang : langs) {
        make_db_curs("delete from EVENTS_BILINGUAL where id1 = :move_id and lang = :lang and type = :evtDisp ",
            PgOra::getRWSession("EVENTS_BILINGUAL"))
            .stb()
            .bind(":move_id", move_id.get())
            .bind(":lang", lang.code)
            .bind(":evtDisp", EncodeEventType(evtDisp))
            .exec();
    }
}

void arx_events_by_point_id(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    LogTrace(TRACE6) << __FUNCTION__ << " point_id: " << point_id ;
    dbo::Session session;
    std::vector<dbo::Lang_Types> langs = session.query<dbo::Lang_Types>();
    for(const auto & lang : langs) {
        std::vector<dbo::Events_Bilingual> events =  session.query<dbo::Events_Bilingual>()
                .where(" id1 = :point_id and lang = :lang and type in "
                       " (:evtFlt, :evtGraph, :evtFltTask, :evtPax, :evtPay, :evtTlg, :evtPrn)")
                .for_update(true)
                .setBind({{"lang",       lang.code},
                          {"point_id",   point_id.get()},
                          {"evtFlt",     EncodeEventType(evtFlt)},
                          {"evtGraph",   EncodeEventType(evtGraph)},
                          {"evtFltTask", EncodeEventType(evtFltTask)},
                          {"evtPax",     EncodeEventType(evtPax)},
                          {"evtPay",     EncodeEventType(evtPay)},
                          {"evtTlg",     EncodeEventType(evtTlg)},
                          {"evtPrn",     EncodeEventType(evtPrn)},
                         });
        for(const auto & ev : events) {
            dbo::Arx_Events aev(ev, part_key);
            session.insert(aev);
        }
    }
}

void deleteEventsByPointId(const PointId_t& point_id)
{
    LogTrace(TRACE6) << __FUNCTION__ << " point_id: " << point_id ;
    dbo::Session session;
    std::vector<dbo::Lang_Types> langs = session.query<dbo::Lang_Types>();
    for(const auto & lang : langs) {
        make_db_curs("delete from EVENTS_BILINGUAL where id1 = :point_id and lang = :lang "
                     "and type in (:evtFlt, :evtGraph, :evtFltTask, :evtPax, :evtPay, :evtTlg, :evtPrn) ",
                     PgOra::getRWSession("EVENTS_BILINGUAL"))
                .stb()
                .bind(":point_id",   point_id.get())
                .bind(":lang",       lang.code)
                .bind(":evtFlt",     EncodeEventType(evtFlt))
                .bind(":evtGraph",   EncodeEventType(evtGraph))
                .bind(":evtFltTask", EncodeEventType(evtFltTask))
                .bind(":evtPax",     EncodeEventType(evtPax))
                .bind(":evtPay",     EncodeEventType(evtPay))
                .bind(":evtTlg",     EncodeEventType(evtTlg))
                .bind(":evtPrn",     EncodeEventType(evtPrn))
                .exec();
    }
}

void arx_mark_trips(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    LogTrace(TRACE6) << __func__ << " point_id: " << point_id ;
    dbo::Session session;
    std::vector<int> pax_grp_id_marks = session.query<int>("SELECT DISTINCT point_id_mark")
            .from("pax_grp")
            .where("point_dep = :point_id")
            .setBind({{"point_id", point_id.get()}});
    std::vector<dbo::Mark_Trips> mark_trips;
    for(const auto &id_mark : pax_grp_id_marks) {
        std::optional<dbo::Mark_Trips> trip = session.query<dbo::Mark_Trips>()
                .where("point_id = :id_mark")
                .setBind({{"id_mark",id_mark}});
        if(trip) {
            mark_trips.push_back(*trip);
        }
    }
    for(const auto & mark_trip : mark_trips) {
        dbo::Arx_Mark_Trips amt(mark_trip, part_key);
        session.noThrowError(DbCpp::ResultCode::ConstraintFail).insert(amt);
    }
}

void arx_pax_grp(const std::vector<dbo::Pax_Grp> & pax_grps, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    for(const auto &gr : pax_grps) {
        dbo::Arx_Pax_Grp apg(gr, part_key);
        session.insert(apg);
    }
}

std::vector<dbo::Pax_Grp> read_pax_grp(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::Pax_Grp> pax_grps = session.query<dbo::Pax_Grp>()
        .where("point_dep = :point_id")
        .for_update(true)
        .setBind({{"point_id", point_id.get()}});
    return pax_grps;
}

void arx_self_ckin_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::Self_Ckin_Stat> ckin_stats = session.query<dbo::Self_Ckin_Stat>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : ckin_stats) {
        dbo::Arx_Self_Ckin_Stat ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_rfisc_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::RFISC_STAT> rfisc_stats = session.query<dbo::RFISC_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : rfisc_stats) {
        dbo::ARX_RFISC_STAT ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_stat_services(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_SERVICES> stat_services = session.query<dbo::STAT_SERVICES>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_services) {
        dbo::ARX_STAT_SERVICES ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_stat_rem(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_REM> stat_rems = session.query<dbo::STAT_REM>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_rems) {
        dbo::ARX_STAT_REM ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_limited_cap_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::LIMITED_CAPABILITY_STAT> stat_lcs = session.query<dbo::LIMITED_CAPABILITY_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_lcs) {
        dbo::ARX_LIMITED_CAPABILITY_STAT ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_pfs_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::PFS_STAT> stat_pfs = session.query<dbo::PFS_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_pfs) {
        dbo::ARX_PFS_STAT ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_stat_ad(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_AD> stat_ad = session.query<dbo::STAT_AD>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_ad) {
        dbo::ARX_STAT_AD ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_stat_ha(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_HA> stat_ha = session.query<dbo::STAT_HA>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_ha) {
        dbo::ARX_STAT_HA ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_stat_vo(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_VO> stat_vo = session.query<dbo::STAT_VO>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_vo) {
        dbo::ARX_STAT_VO ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_stat_reprint(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_REPRINT> stat_reprint = session.query<dbo::STAT_REPRINT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_reprint) {
        dbo::ARX_STAT_REPRINT ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_trfer_pax_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::TRFER_PAX_STAT> stat_reprint = session.query<dbo::TRFER_PAX_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_reprint) {
        dbo::ARX_TRFER_PAX_STAT ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_bi_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::BI_STAT> bi_stats = session.query<dbo::BI_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : bi_stats) {
        dbo::ARX_BI_STAT ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_agent_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    std::vector<dbo::AGENT_STAT> agents_stat{};
    dbo::Session session(dbo::Postgres);
    agents_stat = PgOra::supportsPg("AGENT_STAT") ?  session.query<dbo::AGENT_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}})
                                                  : dbo::readOraAgentsStat(point_id);

    for(const auto &as : agents_stat) {
        dbo::ARX_AGENT_STAT ascs(as, part_key);
        ascs.part_key = ascs.ondate;
        ascs.point_part_key = part_key;
        session.insert(ascs);
    }
}

void arx_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT> stats = session.query<dbo::STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_STAT ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_trfer_stat(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::TRFER_STAT> stats = session.query<dbo::TRFER_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_TRFER_STAT ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_tlg_out(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::TLG_OUT> stats = session.query<dbo::TLG_OUT>()
        .where("point_id = :point_id AND type<>'LCI'")
        .for_update(true)
        .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        Dates::DateTime_t part_key_nvl = dbo::coalesce(cs.time_send_act, cs.time_send_scd, cs.time_create);
        dbo::ARX_TLG_OUT ascs(cs, part_key_nvl);
        session.insert(ascs);
    }
}

void deleteTlgOut(const PointId_t& point_id)
{
    dbo::Session session;
    std::vector<dbo::TLG_OUT> stats = session.query<dbo::TLG_OUT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});
    for(const auto &cs : stats) {
        make_db_curs("delete from TYPEB_OUT_EXTRA where tlg_id = :tlg_id ",
                     PgOra::getRWSession("TYPEB_OUT_EXTRA")).bind(":tlg_id", cs.id).exec();
        make_db_curs("delete from TYPEB_OUT_ERRORS where tlg_id = :tlg_id ",
                     PgOra::getRWSession("TYPEB_OUT_ERRORS")).bind(":tlg_id", cs.id).exec();
        make_db_curs("delete from TLG_OUT where id = :id ",
                     PgOra::getRWSession("TLG_OUT")).bind(":id", cs.id).exec();
    }
}

void arx_trip_classes(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::TRIP_CLASSES> stats = session.query<dbo::TRIP_CLASSES>()
            .where("point_id = :point_id").setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_TRIP_CLASSES ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_trip_delays(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::TRIP_DELAYS> stats = session.query<dbo::TRIP_DELAYS>()
            .where("point_id = :point_id").setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_TRIP_DELAYS ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_trip_load(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::TRIP_LOAD> stats = session.query<dbo::TRIP_LOAD>()
            .where("point_dep = :point_id").setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_TRIP_LOAD ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_trip_sets(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::TRIP_SETS> trip_sets = session.query<dbo::TRIP_SETS>()
            .where("point_id = :point_id").setBind({{"point_id", point_id.get()}});


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
        ats.part_key = part_key;
        session.insert(ats);
    }
}

void arx_crs_displace2(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::CRS_DISPLACE2> stats = session.query<dbo::CRS_DISPLACE2>()
            .where("point_id_spp = :point_id").setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_CRS_DISPLACE2 ascs(cs, part_key);
        ascs.point_id_tlg = ASTRA::NoExists;
        session.insert(ascs);
    }
}

void arx_trip_stages(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::TRIP_STAGES> stats = session.query<dbo::TRIP_STAGES>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_TRIP_STAGES ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_bag_receipts(const PointId_t& point_id, const Dates::DateTime_t & part_key)
{
    LogTrace1 <<__FUNCTION__ << " point_id: " << point_id;
    dbo::Session session;
    std::vector<dbo::BAG_RECEIPTS> bag_receipts = session.query<dbo::BAG_RECEIPTS>()
            .where("bag_receipts.point_id = :point_id ")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &br : bag_receipts) {
        dbo::ARX_BAG_RECEIPTS abr(br, part_key);
        session.insert(abr);
        arx_bag_pay_types(br.receipt_id, part_key);
    }
}

void arx_bag_pay_types(int receipt_id , const Dates::DateTime_t & part_key)
{
    if(receipt_id != ASTRA::NoExists) {
        dbo::Session session;
        std::vector<dbo::BAG_PAY_TYPES> bag_receipts = session.query<dbo::BAG_PAY_TYPES>()
                .where("receipt_id = :receipt_id")
                .for_update(true)
                .setBind({{"receipt_id", receipt_id}});

        for(const auto &bpt : bag_receipts) {
            dbo::ARX_BAG_PAY_TYPES abpt(bpt, part_key);
            session.insert(abpt);
        }
    }
}

void deleteStatServices(const PointId_t& point_id)
{
    make_db_curs("delete from STAT_SERVICES where point_id = :point_id ",
        PgOra::getRWSession("STAT_SERVICES")).bind(":point_id", point_id.get()).exec();
}

void deleteStatAd(const PointId_t& point_id)
{
    make_db_curs("delete from STAT_AD where point_id = :point_id ",
        PgOra::getRWSession("STAT_AD")).bind(":point_id", point_id.get()).exec();
}
void deleteBiStat(const PointId_t& point_id)
{
    make_db_curs("delete from BI_STAT where point_id = :point_id ",
        PgOra::getRWSession("BI_STAT")).bind(":point_id", point_id.get()).exec();
}

void deleteBagReceipts(const PointId_t& point_id)
{
    LogTrace1 <<__FUNCTION__ << " point_id: " << point_id;
    std::vector<dbo::BAG_RECEIPTS> bag_receipts;
    dbo::BAG_RECEIPTS bag;
    auto cur = make_db_curs("SELECT receipt_id, kit_id FROM bag_receipts "
                            "WHERE point_id = :point_id "
                            "FOR UPDATE",
                            PgOra::getRWSession("BAG_RECEIPTS"));
    cur.def(bag.receipt_id).defNull(bag.kit_id, ASTRA::NoExists).bind(":point_id", point_id.get()).exec();
    while(!cur.fen()) {
        bag_receipts.push_back(bag);
        bag = {};
    }

    for(const auto &br : bag_receipts) {
        make_db_curs("delete from BAG_PAY_TYPES where receipt_id = :receipt_id",
                     PgOra::getRWSession("BAG_PAY_TYPES")).bind(":receipt_id", br.receipt_id).exec();
        make_db_curs("delete from BAG_RECEIPTS where receipt_id = :receipt_id",
                     PgOra::getRWSession("BAG_RECEIPTS")).bind(":receipt_id", br.receipt_id).exec();
    }

    for(const auto & br: bag_receipts) {
        make_db_curs("delete from BAG_RCPT_KITS where kit_id = :kit_id ",
                     PgOra::getRWSession("BAG_RCPT_KITS")).bind(":kit_id", br.kit_id).exec();
    }
}

void arx_annul_bags_tags(const GrpId_t& grp_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::ANNUL_BAG> annul_bags = session.query<dbo::ANNUL_BAG>()
        .where("grp_id = :grp_id").for_update(true).setBind({{"grp_id", grp_id.get()}});

    for(const auto& ab : annul_bags) {
        std::vector<dbo::ANNUL_TAGS> annul_tags = session.query<dbo::ANNUL_TAGS>()
            .where("id = :id").for_update(true).setBind({{"id", ab.id}});
        for(const auto &at : annul_tags) {
            dbo::ARX_ANNUL_TAGS aat(at, part_key);
            session.insert(aat);
        }

        dbo::ARX_ANNUL_BAG ascs(ab, part_key);
        session.insert(ascs);
    }
}

void delete_annul_bags_tags(const GrpId_t& grp_id)
{
    std::vector<dbo::ANNUL_BAG> annul_bags = dbo::Session().query<dbo::ANNUL_BAG>()
        .where("grp_id = :grp_id").for_update(true).setBind({{"grp_id", grp_id.get()}});

    for(const auto &ab : annul_bags) {
        make_db_curs("delete from ANNUL_TAGS where id = :id ", PgOra::getRWSession("ANNUL_TAGS"))
                .bind(":id", ab.id).exec();
        make_db_curs("delete from ANNUL_BAG where id = :id ", PgOra::getRWSession("ANNUL_BAG"))
                .bind(":id", ab.id).exec();
    }
}

void arx_unaccomp_bag_info(const GrpId_t& grp_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::UNACCOMP_BAG_INFO> bag_infos = session.query<dbo::UNACCOMP_BAG_INFO>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : bag_infos) {
        dbo::ARX_UNACCOMP_BAG_INFO ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_bag2(const GrpId_t& grp_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::BAG2> bags2 = session.query<dbo::BAG2>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : bags2) {
        dbo::ARX_BAG2 ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_bag_prepay(const GrpId_t& grp_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::BAG_PREPAY> bag_prepay = session.query<dbo::BAG_PREPAY>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : bag_prepay) {
        dbo::ARX_BAG_PREPAY ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_bag_tags(const GrpId_t& grp_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::BAG_TAGS> bag_tags = session.query<dbo::BAG_TAGS>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : bag_tags) {
        dbo::ARX_BAG_TAGS ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_paid_bag(const GrpId_t& grp_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::PAID_BAG> paid_bag = session.query<dbo::PAID_BAG>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : paid_bag) {
        dbo::ARX_PAID_BAG ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_pay_services(const GrpId_t& grp_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::PAY_SERVICES> services = session.query<dbo::PAY_SERVICES>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &s : services) {
        dbo::ARX_PAY_SERVICES ascs(s, part_key);
        session.insert(ascs);
    }
}

void arx_pax(const std::vector<dbo::PAX>& paxes, const GrpId_t& grp_id,  const PointId_t& point_id,
             const Dates::DateTime_t & part_key)
{
    LogTrace(TRACE6) << __func__ << " grp_id: " << grp_id << " point_id: " << point_id << " part_key: " << part_key;
    dbo::Session session;
    for(const auto &cs : paxes) {
        std::string seat_no = salons::get_seat_no(PaxId_t(cs.pax_id), cs.seats, cs.is_jmp, "", point_id, "one" );
        int excess_pc = countPaidExcessPC(PaxId_t(cs.pax_id));
        if (excess_pc == 0) {
          excess_pc = ASTRA::NoExists;
        }
        dbo::ARX_PAX ascs(cs, part_key, excess_pc, seat_no);
        session.insert(ascs);
    }
}

std::vector<dbo::PAX> read_pax(const GrpId_t& grp_id)
{
    dbo::Session session;
    std::vector<dbo::PAX> paxes = session.query<dbo::PAX>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});
    return paxes;
}

void arx_transfer(const GrpId_t& grp_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::TRANSFER> trfer = session.query<dbo::TRANSFER>()
            .where("grp_id = :grp_id ")
            .for_update(true)
            .setBind({{":grp_id", grp_id.get()}});

    for(const auto& tr : trfer) {
        if(tr.transfer_num > 0) {
            std::optional<dbo::TRFER_TRIPS> trip = session.query<dbo::TRFER_TRIPS>()
                    .where("point_id = :tp")
                    .for_update(true)
                    .setBind({{":tp", tr.point_id_trfer}});

            if(trip){
                dbo::ARX_TRANSFER atr(tr, *trip, part_key);
                session.insert(atr);
            }
        }
    }
}

void arx_tckin_segments(const GrpId_t& grp_id, const Dates::DateTime_t & part_key)
{
    LogTrace(TRACE3) << __FUNCTION__ << " grp_id: " << grp_id;
    dbo::Session session;
    std::vector<dbo::TCKIN_SEGMENTS> segs = session.query<dbo::TCKIN_SEGMENTS>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{":grp_id", grp_id.get()}});

    for(const auto& s : segs) {
        if(s.seg_no > 0) {
            std::optional<dbo::TRFER_TRIPS> trip = session.query<dbo::TRFER_TRIPS>()
                    .where("point_id = :tp")
                    .for_update(true)
                    .setBind({{":tp", s.point_id_trfer}});

            if(trip) {
                dbo::ARX_TCKIN_SEGMENTS atr(s, *trip, part_key);
                session.insert(atr);
            }
        }
    }
}

size_t delete_trfer_trips(const GrpId_t& grp_id)
{
    LogTrace(TRACE6) << __func__;
    size_t row_count = 0;
    dbo::Session session;
    std::vector<dbo::TRANSFER> trfers = session.query<dbo::TRANSFER>().where("grp_id = :grp_id ")
        .for_update(true).setBind({{":grp_id", grp_id.get()}});
    std::vector<dbo::TCKIN_SEGMENTS> segs = session.query<dbo::TCKIN_SEGMENTS>().where("grp_id = :grp_id")
        .for_update(true).setBind({{":grp_id", grp_id.get()}});

    std::set<int> tr_point_ids   = algo::transform<std::set>(trfers, [&](auto tr){return tr.point_id_trfer;});
    std::set<int> segm_point_ids = algo::transform<std::set>(segs,   [&](auto ts){return ts.point_id_trfer;});
    tr_point_ids.insert(begin(segm_point_ids), end(segm_point_ids));

    make_db_curs("delete from TRANSFER where grp_id = :grp_id ",
                 PgOra::getRWSession("TRANSFER")).bind(":grp_id", grp_id.get()).exec();
    make_db_curs("delete from TCKIN_SEGMENTS where grp_id = :grp_id ",
                 PgOra::getRWSession("TCKIN_SEGMENTS")).bind(":grp_id", grp_id.get()).exec();

    for(const auto & point_id_trfer : tr_point_ids) {
        auto cur = make_db_curs(
"delete from TRFER_TRIPS where POINT_ID=:point_id_trfer AND "
"NOT EXISTS (SELECT transfer.point_id_trfer FROM transfer WHERE  transfer.point_id_trfer=:point_id_trfer) AND "
"NOT EXISTS (SELECT tckin_segments.point_id_trfer FROM tckin_segments WHERE  tckin_segments.point_id_trfer=:point_id_trfer) ",
    PgOra::getRWSession("TRFER_TRIPS"));
        cur.bind(":point_id_trfer", point_id_trfer).exec();
        row_count += cur.rowcount();
    }
    return row_count;
}

void arx_value_bag(const GrpId_t& grp_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::VALUE_BAG> bags2 = session.query<dbo::VALUE_BAG>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : bags2) {
        dbo::ARX_VALUE_BAG ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_grp_norms(const GrpId_t& grp_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::GRP_NORMS> norms = session.query<dbo::GRP_NORMS>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : norms) {
        dbo::ARX_GRP_NORMS ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_pax_norms(const PaxId_t& pax_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::PAX_NORMS> norms = session.query<dbo::PAX_NORMS>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});

    for(const auto &cs : norms) {
        dbo::ARX_PAX_NORMS ascs(cs, part_key);
        session.insert(ascs);
    }
}


void arx_pax_rem(const PaxId_t& pax_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::PAX_REM> norms = session.query<dbo::PAX_REM>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});

    for(const auto &cs : norms) {
        dbo::ARX_PAX_REM ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_transfer_subcls(const PaxId_t& pax_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::TRANSFER_SUBCLS> norms = session.query<dbo::TRANSFER_SUBCLS>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});

    for(const auto &cs : norms) {
        dbo::ARX_TRANSFER_SUBCLS ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_pax_doc(const PaxId_t& pax_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::PAX_DOC> docs = session.query<dbo::PAX_DOC>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});
    LogTrace(TRACE6) << " docs found : " << docs.size();

    for(const auto &d : docs) {
        dbo::ARX_PAX_DOC ad(d, part_key);
        session.insert(ad);
    }
}

void arx_pax_doco(const PaxId_t& pax_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::PAX_DOCO> norms = session.query<dbo::PAX_DOCO>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});

    for(const auto &cs : norms) {
        dbo::ARX_PAX_DOCO ascs(cs, part_key);
        session.insert(ascs);
    }
}

void arx_pax_doca(const PaxId_t& pax_id, const Dates::DateTime_t & part_key)
{
    dbo::Session session;
    std::vector<dbo::PAX_DOCA> norms = session.query<dbo::PAX_DOCA>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});

    for(const auto &cs : norms) {
        dbo::ARX_PAX_DOCA ascs(cs, part_key);
        session.insert(ascs);
    }
}

void deleteByPaxes(const PaxId_t& pax_id)
{
    if(ARX::CLEANUP_PG()) {
        make_db_curs("delete from PAX_NORMS where pax_id = :pax_id ",        PgOra::getRWSession("PAX_NORMS")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_REM where pax_id = :pax_id ",          PgOra::getRWSession("PAX_REM")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from TRANSFER_SUBCLS where pax_id = :pax_id ",  PgOra::getRWSession("TRANSFER_SUBCLS")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_DOC where pax_id = :pax_id ",          PgOra::getRWSession("PAX_DOC")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_DOCO where pax_id = :pax_id ",         PgOra::getRWSession("PAX_DOCO")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_DOCA where pax_id = :pax_id ",         PgOra::getRWSession("PAX_DOCA")).bind(":pax_id", pax_id.get()).exec();

        make_db_curs("delete from PAX_EVENTS WHERE pax_id=:pax_id ",         PgOra::getRWSession("PAX_EVENTS")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from STAT_AD WHERE pax_id=:pax_id ",            PgOra::getRWSession("STAT_AD")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from CONFIRM_PRINT WHERE pax_id=:pax_id ",      PgOra::getRWSession("CONFIRM_PRINT")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_FQT WHERE pax_id=:pax_id ",            PgOra::getRWSession("PAX_FQT")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_ASVC WHERE pax_id=:pax_id ",           PgOra::getRWSession("PAX_ASVC")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_EMD WHERE pax_id=:pax_id ",            PgOra::getRWSession("PAX_EMD")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_BRANDS WHERE pax_id=:pax_id ",         PgOra::getRWSession("PAX_BRANDS")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_REM_ORIGIN WHERE pax_id=:pax_id ",     PgOra::getRWSession("PAX_REM_ORIGIN")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_SEATS WHERE pax_id=:pax_id ",          PgOra::getRWSession("PAX_SEATS")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from ROZYSK WHERE pax_id=:pax_id ",             PgOra::getRWSession("ROZYSK")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from TRIP_COMP_LAYERS WHERE pax_id=:pax_id ",   PgOra::getRWSession("TRIP_COMP_LAYERS")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_CUSTOM_ALARMS WHERE pax_id=:pax_id ",  PgOra::getRWSession("PAX_CUSTOM_ALARMS")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_SERVICE_LISTS WHERE pax_id=:pax_id ",  PgOra::getRWSession("PAX_SERVICE_LISTS")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_SERVICES WHERE pax_id=:pax_id ",       PgOra::getRWSession("PAX_SERVICES")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_SERVICES_AUTO WHERE pax_id=:pax_id ",  PgOra::getRWSession("PAX_SERVICES_AUTO")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAID_RFISC WHERE pax_id=:pax_id ",         PgOra::getRWSession("PAID_RFISC")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_NORMS_TEXT WHERE pax_id=:pax_id ",     PgOra::getRWSession("PAX_NORMS_TEXT")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from SBDO_TAGS_GENERATED WHERE pax_id=:pax_id ",PgOra::getRWSession("SBDO_TAGS_GENERATED")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_CALC_DATA WHERE pax_calc_data_id=:pax_id ",PgOra::getRWSession("PAX_CALC_DATA")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_CONFIRMATIONS WHERE pax_id=:pax_id ",  PgOra::getRWSession("PAX_CONFIRMATIONS")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("update SERVICE_PAYMENT SET pax_id=NULL WHERE pax_id=:pax_id ",PgOra::getRWSession("SERVICE_PAYMENT")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX_ALARMS where pax_id=:pax_id ",         PgOra::getRWSession("PAX_ALARMS")).bind(":pax_id", pax_id.get()).exec();
        make_db_curs("delete from PAX WHERE pax_id=:pax_id ",                PgOra::getRWSession("PAX")).bind(":pax_id", pax_id.get()).exec();
    }
}

void deleteByGrpId(const GrpId_t& grp_id)
{
    if(ARX::CLEANUP_PG()) {
        delete_trfer_trips(grp_id);

        make_db_curs("delete from UNACCOMP_BAG_INFO where grp_id = :grp_id ",PgOra::getRWSession("UNACCOMP_BAG_INFO")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from BAG2 where grp_id = :grp_id ",             PgOra::getRWSession("BAG2")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from BAG_PREPAY where grp_id = :grp_id ",       PgOra::getRWSession("BAG_PREPAY")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from BAG_TAGS where grp_id = :grp_id ",         PgOra::getRWSession("BAG_TAGS")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from PAID_BAG where grp_id = :grp_id ",         PgOra::getRWSession("PAID_BAG")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from PAY_SERVICES where grp_id = :grp_id ",     PgOra::getRWSession("PAY_SERVICES")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from VALUE_BAG where grp_id = :grp_id ",        PgOra::getRWSession("VALUE_BAG")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from GRP_NORMS where grp_id = :grp_id ",        PgOra::getRWSession("GRP_NORMS")).bind(":grp_id", grp_id.get()).exec();

        make_db_curs("delete from PAID_BAG_EMD_PROPS where grp_id=:grp_id ", PgOra::getRWSession("PAID_BAG_EMD_PROPS")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from SERVICE_PAYMENT where grp_id=:grp_id ",    PgOra::getRWSession("SERVICE_PAYMENT")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from TCKIN_PAX_GRP where grp_id=:grp_id ",      PgOra::getRWSession("TCKIN_PAX_GRP")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from PNR_ADDRS_PC where grp_id=:grp_id ",       PgOra::getRWSession("PNR_ADDRS_PC")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from GRP_SERVICE_LISTS where grp_id=:grp_id ",  PgOra::getRWSession("GRP_SERVICE_LISTS")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from BAG_TAGS_GENERATED where grp_id=:grp_id ", PgOra::getRWSession("BAG_TAGS_GENERATED")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from MPS_EXCHANGE where grp_id=:grp_id",        PgOra::getRWSession("MPS_EXCHANGE")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from SVC_PRICES where grp_id=:grp_id",          PgOra::getRWSession("SVC_PRICES")).bind(":grp_id", grp_id.get()).exec();
        make_db_curs("delete from PAX_GRP where grp_id=:grp_id",             PgOra::getRWSession("PAX_GRP")).bind(":grp_id", grp_id.get()).exec();
    }
}

void deleteAodbBag(const PointId_t& point_id)
{
    if(ARX::CLEANUP_PG()) {
        make_db_curs("delete from AODB_BAG where EXISTS (select AODB_PAX.PAX_ID, AODB_PAX.POINT_ADDR "
                     "from AODB_PAX where AODB_PAX.POINT_ID=:point_id AND AODB_PAX.PAX_ID = AODB_BAG.PAX_ID "
                     "AND AODB_PAX.POINT_ADDR = AODB_BAG.POINT_ADDR) ",
                     PgOra::getRWSession("AODB_BAG")).bind(":point_id", point_id.get()).exec();
    }
}

void deleteTransferPaxStat(const PointId_t& point_id)
{
    make_db_curs("delete from TRFER_PAX_STAT where point_id = :point_id ",
                 PgOra::getRWSession("TRFER_PAX_STAT")).bind(":point_id", point_id.get()).exec();
}

void deleteByPointId(const PointId_t& point_id)
{
    LogTrace(TRACE6) << __func__ << " point_id: " << point_id;
    CheckIn::TCrsCountersMap::deleteCrsCountersOnly(point_id);
    if(ARX::CLEANUP_PG()) {
        deleteAodbBag(point_id);
        deleteTlgOut(point_id);
        deleteEventsByPointId(point_id);
        make_db_curs("delete from SELF_CKIN_STAT where point_id = :point_id ",PgOra::getRWSession("SELF_CKIN_STAT")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from RFISC_STAT where point_id = :point_id ",PgOra::getRWSession("RFISC_STAT")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from STAT_REM where point_id = :point_id ",PgOra::getRWSession("STAT_REM")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from LIMITED_CAPABILITY_STAT where point_id = :point_id ",PgOra::getRWSession("LIMITED_CAPABILITY_STAT")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from PFS_STAT where point_id = :point_id ",PgOra::getRWSession("PFS_STAT")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from STAT_HA where point_id = :point_id ",PgOra::getRWSession("STAT_HA")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from STAT_VO where point_id = :point_id ",PgOra::getRWSession("STAT_VO")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from STAT_REPRINT where point_id = :point_id ",PgOra::getRWSession("STAT_REPRINT")).bind(":point_id", point_id.get()).exec();

        make_db_curs("delete from AGENT_STAT where point_id = :point_id ",PgOra::getRWSession("AGENT_STAT")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from STAT where point_id = :point_id ",PgOra::getRWSession("STAT")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from TRFER_STAT where point_id = :point_id ",PgOra::getRWSession("TRFER_STAT")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from TRIP_CLASSES WHERE point_id=:point_id" , PgOra::getRWSession("TRIP_CLASSES")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from TRIP_DELAYS WHERE point_id=:point_id" , PgOra::getRWSession("trip_delays")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from TRIP_LOAD WHERE point_dep=:point_id" , PgOra::getRWSession("trip_load")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from TRIP_SETS WHERE point_id=:point_id" , PgOra::getRWSession("trip_sets")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from CRS_DISPLACE2 WHERE point_id_spp=:point_id" , PgOra::getRWSession("CRS_DISPLACE2")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from TRIP_STAGES where point_id = :point_id ",PgOra::getRWSession("TRIP_STAGES")).bind(":point_id", point_id.get()).exec();

        make_db_curs("delete from aodb_pax_change WHERE point_id=:point_id" , PgOra::getRWSession("aodb_pax_change")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from aodb_unaccomp WHERE point_id=:point_id" , PgOra::getRWSession("aodb_unaccomp")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from aodb_pax WHERE point_id=:point_id" , PgOra::getRWSession("aodb_pax")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from aodb_points WHERE point_id=:point_id" , PgOra::getRWSession("aodb_points")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from exch_flights WHERE point_id=:point_id" , PgOra::getRWSession("exch_flights")).bind(":point_id", point_id.get()).exec();
        make_db_curs("delete from counters2 WHERE point_dep=:point_id" , PgOra::getRWSession("counters2")).bind(":point_id", point_id.get()).exec();

        make_db_curs("DELETE FROM snapshot_points WHERE point_id=:point_id" , PgOra::getRWSession("snapshot_points")).bind(":point_id", point_id.get()).exec();
        make_db_curs("UPDATE tag_ranges2 SET point_id=NULL WHERE point_id=:point_id" , PgOra::getRWSession("tag_ranges2")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM tlg_binding WHERE point_id_spp=:point_id" , PgOra::getRWSession("tlg_binding")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_bp WHERE point_id=:point_id" , PgOra::getRWSession("trip_bp")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_hall WHERE point_id=:point_id" , PgOra::getRWSession("trip_hall")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_bt WHERE point_id=:point_id" , PgOra::getRWSession("trip_bt")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_ckin_client WHERE point_id=:point_id" , PgOra::getRWSession("trip_ckin_client")).bind(":point_id", point_id.get()).exec();

        make_db_curs("DELETE FROM trip_comp_rem WHERE point_id=:point_id" , PgOra::getRWSession("trip_comp_rem")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_comp_rates WHERE point_id=:point_id" , PgOra::getRWSession("trip_comp_rates")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_comp_rfisc WHERE point_id=:point_id" , PgOra::getRWSession("trip_comp_rfisc")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_comp_baselayers WHERE point_id=:point_id" , PgOra::getRWSession("TRIP_COMP_BASELAYERS")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_comp_elems WHERE point_id=:point_id" , PgOra::getRWSession("trip_comp_elems")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_comp_layers WHERE point_id=:point_id" , PgOra::getRWSession("trip_comp_layers")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_crew WHERE point_id=:point_id" , PgOra::getRWSession("trip_crew")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_data WHERE point_id=:point_id" , PgOra::getRWSession("trip_data")).bind(":point_id", point_id.get()).exec();

        make_db_curs("DELETE FROM trip_final_stages WHERE point_id=:point_id" , PgOra::getRWSession("trip_final_stages")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_stations WHERE point_id=:point_id" , PgOra::getRWSession("trip_stations")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_paid_ckin WHERE point_id=:point_id" , PgOra::getRWSession("trip_paid_ckin")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_calc_data WHERE point_id=:point_id" , PgOra::getRWSession("trip_calc_data")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_pers_weights WHERE point_id=:point_id" , PgOra::getRWSession("trip_pers_weights")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_auto_weighing WHERE point_id=:point_id" , PgOra::getRWSession("trip_auto_weighing")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM pax_seats WHERE point_id=:point_id"       , PgOra::getRWSession("pax_seats")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_tasks WHERE point_id=:point_id"       , PgOra::getRWSession("trip_tasks")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM counters_by_subcls WHERE point_id=:point_id"       , PgOra::getRWSession("counters_by_subcls")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_vouchers WHERE point_id=:point_id"            , PgOra::getRWSession("trip_vouchers")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM confirm_print_vo_unreg WHERE point_id = :point_id", PgOra::getRWSession("confirm_print_vo_unreg")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM hotel_acmd_pax WHERE point_id = :point_id"        , PgOra::getRWSession("hotel_acmd_pax")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM hotel_acmd_free_pax WHERE point_id = :point_id"   , PgOra::getRWSession("hotel_acmd_free_pax")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM hotel_acmd_dates WHERE point_id = :point_id"      , PgOra::getRWSession("hotel_acmd_dates")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_alarms WHERE point_id=:point_id"               , PgOra::getRWSession("TRIP_ALARMS")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_rpt_person WHERE point_id=:point_id"           , PgOra::getRWSession("TRIP_RPT_PERSON")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM trip_apis_params WHERE point_id=:point_id"          , PgOra::getRWSession("TRIP_APIS_PARAMS")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM apps_messages WHERE msg_id in (SELECT cirq_msg_id FROM APPS_PAX_DATA where point_id= :point_id)",
                     PgOra::getRWSession("APPS_MESSAGES")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM apps_messages WHERE msg_id in (SELECT cicx_msg_id FROM APPS_PAX_DATA where point_id=:point_id)",
                     PgOra::getRWSession("APPS_MESSAGES")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM apps_messages WHERE msg_id in (SELECT msg_id FROM APPS_MANIFEST_DATA where point_id=:point_id)",
                     PgOra::getRWSession("APPS_MESSAGES")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM apps_pax_data WHERE point_id=:point_id", PgOra::getRWSession("APPS_PAX_DATA")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM apps_manifest_data WHERE point_id=:point_id", PgOra::getRWSession("APPS_MANIFEST_DATA")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM iapi_pax_data WHERE point_id=:point_id"             , PgOra::getRWSession("IAPI_PAX_DATA")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM utg_prl WHERE point_id=:point_id"                   , PgOra::getRWSession("UTG_PRL")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM wb_msg_text where id in (SELECT id FROM wb_msg WHERE point_id = :point_id)", PgOra::getRWSession("WB_MSG_TEXT")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM wb_msg where point_id = :point_id",                   PgOra::getRWSession("WB_MSG")).bind(":point_id", point_id.get()).exec();
        make_db_curs("UPDATE trfer_trips SET point_id_spp=NULL WHERE point_id_spp=:point_id ", PgOra::getRWSession("trfer_trips")).bind(":point_id", point_id.get()).exec();
        make_db_curs("DELETE FROM del_vo WHERE point_id=:point_id",                    PgOra::getRWSession("DEL_VO")).bind(":point_id", point_id.get()).exec();
    }
    deleteEtickets(point_id.get());
    deleteEmdocs(point_id.get());
}

void deleteByMoveId(const MoveId_t & move_id)
{
    LogTrace(TRACE6) << __func__ << " move_id: " << move_id;
    if(ARX::CLEANUP_PG()) {
        make_db_curs("DELETE FROM points WHERE move_id=:move_id",   PgOra::getRWSession("POINTS")).bind(":move_id", move_id.get()).exec();
        make_db_curs("DELETE FROM move_ref WHERE move_id=:move_id", PgOra::getRWSession("MOVE_REF")).bind(":move_id", move_id.get()).exec();
        deleteEventsByMoveId(move_id);
    }
}

//============================= TArxMoveNoFlt =============================
//STEP 2
int arx_tlgout_noflt(const Dates::DateTime_t& arx_date, int remain_rows)
{
    LogTrace(TRACE6) << __func__ << " arx_date: " << arx_date;
    dbo::Session session;

    std::vector<dbo::TLG_OUT> tlg_outs = session.query<dbo::TLG_OUT>()
            .where("POINT_ID is null and TIME_CREATE < :arx_date")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});


    for(const auto &tg : tlg_outs) {
        Dates::DateTime_t part_key_nvl = dbo::coalesce(tg.time_send_act, tg.time_send_scd, tg.time_create);
        dbo::ARX_TLG_OUT ascs(tg, part_key_nvl);
        session.insert(ascs);
    }

    if(ARX::CLEANUP_PG()) {
        for(const auto & tg: tlg_outs) {
            make_db_curs("DELETE FROM typeb_out_extra WHERE tlg_id=:id", PgOra::getRWSession("TYPEB_OUT_EXTRA")).bind(":id", tg.id).exec();
            make_db_curs("DELETE FROM typeb_out_errors WHERE tlg_id=:id", PgOra::getRWSession("TYPEB_OUT_ERRORS")).bind(":id", tg.id).exec();
            make_db_curs("DELETE FROM tlg_out WHERE id=:id", PgOra::getRWSession("TLG_OUT")).bind(":id", tg.id).exec();
        }
    }

    return tlg_outs.size();
}

int arx_events_noflt2(const Dates::DateTime_t& arx_date, int remain_rows)
{
    LogTrace(TRACE6) << __func__ << " arx_date: " << arx_date;
    dbo::Session session;
    std::vector<dbo::Events_Bilingual> events =  session.query<dbo::Events_Bilingual>()
            .where("ID1 is NULL  and TYPE = :evtTlg and TIME >= :elapsed and TIME < :arx_date")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date},
                      {":elapsed", arx_date - Dates::days(30)},
                      {":evtTlg", EncodeEventType(evtTlg)},
                      {":remain_rows", remain_rows}});

    for(const auto & ev : events) {
        dbo::Arx_Events aev(ev, ev.time);
        session.insert(aev);
    }
    if(ARX::CLEANUP_PG()) {
        for(const auto & ev : events) {
            make_db_curs("DELETE FROM events_bilingual WHERE ID1 is NULL  and type = :type and TIME = :time",
                         PgOra::getRWSession("EVENTS_BILINGUAL"))
                    .bind(":type", ev.type)
                    .bind(":time", ev.time)
                    .exec();
        }
    }
    return events.size();
}

int arx_events_noflt3(const Dates::DateTime_t& arx_date, int remain_rows)
{
    LogTrace(TRACE6) << __func__ << " arx_date: " << arx_date;
    dbo::Session session;
    std::vector<dbo::Events_Bilingual> events =  session.query<dbo::Events_Bilingual>()
            .where("TIME >= :arx_date - 30 and TIME < :arx_date and type not in "
                   " (:evtSeason, :evtDisp, :evtFlt, :evtGraph, :evtFltTask, :evtPax, :evtPay, :evtTlg, :evtPrn)")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows},
                      {":evtSeason",  EncodeEventType(evtSeason)},
                      {":evtDisp",    EncodeEventType(evtDisp)},
                      {":evtFlt",     EncodeEventType(evtFlt)},
                      {":evtGraph",   EncodeEventType(evtGraph)},
                      {":evtFltTask", EncodeEventType(evtFltTask)},
                      {":evtPax",     EncodeEventType(evtPax)},
                      {":evtPay",     EncodeEventType(evtPay)},
                      {":evtTlg",     EncodeEventType(evtTlg)},
                      {":evtPrn",     EncodeEventType(evtPrn)}});
    for(const auto & ev : events) {
        dbo::Arx_Events aev(ev, ev.time);
        session.insert(aev);
    }

    if(ARX::CLEANUP_PG()) {
        for(const auto & ev : events) {
            make_db_curs("DELETE FROM events_bilingual WHERE type = :type and TIME = :time",
                         PgOra::getRWSession("EVENTS_BILINGUAL"))
                    .bind(":type", ev.type)
                    .bind(":time", ev.time)
                    .exec();
        }
    }
    return events.size();
}

int arx_stat_zamar(const Dates::DateTime_t& arx_date, int remain_rows)
{
    LogTrace(TRACE6) << __func__ << " arx_date: " << arx_date;
    dbo::Session session;
    std::vector<dbo::STAT_ZAMAR> stats =  session.query<dbo::STAT_ZAMAR>()
            .where("TIME < :arx_date")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});
    for(const auto & s : stats) {
        dbo::ARX_STAT_ZAMAR as(s, s.time);
        session.insert(as);
    }
    if(ARX::CLEANUP_PG()) {
        for(const auto & s : stats) {
            make_db_curs("DELETE FROM STAT_ZAMAR WHERE TIME = :time",PgOra::getRWSession("STAT_ZAMAR"))
                    .bind(":time", s.time)
                    .exec();
        }
    }

    return stats.size();
}

void move_noflt(const Dates::DateTime_t& arx_date, int max_rows, int time_duration, int& step)
{
    LogTrace(TRACE6) << __FUNCTION__ << " arx_date: " << arx_date;
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

bool TArxMoveNoFlt::Next(size_t max_rows, int duration)
{
    LogTrace(TRACE6) << __func__;
    if(step <= 0) {
        step = 1;
    }
    Dates::DateTime_t arx_date = utcdate-Dates::days(ARX::ARX_DAYS());
    HelpCpp::Timer timer;
    move_noflt(arx_date, max_rows, duration, step);
    ASTRA::commitAndCallCommitHooks();
    LogTrace(TRACE6) << " MoveNoFlt time: " << timer.elapsedSeconds();
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
    point_ids_count=0;
};

void TArxTlgTrips::readTlgTripPoints(const Dates::DateTime_t& arx_date, size_t max_rows)
{
    tlg_trip_points.reserve(max_rows);
    int point_id_tlg;
    auto cur = make_db_curs("SELECT point_id "
                            "FROM tlg_trips "
                            "WHERE scd<:arx_date FETCH FIRST :max_rows ROWS ONLY ",
                            PgOra::getROSession("TLG_TRIPS"));
    cur.def(point_id_tlg)
       .bind(":arx_date", arx_date)
       .bind(":max_rows", max_rows)
       .exec();
    while(!cur.fen()) {
        const std::set<PointId_t> point_id_set = getPointIdsSppByPointIdTlg(PointIdTlg_t(point_id_tlg));
        if(point_id_set.empty()) {
            tlg_trip_points.emplace_back(point_id_tlg);
        }
    }
}

void arx_tlg_trip(const PointIdTlg_t& point_id)
{
    HelpCpp::Timer timer;
    if(ARX::CLEANUP_PG()) {
        LogTrace(TRACE6) << __func__ << " point_id: " << point_id;
        TypeB::deleteTypeBData(point_id);
        TypeB::deleteTypeBDataStat(point_id);
        TypeB::nullCrsDisplace2_point_id_tlg(point_id);
        TypeB::deleteTlgCompLayers(point_id);
        TypeB::deleteCrsDataStat(point_id);
        TrferList::deleteTransferData(point_id);
    }
    LogTrace(TRACE6) << " arx_tlg_trip time: " << timer.elapsedSeconds();
}


bool TArxTlgTrips::Next(size_t max_rows, int duration)
{
    if(tlg_trip_points.empty()) {
        HelpCpp::Timer timer;
        readTlgTripPoints(utcdate-Dates::days(ARX::ARX_DAYS()), max_rows);
        LogTrace(TRACE6) << " readTlgTripPoints time: " << timer.elapsedSeconds();
    }
    HelpCpp::Timer timer;
    while (!tlg_trip_points.empty())
    {
        PointIdTlg_t point_id = tlg_trip_points.front();
        tlg_trip_points.erase(tlg_trip_points.begin());
        try
        {
            arx_tlg_trip(point_id);
            ASTRA::commitAndCallCommitHooks();
            proc_count++;
        }
        catch(...)
        {
            ProgError( STDLOG, "tlg_trips.point_id=%d", point_id.get() );
            throw;
        };
    };
    LogTrace(TRACE6) << " TArxTlgTrips time: " << timer.elapsedSeconds();
    return false;
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

void TArxTypeBIn::readTlgIds(const Dates::DateTime_t& arx_date, size_t max_rows)
{
    LogTrace(TRACE6) << __func__ << " arx_date: " << arx_date;
    int tlg_id;
    Dates::DateTime_t time_receive;
    auto cur = make_db_curs(
                "SELECT id,time_receive "
                "FROM tlgs_in "
                "WHERE time_receive < :arx_date AND "
                "      NOT EXISTS(SELECT * FROM tlg_source WHERE tlg_source.tlg_id=tlgs_in.id) AND "
                "      NOT EXISTS(SELECT * FROM tlgs_in a WHERE a.id=tlgs_in.id AND time_receive >= :arx_date) ",
                PgOra::getROSession({"TLGS_IN", "TLG_SOURCE"}));
    cur.def(tlg_id)
       .def(time_receive)
       .bind(":arx_date", arx_date)
       .exec();
    while(!cur.fen()) {
        if(tlg_ids.size() == max_rows) {
            break;
        }
        tlg_ids.try_emplace(tlg_id, time_receive);
    }
}

void move_typeb_in(int tlg_id)
{
    if(ARX::CLEANUP_PG()) {
        make_db_curs("DELETE FROM typeb_in_body WHERE id=:id",            PgOra::getRWSession("TYPEB_IN_BODY")).bind(":id", tlg_id).exec();
        make_db_curs("DELETE FROM typeb_in_errors WHERE tlg_id=:id",      PgOra::getRWSession("TYPEB_IN_ERRORS")).bind(":id", tlg_id).exec();
        make_db_curs("DELETE FROM typeb_in_history WHERE prev_tlg_id=:id",PgOra::getRWSession("TYPEB_IN_HISTORY")).bind(":id", tlg_id).exec();
        make_db_curs("DELETE FROM typeb_in_history WHERE tlg_id=:id",     PgOra::getRWSession("TYPEB_IN_HISTORY")).bind(":id", tlg_id).exec();
        make_db_curs("DELETE FROM tlgs_in WHERE id=:id",                  PgOra::getRWSession("TLGS_IN")).bind(":id", tlg_id).exec();
        make_db_curs("DELETE FROM typeb_in WHERE id=:id",                 PgOra::getRWSession("TYPEB_IN")).bind(":id", tlg_id).exec();
    }
}

bool TArxTypeBIn::Next(size_t max_rows, int duration)
{
    LogTrace(TRACE6) << __func__;
    if(tlg_ids.empty()) {
        HelpCpp::Timer timer;
        readTlgIds(utcdate - Dates::days(ARX::ARX_DAYS()), max_rows);
        LogTrace(TRACE6) << " readTlgIds time: " << timer.elapsedSeconds();
    }
    HelpCpp::Timer timer;
    while (!tlg_ids.empty())
    {
        int tlg_id = tlg_ids.begin()->first;
        tlg_ids.erase(tlg_ids.begin());

        try
        {
            //в архив
            move_typeb_in(tlg_id);
            ASTRA::commitAndCallCommitHooks();
            proc_count++;
        }
        catch(...)
        {
            ProgError( STDLOG, "typeb_in.id=%d", tlg_id );
            throw;
        };
    };
    LogTrace(TRACE6) << " TArxTypeBIn time: " << timer.elapsedSeconds();
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
    dbo::Session session;

    std::vector<dbo::BAG_NORMS> bag_norms = session.query<dbo::BAG_NORMS>()
            .where("last_date < :arx_date AND "
                   "NOT EXISTS (SELECT * FROM pax_norms WHERE pax_norms.norm_id=bag_norms.id FETCH FIRST 1 ROWS ONLY) AND "
                   "NOT EXISTS (SELECT * FROM grp_norms WHERE grp_norms.norm_id=bag_norms.id FETCH FIRST 1 ROWS ONLY)")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date},
                      {":remain_rows", remain_rows}});

    for(const auto & bn : bag_norms) {
        dbo::ARX_BAG_NORMS abn(bn, bn.last_date);
        session.insert(abn);

        if(ARX::CLEANUP_PG()) {
            make_db_curs("DELETE FROM BAG_NORMS WHERE id = :id", PgOra::getRWSession("BAG_NORMS")).bind(":id", bn.id).exec();
        }
    }
    return bag_norms.size();
}

int arx_bag_rates(const Dates::DateTime_t& arx_date, int remain_rows)
{
    dbo::Session session;

    std::vector<dbo::BAG_RATES> bag_rates = session.query<dbo::BAG_RATES>()
            .where("last_date < :arx_date AND "
                   "NOT EXISTS (SELECT * FROM paid_bag WHERE paid_bag.rate_id=bag_rates.id FETCH FIRST 1 ROWS ONLY)")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date},
                      {":remain_rows", remain_rows}});

    for(const auto & bn : bag_rates) {
        dbo::ARX_BAG_RATES abn(bn, bn.last_date);
        session.insert(abn);
        if(ARX::CLEANUP_PG()) {
            make_db_curs("DELETE FROM BAG_RATES WHERE id = :id", PgOra::getRWSession("BAG_RATES")).bind(":id", bn.id).exec();
        }
    }
    return bag_rates.size();
}

int arx_value_bag_taxes(const Dates::DateTime_t& arx_date, int remain_rows)
{
    dbo::Session session;

    std::vector<dbo::VALUE_BAG_TAXES> bag_taxes = session.query<dbo::VALUE_BAG_TAXES>()
            .where("last_date < :arx_date AND "
                   "NOT EXISTS ("
                   "SELECT * FROM value_bag "
                   "WHERE value_bag.tax_id=value_bag_taxes.id "
                   "FETCH FIRST 1 ROWS ONLY)")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date},
                      {":remain_rows", remain_rows}});

    for(const auto & bn : bag_taxes) {
        dbo::ARX_VALUE_BAG_TAXES abn(bn, bn.last_date);
        session.insert(abn);
        if(ARX::CLEANUP_PG()) {
            make_db_curs("DELETE FROM VALUE_BAG_TAXES WHERE id = :id", PgOra::getRWSession("VALUE_BAG_TAXES")).bind(":id", bn.id).exec();
        }
    }
    return bag_taxes.size();
}

int arx_exchange_rates(const Dates::DateTime_t& arx_date, int remain_rows)
{
    dbo::Session session;

    std::vector<dbo::EXCHANGE_RATES> exc_rates = session.query<dbo::EXCHANGE_RATES>()
            .where("last_date < :arx_date")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date},
                      {":remain_rows", remain_rows}});

    for(const auto & bn : exc_rates) {
        dbo::ARX_EXCHANGE_RATES abn(bn, bn.last_date);
        session.insert(abn);
        if(ARX::CLEANUP_PG()) {
            make_db_curs("DELETE FROM EXCHANGE_RATES WHERE id = :id", PgOra::getRWSession("EXCHANGE_RATES")).bind(":id", bn.id).exec();
        }
    }
    return exc_rates.size();
}

int delete_from_mark_trips(const Dates::DateTime_t& arx_date, int remain_rows)
{
    dbo::Session session;

    std::vector<dbo::Mark_Trips> mark_trips = session.query<dbo::Mark_Trips>()
            .where("scd < :arx_date")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date},
                      {":remain_rows", remain_rows}});

    if(ARX::CLEANUP_PG()) {
        for(const auto & m : mark_trips) {
            make_db_curs("delete from MARK_TRIPS where POINT_ID = :point_id AND "
                         "NOT EXISTS (SELECT PAX_GRP.point_id_mark from PAX_GRP WHERE PAX_GRP.point_id_mark = :point_id) ",
                         PgOra::getRWSession("MARK_TRIPS")).bind(":point_id", m.point_id)
                    .exec();
        }
    }
    return mark_trips.size();
}

void norms_rates_etc(const Dates::DateTime_t& arx_date, int max_rows, int time_duration, int& step)
{
    LogTrace(TRACE6) << __func__ << " arx_date: " << arx_date;
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
    LogTrace(TRACE6) << __func__;
    if(step <= 0) {
        step = 1;
    }
    Dates::DateTime_t arx_date = utcdate-Dates::days(ARX::ARX_DAYS()-15);
    HelpCpp::Timer timer;
    norms_rates_etc(arx_date, max_rows, duration, step);
    ASTRA::commitAndCallCommitHooks();
    LogTrace(TRACE6) << " TArxNormsRatesEtc time: " << timer.elapsedSeconds();
    proc_count++;
    return step > 0;
};

string TArxNormsRatesEtc::TraceCaption()
{
    return "TArxNormsRatesEtc";
};


//============================= TArxTlgsFilesEtc===========
// STEP 6

std::vector<int> get_tlgs(const Dates::DateTime_t& arx_date, int remain_rows)
{
    int id{};
    DbCpp::CursCtl cur = make_db_curs(
                "SELECT ID FROM TLGS WHERE time < :arx_date FETCH FIRST :remain_rows ROWS ONLY",
                PgOra::getROSession("TLGS")
                );

    cur.bind(":arx_date", arx_date)
            .bind(":remain_rows", remain_rows)
            .def(id)
            .exec();

    std::vector<int> ids;

    while (!cur.fen()) {
        ids.push_back(id);
    }

    return ids;
}

void arx_tlg_stat(const std::vector<int>& ids)
{
    dbo::Session session;

    std::vector<std::vector<dbo::TLG_STAT>> allStats;

    for (int id : ids) {
        allStats.emplace_back(session.query<dbo::TLG_STAT>()
                              .where("QUEUE_TLG_ID = :id and TIME_SEND is not null")
                              .setBind({{":id", id}})
                              );
    }

    for (const auto& stats: allStats) {
        for (const auto& s: stats) {
            dbo::ARX_TLG_STAT ats(s, s.time_send);
            session.insert(ats);
        }
    }
}

void del_tlgs(const std::vector<int>& ids)
{
    for (int id : ids) {
        make_db_curs("DELETE FROM TEXT_TLG_H2H WHERE MSG_ID = :id", PgOra::getRWSession("TEXT_TLG_H2H")).stb().bind(":id", std::to_string(id)).exec();
        make_db_curs("DELETE FROM TLG_STAT WHERE QUEUE_TLG_ID = :id", PgOra::getRWSession("TLG_STAT")).bind(":id", id).exec();
        make_db_curs("DELETE FROM TLG_ERROR WHERE ID = :id", PgOra::getRWSession("TLG_ERROR")).bind(":id", id).exec();
        make_db_curs("DELETE FROM TLG_QUEUE WHERE ID = :id", PgOra::getRWSession("TLG_QUEUE")).bind(":id", id).exec();
        make_db_curs("DELETE FROM TLGS_TEXT WHERE ID = :id", PgOra::getRWSession("TLGS_TEXT")).bind(":id", id).exec();
        make_db_curs("DELETE FROM TLGS WHERE ID = :id",      PgOra::getRWSession("TLGS")).bind(":id", id).exec();
    }
}

int arx_tlgs(const Dates::DateTime_t& arx_date, int remain_rows)
{
    const std::vector<int> tlgs
            = get_tlgs(arx_date, remain_rows);

    arx_tlg_stat(tlgs);

    del_tlgs(tlgs);

    return tlgs.size();
}

int delete_files(const Dates::DateTime_t& arx_date, int remain_rows)
{
    dbo::Session session;

    LogTrace5 << __func__ << " arx_date: " << arx_date << " remain_rows: " << remain_rows;
    std::vector<dbo::FILES> files = session.query<dbo::FILES>()
            .where("time < :arx_date")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date},
                      {":remain_rows", remain_rows}});
    LogTrace5 << " FILES DELETED SIZE: " << files.size();
    for(const auto & f : files) {
        LogTrace5 << " f.id: " << f.id << " f.time: " << f.time;
    }
    for(const auto & f : files) {
        make_db_curs("delete from FILE_PARAMS where ID = :id", PgOra::getRWSession("FILE_PARAMS")).bind(":id", f.id).exec();
        make_db_curs("delete from FILE_QUEUE where ID = :id", PgOra::getRWSession("FILE_QUEUE")).bind(":id", f.id).exec();
        make_db_curs("delete from FILE_ERROR where ID = :id", PgOra::getRWSession("FILE_ERROR")).bind(":id", f.id).exec();
        make_db_curs("delete from FILES where ID = :id", PgOra::getRWSession("FILES")).bind(":id", f.id).exec();
    }
    return files.size();
}

int delete_kiosk_events(const Dates::DateTime_t& arx_date, int remain_rows)
{
    dbo::Session session;

    std::vector<dbo::KIOSK_EVENTS> events = session.query<dbo::KIOSK_EVENTS>()
            .where("TIME < :arx_date")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date},
                      {":remain_rows", remain_rows}});

    if(ARX::CLEANUP_PG()) {
        for(const auto & ev : events) {
            make_db_curs("delete from KIOSK_EVENTS where ID = :id ", PgOra::getRWSession("KIOSK_EVENTS")).bind(":id", ev.id).exec();
            make_db_curs("delete from KIOSK_EVENT_PARAMS where EVENT_ID = :id", PgOra::getRWSession("KIOSK_EVENT_PARAMS")).bind(":id", ev.id).exec();
        }
    }
    return events.size();
}

int delete_rozysk(const Dates::DateTime_t& arx_date, int remain_rows)
{
    if(ARX::CLEANUP_PG()) {
        auto cur = make_db_curs(
            "delete from ROZYSK where TIME in "
            "(SELECT TIME FROM ROZYSK where TIME < :arx_date FETCH FIRST :remain_rows ROWS ONLY)",
                    PgOra::getRWSession("ROZYSK"));
        cur.bind(":arx_date", arx_date).bind(":remain_rows", remain_rows).exec();
        return cur.rowcount();
    }
    return 0;
}

int delete_aodb_spp_files(const Dates::DateTime_t& arx_date, int remain_rows)
{
    LogTrace(TRACE6) << __func__ << " arx_date: " << arx_date << " remain_rows: " << remain_rows;
    int rowsize = 0;
    if(ARX::CLEANUP_PG()) {
        dbo::Session session;
        std::string spp_filename = "SPP" + HelpCpp::string_cast(arx_date, "%y%m%d") + ".txt";
        std::vector<dbo::AODB_SPP_FILES> files = session.query<dbo::AODB_SPP_FILES>()
                .where("filename < :filename ")
                .fetch_first(":remain_rows")
                .for_update(true)
                .setBind({{":filename", spp_filename},
                          {":remain_rows", remain_rows}});

        for(const auto & f : files) {
            LogTrace(TRACE6) << " delete filename: " << f.filename << " point_addr: " << f.point_addr
                             << " airline: " << f.airline;
            auto cur = make_db_curs("delete from AODB_EVENTS "
                                    "where (filename, point_addr, airline) in ( "
                                    "select filename,point_addr, airline from AODB_EVENTS "
                                    "where filename= :filename and point_addr=:point_addr and airline= :airline "
                                    "FETCH FIRST :remain_rows ROWS ONLY)",
                                    PgOra::getRWSession("AODB_EVENTS"));
            cur.bind(":filename", f.filename)
                    .bind(":point_addr", f.point_addr)
                    .bind(":airline", f.airline)
                    .bind(":remain_rows", remain_rows)
                    .exec();
            rowsize += cur.rowcount();
            LogTrace(TRACE6) << " delete aodb_events: " << cur.rowcount();
            auto cur2 = make_db_curs("delete from AODB_SPP_FILES "
                                     "where (filename, point_addr, airline) in ( "
                                     "select filename,point_addr, airline from AODB_SPP_FILES "
                                     "where filename= :filename and point_addr= :point_addr and airline= :airline "
                                     "FETCH FIRST :remain_rows ROWS ONLY)",
                                     PgOra::getRWSession("AODB_SPP_FILES"));
            cur2.bind(":filename", f.filename)
                    .bind(":point_addr", f.point_addr)
                    .bind(":airline", f.airline)
                    .bind(":remain_rows", remain_rows)
                    .exec();
            rowsize += cur2.rowcount();
            LogTrace(TRACE6) << " delete aodb_events: " << cur2.rowcount();
        }
    }
    return rowsize;
}

int delete_eticks_display(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto cur = make_db_curs(
                "delete from ETICKS_DISPLAY "
                "where (TICKET_NO, COUPON_NO) in ("
                "select TICKET_NO, COUPON_NO "
                "from ETICKS_DISPLAY "
                "where LAST_DISPLAY < :arx_date "
                "FETCH FIRST :remain_rows ROWS ONLY) ",
                PgOra::getRWSession("ETICKS_DISPLAY"));
    cur.bind(":arx_date", arx_date).bind(":remain_rows", remain_rows).exec();
    return cur.rowcount();
}

int delete_eticks_display_tlgs(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto cur = make_db_curs(
                "delete from ETICKS_DISPLAY_TLGS "
                "where (TICKET_NO, COUPON_NO) in ("
                "select TICKET_NO, COUPON_NO "
                "from ETICKS_DISPLAY_TLGS "
                "where LAST_DISPLAY < :arx_date "
                "FETCH FIRST :remain_rows ROWS ONLY) ",
                PgOra::getRWSession("ETICKS_DISPLAY_TLGS"));
    cur.bind(":arx_date", arx_date).bind(":remain_rows", remain_rows).exec();
    return cur.rowcount();
}

int delete_emdocs_display(const Dates::DateTime_t& arx_date, int remain_rows)
{
    auto cur = make_db_curs(
                "delete from EMDOCS_DISPLAY "
                "where (EMD_NO, EMD_COUPON) in ("
                "select EMD_NO, EMD_COUPON "
                "from EMDOCS_DISPLAY "
                "where LAST_DISPLAY < :arx_date "
                "FETCH FIRST :remain_rows ROWS ONLY) ",
                PgOra::getRWSession("EMDOCS_DISPLAY"));
    cur.bind(":arx_date", arx_date).bind(":remain_rows", remain_rows).exec();
    return cur.rowcount();
}

void tlgs_files_etc(const Dates::DateTime_t& arx_date, int max_rows, int time_duration, int& step)
{
    LogTrace(TRACE6) << __func__ << " arx_date: " << arx_date;
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
    LogTrace(TRACE6) << __func__;
    if(step <= 0) {
        step = 1;
    }
    Dates::DateTime_t arx_date = utcdate - Dates::days(ARX::ARX_DAYS());
    HelpCpp::Timer timer;
    tlgs_files_etc(arx_date, max_rows, duration, step);
    ASTRA::commitAndCallCommitHooks();
    LogTrace(TRACE6) << "TArxTlgsFilesEtc time: " << timer.elapsedSeconds();
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

    if (time(NULL)-prior_exec<ARX::ARX_SLEEP()) return false;

    dbo::initStructures();

    time_t time_finish = time(NULL)+ARX::ARX_DURATION();

    if (prior_utcdate.date() != utcdate.date())
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
            } while(arxMove->Next(ARX::ARX_MAX_ROWS(),duration));

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

} //namespace PG_ARX

#ifdef XP_TESTING

#include "xp_testing.h"
#include "tlg/tlg.h"
#include "dbostructures.h"
#include "tlg/postpone_edifact.h"
#include "telegram.h"
#include <serverlib/TlgLogger.h>

bool del_from_tlg_queue(const AIRSRV_MSG& tlg_in);
bool del_from_tlg_queue_by_status(const AIRSRV_MSG& tlg_in, const std::string& status);
bool upd_tlg_queue_status(const AIRSRV_MSG& tlg_in,
                          const std::string& curStatus, const std::string& newStatus);
bool upd_tlgs_by_error(const AIRSRV_MSG& tlg_in, const std::string& error);
bool update_tlg_stat_time_send(const AIRSRV_MSG& tlg_in);
bool update_tlg_stat_time_receive(const AIRSRV_MSG& tlg_in);

// namespace TlgHandling {
// void updateTlgToPostponed(const tlgnum_t& tnum);
// bool isTlgPostponed(const tlgnum_t& tnum);
// struct PostponeEdiHandling;
// //void PostponeEdiHandling::addToQueue(const tlgnum_t& tnum);

// }

namespace PG_ARX {

bool test_arx_daily(const Dates::DateTime_t& utcdate, int step)
{
    LogTrace(TRACE6) << __FUNCTION__ << " step: " << step;

    dbo::initStructures();
    auto arxMove = create_arx_manager(utcdate, step);

    LogTrace(TRACE6) << "arx_daily_pg: "
                     << arxMove->TraceCaption()
                     << " started";

    arxMove->BeforeProc();
    time_t time_finish = time(NULL)+ARX::ARX_DURATION();
    int duration = 0;
    do{
        duration = time_finish - time(NULL);
        if (duration<=0)
        {
            LogTrace(TRACE6) << "arx_daily_pg: "
                             << arxMove->Processed()
                             << " iterations processed";
            return false;
        };
    }
    while(arxMove->Next(ARX::ARX_MAX_ROWS(), duration));
    LogTrace(TRACE6) << "arx_daily_pg: "
                     << arxMove->TraceCaption()
                     << " finished";

    return true;
}

} //namespace PG_ARX

std::string tlgText()
{
    return "MOWKB1H"
           ".MOWRMUT 122100"
           "PNL"
           "UT933/14MAR DME PART1"
           "CFG/008C120Y"
           "RBD C/C Y/YBHKLMNT"
           "AVAIL"
           "DME  PRG"
           "C008"
           "Y120"
           "-PRG000C"
           "-PRG000M"
           "-PRG000Y"
           "-PRG000T"
           "-PRG000N"
           "-PRG000B"
           "-PRG000H"
           "-PRG000L"
           "-PRG000K"
           "ENDPNL";
}

bool cmpTlgErrorMsg(int tlg_num, const std::string& compareWith)
{
    std::string data;

    DbCpp::CursCtl cur = make_db_curs(
                "SELECT MSG FROM TLG_ERROR WHERE ID=:id",
                PgOra::getROSession("TLG_ERROR")
                );
    cur.stb()
            .bind(":id", tlg_num)
            .def(data)
            .EXfet();

    return DbCpp::ResultCode::NoDataFound != cur.err()
            && 1 == cur.rowcount()
            && compareWith == data;
}

bool cmpTlgError(int tlg_num, const std::string& compareWith)
{
    std::string data;

    DbCpp::CursCtl cur = make_db_curs(
                "SELECT ERROR FROM TLGS WHERE ID=:id",
                PgOra::getROSession("TLGS")
                );
    cur.stb()
            .bind(":id", tlg_num)
            .def(data)
            .EXfet();

    return DbCpp::ResultCode::NoDataFound != cur.err()
            && 1 == cur.rowcount()
            && compareWith == data;
}

bool cmpTlgQueueProcAttempt(int tlg_num, const int compareWith)
{
    int data;

    DbCpp::CursCtl cur = make_db_curs(
                "SELECT PROC_ATTEMPT FROM TLG_QUEUE WHERE ID=:id",
                PgOra::getROSession("TLG_QUEUE")
                );
    cur.stb()
            .bind(":id", tlg_num)
            .def(data)
            .EXfet();

    return DbCpp::ResultCode::NoDataFound != cur.err()
            && 1 == cur.rowcount()
            && compareWith == data;
}

bool cmpTlg(const int tlg_num, const char* withReceiver, const char* withSender, const char* withType)
{
    std::string receiver;
    std::string sender;
    std::string type;

    DbCpp::CursCtl cur = make_db_curs(
                "SELECT RECEIVER, SENDER, TYPE FROM TLGS WHERE ID=:id",
                PgOra::getROSession("TLGS")
                );
    cur.stb()
            .bind(":id", tlg_num)
            .def(receiver)
            .def(sender)
            .def(type)
            .EXfet();

    return DbCpp::ResultCode::NoDataFound != cur.err()
            && 1 == cur.rowcount()
            && withReceiver == receiver
            && withSender == sender
            && withType == type;
}

bool cmpTlgQueue(const int tlg_num, const char* withSender, const char* withType, const char* withStatus)
{
    std::string status;
    std::string type;
    std::string sender;

    DbCpp::CursCtl cur = make_db_curs(
                "SELECT STATUS, TYPE, SENDER FROM TLG_QUEUE WHERE ID=:id",
                PgOra::getROSession("TLG_QUEUE")
                );
    cur.stb()
            .bind(":id", tlg_num)
            .def(status)
            .def(type)
            .def(sender)
            .EXfet();

    return DbCpp::ResultCode::NoDataFound != cur.err()
            && 1 == cur.rowcount()
            && withSender == sender
            && withType == type
            && withStatus == status;
}

START_TEST(check_getTlgText)
{
    const std::string tlg_text = "TEST";
    const int tlg_id = 42;
    putTlgText(tlg_id, tlg_text);
    fail_unless(getTlgText(tlg_id) == tlg_text);        // src/tlg/tlg.cpp
}
END_TEST;

START_TEST(check_loadTlg)
{
    const int tlg_id = loadTlg(tlgText());                // src/tlg/tlg.cpp
    fail_unless(getTlgText(tlg_id) == tlgText());         // src/tlg/tlg.cpp
    fail_unless(procTlg(tlg_id));                         // src/tlg/tlg.cpp
    fail_unless(procTlg(tlg_id));                         // src/tlg/tlg.cpp
    fail_unless(cmpTlgQueueProcAttempt(tlg_id, 2));       // current
    fail_unless(deleteTlg(tlg_id));                       // src/tlg/tlg.cpp
    fail_if(procTlg(tlg_id));                             // src/tlg/tlg.cpp

    errorTlg(tlg_id,"PARS", "bad system");                // src/tlg/tlg.cpp
    fail_unless(cmpTlgErrorMsg(tlg_id, "bad system"));    // current
    errorTlg(tlg_id,"PROC", "proc_attempt");              // src/tlg/tlg.cpp
    fail_unless(cmpTlgError(tlg_id, "PROC"));             // current
}
END_TEST;

START_TEST(check_saveTlg)
{
    const int tlg_id = saveTlg(
                "RECVR",
                "SENDR",
                "INB",
                tlgText()
                );                                                    // src/tlg/tlg.cpp
    fail_unless(getTlgText(tlg_id) == tlgText());         // src/tlg/tlg.cpp
    fail_unless(cmpTlg(tlg_id, "RECVR", "SENDR", "INB")); // current
}
END_TEST;

START_TEST(check_putTlgToOutQue)
{
    const int tlg_id = saveTlg(
                "RECVR",
                "SENDR",
                "OUTA",
                "test"
                );                                                    // src/tlg/tlg.cpp
    const int priority = 0;
    const int ttl = 0;
    putTlg2OutQueue_wrap("RECVR", "SENDR", "OUTA", "test", priority, tlg_id, ttl);

    AIRSRV_MSG tlg_in = {
        .num = tlg_id,
        .type = {},
        .Sender = {},
        .Receiver = "SENDR",
    };

    fail_unless(upd_tlg_queue_status(tlg_in, "PUT", "GUT"));
    fail_unless(cmpTlgQueue(tlg_id, "SENDR", "OUTA", "GUT"));
    fail_unless(del_from_tlg_queue_by_status(tlg_in, "GUT"));
    fail_if(cmpTlgQueue(tlg_id, "SENDR", "OUTA", "GUT"));

    putTlg2OutQueue_wrap("RECVR", "SENDR", "OAPP", "test", priority, tlg_id, ttl);
    fail_unless(cmpTlgQueue(tlg_id, "SENDR", "OAPP", "PUT"));
    fail_unless(del_from_tlg_queue(tlg_in));
    fail_if(cmpTlgQueue(tlg_id, "SENDR", "OAPP", "PUT"));

    putTlg2OutQueue_wrap("RECVR", "SENDR", "OUTB", "test", priority, tlg_id, ttl);
    fail_unless(upd_tlgs_by_error(tlg_in, "EPIC"));       // src/tlg/main_srv.cpp
    fail_unless(cmpTlgError(tlg_id, "EPIC"));             // current

    fail_unless(cmpTlgQueue(tlg_id, "SENDR", "OUTB", "PUT"));
}
END_TEST;

START_TEST(check_sendTlg)
{
    TlgLogger::setLogging();

    std::string tlg_text = "SOME ANSWER";

    const int tlg_id = sendTlg(
                "RECVR",
                "SENDR",
                qpOutB,
                20,
                tlg_text,
                11,
                11
                );

    TTripInfo fltInfo;

    TTlgStat().putTypeBOut(
                tlg_id,
                11,
                11,
                TTlgStatPoint("SENDRSI", "SENDR", "SENB", ""),
                TTlgStatPoint("RECVRSI", "RECVR", "RECVR", ""),
                NowUTC(),
                "OAPP",
                tlg_text.size(),
                fltInfo,
                "MRK",
                "EXTRA"
                );

    AIRSRV_MSG tlg_in = {
        .num = tlg_id,
        .type = {},
        .Sender = {},
        .Receiver = "SENDR",
    };

    fail_unless(update_tlg_stat_time_send(tlg_in));
    fail_unless(update_tlg_stat_time_receive(tlg_in));
}
END_TEST;

START_TEST(check_arx_tlgs)
{
    const int tlg_id = loadTlg(tlgText());                // src/tlg/tlg.cpp
    fail_unless(procTlg(tlg_id));                         // src/tlg/tlg.cpp
    errorTlg(tlg_id,"PARS", "bad system");                // src/tlg/tlg.cpp

    dbo::initStructures();
    const auto date = Dates::second_clock::universal_time() + Dates::days(15);
    fail_unless(1 == PG_ARX::arx_tlgs(date, 1));          // src/arx_daily_pg.cpp

    fail_if(procTlg(tlg_id));
    fail_if(cmpTlgErrorMsg(tlg_id, "bad system"));
}
END_TEST;

START_TEST(check_isTlgPostponed)
{
    const int tlg_id = loadTlg(tlgText());                // src/tlg/tlg.cpp
    tlgnum_t tlgnum(to_string(tlg_id), false);
    fail_if(TlgHandling::isTlgPostponed(tlgnum));
    TlgHandling::updateTlgToPostponed(tlgnum);
    fail_unless(TlgHandling::isTlgPostponed(tlgnum));
}
END_TEST;

#define SUITENAME "tlg_queue"
TCASEREGISTER(testInitDB, testShutDBConnection)
{
    ADD_TEST(check_getTlgText);
    ADD_TEST(check_loadTlg);
    ADD_TEST(check_saveTlg);
    ADD_TEST(check_arx_tlgs);
    ADD_TEST(check_putTlgToOutQue);
    ADD_TEST(check_sendTlg);
    ADD_TEST(check_isTlgPostponed);
}
TCASEFINISH;
#undef SUITENAME

#endif //XP_TESTING
