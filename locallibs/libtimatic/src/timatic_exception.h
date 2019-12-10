#pragma once

#include <serverlib/exception.h>

namespace Timatic {

class Error : public ServerFramework::Exception {
public:
    Error(BIGLOG_SIGNATURE, const char *fmt, ...);
    const std::string &nick() const { return nick_; }
    const std::string &file() const { return file_; }
    int line() const { return line_; }
    const std::string &func() const { return func_; }
    const char *what() const noexcept override { return msg_.c_str(); }

protected:
    Error(BIGLOG_SIGNATURE);
    std::string nick_;
    std::string file_;
    int line_;
    std::string func_;
    std::string msg_;
};

class SessionError : public Error {
public:
    SessionError(BIGLOG_SIGNATURE, const char *fmt, ...);
};

class ValidateError : public Error {
public:
    ValidateError(BIGLOG_SIGNATURE, const char *fmt, ...);
};

} // Timatic
