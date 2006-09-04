#ifndef _CHECKIN_H_
#define _CHECKIN_H_

#include <string>
#include <map>
#include <libxml/tree.h>
#include "JxtInterface.h"		
#include "astra_ticket.h"

class CheckInInterface : public JxtInterface
{
public:
  CheckInInterface() : JxtInterface("","CheckIn")
  {
     Handler *evHandle;
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::SavePax);
     AddEvent("SavePax",evHandle);          
  };	
  
  void SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);    
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};  
  
  void SavePaxRem(xmlNodePtr paxNode);
  std::string SavePaxNorms(xmlNodePtr paxNode, std::map<int,std::string> &norms);
  void SaveTransfer(xmlNodePtr grpNode);
};


#endif
