#pragma once

#include <iosfwd>
#include <string>

namespace HelpCpp
{

class Timer
{
public:
    Timer(bool startOnCreate = true);

    Timer(const std::string& name, bool startOnCreate = true);

    Timer(const Timer&) = delete;

    Timer(Timer&&) = delete;

    const std::string& name() const;

    void restart();

    long elapsedSeconds() const;

    long elapsedMilliseconds() const;

    long elapsedMicroseconds() const;

    friend std::ostream& operator<<(std::ostream& os, const Timer&);

private:
    std::string name_;
    long startTime_;
};

} // namespace HelpCpp
