#include "iatci_api.h"
#include "iatci_serialization.h"

#include "etick.h" // ChangeOfStatus
#include "astra_ticket.h" // Ticket
#include "astra_context.h"
#include "astra_api.h"
#include "astra_msg.h"
#include "edi_utils.h"

#include <serverlib/dates_io.h>
#include <serverlib/cursctl.h>
#include <serverlib/int_parameters_oci.h>
#include <serverlib/rip_oci.h>
#include <serverlib/xml_stuff.h>
#include <etick/exceptions.h>

#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci
{

using namespace AstraEdifact;
using namespace Ticketing;
using namespace Ticketing::TickExceptions;


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
    return astra_api::checkinIatciPax(ckiParams);
}

Result checkinPax(tlgnum_t postponeTlgNum)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << " by tlgnum " << postponeTlgNum;

    XMLDoc ediSessCtxt;
    getEdiSessionCtxt(postponeTlgNum, true, "iatci::checkinPax", ediSessCtxt);

    xml_decode_nodelist(ediSessCtxt.docPtr()->children);
    xmlNodePtr rootNode = NodeAsNode("/context", ediSessCtxt.docPtr());

    if(GetNode("@req_ctxt_id", rootNode))
    {
        tst();
        int reqCtxtId = NodeAsInteger("@req_ctxt_id", rootNode);
        LogTrace(TRACE3) << "req_ctxt_id=" << reqCtxtId;

        XMLDoc termReqCtxt;
        AstraEdifact::getTermRequestCtxt(reqCtxtId, true,
                                         "iatci::checkinPax", termReqCtxt);
        xmlNodePtr termReqNode = NodeAsNode("/query", termReqCtxt.docPtr())->children;
        ASSERT(termReqNode != NULL);

        XMLDoc ediResCtxt;
        AstraEdifact::getEdiResponseCtxt(reqCtxtId, true,
                                         "iatci::checkinPax", ediResCtxt, false/*throw*/);
        if(ediResCtxt.docPtr() == NULL) {
            // сюда попадем если случится таймаут
            throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "");
        }
        xmlNodePtr ediResNode = NodeAsNode("/context", ediResCtxt.docPtr());
        ASSERT(ediResNode != NULL);

        tst();
        return astra_api::checkinIatciPax(termReqNode, ediResNode);
    }

    return Result::makeFailResult(Result::Checkin,
                                  ErrorDetails(Ticketing::AstraErr::EDI_PROC_ERR));
}

Result cancelCheckin(const CkxParams& ckxParams)
{
   return astra_api::cancelCheckinIatciPax(ckxParams);
}

Result cancelCheckin(tlgnum_t postponeTlgNum)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << " by tlgnum " << postponeTlgNum;
    XMLDoc ediSessCtxt;
    getEdiSessionCtxt(postponeTlgNum, true, "iatci::checkinPax", ediSessCtxt);

    xml_decode_nodelist(ediSessCtxt.docPtr()->children);
    xmlNodePtr rootNode = NodeAsNode("/context", ediSessCtxt.docPtr());
    if(GetNode("@req_ctxt_id", rootNode))
    {
        tst();
        int reqCtxtId = NodeAsInteger("@req_ctxt_id", rootNode);
        LogTrace(TRACE3) << "req_ctxt_id=" << reqCtxtId;

        XMLDoc termReqCtxt;
        AstraEdifact::getTermRequestCtxt(reqCtxtId, true,
                                         "iatci::cancelCheckinPax", termReqCtxt);
        xmlNodePtr termReqNode = NodeAsNode("/query", termReqCtxt.docPtr())->children;
        ASSERT(termReqNode != NULL);

        XMLDoc ediResCtxt;
        AstraEdifact::getEdiResponseCtxt(reqCtxtId, true,
                                         "iatci::cancelCheckinPax", ediResCtxt, false/*throw*/);
        if(ediResCtxt.docPtr() == NULL) {
            // сюда попадем если случится таймаут
            throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "");
        }

        xmlNodePtr ediResNode = NodeAsNode("/context", ediResCtxt.docPtr());
        ASSERT(ediResNode != NULL);

        tst();
        return astra_api::cancelCheckinIatciPax(termReqNode, ediResNode);
    }

    TST();
    return Result::makeFailResult(Result::Cancel,
                                  ErrorDetails(Ticketing::AstraErr::EDI_PROC_ERR));
}

Result updateCheckin(const CkuParams& ckuParams)
{
    return astra_api::updateIatciPax(ckuParams);
}

Result reprintBoardingPass(const BprParams& bprParams)
{
    // TODO вызов функций Астры
    FlightDetails flight4Checkin(bprParams.flight().airline(),
                                 bprParams.flight().flightNum(),
                                 bprParams.flight().depPort(),
                                 bprParams.flight().arrPort(),
                                 Dates::rrmmdd("150217"),
                                 Dates::rrmmdd("150217"),
                                 Dates::hh24mi("1000"),
                                 Dates::hh24mi("1330"),
                                 Dates::hh24mi("0930"));

    PaxDetails pax4Checkin(bprParams.pax().surname(),
                           bprParams.pax().name(),
                           bprParams.pax().type(),
                           boost::none,
                           bprParams.pax().qryRef(),
                           flight4Checkin.toShortKeyString());

    FlightSeatDetails seat4Checkin("03A",
                                   "C",
                                   "0030",
                                   SeatDetails::NonSmoking);

    boost::optional<CascadeHostDetails> cascadeDetails;
    if(findCascadeFlight(bprParams.flight()))
        cascadeDetails = CascadeHostDetails(flight4Checkin.airline());

    return Result::makeReprintResult(Result::Ok,
                                     flight4Checkin,
                                     pax4Checkin,
                                     seat4Checkin,
                                     cascadeDetails);
}

Result fillPasslist(const PlfParams& plfParams)
{
    return astra_api::fillPaxList(plfParams);
}

Result fillSeatmap(const SmfParams& smfParams)
{
    return astra_api::fillSeatmap(smfParams);
}

boost::optional<FlightDetails> findCascadeFlight(const FlightDetails& flight)
{    
    // TODO пока не реализовано
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
