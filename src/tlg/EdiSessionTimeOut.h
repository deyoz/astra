//
// C++ Interface: EdiSessionTimeOut
//
// Description:
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
//
//
#pragma once

#include <edilib/EdiSessionTimeOut.h>
#include <edilib/edi_types.h>

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
//void HandleEdiSessCONTRL(edilib::EdiSessionId_t Id);

}//namespace edifact
