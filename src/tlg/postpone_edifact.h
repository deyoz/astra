#pragma once

#include <libtlg/tlgnum.h>
#include <edilib/EdiSessionId_t.h>

#include <list>


namespace TlgHandling
{

class TlgToBePostponed
{
    tlgnum_t m_tlgNum;
public:
    TlgToBePostponed(tlgnum_t tnum)
        : m_tlgNum(tnum)
    {}

    const tlgnum_t tlgNum() const { return m_tlgNum; }
};

//-----------------------------------------------------------------------------

class PostponeEdiHandling
{
protected:
    static void insertDb(const tlgnum_t& tnum, edilib::EdiSessionId_t sessId);
    static boost::optional<tlgnum_t> deleteDb(edilib::EdiSessionId_t sessId);
    static void addToQueue(const tlgnum_t& tnum);
public:
    static void postpone(const tlgnum_t& tnum, edilib::EdiSessionId_t sessId);
    static void postpone(int tnum, edilib::EdiSessionId_t sessId);
    static boost::optional<tlgnum_t> deleteWaiting(edilib::EdiSessionId_t sessId);
    static boost::optional<tlgnum_t> findPostponeTlg(edilib::EdiSessionId_t sessId);
    static void deleteWaiting(const tlgnum_t& tnum);
};

//-----------------------------------------------------------------------------

void updateTlgToPostponed(const tlgnum_t& tnum);
bool isTlgPostponed(const tlgnum_t& tnum);

}//namespace TlgHandling
