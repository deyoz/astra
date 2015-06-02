#ifndef _ASYNCNET_HPP_
#define _ASYNCNET_HPP_

#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
#include <string>
#include <queue>
#include <vector>
#include "astra_utils.h"

#ifdef USE_THREADS
    #include <boost/thread/mutex.hpp>
#endif




namespace asyncnet {

struct Sharedbuf {
    Sharedbuf(const char *d, std::size_t len_) : arr(new char[len_]), len(len_)
    {
        memcpy(arr.get(), d, len);
    }
    char *get()
    {
        return arr.get();
    }
    std::size_t size()
    {
        return len;
    }

private:
    boost::shared_array<char>   arr;
    std::size_t                 len;



struct Dest {
    Dest(const std::string &h, const std::string &p) : host(h), port(p)
    {
    }
    Dest(const std::string &h, const int p) : host(h)
    {
        port = to_string(p);
    }
    std::string   host;
    std::string   port;
};






class AsyncTcpSock : boost::noncopyable {
protected:
    AsyncTcpSock(boost::asio::io_service &io,
                 const std::string &desc_,
                 const Dest &d,
                 const bool infinity_, /* if i should try to connect forever */
                 const int heartbeat_,
                 const bool keepconn_,
                 std::size_t rxmax = 1024 * 4,
                 std::size_t txmax = 1024 * 4);

    AsyncTcpSock(boost::asio::io_service &io,
                 const std::string &desc,
                 const std::vector<Dest> &,
                 const bool infinity_, /* if i should try to connect forever */
                 const int heartbeat_,
                 const bool keepconn_,
                 std::size_t rxmax = 1024 * 4,
                 std::size_t txmax = 1024 * 4);

    void start_connecting();
    void start_heartbeat();
    void set_dest(const Dest &);
    void set_dest(const std::vector<Dest> &);
    void set_heartbeat(const int);
    void set_infinity(const bool);
    void send(const void *, const std::size_t);
    void close();

private:
    const std::string                           desc; /* description */
    boost::asio::strand                         strand;
    boost::asio::ip::tcp::socket                sock;
    boost::asio::ip::tcp::endpoint              current;
    boost::asio::ip::tcp::resolver              resolver;
    boost::asio::ip::tcp::resolver::iterator    resolv_iter;
    int                                         heartbeat;
    boost::asio::deadline_timer                 timer;
    std::vector<Dest>                           dest;
    int                                         cur_dest;
    bool                                        infinite;   // try to connect until it succeeds
    bool                                        keepconn;

    boost::asio::streambuf                      tx;
    boost::asio::streambuf                      rx;

    bool                                        connected; /* don't know how to avoid using of this */
    bool                                        read_act;
    bool                                        write_act;
#ifdef USE_THREADS
    boost::mutex                                mutex_connected;
#endif


    void read_set_act();
    void read_unset_act();
    bool is_read_act();
    void write_set_act();
    void write_unset_act();
    bool is_write_act();

    void set_connected();
    void set_disconnected();
    bool is_connected();
    void connect();
    inline void lock();
    inline void unlock();
    void resolve();
    void resolve_handler(const boost::system::error_code &, boost::asio::ip::tcp::resolver::iterator);
    void connect_handler(const boost::system::error_code &);
    virtual void usr_connect_handler() {}
    void connect_timeout_handler(const boost::system::error_code &);
    virtual void usr_connect_failed_handler() {} /* when all the attempts are failed */
    void push_into_tx(Sharedbuf);
    virtual void write();
    void write_handler(const boost::system::error_code &, std::size_t);
    virtual void read();
    void read_handler(const boost::system::error_code &, std::size_t);
    virtual std::size_t usr_read_handler(const char *, std::size_t) {}
    void conn_broken_handler();
    virtual void usr_conn_broken_handler() {}
    void do_start_heartbeat();
    void restart_heartbeat();
    void heartbeat_handler(const boost::system::error_code &);
    virtual void usr_heartbeat_handler() {}
    void stop_heartbeat();
    void do_set_infinity(const bool);
    void do_set_dest(const std::vector<Dest>);
    void do_close();
    void start_connect_timer();
    void stop_connect_timer();
    void do_set_heartbeat(const int);
    void set_nodelay(bool);
};









} // namespace asyncnet


#endif //_ASYNCNET_HPP_
