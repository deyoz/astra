#include <string>
#include <arpa/inet.h>

#include "jxt_xml_cont.h"
#include "gettext.h"

#include <serverlib/perfom.h>
#include <serverlib/posthooks.h>
#include <serverlib/testmode.h>
#include <serverlib/ocilocal.h>
#include "jxtlib.h"
#include "jxt_cont.h"
#include "jxt_cont_impl.h"
#include "jxt_stuff.h"
#include "jxt_sys_reqs.h"
#include "xml_tools.h"
#include "xml_stuff.h"
#include "cont_tools.h"
#include "JxtInterface.h"
#include "JxtEdiHelpManager.h"
#include <serverlib/query_runner.h>
#include "xmllibcpp.h"
#include "xml_msg.h"

#include "AccessException.h"
#include "JxtGlobalInterface.h"
#include <serverlib/ourtime.h>
#include <serverlib/str_utils.h>

#define NICKNAME "NONSTOP"
#define NICKTRACE NONSTOP_TRACE
#include <serverlib/slogger.h>

namespace jxtlib
{

jxtlib_exception::jxtlib_exception (const char* n, const char* f, int l, const char* fn, const std::string &msg)
    : comtech::Exception(n, f, l, fn, getLocalizedText(msg.c_str()))
{
}

jxtlib_exception::jxtlib_exception (const std::string &nick_, const std::string &fl_,
     int ln_, const std::string &msg) : jxtlib_exception{ nick_.c_str(), fl_.c_str(), ln_, "", msg}
{
  ProgTrace(getRealTraceLev(1),
    nick_.c_str(), fl_.c_str(), ln_, "thrown jxtlib_exception: '%s'", this->what());
}
jxtlib_exception::jxtlib_exception (const std::string &msg)
     : comtech::Exception(getLocalizedText(msg.c_str()))
{
}

JXTLibCallbacks::JXTLibCallbacks()
{
  //JXTLib::Instance()->SetCallbacks(this);
}

JXTLib *JXTLib::Instance()
{
  static JXTLib instance;
  return &instance;
}

namespace
{
void onFailCore()
{
  ServerFramework::applicationCallbacks()->rollback_db();
  JxtContext::JxtContHolder::Instance()->reset();
  callRollbackPostHooks();
  callPostHooksAlways();
  emptyHookTables();
}
void onSuccessCore()
{
  callPostHooksBefore();
  if(ServerFramework::applicationCallbacks()->commit_db())
  {
    ProgError(STDLOG,"commit_db failed!!!");
    return onFailCore();
  }
  callPostHooksAfter();
  callPostHooksAlways();
  emptyHookTables();
}
} // namespace

JxtGlobalInterface * JXTLibCallbacks::CreateGlobalInterface()
{
    return new JxtGlobalInterface;
}

void JXTLibCallbacks::initJxtContext(const std::string &pult)
{
  JxtContext::JxtContHolder::Instance()
    ->setHandler(new JxtContext::JxtContHandlerSir(pult));
}

void JXTLibCallbacks::displayMain()
{
  ProgTrace(TRACE2,"displayMain");
}

void JXTLibCallbacks::refreshMenu()
{
  ProgTrace(TRACE2,"refreshMenu");
}

void JXTLibCallbacks::mainWindow()
{
  ProgTrace(TRACE2,"mainWindow");
}

void JXTLibCallbacks::closeHandle()
{
//  ProgTrace(TRACE2,"closeHandle");
  	XMLRequestCtxt *ctxt = getXmlCtxt();
	xmlNodePtr reqNode = ctxt->reqDoc->children->children->children;
	xmlNodePtr resNode = ctxt->resDoc->children->children;
	close_handle(ctxt, reqNode, resNode);
}

void JXTLibCallbacks::updateData()
{
//  ProgTrace(TRACE2,"updateData");
	XMLRequestCtxt *ctxt = getXmlCtxt();
	xmlNodePtr reqNode = ctxt->reqDoc->children->children->children;
	xmlNodePtr resNode = ctxt->resDoc->children->children;
    UpdateData(ctxt, reqNode, resNode);
}

void JXTLibCallbacks::quitWindow()
{
//  ProgTrace(TRACE2,"quitWindow");
	XMLRequestCtxt *ctxt = getXmlCtxt();
	xmlNodePtr reqNode = ctxt->reqDoc->children->children->children;
	xmlNodePtr resNode = ctxt->resDoc->children->children;
	quit_window(ctxt, reqNode, resNode);
}

void JXTLibCallbacks::updateXmlData()
{
//  ProgTrace(TRACE2,"updateXmlData");
	XMLRequestCtxt *ctxt = getXmlCtxt();
	xmlNodePtr reqNode = ctxt->reqDoc->children->children->children;
	xmlNodePtr resNode = ctxt->resDoc->children->children;
	UpdateXmlData(ctxt, reqNode, resNode);
}

void JXTLibCallbacks::Logout()
{
  ProgTrace(TRACE2,"Logout");
}

void JXTLibCallbacks::startTask()
{
  ProgTrace(TRACE2,"startTask");
}

XMLRequestCtxt *JXTLibCallbacks::GetXmlRequestCtxt(bool reset)
{
  //ProgTrace(TRACE5, "GetXmlRequestCtxt");
  return XMLRequestCtxt::Instance(reset);
}

std::string JXTLibCallbacks::Main(const std::string &head, const std::string &body,
                                  const std::string &pult, const std::string &opr)
{
    xmlNodePtr resNode=0, reqNode=0;  // query and answer nodes

    InitLogTime(pult.c_str());  // prepare log heading part
    PerfomInit();

    XMLRequestCtxt* xmlRC = JXTLib::Instance()->GetCallbacks()->GetXmlRequestCtxt();
    if (!xmlRC)
        throw jxtlib_exception(STDLOG, "Failed to get XMLRequestCtxt");

    //  XMLRequestCtxt &xmlRC=*XMLRequestCtxt::Instance();  // request context
    static ILanguage iLanguage = ILanguage::getILanguage();

    ProgTrace(TRACE2,"****************************************");
    ProgTrace(TRACE2,"****************************************");
    ProgTrace(TRACE2,"%.6s", pult.c_str());
    ProgTrace(TRACE1,"New request received from %s (%.12s)", pult.c_str(), opr.c_str());
    ProgTrace(TRACE2,"terminal query(%zd) = '%.*s'",
              body.size(), static_cast<int>(body.size() > 2000 ? 2000 : body.size()), body.c_str());
    ProgTrace(TRACE2,"****************************************");

    try
    {
        this->initJxtContext(pult);
        xmlRC->Init(body, pult, opr);  // fill context
        reqNode = xmlRC->reqDoc->children->children->children; // command tag
        resNode = xmlRC->resDoc->children->children; // <answer> node
        ProgTrace(TRACE1,"command tag: <%s>", reqNode->name);

        writeContext("OPR", xmlRC->opr.c_str()); // store current opr id
        writeSysContext("OPR", xmlRC->opr.c_str());

        auto iface = getprop(reqNode->parent, "id");  // get iface id from <query>

        std::string ifaceName;
        if (iface)
            ifaceName = iface;

        this->UserBefore(head, body);
        JxtInterfaceMng::Instance()->OnEventWithHook(ifaceName,
                                  reinterpret_cast<const char*>(reqNode->name), xmlRC,
                                  xmlRC->reqDoc->children->children->children,
                                  xmlRC->resDoc->children->children);
        addXmlBM(*xmlRC);
        this->UserAfter();

        resNode = xmlRC->resDoc->children->children;
        xmlSetProp(resNode, "lang", iLanguage.getCode(xmlRC->getLang()));
        onSuccessCore();
        ProgTrace(TRACE1,"Everything's fine");
    }
    catch (const AccessException& e)
    {
        e.print(TRACE1);
        HandleAccessException(resNode, e);
        onFailCore();
    }
    catch(jxtlib_custom_exception &e)
    {
        tst();
        addXmlBM(*getXmlCtxt());
        onFailCore();
    }
    catch(jxtlib_exception &e)
    {
        ProgError(STDLOG,"jxtlib_exception: '%s'",e.what());
        xmlRC->resDoc=createExceptionDoc(getLocalizedText("Ошибка программы!"),0,e.what());
        onFailCore();
    }
    catch(comtech::Exception &e)
    {
        ProgTrace(TRACE1, "Main threw smth. Calling HandleException");
        HandleException(&e);
        onFailCore();
    }

    std::string answer;

    if(xmlRC->resDoc)
    {
        /* перекодируем из CP866 в UTF-8 */
        if(!xmlRC->donotencode2UTF8() &&
            xml_encode_nodelist(xmlRC->resDoc->children))
        {
            ProgError(STDLOG,"Outgoing xml-document is corrupted and cannot"
                    "be converted to utf-8");
            const char *tmp="<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                    "<term><answer handle='0'><command><error product='Сирена'>"
                    "Ошибка программы!<description>Ошибка перекодировки!"
                    "</description></error></command></answer></term>";
            answer = CP866toUTF8(tmp);
        }
        else
        {
            int anslen = 0;
            xmlChar* ans = nullptr;
            xmlDocDumpMemory(xmlRC->resDoc,&ans,&anslen);
            ProgTrace(TRACE2,"anslen=%i",anslen);
            answer.assign(ans, ans+anslen);
            xmlFree(ans);
        }
    }
    else
        ProgError(STDLOG,"resDoc is NULL");

    JXTLib::Instance()->GetCallbacks()->GetXmlRequestCtxt(true); // reset context
    InitLogTime(0);  // prepare log heading part

    return answer;
}

int JXTLibCallbacks::Main(const char *body, int blen, const char *head, int hlen, char **res, int len)
{
    std::string opr = std::string(head + 53, 12);
    std::string pult= std::string(head + 45, 6);
    pult = StrUtils::RPad(pult, 6, ' ');

    std::string answer = Main(std::string(head, hlen), std::string(body, blen), pult, opr);
    int newlen = answer.length() + hlen;

    if(newlen > len && (*res=(char *)malloc(newlen+1))==NULL)
    {
        ProgError(STDLOG,"Cannot allocate memory");
        return -1;
    }
    if(*res==NULL) // we could not try to allocate memory for res
    {
        ProgTrace(TRACE2,"*res is NULL");
        return -1;
    }
    memcpy(*res, head, hlen);

    /* после заголовка пишем в res ответное сообщение */
    memcpy(*res+hlen, answer.c_str(), answer.length());

    size_t anslen = answer.length();

    for(size_t i = 0; i < anslen && i<5000;i+=2000)
        ProgTrace(TRACE2,"res(%zd)=%.*s", anslen,
                  static_cast<int>((anslen - i) > 2000 ? 2000 : (anslen - i)), (*res) + hlen + i);
    /* переводим длину ответа в сетевой формат */
    const uint32_t nanslen=htonl(anslen);
    memcpy(*res+1, &nanslen, sizeof(nanslen));

    return newlen;
}

void JXTLibCallbacks::HandleException(comtech::Exception *e)
{
    ProgTrace(TRACE1,"exception: '%s'",e->what());
}

void JXTLibCallbacks::HandleAccessException(xmlNodePtr resNode, const AccessException& e)
{
    ProgTrace(TRACE1,"AccessException: '%s'",e.what());
    addXmlMessageBoxFmt(resNode, e.what());
}

struct JXTLib::Impl
{
      struct greater_str_n {
          bool operator() (std::string const& a, std::string const& b) const noexcept {
              auto const min_len = std::min(a.size(), b.size());
              return a.compare(0, min_len, b) < 0;
          }
      };

    std::map<std::string, std::unique_ptr<XmlDataDesc>, greater_str_n> data_impls;
    std::unique_ptr<JXTLibCallbacks> jxt_lc;

    Impl() : jxt_lc(std::make_unique<JXTLibCallbacks>()) {}
};

JXTLib::JXTLib() : pimpl(std::make_unique<Impl>()) {}

const XmlDataDesc *JXTLib::getDataImpl(const std::string &data_id)
{
    auto const pos = pimpl->data_impls.find(data_id);
    if(pos == pimpl->data_impls.end())
        return nullptr;
    return pos->second.get();
}

JXTLib *JXTLib::addDataImpl(std::unique_ptr<XmlDataDesc> xdd)
{
    pimpl->data_impls[xdd->data_id] = std::move(xdd);
    return this;
}

JXTLibCallbacks * JXTLib::GetCallbacks()
{
    return pimpl->jxt_lc.get();
}

void JXTLib::SetCallbacks(std::unique_ptr<JXTLibCallbacks> p)
{
    pimpl->jxt_lc.swap(p);
}

} // namespace jxtlib

#include "lngv.h"
#include "gettext.h"

namespace loclib
{

LocaleLibCallbacks::LocaleLibCallbacks()
{
  //LocaleLib::Instance()->SetCallbacks(this);
}

int LocaleLibCallbacks::getCurrLang()
{
  int lang=-1;
  XMLRequestCtxt *ctxt=getXmlCtxt();
  lang=(ctxt==NULL)?1 /* russian by default */:ctxt->getLang();
  return lang;
}

void LocaleLibCallbacks::prepare_localization_map(LocalizationMap &lm,
                                                  bool search_for_dups)
{
  lm.clear();
}

const char *LocaleLibCallbacks::get_msg_by_num(unsigned int code, int lang)
{
  return (lang==ENGLISH) ? "MESSAGE NUMBER OUT OF RANGE" :
                           "НОМЕР СООБЩЕНИЯ НЕ ИЗВЕСТЕН СИСТЕМЕ";
}

LocaleLib *LocaleLib::Instance()
{
  static LocaleLib *instance=0;
  if(!instance)
    instance=new LocaleLib;
  return instance;
}

} // namespace loclib


namespace ServerFramework
{

std::string JxtEdiHelpManager::make_text(const std::string& text)
{
  /* Упаковка txt в XML */
  const int max_kick_text_size=1000;

  std::string iface_id=readSysContextNVL("CUR_IFACE","");
  std::string handle=readSysContextNVL("HANDLE","");
  std::string oper=readSysContextNVL("OPR","");

	if(iface_id.empty() || handle.empty() || oper.empty())
  {
    LogError(STDLOG)<<"Failed to read what we wanted to: "
		          "CUR_IFACE='"<<iface_id<<"', HANDLE='"<<handle<<"', OPR='"<<oper<<"'";
    //throw sirena_exception("Error in readSysContext");
    if(handle.empty())
      handle="0";
	}

  std::string res="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<term>"
    "<query handle='"+handle+"' id='"+iface_id+"' ver='0' opr='"+oper+"'>";

	if(iface_id=="sirena_terminal")
  {
    /* Запрос от XML sirena_terminal */
    res+="<re_sirena_request><sirena_request><str>"+text+
         "</str></sirena_request></re_sirena_request>";
  }
  else
  {
    /* Запрос не от XML sirena_terminal */
    res+="<kick>"+text+"</kick>";
	}
	res+="</query></term>";

  if((int)res.size()>max_kick_text_size)
  {
     ProgError(STDLOG,"Too long text for kick: %zd bytes", res.size());
     throw jxtlib::jxtlib_exception(STDLOG,"Too long text for kick");
  }

  ProgTrace(TRACE1,"make_text created '%s'",res.c_str());
  return CP866toUTF8(res);
}
} // namespace ServerFramework

