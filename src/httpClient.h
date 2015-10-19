#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include "file_queue.h"
#include "astra_utils.h"

const std::string APIS_TR = "APIS_TR";

struct RequestInfo
{
  std::string host;
  uint16_t port;
  std::string method;
  std::string path;
  std::string action;
  std::string content;
  std::string login;
  std::string pswd;
  bool using_ssl;
  int timeout; // for request and answer timer in msec
  std::map<std::string,std::string> headers;
  RequestInfo() {
    using_ssl = false;
    method = "POST";
    timeout = 10000;
  }
};

struct ResponseInfo
{
  std::vector<std::string> headers;
  uint32_t status_code;
  std::string content;
  uint32_t content_length;
  bool pr_contentLength;
  bool ready;
  void clear() {
    headers.clear();
    status_code = 0;
    content_length = 0;
    pr_contentLength = false;
    content.clear();
    ready = false;
  }
  bool success() {
    return ready;
  }
  std::string toString() {
    std::ostringstream ss;
    ss << "status_code=" << status_code;
    if ( pr_contentLength ) {
      ss << ",content_length=" << content_length;
    }
    ss << ",ready=" << ready;
    return ss.str();
  }
};

class TApisTRFilter {
    private:
    public:
        static TFilterQueue *Instance() {
            static TFilterQueue *_instance = 0;
            if ( !_instance ) {
                _instance = new TFilterQueue( OWN_POINT_ADDR(), APIS_TR, ASTRA::NoExists, ASTRA::NoExists, false, 10 );
            }
            return _instance;
        }
};

void httpClient_main(const RequestInfo& request, ResponseInfo& response);
void send_apis_tr();
void process_reply (const std::string& result);

#endif // HTTPCLIENT_H
