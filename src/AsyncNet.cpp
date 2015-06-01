#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <string>
#include <queue>
#include <vector>

#ifdef USE_THREADS
    #include <boost/thread/mutex.hpp>
#endif

#include <string.h>
#include <stdio.h>


#include "AsyncNet.hpp"



namespace asyncnet {

AsyncTcpSock::AsyncTcpSock(boost::asio::io_service &io, const std::string &desc_,
                           const Dest &d, const bool infininty_, const int heart,
                           const bool keepconn_, std::size_t rxmax,
                           std::size_t txmax)
    : strand(io), sock(io), resolver(io), timer(io), dest(1, d),
      infinite(infininty_), heartbeat(heart), cur_dest(0), keepconn(keepconn_),
      desc(desc_), rx(rxmax, desc), tx(txmax, desc), connected(false)
{
    /*
     * Since rx is constructed with const size and can not be changed,
     * the maximum size of processed data at a time will equal to rxmax.
     */
}

AsyncTcpSock::AsyncTcpSock(boost::asio::io_service &io, const std::string &desc_,
                           const std::vector<Dest> &d, const bool infininty_,
                           const int heart, const bool keepconn_,
                           std::size_t rxmax, std::size_t txmax)
    : strand(io), sock(io), resolver(io), timer(io), dest(d),
      infinite(infininty_), heartbeat(heart), cur_dest(0), keepconn(keepconn_),
      desc(desc_), rx(rxmax, desc), tx(txmax, desc), connected(false)
{
    /*
     * Since rx is constructed with const size and can not be changed,
     * the maximum size of processed data at a time will equal to rxmax.
     */
}

void AsyncTcpSock::start_connecting()
{
    close();
    strand.post(boost::bind(&AsyncTcpSock::resolve, this));
}

void AsyncTcpSock::resolve()
{
    if (cur_dest == dest.size()) {
        printf("%s: There are no more destinations to try to connect to\n",
               desc.c_str());
        cur_dest = 0;
        printf("%s: calling usr_connect_failed_handler()\n", desc.c_str());
        usr_connect_failed_handler();
        if (infinite) {
            printf("%s: infinite flag is set. Let's try to connect until it succeeds\n",
                   desc.c_str());
        } else {
            printf("%s: infinite flag is not set. I have nothing to do\n",
                   desc.c_str());
            return;
        }
    }

    const Dest &d = dest[cur_dest++];
    boost::asio::ip::tcp::resolver::query query(d.host, d.port);
    printf("%s: trying to resolve host from \"%s\"\n", desc.c_str(), d.host.c_str());
    resolver.async_resolve(query,
                           strand.wrap(boost::bind(&AsyncTcpSock::resolve_handler,
                                                   this,
                                                   _1,
                                                   _2)));
}

void AsyncTcpSock::resolve_handler(const boost::system::error_code &err,
                                   boost::asio::ip::tcp::resolver::iterator iterator)
{
//    printf("%s resolve_handler(): %s\n", desc.c_str(), err.message().c_str());
    if (!err) {
        if (iterator == boost::asio::ip::tcp::resolver::iterator()) {
            printf("%s: resolve has failed\n", desc.c_str());
            return resolve();
        } else {
            resolv_iter = iterator;
            printf("%s: resolve has succeeded. Trying to connect()\n", desc.c_str());
            return connect();
        }

    } else if (err != boost::asio::error::operation_aborted) {
        printf("%s: resolve error: %s\n", desc.c_str(), err.message().c_str());
        return resolve();
    }
}

void AsyncTcpSock::start_heartbeat()
{
    strand.post(boost::bind(&AsyncTcpSock::do_start_heartbeat, this));
}

void AsyncTcpSock::set_dest(const Dest &d)
{
    std::vector<Dest> tmp(1, d);
    strand.post(boost::bind(&AsyncTcpSock::do_set_dest, this, tmp));
}

void AsyncTcpSock::set_dest(const std::vector<Dest> &d)
{
    std::vector<Dest> tmp(d);
    strand.post(boost::bind(&AsyncTcpSock::do_set_dest, this, tmp));
}

void AsyncTcpSock::set_infinity(const bool infin)
{
    strand.post(boost::bind(&AsyncTcpSock::do_set_infinity, this, infin));
}
/*
int AsyncTcpSock::send(const void *d, const std::size_t n)
{
    if (!is_connected()) {
        return -1;
    }
    Txbuf buf(d, n);
    return push_in_tx(buf);
}
*/
int AsyncTcpSock::send(const void *d, const std::size_t n)
{
    std::size_t sz = tx.size();

    if (sz + n >= tx.max_size())
        throw;

    tx.sputn(d, n);

    if (sz == 0) {
        /*
         * if sz was equal to 0 that means that
         * there weren't active write-handlers
         */
        write();
    }

}

/*int AsyncTcpSock::send(const Buf &b)
{
    if (!is_connected()) {
        return -1;
    }
    Txbuf buf(b.get_data(), b.get_length());
    return push_in_tx(buf);
}
*/
int AsyncTcpSock::push_in_tx(const Txbuf &buf)
{
    tx.lock();
    bool empty = tx.empty();
    tx.push(buf);
//    tx.unlock();
    if (empty)
        write();
    else
        tx.unlock();
}

void AsyncTcpSock::close()
{
    strand.post(boost::bind(&AsyncTcpSock::do_close, this));
}

void AsyncTcpSock::connect()
{
    const boost::asio::ip::tcp::endpoint ep = *resolv_iter;
    printf("%s: trying to connect to %s:%d\n",
           desc.c_str(),
           ep.address().to_string().c_str(),
           ep.port());

    sock.async_connect(ep, strand.wrap(boost::bind(&AsyncTcpSock::connect_handler, this, _1)));
    start_connect_timer();
}

void AsyncTcpSock::connect_handler(const boost::system::error_code &err)
{
    stop_connect_timer();
//    printf("%s boost::asio::ip::tcp::endpoint ep = *resolv_iter\n", desc.c_str());
    const boost::asio::ip::tcp::endpoint ep = *resolv_iter;
    if (!err) {
        cur_dest = 0;
        printf("%s: connection to %s:%d has been successfully established\n",
               desc.c_str(),
               ep.address().to_string().c_str(),
               ep.port());

        read();
        set_connected();
        usr_connect_handler();

    } else {
        sock.close();
        printf("%s: connection to %s:%d has been failed: %s\n",
               desc.c_str(),
               ep.address().to_string().c_str(),
               ep.port(),
               err.message().c_str());

        if (++resolv_iter != boost::asio::ip::tcp::resolver::iterator()) {
            return connect();
        } else {
            return resolve();
        }
    }
}

void AsyncTcpSock::connect_timeout_handler(const boost::system::error_code &err)
{
    if (!err) {
        sock.close();
        const boost::asio::ip::tcp::endpoint ep = *resolv_iter;
        printf("%s: connection to %s:%d has been failed: timeout passed\n", desc.c_str(), ep.address().to_string().c_str(), ep.port());
    }
}


void AsyncTcpSock::write()
{
//    tx.lock();
    boost::asio::async_write(sock, tx.data(),
                             strand.wrap(boost::bind(&AsyncTcpSock::write_handler, this, _1, _2)));
    /* if we have written smth then we should defer the next heartbeat */
//    tx.unlock();
    restart_heartbeat();
}

void AsyncTcpSock::write_handler(const boost::system::error_code &err, std::size_t n)
{
    if (!err && is_connected()) {
        tx.consume(n);
        if (tx.size() > 0)
            write();

    } else if (err != boost::asio::error::operation_aborted) {
        printf("%s: writing to socket failed: %s\n", desc.c_str(), err.message().c_str());
        conn_broken_handler();
    }
}

void AsyncTcpSock::read()
{
    std::size_t free_space = rx.max_size() - tx.size();
    sock.async_receive(rx.prepare(free_space), strand.wrap(boost::bind(&AsyncTcpSock::read_handler, this, _1, _2)));
}

void AsyncTcpSock::read_handler(const boost::system::error_code &err, std::size_t n)
{
    if (err && n) {
        printf("read_handler() %s\n", err.message().c_str());
        printf("GOD DAMN STRANGE ERROR! CHECK IT\n");
        abort();
    }


    if (!err) {
        std::istream is(&rx); //
        std::ostream os(&rx); //     all this are ugly.
        std::string  str;     //


        rx.commit(n);

        std::size_t consumed = usr_read_handler(rx.(), rx.get_usr_data_size());
        rx.consume(consumed);
        if (rx.is_space_left()) {
            return read();
        } else {
            printf("%s: rx buffer reached its limit. async_receive() won't be called until bytes are consumed from the rx buffer.\n",
                   desc.c_str());
        }
    } else if (err != boost::asio::error::operation_aborted) {
        printf("%s: reading from the socket failed: %s\n",
               desc.c_str(),
               err.message().c_str());

        return conn_broken_handler();
    }
}

void AsyncTcpSock::do_start_heartbeat()
{
    if (!heartbeat)
        return;

    printf("%s do_start_heartbeat()\n", desc.c_str());
    timer.expires_from_now(boost::posix_time::seconds(heartbeat));
    timer.async_wait(strand.wrap(boost::bind(&AsyncTcpSock::heartbeat_handler, this, _1)));
}

void AsyncTcpSock::restart_heartbeat()
{
    /* if heartbeat has not been strarted by user or its interval is 0 then we do nothing*/
    if (timer.cancel() && heartbeat != 0)
        do_start_heartbeat();
}

void AsyncTcpSock::heartbeat_handler(const boost::system::error_code &err)
{
    if (!err) {
        usr_heartbeat_handler();
        start_heartbeat();
    }
}

void AsyncTcpSock::stop_heartbeat()
{
    timer.cancel();
}


void AsyncTcpSock::conn_broken_handler()
{
    if (!is_connected())
        return;
    printf("%s: connection is broken\n", desc.c_str());
    do_close();
    printf("%s: calling usr_conn_broken_handler()\n", desc.c_str());
    usr_conn_broken_handler();
    if (keepconn) {
        printf("%s: keepconn is set. Auto reconnecting...\n", desc.c_str());
        connect();
//        strand.post(boost::bind(&AsyncTcpSock::connect, this));
    } else {
        printf("%s: keepconn is not set. I have nothing to do\n", desc.c_str());
    }
}

void AsyncTcpSock::do_set_infinity(const bool b)
{
    infinite = b;
}

void AsyncTcpSock::do_set_dest(std::vector<Dest> v)
{
    dest = v;
    cur_dest = 0;
}

void AsyncTcpSock::start_connect_timer()
{
    timer.expires_from_now(boost::posix_time::seconds(5));
    timer.async_wait(strand.wrap(boost::bind(&AsyncTcpSock::connect_timeout_handler, this, _1)));
}

void AsyncTcpSock::stop_connect_timer()
{
    timer.cancel();
}

void AsyncTcpSock::set_heartbeat(const int t)
{
    strand.post(boost::bind(&AsyncTcpSock::do_set_heartbeat, this, t));
}

void AsyncTcpSock::do_set_heartbeat(const int t)
{
    heartbeat = t;
    restart_heartbeat();
}


void AsyncTcpSock::do_close()
{
    set_disconnected();
    sock.close();
    /*
     * heartbeat and connect_timer actually work with common timer
     */
    stop_heartbeat();
//    stop_connect_timer();
    resolver.cancel();
//    rx.reset();
    cur_dest = 0;
    tx.clean();
}

void AsyncTcpSock::set_connected()
{
    lock();
    connected = true;
    unlock();
}

void AsyncTcpSock::set_disconnected()
{
    lock();
    connected = false;
    unlock();
}

bool AsyncTcpSock::is_connected()
{
    lock();
    bool r = connected;
    unlock();
    return r;
}

#ifdef USE_THREADS
inline void AsyncTcpSock::lock()
{
    return mutex_connected.lock();
}

inline void AsyncTcpSock::unlock()
{
    return mutex_connected.unlock();
}
#else
inline void AsyncTcpSock::lock()
{
}

inline void AsyncTcpSock::unlock()
{
}
#endif


} // namespace asyncnet
