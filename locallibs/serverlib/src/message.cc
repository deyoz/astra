#if HAVE_CONFIG_H
#endif

#include <boost/format.hpp>

#include "message.h"
#include "exception.h"
#include "encstring.h"
#include "lngv_user.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

static std::shared_ptr<message_details::Localization> loc;

MSG_BINDER_TRIVIAL_IMPL(EncString)

namespace message_details {

template<> std::string translate_impl<Message>(const UserLanguage& lang, const Message& v)
{
    return v.toString(lang);
}

void setLocalization(std::shared_ptr<Localization> l)
{
    loc = l;
}
} // message_details

//#############################################################################
Message::Message() : file_(0), line_(0), error_(false)
{}

Message::Message(const char* nick, const char* file, int line, const message_details::MsgCnt& m, bool error)
    : msg_(m.msg), file_(file), line_(line), error_(error)
{}

Message::~Message() {}

Message& Message::tag(const std::string& tag)
{
    tags_.push_back(tag);
    return *this;
}

const std::vector<std::string>& Message::tags() const
{
    return tags_;
}

std::string Message::toString(const UserLanguage& lang) const
{
    if (msg_.empty()) {
        return msg_;
    }
    try {
        boost::format msgFmt(loc ? loc->localize(msg_, lang) : msg_);
        for(const TrFunc_t& f:  bindings_) {
            msgFmt % f(lang);
        }
        return msgFmt.str();
    } catch (const boost::io::format_error& e) {
        LogError(STDLOG) << "Invalid format: "
            << e.what() << " "
            << file_ << ":" << line_
            << " [" << msg_ << "]";
        throw ServerFramework::Exception("Message with invalid format.");
    }
}

std::ostream& operator<<(std::ostream& s, const Message& msg)
{
    if (msg) {
        s << "[" << msg.file_ << ":" << msg.line_ << "] " << msg.toString(UserLanguage::en_US());
    }
    return s;
}

#ifdef XP_TESTING
#include "xp_test_utils.h"
#include "checkunit.h"

START_TEST(primitiveTypes)
{
    fail_unless(Message(STDLOG, _("lol%1%ka")).bind(123).toString(UserLanguage::en_US()) == "lol123ka");
    fail_unless(Message(STDLOG, _("lol%1%ka")).bind(1.2).toString(UserLanguage::en_US()) == "lol1.2ka");
    fail_unless(Message(STDLOG, _("lo%1%a")).bind("lk").toString(UserLanguage::en_US()) == "lolka");
    fail_unless(Message(STDLOG, _("lol%1%a")).bind('k').toString(UserLanguage::en_US()) == "lolka");
    fail_unless(Message(STDLOG, _("lo%1%a")).bind(std::string("lk")).toString(UserLanguage::en_US()) == "lolka");
    Message msg(STDLOG, _("[inner msg %1%]"));
    msg.bind(321);
    msg = Message(STDLOG, _("bind self=[%1%]")).bind(msg);
    fail_unless(msg.toString(UserLanguage::en_US()) == "bind self=[[inner msg 321]]");
} END_TEST

#define SUITENAME "Message"
TCASEREGISTER(0, 0)
{
    ADD_TEST(primitiveTypes);
}
TCASEFINISH

void init_message_tests() {}

#endif // XP_TESTING
