/*
*  C++ Implementation: EdiSessionRc
*
* Description: edisession resource control
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2010
*
*/
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "edilib/edi_session_rc.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

#include "edilib/edilib_db_callbacks.h"
#include <boost/date_time/posix_time/ptime.hpp>


namespace edilib
{

//-----------------------------------------------------------------------------

void EdiSessionRc::writeDb() const
{
    edilib::EdilibDbCallbacks::instance()->ediSessionRcWriteDb(*this);
}

void EdiSessionRc::add( const EdiSessionId_t& sessId,
                        const std::string& type,
                        int timeout )
{
    EdiSessionRc ediSessRc( sessId, type, timeout );
    ediSessRc.writeDb();
}

void EdiSessionRc::remove( const EdiSessionId_t& sessId )
{
    edilib::EdilibDbCallbacks::instance()->ediSessionRcDeleteDb(sessId);
}

bool EdiSessionRc::exists( const EdiSessionId_t& sessId )
{
    return edilib::EdilibDbCallbacks::instance()->ediSessionRcIsExists(sessId);
}

EdiSessionRc EdiSessionRc::readById( const EdiSessionId_t& sessId )
{
    return edilib::EdilibDbCallbacks::instance()->ediSessionRcReadById(sessId);
}

void EdiSessionRc::readExpired(std::list< EdiSessionRc >& lExpired)
{
    edilib::EdilibDbCallbacks::instance()->ediSessionRcReadExpired(lExpired);
}

std::ostream& operator<<( std::ostream& os, const EdiSessionRc& ediSessRc )
{
    os << "sess_id:" << ediSessRc.sessionId()
            << "; type:" << ediSessRc.type()
            << "; timeout:" << ediSessRc.timeOut();
    return os;
}

}// namespace edilib
