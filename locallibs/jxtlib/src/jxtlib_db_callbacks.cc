#include "jxtlib_db_callbacks.h"
#include "jxtlib_dbora_callbacks.h"
#include "jxtlib.h"
#include "jxt_tools.h"
#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

namespace jxtlib
{

JxtlibDbCallbacks *JxtlibDbCallbacks::Instance = 0;
JxtlibDbCallbacks *JxtlibDbCallbacks::instance()
{
    if(JxtlibDbCallbacks::Instance == 0)
      // JxtlibDbCallbacks::Instance = new JxtlibDbOraCallbacks();
      throw jxtlib_exception(STDLOG, "JxtlibDbCallbacks: instance is NULL");
    return Instance;
}

void JxtlibDbCallbacks::setJxtlibDbCallbacks(JxtlibDbCallbacks *cb)
{
    if(JxtlibDbCallbacks::Instance)
      delete JxtlibDbCallbacks::Instance;
    JxtlibDbCallbacks::Instance = cb;
}

JxtlibDbCallbacks::~JxtlibDbCallbacks()
{
}

IfaceLinks::IfaceLinks(const std::string &id_, const std::string &type_,
                       const std::string &lang_, bool need_ver)
{
  ProgTrace(TRACE1,"IfaceLinks::IfaceLinks: id='%s', type='%s', "
    "lang='%s'",id_.c_str(),type_.c_str(),lang_.c_str());
  id=id_;
  type=type_;
  lang=lang_;
  ver=0;
  if(need_ver)
  {
    ver=getXmlDataVer(type.c_str(),id.c_str());
    if (ver < 0) {
      ProgTrace(TRACE1, "getXmlDataVer failed: type='%s' id='%s'", type.c_str(), id.c_str());
      ver=-1;
    }
  }
}

} // namespace jxtlib
