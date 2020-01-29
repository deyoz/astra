#pragma once

#include <memory>
#include <functional>
#include <chrono>

namespace HelpCpp
{

template<typename T>
class TimeCaching
{
public:
    TimeCaching(const std::chrono::steady_clock::duration& td, std::function<T ()> f)
        : lifetime_(td), setter_(f), lastSetTm_(now() - (td + td))
    {}
    const T& get() const {
        if (!value_ || expired()) {
            setValue();
        }
        return *value_;
    }
private:
    bool expired() const {
        return ((now() - lastSetTm_) > lifetime_);
    }

    void setValue() const {
        value_.reset(new T(setter_()));
        lastSetTm_ = now();
    }

    static std::chrono::steady_clock::time_point now() {
        return std::chrono::steady_clock::now();
    }

    const std::chrono::steady_clock::duration lifetime_;
    const std::function<T ()> setter_;
    mutable std::chrono::steady_clock::time_point lastSetTm_;
    mutable std::unique_ptr<T> value_;
};

} // ServerFramework
