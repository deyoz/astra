#ifndef _DEPECHES_HPP_
#define _DEPECHES_HPP_

#include "AsyncNet.hpp"

#include <boost/shared_ptr.hpp>
#include <map>
#include <list>
#include <string>


namespace depeches {


struct Clock {
    Clock()
    {
        current();
    }

    Clock(const int sec)
    {
        current();
        add_sec(sec);
    }

    bool operator<(const Clock &other) const
    {
        return tp.tv_sec < other.tp.tv_sec;
    }

    bool is_expired() const
    {
        Clock clk;
        return *this < clk;
    }

    void print()
    {
        printf("sec is %lu", tp.tv_sec);
    }

private:
    void current()
    {
        if (clock_gettime(CLOCK_MONOTONIC, &tp)) {
            printf("clock_gettime() error\n");
        } else {
        }
    }
    void add_sec(const int sec)
    {
        tp.tv_sec += sec;
    }

    struct timespec tp;
};
/* ========================================================================== */

struct DepecheSettings {
    virtual DepecheSettings *get_copy() const
    {
        return new DepecheSettings(*this);
    }
};

/* ========================================================================== */

typedef int depeche_id_t;

struct Depeche {
    typedef enum { OK = 0, EXPIRED } depeche_status;

    Depeche(const std::string &tlg_,
            const DepecheSettings &s,
            const depeche_id_t id_,
            const int timeout)
        : clk(timeout),
          id(id_),
          tlg(tlg_)
    {
        settings.reset(s.get_copy());
    }


    const std::string                       tlg;
    boost::shared_ptr<DepecheSettings>      settings;
    const depeche_id_t                      id; /* external id */
    const Clock                             clk; /* it is a moment when it might be considered as expired */
};




struct BagmessageSetting : DepecheSettings {
    BagmessageSetting() {}
    BagmessageSetting(const std::string &ip,
                      int port,
                      const std::string &appid_,
                      const std::string &passwd_,
                      bool keepconn,
                      int heartbeat)
        : dest(ip, port),
          appid_max(8),
          appid(appid_, 0, appid_max),
          passwd(passwd_),
          keep_connection(keepconn),
          heartbeat_interval(heartbeat)
    {
    }

    BagmessageSetting(const asyncnet::Dest &d,
                      const std::string &appid_,
                      const std::string &passwd_,
                      bool keepconn,
                      int heartbeat)
        : dest(d),
          appid_max(8),
          appid(appid_, 0, appid_max),
          passwd(passwd_),
          keep_connection(keepconn),
          heartbeat_interval(heartbeat)
    {
    }

    virtual BagmessageSetting *get_copy()
    {
        return new BagmessageSetting(*this);
    }


    asyncnet::Dest      dest;
    const int           appid_max;
    std::string         appid;
    std::string         passwd;
    bool                keep_connection;
    int                 heartbeat_interval;
};


/* ========================================================================== */


class Bagmessage : asyncnet::AsyncTcpSock {
public:
    Bagmessage(boost::asio::io_service &io,
               const BagmessageSetting &,
               boost::function<void(depeche_id_t, Depeche::depeche_status)>);

    send_depeche(const Depeche &);


private:
/*
 *      struct __attribute__ ((__packed__)) header {
 *      char              appid[8]; any unused bytes should be zero-filled
 *      unsigned short    version;
 *      unsigned short    type;
 *      unsigned short    num;
 *      unsigned short    len;
 *      char              reserved[4];
 *    };
 */

    enum msg_type {
        LOGIN_RQST = 1,
        LOGIN_ACCEPT,
        LOGIN_REJECT,
        DATA,
        ACK_DATA,
        ACK_MSG,
        NAK_MSG,
        STATUS,
        DATA_ON,
        DATA_OFF,
        LOG_OFF
    };

    typedef enum { PARSER_HEADER, PARSER_BODY, PARSER_ERR } parser_state;


    BagmessageSetting           settings;
    parser_state                state;
    std::bitset<65536>          bitset;
    std::map<int, >

    void bitset_reset();
    int bitset_get_fz(); /* get first zero */
    void bitset_return_bit(const int i);

    void header_build(net::Buf &buf, const enum msg_type, const int mes_number, const int data_length);
    void message_build(net::Buf &buf, const enum msg_type, const int mes_number, const std::string &data);
    void message_build(net::Buf &buf, const enum msg_type, const int mes_number);
    void login();



    enum parser_state header_parser(const void *data);
    const std::size_t expected_size();

};





} //namespace depeches





#endif
