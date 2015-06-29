#include "Depeches.hpp"
#include "AsyncNet.hpp"

#include <boost/bind.hpp>
#include "stl_utils.h"

#include "serverlib/exception.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"



namespace depeches {


Bagmessage::Bagmessage(boost::asio::io_service &io,
                       const BagmessageSettings &set,
                       boost::function<void(depeche_id_t, depeche_status_t)> callback,
                       const std::string &desc_,
                       std::size_t sock_rxsize,
                       std::size_t sock_txsize)
    : AsyncTcpSock(io,
                   "Bagmessage's AsyncTcpSock " + desc_,
                   set.dest,
                   set.heartbeat_interval,
                   sock_rxsize,
                   sock_txsize),
      strand(io),
      timerdog(io),
      settings(set),
      usrcallback(callback),
      desc(desc_)
{
    init();

    if (settings.keep_connection)
        start_connecting();
}

//Bagmessage::Bagmessage(boost::asio::io_service &io,
//                       const BagmessageSettings &set,
//                       boost::function<void(depeche_id_t, depeche_status_t)> callback,
//                       const std::string &desc_)
//    : AsyncTcpSock(io,
//                   "Bagmessage's AsyncTcpSock " + desc_,
//                   set.dest,
//                   set.heartbeat_interval),
//      strand(io),
//      timerdog(io),
//      settings(set),
//      usrcallback(callback),
//      desc(desc_)
//{
//    init();

//    if (settings.keep_connection)
//        start_connecting();
//}

Bagmessage::Bagmessage(boost::asio::io_service &io,
           boost::function<void(depeche_id_t, depeche_status_t)> callback,
           const std::string &desc_,
           std::size_t sock_rxsize,
           std::size_t sock_txsize)
    : AsyncTcpSock(io,
                   "Bagmessage's AsyncTcpSock " + desc_,
                   sock_rxsize,
                   sock_txsize),
      strand(io),
      timerdog(io),
      usrcallback(callback),
      desc(desc_)
{
    init();
}

//Bagmessage::Bagmessage(boost::asio::io_service &io,
//           boost::function<void(depeche_id_t, depeche_status_t)> callback,
//           const std::string &desc_)
//    : AsyncTcpSock(io,
//                   "Bagmessage's AsyncTcpSock " + desc_),
//      strand(io),
//      timerdog(io),
//      usrcallback(callback),
//      desc(desc_)
//{
//    init();
//}


void Bagmessage::init()
{
    set_log_off();
}



void Bagmessage::send_depeche(const std::string         &tlg,
                              const BagmessageSettings  &set,
                              depeche_id_t              id,
                              int                       timeout)
{
    BagmessageDepeche dep(tlg, set, id, timeout);
    strand.post(boost::bind(&Bagmessage::do_send_depeche, this, dep));
}

void Bagmessage::check_state()
{

}


void Bagmessage::do_send_depeche(BagmessageDepeche dep)
{
    if (!have_depeches())
        dog_wakeup();

    save_depeche(dep);
    insert_to_clocks(dep);


//    if (settings.max_pending != 0) {
//        if (pending_depeches.size() >= settings.max_pending) {
            /*
             * too many of the depeches remain without reply.
             */
//            return;
//        }
//    }

//    const std::string curkey = settings.get_key();
//    if (curkey != dep.settings.get_key()) {
//        if (pending_depeches.size() != 0) {
            /*
             * if we are here that means currently we're working
             * with depeches with other settings and can't handle
             * them right now.
             * We just saved them, so we will handle them later.
             */
//            return;
//        }
//    }

//    real_send(dep);
}

void Bagmessage::change_minor_params(const BagmessageSettings &depset)
{
    if (settings.delay != depset.delay)
        settings.delay = depset.delay;

    if (settings.heartbeat_interval != depset.heartbeat_interval) {
        set_heartbeat(depset.heartbeat_interval);
        settings.heartbeat_interval = depset.heartbeat_interval;
    }

    if (settings.keep_connection != depset.keep_connection)
        settings.keep_connection = depset.keep_connection;

    if (settings.max_pending != depset.max_pending)
        settings.max_pending = depset.max_pending;

    if (settings.infinity != depset.infinity)
        settings.infinity = depset.infinity;
}

void Bagmessage::change_major_params(const BagmessageSettings &depset)
{
    /*
     * this function leads to reconnection
     */

    bool changed = false;

    if (depset.dest != settings.dest) {
        changed = true;
        settings.dest = depset.dest;
    }
    if (depset.appid != settings.appid) {
        changed = true;
        settings.appid = depset.appid;
    }
    if (depset.passwd != settings.passwd) {
        changed = true;
        settings.passwd = depset.passwd;
    }


    if (changed) {
        if (have_pending()) {
            throw ServerFramework::Exception("",
                                             __FILE__,
                                             __LINE__,
                                             __FUNCTION__,
                                             desc + " Bagmessage::change_major_params() changing connection's params whereas there are pending depeches!");
        }

        set_dest(settings.dest);
        logoff();
        stop_working();
        start_connecting();
    }
}

void Bagmessage::set_params(const BagmessageSettings &set)
{
    strand.post(boost::bind(&Bagmessage::do_set_params, this, set));
}

void Bagmessage::do_set_params(const BagmessageSettings set)
{
    change_major_params(set);
    change_minor_params(set);
}

void Bagmessage::set_minor_params(const BagmessageSettings &set)
{
    strand.post(boost::bind(&Bagmessage::do_set_minor_params, this, set));
}

void Bagmessage::do_set_minor_params(const BagmessageSettings set)
{
    change_minor_params(set);
}

void Bagmessage::set_major_params(const BagmessageSettings &set)
{
    strand.post(boost::bind(&Bagmessage::do_set_major_params, this, set));
}

void Bagmessage::do_set_major_params(const BagmessageSettings set)
{
    change_major_params(set);
}


void Bagmessage::real_send(const BagmessageDepeche &dep)
{
    change_major_params(dep.settings);
    change_minor_params(dep.settings);

    int mess_id = bitset_get_fz();
    asyncnet::Netbuf buf(asyncnet::Netbuf::LITTLEENDIAN);
    message_build(buf, ACK_DATA, mess_id, dep.tlg);
    send(buf.data(), buf.size());
    save_pending(mess_id, dep.id);
    set_next_send_permitted_time();
}

void Bagmessage::set_next_send_permitted_time()
{
    if (settings.delay != 0) {
        Clock new_time_send(settings.delay);
        next_send = new_time_send;
    }
}

bool Bagmessage::is_send_permitted()
{
    if (settings.delay != 0 && !next_send.is_expired())
        return false;

    if (settings.max_pending != 0 && pending_count() >= settings.max_pending)
        return false;

    if (!bitset_have_free_bit())
        return false;

    return true;
}

void Bagmessage::save_depeche(const BagmessageDepeche &dep)
{
    if (depeche_exists(dep.id)) {
        throw ServerFramework::Exception("",
                                         __FILE__,
                                         __LINE__,
                                         __FUNCTION__,
                                         desc + " Bagmessage::save_depeche() I have already gotten the depeche with the same id");
    }

    group_depeche(dep);
    saved_depeches[dep.id] = dep;
}

void Bagmessage::group_depeche(const BagmessageDepeche &dep)
{
    const std::string key = dep.settings.get_key();
    std::set<depeche_id_t> &ids = depeche_grps[key];

    ids.insert(dep.id);
}

void Bagmessage::remove_depeche_from_group(const BagmessageDepeche &dep)
{
    const std::string key = dep.settings.get_key();
    std::set<depeche_id_t> &ids = depeche_grps[key];

    ids.erase(dep.id);
}

void Bagmessage::insert_to_clocks(const BagmessageDepeche &dep)
{
    /*
     * inserting clock in ascending order.
     */
    clock2id tmp;
    tmp.clk = dep.clk;
    tmp.id = dep.id;

    if (clocks.empty()) {
        clocks.push_front(tmp);
        return;
    }

    if (tmp.clk > clocks.back().clk) {
        clocks.push_back(tmp);
        return;
    }

    std::list<clock2id>::iterator it = clocks.begin();
    for ( ; it != clocks.end(); ++it) {
        if (tmp.clk < it->clk) {
            clocks.insert(it, tmp);
            break;
        }
    }
}

void Bagmessage::insert_to_clocks_pending(const BagmessageDepeche &dep, int mess_id)
{
    clock2messid tmp;
    tmp.clk = dep.clk;
    tmp.mess_id = mess_id;

    if (clocks_pending.empty()) {
        clocks_pending.push_front(tmp);
        return;
    }

    if (tmp.clk > clocks_pending.back().clk) {
        clocks_pending.push_back(tmp);
        return;
    }

    std::list<clock2messid>::iterator it = clocks_pending.begin();
    for ( ; it != clocks_pending.end(); ++it) {
        if (tmp.clk < it->clk) {
            clocks_pending.insert(it, tmp);
            break;
        }
    }
}

void Bagmessage::usr_connect_handler()
{
    return login();
}

Bagmessage::parsing_state_t Bagmessage::header_parser(const char *header)
{
    /*
     * We are waiting for full header to come. So, it is size known as 20
     */
    asyncnet::Netbuf buf(header, 20, asyncnet::Netbuf::LITTLEENDIAN);


    std::string appid;
    unsigned short version;
    unsigned short mes_type;
    unsigned short mess_id;
    unsigned short len;

    appid = buf.take_string(8);
    if (appid != settings.appid) {
        throw "incoming appid is wrong";
    }

    buf >> version;
    if (version != 2) {
        throw "incoming version is wrong";
    }

    buf >> mes_type;
    buf >> mess_id;
    buf >> len;


    msg_type_t type = (msg_type_t)mes_type;
    switch (type) {
    case LOGIN_ACCEPT:
    case LOGIN_REJECT:
        handle_login(type);
        break;

    case ACK_MSG:
    case NAK_MSG:
        handle_message(type, mess_id);
        break;

    case STATUS:
        LogTrace(TRACE5) << desc << " received heartbeat message";
        break;

    case LOG_OFF:
        handle_logoff();
        break;

    case LOGIN_RQST:
    case DATA:
    case ACK_DATA:
    case DATA_ON:
    case DATA_OFF:
        LogError(STDLOG) << desc << " i haven't unexpected such type of message";
        return PARSER_ERR;
        break;

    default:
        LogError(STDLOG) << desc << " invalid type of message";
        return PARSER_ERR;
    }

    return PARSER_HEADER;
}

void Bagmessage::handle_logoff()
{
    strand.post(boost::bind(&Bagmessage::do_handle_logoff, this));
}

void Bagmessage::do_handle_logoff()
{
    set_log_off();
    LogError(STDLOG) << desc << " received logoff message from the server. closing the socket";
    stop_working();
    if (settings.keep_connection) {
        LogError(STDLOG) << desc << " keep_connection is set to TRUE. So, I will try to reconnect";
        start_connecting();
    }
}

void Bagmessage::handle_message(msg_type_t type, int mess_id)
{
    strand.post(boost::bind(&Bagmessage::do_handle_message, this, type, mess_id));
}

void Bagmessage::handle_login(msg_type_t type)
{
    strand.post(boost::bind(&Bagmessage::do_handle_login, this, type));
}

void Bagmessage::do_handle_message(msg_type_t type, int mess_id)
{
    bitset_return_bit(mess_id);
    depeche_id_t id = find_pending_id(mess_id);

    if (type == ACK_MSG) {
        LogTrace(TRACE5) << desc << "positive reply received to depeche with internal id "
                         << tostring(id);
        forget_depeche(id, OK);
    } else {
        LogError(STDLOG) << desc << " negative reply received to depeche with internal id"
                         << tostring(id);
        forget_depeche(id, FAIL);
    }

    remove_pending(mess_id);
}

void Bagmessage::do_handle_login(msg_type_t type)
{
    if (type == LOGIN_ACCEPT) {
        LogTrace(TRACE5) << desc << " login accepted!";
        start_heartbeat();
        bitset_init();
        set_log_on();
        next_send.reset();
        clear_all_pending();
        if (have_depeches())
            dog_wakeup();
    } else {
        LogError(STDLOG) << desc << " login rejected";
    }
}

bool Bagmessage::have_depeches()
{
    return !saved_depeches.empty();
}

std::size_t Bagmessage::usr_read_handler(const char *data, std::size_t data_len)
{
    std::size_t total_consumed = 0;
    std::size_t n;

    while ( (n = expected_size()) <= (data_len - total_consumed) ) {
        switch (state) {
        case PARSER_HEADER:
            state = header_parser(data + total_consumed);
            break;

        default:
            throw ServerFramework::Exception("",
                                             __FILE__,
                                             __LINE__,
                                             __FUNCTION__,
                                             desc + " Bagmessage::usr_read_handler() parser error has occured");
        }
        total_consumed += n;
    }
    return total_consumed;
}

std::size_t Bagmessage::expected_size()
{
    switch (state) {
    case PARSER_HEADER:
        return 20;
    default:
        return 0;
    }

}

void Bagmessage::usr_conn_broken_handler()
{
    strand.post(boost::bind(&Bagmessage::do_usr_conn_broken_handler, this));
}

void Bagmessage::do_usr_conn_broken_handler()
{
    LogError(STDLOG) << desc << " the connection was broken";
    if (settings.keep_connection) {
        LogError(STDLOG) << desc << " keep_connection is set to TRUE. So, I will try to reconnect";
        start_connecting();
    }
}

void Bagmessage::login()
{
    strand.post(boost::bind(&Bagmessage::do_login, this));
}

void Bagmessage::do_login()
{
    asyncnet::Netbuf buf(asyncnet::Netbuf::LITTLEENDIAN);
    message_build(buf, LOGIN_RQST, 0, settings.passwd);
    send(buf.data(), buf.size());
    state = PARSER_HEADER;
}

void Bagmessage::logoff()
{
    asyncnet::Netbuf buf(asyncnet::Netbuf::LITTLEENDIAN);
    message_build(buf, LOG_OFF, 0);
    send(buf.data(), buf.size());
    set_log_off();
}

bool Bagmessage::is_logged_on()
{
    return logged_on;
}

void Bagmessage::set_log_on()
{
    logged_on = true;
}

void Bagmessage::set_log_off()
{
    logged_on = false;
}

void Bagmessage::usr_heartbeat_handler()
{
    asyncnet::Netbuf buf(asyncnet::Netbuf::LITTLEENDIAN);
    message_build(buf, STATUS, 0);
    send(buf.data(), buf.size());
}

void Bagmessage::header_build(asyncnet::Netbuf &buf, msg_type_t type, int mess_id, int data_length)
{
    static const u_int16_t version = 2;


    buf << settings.appid;
    buf.fillwith(0, 8 - settings.appid.size());

    buf << version
        << (u_int16_t) type
        << (u_int16_t) mess_id
        << (u_int16_t) data_length;

    buf.fillwith(0, 4); /* reserved */
}

void Bagmessage::message_build(asyncnet::Netbuf &buf, msg_type_t type, int mess_id, const std::string &data)
{
    header_build(buf, type, mess_id, data.size());
    buf << data;
}

void Bagmessage::message_build(asyncnet::Netbuf &buf, msg_type_t type, int mess_id)
{
    header_build(buf, type, mess_id, 0);
}

bool Bagmessage::bitset_have_free_bit()
{
    return bitset.count() == bitset.size() ? false : true;
}

int Bagmessage::bitset_get_fz()
{
    if (bitset.count() == bitset.size()) {
        /*
         * We check before call this whether we have free bits in the bitset.
         * So, we never should reach here
         */
        throw ServerFramework::Exception("",
                                         __FILE__,
                                         __LINE__,
                                         __FUNCTION__,
                                         desc + " Bagmessage::bitset_get_fz() strange error. Check the code");
    }

    for (int bit = 0; bit < 65536; ++bit)
        if (!bitset.test(bit)) {
            bitset.set(bit, 1);
            return bit;
        }

    return -1;
}

void Bagmessage::bitset_return_bit(int bit)
{
    bitset.set(bit, 0);
}

void Bagmessage::bitset_init()
{
    bitset.reset();
}

void Bagmessage::dog_wakeup()
{
    if (settings.delay == 0 || settings.delay > 1000)
        timerdog.expires_from_now(boost::posix_time::seconds(1));
    else
        timerdog.expires_from_now(boost::posix_time::milliseconds(settings.delay));

    timerdog.async_wait(strand.wrap(boost::bind(&Bagmessage::dog, this)));
}

void Bagmessage::usr_connect_failed_handler()
{
    strand.post(boost::bind(&Bagmessage::do_usr_connect_failed_handler, this));
}

void Bagmessage::do_usr_connect_failed_handler()
{
    LogError(STDLOG) << desc << " can't connect to the server";
    if (settings.infinity) {
        LogError(STDLOG) << desc << " infinity flag is set. i will try to connect";
        start_connecting();
    }
}

void Bagmessage::check_expired_depeches()
{
    Clock now; /* default constructor sets current time */
    std::list<clock2id>::iterator it = clocks.begin();
    /*
     * In this while loop we are finding expired depeches
     */
    while (it != clocks.end()) {
        if (it->clk < now) {
            forget_depeche(it->id, EXPIRED);
            clocks.erase(it++);
        } else {
            break;
        }
    }
}

void Bagmessage::check_expired_pending()
{
    Clock now; /* default constructor sets current time */
    std::list<clock2messid>::iterator it = clocks_pending.begin();
    /*
     * In this while loop we are finding expired depeches
     */
    while (it != clocks_pending.end()) {
        if (it->clk < now) {
            remove_pending(it->mess_id);
            clocks_pending.erase(it++);
        } else {
            break;
        }
    }
}

void Bagmessage::dog()
{
    check_expired_depeches();
    check_expired_pending();



    if (!have_depeches())
        return;

    if (!is_connected()) {
        LogError(STDLOG) << desc << " Can't send anything. No connection";
        return;
    }

    if (!is_logged_on()) {
        /*
         * it's really god damn strange situation
         */
        LogError(STDLOG) << desc << " The connection seems to be established, but i am not logged on";
        return;
    }

    /*
     * If we have no free bits in the bitset let's check
     * whether there are too long pending depeches.
     */
    if (bitset_have_free_bit() == false) {
        LogError(STDLOG) << desc << " We shouldn't be in this branch! Invent something to avoid this!";
        throw "FUCK!";
    }


    /***************************************/

    const std::string key = settings.get_key();
    std::set<depeche_id_t> &ids = depeche_grps[key];
    std::set<depeche_id_t>::iterator it2 = ids.begin();
    for ( ; it2 != ids.end(); ++it2) {
        if (!is_send_permitted())
            break;
        BagmessageDepeche &dep = take_saved_depeche(*it2);
        real_send(dep);
    }

    if (have_depeches())
        dog_wakeup();
}


bool Bagmessage::depeche_exists(depeche_id_t id)
{
    return saved_depeches.end() == saved_depeches.find(id) ? false : true;
}

BagmessageDepeche &Bagmessage::take_saved_depeche(depeche_id_t id)
{
    if (!depeche_exists(id)) {
        throw ServerFramework::Exception("",
                                         __FILE__,
                                         __LINE__,
                                         __FUNCTION__,
                                         desc + " Bagmessage::take_saved_depeche() taking not-existinig depeche. Check the code!");
    }
    return saved_depeches[id];
}


void Bagmessage::forget_depeche(depeche_id_t id, depeche_status_t status)
{
    /*
     * the function is called from two places:
     * 1) when iterating through the clocks list
     * 2) when we received reply
     */
    if (depeche_exists(id) == false) {
        /*
         * if we are here it means that
         * depeche has been already forgotten because of two cases:
         * 1) timer had already passed
         * 2) we'd received a reply
         */
        return;
    }

//    if (status == EXPIRED) {
//        std::map<int, depeche_id_t>::iterator it = pending_depeches.begin();
//        for ( ; it != pending_depeches.end(); ++it) {

//        }
//    }

    usrcallback(id, status);
    BagmessageDepeche &dep = take_saved_depeche(id);
    remove_depeche_from_group(dep);
    remove_saved_depeche(id);
}

void Bagmessage::remove_saved_depeche(depeche_id_t id)
{
    if (!depeche_exists(id)) {
        throw ServerFramework::Exception("",
                                         __FILE__,
                                         __LINE__,
                                         __FUNCTION__,
                                         desc + " Bagmessage::remove_saved_depeche() removing not-existinig depeche. Check the code!");
    }
    saved_depeches.erase(id);
}


void Bagmessage::save_pending(int mess_id, depeche_id_t id)
{
    if (pending_exists(mess_id)) {
        throw ServerFramework::Exception("",
                                         __FILE__,
                                         __LINE__,
                                         __FUNCTION__,
                                         desc + " Bagmessage::save_pending() we have already gotten pending depeche with this mess id. Check the code");
    }

    pending_depeches[mess_id] = id;
}

depeche_id_t Bagmessage::find_pending_id(int mess_id)
{
    if (!pending_exists(mess_id)) {
        throw ServerFramework::Exception("",
                                         __FILE__,
                                         __LINE__,
                                         __FUNCTION__,
                                         desc + " Bagmessage::find_pending_id() the code designed in way when we always should get some id. Check the code");
    }

    return pending_depeches[mess_id];
}

bool Bagmessage::pending_exists(int mess_id)
{
    return pending_depeches.end() == pending_depeches.find(mess_id) ? false : true;
}

void Bagmessage::remove_pending(int mess_id)
{
//    if (!pending_exists(mess_id)) {
//        throw ServerFramework::Exception("",
//                                         __FILE__,
//                                         __LINE__,
//                                         __FUNCTION__,
//                                         desc + " Bagmessage::remove_pending() removing not existing pending depeche");
//    }

    pending_depeches.erase(mess_id);
}


void Bagmessage::clear_all_pending()
{
    pending_depeches.clear();
}

bool Bagmessage::have_pending()
{
    return !pending_depeches.empty();
}

unsigned int Bagmessage::pending_count()
{
    return pending_depeches.size();
}


} // namespace depeches
