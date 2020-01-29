#pragma once

#include <functional>
#include <stdexcept>

namespace HelpCpp
{

template<typename T>
class CallWithFallback
{
public:
    CallWithFallback(std::function<T ()> f,
            std::function<T ()> fallback,
            std::function<void (const std::exception&)> onFail)
        : f_(f), fallback_(fallback), onFail_(onFail)
    {}
    T get() {
        try {
            return f_();
        } catch (const std::exception& e) {
            onFail_(e);
        } catch (...) {
            onFail_(std::runtime_error("CallWithFallback completely failed"));
        }
        return fallback_();
    }

private:
    std::function<T ()> f_, fallback_;
    std::function<void (const std::exception& e)> onFail_;
};

} // HelpCpp
