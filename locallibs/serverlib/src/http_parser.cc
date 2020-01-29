#include <cctype>
#include <algorithm>
#include "http_parser.h"
#include "exception.h"
#include <iostream>

#define NICKNAME ""
#include "slogger.h"

namespace ServerFramework {

namespace HTTP {

namespace status_strings
{
    const char ok[] =
        "HTTP/1.1 200 OK\r\n";
    const char created[] =
        "HTTP/1.1 201 Created\r\n";
    const char accepted[] =
        "HTTP/1.1 202 Accepted\r\n";
    const char no_content[] =
        "HTTP/1.1 204 No Content\r\n";
    const char multiple_choices[] =
        "HTTP/1.1 300 Multiple Choices\r\n";
    const char moved_permanently[] =
        "HTTP/1.1 301 Moved Permanently\r\n";
    const char moved_temporarily[] =
        "HTTP/1.1 302 Moved Temporarily\r\n";
    const char not_modified[] =
        "HTTP/1.1 304 Not Modified\r\n";
    const char bad_request[] =
        "HTTP/1.1 400 Bad Request\r\n";
    const char unauthorized[] =
        "HTTP/1.1 401 Unauthorized\r\n";
    const char forbidden[] =
        "HTTP/1.1 403 Forbidden\r\n";
    const char not_found[] =
        "HTTP/1.1 404 Not Found\r\n";
    const char internal_server_error[] =
        "HTTP/1.1 500 Internal Server Error\r\n";
    const char not_implemented[] =
        "HTTP/1.1 501 Not Implemented\r\n";
    const char bad_gateway[] =
        "HTTP/1.1 502 Bad Gateway\r\n";
    const char service_unavailable[] =
        "HTTP/1.1 503 Service Unavailable\r\n";

const std::string to_string(reply::status_type status)
{
    switch (status)
    {
    case reply::ok:
        return ok;
    case reply::created:
        return created;
    case reply::accepted:
        return accepted;
    case reply::no_content:
        return no_content;
    case reply::multiple_choices:
        return multiple_choices;
    case reply::moved_permanently:
        return moved_permanently;
    case reply::moved_temporarily:
        return moved_temporarily;
    case reply::not_modified:
        return not_modified;
    case reply::bad_request:
        return bad_request;
    case reply::unauthorized:
        return unauthorized;
    case reply::forbidden:
        return forbidden;
    case reply::not_found:
        return not_found;
    case reply::internal_server_error:
        return internal_server_error;
    case reply::not_implemented:
        return not_implemented;
    case reply::bad_gateway:
        return bad_gateway;
    case reply::service_unavailable:
        return service_unavailable;
    default:
        return internal_server_error;
    }
}

} // namespace status_strings

namespace stock_replies
{
const char ok[] = "";
const char created[] =
    "<html>"
    "<head><title>Created</title></head>"
    "<body><h1>201 Created</h1></body>"
    "</html>";
const char accepted[] =
    "<html>"
    "<head><title>Accepted</title></head>"
    "<body><h1>202 Accepted</h1></body>"
    "</html>";
const char no_content[] =
    "<html>"
    "<head><title>No Content</title></head>"
    "<body><h1>204 Content</h1></body>"
    "</html>";
const char multiple_choices[] =
    "<html>"
    "<head><title>Multiple Choices</title></head>"
    "<body><h1>300 Multiple Choices</h1></body>"
    "</html>";
const char moved_permanently[] =
    "<html>"
    "<head><title>Moved Permanently</title></head>"
    "<body><h1>301 Moved Permanently</h1></body>"
    "</html>";
const char moved_temporarily[] =
    "<html>"
    "<head><title>Moved Temporarily</title></head>"
    "<body><h1>302 Moved Temporarily</h1></body>"
    "</html>";
const char not_modified[] =
    "<html>"
    "<head><title>Not Modified</title></head>"
    "<body><h1>304 Not Modified</h1></body>"
    "</html>";
const char bad_request[] =
    "<html>"
    "<head><title>Bad Request</title></head>"
    "<body><h1>400 Bad Request</h1></body>"
    "</html>";
const char unauthorized[] =
    "<html>"
    "<head><title>Unauthorized</title></head>"
    "<body><h1>401 Unauthorized</h1></body>"
    "</html>";
const char forbidden[] =
    "<html>"
    "<head><title>Forbidden</title></head>"
    "<body><h1>403 Forbidden</h1></body>"
    "</html>";
const char not_found[] =
    "<html>"
    "<head><title>Not Found</title></head>"
    "<body><h1>404 Not Found</h1></body>"
    "</html>";
const char internal_server_error[] =
    "<html>"
    "<head><title>Internal Server Error</title></head>"
    "<body><h1>500 Internal Server Error</h1></body>"
    "</html>";
const char not_implemented[] =
    "<html>"
    "<head><title>Not Implemented</title></head>"
    "<body><h1>501 Not Implemented</h1></body>"
    "</html>";
const char bad_gateway[] =
    "<html>"
    "<head><title>Bad Gateway</title></head>"
    "<body><h1>502 Bad Gateway</h1></body>"
    "</html>";
const char service_unavailable[] =
    "<html>"
    "<head><title>Service Unavailable</title></head>"
    "<body><h1>503 Service Unavailable</h1></body>"
    "</html>";

std::string to_string(reply::status_type status)
{
    switch (status)
    {
    case reply::ok:
        return ok;
    case reply::created:
        return created;
    case reply::accepted:
        return accepted;
    case reply::no_content:
        return no_content;
    case reply::multiple_choices:
        return multiple_choices;
    case reply::moved_permanently:
        return moved_permanently;
    case reply::moved_temporarily:
        return moved_temporarily;
    case reply::not_modified:
        return not_modified;
    case reply::bad_request:
        return bad_request;
    case reply::unauthorized:
        return unauthorized;
    case reply::forbidden:
        return forbidden;
    case reply::not_found:
        return not_found;
    case reply::internal_server_error:
        return internal_server_error;
    case reply::not_implemented:
        return not_implemented;
    case reply::bad_gateway:
        return bad_gateway;
    case reply::service_unavailable:
        return service_unavailable;
    default:
        return internal_server_error;
    }
} 

} //namespace stock_replies

std::string find_header(const std::string& name, const std::vector<header>& headers, const std::string& def)
{
    for (size_t i = 0; i < headers.size(); ++i) {
        if (headers[i].name == name) {

            return headers[i].value;
        }
    }

    return def;
}

size_t content_length(const std::vector<header>& headers)
{
    try {
        return std::stol(find_header("Content-Length", headers, ""));
    } catch (std::invalid_argument&) {
        return 0;
    }
}

std::string request::to_string() const 
{
    static const size_t VER_BUF_SIZE = 3;
    static const char HEADERS_SEPARATOR[] = ": ";
    static const char END_LINE[] = "\r\n";
    static const char HTTP[] = "HTTP/";
    char ver_buf[VER_BUF_SIZE + 1];
    size_t len = method.size();

    len += 1; // ' '
    len += uri.size();
    len += 1; // ' '
    len += sizeof(HTTP) - 1;
    len += sizeof(ver_buf) - 1;
    for(std::vector<HTTP::header>::const_iterator i = headers.begin(); i != headers.end(); ++i) {
        len += i->name.size() + 2 /*": "*/ + i->value.size() + 2 /*"\r\n"*/;
    }
    len += 2; //"\r\n"

    std::string buf;
    std::back_insert_iterator<std::string> bii(buf);

    buf.reserve(len);
    buf.append(method);
    buf.append(1, ' ');
    buf.append(uri);
    buf.append(1, ' ');
    buf.append(HTTP, sizeof(HTTP) - 1);
    snprintf(ver_buf, sizeof(ver_buf), "%d.%d", http_version_major, http_version_minor);
    buf.append(ver_buf, sizeof(ver_buf) - 1);
    buf.append(END_LINE, sizeof(END_LINE) - 1);
    for(std::vector<HTTP::header>::const_iterator i = headers.begin(); i != headers.end(); ++i) {
        buf.append(i->name);
        buf.append(HEADERS_SEPARATOR, sizeof(HEADERS_SEPARATOR) - 1);
        buf.append(i->value);
        buf.append(END_LINE, sizeof(END_LINE) - 1);
    }
    buf.append(END_LINE, sizeof(END_LINE) - 1);

    return buf;
}

void reply::to_buffer(std::vector<uint8_t>& buf)
{
    static const char HEADERS_SEPARATOR[] = ": ";
    static const char END_LINE[] = "\r\n";
    const std::string& st = HTTP::status_strings::to_string(status);
    size_t len = st.size();

    for(std::vector<HTTP::header>::iterator i = headers.begin(); i != headers.end(); ++i) {
        len += i->name.size() + 2 /*": "*/ + i->value.size() + 2 /*"\r\n"*/;
    }
    len += 2; //"\r\n"
    len += content.size();

    buf.clear();
    buf.reserve(len);
    std::back_insert_iterator<std::vector<uint8_t> > bii(buf);

    std::copy(st.begin(), st.end(), bii);
    for(std::vector<HTTP::header>::iterator i = headers.begin(); i != headers.end(); ++i) {
        std::copy(i->name.begin(), i->name.end(), bii);
        std::copy(HEADERS_SEPARATOR, HEADERS_SEPARATOR + sizeof(HEADERS_SEPARATOR) - 1, bii);
        std::copy(i->value.begin(), i->value.end(), bii);
        std::copy(END_LINE, END_LINE + sizeof(END_LINE) - 1, bii);
    }
    std::copy(END_LINE, END_LINE + sizeof(END_LINE) - 1, bii);
    std::copy(content.begin(), content.end(), bii);
}

reply reply::stock_reply(reply::status_type status, std::string&& content, std::string&& type)
{
    auto tostr = [](size_t i){  std::ostringstream s;  s << i;  return s.str();  };
    reply rep;
    rep.status = status;
    rep.content = content;
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = tostr(rep.content.size());
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = type;
    return rep;
}

reply reply::stock_reply(reply::status_type status)
{
    return stock_reply(status, stock_replies::to_string(status), "text/html");
}

void reply::add_header(const header& h)
{
    headers.push_back(h);
}

void reply::add_header(const std::string& name, const std::string& value)
{
    header h;
    h.name = name;
    h.value = value;
    headers.push_back(h);
}

} // namespace HTTP

} // namespace ServerFramework
