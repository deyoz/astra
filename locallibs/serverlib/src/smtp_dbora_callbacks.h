#pragma once
#include "smtp_db_callbacks.h"

namespace OciCpp {
    class CursCtl;
    class Curs8Ctl;
}

namespace SMTP {

class EmailMsgDbOraCallbacks: public EmailMsgDbCallbacks
{
public:
    typedef OciCpp::CursCtl (*MakeCursFunction)(const char*, const char*, int, const std::string&);
    typedef OciCpp::Curs8Ctl (*MakeCurs8Function)(const char*, const char*, int, const std::string&);
    typedef void (*CommitFunction)(void);

    virtual std::list<msg_id_length> msgListToSend(time_t interval,const size_t loop_max_count) const override;
    virtual std::vector<char> readMsg(const std::string &id, unsigned size) const override;
    virtual void markMsgSent(const std::string &id) const override;
    virtual void markMsgSentError(const std::string &id, int err_code, const std::string &error_text) const override;
    virtual std::string saveMsg(const std::string& txt, const std::string& type, bool send_now) const override;
    virtual void markMsgForSend(const std::string &id) const override;
    virtual void deleteDelayed(const std::string &id) const override;
    virtual bool processEnable() const override;
    virtual void commit() const override;

    EmailMsgDbOraCallbacks(MakeCursFunction specialCurs_, MakeCurs8Function specialCurs8_, CommitFunction specialCommit_);
private:
    MakeCursFunction specialCurs_;
    MakeCurs8Function specialCurs8_;
    CommitFunction specialCommit_;

    OciCpp::CursCtl makeSpecialCursor(const std::string& query) const;
    OciCpp::Curs8Ctl makeSpecialCursor8(const std::string& query) const;
    void specialCommit() const;
};

} //namespace SMTP
