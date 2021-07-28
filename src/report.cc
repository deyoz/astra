#include "report.h"
#include "PgOraConfig.h"

#include <serverlib/dbcpp_cursctl.h>

#define NICKNAME "ANTON"
#include <serverlib/slogger.h>


namespace ASTRA {

std::optional<AirportCode_t> get_last_trfer_airp(const GrpId_t& grpId)
{
    auto cur = make_db_curs(
"select AIRP_ARV "
"from TRANSFER "
"where GRP_ID = :grp_id and PR_FINAL <> 0",
                PgOra::getROSession("TRANSFER"));

    std::string airp_arv;
    cur
            .def(airp_arv)
            .bind(":grp_id", grpId.get())
            .exfet();
    if(cur.err() != DbCpp::ResultCode::NoDataFound) {
        return AirportCode_t(airp_arv);
    }

    return {};
}

//std::vector<Dates::DateTime_t> get_airline_period_first_dates(const AirlineCode_t& airl)
//{
//    auto cur = make_db_curs(
//"select distinct CHANGE_POINT as PERIOD_FIRST_DATE "
//"from PACTS, "
//"    (select FIRST_DATE as CHANGE_POINT "
//"     from PACTS "
//"     where AIRLINE = :airl "
//"    union "
//"     select LAST_DATE "
//"     from PACTS "
//"     where AIRLINE = :airl and LAST_DATE is not null) "
//"where AIRLINE = :airl and "
//"CHANGE_POINT >= FIRST_DATE and "
//"(LAST_DATE is null or LAST_DATE > CHANGE_POINT)",
//                PgOra::getROSession("PACTS"));

//    Dates::DateTime_t firstDate;
//    cur
//            .def(firstDate)
//            .bind(":airl", airl.get())
//            .exec();

//    std::vector<Dates::DateTime_t> firstDates;
//    while(!cur.fen()) {
//        firstDates.emplace_back(firstDate);
//    }

//    return firstDates;
//}

//std::optional<Dates::DateTime_t> get_airline_period_last_date(const AirlineCode_t& airl,
//                                                              const Dates::DateTime_t& periodFirstDate)
//{
//    auto cur = make_db_curs(
//"select min(PERIOD_LAST_DATE) from "
//"   (select FIRST_DATE as PERIOD_LAST_DATE from PACTS "
//"    where AIRLINE = :airl and FIRST_DATE > :first_date "
//"    union "
//"    select LAST_DATE from PACTS "
//"    where AIRLINE = :airl and LAST_DATE > :first_date and LAST_DATE is not null)",
//                PgOra::getROSession("PACTS"));

//    Dates::DateTime_t lastDate;
//    cur
//            .def(lastDate)
//            .bind(":airl",       airl.get())
//            .bind(":first_date", periodFirstDate)
//            .EXfet();

//    if(cur.err() != DbCpp::ResultCode::NoDataFound) {
//        return lastDate;
//    }

//    return {};
//}

//std::vector<Dates::time_period> get_airline_periods(const AirlineCode_t& airl,
//                                                    const Dates::DateTime_t& firstDate,
//                                                    const Dates::DateTime_t& lastDate)
//{
//    std::vector<Dates::DateTime_t> periodFirstDates = get_airline_period_first_dates(airl);

//    std::vector<Dates::time_period> periods;
//    for(const auto& periodFirstDate: periodFirstDates) {
//        auto periodLastDateOpt = get_airline_period_last_date(airl, periodFirstDate);
//        if((!periodLastDateOpt || (periodLastDateOpt && *periodLastDateOpt > firstDate)) && periodFirstDate < lastDate) {
//            if(periodLastDateOpt) {
//                periods.emplace_back(std::max(periodFirstDate, firstDate),
//                                     std::min(*periodLastDateOpt, lastDate));
//            } else {
//                periods.emplace_back(std::max(periodFirstDate, firstDate),
//                                     lastDate);
//            }
//        }
//    }

//    return periods;
//}

//std::vector<Dates::DateTime_t> get_airport_period_first_dates(const AirportCode_t& airp)
//{
//    auto cur = make_db_curs(
//"select distinct CHANGE_POINT as PERIOD_FIRST_DATE "
//"from PACTS, "
//"    (select FIRST_DATE as CHANGE_POINT "
//"     from PACTS "
//"     where AIRP = :airp "
//"    union "
//"     select LAST_DATE "
//"     from PACTS "
//"     where AIRP = :airp and LAST_DATE is not null) "
//"where AIRP = :airp and "
//"CHANGE_POINT >= FIRST_DATE and "
//"(LAST_DATE is null or LAST_DATE > CHANGE_POINT) and "
//" AIRLINE is null",
//                PgOra::getROSession("PACTS"));

//    Dates::DateTime_t firstDate;
//    cur
//            .def(firstDate)
//            .bind(":airp", airp.get())
//            .exec();

//    std::vector<Dates::DateTime_t> firstDates;
//    while(!cur.fen()) {
//        firstDates.emplace_back(firstDate);
//    }

//    return firstDates;
//}

//std::optional<Dates::DateTime_t> get_airport_period_last_date(const AirportCode_t& airp,
//                                                              const Dates::DateTime_t& periodFirstDate)
//{
//    auto cur = make_db_curs(
//"select min(PERIOD_LAST_DATE) from "
//"   (select FIRST_DATE as PERIOD_LAST_DATE from PACTS "
//"    where AIRP = :airp and FIRST_DATE > :first_date "
//"   union "
//"    select LAST_DATE from PACTS "
//"    where AIRP = :airp and LAST_DATE > :first_date and LAST_DATE is not null)",
//                PgOra::getROSession("PACTS"));

//    Dates::DateTime_t lastDate;
//    cur
//            .def(lastDate)
//            .bind(":airp",       airp.get())
//            .bind(":first_date", periodFirstDate)
//            .EXfet();

//    if(cur.err() != DbCpp::ResultCode::NoDataFound) {
//        return lastDate;
//    }

//    return {};
//}

//std::vector<Dates::time_period> get_airport_periods(const AirportCode_t& airp,
//                                                    const Dates::DateTime_t& firstDate,
//                                                    const Dates::DateTime_t& lastDate)
//{
//    std::vector<Dates::DateTime_t> periodFirstDates = get_airport_period_first_dates(airp);

//    std::vector<Dates::time_period> periods;
//    for(const auto& periodFirstDate: periodFirstDates) {
//        auto periodLastDateOpt = get_airport_period_last_date(airp, periodFirstDate);
//        if((!periodLastDateOpt || (periodLastDateOpt && *periodLastDateOpt > firstDate)) && periodFirstDate < lastDate) {
//            if(periodLastDateOpt) {
//                periods.emplace_back(std::max(periodFirstDate, firstDate),
//                                     std::min(*periodLastDateOpt, lastDate));
//            } else {
//                periods.emplace_back(std::max(periodFirstDate, firstDate),
//                                     lastDate);
//            }
//        }
//    }

//    return periods;
//}

}//namespace ASTRA
