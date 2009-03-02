#ifndef _EDI_MSG_H_
#define _EDI_MSG_H_

#include <string>
#include <map>
#include "etick/etick_msg.h"
#include "etick/exceptions.h"

#define REGERR(x) static const Ticketing::ErrMsg_t x

typedef Ticketing::TickExceptions::tick_exception edi_exception;
typedef Ticketing::TickExceptions::tick_fatal_except edi_fatal_except;
typedef Ticketing::TickExceptions::tick_soft_except edi_soft_except;
struct EdiErr{
    REGERR(EDI_PROC_ERR);
    REGERR(EDI_INV_MESSAGE_F);
    REGERR(EDI_NS_MESSAGE_F);
    REGERR(EDI_SYNTAX_ERR);

};
#undef REGERR
#endif /*_EDI_MSG_H_*/
