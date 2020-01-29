#pragma once

#include <functional>
#include <vector>

namespace HelpCpp {

class scope_guard
{
    std::vector< std::function<void()> > fns;
public:
    scope_guard() = default;
    scope_guard(const scope_guard&) = delete;

    scope_guard& attach(const std::function<void()>& f)
    {
        fns.emplace(fns.begin(), f);
        return *this;
    }

    scope_guard& attach(std::function<void()>&& f)
    {
        fns.emplace(fns.begin(), std::move(f));
        return *this;
    }

    ~scope_guard()
    {
        for (const auto& f : fns) {
            f();
        }
    }

    void dismiss()
    {
        fns.clear();
    }
};

} //HelpCpp
