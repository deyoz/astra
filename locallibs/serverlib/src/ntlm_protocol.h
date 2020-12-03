#pragma once

#include "httpsrv.h"
#include <boost/serialization/export.hpp>
#include <functional>

namespace boost::system { class error_code; }
namespace sirena_net { class stream_holder; }

namespace httpsrv::protocol::ntlm {

class V2Data : public httpsrv::protocol::Data {
public:
    V2Data(const std::string &domain, const std::string &username, const std::string &password);
    Type type() const { return httpsrv::protocol::Type::NTLMv2; }
    std::string domain;
    std::string username;
    std::string password;
};

using WriteHandler = std::function<void(const boost::system::error_code &)>;
void usev2(sirena_net::stream_holder &holder, const HttpReq &req, WriteHandler &&handler);

} // namespace httpsrv::protocol::ntlm

BOOST_CLASS_EXPORT_KEY(httpsrv::protocol::ntlm::V2Data)
