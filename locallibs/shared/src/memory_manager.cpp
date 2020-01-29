#include <stdlib.h>
#include "memory_manager.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

#define STDLOG_PARAMS nick, file, line

using namespace std;

TMemoryManager::TMemoryManager(STDLOG_SIGNATURE)
{
  TMemoryManager::file=file;
  TMemoryManager::line=line;
  TMemoryManager::trace_memory=true;
};

TMemoryManager::~TMemoryManager()
{
  for(map<void*, pair<string, int> >::const_iterator i=memList.begin(); i!=memList.end(); i++)
  {
    ProgError( STDLOG, "Pointer is not released. Pointer created here: %s:%d. MemoryManager created here: %s:%d",
                       i->second.first.c_str(),
                       i->second.second,
                       TMemoryManager::file.c_str(),
                       TMemoryManager::line);
  };
};

void* TMemoryManager::calloc(size_t nmemb, size_t size, STDLOG_SIGNATURE)
{
  void *p=::calloc(nmemb, size);
  if (trace_memory && p!=NULL) memList[p]=make_pair(file,line);
  return p;
};

void* TMemoryManager::malloc(size_t size, STDLOG_SIGNATURE)
{
  void *p=::malloc(size);
  if (trace_memory && p!=NULL) memList[p]=make_pair(file,line);
  return p;
};

void  TMemoryManager::free(void *ptr, STDLOG_SIGNATURE)
{
  if (ptr!=NULL)
  {
    if (trace_memory && memList.erase(ptr)==0)
      ProgError(STDLOG_PARAMS, "Wrong pointer. MemoryManager created here: %s:%d",
                               TMemoryManager::file.c_str(),
                               TMemoryManager::line);
    ::free(ptr);
  };
};

void* TMemoryManager::realloc(void *ptr, size_t size, STDLOG_SIGNATURE)
{
  if (ptr!=NULL)
  {
    if (trace_memory && memList.find(ptr)==memList.end())
      ProgError(STDLOG_PARAMS, "Wrong pointer. MemoryManager created here: %s:%d",
                               TMemoryManager::file.c_str(),
                               TMemoryManager::line);
  };
  void *p=::realloc(ptr, size);
  if (trace_memory && p!=NULL && p!=ptr)
  {
    memList.erase(ptr);
    memList[p]=make_pair(file,line);
  };
  return p;
};

void* TMemoryManager::create(void *ptr, STDLOG_SIGNATURE)
{
  if (trace_memory && ptr!=NULL) memList[ptr]=make_pair(file,line);
  return ptr;
};

void  TMemoryManager::destroy(void *ptr, STDLOG_SIGNATURE)
{
  if (ptr!=NULL)
  {
    if (trace_memory && memList.erase(ptr)==0)
      ProgError(STDLOG_PARAMS, "Wrong pointer. MemoryManager created here: %s:%d",
                               TMemoryManager::file.c_str(),
                               TMemoryManager::line);
  };
};

unsigned int TMemoryManager::count()
{
  return memList.size();
}

bool TMemoryManager::is_trace_memory()
{
  return trace_memory;
};

