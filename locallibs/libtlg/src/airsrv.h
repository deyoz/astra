#ifndef AIRSRV_H
#define AIRSRV_H

#include <map>
#include <vector>
#include <deque>
#include <boost/asio/io_service.hpp>

#include "telegrams.h"
#include "gateway.h"
#include "consts.h"

struct tlgnum_t;
struct INCOMING_INFO;

namespace telegrams
{
struct AIRSRV_MSG;

class Airsrv
{
public:
    typedef std::map<Direction, std::string> SendersSockets;
    typedef std::function<int (AIRSRV_MSG& , const tlgnum_t& , const RouterInfo& )> HandlerType;

    Airsrv(int supervisorSocket, const char* portName, const SendersSockets& senders);
    virtual ~Airsrv();
    virtual void processMessage(AIRSRV_MSG& msg, const RouterInfo& ri);
    int run();
    virtual void commit();
    virtual void rollback();

protected:
    virtual int getQueueNumber(const char *tlgBody, bool ediTlg, int rtrIndex);
    virtual void afterReceiveTlg(const tlgnum_t& msgId, const char* tlgText);
    virtual bool isExpressTlg(const tlgnum_t& num, const char *tlgBody) { return false; }
    void setHandler(TLG_TYPE type, const HandlerType& handler); // add or replace, if exists

protected:
    bool wasAckForTlg(const tlgnum_t& tlgNum) const;
    void ackForTlg(const tlgnum_t& tlgNum);
    void kickSender(const tlgnum_t& tlgNum);
    int processExpiredTlg(const INCOMING_INFO& ii, const char* tlgBody);
    virtual int receiveExpressTlg(const RouterInfo&, const hth::HthInfo*, int/*ttl*/, char* /*body*/) { return 0; }

protected:
    int tlg_in(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri);
    int tlg_ack(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri);
    int tlg_f_ack(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri);
    int tlg_f_neg(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri);
    int tlg_ack_ack(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri);
    int tlg_err_cfg(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri);
    int tlg_err_tlg(AIRSRV_MSG& tlg, const tlgnum_t& tlgNum, const RouterInfo& ri);

    int processTlg(AIRSRV_MSG& tlg, const RouterInfo& ri);

private:
    typedef std::deque<tlgnum_t> AckedTlgNums;
    typedef std::map<unsigned short int, HandlerType> HandlersMap;
    // список номеров на которые приходили квитанции подтверждения ACK/F_ACK
    AckedTlgNums ackedNums_;
    SendersSockets senders_;
    HandlersMap handlersMap_;
    void* pImpl_;
};


}   //  telegrams

#endif /* AIRSRV_H */

