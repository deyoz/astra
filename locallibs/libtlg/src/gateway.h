#ifndef LIBTLG_GATEWAY_H
#define LIBTLG_GATEWAY_H

#include <stdlib.h>
#include "consts.h"

namespace telegrams
{
/**
 * тип телеграмы (поле type в AIRSRV_MSG) 
 * */
enum TLG_TYPE
{
    TLG_IN = 0,         ///< принимаемые от шлюза сообщения
    TLG_OUT,            ///< принимаемые/отправляемые на шлюз сообщения
    TLG_ACK,            ///< Квитанция о получении роутером
    TLG_F_ACK,          ///< Квитанция о получении адресатом
    TLG_F_NEG,          ///< Квитанция о недоставке
    TLG_CRASH,          ///< Все сообщения, не подтвержденные TLG_F_ACK, потеряны
    TLG_ACK_ACK,        ///< Квитанция о получении адресатом квитанций TLG_F_ACK, TLG_F_NEG и TLG_CRASH
    TLG_ERR_CFG,        ///< Квитанция о невозможности передачи тлг через ГРСшлюз
    TLG_CONN_REQ,       ///< Запрос на установку логич связи в Сирену
    TLG_CONN_RES,       ///< Ответ из Сирены
    TLG_FLOW_A_ON,      ///< Открыть Type-A бронир в инвент отправителе тлг
    TLG_FLOW_A_OFF,     ///< Закрыть  - || -
    TLG_FLOW_B_ON,      ///< Открыть Type-B бронир в инвент отправителе тлг
    TLG_FLOW_B_OFF,     ///< Закрыть  - || -
    TLG_FLOW_AB_ON,     ///< Открыть всё
    TLG_FLOW_AB_OFF,    ///< Закрыть всё
    TLG_ERR_TLG,        ///< Дефектная тлг
};

#define ROT_NAME_LEN 5
/**
 * сообщение для отправки
 * все длинные и двухбайтные целые передаются в network формате 
 * */
struct AIRSRV_MSG
{
    int32_t num;                   ///< номер телеграммы
    unsigned short int type;        ///< тип телеграмы (TLG_TYPE)
    char Sender[ROT_NAME_LEN + 1];  ///< пятисимвольный адрес, завершенный нулем
    char Receiver[ROT_NAME_LEN + 1];///< пятисимвольный адрес, завершенный нулем
    unsigned short int TTL;         ///< время актуальности телеграммы в секундах
    char body[MAX_TLG_SIZE];        ///< тело телеграмы
};

/**
 * считает реальную длину телеграммы, основываясь на том,
 *  что body заканчивается нулем */
extern "C" size_t tlgLength(const AIRSRV_MSG& t);
extern "C" char* tlgTime();

} // namespace telegrams

#endif /* LIBTLG_GATEWAY_H */

