#pragma once
#include <boost/optional.hpp>
#include <list>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace ServerFramework
{
class InternalMsgId;

struct EdiHelp {
    std::array<uint32_t,3> id;
    char address[60];
    std::string txt;
    char instance[4];
    unsigned short binlen;
    int session_id;
    boost::posix_time::ptime timeout;
    std::string pult;
};

struct ConfirmInfo {
    unsigned leftover;
    std::string signalTxt;
    std::string instanceName;
    std::string address;
    std::array<uint32_t,3> id;
};

class EdiHelpDbCallbacks
{
    static EdiHelpDbCallbacks *Instance;
public:
    virtual boost::optional<ConfirmInfo> confirm_notify_oraside(const char *pult, int session_id) const = 0;
    virtual boost::optional<ConfirmInfo> confirm_notify_oraside(const InternalMsgId& msgid, int session_id,
                                                                const std::string &instance_name) const = 0;
    virtual std::list<EdiHelp> select_all(const char *pult) const = 0;
    virtual std::list<EdiHelp> select_all(const InternalMsgId& msgid) const = 0;
    virtual boost::optional<EdiHelp> select_one(const InternalMsgId& msgid, int session_id) const = 0;
    virtual void clear_old_records() const = 0;
    virtual void create_db(const EdiHelp &eh, bool clear_other_intmsgid, bool autonomous = false) const = 0;

    static EdiHelpDbCallbacks *instance();
    static void setEdiHelpDbCallbacks(EdiHelpDbCallbacks *cb);
    virtual ~EdiHelpDbCallbacks();
};

} // namespace ServerFramework
