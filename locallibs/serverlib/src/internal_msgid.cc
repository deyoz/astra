#include <array>
#if HAVE_CONFIG_H
#endif

#include <cstdio>
#include <iostream>

#include "internal_msgid.h"

namespace ServerFramework
{

InternalMsgId::InternalMsgId(const std::array<uint32_t,3>& msgid) : MsgId(msgid) {}

InternalMsgId::InternalMsgId(uint32_t m1, uint32_t m2, uint32_t m3) : MsgId{m1,m2,m3} {}

std::string InternalMsgId::asString() const
{
    char buf[25];
    sprintf(buf, "%8.08x%8.08x%8.08x", MsgId[0], MsgId[1], MsgId[2]);
    return std::string(buf, 24);
}

InternalMsgId InternalMsgId::fromString(const std::string& s)
{
    std::array<uint32_t,3> id = {};
    if(3 != sscanf(s.c_str(), "%8x%8x%8x", &id[0], &id[1], &id[2]))
        std::cerr << "InternalMsgId::fromString(" << s << ") failed" << std::endl; // throw smth
    return InternalMsgId(id);
}

std::string InternalMsgId::sepString() const
{
    char str[100];
    sprintf(str, "%d %d %d", MsgId[0], MsgId[1], MsgId[2]);
    return str;
}

const std::array<uint32_t,3>& InternalMsgId::id() const
{
    return MsgId;
}

std::ostream& operator<<(std::ostream& o, const InternalMsgId& m)
{
    return o << m.asString() << " (" << m.sepString() << ")";
}

bool InternalMsgId::operator==(const InternalMsgId& x) const
{
    return MsgId == x.MsgId;
}

}

