#pragma once

#include "iatci_types.h"

#include <list>


namespace iatci {


class IfmFlights
{
    FlightDetails                  m_rcvFlight;
    boost::optional<FlightDetails> m_dlvFlight;
    
public:
    IfmFlights(const FlightDetails& rcvFlight,
               const boost::optional<FlightDetails>& dlvFlight);

    const FlightDetails&                  rcvFlight() const { return m_rcvFlight; }
    const boost::optional<FlightDetails>& dlvFlight() const { return m_dlvFlight; }
};

//---------------------------------------------------------------------------------------

class IfmAction
{
public:
    enum IfmAction_t {
        Del,
        Chg
    };

private:
    IfmAction_t m_act;

public:
    IfmAction(IfmAction_t act);

    IfmAction_t act() const { return m_act; }
    std::string actAsString() const;
};

//---------------------------------------------------------------------------------------

class IfmPaxes
{
    std::list<PaxDetails> m_paxes;

public:
    IfmPaxes(const PaxDetails& pax);
    IfmPaxes() {}
    void addPax(const PaxDetails& pax);
    PaxDetails firstPax() const;

    const std::list<PaxDetails>& paxes() const { return m_paxes; }
};

//---------------------------------------------------------------------------------------

class IfmMessage
{
    IfmFlights m_flights;
    IfmAction  m_act;
    IfmPaxes   m_paxes;

public:
    IfmMessage(const IfmFlights& flights,
               const IfmAction& act,
               const IfmPaxes& paxes);

    const IfmFlights& flights() const { return m_flights; }
    const IfmAction&  action()  const { return m_act;     }
    const IfmPaxes&   paxes()   const { return m_paxes;   }

    void send();

protected:
    std::string msg() const;
};


}//namespace iatci
