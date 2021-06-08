
#include "astra_misc.h"
#include "PgOraConfig.h"

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

namespace gtimer {

bool checkPrDel(const int point_id)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT 1 FROM points "
       "WHERE pr_del = 0 "
         "AND point_id = :point_id",
        PgOra::getROSession("POINTS")
    );

    int flag;

    cur.stb()
       .defNull(flag, 0)
       .bind(":point_id", point_id)
       .exfet();

    if (DbCpp::ResultCode::NoDataFound == cur.err()) {
        return false;
    }

    return true;
}

bool isClientStage(const int point_id, const TStage stage, int& pr_permit)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT MAX(pr_permit) "
       "FROM ckin_client_stages "
       "LEFT OUTER JOIN trip_ckin_client "
           "ON (trip_ckin_client.point_id = :point_id "
           "AND ckin_client_stages.CLIENT_TYPE = trip_ckin_client.CLIENT_TYPE) "
       "WHERE ckin_client_stages.stage_id = :stage_id",
        PgOra::getROSession({"CKIN_CLIENT_STAGES", "TRIP_CKIN_CLIENT"})
    );

    cur.stb()
       .defNull(pr_permit, 0)
       .bind(":point_id", point_id)
       .bind(":stage_id", int(stage))
       .exfet();

    // результат соответствует названию функции?
    if (DbCpp::ResultCode::NoDataFound == cur.err()) {
        pr_permit = 0;
        return false;
    }
    // это не клиентский шаг.
    return true;
}

std::map<TStage, bool> getClientStagePermits(const int point_id)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT stage_id FROM graph_stages",
        PgOra::getROSession("GRAPH_STAGES")
    );

    int stage_id;

    cur.def(stage_id)
       .exec();

    std::map<TStage, bool> result;

    while (!cur.fen()) {
        int pr_permit = false;
        if (isClientStage(point_id, TStage(stage_id), pr_permit)) {
            result.insert({TStage(stage_id), pr_permit});
        }
    }

    return result;
}

std::set<std::pair<TStage, TStage_Type>> getStageStatuses()
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT stage_id, stage_type "
       "FROM stage_statuses",
        PgOra::getROSession("STAGE_STATUSES")
    );

    int stage_id;
    int stage_type;

    cur.def(stage_id)
       .def(stage_type)
       .exec();

    std::set<std::pair<TStage, TStage_Type>> result;

    while (!cur.fen()) {
        result.insert({TStage(stage_id), TStage_Type(stage_type)});
    }

    return result;
}

TStage getCurrStage(const int point_id, const TStage_Type stageType, const std::set<std::pair<TStage, TStage_Type>>& stageStatuses)
{
    std::map<TStage, bool> stagePermitMap = getClientStagePermits(point_id);

    DbCpp::CursCtl cur = make_db_curs(
       "SELECT target_stage, level "
       "FROM "
         "(SELECT target_stage,cond_stage, act "
          "FROM graph_rules, trip_stages "
          "WHERE trip_stages.stage_id = graph_rules.target_stage "
            "AND point_id = :point_id AND next = 1) "
       "START WITH cond_stage = 0 AND act IS NOT NULL "
       "CONNECT BY PRIOR target_stage = cond_stage AND act IS NOT NULL ",
        PgOra::getROSession({"GRAPH_RULES", "TRIP_STAGES"})
    );

    int target_stage;
    int level;
    int currLevel = 0;
    int currStageId = 0;

    cur.stb()
       .def(target_stage)
       .def(level)
       .bind(":point_id", point_id)
       .exec();

    while (!cur.fen()) {
        const auto iterator = stagePermitMap.find(TStage(target_stage));
        int pr_permit = stagePermitMap.end() != iterator
          ? iterator->second
          : 1;

        if (pr_permit) {
            if (currLevel > level) {
                break;
            }
            if (stageStatuses.count({TStage(target_stage), stageType})) {
                currStageId = target_stage;
                currLevel = level;
            }
        }
    }

    return TStage(currStageId);
}

void sync_trip_final_stages(const int point_id)
{
    std::set<std::pair<TStage, TStage_Type>> stageStatuses = getStageStatuses();

    DbCpp::CursCtl cur = make_db_curs(
       "SELECT stage_types.id, trip_final_stages.stage_id "
       "FROM stage_types "
       "LEFT JOIN trip_final_stages "
           "ON stage_types.id = trip_final_stages.stage_type "
          "AND trip_final_stages.point_id = :point_id",
        PgOra::getROSession({"STAGE_TYPES", "TRIP_FINAL_STAGES"})
    );

    int stage_type_id;
    int trip_final_stage_id;

    cur.stb()
       .def(stage_type_id)
       .defNull(trip_final_stage_id, ASTRA::NoExists)
       .bind(":point_id", point_id)
       .exec();

    while (!cur.fen()) {
        TStage stage = getCurrStage(point_id, TStage_Type(stage_type_id), stageStatuses);
        if (dbo::isNull(trip_final_stage_id)) {
            make_db_curs(
               "INSERT INTO trip_final_stages( point_id, stage_type, stage_id) "
                                     "VALUES (:point_id,:stage_type,:stage_id)",
                PgOra::getRWSession("TRIP_FINAL_STAGES"))
               .stb()
               .bind(":point_id", point_id)
               .bind(":stage_type", stage_type_id)
               .bind(":stage_id", int(stage))
               .exec();
        } else if (trip_final_stage_id != int(stage)) {
            make_db_curs(
               "UPDATE trip_final_stages SET stage_id = :stage_id "
               "WHERE point_id = :point_id "
                 "AND stage_type = :stage_type",
                PgOra::getRWSession("TRIP_FINAL_STAGES"))
               .stb()
               .bind(":stage_id", int(stage))
               .bind(":point_id", point_id)
               .bind(":stage_type", stage_type_id)
               .exec();
        }
    }

    return;
}

void setTripStageTime(const int point_id, const TStage stage, const Dates::DateTime_t act)
{
    DbCpp::CursCtl cur = make_db_curs(
        PgOra::supportsPg("TRIP_STAGES")
         ? "INSERT INTO trip_stages( point_id, stage_id, scd, est, act, pr_auto, pr_manual) "
                            "VALUES(:point_id,:stage_id,:act,NULL,:act, "    "0, "      "1) "
           "ON CONFLICT(point_id, stage_id) "
           "DO UPDATE trip_stages "
           "SET act = :act "
           "WHERE point_id = :point_id "
             "AND stage_id = :stage_id"
         : "UPDATE trip_stages "
           "SET act = :act "
           "WHERE point_id = :point_id "
             "AND stage_id = :stage_id; "
           "IF SQL%NOTFOUND THEN "
           "INSERT INTO trip_stages( point_id, stage_id, scd, est, act, pr_auto, pr_manual) "
                            "VALUES(:point_id,:stage_id,:act,NULL,:act, "    "0, "      "1);",
        PgOra::getROSession("TRIP_STAGES")
    );

    cur.stb()
       .bind(":point_id", point_id)
       .bind(":stage_id", int(stage))
       .bind(":act", act)
       .exec();

    sync_trip_final_stages(point_id);
}

bool execStage(const int point_id, const TStage stage, Dates::DateTime_t& act)
{
    bool result = checkPrDel(point_id);
    if (false == result) {
        return false;
    }

    Dates::DateTime_t utc = Dates::second_clock::universal_time();

    DbCpp::CursCtl cur = make_db_curs(
       "SELECT cond_stage, num, CASE cond_stage WHEN 0 THEN :utc WHEN 99 THEN NULL ELSE act END act "
       "FROM trip_stages, "
       "(SELECT cond_stage, num "
          "FROM trip_stages, graph_rules "
         "WHERE trip_stages.point_id = :point_id AND "
               "trip_stages.stage_id = :stage_id AND "
               "graph_rules.target_stage = :stage_id AND "
               "graph_rules.next = 1) "
         "WHERE trip_stages.point_id(+) = :point_id AND "
               "trip_stages.stage_id(+) = cond_stage "
       "ORDER BY num, act DESC",
        PgOra::getROSession({"TRIP_STAGES","GRAPH_RULES"})
    );

    int cond_stage;
    int num;
    Dates::DateTime_t condAct;

    cur.stb()
       .def(cond_stage)
       .def(num)
       .defNull(condAct, Dates::not_a_date_time)
       .bind(":point_id", point_id)
       .bind(":stage_id", int(stage))
       .bind(":utc", utc)
       .exec();

    int old_num = ASTRA::NoExists;

    while (!cur.fen()) {
        if (old_num != num) {
            if (result && dbo::isNotNull(old_num)) {
                break;
            }
            result = true;
            old_num = num;
        }
        int pr_permit;
        if (result && dbo::isNull(condAct) // нет факта выполнения, проверка на web-шаг
         && (isClientStage(point_id, TStage(cond_stage), pr_permit) || pr_permit)) {
            result = false;
        }
    }

    if (false == result) {
        return false;
    }

    // нужно ли тут опять брать текущее время?
    // utc = Dates::second_clock::universal_time();
    Dates::time_duration td = utc.time_of_day();
    act = Dates::DateTime_t(utc.date(), Dates::hours(td.hours()) + Dates::minutes(td.minutes()));

    setTripStageTime(point_id, stage, act);

    return true;
}

} // namespace gtimer
