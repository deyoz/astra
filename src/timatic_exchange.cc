#include "timatic_exchange.h"
#include "edi_utils.h"
#include "libtimatic/timatic_request.h"
#include "libtimatic/timatic_service.h"
#include "md5_sum.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC::date_time;

namespace Timatic {
    class TimaticClient : public ExchangeIterface::HTTPClient
    {
        private:
            string jsessionID;
        public:
            virtual string makeHttpPostRequest( const string& postbody) const {
                ostringstream os;
                os
                    << "POST " << resource << " HTTP/1.0\r\n"
                    << "Host: " <<  m_addr.host << ":" << m_addr.port << "\r\n"
                    <<  (jsessionID.empty() ? "" : "Cookie: JSESSIONID=" + jsessionID + ";\r\n")
                    << "Content-Type: text/xml; charset=utf-8\r\n"
                    << "Content-Length: " << postbody.size() << "\r\n\r\n"
                    << postbody;
                return os.str();
            }
            TimaticClient( const TimaticSession &session ):
                ExchangeIterface::HTTPClient(session.config.host, session.config.port, 15000, true, "/timaticwebservices/timatic3services", {})
                {
                    jsessionID = session.fromDB().jsessionID;
                }
            ~TimaticClient(){}
    };

    void TimaticExchangeIface::BeforeRequest(xmlNodePtr& reqNode, xmlNodePtr externalSysResNode, const SirenaExchange::TExchange& req)
    {
        AstraLocale::showProgError("MSG.TIMATIC_CONNECT_ERROR");
        NewTextChild( reqNode, "exchangeId", req.exchangeId() );
    }

    void TimaticExchangeIface::BeforeResponseHandle(int reqCtxtId, xmlNodePtr& reqNode, xmlNodePtr& ResNode, std::string& exchangeId)
    {
        xmlNodePtr exchangeIdNode = NodeAsNode( "exchangeId", reqNode );
        exchangeId = NodeAsString( exchangeIdNode );
        RemoveNode(exchangeIdNode);
        LogTrace(TRACE5) << "exchangeId=" << exchangeId;
    }

    void TimaticExchangeIface::response_DataReq(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
    {
        Res res(reqNode, externalSysResNode);
        processReq(reqNode, resNode, res.http_data.get());
    }

    void TimaticExchangeIface::response_LoginReq(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
    {
        Data data(reqNode, externalSysResNode);
        if(not data.http_data->get<LoginResp>().successfulLogin()) {
            data.session.invalidate();
            throw AstraLocale::UserException("Timatic: login failed");
        } else
            data.request();
    }

    void TimaticExchangeIface::response_CheckNameReq(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
    {
        Login login(reqNode, externalSysResNode);
        if(not login.http_data->response()->errors().empty()) {
            login.session.invalidate();
            processReq(reqNode, resNode, login.http_data.get());
        } else
            login.request();
    }

    void TimaticExchangeIface::Request(const std::string& ifaceName, TimaticExchange& req)
    {
        TimaticClient client( req.session );
        ExchangeIface* iface=dynamic_cast<ExchangeIface*>(JxtInterfaceMng::Instance()->GetInterface(ifaceName));
        iface->DoRequest(req.reqNode, nullptr, req, client);
    }

    void TimaticExchangeIface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
    {
        using namespace AstraEdifact;

        const std::string DefaultAnswer = "<answer/>";
        std::string pult = TReqInfo::Instance()->desk.code;
        LogTrace(TRACE3) << __FUNCTION__ << " for pult [" << pult << "]";
        boost::optional<httpsrv::HttpResp> resp = ExchangeIterface::HTTPClient::receive(pult,domainName);
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
            int req_ctxt_id = NodeAsInteger("@req_ctxt_id", reqNode);
            XMLDoc termReqCtxt;
            getTermRequestCtxt(req_ctxt_id, true, "ExchangeIface::KickHandler", termReqCtxt);
            xmlNodePtr termReqNode = NodeAsNode("/term/query", termReqCtxt.docPtr())->children;
            if(termReqNode == NULL)
                throw EXCEPTIONS::Exception("ExchangeIface::KickHandler: context TERM_REQUEST termReqNode=NULL");;
            XMLDoc answerResDoc;
            answerResDoc = ASTRA::createXmlDoc2(DefaultAnswer);
            if(resp) NewTextChild(answerResDoc.docPtr()->children, "content", StrUtils::b64_encode(resp->text));
            std::string exchangeId;
            BeforeResponseHandle(req_ctxt_id, termReqNode, answerResDoc.docPtr()->children, exchangeId);
            handleResponse(exchangeId, termReqNode, answerResDoc.docPtr()->children, resNode);
        }
    }

    void Data::build(std::string &content) const
    {
        if(not reqNode) throw Exception("Data::build: reqNode not defined");

        std::shared_ptr<Timatic::Request> req;
        if(string("run") == (const char *)reqNode->name)
            req = parseRequest(reqNode);
        else
            req = parseDocRequest(reqNode);
        req->requestID(2);
        req->subRequestID("ec81d2080f1ac94d14f9ba4d9fce");
        content = req->content(session.fromDB());
    }

    void Login::build(std::string &content) const
    {
        Timatic::CheckNameResp checkResp = http_data.get().get<Timatic::CheckNameResp>();
        Timatic::LoginReq request;
        request.requestID(2);
        request.subRequestID(std::to_string(time(nullptr)));
        request.loginInfo(md5_sum(md5_sum(session.config.pwd) + checkResp.randomNumber() + checkResp.sessionID()));
        session.set_sessionID(checkResp.sessionID());
        content = request.content(session.fromDB());
    }

    void CheckName::build(std::string &content) const
    {
        Timatic::CheckNameReq request;
        request.requestID(1);
        request.subRequestID(std::to_string(time(nullptr)));
        request.username(session.config.username);
        request.subUsername(session.config.sub_username);
        content = request.content({});
    }

    TimaticExchange::TimaticExchange(xmlNodePtr _reqNode, xmlNodePtr externalSysResNode)
        : session(_reqNode), reqNode(_reqNode)
    {
        http_data = Timatic::HttpData(StrUtils::b64_decode(NodeAsString("content", externalSysResNode)));
        if(http_data->httpCode() != 200) {
            session.invalidate();
            throw UserException("MSG.TIMATIC.EXCHANGE_ERROR", LParams() << LParam("msg", http_data->content()));
        }
        session.set_jsessionID(http_data->jsessionID());
    }

    const ExtSession &ExtSession::fromXML(xmlNodePtr node)
    {
        clear();
        if(not node) return *this;
        xmlNodePtr curNode = node->children;
        curNode = GetNodeFast("session", curNode);
        if(curNode) {
            curNode = curNode->children;
            sessionID = NodeAsStringFast("session_id", curNode);
            jsessionID = NodeAsStringFast("jsession_id", curNode);
        }
        return *this;
    }

    void ExtSession::clear()
    {
        sessionID.clear();
        jsessionID.clear();
    }

    void ExtSession::toXML(xmlNodePtr node)
    {
        if(empty()) return;
        if(GetNode("session", node)) return;

        xmlNodePtr sessNode = NewTextChild(node, "session");
        NewTextChild(sessNode, "session_id", sessionID);
        NewTextChild(sessNode, "jsession_id", jsessionID);
    }

    void TimaticExchange::request()
    {
        session.config.toXML(reqNode);
        TimaticExchangeIface::Request( TimaticExchangeIface::getServiceName(), *this );
    }

    
    void TimaticSession::clear()
    {
        config.clear();
    }

    ExtSession TimaticSession::fromDB() const
    {
        TCachedQuery Qry(
                "select * from timatic_session where "
                "   host = :host and "
                "   port = :port and "
                "   username = :username and "
                "   sub_username = :sub_username and "
                "   pwd = :pwd and "
                "   expire > :expire ",
                QParams()
                << QParam("host", otString, config.host)
                << QParam("port", otInteger, config.port)
                << QParam("username", otString, config.username)
                << QParam("sub_username", otString, config.sub_username)
                << QParam("pwd", otString, config.pwd)
                << QParam("expire", otDate, NowUTC()));
        Qry.get().Execute();

        if(not Qry.get().Eof)
            return ExtSession(
                    Qry.get().FieldAsString("sessionID"),
                    Qry.get().FieldAsString("jsessionID"));
        return {};
    }

    void TimaticSession::toDB(const ExtSession &s) const
    {
        bool invalidate = s.sessionID.empty() and s.jsessionID.empty();
        QParams QryParams;
        QryParams
            << QParam("host", otString, config.host)
            << QParam("port", otInteger, config.port)
            << QParam("username", otString, config.username)
            << QParam("sub_username", otString, config.sub_username)
            << QParam("pwd", otString, config.pwd)
            << QParam("expire", otDate, NowUTC() + 27./(24.*60.));
        if(invalidate or not s.jsessionID.empty()) QryParams << QParam("jsessionID", otString, s.jsessionID);
        if(invalidate or not s.sessionID.empty()) QryParams << QParam("sessionID", otString, s.sessionID);

        string SQLText =
            "declare "
            "   pragma autonomous_transaction; "
            "begin "
            "   insert into timatic_session ( "
            "      host, "
            "      port, "
            "      username, "
            "      sub_username, "
            "      pwd, ";
        if(invalidate or not s.jsessionID.empty()) SQLText += " jsessionID, ";
        if(invalidate or not s.sessionID.empty()) SQLText += " sessionID, ";
        SQLText +=
            "      expire "
            "   ) values ( "
            "      :host, "
            "      :port, "
            "      :username, "
            "      :sub_username, "
            "      :pwd, ";
        if(invalidate or not s.jsessionID.empty()) SQLText += " :jsessionID, ";
        if(invalidate or not s.sessionID.empty()) SQLText += " :sessionID, ";
        SQLText +=
            "      :expire "
            "      ); "
            "   commit; "
            "exception "
            "   when dup_val_on_index then "
            "       update timatic_session set ";
        if(invalidate or not s.jsessionID.empty()) SQLText += " jsessionID = :jsessionID, ";
        if(invalidate or not s.sessionID.empty()) SQLText += " sessionID = :sessionID, ";
        SQLText += 
            "           expire = :expire "
            "       where "
            "           host = :host and "
            "           port = :port and "
            "           username = :username and "
            "           sub_username = :sub_username and "
            "           pwd = :pwd; "
            "       commit; "
            "end; ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
    }
}
