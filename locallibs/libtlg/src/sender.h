#ifndef LIBTLG_SENDER_H
#define LIBTLG_SENDER_H

#include <string>
#include <list>
#include <time.h>
#include <boost/asio/io_service.hpp>
#include "consts.h"
#include "tlgnum.h"

struct TlgInfo;

namespace telegrams
{
struct RouterInfo;

struct RoutersWithTlg {
    int routerNum;
    tlgnum_t tlgNum;

    RoutersWithTlg(const int router, const tlgnum_t& tlg);
};

typedef std::vector<RoutersWithTlg> RoutersWithTlgs;

class Sender
{
public:
    Sender(int supervisorSocket,
           const std::string& handlerName,
           const char* senderSocketName,
           const char* senderPortName,
           Direction queueType);

    virtual ~Sender();

    int run();
    Direction queueType() const;
    virtual bool haveUnknownRouters(const std::list<RouterInfo>& routers) = 0;
    virtual boost::optional<tlgnum_t> getTlg(TlgInfo& info, std::string& tlgText);
    virtual boost::optional<tlgnum_t> getTlg4Sending(TlgInfo& info) = 0;
    virtual size_t fillRoutersToSend(RoutersWithTlgs& routersToSend, int queueType);
    virtual int32_t tlgnumAsInt32(const tlgnum_t& tlgNum);

    virtual void commit();
    virtual void rollback();

private:
    void *pImpl_;
};

} // namespace telegrams

#endif /* LIBTLG_SENDER_H */

