#ifndef _DEV_TUNING_H_
#define _DEV_TUNING_H_

#include <libxml/tree.h>
#include <string>
#include "jxtlib/JxtInterface.h"

class DevTuningInterface : public JxtInterface
{
public:
  DevTuningInterface() : JxtInterface("","DevTuning")
  {
     Handler *evHandle;
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::Cache);
     AddEvent("cache",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::ApplyCache);
     AddEvent("cache_apply",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::Load);
     AddEvent("Load",evHandle);
     evHandle=JxtHandler<DevTuningInterface>::CreateHandler(&DevTuningInterface::UpdateCopy);
     AddEvent("Copy",evHandle);
     AddEvent("Update",evHandle);
  };
  void UpdateCopy(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Load(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void Cache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void ApplyCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};


#endif /*_DEV_TUNING_H_*/


