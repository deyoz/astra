#ifndef _TRIPINFO_H_
#define _TRIPINFO_H_

#include <libxml/tree.h>
#include <string>
#include "JxtInterface.h"

struct TTrferItem {
  std::string last_trfer;
  int hall_id;
  std::string hall_name;
  int seats,adult,child,baby,foreigner;
  int umnr,vip,rkWeight,bagAmount,bagWeight,excess;
};


struct TCounterItem {
  std::string cl;
  std::string target;
  std::vector<TTrferItem> trfer;
  int cfg,resa,tranzit;
};

class TripInfoInterface : public JxtInterface
{
private:
  void readTripHeader( int point_id, xmlNodePtr dataNode );
  void readTripCounters( int point_id, xmlNodePtr dataNode );
public:
  TripInfoInterface() : JxtInterface("","tripinfo")
  {
     Handler *evHandle;
     evHandle=JxtHandler<TripInfoInterface>::CreateHandler(&TripInfoInterface::ReadTrips);
     AddEvent("ReadTrips",evHandle);     
     evHandle=JxtHandler<TripInfoInterface>::CreateHandler(&TripInfoInterface::ReadTripInfo);
     AddEvent("ReadTripInfo",evHandle);               
  };

  void ReadTrips(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ReadTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

 
#endif /*_TRIPINFO_H_*/

