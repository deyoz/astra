#ifndef _DEPECHES_HPP_
#define _DEPECHES_HPP_

#include "AsyncNet.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <map>
#include <list>
#include <string>
#include <bitset>


namespace depeches {


struct Clock {
    Clock()
    {
        current();
    }

    Clock(int sec)
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


private:
    void current()
    {
        if (clock_gettime(CLOCK_MONOTONIC, &tp)) {
            printf("clock_gettime() error\n");
        } else {
        }
    }
    void add_sec(int sec)
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
    virtual ~DepecheSettings()
    {
    }
};

/* ========================================================================== */

typedef int depeche_id_t;

struct Depeche {
    typedef enum { OK = 0, EXPIRED, NO_FREE_SLOT, FAIL } depeche_status_t;

    Depeche()
    {
    }

    Depeche(const std::string &tlg_,
            const DepecheSettings &s,
            const depeche_id_t id_,
            const int timeout)
        : tlg(tlg_),
          id(id_),
          clk(timeout)
    {
        settings.reset(s.get_copy());
    }


    std::string                             tlg;
    boost::shared_ptr<DepecheSettings>      settings;
    depeche_id_t                            id; /* external id */
    Clock                                   clk; /* it is a moment when it might be considered as expired */
};




struct BagmessageSettings : DepecheSettings {
    BagmessageSettings() {}
    BagmessageSettings(const std::string &ip,
                      int port,
                      const std::string &appid_,
                      const std::string &passwd_,
                      bool keepconn,
                      int heartbeat)
        : dest(ip, port),
          appid(appid_),
          passwd(passwd_),
          keep_connection(keepconn),
          heartbeat_interval(heartbeat)
    {
        if (appid.size() > 8)
            throw;
    }

    BagmessageSettings(const asyncnet::Dest &d,
                      const std::string &appid_,
                      const std::string &passwd_,
                      bool keepconn,
                      int heartbeat)
        : dest(d),
          appid(appid_),
          passwd(passwd_),
          keep_connection(keepconn),
          heartbeat_interval(heartbeat)
    {
        if (appid.size() > 8)
            throw;
    }

    virtual BagmessageSettings *get_copy()
    {
        return new BagmessageSettings(*this);
    }


    asyncnet::Dest      dest;
    std::string         appid;
    std::string         passwd;
    bool                keep_connection;
    int                 heartbeat_interval;
};


/* ========================================================================== */


class Bagmessage : asyncnet::AsyncTcpSock {
public:
    Bagmessage(boost::asio::io_service &io,
               const BagmessageSettings &,
               boost::function<void(depeche_id_t, Depeche::depeche_status_t)>,
               const std::string &desc_);

    void send_depeche(const std::string &tlg,
                      const BagmessageSettings &set,
                      depeche_id_t,
                      int timeout);


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

    enum msg_type_t {
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

    typedef enum {
        PARSER_HEADER,
        PAR_BODY,
        PARSER_ERR

    } parsing_state_t;

    parsing_state_t             state;
    bool                        logged_on;
    boost::asio::strand         strand;
    BagmessageSettings          settings;
    std::bitset<65536>          bitset;
    std::map<int, Depeche>      deps;

    boost::function<void(depeche_id_t, Depeche::depeche_status_t)>    usrcallback;

    const std::string           desc; /* description */





    void do_send_depeche(Depeche );
    void save_depeche(int mess_id, const Depeche &);
    void real_send(int, const Depeche &);

    void bitset_init();
    int bitset_get_fz(); /* gives the first zero bit */
    void bitset_return_bit(int i); /* frees bit */

    bool is_logged_on();
    void set_log_on();
    void set_log_off();


    void header_build(asyncnet::Netbuf &buf, enum msg_type_t, int mes_number, int data_length);
    void message_build(asyncnet::Netbuf &buf, enum msg_type_t, int mes_number, const std::string &data);
    void message_build(asyncnet::Netbuf &buf, enum msg_type_t, int mes_number);
    void login();


    virtual void usr_connect_handler();
    virtual std::size_t usr_read_handler(const char *, std::size_t);
    virtual void usr_heartbeat_handler();


    parsing_state_t header_parser(const char *data);
    std::size_t expected_size();

};





} //namespace depeches





#endif
