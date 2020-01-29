#pragma once
#include <string>
#include <vector>
#include <list>

namespace SMTP {

struct msg_id_length
{
    std::string id;
    unsigned length;
};

class EmailMsgDbCallbacks
{
public:
    virtual std::list<msg_id_length> msgListToSend(time_t interval,const size_t loop_max_count) const = 0;
    virtual std::vector<char> readMsg(const std::string &id, unsigned size) const = 0;
    virtual void markMsgSent(const std::string &id) const = 0;
    virtual void markMsgSentError(const std::string &id, int err_code, const std::string &error_text) const = 0;
    virtual std::string saveMsg(const std::string& txt, const std::string& type, bool send_now) const = 0;
    virtual void markMsgForSend(const std::string &id) const = 0;
    virtual void deleteDelayed(const std::string &id) const = 0;
    virtual bool processEnable() const = 0;
    virtual void commit() const = 0;

    virtual ~EmailMsgDbCallbacks(){};
};
} // namespace SMTP
