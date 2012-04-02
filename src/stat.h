#ifndef _STAT_H_
#define _STAT_H_

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"

namespace STAT {
    xmlNodePtr set_variables(xmlNodePtr resNode, std::string lang = "");
}

class StatInterface : public JxtInterface
{
public:
  StatInterface() : JxtInterface("123","stat")
  {
     Handler *evHandle;
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::PaxListRun);
     AddEvent("PaxListRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::PaxSrcRun);
     AddEvent("PaxSrcRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::FltLogRun);
     AddEvent("FltLogRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::LogRun);
     AddEvent("LogRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::SystemLogRun);
     AddEvent("SystemLogRun",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::FltCBoxDropDown);
     AddEvent("FltCBoxDropDown",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::TestFltCBoxDropDown);
     AddEvent("TestFltCBoxDropDown",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::CommonCBoxDropDown);
     AddEvent("CommonCBoxDropDown",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::RunStat);
     AddEvent("run_stat",evHandle);
     evHandle=JxtHandler<StatInterface>::CreateHandler(&StatInterface::TestRunStat);
     AddEvent("TestRunStat",evHandle);
  };

  void FltCBoxDropDown(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void TestFltCBoxDropDown(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CommonCBoxDropDown(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void FltLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SystemLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PaxListRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PaxSrcRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RunStat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void TestRunStat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

#endif
