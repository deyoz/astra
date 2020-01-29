#pragma once

namespace ServerFramework
{

class noncopyable
{
protected:
    noncopyable() = default;
    ~noncopyable() = default;

    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

} // namespace ServerFramework
namespace comtech = ServerFramework;
