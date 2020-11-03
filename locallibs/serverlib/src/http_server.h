#ifndef SERVERLIB_HTTP_SERVER_H
#define SERVERLIB_HTTP_SERVER_H

#include "daemon_event.h"
#include "daemon_task.h"
#include "http_types.h"

namespace ServerFramework
{

class HttpServer
    : public DaemonEvent
{
public:
    HttpServer(const std::string& host, const std::string& port);
    virtual ~HttpServer();
    virtual void init();
    virtual void handleRequest(const std::string& path, const http::request& req, http::reply& rep) = 0;

    static void returnStatics(std::string path, const http::request& req, http::reply& rep);
private:
    using DaemonEvent::runTasks;
    virtual void runTasks(const char* buff, size_t buffSz) {}
    virtual void runTasks(const boost::posix_time::ptime&) {}
    virtual void runTasks() {}
    class Impl;
    boost::shared_ptr<HttpServer::Impl> pImpl_;
};

} // ServerFramework

#endif /* SERVERLIB_HTTP_SERVER_H */

