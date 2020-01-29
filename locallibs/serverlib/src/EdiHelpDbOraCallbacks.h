#pragma once
#include "serverlib/EdiHelpDbCallbacks.h"

namespace ServerFramework
{

class EdiHelpDbOraCallbacks : public EdiHelpDbCallbacks
{
public:
    virtual boost::optional<ConfirmInfo> confirm_notify_oraside(const char *pult, int session_id) const override;
    virtual boost::optional<ConfirmInfo> confirm_notify_oraside(const InternalMsgId& msgid, int session_id,
                                                                const std::string &instance_name) const override;
    virtual void clear_old_records() const override;
    virtual std::list<EdiHelp> select_all(const char *pult) const override;
    virtual std::list<EdiHelp> select_all(const InternalMsgId& msgid) const override;
    virtual boost::optional<EdiHelp> select_one(const InternalMsgId& msgid, int session_id) const override;
    virtual void create_db(const EdiHelp &eh, bool clear_other_intmsgid, bool autonomous = false) const override;

    virtual ~EdiHelpDbOraCallbacks() {}
};

} // namespace ServerFramework
