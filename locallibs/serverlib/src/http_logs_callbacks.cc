#include "http_logs_callbacks.h"

namespace ServerFramework {
namespace HttpLogs {

// ==================================================================
// HttpReq, HttpResp

static HttpReqLogCallback httpReqLogCallback;
static HttpRespLogCallback httpRespLogCallback;

void registerCallback(const HttpReqLogCallback &cb)
{
    httpReqLogCallback = cb;
}

void registerCallback(const HttpRespLogCallback &cb)
{
    httpRespLogCallback = cb;
}

std::pair<int64_t, int32_t> log(const httpsrv::HttpReq &req, const std::map<std::string, std::string> &data)
{
    if (httpReqLogCallback)
        return httpReqLogCallback(req, data);
    else
        return {0, 0};
}

void log(const httpsrv::HttpResp &resp)
{
    if (httpRespLogCallback)
        httpRespLogCallback(resp);
}

// ==================================================================
// HTTP::request, HTTP::reply

static HTTPRequestLogCallback httpRequestLogCallback;
static HTTPReplyLogCallback httpReplyLogCallback;

void registerCallback(const HTTPRequestLogCallback &cb)
{
    httpRequestLogCallback = cb;
}

void registerCallback(const HTTPReplyLogCallback &cb)
{
    httpReplyLogCallback = cb;
}

HTTPLogData log(const std::vector<uint8_t> &req, std::map<std::string, std::string> &&data)
{
    if (httpRequestLogCallback)
        return httpRequestLogCallback(req, data);
    else
        return {};
}

void log(const std::vector<uint8_t> &resp, const HTTPLogData &logdata)
{
    if (httpReplyLogCallback)
        httpReplyLogCallback(resp, logdata);
}

} // HttpLogs
} // ServerFramework
