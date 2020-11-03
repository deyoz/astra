//
// C++ Interface: typeb_msg
//
// Description: Локализация сообщений парсинга type_b
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _TYPEB_MSG_H_
#define _TYPEB_MSG_H_
#include <etick/etick_msg.h>

#define REGERR(x) DEF_ERR__( x )

namespace typeb_parser
{
    struct TBMsg
    {
        REGERR(INV_ADDRESSES);
        REGERR(UNKNOWN_MSG_TYPE);
        REGERR(UNKNOWN_MESSAGE_ELEM);

        REGERR(MISS_NECESSARY_ELEMENT);
        REGERR(TOO_FEW_ELEMENTS);
        REGERR(TOO_LONG_MSG_LINE);
        REGERR(EMPTY_MSG_LINE);

        REGERR(INV_NAME_ELEMENT);
        REGERR(WRONG_CHARS_IN_NAME);
        REGERR(INV_REMARK_STATUS);
        REGERR(INV_TICKET_NUM);
        REGERR(MISS_COUPON_NUM);
        REGERR(INV_COUPON_NUM);

        REGERR(PROG_ERR);
        REGERR(INV_DATETIME_FORMAT_z1z);
        REGERR(INV_FORMAT_z1z);
        REGERR(INV_FORMAT_z1z_FIELD_z2z);

        REGERR(INV_RL_ELEMENT);
        REGERR(UNKNOWN_SSR_CODE);
        REGERR(NO_HANDLER_FOR_THIS_TYPE);
        REGERR(EMPTY_SSR_BODY);
        REGERR(PARTLY_HANDLED);

        REGERR(INV_NUMS_BY_DEST_ELEM);
        REGERR(INV_CATEGORY_AP_ELEM);
        REGERR(INV_CATEGORY_ELEM);

        REGERR(INV_TICKNUM);
        REGERR(INV_END_ELEMENT);
        REGERR(INV_FQTx_FORMAT); 
    };
}
#undef REGERR

#endif /*_TYPEB_MSG_H_*/
