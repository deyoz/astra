#pragma once

#include "posthooks.h"
#include <string>
#include <functional>

namespace deffered_exec {

class RequestId {
public:
    const static std::size_t max_length {15};
    const std::string id;

public:
    template <typename T>
    RequestId(T&& id) : id(std::forward<T>(id)) {}

public:
    static RequestId create();
    bool exists() const;
    void save() const;
    void remove() const;
    bool operator<(const RequestId& v) const noexcept;
};

class PostHookRequest final : public Posthooks::BaseHook {
private:
    using Callable = std::function<void(void)>;
    Callable callable;
    RequestId id;

public:
    PostHookRequest(const PostHookRequest& req) = default;
    PostHookRequest(PostHookRequest&& req) = default;
    PostHookRequest(const Callable& callable);

public:
    PostHookRequest* clone() const override;
    void run() override;
    bool less2(const Posthooks::BaseHook* v) const noexcept override;
    bool operator<(const Posthooks::BaseHook* v) const noexcept;
};

template <typename Callable, typename... Args>
void call_after_commit(Callable&& request, Args&& ...args) {
    // there is a buf in gcc in expanding parameter pack in lambda.
    // auto fn = [request, args...] () mutable { request(args...); };
    auto fn = std::bind(request, args...);
    Posthooks::sethAfter(PostHookRequest(fn));
}

} /* namespace deffered_exec */
