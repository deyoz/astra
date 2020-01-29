#ifndef __MSG_CONST_H_
#define __MSG_CONST_H_


#define MSG_TEXT            0x01 /* 1 */
#define MSG_BINARY          0x02 /* 2 */
#define MSG_COMPRESSED      0x04 /* 4 */
#define MSG_ENCRYPTED       0x08 /* 8 */
#define MSG_CAN_COMPRESS    0x10 /* 16 */
#define MSG_SPECIAL_PROC    0x20 /* 32 */
#define MSG_PUB_CRYPT       0x40 /* 64 */
#define MSG_SYS_ERROR       0x80 /* 128 */

#define REQ_NOT_PROCESSED   0x01 /* 1 */
#define MSG_MESPRO_CRYPT    0x02 /* 2 */
#define FLAG_INVALID_XUSER  0x04 /* 4 */       // ������ ����� ���짮��⥫�

// COMM_PARAMS_BYTE=84; // 84
#define MSG_TYPE_PERESPROS  0x01 /* 1 */

#define JXT_DEV             0x01 /* 1 */

/*LEVBDATA format
 *  12 byte - internal msgid (unique int + pid +time_t)
 *  4 bytes - flags
 *  1 byte - timeout
 * */
#define SIRENATECHHEAD 14
#define IDLENA 32
#define IDLENB 10
#define LEVBDATA 32
#define PUL_KEYS_LEN 8

// 1. 0/1 Head type
// 2. 1/12 Internal msgid (unique int + pid + time_t)
// 3. 12/4 Timeout
// 4. 16/1 Flags
// 5. 17/2 Reserved
#define HEADTYPE_HTTP 8
#define HTTP_HEADER_PREFIX 20
#define HTTP_HEADER_TIMEOUT 13
#define HTTP_HEADER_FLAGS  17

// 1. 0/4 ����� ⥫� ᮮ�饭��
// 2. 4/4 �६� ᮧ����� ����� (���-�� ᥪ㭤 � 1 ﭢ��� 1970 GMT)
// 3. 8/4 �����䨪��� ᮮ�饭��
// 4. 12/32 ��१�ࢨ஢��� (��������� �㫥�� ���⮬)
// 5. 44/2 �����䨪��� ������
// 6. 46/1 1-� ���� 䫠��� ᮮ�饭��
// 7. 47/1 2-� ���� 䫠��� ᮮ�饭��
// 8. 48/4 �����䨪��� ᨬ����筮�� ����
// 9  52/48 ��१�ࢨ஢��� (��������� �㫥�� ���⮬)
#define HLENAOUT2  (4+4+4+32+8+16+16+4+4+8) // 100 !

#define LEVBDATAOFF (HLENAOUT2+IDLENA+IDLENB+1) // 143


#define COMM_PARAMS_BYTE 84  // 84
#define GRP3_PARAMS_BYTE 85  // 85/86
#define GRP2_PARAMS_BYTE 47  // 47/48


/* ᮮ�饭�� ����頥��� � ��।� �� �஢�� B �
 * ������� ���⢥ত���� ��᫥ 祣� �ந�室�� ������
 * � ������� ⮣� ����� ����� ��襫 � ���⢥ত����
 * � ��砥 ⠩����  ��室��� ᮮ�饭�� ��ࠢ�����
 * ���짮��⥫�*/
#define MSG_ANSW_STORE_WAIT_SIG 0x00000002
/*�ਧ��� ⮣� �� �� �⢥� �� ��⮬���᪨� ������  */
#define MSG_ANSW_ANSWER         0x00000004
/*�ਧ��� ⮣� �� �� ��� �� ��᪮�쪨� �����ᮢ
 * ⠩���� �������� � ��砫� � ����� �� ��������*/
#define MSG_ANSW_SINGLE_TIMEOUT 0x00000008
#define MSG_DEBUG               0x00000010
/*
#define 0x00000020
#define 0x00000040
#define 0x00000080
*/

#endif // __MSG_CONST_H_

