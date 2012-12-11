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

int alter_db(int argc,char **argv);
bool alter_arx(void);
int alter_pax_doc(int argc,char **argv);
int alter_pax_doc2(int argc,char **argv);
int alter_arx_pax_doc(int argc,char **argv);
int alter_arx_pax_doc2(int argc,char **argv);
int alter_pax_doc3(int argc,char **argv);
int alter_pax_doco3(int argc,char **argv);
int alter_crs_pax_doc3(int argc,char **argv);
int alter_crs_pax_doco3(int argc,char **argv);
int alter_pax_doc4(int argc,char **argv);
int alter_arx_pax_doc3(int argc,char **argv);
int alter_arx_pax_doco3(int argc,char **argv);
int alter_arx_pax_doc4(int argc,char **argv);
int put_move_arx_ext(int argc,char **argv);
int alter_bag_pool_num(int argc,char **argv);
int check_trfer_tckin_set(int argc,char **argv);
int alter_trfer_tckin_set(int argc,char **argv);
int create_tlg(int argc,char **argv);

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
