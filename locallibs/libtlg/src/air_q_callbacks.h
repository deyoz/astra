#ifndef AIR_Q_CALLBACKS_H
#define AIR_Q_CALLBACKS_H

#include "tlgnum.h"

#include <serverlib/strong_types.h>

#include <string>
#include <list>
#include <vector>
#include <initializer_list>

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>


namespace telegrams
{
    
DEFINE_NATURAL_NUMBER_TYPE(QNum_t, int, "Queue num", "Queue num");
DEFINE_NATURAL_NUMBER_TYPE(SysQ_t, int, "Sys queue", "Sys queue");

//---------------------------------------------------------------------------------------

inline QNum_t getQNum(int qnum)
{
    return static_cast<QNum_t>(qnum);
}

inline SysQ_t getSysQ(int sysq)
{
    return static_cast<SysQ_t>(sysq);
}

//---------------------------------------------------------------------------------------

struct AirQOrder
{
    enum OrderField
    {
        MsgId,
        Rdate,
    };

    std::vector< OrderField > Fields;

    AirQOrder() {}
    AirQOrder(std::initializer_list<OrderField> fields)
        : Fields(fields)
    {}
    
    static std::string fieldPlaceholder(OrderField of);
    
    
    const std::vector<OrderField>& fields() const { return Fields; }
    AirQOrder& addField(OrderField of) { Fields.push_back(of); return *this; }
};

//---------------------------------------------------------------------------------------

struct AirQFilter
{
    boost::optional< tlgnum_t > MsgId;
    boost::optional< tlgnum_t > MsgIdFrom;
    boost::optional< int >      OldId;
    boost::optional< QNum_t >   QNum;
    boost::optional< SysQ_t >   SysQ;
    boost::optional< boost::posix_time::ptime > RDateFrom;
    boost::optional< boost::posix_time::ptime > RDateTo;

    AirQFilter() {}
    AirQFilter(const tlgnum_t& mid) : MsgId(mid) {}

    AirQFilter&     setMsgId(const tlgnum_t& mid) { MsgId.reset(mid); return *this; }
    AirQFilter& setMsgIdFrom(const tlgnum_t& midFrom) { MsgIdFrom.reset(midFrom); return *this; }
    AirQFilter&     setOldId(int oid) { OldId.reset(oid); return *this; }
    AirQFilter&      setQNum(QNum_t qn) { QNum.reset(qn); return *this; }
    AirQFilter&      setQNum(int qn) { QNum.reset(QNum_t(qn)); return *this; }
    AirQFilter&      setSysQ(SysQ_t sq) { SysQ.reset(sq); return *this; }
    AirQFilter&      setSysQ(int sq) { SysQ.reset(SysQ_t(sq)); return *this; }
    AirQFilter& setRDateFrom(const boost::posix_time::ptime& rdf) { RDateFrom.reset(rdf); return *this; }
    AirQFilter&   setRDateTo(const boost::posix_time::ptime& rdt) { RDateTo.reset(rdt); return *this; }
};

//---------------------------------------------------------------------------------------

struct AirQStat
{
    tlgnum_t msg_id;
    int q_num;
    int cnt;

    AirQStat(const tlgnum_t& msgId, int qNum, int cnt_ = 0)
        : msg_id(msgId), q_num(qNum), cnt(cnt_)
    {}
};

//---------------------------------------------------------------------------------------

struct AirQOutStat
{
    boost::optional<AirQStat> tpa = boost::none;   // TYPEA_OUT
    boost::optional<AirQStat> tpb = boost::none;   // OTHER_OUT
    boost::optional<AirQStat> dlv = boost::none;   // WAIT_DELIV
    AirQOutStat();
};

//---------------------------------------------------------------------------------------

struct AirQPartFilter
{
    boost::optional< std::string > MsgUid;
    boost::optional< int > Peer;
    boost::optional< int > EndFlag;
    
    AirQPartFilter& setMsgUid(const std::string& msgUid) { MsgUid.reset(msgUid); return *this; }
    AirQPartFilter& setPeer(int p) { Peer.reset(p); return *this; }
    AirQPartFilter& setEndFlag(int ef) { EndFlag.reset(ef); return *this; }
};

//---------------------------------------------------------------------------------------

struct AirQPartOrder
{
    enum OrderField
    {
        Part,
    };
    
    static std::string fieldPlaceholder(OrderField of);
    
    std::vector<OrderField> Fields;

    AirQPartOrder() {}
    AirQPartOrder(std::initializer_list<OrderField> fields)
        : Fields(fields)
    {}
    
    const std::vector<OrderField> &fields() const { return Fields; }
    AirQPartOrder& addField(OrderField of) { Fields.push_back(of); return *this; }
};

//---------------------------------------------------------------------------------------

enum class MsgProcessStatus : short
{
    None,
    LastAttempt,
    AlreadyProcessed
};

//---------------------------------------------------------------------------------------

struct AirQ
{
    tlgnum_t MsgId;
    QNum_t   QNum;
    SysQ_t   SysQ;
    int      OldId;
    int      Attempts;
};

//---------------------------------------------------------------------------------------

class AirQDbSupport
{
public:
    static std::string selectQuery(const std::string& selExpr,
                                   const AirQFilter& filter,
                                   const AirQOrder& order);

    static std::string updateQuery(const std::string& updExpr,
                                   const AirQFilter& filter,
                                   const AirQOrder& order,
                                   bool autonomous = false);

    static std::string insertQuery(bool autonomous = false);

    static std::string deleteQuery(bool autonomous = false);
    static std::string deleteQuery(const AirQFilter& filter, bool autonomous = false);
    static std::string deleteQuery(const std::list<QNum_t>& qNums, bool autonomous = false);

    static std::string countQuery();
    static std::string countQuery(const AirQFilter& filter);

protected:
    static std::string selectText(const std::string& selExpr);
    static std::string updateText(const std::string& updExpr);
    static std::string whereText(const AirQFilter& filter);
    static std::string whereText(const std::list<QNum_t>& qNums);
    static std::string orderByText(const AirQOrder& order);
};

//---------------------------------------------------------------------------------------

class AirQPartDbSupport
{
public:
    static std::string selectQuery(const std::string& selExpr,
                                   const AirQPartFilter& filter,
                                   const AirQPartOrder& order);

    static std::string insertQuery(bool autonomous = false);

    static std::string deleteQuery(bool autonomous = false);
    static std::string deleteQuery(const AirQPartFilter& filter, bool autonomous = false);

    static std::string countQuery();
    static std::string countQuery(const AirQPartFilter& filter);

protected:
    static std::string selectText(const std::string& selExpr);
    static std::string whereText(const AirQPartFilter& filter);
    static std::string orderByText(const AirQPartOrder& order);
};

//---------------------------------------------------------------------------------------

struct AirQPart
{
    int         Part;
    int         Peer;
    tlgnum_t    MsgIdLocal;
    int         EndFlag;
    std::string MsgUid;
};

//---------------------------------------------------------------------------------------

class AirQCallbacks
{
public:
    virtual boost::optional<AirQ> selectNext(const AirQFilter& f, const AirQOrder& o) const = 0;

    virtual std::list<AirQ> select(const AirQFilter& f, const AirQOrder& o) const = 0;

    virtual std::list<QNum_t> selectQNums(SysQ_t sysQ) const = 0;

    virtual void updateQNum(const tlgnum_t& msgId, QNum_t qNum) const = 0;
    virtual void updateSysQ(const tlgnum_t& msgId, SysQ_t sysQ) const = 0;

    virtual void incAttempts(const tlgnum_t& msgId) const = 0;
    virtual void decAttempts(const tlgnum_t& msgId) const = 0;

    virtual bool insert(const AirQ& rec) const = 0;

    virtual void remove(const AirQFilter& f) const = 0;
    virtual void remove(const std::list<QNum_t>& qNums) const = 0;

    virtual std::list<AirQStat>       getQStats(SysQ_t sysQ) const = 0;
    virtual boost::optional<AirQStat>  getQStat(SysQ_t sysQ, QNum_t qNum) const = 0;

    virtual std::list<AirQOutStat> getQOutStats() const = 0;

    virtual MsgProcessStatus checkProcessStatus(const tlgnum_t& msgId) const = 0;

    virtual void commit() const = 0;
    virtual void rollback() const = 0;

#ifdef XP_TESTING
    virtual void clear() const = 0;
    virtual size_t size() const = 0;
#endif //XP_TESTING

    virtual ~AirQCallbacks() {}
};

//---------------------------------------------------------------------------------------

class AirQPartCallbacks
{
public:
    virtual std::list<AirQPart> select(const AirQPartFilter& f, const AirQPartOrder& o) const = 0;

    virtual bool insert(const AirQPart& rec) const = 0;

    virtual void remove(const AirQPartFilter& f) const = 0;

    virtual unsigned count(const AirQPartFilter& f) const = 0;

    virtual void commit() const = 0;
    virtual void rollback() const = 0;

#ifdef XP_TESTING
    virtual void clear() const = 0;
    virtual size_t size() const = 0;
#endif //XP_TESTING

    virtual ~AirQPartCallbacks() {}
};

//---------------------------------------------------------------------------------------

class AirQManager
{
public:
    static std::string schema();
    static std::string tableWithSchema();

    AirQManager(AirQCallbacks* cb);

    AirQCallbacks* callbacks() const;

    boost::optional<AirQ> selectNext(const AirQFilter& f, const AirQOrder& o) const;
    std::list<AirQ> select(const AirQFilter& f, const AirQOrder& o = {}) const;
    boost::optional<AirQ> find(const AirQFilter& f) const;
    std::list<QNum_t> selectQNums(SysQ_t sysQ) const;
    std::list<QNum_t> selectQNums(int sysQ) const;
    void updateQNum(const tlgnum_t& msgId, QNum_t qNum) const;
    void updateQNum(const tlgnum_t& msgId, int qNum) const;
    void updateSysQ(const tlgnum_t& msgId, SysQ_t sysQ) const;
    void updateSysQ(const tlgnum_t& msgId, int sysQ) const;
    void incAttempts(const tlgnum_t& msgId) const;
    void decAttempts(const tlgnum_t& msgId) const;
    bool insert(const AirQ& rec) const;
    bool insert(const tlgnum_t& msgId, int qNum, int sysQ, int oldId = 0, int attempts = 0) const;
    void remove(const AirQFilter& f) const;
    void remove(const std::list<QNum_t>& qNums) const;
    std::list<AirQStat>        getQStats(SysQ_t sysQ) const;
    std::list<AirQStat>        getQStats(int sysQ) const;
    boost::optional<AirQStat>  getQStat(SysQ_t sysQ, QNum_t qNum) const;
    std::list<AirQOutStat>     getQOutStats() const;
    MsgProcessStatus checkProcessStatus(const tlgnum_t& msgId) const;
    void commit() const;
    void rollback() const;

#ifdef XP_TESTING
    void clear() const;
    size_t size() const;
#endif //XP_TESTING

    ~AirQManager();

private:
    AirQCallbacks* m_cb;
};

//---------------------------------------------------------------------------------------

class AirQPartManager
{
public:
    static std::string schema();
    static std::string tableWithSchema();

    AirQPartManager(AirQPartCallbacks* cb);

    AirQPartCallbacks* callbacks() const;

    std::list<AirQPart> select(const AirQPartFilter& f, const AirQPartOrder& o = {}) const;
    bool insert(const AirQPart& rec) const;
    bool insert(int part, int peer, const tlgnum_t& msgIdLocal, bool endFlag, const std::string& msgUid) const;
    void remove(const AirQPartFilter& f) const;
    unsigned count(const AirQPartFilter& f) const;
    void commit() const;
    void rollback() const;

#ifdef XP_TESTING
    void clear() const;
    size_t size() const;
#endif //XP_TESTING

    ~AirQPartManager();

private:
    AirQPartCallbacks* m_cb;
};

}//namespace telegrams

#endif /*AIR_Q_CALLBACKS_H*/
