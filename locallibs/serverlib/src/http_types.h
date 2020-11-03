#ifndef SERVERLIB_HTTP_TYPES_H
#define SERVERLIB_HTTP_TYPES_H

#include <string>
#include <vector>
#include <functional>

namespace http
{
struct header
{
  std::string name;
  std::string value;
};
std::string find_header(const std::string& name, const std::vector<header>& headers, const std::string& def = "");
/// A reply to be sent to a client.
struct reply
{
  /// The status of the reply.
  enum status_type
  {
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
  } status;

  /// The headers to be included in the reply.
  std::vector<header> headers;

  /// The content to be sent in the reply.
  std::string content;

  /// Get a stock reply.
  void add_header(const header& h);
  void add_header(const std::string& name, const std::string& value);
  static reply stock_reply(status_type status);
};
/// A request received from a client.
struct request
{
  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<header> headers;
  std::string content;
};
typedef std::function<void(const std::string&, const request&, reply&)> request_handler_t;
} // http

#endif /* SERVERLIB_HTTP_TYPES_H */

