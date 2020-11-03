#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <functional>
#include <arpa/inet.h>
#include <stdexcept>
#include <assert.h>

#include "blev.h"
#include "msg_const.h"
#include "sirena_queue.h"
#include "fastcgi.h"

#define NICKNAME "MIKHAIL"
#include "slogger.h"
#include "test.h"

namespace ServerFramework {

bool MsgId::in_ahead(std::vector<uint8_t>& head) const
{
    assert( head.size() );
    void* ahead_msgid = head.data() + LEVBDATAOFF;
    return memcmp(ahead_msgid, b, sizeof(b)) == 0;
}

std::ostream& operator<<(std::ostream& os, const MsgId& id)
{
    os << id.b[0] << ' ' << id.b[1] << ' ' << id.b[2];
    return os;
}

//-----------------------------------------------------------------------

BLev::BLev()
{
    m_buf[0] = 0;
    m_buf[1] = getpid();
}

//-----------------------------------------------------------------------

uint16_t BLev::client_id(const uint8_t* header) const
{
    return 0;
}

//-----------------------------------------------------------------------

uint32_t BLev::message_id(const uint8_t* header) const
{
    return 0;
}

//-----------------------------------------------------------------------

/* heaeder: 
 *    1b hdr
 *  100b from inet
 *   42b garbage
 *   12b msgid
 *   40b trash
 *  ----
 *  205b total
 */
void BLev::ok_build_all_the_stuff(std::vector<uint8_t>& reqHead, const timeval& tmstamp) const
{
    ProgTrace(TRACE7,"%s", __FUNCTION__);
    uint8_t* h = reqHead.data();
    memcpy(h + LEVBDATAOFF - 2, h + params_byte(), 2);

    h[0] = type();

    if(h[COMM_PARAMS_BYTE] & MSG_TYPE_PERESPROS)
    {
        uint32_t msgid[3];
        memcpy(msgid, h+LEVBDATAOFF, 12);
        ProgTrace(TRACE0,"%s : { %u %u %u } is peresprosable", __FUNCTION__, msgid[0], msgid[1], msgid[2]);
        throw std::logic_error("BLev::ok_build_all_the_stuff must not be called for peresprosable messages");
    }

    ++m_buf[0];
    m_buf[2] = tmstamp.tv_sec;
    memcpy(h + LEVBDATAOFF, m_buf, 12);
    ProgTrace(TRACE1,"%s : done msgid { %u %u %u }", __FUNCTION__, m_buf[0], m_buf[1], m_buf[2]);
}

void BLev::fill_with_msgid( std::vector<uint8_t>& h, const MsgId& m )
{
    memcpy(h.data() + LEVBDATAOFF, m.b, 12);
}

//-----------------------------------------------------------------------

uint32_t BLev::blen(const uint8_t* const m) const
{
    ProgTrace(TRACE7,"%s", __FUNCTION__);
    uint32_t l = (m[0]<<24) + (m[1]<<16) + (m[2]<<8) + m[3];
    //memcpy(&l, m.data(), 4);
    //l = ntohl(l);
    ProgTrace(TRACE7,"%s:: l=%u", __FUNCTION__, l);
    return l;
}

//-----------------------------------------------------------------------

uint32_t BLev::enqueable(std::vector<uint8_t>& m) const
{
    ProgTrace(TRACE7,"%s", __FUNCTION__);

    if( m.size() < LEVBDATAOFF + 12 + 4 + 1 )
        throw std::runtime_error("BLev::enqueable( too short message )");
    const uint8_t* h = static_cast<const uint8_t*>(m.data());
    const uint8_t* ah = h + LEVBDATAOFF;

    uint32_t msgid[3];
    memcpy(msgid, ah, 12);
    ah += 12;

    uint32_t flags;
    memcpy(&flags, ah, 4);
    ah += 4;

    ProgTrace(TRACE7,"%s:: { %u %u %u } size=%zu flags=0x%x tmout=%u", __FUNCTION__, msgid[0], msgid[1], msgid[2], m.size(), flags, *ah);

    return (flags & MSG_ANSW_STORE_WAIT_SIG) ? *ah : 0;
}

//-----------------------------------------------------------------------

void BLev::make_signal_sendable(
        const std::vector<uint8_t>& signal,
        std::vector<uint8_t>& head,
        std::vector<uint8_t>& data) const
{
    static const uint8_t offset = 4 * sizeof(uint32_t);

    ProgTrace(TRACE7,"%s", __FUNCTION__);

    size_t body_size = signal.size() - offset;
    void* signal_data = const_cast<uint8_t*>(signal.data()) + offset;

    data.resize(body_size);
    memcpy(data.data(), signal_data, body_size);

    uint8_t* const a_head = head.data();
    a_head[COMM_PARAMS_BYTE] = MSG_TYPE_PERESPROS;

    uint8_t* flags = a_head + params_byte();
    uint8_t* flags_backup = a_head + LEVBDATAOFF - 2;
    flags[0] = flags_backup[0] & ( MSG_CAN_COMPRESS | MSG_COMPRESSED | MSG_ENCRYPTED | MSG_PUB_CRYPT );
    flags[1] = flags_backup[0] & ( MSG_MESPRO_CRYPT );

    uint32_t msgid[3];
    memcpy(msgid, flags_backup+2, 12);
    ProgTrace(TRACE7,"%s : done hdr=%u msgid { %u %u %u }", __FUNCTION__, *a_head, msgid[0], msgid[1], msgid[2]);
}

//-----------------------------------------------------------------------

const MsgId& BLev::signal_msgid(const std::vector<uint8_t>& m) const
{
    static MsgId msgid;

    ProgTrace(TRACE7,"%s", __FUNCTION__);
    if(m.size() < 16) {
        throw std::runtime_error("BLev::signal_msgid( too short message )");
    }

    const uint32_t* b = reinterpret_cast<const uint32_t*>(m.data());

    if(*b != htonl(1)) {
        throw std::runtime_error("BLev::signal_msgid( of type != htonl(1) )");
    }

    memcpy(msgid.b, b+1, sizeof msgid.b);
    ProgTrace(TRACE7,"%s : deduced { %u %u %u }", __FUNCTION__, msgid.b[0], msgid.b[1], msgid.b[2]);

    return msgid;
}

//-----------------------------------------------------------------------

size_t BLev::filter(std::vector<uint8_t>& h) const
{
    ProgTrace(TRACE7,"%s", __FUNCTION__);
    if(h.size() < bhead())
    {
        ProgTrace(TRACE7,"%s:: res.size < %u", __FUNCTION__, bhead());

        return 0;
    }

    return bhead() - hlen();
}

//-----------------------------------------------------------------------

void BLev::fill_endpoint(std::vector<uint8_t>& head, const std::string& ep)
{
    if(head.size() < bhead())
        throw std::logic_error("fill_endpoint( head.size < 205 )");

    char* h = reinterpret_cast<char*>(head.data());
    ep.copy(h + 1 + HLENAOUT2, IDLENA + IDLENB - 2);
    if (ep.size() < IDLENA + IDLENB - 2) {
        h[ep.size()] = '\0';
    }
}

//-----------------------------------------------------------------------

std::string BLev::remote_endpoint(std::vector<uint8_t>& h)
{
    if(h.size() < bhead())
        throw std::logic_error("remote_endpoint( m.size < 205 )");

    const char* data = reinterpret_cast<const char*>(h.data());
    std::string ep(data + 1 + HLENAOUT2, IDLENA + IDLENB - 2);
    std::string::size_type z = ep.find('\0');
    if(z != std::string::npos)
        ep.resize(z);

    return ep;
}

//-----------------------------------------------------------------------

uint8_t* BLev::prepare_head( std::vector<uint8_t>& h ) const
{
    h.resize(bhead(), 0);
    //memset(h.data(), 0, h.size());

    ProgTrace(TRACE7,"%s", __FUNCTION__);

    return h.data();
}

//-----------------------------------------------------------------------
uint8_t* BLev::prepare_proxy_head(std::vector<uint8_t>& h) const
{
    h.resize(hlen(), 0);
    return h.data();
}
//-----------------------------------------------------------------------

struct B1Lev : public BLev
{
    uint8_t hlen() const {  return SIRENATECHHEAD;  }
    uint8_t params_byte() const {  return GRP3_PARAMS_BYTE;  }
    uint8_t type() const {  return 1;  };
    void make_expired(std::vector<uint8_t>& head, std::vector<uint8_t>& data) const;
    //void make_excrescent(const std::vector<uint8_t>& head, std::vector<uint8_t>& data) const;
    uint8_t* prepare_head( std::vector<uint8_t>& h ) const
    {
        LogTrace(TRACE7) << __FUNCTION__;

        return BLev::prepare_head(h) + bhead() - hlen();
    }
};

void B1Lev::make_expired(std::vector<uint8_t>& head, std::vector<uint8_t>& data) const
{
    ProgTrace(TRACE7,"%s", __FUNCTION__);
    const char txt[] = "èéÇíéêàíÖ áÄèêéë óÖêÖá 15 ëÖäìçÑ"; // CP866 !!!
    const uint16_t blen = sizeof(txt);

    data.resize(blen);
    memcpy(data.data(), txt, blen);

    uint8_t* r = head.data() + bhead() - hlen();

    uint16_t nblen = /*hlen()*/ + blen;
    r[0] = nblen/256;
    r[1] = nblen%256;
    r[2]=0x18;
    r[3]=0x1;
    r[4]=0x80|0x20;
    r[5]=0x80;
    r[6]=0;
    r[7]=0;
}

//-----------------------------------------------------------------------

struct B2Lev : public BLev
{
    uint8_t hlen() const {  return HLENAOUT2;  } // 100;
    uint8_t params_byte() const {  return GRP2_PARAMS_BYTE;  }
    uint8_t type() const {  return 2;  };
    void make_expired(std::vector<uint8_t>& head, std::vector<uint8_t>& data) const;
    //void make_excrescent(const std::vector<uint8_t>& head, std::vector<uint8_t>& data) const;
    size_t filter(std::vector<uint8_t>& m) const;
    uint8_t* prepare_head(std::vector<uint8_t>& h) const
    {
        return BLev::prepare_head(h) + 1;
    }
    virtual uint16_t client_id(const uint8_t* header) const;
    virtual uint32_t message_id(const uint8_t* header) const;
};

void B2Lev::make_expired(std::vector<uint8_t>& head, std::vector<uint8_t>& data) const
{
    ProgTrace(TRACE7,"%s", __FUNCTION__);

    createQueueFullWSAns(head, data);
}

size_t B2Lev::filter( std::vector<uint8_t>& m ) const
{
    if( size_t offset = BLev::filter(m) )
    {
        memmove(m.data() + offset, m.data() + 1, hlen());

        return offset;
    }

    return 0;
}

uint16_t B2Lev::client_id(const uint8_t* header) const
{
    return ntohs(*reinterpret_cast<const uint16_t*>(header + 44));
}

uint32_t B2Lev::message_id(const uint8_t* header) const
{
    return ntohl(*reinterpret_cast<const uint32_t*>(header + 8));
}

//-----------------------------------------------------------------------

struct B3Lev : public BLev
{
    uint8_t hlen() const {  return HLENAOUT2;  } // 100;
    uint8_t params_byte() const {  return GRP3_PARAMS_BYTE;  }
    uint8_t type() const {  return 3;  };
    void make_expired(std::vector<uint8_t>& head, std::vector<uint8_t>& data) const;
    //void make_excrescent(const std::vector<uint8_t>& head, std::vector<uint8_t>& data) const;
    size_t filter( std::vector<uint8_t>& m ) const;
    uint8_t* prepare_head( std::vector<uint8_t>& h ) const
    {
        return BLev::prepare_head(h) + 1;
    }
};

void B3Lev::make_expired(std::vector<uint8_t>& head, std::vector<uint8_t>& data) const
{
    ProgTrace(TRACE7,"%s", __FUNCTION__);

    createQueueFullWSAns(head, data);
}

size_t B3Lev::filter( std::vector<uint8_t>& m ) const
{
    if( size_t offset = BLev::filter(m) )
    {
        memmove(m.data() + offset, m.data() + 1, hlen());

        return offset;
    }

    return 0;
}

//-----------------------------------------------------------------------

struct B4Lev : public BLev
{
    uint8_t hlen() const {  return FCGI_HEADER_LEN;  }
    //uint32_t blen(const std::vector<uint8_t>& m) const;
    uint8_t params_byte() const {  return GRP3_PARAMS_BYTE;  }
    uint8_t type() const {  return 4;  };
    void make_expired(std::vector<uint8_t>& head, std::vector<uint8_t>& data) const;
    //void make_excrescent(const std::vector<uint8_t>& head, std::vector<uint8_t>& data) const;
    size_t filter( std::vector<uint8_t>& m ) const {
        return bhead();
    }
    uint8_t* prepare_head( std::vector<uint8_t>& h ) const
    {
        return BLev::prepare_head(h) + 1;
    }
};

void B4Lev::make_expired(std::vector<uint8_t>& head, std::vector<uint8_t>& data) const
{
    ProgTrace(TRACE7,"%s", __FUNCTION__);
    // TODO make an fcgi_stderr message
}
/*
uint32_t B4Lev::blen(const std::vector<uint8_t>& m) const
{
    ProgTrace(TRACE7,"%s", __FUNCTION__);
    assert(m.size() == sizeof(FCGI_Header));
    const FCGI_Header* h = reinterpret_cast<const FCGI_Header*>(m.data());
    return (h->contentLengthB1 << 8) + h->contentLengthB0 + h->paddingLength;
}
*/
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

BLev* make_blev(int headtype)
{
    switch(headtype)
    {
        case 1:   return new B1Lev;
        case 2:   return new B2Lev;
        case 3:   return new B3Lev;
        case 4:   return new B4Lev;
        default:  throw std::logic_error("invalid headtype");
    }
}

void timespamp_msg(const char* file, int line, const void* m, const char* msg)
{
    //if(not readIntFromTcl("BLEV_TIMINGS", 1))
    //    return;
    int lev = 0; // 1
    if(m)
    {
        uint32_t msgid[3];
        const uint8_t* h = static_cast<const uint8_t*>(m);
        if(memcmp(h+1,"\xff\xff\xff\xff",4))
            memcpy(msgid, h+LEVBDATAOFF, 12);
        else
            memcpy(msgid, h+101, 12);
        ProgTrace(lev, NICKNAME, file, line, "timespamp_msg { %u %u %u } : %s", msgid[0], msgid[1], msgid[2], msg);
    }
    else
    {
        ProgTrace(lev, NICKNAME, file, line, "timespamp_msg %s", msg);
    }
}

} // namespace ServerFramework
