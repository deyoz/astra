#include "dbostructures.h"

#define NICKNAME "FELIX"
#define NICKTRACE FELIX_TRACE
#include <serverlib/slogger.h>

namespace dbo
{

#define INIT_DBO(class_name) { \
    auto & session = dbo::Session::getInstance(); \
    session.MapClass<class_name>(#class_name); \
}

void initStructures()
{
    static bool initialized = false;
    if(!initialized) {
        //auto & session = dbo::Session::getInstance();
        //session.MapClass<Points>("Points");
        INIT_DBO(Points);
        INIT_DBO(Arx_Points);
        INIT_DBO(Move_Arx_Ext);
        INIT_DBO(Move_Ref);
        INIT_DBO(Arx_Move_Ref);
        INIT_DBO(Lang_Types);
        INIT_DBO(Events_Bilingual);
        INIT_DBO(Arx_Events);
        INIT_DBO(Pax_Grp);
        INIT_DBO(Arx_Pax_Grp);
        INIT_DBO(Mark_Trips);
        INIT_DBO(Arx_Mark_Trips);
        INIT_DBO(Self_Ckin_Stat);
        INIT_DBO(Arx_Self_Ckin_Stat);
        INIT_DBO(RFISC_STAT);
        INIT_DBO(ARX_RFISC_STAT);
        INIT_DBO(STAT_SERVICES);
        INIT_DBO(ARX_STAT_SERVICES);
        INIT_DBO(STAT_REM);
        INIT_DBO(ARX_STAT_REM);
        INIT_DBO(LIMITED_CAPABILITY_STAT);
        INIT_DBO(ARX_LIMITED_CAPABILITY_STAT);
        INIT_DBO(PFS_STAT);
        INIT_DBO(ARX_PFS_STAT);
        INIT_DBO(STAT_AD);
        INIT_DBO(ARX_STAT_AD);
        INIT_DBO(STAT_HA);
        INIT_DBO(ARX_STAT_HA);
        INIT_DBO(STAT_VO);
        INIT_DBO(ARX_STAT_VO);
        INIT_DBO(STAT_REPRINT);
        INIT_DBO(ARX_STAT_REPRINT);
        INIT_DBO(TRFER_PAX_STAT);
        INIT_DBO(ARX_TRFER_PAX_STAT);
        INIT_DBO(BI_STAT);
        INIT_DBO(ARX_BI_STAT);
        INIT_DBO(AGENT_STAT);
        INIT_DBO(ARX_AGENT_STAT);
        INIT_DBO(STAT);
        INIT_DBO(ARX_STAT);
        INIT_DBO(TRFER_STAT);
        INIT_DBO(ARX_TRFER_STAT);
        INIT_DBO(TLG_OUT);
        INIT_DBO(ARX_TLG_OUT);
        INIT_DBO(TRIP_CLASSES);
        INIT_DBO(ARX_TRIP_CLASSES);
        INIT_DBO(TRIP_DELAYS);
        INIT_DBO(ARX_TRIP_DELAYS);
        INIT_DBO(TRIP_LOAD);
        INIT_DBO(ARX_TRIP_LOAD);
        INIT_DBO(TRIP_SETS);
        INIT_DBO(ARX_TRIP_SETS);
        INIT_DBO(CRS_DISPLACE2);
        INIT_DBO(ARX_CRS_DISPLACE2);
        INIT_DBO(TRIP_STAGES);
        INIT_DBO(ARX_TRIP_STAGES);
        INIT_DBO(BAG_RECEIPTS);
        INIT_DBO(ARX_BAG_RECEIPTS);
        INIT_DBO(BAG_PAY_TYPES);
        INIT_DBO(ARX_BAG_PAY_TYPES);
        INIT_DBO(ANNUL_BAG);
        INIT_DBO(ARX_ANNUL_BAG);
        INIT_DBO(ANNUL_TAGS);
        INIT_DBO(ARX_ANNUL_TAGS);
        INIT_DBO(UNACCOMP_BAG_INFO);
        INIT_DBO(ARX_UNACCOMP_BAG_INFO);
        INIT_DBO(BAG2);
        INIT_DBO(ARX_BAG2);
        INIT_DBO(BAG_PREPAY);
        INIT_DBO(ARX_BAG_PREPAY);
        INIT_DBO(BAG_TAGS);
        INIT_DBO(ARX_BAG_TAGS);
        INIT_DBO(PAID_BAG);
        INIT_DBO(ARX_PAID_BAG);
        INIT_DBO(PAX);
        INIT_DBO(ARX_PAX);
        INIT_DBO(TRANSFER);
        INIT_DBO(TRFER_TRIPS);
        INIT_DBO(ARX_TRANSFER);
        INIT_DBO(TCKIN_SEGMENTS);
        INIT_DBO(ARX_TCKIN_SEGMENTS);
        INIT_DBO(VALUE_BAG);
        INIT_DBO(ARX_VALUE_BAG);
        INIT_DBO(GRP_NORMS);
        INIT_DBO(ARX_GRP_NORMS);
        INIT_DBO(PAX_NORMS);
        INIT_DBO(ARX_PAX_NORMS);
        INIT_DBO(PAX_REM);
        INIT_DBO(ARX_PAX_REM);
        INIT_DBO(TRANSFER_SUBCLS);
        INIT_DBO(ARX_TRANSFER_SUBCLS);
        INIT_DBO(PAX_DOC);
        INIT_DBO(ARX_PAX_DOC);
        INIT_DBO(PAX_DOCO);
        INIT_DBO(ARX_PAX_DOCO);
        INIT_DBO(PAX_DOCA);
        INIT_DBO(ARX_PAX_DOCA);
        INIT_DBO(STAT_ZAMAR);
        INIT_DBO(ARX_STAT_ZAMAR);
        INIT_DBO(TLG_TRANSFER);
        INIT_DBO(TRFER_GRP);
        INIT_DBO(BAG_NORMS);
        INIT_DBO(ARX_BAG_NORMS);
        INIT_DBO(BAG_RATES);
        INIT_DBO(ARX_BAG_RATES);
        INIT_DBO(VALUE_BAG_TAXES);
        INIT_DBO(ARX_VALUE_BAG_TAXES);
        INIT_DBO(EXCHANGE_RATES);
        INIT_DBO(ARX_EXCHANGE_RATES);
        INIT_DBO(TLG_STAT);
        INIT_DBO(ARX_TLG_STAT);
        INIT_DBO(TLGS);
        INIT_DBO(FILES);
        INIT_DBO(KIOSK_EVENTS);
        INIT_DBO(AODB_SPP_FILES);
    }
    initialized = true;
}


std::vector<dbo::AGENT_STAT> readOraAgentsStat(PointId_t point_id)
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
    int user_id = 0;

    auto cur = make_curs("select "
    "  ast.user_id, ast.desk, ast.ondate, ast.pax_time, ast.pax_amount, "
    "  ast.dpax_amount.inc, ast.dpax_amount.dec, "
    "  ast.dtckin_amount.inc, ast.dtckin_amount.dec, "
    "  ast.dbag_amount.inc, ast.dbag_amount.dec, "
    "  ast.dbag_weight.inc, ast.dbag_weight.dec, "
    "  ast.drk_amount.inc, ast.drk_amount.dec, "
    "  ast.drk_weight.inc, ast.drk_weight.dec "
    "from agent_stat ast where ast.point_id = :point_id ");
    cur.def(user_id).def(desk).def(ondate).def(pax_time).def(pax_amount)
       .def(dpax_amount_inc).def(dpax_amount_dec)
       .def(dtckin_amount_inc).def(dtckin_amount_dec)
       .def(dbag_amount_inc).def(dbag_amount_dec)
       .def(dbag_weight_inc).def(dbag_weight_dec)
       .def(drk_amount_inc).def(drk_amount_dec)
       .def(drk_weight_inc).def(drk_weight_dec)
       .bind(":point_id", point_id)
       .exec();

    std::vector<dbo::AGENT_STAT> agents_stat;
    while(!cur.fen()) {
        dbo::AGENT_STAT as;
        as.dpax_amount_inc = dpax_amount_inc;
        as.dpax_amount_dec = dpax_amount_dec;
        as.dtckin_amount_inc = dtckin_amount_inc;
        as.dtckin_amount_dec = dtckin_amount_dec;
        as.dbag_amount_dec = dbag_amount_dec;
        as.dbag_amount_inc = dbag_amount_inc;
        as.dbag_weight_dec = dbag_weight_dec;
        as.dbag_weight_inc = dbag_weight_inc;
        as.drk_amount_inc = drk_amount_inc;
        as.drk_amount_dec = drk_amount_dec;
        as.drk_weight_inc = drk_weight_inc;
        as.drk_weight_dec = drk_weight_dec;
        as.desk = desk;
        as.ondate = ondate;
        as.pax_time = pax_time;
        as.pax_amount = pax_amount;
        as.point_id = point_id.get();

        agents_stat.push_back(std::move(as));
    }
    return agents_stat;
}

std::vector<dbo::ARX_AGENT_STAT> readOraArxAgentsStat()
{
    int point_id;
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
    int user_id = 0;
    Dates::DateTime_t part_key;
    Dates::DateTime_t point_part_key;

    auto cur = make_curs("select "
    "  ast.point_id, ast.user_id, ast.desk, ast.ondate, ast.pax_time, ast.pax_amount, "
    "  ast.dpax_amount.inc, ast.dpax_amount.dec, "
    "  ast.dtckin_amount.inc, ast.dtckin_amount.dec, "
    "  ast.dbag_amount.inc, ast.dbag_amount.dec, "
    "  ast.dbag_weight.inc, ast.dbag_weight.dec, "
    "  ast.drk_amount.inc, ast.drk_amount.dec, "
    "  ast.drk_weight.inc, ast.drk_weight.dec, "
    "  ast.part_key, ast.point_part_key "
    " from arx_agent_stat ast");
    cur.def(point_id).def(user_id).def(desk).def(ondate).def(pax_time).def(pax_amount)
       .def(dpax_amount_inc).def(dpax_amount_dec)
       .def(dtckin_amount_inc).def(dtckin_amount_dec)
       .def(dbag_amount_inc).def(dbag_amount_dec)
       .def(dbag_weight_inc).def(dbag_weight_dec)
       .def(drk_amount_inc).def(drk_amount_dec)
       .def(drk_weight_inc).def(drk_weight_dec)
       .def(part_key).def(point_part_key)
       .exec();

    std::vector<dbo::ARX_AGENT_STAT> agents_stat;
    while(!cur.fen()) {
        dbo::ARX_AGENT_STAT as;
        as.point_id = point_id;
        as.dpax_amount_inc = dpax_amount_inc;
        as.dpax_amount_dec = dpax_amount_dec;
        as.dtckin_amount_inc = dtckin_amount_inc;
        as.dtckin_amount_dec = dtckin_amount_dec;
        as.dbag_amount_dec = dbag_amount_dec;
        as.dbag_amount_inc = dbag_amount_inc;
        as.dbag_weight_dec = dbag_weight_dec;
        as.dbag_weight_inc = dbag_weight_inc;
        as.drk_amount_inc = drk_amount_inc;
        as.drk_amount_dec = drk_amount_dec;
        as.drk_weight_inc = drk_weight_inc;
        as.drk_weight_dec = drk_weight_dec;
        as.desk = desk;
        as.ondate = ondate;
        as.pax_time = pax_time;
        as.pax_amount = pax_amount;
        as.part_key = part_key;
        as.point_part_key = point_part_key;
        agents_stat.push_back(std::move(as));
    }
    return agents_stat;
}

}

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef XP_TESTING

#include "xp_testing.h"

using namespace xp_testing;

START_TEST(test_bag_prepay)
{
    LogTrace1 << " MY TEST DOUBLE PREPAY";
    std::string aircode = "AER";
    int grp_id = 5;
    int receipt_id = 12;
    std::string no = "n";
    double value = 102.21234;
    Dates::DateTime_t part_key = Dates::second_clock::universal_time();

    auto cur = get_pg_curs("INSERT INTO ARX_BAG_PREPAY(aircode, grp_id, receipt_id, value, "
                           "no, part_key) VALUES(:aircode, :grp_id, :receipt_id, :value, "
                           ":no, :part_key)");
    cur.bind(":aircode", aircode)
       .bind(":grp_id", grp_id)
       .bind(":receipt_id", receipt_id)
       .bind(":no", no)
       .bind(":value", value)
       .bind(":part_key", part_key)
       .exec();


    int read_receipt_id = 0;
    double read_value = 1.0;

    auto cur2 = get_pg_curs("select RECEIPT_ID, VALUE from ARX_BAG_PREPAY where GRP_ID = 5 ");
    cur2.def(read_receipt_id).defNull(read_value, 0.0).EXfet();

    auto cur3 = get_pg_curs("delete from ARX_BAG_PREPAY");
    cur3.exec();
    fail_unless(12 == read_receipt_id, "failed: read_receipt_id=%lf expected 12", read_receipt_id);
    fail_unless(102.21234 == read_value, "failed: read_value=%lf expected 102.21234", read_value);
}
END_TEST;


START_TEST(test_bag_pay_types)
{
    LogTrace1 << " MY TEST LONG DOUBLE BAG PAY TYPES";
    int num = 2;
    float pay_rate_sum = 1034.33;
    std::string pay_type = "ab";
    int receipt_id = 12;
    Dates::DateTime_t part_key = Dates::second_clock::universal_time();

    auto cur = get_pg_curs("INSERT INTO ARX_BAG_PAY_TYPES(num, pay_rate_sum, pay_type, receipt_id, part_key) "
                           " VALUES(:num, :pay_rate_sum, :pay_type, :receipt_id, :part_key)");
    cur.bind(":num", num)
       .bind(":pay_rate_sum", pay_rate_sum)
       .bind(":pay_type", pay_type)
       .bind(":receipt_id", receipt_id)
       .bind(":part_key", part_key)
       .exec();


    int read_receipt_id = 0;
    float read_pay_rate_sum = 1.0;

    auto cur2 = get_pg_curs("select receipt_id, pay_rate_sum from ARX_BAG_PAY_TYPES where NUM = 2 ");
    cur2.def(read_receipt_id).defNull(read_pay_rate_sum, 1.2).EXfet();

    auto cur3 = get_pg_curs("delete from ARX_BAG_PAY_TYPES");
    cur3.exec();
    fail_unless(12 == read_receipt_id, "failed: read_receipt_id=%lf expected 12", read_receipt_id);
    fail_unless(std::abs(1034.33 - read_pay_rate_sum) < 0.0001, "failed: read_value=%lf expected near "
                                                                "1034.33", read_pay_rate_sum);
}
END_TEST;

START_TEST(test_bag_value)
{
    LogTrace1 << " MY TEST LONG DOUBLE BAG VALUE";
    int grp_id = 5;
    int num = 2;
    float tax = 124.1;
    double value = 124.12;
    std::string value_cur = "ab";
    Dates::DateTime_t part_key = Dates::second_clock::universal_time();

    auto cur = get_pg_curs("INSERT INTO ARX_VALUE_BAG(grp_id, num, value, value_cur, tax, part_key) "
                           " VALUES(:grp_id, :num, :value, :value_cur, :tax, :part_key)");
    cur.bind(":grp_id", grp_id)
       .bind(":num", num)
       .bind(":value", value)
       .bind(":value_cur", value_cur)
       .bind(":tax", tax)
       .bind(":part_key", part_key)
       .exec();


    float read_tax = 0;
    float read_value = 0;

    auto cur2 = get_pg_curs("select tax, value from ARX_VALUE_BAG where NUM = 2 ");
    cur2.defNull(read_tax, 0.0).defNull(read_value, 0.0).EXfet();

    auto cur3 = get_pg_curs("delete from ARX_VALUE_BAG");
    cur3.exec();
    fail_unless(std::abs(124.1 - read_tax) < 0.0001, "failed: read_receipt_id=%lf expected near 124.1", read_tax);
    fail_unless(std::abs(124.12 == read_value) < 0.0001, "failed: read_value=%lf expected near 124.12", read_value);
}
END_TEST;


#define SUITENAME "dbo_tests"
TCASEREGISTER( nullptr, nullptr)
{
    ADD_TEST( test_bag_prepay );
    ADD_TEST( test_bag_pay_types );
    ADD_TEST( test_bag_value );
}
TCASEFINISH;
#undef SUITENAME


#endif //XP_TESTING

