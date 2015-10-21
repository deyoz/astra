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

enum THTTPErrorOperation { toUnknown, toDeadline, toResolve, toConnect, toHandshake, toWrite, toStatus, toReadHeaders, toAnswerReady, toReadContent };

std::string StrHTTPErrorOperation( const THTTPErrorOperation &operation );


/*ответ от сервера
 *  completed=true - пришел правильный ответ
 *  content_length - длина тела
 *  content - тело
 *  error_code - код ошибки - когда completed=true, тогда возможно 2 варианта error_code=0(succes) или err_code=end of file
 *  error_message - текст ошибки
 *  error_operation - при какой операции случилась ошибка
 */
struct ResponseInfo
{
  std::vector<std::string> headers;
  uint32_t status_code;
  std::string content;
  uint32_t content_length;
  bool pr_contentLength;
  bool completed;
  THTTPErrorOperation error_operation;
  int error_code;
  std::string error_message;
  void clear() {
    headers.clear();
    status_code = 0;
    content_length = 0;
    pr_contentLength = false;
    content.clear();
    completed = false;
    error_operation = toUnknown;
    error_message.clear();
  }
  std::string toString() {
    std::ostringstream ss;
    ss << "status_code=" << status_code << ",what=" << error_message << ",where=" + StrHTTPErrorOperation( error_operation );
    if ( pr_contentLength ) {
      ss << ",content_length=" << content_length;
    }
    ss << ",completed=" << completed;
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
