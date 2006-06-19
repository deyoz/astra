#ifndef __JXTLIB_SIR_H__
#define __JXTLIB_SIR_H__

#ifdef __cplusplus

#include "jxtlib.h"

/*****************************************************************************/
/*********************      SirenaJxtCallbacks      **************************/
/*****************************************************************************/
class AstraJxtCallbacks : public jxtlib::JXTLibCallbacks
{
  public:
    AstraJxtCallbacks() : JXTLibCallbacks()
    {
      jxtlib::JXTLib::Instance()
      ->SetCallbacks(this);
    }
//    virtual int Main(const char *body, int blen, const char *head, int hlen,
//                       char **res, int len);


};

/*****************************************************************************/
/*********************     SirenaLocaleCallbacks    **************************/
/*****************************************************************************/
#include "gettext.h"

class AstraLocaleCallbacks : public loclib::LocaleLibCallbacks
{
  public:
    AstraLocaleCallbacks() : loclib::LocaleLibCallbacks()
    {
      loclib::LocaleLib::Instance()->SetCallbacks(this);
    }
/*
    virtual int getCurrLang();
    virtual void prepare_localization_map(LocalizationMap &lm,
                                          bool search_for_dups);
    virtual const char *get_msg_by_num(unsigned int code, int lang);
*/    
};

#endif /* __cplusplus */

#endif /* __JXTLIB_SIR_H__ */
