#include "IatciIfm.h"
#include "TpbMessage.h"
#include "basetables.h"
#include "remote_system_context.h"

#include <serverlib/dates.h>

#include <sstream>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace iatci {

//---------------------------------------------------------------------------------------

IfmFlights::IfmFlights(const FlightDetails& rcvFlight,
                       const boost::optional<FlightDetails>& dlvFlight)
    : m_rcvFlight(rcvFlight),
      m_dlvFlight(dlvFlight)
{}

//---------------------------------------------------------------------------------------

IfmAction::IfmAction(IfmAction_t act)
    : m_act(act)
{}

std::string IfmAction::actAsString() const
{
    if(m_act == IfmAction::Del) {
        return "DEL";
    } else if(m_act == IfmAction::Chg) {
        return "CHG";
    } else {
        ASSERT(false);
        return "";
    }
}

//---------------------------------------------------------------------------------------

IfmPaxes::IfmPaxes(const PaxDetails& pax)
{
    addPax(pax);
}

void IfmPaxes::addPax(const PaxDetails& pax)
{
    m_paxes.push_back(pax);
}

PaxDetails IfmPaxes::firstPax() const
{
    ASSERT(!m_paxes.empty());
    return m_paxes.front();
}

//---------------------------------------------------------------------------------------

IfmMessage::IfmMessage(const IfmFlights& flights,
                       const IfmAction& act,
                       const IfmPaxes& paxes)
    : m_flights(flights),
      m_act(act),
      m_paxes(paxes)
{}

void IfmMessage::send()
{
    using Ticketing::RemoteSystemContext::DcsSystemContext;
    DcsSystemContext *dcsCont = DcsSystemContext::read(flights().rcvFlight().airline(),
                                                       flights().rcvFlight().flightNum());
    ASSERT(dcsCont);
    if(dcsCont->iatciSettings().ifmSupported()) {
        LogTrace(TRACE1) << "IFM supported. Sending...";                                                       
        airimp::TpbMessage tpbMsg(msg(), dcsCont);
        tpbMsg.send();
    } else {
        LogTrace(TRACE1) << "IFM doesn't supported. Skipping...'";
    }
}

std::string IfmMessage::msg() const
{
    std::stringstream m;

    // BEGIN OF MESSAGE
    m << "IFM\n";
    if(flights().dlvFlight()) {
        // DELIVERING FLIGHT
        m << BaseTables::Company(flights().dlvFlight()->airline())->code(ENGLISH);
        m << flights().dlvFlight()->flightNum();
        m << "/" << Dates::ddmon(flights().dlvFlight()->depDate(), ENGLISH);
        m << " " << BaseTables::Port(flights().dlvFlight()->depPort())->code(ENGLISH);
        m << "\n";
    }

    // RECEIVING FLIGHT
    m << "-" << BaseTables::Company(flights().rcvFlight().airline())->code(ENGLISH);
    m << flights().rcvFlight().flightNum();
    m << "/" << Dates::ddmon(flights().rcvFlight().depDate(), ENGLISH);
    m << " " << BaseTables::Port(flights().rcvFlight().depPort())->code(ENGLISH);
    m << BaseTables::Port(flights().rcvFlight().arrPort())->code(ENGLISH);
    m << "\n";

    // ACTION INDICATOR
    m << action().actAsString();
    m << "\n";

    // PASSENGER INFORMATION
    m << "1" << paxes().firstPax().surname();
    m << "/" << paxes().firstPax().name();
    m << "\n";

    // END OF MESSAGE
    m << "ENDIFM\n"; 

    return m.str();
}

}//namespace iatci
