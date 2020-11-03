/*	2006 by Roman Kovalev 	*/
/*	roman@pike.dev.sirena2000.ru		*/
#include "etick/etick_msg.h"
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

#define REGERR(x,e,r) const ErrMsg_t EtErr::x="EtErr::"#x;\
    namespace {\
    ErrMsgs x (EtErr::x, e,r);\
    }

namespace Ticketing{

LocalizationMap &getLocalizMap(Language l)
{
    static LocalizationMap *locMapEN=0;
    static LocalizationMap *locMapRU=0;

    if(l == RUSSIAN){
        if(!locMapRU){
            locMapRU = new LocalizationMap;
        }
        return *locMapRU;
    } else {
        if(!locMapEN){
            locMapEN = new LocalizationMap;
        }
        return *locMapEN;
    }
}

std::string ErrMsgs::Localize(const ErrMsg_t & key, Language l)
{
    const LocalizationMap &loc_map = getLocalizMap(l);
    LocalizationMap::const_iterator pos=loc_map.find(key);
    if(pos==loc_map.end()) // not found
    {
        LogError(STDLOG) << "Номер ошибки не известен системе";
        return l==RUSSIAN?"Ошибка в программе":"Program error";
    } else {
        return pos->second;
    }
}


    REGERR(ProgErr,
           "Program error",
           "Ошибка в программе");
    REGERR(WRONG_CC,
           "Credit card data missing or invalid",
           "Информация по кредитной карте отсутствует или неверна");
    REGERR(INV_COUPON_STATUS,
           "Invalid ticket/coupon status",
           "Неверный статус билета/купона");
    REGERR(INV_ITIN_STATUS,
           "Invalid Segment Status",
           "Неверный Статус Сегмента");
    REGERR(INV_DATE,
           "Invalid date",
           "Неверная дата");
    REGERR(INV_TIME,
           "Invalid time",
           "Неверное время");
    REGERR(INV_RBD,
           "Invalid or missing reservation booking designator",
           "Код подкласса неверен или отсутствует");
    REGERR(INV_TICK_ACT,
           "Invalid Ticket Action Code",
           "Неверный Код Действия Билета");
    REGERR(INV_CPN_ACT,
           "Invalid Coupon Action Code",
           "Неверный Код Действия Купона");
    REGERR(INV_FOP_ACT,
           "Invalid Fop Action Code",
           "Неверный Код Действия Формы Оплаты");
    REGERR(INV_TAX_AMOUNT,
           "Invalid Tax Amount",
           "Неверная Величина Сбора");
    REGERR(MISS_MONETARY_INF,
           "Missing and/or invalid monetary information",
           "Информация об оплате отсутствует и/или не верна");
    REGERR(INV_PASS_TYPE,
           "Invalid or ineligible passenger type code",
           "Ошибочная категория пассажира");
    REGERR(INV_IFT_QUALIFIER,
           "Free text qualifier error",
           "Ошибочный классификатор свободного текста");

    REGERR(INV_FOID,
           "Missing or invalid airport check-in identification (FOID)",
           "Форма идентификации пассажира в аэропорту (FOID) неверна или отсутствует");
    REGERR(INV_COUPON_NUM,
           "Invalid coupon number in the ticket %1%. Coupon number is %2%",
           "Неверное количество купонов в билете %1%. Всего купонов %2%")
    REGERR(ETS_INV_LUGGAGE,
           "Invalid or missing baggage details",
           "Норма багажа неверна или отсутствует")
}
