/*
*  C++ Implementation: typeb_msg
*
* Description: ���������� ᮮ�饭�� ���ᨭ�� type_b
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/

#include "typeb_msg.h"

#define REGERR(x,e,r) REG_ERR__(TBMsg, x,e,r)

namespace typeb_parser
{
    REGERR(INV_ADDRESSES,
           "Invalid senser/receiver field",
           "�訡�� � ���� ��ࠢ�⥫�/�����⥫�");
    REGERR(UNKNOWN_MSG_TYPE,
           "Unknown type_b message code type",
           "��������� ⨯ type_b ᮮ�饭��");
    REGERR(UNKNOWN_MESSAGE_ELEM,
           "Unknown message element",
           "��������� ����� ᮮ�饭��");
    REGERR(MISS_NECESSARY_ELEMENT,
           "Missed necessary element",
           "�ய�饭 ��易⥫�� �����");
    REGERR(TOO_FEW_ELEMENTS,
           "Too few elements",
           "���誮� ���� ����⮢ ������� ⨯�");
    REGERR(TOO_LONG_MSG_LINE,
           "Too long TypeB message line",
           "���誮� ������� ��ப� TypeB ᮮ�饭��");
    REGERR(EMPTY_MSG_LINE,
          "Empty TypeB message line",
          "����� ��ப� � TypeB ᮮ�饭��");
    REGERR(PROG_ERR,
          "Program error at time of parsing",
          "�ணࠬ���� �訡�� �� ࠧ��� ᮮ�饭��");
    REGERR(INV_DATETIME_FORMAT_z1z,
          "������ �ଠ� ����/�६��� '%1%'",
          "Invalid date/time format '%1%'");
    REGERR(INV_FORMAT_z1z,
          "Invalid format of '%1%'",
          "������ �ଠ� ����� '%1%'");
    REGERR(INV_FORMAT_z1z_FIELD_z2z,
           "Invalid format of '%1%' field %2%",
           "������ �ଠ� ����� '%1%' � ���� %2%");
    REGERR(INV_NAME_ELEMENT,
          "Error in name element",
          "�訡�� � ᥣ���� ����");
    REGERR(WRONG_CHARS_IN_NAME,
          "Wrong characters in name element",
          "�������⨬� ����� ᨬ����� � ����� ����");

    REGERR(INV_REMARK_STATUS,
          "Invalid remark status code",
          "������ ��� ����� ६�ન");
    REGERR(INV_TICKET_NUM,
          "Invalid ticket number in remark",
          "������ ����� ����� � ६�થ");
    REGERR(MISS_COUPON_NUM,
          "Missing coupon number in remark",
          "�ய�饭 ����� �㯮�� � ६�થ");
    REGERR(INV_COUPON_NUM,
           "Invalid coupon number",
           "������ ����� �㯮��");

    REGERR(INV_RL_ELEMENT,
           "Invalid recloc element",
           "������ Recloc �������");
    REGERR(UNKNOWN_SSR_CODE,
           "Unknown SSR code",
           "��������� ��� SSR");
    REGERR(NO_HANDLER_FOR_THIS_TYPE,
           "There are no handler for this type of airimp message",
           "��� ��ࠡ��稪� ��� ������� ⨯� airimp ᮮ�饭��");
    REGERR(EMPTY_SSR_BODY,
           "Empty SSR body",
           "���⮩ SSR");
    REGERR(PARTLY_HANDLED,
           "Message partly handled",
           "����饭�� �뫮 ��ࠡ�⠭� ���筮");
    REGERR(INV_TICKNUM,
        "INVALID TICKET NUMBER",
        "������������ ����� ������");

    REGERR(INV_NUMS_BY_DEST_ELEM,
           "Invalid Numerics by destination element",
           "������ ������� 'Numerics by destination'");
    REGERR(INV_CATEGORY_AP_ELEM,
           "Invalid Category by destination element",
           "������ ������� 'Category by destination'");
    REGERR(INV_CATEGORY_ELEM,
           "Invalid Category element",
           "������ ������� 'Category'");
    REGERR(INV_END_ELEMENT,
           "Invalid END element",
           "������ ������� 'End element'");
           
    REGERR(INV_FQTx_FORMAT,
           "Invalid FQTX element format",
           "������ ����� FQTX");
}

