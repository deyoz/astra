#pragma once

#include <string>
#include <memory>
#include <type_traits>

#include "string_cast.h"

class Message;
class UserLanguage;
class EncString;
struct Period;
class Freq;

namespace boost { namespace gregorian {
class date;
class date_duration;
} }

namespace boost { namespace posix_time {
class ptime;
class time_duration;
} }

namespace message_details {

template <typename T>
std::string translate_impl(const UserLanguage&, const T&) = delete;

#define MSG_BINDER_DECL(Type) \
template<> std::string translate_impl<Type>(const UserLanguage&, const Type&);

#define MSG_BINDER_TRIVIAL_IMPL(Type) \
namespace message_details { \
template<> std::string translate_impl<Type>(const UserLanguage&, const Type& v) { \
    return HelpCpp::string_cast(v); \
} }

template<typename T>
typename std::enable_if<
        std::is_arithmetic<T>::value || std::is_same<std::string, T>::value,
        std::string
    >::type
translate(const UserLanguage&, const T& v)
{
    return HelpCpp::string_cast(v);
}

template<typename T>
typename std::enable_if<
        !(std::is_arithmetic<T>::value || std::is_same<std::string, T>::value),
        std::string
    >::type
translate(const UserLanguage& l, const T& v)
{
    return translate_impl(l, v);
}

MSG_BINDER_DECL(Message)
MSG_BINDER_DECL(EncString)

MSG_BINDER_DECL(Period)
MSG_BINDER_DECL(Freq)
MSG_BINDER_DECL(boost::gregorian::date)
MSG_BINDER_DECL(boost::posix_time::ptime)
MSG_BINDER_DECL(boost::posix_time::time_duration)
MSG_BINDER_DECL(boost::gregorian::date_duration)

struct MsgCnt
{
    const std::string& msg;
    explicit MsgCnt(const std::string& s) : msg(s) {}
};

struct Localization
{
    virtual std::string localize(const std::string&, const UserLanguage&) const = 0;
    virtual ~Localization() {}
};

void setLocalization(std::shared_ptr<Localization>);

} // message_details
