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

#include <unistd.h> /* for sysconf() */
#define _BSD_SOURCE
#include <endian.h>


namespace asyncnet {


struct Sharedbuf {
    Sharedbuf(const char *d, std::size_t len_) : arr(new char[len_]), len(len_)
    {
        memcpy(arr.get(), d, len);
    }
    const char *get()
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
};

struct Netbuf {
    /*
     * All the operators>> and << are called with integer arguments
     * convert the byte encoding of integer values between
     * the host order and chosen order of this object if they are diffirent.
     * it doesn't convert content of strings.
     */
    typedef enum { BIGENDIAN, LITTLEENDIAN } endianness_t;

    Netbuf(endianness_t e)
        : byteorder(e)
    {
        init();
    }


    Netbuf(std::size_t sz, endianness_t e)
        : byteorder(e)
    {
        init();
        arr.reserve(sz);
    }

    Netbuf(const void *data, std::size_t n, endianness_t e)
        : byteorder(e)
    {
        init();
        copy(data, n);
    }

    Netbuf &operator<<(uint16_t nu)
    {
        if (byteorder == LITTLEENDIAN)
            nu = htole16(nu);
        else
            nu = htobe16(nu);

        copy((const void *)&nu, sizeof(nu));
        return *this;
    }

    Netbuf &operator<<(const std::string &str)
    {
        copy(str.c_str(), str.size());
        return *this;
    }

    void fillwith(const int c, const std::size_t n)
    {
        resize(used + n);
        memset(addr() + used, c, n);
        used += n;
    }

    void append(const void *data, std::size_t n)
    {
        copy(data, n);
    }

    const char *data()
    {
        return addr();
    }

    std::size_t size()
    {
        return used;
    }

    void skip(std::size_t n)
    {
        check(n);
        consume(n);
    }

    Netbuf &operator>>(uint16_t &nu)
    {
        check(sizeof(nu));

        uint16_t us;
        us = *(uint16_t *)addr();

        if (byteorder == LITTLEENDIAN)
            us = le16toh(us);
        else
            us = be16toh(us);

        nu = us;
        consume(sizeof(nu));
        return *this;
    }

    Netbuf &operator>>(std::string &str)
    {
        str.append(addr(), used);
        consume(used);
        return *this;
    }

    std::string take_string(std::size_t n)
    {
        check(n);

        std::string str(addr(), n);
        consume(n);
        return str;
    }

private:
    std::vector<char>   arr;
    std::size_t         used;
    std::size_t         rcursor;
    std::size_t         rcursor_threshold;
    endianness_t        byteorder;

    void init()
    {
        rcursor = 0;
        used = 0;
        init_rcursor_threshold();
    }

    void init_rcursor_threshold()
    {
        long pagesize = sysconf(_SC_PAGESIZE);
        /*
         * Some processor architectures use the huge page size.
         * We aren't gonna deal with this and just will use 4096
         */
//        printf("pagesize = %ld\n", pagesize);
        if (pagesize > 8192 || pagesize == -1)
            rcursor_threshold = 4096;
        else
            rcursor_threshold = pagesize;
//        perror("sysconf");
//        printf("rcursor_threshold = %ld\n", rcursor_threshold);
    }

    void resize(std::size_t n)
    {
        arr.resize(n);
    }

    char *addr()
    {
        return &arr[0] + rcursor;
    }

    void copy(const void *data, std::size_t n)
    {
        resize(used + n);
        memcpy(addr() + used, data, n);
        used += n;
    }

    void consume(std::size_t n)
    {
        if (n > used)
            throw;

        used -= n;
        rcursor += n;
        /*
         * To avoid often copying to the beginning of the array
         */
        if (rcursor >= used && rcursor >= rcursor_threshold) {
            arr.erase(arr.begin(), arr.begin() + rcursor);
            rcursor = 0;
        }
    }

    void check(std::size_t n)
    {
        if (n > used)
            throw;
    }

/*    template<class T>
    void consume(T &o)
    {
        if (sizeof(T) > used)
            throw;
        char *ptr = addr();
        std::size_t n = sizeof(T);
        o = *(T*)ptr;
        memmove(ptr, ptr + n, used - n);
        used -= n;
    }
*/
};



struct Dest {
    Dest(const std::string &h, const std::string &p) : host(h), port(p)
    {
    }
    Dest(const std::string &h, const int p) : host(h)
    {
        port = to_string(p);
    }
    Dest()
    {
    }
    std::string   host;
    std::string   port;
};






class AsyncTcpSock : boost::noncopyable {
public:
    virtual ~AsyncTcpSock()
    {
    }

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
    void send(const char *, const std::size_t);
    void close();
    bool is_connected();

private:
    const std::string                           desc; /* description */
    std::vector<Dest>                           dest;
    bool                                        infinite;   // try to connect until it succeeds
    int                                         heartbeat;
    bool                                        keepconn;
    boost::asio::strand                         strand;
    boost::asio::ip::tcp::socket                sock;
    boost::asio::ip::tcp::endpoint              current;
    boost::asio::ip::tcp::resolver              resolver;
    boost::asio::ip::tcp::resolver::iterator    resolv_iter;
    boost::asio::deadline_timer                 timer;

    unsigned int                                cur_dest;

    boost::asio::streambuf                      rx;
    boost::asio::streambuf                      tx;

//    RxBuffer                                    rx;
//    TxQueue                                     tx;


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
    void connect();
    inline void lock();
    inline void unlock();
    void resolve();
    void resolve_handler(const boost::system::error_code &, boost::asio::ip::tcp::resolver::iterator);
    void connect_handler(const boost::system::error_code &);
    virtual void usr_connect_handler() {}
    void connect_timeout_handler(const boost::system::error_code &);
    virtual void usr_connect_failed_handler(); /* when all the attempts are failed */
    void do_send(Sharedbuf);
    virtual void write();
    void write_handler(const boost::system::error_code &, std::size_t);
    virtual void read();
    void read_handler(const boost::system::error_code &, std::size_t);
    virtual std::size_t usr_read_handler(const char *, std::size_t) = 0;
    void conn_broken_handler();
    virtual void usr_conn_broken_handler();
    void do_start_heartbeat();
    void restart_heartbeat();
    void heartbeat_handler(const boost::system::error_code &);
    virtual void usr_heartbeat_handler() {}
    void stop_heartbeat();
    void do_set_infinity(bool);
    void do_set_dest(std::vector<Dest>);
    void do_close();
    void start_connect_timer();
    void stop_connect_timer();
    void do_set_heartbeat(int);
    void set_nodelay(bool);
};









} // namespace asyncnet


#endif //_ASYNCNET_HPP_
