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

class ZamarDSMInterface: public JxtInterface
{
    public:
        ZamarDSMInterface(): JxtInterface("456", "ZamarDSM")
        {
            AddEvent("PassengerSearch",    JXT_HANDLER(ZamarDSMInterface, PassengerSearch));
        }

        void PassengerSearch(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

class PassengerSearchResult
{
  PrintInterface::BPPax bppax;
  TTripInfo trip_info;
  CheckIn::TSimplePaxGrpItem grp_item;
  CheckIn::TSimplePaxItem pax_item;
  CheckIn::TPaxDocItem doc;
  TMktFlight mkt_flt;
  
  // sessionId
  std::string sessionId;
  // flightStatus
  TStage flightCheckinStage = sNoActive;
  // pnr
  TPnrAddrs pnr;
  // baggageTags
  std::multiset<TBagTagNumber> baggageTags;
  
public:
  PassengerSearchResult& fromXML(xmlNodePtr reqNode);
  const PassengerSearchResult& toXML(xmlNodePtr resNode) const;
  static void errorXML(xmlNodePtr resNode, const std::string& msg);
};

#endif // ZAMAR_DSM_H
