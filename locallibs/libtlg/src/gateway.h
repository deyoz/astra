#ifndef LIBTLG_GATEWAY_H
#define LIBTLG_GATEWAY_H

#include <stdlib.h>
#include "consts.h"

namespace telegrams
{
/**
 * ⨯ ⥫��ࠬ� (���� type � AIRSRV_MSG) 
 * */
enum TLG_TYPE
{
    TLG_IN = 0,         ///< �ਭ������ �� � ᮮ�饭��
    TLG_OUT,            ///< �ਭ������/��ࠢ�塞� �� �� ᮮ�饭��
    TLG_ACK,            ///< ���⠭�� � ����祭�� ���஬
    TLG_F_ACK,          ///< ���⠭�� � ����祭�� ����⮬
    TLG_F_NEG,          ///< ���⠭�� � �����⠢��
    TLG_CRASH,          ///< �� ᮮ�饭��, �� ���⢥ত���� TLG_F_ACK, ������
    TLG_ACK_ACK,        ///< ���⠭�� � ����祭�� ����⮬ ���⠭権 TLG_F_ACK, TLG_F_NEG � TLG_CRASH
    TLG_ERR_CFG,        ///< ���⠭�� � ������������ ��।�� ⫣ �१ �����
    TLG_CONN_REQ,       ///< ����� �� ��⠭���� ����� �裡 � ��७�
    TLG_CONN_RES,       ///< �⢥� �� ��७�
    TLG_FLOW_A_ON,      ///< ������ Type-A �஭�� � ������ ��ࠢ�⥫� ⫣
    TLG_FLOW_A_OFF,     ///< �������  - || -
    TLG_FLOW_B_ON,      ///< ������ Type-B �஭�� � ������ ��ࠢ�⥫� ⫣
    TLG_FLOW_B_OFF,     ///< �������  - || -
    TLG_FLOW_AB_ON,     ///< ������ ���
    TLG_FLOW_AB_OFF,    ///< ������� ���
    TLG_ERR_TLG,        ///< ��䥪⭠� ⫣
};

#define ROT_NAME_LEN 5
/**
 * ᮮ�饭�� ��� ��ࠢ��
 * �� ������ � ���塠��� 楫� ��।����� � network �ଠ� 
 * */
struct AIRSRV_MSG
{
    int32_t num;                   ///< ����� ⥫��ࠬ��
    unsigned short int type;        ///< ⨯ ⥫��ࠬ� (TLG_TYPE)
    char Sender[ROT_NAME_LEN + 1];  ///< ���ᨬ����� ����, �����襭�� �㫥�
    char Receiver[ROT_NAME_LEN + 1];///< ���ᨬ����� ����, �����襭�� �㫥�
    unsigned short int TTL;         ///< �६� ���㠫쭮�� ⥫��ࠬ�� � ᥪ㭤��
    char body[MAX_TLG_SIZE];        ///< ⥫� ⥫��ࠬ�
};

/**
 * ��⠥� ॠ���� ����� ⥫��ࠬ��, �᭮�뢠��� �� ⮬,
 *  �� body �����稢����� �㫥� */
extern "C" size_t tlgLength(const AIRSRV_MSG& t);
extern "C" char* tlgTime();

} // namespace telegrams

#endif /* LIBTLG_GATEWAY_H */

