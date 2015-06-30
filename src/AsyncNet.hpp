#ifndef _ASYNCNET_HPP_
#define _ASYNCNET_HPP_

#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
#include <string>
#include <queue>
#include <vector>
#include "stl_utils.h"
#include "serverlib/exception.h"

#ifdef USE_THREADS
    #include <boost/thread/mutex.hpp>
#endif

#include <unistd.h> /* for sysconf() */
#ifndef _BSD_SOURCE
 #define _BSD_SOURCE
#endif
#include <endian.h> /* for htole16(), htobe16(), le16toh(), be16toh() */


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
        prepare(sz);
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

    void fillwith(int c, const std::size_t n)
    {
        prepare(n);
        memset(waddr(), c, n);
        wcursor += n;
        data_sz += n;
    }

    void append(const void *data, std::size_t n)
    {
        copy(data, n);
    }

    const char *data()
    {
        return raddr();
    }

    std::size_t size()
    {
        return data_sz;
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
        us = *(uint16_t *)raddr();

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
        str.append(raddr(), data_sz);
        consume(data_sz);
        return *this;
    }

    std::string take_string(std::size_t n)
    {
        check(n);

        std::string str(raddr(), n);
        consume(n);
        return str;
    }

private:
    std::vector<char>   arr;
    std::size_t         wcursor;
    std::size_t         rcursor;
    std::size_t         data_sz;
    std::size_t         rcursor_threshold;
    endianness_t        byteorder;

    void init()
    {
        rcursor = 0;
        wcursor = 0;
        data_sz = 0;
        init_rcursor_threshold();
    }

    void init_rcursor_threshold()
    {
        long pagesize = sysconf(_SC_PAGESIZE);
        /*
         * Some processor architectures use the huge page size.
         * We aren't gonna deal with this and just will use 4096
         */
        if (pagesize > 8192 || pagesize == -1)
            rcursor_threshold = 4096 * 100;
        else
            rcursor_threshold = pagesize * 100;
    }

    void prepare(std::size_t n)
    {
        arr.resize(wcursor + n);
    }

    char *raddr()
    {
        return &arr[0] + rcursor;
    }

    char *waddr()
    {
        return &arr[0] + wcursor;
    }

    void copy(const void *data, std::size_t n)
    {
        if (rcursor >= rcursor_threshold) {
            arr.erase(arr.begin(), arr.begin() + rcursor);
            wcursor -= rcursor;
            rcursor = 0;
        }

        prepare(n);
        memcpy(waddr(), data, n);
        wcursor += n;
        data_sz += n;
    }

    void consume(std::size_t n)
    {
        if (n > data_sz) {
            std::string str = "conume: " + tostring(n) + ", consists: " + tostring(data_sz);
            throw ServerFramework::Exception("",
                                             __FILE__,
                                             __LINE__,
                                             __FUNCTION__,
                                             " Netbuf::consume() trying to consume more than it consists: " + str);
        }

        rcursor += n;
        data_sz -= n;
    }

    void check(std::size_t n)
    {
        if (n > data_sz) {
            std::string str = "conume: " + tostring(n) + ", consists: " + tostring(data_sz);
            throw ServerFramework::Exception("",
                                             __FILE__,
                                             __LINE__,
                                             __FUNCTION__,
                                             " Netbuf::check() trying to consume more than it consists: " + str);
        }
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
        port = tostring(p);
    }
    Dest()
    {
    }
    bool operator!=(const Dest &o) const
    {
        return host != o.host || port != o.port;
    }

    std::string   host;
    std::string   port;
};






class AsyncTcpSock : boost::noncopyable {
public:
    virtual ~AsyncTcpSock()
    {
    }

    /*
     * Probably it will be better to hide these ones.
     * Also i recommend  you to wrap all actions on the strand or to use mutex.
     */

    void start_connecting();
    void start_heartbeat();
    void start_reconnect_timer(int);
    void stop_working();
    void stop_heartbeat();
    void set_dest(const Dest &);
    void set_dest(const std::vector<Dest> &);
    void set_heartbeat(int);
    void send_with_excep(const char *, const std::size_t);
    void send(const char *, const std::size_t);
    bool is_working();
    bool is_connected();
protected:
    /*
     * IMPORTANT!
     * Since rx and tx are constructed with const size and can not be changed,
     * the maximum size of processed data at a time will be limited.
     * Choose the sizes that fit your needs!
     */
    AsyncTcpSock(boost::asio::io_service &io,
                 const std::string &desc_,     /* description. used in LogTraces, and Exception's messages */
                 const Dest &d,
                 int heartbeat_,
                 std::size_t rxmax,
                 std::size_t txmax);

    AsyncTcpSock(boost::asio::io_service &io,
                 const std::string &desc,     /* description. used in LogTraces, and Exception's messages */
                 const std::vector<Dest> &,
                 int heartbeat_,
                 std::size_t rxmax,
                 std::size_t txmax);

    AsyncTcpSock(boost::asio::io_service &io,
                 const std::string &desc,     /* description. used in LogTraces, and Exception's messages */
                 std::size_t rxmax,
                 std::size_t txmax);

//    AsyncTcpSock(boost::asio::io_service &io,
//                 const std::string &desc_,     /* description. used in LogTraces, and Exception's messages */
//                 const Dest &d,
//                 int heartbeat_);

//    AsyncTcpSock(boost::asio::io_service &io,
//                 const std::string &desc,     /* description. used in LogTraces, and Exception's messages */
//                 const std::vector<Dest> &,
//                 int heartbeat_);

//    AsyncTcpSock(boost::asio::io_service &io,
//                 const std::string &desc);     /* description. used in LogTraces, and Exception's messages */
private:
    const std::string                           desc; /* description */
    std::vector<Dest>                           dest;
    int                                         heartbeat; /* heartbeat interval in milliseconds */
    boost::asio::strand                         strand;
    boost::asio::ip::tcp::socket                sock;
    boost::asio::ip::tcp::resolver              resolver;
    boost::asio::ip::tcp::resolver::iterator    resolv_iter;
    boost::asio::deadline_timer                 timerbeat;
    boost::asio::deadline_timer                 timerconn;
    boost::asio::deadline_timer                 timerreconn;  /* e.g. to delay between reconnection */

    unsigned int                                cur_dest; /* index of next destination to which we will try to connect */

    boost::asio::streambuf                      rx;
    boost::asio::streambuf                      tx;

    /*
     * can't assure u whether tmp_tx is necessary
     */
    std::string                                 tmp_tx;
    bool                                        connected; /* don't know how to avoid using of this */
    bool                                        working; /*  */
    bool                                        read_act; /* true if read_handler is put into boost handlers queue till it is called */
    bool                                        write_act; /* true if write_handler is put into boost handlers queue till it is called */
#ifdef USE_THREADS
    boost::mutex                                mutex_connected;
    boost::mutex                                mutex_working;
#endif

    void read_set_act();
    void read_unset_act();
    bool is_read_act();
    void write_set_act();
    void write_unset_act();
    bool is_write_act();
    void set_connected();
    void set_disconnected();
    void set_working();
    void set_notworking();
    void connect();
    inline void lock_mutex_connected();
    inline void unlock_mutex_connected();
    inline void lock_mutex_working();
    inline void unlock_mutex_working();
    void resolve();
    void reconnect(const boost::system::error_code &);
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
    /*
     * usr_read_handler():
     * If received chunk is not enough to be processed -
     * just use return 0 or copy it and return its size.
     *
     * If you no longer need some chunk of data - return its size.
     *
     * The pointer to data is guaranteed to be valid only until the moments this function returns,
     * so, in order to use it at any other moment you like then you should copy the data pointed by the pointer to your buffers (e.g. memcpy).
     */
    virtual std::size_t usr_read_handler(const char *, std::size_t) = 0;
    void conn_broken_handler();
    virtual void usr_conn_broken_handler();
    void do_start_heartbeat();
    void do_stop_heartbeat();
    void restart_heartbeat();
    void heartbeat_handler(const boost::system::error_code &);
    virtual void usr_heartbeat_handler() {}
    void do_set_dest(std::vector<Dest>);
    void do_close();
    void start_connect_timer();
    void do_start_reconnect_timer(int);
    void stop_connect_timer();
    void stop_reconnect_timer();
    void do_set_heartbeat(int);
    void set_nodelay(bool);
    void init();
};









} // namespace asyncnet


#endif //_ASYNCNET_HPP_
