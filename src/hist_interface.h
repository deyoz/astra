#ifndef HIST_INTERFACE_H
#define HIST_INTERFACE_H

#include <libxml/tree.h>
#include "jxtlib/JxtInterface.h"

class HistoryInterface : public JxtInterface
{
public:
  HistoryInterface() : JxtInterface("","history")
  {
     Handler *evHandle;
     evHandle=JxtHandler<HistoryInterface>::CreateHandler(&HistoryInterface::LoadHistory);
     AddEvent("history",evHandle);
  }

  void LoadHistory(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}
};

#endif // HIST_INTERFACE_H
