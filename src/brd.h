#ifndef _BRD_H_
#define _BRD_H_

#include <libxml/tree.h>
#include "JxtInterface.h"

class BrdInterface : public JxtInterface
{
private:
    static bool PaxUpdate(int point_id, int pax_id, int &tid, bool mark, bool pr_exam_with_brd);
public:
  BrdInterface() : JxtInterface("123","brd")
  {
     Handler *evHandle;
     //vlad
     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::PaxList);
     AddEvent("PaxList",evHandle);
     AddEvent("PaxByPaxId",evHandle);
     AddEvent("PaxByRegNo",evHandle);

     evHandle=JxtHandler<BrdInterface>::CreateHandler(&BrdInterface::DeplaneAll);
     AddEvent("DeplaneAll",evHandle);
  };

  void PaxList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DeplaneAll(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  static void readTripData( int point_id, xmlNodePtr dataNode );
  static void readTripCounters( int point_id, xmlNodePtr dataNode );
  static void GetPax(xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
