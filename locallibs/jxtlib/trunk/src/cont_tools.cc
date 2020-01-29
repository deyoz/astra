#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <string.h>
#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include <serverlib/test.h>

#include <string>
#include <sstream>
#include <map>

#include "cont_tools.h"
#include "jxt_cont.h"
#include "jxt_handle.h"
#include "jxt_xml_cont.h"
#include "jxt_stuff.h"

using namespace JxtContext;
using namespace std;

int setCurContext(const char *term, int handle)
{
  getJxtContHandler()->setCurrentContext(handle);
  return 0;
}

void readcont(const char *name, char *buf, int bufsize)
{
  snprintf(buf,bufsize,"%s",readContext(name).c_str());
}

void SaveContextsHook()
{
  JxtContHolder::Instance()->finalize(true);
}

std::string readContext(const char *name)
{
  return readContextNVL(name, "");
}

std::string readContextNVL(const char *name, const char *NVL)
{
  return getJxtContHandler()->currContext()->read(name, NVL);
}

int writeContext(const char* name, bool val)
{
    return writeContext<int>(name, val);
}

int writeContext(const char* name, const std::string& val)
{
    return writeContext(name, val.c_str());
}

extern "C" int writeContext(const char *name, const char *value)
{
  if(!name)
  {
    ProgTrace(TRACE1,"name='%s'",name);
    throw JxtContException("writeContext");
  }
  if(!value)
    getJxtContHandler()->currContext()->remove(name);
  else
    getJxtContHandler()->currContext()->write(name,value);
  return 0;
}

extern "C" int readContextInt(const char *name, int NVL)
{
  return getJxtContHandler()->currContext()->readInt(name,NVL);
}

extern "C" int writeContextInt(const char *name, int value)
{
  if(!name)
  {
    ProgTrace(TRACE1,"name='%s', value='%i'",name,value);
    throw JxtContException("writeContext");
  }
  getJxtContHandler()->currContext()->write(name,value);
  return 0;
}

extern "C" int readWContextInt(int handle, const char *name, int NVL)
{
  return getJxtContHandler()->getContext(handle)->readInt(name,NVL);
}

std::string readSysContext(const char *name)
{
  return getJxtContHandler()->sysContext()->read(name);
}

extern "C" int readSysContextInt(const char *name, int NVL)
{
  return getJxtContHandler()->sysContext()->readInt(name,NVL);
}

std::string readSysContextNVL(const char *name, const char *NVL)
{
  return getJxtContHandler()->sysContext()->read(name,NVL);
}

extern "C" int writeSysContext(const char *name, const char *value)
{
  if(!name || !value)
  {
    ProgTrace(TRACE1,"name='%s', value='%s'",name,value);
    throw JxtContException("writeContext");
  }
  getJxtContHandler()->sysContext()->write(name,value);
  return 0;
}

extern "C" int writeSysContextInt(const char *name, int value)
{
  if(!name)
  {
    ProgTrace(TRACE1,"name='%s', value='%i'",name,value);
    throw JxtContException("writeContext");
  }
  getJxtContHandler()->currContext()->write(name,value);
  return 0;
}

