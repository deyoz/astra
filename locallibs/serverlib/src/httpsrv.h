#ifndef __HTTPSRV_H
#define __HTTPSRV_H

#include <vector>
#include <string>
#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "strong_types.h"
#include "internal_msgid.h"

namespace httpsrv {

DEFINE_STRING_WITH_LENGTH_VALIDATION(Pult, 6, 6, "PULT", "�����");
DEFINE_STRING_WITH_LENGTH_VALIDATION(Domain, 1, 51, "HTTP DAEMON DOMAIN", ""); /* e.g. "RAIL", "AEZH", etc */
DEFINE_STRING_WITH_LENGTH_VALIDATION(Certificate, 1, 4000, "SSL CERTIFICATE", "SSL ����������"); /* X509 certificate */
DEFINE_STRING_WITH_LENGTH_VALIDATION(PrivateKey, 1, 4000, "SSL PRIVATE KEY", "�������� ���� SSL");
DEFINE_STRING_WITH_LENGTH_VALIDATION(CorrelationID, 1, 256, "CORRELATION ID", "CORRELATION ID");

DEFINE_BOOL(UseSSLFlag);

struct Stat {
    unsigned activeSessions;
    unsigned activeConnections;
    unsigned totalConnections;
    unsigned totalSessions;
    unsigned totalConnErr;
    unsigned totalConnErrTimeout;
    unsigned totalConnErrHandshake;
    unsigned nRetries;
    unsigned nRetriesOK;
};

std::ostream& operator<<(std::ostream& out, const Stat& stat);

struct HostAndPort {
    std::string host;
    unsigned port;
    HostAndPort(std::string host, unsigned port): host(std::move(host)), port(port) {}
    HostAndPort(const std::string& host_colon_port);
};

/* �ந����쭠� (user-defined) ���ଠ��, ����� ���� ������� �� ������� �⢥� */
struct CustomData {
    std::string s1; /* max. 255 chars */
    std::string s2; /* max. 255 chars */
    int n1 = 0;
    int n2 = 0;
};

/* ����� ��� SSL ��⥭�䨪�樨 ������ */
struct ClientAuth {
    Certificate cert;
    PrivateKey pkey;
    ClientAuth(const Certificate& cert, const PrivateKey& pkey): cert(cert), pkey(pkey) {}
};

/* ����� ��� �����䨪�樨 �����/�⢥� ��� ��᫥���饩 �롮ન ⥬ �� �����⮬ */
class ReqCorrelationData {
public:
    explicit ReqCorrelationData(Pult&& p) : pult(std::move(p)) {}
    explicit ReqCorrelationData(ServerFramework::InternalMsgId&& id) : MsgId(std::move(id)) {}
    explicit ReqCorrelationData(CorrelationID&& cid) : CorrId(std::move(cid)) {}
    bool operator==(const ReqCorrelationData& rhs) const {
        return (this == &rhs) || (pult == rhs.pult && MsgId == rhs.MsgId && CorrId == rhs.CorrId) ;
    };
    boost::optional<Pult> pult;
    boost::optional<ServerFramework::InternalMsgId> MsgId;
    boost::optional<CorrelationID> CorrId;
protected:
    ReqCorrelationData() {}
};


struct HttpReq {
    ReqCorrelationData correlation;
    Domain domain;
    HostAndPort hostAndPort;
    boost::posix_time::ptime time; /* �६� ����饭�� ����� � �� */
    std::string text; /* HTTP ⥪�� �����, ������ HTTP ��������� */
    std::vector<boost::posix_time::ptime> deadlines; /* �६� ����㯫���� ⠩���⮢ ��� ������ ����⪨ */
    CustomData customData;
    boost::optional<ClientAuth> clientAuth;
    std::string peresprosReq;
    UseSSLFlag useSSL;
    int64_t recId;
    int32_t importance;
    std::size_t groupSize;
    HttpReq(
            const ReqCorrelationData& corr,
            const Domain& domain,
            const HostAndPort& hostAndPort,
            const boost::posix_time::ptime& time,
            const std::string& text,
            const std::vector<boost::posix_time::ptime>& deadlines,
            const CustomData& customData,
            const boost::optional<ClientAuth>& clientAuth,
            const std::string& peresprosReq,
            const UseSSLFlag& useSSL,
            const int64_t recId,
            const int32_t importance,
            const std::size_t groupSize = 1);
};

/* Communication error code */
enum CommErrCode {
    COMMERR_SSL = 1,
    COMMERR_TIMEOUT,
    COMMERR_RESOLVE,
    COMMERR_CONNECT,
    COMMERR_HANDSHAKE,
    COMMERR_WRITE,
    COMMERR_READ_HEADERS,
    COMMERR_READ_CHUNK_HEADER,
    COMMERR_CHUNK_SIZE,
    COMMERR_READ_CHUNK_DATA,
    COMMERR_READ_CHUNK_DATA_CRLF,
    COMMERR_READ_CONTENT
};

/* Communication error */
struct CommErr {
    CommErrCode code;
    std::string errMsg;

    CommErr(const CommErrCode& code, const std::string& errMsg):
        code(code), errMsg(errMsg)
    {}
};

struct HttpResp {
    boost::posix_time::ptime time; /* �६� ����祭�� �⢥� */
    std::string text; /* HTTP �⢥�, ������ HTTP ��������� */
    boost::optional<CommErr> commErr;
    HttpReq req;  /* �����, �� ����� ����祭 �⢥� */

    HttpResp(
            const boost::posix_time::ptime& time,
            const std::string& text,
            const boost::optional<CommErr>& commErr,
            const HttpReq& req);
};

/*
 * �������� HTTP ����� � �� � ����� HTTP �����.
 * ����� ����� �㭪樨 ��� 㤮��⢠ ࠡ��� � ����易⥫�묨 ��ࠬ��ࠬ�.
 *
 * domain - �ந������ ��ப��� �����䨪��� "�।��⭮� ������",
 * ���ਬ�� Domain("RAIL"). ��㦨� ��� ���ᯥ祭�� �����������஢����� ࠡ���
 * ࠧ����� �����⥬ ��७� (���, ��������...)
 */
class DoHttpRequest {
public:
    DoHttpRequest(std::string const& host, unsigned port, std::string reqText);
    DoHttpRequest(
            Pult pult,
            Domain domain,
            HostAndPort hostAndPort,
            std::string reqText);
    DoHttpRequest(
        ServerFramework::InternalMsgId MsgId,
        Domain domain,
        HostAndPort hostAndPort,
        std::string reqText);
    DoHttpRequest(
        CorrelationID cid,
        Domain domain,
        HostAndPort hostAndPort,
        std::string reqText);
    /* ����易⥫�� ��ࠬ���� */
    DoHttpRequest& setCustomData(const CustomData& customData);
    DoHttpRequest& setClientAuth(const ClientAuth& clientAuth);
    DoHttpRequest& setMaxTryCount(unsigned maxTryCount); /* ���ᨬ��쭮� �᫮ ����⮪ ��९��뫪� ����� */
    DoHttpRequest& setTimeout(const boost::posix_time::time_duration& timeout); /* ��� ������ ����⪨ */
    DoHttpRequest& setPeresprosReq(const std::string& req);
    DoHttpRequest& setPeresprosTimeout(const boost::posix_time::time_duration& timeout);
    DoHttpRequest& setSSL(const UseSSLFlag& useSSL); /* ॠ�쭮 �㦭� ⮫쪮 �⮡� �몫���� ssl */
    DoHttpRequest& setDeffered(const bool isDeffered = false); /* ������ � ᮪�� ⮫쪮 ��᫥ ������ */
    DoHttpRequest& doHttpLog();
    DoHttpRequest& doHttpLog(std::map<std::string, std::string> &&data);
    DoHttpRequest& setGroupSize(const std::size_t size);

    int64_t operator()(); /* �������� ����� � �� � ᪮�䨣��஢��� ������ */
private:
    DoHttpRequest(
            ReqCorrelationData&& corr,
            Domain&& domain,
            HostAndPort&& hostAndPort,
            std::string&& reqText);
    const ReqCorrelationData correlation_;
    const Domain domain_;
    const HostAndPort hostAndPort_;
    const std::string reqText_;
    CustomData customData_;
    boost::optional<ClientAuth> clientAuth_;
    boost::posix_time::time_duration timeout_; /* default = 15 */
    std::string peresprosReq_;
    boost::posix_time::time_duration peresprosTimeout_; /* default = (timeout_ * maxTryCount) + 10 */
    unsigned maxTryCount_; /* default = 1 */
    UseSSLFlag useSSL_; /* default = true (using ssl)*/
    bool isDeffered_;
    int64_t recId_;
    int32_t importance_{0};
    std::map<std::string, std::string> data_;
    std::size_t groupSize_{1};
};

struct HasNotSentAQueryBefore : public ServerFramework::Exception
{
    explicit HasNotSentAQueryBefore();
};

/* ����稢 ������, obrzap �⠥� �⢥�� �� ᮪�� ������ */
std::vector<HttpResp> FetchHttpResponses(Pult pult, const Domain& domain, bool waitAll = true, bool throw_on_hasntsent = false, bool fetchAllResponses = true);
std::vector<HttpResp> FetchHttpResponses(const ServerFramework::InternalMsgId& MsgId, const Domain& domain, bool waitAll = true, bool throw_on_hasntsent = false);
std::vector<HttpResp> FetchHttpResponses(CorrelationID cid, const Domain& domain, bool waitAll = true, bool throw_on_hasntsent = false);
std::vector<HttpResp> FetchHttpResponses(bool waitAll = true, bool throw_on_hasntsent = false);

boost::optional<Stat> GetStat();

struct HttpPayload {
    unsigned httpRespCode; /* 200 = OK */
    std::string text;
};
boost::optional<HttpPayload> HttpResponsePayload(const std::string& httpResponse);

enum CheckHttpFlags
{
    Off    = 0x0000,
    Body   = 0x0001,
    Header = 0x0002,
    XML    = 0x0004,
    JSON   = 0x0008,
    Full   = (Body | Header)
};

#ifdef XP_TESTING
namespace xp_testing {
std::string MakeDefaultHttpsHeaders(size_t contentLen);
void add_forecast(const std::string& quote_response_quote);
void set_ignore_next_request(bool x = true);
void set_capture_request(bool x = true);
void set_check_http_headers(int x = CheckHttpFlags::Full);
void set_need_real_http(bool x = true);
void set_need_real_commit(bool x = true);
std::vector<std::string> getOutgoingHttpRequests();
}
#endif // XP_TESTING
bool forkAllowed();

} /* namespace httpsrv */

#endif /* #ifndef __HTTPSRV_H */
