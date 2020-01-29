#include <limits>
#include <cstring>
#include "fcgi.h"
#include "exception.h"
#include "fastcgi.h"

#define NICKNAME "MIKHAIL"
#include "slogger.h"

namespace ServerFramework
{
namespace Fcgi
{

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

uint16_t requestId(const std::vector<uint8_t>& body)
{
    if (body.size() < 4) {
        ProgTrace(TRACE0, "requestId failed: body.size=%zd", body.size());
        return 0;
    }
    return body[2] * 256 + body[3];
}

struct Request::Impl
{
    const std::vector<uint8_t>& body;

    Impl(const std::vector<uint8_t>& b);
    std::map<std::string,std::string> params() const;
    std::string stdin_text() const;
    uint16_t id() const;
};

Request::Impl::Impl(const std::vector<uint8_t>& b)
    : body(b)
{
    if(body.size() < sizeof(FCGI_BeginRequestRecord))
        throw comtech::Exception("too short fcgi incoming stream body");

    const FCGI_BeginRequestRecord* brr = reinterpret_cast<const FCGI_BeginRequestRecord*>(&body[0]);
    if (brr->header.version != FCGI_VERSION_1) {
        throw comtech::Exception("version != FCGI_VERSION_1");
    }
    if (brr->header.type != FCGI_BEGIN_REQUEST) {
        throw comtech::Exception("header.type != FCGI_BEGIN_REQUEST");
    }
    if (brr->body.roleB1 * 256 + brr->body.roleB0 != FCGI_RESPONDER) {
        throw comtech::Exception("body.role != FCGI_RESPONDER");
    }
}

template <typename T> T collect_data(uint8_t type, const std::vector<uint8_t>& b)
{
    T s;
    const uint8_t* p = b.data();
    while(p < b.data() + b.size())
    {
        const FCGI_Header* h = reinterpret_cast<const FCGI_Header*>(p);
        const uint16_t contentLength = h->contentLengthB1*256 + h->contentLengthB0;
        LogTrace(TRACE1)<<__FUNCTION__<<" : h = { type="<<static_cast<int>(h->type)<<" contentLength="<<contentLength<<" paddingLength="<<static_cast<int>(h->paddingLength)<<" }";
        p += sizeof(*h);
        if(h->type == type)
            s.insert(s.end(), p, p + contentLength);
        p += contentLength + h->paddingLength;
    }
    return s;
}

std::string Request::Impl::stdin_text() const
{
    return collect_data<std::string>(FCGI_STDIN, body);
}

std::map<std::string,std::string> Request::Impl::params() const
{
    std::vector<uint8_t> b = collect_data< std::vector<uint8_t> >(FCGI_PARAMS, body);

    if(b.empty())
        return std::map<std::string,std::string>();
    else if(b.size()<2)
        throw comtech::Exception(" Request::Impl::params() :: b.size()<2");

    std::map<std::string,std::string> p;

    do
    {
        uint8_t k = b[0], v = b[1];
        LogTrace(TRACE5)<< "k="<<k<<" v="<<v;
        p[ std::string(b.data()+2, b.data()+2+k) ] = std::string(b.data()+2+k, b.data()+2+k+v);
        b.erase(b.begin(), b.begin()+2+k+v);
    }
    while(not b.empty());

    return p;
}

uint16_t Request::Impl::id() const
{
    return requestId(body);
}

//-----------------------------------------------------------------------

Request::Request(const std::vector<uint8_t>& body)
    : impl(new Request::Impl(body))
{}

Request::~Request()
{
    delete impl;
}

std::map<std::string,std::string> Request::params() const
{
    return impl->params();
}

std::string Request::stdin_text() const
{
    return impl->stdin_text();
}

uint16_t Request::id() const
{
    return impl->id();
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

void push_data(uint8_t t, uint16_t req_id, std::vector<uint8_t>& buf, const void* data, size_t size)
{
    do
    {
        const size_t chunk_size = std::min(static_cast<size_t>( std::numeric_limits<uint16_t>::max() ), size);

        const FCGI_Header h = {  FCGI_VERSION_1, t, static_cast<uint8_t>(req_id>>8), static_cast<uint8_t>(req_id&0xff),
                                                    static_cast<uint8_t>(chunk_size>>8), static_cast<uint8_t>(chunk_size&0xff),
                                                    static_cast<uint8_t>((8-chunk_size%8)%8)  };

        const size_t old_buf_size = buf.size();
        buf.resize(old_buf_size + sizeof(h) + chunk_size + h.paddingLength);

        std::memcpy(buf.data() + old_buf_size, &h, sizeof(h));
        std::memcpy(buf.data() + old_buf_size + sizeof(h), data, chunk_size);

        size -= chunk_size;
        data = static_cast<const uint8_t*>(data) + chunk_size;
    }
    while(size);
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

struct StdOutStream : public Stream
{
    uint8_t type() const {  return FCGI_STDOUT;  }
    StdOutStream(uint16_t r, std::vector<uint8_t>& b) : Stream(r,b) {}
    ~StdOutStream() {  push_data(type(), req_id, buf, NULL, 0);  }
};

struct StdErrStream : public Stream
{
    uint8_t type() const {  return FCGI_STDERR;  }
    StdErrStream(uint16_t r, std::vector<uint8_t>& b) : Stream(r,b) {}
    ~StdErrStream() {  push_data(type(), req_id, buf, NULL, 0);  }
};

//-----------------------------------------------------------------------

Stream& Stream::operator<<(const char* s)
{
    push_data(type(), req_id, buf, s, strlen(s));
    return *this;
}

Stream& Stream::operator<<(const std::string& s)
{
    push_data(type(), req_id, buf, s.data(), s.size());
    return *this;
}

Stream& Stream::operator<<(const std::vector<char>& v)
{
    push_data(type(), req_id, buf, v.data(), v.size());
    return *this;
}

Stream& Stream::operator<<(const std::vector<uint8_t>& v)
{
    push_data(type(), req_id, buf, v.data(), v.size());
    return *this;
}

Stream::Stream(uint16_t r, std::vector<uint8_t>& b)
    : req_id(r), buf(b)
{
}

Stream::~Stream()
{
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

struct AppExitCode
{
    uint32_t value;
    uint16_t req_id;
    uint8_t status;
    std::vector<uint8_t>& out;

    AppExitCode(uint8_t s, uint16_t r, std::vector<uint8_t>& o)
        : value(0), req_id(r), status(s), out(o) {}
    ~AppExitCode()
    {
        FCGI_EndRequestBody e = { static_cast<uint8_t>(value>>24), static_cast<uint8_t>((value>>16)&0xff),
                                  static_cast<uint8_t>((value>>8)&0xff), static_cast<uint8_t>(value&0xff), status };
        push_data(FCGI_END_REQUEST, req_id, out, &e, sizeof(e));
    }
};

struct Response::Impl
{
    Impl(uint16_t req_id, std::vector<uint8_t>& out)
        : app_exit_code(FCGI_REQUEST_COMPLETE, req_id, out),
          std_out(req_id,out),
          std_err(req_id,out)
    {}
    ~Impl()
    {}

    AppExitCode app_exit_code;

    StdOutStream std_out;
    StdErrStream std_err;
};

//-----------------------------------------------------------------------

Response::Response(uint16_t r, std::vector<uint8_t>& a)
    : impl(new Response::Impl(r,a))
{}

Response::~Response()
{
    delete impl;
}

Stream& Response::StdOut()
{
    return impl->std_out;
}

Stream& Response::StdErr()
{
    return impl->std_err;
}

void Response::set_status(uint8_t c)
{
    impl->app_exit_code.status = c;
}

void Response::set_app_exit_code(uint32_t c)
{
    impl->app_exit_code.value = c;
}

} // Fcgi
} // ServerFramework

