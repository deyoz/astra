#include "check_grp_unification.h"

#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"
#include "pax_db.h"
#include "grp_db.h"
#include "passenger.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

namespace {

struct BaggagePool
{
  int num;
  int value_bag_num;
};

std::vector<BaggagePool> loadBaggagePools(GrpId_t grp_id, int bag_pool_num)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id;
    std::vector<BaggagePool> result;
    BaggagePool pool = { ASTRA::NoExists, ASTRA::NoExists };
    auto cur = make_db_curs(
          "SELECT num, value_bag_num "
          "FROM bag2 "
          "WHERE grp_id=:grp_id "
          "AND bag_pool_num=:bag_pool_num ",
          PgOra::getROSession("BAG2"));

    cur.stb()
        .def(pool.num)
        .defNull(pool.value_bag_num, ASTRA::NoExists)
        .bind(":grp_id", grp_id.get())
        .bind(":bag_pool_num", bag_pool_num)
        .exec();

    while (!cur.fen()) {
      result.push_back(pool);
    }
    LogTrace(TRACE6) << __func__
                     << ": count=" << result.size();
    return result;
}

std::set<int> loadValueBaggageNums(GrpId_t grp_id)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id;
    std::set<int> result;
    int num = ASTRA::NoExists;
    auto cur = make_db_curs(
          "SELECT num "
          "FROM value_bag "
          "WHERE grp_id=:grp_id ",
          PgOra::getROSession("VALUE_BAG"));

    cur.stb()
        .def(num)
        .bind(":grp_id", grp_id.get())
        .exec();

    while (!cur.fen()) {
      result.insert(num);
    }
    LogTrace(TRACE6) << __func__
                     << ": count=" << result.size();
    return result;
}

std::set<int> loadBaggageNums(GrpId_t grp_id)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id;
    std::set<int> result;
    int num = ASTRA::NoExists;
    auto cur = make_db_curs(
          "SELECT num "
          "FROM bag2 "
          "WHERE grp_id=:grp_id ",
          PgOra::getROSession("BAG2"));

    cur.stb()
        .def(num)
        .bind(":grp_id", grp_id.get())
        .exec();

    while (!cur.fen()) {
      result.insert(num);
    }
    LogTrace(TRACE6) << __func__
                     << ": count=" << result.size();
    return result;
}

std::set<int> loadBaggageTagNums(GrpId_t grp_id)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id;
    std::set<int> result;
    int num = ASTRA::NoExists;
    auto cur = make_db_curs(
          "SELECT num "
          "FROM bag_tags "
          "WHERE grp_id=:grp_id ",
          PgOra::getROSession("BAG_TAGS"));

    cur.stb()
        .def(num)
        .bind(":grp_id", grp_id.get())
        .exec();

    while (!cur.fen()) {
      result.insert(num);
    }
    LogTrace(TRACE6) << __func__
                     << ": count=" << result.size();
    return result;
}

bool clearEventsBilingual(GrpId_t grp_id, PointId_t point_id, RegNo_t reg_no)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", point_id=" << point_id
                     << ", reg_no=" << reg_no;
    auto cur = make_db_curs(
          "UPDATE events_bilingual "
          "SET id2=NULL "
          "WHERE lang IN (:ru, :en) "
          "AND type IN (:evt_pax, :evt_pay) "
          "AND id1=:point_id "
          "AND id2=:reg_no "
          "AND id3=:grp_id "
          "AND NOT EXISTS( "
          "   SELECT reg_no FROM pax "
          "   WHERE grp_id=:grp_id "
          "   AND reg_no=:reg_no) ", // из-за возможного дублирования reg_no
          PgOra::getRWSession({"EVENTS_BILINGUAL","PAX"}));
    cur.stb()
        .bind(":grp_id", grp_id.get())
        .bind(":point_id", point_id.get())
        .bind(":reg_no", reg_no.get())
        .bind(":evt_pax", EncodeEventType(ASTRA::evtPax))
        .bind(":evt_pay", EncodeEventType(ASTRA::evtPay))
        .bind(":ru", "RU")
        .bind(":en", "EN")
        .exec();

    LogTrace(TRACE6) << __func__
                     << ": rowcount=" << cur.rowcount();
    return cur.rowcount() > 0;
}

bool clearEventsBilingual(GrpId_t grp_id, PointId_t point_id)
{
    // не чистим mark_trips потому что будет слишком долгая проверка pax_grp.point_id_mark
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", point_id=" << point_id;
    auto cur = make_db_curs(
          "UPDATE events_bilingual "
          "SET id2=NULL "
          "WHERE lang IN (:ru, :en) "
          "AND type = :evt_pax "
          "AND id1=:point_id "
          "AND id3=:grp_id ",
          PgOra::getRWSession("EVENTS_BILINGUAL"));
    cur.stb()
        .bind(":grp_id", grp_id.get())
        .bind(":point_id", point_id.get())
        .bind(":evt_pax", EncodeEventType(ASTRA::evtPax))
        .bind(":ru", "RU")
        .bind(":en", "EN")
        .exec();

    LogTrace(TRACE6) << __func__
                     << ": rowcount=" << cur.rowcount();
    return cur.rowcount() > 0;
}

int getCurrentSeqTid()
{
    LogTrace(TRACE6) << __func__;
    const int result = PgOra::getSeqCurrVal_int("CYCLE_TID__SEQ");
    LogTrace(TRACE6) << __func__
                     << ": result=" << result;
    return result;
}

bool updatePax_BagPoolNum(GrpId_t grp_id, int bag_pool_num, int new_bag_pool_num)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", bag_pool_num=" << bag_pool_num
                     << ", new_bag_pool_num=" << new_bag_pool_num;
    auto cur = make_db_curs(
          "UPDATE pax SET "
          "bag_pool_num=:new_bag_pool_num, "
          "tid=:tid "
          "WHERE grp_id=:grp_id "
          "AND bag_pool_num=:bag_pool_num",
          PgOra::getRWSession("PAX"));
    cur.stb()
        .bind(":grp_id", grp_id.get())
        .bind(":bag_pool_num", bag_pool_num)
        .bind(":new_bag_pool_num", new_bag_pool_num)
        .bind(":tid", getCurrentSeqTid())
        .exec();

    LogTrace(TRACE6) << __func__
                     << ": rowcount=" << cur.rowcount();
    return cur.rowcount() > 0;
}

bool updateValueBag_Num(GrpId_t grp_id, int num, int new_num)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", num=" << num
                     << ", new_num=" << new_num;
    auto cur = make_db_curs(
          "UPDATE value_bag "
          "SET num=:new_num "
          "WHERE grp_id=:grp_id "
          "AND num=:num",
          PgOra::getRWSession("VALUE_BAG"));
    cur.stb()
        .bind(":grp_id", grp_id.get())
        .bind(":num", num)
        .bind(":new_num", new_num)
        .exec();

    LogTrace(TRACE6) << __func__
                     << ": rowcount=" << cur.rowcount();
    return cur.rowcount() > 0;
}

bool updateBag2_Num(GrpId_t grp_id, int bag_num, int new_bag_num)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", bag_num=" << bag_num
                     << ", new_bag_num=" << new_bag_num;
    auto cur = make_db_curs(
          "UPDATE bag2 "
          "SET num=:new_num "
          "WHERE grp_id=:grp_id "
          "AND num=:num",
          PgOra::getRWSession("BAG2"));
    cur.stb()
        .bind(":grp_id", grp_id.get())
        .bind(":num", bag_num)
        .bind(":new_num", new_bag_num)
        .exec();

    LogTrace(TRACE6) << __func__
                     << ": rowcount=" << cur.rowcount();
    return cur.rowcount() > 0;
}

bool updateBag2_ValueBagNum(GrpId_t grp_id, int value_bag_num, int new_value_bag_num)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", value_bag_num=" << value_bag_num
                     << ", new_value_bag_num=" << new_value_bag_num;
    auto cur = make_db_curs(
          "UPDATE bag2 "
          "SET value_bag_num=:new_value_bag_num "
          "WHERE grp_id=:grp_id "
          "AND value_bag_num=:value_bag_num",
          PgOra::getRWSession("BAG2"));
    cur.stb()
        .bind(":grp_id", grp_id.get())
        .bind(":value_bag_num", value_bag_num)
        .bind(":new_value_bag_num", new_value_bag_num)
        .exec();

    LogTrace(TRACE6) << __func__
                     << ": rowcount=" << cur.rowcount();
    return cur.rowcount() > 0;
}

bool updateBag2_BagPoolNum(GrpId_t grp_id, int bag_pool_num, int new_bag_pool_num)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", bag_pool_num=" << bag_pool_num
                     << ", new_bag_pool_num=" << new_bag_pool_num;
    auto cur = make_db_curs(
          "UPDATE bag2 "
          "SET bag_pool_num=:new_bag_pool_num "
          "WHERE grp_id=:grp_id "
          "AND bag_pool_num=:bag_pool_num",
          PgOra::getRWSession("BAG2"));
    cur.stb()
        .bind(":grp_id", grp_id.get())
        .bind(":bag_pool_num", bag_pool_num)
        .bind(":new_bag_pool_num", new_bag_pool_num)
        .exec();

    LogTrace(TRACE6) << __func__
                     << ": rowcount=" << cur.rowcount();
    return cur.rowcount() > 0;
}

bool updateBagTags_BagNum(GrpId_t grp_id, int bag_num, int new_bag_num)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", bag_num=" << bag_num
                     << ", new_bag_num=" << new_bag_num;
    auto cur = make_db_curs(
          "UPDATE bag_tags "
          "SET bag_num=:new_bag_num "
          "WHERE grp_id=:grp_id "
          "AND bag_num=:bag_num",
          PgOra::getRWSession("BAG_TAGS"));
    cur.stb()
        .bind(":grp_id", grp_id.get())
        .bind(":bag_num", bag_num)
        .bind(":new_bag_num", new_bag_num)
        .exec();

    LogTrace(TRACE6) << __func__
                     << ": rowcount=" << cur.rowcount();
    return cur.rowcount() > 0;
}

bool updateBagTags_Num(GrpId_t grp_id, int bag_tag_num, int new_bag_tag_num)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", bag_tag_num=" << bag_tag_num
                     << ", new_bag_tag_num=" << new_bag_tag_num;
    auto cur = make_db_curs(
          "UPDATE bag_tags "
          "SET num=:new_num "
          "WHERE grp_id=:grp_id "
          "AND num=:num",
          PgOra::getRWSession("BAG_TAGS"));
    cur.stb()
        .bind(":grp_id", grp_id.get())
        .bind(":num", bag_tag_num)
        .bind(":new_num", new_bag_tag_num)
        .exec();

    LogTrace(TRACE6) << __func__
                     << ": rowcount=" << cur.rowcount();
    return cur.rowcount() > 0;
}

bool updatePaxGrp_BagRefuse(GrpId_t grp_id, int bag_refuse)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", bag_refuse=" << bag_refuse;
    auto cur = make_db_curs(
          "UPDATE pax_grp "
          "SET bag_refuse=:bag_refuse "
          "WHERE grp_id=:grp_id ",
          PgOra::getRWSession("PAX_GRP"));
    cur.stb()
        .bind(":grp_id", grp_id.get())
        .bind(":bag_refuse", bag_refuse)
        .exec();

    LogTrace(TRACE6) << __func__
                     << ": rowcount=" << cur.rowcount();
    return cur.rowcount() > 0;
}

void resetBaggageNums(GrpId_t grp_id)
{
    int new_value_bag_num = 1;
    const std::set<int> valueBagNums = loadValueBaggageNums(grp_id);
    for (int value_bag_num: valueBagNums) {
        if (value_bag_num != new_value_bag_num) {
            updateValueBag_Num(grp_id, value_bag_num, new_value_bag_num);
            updateBag2_ValueBagNum(grp_id, value_bag_num, new_value_bag_num);
        }
        new_value_bag_num += 1;
    }

    int new_bag_num = 1;
    const std::set<int> bagNums = loadBaggageNums(grp_id);
    for (int bag_num: bagNums) {
        if (bag_num != new_bag_num) {
            updateBag2_Num(grp_id, bag_num, new_bag_num);
            updateBagTags_BagNum(grp_id, bag_num, new_bag_num);
        }
        new_bag_num += 1;
    }

    int new_bag_tag_num = 1;
    const std::set<int> bagTagNums = loadBaggageTagNums(grp_id);
    for (int bag_tag_num: bagTagNums) {
        if (bag_tag_num != new_bag_tag_num) {
            updateBagTags_Num(grp_id, bag_tag_num, new_bag_tag_num);
        }
        new_bag_tag_num += 1;
    }
    // удалить из pax_norms, paid_bag???
}

} // namespace


void checkGroupUnification(GrpId_t grp_id)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id;
    CheckIn::TSimplePaxGrpItem group;
    if (!group.getByGrpId(grp_id.get())) {
        LogTrace(TRACE6) << __func__
                         << ": group not found by grp_id=" << grp_id;
        return;
    }
    int checked = 0;
    int refused = 0;
    int deleted = 0;
    std::map<int,bool> del_pools;
    if (!group.is_unaccomp()) {
        // это не багаж без сопровождения
        const std::list<CheckIn::TSimplePaxItem> paxList = CheckIn::TSimplePaxItem::getByGrpId(grp_id);
        LogTrace(TRACE6) << __func__
                         << ": grp_id=" << grp_id
                         << ": paxList.size()=" << paxList.size();
        for (const CheckIn::TSimplePaxItem& pax: paxList) {
            if (pax.bag_pool_num != ASTRA::NoExists
                && del_pools.find(pax.bag_pool_num) == del_pools.end())
            {
                del_pools[pax.bag_pool_num] = true;
            }

            if (pax.refuse == ASTRA::refuseAgentError) {
                deleted += 1;
                deletePaxData(PaxId_t(pax.paxId()));
                clearEventsBilingual(grp_id, PointId_t(group.point_dep), RegNo_t(pax.reg_no));
            } else {
                if (pax.bag_pool_num != ASTRA::NoExists) {
                    del_pools[pax.bag_pool_num] = false;
                }
                if (!pax.refuse.empty()) {
                    refused += 1;
                } else {
                    checked += 1;
                }
            }
        } // for

        if (deleted > 0 && (checked + refused) > 0) {
            // надо удалить багаж, который привязан к удалённым пассажирам
            int new_bag_pool_num = 1;
            for (auto delPool: del_pools) {
                int bag_pool_num = delPool.first;
                bool pool_deleted = delPool.second;
                if (pool_deleted) {
                    // пул удалён
                    const std::vector<BaggagePool> bagPool = loadBaggagePools(grp_id, bag_pool_num);
                    for (const BaggagePool& bag: bagPool) {
                        deleteBagTags(grp_id, bag.num);
                        deleteBag2(grp_id, bag.num);
                        deleteValueBag(grp_id, bag.value_bag_num);
                        deleteUnaccompBagInfo(grp_id, bag.value_bag_num);
                        // value_bag, paid_bag?
                    }
                } else {
                    // пул не удалён
                    if (bag_pool_num != new_bag_pool_num) {
                        updatePax_BagPoolNum(grp_id, bag_pool_num, new_bag_pool_num);
                        updateBag2_BagPoolNum(grp_id, bag_pool_num, new_bag_pool_num);
                    }
                    new_bag_pool_num += 1;
                }
            } // for del_pool

            resetBaggageNums(grp_id);
        } // deleted

        if (checked == 0 && refused > 0) {
            updatePaxGrp_BagRefuse(grp_id, 1);
        }
    } else {
        checked = 0;
        refused = 0;
    }

    if ((!group.is_unaccomp() && (checked + refused) == 0)
        || (group.is_unaccomp() && group.bag_refuse == ASTRA::refuseAgentError))
    {
        deleteGroupData(grp_id);
        clearEventsBilingual(grp_id, PointId_t(group.point_dep));
    }

    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", checked=" << checked
                     << ", refused=" << refused
                     << ", deleted=" << deleted;
}
