#pragma once

#include "air_q_callbacks.h"


namespace telegrams {

class AirQOraCallbacks: public AirQCallbacks
{
public:
    virtual boost::optional<AirQ> selectNext(const AirQFilter& f, const AirQOrder& o) const;

    virtual std::list<AirQ> select(const AirQFilter& f, const AirQOrder& o) const;

    virtual std::list<QNum_t> selectQNums(SysQ_t sysQ) const;

    virtual void updateQNum(const tlgnum_t& msgId, QNum_t qNum) const;
    virtual void updateSysQ(const tlgnum_t& msgId, SysQ_t sysQ) const;

    virtual void incAttempts(const tlgnum_t& msgId) const;
    virtual void decAttempts(const tlgnum_t& msgId) const;

    virtual bool insert(const AirQ& rec) const;

    virtual void remove(const AirQFilter& filter) const;
    virtual void remove(const std::list<QNum_t>& qNums) const;

    virtual std::list<AirQStat>       getQStats(SysQ_t sysQ) const;
    virtual boost::optional<AirQStat>  getQStat(SysQ_t sysQ, QNum_t qNum) const;

    virtual std::list<AirQOutStat> getQOutStats() const;

    virtual MsgProcessStatus checkProcessStatus(const tlgnum_t& msgId) const;

    virtual void commit() const;
    virtual void rollback() const;

#ifdef XP_TESTING
    virtual void clear() const;
    virtual size_t size() const;

#endif //XP_TESTING

    virtual ~AirQOraCallbacks() {}
};

//---------------------------------------------------------------------------------------

class AirQPartOraCallbacks: public AirQPartCallbacks
{
public:
    virtual std::list<AirQPart> select(const AirQPartFilter& f, const AirQPartOrder& o) const;

    virtual bool insert(const AirQPart& rec) const;

    virtual void remove(const AirQPartFilter& f) const;

    virtual unsigned count(const AirQPartFilter& f) const;

    virtual void commit() const;
    virtual void rollback() const;

#ifdef XP_TESTING
    virtual void clear() const;
    virtual size_t size() const;
#endif //XP_TESTING

    virtual ~AirQPartOraCallbacks() {}
};

}//namespace telegrams
