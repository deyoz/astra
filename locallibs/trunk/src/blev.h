#ifndef __BLEV_H_
#define __BLEV_H_

#include <string.h>
#include <vector>
#include <string>
#include <stdint.h>

namespace ServerFramework {

struct MsgId
{
    static const int BUF_SIZE = 3;
    static const int SIZE = BUF_SIZE * sizeof(uint32_t);

    uint32_t b[BUF_SIZE];

    bool in_ahead(std::vector<uint8_t>&) const;

    MsgId() {
        memset(b, 0, sizeof(b));
    }

    bool operator<(const MsgId& rhs) const {
        return (memcmp(b, rhs.b, sizeof(uint32_t) * BUF_SIZE) < 0);
    }
};

std::ostream& operator<<(std::ostream& os, const MsgId& id);

class BLev
{
    mutable uint32_t m_buf[3];
    virtual uint8_t params_byte() const = 0;
    virtual uint8_t type() const = 0;
public:
    virtual size_t filter(std::vector<uint8_t>& h) const; // returns the outgoing message's offset to read from
    virtual void make_expired(std::vector<uint8_t>& head, std::vector<uint8_t>& data) const = 0;
    virtual void make_excrescent(std::vector<uint8_t>& head, std::vector<uint8_t>& data) const {
        make_expired(head, data);
    }
    virtual uint8_t* prepare_head(std::vector<uint8_t>&) const = 0;
    virtual uint8_t* prepare_proxy_head(std::vector<uint8_t>& ) const;
    virtual uint8_t hlen() const = 0;
    virtual uint16_t client_id(const uint8_t* header) const;
    virtual uint32_t message_id(const uint8_t* header) const;
    void ok_build_all_the_stuff(std::vector<uint8_t>& reqHead, const timeval& tmstamp) const;
    void fill_endpoint(std::vector<uint8_t>& , const std::string&);
    std::string remote_endpoint( std::vector<uint8_t>& );
    virtual uint32_t blen(const uint8_t* const m) const;
    uint32_t enqueable( std::vector<uint8_t>& ) const;
    void make_signal_sendable(
            const std::vector<uint8_t>& signal,
            std::vector<uint8_t>& ansHead,
            std::vector<uint8_t>& ansData) const;
    void fill_with_msgid( std::vector<uint8_t>& h, const MsgId& m );
    const MsgId& signal_msgid(const std::vector<uint8_t>& m) const;
    static uint8_t bhead() {  return 205;  }
    virtual ~BLev() {}
    BLev();
};

BLev* make_blev(int headtype);

void timespamp_msg(const char* file, int line, const void* m, const char* msg);
} // namespace ServerFramework

#endif // __BLEV_H_
