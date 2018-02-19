#ifndef LIBRA_H
#define LIBRA_H

#include <serverlib/oci8.h>
#include "jxtlib/JxtInterface.h"


class LibraInterface : public JxtInterface
{
private:
public:
  LibraInterface() : JxtInterface("","libra")
  {
     Handler *evHandle;
     evHandle=JxtHandler<LibraInterface>::CreateHandler(&LibraInterface::Exec);
     AddEvent("request",evHandle);
  }
  OciCpp::Oci8Session* instance() {
    static OciCpp::Oci8Session* sess = 0;
    if ( sess == 0 ) {
       sess = new OciCpp::Oci8Session( "LIBRA", "LIBRA", 0, "balance/balance@balance");
    }
    return sess;
  }
  void Exec(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {}
};


#endif // LIBRA_H
