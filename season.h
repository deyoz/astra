#ifndef _SEASON_H_
#define _SEASON_H_

#include <libxml/tree.h>
#include "JxtInterface.h"		
#include "basic.h"		

class SeasonInterface : public JxtInterface
{
public:
  SeasonInterface() : JxtInterface("123","season")
  {
     Handler *evHandle;
     evHandle=JxtHandler<SeasonInterface>::CreateHandler(&SeasonInterface::Read);
     AddEvent("season_read",evHandle);
  };	
  
  void Read(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);  
};

#endif
