#include "astra_callbacks.h"
#include "cache.h"

#define NICKNAME "VLAD"
#include "test.h"

#include "jxtlib.h"

using namespace jxtlib;

void AstraCallbacks::InitInterfaces()
{
  ProgTrace(TRACE3, "AstraCallbacks::InitInterfaces");
  new CacheInterface();  
};	

