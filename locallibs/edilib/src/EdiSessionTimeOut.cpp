/*
*  C++ Implementation: EdiSessionTimeOut
*
* Description:
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
*
*/
#include <boost/scoped_ptr.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "edilib/EdiSessionTimeOut.h"
#include "edilib/edilib_db_callbacks.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace edilib
{

EdiSessionTimeOut::EdiSessionTimeOut(edi_msg_types_t msg_type,
                                     const std::string &func_code,
                                     EdiSessionId_t id,
                                     int timeout)
    :EdiSessionId(id), MsgType(msg_type), FuncCode(func_code), Timeout(timeout)
{

}

void EdiSessionTimeOut::add(edi_msg_types_t msg_type,
                            const std::string &func_code,
                            EdiSessionId_t id,
                            int timeout)
{
    EdiSessionTimeOut SessTO(msg_type, func_code, id, timeout);
    SessTO.writeDb();
}


boost::posix_time::ptime getTimeOut(int to)
{
    return boost::posix_time::second_clock::local_time() + boost::posix_time::time_duration(0,0,std::abs(to));
}

int expiredSecs(const boost::posix_time::ptime &d)
{
    return (boost::posix_time::second_clock::local_time() - d).seconds();
}

std::string EdiSessionTimeOut::msgTypeStr() const
{
    return GetEdiMsgNameByType(msgType());
}

void EdiSessionTimeOut::writeDb()
{
    EdilibDbCallbacks::instance()->ediSessionToWriteDb(*this);
}

edi_msg_types_t EdiSessionTimeOut::answerMsgType() const
{
    return GetEdiAnswerByType(msgType());
}

EdiSessionTimeOut EdiSessionTimeOut::readById(EdiSessionId_t Id)
{
    return EdilibDbCallbacks::instance()->ediSessionToReadById(Id);
}

void EdiSessionTimeOut::readExpiredSessions(std::list<EdiSessionTimeOut> & lExpired)
{
    EdilibDbCallbacks::instance()->ediSessionToReadExpired(lExpired);
}

std::ostream & operator <<(std::ostream & s, const EdiSessionTimeOut & edisess)
{
    s << "sess_id: " << edisess.ediSessionId() <<
            "; msgType: " << edisess.msgType() <<
            " [" << GetEdiMsgNameByType(edisess.msgType()) << "]" <<
            "; func_code: " << edisess.funcCode() <<
            "; time_out: " << edisess.timeout();

    return s;
}

EdiSession::ReadStatus EdiSessionTimeOut::lock() const
{
    EdiSession::ReadResult res = EdilibDbCallbacks::instance()->ediSessionReadByIda(ediSessionId(), true, 0);

    switch(res.status)
    {
        case EdiSession::NoDataFound:
            LogTrace(TRACE1) << "EdiSession not found by ida: " << ediSessionId();
            break;
        case EdiSession::Locked:
            LogTrace(TRACE1) << "EdiSession locked. ida: " << ediSessionId();
            break;
        case EdiSession::ReadOK:
            LogTrace(TRACE3) << "EdiSession locked OK. ida: " << ediSessionId();
            break;
    }

    return res.status;
}

bool EdiSessionTimeOut::exists(EdiSessionId_t Id)
{
    return EdilibDbCallbacks::instance()->ediSessionToIsExists(Id);
}

void EdiSessionTimeOut::deleteDb(EdiSessionId_t sessid)
{
    EdilibDbCallbacks::instance()->ediSessionToDeleteDb(sessid);
}

void EdiSessionTimeOut::deleteEdiSession() const
{
    EdilibDbCallbacks::instance()->ediSessionDeleteDb(ediSessionId());
}

}
