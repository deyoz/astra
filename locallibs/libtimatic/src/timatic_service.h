#pragma once

#include "timatic_http.h"
#include "timatic_request.h"
#include "timatic_response.h"

namespace ServerFramework { class InternalMsgId; }

namespace Timatic {

class Service {
public:
    Service(const Config &config);

    const Config &config() const { return config_; }
    const Session &session() const { return session_; }

    bool ready() const;
    void reset();

    void send(const Request &request, const ServerFramework::InternalMsgId &msgid);
    std::vector<HttpData> fetch(const ServerFramework::InternalMsgId &msgid);

private:
    Config config_;
    Session session_;
};

} // Timatic
