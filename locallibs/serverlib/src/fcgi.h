#ifndef SERVERLIB_FCGI_H
#define SERVERLIB_FCGI_H

#include <map>
#include <string>
#include <vector>
#include <stdint.h>

namespace ServerFramework {
namespace Fcgi
{

struct Stream
{
    uint16_t req_id;
    std::vector<uint8_t>& buf;

    virtual uint8_t type() const = 0;
    Stream& operator<<(const char*);
    Stream& operator<<(const std::string&);
    Stream& operator<<(const std::vector<char>&);
    Stream& operator<<(const std::vector<uint8_t>&);
    Stream(uint16_t r, std::vector<uint8_t>& b);
    virtual ~Stream();
};

uint16_t requestId(const std::vector<uint8_t>&);

class Request
{
public:
    Request(const std::vector<uint8_t>&);
    ~Request();

    std::map< std::string, std::string > params() const;
    std::string stdin_text() const;
    uint16_t id() const;
private:
    struct Impl;
    Impl* impl;
};

class Response
{
public:
    Response(uint16_t req_id, std::vector<uint8_t>& buf);
    ~Response();

    Stream& StdOut();
    Stream& StdErr();
    void set_status(uint8_t);
    void set_app_exit_code(uint32_t);
private:
    struct Impl;
    Impl* impl;
};

} // Fcgi
} // ServerFramework

#endif /* SERVERLIB_FCGI_H */

