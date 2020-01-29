#include <array>
#if HAVE_CONFIG_H
#endif

#include "ehelpsig.h"
#include "posthooks.h"
#include "tclmon.h"
#include "EdiHelpManager.h"
#include "internal_msgid.h"
#include "serverlib/str_utils.h"

#define NICKNAME "SYSTEM"
#include "slogger.h"

#ifdef XP_TESTING

#include "testmode.h"
#include "checkunit.h"
#include "exception.h"
#include "func_placeholders.h"

static std::string saved_signal;

namespace ServerFramework {

std::string signal_matches_config_for_perespros(const std::string& s) {  return saved_signal == s ? std::string() : saved_signal;  }

void setSavedSignal(const std::string& sigtext_)
{
    saved_signal = sigtext_;
}

static std::string FP_set_saved_signal(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 1);

    setSavedSignal(p.at(0));

    return {};
}

FP_REGISTER("set_saved_signal", FP_set_saved_signal)

}

#endif // XP_TESTING

using namespace Posthooks;

EdiHelpSignal * EdiHelpSignal::clone() const
{
    return new EdiHelpSignal(*this);
}
bool EdiHelpSignal::less2(const BaseHook *p) const noexcept
{
    const EdiHelpSignal *e = dynamic_cast<const EdiHelpSignal *>(p);
    int compare = strcmp(ADDR, e->ADDR);
    if (compare < 0)
        return true;
    else if (compare > 0)
        return false;
    compare = strcmp(sigtext, e->sigtext);
    if (compare < 0)
        return true;
    else if (compare > 0)
        return false;
    return (memcmp(msg1, e->msg1, sizeof(msg1)) < 0);
}

void EdiHelpSignal::run()
{
    std::array<uint32_t,3> id;
    std::copy(msg1+1, msg1+4, id.begin());
    ServerFramework::InternalMsgId msgid(id);
#ifdef XP_TESTING
    if (inTestMode()) 
    {
        saved_signal = sigtext;
        ProgTrace(TRACE1,"\"sending\" in test mode %s %.20s (%s)", msgid.asString().c_str(), sigtext, ADDR);
        return;
    }
#endif
    char buf[max_buf_size];
    memcpy(buf, msg1, sizeof(msg1));
    strcpy(buf + sizeof(msg1), sigtext);
    send_signal(ADDR, buf, sizeof(msg1) + strlen(sigtext) + 1);
    ProgTrace(TRACE1,"sent: %s %.20s (%s)", msgid.asString().c_str(), sigtext, ADDR);
}

EdiHelpSignal::EdiHelpSignal(const ServerFramework::InternalMsgId& msg, const std::string& adr, const std::string& txt)
{
    msg1[0]=htonl(1);
    std::copy(msg.id().begin(), msg.id().end(), msg1+1);
    safe_strcpy(sigtext, txt);
    safe_strcpy(ADDR, adr);
}

