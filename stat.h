#ifndef _STAT_H_
#define _STAT_H_

#include <libxml/tree.h>
#include "JxtInterface.h"		

class StatInterface : public JxtInterface
{
public:
  StatInterface() : JxtInterface("123","stat")
  {
     Handler *evHandle;
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::DepStatRun);
     AddEvent("DepStatRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::BagTagStatRun);
     AddEvent("BagTagStatRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::PaxListRun);
     AddEvent("PaxListRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::PaxLog);
     AddEvent("LogRun",evHandle);
     AddEvent("FltLogRun",evHandle);
     AddEvent("SystemLogRun",evHandle);
  };	
  
  void PaxLog(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PaxListRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void BagTagStatRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void DepStatRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);  
};

#endif
