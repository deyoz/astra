//---------------------------------------------------------------------------
#ifndef _EMPTY_PROC_H_
#define _EMPTY_PROC_H_

#ifndef __WIN32__
#include <tcl.h>
int main_empty_proc_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
#endif
int get_events_stat(int argc,char **argv);
int get_events_stat2(int argc,char **argv);
int get_sirena_rozysk_stat(int argc,char **argv);
int create_tlg(int argc,char **argv);
int get_basel_aero_stat(int argc,char **argv);

class TestInterface : public JxtInterface
{
public:
  TestInterface() : JxtInterface("","test")
  {
     Handler *evHandle;
     evHandle=JxtHandler<TestInterface>::CreateHandler(&TestInterface::TestRequestDup);
     AddEvent("test_request_dup",evHandle);
  };

  void TestRequestDup(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};
};

#endif
