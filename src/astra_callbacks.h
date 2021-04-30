#ifndef _ASTRA_CALLBACKS_H_
#define _ASTRA_CALLBACKS_H_

#include "jxtlib/jxtlib.h"
#include "jxtlib/jxt_cont_impl.h"
#include "string"

class AstraJxtCallbacks : public jxtlib::JXTLibCallbacks
{
  private:
    void (*fp_post_process)();
  public:
    AstraJxtCallbacks();
    void SetPostProcessXMLAnswerCallback(void (*post_process_ptr)());
    void ResetPostProcessXMLAnswerCallback();

    virtual void InitInterfaces();
    virtual void HandleException(ServerFramework::Exception *e);
    virtual void UserBefore(const std::string &head, const std::string &body);
    virtual void UserAfter();
    virtual void initJxtContext(const std::string &pult) override;
};

class AstraJxtContHandlerSir : public JxtContext::JxtContHandlerSir
{
  private:
    virtual JxtContext::JxtCont *createContext(int handle) override;
  public:
    AstraJxtContHandlerSir(const std::string &pult)
        : JxtContext::JxtContHandlerSir(pult)
    {
    }
};

class AstraJxtContSir : public JxtContext::JxtContSir
{
  public:
    explicit AstraJxtContSir(const std::string &pult, int hnd,
                             const JxtContext::JxtContStatus &stat = JxtContext::UNCHANGED)
        : JxtContext::JxtContSir(pult, hnd, stat) {}

  private:
    virtual void addRow(const JxtContext::JxtContRow *row) override;
    virtual void deleteRow(const JxtContext::JxtContRow *row) override;
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

