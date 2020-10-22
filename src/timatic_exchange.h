#pragma once

#include "ExchangeIface.h"
#include "timatic_request.h"
#include "libtimatic/timatic_http.h"

namespace Timatic {

    struct ExtSession: public Session {
        void toXML(xmlNodePtr node);
        const ExtSession &fromXML(xmlNodePtr node);
        bool empty() const { return sessionID.empty() or jsessionID.empty(); }
        void clear();
        ExtSession(const std::string &_sessionID, const std::string &_jsessionID)
        {
            sessionID = _sessionID;
            jsessionID = _jsessionID;
        }
        ExtSession(): ExtSession({}, {}) {}
    };

    struct TimaticSession {
        TTimaticAccess config;

        ExtSession fromDB() const;
        void toDB(const ExtSession &s) const;
        void clear();
        void invalidate() const { return toDB(ExtSession({}, {})); }
        void set_sessionID(const std::string &sessionID) const { return toDB(ExtSession(sessionID, {})); }
        void set_jsessionID(const std::string &jsessionID) const { return toDB(ExtSession({}, jsessionID)); }

        TimaticSession() { clear(); }
        TimaticSession(const TTimaticAccess &a): config(a) {}
        TimaticSession(xmlNodePtr reqNode)
        {
            config.fromXML(reqNode);
        }
    };

    class TimaticExchange: public SirenaExchange::TExchange {
        public:
            TimaticSession session;
            xmlNodePtr reqNode;

            virtual bool isRequest() const {
                return true;
            }
            void request();
            TimaticExchange(const TTimaticAccess &_config, xmlNodePtr _reqNode): session(_config), reqNode(_reqNode) {}
            TimaticExchange(xmlNodePtr _reqNode, xmlNodePtr externalSysResNode);
            boost::optional<HttpData> http_data;
    };

    class Res: public TimaticExchange {
        public:
            virtual std::string exchangeId() const { return "ResReq";}
            virtual std::string methodName() const {
                return "Res";
            }
            virtual void clear() {}
            virtual void build(std::string &content) const {};
            Res(xmlNodePtr _reqNode, xmlNodePtr externalSysResNode): TimaticExchange(_reqNode, externalSysResNode) {}
    };

    class CheckName: public TimaticExchange {
        public:
            virtual std::string exchangeId() const { return "CheckNameReq";}
            virtual std::string methodName() const {
                return "CheckName";
            }
            virtual void clear() {}
            virtual void build(std::string &content) const;
            CheckName(const TTimaticAccess &_config, xmlNodePtr _reqNode): TimaticExchange(_config, _reqNode) {}
    };

    class Login: public TimaticExchange {
        public:
            virtual std::string exchangeId() const { return "LoginReq";}
            virtual std::string methodName() const {
                return "Login";
            }
            virtual void clear() {}
            virtual void build(std::string &content) const;
            Login(xmlNodePtr _reqNode, xmlNodePtr externalSysResNode): TimaticExchange(_reqNode, externalSysResNode) {}
    };

    class Data: public TimaticExchange {
        public:
            virtual std::string exchangeId() const { return "DataReq";}
            virtual std::string methodName() const {
                return "Data";
            }
            virtual void clear() {}
            virtual void build(std::string &content) const;
            Data(xmlNodePtr _reqNode, xmlNodePtr externalSysResNode): TimaticExchange(_reqNode, externalSysResNode) {}
            Data(const TTimaticAccess &access, xmlNodePtr _reqNode): TimaticExchange(access, _reqNode) {}
    };


    class TimaticExchangeIface: public ExchangeIterface::ExchangeIface
    {
        private:
        protected:
            virtual void BeforeRequest(xmlNodePtr& reqNode, xmlNodePtr externalSysResNode, const SirenaExchange::TExchange& req);
            virtual void BeforeResponseHandle(int reqCtxtId, xmlNodePtr& reqNode, xmlNodePtr& ResNode, std::string& exchangeId);
        public:
            static const std::string getServiceName() {
                return "timatic_exchange";
            }
            TimaticExchangeIface() : ExchangeIterface::ExchangeIface(getServiceName()) {
                domainName = "TIMATIC";
                addResponseHandler("CheckNameReq", response_CheckNameReq);
                addResponseHandler("LoginReq", response_LoginReq);
                addResponseHandler("DataReq", response_DataReq);
            }

            static void response_CheckNameReq(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
            static void response_LoginReq(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
            static void response_DataReq(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);

            static void Request(const std::string& ifaceName, TimaticExchange& req);
            virtual void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
    };

}
