#ifndef _EDI_SESS_H_
#define _EDI_SESS_H_

#include "edi_types.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
 ��⠥� ��������� EDIFACT ᮮ�饭��
 UNB/UNH
 return EDI_MES_OK  - Ok
 return EDI_MES_ERR - Error
 */
int ReadEdiContext(const char *Mes, edi_mes_head *pHead);

/*
 ��⠥� ��������� EDIFACT ᮮ�饭��, �� �� �⠥� ⨯�
 ��, ���ਬ��, ����� �� ����!
 UNB/UNH
 return EDI_MES_OK  - Ok
 return EDI_MES_ERR - Error
 */
int ReadEdiContextNoTypes(const char *Mes, edi_mes_head *pHead);


int ReadEdiContext_(const char *Mes, edi_mes_head *pHead,
		    EDI_MESSAGES_STRUCT *pTempMes,
                    EDI_REAL_MES_STRUCT *pCurMes,
		    short read_types);


/******************************************************************/
/*�������� ���ਡ��� ᮮ�饭�� 				  */
/*	return  0 - Ok 						  */
/*	return -1 - �ணࠬ���� �訡��				  */
/*      return  1 - ����� ����                                 */
/******************************************************************/
int SetMesHead(edi_mes_head *pHead, short read_types);

int SetMesHead_(edi_mes_head *pHead, EDI_REAL_MES_STRUCT *pMes, short read_types);

/*
 ������� �������� ������ ᮮ�饭�� mes_type
 �������� ��ࢮ���. ����묨 �� pHead(�� UNH/UNB)
 return  0 - Ok
 return  <0 - error
 */
int CreateMesByHead(edi_mes_head *pHead);

int CreateAnswerByHead(edi_mes_head *pHead);

int initEdiTime(edi_mes_head *pHead);

#ifdef __cplusplus
}
#endif

#endif /* _EDI_SESS_H_ */
