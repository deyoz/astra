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
        LogError(STDLOG) << "����� �訡�� �� �����⥭ ��⥬�";
        return l==RUSSIAN?"�訡�� � �ணࠬ��":"Program error";
    } else {
        return pos->second;
    }
}


    REGERR(ProgErr,
           "Program error",
           "�訡�� � �ணࠬ��");
    REGERR(WRONG_CC,
           "Credit card data missing or invalid",
           "���ଠ�� �� �।�⭮� ���� ��������� ��� ����ୠ");
    REGERR(INV_COUPON_STATUS,
           "Invalid ticket/coupon status",
           "������ ����� �����/�㯮��");
    REGERR(INV_ITIN_STATUS,
           "Invalid Segment Status",
           "������ ����� �������");
    REGERR(INV_DATE,
           "Invalid date",
           "����ୠ� ���");
    REGERR(INV_TIME,
           "Invalid time",
           "����୮� �६�");
    REGERR(INV_RBD,
           "Invalid or missing reservation booking designator",
           "��� �������� ����७ ��� ���������");
    REGERR(INV_TICK_ACT,
           "Invalid Ticket Action Code",
           "������ ��� ����⢨� �����");
    REGERR(INV_CPN_ACT,
           "Invalid Coupon Action Code",
           "������ ��� ����⢨� �㯮��");
    REGERR(INV_FOP_ACT,
           "Invalid Fop Action Code",
           "������ ��� ����⢨� ���� ������");
    REGERR(INV_TAX_AMOUNT,
           "Invalid Tax Amount",
           "����ୠ� ����稭� ����");
    REGERR(MISS_MONETARY_INF,
           "Missing and/or invalid monetary information",
           "���ଠ�� �� ����� ��������� �/��� �� ��ୠ");
    REGERR(INV_PASS_TYPE,
           "Invalid or ineligible passenger type code",
           "�訡�筠� ��⥣��� ���ᠦ��");
    REGERR(INV_IFT_QUALIFIER,
           "Free text qualifier error",
           "�訡��� �����䨪��� ᢮������� ⥪��");

    REGERR(INV_FOID,
           "Missing or invalid airport check-in identification (FOID)",
           "��ଠ �����䨪�樨 ���ᠦ�� � ��ய���� (FOID) ����ୠ ��� ���������");
    REGERR(INV_COUPON_NUM,
           "Invalid coupon number in the ticket %1%. Coupon number is %2%",
           "����୮� ������⢮ �㯮��� � ����� %1%. �ᥣ� �㯮��� %2%")
    REGERR(ETS_INV_LUGGAGE,
           "Invalid or missing baggage details",
           "��ଠ ������ ����ୠ ��� ���������")
}
