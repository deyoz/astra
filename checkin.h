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
     evHandle=JxtHandler<CheckInInterface>::CreateHandler(&CheckInInterface::LoadPax);
     AddEvent("LoadPax",evHandle);     
  };	
  
  void SavePax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);    
  void LoadPax(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);    
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};  
  
  void SavePaxRem(xmlNodePtr paxNode);
  std::string SavePaxNorms(xmlNodePtr paxNode, std::map<int,std::string> &norms);
  void SaveTransfer(xmlNodePtr grpNode);
  void SaveBag(xmlNodePtr grpNode);
  void SavePaidBag(xmlNodePtr grpNode);
  
  void SaveBagToLog(xmlNodePtr grpNode);
  void SaveTagPacks(xmlNodePtr node);     
  
  void LoadPaxRem(xmlNodePtr paxNode);
  void LoadPaxNorms(xmlNodePtr paxNode);
  void LoadTransfer(xmlNodePtr grpNode);
  void LoadBag(xmlNodePtr grpNode);   
  void LoadPaidBag(xmlNodePtr grpNode);  
};


#endif
