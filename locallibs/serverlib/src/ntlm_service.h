#pragma once

#include <functional>

namespace boost::system { class error_code; }
namespace sirena_net { class stream_holder; }

namespace httpsrv::ntlm {

using AuthenticateHandler = std::function<void(const boost::system::error_code &)>;
void authenticate(sirena_net::stream_holder &holder, AuthenticateHandler &&handler);

} // namespace httpsrv::ntlm
