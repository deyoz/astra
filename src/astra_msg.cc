#include "astra_msg.h"

#define ADDERR(s,x,e,r) const Ticketing::ErrMsg_t s::x="MSG."#x;\
    namespace {\
    Ticketing::ErrMsgs x (s::x, e,r);\
}
#define ADDMSG(s,x,e,r) const Ticketing::AstraMsg_t s::x="MSG."#x;\
    namespace {\
    Ticketing::ErrMsgs x (s::x, e,r);\
}

#define REGERR(x,e,r) ADDERR(AstraErr,x,e,r)
#define REGMSG(x,e,r) ADDMSG(AstraMsg,x,e,r)

namespace Ticketing {

REGERR(TIMEOUT_ON_HOST_3,
       "TIMEOUT OCCURED ON HOST 3",
       "�������")
REGERR(EDI_PROC_ERR,
       "UNABLE TO PROCESS - SYSTEM ERROR",
       "���������� ���������� - ��������� ������")
REGERR(EDI_INV_MESSAGE_F,
       "MESSAGE FUNCTION INVALID",
       "�������� ������� ���������")
REGERR(EDI_NS_MESSAGE_F,
       "MESSAGE FUNCTION NOT SUPPORTED",
       "������� ��������� �� ��������������")
REGERR(EDI_SYNTAX_ERR,
       "EDIFACT SYNTAX MESSAGE ERROR",
       "�������������� ������ � ��������� EDIFACT")
REGERR(PAX_SURNAME_NF,
       "PASSENGER SURNAME NOT FOUND",
       "�������� �� ������")
REGERR(INV_FLIGHT_DATE,
       "INVALID FLIGHT/DATE",
       "�������� ����/����")
REGERR(TOO_MANY_PAX_WITH_SAME_SURNAME,
       "TOO MANY PASSENGERS WITH SAME SURNAME",
       "�������� ����� ���������� � ���������� ��������")
REGERR(FLIGHT_NOT_FOR_THROUGH_CHECK_IN,
       "FLIGHT NOT OPEN FOR THROUGH CHECK-IN",
       "�������� ����������� ����������� ��� �����")
REGERR(PAX_ALREADY_CHECKED_IN,
       "PASSENGER SURNAME ALREADY CHECKED IN",
       "�������� � ��������� �������� ��� ���������������")
REGERR(BAGGAGE_WEIGHT_REQUIRED,
       "BAGGAGE WEIGHT REQUIRED",
       "��������� ��� ������")
REGERR(NO_SEAT_SELCTN_ON_FLIGHT,
       "NO SEAT SELECTION ON THIS FLIGHT",
       "����������� ����������� ������ ����� �� �����")
REGERR(TOO_MANY_PAXES,
       "TOO MANY PASSENGERS",
       "�������� ����� ����������")
REGERR(TOO_MANY_INFANTS,
       "TOO MANY INFANTS",
       "�������� ����� ���������")
REGERR(SMOKING_ZONE_UNAVAILABLE,
       "SMOKING ZONE UNAVAILABLE",
       "���� ��� ������� ����������")
REGERR(NON_SMOKING_ZONE_UNAVAILABLE,
       "NON-SMOKING ZONE UNAVAILABLE",
       "���� ��� ��������� ����������")
REGERR(PAX_SURNAME_NOT_CHECKED_IN,
       "PASSENGER SURNAME NOT CHECKED-IN",
       "�������� � ��������� �������� �� ���������������")
REGERR(CHECK_IN_SEPARATELY,
       "CHECK-IN SEPARATELY",
       "��������� ���������� �����������")
REGERR(UPDATE_SEPARATELY,
       "UPDATE SEPARATELY",
       "��������� ���������� ����������")
REGERR(CASCADED_QUERY_TIMEOUT,
       "CASCADED QUERY TIMED OUT",
       "������� ���������� �������")
REGERR(ID_CARD_REQUIRED,
       "PASSPORT/ID DOCUMENT DATA REQUIRED",
       "��������� ����� ��������/���������")
REGERR(DOC_EXPIRE_DATE_REQUIRED,
       "API DOCUMENT EXPIRE DATE REQUIRED",
       "��������� ���� �������� ��������/���������")
REGERR(INV_SEAT,
       "INVALID SEAT NUMBER",
       "�������� ����� �����")
REGERR(FLIGHT_CLOSED,
       "FLIGHT CLOSED",
       "���� ������")
REGERR(UNABLE_TO_GIVE_SEAT,
       "UNABLE TO GIVE SEAT",
       "���������� ������� ������ �����")
REGERR(FUNC_NOT_SUPPORTED,
       "FUNCTION NOT SUPPORTED",
       "�������� �� ��������������")
REGERR(INV_COUPON_STATUS,
       "INVALID TICKET/DOCUMENT COUPON STATUS",
       "�������� ������ ������ ������/���������")

}//namespace Ticketing


#undef ADDERR
#undef ADDMSG
#undef REGERR
#undef REGMSG
