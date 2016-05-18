#pragma once

#include <jxtlib/JxtInterface.h>

#include <list>


namespace iatci { class Result; }


class IatciInterface: public JxtInterface
{
    enum RequestType { Cki, Cku, Ckx, Bpr, Plf, Smp, };

public:
    IatciInterface()
        : JxtInterface("", "IactiInterface")
    {
        AddEvent("InitialRequest",  JXT_HANDLER(IatciInterface, InitialRequest));
        AddEvent("UpdateRequest",   JXT_HANDLER(IatciInterface, UpdateRequest));
        AddEvent("CancelRequest",   JXT_HANDLER(IatciInterface, CancelRequest));
        AddEvent("ReprintRequest",  JXT_HANDLER(IatciInterface, ReprintRequest));
        AddEvent("PasslistRequest", JXT_HANDLER(IatciInterface, PasslistRequest));
        AddEvent("SeatmapRequest",  JXT_HANDLER(IatciInterface, SeatmapRequest));
        AddEvent("kick",            JXT_HANDLER(IatciInterface, KickHandler));
    }

    /* returns: true - req was sent, false - otherwise*/
    static RequestType ClassifyCheckInRequest(xmlNodePtr reqNode);
    static bool DispatchCheckInRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode);
    static bool WillBeSentCheckInRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode);

    static bool MayNeedSendIatci(xmlNodePtr reqNode);

    // Initial Through Check-in Interchange
    void InitialRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    static bool InitialRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode);

    // Through Check-in Update Interchange
    void UpdateRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    static bool UpdateRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode);

    // Through Check-in Cancel Interchange
    void CancelRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    static bool CancelRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode);

    // Boarding Pass Reprint Interchange
    void ReprintRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    // Passenger List Function Interchange
    void PasslistRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    static void PasslistRequest(xmlNodePtr reqNode, int grpId);

    // Seat Map Function Interchange
    void SeatmapRequest(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    //
    // Kick
    void KickHandler(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    // Kick handlers
    void CheckinKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                            const std::list<iatci::Result>& lRes);
    void UpdateKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                           const std::list<iatci::Result>& lRes);
    void CancelKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                           const std::list<iatci::Result>& lRes);
    void ReprintKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                            const std::list<iatci::Result>& lRes);
    void PasslistKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                             const std::list<iatci::Result>& lRes);
    void SeatmapKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                            const std::list<iatci::Result>& lRes);
    void SeatmapForPassengerKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                                        const std::list<iatci::Result>& lRes);

    // Timeout Kick handler
    void TimeoutKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode);

protected:
    enum KickAction_e { ActSavePax, ActRollbackStatus, ActLoadPax };
    void DoKickAction(xmlNodePtr reqNode, xmlNodePtr resNode,
                      const std::list<iatci::Result>& lRes,
                      const std::string& resNodeName,
                      KickAction_e act);
};
