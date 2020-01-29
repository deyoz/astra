#include <string>
#include <map>
#include <stdarg.h>
#include <string.h>
#define NICKNAME "ILYA"
#define NICKTRACE ILYA_TRACE
#include <serverlib/test.h>
#include "lngv.h"
#include "gettext.h"
#include "jxtlib.h"

using namespace std;

const int KEY_LANGUAGE=RUSSIAN; // lngv.h
const bool SEARCH_FOR_DUPS=true;
const int MAX_TEXT_LENGTH=2000;

LocalizationMap &getLocMap(int lang)
{
  static bool en_map_initialized=false;
  static LocalizationMap en_map; // not quite sure we'll have any more
  static int ru_map_initialized=false;
  static LocalizationMap ru_map; // not quite sure we'll have any more

  if(lang==ENGLISH)
  {
      if(!en_map_initialized)
      {
          loclib::LocaleLib::Instance()->GetCallbacks()
                  ->prepare_localization_map(en_map,SEARCH_FOR_DUPS);
          en_map_initialized=true;
      }
      return en_map;
  }

  if(!ru_map_initialized)
  {
      loclib::LocaleLib::Instance()->GetCallbacks()
              ->prepare_localization_ru_map(ru_map,SEARCH_FOR_DUPS);
      ru_map_initialized=true;
  }
  return ru_map;
}

char *_getLocalizedText(const string &key_str, va_list ap, int lng=-1)
{
  ProgTrace(TRACE1,"key_str='%s'",key_str.c_str());
  int lang=lng<0?getCurrLang():lng;
  static char result[MAX_TEXT_LENGTH+1];
  if(lang==KEY_LANGUAGE)
    vsnprintf(result,MAX_TEXT_LENGTH,key_str.c_str(),ap);
  else
  {
    LocalizationMap &loc_map=getLocMap(lang);
    LocalizationMap::iterator pos=loc_map.find(key_str);
    if(pos==loc_map.end()) // not found
    {
      ProgTrace(TRACE1,"!!?!! no translation for '%.2000s'",
                /*lang_code.c_str(),*/key_str.c_str());
      vsnprintf(result,MAX_TEXT_LENGTH,key_str.c_str(),ap);
    }
    else
      vsnprintf(result,MAX_TEXT_LENGTH,pos->second.c_str(),ap);
  }
  return result;
}

std::string getLocalText(const std::string &key, int lang)
{
    const LocalizationMap &loc_map = getLocMap(lang);
    LocalizationMap::const_iterator pos=loc_map.find(key);
    if(pos==loc_map.end()) // not found
    {
        ProgError(STDLOG,"Œ… ˜ˆŠˆ … ˆ‡‚…‘’… ‘ˆ‘’…Œ…");
        return lang==RUSSIAN?"˜ˆŠ€ ‚ ƒ€ŒŒ…":"PROGRAM ERROR";
    } else {
        return pos->second;
    }
}

const char *getLocalCText(const std::string &key, int lang)
{
    return getLocalText(key, lang).c_str();
}

const char *getLocalizedText(const char *key_str, ...)
{
  va_list ap;
  va_start(ap,key_str);
  char *result=_getLocalizedText(key_str?key_str:"",ap);
  va_end(ap);
  return result;
}

const char *getLocalizedText(int lang, const char *key_str, ...)
{
  va_list ap;
  va_start(ap,key_str);
  char *result=_getLocalizedText(key_str?key_str:"",ap,lang);
  va_end(ap);
  return result;
}

namespace
{
int get_param_text(char *s, unsigned code, va_list ap, int lng=-1)
{
  int lang=lng<0?loclib::LocaleLib::Instance()->GetCallbacks()->getCurrLang():lng;
  const char *str=loclib::LocaleLib::Instance()->GetCallbacks()
                                               ->get_msg_by_num(code,lang);
  if(!str)
  {
    str=(lang==ENGLISH) ? "MESSAGE NUMBER OUT OF RANGE" :
                          "Œ… ˜ˆŠˆ … ˆ‡‚…‘’… ‘ˆ‘’…Œ…";
  }
  vsprintf(s,str,ap);
  return strlen(s);
}
}

const char *getLocalizedTextL(unsigned err_code, int lang, ...)
{
  static char result[MAX_TEXT_LENGTH+1];
  va_list ap;
  va_start(ap,lang);
  get_param_text(result,err_code,ap,lang);
  va_end(ap);
  return result;
}

const char *getLocalizedText(unsigned err_code, ...)
{
  static char result[MAX_TEXT_LENGTH+1];
  va_list ap;
  va_start(ap,err_code);
  get_param_text(result,err_code,ap);
  va_end(ap);
  return result;
}

