#include "astra_msg.h"

#define REGERR(x,e,r) ADDERR(AstraErr,x,e,r)
#define REGMSG(x,e,r) ADDMSG(AstraMsg,x,e,r)

namespace Ticketing
{

REGERR(TIMEOUT_ON_HOST_3,
       "Timeout occured on host 3",
       "�������");
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

}//namespace Ticketing
