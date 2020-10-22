//
// C++ Interface: TlgLogger
//
// Description: logger for telegrams
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
//
//
#pragma once
#include <sstream>
#include <memory>

class LogWriter;

class TlgLogger
{
public:
    explicit TlgLogger() = default;
    TlgLogger(const TlgLogger&) = delete;
    TlgLogger(TlgLogger&&); // no implementation due to copy elision
    ~TlgLogger();

    template<typename T>
    TlgLogger& operator<<(const T& t) {
        stream_ << t;
        return *this;
    }

    static void setLogging(const char* fileName = "tlg.log", const bool writeToRegularLog = true);
private:
    void flush();

    static std::unique_ptr<LogWriter> lw_;
    static bool writeToRegularLog_;

    std::stringstream stream_;
};

inline TlgLogger LogTlg()
{
    return TlgLogger();
}
