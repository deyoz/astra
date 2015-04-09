#pragma once

#include <libtlg/tlgnum.h>
#include <edilib/EdiSessionId_t.h>


namespace TlgHandling
{

class TlgToBePostponed
{
    edilib::EdiSessionId_t m_sessId;
public:
    TlgToBePostponed(edilib::EdiSessionId_t sessId)
        : m_sessId(sessId)
    {}

    edilib::EdiSessionId_t sessionId() const { return m_sessId; }
};

//-----------------------------------------------------------------------------

class PostponeEdiHandling
{
protected:
    static void insertDb(const tlgnum_t& tnum, edilib::EdiSessionId_t sessId);
    static tlgnum_t deleteDb(edilib::EdiSessionId_t sessId);
    static void addToQueue(const tlgnum_t& tnum);
public:
    static void postpone(const tlgnum_t& tnum, edilib::EdiSessionId_t sessId);
    static void postpone(int tnum, edilib::EdiSessionId_t sessId);
    static tlgnum_t deleteWaiting(edilib::EdiSessionId_t sessId);
    static tlgnum_t findPostponeTlg(edilib::EdiSessionId_t sessId);
};

//-----------------------------------------------------------------------------

void updateTlgToPostponed(const tlgnum_t& tnum);
bool isTlgPostponed(const tlgnum_t& tnum);

}//namespace TlgHandling
