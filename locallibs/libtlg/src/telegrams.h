#ifndef TELEGRAMS_H
#define TELEGRAMS_H

#include <list>
#include <string>
#include <boost/optional.hpp>

#include "air_q_callbacks.h"
#include "filter.h"
#include "consts.h"
#include  <serverlib/expected.h>

/**
 * @file
 * @brief Telegrams class
 * */

struct tlgnum_t;
struct OUT_INFO;
struct INCOMING_INFO;
struct TlgInfo;
namespace hth { struct HthInfo; }
namespace telegrams
{
struct AIRSRV_MSG;

/**
 * @class RouterInfo
 * @brief описание роутера
 * */
struct RouterInfo
{
    RouterInfo();
    int ida;
    bool hth;
    bool tpb;
    bool true_tpb;
    bool translit;
    bool censor;
    bool blockAnswers;
    bool blockRequests;
    size_t max_part_size;
    size_t max_hth_part_size;
    size_t max_typeb_part_size;
    std::string canonName;
    std::string senderName;
    std::string address;
    size_t port;

    /**
     * максимальный размер передаваемой по UDP телеграммы */
    static size_t defaultMaxPartSize;
    /**
     * максимальный размер части H2H телеграмы */
    static size_t defaultMaxHthPartSize;
    /**
     * максимальный размер TypeB телеграмы */
    static size_t defaultMaxTypeBPartSize;

    bool operator<(const RouterInfo& other) {
        return this->ida < other.ida;
    }
};

bool operator<(const RouterInfo& lhs, const int rhs);

bool operator<(const int lhs, const RouterInfo& rhs);

std::ostream& operator<< (std::ostream& os, const RouterInfo& ri);

enum BadTlgErrors {
    ROT_ERR = -999,
    POINTS_ERR,
    WRITE_HTH_LONG_ERR,
    WRITE_HTH_PART_ERR,
    WRITE_LONG_ERR,
    WRITE_PART_ERR,
    TYPE_ERR,
    BAD_EDIFACT
};

class ErrorQueue
{
public:
    virtual ~ErrorQueue() {}
    virtual void addTlg(const tlgnum_t& tlgNum, const std::string& category, int errQueue) = 0;
    virtual void addTlg(const tlgnum_t& tlgNum, const std::string& category, int errQueue, const std::string& errText) = 0;
    virtual void delTlg(const tlgnum_t& tlgNum) = 0;
};

class HandlerQueue
{
public:
    virtual ~HandlerQueue() {}
    virtual void addTlg(const tlgnum_t& tlgNum, int handler, int ttl) = 0;
    virtual void setHandler(const tlgnum_t& tlgNum, int handler) = 0;
    virtual void delTlg(const tlgnum_t& tlgNum, int handler) = 0;
};

class GatewayQueue
{
public:
    virtual ~GatewayQueue() {}
    virtual void addTlg(const tlgnum_t& tlgNum, Direction queueType, int router, int ttl) = 0;
    virtual void ackTlg(const tlgnum_t& tlgNum) = 0;
    virtual void resendTlg(const tlgnum_t& tlgNum) = 0;
    virtual void delTlg(const tlgnum_t& tlgNum) = 0;
    virtual int getQueueType(const tlgnum_t &tlgNum) = 0;
};

struct TlgResult
{
    tlgnum_t tlgNum;

    TlgResult(const tlgnum_t& tlgNum_)
        : tlgNum(tlgNum_)
    {}
};

/**
 * @brief abstract class with no DB specific
*/
class TlgCallbacksAbstractDb
{
public:
        virtual ~TlgCallbacksAbstractDb()
    {}
    /**
     * Регистрирует hook нужному обработчику тлг
     * */
    virtual void registerHandlerHook(size_t handler) = 0;

    virtual ErrorQueue& errorQueue() = 0;
    virtual HandlerQueue& handlerQueue() = 0;
    virtual GatewayQueue& gatewayQueue() = 0;

    virtual bool saveBadTlg(const AIRSRV_MSG& tlg, int error) = 0;
    virtual Expected<TlgResult, int> writeTlg(INCOMING_INFO* ii, const char* body, bool kickAirimp = true);
    virtual Expected<TlgResult, int> putTlg(const std::string& tlgText,
            tlg_text_filter filter = tlg_text_filter(),
            int from_addr = 0, int to_addr = 0, hth::HthInfo *hth = 0) = 0;

    virtual Expected<TlgResult, int> putTlg2OutQueue(OUT_INFO *oi, const char *body, std::list<tlgnum_t>* tlgParts = 0) = 0;
    virtual int getTlg(const tlgnum_t& tlgNum, TlgInfo& info, std::string& tlgText);
    virtual int readTlg(const tlgnum_t& msg_id, std::string& tlgText);
    virtual int readTlg(const tlgnum_t& msg_id, boost::posix_time::ptime& saveTime, int& router, std::string& tlgText) = 0;

    virtual Expected<tlgnum_t, int> sendExpressTlg(const char* expressSenderPortName, const OUT_INFO& oi, const char* body);

    virtual void readAllRouters(std::list<RouterInfo>& routers) = 0;
    virtual int getRouterInfo(int router, RouterInfo &ri) = 0;
    /**
     * Функция заполняет следующий номер телеграмы.
     * */
    virtual boost::optional<tlgnum_t> nextNum() = 0;
    virtual tlgnum_t nextExpressNum() = 0;
    /**
     * Вызывается у телеграмы истек TTL
     * @param body тело телеграммы
     * @param from_our должен быть true в случае отправки тлг, false в случае приемки устаревшей телеграммы
     * @param router номер роутера
     * @param hthInfo HTH-заголовок
     * */
    virtual bool ttlExpired(const std::string& tlgText, bool from_our, int router, boost::optional<hth::HthInfo> hthInfo) = 0;

    virtual bool tlgIsHth(const tlgnum_t& msgId) = 0;
    virtual int readHthInfo(const tlgnum_t& msgId, hth::HthInfo& hthInfo) = 0;
    virtual int writeHthInfo(const tlgnum_t& msgId, const hth::HthInfo& hthInfo) = 0;
    virtual void deleteHth(const tlgnum_t&) = 0;
    virtual int defaultHandler() const { return m_defaultHandler; }
    virtual void splitError(const tlgnum_t& num) = 0;
    virtual void joinError(const tlgnum_t& num);

    virtual std::list<tlgnum_t> partsReadAll(int rtr, const std::string& uid) = 0;
    virtual int partsCountAll(int rtr, const std::string& uid) = 0;
    virtual void partsDeleteAll(int rtr, const std::string& uid) = 0;
    virtual void partsAdd(int rtr, const tlgnum_t& localMsgId, int partnumber, bool endFlag, const std::string& msgUid) = 0;
    virtual int partsEndNum(int rtr, const std::string& msgUid) = 0;

    virtual void savepoint(const std::string &sp_name) const = 0;
    virtual void rollback(const std::string &sp_name) const = 0;
    virtual void rollback() const = 0;

    TlgCallbacksAbstractDb();
protected:
    int m_defaultHandler;
};

/**
 * @class TlgCallbacks
 * @brief all system specifics (mostly DB work)
 * */
class TlgCallbacks: public TlgCallbacksAbstractDb
{
public:
    virtual ~TlgCallbacks() {}

    virtual bool saveBadTlg(const AIRSRV_MSG& tlg, int error) override;
    virtual Expected<TlgResult, int> putTlg(const std::string& tlgText,
            tlg_text_filter filter = tlg_text_filter(),
            int from_addr = 0, int to_addr = 0, hth::HthInfo *hth = 0) override;

    virtual int readTlg(const tlgnum_t& msg_id, boost::posix_time::ptime& saveTime, int& router, std::string& tlgText) override = 0;
    virtual int readTlg(const tlgnum_t& msg_id, std::string& tlgText) override;

    /**
     * Функция заполняет следующий номер телеграмы.
     * */
    virtual boost::optional<tlgnum_t> nextNum() override;
    virtual tlgnum_t nextExpressNum() override;
    /**
     * Вызывается у телеграмы истек TTL
     * @param body тело телеграммы
     * @param from_our должен быть true в случае отправки тлг, false в случае приемки устаревшей телеграммы
     * @param router номер роутера
     * @param hthInfo HTH-заголовок
     * */
    virtual bool tlgIsHth(const tlgnum_t& msgId) override;
    virtual int readHthInfo(const tlgnum_t& msgId, hth::HthInfo& hthInfo) override;
    virtual int writeHthInfo(const tlgnum_t& msgId, const hth::HthInfo& hthInfo) override;
    virtual void deleteHth(const tlgnum_t&) override;
    virtual int defaultHandler() const override {   return m_defaultHandler; }
    virtual void splitError(const tlgnum_t& num) override;

    virtual std::list<tlgnum_t> partsReadAll(int rtr, const std::string& uid) override;
    virtual int partsCountAll(int rtr, const std::string& uid) override;
    virtual void partsDeleteAll(int rtr, const std::string& uid) override;
    virtual void partsAdd(int rtr, const tlgnum_t& localMsgId, int partnumber, bool endFlag, const std::string& msgUid) override;
    virtual int partsEndNum(int rtr, const std::string& msgUid) override;

    virtual void savepoint(const std::string &sp_name) const override;
    virtual void rollback(const std::string &sp_name) const override;
    virtual void rollback() const override;
};

class Telegrams
{
public:
    static Telegrams* Instance();

    TlgCallbacksAbstractDb* callbacks();
    void setCallbacks(TlgCallbacksAbstractDb *tc);

    AirQManager* airQManager();
    void setAirQCallbacks(AirQCallbacks *cb);

    AirQPartManager* airQPartManager();
    void setAirQPartCallbacks(AirQPartCallbacks *cb);
private:
    Telegrams();
    TlgCallbacksAbstractDb *m_tc;
    AirQManager *m_airq_mgr;
    AirQPartManager *m_airqpart_mgr;
};

/**
 * Для замены неудобной записи Telegrams::Instance()->callbacks()
 * @return указатель на класс с callback-функциями
 * */
inline TlgCallbacksAbstractDb* callbacks()
{
    return Telegrams::Instance()->callbacks();
}

inline AirQManager* airQManager()
{
    return Telegrams::Instance()->airQManager();
}

inline AirQPartManager* airQPartManager()
{
    return Telegrams::Instance()->airQPartManager();
}

} // namespace telegrams

#endif /* TELEGRAMS_H */

