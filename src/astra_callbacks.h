#ifndef _ASTRA_CALLBACKS_H_
#define _ASTRA_CALLBACKS_H_

#include "jxtlib/jxtlib.h"

class AstraJxtCallbacks : public jxtlib::JXTLibCallbacks
{
  public:
    AstraJxtCallbacks() : JXTLibCallbacks()
    {
      jxtlib::JXTLib::Instance()->SetCallbacks(this);
    }
    virtual void InitInterfaces();
    virtual void HandleException(ServerFramework::Exception *e);
    virtual void UserBefore(const std::string &head, const std::string &body);
    virtual void UserAfter();
};

/*****************************************************************************/
/*********************     SirenaLocaleCallbacks    **************************/
/*****************************************************************************/
#include "jxtlib/gettext.h"

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

