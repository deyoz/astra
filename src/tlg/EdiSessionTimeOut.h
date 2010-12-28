//
// C++ Interface: EdiSessionTimeOut
//
// Description:
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
//
//
#ifndef _EDISESSIONTIMEOUT_H_
#define _EDISESSIONTIMEOUT_H_
#include "edilib/EdiSessionTimeOut.h"
#include "edilib/edi_types.h"

namespace edifact
{

/**
 * @brief handle one to
 * @param to
 */
void HandleEdiSessTimeOut(const edilib::EdiSessionTimeOut &to);
/**
 * @brief On CONTRL handler
 * @param Id
 */
void HandleEdiSessCONTRL(edilib::EdiSessionId_t Id);

}

#endif /*_EDISESSIONTIMEOUT_H_*/
