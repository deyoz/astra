#include "air_q_dbpg_callbacks.h"

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

#define M_UNIMPLEMENTED throw ServerFramework::Exception(STDLOG, __FUNCTION__, "unimplemented!");


namespace telegrams {
boost::optional<AirQ> AirQPgCallbacks::selectNext(const AirQFilter& f,
                                                  const AirQOrder& o) const
{
    M_UNIMPLEMENTED
}

std::list<AirQ> AirQPgCallbacks::select(const AirQFilter& f,
                                        const AirQOrder& o) const
{
    M_UNIMPLEMENTED
}

std::list<QNum_t> AirQPgCallbacks::selectQNums(SysQ_t sysQ) const
{
    M_UNIMPLEMENTED
}

void AirQPgCallbacks::updateQNum(const tlgnum_t& msgId, QNum_t qNum) const
{
    M_UNIMPLEMENTED
}

void AirQPgCallbacks::updateSysQ(const tlgnum_t& msgId, SysQ_t sysQ) const
{
    M_UNIMPLEMENTED
}

void AirQPgCallbacks::incAttempts(const tlgnum_t& msgId) const
{
    M_UNIMPLEMENTED
}

void AirQPgCallbacks::decAttempts(const tlgnum_t& msgId) const
{
    M_UNIMPLEMENTED
}

bool AirQPgCallbacks::insert(const AirQ& rec) const
{
    M_UNIMPLEMENTED
}

void AirQPgCallbacks::remove(const AirQFilter& f) const
{
    M_UNIMPLEMENTED
}

void AirQPgCallbacks::remove(const std::list<QNum_t>& qNums) const
{
    M_UNIMPLEMENTED
}

std::list<AirQStat> AirQPgCallbacks::getQStats(SysQ_t sysQ) const
{
    M_UNIMPLEMENTED
}

boost::optional<AirQStat> AirQPgCallbacks::getQStat(SysQ_t sysQ, QNum_t qNum) const
{
    M_UNIMPLEMENTED
}

std::list<AirQOutStat> AirQPgCallbacks::getQOutStats() const
{
    M_UNIMPLEMENTED
}

MsgProcessStatus AirQPgCallbacks::checkProcessStatus(const tlgnum_t& msgId) const
{
    M_UNIMPLEMENTED
}

void AirQPgCallbacks::commit() const
{
    M_UNIMPLEMENTED
}

void AirQPgCallbacks::rollback() const
{
    M_UNIMPLEMENTED
}

#ifdef XP_TESTING
void AirQPgCallbacks::clear() const
{
    M_UNIMPLEMENTED
}

size_t AirQPgCallbacks::size() const
{
    M_UNIMPLEMENTED
}
#endif //XP_TESTING

//---------------------------------------------------------------------------------------

std::list<AirQPart> AirQPartPgCallbacks::select(const AirQPartFilter& f,
                                                const AirQPartOrder& o) const
{
    M_UNIMPLEMENTED
}

bool AirQPartPgCallbacks::insert(const AirQPart& rec) const
{
    M_UNIMPLEMENTED
}

void AirQPartPgCallbacks::remove(const AirQPartFilter& f) const
{
    M_UNIMPLEMENTED
}

unsigned AirQPartPgCallbacks::count(const AirQPartFilter& f) const
{
    M_UNIMPLEMENTED
}

void AirQPartPgCallbacks::commit() const
{
    M_UNIMPLEMENTED
}

void AirQPartPgCallbacks::rollback() const
{
    M_UNIMPLEMENTED
}

#ifdef XP_TESTING
void AirQPartPgCallbacks::clear() const
{
    M_UNIMPLEMENTED
}

size_t AirQPartPgCallbacks::size() const
{
    M_UNIMPLEMENTED
}
#endif //XP_TESTING

}//namespace telegrams

#undef M_UNIMPLEMENTED
