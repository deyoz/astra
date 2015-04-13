#ifndef _ASTRA_CALLBACKS_H_
#define _ASTRA_CALLBACKS_H_

#include "jxtlib/jxtlib.h"
#include "string"

class AstraJxtCallbacks : public jxtlib::JXTLibCallbacks
{
  private:
    void (*fp_post_process)();
  public:
    AstraJxtCallbacks() : JXTLibCallbacks()
    {
      jxtlib::JXTLib::Instance()->SetCallbacks(this);
      fp_post_process = NULL;
    }
    void SetPostProcessXMLAnswerCallback(void (*post_process_ptr)())
    {
        fp_post_process = post_process_ptr;
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

bool BuildMsgForTermRequestDup(const std::string &pult,
                               const std::string &opr,
                               const std::string &body,
                               std::string &msg);
bool BuildMsgForWebRequestDup(short int client_id,
                              const std::string &body,
                              std::string &msg);
void CheckTermResDoc();

#endif /*_ASTRACALLBACKS_H_*/

