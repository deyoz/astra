#ifndef ZAMAR_DSM_H
#define ZAMAR_DSM_H

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"
#include "date_time.h"
#include "astra_consts.h"
#include "passenger.h"
#include "print.h"
#include "astra_misc.h"

using BASIC::date_time::TDateTime;

enum class ZamarType { DSM, SBDO };

//-----------------------------------------------------------------------------------
// DSM

class ZamarDSMInterface: public JxtInterface
{
    public:
        ZamarDSMInterface(): JxtInterface("456", "ZamarDSM")
        {
            AddEvent("PassengerSearch",    JXT_HANDLER(ZamarDSMInterface, PassengerSearch));
        }

        void PassengerSearch(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

//-----------------------------------------------------------------------------------

class PassengerSearchResult
{
  PrintInterface::BPPax bppax;
  int point_id;
  TTripInfo trip_info;
  CheckIn::TSimplePaxGrpItem grp_item;
  CheckIn::TSimplePaxItem pax_item;
  CheckIn::TPaxDocItem doc;
  CheckIn::TPaxDocoItem doco;
  bool doc_exists;
  bool doco_exists;
  TMktFlight mkt_flt;
  
  // sessionId
  std::string sessionId;
  // flightStatus
  TStage flightCheckinStage = sNoActive;
  // pnr
  TPnrAddrs pnrs;
  // baggageTags
  std::multimap<TBagTagNumber, CheckIn::TBagItem> bagTagsExtended;
  
public:
  PassengerSearchResult& fromXML(xmlNodePtr reqNode, ZamarType type);
  const PassengerSearchResult& toXML(xmlNodePtr resNode, ZamarType type) const;
  static void errorXML(xmlNodePtr resNode, const std::string& cmd, const std::string& err);
};

//-----------------------------------------------------------------------------------
// SBDO

class ZamarSBDOInterface: public JxtInterface
{
    public:
        ZamarSBDOInterface(): JxtInterface("457", "ZamarSBDO") // ��筨�� ��ࠬ���� ���������
        {
            AddEvent("PassengerSearchSBDO",    JXT_HANDLER(ZamarSBDOInterface, PassengerSearchSBDO));
            AddEvent("PassengerBaggageTagAdd",    JXT_HANDLER(ZamarSBDOInterface, PassengerBaggageTagAdd));
            AddEvent("PassengerBaggageTagConfirm",    JXT_HANDLER(ZamarSBDOInterface, PassengerBaggageTagConfirm));
            AddEvent("PassengerBaggageTagRevoke",    JXT_HANDLER(ZamarSBDOInterface, PassengerBaggageTagRevoke));
        }

        void PassengerSearchSBDO(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void PassengerBaggageTagAdd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void PassengerBaggageTagConfirm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void PassengerBaggageTagRevoke(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif // ZAMAR_DSM_H
