#include "cache.h"

#define NICKNAME "DJEK"
#include "test.h"

void CacheInterface::LoadCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE2, "CacheInterface::LoadCache");
};

void CacheInterface::SaveCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE2, "CacheInterface::SaveCache");	
};

void CacheInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};

