#ifndef TLGSPLIT_H
#define TLGSPLIT_H

/**
  * @file
  * @brief split and join telegrams
  */
#include <list>
#include <string>
#include "filter.h"
#include "hth.h"

struct tlgnum_t;
struct INCOMING_INFO;
namespace telegrams
{
struct RouterInfo;
class SimpleAirimpJoiner
{
public:
    typedef std::list<std::string> parts_t;
    SimpleAirimpJoiner(const std::string& tlgType,
            const std::list<std::string>& repetitiveHeaderStrings = std::list<std::string>());
    bool combineParts(std::string& fullText, const parts_t& parts);
    int isEnd(const std::string& txt);
private:
    bool isRepetetiveElement(const std::string& txt);
    void preparePart(std::string& txt, bool firstPart);

    std::string tlgType_;
    const std::list<std::string> repetitiveHeaderStrings_;
};
/**
 * @brief checks whether all parts are available, joins them and saves new big telegram
 * @return telegram number (new number if parts were joined), or error code (<=0)
 * */
int joinTlgParts(const char* tlgText, const RouterInfo& ri, const INCOMING_INFO& ii,
        tlgnum_t& localMsgId, const tlgnum_t& remoteMsgId);

typedef std::list<tlgnum_t> tlgnums_t;
typedef tlgnums_t::const_iterator tlgnums_cit_t;
/**
 * @brief checks telegram length and splits it if needed
 * @return 0 on success, < 0 on error, numbers of parts (nums)
 * */
int split_tlg(const tlgnum_t& msgId, tlg_text_filter filter,  const hth::HthInfo* const h2h,
              int router_num, bool edi_tlg, const std::string& tlgText, tlgnums_t& nums);

/**
 * минимальный размер части */
extern size_t MIN_PART_LEN;
}

#endif /* TLGSPLIT_H */

