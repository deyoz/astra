#ifndef _DOCS_H_
#define _DOCS_H_

#include <libxml/tree.h>
#include "JxtInterface.h"

void RunRpt(std::string name, xmlNodePtr reqNode, xmlNodePtr resNode);
void get_report_form(const std::string name, std::string &form);
void PaxListVars(int point_id, int pr_lat, xmlNodePtr variablesNode, double f);

class DocsInterface : public JxtInterface
{
public:
  DocsInterface() : JxtInterface("","docs")
  {
     Handler *evHandle;
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::SaveForm);
     AddEvent("save_form",evHandle);
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::LoadForm);
     AddEvent("load_form",evHandle);
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::SaveReport);
     AddEvent("save_report",evHandle);
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::RunReport);
     AddEvent("run_report",evHandle);
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::GetFltInfo);
     AddEvent("GetFltInfo",evHandle);
     evHandle=JxtHandler<DocsInterface>::CreateHandler(&DocsInterface::GetSegList);
     AddEvent("GetSegList",evHandle);
  };

  void SaveForm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void LoadForm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SaveReport(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RunReport(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetFltInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetSegList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

#endif
