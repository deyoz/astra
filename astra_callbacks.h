#ifndef _ASTRA_CALLBACKS_H_
#define _ASTRA_CALLBACKS_H_

#include "jxtlib.h"

class AstraJxtCallbacks : public jxtlib::JXTLibCallbacks
{
  public:
    AstraJxtCallbacks() : JXTLibCallbacks()
    {
      jxtlib::JXTLib::Instance()->SetCallbacks(this);
    }      
    virtual void InitInterfaces();    
    virtual void HandleException(std::exception *e);    
    virtual void UserBefore(const char *body, int blen, const char *head,
                          int hlen, char **res, int len);
    virtual void UserAfter();    
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

#endif /*_ASTRACALLBACKS_H_*/

