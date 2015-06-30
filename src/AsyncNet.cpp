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

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"




namespace asyncnet {

AsyncTcpSock::AsyncTcpSock(boost::asio::io_service &io,
                           const std::string &desc_,
                           const Dest &d,
                           int heart,
                           std::size_t rxmax,
                           std::size_t txmax)
    : desc(desc_),
      dest(1, d),
      heartbeat(heart),
      strand(io),
      sock(io),
      resolver(io),
      timerbeat(io),
      timerconn(io),
      timerreconn(io),
      cur_dest(0),
      rx(rxmax),
      tx(txmax)
{
    /*
     * Since rx and tx are constructed with const size and can not be changed,
     * the maximum size of processed data at a time will be limited.
     */
    init();
}

AsyncTcpSock::AsyncTcpSock(boost::asio::io_service &io,
                           const std::string &desc_,
                           const std::vector<Dest> &d,
                           int heart,
                           std::size_t rxmax,
                           std::size_t txmax)
    : desc(desc_),
      dest(d),
      heartbeat(heart),
      strand(io),
      sock(io),
      resolver(io),
      timerbeat(io),
      timerconn(io),
      timerreconn(io),
      cur_dest(0),
      rx(rxmax),
      tx(txmax)
{
    /*
     * Since rx and tx are constructed with const size and can not be changed,
     * the maximum size of processed data at a time will be limited.
     */
    init();
}

AsyncTcpSock::AsyncTcpSock(boost::asio::io_service &io,
             const std::string &desc_,     /* description. used in LogTraces, and Exception's messages */
             std::size_t rxmax,
             std::size_t txmax)
    : desc(desc_),
      heartbeat(0),
      strand(io),
      sock(io),
      resolver(io),
      timerbeat(io),
      timerconn(io),
      timerreconn(io),
      cur_dest(0),
      rx(rxmax),
      tx(txmax)
{
    /*
     * Since rx and tx are constructed with const size and can not be changed,
     * the maximum size of processed data at a time will be limited.
     */
    init();
}


//AsyncTcpSock::AsyncTcpSock(boost::asio::io_service &io,
//                           const std::string &desc_,
//                           const Dest &d,
//                           int heart)
//    : desc(desc_),
//      dest(1, d),
//      heartbeat(heart),
//      strand(io),
//      sock(io),
//      resolver(io),
//      timerbeat(io),
//      timerconn(io),
//      cur_dest(0)
//{
//    /*
//     * Since rx and tx are constructed with const size and can not be changed,
//     * the maximum size of processed data at a time will be limited.
//     */
//    init();
//}

//AsyncTcpSock::AsyncTcpSock(boost::asio::io_service &io,
//                           const std::string &desc_,
//                           const std::vector<Dest> &d,
//                           int heart)
//    : desc(desc_),
//      dest(d),
//      heartbeat(heart),
//      strand(io),
//      sock(io),
//      resolver(io),
//      timerbeat(io),
//      timerconn(io),
//      cur_dest(0)
//{
//    /*
//     * Since rx and tx are constructed with const size and can not be changed,
//     * the maximum size of processed data at a time will be limited.
//     */
//    init();
//}

//AsyncTcpSock::AsyncTcpSock(boost::asio::io_service &io,
//             const std::string &desc_)     /* description. used in LogTraces, and Exception's messages */
//    : desc(desc_),
//      heartbeat(0),
//      strand(io),
//      sock(io),
//      resolver(io),
//      timerbeat(io),
//      timerconn(io),
//      cur_dest(0)
//{
//    /*
//     * Since rx and tx are constructed with const size and can not be changed,
//     * the maximum size of processed data at a time will be limited.
//     */
//    init();
//}



void AsyncTcpSock::init()
{
    LogTrace(TRACE5) << desc
                     << " AsyncTcpSock has been constructed. Its rx and tx buffers' max sizes respectively are "
                     << rx.max_size() << " and "
                     << tx.max_size();
    set_disconnected();
    set_notworking();
    write_unset_act();
    read_unset_act();
}

void AsyncTcpSock::start_connecting()
{
    if (is_working())
        stop_working();
    strand.post(boost::bind(&AsyncTcpSock::resolve, this));
}

void AsyncTcpSock::reconnect(const boost::system::error_code &err)
{
    if (!err) {
        return resolve();
    }
}

void AsyncTcpSock::resolve()
{
    if (cur_dest == dest.size()) {
//        LogTrace(TRACE5) << desc << " There are no more destinations to try to connect to";
        cur_dest = 0;
        set_notworking();
//        LogTrace(TRACE5) << desc << " calling usr_connect_failed_handler()";
        return usr_connect_failed_handler();
    }

    set_working();
    const Dest &d = dest[cur_dest++];
    boost::asio::ip::tcp::resolver::query query(d.host, d.port);
    LogTrace(TRACE5) << desc << " trying to resolve host from " << d.host;
    resolver.async_resolve(query, strand.wrap(boost::bind(&AsyncTcpSock::resolve_handler,
                                                          this,
                                                           _1,
                                                           _2)));
}

void AsyncTcpSock::usr_connect_failed_handler()
{
    std::string txt;
    txt = " connecting to all the resolved addresses was failed. i will try to connect in 5 secs. You might want to redefine this function";
    LogError(STDLOG) << desc << txt;
    do_start_reconnect_timer(5);
}


void AsyncTcpSock::resolve_handler(const boost::system::error_code &err,
                                   boost::asio::ip::tcp::resolver::iterator iterator)
{
    if (!err) {
        if (iterator == boost::asio::ip::tcp::resolver::iterator()) {
            LogError(STDLOG) << desc << " resolve has failed";
            return resolve();
        } else {
            resolv_iter = iterator;
//            LogTrace(TRACE5) << desc << " resolve has succeeded. Trying to connect";
            return connect();
        }

    } else if (err != boost::asio::error::operation_aborted) {
        LogError(STDLOG) << desc << " resolve error: " << err.message();
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

void AsyncTcpSock::send_with_excep(const char *d, const std::size_t n)
{
    /*
     * if we write to the socket when it is not connected, eventually
     * we will come to conn_broken_handler(). but it won't be good -
     * the connection wasn't broken - it just hadn't been established before.
     * so, better let's check it here
     */
    if (!is_connected()) {
        throw ServerFramework::Exception("",
                                         __FILE__,
                                         __LINE__,
                                         __FUNCTION__,
                                         desc + " AsyncTcpSock::send_with_excep() the socket is not connected)");
    }

    send(d, n);
}

void AsyncTcpSock::send(const char *d, const std::size_t n)
{
    Sharedbuf buf(d, n);
    strand.post(boost::bind(&AsyncTcpSock::do_send, this, buf));
}

void AsyncTcpSock::do_send(Sharedbuf buf)
{
    if (!is_connected()) {
        LogError(STDLOG) << desc << " AsyncTcpSock::do_send() writting to the socket when it is not connected";
        return;
    }

    try
    {
        tx.sputn(buf.get(), buf.size());
    }
    catch (const std::length_error &err)
    {
        throw ServerFramework::Exception("",
                                         __FILE__,
                                         __LINE__,
                                         __FUNCTION__,
                                         desc + " AsyncTcpSock::do_send() there is no free space in the tx buffer. Its max size is " + tostring(tx.max_size()));
    }

    if (!is_write_act())
        return write();
}


void AsyncTcpSock::stop_working()
{
    strand.post(boost::bind(&AsyncTcpSock::do_close, this));
}

void AsyncTcpSock::connect()
{
    const boost::asio::ip::tcp::endpoint ep = *resolv_iter;
//    LogTrace(TRACE5) << desc
//                     << " trying to connect to "
//                     << ep.address().to_string()
//                     << ":"
//                     << tostring(ep.port());

    sock.async_connect(ep, strand.wrap(boost::bind(&AsyncTcpSock::connect_handler, this, _1)));
    start_connect_timer();
}

void AsyncTcpSock::connect_handler(const boost::system::error_code &err)
{
    stop_connect_timer();

    const boost::asio::ip::tcp::endpoint ep = *resolv_iter;
    const std::string deststr = ep.address().to_string() + ":" + tostring(ep.port());

    if (!err) {
        cur_dest = 0;
        LogTrace(TRACE5) << desc
                         << " connection to "
                         << deststr
                         << " has been successfully established";

        write_unset_act();
        read_unset_act();
        set_connected();
        set_nodelay(true);
        read();
        usr_connect_handler();

    } else {
        sock.close();
        LogError(STDLOG) << desc
                         << " connection to "
                         << deststr
                         << " has been failed: "
                         << err.message();

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
        const std::string deststr = ep.address().to_string() + ":" + tostring(ep.port());
//        LogTrace(TRACE5) << desc << " connection to " << deststr << " has been failed: timeout passed. Cancelling..";
    }
}


void AsyncTcpSock::write()
{
    tmp_tx.assign(boost::asio::buffers_begin(tx.data()),
                  boost::asio::buffers_begin(tx.data()) + tx.size());

    boost::asio::async_write(sock, boost::asio::buffer(tmp_tx),
                             strand.wrap(boost::bind(&AsyncTcpSock::write_handler, this, _1, _2)));

    write_set_act();

    /* if we have written smth then we should defer the next heartbeat */
    restart_heartbeat();
}

void AsyncTcpSock::write_handler(const boost::system::error_code &err, std::size_t n)
{
    write_unset_act();
    if (!err) {
        tx.consume(n);
        if (tx.size() > 0)
            return write();

    } else if (err != boost::asio::error::operation_aborted) {
        LogError(STDLOG) << desc << " writing to socket failed: " << err.message();
        conn_broken_handler();
    }
}

void AsyncTcpSock::read()
{
    std::size_t free_space = rx.max_size() - rx.size();
    sock.async_receive(rx.prepare(free_space), strand.wrap(boost::bind(&AsyncTcpSock::read_handler, this, _1, _2)));
    read_set_act();
}

void AsyncTcpSock::read_handler(const boost::system::error_code &err, std::size_t n)
{
    /*
     * Even if a connection is broken it is possible
     * that data might be read from the socket.
     * Thus err could be 0 even if connection is already broken.
     */
    read_unset_act();


    if (err && n) {
        /*
         * GOD DAMN STRANGE ERROR!
         * I've never run into this problem, so, probably this IF section should
         * be completely removed!
         */
        LogError(STDLOG) << desc
                         << "Despite the error has occurred on socket we have read some bytes from it: "
                         << err.message();
        std::size_t consumed;
        const char *data_ptr;
        std::size_t data_sz;

        rx.commit(n);
        data_ptr = boost::asio::buffer_cast<const char*>(rx.data());
        data_sz = boost::asio::buffer_size(rx.data());
        consumed = usr_read_handler(data_ptr, data_sz);
        rx.consume(consumed);
        if (rx.size() < rx.max_size()) {
            return read();
        } else {
            do_close();
            const std::string txt = " AsyncTcpSock::read_handler() BAD ERROR. There is no free space in the rxbuffer. reading from socket is not more possible. Check the derived class!";
            throw ServerFramework::Exception("",
                                             __FILE__,
                                             __LINE__,
                                             __FUNCTION__,
                                             desc + txt);
        }
    }


    if (!err) {
        std::size_t consumed;
        const char *data_ptr;
        std::size_t data_sz;

        rx.commit(n);
        data_ptr = boost::asio::buffer_cast<const char*>(rx.data());
        data_sz = boost::asio::buffer_size(rx.data());
        consumed = usr_read_handler(data_ptr, data_sz);
        rx.consume(consumed);
        if (rx.size() < rx.max_size()) {
            return read();
        } else {
            do_close();
            const std::string txt = " AsyncTcpSock::read_handler() BAD ERROR. There is no free space in the rxbuffer. reading from socket is not more possible. Check the derived class!";
            throw ServerFramework::Exception("",
                                             __FILE__,
                                             __LINE__,
                                             __FUNCTION__,
                                             desc + txt);
        }


    } else if (err != boost::asio::error::operation_aborted) {
        LogError(STDLOG) << desc << " reading from the socket failed: " << err.message();
        return conn_broken_handler();
    }
}

void AsyncTcpSock::do_start_heartbeat()
{
    if (!heartbeat)
        return;

    timerbeat.expires_from_now(boost::posix_time::milliseconds(heartbeat));
    timerbeat.async_wait(strand.wrap(boost::bind(&AsyncTcpSock::heartbeat_handler, this, _1)));
}

void AsyncTcpSock::restart_heartbeat()
{
    /* if heartbeat has not been started by user or its interval is 0 then we do nothing*/
    if (timerbeat.cancel() && heartbeat != 0)
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
    strand.post(boost::bind(&AsyncTcpSock::do_stop_heartbeat, this));
}

void AsyncTcpSock::do_stop_heartbeat()
{
    timerbeat.cancel();
}


void AsyncTcpSock::conn_broken_handler()
{
    /*
     * If anything error occurs and connection gets broken
     * but socket has still data in its rx buffer,
     * there is likelihood writing handler will call this function.
     * In order not to lose data we use is_read_act() which
     * returns true until socket has data
     */
    set_disconnected();    

    if (is_read_act() || is_write_act())
        return;

    tx.consume(tx.size());
    rx.consume(rx.size());
    stop_heartbeat();
    cur_dest = 0;
    sock.close();
    set_notworking();
    return usr_conn_broken_handler();
}

void AsyncTcpSock::usr_conn_broken_handler()
{
    std::string txt;
    /*
     * i don't use line break here
     * so that user may grep this
     */
    txt = " usr_conn_broken_handler() the connection is broken. I will try to reconnect in 5 secs. You might want to redefine this function";
    LogError(STDLOG) << desc << txt;
    do_start_reconnect_timer(5);
}


void AsyncTcpSock::do_set_dest(std::vector<Dest> v)
{
    dest = v;
    cur_dest = 0;
}

void AsyncTcpSock::start_connect_timer()
{
    /*
     * Probably, it is better to make this parameter also configurable! now it's 10 SECONDS!!!
     */
    timerconn.expires_from_now(boost::posix_time::seconds(10));
    timerconn.async_wait(strand.wrap(boost::bind(&AsyncTcpSock::connect_timeout_handler, this, _1)));
}

void AsyncTcpSock::start_reconnect_timer(int delay)
{
    strand.post(boost::bind(&AsyncTcpSock::do_start_reconnect_timer, this, delay));
}

void AsyncTcpSock::do_start_reconnect_timer(int delay)
{
    timerreconn.expires_from_now(boost::posix_time::seconds(delay));
    timerreconn.async_wait(strand.wrap(boost::bind(&AsyncTcpSock::reconnect, this, _1)));
}

void AsyncTcpSock::stop_connect_timer()
{
    timerconn.cancel();
}

void AsyncTcpSock::stop_reconnect_timer()
{
    timerreconn.cancel();
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
    set_notworking();
    sock.close();
    stop_connect_timer();
    do_stop_heartbeat();
    stop_reconnect_timer();
    resolver.cancel();
    cur_dest = 0;
    tx.consume(tx.size());
    rx.consume(rx.size());
}

void AsyncTcpSock::set_nodelay(bool val)
{
    boost::asio::ip::tcp::no_delay option(val);
    sock.set_option(option);
}


void AsyncTcpSock::set_connected()
{
    lock_mutex_connected();
    connected = true;
    unlock_mutex_connected();
}

void AsyncTcpSock::set_disconnected()
{
    lock_mutex_connected();
    connected = false;
    unlock_mutex_connected();
}

bool AsyncTcpSock::is_connected()
{
    lock_mutex_connected();
    bool r = connected;
    unlock_mutex_connected();
    return r;
}

void AsyncTcpSock::set_working()
{
    lock_mutex_working();
    working = true;
    unlock_mutex_working();
}

void AsyncTcpSock::set_notworking()
{
    lock_mutex_working();
    working = false;
    unlock_mutex_working();
}

bool AsyncTcpSock::is_working()
{
    lock_mutex_working();
    bool r = working;
    unlock_mutex_working();
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
void AsyncTcpSock::lock_mutex_connected()
{
    return mutex_connected.lock();
}

void AsyncTcpSock::unlock_mutex_connected()
{
    return mutex_connected.unlock();
}

void AsyncTcpSock::lock_mutex_working()
{
    return mutex_working.lock();
}

void AsyncTcpSock::unlock_mutex_working()
{
    return mutex_working.unlock();
}
#else
void AsyncTcpSock::lock_mutex_connected()
{
}

void AsyncTcpSock::unlock_mutex_connected()
{
}

void AsyncTcpSock::lock_mutex_working()
{
}

void AsyncTcpSock::unlock_mutex_working()
{
}
#endif


} // namespace asyncnet
