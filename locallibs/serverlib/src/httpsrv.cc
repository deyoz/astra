#define NICKNAME "DMITRYVM"
#define NICKTRACE DMITRYVM_TRACE
#include "slogger.h"

#include "httpsrv.h"
#include <list>
#include <set>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <cstdlib>
#include <queue>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <memory>

#include <type_traits>
#include "monitor_ctl.h"
#include "logger.h"
#include "ourtime.h"
#include "cursctl.h"
#include "testmode.h"
#include "posthooks.h"
#include "daemon_kicker.h"
#include "daemon_event.h"
#include "dates_oci.h"
#include "daemon_impl.h"
#include "zlib_employment.h"
#include "EdiHelpManager.h"
#include "query_runner.h"
#include "stream_holder.h"
#include "deffered_exec.h"
#include "checkunit.h"
#include "noncopyable.h"
#include "xml_tools.h"
#include "http_logs_callbacks.h"
#include "ntlm_service.h"

static const char* const HTTPSRV_CMD_TCLVAR = "HTTPSRV_CMD"; /* Сокет для пинков */
static size_t MAX_CONN_POOL_SIZE = 4096;

void connect_oracle(); /* from obrzap.cc */

#if 0
#define LogError(x) std::cerr << std::endl << __FILE__ << ':' << __LINE__ << ':'
#define LogTrace(x) std::cerr << std::endl << __FILE__ << ':' << __LINE__ << ':'
#endif

namespace {
struct Forecast {
    std::string httpResponse;
};

static struct {
    std::queue<Forecast> forecasts;
    std::queue<httpsrv::HttpResp> responses;
    std::deque<httpsrv::HttpReq> requests;
    bool ignoreNextRequest = false;
    bool captureRequest = false;
    int checkHttpHeaders = httpsrv::CheckHttpFlags::Off;
    bool need_real_http = false;
    bool need_real_commit = false;
} _ForTests = {};
}//anonymous ns

namespace ServerFramework {

template<class Archive>
void serialize(Archive & ar, InternalMsgId& t, const unsigned int version)
{
}


template<class Archive>
void save_construct_data(Archive& ar, const InternalMsgId* p, const unsigned int version)
{
    ar << p->id();
}

template<class Archive>
void load_construct_data(Archive& ar, InternalMsgId* p, const unsigned int version)
{
    std::decay_t<decltype(std::declval<InternalMsgId>().id())> raw;
    ar >> raw;
    new (p) InternalMsgId(std::move(raw));
}


}//ServerFramework


namespace httpsrv {

class ReqCorrelationData_io : public ReqCorrelationData {
public:
    ReqCorrelationData_io() {}
};

std::ostream& operator<<(std::ostream& s, const ReqCorrelationData& d) {
    if (d.pult) {
        s << "pult: " << d.pult->str() ;
    } else if (d.MsgId) {
        s << "MsgId: " <<  *d.MsgId ;
    } else if (d.CorrId) {
        s << "CorrId: " << d.CorrId->str();
    } else {
        s << "!!! Empty correlation data !!! ";
    }
    return s;
}

HostAndPort::HostAndPort(const std::string& host_colon_port)
{
    auto c = host_colon_port.find(':');
    if(c == std::string::npos)
        throw ServerFramework::Exception(STDLOG, __FUNCTION__, "malformed argument: can't find ':'");
    host = host_colon_port.substr(0,c);
    port = std::stoul(host_colon_port.substr(c+1));
}

HttpReq::HttpReq(const ReqCorrelationData& correlation,
        const Domain& domain,
        const HostAndPort& hostAndPort,
        const boost::posix_time::ptime& time,
        const std::string& text,
        const std::vector<boost::posix_time::ptime>& deadlines,
        const CustomData& customData,
        const CustomAuth& customAuth,
        const boost::optional<ClientAuth>& clientAuth,
        const std::string& peresprosReq,
        const UseSSLFlag& useSSL,
        const int64_t recId,
        const int32_t importance,
        const std::size_t groupSize):
    correlation(correlation),
    domain(domain),
    hostAndPort(hostAndPort),
    time(time),
    text(text),
    deadlines(deadlines),
    customData(customData),
    customAuth(customAuth),
    clientAuth(clientAuth),
    peresprosReq(peresprosReq),
    useSSL(useSSL),
    recId(recId),
    importance(importance),
    groupSize(groupSize)
{}

HttpResp::HttpResp(
        const boost::posix_time::ptime& time,
        const std::string& text,
        const boost::optional<CommErr>& commErr,
        const HttpReq& req):
    time(time),
    text(text),
    commErr(commErr),
    req(req)
{}

/*******************************************************************************
 * Статистика
 ******************************************************************************/

Stat _Stat;

std::ostream& operator<<(std::ostream& out, const Stat& stat)
{
    out << "activeSessions = " << stat.activeSessions
        << ", activeConnections = " << stat.activeConnections
        << ", totalConnections = " << stat.totalConnections
        << ", totalSessions = " << stat.totalSessions
        << ", totalConnErr = " << stat.totalConnErr
        << ", totalConnErrTimeout = " << stat.totalConnErrTimeout
        << ", totalConnErrHandshake = " << stat.totalConnErrHandshake
        << ", nRetries = " << stat.nRetries
        << ", nRetriesOK = " << stat.nRetriesOK;
    return out;
}

template<class Archive>
void serialize(Archive& ar, Stat& t, const unsigned int version)
{
    ar & t.activeSessions;
    ar & t.activeConnections;
    ar & t.totalConnections;
    ar & t.totalSessions;
    ar & t.totalConnErr;
    ar & t.totalConnErrTimeout;
    ar & t.totalConnErrHandshake;
    ar & t.nRetries;
    ar & t.nRetriesOK;
}

/*******************************************************************************
 * Утилиты
 ******************************************************************************/

static boost::optional<size_t> ParseContentLength(const std::string& httpHeaders)
{
    static const boost::regex re(".*\r?\nContent-Length: (\\d+)\r?\n.*", boost::regex::icase);
    boost::smatch what;
    if (!boost::regex_match(httpHeaders, what, re))
        return boost::optional<size_t>();
    return boost::lexical_cast<size_t>(what[1]);
}

static bool ChunkedTransferEncoding(const std::string& httpHeaders)
{
    static const boost::regex re(".*\r?\nTransfer-Encoding: chunked\r?\n.*", boost::regex::icase);
    return boost::regex_match(httpHeaders, re);
}

static bool isAnswerGZipped(const std::string& httpHeaders)
{
    static const boost::regex re(".*\r?\nContent-Encoding: gzip\r?\n.*", boost::regex::icase);
    return boost::regex_match(httpHeaders, re);
}


static boost::asio::ip::tcp::resolver::query MakeResolverQuery(const HostAndPort& hostAndPort)
{
    std::ostringstream port;
    port << hostAndPort.port;
    return boost::asio::ip::tcp::resolver::query(hostAndPort.host, port.str());
}

static std::string MakeErrorHttpResponse(CommErr const & err)
{
    const auto content = err.errMsg.empty() ? "error description not available" : err.errMsg;

    std::ostringstream out;
    out << "HTTP/1.0 " << (err.code == COMMERR_TIMEOUT ? 504 : 500) << " Internal Server Error\r\n"
        << "Content-Type: text/plain; charset=UTF-8\r\n"
        << "Content-Length: " << content.size() << "\r\n"
        << "Server: HTTPSRV\r\n\r\n"
        << content;

    return out.str();
}

static void Commit()
{
    if (!inTestMode() || _ForTests.need_real_commit) {
        callPostHooksBefore();
        if (inTestMode())
            commitInTestMode();
        else
            make_curs("COMMIT").exec();
        callPostHooksAfter();
        emptyHookTables();
    }
}

/*******************************************************************************
 * Сериализация
 ******************************************************************************/

typedef boost::archive::text_iarchive IArchive_t;
typedef boost::archive::text_oarchive OArchive_t;

/* CustomData */

template<class Archive>
void serialize(Archive& ar, CustomData& t, const unsigned int version)
{
    ar & t.s1;
    ar & t.s2;
    ar & t.n1;
    ar & t.n2;
}

/* ReqCorrelationData */
template<class Archive>
void load(Archive & ar, ReqCorrelationData & t, const unsigned int version)
{
    std::string pult;
    ServerFramework::InternalMsgId* MsgId = nullptr;
    std::string CorrId;
    ar >> pult;
    ar >> MsgId;
    std::unique_ptr<ServerFramework::InternalMsgId> MsgId_free(MsgId);
    ar >> CorrId;
    int field_count = 0;
    if (!pult.empty()) {
        t.pult = Pult(pult);
        ++field_count;
    } else {
        t.pult.reset();
    }
    if (MsgId) {
        t.MsgId = *MsgId ;
        ++field_count;
    } else {
        t.MsgId.reset();
    }
    if (!CorrId.empty()) {
        t.CorrId = CorrelationID(CorrId);
        ++field_count;
    } else {
        t.CorrId.reset();
    }
    if (field_count != 1)
        throw ServerFramework::Exception(STDLOG, __FUNCTION__, "invalid correlation data");
}

template<class Archive>
void save(Archive & ar, const ReqCorrelationData& t, const unsigned int version)
{
    ar << (t.pult ? t.pult->str() : std::string());
    const ServerFramework::InternalMsgId *MsgId = nullptr;
    if(t.MsgId) {
        MsgId = t.MsgId.get_ptr();
    }
    ar << MsgId;
    ar << (t.CorrId ? t.CorrId->str() : std::string());
}


template<class Archive>
void serialize(Archive & ar, ReqCorrelationData& t, const unsigned int version)
{
    boost::serialization::split_free(ar, t, version);
}

/* ReqCorrelationData_io */

template<class Archive>
void serialize(Archive & ar, ReqCorrelationData_io& t, const unsigned int version)
{
    serialize(ar, static_cast<ReqCorrelationData&>(t), version);
}

/* ClientAuth */

template<class Archive>
void serialize(Archive & ar, ClientAuth& t, const unsigned int version)
{
}

template<class Archive>
void save_construct_data(Archive& ar, const ClientAuth* p, const unsigned int version)
{
    ar << p->cert.str();
    ar << p->pkey.str();
}

template<class Archive>
void load_construct_data(Archive& ar, ClientAuth* p, const unsigned int version)
{
    std::string cert, pkey;
    ar >> cert;
    ar >> pkey;
    new (p) ClientAuth(Certificate(cert), PrivateKey(pkey));
}

/* HttpReq */

template<class Archive>
void serialize(Archive & ar, HttpReq& t, const unsigned int version)
{
}

template<class Archive>
void save_construct_data(Archive& ar, const HttpReq* p, const unsigned int version)
{
    ar << p->correlation;
    ar << p->domain.str();
    ar << p->hostAndPort.host;
    ar << p->hostAndPort.port;
    ar << p->time;
    ar << p->text;
    ar << p->deadlines;
    ar << p->customData;
    ar << p->customAuth;
    const ClientAuth* clientAuth = p->clientAuth ? &(*p->clientAuth) : NULL;
    ar << clientAuth;
    ar << p->peresprosReq;
    int useSSL = (p->useSSL.getInt());
    ar << useSSL;
    ar << p->recId;
    ar << p->importance;
    ar << p->groupSize;
}

template<class Archive>
void load_construct_data(Archive& ar, HttpReq* p, const unsigned int version)
{
    std::string domain;
    std::string host;
    unsigned port = 0;
    boost::posix_time::ptime time;
    std::string text;
    std::vector<boost::posix_time::ptime> deadlines;
    CustomData customData;
    CustomAuth customAuth = CustomAuth::None;
    ClientAuth* clientAuth = NULL;
    std::string peresprosReq;
    int useSSL = 0;
    int64_t recId = 0;
    int32_t importance = 0;
    std::size_t groupSize = 1;
    ReqCorrelationData_io correlation;

    ar >> correlation;
    ar >> domain;
    ar >> host;
    ar >> port;
    ar >> time;
    ar >> text;
    ar >> deadlines;
    ar >> customData;
    ar >> customAuth;
    ar >> clientAuth;
    std::unique_ptr<ClientAuth> clientAuth_free(clientAuth);
    ar >> peresprosReq;
    ar >> useSSL;
    ar >> recId;
    ar >> importance;
    ar >> groupSize;

    new (p) HttpReq(
            correlation,
            Domain(domain),
            HostAndPort(host, port),
            time,
            text,
            deadlines,
            customData,
            customAuth,
            clientAuth ? *clientAuth : boost::optional<ClientAuth>(),
            peresprosReq,
            UseSSLFlag(useSSL),
            recId,
            importance,
            groupSize);
}

/* CommErr */

template<class Archive>
void serialize(Archive & ar, CommErr& t, const unsigned int version)
{
}

template<class Archive>
void save_construct_data(Archive& ar, const CommErr* p, const unsigned int version)
{
    ar << p->code;
    ar << p->errMsg;
}

template<class Archive>
void load_construct_data(Archive& ar, CommErr* p, const unsigned int version)
{
    CommErrCode code;
    std::string errMsg;
    ar >> code;
    ar >> errMsg;
    new (p) CommErr(code, errMsg);
}

/* HttpResp */

template<class Archive>
void serialize(Archive & ar, HttpResp& t, const unsigned int version)
{
}

template<class Archive>
void save_construct_data(Archive& ar, const HttpResp* p, const unsigned int version)
{
    const HttpReq* req = &p->req;
    ar << p->time;
    ar << p->text;
    const CommErr* commErr = p->commErr ? &(*p->commErr) : NULL;
    ar << commErr;
    ar << req;
}

template<class Archive>
void load_construct_data(Archive& ar, HttpResp* p, const unsigned int version)
{
    boost::posix_time::ptime time;
    std::string text;
    CommErr* commErr = NULL;
    HttpReq* req = NULL;

    ar >> time;
    ar >> text;
    ar >> commErr;
    std::unique_ptr<CommErr> commErr_free(commErr);
    ar >> req;
    std::unique_ptr<HttpReq> req_free(req);
    new (p) HttpResp(
            time,
            text,
            commErr ? *commErr : boost::optional<CommErr>(),
            *req);
}





/*******************************************************************************
 * Работа с сертификатами
 ******************************************************************************/

struct SSLException: public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// <<str>> _must_ live at least until the bio created is not destroyed
static std::shared_ptr<BIO> NewStrBIO(const std::string& str)
{
    BIO* bio = BIO_new_mem_buf(const_cast<char*>(str.data()), (int)str.size());
    if (bio == NULL)
        throw SSLException("BIO_new_mem_buf failed");
    return std::shared_ptr<BIO>(bio, BIO_vfree);
}

static std::shared_ptr<X509> NewX509(const Certificate& cert)
{
    auto cert_bio = NewStrBIO(cert.str());
    X509* x509_cert = PEM_read_bio_X509(cert_bio.get(), NULL, NULL, NULL);
    if (x509_cert == NULL)
        throw SSLException("PEM_read_bio_X509 failed");
    return std::shared_ptr<X509>(x509_cert, X509_free);
}

static std::shared_ptr<EVP_PKEY> NewEVP_PKEY(const PrivateKey& pkey)
{
    auto pkey_bio = NewStrBIO(pkey.str());
    EVP_PKEY* evp_pkey = ::PEM_read_bio_PrivateKey(pkey_bio.get(), NULL, NULL, NULL);
    if (evp_pkey == NULL)
        throw SSLException("PEM_read_bio_PrivateKey failed");
    return std::shared_ptr<EVP_PKEY>(evp_pkey, EVP_PKEY_free);
}

static void UseCertAndKey(SSL_CTX* ctx, X509* cert, EVP_PKEY* pkey)
{
    if (!X509_check_private_key(cert, pkey))
        throw SSLException("private key mismatch cert certificate");

    if (SSL_CTX_use_certificate(ctx, cert) != 1)
        throw SSLException("SSL_CTX_use_certificate failed");

    if (SSL_CTX_use_PrivateKey(ctx, pkey) != 1)
        throw SSLException("SSL_CTX_use_PrivateKey failed");
}

static void UseCACerts(SSL_CTX* ctx, const std::vector<X509*>& certs)
{
    X509_STORE* x509_store = X509_STORE_new();
    if (x509_store == NULL)
        throw SSLException("X509_STORE_new failed");

    for (X509* cert:  certs) {
        if (!X509_STORE_add_cert(x509_store, cert))
            throw SSLException("X509_STORE_add_cert failed");
    }

    SSL_CTX_set_cert_store(ctx, x509_store);
}

/*******************************************************************************
 * Чтение CA сертификатов
 ******************************************************************************/

static std::vector<Certificate> ReadCACerts(const Domain& domain)
{
    std::vector<Certificate> result;
    std::string cert;
    OciCpp::CursCtl curs = make_curs("SELECT cert FROM httpca WHERE domain = :domain OR domain IS NULL");
    curs.stb()
        .def(cert)
        .bind(":domain", domain.str());
    curs.exec();
    while (!curs.fen())
        result.push_back(Certificate(cert));
    return result;
}

/*******************************************************************************
 * Поддержка Transfer-Encoding: chunked
 ******************************************************************************/

/* Возвращает смещение CRLF */
static boost::optional<size_t> PeekCRLF(boost::asio::streambuf& sbuf)
{
    typedef boost::asio::buffers_iterator<boost::asio::streambuf::const_buffers_type> BufIt_t;
    const boost::asio::streambuf::const_buffers_type buffers = sbuf.data();
    const BufIt_t begin = BufIt_t::begin(buffers), end = BufIt_t::end(buffers);
    for (BufIt_t it = begin; it != end; ++it)
        if (*it == '\r' && (it + 1) != end && *(it + 1) == '\n')
            return std::distance(begin, it);
    return boost::none;
}

static boost::optional<size_t> ParseChunkHeader(const std::string& header)
{
    static const boost::regex re("([0-9a-zA-Z]+).*\r?\n");
    boost::smatch what;
    if (!boost::regex_match(header, what, re))
        return boost::none;
    return std::strtol(what[1].str().c_str(), NULL, 16);
}

static bool ConsumeCRLF(boost::asio::streambuf& sbuf)
{
    std::istream in(&sbuf);
    char c = 0;
    return in.get(c) && c == '\r' && in.get(c) && c == '\n';
}

/*******************************************************************************
 * IHttpConn
 ******************************************************************************/

struct IHttpConn;

typedef std::shared_ptr<IHttpConn> HttpConnPtr_t;
typedef std::function<void (HttpConnPtr_t conn)> FinalCallback_t;

struct IHttpConn: private comtech::noncopyable {
    enum State { BUSY, DONE, ERROR };
    virtual ~IHttpConn() {}
    virtual void startDisposeTimer(const std::size_t seconds) = 0;
    virtual void markForDispose() = 0;
    virtual bool markedForDispose() const = 0;
    virtual const HttpReq& req() = 0;
    virtual void setFinalCallback(FinalCallback_t finalCallback) = 0;
    virtual State state() const = 0;
    virtual boost::optional<HttpResp> getResponse() const = 0;
    virtual bool responseWasSent() const = 0;
    virtual void responseWasSent(const bool value) = 0;
};

/*******************************************************************************
 * Connection wrapper with multiple retries on error
 ******************************************************************************/

typedef std::function<HttpConnPtr_t (
        boost::asio::io_service& io,
        const boost::posix_time::ptime& deadline,
        FinalCallback_t&& finalCallback)>
    CreateConnectionWithDeadline_t;

class ConnWithRetries: public std::enable_shared_from_this<ConnWithRetries>, public IHttpConn {
public:
    static std::shared_ptr<ConnWithRetries> create(
            boost::asio::io_service& io,
            CreateConnectionWithDeadline_t&& createConn,
            const std::vector<boost::posix_time::ptime>& deadlines,
            FinalCallback_t&& finalCallback)
    {
        std::shared_ptr<ConnWithRetries> conn(new ConnWithRetries(
                    io, std::move(createConn), deadlines, std::move(finalCallback)));
        conn->init();
        return conn;
    }

    virtual void startDisposeTimer(const std::size_t seconds)
    {
        currentConn_->startDisposeTimer(seconds);
    }

    virtual void markForDispose()
    {
        currentConn_->markForDispose();
        markedForDispose_ = true;
    }

    virtual bool markedForDispose() const
    {
        return markedForDispose_ || (isAllRetriesUsed() && currentConn_->markedForDispose());
    }

    virtual bool responseWasSent() const
    {
        return responseWasSent_;
    }

    virtual void responseWasSent(const bool value)
    {
        responseWasSent_ = value;
    }

    virtual void setFinalCallback(FinalCallback_t finalCallback)
    {
        if (not finalCallback) {
            return;
        }

        finalCallback_ = std::move(finalCallback);
        if (state() != BUSY) {
            LogTrace(TRACE5) << __FUNCTION__ << ": setting final callback while in final state";
            callFinalCallback();
        }
    }

    virtual const HttpReq& req()
    {
        return currentConn_->req();
    }

    virtual State state() const
    {
        switch (currentConn_->state()) {
            case BUSY:
                return BUSY;

            case DONE:
                return DONE;

            case ERROR:
                return isAllRetriesUsed() ? ERROR : BUSY;
        }
        LogError(STDLOG) << __FUNCTION__ << ": unexpected connection state = " << currentConn_->state();
        assert(0);
    }

    virtual boost::optional<HttpResp> getResponse() const
    {
        /* Внимание - switch по собственному state(), а не по currentConn_->state() */
        switch (state()) {
            case BUSY:
                return boost::optional<HttpResp>();

            case DONE:
            case ERROR:
                return currentConn_->getResponse();
        }
        LogError(STDLOG) << __FUNCTION__ << ": unexpected connection state = " << state();
        assert(0);
    }
private:
    ConnWithRetries(
            boost::asio::io_service& io,
            CreateConnectionWithDeadline_t&& createConn,
            const std::vector<boost::posix_time::ptime>& deadlines,
            FinalCallback_t&& finalCallback
            ):
        io_(io),
        createConn_(std::move(createConn)),
        deadlines_(deadlines),
        finalCallback_(std::move(finalCallback)),
        markedForDispose_(false),
        numAttempts_(0)
    {
    }

    bool isAllRetriesUsed() const
    {
        return deadlines_.empty();
    }

    void init()
    {
        nextAttempt();
    }

    void inFinalState()
    {
        if (finalCallback_)
            callFinalCallback();
    }

    void nextAttempt()
    {
        ASSERT(!isAllRetriesUsed());
        LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": creating new sub-connection with deadline = " << deadlines_.at(0);

        if (currentConn_)
            currentConn_->markForDispose();
        currentConn_ = createConn_(io_, deadlines_.at(0), std::bind(&ConnWithRetries::currentConnectionComplete, shared_from_this(), std::placeholders::_1));
        deadlines_.erase(deadlines_.begin());

        ++numAttempts_;
        if (numAttempts_ > 1)
            ++_Stat.nRetries;
    }

    void callFinalCallback()
    {
        io_.post(std::bind(finalCallback_, shared_from_this()));
        finalCallback_ = nullptr;
    }

    void currentConnectionComplete(HttpConnPtr_t conn)
    {
        LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": sub-conn = " << conn << ", sub-conn state = " << conn->state();
        if (!isAllRetriesUsed() &&
            conn->state() == ERROR &&
            conn->getResponse()->commErr->code == COMMERR_TIMEOUT)
        {
            nextAttempt();
        }
        else {
            if (conn->state() == DONE && numAttempts_ > 1)
                ++_Stat.nRetriesOK;
            deadlines_.clear();
            inFinalState();
        }
    }

    boost::asio::io_service& io_;
    CreateConnectionWithDeadline_t createConn_;
    std::vector<boost::posix_time::ptime> deadlines_;
    FinalCallback_t finalCallback_; /* Вызывается при завершении работы */
    bool markedForDispose_;
    std::shared_ptr<IHttpConn> currentConn_;
    size_t numAttempts_;
    bool responseWasSent_ {false};
};



/*******************************************************************************
 * HttpsConn
 ******************************************************************************/

static std::string FirstLine(const std::string& text)
{
    /* line_end_pos may be std::string::npos, but it's OK for substr */
    const size_t line_end_pos = std::min(
            text.find('\r'),
            text.find('\n'));
    return text.substr(0, line_end_pos);
}

/* Для LogTrace/LogError */
static std::string LogReq(const HttpReq& req)
{
    std::ostringstream out;
    out << req.hostAndPort.host << ':' << req.hostAndPort.port
        << ' '
        << FirstLine(req.text)
        << ' '
        << req.correlation
        << ' '
        << req.time
        << ' '
        << req.domain;
    return out.str();
}

static boost::posix_time::milliseconds getConnectionTimeout(const Domain &domain)
{
  static int default_TO=-1;
  if(default_TO==-1) // not initialized yet
    default_TO=readIntFromTcl("CONN_TIMEOUT_DEFAULT",500);
  std::string dStr=domain.str();
  std::string::size_type pos=dStr.find("/");
  if(pos!=std::string::npos)
    dStr.resize(pos);

  return boost::posix_time::milliseconds(readIntFromTcl("CONN_TIMEOUT_"+dStr,default_TO));
}

/* Отправка запроса и получение ответа */
class HttpsConn: public std::enable_shared_from_this<HttpsConn>, public IHttpConn {
public:
    typedef sirena_net::stream_holder stream_holder;
    static std::shared_ptr<HttpsConn> create(
            boost::asio::io_service& io,
            const HttpReq& req,
            const boost::posix_time::ptime& deadline,
            FinalCallback_t&& finalCallback)
    {
        std::shared_ptr<HttpsConn> conn(new HttpsConn(io, req, deadline, std::move(finalCallback)));
        conn->init();
        return conn;
    }

    ~HttpsConn()
    {
        --_Stat.activeConnections;
        LogTrace(TRACE5) << __FUNCTION__ << ": dropping connection: " << this;
    }

    virtual void startDisposeTimer(const std::size_t seconds)
    {
        disposeTimer_.expires_from_now(boost::posix_time::seconds(seconds));
        const auto self = shared_from_this();
        const auto callback =
            [this, self](const auto& err) {
                this->onDisposeTimeout(err);
            };
        disposeTimer_.async_wait(callback);
    }

    virtual void markForDispose()
    {
        LogTrace(TRACE5) << __FUNCTION__ << ": marked for dispose: " << this;
        markedForDispose_ = true;
        disposeTimer_.cancel();
    }

    virtual bool markedForDispose() const
    {
        return markedForDispose_;
    }

    virtual bool responseWasSent() const
    {
        return responseWasSent_;
    }

    virtual void responseWasSent(const bool value)
    {
        responseWasSent_ = value;
    }

    virtual void setFinalCallback(FinalCallback_t finalCallback)
    {
        if (not finalCallback) {
            return;
        }

        finalCallback_ = std::move(finalCallback);
        if (state_ != BUSY) {
            callFinalCallback();
        }
    }

    virtual const HttpReq& req()
    {
        return req_;
    }

    virtual State state() const
    {
        return state_;
    }

    virtual boost::optional<HttpResp> getResponse() const
    {
        std::string httpResponse;
        switch (state_) {
            case BUSY:
                return boost::optional<HttpResp>();

            case DONE:
                if (isAnswerGZipped(resp_.headers)) {
                    httpResponse = resp_.headers;
#ifdef WITH_ZLIB
                    std::vector<uint8_t> res;
                    if(int err = Zlib::decompress(std::vector<uint8_t>(resp_.content.begin(), resp_.content.end()), res, Zlib::GZip))
                        throw ServerFramework::Exception(STDLOG, __FUNCTION__, "Zlib::decompress failed with "+std::to_string(err));
                    httpResponse.append(res.begin(), res.end());
#endif
                }
                else
                    httpResponse = resp_.headers + resp_.content;
                break;

            case ERROR:
                httpResponse = MakeErrorHttpResponse(*commErr_);
                break;
        }
        LogTrace(TRACE5) << __FUNCTION__ << ": response = " << httpResponse;

        return HttpResp(
                boost::posix_time::microsec_clock::local_time(),
                httpResponse,
                commErr_,
                req_);
    }
private:
    HttpsConn(boost::asio::io_service& io, const HttpReq& req, const boost::posix_time::ptime& deadline, FinalCallback_t&& finalCallback):
        io_(io),
        finalCallback_(std::move(finalCallback)),
        state_(BUSY),
        req_(req),
        deadline_(deadline),
        timer_(io),
        connectTimer_(io),
        disposeTimer_(io),
        resolver_(io),
        sslContext_(boost::asio::ssl::context::sslv23),
        markedForDispose_(false)
    {}

    void init()
    {
        ++_Stat.totalConnections;
        ++_Stat.activeConnections;

        LogTrace(TRACE1) << __FUNCTION__ << ": new connection: " << this << " " << req_.correlation << "  domain: " << req_.domain;
        const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        LogTrace(TRACE5) << __FUNCTION__ << ": now = " << now << ", deadline = " << deadline_;

        timer_.expires_from_now(deadline_ - now);
        timer_.async_wait(std::bind(&HttpsConn::onTimeout, shared_from_this(), std::placeholders::_1 /*error*/));
        connectTimer_.expires_at(boost::posix_time::pos_infin);

        try {
            if (req_.useSSL) {
                sslContext_.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2);
                SSL_CTX* ctx = sslContext_.native_handle();

                const std::vector<Certificate> caCerts = ReadCACerts(req_.domain);
                if (not caCerts.empty()) {
                    LogTrace(TRACE5) << __FUNCTION__ << ": loaded " << caCerts.size() << " CA certificates";

                    std::vector<X509*> caPtrs;
                    for (const Certificate& cert:  caCerts)
                    {
                        std::shared_ptr<X509> x509_cert = NewX509(cert);
                        auth_.caCerts.push_back(x509_cert);
                        caPtrs.push_back(x509_cert.get());
                    }

                    UseCACerts(ctx, caPtrs);
                    sslContext_.set_verify_mode(boost::asio::ssl::context_base::verify_peer);
                }

                if (req_.clientAuth) {
                    LogTrace(TRACE5) << __FUNCTION__ << ": using client auth";
                    auth_.clientCert = NewX509(req_.clientAuth->cert);
                    auth_.clientPkey = NewEVP_PKEY(req_.clientAuth->pkey);
                    UseCertAndKey(ctx, auth_.clientCert.get(), auth_.clientPkey.get());

                }
                stream_ = stream_holder::create_stream(io_, &sslContext_);
            }
            else
                stream_ = stream_holder::create_stream(io_);

            resolver_.async_resolve(
                    MakeResolverQuery(req_.hostAndPort),
                    std::bind(
                        &HttpsConn::onResolveComplete,
                        shared_from_this(),
                        std::placeholders::_1 /*error*/,
                        std::placeholders::_2 /*iterator*/));
        } catch (const SSLException& e) {
            LogError(STDLOG) << __FUNCTION__ << ": SSL exception: " << e.what() << ": " << LogReq(req_);
            setErrorState(COMMERR_SSL, std::string("SSL error: ") + e.what());
        }
    }

    void inFinalState()
    {
        LogTrace(TRACE1) << __FUNCTION__ << ": state = " << state_ << ": " << LogReq(req_);

        assert(state_ != BUSY);
        timer_.cancel();
        if (stream_ && stream_.lowest_layer().is_open()) {
            LogTrace(TRACE5) << __FUNCTION__ << ": cancelling I/O on the socket";
            stream_.lowest_layer().cancel();
        }
        resolver_.cancel();

        if (finalCallback_) {
            callFinalCallback();
        }
    }



    void callFinalCallback()
    {
        io_.post(std::bind(finalCallback_, shared_from_this()));
        finalCallback_ = NULL;
    }

    void setErrorState(CommErrCode errCode, const std::string& errMsg)
    {
        /* Без этого условия в случае тайм-аута можем упасть */
        if (state_ != ERROR) {
            ++_Stat.totalConnErr;
            LogTrace(TRACE1) << __FUNCTION__ << ": " << errMsg;
            state_ = ERROR;
            commErr_ = CommErr(errCode, errMsg);
            inFinalState();
        } else
            LogTrace(TRACE1) << __FUNCTION__ << ": " << errMsg << " in the error state = " << commErr_->errMsg;
    }

    void setErrorState(CommErrCode errCode, const boost::system::error_code& err)
    {
        setErrorState(errCode, err.message());
    }

    void setDoneState()
    {
        state_ = DONE;
        inFinalState();
    }

    void onTimeout(const boost::system::error_code& err)
    {
        if (!err) {
            if (state_ != ERROR)
                ++_Stat.totalConnErrTimeout;

            if(req().domain == Domain("AEZH_SOAP") or req().domain == Domain("AEZH_GET_SCHEDULE")) {
                LogTrace(TRACE1) << __FUNCTION__ << ": " << this << ": " << err.message();
                setErrorState(COMMERR_TIMEOUT,
                              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                              "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                                "<SOAP-ENV:Body>"
                                  "<SOAP-ENV:Fault>"
                                    "<faultcode>http_io_exception</faultcode>"
                                    "<faultstring>Connection timeout</faultstring>"
                                  "</SOAP-ENV:Fault>"
                                "</SOAP-ENV:Body>"
                              "</SOAP-ENV:Envelope>");
            } else {
                LogError(STDLOG) << __FUNCTION__ << ": " << this << ": timeout: " << LogReq(req_);
                setErrorState(COMMERR_TIMEOUT, "timeout");
            }
        }
    }

    void onDisposeTimeout(const boost::system::error_code& err)
    {
        if (!err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": dispose timeout: " << LogReq(req_);
            markForDispose();
        }
    }

    void onResolveComplete(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint)
    {
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message() << ": " << LogReq(req_);
            setErrorState(COMMERR_RESOLVE, err);
        } else if (state_ == BUSY) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": OK";
            connectTimer_.expires_from_now(getConnectionTimeout(req().domain));
            connectTimer_.async_wait(std::bind(&HttpsConn::onConnectExpiredOrError,shared_from_this(),endpoint, std::placeholders::_1 /*error*/));
            stream_.lowest_layer().async_connect(
                    *endpoint,
                    std::bind(&HttpsConn::onConnectComplete, shared_from_this(), endpoint, std::placeholders::_1 /*error*/));
        } else
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": state changed to " << state_;
    }

    void onConnectExpiredOrError(boost::asio::ip::tcp::resolver::iterator endpoint, const boost::system::error_code& err)
    {
      LogTrace(TRACE1)<< __FUNCTION__ << ": " << this << ": " << "err: "<<(err?err.message():"no err - timeout by connectionTimer");
      if(err.value()==ECANCELED) // timer async_wait() canceled by .cancel() or setting new expiration value
        return;
      if(err)
      {
        LogTrace(TRACE1)<< __FUNCTION__ << ": " << this << ": " << err.message() << ": " << LogReq(req_);
        setErrorState(COMMERR_CONNECT,err);
      }
      if (state_ == BUSY) {
        if(connectTimer_.expires_at()>boost::asio::deadline_timer::traits_type::now())
          connectTimer_.cancel();
        boost::system::error_code ignored_err;
        stream_.lowest_layer().close(ignored_err);
        ++endpoint;
        if(endpoint == boost::asio::ip::tcp::resolver::iterator())
        {
          LogTrace(TRACE1) << __FUNCTION__ << ": " << this << ": all connection attempts failed";
          setErrorState(COMMERR_CONNECT, err?err.message():"CONNECTION TIMEOUT");
          return;
          // call onConnectComplete() instead of return because it may generate custom err message for AEZH domain
          //return onConnectComplete(endpoint,err?err:boost::system::error_code(boost::system::errc::timed_out, boost::system::system_category()));
        }
        LogTrace(TRACE5)<<__FUNCTION__ << ": " << this << ": try next connection endpoint";
        connectTimer_.expires_from_now(getConnectionTimeout(req().domain));
        connectTimer_.async_wait(std::bind(&HttpsConn::onConnectExpiredOrError,shared_from_this(),endpoint, std::placeholders::_1 /*error*/));
        stream_.lowest_layer().async_connect(
                  *endpoint,
                    std::bind(&HttpsConn::onConnectComplete, shared_from_this(), endpoint, std::placeholders::_1 /*error*/));
      } else
          LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": state changed to " << state_;
    }

    void onConnectComplete(boost::asio::ip::tcp::resolver::iterator endpoint, const boost::system::error_code& err)
    {
        LogTrace(TRACE1)<< __FUNCTION__ << ": " << this << ": " << "err: "<<(err?err.message():"no err");
        connectTimer_.expires_at(boost::posix_time::pos_infin);
        if (err) {
            if(endpoint != boost::asio::ip::tcp::resolver::iterator() && err.value()!=ECANCELED) {
              return onConnectExpiredOrError(endpoint,err);
            }
            if(req().domain == Domain("AEZH_SOAP") or req().domain == Domain("AEZH_GET_SCHEDULE")) {
                LogTrace(TRACE1) << __FUNCTION__ << ": " << this << ": " << err.message();
                setErrorState(COMMERR_CONNECT,
                              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                              "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                                "<SOAP-ENV:Body>"
                                  "<SOAP-ENV:Fault>"
                                    "<faultcode>http_io_exception</faultcode>"
                                    "<faultstring>" + err.message() + "</faultstring>"
                                  "</SOAP-ENV:Fault>"
                                "</SOAP-ENV:Body>"
                              "</SOAP-ENV:Envelope>");
            } else {
                LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message() << ": " << LogReq(req_);
                setErrorState(COMMERR_CONNECT, err);
            }
        } else if (state_ == BUSY) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": OK";
            stream_.async_handshake(
                    stream_holder::client,
                    std::bind(&HttpsConn::onHandshakeComplete, shared_from_this(), std::placeholders::_1 /*error*/));
        } else
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": state changed to " << state_;
        connectTimer_.cancel();
    }

    void onHandshakeComplete(const boost::system::error_code& err)
    {
        if (err) {
            if (state_ != ERROR)
                ++_Stat.totalConnErrHandshake;

            if(req().domain == Domain("AEZH_SOAP")) {
                LogTrace(TRACE1) << __FUNCTION__ << ": " << this << ": " << err.message();
                setErrorState(COMMERR_HANDSHAKE,
                              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                              "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                                "<SOAP-ENV:Body>"
                                  "<SOAP-ENV:Fault>"
                                    "<faultcode>http_io_exception</faultcode>"
                                    "<faultstring>handshake : " + err.message() + "</faultstring>"
                                  "</SOAP-ENV:Fault>"
                                "</SOAP-ENV:Body>"
                              "</SOAP-ENV:Envelope>");
            } else {
                LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message() << ": " << LogReq(req_);
                setErrorState(COMMERR_HANDSHAKE, err);
            }
        } else if (state_ == BUSY) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": OK";
            if (req_.customAuth == CustomAuth::NTLM) {
                httpsrv::ntlm::authenticate(
                            stream_,
                            std::bind(&HttpsConn::onAuthenticateComplete, shared_from_this(), std::placeholders::_1 /*error*/));
            } else {
                boost::asio::async_write(
                            stream_,
                            boost::asio::buffer(req_.text),
                            std::bind(&HttpsConn::onWriteComplete, shared_from_this(), std::placeholders::_1 /*error*/));
            }
        } else
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": state changed to " << state_;
    }

    void onAuthenticateComplete(const boost::system::error_code& err)
    {
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message() << ": " << LogReq(req_);
            setErrorState(COMMERR_AUTHENTICATE, err);
        } else if (state_ == BUSY) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": OK";
            boost::asio::async_write(
                    stream_,
                    boost::asio::buffer(req_.text),
                    std::bind(&HttpsConn::onWriteComplete, shared_from_this(), std::placeholders::_1 /*error*/));
        } else
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": state changed to " << state_;
    }

    void onWriteComplete(const boost::system::error_code& err)
    {
        static const boost::regex end_of_header_re("\r?\n\r?\n");
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message() << ": " << LogReq(req_);
            setErrorState(COMMERR_WRITE, err);
        } else if (state_ == BUSY) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": OK";
            boost::asio::async_read_until(
                    stream_,
                    sbuf_,
                    end_of_header_re, /* Конец HTTP заголовков */
                    std::bind(
                        &HttpsConn::onReadHeadersComplete,
                        shared_from_this(),
                        std::placeholders::_1 /*error*/,
                        std::placeholders::_2 /*bytes_transferred*/));
        } else
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": state changed to " << state_;
    }

    void readChunkHeader()
    {
        const boost::optional<size_t> crlfOffset = PeekCRLF(sbuf_);
        if (crlfOffset) {
            onReadChunkHeader(boost::system::error_code(), *crlfOffset + 2);
        } else {
            static const boost::regex end_of_line_re("\r?\n");
            boost::asio::async_read_until(
                    stream_,
                    sbuf_,
                    end_of_line_re,
                    std::bind(
                        &HttpsConn::onReadChunkHeader,
                        shared_from_this(),
                        std::placeholders::_1 /*error*/,
                        std::placeholders::_2 /*bytes_transferred*/));
        }
    }

    void readChunkData(size_t chunkSize)
    {
        LogTrace(TRACE5) << __FUNCTION__ << ": chunkSize = " << chunkSize;

        const size_t chunkPlusCRLF = chunkSize + 2;
        /* Сколько байт не хватает в буфере? */
        const size_t transferAtLeast = sbuf_.size() > chunkPlusCRLF ?
            0 :
            chunkPlusCRLF - sbuf_.size();

        boost::asio::async_read(
                stream_,
                sbuf_,
                boost::asio::transfer_at_least(transferAtLeast),
                std::bind(
                    &HttpsConn::onReadChunkData,
                    shared_from_this(),
                    std::placeholders::_1 /*error*/,
                    std::placeholders::_2 /*bytes_transferred*/,
                    chunkSize));
    }

    void readUntilEof(const boost::system::error_code& err)
    {
        if (!err)
        {
            boost::asio::async_read(
                    stream_,
                    sbuf_,
                    boost::asio::transfer_at_least(1),
                    std::bind(
                        &HttpsConn::readUntilEof,
                        shared_from_this(),
                        std::placeholders::_1 /*error*/));
        }
        else if (err == boost::asio::error::eof or
                (err.category() == boost::asio::error::get_ssl_category() and
#if defined SSL_R_SHORT_READ
                 err.value() == ERR_PACK(ERR_LIB_SSL, 0, SSL_R_SHORT_READ)
#else
                 ERR_GET_REASON(err.value())==boost::asio::ssl::error::stream_truncated
#endif
                 ))
        {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ", avail = " << sbuf_.size();
            resp_.content = GetRead();
            setDoneState();
        }
        else
        {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message() << ": " << LogReq(req_);
            setErrorState(COMMERR_READ_CONTENT, err);
        }
    }

    void onReadHeadersComplete(const boost::system::error_code& err, size_t headersLen)
    {
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message() << ": " << LogReq(req_);
            setErrorState(COMMERR_READ_HEADERS, err);
        } else if (state_ == BUSY) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": OK";
            resp_.headers = GetRead(headersLen);

            if (ChunkedTransferEncoding(resp_.headers)) {
                LogTrace(TRACE5) << __FUNCTION__ << ": chunked transfer encoding";
                readChunkHeader();
            } else if (const boost::optional<size_t> contentLen = ParseContentLength(resp_.headers)) {
                /* Сколько байт осталось дочитать */
                const size_t rest = sbuf_.size() < *contentLen ?
                    *contentLen - sbuf_.size() :
                    0;

                LogTrace(TRACE5) << __FUNCTION__ << ": headersLen = " << headersLen
                    << ", contentLen = " << *contentLen
                    << ", avail = " << sbuf_.size()
                    << ", rest = " << rest;
                boost::asio::async_read(
                        stream_,
                        sbuf_,
                        boost::asio::transfer_at_least(rest),
                        std::bind(
                            &HttpsConn::onReadContentComplete,
                            shared_from_this(),
                            std::placeholders::_1 /*error*/,
                            std::placeholders::_2 /*bytes_transferred*/,
                            *contentLen));
            } else {
                boost::asio::async_read(
                        stream_,
                        sbuf_,
                        boost::asio::transfer_at_least(1),
                        std::bind(
                            &HttpsConn::readUntilEof,
                            shared_from_this(),
                            std::placeholders::_1 /*error*/));
            }
        } else
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": state changed to " << state_;
    }

    void onReadChunkHeader(const boost::system::error_code& err, size_t bytes_transferred)
    {
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message() << ": " << LogReq(req_);
            setErrorState(COMMERR_READ_CHUNK_HEADER, err);
        } else if (state_ == BUSY) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": bytes_transferred = " << bytes_transferred
                << ", avail = " << sbuf_.size();

            assert(sbuf_.size() >= bytes_transferred);
            const std::string chunkHeader = GetRead(bytes_transferred);

            const boost::optional<size_t> chunkSize = ParseChunkHeader(chunkHeader);
            if (!chunkSize)
                setErrorState(COMMERR_CHUNK_SIZE, "failed to read chunk size");
            else if (*chunkSize == 0)
                setDoneState();
            else
                readChunkData(*chunkSize);
        } else
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": state changed to " << state_;
    }

    void onReadChunkData(const boost::system::error_code& err, size_t bytes_transferred, size_t chunkSize)
    {
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message() << ": " << LogReq(req_);
            setErrorState(COMMERR_READ_CHUNK_DATA, err);
        } else if (state_ == BUSY) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": bytes_transferred = " << bytes_transferred
                << ", avail = " << sbuf_.size();
            assert(sbuf_.size() >= chunkSize + 2);
            AppendRead(resp_.content, chunkSize);

            if (!ConsumeCRLF(sbuf_))
                setErrorState(COMMERR_READ_CHUNK_DATA_CRLF, "no CRLF after chunk data");
            else
                readChunkHeader();
        } else
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": state changed to " << state_;
    }

    void onReadContentComplete(const boost::system::error_code& err, size_t bytes_transferred, size_t contentLen)
    {
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message() << ": " << LogReq(req_);
            setErrorState(COMMERR_READ_CONTENT, err);
        } else if (state_ == BUSY) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": bytes_transferred = " << bytes_transferred
                << ", avail = " << sbuf_.size();
            assert(sbuf_.size() >= contentLen);
            resp_.content = GetRead(contentLen);
            setDoneState();
        } else
            LogTrace(TRACE5) << __FUNCTION__ << ": " << this << ": state changed to " << state_;
    }

    //consumes from buffer, and appends to string
    void AppendRead(std::string& data, size_t len = std::string::npos)
    {
        if (len)
        {
            auto beg = boost::asio::buffers_begin(sbuf_.data());
            size_t consume_len = ((std::string::npos == len) ? sbuf_.size() : len);
            data.reserve(data.size() + consume_len);
            data.append(beg, beg + consume_len);
            sbuf_.consume(consume_len);
        }
    }

    //returns consumed string
    std::string GetRead(size_t len = std::string::npos)
    {
        if (len)
        {
            auto beg = boost::asio::buffers_begin(sbuf_.data());
            size_t consume_len = ((std::string::npos == len) ? sbuf_.size() : len);
            std::string data(beg, beg + consume_len);
            sbuf_.consume(consume_len);
            return data;
        }
        return std::string();
    }


    boost::asio::io_service& io_;
    FinalCallback_t finalCallback_; /* Вызывается при завершении работы */
    State state_;
    boost::optional<CommErr> commErr_; /* Содержит описание ошибки в случае state_ == ERROR */
    const HttpReq req_;
    boost::posix_time::ptime deadline_;
    boost::asio::deadline_timer timer_;
    boost::asio::deadline_timer connectTimer_;
    boost::asio::deadline_timer disposeTimer_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ssl::context sslContext_;
//    std::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket> > socket_;
    stream_holder stream_;
    boost::asio::streambuf sbuf_; /* Сюда читаем ответ сервера */
    struct {
        std::string headers;
        std::string content;
    } resp_;
    struct {
        std::shared_ptr<X509> clientCert;
        std::shared_ptr<EVP_PKEY> clientPkey;
        std::vector<std::shared_ptr<X509> > caCerts;
    } auth_;
    bool markedForDispose_;
    bool responseWasSent_ {false};
};

/*******************************************************************************
 * HttpConnPool
 ******************************************************************************/

void ShowStat(const Stat& stat)
{
    LogTrace(TRACE1) << __FUNCTION__ << ": " << stat;
}

static std::size_t getRequestsCount(
    const ReqCorrelationData& corr,
    const Domain& domain,
    const std::list<HttpConnPtr_t>& pool) noexcept;

static bool isRequestReady(const HttpConnPtr_t& conn) noexcept;

static std::size_t getReadyRequestsCount(
    const ReqCorrelationData& corr,
    const Domain& domain,
    const bool waitAll,
    const std::list<HttpConnPtr_t>& pool) noexcept;

static bool isRequestsReady(
    const ReqCorrelationData& corr,
    const Domain& domain,
    const bool waitAll,
    const std::size_t groupSize,
    const std::list<HttpConnPtr_t>& pool) noexcept;

static bool HaveRequestsPerespros(
    const HttpConnPtr_t& current,
    const std::list<HttpConnPtr_t>& pool) noexcept;

static HttpConnPtr_t findMatchedConnection(
    const ReqCorrelationData& corr,
    const Domain& domain,
    const bool waitAll,
    const std::list<HttpConnPtr_t>& pool) noexcept;

static void disposeConnections(
    const ReqCorrelationData& corr,
    const Domain& domain,
    const std::list<HttpConnPtr_t>& pool,
    const HttpConnPtr_t& current) noexcept;

/*
 * Пул соединений со шлюзом.
 */
class HttpConnPool: private comtech::noncopyable {
public:
    HttpConnPool(boost::asio::io_service& io):
        io_(io),
        timer_(io, boost::posix_time::seconds(WATCH_INTERVAL))
    {
        timer_.async_wait(std::bind(&HttpConnPool::onTimer, this, std::placeholders::_1 /*error*/));
    }

    ~HttpConnPool()
    {
        ShowStat(_Stat);
    }

    void doRequest(const HttpReq& req)
    {
        ShowStat(_Stat);
        HttpConnPtr_t conn = createNewConnection(req);
        if (!conn) {
            LogError(STDLOG) << __FUNCTION__ << ": pool size reached limit (" << pool_.size() << ")";
            return;
        }
    }

    void dispose()
    {
        std::list<HttpConnPtr_t>::iterator it = pool_.begin();
        while (it != pool_.end()) {
            std::list<HttpConnPtr_t>::iterator next = it;
            ++next;
            if ((*it)->markedForDispose())
                pool_.erase(it);
            it = next;
        }
    }

    const std::list<HttpConnPtr_t>& get() const noexcept
    {
        return pool_;
    }

    std::list<HttpConnPtr_t>& get() noexcept
    {
        return pool_;
    }

private:
    static const std::size_t WATCH_INTERVAL = 10;

    void onTimer(const boost::system::error_code& err)
    {
        if (err)
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message();

        LogTrace(TRACE5) << __FUNCTION__ << ": timer event";
        ShowStat(_Stat);
        dispose();

        timer_.expires_from_now(boost::posix_time::seconds(WATCH_INTERVAL));
        timer_.async_wait(std::bind(&HttpConnPool::onTimer, this, std::placeholders::_1 /*error*/));
        monitor_idle();
    }

    FinalCallback_t connFinishCallback(const HttpReq& req)
    {
        return
            [pool = &pool_](const auto& conn)
            {
                if (isRequestsReady(
                        conn->req().correlation,
                        conn->req().domain,
                        false,
                        conn->req().groupSize,
                        *pool)) {

                    LogTrace(TRACE1) << __FUNCTION__ << ": request is ready";

                    disposeConnections(conn->req().correlation, conn->req().domain, *pool, conn);

                    if (not HaveRequestsPerespros(conn, *pool)) {
                        LogTrace(TRACE1) << __FUNCTION__;
                        return;
                    }

                    const auto& correlation = conn->req().correlation;

                    if (correlation.pult) {
                        LogTrace(TRACE1) << __FUNCTION__;
                        ServerFramework::EdiHelpManager::confirm_notify(correlation.pult->str().c_str());
                        Commit();
                    }
                    else if (correlation.MsgId) {
                        LogTrace(TRACE1) << __FUNCTION__;
                        ServerFramework::EdiHelpManager::confirm_notify(*correlation.MsgId);
                        Commit();
                    }
                    else {
                        LogError(STDLOG) << __FUNCTION__ << ": " << "Invalid Correlation Data ";
                    }
                }
                else {
                    conn->startDisposeTimer(120); // set group size = 100 and sent only 1 request
                    conn->setFinalCallback(nullptr);
                }
            };
    }

    HttpConnPtr_t createNewConnection(const HttpReq& req)
    {
        if (pool_.size() == MAX_CONN_POOL_SIZE) {
            return HttpConnPtr_t();
        }

        const auto callback =
            std::bind(
                HttpsConn::create,
                std::placeholders::_1,
                req,
                std::placeholders::_2,
                std::placeholders::_3);

        pool_.push_back(
            ConnWithRetries::create(
                io_,
                callback,
                req.deadlines,
                connFinishCallback(req)));

        return pool_.back();
    }

    boost::asio::io_service& io_;
    boost::asio::deadline_timer timer_;
    std::list<HttpConnPtr_t> pool_;
};

static std::size_t getRequestsCount(
    const ReqCorrelationData& corr,
    const Domain& domain,
    const std::list<HttpConnPtr_t>& pool) noexcept
{
    std::size_t count = 0;

    for (const auto& conn : pool) {
        if (conn->req().domain == domain and conn->req().correlation == corr) {
            ++count;
        }
    }

    return count;
}

static bool isRequestReady(const HttpConnPtr_t& conn) noexcept
{
    return conn->state() != IHttpConn::BUSY;
}

static std::size_t getReadyRequestsCount(
    const ReqCorrelationData& corr,
    const Domain& domain,
    const bool waitAll,
    const std::list<HttpConnPtr_t>& pool) noexcept
{
    std::size_t count = 0;

    for (const auto& conn : pool) {
        if (conn->req().domain == domain and conn->req().correlation == corr) {
            if (waitAll or isRequestReady(conn)) {
                ++count;
            }
        }
    }

    return count;
}

static bool isRequestsReady(
    const ReqCorrelationData& corr,
    const Domain& domain,
    const bool waitAll,
    const std::size_t groupSize,
    const std::list<HttpConnPtr_t>& pool) noexcept
{
    const auto readyRequestsCount = getReadyRequestsCount(
        corr,
        domain,
        waitAll,
        pool);
    LogTrace(TRACE1) << __FUNCTION__ << ": readyRequestsCount = " << readyRequestsCount << ", groupSize = " << groupSize;
    return readyRequestsCount >= groupSize;
}

static bool HaveRequestsPerespros(
    const HttpConnPtr_t& current,
    const std::list<HttpConnPtr_t>& pool) noexcept
{
    if (not current->req().peresprosReq.empty()) {
        return true;
    }

    for (const auto& conn : pool) {
        if (isRequestReady(conn) and
            conn->req().domain == current->req().domain and
            conn->req().correlation == current->req().correlation and
            not conn->req().peresprosReq.empty()) {
            return true;
        }
    }

    return false;
}

static HttpConnPtr_t findMatchedConnection(
    const ReqCorrelationData& corr,
    const Domain& domain,
    const bool waitAll,
    const std::list<HttpConnPtr_t>& pool) noexcept
{
    for (const HttpConnPtr_t& conn : pool) {
        if (conn->req().domain == domain and conn->req().correlation == corr) {
            LogTrace(TRACE1) << __FUNCTION__ << ": matched " << domain << ", " << corr;
            if (waitAll or
                isRequestsReady(
                    conn->req().correlation,
                    conn->req().domain,
                    waitAll,
                    conn->req().groupSize,
                    pool)) {
                if (not conn->responseWasSent()) {
                    LogTrace(TRACE1) << __FUNCTION__ << ": matched not sent " << domain << ", " << corr;
                    return conn;
                }
            }
        }
    }

    return nullptr;
}

static void disposeConnections(
    const ReqCorrelationData& corr,
    const Domain& domain,
    const std::list<HttpConnPtr_t>& pool,
    const HttpConnPtr_t&) noexcept
{
    for (const HttpConnPtr_t& conn : pool) {
        if (conn->req().domain == domain and conn->req().correlation == corr) {
            LogTrace(TRACE1) << __FUNCTION__ << ": set for dispose " << domain << ", " << corr;
            conn->startDisposeTimer(30);
            conn->setFinalCallback(nullptr);
        }
    }
}

/*******************************************************************************
 * Обмен данными между obrzap и httpsrv (через UNIX domain socket)
 ******************************************************************************/

static std::string MakeSizeHeader(size_t size)
{
    std::ostringstream out;
    out << std::setw(9) << std::setfill('0') << size;
    LogTrace(TRACE1) << __FUNCTION__ << ": " << size;
    LogTrace(TRACE1) << __FUNCTION__ << ": " << out.str();
    return out.str();
}

static std::string AppendSizeHeader(const std::string& data)
{
    return MakeSizeHeader(data.size()) + data;
}

const std::string SerializeResp(const HttpResp& resp)
{
    LogTrace(TRACE1) << __FUNCTION__ << ": ";
    const HttpResp* respPtr = &resp;
    std::ostringstream data;
    OArchive_t ar(data);
    ar << respPtr;
    return data.str();
}

const std::string SerializeStat(const Stat& stat)
{
    std::ostringstream data;
    OArchive_t ar(data);
    ar << stat;
    return data.str();
}

typedef boost::asio::local::stream_protocol::socket LocalSocket_t;
typedef std::shared_ptr<LocalSocket_t> LocalSocketPtr_t;

static const std::string NO_REQS_FOUND("xFFFFFFFF");

class Session: public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::io_service& io, const LocalSocketPtr_t& socket, HttpConnPool& connPool):
        io_(io),
        socket_(socket),
        connPool_(connPool)
    {
        LogTrace(TRACE5) << __FUNCTION__ << ": " << this;
        assert(socket_);
        ++_Stat.totalSessions;
        ++_Stat.activeSessions;
    }

    ~Session()
    {
        LogTrace(TRACE5) << __FUNCTION__ << ": " << this;
        --_Stat.activeSessions;
    }

    void start()
    {
        assert(cmd_.empty());
        cmd_.resize(12);
        boost::asio::async_read(
                *socket_,
                boost::asio::buffer(cmd_),
                std::bind(
                    &Session::onReadCmd,
                    shared_from_this(),
                    std::placeholders::_1 /*error*/,
                    std::placeholders::_2 /*bytes_transferred*/));
    }
private:
    void onReadCmd(const boost::system::error_code& err, const std::size_t bytes_transferred)
    {
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message();
        } else {
            const auto dataSize = boost::lexical_cast<std::size_t>(std::string(cmd_.begin() + 3, cmd_.end()));
            LogTrace(TRACE5) << __FUNCTION__ << ": data size = " << dataSize;
            data_.resize(dataSize);

            boost::asio::async_read(
                    *socket_,
                    boost::asio::buffer(data_),
                    std::bind(
                        &Session::onReadData,
                        shared_from_this(),
                        std::placeholders::_1 /*error*/,
                        std::placeholders::_2 /*bytes_transferred*/));
        }
    }

    void onReadData(const boost::system::error_code& err, const std::size_t bytes_transferred)
    {
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message();
        } else {
            const std::string cmdCode(cmd_.begin(), cmd_.begin() + 3);
            if (cmdCode == "PUT")
                put();
            else if (cmdCode == "GET")
                get();
            else if (cmdCode == "STA")
                stat();
            else
                LogError(STDLOG) << __FUNCTION__ << ": unknown command code = " << cmdCode;
        }
    }

    void put()
    {
        const std::string data(data_.begin(), data_.end());
        std::istringstream in(data);
        IArchive_t ar(in);
        HttpReq* req = nullptr;
        ar >> req;
        std::unique_ptr<HttpReq> req_free(req);
        put_(*req);
    }

    void put_(const HttpReq& req)
    {
        connPool_.doRequest(req);
    }

    void get()
    {
        const std::string data(data_.begin(), data_.end());
        std::istringstream in(data);
        IArchive_t ar(in);
        std::string domain;
        bool waitAll = false;
        ReqCorrelationData_io corr;
        ar >> corr;
        ar >> domain;
        ar >> waitAll;
        get_(corr, Domain(domain), waitAll);
    }

    void setupDispose(
        const ReqCorrelationData& corr,
        const Domain& domain)
    {
        for (const HttpConnPtr_t& conn : connPool_.get()) {
            if (conn->req().domain == domain and conn->req().correlation == corr) {
                LogTrace(TRACE1) << __FUNCTION__ << ": " << domain << ", " << corr;
                conn->markForDispose();
                io_.post(std::bind(&HttpConnPool::dispose, &connPool_));
            }
        }
    }

    void setupConnectionCallback(
        const ReqCorrelationData& corr,
        const Domain& domain,
        const bool waitAll,
        const HttpConnPtr_t& conn)
    {
        LogTrace(TRACE1) << __FUNCTION__ << ": " << domain << ", " << corr;

        const auto self = shared_from_this();
        const auto callback =
            [this, self, corr, domain, waitAll](const auto& conn) {
                const auto resp = conn->getResponse();
                assert(resp);
                this->asyncWrite(SerializeResp(*resp), conn, waitAll);
            };
        conn->setFinalCallback(callback);
    }

    void get_(const ReqCorrelationData& corr, const Domain& domain, const bool waitAll)
    {
        if (getRequestsCount(corr, domain, connPool_.get()) == 0) {
            boost::asio::async_write(
                *socket_,
                boost::asio::buffer(NO_REQS_FOUND),
                [self = shared_from_this()](const auto& error, const size_t N) {
                    self->onWriteTrailer(error);
                });
            return;
        }

        const auto conn = findMatchedConnection(corr, domain, waitAll, connPool_.get());
        if (not conn) {
            writeTrailer();
            return;
        }

        setupConnectionCallback(corr, domain, waitAll, conn);
    }

    void stat()
    {
        asyncWrite(SerializeStat(_Stat),  nullptr, false);
    }

    void asyncWrite(
        const std::string& data,
        const HttpConnPtr_t& conn,
        const bool waitAll)
    {
        const std::shared_ptr<const std::string> data_ptr(new std::string(AppendSizeHeader(data)));
        const auto self = shared_from_this();
        const auto callback =
            [this, self, conn, data_ptr, waitAll](const boost::system::error_code& err, const std::size_t&) {
                onWrite(err, conn, waitAll);
            };
        boost::asio::async_write(*socket_, boost::asio::buffer(*data_ptr), callback);
    }

    //Making a call to async_write while another one is in progress is not allowed.
    //https://stackoverflow.com/questions/45813835/boostasio-ordering-of-data-sent-to-a-socket
    void onWrite(
        const boost::system::error_code& err,
        const HttpConnPtr_t& conn,
        const bool waitAll)
    {
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message();
            return;
        }

        if (not conn) {
            writeTrailer();
            return;
        }

        conn->responseWasSent(true);
        const auto newConn = findMatchedConnection(conn->req().correlation, conn->req().domain, waitAll, connPool_.get());
        if (not newConn) {
            setupDispose(conn->req().correlation, conn->req().domain);
            writeTrailer();
            return;
        }

        setupConnectionCallback(newConn->req().correlation, newConn->req().domain, waitAll, newConn);
    }

    void writeTrailer()
    {
        static const std::string trailer = MakeSizeHeader(0);
        boost::asio::async_write(
                *socket_,
                boost::asio::buffer(trailer),
                std::bind(
                    &Session::onWriteTrailer,
                    shared_from_this(),
                    std::placeholders::_1 /*error*/));
    }

    void onWriteTrailer(const boost::system::error_code& err)
    {
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message();
        } else {
            LogTrace(TRACE5) << __FUNCTION__ << ": OK";
        }
    }

    /* The socket used to communicate with the client. */
    boost::asio::io_service& io_;
    const LocalSocketPtr_t socket_;
    std::vector<char> cmd_;
    std::vector<char> data_;
    HttpConnPool& connPool_;
};

typedef std::shared_ptr<Session> SessionPtr_t;

/*******************************************************************************
 * Сервер, принимающий коннекты от obrzap-ов (на UNIX domain socket)
 ******************************************************************************/

class Server {
public:
    Server(boost::asio::io_service& io, HttpConnPool& connPool, const std::string& sockName):
        io_(io),
        acceptor_(io, boost::asio::local::stream_protocol::endpoint(sockName)),
        connPool_(connPool)
    {
        accept();
    }
private:
    void accept()
    {
        LogTrace(TRACE5) << __FUNCTION__ << ": accepting";
        const LocalSocketPtr_t socket(new LocalSocket_t(io_));
        acceptor_.async_accept(
                *socket,
                std::bind(&Server::onAccept, this, socket, std::placeholders::_1 /*error*/));
    }

    void onAccept(const LocalSocketPtr_t& socket, const boost::system::error_code& err)
    {
        if (err) {
            LogError(STDLOG) << __FUNCTION__ << ": " << this << ": " << err.message();
        } else {
            LogTrace(TRACE5) << __FUNCTION__ << ": accepted connection";
            monitor_idle_zapr(1);
            const SessionPtr_t newSession(new Session(io_, socket, connPool_));
            newSession->start();
            accept();
        }
    }

    boost::asio::io_service& io_;
    boost::asio::local::stream_protocol::acceptor acceptor_;
    HttpConnPool& connPool_;
};

/*******************************************************************************
 * Main logic
 ******************************************************************************/

static std::string GetSignalSockName()
{
    return get_signalsock_name(getTclInterpretator(), Tcl_NewStringObj(HTTPSRV_CMD_TCLVAR, -1), 0,
#ifdef XP_TESTING
            TestCases::GetJobIdx()
#else
            0
#endif// XP_TESTING
            );
}

static void Run(const boost::optional<int>& supervisorSocket)
{
    boost::asio::io_service& io = ServerFramework::system_ios::Instance();
    HttpConnPool connPool(io);
    std::unique_ptr<ServerFramework::ControlPipeEvent> control;
    const std::string sockName = GetSignalSockName();

    LogTrace(TRACE1) << __FUNCTION__ << ": sockName = " << sockName;
    unlink(sockName.c_str());
    if (supervisorSocket) {
        control.reset(new ServerFramework::ControlPipeEvent());
        control->init();
    }
    Server server(io, connPool, sockName);
    ServerFramework::Run();
}

/*******************************************************************************
 * Внешний интерфейс
 ******************************************************************************/

static boost::posix_time::time_duration DefaultPeresprosTimeout(
        const boost::posix_time::time_duration& timeout,
        unsigned maxTryCount)
{
    return (timeout * maxTryCount) + boost::posix_time::seconds(10);
}

//Q: Why are parameters passed by value?
//A: Please read 'Effective Modern C++' by S.Meyers item 41
DoHttpRequest::DoHttpRequest(
        Pult pult,
        Domain domain,
        HostAndPort hostAndPort,
        std::string reqText): DoHttpRequest(
            ReqCorrelationData(std::move(pult)),
            std::move(domain),
            std::move(hostAndPort),
            std::move(reqText)
            )
{
}


DoHttpRequest::DoHttpRequest(std::string const& host, unsigned port, std::string reqText)
    : DoHttpRequest(ServerFramework::getQueryRunner().getEdiHelpManager().msgId(),
                    Domain("default"), HostAndPort(host, port), std::move(reqText))
{
}


DoHttpRequest::DoHttpRequest(Domain domain, HostAndPort hostAndPort, std::string reqText)
    : DoHttpRequest(ServerFramework::getQueryRunner().getEdiHelpManager().msgId(),
                    std::move(domain), std::move(hostAndPort), std::move(reqText))
{
}


DoHttpRequest::DoHttpRequest(
        ServerFramework::InternalMsgId MsgId,
        Domain domain,
        HostAndPort hostAndPort,
        std::string reqText): DoHttpRequest(
            ReqCorrelationData(std::move(MsgId)),
            std::move(domain),
            std::move(hostAndPort),
            std::move(reqText)
            )
{
}


DoHttpRequest::DoHttpRequest(
        CorrelationID cid,
        Domain domain,
        HostAndPort hostAndPort,
        std::string reqText): DoHttpRequest(
            ReqCorrelationData(std::move(cid)),
            std::move(domain),
            std::move(hostAndPort),
            std::move(reqText)
            )
{
}

DoHttpRequest::DoHttpRequest(
        ReqCorrelationData&& corr,
        Domain&& domain,
        HostAndPort&& hostAndPort,
        std::string&& reqText):
    correlation_(std::move(corr)),
    domain_(std::move(domain)),
    hostAndPort_(std::move(hostAndPort)),
    reqText_(std::move(reqText)),
    customAuth_(CustomAuth::None),
    timeout_(boost::posix_time::seconds(15)),
    maxTryCount_(1),
    useSSL_(true),
    isDeffered_(false),
    recId_(0)
{
}


DoHttpRequest& DoHttpRequest::setCustomData(const CustomData& customData)
{
    customData_ = customData;
    return *this;
}

DoHttpRequest& DoHttpRequest::setCustomAuth(const CustomAuth& customAuth)
{
    customAuth_ = customAuth;
    return *this;
}

DoHttpRequest& DoHttpRequest::setClientAuth(const ClientAuth& clientAuth)
{
    clientAuth_ = clientAuth;
    useSSL_ = UseSSLFlag(true);
    return *this;
}

DoHttpRequest& DoHttpRequest::setMaxTryCount(unsigned maxTryCount)
{
    ASSERT(maxTryCount > 0);
    maxTryCount_ = maxTryCount;
    return *this;
}

DoHttpRequest& DoHttpRequest::setTimeout(const boost::posix_time::time_duration& timeout)
{
    timeout_ = timeout;
    return *this;
}

DoHttpRequest& DoHttpRequest::setPeresprosTimeout(const boost::posix_time::time_duration& timeout)
{
    peresprosTimeout_ = timeout;
    return *this;
}

DoHttpRequest& DoHttpRequest::setPeresprosReq(const std::string& req)
{
    ASSERT(correlation_.pult || correlation_.MsgId);
    peresprosReq_ = req;
    return *this;
}

DoHttpRequest& DoHttpRequest::setSSL(const UseSSLFlag& useSSL)
{
    useSSL_ = useSSL;
    return *this;
}

DoHttpRequest& DoHttpRequest::setDeffered(const bool isDeffered)
{
    isDeffered_ = isDeffered;
    return *this;
}

DoHttpRequest& DoHttpRequest::doHttpLog()
{
    recId_ = -1;
    return *this;
}

DoHttpRequest& DoHttpRequest::doHttpLog(std::map<std::string, std::string> &&data)
{
    data_ = std::move(data);
    return doHttpLog();
}

DoHttpRequest& DoHttpRequest::setGroupSize(const std::size_t size)
{
    groupSize_ = size;
    return *this;
}

const std::string SerializeRequest(const HttpReq& req)
{
    const HttpReq* reqPtr = &req;
    std::ostringstream data;
    OArchive_t ar(data);
    ar << reqPtr;
    return data.str();
}

static void ForkForTest();

static bool Connect(LocalSocket_t& socket)
{
#ifdef XP_TESTING
    if (inTestMode() and not _ForTests.need_real_http) {
        return false;
    }
#endif // 0
    const int RETRY_MAX = 5;
    boost::system::error_code err;
    for (int i = 0; i < RETRY_MAX; ++i) {
        socket.connect(boost::asio::local::stream_protocol::endpoint(GetSignalSockName()), err);
        if (!err)
            return true;

        LogTrace(TRACE1) << __FUNCTION__ << ": " << err.message();
        sleep(1);
    }
    LogError(STDLOG) << __FUNCTION__ << ": " << GetSignalSockName() << ": " << err.message();
    return false;
}

static void IPC_SendRequest(LocalSocket_t& socket, const std::string& cmd, const std::string& data)
{
    const std::string packet = cmd + MakeSizeHeader(data.size()) + data;
    boost::asio::write(socket, boost::asio::buffer(packet));
}

static bool NoRequestsFound(const std::vector<char>& bodyLength) noexcept
{
    return std::equal(bodyLength.begin(), bodyLength.end(), NO_REQS_FOUND.c_str());
}

static std::size_t GetBodyLength(const std::vector<char>& bodyLength) noexcept
{
    const auto number = std::string(bodyLength.begin(), bodyLength.end());
    LogTrace(TRACE1) << __FUNCTION__ << ": " << number;

    try {
        return std::stoul(number);
    } catch (const std::invalid_argument& e) {
        LogError(STDLOG) << __FUNCTION__ << ": " << number;
    } catch (const std::out_of_range& e) {
        LogError(STDLOG) << __FUNCTION__ << ": " << number;
    }

    return 0;
}

static std::vector<char> ReadHeader(LocalSocket_t& socket)
{
    std::vector<char> header(9);
    const auto readn = boost::asio::read(socket, boost::asio::buffer(header));
    LogTrace(TRACE1) << __FUNCTION__ << ": readn = " << readn;
    return header;
}

static std::string ReadBody(LocalSocket_t& socket, const std::vector<char>& header)
{
    const auto bodyLength = GetBodyLength(header);
    if (bodyLength == 0) {
        LogTrace(TRACE1) << __FUNCTION__ << ": ";
        return "";
    }

    LogTrace(TRACE1) << __FUNCTION__ << ": read header = " << std::string(header.begin(), header.end());
    LogTrace(TRACE1) << __FUNCTION__ << ": read bodyLength = " << bodyLength;
    std::vector<char> data(bodyLength, 0);
    const auto readn = boost::asio::read(socket, boost::asio::buffer(data));
    LogTrace(TRACE1) << __FUNCTION__ << ": readn = " << readn;
    return std::string(data.begin(), data.end());
}

static std::vector<std::string> IPC_ReadAnswers(LocalSocket_t& socket, const bool throw_on_hasntsent)
{
    std::vector<std::string> result;

    for (;;) {
        const auto header = ReadHeader(socket);
        if (NoRequestsFound(header)) {
            LogTrace(TRACE1) << __FUNCTION__ << ": ";
            if (throw_on_hasntsent) {
                throw HasNotSentAQueryBefore();
            }

            break;
        }

        const auto body = ReadBody(socket, header);
        if (body.empty()) {
            LogTrace(TRACE1) << __FUNCTION__ << ": ";
            break;
        }

        LogTrace(TRACE1) << __FUNCTION__ << ": ";
        result.push_back(body);
    }

    return result;
}

static std::vector<boost::posix_time::ptime> GenerateDeadlines(
        const boost::posix_time::ptime& now,
        const boost::posix_time::time_duration& timeout,
        unsigned maxTryCount)
{
    std::vector<boost::posix_time::ptime> result;
    for (unsigned i = 0; i < maxTryCount; ++i)
        result.push_back(now + (timeout * (i + 1)));
    return result;
}

static void validatePeresprosTimeOut(const Dates::time_duration &timeout,
    const Dates::time_duration peresprosTimeout,
    unsigned maxTryCount)
{
    const boost::posix_time::time_duration defVal = DefaultPeresprosTimeout(timeout, maxTryCount);
    if (peresprosTimeout < defVal) {
        LogError(STDLOG) << __FUNCTION__ << ": perespros timeout = " << peresprosTimeout
            << " is less than default value = " << defVal;
    }
}

bool forkAllowed()
{
    const char *allow = getenv ("XP_FORK_HTTPSRV");
    if(allow && strcmp(allow, "1") == 0)
        return true;
    else
        return false;
}

int64_t DoHttpRequest::operator()()
{
    if(peresprosTimeout_ == Dates::time_duration()) {
        peresprosTimeout_ = DefaultPeresprosTimeout(timeout_, maxTryCount_);
        LogTrace(TRACE5) << "Setting default peresprosTimeout = " << peresprosTimeout_;
    } else {
        validatePeresprosTimeOut(timeout_, peresprosTimeout_, maxTryCount_);
    }

    if (_ForTests.ignoreNextRequest) {
        LogTrace(TRACE1) << __FUNCTION__ << ": ignoring request due to ignoreNextRequest flag";
        _ForTests.ignoreNextRequest = false;
        return 0;
    }

    static bool forkedForTest = false;
    LogTrace(TRACE1) << __FUNCTION__ << ": forecasts.size() = " << _ForTests.forecasts.size();
    if ((inTestMode() || isNosir()) && !forkedForTest && _ForTests.forecasts.empty()) {
        if(not _ForTests.need_real_http and not forkAllowed())
            throw ServerFramework::Exception(STDLOG, __FUNCTION__, "Fork not allowed while forecast list is empty");
        ForkForTest();
        _ForTests.need_real_http = true;
        forkedForTest = true;
    }

    LogTrace(TRACE5) << __FUNCTION__ << ": " << correlation_;
    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    const HttpReq req(
            correlation_,
            domain_,
            hostAndPort_,
            now,
            reqText_,
            GenerateDeadlines(now, timeout_, maxTryCount_),
            customData_,
            customAuth_,
            clientAuth_,
            peresprosReq_,
            useSSL_,
            recId_,
            importance_,
            groupSize_);

    if (inTestMode() && !forkedForTest && _ForTests.captureRequest) {
            _ForTests.requests.push_back(req);
    }

    if (recId_ == -1) {
        const auto pair = ServerFramework::HttpLogs::log(req, data_);
        HttpReq *req2 = const_cast<HttpReq *>(&req);
        req2->recId = pair.first;
        req2->importance = pair.second;
    }

    if (!_ForTests.forecasts.empty()) {
        const Forecast forecast = _ForTests.forecasts.front();
        _ForTests.forecasts.pop();
        const HttpResp resp(now, forecast.httpResponse, boost::optional<CommErr>(), req);
        _ForTests.responses.push(resp);
        LogTrace(TRACE1) << __FUNCTION__ << ": response from forecasts";
    }
    else
    {
        auto send_request = [req](){
            boost::asio::io_service io;
            LocalSocket_t socket(io);
            if (!Connect(socket))
                return;
            IPC_SendRequest(socket, "PUT", SerializeRequest(req));
        };

        if (isDeffered_) {
            deffered_exec::call_after_commit([req, send_request](){
                send_request();
            });
        }
        else {
            send_request();
        }
    }
    if (!peresprosReq_.empty()) {
        ServerFramework::getQueryRunner().getEdiHelpManager().configForPerespros(STDLOG, peresprosReq_.c_str(), 0, peresprosTimeout_.total_seconds() + 3);
#ifdef XP_TESTING
        if(inTestMode()) {
            ServerFramework::setSavedSignal(peresprosReq_);
        }
#endif // XP_TESTING
    }
    return req.recId;
}

static std::string SerializeFetchParams(const ReqCorrelationData& corr, const Domain& domain, bool waitAll)
{
    std::ostringstream data;
    OArchive_t ar(data);
    ar << corr;
    ar << domain.str();
    ar << waitAll;
    return data.str();
}

static HttpResp DeserializeHttpResp(const std::string& data)
{
    std::istringstream in(data);
    IArchive_t ar(in);
    HttpResp* resp = NULL;
    ar >> resp;
    std::unique_ptr<HttpResp> resp_free(resp);
    return *resp;
}

static Stat DeserializeStat(const std::string& data)
{
    std::istringstream in(data);
    IArchive_t ar(in);
    Stat stat = {};
    ar >> stat;
    return stat;
}

static std::vector<HttpResp> FetchHttpResponses(const ReqCorrelationData& corr, const Domain& domain, bool waitAll, bool throw_on_hasntsent, bool fetchAllResponses = true)
{
    ProgTrace(TRACE5, "%s(waitAll=%s)", __func__, waitAll ? "true" : "false");
    if (inTestMode() and not _ForTests.need_real_http) {
        std::vector<HttpResp> result;
        while (!_ForTests.responses.empty()) {
            result.push_back(_ForTests.responses.front());
            _ForTests.responses.pop();
            if (not fetchAllResponses) {
                break;
            }
        }
        for (const auto &resp : result) {
            if (resp.req.recId > 0)
                ServerFramework::HttpLogs::log(resp);
        }
        LogTrace(TRACE1) << __FUNCTION__ << ": responses from forecasts";
        return result;
    }

    boost::asio::io_service io;
    LocalSocket_t socket(io);
    if (!Connect(socket))
    {
        tst();
        return std::vector<HttpResp>();
    }
    /* Посылаем параметры */
    LogTrace(TRACE1) << __FUNCTION__ << ": ";
    IPC_SendRequest(socket, "GET", SerializeFetchParams(corr, domain, waitAll));
    /* Читаем ответы */
    LogTrace(TRACE1) << __FUNCTION__ << ": ";
    auto answers = IPC_ReadAnswers(socket, throw_on_hasntsent);
    std::vector<HttpResp> result;
    std::transform(answers.begin(), answers.end(), std::back_inserter(result), DeserializeHttpResp);
    for (const auto &resp : result) {
        if (resp.req.recId > 0)
            ServerFramework::HttpLogs::log(resp);
    }
    LogTrace(TRACE5) << __FUNCTION__ << ": " << corr << ", result.size() = " << result.size();
    return result;
}

std::vector<HttpResp> FetchHttpResponses(Pult pult, const Domain& domain, bool waitAll, bool throw_on_hasntsent, bool fetchAllResponses)
{
    ReqCorrelationData corr(std::move(pult));
    return FetchHttpResponses(corr, domain, waitAll,throw_on_hasntsent, fetchAllResponses);
}

std::vector<HttpResp> FetchHttpResponses(const Domain& domain, bool waitAll, bool throw_on_hasntsent)
{
    return FetchHttpResponses(ServerFramework::getQueryRunner().getEdiHelpManager().msgId(), domain, waitAll,throw_on_hasntsent);
}

std::vector<HttpResp> FetchHttpResponses(const ServerFramework::InternalMsgId& MsgId, const Domain& domain, bool waitAll, bool throw_on_hasntsent)
{
    //only array field transferred
    ReqCorrelationData corr(ServerFramework::InternalMsgId(MsgId.id()));
    return FetchHttpResponses(corr, domain, waitAll,throw_on_hasntsent);
}

std::vector<HttpResp> FetchHttpResponses(bool waitAll, bool throw_on_hasntsent)
{
    return FetchHttpResponses(ServerFramework::getQueryRunner().getEdiHelpManager().msgId(),
                              Domain("default"), waitAll, throw_on_hasntsent);
}

std::vector<HttpResp> FetchHttpResponses(CorrelationID cid, const Domain& domain, bool waitAll, bool throw_on_hasntsent)
{
    ReqCorrelationData corr(std::move(cid));
    return FetchHttpResponses(corr, domain, waitAll,throw_on_hasntsent);
}


boost::optional<Stat> GetStat()
{
    boost::asio::io_service io;
    LocalSocket_t socket(io);
    if (!Connect(socket))
        return boost::optional<Stat>();

    /* Посылаем запрос на получение статистики */
    IPC_SendRequest(socket, "STA", "");
    /* Читаем ответы */
    const std::vector<std::string> answers = IPC_ReadAnswers(socket, false);
    if (answers.size() != 1) {
        LogError(STDLOG) << __FUNCTION__ << ": failed to get stat";
        return boost::optional<Stat>();
    }
    return DeserializeStat(answers.at(0));
}

/*******************************************************************************
 * ForkForTest
 ******************************************************************************/

static void ForkForTest()
{
    if (fork() == 0) {
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        set_signal(term3);
        auto save_sess = OciCpp::mainSessionPtr();
        InitLogTime("HTTP_F");
        OciCpp::createMainSession(STDLOG,get_connect_string());
        httpsrv::Run(boost::optional<int>());
    } else {
        sleep(3);
    }
}

boost::optional<HttpPayload> HttpResponsePayload(const std::string& httpResponse)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": httpResponse = " << httpResponse;

    static const boost::regex re("HTTP[^ ]+ (\\d+) .*?\r?\n\r?\n(.*)");
    boost::smatch what;
    if (!boost::regex_match(httpResponse, what, re)) {
        LogTrace(TRACE1) << __FUNCTION__ << ": invalid HTTP response format: " << httpResponse;
        return boost::optional<HttpPayload>();
    }

    const HttpPayload payload = {
        boost::lexical_cast<unsigned>(what[1]),
        what[2]
    };
    return payload;
}

std::pair<unsigned, unsigned> status_and_offset(std::string const& txt)
{
    if(txt.compare(0, 4, "HTTP") != 0)
        return {};
    size_t first_space = txt.find(' ');
    if(first_space == txt.npos)
        return {};
    size_t second_space = txt.find_first_of(" \r", first_space+1);
    size_t rnrn = txt.find("\r\n\r\n", second_space);
    return { std::stoi(txt.substr(first_space+1, second_space-first_space-1)),
             rnrn == txt.npos ? txt.size() : rnrn + 4 };
}

HasNotSentAQueryBefore::HasNotSentAQueryBefore() : ServerFramework::Exception("httpsrv::HasNotSentAQueryBefore") {}

} /* namespace httpsrv */

/*******************************************************************************
 * main
 ******************************************************************************/

int main_httpsrv(int supervisorSocket, int argc, char* argv[])
{
    set_signal(term3);
    InitLogTime("HTTPSRV");
    monitor_special();
    OciCpp::createMainSession(STDLOG,get_connect_string());

    httpsrv::Run(boost::optional<int>(supervisorSocket));
    return 0;
}

/*******************************************************************************
 * Утилиты для тестирования
 ******************************************************************************/

#ifdef XP_TESTING
#include "func_placeholders.h"
#include "text_codec.h"
#include "str_utils.h"

namespace
{
const std::string rnrn("\r\n\r\n");
}

namespace httpsrv {
namespace xp_testing {

std::string MakeDefaultHttpsHeaders(size_t contentLen)
{
    std::ostringstream out;
    out << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: text/xml; charset=utf-8\r\n"
        << "Content-Length: " << contentLen << "\r\n";
    return out.str();
}

std::string FixHttpHeaders(const std::string& httpHeaders) {
    const auto headers = StrUtils::split_string<std::vector<std::string>>(httpHeaders, '\n');
    std::ostringstream out;
    for (auto&& header : headers)
        out << header << "\r\n";
    return out.str();
}

void add_forecast(const std::string& quote_response_quote)
{
    const Forecast forecast = { quote_response_quote };
    _ForTests.forecasts.push(forecast);
    LogTrace(TRACE1) << __FUNCTION__ << ": _ForTests.forecasts.size() = " << _ForTests.forecasts.size();
}

void set_ignore_next_request(bool x)
{
    _ForTests.ignoreNextRequest = x;
}

void set_capture_request(bool x)
{
    _ForTests.captureRequest = x;
}

void set_check_http_headers(int x)
{
    _ForTests.checkHttpHeaders = x;
}

void set_need_real_http(bool x)
{
    _ForTests.need_real_http = x;
}

void set_need_real_commit(bool x)
{
    _ForTests.need_real_commit = x;
}

static std::string formatJsonString(std::string json) { return json; }

std::vector<std::string> getOutgoingHttpRequests()
{
    using httpsrv::CheckHttpFlags;
    std::vector<std::string> result;
    for (const httpsrv::HttpReq& httpReq: _ForTests.requests) {
        const std::string& reqtext = httpReq.text;
        std::string::size_type body_start = reqtext.find(rnrn);
        std::string body;
        if ((_ForTests.checkHttpHeaders & CheckHttpFlags::Body) && body_start != std::string::npos)
        {
            body = reqtext.substr(body_start + rnrn.size());
            if ((_ForTests.checkHttpHeaders & CheckHttpFlags::XML)) {
                body = formatXmlString(body);
            }
            if ((_ForTests.checkHttpHeaders & CheckHttpFlags::JSON)) {
                body = formatJsonString(body);
            }
        }
        std::string header;
        if ((_ForTests.checkHttpHeaders & CheckHttpFlags::Header) && body_start != std::string::npos && body_start > 0)
        {
            header = reqtext.substr(0, body_start + rnrn.size() / 2);
            StrUtils::replaceSubstr(header, "\r\n", "\n");
        }
        std::string ret;
        if (not header.empty()) {
            ret = header;
            if (not body.empty()) {
                ret += "\n";
            }
        }
        ret += body;
        result.push_back(ret);
    }
    _ForTests.requests.clear();
    return result;
}


} //namespace xp_testing
} //namespace httpsrv

static std::string FP_utf8(const std::vector<std::string>& args)
{
    ASSERT(args.size() == 1);
    static HelpCpp::TextCodec codec("CP866", "UTF-8");
    return codec.encode(args.at(0));
}

static std::string FP_http_forecast(const std::vector<tok::Param>& args)
{
    tok::ValidateParams(args, 0, 0, "content headers file");
    std::string httpContent;
    if(std::none_of(args.begin(), args.end(), [](auto&a){ return a.name == "content"; }))
    {
        auto const fname = tok::GetValue_ThrowUndefined(args, "file");
        std::ifstream f(fname.c_str());
        httpContent.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }
    else
        httpContent = tok::GetValue(args, "content", "");
    std::string httpHeaders = tok::GetValue(args, "headers", "");
    if (httpHeaders.empty())
        httpHeaders = httpsrv::xp_testing::MakeDefaultHttpsHeaders(httpContent.size());
    else
        httpHeaders = httpsrv::xp_testing::FixHttpHeaders(httpHeaders);
    httpsrv::xp_testing::add_forecast(httpHeaders + "\r\n" + httpContent);
    return std::string();
}

static std::string FP_http_ignore_next_request(const std::vector<tok::Param>& args)
{
    ASSERT(args.empty());
    _ForTests.ignoreNextRequest = true;
    return std::string();
}

static std::string FP_http_update_first_forecast(const std::vector<tok::Param>& args)
{
    tok::ValidateParams(args, 0, 0, "content headers");
    const std::string httpContent = tok::GetValue(args, "content", "");
    const std::string httpHeaders = tok::GetValue(args, "headers", httpsrv::xp_testing::MakeDefaultHttpsHeaders(httpContent.size()));
    const Forecast forecast = { httpHeaders + "\r\n" + httpContent };
    ASSERT(not _ForTests.responses.empty());
    _ForTests.responses.front().text = forecast.httpResponse;
    return std::string();
}

static std::string FP_http_delete_first_response(const std::vector<tok::Param>& args)
{
    ASSERT(not _ForTests.responses.empty());
    _ForTests.responses.pop();
    return std::string();
}

static std::string FP_http_request_capture(const std::vector<tok::Param>& args)
{
    using httpsrv::CheckHttpFlags;
    tok::ValidateParams(args, 0, 0, "check type");
    const std::string check = tok::GetValue(args, "check", "off");
    const std::string type = tok::GetValue(args, "type", "");
    if (type == "xml") {
        _ForTests.checkHttpHeaders |= CheckHttpFlags::XML;
    }
    if (type == "json") {
        _ForTests.checkHttpHeaders |= CheckHttpFlags::JSON;
    }
    if (check == "off") {
        _ForTests.checkHttpHeaders = CheckHttpFlags::Off;
        _ForTests.captureRequest = false;
        _ForTests.requests.clear();
    } else if (check == "body") {
        _ForTests.checkHttpHeaders |= CheckHttpFlags::Body;
        _ForTests.captureRequest = true;
    } else if (check == "header") {
        _ForTests.checkHttpHeaders |= CheckHttpFlags::Header;
        _ForTests.captureRequest = true;
    } else if (check == "full") {
        _ForTests.checkHttpHeaders |= CheckHttpFlags::Full;
        _ForTests.captureRequest = true;
    } else {
        ASSERT(false);
    }
    return std::string();
}

static std::string FP_http_no_forecasts(const std::vector<tok::Param>& args)
{
    ASSERT(_ForTests.responses.empty());
    ASSERT(_ForTests.forecasts.empty());
    return "";
}

FP_REGISTER("utf8", FP_utf8);
FP_REGISTER("http_forecast", FP_http_forecast);
FP_REGISTER("http_ignore_next_request", FP_http_ignore_next_request);
FP_REGISTER("http_update_first_forecast", FP_http_update_first_forecast);
FP_REGISTER("http_delete_first_response", FP_http_delete_first_response);
FP_REGISTER("http_request_capture", FP_http_request_capture);
FP_REGISTER("http_no_forecasts", FP_http_no_forecasts);

#endif /* #ifdef XP_TESTING */
