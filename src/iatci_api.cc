#include "iatci_api.h"
#include "iatci_serialization.h"

#include <serverlib/dates.h>
#include <serverlib/cursctl.h>
#include <serverlib/int_parameters_oci.h>

#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci
{

static const int MaxSerializedDataLen = 1024;

static std::string serialize(const std::list<iatci::Result>& lRes)
{
    std::ostringstream os;
    {
        boost::archive::text_oarchive oa(os);
        oa << lRes;
    }
    return os.str();
}

static void deserialize(std::list<iatci::Result>& lRes, const std::string& data)
{
    std::stringstream is;
    is << data;
    {
        boost::archive::text_iarchive ia(is);
        ia >> lRes;
    }
}

Result checkinPax(const CkiParams& ckiParams)
{
    // TODO вызов функций Астры
    FlightDetails flight4Checkin(ckiParams.flight().airline(),
                                 ckiParams.flight().flightNum(),
                                 ckiParams.flight().depPoint(),
                                 ckiParams.flight().arrPoint(),
                                 Dates::rrmmdd("150217"),
                                 Dates::rrmmdd("150217"),
                                 Dates::hh24mi("1000"),
                                 Dates::hh24mi("1330"),
                                 Dates::hh24mi("0930"));

    PaxDetails pax4Checkin(ckiParams.pax().surname(),
                           ckiParams.pax().name(),
                           ckiParams.pax().type(),
                           ckiParams.pax().qryRef(),
                           flight4Checkin.toShortKeyString());

    FlightSeatDetails seat4Checkin("03A",
                                   "C",
                                   "0030",
                                   SeatDetails::NonSmoking);

    boost::optional<CascadeHostDetails> cascadeDetails;
    if(findCascadeFlight(ckiParams))
        cascadeDetails = CascadeHostDetails(flight4Checkin.airline());

    return Result::makeCheckinResult(Result::Ok,
                                     flight4Checkin,
                                     pax4Checkin,
                                     seat4Checkin,
                                     cascadeDetails);
}

Result cancelCheckin(const CkxParams& ckxParams)
{
    // TODO вызов функций Астры
    return Result::makeCancelResult(Result::Ok,
                                    ckxParams.flight());
}

boost::optional<FlightDetails> findCascadeFlight(const CkiParams& ckiParams)
{
    // TODO вызов функций Астры
    if(ckiParams.flight().airline() == "SU"
       && ckiParams.flight().flightNum() == Ticketing::FlightNum_t(200))
    {
        return FlightDetails("UT",
                             Ticketing::FlightNum_t(300),
                             "AER",
                             "SVO",
                             Dates::rrmmdd("150222"),
                             Dates::rrmmdd("150222"));
    }

    return boost::none;
}

void saveDeferredCkiData(tlgnum_t msgId, const std::list<Result>& lRes)
{
    std::string serialized = serialize(lRes);

    LogTrace(TRACE3) << __FUNCTION__
                     << " by msgId: " << msgId
                     << " data:\n" << serialized
                     << " [size=" << serialized.size() << "]";

    OciCpp::CursCtl cur = make_curs(
"insert into DEFERRED_CKI_DATA(MSG_ID, DATA) "
"values (:msg_id, :data)");
    cur.bind(":msg_id", msgId.num)
       .bind(":data", serialized)
       .exec();
}

std::list<Result> loadDeferredCkiData(tlgnum_t msgId)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by msgId: " << msgId;

    char data[MaxSerializedDataLen + 1] = {};

    OciCpp::CursCtl cur = make_curs(
"begin\n"
":data:=NULL;\n"
"delete from DEFERRED_CKI_DATA where MSG_ID=:msg_id "
"returning DATA into :data; \n"
"end;");
    cur.bind(":msg_id", msgId.num)
       .bindOutNull(":data", data, "")
       .exec();

    std::string serialized(data);

    if(serialized.empty()) {
        tst();
        return std::list<Result>();
    }

    std::list<Result> lRes;
    deserialize(lRes, serialized);
    return lRes;
}

void saveCkiData(edilib::EdiSessionId_t sessId, const std::list<Result>& lRes)
{
    std::string serialized = serialize(lRes);

    LogTrace(TRACE3) << __FUNCTION__
                     << " by sessId: " << sessId
                     << " data:\n" << serialized
                     << " [size=" << serialized.size() << "]";

    OciCpp::CursCtl cur = make_curs(
"insert into CKI_DATA(EDISESSION_ID, DATA) "
"values (:sessid, :data)");
    cur.bind(":sessid", sessId)
       .bind(":data", serialized)
       .exec();
}

std::list<Result> loadCkiData(edilib::EdiSessionId_t sessId)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by sessId: " << sessId;

    char data[MaxSerializedDataLen + 1] = {};

    OciCpp::CursCtl cur = make_curs(
"begin\n"
":data:=NULL;\n"
"delete from CKI_DATA where EDISESSION_ID=:sessid "
"returning DATA into :data; \n"
"end;");

    cur.bind(":sessid", sessId)
       .bindOutNull(":data", data, "")
       .exec();

    std::string serialized(data);
    if(serialized.empty()) {
        tst();
        return std::list<Result>();
    }

    std::list<Result> lRes;
    deserialize(lRes, serialized);
    return lRes;
}

}//namespace iatci
