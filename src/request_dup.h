//---------------------------------------------------------------------------
#ifndef _REQUEST_DUP_H_
#define _REQUEST_DUP_H_

#include "jxtlib/JxtInterface.h"

bool SEND_REQUEST_DUP();
bool RECEIVE_REQUEST_DUP();

void throw_if_request_dup(const std::string &where);

int main_request_dup_tcl(int supervisorSocket, int argc, char *argv[]);

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

bool BuildMsgForTermRequestDup(const std::string &pult,
                               const std::string &opr,
                               const std::string &body,
                               std::string &msg);

bool BuildMsgForWebRequestDup(short int client_id,
                              const std::string &body,
                              std::string &msg);

#endif
