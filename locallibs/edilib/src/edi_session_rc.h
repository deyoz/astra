/*
*  C++ Interface: EdiSessionRc
*
* Description: edisession resource control
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2010
*
*/

#ifndef _EDILIB_EDISESSION_RC_H_
#define _EDILIB_EDISESSION_RC_H_

#include <list>

#include "edilib/EdiSessionId_t.h"


namespace edilib
{

///@brief edisession resource control
class EdiSessionRc
{
    EdiSessionId_t SessId;
    std::string Type;
    int TimeOut;

public:
    EdiSessionRc( const EdiSessionId_t &sessId, const std::string &type, int to )
        : SessId( sessId ), Type( type ), TimeOut( to )
    {}

    const EdiSessionId_t& sessionId() const { return SessId; }
    const std::string& type() const { return Type; }
    int timeOut() const { return TimeOut; }

    void writeDb() const;

    static void add( const EdiSessionId_t& sessId,
                     const std::string& type,
                     int timeout );

    static void remove( const EdiSessionId_t& sessId );

    static bool exists( const EdiSessionId_t& sessId );

    static EdiSessionRc readById( const EdiSessionId_t& sessId );

    static void readExpired( std::list< EdiSessionRc >& lExpired );
};

std::ostream& operator<<( std::ostream& os, const EdiSessionRc& ediSess );

//-----------------------------------------------------------------------------



} // namespace edilib
#endif // _EDILIB_EDISESSION_RC_H
