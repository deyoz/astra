#ifndef _ASYNCNET_HPP_
#define _ASYNCNET_HPP_

#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
#include <string>
#include <queue>
#include <vector>

#ifdef USE_THREADS
    #include <boost/thread/mutex.hpp>
#endif

#include <stdio.h>
#include <string.h>


namespace asyncnet {

class Buf {
public:
    Buf(const std::size_t sz);

    const void *get_data() const
    {
        return arr.get();
    }
    const std::size_t get_length() const
    {
        return cursor;
    }
    void fillwith(const int c, const std::size_t n)
    {
        prepare(n);
        memset(arr.get() + cursor, c, n);
        cursor += n;
    }

    void reset()
    {
        cursor = 0;
    }


    void add(const void *mem, const std::size_t n)
    {
        prepare(n);
        copy(mem, n);
    }

    void operator<<(const unsigned int a)
    {
        prepare(sizeof(a));
        copy(&a, sizeof(a));
    }

    void operator<<(const unsigned short a)
    {
        prepare(sizeof(a));
        copy(&a, sizeof(a));
    }

    void operator<<(const unsigned long a)
    {
        prepare(sizeof(a));
        copy(&a, sizeof(a));
    }

    void operator<<(const unsigned char c)
    {
        prepare(sizeof(c));
        copy(&c, sizeof(c));
    }

    void operator<<(const std::string &str)
    {
        prepare(str.size());
        copy(str.data(), str.size());
    }

/*    void operator<<(const char *)
    {
        std::size_t len = strlen(p);
        prepare(len);
        copy(p, len);
    }
*/
private:
    boost::shared_array<char> arr;
    std::size_t total_size;
    std::size_t cursor;

    void prepare(std::size_t n)
    {
        if (cursor + n > total_size) {
            reallo(n);
            /* swearing here */
        }
    }

    void copy(const void *p, std::size_t n)
    {
        memcpy(arr.get() + cursor, p, n);
        cursor += n;
    }

    void reallo(std::size_t sz)
    {
        std::size_t new_sz = total_size + sz;

        /* let it fit the pagesize.
         * but probably the pagesize is not 4096 there where it will run
         */
        new_sz = (new_sz + 4095) & (~4095);

        boost::shared_array<char> new_arr(new char[new_sz]);
        memcpy(new_arr.get(), arr.get(), cursor);

        arr = new_arr;
        total_size = new_sz;
    }
};



struct Dest {
    Dest(const std::string &h, const std::string &p) : host(h), port(p)
    {
    }
    Dest(const std::string &h, const int p) : host(h), port("")
    {
        char buf[8];
        if (p < 0 || p > 65535)
            throw;
        /* are u gonna like it? */
        sprintf(buf, "%d", p);
        port = buf;
    }
    std::string   host;
    std::string   port;
};






class AsyncTcpSock : boost::noncopyable {
protected:
    AsyncTcpSock(boost::asio::io_service &io,
                 const std::string &desc_,
                 const Dest &d,
                 const bool infinity_,
                 const int heartbeat_,
                 const bool keepconn_,
                 std::size_t rxmax = 1024 * 16,
                 std::size_t txmax = 1024 * 16);

    AsyncTcpSock(boost::asio::io_service &io,
                 const std::string &desc,
                 const std::vector<Dest> &,
                 const bool infinity_,
                 const int heartbeat_,
                 const bool keepconn_,
                 std::size_t rxmax = 1024 * 16,
                 std::size_t txmax = 1024 * 16);

    void start_connecting();
    void start_heartbeat();
    void set_dest(const Dest &);
    void set_dest(const std::vector<Dest> &);
    void set_heartbeat(const int);
    void set_infinity(const bool);
    int send(const void *, const std::size_t);
    int send(std::string &);
//    int send(const Buf &);
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
//    TxQueue                                     tx;
//    Rxbuf                                       rx;
    boost::asio::streambuf                      rx;
    boost::asio::streambuf                      tx;

    bool                                        connected; /* don't know how to avoid using of this */
#ifdef USE_THREADS
    boost::mutex                                mutex_connected;
#endif

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
    virtual void usr_connect_failed_handler() {}
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
    int push_in_tx(const Txbuf &);

};









} // namespace asyncnet


#endif //_ASYNCNET_HPP_
