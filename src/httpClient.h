#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <cstdlib>
#include <string>
#include <vector>
#include <map>

#include <serverlib/httpsrv.h>

#include "astra_utils.h"

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


/*�⢥� �� �ࢥ�
 *  completed=true - ��襫 �ࠢ���� �⢥�
 *  content_length - ����� ⥫�
 *  content - ⥫�
 *  error_code - ��� �訡�� - ����� completed=true, ⮣�� �������� 2 ��ਠ�� error_code=0(succes) ��� err_code=end of file
 *  error_message - ⥪�� �訡��
 *  error_operation - �� ����� ����樨 ��稫��� �訡��
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
    ss << "status_code=" << status_code << ",error_code=" << error_code << ",what=" << error_message << ",where=" + StrHTTPErrorOperation( error_operation );
    if ( pr_contentLength ) {
      ss << ",content_length=" << content_length;
    }
    ss << ",completed=" << completed;
    return ss.str();
  }
};

void httpClient_main(const RequestInfo& request, ResponseInfo& response);

/////////////////////////////////////////////////////////////////////////////////////////

namespace edifact { class KickInfo; }

/////////////////////////////////////////////////////////////////////////////////////////

namespace Http {

class Client
{
public:
    // �ᨭ�஭
    void sendGetRequest(const std::string& reqText,
                        const std::string& reqPath,
                        const edifact::KickInfo& kickInfo) const;
    void sendPostRequest(const std::string& reqText,
                         const std::string& reqPath,
                         const edifact::KickInfo& kickInfo) const;
    // ᨭ�஭
    void sendGetRequest(const std::string& reqText,
                        const std::string& reqPath) const;
    void sendPostRequest(const std::string& reqText,
                         const std::string& reqPath) const;

    boost::optional<httpsrv::HttpResp> receive() const;

    virtual ~Client() {}

protected:
    virtual httpsrv::HostAndPort          addr() const = 0;
    virtual httpsrv::Domain             domain() const = 0;
    virtual boost::posix_time::seconds timeout() const = 0;
    virtual httpsrv::UseSSLFlag         useSsl() const;

private:
    void sendRequest_(const std::string& reqType, const std::string& reqText,
                      const std::string& reqPath,
                      const boost::optional<edifact::KickInfo>& kickInfo) const;
};

}//namespace Http

#endif // HTTPCLIENT_H
