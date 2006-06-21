#ifndef _CACHE_H_
#define _CACHE_H_

#include "JxtInterface.h"		

class CacheInterface : public JxtInterface
{
public:
  CacheInterface() : JxtInterface("","cache_iface")
  {
     Handler *evHandle;
     evHandle=JxtHandler<CacheInterface>::CreateHandler(&CacheInterface::LoadCache);
     AddEvent("cache",evHandle);
     evHandle=JxtHandler<CacheInterface>::CreateHandler(&CacheInterface::SaveCache); 	
     AddEvent("cache_apply",evHandle);
  };	
  
  void LoadCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SaveCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);  
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);  
};

#endif /*_CACHE_H_*/

