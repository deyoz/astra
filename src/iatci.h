#pragma once

#include <jxtlib/JxtInterface.h>

#include <list>


namespace iatci { class Result; }


class IactiInterface: public JxtInterface
{
public:
    IactiInterface()
        : JxtInterface("", "IactiInterface")
    {
        AddEvent("InitialRequest",  JXT_HANDLER(IactiInterface, InitialRequest));
        AddEvent("UpdateRequest",   JXT_HANDLER(IactiInterface, UpdateRequest));
        AddEvent("CancelRequest",   JXT_HANDLER(IactiInterface, CancelRequest));
        AddEvent("ReprintRequest",  JXT_HANDLER(IactiInterface, ReprintRequest));
        AddEvent("PasslistRequest", JXT_HANDLER(IactiInterface, PasslistRequest));
        AddEvent("SeatmapRequest",  JXT_HANDLER(IactiInterface, SeatmapRequest));
        AddEvent("kick",            JXT_HANDLER(IactiInterface, KickHandler));
    }

    // Initial Through Check-in Interchange
    void InitialRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    // Through Check-in Update Interchang
    void UpdateRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    // Through Check-in Cancel Interchange
    void CancelRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    // Boarding Pass Reprint Interchange
    void ReprintRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    // Passenger List Function Interchange
    void PasslistRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    // Seat Map Function Interchange
    void SeatmapRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    //
    // Kick
    void KickHandler(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    // Kick handlers
    void CheckinKickHandler(xmlNodePtr resNode, const std::list<iatci::Result>& lRes);
    void UpdateKickHandler(xmlNodePtr resNode, const std::list<iatci::Result>& lRes);
    void CancelKickHandler(xmlNodePtr resNode, const std::list<iatci::Result>& lRes);
    void PasslistKickHandler(xmlNodePtr resNode, const std::list<iatci::Result>& lRes);
    void SeatmapKickHandler(xmlNodePtr resNode, const std::list<iatci::Result>& lRes);
    void SeatmapForPassengerKickHandler(xmlNodePtr resNode, const std::list<iatci::Result>& lRes);

    // Timeout Kick handler
    void TimeoutKickHandler(xmlNodePtr resNode);
};
