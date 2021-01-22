#include <boost/asio.hpp>
#include <cassert>
#include <iostream>
#include <memory>
#include <regex>

namespace bsy = boost::system;
namespace bio = boost::asio;
using tcp = boost::asio::ip::tcp;

//-----------------------------------------------------------------------------

namespace {

const std::string RNRN = "\r\n\r\n";
const std::string TYPE1_STR = "Authorization: NTLM TlRMTVNTUAABAAAABoIIAA==";
const std::string TYPE3_STR = "Authorization: NTLM TlRMTVNTUAADAAAAGAAYAEAAAADWANYAWAAAAAcABwAuAQAACQAJADUBAAAAAAAAPgEAAAAAAAAAAAAABoKJAof0mmfuoDxPerM3asZLidn///8AESIzRBnTnK9vfG7hTezyC0broeQBAQAAAAAAAADW1rwuEtYB////ABEiM0QAAAAAAgAQAEQATwBCAFIATwBMAEUAVAABAA4AQQBFAFIATwBXAEUAQgAEABwAZABvAGIAcgBvAGwAZQB0AC4AbABvAGMAYQBsAAMALABhAGUAcgBvAFcARQBCAC4AZABvAGIAcgBvAGwAZQB0AC4AbABvAGMAYQBsAAUAHABkAG8AYgByAG8AbABlAHQALgBsAG8AYwBhAGwABwAIAC3cap2EEtYBAAAAAAAAAABBZXJvd2Vic2lyZW5hYXBp";

const std::string NOAUTH_RESP =
        "HTTP/1.1 401 Unauthorized\r\n"
        "Content-Type: text/html\r\n"
        "Server: Microsoft-IIS/8.5\r\n"
        "WWW-Authenticate: Negotiate\r\n"
        "WWW-Authenticate: NTLM\r\n"
        "X-Powered-By: ASP.NET\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: 7\r\n\r\n"
        "[12345]";

const std::string TYPE2_RESP =
        "HTTP/1.1 401 Unauthorized\r\n"
        "Content-Type: text/html; charset=us-ascii\r\n"
        "Server: Microsoft-HTTPAPI/2.0\r\n"
        "WWW-Authenticate: NTLM TlRMTVNTUAACAAAACAAIADgAAAAGgokCX4f38wopztwAAAAAAAAAAKYApgBAAAAABgOAJQAAAA9ET0JST0xFVAIAEABEAE8AQgBSAE8ATABFAFQAAQAOAEEARQBSAE8AVwBFAEIABAAcAGQAbwBiAHIAbwBsAGUAdAAuAGwAbwBjAGEAbAADACwAYQBlAHIAbwBXAEUAQgAuAGQAbwBiAHIAbwBsAGUAdAAuAGwAbwBjAGEAbAAFABwAZABvAGIAcgBvAGwAZQB0AC4AbABvAGMAYQBsAAcACAAt3GqdhBLWAQAAAAA=\r\n"
        "Content-Length: 7\r\n\r\n"
        "[12345]";

const std::string AUTH_RESP =
        "HTTP/1.1 200 OK\r\n"
        "Cache-Control: no-cache\r\n"
        "Pragma: no-cache\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Expires: -1\r\n"
        "Server: Microsoft-IIS/8.5\r\n"
        "X-AspNet-Version: 4.0.30319\r\n"
        "Persistent-Auth: true\r\n"
        "X-Powered-By: ASP.NET\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: 2\r\n\r\n"
        "[]";

enum class Stage {
    NoAuth,
    Type2,
    Auth
};

enum class Behavior {
    Normal,
    Timeout
};

std::string getData(bio::streambuf &sbuf, size_t len)
{
    if (len > sbuf.size())
        throw std::runtime_error("getData len=" + std::to_string(len) + " sbuf.size=" + std::to_string(sbuf.size()));

    auto beg = bio::buffers_begin(sbuf.data());
    std::string data(beg, beg + len);
    sbuf.consume(len);

    return data;
}

size_t getContentLen(const std::string &header)
{
    static const std::regex re("Content-Length: (\\d+)\r?\n");
    std::smatch sm;

    if (std::regex_search(header, sm, re))
        return std::stoul(sm[1]);

    return 0;
}

class Conn : public std::enable_shared_from_this<Conn> {
public:
    Conn(bio::io_service &service, Behavior behavior)
        : stage_(Stage::NoAuth), socket_(service), behavior_(behavior)
    {
    }

    tcp::socket &socket() { return socket_; }
    void handle() { readData(); }

private:
    void readData()
    {
        header_ = {};
        content_ = {};
        readHeader();
    }

    void readHeader()
    {
        auto ptr = shared_from_this();
        bio::async_read_until(socket_, sbuf_, RNRN, [ptr](const auto &ec, auto bytes) {
            ptr->onReadHeader(ec, bytes, bytes);
        });
    }

    void readContent(size_t contentLen)
    {
        auto ptr = shared_from_this();
        bio::async_read(socket_, sbuf_, bio::transfer_at_least(contentLen), [ptr, contentLen](const auto &ec, auto bytes) {
           ptr->onReadContent(ec, bytes, contentLen);
        });
    }

    void writeData(const std::string &str)
    {
        auto ptr = shared_from_this();
        bio::async_write(socket_, bio::buffer(str), [ptr](const auto &ec, auto bytes) {
            ptr->onWriteComplete(ec, bytes);
        });
    }

    void onReadHeader(const bsy::error_code &ec, size_t /*bytes*/, size_t headerLen)
    {
        if (ec) {
            if (headerLen == 0) return; // EOF
            throw std::runtime_error(ec.message());
        }

        header_ = getData(sbuf_, headerLen);

        if (size_t contentLen = getContentLen(header_))
            readContent(contentLen);
        else
            onReadComplete();
    }

    void onReadContent(const bsy::error_code &ec, size_t /*bytes*/, size_t contentLen)
    {
        if (ec) throw std::runtime_error(ec.message());
        content_ = getData(sbuf_, contentLen);
        onReadComplete();
    }

    void onReadComplete()
    {
        if (stage_ == Stage::NoAuth) {
            if (header_.find(TYPE1_STR) == std::string::npos) {
                stage_ = Stage::NoAuth;
                writeData(NOAUTH_RESP);
            } else {
                stage_ = Stage::Type2;
                writeData(TYPE2_RESP);
            }
        }

        else if (stage_ == Stage::Type2) {
            if (behavior_ == Behavior::Timeout)
                sleep(20);
            if (header_.find(TYPE3_STR) == std::string::npos) {
                stage_ = Stage::NoAuth;
                writeData(NOAUTH_RESP);
            } else {
                stage_ = Stage::Auth;
                writeData(AUTH_RESP);
            }
        }

        else if (stage_ == Stage::Auth) {
            writeData(AUTH_RESP);
        }
    }

    void onWriteComplete(const bsy::error_code &ec, size_t /*bytes*/)
    {
        if (ec) throw std::runtime_error(ec.message());
        readData();
    }

    Stage stage_;
    tcp::socket socket_;
    Behavior behavior_;
    bio::streambuf sbuf_;
    std::string header_;
    std::string content_;
};

class Server {
public:
    Server(bio::io_service &service, int port, Behavior behavior)
        : service_(service), acceptor_(service, tcp::endpoint(tcp::v4(), port)), behavior_(behavior)
    {
        startAccept();
    }

private:
    void startAccept()
    {
        conns_.emplace_back(std::make_shared<Conn>(service_, behavior_));

        auto conn = conns_.back();
        acceptor_.async_accept(conn->socket(), [this, conn](const auto &ec) {
            this->handleAccept(ec, conn);
        });
    }

    void handleAccept(const bsy::error_code &ec, std::shared_ptr<Conn> conn)
    {
        if (ec) throw std::runtime_error(ec.message());
        conn->handle();
        //startAccept();
    }

    bio::io_service &service_;
    tcp::acceptor acceptor_;
    Behavior behavior_;
    std::vector<std::shared_ptr<Conn>> conns_;
};

int runTestServer(int argc, char **argv) try
{
    if (argc != 3)
        throw std::runtime_error("argc != 3");

    Behavior behavior = Behavior::Normal;
    if (std::strcmp(argv[2], "timeout") == 0)
        behavior = Behavior::Timeout;

    bio::io_service service;
    Server server(service, std::atoi(argv[1]), behavior);
    return service.run();
}
catch (const std::exception &ex)
{
    std::cerr << ex.what() << std::endl;
    return 1;
}

} // namespace

//-----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    return runTestServer(argc, argv);
}
