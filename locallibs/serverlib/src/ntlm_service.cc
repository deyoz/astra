#include "ntlm_service.h"
#include "ntlm_msg.h"
#include "stream_holder.h"

#include <boost/asio.hpp>

#include <iostream>
#include <memory>
#include <regex>

namespace bsy = boost::system;
namespace bio = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace {

const std::string HOST_STR = "testfares4x.pobeda.aero";
const std::string DOMAIN_STR = "Aeroweb";
const std::string USERNAME_STR = "sirenaapi";
const std::string PASSWORD_STR = "v1|VF@3x2y8hUGeRc6R2";
const std::string DELIM_STR = "\r\n\r\n";

enum class Stage {
    NoAuth,
    Hello,
    Type1,
    Type3,
    Auth
};

std::string getData(bio::streambuf &sbuf, size_t len)
{
    if (len > sbuf.size())
        len = sbuf.size();

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

class AsyncService : public std::enable_shared_from_this<AsyncService> {
public:
    AsyncService(sirena_net::stream_holder &holder, httpsrv::ntlm::AuthenticateHandler &&handler)
        : holder_(holder), handler_(std::move(handler)), stage_(Stage::NoAuth), mdata_()
    {
        mdata_.ascDomain = DOMAIN_STR;
        mdata_.ascUsername = USERNAME_STR;
        mdata_.ascPassword = PASSWORD_STR;
    }

    void authenticate()
    {
        onReadComplete();
    }

private:
    void readHeader()
    {
        auto ptr = shared_from_this();
        bio::async_read_until(holder_, sbuf_, DELIM_STR, [ptr](const auto &ec, auto bytes) {
            ptr->onReadHeader(ec, bytes, bytes);
        });
    }

    void readContent(size_t contentLen)
    {
        size_t atleast = contentLen - sbuf_.size();

        if (atleast > 0) {
            auto ptr = shared_from_this();
            bio::async_read(holder_, sbuf_, bio::transfer_at_least(atleast), [ptr, contentLen](const auto &ec, auto bytes) {
                ptr->onReadContent(ec, bytes, contentLen);
            });
        } else {
            onReadContent({}, sbuf_.size(), contentLen);
        }
    }

    void readData()
    {
        header_ = {};
        content_ = {};
        readHeader();
    }

    void writeData(const std::string &str)
    {
        auto ptr = shared_from_this();

        if (holder_.is_ssl())
            bio::async_write(holder_.ssl_stream(), bio::buffer(str), [ptr](const auto &ec, auto bytes) {
                ptr->onWriteComplete(ec, bytes);
            });
        else
            bio::async_write(holder_.stream(), bio::buffer(str), [ptr](const auto &ec, auto bytes) {
                ptr->onWriteComplete(ec, bytes);
            });
    }

    //

    void onReadHeader(const bsy::error_code &ec, size_t /*bytes*/, size_t headerLen)
    {
        if (ec) {
            if (headerLen == 0) handler_({}); // EOF
            else handler_(ec);
            return;
        }

        header_ = getData(sbuf_, headerLen);

        if (size_t contentLen = getContentLen(header_))
            readContent(contentLen);
        else
            onReadComplete();
    }

    void onReadContent(const bsy::error_code &ec, size_t /*bytes*/, size_t contentLen)
    {
        if (ec) {
            handler_(ec);
            return;
        }

        content_ = getData(sbuf_, contentLen);
        onReadComplete();
    }

    void onReadComplete()
    {
        if (stage_ == Stage::NoAuth) {
            std::ostringstream oss;
            oss << "GET / HTTP/1.1" << "\r\n";
            oss << "Host: " << HOST_STR << "\r\n";
            oss << "Accept: */*" << "\r\n";
            oss << "\r\n";

            stage_ = Stage::Hello;
            writeData(oss.str());
        }

        else if (stage_ == Stage::Hello) {
            static const std::string STATUS = "HTTP/1.1 401 Unauthorized";
            static const std::string NEGO = "WWW-Authenticate: Negotiate";
            static const std::string NTLM = "WWW-Authenticate: NTLM";

            if (header_.find(STATUS) == std::string::npos) {
                handler_(bsy::error_code(bsy::errc::invalid_argument, bsy::generic_category()));
                return;
            }

            if (header_.find(NEGO) == std::string::npos && header_.find(NTLM) == std::string::npos) {
                handler_(bsy::error_code(bsy::errc::not_supported, bsy::generic_category()));
                return;
            }

            std::ostringstream oss;
            oss << "GET / HTTP/1.1" << "\r\n";
            oss << "Host: " << HOST_STR << "\r\n";
            oss << "Authorization: NTLM " << type1msgFunc(mdata_) << "\r\n";
            oss << "Accept: */*" << "\r\n";
            oss << "\r\n";

            stage_ = Stage::Type1;
            writeData(oss.str());
        }

        else if (stage_ == Stage::Type1) {
            static const std::regex re("WWW-Authenticate:\\sNTLM\\s(\\S+)");

            std::string encoded;
            std::smatch sm;
            std::regex_search(header_, sm, re);
            if (sm.ready())
                encoded = sm.str(1);

            if (encoded.empty()) {
                handler_(bsy::error_code(bsy::errc::bad_address, bsy::generic_category()));
                return;
            }

            type2msgFunc(encoded, mdata_);

            std::ostringstream oss;
            oss << "GET /FareSearch?clientId=sirena_R4QK0QXOKSU4BMC HTTP/1.1" << "\r\n";
            oss << "Host: " << HOST_STR << "\r\n";
            oss << "Authorization: NTLM " << type3msgFunc(mdata_) << "\r\n";
            oss << "Accept: */*" << "\r\n";
            oss << "\r\n";

            stage_ = Stage::Type3;
            writeData(oss.str());
        }

        else if (stage_ == Stage::Type3) {
            static const std::string STATUS = "HTTP/1.1 200 OK";

            if (header_.find(STATUS) == std::string::npos) {
                handler_(bsy::error_code(bsy::errc::file_exists, bsy::generic_category()));
                return;
            }

            stage_ = Stage::Auth;
            handler_({});
        }
    }

    void onWriteComplete(const bsy::error_code &ec, size_t /*bytes*/)
    {
        if (ec) {
            handler_(ec);
            return;
        }

        if (stage_ == Stage::Hello || stage_ == Stage::Type1 || stage_ == Stage::Type3) {
            readData();
            return;
        }

        handler_({});
    }

    sirena_net::stream_holder &holder_;
    httpsrv::ntlm::AuthenticateHandler handler_;
    Stage stage_;
    bio::streambuf sbuf_;
    std::string header_;
    std::string content_;
    MsgData mdata_;
};

} // namespace

//-----------------------------------------------------------------------------

namespace httpsrv::ntlm {

void authenticate(sirena_net::stream_holder &holder, AuthenticateHandler &&handler)
{
    auto service = std::make_shared<AsyncService>(holder, std::move(handler));
    service->authenticate();
}

} // namespace httpsrv::ntlm
