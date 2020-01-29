#ifndef __SERVERLIB_HTTP_LOGS_CALLBACKS_H_
#define __SERVERLIB_HTTP_LOGS_CALLBACKS_H_

#include <boost/date_time/posix_time/ptime.hpp>
#include <functional>
#include <map>
#include <vector>

namespace httpsrv { struct HttpReq; struct HttpResp; }
namespace ServerFramework { namespace HTTP { struct request; struct reply; } }

namespace ServerFramework {
namespace HttpLogs {

using httpsrv::HttpReq;
using httpsrv::HttpResp;
using ServerFramework::HTTP::request;
using ServerFramework::HTTP::reply;

// ==================================================================
// HttpReq, HttpResp

using HttpReqLogCallback = std::function<std::pair<int64_t, int32_t>(const HttpReq &, const std::map<std::string, std::string> &)>;
using HttpRespLogCallback = std::function<void(const HttpResp &)>;

void registerCallback(const HttpReqLogCallback &);
void registerCallback(const HttpRespLogCallback &);

std::pair<int64_t, int32_t> log(const HttpReq &req, const std::map<std::string, std::string> &data);
void log(const HttpResp &resp);

// ==================================================================
// HTTP::request, HTTP::reply

struct HTTPLogData {
    int64_t recid{0};
    int32_t importance{0};
    boost::posix_time::ptime reqdate;
    int32_t code{0};
};

using HTTPRequestLogCallback = std::function<HTTPLogData(const std::vector<uint8_t> &, const std::map<std::string, std::string> &)>;
using HTTPReplyLogCallback = std::function<void(const std::vector<uint8_t> &, const HTTPLogData &)>;

void registerCallback(const HTTPRequestLogCallback &);
void registerCallback(const HTTPReplyLogCallback &);

HTTPLogData log(const std::vector<uint8_t> &req, std::map<std::string, std::string> &&data);
void log(const std::vector<uint8_t> &resp, const HTTPLogData &logdata);

} // HttpLogs
} // ServerFramework

#endif // __SERVERLIB_HTTP_LOGS_CALLBACKS_H_
