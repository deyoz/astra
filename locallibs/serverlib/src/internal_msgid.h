#ifndef __INTERNAL_MSGID_
#define __INTERNAL_MSGID_

#include <array>
#include <string>
#include <iosfwd>

namespace ServerFramework
{

class InternalMsgId
{
    std::array<uint32_t,3> MsgId;
    mutable std::string AsString;
    mutable std::string OldStyleStting;

public:
    explicit InternalMsgId(const std::array<uint32_t,3>& msgid_);
    explicit InternalMsgId(uint32_t m1, uint32_t m2, uint32_t m3);

    /**
      * @brief normalized long hex string
    */
    const std::string &asString() const;
    static InternalMsgId fromString(const std::string& );

    /**
      * @brief old style separated id string
    */
    const std::string &sepString() const;

    const std::array<uint32_t,3>& id() const;
    const uint32_t& id(size_t i) const {  return MsgId[i];  }
    uint32_t& id(size_t i) {  return MsgId[i];  }

    bool operator==(const InternalMsgId& x) const;
    bool operator!=(const InternalMsgId& x) const {  return not this->operator==(x);  }
};

std::ostream& operator<<(std::ostream&, const InternalMsgId&);

}

#endif // __INTERNAL_MSGID_
