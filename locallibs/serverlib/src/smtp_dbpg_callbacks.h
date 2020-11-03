#pragma once
#include "smtp_db_callbacks.h"

namespace PgCpp {
  namespace details {
    class SessionDescription;
  }
}

namespace SMTP {

class EmailMsgDbPgCallbacks: public EmailMsgDbCallbacks
{
    PgCpp::details::SessionDescription *sd;
public:
    virtual std::list<msg_id_length> msgListToSend(time_t interval,const size_t loop_max_count) const override;
    virtual std::vector<char> readMsg(const std::string &id, unsigned size) const override;
    virtual void markMsgSent(const std::string &id) const override;
    virtual void markMsgSentError(const std::string &id, int err_code, const std::string &error_text) const override;
    virtual std::string saveMsg(const std::string& txt, const std::string& type, bool send_now) const override;
    virtual void markMsgForSend(const std::string &id) const override;
    virtual void deleteDelayed(const std::string &id) const override;
    virtual bool processEnable() const override;
    virtual void commit() const override;

    EmailMsgDbPgCallbacks(PgCpp::details::SessionDescription *sd):sd(sd) {}
};

} //namespace SMTP
