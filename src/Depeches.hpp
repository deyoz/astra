#ifndef _DEPECHES_HPP_
#define _DEPECHES_HPP_

#include "AsyncNet.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <map>
#include <list>
#include <string>
#include <bitset>
#include <set>
#include <time.h> /* for clock_gettime() */

#include "serverlib/exception.h"

namespace depeches {


struct Clock {
    Clock()
    {
        current();
    }
    Clock(int msec)
    {
        current();
        add_msec(msec);
    }
    Clock(const Clock &o)
    {
        tp = o.tp;
    }
    const Clock& operator=(const Clock &o)
    {
        tp = o.tp;
        return *this;
    }
    bool operator<(const Clock &o) const
    {
        if (tp.tv_sec != o.tp.tv_sec) {
            if (tp.tv_sec < o.tp.tv_sec)
                return true;
            else
                return false;
        } else if (tp.tv_nsec < o.tp.tv_nsec) {
            return true;
        } else {
            return false;
        }
    }
    bool operator>(const Clock &o) const
    {
        if (tp.tv_sec != o.tp.tv_sec) {
            if (tp.tv_sec > o.tp.tv_sec)
                return true;
            else
                return false;
        } else if (tp.tv_nsec > o.tp.tv_nsec) {
            return true;
        } else {
            return false;
        }
    }
    bool is_expired() const
    {
        Clock clk;
        return *this < clk;
    }
    void reset()
    {
        tp.tv_sec = 0;
        tp.tv_nsec = 0;
    }

private:
    void current()
    {
        if (clock_gettime(CLOCK_MONOTONIC, &tp)) {
            throw ServerFramework::Exception("",
                                             __FILE__,
                                             __LINE__,
                                             __FUNCTION__,
                                             "there is something wrong with clock_gettime()");
        }
    }
    void add_msec(int msec)
    {
        tp.tv_sec += msec / 1000;
        tp.tv_nsec += (msec % 1000) * 1000000;

        if (tp.tv_nsec >= 1000000000) {
            ++tp.tv_sec;
            tp.tv_nsec -= 1000000000;
        }
    }

    struct timespec tp;
};


/* ========================================================================== */

typedef int depeche_id_t;
enum depeche_status_t { OK = 0, EXPIRED, FAIL };


struct BagmessageSettings {
    BagmessageSettings()
    {
    }
    BagmessageSettings(const std::string &ip,
                       int port,
                       const std::string &appid_,
                       const std::string &passwd_,
                       bool keepconn,
                       unsigned int heartbeat,
                       unsigned int max_pending_,
                       unsigned int delay_,
                       bool infinity_)
        : dest(ip, port),
          appid(appid_),
          passwd(passwd_),
          keep_connection(keepconn),
          heartbeat_interval(heartbeat),
          max_pending(max_pending_),
          delay(delay_),
          infinity(infinity_)
    {
        check_appid();
    }
    BagmessageSettings(const asyncnet::Dest &d,
                       const std::string &appid_,
                       const std::string &passwd_,
                       bool keepconn,
                       unsigned int heartbeat,
                       unsigned int max_pending_,
                       unsigned int delay_,
                       bool infinity_)
        : dest(d),
          appid(appid_),
          passwd(passwd_),
          keep_connection(keepconn),
          heartbeat_interval(heartbeat),
          max_pending(max_pending_),
          delay(delay_),
          infinity(infinity_)
    {
        check_appid();
    }
    std::string get_key() const
    {
        return dest.host + dest.port + appid + passwd;
    }
    void check_appid()
    {
        if (appid.size() > 8)
            throw ServerFramework::Exception("",
                                             __FILE__,
                                             __LINE__,
                                             __FUNCTION__,
                                             "too long appid. According to Bagmessage docs it should be not longer than 8 bytes");
    }
    /*
     * these ones are major params!
     * changing them requires reconnection
     */
    asyncnet::Dest  dest;
    std::string     appid;
    std::string     passwd;

    /*
     * the following params are considered as minor params! that means changing them
     * doesn't require reconnection
     */
    bool            keep_connection;
    unsigned int     heartbeat_interval; /* milliseconds */

    /*
     * The maximum amount of depeches that may be sent but remain without any asnwer.
     * if 0 then no limit.
     */
    unsigned int    max_pending;
    unsigned int    delay; /* delay between sending depeches in milliseconds. if 0 then no delay */
    bool   infinity; /* try to connect infinity */
};

struct BagmessageDepeche {
    BagmessageDepeche()
    {
    }
    BagmessageDepeche(const std::string &tlg_,
                      const BagmessageSettings &s,
                      depeche_id_t id_,
                      const int timeout) /* milliseconds from construction moment */
        : tlg(tlg_),
          settings(s),
          id(id_),
          clk(timeout)
    {
    }
    std::string         tlg;
    BagmessageSettings  settings;
    depeche_id_t        id; /* external id */
    Clock               clk; /* it is a moment when it might be considered as expired */
};




class Bagmessage : public asyncnet::AsyncTcpSock {
public:
    Bagmessage(boost::asio::io_service &io,
               const BagmessageSettings &,
               boost::function<void(depeche_id_t, depeche_status_t)>,
               const std::string &desc_,
               std::size_t sock_rxsize,
               std::size_t sock_txsize);
//    Bagmessage(boost::asio::io_service &io,
//               const BagmessageSettings &,
//               boost::function<void(depeche_id_t, depeche_status_t)>,
//               const std::string &desc_);
    Bagmessage(boost::asio::io_service &io,
               boost::function<void(depeche_id_t, depeche_status_t)>,
               const std::string &desc_,
               std::size_t sock_rxsize,
               std::size_t sock_txsize);
//    Bagmessage(boost::asio::io_service &io,
//               boost::function<void(depeche_id_t, depeche_status_t)>,
//               const std::string &desc_);

    /*
     * man who uses send_depeche() is in charge of the order of depeches!
     */
    void send_depeche(const std::string         &tlg,
                      const BagmessageSettings  &set,
                      depeche_id_t              id,
                      int                       timeout); /* in milliseconds */
    void set_params(const BagmessageSettings &);
    void set_minor_params(const BagmessageSettings &);
    void set_major_params(const BagmessageSettings &);
    void clear_all();
private:
/*
 *      struct __attribute__ ((__packed__)) header {
 *      char              appid[8]; any unused bytes should be zero-filled
 *      unsigned short    version;
 *      unsigned short    type;
 *      unsigned short    num;   --- this is presented as mess_id in the code
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
    enum parsing_state_t {
        PARSER_HEADER,
        PARSER_BODY,
        PARSER_ERR
    };
    parsing_state_t                                         state;
    bool                                                    logged_on;
    boost::asio::strand                                     strand;
    boost::asio::deadline_timer                             timerdog;
    BagmessageSettings                                      settings;
    std::bitset<65536>                                      bitset; /*  */
    unsigned int                                            max_pending;
    boost::function<void(depeche_id_t, depeche_status_t)>   usrcallback;
    const std::string                                       desc; /* description */

    struct clock2id {
        Clock           clk;
        depeche_id_t    id;
    };
    struct clock2messid {
        Clock           clk;
        int             mess_id;
    };

    std::map<int, depeche_id_t>                      pending_depeches; /* sent depeches that wait for reply to come.  bagmessage mess_id -> astra internal id */
    Clock                                            next_send; /* next sending permitted time. used if delay between sending is set */
    std::map<std::string, std::set<depeche_id_t> >   depeche_grps; /* depeches grouped by settings */
    std::map<depeche_id_t, BagmessageDepeche>        saved_depeches /* all the saved depeches */;
    std::list<clock2id>                              clocks;
    std::list<clock2messid>                          clocks_pending;


    void init();
    void do_clear_all();

    void check_state();

    void do_send_depeche(BagmessageDepeche);
    void save_depeche(const BagmessageDepeche &);
    void group_depeche(const BagmessageDepeche &);
    void remove_depeche_from_group(const BagmessageDepeche &);
    void insert_to_clocks(const BagmessageDepeche &);
    void insert_to_clocks_pending(const BagmessageDepeche &, int);
    void real_send(const BagmessageDepeche &);
    void change_minor_params(const BagmessageSettings &);
    void change_major_params(const BagmessageSettings &);
    void bitset_init();
    int bitset_get_fz(); /* gives the first zero bit */
    bool bitset_have_free_bit();
    void bitset_return_bit(int i); /* frees bit */
    bool is_logged_on();
    void set_log_on();
    void set_log_off();
    void login();
    void do_login();
    void logoff();
    /* ------------incoming messages handlers----------- */
    void handle_logoff();
    void do_handle_logoff();
    void handle_login(msg_type_t);
    void do_handle_login(msg_type_t);
    void handle_message(msg_type_t, int);
    void do_handle_message(msg_type_t, int);
    /* ------------incoming messages handlers----------- end */
    void header_build(asyncnet::Netbuf &buf, enum msg_type_t, int mes_number, int data_length);
    void message_build(asyncnet::Netbuf &buf, enum msg_type_t, int mes_number, const std::string &data);
    void message_build(asyncnet::Netbuf &buf, enum msg_type_t, int mes_number);
    void do_set_params(const BagmessageSettings);
    void do_set_minor_params(const BagmessageSettings);
    void do_set_major_params(const BagmessageSettings);
    virtual void usr_connect_handler();
    virtual std::size_t usr_read_handler(const char *, std::size_t);
    virtual void usr_heartbeat_handler();
    virtual void usr_conn_broken_handler();
    virtual void usr_connect_failed_handler();
    void do_usr_conn_broken_handler();
    void do_usr_connect_failed_handler();
    void dog();
    void dog_wakeup();
    void check_expired_depeches();
    void check_expired_pending();
    bool depeche_exists(depeche_id_t);
    bool have_depeches();
    BagmessageDepeche &take_saved_depeche(depeche_id_t);
    void forget_depeche(depeche_id_t, depeche_status_t);
    void remove_saved_depeche(depeche_id_t);
    void set_next_send_permitted_time();
    bool is_send_permitted();
    void save_pending(int, depeche_id_t);
    depeche_id_t find_pending_id(int);
    void remove_pending(int);
    bool pending_exists(int);
    void clear_all_pending();
    bool have_pending();
    unsigned int pending_count();
    parsing_state_t header_parser(const char *data);
    std::size_t expected_size();
};





} //namespace depeches





#endif
