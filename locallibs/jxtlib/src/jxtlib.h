#ifndef __JXTLIB_H__
#define __JXTLIB_H__

#ifdef __cplusplus

#include <exception>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <functional>
#include <map>
#include <libxml/tree.h>

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

class E_throw_xmlerr : public jxtlib::jxtlib_custom_exception
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
  virtual void Force2Register()
  {}
  virtual void InitInterfaces()
  {}
  virtual bool AlreadyLogged()
  { return true;  }
  virtual void BlockPult()
  {}

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
  virtual void fillJxtBgndJobsList()
  {
  }
  // moved out because of Oci
  //virtual void getIfaceLinks(xmlNodePtr resNode, const std::string &iface_id);
  //virtual long lib_getXmlDataVer(const std::string &type, const std::string &id);
  //virtual long lib_getDataVer(const std::string &data_id);
  //virtual void updateJxtData(const char *id, long term_ver, xmlNodePtr resNode);
  //virtual void insertXmlData(const std::string &type, const std::string &id,
  //                           const std::string &data);
  //virtual std::string getXmlData(const std::string &type,
  //                               const std::string &type, long ver);

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
  private:
      struct greater_str_n {
          bool operator() (std::string const& a, std::string const& b) const noexcept {
              auto const min_len = std::min(a.size(), b.size());
              return a.compare(0, min_len, b) < 0;
          }
      };
  std::map<std::string, XmlDataDesc *, greater_str_n> data_impls;
    JXTLibCallbacks *jxt_lc;
    JXTLib()
    {
      jxt_lc=new JXTLibCallbacks();
      data_impls.clear();
    }
  public:
    static JXTLib *Instance();
    JXTLibCallbacks *GetCallbacks()
    {
      return jxt_lc;
    }
    void SetCallbacks(JXTLibCallbacks *jxt_lc_)
    {
      delete jxt_lc;
      jxt_lc=jxt_lc_;
    }
    JXTLib *addDataImpl(XmlDataDesc *xdd);
    const XmlDataDesc *getDataImpl(const std::string &data_id);
};

} // namespace jxtlib


#include "gettext.h"
namespace loclib
{

class LocaleLibCallbacks
{
  public:
    LocaleLibCallbacks();
    virtual ~LocaleLibCallbacks() {}
    virtual int getErrorMessageId()
    {
      return 2;
    }
    virtual int getMessageId()
    {
      return 1;
    }
    virtual int getUnknownMessageId()
    {
      return 0;
    }
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
