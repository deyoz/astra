#ifndef _SERVERLIB_DISPATCHER_FRONTEND_H_
#define _SERVERLIB_DISPATCHER_FRONTEND_H_

#include "base_frontend.h"

namespace Dispatcher {

class TxtUdpFrontend
{
    typedef boost::asio::ip::udp::endpoint EndpointType;
    typedef std::set<std::shared_ptr<EndpointType> > ClientConnectionSet;
public:
    typedef ClientConnectionSet::key_type KeyType;

public:
    TxtUdpFrontend(
            boost::asio::io_service& io_s,
            const uint8_t headType,
            const std::string& addr,
            const unsigned short port,
            Dispatcher<TxtUdpFrontend>& d,
            const size_t
    );
    //-----------------------------------------------------------------------
    TxtUdpFrontend(const TxtUdpFrontend& ) = delete;
    TxtUdpFrontend& operator=(const TxtUdpFrontend& ) = delete;
    //-----------------------------------------------------------------------
    TxtUdpFrontend(TxtUdpFrontend&& ) = delete;
    TxtUdpFrontend& operator=(TxtUdpFrontend&& ) = delete;
    //-----------------------------------------------------------------------
    ~TxtUdpFrontend();
    //-----------------------------------------------------------------------
    void sendAnswer(
            const uint64_t ,
            const KeyType& connectionId,
            std::vector<uint8_t>& answerHead,
            std::vector<uint8_t>& answerData,
            std::size_t queueSize, /*size of the message queue*/
            std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void timeoutExpired(
            const uint64_t balancerId,
            const KeyType& connectionId,
            std::vector<uint8_t>& head,
            std::vector<uint8_t>& data,
            std::size_t queueSize, /*size of the message queue*/
            std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void queueOverflow(
            const uint64_t balancerId,
            const KeyType& clientConnectionId,
            std::vector<uint8_t>& head,
            std::vector<uint8_t>& data,
            std::size_t queueSize, /*size of the message queue*/
            std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void rmConnectionId(const KeyType& connectionId) {
        waitAnswerClients_.erase(connectionId);
    }
    //-----------------------------------------------------------------------
    uint32_t answeringNow(std::vector<uint8_t>& ansHeader);
    //-----------------------------------------------------------------------
    const ServerFramework::MsgId& msgIdFromSignal(const std::vector<uint8_t>& signal) {
        return blev_->signal_msgid(signal);
    }
    //-----------------------------------------------------------------------
    const ServerFramework::MsgId& msgIdFromHeader(const std::vector<uint8_t>& head) {
        static ServerFramework::MsgId id;

        assert((LEVBDATAOFF + sizeof(id.b)) <= head.size());
        memcpy(id.b, head.data() + LEVBDATAOFF, sizeof(id.b));

        return id;
    }
    //-----------------------------------------------------------------------
    void makeSignalSendable(const std::vector<uint8_t>& signal,
            std::vector<uint8_t>& ansHead, std::vector<uint8_t>& ansData) {
        blev_->make_signal_sendable(signal, ansHead, ansData);
    }
    //-----------------------------------------------------------------------
private:
    void read();
    //-----------------------------------------------------------------------
    void handleRead(
            const std::shared_ptr<EndpointType>& ep,
            const boost::system::error_code& e,
            const size_t bytes_transferred);
    //-----------------------------------------------------------------------
    void handleWrite(
            const std::shared_ptr<const std::vector<uint8_t> >& ansHead,
            const std::shared_ptr<const std::vector<uint8_t> >& ansData,
            const boost::system::error_code& e
    );
    //-----------------------------------------------------------------------
private:
    static const int MAX_DATAGRAM_SIZE = 4096;

private:
    const std::unique_ptr<ServerFramework::BLev> blev_;
    Dispatcher<TxtUdpFrontend>& dispatcher_;
    boost::asio::ip::udp::socket socket_;
    ClientConnectionSet waitAnswerClients_;
    std::vector<uint8_t> buf_;
};

class AirSrvFrontend
{
public:
    typedef int KeyType;

public:
    AirSrvFrontend(boost::asio::io_service& io_s, const char* clientSockName, Dispatcher<AirSrvFrontend>& d);
    //-----------------------------------------------------------------------
    AirSrvFrontend(const AirSrvFrontend& ) = delete;
    AirSrvFrontend& operator=(const AirSrvFrontend& ) = delete;
    //-----------------------------------------------------------------------
    AirSrvFrontend(AirSrvFrontend&& ) = delete;
    AirSrvFrontend& operator=(AirSrvFrontend&& ) = delete;
    //-----------------------------------------------------------------------
    ~AirSrvFrontend();
    //-----------------------------------------------------------------------
    void sendAnswer(const uint64_t ,
                    const KeyType& connectionId,
                    std::vector<uint8_t>& answerHead,
                    std::vector<uint8_t>& answerData,
                    std::size_t queueSize, /*size of the message queue*/
                    std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void timeoutExpired(const uint64_t balancerId,
                        const KeyType& connectionId,
                        std::vector<uint8_t>& head,
                        std::vector<uint8_t>& data,
                        std::size_t queueSize, /*size of the message queue*/
                        std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void queueOverflow(const uint64_t balancerId,
                       const KeyType& connectionId,
                       std::vector<uint8_t>& head,
                       std::vector<uint8_t>& data,
                       std::size_t queueSize, /*size of the message queue*/
                       std::time_t waitTime /*time wait the message in queue*/
    );
    //-----------------------------------------------------------------------
    void rmConnectionId(const KeyType& connectionId)
    { }
    //-----------------------------------------------------------------------
    uint32_t answeringNow(std::vector<uint8_t>& ansHeader) {

        return 0;
    }
    //-----------------------------------------------------------------------
    const ServerFramework::MsgId& msgIdFromSignal(const std::vector<uint8_t>& signal) {
        static const ServerFramework::MsgId id;

        return id;
    }
    //-----------------------------------------------------------------------
    const ServerFramework::MsgId& msgIdFromHeader(const std::vector<uint8_t>& head) {
        static const ServerFramework::MsgId id;

        return id;
    }
    //-----------------------------------------------------------------------
    void makeSignalSendable(const std::vector<uint8_t>& signal,
            std::vector<uint8_t>& ansHead, std::vector<uint8_t>& ansData)
    { }
    //-----------------------------------------------------------------------
private:
    void asyncAccept();
    //-----------------------------------------------------------------------
    void handleAccept(const boost::system::error_code& e);
    //-----------------------------------------------------------------------
    void readSize();
    //-----------------------------------------------------------------------
    void handleReadSize(const boost::system::error_code& e);
    //-----------------------------------------------------------------------
    void handleReadData(const boost::system::error_code& e);
    //-----------------------------------------------------------------------
    void handleError(const char* functionName, const boost::system::error_code& e);
    //-----------------------------------------------------------------------
private:
    boost::asio::local::stream_protocol::acceptor acceptor_;
    boost::asio::local::stream_protocol::socket socket_;
    Dispatcher<AirSrvFrontend>& dispatcher_;
    uint32_t size_;
    std::vector<uint8_t> fakeHead_;
    std::vector<uint8_t> data_;
};

//-----------------------------------------------------------------------

class TcpFrontend
    : public bf::BaseTcpFrontend<Dispatcher<TcpFrontend> >
{
public:
    TcpFrontend(
            boost::asio::io_service& io_s,
            const uint8_t headType,
            const boost::optional<std::pair<std::string, unsigned short> >& addr,
            const boost::optional<std::pair<std::string, unsigned short> >& balancerAddr,
            Dispatcher<TcpFrontend>& d,
            const size_t maxOpenConnections)
        : BaseTcpFrontend(io_s, headType, addr, balancerAddr, d, maxOpenConnections, { }, false)
    { }

    TcpFrontend(const TcpFrontend& ) = delete;
    TcpFrontend& operator=(const TcpFrontend& ) = delete;

    TcpFrontend(TcpFrontend&& ) = delete;
    TcpFrontend& operator=(TcpFrontend&& ) = delete;
};

//-----------------------------------------------------------------------

class HttpFrontend :
    public bf::BasePlainHttpFrontend< Dispatcher< HttpFrontend > >
{
public:
    HttpFrontend(
            boost::asio::io_service& io_s,
            const uint8_t headType,
            const std::string& addr,
            const unsigned short port,
            Dispatcher< HttpFrontend >& d,
            const size_t
    );

    ~HttpFrontend() override = default;
};

//-----------------------------------------------------------------------

class HttpSSLFrontend :
    public bf::BaseHttpFrontend< boost::asio::ssl::stream<boost::asio::ip::tcp::socket>, Dispatcher< HttpSSLFrontend > >
{
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SocketType;

public:
    HttpSSLFrontend(
            boost::asio::io_service& io_s,
            const uint8_t headType,
            const std::string& addr,
            const unsigned short port,
            Dispatcher<HttpSSLFrontend>& d,
            const std::string& certificateFileName,
            const std::string& privateKeyFileName,
            const std::string& dhParamsFileName
    );

    ~HttpSSLFrontend();

private:
    std::string getPasswd() const {
        return "";
    }

    void accept();

    void handleAccept(const boost::system::error_code& e);

    void handleHandShake(
            const std::shared_ptr<Connection<SocketType> >& conn,
            const boost::system::error_code& e
    );

private:
    std::shared_ptr<Connection<SocketType> > newConnection_;
    boost::asio::ssl::context context_;
};

//-----------------------------------------------------------------------

} //namespace Dispatcher

#endif //_SERVERLIB_DISPATCHER_FRONTEND_H_
