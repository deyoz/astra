#ifndef _BRD_H_
#define _BRD_H_

#include <libxml/tree.h>
#include "JxtInterface.h"

class BrdInterface : public JxtInterface
{
private:
    void SetCounters(xmlNodePtr dataNode);
    int PaxUpdate(int pax_id, int &tid, int pr_brd);
public:
  BrdInterface() : JxtInterface("123","brd")
  {
     Handler *evHandle;
     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::BrdList);
     AddEvent("brd_list",evHandle);
     AddEvent("search_reg",evHandle);
     AddEvent("search_bar",evHandle);
     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::PaxUpd);
     AddEvent("brd_paxupd",evHandle);
     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::Deplane);
     AddEvent("brd_deplane",evHandle);
  };

  void Deplane(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PaxUpd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void BrdList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static void Trip(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
