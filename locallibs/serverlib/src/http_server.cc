#if HAVE_CONFIG_H
#endif

#include <fstream>
#include <set>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>

#include "http_server.h"
#include "daemon_impl.h"
#include "noncopyable.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"
#include <iostream>

using http::request;
using http::reply;
using http::header;
using http::request_handler_t;

#warning http-server implementation is not reliable
namespace http {

std::string find_header(const std::string& name, const std::vector<header>& headers, const std::string& def)
{
    for (size_t i = 0; i < headers.size(); ++i) {
        if (headers[i].name == name) {
            return headers[i].value;
        }
    }
    return def;
}
namespace status_strings {

const std::string ok =
    "HTTP/1.0 200 OK\r\n";
const std::string created =
    "HTTP/1.0 201 Created\r\n";
const std::string accepted =
    "HTTP/1.0 202 Accepted\r\n";
const std::string no_content =
    "HTTP/1.0 204 No Content\r\n";
const std::string multiple_choices =
    "HTTP/1.0 300 Multiple Choices\r\n";
const std::string moved_permanently =
    "HTTP/1.0 301 Moved Permanently\r\n";
const std::string moved_temporarily =
    "HTTP/1.0 302 Moved Temporarily\r\n";
const std::string not_modified =
    "HTTP/1.0 304 Not Modified\r\n";
const std::string bad_request =
    "HTTP/1.0 400 Bad Request\r\n";
const std::string unauthorized =
    "HTTP/1.0 401 Unauthorized\r\n";
const std::string forbidden =
    "HTTP/1.0 403 Forbidden\r\n";
const std::string not_found =
    "HTTP/1.0 404 Not Found\r\n";
const std::string internal_server_error =
    "HTTP/1.0 500 Internal Server Error\r\n";
const std::string not_implemented =
    "HTTP/1.0 501 Not Implemented\r\n";
const std::string bad_gateway =
    "HTTP/1.0 502 Bad Gateway\r\n";
const std::string service_unavailable =
    "HTTP/1.0 503 Service Unavailable\r\n";

boost::asio::const_buffer to_buffer(reply::status_type status)
{
    switch (status)
    {
    case reply::ok:
        return boost::asio::buffer(ok);
    case reply::created:
        return boost::asio::buffer(created);
    case reply::accepted:
        return boost::asio::buffer(accepted);
    case reply::no_content:
        return boost::asio::buffer(no_content);
    case reply::multiple_choices:
        return boost::asio::buffer(multiple_choices);
    case reply::moved_permanently:
        return boost::asio::buffer(moved_permanently);
    case reply::moved_temporarily:
        return boost::asio::buffer(moved_temporarily);
    case reply::not_modified:
        return boost::asio::buffer(not_modified);
    case reply::bad_request:
        return boost::asio::buffer(bad_request);
    case reply::unauthorized:
        return boost::asio::buffer(unauthorized);
    case reply::forbidden:
        return boost::asio::buffer(forbidden);
    case reply::not_found:
        return boost::asio::buffer(not_found);
    case reply::internal_server_error:
        return boost::asio::buffer(internal_server_error);
    case reply::not_implemented:
        return boost::asio::buffer(not_implemented);
    case reply::bad_gateway:
        return boost::asio::buffer(bad_gateway);
    case reply::service_unavailable:
        return boost::asio::buffer(service_unavailable);
    default:
        return boost::asio::buffer(internal_server_error);
    }
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

/// Convert the reply into a vector of buffers. The buffers do not own the
/// underlying memory blocks, therefore the reply object must remain valid and
/// not be changed until the write operation has completed.
std::vector<boost::asio::const_buffer> to_buffers(reply& r)
{
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(status_strings::to_buffer(r.status));
    for (std::size_t i = 0; i < r.headers.size(); ++i)
    {
        header& h = r.headers[i];
        buffers.push_back(boost::asio::buffer(h.name));
        buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
        buffers.push_back(boost::asio::buffer(h.value));
        buffers.push_back(boost::asio::buffer(misc_strings::crlf));
    }
    buffers.push_back(boost::asio::buffer(misc_strings::crlf));
    buffers.push_back(boost::asio::buffer(r.content));
    return buffers;
}

namespace stock_replies {

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

} // namespace stock_replies

static const auto tostr = [](size_t i){  std::ostringstream s;  s << i;  return s.str();  };

reply reply::stock_reply(reply::status_type status)
{
    reply rep;
    rep.status = status;
    rep.content = stock_replies::to_string(status);
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = tostr(rep.content.size());
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = "text/html";
    return rep;
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

} // namespace http
// реализация Http-сервера целиком взята из примеров использования asio
// без глубокого анализа и не может считаться рабочей
// сделано только для того, чтобы как-то отвечать на Http-запросы
namespace
{

namespace mime_types {
/// Convert a file extension into a MIME type.
struct mapping
{
    const char* extension;
    const char* mime_type;
} mappings[] =
{
    { "gif", "image/gif" },
    { "htm", "text/html" },
    { "html", "text/html" },
    { "jpg", "image/jpeg" },
    { "png", "image/png" },
    { 0, 0 } // Marks end of list.
};

std::string extension_to_type(const std::string& extension)
{
    for (mapping* m = mappings; m->extension; ++m)
    {
        if (m->extension == extension)
        {
            return m->mime_type;
        }
    }

    return "text/plain";
}

} // namespace mime_types

static size_t content_length(const std::vector<header>& headers)
{
    std::cout << "content_length2\n";
    try {
        return std::stol(find_header("Content-Length", headers, ""));
    } catch (std::invalid_argument&) {
        return 0;
    }
    std::cout << "content_length2 ok\n";
}

class request_parser
{
public:
    /// Construct ready to parse the request method.
    request_parser()
        : state_(method_start)
    {}

    /// Reset to initial parser state.
    void reset()
    {
        state_ = method_start;
    }

    /// Parse some data. The tribool return value is true when a complete request
    /// has been parsed, false if the data is invalid, indeterminate when more
    /// data is required. The InputIterator return value indicates how much of the
    /// input has been consumed.
    template <typename InputIterator>
        boost::tuple<boost::tribool, InputIterator> parse(request& req,
                InputIterator begin, InputIterator end)
        {
            while (begin != end)
            {
                boost::tribool result = consume(req, *begin++);
                if (result || !result)
                    return boost::make_tuple(result, begin);
            }
            boost::tribool result = boost::indeterminate;
            return boost::make_tuple(result, begin);
        }

private:
    /// Handle the next character of input.
    boost::tribool consume(request& req, char input)
    {
        switch (state_)
        {
        case method_start:
            if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return false;
            }
            else
            {
                state_ = method;
                req.method.push_back(input);
                return boost::indeterminate;
            }
        case method:
            if (input == ' ')
            {
                state_ = uri;
                return boost::indeterminate;
            }
            else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return false;
            }
            else
            {
                req.method.push_back(input);
                return boost::indeterminate;
            }
        case uri_start:
            if (is_ctl(input))
            {
                return false;
            }
            else
            {
                state_ = uri;
                req.uri.push_back(input);
                return boost::indeterminate;
            }
        case uri:
            if (input == ' ')
            {
                state_ = http_version_h;
                return boost::indeterminate;
            }
            else if (is_ctl(input))
            {
                return false;
            }
            else
            {
                req.uri.push_back(input);
                return boost::indeterminate;
            }
        case http_version_h:
            if (input == 'H')
            {
                state_ = http_version_t_1;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_t_1:
            if (input == 'T')
            {
                state_ = http_version_t_2;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_t_2:
            if (input == 'T')
            {
                state_ = http_version_p;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_p:
            if (input == 'P')
            {
                state_ = http_version_slash;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_slash:
            if (input == '/')
            {
                req.http_version_major = 0;
                req.http_version_minor = 0;
                state_ = http_version_major_start;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_major_start:
            if (is_digit(input))
            {
                req.http_version_major = req.http_version_major * 10 + input - '0';
                state_ = http_version_major;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_major:
            if (input == '.')
            {
                state_ = http_version_minor_start;
                return boost::indeterminate;
            }
            else if (is_digit(input))
            {
                req.http_version_major = req.http_version_major * 10 + input - '0';
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_minor_start:
            if (is_digit(input))
            {
                req.http_version_minor = req.http_version_minor * 10 + input - '0';
                state_ = http_version_minor;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case http_version_minor:
            if (input == '\r')
            {
                state_ = expecting_newline_1;
                return boost::indeterminate;
            }
            else if (is_digit(input))
            {
                req.http_version_minor = req.http_version_minor * 10 + input - '0';
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case expecting_newline_1:
            if (input == '\n')
            {
                state_ = header_line_start;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case header_line_start:
            if (input == '\r')
            {
                state_ = expecting_newline_3;
                return boost::indeterminate;
            }
            else if (!req.headers.empty() && (input == ' ' || input == '\t'))
            {
                state_ = header_lws;
                return boost::indeterminate;
            }
            else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return false;
            }
            else
            {
                req.headers.push_back(header());
                req.headers.back().name.push_back(input);
                state_ = header_name;
                return boost::indeterminate;
            }
        case header_lws:
            if (input == '\r')
            {
                state_ = expecting_newline_2;
                return boost::indeterminate;
            }
            else if (input == ' ' || input == '\t')
            {
                return boost::indeterminate;
            }
            else if (is_ctl(input))
            {
                return false;
            }
            else
            {
                state_ = header_value;
                req.headers.back().value.push_back(input);
                return boost::indeterminate;
            }
        case header_name:
            if (input == ':')
            {
                state_ = space_before_header_value;
                return boost::indeterminate;
            }
            else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
            {
                return false;
            }
            else
            {
                req.headers.back().name.push_back(input);
                return boost::indeterminate;
            }
        case space_before_header_value:
            if (input == ' ')
            {
                state_ = header_value;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case header_value:
            if (input == '\r')
            {
                state_ = expecting_newline_2;
                return boost::indeterminate;
            }
            else if (is_ctl(input))
            {
                return false;
            }
            else
            {
                req.headers.back().value.push_back(input);
                return boost::indeterminate;
            }
        case expecting_newline_2:
            if (input == '\n')
            {
                state_ = header_line_start;
                return boost::indeterminate;
            }
            else
            {
                return false;
            }
        case expecting_newline_3:
            if (input == '\n') {
                content_length_ = content_length(req.headers);
                if (content_length_ != 0) {
                    read_content_length_ = 0;
                    state_ = reading_content;
                    return boost::indeterminate;
                } else {
                    return true;
                }
            } else {
                return false;
            }
        case reading_content:
            req.content += input;
            ++read_content_length_;
            if (read_content_length_ == content_length_) {
                return true;
            } else {
                return boost::indeterminate;
            }
        default:
            return false;
        }
    }

    /// Check if a byte is an HTTP character.
    static bool is_char(int c)
    {
        return c >= 0 && c <= 127;
    }

    /// Check if a byte is an HTTP control character.
    static bool is_ctl(int c)
    {
        return (c >= 0 && c <= 31) || (c == 127);
    }

    /// Check if a byte is defined as an HTTP tspecial character.
    static bool is_tspecial(int c)
    {
        switch (c)
        {
        case '(': case ')': case '<': case '>': case '@':
        case ',': case ';': case ':': case '\\': case '"':
        case '/': case '[': case ']': case '?': case '=':
        case '{': case '}': case ' ': case '\t':
            return true;
        default:
            return false;
        }
    }

    /// Check if a byte is a digit.
    static bool is_digit(int c)
    {
        return c >= '0' && c <= '9';
    }

    /// The current state of the parser.
    enum state
    {
        method_start,
        method,
        uri_start,
        uri,
        http_version_h,
        http_version_t_1,
        http_version_t_2,
        http_version_p,
        http_version_slash,
        http_version_major_start,
        http_version_major,
        http_version_minor_start,
        http_version_minor,
        expecting_newline_1,
        header_line_start,
        header_lws,
        header_name,
        space_before_header_value,
        header_value,
        expecting_newline_2,
        expecting_newline_3,
        reading_content
    } state_;
    size_t content_length_;
    size_t read_content_length_;
};

class request_handler
  : private comtech::noncopyable
{
public:
    /// Construct with a directory containing files to be served.
    explicit request_handler(request_handler_t f)
        : handler_(f)
    {}
    /// Handle a request and produce a reply.
    void handle_request(const request& req, reply& rep)
    {
        std::string request_path;
        if (!url_decode(req.uri, request_path)) {
            rep = reply::stock_reply(reply::bad_request);
            return;
        }
        // Request path must be absolute and not contain "..".
        if (request_path.empty() || request_path[0] != '/'
                || request_path.find("..") != std::string::npos) {
            rep = reply::stock_reply(reply::bad_request);
            return;
        }
        handler_(request_path, req, rep);
        if(find_header("Content-Length", rep.headers, std::string()).empty())
            rep.add_header("Content-Length", http::tostr(rep.content.size()));
    }
private:
    /// Perform URL-decoding on a string. Returns false if the encoding was invalid.
    static bool url_decode(const std::string& in, std::string& out)
    {
        out.clear();
        out.reserve(in.size());
        for (std::size_t i = 0; i < in.size(); ++i) {
            if (in[i] == '%') {
                if (i + 3 <= in.size()) {
                    int value = 0;
                    std::istringstream is(in.substr(i + 1, 2));
                    if (is >> std::hex >> value) {
                        out += static_cast<char>(value);
                        i += 2;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            } else if (in[i] == '+') {
                out += ' ';
            } else {
                out += in[i];
            }
        }
        return true;
    }
    request_handler_t handler_;
};

class connection_manager;
class connection
  : public std::enable_shared_from_this<connection>,
    private comtech::noncopyable
{
public:
    connection(boost::asio::io_service& io_service,
            connection_manager& manager, request_handler& handler);
    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }
    void start()
    {
        socket_.async_read_some(boost::asio::buffer(buffer_),
                boost::bind(&connection::handle_read, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    }
    void stop()
    {
        socket_.close();
    }

private:
    void handle_read(const boost::system::error_code& e, std::size_t bytes_transferred);
    void handle_write(const boost::system::error_code& e);

    boost::asio::ip::tcp::socket socket_;
    connection_manager& connection_manager_;
    request_handler& request_handler_;
    boost::array<char, 8192> buffer_;
    std::vector<char> content_;
    request request_;
    request_parser request_parser_;
    reply reply_;
};
typedef std::shared_ptr<connection> connection_ptr;

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
class connection_manager
  : private comtech::noncopyable
{
public:
    void start(const connection_ptr& c)
    {
        connections_.insert(c);
        c->start();
    }
    void stop(const connection_ptr& c)
    {
        connections_.erase(c);
        c->stop();
    }
    void stop_all()
    {
        for(auto& c : connections_)
            c->stop();
        connections_.clear();
    }

private:
    std::set<connection_ptr> connections_;
};

connection::connection(boost::asio::io_service& io_service,
        connection_manager& manager, request_handler& handler)
    : socket_(io_service), connection_manager_(manager), request_handler_(handler)
{
    reply_.headers.reserve(10);
}

void connection::handle_read(const boost::system::error_code& e, std::size_t bytes_transferred)
{
    if (!e) {
        boost::tribool result;
        boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
                request_, buffer_.data(), buffer_.data() + bytes_transferred);

        if (result) {
            request_handler_.handle_request(request_, reply_);
            boost::asio::async_write(socket_, to_buffers(reply_),
                    boost::bind(&connection::handle_write, shared_from_this(),
                        boost::asio::placeholders::error));
        } else if (!result) {
            reply_ = reply::stock_reply(reply::bad_request);
            boost::asio::async_write(socket_, to_buffers(reply_),
                    boost::bind(&connection::handle_write, shared_from_this(),
                        boost::asio::placeholders::error));
        } else {
            socket_.async_read_some(boost::asio::buffer(buffer_),
                    boost::bind(&connection::handle_read, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
        }
    } else if (e != boost::asio::error::operation_aborted) {
        connection_manager_.stop(shared_from_this());
    }
}
void connection::handle_write(const boost::system::error_code& e)
{
    if (!e) {
        // Initiate graceful connection closure.
        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    }
    if (e != boost::asio::error::operation_aborted) {
        connection_manager_.stop(shared_from_this());
    }
}


class server
  : private comtech::noncopyable
{
public:
    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    server(const std::string& address, const std::string& port, request_handler_t f)
        : host_(address), port_(port), io_service_(ServerFramework::system_ios::Instance()), acceptor_(io_service_), connection_manager_(),
        new_connection_(new connection(io_service_, connection_manager_, request_handler_)),
        request_handler_(f)
    {
    }
    void init()
    {
        boost::asio::ip::tcp::resolver resolver(io_service_);
        boost::asio::ip::tcp::resolver::query query(host_, port_);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
        acceptor_.async_accept(new_connection_->socket(),
                boost::bind(&server::handle_accept, this,
                    boost::asio::placeholders::error));
    }
    void stop()
    {
        // Post a call to the stop function so that server::stop() is safe to call
        // from any thread.
        io_service_.post(boost::bind(&server::handle_stop, this));
    }
private:
    void handle_accept(const boost::system::error_code& e)
    {
        if (!e) {
            connection_manager_.start(new_connection_);
            new_connection_.reset(new connection(io_service_,
                        connection_manager_, request_handler_));
            acceptor_.async_accept(new_connection_->socket(),
                    boost::bind(&server::handle_accept, this,
                        boost::asio::placeholders::error));
        }
    }
    void handle_stop()
    {
        acceptor_.close();
        connection_manager_.stop_all();
    }
    std::string host_, port_;
    boost::asio::io_service& io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;
    connection_manager connection_manager_;
    connection_ptr new_connection_;
    request_handler request_handler_;
};

} // namespace

namespace ServerFramework
{

struct HttpServer::Impl
{
    Impl(const std::string& host, const std::string& port, request_handler_t f)
        : srv_(host, port, f)
    {}
    void init() {
        srv_.init();
    }
    server srv_;
};

/***  HttpServer  ***/
HttpServer::HttpServer(const std::string& host, const std::string& port)
    : pImpl_(new HttpServer::Impl(host, port, boost::bind(&HttpServer::handleRequest, this, _1, _2, _3)))
{}

HttpServer::~HttpServer()
{}

void HttpServer::init()
{
    pImpl_->init();
}

void HttpServer::returnStatics(std::string request_path, const http::request& req, http::reply& rep)
{
    //If path ends in slash (i.e. is a directory) then add "index.html".
    if (request_path[request_path.size() - 1] == '/') {
        request_path += "index.html";
    }
    //Determine the file extension.
    std::size_t last_slash_pos = request_path.find_last_of("/");
    std::size_t last_dot_pos = request_path.find_last_of(".");
    std::string extension;
    if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos) {
        extension = request_path.substr(last_dot_pos + 1);
    }
    //Open the file to send back.
    std::string full_path = request_path;
    std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
    if (!is) {
        rep = reply::stock_reply(reply::not_found);
        return;
    }
    //Fill out the reply to be sent to the client.
    rep.status = reply::ok;
    char buf[512];
    while (is.read(buf, sizeof(buf)).gcount() > 0)
        rep.content.append(buf, is.gcount());
    rep.add_header("Content-Type", mime_types::extension_to_type(extension));
}

} // ServerFramework
