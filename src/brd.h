#ifndef _BRD_H_
#define _BRD_H_

#include <libxml/tree.h>
#include "JxtInterface.h"

class BrdInterface : public JxtInterface
{
private:
    static void SetCounters(int point_id, xmlNodePtr dataNode);
    static bool PaxUpdate(int pax_id, int &tid, bool mark);
public:
  BrdInterface() : JxtInterface("123","brd")
  {
     Handler *evHandle;
     //denis
     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::BrdList);
     AddEvent("brd_list",evHandle);
     AddEvent("search_reg",evHandle);
     AddEvent("search_bar",evHandle);
     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::PaxUpd);
     AddEvent("brd_paxupd",evHandle);
     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::Deplane);
     AddEvent("brd_deplane",evHandle);
     //vlad
     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::PaxList);
     AddEvent("PaxList",evHandle);
     AddEvent("PaxByPaxId",evHandle);
     AddEvent("PaxByRegNo",evHandle);

     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::DeplaneAll);
     AddEvent("DeplaneAll",evHandle);
  };

  void Deplane(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PaxUpd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void BrdList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  void PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeplaneAll(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  static void readTripCounters( int point_id, xmlNodePtr dataNode );
  static void GetPax(xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
