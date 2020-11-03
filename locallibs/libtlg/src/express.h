#ifndef LIBTLG_EXPRESS_H
#define LIBTLG_EXPRESS_H

#include <string>
#include "tlgnum.h"
#include "hth.h"
#include <boost/optional.hpp>

namespace telegrams
{
namespace express
{

struct Header
{
    size_t size;
    int ttl;
    int routerNum;
    tlgnum_t tlgNum;

    Header(const tlgnum_t& tlgNum_)
        : tlgNum(tlgNum_)
    {}
};

std::string makeReceiveHeader(int routerNum, int ttl, const tlgnum_t&);
boost::optional<Header> parseReceiveHeader(const char* msg);
std::string makeSendHeader(int routerNum, int ttl, const tlgnum_t& tlgNum);
boost::optional<Header> parseSendHeader(const char* msg);

struct ExpressMessage
{
    int routerNum;
    int ttl;
    tlgnum_t tlgNum;
    boost::optional<hth::HthInfo> hthInfo;
    std::string tlgText;

    ExpressMessage(const tlgnum_t& tlgNum_)
            : tlgNum(tlgNum_)
    {}
};

boost::optional<ExpressMessage> parseExpressMessage(const char* buff, size_t buffSz);

} // express
} // telegrams

#endif /* LIBTLG_EXPRESS_H */

