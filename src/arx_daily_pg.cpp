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

//#include "pg_session.h"
#include <serverlib/pg_cursctl.h>
#include <serverlib/pg_rip.h>
#include <serverlib/cursctl.h>
#include <serverlib/testmode.h>
#include <serverlib/dates_oci.h>
#include <serverlib/dates_io.h>
#include <serverlib/tcl_utils.h>
#include <serverlib/rip_oci.h>
#include <serverlib/oci_rowid.h>

#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"
#include "tlg/typeb_db.h"
#include "pax_db.h"

//#include "serverlib/dump_table.h"
//#include "hooked_session.h"

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


}//namespace ARX

/////////////////////////////////////////////////////////////////////////////////////////

bool arx_daily_pg(TDateTime utcdate)
{
    LogTrace5 << __func__ << " utcdate: " << utcdate;
    if(utcdate == ASTRA::NoExists) {
        LogTrace5 << " utcdate incorrect";
        return false;
    }
    return PG_ARX::arx_daily(DateTimeToBoost(utcdate));
}

/////////////////////////////////////////////////////////////////////////////////////////

namespace  PG_ARX
{
//by move_id
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
void arx_pay_services(const GrpId_t& grp_id, const Dates::DateTime_t& part_key);
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

struct birks_row
{
    std::string tag_type;
    int no_len=0;
    std::string color;
    std::string color_view;
    long long first=0;
    long long last=0;
    long long no=0;
};

bool operator <(const birks_row & ls, const birks_row & rs)
{
    return std::tie(ls.tag_type, ls.color, ls.no) < std::tie(rs.tag_type, rs.color, rs.no);
}

std::optional<std::string> build_birks_str(const std::set<birks_row> & birks)
{
    std::string res; //VARCHAR2(4000)
    std::string noStr; //VARCHAR2(15);
    std::string firstStr; //VARCHAR2(17)
    std::string lastStr; //VARCHAR2(3)
    int diff; //BINARY_INTEGER;

    ckin::birks_row curRow;
    ckin::birks_row oldRow;
    ckin::birks_row oldRow2;

    for(const auto & row : birks) {
        curRow = row;
        diff = 1;
        oldRow = curRow;
        oldRow2 = {};
        if(oldRow.tag_type == curRow.tag_type  &&
                   ((!oldRow.color.empty() && !curRow.color.empty() &&
                    oldRow.color==curRow.color) ||
                    (oldRow.color.empty() && curRow.color.empty())) &&
                    oldRow.first==curRow.first &&
                    oldRow.last+diff==curRow.last)
        {
            diff += 1;
        } else {
            if(oldRow2.tag_type==oldRow.tag_type &&
              ((!oldRow2.color.empty() && !oldRow.color.empty() && oldRow2.color==oldRow.color) ||
               (oldRow2.color.empty() && oldRow.color.empty())) && oldRow2.first==oldRow.first ) {
                firstStr = StrUtils::lpad(std::to_string(oldRow.last),3,'0');
                if(!res.empty()) {
                    res+=",";
                }
            } else {
                firstStr = oldRow.color_view;
                noStr = std::to_string(oldRow.first*1000+oldRow.last);
                if (noStr.length() < oldRow.no_len) {
                  firstStr += StrUtils::lpad(noStr,oldRow.no_len,'0');
                } else {
                  firstStr += noStr;
                }
                oldRow2 = oldRow;
                if(!res.empty()) {
                    res += ", ";
                }
            }

            if(diff != 1) {
              lastStr = StrUtils::lpad(std::to_string(oldRow.last+diff-1),3,'0');
              res = res + firstStr + '-' + lastStr;
            } else {
              res += firstStr;
            }
            diff = 1;
            oldRow = curRow;
        }
    }
    return res;
}

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
    TypeB::deleteTypeBData(PointIdTlg_t(point_id.get()), "", "", false/*delete_trip_comp_layers*/);
    auto cur = make_curs(
                "BEGIN \n"
                "   ckin.delete_typeb_data(:point_id, NULL, NULL, FALSE); \n"
                "END;");
    cur.bind(":point_id", point_id)
            .exec();
}

}


std::optional<int> get_bag_pool_pax_id(Dates::DateTime_t part_key, int grp_id,
                                       std::optional<int> bag_pool_num, int include_refused)
{
    if(!bag_pool_num) {
        return std::nullopt;
    }
    std::optional<int> res;
    int pax_id;
    std::string refuse;

    auto cur = make_db_curs(
                "SELECT pax_id, refuse "
                "FROM arx_pax "
                "WHERE PART_KEY=:part_key and GRP_ID=:grp_id and BAG_POOL_NUM=:bag_pool_num "
                "ORDER BY  CASE WHEN pers_type='ВЗ' THEN 0 WHEN pers_type='РБ' THEN 0 ELSE 1 END, "
                "          CASE WHEN seats=0 THEN 1 ELSE 0 END, "
                "          CASE WHEN refuse is null THEN 0 ELSE 1 END, "
                "          CASE WHEN pers_type='ВЗ' THEN 0 WHEN pers_type='РБ' THEN 1 ELSE 2 END, "
                "          reg_no ",
                PgOra::getROSession("ARX_PAX"));
    cur.stb()
       .def(pax_id)
       .defNull(refuse, "")
       .bind(":part_key", part_key)
       .bind(":grp_id", grp_id)
       .bind(":bag_pool_num", bag_pool_num.value_or(0))
       .exec();
    if(!cur.fen()) {
        res = pax_id;
        if(include_refused == 0 && !refuse.empty()) {
            res = std::nullopt;
        }
    }
    return res;
}

TBagInfo get_bagInfo2(Dates::DateTime_t part_key, int grp_id, std::optional<int> pax_id,
                       std::optional<int> bag_pool_num)
{
    TBagInfo bagInfo{};
    bagInfo.grp_id = grp_id;
    bagInfo.pax_id = pax_id;

    std::optional<int> pool_pax_id = pax_id;
    if(pax_id) {
        if(!bag_pool_num) {
            return bagInfo;
        }
        pool_pax_id = get_bag_pool_pax_id(part_key, grp_id, bag_pool_num);
    }

    if(!pax_id || (pool_pax_id && pool_pax_id == pax_id)) {
        std::string query = "SELECT "
                            "SUM(CASE WHEN pr_cabin=0 THEN amount ELSE NULL END) AS bagAmount, "
                            "SUM(CASE WHEN pr_cabin=0 THEN weight ELSE NULL END) AS bagWeight, "
                            "SUM(CASE WHEN pr_cabin=0 THEN NULL ELSE amount END) AS rkAmount, "
                            "SUM(CASE WHEN pr_cabin=0 THEN NULL ELSE weight END) AS rkWeight "
                            "FROM arx_bag2 "
                            "WHERE part_key=:part_key AND grp_id=:grp_id ";
        query += pax_id ? " AND bag_pool_num=:bag_pool_num " : "";
        auto cur = make_db_curs(query, PgOra::getROSession("ARX_BAG2"));
        cur.stb()
           .defNull(bagInfo.bagAmount,ASTRA::NoExists)
           .defNull(bagInfo.bagWeight,ASTRA::NoExists)
           .defNull(bagInfo.rkAmount,ASTRA::NoExists)
           .defNull(bagInfo.rkWeight,ASTRA::NoExists)
           .bind(":part_key", part_key)
           .bind(":grp_id", grp_id);
        if(pax_id) {
            cur.bind(":bag_pool_num", bag_pool_num.value_or(0));
        }
        cur.EXfet();
        if(cur.err() == DbCpp::ResultCode::NoDataFound) {
            LogTrace(TRACE5) << __FUNCTION__ << " Query error. Not found data by grp_id: " << grp_id
                             << " part_key: " << part_key ;
            return bagInfo;
        }
    }
    return bagInfo;
}

std::optional<int> get_bagAmount2(Dates::DateTime_t part_key, int grp_id,
                    std::optional<int> pax_id,
                    std::optional<int> bag_pool_num)
{
    TBagInfo bagInfo = get_bagInfo2(part_key, grp_id, pax_id, bag_pool_num);
    return bagInfo.bagAmount;
}

std::optional<int> get_bagWeight2(Dates::DateTime_t part_key, int grp_id,
                                  std::optional<int> pax_id,
                                  std::optional<int> bag_pool_num)
{
    TBagInfo bagInfo = get_bagInfo2(part_key, grp_id, pax_id, bag_pool_num);
    return bagInfo.bagWeight;
}

std::optional<int> get_rkAmount2(Dates::DateTime_t part_key, int grp_id,
                    std::optional<int> pax_id,
                    std::optional<int> bag_pool_num)
{
    TBagInfo bagInfo = get_bagInfo2(part_key, grp_id, pax_id, bag_pool_num);
    return bagInfo.rkAmount;
}

std::optional<int> get_rkWeight2(Dates::DateTime_t part_key, int grp_id,
                                 std::optional<int> pax_id,
                                 std::optional<int> bag_pool_num)
{
    TBagInfo bagInfo = get_bagInfo2(part_key, grp_id, pax_id, bag_pool_num);
    return bagInfo.rkWeight;
}

std::optional<int> get_excess_wt(Dates::DateTime_t part_key, int grp_id,
                                 std::optional<int> pax_id,
                                 std::optional<int> excess_wt, std::optional<int> excess_nvl,
                                 int bag_refuse)
{
    int excess = 0 ;
    std::optional<int> main_pax_id;
    if((!excess_wt && !excess_nvl) || !bag_refuse) {
        auto cur = make_db_curs("SELECT (CASE WHEN BAG_REFUSE=0 THEN COALESCE(EXCESS_WT, EXCESS) ELSE 0 END) "
                                   "FROM ARX_PAX_GRP "
                                   "WHERE part_key=:part_key AND grp_id=:grp_id ",
                                   PgOra::getROSession("ARX_PAX_GRP"));
        cur.def(excess)
           .bind(":part_key", part_key)
           .bind(":grp_id", grp_id)
           .EXfet();
        if(cur.err() == DbCpp::ResultCode::NoDataFound) {
            LogTrace(TRACE5) << __FUNCTION__ << " Query error. Not found data by grp_id: " << grp_id
                             << " part_key: " << part_key ;
            return std::nullopt;
        }
    } else {
        if(bag_refuse == 0) {
            if(excess_wt) return excess_wt;
            if(excess_nvl) return excess_nvl;
            return std::nullopt;
        } else excess=0;
    }

    if(pax_id) {
        main_pax_id = get_main_pax_id2(part_key, grp_id);
    }
    if(!(!pax_id || (main_pax_id && main_pax_id==pax_id))) {
        return std::nullopt;
    }
    if(excess == 0) return std::nullopt;
    return excess;
}


std::optional<int> get_main_pax_id2(Dates::DateTime_t part_key, int grp_id, int include_refused)
{
    std::optional<int> res;
    int pax_id;
    std::string refuse;
    auto cur = make_db_curs("select PAX_ID, REFUSE from ARX_PAX "
                               "where PART_KEY=:part_key and GRP_ID=:grp_id "
                               "order by case when BAG_POOL_NUM is null  then 1 else 0 end, "
                               "         case when PERS_TYPE='ВЗ' then 0 when pers_type='РБ' then 0 else 1 end, "
                               "         case when SEATS=0 THEN 1 else 0 end, "
                               "         case when REFUSE is null then 0 else 1 end, "
                               "         case when PERS_TYPE='ВЗ' then 0 when PERS_TYPE='РБ' then 1 else 2 end, "
                               "         reg_no",
                               PgOra::getROSession("ARX_PAX"));
    cur.def(pax_id)
       .defNull(refuse, "")
       .bind(":part_key", part_key)
       .bind(":grp_id", grp_id)
       .exec();
    if(!cur.fen()) {
        res = pax_id;
        if(include_refused == 0 && !refuse.empty()) {
            res = std::nullopt;
        }
    }
    return res;
}

int bag_pool_refused(Dates::DateTime_t part_key, int grp_id, int bag_pool_num,
                     std::optional<std::string> vclass, int bag_refuse)
{
    if(bag_refuse != 0) return 1;
    if(!vclass) return 0;
    int n = 0;
    auto cur = make_db_curs("select sum(case when REFUSE is null then 1 else 0 end) from ARX_PAX "
                               "where PART_KEY=:part_key and GRP_ID=:grp_id and BAG_POOL_NUM=:bag_pool_num; ",
                               PgOra::getROSession("ARX_PAX"));
    cur.def(n)
       .bind(":part_key", part_key)
       .bind(":grp_id", grp_id)
       .bind(":bag_pool_num", bag_pool_num)
       .EXfet();
    if(cur.err() == DbCpp::ResultCode::NoDataFound || n==0) {
        return 1;
    } else
        return 0;
}

std::set<ckin::birks_row> read_birks(Dates::DateTime_t part_key, int grp_id, const std::string& lang, bool bag_num=true)
{
    std::set<ckin::birks_row> birks;
    ckin::birks_row row;
    std::string pg_select = "select TAG_TYPE, COLOR, TRUNC(NO/1000) AS first, MOD(NO,1000) AS last, no ";
    std::string ora_select = "select NO_LEN, case :lang when 'RU' then TAG_COLORS.CODE else "
                "  coalesce(TAG_COLORS.CODE_LAT,TAG_COLORS.CODE) end as COLOR_VIEW ";

    auto cur = make_db_curs(pg_select + " from ARX_BAG_TAGS "
                               "where PART_KEY=:part_key AND grp_id=:grp_id " +
                               (bag_num ? "" : " AND BAG_NUM is null "),
                            PgOra::getROSession("ARX_PAX_GRP"));
    cur.def(row.tag_type)
       .def(row.color)
       .def(row.first)
       .def(row.last)
       .def(row.no)
       .bind(":part_key", part_key)
       .bind(":grp_id", grp_id)
       .exec();
    while(!cur.fen()) {
        auto cur2 = make_db_curs(ora_select + " from TAG_TYPES,TAG_COLORS "
                             "where TAG_TYPES.CODE = :TAG_TYPE and "
                             "TAG_COLORS.CODE = :color; ",
                             PgOra::getROSession("TAG_TYPES"));
        cur2.def(row.no_len)
            .def(row.color_view)
            .bind(":lang", lang)
            .bind(":tag_type", row.tag_type)
            .bind(":color", row.color)
            .EXfet();
        birks.insert(row);
    }
    return birks;
}

std::set<ckin::birks_row> read_birks(Dates::DateTime_t part_key, int grp_id, int bag_pool_num, const std::string& lang)
{
    std::set<ckin::birks_row> birks;
    ckin::birks_row row;
    std::string pg_select = "select TAG_TYPE, COLOR, TRUNC(NO/1000) AS first, MOD(NO,1000) AS last, no ";
    std::string ora_select = "select NO_LEN, case :lang when 'RU' then TAG_COLORS.CODE else "
                "  coalesce(TAG_COLORS.CODE_LAT,TAG_COLORS.CODE) end as COLOR_VIEW ";
    auto cur = make_db_curs(pg_select + " from ARX_BAG2, ARX_BAG_TAGS "
                               "where ARX_BAG2.PART_KEY=ARX_BAG_TAGS.PART_KEY and "
                               "    ARX_BAG2.GRP_ID=ARX_BAG_TAGS.GRP_ID and "
                               "    ARX_BAG2.NUM=ARX_BAG_TAGS.BAG_NUM and "
                               "    ARX_BAG2.PART_KEY=:part_key and "
                               "    ARX_BAG2.GRP_ID=:grp_id and "
                               "    ARX_BAG2.BAG_POOL_NUM=:bag_pool_num",
                               PgOra::getROSession("ARX_BAG2"));
    cur.def(row.tag_type)
       .def(row.color)
       .def(row.first)
       .def(row.last)
       .def(row.no)
       .bind(":part_key", part_key)
       .bind(":grp_id", grp_id)
       .bind(":bag_pool_num", bag_pool_num)
       .exec();
    while(!cur.fen()) {
        auto cur = make_db_curs(ora_select + " from TAG_TYPES,TAG_COLORS "
                             "where TAG_TYPES.CODE = :TAG_TYPE and "
                             "TAG_COLORS.CODE = :color; ",
                             PgOra::getROSession("TAG_TYPES"));
        cur.def(row.no_len)
            .def(row.color_view)
            .bind(":lang", lang)
            .bind(":tag_type", row.tag_type)
            .bind(":color", row.color)
            .EXfet();

        birks.insert(row);
    }
    return birks;
}

std::optional<std::string> get_birks2(Dates::DateTime_t part_key, int grp_id, std::optional<int> pax_id,
                       int bag_pool_num, const std::string& lang)
{
    std::optional<int> pool_pax_id;
    if(pax_id) {
        if(!bag_pool_num) return std::nullopt;
        pool_pax_id = get_bag_pool_pax_id(part_key, grp_id, bag_pool_num);
    }

    std::set<ckin::birks_row> birks;
    if(!pax_id || (pool_pax_id && pool_pax_id==pax_id)) {
        if(!pax_id) {
                birks = read_birks(part_key, grp_id, lang);
        }else {
            if(bag_pool_num==1) {
                /*для тех групп которые регистрировались с терминала без обязательной привязки */
                 std::set<ckin::birks_row> birks1 = read_birks(part_key, grp_id, bag_pool_num, lang);
                 std::set<ckin::birks_row> birks2 = read_birks(part_key, grp_id, lang, false);
                //вместо SQL UNION
                std::merge(birks1.begin(), birks1.end(), birks2.begin(), birks2.end(),
                           std::inserter(birks, birks.begin()));
            } else {
                birks = read_birks(part_key, grp_id, bag_pool_num, lang);
            }
        }
    }
    return ckin::build_birks_str(birks);
}

std::optional<std::string> get_birks2(Dates::DateTime_t part_key, int grp_id, std::optional<int> pax_id,
                       int bag_pool_num, int pr_lat)
{
    if(pr_lat!=1)  {
        return get_birks2(part_key, grp_id, pax_id, bag_pool_num, "");
    } else {
        return get_birks2(part_key, grp_id, pax_id, bag_pool_num, "RU");
    }
}

std::optional<std::string> next_airp(Dates::DateTime_t part_key, int first_point, int point_num)
{
    dbo::Session session;
    std::optional<std::string> airp = session.query<std::string>("SELECT airp")
            .from("arx_points")
            .where("part_key = :part_key AND first_point=:first_point AND point_num > :point_num AND pr_del=0 ORDER BY point_num")
            .setBind({{":part_key", part_key}, {":first_point", first_point}, {":point_num", point_num}});
    return airp;
}


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
    part_key=Dates::not_a_date_time;
    date_range=NoExists;


    dbo::Session session;
    std::vector<dbo::Points> points = session.query<dbo::Points>().where(" MOVE_ID = :move_id ORDER BY point_num ")
            .setBind({{":move_id", move_id.get()}});
    LogTrace5 << " points size : " << points.size();
    Dates::DateTime_t first_date = Dates::not_a_date_time;
    Dates::DateTime_t last_date = Dates::not_a_date_time;
    bool deleted=true;

    for(size_t i = 0; i<points.size(); i++)
    {
        dbo::Points & p = points[i];
        if (p.pr_del != -1) {
            tst();
            deleted=false;
        }
        std::vector<Dates::DateTime_t> temp;
        if(i%2 == 0) {
            temp = {first_date,last_date, p.scd_out, p.est_out, p.act_out};
        } else {
            temp = {first_date,last_date, p.scd_in, p.est_in, p.act_in};
        }
        tst();
        auto fdates = algo::filter(temp, dbo::isNotNull<Dates::DateTime_t>);
        tst();
        for(const auto & t : fdates) LogTrace(TRACE5) << " FILTER: " << t;
        if(auto minIt = std::min_element(fdates.begin(), fdates.end()); minIt != fdates.end()) {
            first_date = *minIt;
        }
        if(auto maxIt = std::max_element(fdates.begin(), fdates.end()); maxIt != fdates.end()) {
            last_date = *maxIt;
        }
        LogTrace(TRACE5) << " myfirst_date: " << first_date << " mylast_date: " << last_date;
    }

    if (!deleted)
    {
        tst();
        if (first_date!=Dates::not_a_date_time && last_date!=Dates::not_a_date_time)
        {
            tst();
            if (last_date < utcdate - Dates::days(ARX::ARX_DAYS()))
            {
                LogTrace5 << " в архив";
                //переместить в архив
                part_key = last_date;
                date_range = BoostToDateTime(last_date) - BoostToDateTime(first_date);
                LogTrace5 << " date_range : " << date_range << " last_date: " << last_date
                          << " first_date: " << first_date;
                return true;
            };
        };
    }
    else
    {
        tst();
        //полностью удаленный рейс
        if (last_date == Dates::not_a_date_time || last_date < (utcdate-Dates::days(ARX::ARX_DAYS())))
        {
            LogTrace5 << "удаленный рейс";
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
    LogTrace(TRACE5) << __FUNCTION__ << " move_id: " << move_id;
    dbo::Session session;
    std::vector<dbo::Points> points = session.query<dbo::Points>()
            .where(" MOVE_ID = :move_id ")
            .setBind({{":move_id",move_id.get()} });
    for(const auto & p : points) {
        bool pr_reg = p.pr_reg != 0;
        if(p.pr_del < 0) continue;
        if (p.pr_del==0 && pr_reg) get_flight_stat(p.point_id, true);
        TReqInfo::Instance()->LocaleToLog("EVT.FLIGHT_MOOVED_TO_ARCHIVE", evtFlt, p.point_id);
    }
};


void TArxMoveFlt::readMoveIds(size_t max_rows)
{
    LogTrace(TRACE5) << __func__;
    int move_id;
    Dates::DateTime_t part_key;
    double date_range;
    auto cur = make_db_curs("SELECT move_id FROM points "
                         "WHERE (time_in > TO_DATE('01.01.1900','DD.MM.YYYY') AND time_in<:arx_date)"
                         " OR (time_out > TO_DATE('01.01.1900','DD.MM.YYYY') AND time_out<:arx_date)"
                         " OR (time_in  = TO_DATE('01.01.1900','DD.MM.YYYY') AND "
                         "      time_out = TO_DATE('01.01.1900','DD.MM.YYYY'))",
                         PgOra::getROSession("points"));
    cur.stb()
       .def(move_id)
       .bind(":arx_date", utcdate-Dates::days(ARX::ARX_DAYS()))
       .exec();

    while(!cur.fen() && (move_ids.size() < max_rows)) {
        tst();
        if (GetPartKey(MoveId_t(move_id), part_key,date_range))
        {
            move_ids.try_emplace(MoveId_t(move_id), part_key);
        }
    }
}

bool TArxMoveFlt::Next(size_t max_rows, int duration)
{
    LogTrace(TRACE5) << __func__;
    readMoveIds(max_rows);
    if(move_ids.empty()) {
        LogTrace5 << " moveids empty";
    }
    while (!move_ids.empty())
    {
        LogTrace(TRACE5) << "MOVE_IDS count: " << move_ids.size();
        MoveId_t move_id = move_ids.begin()->first;
        LogTrace5 << __FUNCTION__ << " move_id: " << move_id;
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
                    if (date_range<0) {
                        LogTrace(TRACE5) << " date_range < 0 :" << date_range;
                        throw Exception("date_range=%f", date_range);
                    }
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
                LogTrace(TRACE5) << " points size: " << points.size();
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
                                arx_pay_services(grp_id, part_key);
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
                ASTRA::commitAndCallCommitHooks();
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
        } else {
            LogTrace5 << __FUNCTION__ << " GetPartKey return false";
        }
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
        dbo::Session session;
        dbo::Move_Arx_Ext ext{date_range, vmove_id.get(), part_key};
        session.insert(ext);
    }
}

void arx_move_ref(const MoveId_t & vmove_id, const Dates::DateTime_t& part_key)
{
    LogTrace(TRACE5) << __FUNCTION__ << " move_id: " << vmove_id;
    dbo::Session session;
    std::vector<dbo::Move_Ref> move_refs = session.query<dbo::Move_Ref>()
            .where(" MOVE_ID = :move_id")
            .setBind({{"move_id",vmove_id.get()} });
    for(const auto & mr : move_refs) {
        dbo::Arx_Move_Ref amr(mr,part_key);
        session.insert(amr);
    }
}

std::vector<dbo::Points> arx_points(const MoveId_t & vmove_id, const Dates::DateTime_t& part_key)
{
    LogTrace(TRACE5) << __FUNCTION__ << " vmove_id : "<< vmove_id << " part_key: " << part_key;
    dbo::Session session;
    std::vector<dbo::Points> points = session.query<dbo::Points>()
            .where(" MOVE_ID = :move_id and PR_DEL<>-1")
            .for_update(true)
            .setBind({{"move_id",vmove_id.get()} });

    //    auto cur2 = make_curs("DELETE FROM points WHERE move_id=:vmove_id");
    //    cur2.bind(":vmove_id", vmove_id).exec();
    for(const auto &p : points) {
        dbo::Arx_Points ap(p, part_key);
        session.insert(ap);
    }
    return points;
}

void arx_events_by_move_id(const MoveId_t & vmove_id, const Dates::DateTime_t& part_key)
{
    LogTrace5 << __FUNCTION__ << " move_id: " << vmove_id << " part_key: " << part_key;
    dbo::Session session;
    std::vector<dbo::Lang_Types> langs = session.query<dbo::Lang_Types>();
    for(const auto & lang : langs) {
        std::vector<dbo::Events_Bilingual> events =  session.query<dbo::Events_Bilingual>()
                .where(" id1 = :move_id and lang = :l and type = :evtDisp")
                .for_update(true)
                .setBind({{"move_id",vmove_id.get()},
                          {"l",lang.code},
                          {"evtDisp", EncodeEventType(evtDisp)}});
        for(const auto & ev : events) {
            dbo::Arx_Events aev(ev,part_key);
            session.insert(aev);
        }
    }
}

void arx_events_by_point_id(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    LogTrace5 << __FUNCTION__ << " point_id: " << point_id << " part_key: " << part_key;
    dbo::Session session;
    std::vector<dbo::Lang_Types> langs = session.query<dbo::Lang_Types>();
    for(const auto & lang : langs) {
        std::vector<dbo::Events_Bilingual> events =  session.query<dbo::Events_Bilingual>()
                .where(" id1 = :point_id and lang = :l and type in "
                       " (:evtFlt, :evtGraph, :evtFltTask, :evtPax, :evtPay, :evtTlg, :evtPrn)")
                .for_update(true)
                .setBind({{"l",lang.code},
                          {"point_id", point_id.get()},
                          {"evtFlt",     EncodeEventType(evtFlt)},
                          {"evtGraph",   EncodeEventType(evtGraph)},
                          {"evtFltTask", EncodeEventType(evtFltTask)},
                          {"evtPax",     EncodeEventType(evtPax)},
                          {"evtPay",     EncodeEventType(evtPay)},
                          {"evtTlg",     EncodeEventType(evtTlg)},
                          {"evtPrn",     EncodeEventType(evtPrn)},
                         });
        for(const auto & ev : events) {
            dbo::Arx_Events aev(ev,part_key);
            session.insert(aev);
        }
    }
}

void arx_mark_trips(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
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
        dbo::Arx_Mark_Trips amt(mark_trip,part_key);
        session.noThrowError(DbCpp::ResultCode::ConstraintFail).insert(amt);
    }
}

std::vector<dbo::Pax_Grp> arx_pax_grp(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    //LogTrace(TRACE3) << __FUNCTION__ << " point_id: " << point_id;
    dbo::Session session;
    std::vector<dbo::Pax_Grp> pax_grps = session.query<dbo::Pax_Grp>()
            .where("point_dep = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &gr : pax_grps) {
        dbo::Arx_Pax_Grp apg(gr,part_key);
        session.insert(apg);
    }
    return pax_grps;
}

void arx_self_ckin_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::Self_Ckin_Stat> ckin_stats = session.query<dbo::Self_Ckin_Stat>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : ckin_stats) {
        dbo::Arx_Self_Ckin_Stat ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_rfisc_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::RFISC_STAT> rfisc_stats = session.query<dbo::RFISC_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : rfisc_stats) {
        dbo::ARX_RFISC_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_services(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_SERVICES> stat_services = session.query<dbo::STAT_SERVICES>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_services) {
        dbo::ARX_STAT_SERVICES ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_rem(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_REM> stat_rems = session.query<dbo::STAT_REM>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_rems) {
        dbo::ARX_STAT_REM ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_limited_cap_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::LIMITED_CAPABILITY_STAT> stat_lcs = session.query<dbo::LIMITED_CAPABILITY_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_lcs) {
        dbo::ARX_LIMITED_CAPABILITY_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_pfs_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::PFS_STAT> stat_pfs = session.query<dbo::PFS_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_pfs) {
        dbo::ARX_PFS_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_ad(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_AD> stat_ad = session.query<dbo::STAT_AD>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_ad) {
        dbo::ARX_STAT_AD ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_ha(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_HA> stat_ha = session.query<dbo::STAT_HA>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_ha) {
        dbo::ARX_STAT_HA ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_vo(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_VO> stat_vo = session.query<dbo::STAT_VO>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_vo) {
        dbo::ARX_STAT_VO ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_stat_reprint(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT_REPRINT> stat_reprint = session.query<dbo::STAT_REPRINT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_reprint) {
        dbo::ARX_STAT_REPRINT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_trfer_pax_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::TRFER_PAX_STAT> stat_reprint = session.query<dbo::TRFER_PAX_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stat_reprint) {
        dbo::ARX_TRFER_PAX_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_bi_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::BI_STAT> bi_stats = session.query<dbo::BI_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : bi_stats) {
        dbo::ARX_BI_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}



void arx_agent_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    auto agents_stat = dbo::readOraAgentsStat(point_id);

    dbo::Session session;
    //    std::vector<dbo::AGENT_STAT> bi_stats = session.query<dbo::AGENT_STAT>()
    //            .where("point_id = :point_id")
    //            .setBind({{"point_id", point_id.get()}});

    for(const auto &as : agents_stat) {
        dbo::ARX_AGENT_STAT ascs(as,part_key);
        ascs.part_key = ascs.ondate;
        ascs.point_part_key =part_key;
        session.insert(ascs);
    }
}

void arx_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::STAT> stats = session.query<dbo::STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_trfer_stat(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::TRFER_STAT> stats = session.query<dbo::TRFER_STAT>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_TRFER_STAT ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_tlg_out(const PointId_t& point_id, const Dates::DateTime_t& part_key)
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

void arx_trip_classes(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::TRIP_CLASSES> stats = session.query<dbo::TRIP_CLASSES>()
            .where("point_id = :point_id").setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_TRIP_CLASSES ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_trip_delays(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::TRIP_DELAYS> stats = session.query<dbo::TRIP_DELAYS>()
            .where("point_id = :point_id").setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_TRIP_DELAYS ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_trip_load(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::TRIP_LOAD> stats = session.query<dbo::TRIP_LOAD>()
            .where("point_dep = :point_id").setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_TRIP_LOAD ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_trip_sets(const PointId_t& point_id, const Dates::DateTime_t& part_key)
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
        ats.part_key =part_key;
        session.insert(ats);
    }
}

void arx_trip_crs_displace2(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::CRS_DISPLACE2> stats = session.query<dbo::CRS_DISPLACE2>()
            .where("point_id_spp = :point_id").setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_CRS_DISPLACE2 ascs(cs,part_key);
        ascs.point_id_tlg = ASTRA::NoExists;
        session.insert(ascs);
    }
}

void arx_trip_stages(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::TRIP_STAGES> stats = session.query<dbo::TRIP_STAGES>()
            .where("point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &cs : stats) {
        dbo::ARX_TRIP_STAGES ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_bag_receipts(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    LogTrace1 <<__FUNCTION__ << " point_id: " << point_id;
    dbo::Session session;
    std::vector<dbo::BAG_RECEIPTS> bag_receipts = session.query<dbo::BAG_RECEIPTS>()
            .from("BAG_RECEIPTS, BAG_RCPT_KITS")
            .where("bag_receipts.kit_id = bag_rcpt_kits.kit_id(+) AND "
                   "bag_receipts.kit_num = bag_rcpt_kits.kit_num(+) AND "
                   "bag_receipts.point_id = :point_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});

    for(const auto &br : bag_receipts) {
        dbo::ARX_BAG_RECEIPTS abr(br,part_key);
        session.insert(abr);
    }
}

void arx_bag_pay_types(const PointId_t& point_id, const Dates::DateTime_t& part_key)
{
    LogTrace1 <<__FUNCTION__ << " point_id: " << point_id;
    dbo::Session session;
    std::vector<dbo::BAG_PAY_TYPES> bag_receipts = session.query<dbo::BAG_PAY_TYPES>()
            .from("BAG_RECEIPTS, BAG_RCPT_KITS, BAG_PAY_TYPES")
            .where("bag_receipts.kit_id = bag_rcpt_kits.kit_id(+) AND "
                   "bag_receipts.kit_num = bag_rcpt_kits.kit_num(+) AND "
                   "bag_receipts.point_id = :point_id AND "
                   "bag_receipts.receipt_id = bag_pay_types.receipt_id")
            .for_update(true)
            .setBind({{"point_id", point_id.get()}});


    for(const auto &bpt : bag_receipts) {
        dbo::ARX_BAG_PAY_TYPES abpt(bpt,part_key);
        session.insert(abpt);
    }
}

void arx_annul_bags_tags(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::ANNUL_BAG> annul_bags = session.query<dbo::ANNUL_BAG>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});
    std::vector<dbo::ANNUL_TAGS> annul_tags = session.query<dbo::ANNUL_TAGS>()
            .where("id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

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
    dbo::Session session;
    std::vector<dbo::UNACCOMP_BAG_INFO> annul_bags = session.query<dbo::UNACCOMP_BAG_INFO>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : annul_bags) {
        dbo::ARX_UNACCOMP_BAG_INFO ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_bag2(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::BAG2> bags2 = session.query<dbo::BAG2>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : bags2) {
        dbo::ARX_BAG2 ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_bag_prepay(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::BAG_PREPAY> bags2 = session.query<dbo::BAG_PREPAY>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : bags2) {
        dbo::ARX_BAG_PREPAY ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_bag_tags(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::BAG_TAGS> bags2 = session.query<dbo::BAG_TAGS>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : bags2) {
        dbo::ARX_BAG_TAGS ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_paid_bag(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::PAID_BAG> bags2 = session.query<dbo::PAID_BAG>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : bags2) {
        dbo::ARX_PAID_BAG ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_pay_services(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::PAY_SERVICES> services = session.query<dbo::PAY_SERVICES>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &s : services) {
        dbo::ARX_PAY_SERVICES ascs(s,part_key);
        session.insert(ascs);
//        make_db_curs("delete from PAY_SERVICES where gpr_id = :grp_id", PgOra::getROSession("PAY_SERVICES"))
//            .bind(":grp_id", grp_id.get())
//            .exec();
    }
}

std::vector<dbo::PAX> arx_pax(const PointId_t& point_id, const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::PAX> paxes = session.query<dbo::PAX>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

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
    dbo::Session session;
    std::vector<dbo::TRANSFER> trfer = session.query<dbo::TRANSFER>()
            .where("grp_id = :grp_id AND transfer_num > 0")
            .for_update(true)
            .setBind({{":grp_id", grp_id.get()}});
    for(const auto& tr : trfer) {
        std::optional<dbo::TRFER_TRIPS> trip = session.query<dbo::TRFER_TRIPS>()
                .where("point_id = :tp")
                .for_update(true)
                .setBind({{":tp", tr.point_id_trfer}});

        if(trip){
            dbo::ARX_TRANSFER atr(tr, *trip,part_key);
            session.insert(atr);
            //delete
            //            auto cur = make_curs("DELETE FROM trfer_trips where point_id = :tp");
            //            cur.bind(":tp", tr.point_id_trfer);
            //            cur.exec();
        }
    }
}

void arx_tckin_segments(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    LogTrace(TRACE3) << __FUNCTION__ << " grp_id: " << grp_id;
    dbo::Session session;
    std::vector<dbo::TCKIN_SEGMENTS> segs = session.query<dbo::TCKIN_SEGMENTS>()
            .where("grp_id = :grp_id and seg_no > 0")
            .for_update(true)
            .setBind({{":grp_id", grp_id.get()}});
    //LogTrace(TRACE3) << " segs size : " << segs.size();

    for(const auto& s : segs) {
        static int added = 0;
        //LogTrace(TRACE3) << " get transfer trips";
        std::optional<dbo::TRFER_TRIPS> trip = session.query<dbo::TRFER_TRIPS>()
                .where("point_id = :tp")
                .for_update(true)
                .setBind({{":tp", s.point_id_trfer}});

        if(trip) {
            dbo::ARX_TCKIN_SEGMENTS atr(s, *trip,part_key);
            session.insert(atr);
            added ++;
            //LogTrace(TRACE3) << " added = " << added;

            //            auto cur = make_curs("DELETE FROM tckin_segments where point_id_trfer = :tp");
            //            cur.bind(":tp", s.point_id_trfer);
            //            cur.exec();
        }
    }
}

void arx_value_bag(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::VALUE_BAG> bags2 = session.query<dbo::VALUE_BAG>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : bags2) {
        dbo::ARX_VALUE_BAG ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_grp_norms(const GrpId_t& grp_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::GRP_NORMS> norms = session.query<dbo::GRP_NORMS>()
            .where("grp_id = :grp_id")
            .for_update(true)
            .setBind({{"grp_id", grp_id.get()}});

    for(const auto &cs : norms) {
        dbo::ARX_GRP_NORMS ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_pax_norms(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::PAX_NORMS> norms = session.query<dbo::PAX_NORMS>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});

    for(const auto &cs : norms) {
        dbo::ARX_PAX_NORMS ascs(cs,part_key);
        session.insert(ascs);
    }
}


void arx_pax_rem(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::PAX_REM> norms = session.query<dbo::PAX_REM>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});

    for(const auto &cs : norms) {
        dbo::ARX_PAX_REM ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_transfer_subcls(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::TRANSFER_SUBCLS> norms = session.query<dbo::TRANSFER_SUBCLS>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});

    for(const auto &cs : norms) {
        dbo::ARX_TRANSFER_SUBCLS ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_pax_doc(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    LogTrace(TRACE5) << __FUNCTION__ << " pax_id:" << pax_id;
    dbo::Session session;
    std::vector<dbo::PAX_DOC> docs = session.query<dbo::PAX_DOC>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});
    LogTrace(TRACE5) << " docs found : " << docs.size();

    for(const auto &d : docs) {
        dbo::ARX_PAX_DOC ad(d,part_key);
        session.insert(ad);
    }
}

void arx_pax_doco(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::PAX_DOCO> norms = session.query<dbo::PAX_DOCO>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});

    for(const auto &cs : norms) {
        dbo::ARX_PAX_DOCO ascs(cs,part_key);
        session.insert(ascs);
    }
}

void arx_pax_doca(const PaxId_t& pax_id, const Dates::DateTime_t& part_key)
{
    dbo::Session session;
    std::vector<dbo::PAX_DOCA> norms = session.query<dbo::PAX_DOCA>()
            .where("pax_id = :pax_id")
            .for_update(true)
            .setBind({{"pax_id", pax_id.get()}});

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
    deletePaxAlarms(pax_id);
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
                         " DELETE FROM trip_pers_weights WHERE point_id=:point_id;       "
                         " DELETE FROM trip_auto_weighing WHERE point_id=:point_id;               "
                         " UPDATE trfer_trips SET point_id_spp=NULL WHERE point_id_spp=:point_id; "
                         " DELETE FROM pax_seats WHERE point_id=:point_id;         "
                         " DELETE FROM trip_tasks WHERE point_id=:point_id;        "
                         " DELETE FROM counters_by_subcls WHERE point_id=:point_id;"
                         " DELETE FROM trip_vouchers WHERE point_id=:point_id;"
                         " DELETE FROM confirm_print_vo_unreg WHERE point_id = :point_id; "
                         " DELETE FROM hotel_acmd_pax WHERE point_id = :point_id; "
                         " DELETE FROM hotel_acmd_free_pax WHERE point_id = :point_id; "
                         " DELETE FROM hotel_acmd_dates WHERE point_id = :point_id; "
                         "END;");
    cur.bind(":point_id", point_id);
    cur.exec();

    make_db_curs("DELETE FROM trip_alarms WHERE point_id=:point_id",     PgOra::getRWSession("TRIP_ALARMS")).bind(":point_id", point_id.get()).exec();
    make_db_curs("DELETE FROM trip_rpt_person WHERE point_id=:point_id", PgOra::getRWSession("TRIP_RPT_PERSON")).bind(":point_id", point_id.get()).exec();
    make_db_curs("DELETE FROM trip_apis_params WHERE point_id=:point_id",PgOra::getRWSession("TRIP_APIS_PARAMS")).bind(":point_id", point_id.get()).exec();
    make_db_curs("DELETE FROM iapi_pax_data WHERE point_id=:point_id",   PgOra::getRWSession("IAPI_PAX_DATA")).bind(":point_id", point_id.get()).exec();
    make_db_curs("DELETE FROM utg_prl WHERE point_id=:point_id",         PgOra::getRWSession("UTG_PRL")).bind(":point_id", point_id.get()).exec();

    make_db_curs("DELETE FROM wb_msg_text where id in (SELECT id FROM wb_msg WHERE point_id = :point_id)", PgOra::getRWSession("WB_MSG_TEXT")).bind(":point_id", point_id.get()).exec();
    make_db_curs("DELETE FROM wb_msg where point_id = :point_id",        PgOra::getRWSession("WB_MSG")).bind(":point_id", point_id.get()).exec();

    deleteEtickets(point_id.get());
    deleteEmdocs(point_id.get());
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
    LogTrace(TRACE5) << __func__ << " arx_date: " << arx_date;
    dbo::Session session;

    std::vector<dbo::TLG_OUT> tlg_outs = session.query<dbo::TLG_OUT>()
            .where("POINT_ID is null and TIME_CREATE < :arx_date")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});


    for(const auto &tg : tlg_outs) {
        Dates::DateTime_t part_key_nvl = dbo::coalesce(tg.time_send_act, tg.time_send_scd, tg.time_create);
        //NVL(time_send_act,NVL(time_send_scd,time_create)
        dbo::ARX_TLG_OUT ascs(tg, part_key_nvl);
        session.insert(ascs);
    }

    return tlg_outs.size();

    //TODO
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
    LogTrace(TRACE5) << __func__ << " arx_date: " << arx_date;
    dbo::Session session;
    //Dates::DateTime_t elapsed = arx_date - Dates::days(30);
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
    //TODO
    //    auto cur = make_curs("DELETE FROM events_bilingual WHERE ID1 is NULL  and type = :evtTlg and TIME >= :arx_date - 30 and TIME < arx_date");
    //    cur.bind(":evtTlg", EncodeEventType(evtTlg))
    //       .bind(":arx_date", arx_date)
    //       .exec();
    return events.size();
}

int arx_events_noflt3(const Dates::DateTime_t& arx_date, int remain_rows)
{
    LogTrace(TRACE5) << __func__ << " arx_date: " << arx_date;
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
    //TODO
    //    auto cur = make_curs("DELETE FROM events_bilingual WHERE ID1 is NULL  and type = :evtTlg and TIME >= :arx_date - 30 and TIME < arx_date");
    //    cur.bind(":evtTlg", EncodeEventType(evtTlg))
    //       .bind(":arx_date", arx_date)
    //       .exec();
    return events.size();
}

int arx_stat_zamar(const Dates::DateTime_t& arx_date, int remain_rows)
{
    LogTrace(TRACE5) << __func__ << " arx_date: " << arx_date;
    dbo::Session session;
    std::vector<dbo::STAT_ZAMAR> stats =  session.query<dbo::STAT_ZAMAR>()
            .where("TIME < :arx_date")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date}, {":remain_rows", remain_rows}});
    for(const auto & ev : stats) {
        dbo::ARX_STAT_ZAMAR aev(ev, ev.time);
        session.insert(aev);
    }
    //todo
    //    auto cur = make_curs("DELETE FROM stat_zamar WHERE TIME < arx_date");
    //    cur.bind(":arx_date", arx_date).exec();
    return stats.size();
}

void move_noflt(const Dates::DateTime_t& arx_date, int max_rows, int time_duration, int& step)
{
    LogTrace(TRACE5) << __FUNCTION__ << " arx_date: " << arx_date;
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
    LogTrace5 << __func__;
    if(step <= 0) {
        step = 1;
    }
    Dates::DateTime_t arx_date = utcdate-Dates::days(ARX::ARX_DAYS());
    move_noflt(arx_date, max_rows, duration, step);
    ASTRA::commitAndCallCommitHooks();
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
    LogTrace5 << __func__ << " point_id: " << point_id;
    ckin::delete_typeb_data(point_id);
    auto cur = make_curs(
                "BEGIN "
                "DELETE FROM typeb_data_stat WHERE point_id = :point_id; "
                "DELETE FROM crs_data_stat WHERE point_id = :point_id; "
                "DELETE FROM tlg_comp_layers WHERE point_id = :point_id; "
                "UPDATE crs_displace2 SET point_id_tlg=NULL WHERE point_id_tlg = :point_id;"
                "END;");
    cur.bind(":point_id", point_id).exec();

    dbo::Session session;
    std::vector<int> pnrids = session.query<int>("SELECT trfer_id").from("tlg_transfer")
            .where("point_id = :point_id")
            .setBind({{":point_id", point_id.get()}});
    std::vector<int> grpids = session.query<int>("SELECT grp_id").from("trfer_grp, tlg_transfer")
            .where("tlg_transfer.trfer_id = trfer_grp.trfer_id AND "
                   "tlg_transfer.point_id_out = :point_id")
            .setBind({{":point_id", point_id.get()}});
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
                          "    tlg_transfer.tlg_id=tlg_source.tlg_id "
                          "FETCH FIRST 1 ROWS ONLY); "
                          "END IF;"
                          "END;");
    cur3.bind(":point_id", point_id).exec();

}


bool TArxTlgTrips::Next(size_t max_rows, int duration)
{
    LogTrace5 << __func__;
    auto points = getTlgTripPoints(utcdate-Dates::days(ARX::ARX_DAYS()), max_rows);

    while (!points.empty())
    {
        PointId_t point_id = points.front();
        points.erase(points.begin());
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
    LogTrace5 << __func__ << " arx_date: " << arx_date;
    std::map<int, Dates::DateTime_t> res;
    int tlg_id;
    Dates::DateTime_t time_receive;
    auto cur = make_curs(
                "SELECT id,time_receive "
                "FROM tlgs_in "
                "WHERE time_receive < :arx_date AND "
                "      NOT EXISTS(SELECT * FROM tlg_source WHERE tlg_source.tlg_id=tlgs_in.id FETCH FIRST 1 ROWS ONLY) AND "
                "      NOT EXISTS(SELECT * FROM tlgs_in a WHERE a.id=tlgs_in.id AND time_receive >= :arx_date FETCH FIRST 1 ROWS ONLY) ");
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
    LogTrace5 << __func__;
    std::map<int, Dates::DateTime_t> tlg_ids = getTlgIds(utcdate - Dates::days(ARX::ARX_DAYS()), max_rows);
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
    }
    return bag_rates.size();
}

int arx_value_bag_taxes(const Dates::DateTime_t& arx_date, int remain_rows)
{
    dbo::Session session;

    std::vector<dbo::VALUE_BAG_TAXES> bag_taxes = session.query<dbo::VALUE_BAG_TAXES>()
            .where("last_date < :arx_date AND "
                   "NOT EXISTS (SELECT * FROM value_bag WHERE value_bag.tax_id=value_bag_taxes.id FETCH FIRST 1 ROWS ONLY)")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date},
                      {":remain_rows", remain_rows}});

    for(const auto & bn : bag_taxes) {
        dbo::ARX_VALUE_BAG_TAXES abn(bn, bn.last_date);
        session.insert(abn);
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

    for(const auto & bn : mark_trips) {
        auto cur = make_db_curs("delete from MARK_TRIPS where POINT_ID = :point_id",
                                PgOra::getROSession("MARK_TRIPS"));
        cur.bind(":point_id", bn.point_id).exec();
    }
    return mark_trips.size();
}

void norms_rates_etc(const Dates::DateTime_t& arx_date, int max_rows, int time_duration, int& step)
{
    LogTrace5 << __func__ << " arx_date: " << arx_date;
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
    LogTrace5 << __func__;
    if(step <= 0) {
        step = 1;
    }
    Dates::DateTime_t arx_date = utcdate-Dates::days(ARX::ARX_DAYS());
    norms_rates_etc(arx_date, max_rows, duration, step);
    ASTRA::commitAndCallCommitHooks();
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

    const char* sql = PgOra::supportsPg("TLGS")
      ? "SELECT ID FROM TLGS WHERE time < :arx_date limit :remain_rows"
      : "SELECT ID FROM TLGS WHERE time < :arx_date AND rownum <= :remain_rows";

    DbCpp::CursCtl cur = make_db_curs(
        sql,
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
        make_db_curs("DELETE FROM TLG_STAT WHERE QUEUE_TLG_ID = :id", PgOra::getRWSession("TLG_STAT")).bind(":id", id).exec();
        make_db_curs("DELETE FROM TLG_ERROR WHERE ID = :id", PgOra::getRWSession("TLG_ERROR")).bind(":id", id).exec();
        make_db_curs("DELETE FROM TLG_QUEUE WHERE ID = :id", PgOra::getRWSession("TLG_QUEUE")).bind(":id", id).exec();
        make_db_curs("DELETE FROM TLGS_TEXT WHERE ID = :id", PgOra::getRWSession("TLGS_TEXT")).bind(":id", id).exec();
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

    std::vector<dbo::FILES> files = session.query<dbo::FILES>()
            .where("time < :arx_date")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date},
                      {":remain_rows", remain_rows}});

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
    auto cur = make_curs("delete from ROZYSK where TIME < :arx_date and ROWNUM <= :remain_rows");
    cur.bind(":arx_date", arx_date).bind(":remain_rows", remain_rows).exec();
    return cur.rowcount();
}

int delete_aodb_spp_files(const Dates::DateTime_t& arx_date, int remain_rows)
{
    int rowsize = 0;
    dbo::Session session;

    std::vector<dbo::AODB_SPP_FILES> files = session.query<dbo::AODB_SPP_FILES>()
            .where("filename<'SPP'||TO_CHAR(:arx_date, 'YYMMDD')||'.txt'")
            .fetch_first(":remain_rows")
            .for_update(true)
            .setBind({{":arx_date", arx_date},
                      {":remain_rows", remain_rows}});

    for(const auto & f : files) {
        auto cur = make_curs("delete from AODB_EVENTS "
                             "where filename= :filename and point_addr= :point_addr "
                             "and airline= :airline AND rownum<= :remain_rows");
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
    LogTrace5 << __func__ << " arx_date: " << arx_date;
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
    LogTrace5 << __func__;
    if(step <= 0) {
        step = 1;
    }
    Dates::DateTime_t arx_date = utcdate - Dates::days(ARX::ARX_DAYS());
    tlgs_files_etc(arx_date, max_rows, duration, step);
    ASTRA::commitAndCallCommitHooks();
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

bool del_from_tlg_queue(const AIRSRV_MSG& tlg_in);
bool del_from_tlg_queue_by_status(const AIRSRV_MSG& tlg_in, const std::string& status);
bool upd_tlg_queue_status(const AIRSRV_MSG& tlg_in,
                          const std::string& curStatus, const std::string& newStatus);
bool upd_tlgs_by_error(const AIRSRV_MSG& tlg_in, const std::string& error);

// namespace TlgHandling {
// void updateTlgToPostponed(const tlgnum_t& tnum);
// bool isTlgPostponed(const tlgnum_t& tnum);
// struct PostponeEdiHandling;
// //void PostponeEdiHandling::addToQueue(const tlgnum_t& tnum);

// }

namespace PG_ARX {

bool test_arx_daily(const Dates::DateTime_t& utcdate, int step)
{
    LogTrace(TRACE5) << __FUNCTION__ << " step: " << step;

    dbo::initStructures();
    auto arxMove = create_arx_manager(utcdate, step);

    LogTrace(TRACE5) << "arx_daily_pg: "
                     << arxMove->TraceCaption()
                     << " started";

    arxMove->BeforeProc();
    time_t time_finish = time(NULL)+ARX::ARX_DURATION();
    int duration = 0;
    do{
        duration = time_finish - time(NULL);
        if (duration<=0)
        {
            LogTrace(TRACE5) << "arx_daily_pg: "
                             << arxMove->Processed()
                             << " iterations processed";
            return false;
        };
    }
    while(arxMove->Next(ARX::ARX_MAX_ROWS(), duration));
    LogTrace(TRACE5) << "arx_daily_pg: "
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

    AIRSRV_MSG tlg_in {
        .num = tlg_id,
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

START_TEST(additional)
{
// src/tlg/typeb_handler.cpp
// handle_tlg
{
    static DB::TQuery TlgQry(PgOra::getROSession("TLG_QUEUE"));
    if (TlgQry.SQLText.empty())
    {
        //внимание порядок объединения таблиц важен!
        TlgQry.Clear();
        TlgQry.SQLText=
            "SELECT tlg_queue.id,tlg_queue.time,ttl, "
            "       tlg_queue.tlg_num,tlg_queue.sender, "
            "       COALESCE(tlg_queue.proc_attempt,0) AS proc_attempt "
            "FROM tlg_queue "
            "WHERE tlg_queue.receiver=:receiver AND "
            "      tlg_queue.type='INB' AND tlg_queue.status='PUT' "
            "ORDER BY tlg_queue.time,tlg_queue.id";
        TlgQry.CreateVariable("receiver",otString,OWN_CANON_NAME());
    };
}

// src/tlg/apps_answer_emul.cc
// handle_tlg
{
    static DB::TQuery TlgQry(PgOra::getROSession("TLG_QUEUE"));
    if (TlgQry.SQLText.empty())
    {
        //внимание порядок объединения таблиц важен!
        TlgQry.Clear();
        TlgQry.SQLText=
            "SELECT tlg_queue.id,tlg_queue.time,ttl, "
            "       tlg_queue.tlg_num,tlg_queue.sender, "
            "       COALESCE(tlg_queue.proc_attempt,0) AS proc_attempt "
            "FROM tlg_queue "
            "WHERE tlg_queue.receiver=:receiver AND "
            "      tlg_queue.type='OAPP' "
            "ORDER BY tlg_queue.time,tlg_queue.id";
        // для теста вместо APPS::getAPPSRotName() используем OWN_CANON_NAME()
        // было:
        // TlgQry.CreateVariable( "receiver", otString, APPS::getAPPSRotName() );
        TlgQry.CreateVariable( "receiver", otString, OWN_CANON_NAME() );
    };
}

// src/tlg/apps_handler.cc
// handle_tlg
{
    static DB::TQuery TlgQry(PgOra::getROSession("TLG_QUEUE"));
    if (TlgQry.SQLText.empty())
    {
        //внимание порядок объединения таблиц важен!
        TlgQry.Clear();
        TlgQry.SQLText=
            "SELECT tlg_queue.id,tlg_queue.time,ttl, "
            "       tlg_queue.tlg_num,tlg_queue.sender, "
            "       COALESCE(tlg_queue.proc_attempt,0) AS proc_attempt "
            "FROM tlg_queue "
            "WHERE tlg_queue.receiver=:receiver AND "
            "      tlg_queue.type='IAPP' AND tlg_queue.status='PUT' "
            "ORDER BY tlg_queue.time,tlg_queue.id";
        TlgQry.CreateVariable( "receiver", otString, OWN_CANON_NAME() );
    };
}

// src/tlg/edi_handler.cpp
// handle_tlg
{
    const std::string handler_id = "Handler Id";
    static DB::TQuery TlgQry(PgOra::getROSession("TLG_QUEUE"));
    if (TlgQry.SQLText.empty())
    {
        //внимание порядок объединения таблиц важен!
        TlgQry.Clear();
        TlgQry.SQLText=
            "SELECT tlg_queue.id,tlg_queue.time,ttl, "
            "       tlg_queue.tlg_num,tlg_queue.sender, "
            "       COALESCE(tlg_queue.proc_attempt,0) AS proc_attempt "
            "FROM tlg_queue "
            "WHERE tlg_queue.receiver=:receiver AND "
            "      tlg_queue.type='INA' AND tlg_queue.status='PUT' AND "
            "      tlg_queue.subtype=:handler_id "
            "ORDER BY tlg_queue.time,tlg_queue.id";
        TlgQry.CreateVariable("receiver",otString,OWN_CANON_NAME());
        TlgQry.CreateVariable("handler_id",otString,handler_id);
    };
}
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
    ADD_TEST(check_isTlgPostponed);
    ADD_TEST(additional);
}
TCASEFINISH;
#undef SUITENAME

#endif //XP_TESTING

