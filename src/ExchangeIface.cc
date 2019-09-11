#include "ExchangeIface.h"
#include "edi_utils.h"
#include "astra_context.h"
#include "astra_iface.h"
#include <serverlib/testmode.h>
#include <serverlib/xml_stuff.h>
#include <serverlib/query_runner.h>
#include <serverlib/EdiHelpManager.h>
#include <boost/asio.hpp>
#define NICKNAME "DJEK"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>


namespace ExchangeIterface
{


/*static std::string makeHttpPostRequest(const std::string& resource,
                                       const std::string& host,
                                       const std::string& postbody)
{
    return "POST " + resource + " HTTP/1.1\r\n"
             "Host: " + host + "\r\n"
             "Content-Type: application/xml; charset=utf-8\r\n"
             "Content-Length: " + HelpCpp::string_cast(postbody.size()) + "\r\n"
             "\r\n" +
             postbody;
}*/


void HTTPClient::sendRequest(const std::string &reqText, const edifact::KickInfo& kickInfo, const std::string& domainName) const
{
    //msgid = kickInfo.msgId;
    LogTrace(TRACE5) << __func__ << "msgid=" << kickInfo.msgId;
    LogTrace(TRACE5) << __func__ << "handle=" << kickInfo.jxt.get().handle;
    LogTrace(TRACE5) << ServerFramework::getQueryRunner().getEdiHelpManager().msgId();
    tst();
    std::string desk = kickInfo.desk.empty() ? "SYSPUL" : kickInfo.desk;

    const std::string path = "";
    const std::string httpPost = makeHttpPostRequest( path, m_addr.host, "Authorization:Basic eG1sX2FzdHJhX0dSVF9VVDpXMFJ5M0VEQng0", reqText );

    LogTrace(TRACE5) << "HTTP Request from [" << desk << "], text:\n" << reqText;

    const httpsrv::Pult pul(desk);
    const httpsrv::Domain domain(domainName);
    const std::string kick(AstraEdifact::make_xml_kick(kickInfo));
    //LogTrace(TRACE5)<<httpPost;
    //LogTrace(TRACE5)<<m_addr.host << ":" <<m_addr.port;
    //LogTrace(TRACE5)<<"timeout=" << m_timeout;

    httpsrv::DoHttpRequest req(ServerFramework::getQueryRunner().getEdiHelpManager().msgId(),
                               domain, m_addr, httpPost);
    req.setTimeout(boost::posix_time::seconds(/*m_timeout*/30))
        .setMaxTryCount(1/*SIRENA_REQ_ATTEMPTS()*/)
        .setSSL(httpsrv::UseSSLFlag(false))
        .setPeresprosReq(kick)
        .setDeffered(true);

#ifdef XP_TESTING
    if (inTestMode()) {
        const std::string httpPostCP866 = UTF8toCP866(httpPost);
        LogTrace(TRACE1) << "request: " << httpPostCP866;
        xp_testing::TlgOutbox::getInstance().push(tlgnum_t("httpreq"),
                        StrUtils::replaceSubstrCopy(httpPostCP866, "\r", ""), 0 /* h2h */);
    }
#endif // XP_TESTING

    req();
}

boost::optional<httpsrv::HttpResp> HTTPClient::receive(const std::string& pult, const std::string &domainName)
{
    const httpsrv::Pult pul(pult);
    const httpsrv::Domain domain(domainName);

    LogTrace(TRACE5)<<ServerFramework::getQueryRunner().getEdiHelpManager().msgId();
tst();
    const std::vector<httpsrv::HttpResp> responses = httpsrv::FetchHttpResponses(
                ServerFramework::getQueryRunner().getEdiHelpManager().msgId(),
                domain);
tst();
    for (const httpsrv::HttpResp& httpResp: responses)
    {
        LogTrace(TRACE1) << "httpResp text: '" << httpResp.text << "'";
    }
tst();
    const size_t responseCount = responses.size();
    if (responseCount == 0)
    {
        LogTrace(TRACE1) << "FetchHttpResponses: hasn't got any responses yet";
        return {};
    }
    if (responseCount > 1)
    {
        LogError(STDLOG) << "FetchHttpResponses: " << responseCount << " responses!";
    }

    return responses.front();
}
//=============================================================================

bool ExchangeIface::equal(const ExchangeResponseHandler& handler1,
                          const ExchangeResponseHandler& handler2)
{
  return (func_equal::getAddress(handler1)==func_equal::getAddress(handler2));
}

bool ExchangeIface::addResponseHandler( const std::string& funcName, const ExchangeResponseHandler& res)
{
  if (!res || funcName.empty()) return false;
  if ( resHandlers.find(funcName) != resHandlers.end() ) {
    return false;
  }
/*  for(const ExchangeResponseHandler& handler : resHandlers)
    if (equal(handler, res)) {
      return false;
    }*/
  resHandlers.emplace( funcName,res );
  LogTrace(TRACE5) << __func__ << " add " << funcName;
  return true;
}

void ExchangeIface::handleResponse(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode) const
{
  std::map<std::string,ExchangeResponseHandler>::const_iterator handler = resHandlers.find(exchangeId);
  if ( handler != resHandlers.end() &&
       (handler->second) ) {
    handler->second(handler->first, reqNode, externalSysResNode, resNode);
  }
  else {
    EXCEPTIONS::Exception(std::string("ExchangeIface::handleResponse: handle response " ) + exchangeId + " not define" );;
  }
}

void ExchangeIface::KickHandler(XMLRequestCtxt *ctxt,
                                     xmlNodePtr reqNode,
                                     xmlNodePtr resNode)
{
    using namespace AstraEdifact;

    const std::string DefaultAnswer = "<answer/>";
    std::string pult = TReqInfo::Instance()->desk.code;
    LogTrace(TRACE3) << __FUNCTION__ << " for pult [" << pult << "]";
    boost::optional<httpsrv::HttpResp> resp = HTTPClient::receive(pult,domainName);
    if(resp) {
        //LogTrace(TRACE3) << "req:\n" << resp->req.text;
        if(resp->commErr) {
             LogError(STDLOG) << "Http communication error! "
                              << "(" << resp->commErr->code << "/" << resp->commErr->errMsg << ")";
        }
    } else {
        LogError(STDLOG) << "Enter to KickHandler but HttpResponse is empty!";
    }

    if(GetNode("@req_ctxt_id",reqNode) != NULL)
    {
    tst();
        int req_ctxt_id = NodeAsInteger("@req_ctxt_id", reqNode);
        XMLDoc termReqCtxt;
        getTermRequestCtxt(req_ctxt_id, true, "ExchangeIface::KickHandler", termReqCtxt);
        xmlNodePtr termReqNode = NodeAsNode("/term/query", termReqCtxt.docPtr())->children;
        if(termReqNode == NULL)
          throw EXCEPTIONS::Exception("ExchangeIface::KickHandler: context TERM_REQUEST termReqNode=NULL");;

        std::string answerStr = DefaultAnswer;
        std::string endAnswer = "</answer>";
        if(resp) {
            const auto fnd_b = resp->text.find("<answer"); //!!!<answer>
            if(fnd_b != std::string::npos) {
                answerStr = resp->text.substr(fnd_b);
            }
            const auto fnd_e = answerStr.find(endAnswer); //!!!<answer>
            if(fnd_e != std::string::npos) {
              answerStr.erase(answerStr.begin() + fnd_e + endAnswer.size(),answerStr.end());
            }
        }

        XMLDoc answerResDoc;
        try
        {
          answerResDoc = ASTRA::createXmlDoc2(answerStr);
        }
        catch(std::exception &e)
        {
          LogError(STDLOG) << "ASTRA::createXmlDoc2(answerStr) error";
          answerResDoc = ASTRA::createXmlDoc2(DefaultAnswer);
        }
        tst();
        xmlNodePtr answerResNode = NodeAsNode("/answer", answerResDoc.docPtr());
        tst();
        std::string exchangeId;
        BeforeResponseHandle(req_ctxt_id, termReqNode, answerResNode, exchangeId);
        handleResponse(exchangeId, termReqNode, answerResNode, resNode);
    }
    tst();
}

void ExchangeIface::DoRequest(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, const SirenaExchange::TExchange& req, const HTTPClient &client)
{
    using namespace AstraEdifact;

    LogTrace(TRACE3) << __FUNCTION__ << ": " << req.exchangeId();

    BeforeRequest( reqNode, externalSysResNode, req );
    int reqCtxtId = AstraContext::SetContext("TERM_REQUEST", XMLTreeToText(reqNode->doc));
    std::string reqText;
    req.build(reqText);
    tst();
    client.sendRequest(reqText, createKickInfo(reqCtxtId,GetIfaceName()), domainName);

//    SvcSirenaInterface* iface=dynamic_cast<SvcSirenaInterface*>(JxtInterfaceMng::Instance()->GetInterface(SvcSirenaInterface::name()));
//    if (iface!=nullptr && iface->addResponseHandler(res))
//      LogTrace(TRACE5) << "added response handler for <" << req.exchangeId() << ">";
}


}
