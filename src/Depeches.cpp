#include "Depeches.hpp"
#include "AsyncNet.hpp"

#include <boost/bind.hpp>

#include "serverlib/exception.h"

#define NICKNAME "AZVEREV"
#define NICKTRACE VLAD_TRACE
#include "serverlib/slogger.h"


#define printf(...)

namespace depeches {


Bagmessage::Bagmessage(boost::asio::io_service &io,
                       const BagmessageSettings &set,
                       boost::function<void(depeche_id_t, Depeche::depeche_status_t)> callback,
                       const std::string &desc_)
    : AsyncTcpSock(io,
                   "bagmessage socket " + desc_,
                   set.dest,
                   true,
                   set.heartbeat_interval,
                   set.keep_connection),
      strand(io),
      settings(set),
      usrcallback(callback),
      desc(desc_)
{
    bitset_init();
    set_log_off();
    start_connecting();
}


void Bagmessage::send_depeche(const std::string &tlg,
                  const BagmessageSettings &set,
                  depeche_id_t id,
                  int timeout)
{
    Depeche dep(tlg, set, id, timeout);
    strand.post(boost::bind(&Bagmessage::do_send_depeche, this, dep));
}

void Bagmessage::do_send_depeche(Depeche dep)
{
    int mess_id;

    mess_id = bitset_get_fz();
    if (mess_id == -1) {
        /*
         * we didn't find a free slot
         */
        usrcallback(dep.id, Depeche::NO_FREE_SLOT);
        return;
    }

    save_depeche(mess_id, dep);
    real_send(mess_id, dep);
}

void Bagmessage::real_send(int mess_id, const Depeche &dep)
{
    asyncnet::Netbuf buf(asyncnet::Netbuf::LITTLEENDIAN);
    message_build(buf, ACK_DATA, mess_id, dep.tlg);
    if (is_logged_on())
        send(buf.data(), buf.size());
}

void Bagmessage::save_depeche(int mess_id, const Depeche &dep)
{
    deps[mess_id] = dep;
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
    unsigned short type;
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

    buf >> type;
    buf >> mess_id;
    buf >> len;

    if (len == 0) {
        printf("len is 0! everything is fine!\n");
    }

    switch (type) {
    case LOGIN_ACCEPT:
        start_heartbeat();
//        printf("login has been accepted\n");
        bitset_init();
        set_log_on();
        break;

    case LOGIN_REJECT:
        printf("login has been rejected\n");
        break;

    case ACK_MSG:
//        printf("ACK messsage with %d id has been received\n", mes_id);
        usrcallback(deps[mess_id].id, Depeche::OK);
        deps.erase(mess_id);
        bitset_return_bit(mess_id);
        break;

    case NAK_MSG:
        usrcallback(deps[mess_id].id, Depeche::FAIL);
        deps.erase(mess_id);
        bitset_return_bit(mess_id);
        break;

    case STATUS:
//        printf("success %d\tbad %d\n", success, fail);
        printf("heartbeat message has been received\n");
        break;

    default:
        printf("undefined type of message\n");
        return PARSER_ERR;
    }

    return PARSER_HEADER;
}

std::size_t Bagmessage::usr_read_handler(const char *data, std::size_t data_len)
{
    std::size_t total_consumed = 0;
    std::size_t n;

    while ( (n = expected_size()) >= (data_len - total_consumed) ) {

        switch (state) {
        case PARSER_HEADER:
            state = header_parser(data + total_consumed);
            break;

        default:
            printf("PARSER ERROR!!!\n");
            abort();
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

void Bagmessage::login()
{
    asyncnet::Netbuf buf(asyncnet::Netbuf::LITTLEENDIAN);
    message_build(buf, LOGIN_RQST, 0, settings.passwd);
    send(buf.data(), buf.size());
    state = PARSER_HEADER;
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

int Bagmessage::bitset_get_fz()
{
    for (int i = 0; i < 65536; ++i)
        if (!bitset.test(i)) {
            bitset.set(i, 1);
            return i;
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

#undef printf

} // namespace depeches
