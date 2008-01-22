#include "edi_msg.h"

//#define REGERR(x,e,r) const ErrMsg_t EtsErr::x="EtsErr::"#x
#define REGERR(x,e,r) const Ticketing::ErrMsg_t EdiErr::x="EdiErr::"#x;\
    namespace {\
    Ticketing::ErrMsgs x (EdiErr::x, e,r);\
}

REGERR(EDI_PROC_ERR,
       "UNABLE TO PROCESS - SYSTEM ERROR",
       "���������� ���������� - ��������� ������");

REGERR(EDI_INV_MESSAGE_F,
       "MESSAGE FUNCTION INVALID",
       "�������� ������� ���������");
REGERR(EDI_NS_MESSAGE_F,
       "MESSAGE FUNCTION NOT SUPPORTED",
       "������� ��������� �� ��������������");
REGERR(EDI_SYNTAX_ERR,
       "EDIFACT SYNTAX MESSAGE ERROR",
       "�������������� ������ � ��������� EDIFACT");

