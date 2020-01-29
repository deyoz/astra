#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include <serverlib/test.h>
#include <string>
#include <map>
#include "jxt_stuff.h"

using namespace std;

ILanguage &ILanguage::getILanguage()
{
  static ILanguage *instance=0;
  if(!instance)
    instance=new ILanguage();
  return *instance;
}

void ILanguage::initILangMap()
{
  lang_map.clear();
  lang_map[0]="EN";
  lang_map[1]="RU";
}

std::string ILanguage::getCode(int ida)
{
  map<int,std::string>::iterator pos=lang_map.find(ida);
  if(pos!=lang_map.end())
    return pos->second;
  ProgError(STDLOG,"Unknown language: ida=%i",ida);
  // use default language - russian
  return getCode(1);
}

int ILanguage::getIda(const std::string &code)
{
  map<int,std::string>::iterator pos;
  for(pos=lang_map.begin();pos!=lang_map.end();++pos)
  {
    if(code==pos->second)
      return pos->first;
  }
  ProgError(STDLOG,"Unknown language: code='%s'",code.c_str());
  // use default language - russian
  return getIda("RU");
}
