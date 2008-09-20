#ifndef _DEV_TUNING_H_
#define _DEV_TUNING_H_

#include <libxml/tree.h>
#include <string>
#include "JxtInterface.h"

class DevTuningInterface : public JxtInterface
{
public:
  DevTuningInterface() : JxtInterface("","DevTuning")
  {
     Handler *evHandle;
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::LoadOperTypes);
     AddEvent("LoadOperTypes",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::LoadBTList);
     AddEvent("LoadBTList",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::LoadBPList);
     AddEvent("LoadBPList",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::LoadPrnForm);
     AddEvent("LoadPrnForm",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::PrevNext);
     AddEvent("PrevNext",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::Save);
     AddEvent("Save",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::GetPrintData);
     AddEvent("GetPrintData",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::LoadPrnForms);
     AddEvent("LoadPrnForms",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::BTListCommit);
     AddEvent("BTListCommit",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::BPListCommit);
     AddEvent("BPListCommit",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::PrnFormsCommit);
     AddEvent("PrnFormsCommit",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::Cache);
     AddEvent("cache",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::ApplyCache);
     AddEvent("apply_cache",evHandle);
  };

  void PrnFormsCommit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void BTListCommit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void BPListCommit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetPrintData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadPrnForms(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Save(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PrevNext(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadPrnForm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadBTList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadBPList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadOperTypes(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Cache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ApplyCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};


#endif /*_DEV_TUNING_H_*/


