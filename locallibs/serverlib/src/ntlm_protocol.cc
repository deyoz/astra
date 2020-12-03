#include "ntlm_protocol.h"
#include "ntlm_common.h"
#include "stream_holder.h"

#define NICKNAME "EFREMOV"
#include "slogger.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>

#include <regex>

namespace bsy = boost::system;
namespace bio = boost::asio;
using tcp = boost::asio::ip::tcp;

//-----------------------------------------------------------------------------

namespace {

enum class NtlmError {
    NullPointer = 1,
    InvalidType,
    RequireHttp11,
    Status401NotFound,
    WWWAuthenticateNotFound,
    Type2MsgNotFound
};

struct NtlmErrorCategory : public boost::system::error_category
{
    const char *name() const noexcept final { return "ntlm"; }
    std::string message(int ev) const final
    {
        switch (static_cast<NtlmError>(ev)) {
        case NtlmError::NullPointer: return "ntlm_null_pointer";
        case NtlmError::InvalidType: return "ntlm_invalid_type";
        case NtlmError::RequireHttp11: return "ntlm_require_http_1_1";
        case NtlmError::Status401NotFound: return "ntlm_status_401_not_found";
        case NtlmError::WWWAuthenticateNotFound: return "ntlm_www_authenticate_not_found";
        case NtlmError::Type2MsgNotFound: return "ntlm_type2_msg_not_found";
        default: return "ntlm_unknown";
        }
    }
};

boost::system::error_code makeError(NtlmError ec)
{
    static NtlmErrorCategory category;
    return {static_cast<int>(ec), category};
}

void registerRelation()
{
    boost::serialization::void_cast_register<httpsrv::protocol::ntlm::V2Data, httpsrv::protocol::Data>
        (static_cast<httpsrv::protocol::ntlm::V2Data *>(nullptr), static_cast<httpsrv::protocol::Data *>(nullptr));
}

bool checkHttpVersion(const std::string &header)
{
    static const std::regex re("\\s+HTTP\\/([1-9].[1-9])\r?\n");
    std::smatch sm;

    if (std::regex_search(header, sm, re))
        return true;
    else
        return false;
}

} // namespace

//-----------------------------------------------------------------------------

namespace {

const std::string RNRN = "\r\n\r\n";

enum class Stage {
    NoAuth,
    Hello,
    Type1,
    Type3
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

class Service : public std::enable_shared_from_this<Service> {
public:
    Service(sirena_net::stream_holder &holder,
            const httpsrv::HttpReq &req,
            httpsrv::protocol::ntlm::WriteHandler &&handler)
        : holder_(holder),
          req_(req),
          handler_(handler),
          stage_(Stage::NoAuth),
          mdata_()
    {
        auto pos = req_.text.find(RNRN);
        if (pos != std::string::npos) {
            reqHeader_ = req_.text.substr(0, pos);
            reqContent_ = req_.text.substr(pos + RNRN.size());
        }

        auto ptr = std::static_pointer_cast<httpsrv::protocol::ntlm::V2Data>(req_.protocolData);
        mdata_.ascDomain = ptr->domain;
        mdata_.ascUsername = ptr->username;
        mdata_.ascPassword = ptr->password;
    }

    void start()
    {
        onReadComplete();
    }

private:
    void readHeader()
    {
        auto ptr = shared_from_this();
        bio::async_read_until(holder_, sbuf_, RNRN, [ptr](const auto &ec, auto bytes) {
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

    void onReadHeader(const bsy::error_code &ec, size_t /*bytes*/, size_t headerLen)
    {
        if (ec) {
            handler_(ec);
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
            oss << reqHeader_ << "\r\n\r\n";
            oss << reqContent_;

            stage_ = Stage::Hello;
            writeData(oss.str());
        }

        else if (stage_ == Stage::Hello) {
            static const std::string STATUS = "401 Unauthorized";
            static const std::string NEGO = "WWW-Authenticate: Negotiate";
            static const std::string NTLM = "WWW-Authenticate: NTLM";

            if (header_.find(STATUS) == std::string::npos) {
                handler_(makeError(NtlmError::Status401NotFound));
                return;
            }

            if (header_.find(NEGO) == std::string::npos && header_.find(NTLM) == std::string::npos) {
                handler_(makeError(NtlmError::WWWAuthenticateNotFound));
                return;
            }

            std::ostringstream oss;
            oss << reqHeader_ << "\r\n";
            oss << "Authorization: NTLM " << type1msgFunc(mdata_) << "\r\n\r\n";
            oss << reqContent_;

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
                handler_(makeError(NtlmError::Type2MsgNotFound));
                return;
            }

            type2msgFunc(encoded, mdata_);

            std::ostringstream oss;
            oss << reqHeader_ << "\r\n";
            oss << "Authorization: NTLM " << type3msgFunc(mdata_) << "\r\n\r\n";
            oss << reqContent_;

            stage_ = Stage::Type3;
            writeData(oss.str());
        }
    }

    void onWriteComplete(const bsy::error_code &ec, size_t /*bytes*/)
    {
        if (ec) {
            handler_(ec);
            return;
        }

        if (stage_ == Stage::Hello || stage_ == Stage::Type1)
        {
            readData();
            return;
        }

        if (stage_ == Stage::Type3) {
            handler_({});
        }
    }

private:
    sirena_net::stream_holder &holder_;
    httpsrv::HttpReq req_;
    httpsrv::protocol::ntlm::WriteHandler handler_;

    Stage stage_;
    MsgData mdata_;

    bio::streambuf sbuf_;
    std::string header_;
    std::string content_;

    std::string reqHeader_;
    std::string reqContent_;
};

} // namespace

//-----------------------------------------------------------------------------

namespace httpsrv::protocol::ntlm {

V2Data::V2Data(const std::string &domain, const std::string &username, const std::string &password)
    : domain(domain), username(username), password(password)
{
}

template <typename Archive>
void serialize(Archive &, V2Data &, const unsigned)
{
}

template <typename Archive>
void save_construct_data(Archive &ar, const V2Data *ob, const unsigned)
{
    registerRelation();

    ar & ob->domain;
    ar & ob->username;
    ar & ob->password;
}

template <typename Archive>
void load_construct_data(Archive &ar, V2Data *ob, const unsigned)
{
    registerRelation();

    std::string domain, username, password;
    ar & domain;
    ar & username;
    ar & password;

    ::new(ob) V2Data(domain, username, password);
}

void usev2(sirena_net::stream_holder &holder, const HttpReq &req, WriteHandler &&handler)
{
    if (req.protocolData == nullptr) {
        LogTrace(TRACE5) << __FUNCTION__ << ": error: nullptr";
        handler(makeError(NtlmError::NullPointer));
        return;
    }

    if (req.protocolData->type() != httpsrv::protocol::Type::NTLMv2) {
        LogTrace(TRACE5) << __FUNCTION__ << ": error: type=" << (int)req.protocolData->type();
        handler(makeError(NtlmError::InvalidType));
        return;
    }

    if (checkHttpVersion(req.text) == false) {
        LogTrace(TRACE5) << __FUNCTION__ << ": error: require http 1.1";
        handler(makeError(NtlmError::RequireHttp11));
        return;
    }

    auto ptr = std::make_shared<Service>(holder, req, std::move(handler));
    LogTrace(TRACE5) << __FUNCTION__ << ": start service";
    ptr->start();
}

} // namespace httpsrv::protocol::ntlm

BOOST_CLASS_EXPORT_IMPLEMENT(httpsrv::protocol::ntlm::V2Data)
