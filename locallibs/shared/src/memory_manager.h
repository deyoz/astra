#ifndef memory_managerH
#define memory_managerH
#include <string>
#include <map>
#include "serverlib/logger.h"

class TMemoryManager
{
  private:
    std::map<void*, std::pair<std::string, int> > memList;
    std::string file;
    int line;
    bool trace_memory;

  public:
    TMemoryManager(STDLOG_SIGNATURE);
    ~TMemoryManager();

    void *calloc(size_t nmemb, size_t size, STDLOG_SIGNATURE);
    void *malloc(size_t size, STDLOG_SIGNATURE);
    void  free(void *ptr, STDLOG_SIGNATURE);
    void *realloc(void *ptr, size_t size, STDLOG_SIGNATURE);
    
    void* create(void *ptr, STDLOG_SIGNATURE);
    void  destroy(void *ptr, STDLOG_SIGNATURE);
    unsigned int count();
    bool is_trace_memory();
};

#endif


