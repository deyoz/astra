
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include "dispatcher_frontend.h"
#include "blev.h"
#include "monitor_ctl.h"
#include "ourtime.h"
#include "dispatcher.h"

#define NICKNAME "MIXA"
#include "slogger.h"

namespace Dispatcher {

TxtUdpFrontend::TxtUdpFrontend(
        boost::asio::io_service& io_s,
        const uint8_t headType,
        const std::string& addr,
        const unsigned short port,
        Dispatcher<TxtUdpFrontend>& d,
        const size_t) :
    blev_(ServerFramework::make_blev(headType)),
    dispatcher_(d),
    socket_(io_s),
    buf_(MAX_DATAGRAM_SIZE)
{
    const boost::asio::ip::address ip = boost::asio::ip::address::from_string(addr);
    const boost::asio::ip::udp::endpoint ep(ip, port);

    socket_.open(boost::asio::ip::udp::v4());
    socket_.bind(ep);
    write_set_cur_req(ServerFramework::utl::stringize(ep).c_str());

    read();
}
//-----------------------------------------------------------------------
TxtUdpFrontend::~TxtUdpFrontend()
{
    boost::system::error_code e;
    socket_.shutdown(boost::asio::ip::udp::socket::shutdown_both, e);
    socket_.close(e);
}
//-----------------------------------------------------------------------
void TxtUdpFrontend::read()
{
    LogTrace(TRACE5) << __FUNCTION__;

    std::shared_ptr<EndpointType> remoteEndpoint(new EndpointType());

    buf_.resize(MAX_DATAGRAM_SIZE);
    socket_.async_receive_from(
            boost::asio::buffer(buf_),
            *remoteEndpoint,
            boost::bind(
                &TxtUdpFrontend::handleRead,
                this,
                remoteEndpoint,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred
            )
    );
}
//-----------------------------------------------------------------------
void TxtUdpFrontend::handleRead(
        const std::shared_ptr<EndpointType>& ep,
        const boost::system::error_code& e,
        const size_t bytes_transferred)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << ep;

    if (!e) {
        if (blev_->hlen() <= bytes_transferred) {
            std::vector<uint8_t> head;
            std::vector<uint8_t> data(bytes_transferred - blev_->hlen());
            uint8_t* buf_to_write_in = blev_->prepare_head(head);

            memcpy(buf_to_write_in, buf_.data(), blev_->hlen());
            if (bytes_transferred == blev_->hlen()) {
                data.resize(2);
                memcpy(data.data(), buf_.data() + 2, 2);
            } else {
                memcpy(data.data(), buf_.data() + blev_->hlen(), data.size());
            }

            struct timeval tmstamp;

            gettimeofday(&tmstamp, 0);
            waitAnswerClients_.insert(ep);
            blev_->ok_build_all_the_stuff(head, tmstamp);
            dispatcher_.takeRequest(
                    0,
                    ep,
                    std::string(),
                    head,
                    data,
                    Dispatcher<TxtUdpFrontend>::WHAT_DROP_IF_QUEUE_FULL::FRONT
            );
        }
    } else {
        LogTrace(TRACE1) << __FUNCTION__ << ": " << *ep << "::  error : " << e.message();
    }
    read();
}
//-----------------------------------------------------------------------
//Content input parameters answerHead and answerData will be change in this function
void TxtUdpFrontend::sendAnswer(const uint64_t ,
                                const KeyType& connectionId,
                                std::vector<uint8_t>& answerHead,
                                std::vector<uint8_t>& answerData,
                                std::size_t queueSize, /*size of the message queue*/
                                std::time_t waitTime /*time wait the message in queue*/)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << connectionId;

    static std::vector<boost::asio::const_buffer> answer;
    const ClientConnectionSet::iterator ep = waitAnswerClients_.find(connectionId);

    if (waitAnswerClients_.end() != ep) {
        auto ansHead = std::make_shared<std::vector<uint8_t> >();
        auto ansData = std::make_shared<std::vector<uint8_t> >();

        ansHead->swap(answerHead);
        ansData->swap(answerData);

        size_t res_offset = blev_->filter(*ansHead);

        answer.clear();
        answer.emplace_back(ansHead->data() + res_offset, ansHead->size() - res_offset);
        answer.emplace_back(ansData->size() ? ansData->data() : nullptr, ansData->size());
        socket_.async_send_to(answer, *(*ep), boost::bind(
                    &TxtUdpFrontend::handleWrite, this, ansHead, ansData, boost::asio::placeholders::error)
        );
        waitAnswerClients_.erase(ep);
    } else {
        LogTrace(TRACE5) << __FUNCTION__ << ": " << connectionId << ": Remote endpoint not find";
    }
}
//-----------------------------------------------------------------------
void TxtUdpFrontend::timeoutExpired(
        const uint64_t balancerId,
        const KeyType& connectionId,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data,
        std::size_t queueSize, /*size of the message queue*/
        std::time_t waitTime /*time wait the message in queue*/)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << connectionId;

    blev_->make_expired(head, data);
    sendAnswer(balancerId, connectionId, head, data, queueSize, waitTime);
}
//-----------------------------------------------------------------------
void TxtUdpFrontend::queueOverflow(
        const uint64_t balancerId,
        const KeyType& clientConnectionId,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data,
        std::size_t queueSize, /*size of the message queue*/
        std::time_t waitTime /*time wait the message in queue*/)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << clientConnectionId;

    blev_->make_excrescent(head, data);
    sendAnswer(balancerId, clientConnectionId, head, data, queueSize, waitTime);
}
//-----------------------------------------------------------------------
void TxtUdpFrontend::handleWrite(
            const std::shared_ptr<const std::vector<uint8_t> >& ansHead,
            const std::shared_ptr<const std::vector<uint8_t> >& ansData,
            const boost::system::error_code& e
        )
{
    LogTrace(TRACE5) << __FUNCTION__;

    if (e) {
        LogTrace(TRACE1) << __FUNCTION__ << "::error : " << e.message();
    }
}
//-----------------------------------------------------------------------
uint32_t TxtUdpFrontend::answeringNow(std::vector<uint8_t>& ansHeader)
{
    static const uint32_t MAX_TIME_WAIT = 768; //second
    uint32_t tmp = blev_->enqueable(ansHeader);

    assert(tmp < MAX_TIME_WAIT);

    return blev_->enqueable(ansHeader);
}
//-----------------------------------------------------------------------
AirSrvFrontend::AirSrvFrontend(
        boost::asio::io_service& io_s,
        const char* clientSockName,
        Dispatcher<AirSrvFrontend>& d) :
    acceptor_(io_s),
    socket_(io_s),
    dispatcher_(d),
    size_(),
    fakeHead_(),
    data_()
{
    InitLogTime("XPR_DISP");
    unlink(clientSockName);
    const boost::asio::local::stream_protocol::endpoint ep(clientSockName);
    acceptor_.open(ep.protocol());
    acceptor_.set_option(boost::asio::local::stream_protocol::acceptor::reuse_address(true));
    acceptor_.bind(ep);
    acceptor_.listen();
    write_set_cur_req(ep.path().c_str());

    asyncAccept();
}
//-----------------------------------------------------------------------
AirSrvFrontend::~AirSrvFrontend()
{
    boost::system::error_code e;

    socket_.shutdown(boost::asio::local::stream_protocol::socket::shutdown_both, e);
}
//-----------------------------------------------------------------------
void AirSrvFrontend::asyncAccept()
{
    acceptor_.async_accept(socket_,
            boost::bind(&AirSrvFrontend::handleAccept, this, boost::asio::placeholders::error));
}
//-----------------------------------------------------------------------
void AirSrvFrontend::handleAccept(const boost::system::error_code& e)
{
    if (!e) {
        readSize();
    } else {
        handleError(__FUNCTION__, e);
    }
}
//-----------------------------------------------------------------------
void AirSrvFrontend::readSize()
{
    boost::asio::async_read(socket_, boost::asio::buffer(&size_, sizeof(size_)),
            boost::bind(&AirSrvFrontend::handleReadSize, this, boost::asio::placeholders::error));
}
//-----------------------------------------------------------------------
void AirSrvFrontend::handleReadSize(const boost::system::error_code& e)
{
    if (!e) {
        data_.resize(ntohl(size_));
        boost::asio::async_read(socket_, boost::asio::buffer(data_),
                boost::bind(&AirSrvFrontend::handleReadData, this, boost::asio::placeholders::error));
    } else {
        handleError(__FUNCTION__, e);
    }
}
//-----------------------------------------------------------------------
void AirSrvFrontend::handleReadData(const boost::system::error_code& e)
{
    if (!e) {
        dispatcher_.takeRequest(0, 0, std::string(), fakeHead_, data_,
                                Dispatcher<AirSrvFrontend>::WHAT_DROP_IF_QUEUE_FULL::BACK);
        readSize();
    }else {
        handleError(__FUNCTION__, e);
    }
}
//-----------------------------------------------------------------------
void AirSrvFrontend::handleError(const char* functionName, const boost::system::error_code& e)
{
    boost::system::error_code error;

    LogTrace(TRACE0) << functionName << "(" << e << "): " << e.message();
    socket_.shutdown(boost::asio::local::stream_protocol::socket::shutdown_both, error);
    socket_.close(error);
    asyncAccept();
}
//-----------------------------------------------------------------------
void AirSrvFrontend::sendAnswer(
        const uint64_t ,
        const KeyType& connectionId,
        std::vector<uint8_t>& answerHead,
        std::vector<uint8_t>& answerData,
        std::size_t queueSize, /*size of the message queue*/
        std::time_t waitTime /*time wait the message in queue*/)
{
    return;
}
//-----------------------------------------------------------------------
void AirSrvFrontend::timeoutExpired(
        const uint64_t balancerId,
        const KeyType& connectionId,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data,
        std::size_t queueSize, /*size of the message queue*/
        std::time_t waitTime /*time wait the message in queue*/)
{
    return;
}

//-----------------------------------------------------------------------
void AirSrvFrontend::queueOverflow(
        const uint64_t balancerId,
        const KeyType& connectionId,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data,
        std::size_t queueSize, /*size of the message queue*/
        std::time_t waitTime /*time wait the message in queue*/)
{
    return;
}
//-----------------------------------------------------------------------
HttpFrontend::HttpFrontend(
        boost::asio::io_service& io_s,
        const uint8_t headType,
        const std::string& addr,
        const unsigned short port,
        Dispatcher<HttpFrontend>& d,
        const size_t) :
    BasePlainHttpFrontend< Dispatcher< HttpFrontend > >(io_s, headType, addr, port, d)//,
{ }
//-----------------------------------------------------------------------
HttpSSLFrontend::HttpSSLFrontend(
            boost::asio::io_service& io_s,
            const uint8_t headType,
            const std::string& addr,
            const unsigned short port,
            Dispatcher<HttpSSLFrontend>& d,
            const std::string& certificateFileName,
            const std::string& privateKeyFileName,
            const std::string& dhParamsFileName) :
    BaseHttpFrontend< HttpSSLFrontend::SocketType, Dispatcher< HttpSSLFrontend > >(io_s, headType, addr, port, d),
    newConnection_(),
    context_(boost::asio::ssl::context::tlsv1)
{
    static const char ciphers[] = "ALL:!aNULL:!ADH:!eNULL:!LOW:!EXP:RC4+RSA:+HIGH:+MEDIUM";

    context_.set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::single_dh_use
            );
    context_.set_password_callback(boost::bind(&HttpSSLFrontend::getPasswd, this));
    context_.use_certificate_chain_file(certificateFileName);
    context_.use_private_key_file(privateKeyFileName, boost::asio::ssl::context::pem);
    context_.use_tmp_dh_file(dhParamsFileName);
    if (!SSL_CTX_set_cipher_list(context_.native_handle(), ciphers)) {
        throw std::logic_error("setting cipher list failed");
    }

    newConnection_.reset(new Connection<SocketType>(io_s, context_));
    accept();
}
//-----------------------------------------------------------------------
HttpSSLFrontend::~HttpSSLFrontend()
{ }
//-----------------------------------------------------------------------
void HttpSSLFrontend::accept()
{
    LogTrace(TRACE5) << __FUNCTION__;

    acceptor_.async_accept(
            newConnection_->socket().lowest_layer(),
            boost::bind(
                &HttpSSLFrontend::handleAccept,
                this,
                boost::asio::placeholders::error
            )
    );
}
//-----------------------------------------------------------------------
void HttpSSLFrontend::handleAccept(const boost::system::error_code& e)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << newConnection_->socket().lowest_layer().native_handle();

    if (!e) {
        newConnection_->socket().async_handshake(boost::asio::ssl::stream_base::server,
                boost::bind(
                    &HttpSSLFrontend::handleHandShake,
                    this,
                    newConnection_,
                    boost::asio::placeholders::error
                    )
        );
    } else {
        LogError(STDLOG) << __FUNCTION__ << ": " << newConnection_->socket().lowest_layer().native_handle()
            << ": error( " << e << " ) - " << e.message();
    }

    newConnection_.reset(new Connection<SocketType>(service_, context_));
    accept();
}
//-----------------------------------------------------------------------
void HttpSSLFrontend::handleHandShake(
        const std::shared_ptr<Connection<SocketType> >& conn,
        const boost::system::error_code& e)
{
    if (!e) {
        readHeaders(conn);
    } else {
        LogTrace(TRACE0) << __FUNCTION__ << ": " << conn->socket().lowest_layer().native_handle()
            << ": error( " << e << " ) - " << e.message();
    }
}

//-----------------------------------------------------------------------

namespace bf { // base frontend
//-----------------------------------------------------------------------
size_t parseContentLength(const uint8_t* const headers, const size_t size)
{
    static const boost::regex re(".*\r?\nContent-Length: *(\\d+)\r?\n.*", boost::regex::icase);
    boost::match_results<const char*> what;

    if (!boost::regex_match(
                reinterpret_cast<const char*>(headers),
                reinterpret_cast<const char*>(headers + size),
                what,
                re))
    {

        return 0;
    }

    return boost::lexical_cast<size_t>(what[1]);
}
//-----------------------------------------------------------------------
bool consumeCRLF(boost::asio::streambuf& sbuf)
{
    std::istream in(&sbuf);
    char c = 0;
    return in.get(c) && (c == '\n' or (c == '\r' && in.get(c) && c == '\n'));
}
//-----------------------------------------------------------------------
boost::optional<size_t> parseChunkHeader(const std::vector<uint8_t>& header)
{
    static const boost::regex re("([0-9a-fA-F]+).*\r?\n");
    boost::match_results<std::vector<uint8_t>::const_iterator> what;

    if (!boost::regex_match(header.begin(), header.end(), what, re)) {

        return boost::none;
    }

    return std::strtol(reinterpret_cast<const char*>(what[1].str().c_str()), NULL, 16);
}
//-----------------------------------------------------------------------
bool chunkedTransferEncoding(const uint8_t* const headers, const size_t size)
{
    static const boost::regex re(".*\r?\nTransfer-Encoding: *chunked\r?\n.*", boost::regex::icase);

    return boost::regex_match(headers + size, headers + size, re);
}

std::vector<uint8_t> normalizeHttpRequestHeader(const std::vector<uint8_t>& buf)
{
    std::vector<uint8_t> result;
    result.reserve(buf.size() + 100);
    bool end_of_header = false;
    for(std::vector<uint8_t>::size_type i = 0; i < buf.size(); i ++) {
        if(buf[i] == '\n' && i > 0 && buf[i-1] != '\r' && !end_of_header)
            result.push_back('\r');
        if(buf[i] == '\n' && i > 0 && buf[i-1] == '\n')
            end_of_header = true;
        if(buf[i] == '\n' && i > 2 && buf[i-1] == '\r' && buf[i-2] == '\n' && buf[i-3] == '\r')
            end_of_header = true;
        result.push_back(buf[i]);
    }
    return result;
}
//-----------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const TraceFlags& tf)
{
    return os << "{ ["
              << (tf.first & MSG_TEXT ? " MSG_TEXT" : "")
              << (tf.first & MSG_BINARY ? " MSG_BINARY" : "")
              << (tf.first & MSG_COMPRESSED ? " MSG_COMPRESSED" : "")
              << (tf.first & MSG_ENCRYPTED ? " MSG_ENCRYPTED" : "")
              << (tf.first & MSG_CAN_COMPRESS ? " MSG_CAN_COMPRESS" : "")
              << (tf.first & MSG_SPECIAL_PROC ? " MSG_SPECIAL_PROC" : "")
              << (tf.first & MSG_PUB_CRYPT ? " MSG_PUB_CRYPT" : "")
              << (tf.first & MSG_SYS_ERROR ? " MSG_SYS_ERROR" : "")
              << " ], ["
              << (tf.second & REQ_NOT_PROCESSED ? " REQ_NOT_PROCESSED" : "")
              << (tf.second & FLAG_INVALID_XUSER ? " FLAG_INVALID_XUSER" : "")
              << " ] }";
}
//-----------------------------------------------------------------------

} // namespace bf

} //namespace Dispatcher
