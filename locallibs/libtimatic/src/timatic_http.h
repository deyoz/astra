#pragma once

#include "timatic_response.h"

#include <memory>

namespace httpsrv { struct HttpResp; }

namespace Timatic {

class HttpData {
public:
    HttpData(const httpsrv::HttpResp &httpResp);

    const std::string header() const { return header_; }
    const std::string content() const { return content_; }
    int httpCode() const { return httpCode_; }
    const std::string &jsessionID() const { return jsessionID_; }
    const std::shared_ptr<const Response> response() const { return response_; }

    template <class T> T get() const {
        static_assert(std::is_base_of<Response, T>::value, "not response descendant");
        if (response_ == nullptr) throw std::runtime_error("response is null");
        if (response_->type() != T::ctype()) throw std::runtime_error("response wrong cast");
        return *std::static_pointer_cast<T>(response_);
    }

private:
    std::string header_;
    std::string content_;
    int httpCode_;
    std::string jsessionID_;
    std::shared_ptr<Response> response_;
};

} // Timatic
