#pragma once

#include <vector>
#include <string>
#include <functional>
#include <iosfwd>

#include "lngv.h"
#include "message_fwd.h"

#define _(str) message_details::MsgCnt(str)

#define CALL_MSG_RET( expr ) if( Message const msg = ( expr ) ) { LogTrace( TRACE1 ) << msg; return msg; }

class Message
{
public:
    Message();
    Message(const char* nick, const char* file, int line, const message_details::MsgCnt& msg, bool error = true);
    ~Message();

    operator bool() const { return error_; }

    Message& tag(const std::string&);
    const std::vector<std::string>& tags() const;

    template<typename T>
    Message& bind(const T& v) {
        bindings_.push_back(std::bind(message_details::translate<T>, std::placeholders::_1, v));
        return *this;
    }
    Message& bind(const char* v) {
        bindings_.push_back(std::bind(message_details::translate<std::string>, std::placeholders::_1, std::string(v)));
        return *this;
    }
    std::string toString(const UserLanguage&) const;
    friend std::ostream& operator<<(std::ostream&, const Message&);
private:
    typedef std::function<std::string (const UserLanguage&)> TrFunc_t;
    std::string msg_;
    std::vector<TrFunc_t> bindings_;
    const char* file_;
    int line_;
    bool error_;
    std::vector<std::string> tags_;
    friend std::ostream& operator<<(std::ostream&, const Message&);
};
