#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <string>
#include <queue>
#include <vector>

#ifdef USE_THREADS
    #include <boost/thread/mutex.hpp>
#endif

#include "AsyncNet.hpp"
#include "serverlib/exception.h"

#define NICKNAME "AZVEREV"
#define NICKTRACE VLAD_TRACE
#include "serverlib/slogger.h"

#define printf(...)


namespace asyncnet {

AsyncTcpSock::AsyncTcpSock(boost::asio::io_service &io,
                           const std::string &desc_,
                           const Dest &d,
                           const bool infininty_, /* if i should try to connect forever */
                           const int heart,
                           const bool keepconn_,
                           std::size_t rxmax,
                           std::size_t txmax)
    : strand(io),
      sock(io),
      resolver(io),
      timer(io),
      dest(1, d),
      infinite(infininty_),
      heartbeat(heart),
      cur_dest(0),
      keepconn(keepconn_),
      desc(desc_),
      rx(9999999),
      tx(9999999, "tx! temp")
//      rx(rxmax),
//      tx(txmax)
{
    /*
     * Since rx and tx are constructed with const size and can not be changed,
     * the maximum size of processed data at a time will be limited.
     */

    set_disconnected();
}

AsyncTcpSock::AsyncTcpSock(boost::asio::io_service &io,
                           const std::string &desc_,
                           const std::vector<Dest> &d,
                           const bool infininty_, /* if i should try to connect forever */
                           const int heart,
                           const bool keepconn_,
                           std::size_t rxmax,
                           std::size_t txmax)
    : strand(io),
      sock(io),
      resolver(io),
      timer(io),
      dest(d),
      infinite(infininty_),
      heartbeat(heart),
      cur_dest(0),
      keepconn(keepconn_),
      desc(desc_),
      rx(9999999),
      tx(9999999, "tx! temp")
//      rx(rxmax),
//      tx(txmax)
{
    /*
     * Since rx and tx are constructed with const size and can not be changed,
     * the maximum size of processed data at a time will be limited.
     */
    set_disconnected();
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

void AsyncTcpSock::send(const char *d, const std::size_t n)
{
    if (!is_connected()) {
        throw ServerFramework::Exception("UWAGA",
                                         __FILE__,
                                         __LINE__,
                                         __FUNCTION__,
                                         desc + "AsyncTcpSock::send() "
                                         "the socket is not connected");
        return;
    }

/*    if (!is_connected()) {
        printf("%s AsyncTcpSock::send() the socket is not connected\n",
               desc.c_str());

        return;
    }
*/
    Sharedbuf buf(d, n);
    strand.post(boost::bind(&AsyncTcpSock::push_into_tx, this, buf));
}

void AsyncTcpSock::push_into_tx(Sharedbuf buf)
{
 /*   try
    {
        tx.sputn(buf.get(), buf.size());
    }
    catch (const std::length_error &err)
    {
        printf("%s push_into_tx() no free space in tx\n", desc.c_str());
        return;
    }
*/
    Txbuf tmp(buf.get(), buf.size());
    tx.push(tmp);
    printf("push_into_tx\n");
    if (!is_write_act())
        return write();
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
    const boost::asio::ip::tcp::endpoint ep = *resolv_iter;
    if (!err) {
        cur_dest = 0;
        printf("%s: connection to %s:%d has been successfully established\n",
               desc.c_str(),
               ep.address().to_string().c_str(),
               ep.port());

        write_unset_act();
        read_unset_act();
        set_connected();
        set_nodelay(true);
        read();
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
//    boost::asio::async_write(sock, tx.data(),
//                             strand.wrap(boost::bind(&AsyncTcpSock::write_handler, this, _1, _2)));

    boost::asio::async_write(sock, tx.get(),
                             strand.wrap(boost::bind(&AsyncTcpSock::write_handler, this, _1, _2)));
    write_set_act();

    /* if we have written smth then we should defer the next heartbeat */
    restart_heartbeat();
}

void AsyncTcpSock::write_handler(const boost::system::error_code &err, std::size_t n)
{
    write_unset_act();
    if (!err && is_connected()) {
//        tx.consume(n);
//        if (tx.size() > 0)
//            write();

        tx.pop();
        if (!tx.empty())
            return write();

    } else if (err != boost::asio::error::operation_aborted) {
        printf("%s: writing to socket failed: %s\n", desc.c_str(), err.message().c_str());
        conn_broken_handler();
    }
}

void AsyncTcpSock::read()
{
//    std::size_t free_space = rx.max_size() - rx.size();
//    sock.async_receive(rx.prepare(free_space), strand.wrap(boost::bind(&AsyncTcpSock::read_handler, this, _1, _2)));
    sock.async_receive(rx.get_free_space(), strand.wrap(boost::bind(&AsyncTcpSock::read_handler, this, _1, _2)));
    read_set_act();
}

void AsyncTcpSock::read_handler(const boost::system::error_code &err, std::size_t n)
{
    read_unset_act();
    if (err && n) {
        printf("read_handler() %s\n", err.message().c_str());
        printf("GOD DAMN STRANGE ERROR! CHECK IT\n");
        abort();
    }


    if (!err) {
//        std::istream in(&rx);
//        std::ostream out(&rx);
//        std::string str;
        std::size_t consumed;

//        rx.commit(n);
//        in >> str;
//        consumed = usr_read_handler(str.c_str(), str.size());
        consumed = usr_read_handler(rx.get_data(), rx.get_data_size());
        rx.consume(consumed);
//        str.erase(0, consumed);
//        out << str; /* we are taking back that which user didn't consume */
//        if (rx.size() < rx.max_size()) {
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
    /* if heartbeat has not been started by user or its interval is 0 then we do nothing*/
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
    set_disconnected();

    if (is_read_act() || is_write_act())
        return;

//    tx.consume(tx.size());
//    rx.consume(rx.size());
    tx.clean();
    rx.consume(rx.get_data_size());
    stop_heartbeat();
    cur_dest = 0;
    sock.close();
    usr_conn_broken_handler();
    if (keepconn) {
        printf("%s: keepconn is set. Auto reconnecting...\n", desc.c_str());
        return connect();
    } else {
        printf("%s: keepconn is not set. I have nothing to do\n", desc.c_str());
    }
}

void AsyncTcpSock::do_set_infinity(bool b)
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

void AsyncTcpSock::set_heartbeat(int t)
{
    strand.post(boost::bind(&AsyncTcpSock::do_set_heartbeat, this, t));
}

void AsyncTcpSock::do_set_heartbeat(int t)
{
    heartbeat = t;
    restart_heartbeat();
}


void AsyncTcpSock::do_close()
{
    set_disconnected();
    sock.close();

    /*
     * heartbeat and connect_timer actually work with common timer,
     * so let's call only stop_heartbeat()
     */
    stop_heartbeat();
//    stop_connect_timer();
    resolver.cancel();
    cur_dest = 0;
//    tx.consume(tx.size());
//    rx.consume(rx.size());
    tx.clean();
    rx.consume(rx.get_data_size());
}

void AsyncTcpSock::set_nodelay(bool val)
{
    boost::asio::ip::tcp::no_delay option(val);
    sock.set_option(option);
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

void AsyncTcpSock::read_set_act()
{
    read_act = true;
}

void AsyncTcpSock::read_unset_act()
{
    read_act = false;
}

bool AsyncTcpSock::is_read_act()
{
    return read_act;
}

void AsyncTcpSock::write_set_act()
{
    write_act = 1;
}

void AsyncTcpSock::write_unset_act()
{
    write_act = 0;
}

bool AsyncTcpSock::is_write_act()
{
    return write_act;
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

#undef printf
} // namespace asyncnet
