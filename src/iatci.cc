#include "iatci.h"
#include "iatci_api.h"
#include "edi_utils.h"
#include "tlg/IatciCkiRequest.h"
#include "tlg/IatciCkuRequest.h"
#include "tlg/IatciCkxRequest.h"
#include "tlg/remote_results.h"
#include "tlg/remote_system_context.h"

#include <serverlib/dates.h>

#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


static iatci::CkiParams getDebugCkiParams()
{
    iatci::OriginatorDetails origin("UT", "SVO");

    iatci::FlightDetails prevFlight("UT",
                                    Ticketing::FlightNum_t(100),
                                    "SVO",
                                    "LED",
                                    Dates::rrmmdd("150220"),
                                    Dates::rrmmdd("150220"),
                                    Dates::hh24mi("0530"),
                                    Dates::hh24mi("1140"));

    iatci::FlightDetails flight("SU",
                                Ticketing::FlightNum_t(200),
                                "LED",
                                "AER",
                                Dates::rrmmdd("150221"),
                                Dates::rrmmdd("150221"));

    iatci::PaxDetails pax("PETROV",
                          "ALEX",
                          iatci::PaxDetails::Male,
                          "UT100");
    iatci::ReservationDetails reserv("Y");
    iatci::SeatDetails seat(iatci::SeatDetails::NonSmoking);
    iatci::BaggageDetails baggage(1, 20);
    boost::optional<iatci::CascadeHostDetails> cascadeDetails;

    iatci::CkiParams ckiParams(origin,
                               flight,
                               prevFlight,
                               pax,
                               reserv,
                               seat,
                               baggage,
                               cascadeDetails);

    return ckiParams;
}

static iatci::CkxParams getDebugCkxParams()
{
    iatci::OriginatorDetails origin("UT", "SVO");

    iatci::FlightDetails flight("SU",
                                Ticketing::FlightNum_t(200),
                                "LED",
                                "AER",
                                Dates::rrmmdd("150221"),
                                Dates::rrmmdd("150221"));

    iatci::PaxDetails pax("IVANOV",
                          "SERGEI",
                          iatci::PaxDetails::Male,
                          "UT100");

    iatci::CkxParams ckxParams(origin,
                               flight,
                               pax);

    return ckxParams;
}

//---------------------------------------------------------------------------------------

void IactiInterface::InitialRequest(XMLRequestCtxt* ctxt,
                                    xmlNodePtr reqNode,
                                    xmlNodePtr resNode)
{
    iatci::CkiParams ckiParams = getDebugCkiParams();
    edifact::KickInfo kickInfo = AstraEdifact::createKickInfo(ASTRA::NoExists, "IactiInterface");
    // send edifact DCQCKI request
    edifact::SendCkiRequest(ckiParams, kickInfo);
}

void IactiInterface::UpdateRequest(XMLRequestCtxt* ctxt,
                                   xmlNodePtr reqNode,
                                   xmlNodePtr resNode)
{
}

void IactiInterface::CancelRequest(XMLRequestCtxt* ctxt,
                                   xmlNodePtr reqNode,
                                   xmlNodePtr resNode)
{
    iatci::CkxParams ckxParams = getDebugCkxParams();
    edifact::KickInfo kickInfo = AstraEdifact::createKickInfo(ASTRA::NoExists, "IactiInterface");
    // send edifact DCQCKX request
    edifact::SendCkxRequest(ckxParams, kickInfo);
}

void IactiInterface::ReprintRequest(XMLRequestCtxt* ctxt,
                                    xmlNodePtr reqNode,
                                    xmlNodePtr resNode)
{

}

void IactiInterface::PasslistRequest(XMLRequestCtxt* ctxt,
                                     xmlNodePtr reqNode,
                                     xmlNodePtr resNode)
{

}

void IactiInterface::SeatmapRequest(XMLRequestCtxt* ctxt,
                                    xmlNodePtr reqNode,
                                    xmlNodePtr resNode)
{

}

void IactiInterface::CheckinKickHandler(xmlNodePtr resNode,
                                        const std::list<iatci::Result>& lRes)
{
    FuncIn(CheckinKickHandler);
    BOOST_FOREACH(const iatci::Result& res, lRes) {
        LogTrace(TRACE3) << "error: " << (res.errorDetails() ? res.errorDetails()->errText() : "None");
    }
    FuncOut(CheckinKickHandler);
}


void IactiInterface::CancelKickHandler(xmlNodePtr resNode,
                                       const std::list<iatci::Result>& lRes)
{
    tst();
}

void IactiInterface::TimeoutKickHandler(xmlNodePtr resNode)
{
    FuncIn(TimeoutKickHandler);
    FuncOut(TimeoutKickHandler);
}

void IactiInterface::KickHandler(XMLRequestCtxt* ctxt,
                                 xmlNodePtr reqNode,
                                 xmlNodePtr resNode)
{
    using namespace edifact;
    FuncIn(KickHandler);
    pRemoteResults res = RemoteResults::readSingle();
    if(res) {
        LogTrace(TRACE3) << *res;

        if(res->status() == RemoteStatus::Timeout)
        {
            TimeoutKickHandler(resNode);
        }
        else
        {
            std::list<iatci::Result> lRes = iatci::loadCkiData(res->ediSession());
            ASSERT(!lRes.empty());
            iatci::Result::Action_e action = lRes.front().action();
            switch(action)
            {
            case iatci::Result::Checkin:
                CheckinKickHandler(resNode, lRes);
                break;
            case iatci::Result::Cancel:
                CancelKickHandler(resNode, lRes);
                break;
            case iatci::Result::Update:
                tst();
            // TODO
            }
        }
    }
    FuncOut(KickHandler);
}
