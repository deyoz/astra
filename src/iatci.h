#pragma once

#include <jxtlib/JxtInterface.h>

#include <list>


namespace iatci { class Result; }


class IatciInterface: public JxtInterface
{
public:
    enum RequestType { Cki, Cku, Ckx, Bpr, Plf, Smp, };
    enum KickAction { ActSavePax, ActReseatPax, ActLoadPax, ActReprint };

    IatciInterface()
        : JxtInterface("", "IactiInterface")
    {
        AddEvent("kick", JXT_HANDLER(IatciInterface, KickHandler));
    }

    static RequestType ClassifyCheckInRequest(xmlNodePtr reqNode);

    static bool DispatchCheckInRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode);
    static bool WillBeSentCheckInRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode);

    static bool MayNeedSendIatci(xmlNodePtr reqNode);

    // Initial Through Check-in Interchange
    static bool InitialRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode);

    // Through Check-in Update Interchange
    static bool UpdateRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode);
    static void UpdateSeatRequest(xmlNodePtr reqNode, int magicId);

    // Through Check-in Cancel Interchange
    static bool CancelRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode);

    // Boarding Pass Reprint Interchange
    static void ReprintRequest(xmlNodePtr reqNode, int grpId);

    // Passenger List Function Interchange
    static void PasslistRequest(xmlNodePtr reqNode, int grpId);

    // Seat Map Function Interchange
    static void SeatmapRequest(xmlNodePtr reqNode, int magicId);
    static void SeatmapRequest(xmlNodePtr reqNode);

protected:
    //
    // Kick
    void KickHandler(XMLRequestCtxt* ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

    void KickHandler_onSuccess(int ctxtId, xmlNodePtr initialReqNode,
                               xmlNodePtr resNode,
                               const std::list<iatci::Result>& lRes);
    void KickHandler_onFailure(int ctxtId, xmlNodePtr initialReqNode,
                               xmlNodePtr resNode,
                               const std::list<iatci::Result>& lRes,
                               const std::string& errCode);
    void KickHandler_onTimeout(int ctxtId, xmlNodePtr initialReqNode, xmlNodePtr resNode);

    // Kick handlers
    void CheckinKickHandler(int ctxtId, xmlNodePtr initialReqNode,
                            xmlNodePtr resNode, const std::list<iatci::Result>& lRes);
    void UpdateKickHandler(int ctxtId, xmlNodePtr initialReqNode,
                           xmlNodePtr resNode, const std::list<iatci::Result>& lRes);
    void CancelKickHandler(int ctxtId, xmlNodePtr initialReqNode,
                           xmlNodePtr resNode, const std::list<iatci::Result>& lRes);
    void ReprintKickHandler(int ctxtId, xmlNodePtr initialReqNode,
                            xmlNodePtr resNode, const std::list<iatci::Result>& lRes);
    void PasslistKickHandler(int ctxtId, xmlNodePtr initialReqNode,
                             xmlNodePtr resNode, const std::list<iatci::Result>& lRes);
    void SeatmapKickHandler(int ctxtId, xmlNodePtr initialReqNode,
                            xmlNodePtr resNode, const std::list<iatci::Result>& lRes);
    void SeatmapForPassengerKickHandler(int ctxtId, xmlNodePtr initialReqNode,
                                        xmlNodePtr resNode, const std::list<iatci::Result>& lRes);

private:
    int GetReqCtxtId(xmlNodePtr kickReqNode) const;
    void DoKickAction(int ctxtId, xmlNodePtr reqNode, xmlNodePtr resNode,
                      const std::list<iatci::Result>& lRes,
                      RequestType reqType,
                      KickAction act);

    void RollbackChangeOfStatus(int ctxtId);
};
