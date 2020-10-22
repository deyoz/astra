#ifndef __JXTLIB_H__
#define __JXTLIB_H__

#ifdef __cplusplus

#include <string>
#include <memory>
#include <libxml/tree.h>

#include "gettext.h"
#include <serverlib/exception.h>


class XMLRequestCtxt;
class AccessException;
class JxtGlobalInterface;

namespace jxtlib
{


struct jxtlib_exception : public comtech::Exception
{
  jxtlib_exception (const char* n, const char* f, int l, const char* fn, const std::string &msg);
  jxtlib_exception (const std::string &nick_, const std::string &fl_, int ln_, const std::string &msg);
  jxtlib_exception (const std::string &msg);
};

class jxtlib_custom_exception : public jxtlib_exception
{
  public:
  jxtlib_custom_exception (const std::string &nick_, const std::string &fl_, int ln_)
    :jxtlib_exception(nick_,fl_,ln_,std::string())
  {
  }
};

class E_throw_xmlerr : public jxtlib_custom_exception
{
  public:
    E_throw_xmlerr(const char *tag, const char *fmt, ...);
    E_throw_xmlerr(const char *tag, const unsigned char *fmt, ...);
    E_throw_xmlerr(const char *tag, unsigned err, ...);
    E_throw_xmlerr(const xmlChar* tag, unsigned err, ...);
    E_throw_xmlerr(const xmlChar* tag, const char* msg);
    E_throw_xmlerr(const char* tag, const std::string &msg);
    E_throw_xmlerr(const std::string& tag, const char* msg);
    E_throw_xmlerr(const std::string& tag, const std::string &msg);
};

class JXTLibCallbacks
{
  public:
  JXTLibCallbacks();
  virtual ~JXTLibCallbacks() {}
  virtual std::string Main(const std::string &head, const std::string &body,
                           const std::string &pult, const std::string &opr);
  virtual int Main(const char *body, int blen, const char *head, int hlen,
                   char **res, int len);

  virtual JxtGlobalInterface * CreateGlobalInterface();
  virtual void Force2Register() {}
  virtual void InitInterfaces() {}
  virtual bool AlreadyLogged() { return true;  }
  virtual void BlockPult() {}

  /*
   *    returns pointer to XMLRequestCtxt class if reset is false,
   *    elsewhere destroys previous XMLRequestCtxt instance,
   *    creates new and returns pointer to it
   * */
  virtual XMLRequestCtxt *GetXmlRequestCtxt(bool reset = false);
  virtual void HandleException(comtech::Exception *e);
  virtual void HandleAccessException(xmlNodePtr resNode, const AccessException& e);
  virtual void UserBefore(const std::string &/*head*/, const std::string &/*body*/) {}
  virtual void UserAfter() {}
  virtual void initJxtContext(const std::string &pult);
  virtual void displayMain();
  virtual void refreshMenu();
  virtual void mainWindow();
  virtual void closeHandle();
  virtual void updateData();
  virtual void updateXmlData();
  virtual void quitWindow();
  virtual void Logout();
  virtual void startTask();
  virtual void fillJxtBgndJobsList() { }
};


struct XmlDataDesc
{
  typedef int int_parameter;
  std::string data_id;
  int_parameter (* getDataVersion)(const std::string &data_id);
  void (* getData)(const std::string &data_id, xmlNodePtr resNode,
                   int_parameter term_version, int_parameter serv_version);
  XmlDataDesc(const std::string &data_id_,
    int_parameter (* getDataVersion_)(const std::string &data_id),
    void (* getData_)(const std::string &data_id, xmlNodePtr resNode,
                   int_parameter term_version, int_parameter serv_version))
      : data_id(data_id_),getDataVersion(getDataVersion_),getData(getData_)
  {
  }
  XmlDataDesc(): data_id(std::string()),getDataVersion(nullptr),getData(nullptr)
  {
  }
};

class JXTLib
{
    struct Impl;
    std::unique_ptr<Impl> pimpl;
    JXTLib();
  public:
    static JXTLib *Instance();
    JXTLibCallbacks *GetCallbacks();
    void SetCallbacks(std::unique_ptr<JXTLibCallbacks>);
    JXTLib* addDataImpl(std::unique_ptr<XmlDataDesc>);
    template<typename... Args> JXTLib* addDataImpl(Args&&... args) {
        return addDataImpl(std::make_unique<XmlDataDesc>(std::forward<Args>(args)...));
    }
    const XmlDataDesc *getDataImpl(const std::string &data_id);
};

} // namespace jxtlib


namespace loclib
{

class LocaleLibCallbacks
{
  public:
    LocaleLibCallbacks();
    virtual ~LocaleLibCallbacks() {}
    virtual int getErrorMessageId() { return 2; }
    virtual int getMessageId() { return 1; }
    virtual int getUnknownMessageId() { return 0; }
    virtual int getCurrLang();
    virtual void prepare_localization_map(LocalizationMap &lm,
                                          bool search_for_dups);

    virtual void prepare_localization_ru_map(LocalizationMap &/*lm*/,
                                          bool /*search_for_dups*/)
    {
    }
    virtual const char *get_msg_by_num(unsigned int code, int lang);
};

class LocaleLib
{
  private:
    LocaleLibCallbacks *loc_lc;
    LocaleLib()
    {
      loc_lc=new LocaleLibCallbacks();
    }
  public:
    static LocaleLib *Instance();
    LocaleLibCallbacks *GetCallbacks()
    {
      return loc_lc;
    }
    void SetCallbacks(LocaleLibCallbacks *loc_lc_)
    {
      delete loc_lc;
      loc_lc=loc_lc_;
    }
};

} // namespace loclib

#endif /* __cplusplus */

#endif /* __JXTLIB_H__ */
