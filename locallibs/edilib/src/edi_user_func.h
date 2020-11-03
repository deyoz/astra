#ifndef _EDI_USER_FUNC_H_
#define _EDI_USER_FUNC_H_
#include <stdarg.h>
#include <stdio.h>
#include <memory>

#include "edi_types.h"
#include "edi_logger.h"

#define EDI_MES_OK             0 /*�� ��*/
#define EDI_MES_NOT_FND       -1 /*������ ᮮ�饭�� ��������� � ���� ��� ���� 祣�-� ���*/
#define EDI_MES_ERR           -2 /*���� �訡�� (�ணࠬ���)*/
#define EDI_MES_STRUCT_ERR    -3 /*�訡�� � ������� ᮮ�饭��*/
#define EDI_MES_TYPES_ERR     -4 /*�訡�� ࠡ��� � ⨯���*/

enum EDI_MES_LANG {
	EDI_ENGLISH=0,
	EDI_RUSSIAN=1
};

/**
 * init edilib call back register. Called on first access to GetEdiTemplateMessages
 */
void RegEdilibInit(int(*init_edilib_)(void));

/******************************************************************/
/* ���樨��� �ணࠬ��� ⨯� ᮮ�饭�� � ��⠭�묨 蠡������ */
/* return 0 - Ok                                                  */
/* return -1- Error                                               */
/******************************************************************/
int InitEdiTypes(EDI_MSG_TYPE *types, size_t num);
int InitEdiTypes_(EDI_MESSAGES_STRUCT *pMesTemp,
		  EDI_MSG_TYPE *types, size_t num);


/*******************************************************************/
/* ����㦠�� ����� �ࠢ����� ᨬ�����,                           */
/* �ਢ�뢠� �� � �����⭮�� 蠡����.                            */
/* �᫨ �㭪�� �� �믮�����, ���� �ᯮ�짮�. ᨬ���� �� 㬮�砭. */
/*******************************************************************/
int InitEdiCharSet(const edi_loaded_char_sets *char_set, size_t num);
int InitEdiCharSet_(EDI_MESSAGES_STRUCT *pMesTemp,
                    const edi_loaded_char_sets *char_set, size_t num);

/*******************************************************************/
/* �����頥� �������� � ����஬ �ࠢ�. ᨬ�����                 */
/* return NULL - ��祣� ���					   */
/*******************************************************************/
Edi_CharSet *GetEdiCurCharSet(void);
Edi_CharSet *GetEdiCurCharSet_(EDI_REAL_MES_STRUCT *pMes);

/********************************************************************/
/* �����頥� �������� � ����஬ �ࠢ�. ᨬ����� �� ⥪�� EDIFACT*/
/* return NULL - ��祣� ���					    */
/********************************************************************/
Edi_CharSet *GetEdiCharSetFromText(const char *text, int len);
Edi_CharSet *GetEdiCharSetFromText_(EDI_REAL_MES_STRUCT *pMes,
				    EDI_MESSAGES_STRUCT *pMesTemp,
				    const char *text, int len);

/*
 * �� �������� ᮮ�饭�� ���⠥� ��� ⨯ � �᫠�
 */
edi_msg_types_t GetEdiMsgTypeByName(const char *mname);
edi_msg_types_t GetEdiMsgTypeByName_(EDI_MESSAGES_STRUCT *pMesTemp,
				     const char *mname);

/*
 return 1 = EDIFACT
 return 0 = ᪮॥ �ᥣ� ���
 */
int IsEdifactText(const char *text, int len);
int IsEdifactText_(EDI_MESSAGES_STRUCT *pMes, const char *text, int len);

struct EDI_MESSAGES_STRUCT_Deleter {  void operator()(EDI_MESSAGES_STRUCT* p) const;  };
typedef std::unique_ptr<EDI_MESSAGES_STRUCT, EDI_MESSAGES_STRUCT_Deleter> pEDI_MESSAGES_STRUCT;
/*
 * ���⠢��� 蠡��� ᮮ�饭��
 * return 0 - Ok
 * return -1 - error
 */
pEDI_MESSAGES_STRUCT CreateTemplateMessages_();

/******************************************************************/
/* 1. ���뢠�� �� ORACLE 蠡���� ᮮ�饭��                       */
/* 2. ����஢���� ��⠭��� ������                               */
/* 3. �८�ࠧ������ ��⠭��� ������ � 㤮��� ��� ��ࠡ�⪨ ��� */
/* 4. ������ ���� ����� �� ��                                 */
/* return -1 �ணࠬ��� �訡��                                    */
/* return -2 �訡�� �⥭�� ORACLE                                 */
/* return  0 �� Ok!                                              */
/******************************************************************/
int CreateTemplateMessages();

/*
 * �����頥� 㪠��⥫� �� 蠡��� ᮮ�饭��
 */
EDI_MESSAGES_STRUCT *GetEdiTemplateMessages(void);

/*
 * ���⠢��� 蠡��� ᮮ�饭�� �� 䠩���
 * �� �室 ��।����� ��४��� � 䠩���� ᮮ�饭��
 * return 0 - Ok
 * return -1 - error
 */
int CreateTemplateMessagesFf(const char *directory);

int CreateEdiLocalMesStruct(void);

/*****************************************************************/
/*������ ��஥ ᮮ�饭��, ������ �⥪, ���뢠�� ����� ᮮ��.*/
/*input : *Mes - ᮮ�饭�� � �ଠ� EDIFACT                     */
/*return -1 - �ணࠬ��� �訡��!!!                               */
/*return -2 - �訡�� ᨭ⠪��!!!                               */
/*return -3 - �������⭮� ᮮ�饭��                              */
/*return  1 - �� Ok!                                            */
/*****************************************************************/
int ReadEdiMessage(const char *Mes);
int ReadEdiMessage_(EDI_MESSAGES_STRUCT *pTempMes,
		            EDI_REAL_MES_STRUCT *pstMes, const char *Mes);

/*
 ������ �� ����� �������� ��⠭���� ᮮ�饭��
 */
void DeleteMesIncoming(void);
/*
 ������ �� ����� �������� ᮡ࠭���� ᮮ�饭��
 */
void DeleteMesOutgoing(void);

/******************************************************************/
/*return 1 - �᫨ ������⥪� ���४⭮ 				  */
/*	     ��ࠡ�⠫� ��।��� ᮮ�饭��			  */
/*return 0 - �訡�� ������⥪�					  */
/******************************************************************/
int IsEdilib(void);

int SetEdilib(int Set);

/*************************************************************/
/*�����頥� 㪠��⥫� �� �����䨪��� ᮮ�饭��.	     */
/*************************************************************/
const char *GetEdiMes_(EDI_REAL_MES_STRUCT *pMes);
const char *GetEdiMes(void);
const char *GetEdiMesW(void);
edi_msg_types_t GetEdiMesType_(EDI_REAL_MES_STRUCT *pMes);
edi_msg_types_t GetEdiMesType(void);
edi_msg_types_t GetEdiMesTypeW(void);


/*�����頥� 㪠��⥫� �� ��. ���� ��४���� � ��⠭�� ᮮ�饭���*/
EDI_REAL_MES_STRUCT *GetEdiMesStruct(void);
/*�����頥� 㪠��⥫� �� ��. ���� ��४���� � ᮧ����� ᮮ�饭���*/
EDI_REAL_MES_STRUCT *GetEdiMesStructW(void);

/********************************************************************/
/*��⠭�������� ���. �����. �� 楫������ ᮮ�饭��(���㫥��� pPoint)*/
/*���樠������� pPoint                                             */
/*return -1 - �ணࠬ���� �訡��                                    */
/*return  0 - �� Ok                                                */
/********************************************************************/
int ResetEdiPoint_(EDI_REAL_MES_STRUCT *pM);
int ResetEdiPoint(void);
int ResetEdiPointW_(EDI_REAL_MES_STRUCT *pMes);
int ResetEdiPointW();

/********************************************************************/
/*��窠 ����� ��᫥����� ����� ����� ������ ��������	    */
/*��⠭���������� ��⮬�⮬ �� ���������� ����� � ᮮ�饭��	    */
/*�㭪�� ���뢠�� WrPoint ��⠭��������� �� ���� �. �����    */
/*⥪�饩 ������ ��������					    */
/*return 0 - ok			                                    */
/********************************************************************/
int ResetEdiWrPoint();
int ResetEdiWrPoint_(EDI_REAL_MES_STRUCT *pMes);


/********************************************************************/
/*���࠭�� ���. �����. � �⥪                                      */
/*return  -1 - �ணࠬ���� �訡��                                    */
/*return   0 - �� Ok                                                */
/********************************************************************/
int PushEdiPoint_(EDI_REAL_MES_STRUCT *pM);
int PushEdiPoint(void);
int PushEdiPointW_(EDI_REAL_MES_STRUCT *pMes);
int PushEdiPointW(void);

/********************************************************************/
/*��������� ���. �����. �� �⥪�                                    */
/*return -1 - ��祣� ���������                                      */
/*return  0 - �� Ok                                                */
/********************************************************************/
int PopEdiPoint_(EDI_REAL_MES_STRUCT *pM);
int PopEdiPoint(void);
int PopEdiPointW_(EDI_REAL_MES_STRUCT *pMes);
int PopEdiPointW(void);

/********************************************************************/
/*��������� ���. �����. �� �⥪� �� 㤠��� �. �⥪�                */
/*return -1 - ��祣� ���������                                      */
/*return  0 - �� Ok                                                */
/********************************************************************/
int PopEdiPoint_wd_(EDI_REAL_MES_STRUCT *pM);
int PopEdiPoint_wd(void);
int PopEdiPoint_wdW_(EDI_REAL_MES_STRUCT *pMes);
int PopEdiPoint_wdW(void);

/********************************************************************/
/*������  �⥪                                                    */
/*return  0 - �� Ok                                                */
/********************************************************************/
int ResetStackPoint_(EDI_REAL_MES_STRUCT *pMes);
int ResetStackPoint(void);
int ResetStackPointW_(EDI_REAL_MES_STRUCT *pMes);
int ResetStackPointW(void);

/********************************************************************/
/*���樠������� pFind, ��� ������ , �᫨ ���樠������ 㦥 �뫠  */
/*return -1 - �ணࠬ���� �訡��                                    */
/*return  0 - �� Ok                                                */
/********************************************************************/
int ResetEdiFind_(EDI_REAL_MES_STRUCT *pMes);
int ResetEdiFind(void);
int ResetEdiFindW(void);

/********************************************************************/
/*���࠭�� �������� ��᫥���� ���᪮� ����� ��� �����ᮬ,        */
/*㪠����� � ����⢥ �室���� ��ࠬ��� (ind < MAX_SAVE_FOUND)    */
/*return -1  - ��㤠�                                              */
/*return  0  - Ok                                                   */
/********************************************************************/
int SaveEdiFound(int ind);
int SaveEdiFoundW(int ind);
int SaveEdiFound_(EDI_REAL_MES_STRUCT *pMes, int ind);

/********************************************************************/
/*����⠭�������� ��࠭���� �����.                               */
/*��������: ���뢠���� ���. ��������,                             */
/*��� ���ன �� ����� ��࠭﫨��                                */
/*return -1   - ��㤠�                                             */
/*return  0   - �� Ok                                              */
/********************************************************************/
int LoadEdiFound_(EDI_REAL_MES_STRUCT *pMes, int ind);
int LoadEdiFound(int ind);
int LoadEdiFoundW(int ind);

/********************************************************************/
/*������ ��࠭���� � SaveEdiFound �����                         */
/********************************************************************/
void FreeEdiFound_(EDI_REAL_MES_STRUCT *pM, int ind);
void FreeEdiFound(int ind);
void FreeEdiFoundW(int ind);

/********************************************************************/
/*������ �� ��࠭���� � SaveEdiFound �����                     */
/********************************************************************/
void ResetEdiFound_(EDI_REAL_MES_STRUCT  *pM);
void ResetEdiFound(void);
void ResetEdiFoundW(void);

/************************************************************/
/*��室�� ����� ������ �� ��� �����                       */
/*Num - �᫨ ���� �ᯥ�� - ���-�� ��������� �. ������   */
/*return -1 - �ணࠬ���� �訡��                            */
/*return  0 - ��祣� �� ��諨 (Num = 0)                     */
/*return  1 - ��諨 Num �. ������ � ������� ������        */
/************************************************************/
int GetDataByName_(EDI_REAL_MES_STRUCT *pMes, int Data, int *Num);
int GetDataByName(int Data, int *Num);

int GetDataByNum_(EDI_REAL_MES_STRUCT *pMes, int Data, int Seq, int *Num);
int GetDataByNum(int Data, int Seq, int *Num);

/**********************************************************/
/*�����頥� 㪠��⥫� �� �������� ����� �� ������      */
/*return NULL - ��㤠�                                   */
/*return char* - 㪠��⥫� �� �����                      */
/**********************************************************/
const char *GetFoundDataByNum(int Num);
const char *GetFoundDataByNum_(EDI_REAL_MES_STRUCT *pMes, int Num);

/**********************************************************/
/*�ந������ ���� ����� ������ (�᫨ ��� �� �� �뫮) */
/*�����頥� 㪠��⥫� �� ����� ������                  */
/*��� 㪠��⥫� �� ������ ��ப� � ��砥 ���������筮�� */
/*Input : data - �. ������                               */
/**********************************************************/
const char *GetDBNz(int data);
const char *GetDBNz_(EDI_REAL_MES_STRUCT *pMes, int data);

/**********************************************************/
/*�ந������ ���� ����� ������ (�᫨ ��� �� �� �뫮) */
/*�����頥� 㪠��⥫� �� ����� ������                  */
/*��� 㪠��⥫� �� NULL � ��砥 ���������筮��          */
/*Input : data - �. ������                               */
/**********************************************************/
const char *GetDBN(int data);
const char *GetDBN_(EDI_REAL_MES_STRUCT *pMes, int data);

/**********************************************************/
/*�ந������ ���� ����� ������ (�᫨ ��� �� �� �뫮) */
/*�����頥� 㪠��⥫� �� ����� ������ �� ����� � ������*/
/*��� 㪠��⥫� �� NULL � ��砥 ��㤠�                  */
/*Input : data - �. ������                               */
/**********************************************************/
const char *GetDBNum(int data, int Num_d);
const char *GetDBNum_(EDI_REAL_MES_STRUCT *pMes, int data, int Num_d);
const char *GetDBNumSeq_(EDI_REAL_MES_STRUCT *pMes, int data, int Num_d, int NumSeq);

/**********************************************************/
/*�ந������ ���� ����� ������ (�᫨ ��� �� �� �뫮) */
/*�����頥� 㪠��⥫� �� ����� ������ �� ����� � ������*/
/*��� 㪠��⥫� �� ������ ��ப� � ��砥 ��㤠�         */
/*Input : data - �. ������                               */
/**********************************************************/
const char *GetDBNumz(int data, int Num_d);
const char *GetDBNumz_(EDI_REAL_MES_STRUCT *pMes, int data, int Num_d);

/************************************************************/
/*��室�� ����� ������ �� ��� ������� �����               */
/*Num - �᫨ ���� �ᯥ�� - ���-�� ��������� �. ������   */
/*�ணࠬ���� �訡�� ��� ���������筮���:		    */
/*if flag return  NULL					    */
/*     else return  '/0'				    */
/*return  char * - �����⥫� �� �. ������ � ������� ������*/
/************************************************************/
const char *GetDBFN(short SegGr , int SegGrNum,
			const char *Segm, int SegmNum,
			const char *Comp, int CompNum,
			int Data  , int DataNum);

const char *GetDBFN_(EDI_REAL_MES_STRUCT *pMes,
		     short SegGr , int SegGrNum,
		     const char *Segm, int SegmNum,
		     const char *Comp, int CompNum,
		     int Data  , int DataNum);

const char *GetDBFNSeq_(EDI_REAL_MES_STRUCT *pMes,
		     short SegGr , int SegGrNum,
		     const char *Segm, int SegmNum,
		     const char *Comp, int CompNum,
		     int Data  , int DataNum, int NumSeq);

const char *GetDBFNz(short SegGr , int SegGrNum,
                     const char *Segm, int SegmNum,
                     const char *Comp, int CompNum,
		     int Data  , int DataNum );

const char *GetDBFNz_(EDI_REAL_MES_STRUCT *pMes,
                      short SegGr , int SegGrNum,
                      const char *Segm, int SegmNum,
                      const char *Comp, int CompNum,
		      int Data  , int DataNum);

const char *GetDBFN__(EDI_REAL_MES_STRUCT *pMes,
                      short SegGr , int SegGrNum,
                      const char *Segm, int SegmNum,
                      const char *Comp, int CompNum,
                      int Data  , int DataNum , int NumSeq, int flag);
/************************************************************/
/*��室�� ����� ������ �� ��� ������� ����� ��� 㪠�����  */
/*����� ᥣ���⭮� ��㯯�				    */
/*Num - �᫨ ���� �ᯥ�� - ���-�� ��������� �. ������   */
/*�ணࠬ���� �訡�� ��� ���������筮���:		    */
/* return  '/0'						    */
/*return  char * - �����⥫� �� �. ������ � ������� ������*/
/************************************************************/
#define GetDBFNSegGrZ(Segm,SegmNum,Comp,CompNum,Data,DataNum) \
	GetDBFNz(0,0,(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum))

/************************************************************/
/*��室�� ����� ������ �� ��� ������� ����� ��� 㪠�����  */
/*����� ᥣ���⭮� ��㯯�				    */
/*Num - �᫨ ���� �ᯥ�� - ���-�� ��������� �. ������   */
/*�ணࠬ���� �訡�� ��� ���������筮���:		    */
/* return  NULL					    	    */
/*return  char * - �����⥫� �� �. ������ � ������� ������*/
/************************************************************/
#define GetDBFNSegGr(Segm,SegmNum,Comp,CompNum,Data,DataNum) \
	GetDBFN(0,0,(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum))

/************************************************************/
/*��室�� ����� ������ �� ��� ������� ����� �� ⥪. ᥣ�  */
/*Num - �᫨ ���� �ᯥ�� - ���-�� ��������� �. ������   */
/*�ணࠬ���� �訡�� ��� ���������筮���:		    */
/* return  '/0'						    */
/*return  char * - �����⥫� �� �. ������ � ������� ������*/
/************************************************************/
#define GetDBFNSegZ(Comp,CompNum,Data,DataNum) \
	GetDBFNz(0,0,NULL,0,(Comp),(CompNum),(Data),(DataNum))

/************************************************************/
/*��室�� ����� ������ �� ��� ������� ����� �� ⥪. ᥣ�  */
/*Num - �᫨ ���� �ᯥ�� - ���-�� ��������� �. ������   */
/*�ணࠬ���� �訡�� ��� ���������筮���:		    */
/* return  NULL						    */
/*return  char * - �����⥫� �� �. ������ � ������� ������*/
/************************************************************/
#define GetDBFNSeg(Comp,CompNum,Data,DataNum) \
	GetDBFN(0,0,NULL,0,(Comp),(CompNum),(Data),(DataNum))

/************************************************************/
/*��室�� ����� ������ �� ��� ������� ����� �� ⥪. ����  */
/*Num - �᫨ ���� �ᯥ�� - ���-�� ��������� �. ������   */
/*�ணࠬ���� �訡�� ��� ���������筮���:		    */
/* return  '/0'						    */
/*return  char * - �����⥫� �� �. ������ � ������� ������*/
/************************************************************/
#define GetDBFNCompZ(Data,DataNum) \
	GetDBFNz(0,0,NULL,0,NULL,0,(Data),(DataNum))

/************************************************************/
/*��室�� ����� ������ �� ��� ������� ����� �� ⥪. ����  */
/*Num - �᫨ ���� �ᯥ�� - ���-�� ��������� �. ������   */
/*�ணࠬ���� �訡�� ��� ���������筮���:		    */
/* return  NULL						    */
/*return  char * - �����⥫� �� �. ������ � ������� ������*/
/************************************************************/
#define GetDBFNComp(Data,DataNum) \
	GetDBFN(0,0,NULL,0,NULL,0,(Data),(DataNum))

/*******⮦� ᠬ�� � ������ன******/
#define GetDBFNSegGrZ_(pMes, Segm,SegmNum,Comp,CompNum,Data,DataNum) \
	GetDBFNz_((pMes),0,0,(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum))

#define GetDBFNSegGr_(pMes,Segm,SegmNum,Comp,CompNum,Data,DataNum) \
	GetDBFN_((pMes),0,0,(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum))

#define GetDBFNSegZ_(pMes,Comp,CompNum,Data,DataNum) \
	GetDBFNz_((pMes),0,0,NULL,0,(Comp),(CompNum),(Data),(DataNum))

#define GetDBFNSeg_(pMes,Comp,CompNum,Data,DataNum) \
	GetDBFN_((pMes),0,0,NULL,0,(Comp),(CompNum),(Data),(DataNum))

#define GetDBFNCompZ_(pMes,Data,DataNum) \
	GetDBFNz_((pMes),0,0,NULL,0,NULL,0,(Data),(DataNum))

#define GetDBFNComp_(pMes,Data,DataNum) \
	GetDBFN_((pMes),0,0,NULL,0,NULL,0,(Data),(DataNum))


/********************************************************************/
/*      a) �᫨ ���� �� ���������  ����.  㦥 �ந��������,        */
/*         ��࠭�稢��� �� ���. ��������                           */
/*         return 1 - Ok                                            */
/*      b) ���᪠ �� �� �뫮.                                      */
/*         �ந������ ����;                                        */
/*         �믮��塞 �㭪� �);                                      */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  0 - ���  ᥣ�. � ������� ���浪��� ����஬             */
/* return  1 - �� Ok                                               */
/********************************************************************/
int SetEdiPointToComposite(const char *Comp, int Num);
int SetEdiPointToCompositeW(const char *Comp, int Num);
int SetEdiPointToComposite_(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int Num);
int SetEdiPointToCompositeW_(EDI_REAL_MES_STRUCT *pMes, const char *Comp, int Num);

/********************************************************************/
/*      a) �᫨ ���� �� ���������  ᥣ�.  㦥 �ந��������,        */
/*         ��࠭�稢��� �� ���. ��������                           */
/*         return 1 - Ok                                            */
/*      b) ���᪠ �� �� �뫮.                                      */
/*         �ந������ ����;                                        */
/*         �믮��塞 �㭪� �);                                      */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  0 - ���  ᥣ�. � ������� ���浪��� ����஬             */
/* return  1 - �� Ok                                               */
/********************************************************************/
int SetEdiPointToSegment(const char *Segm, int Num);
int SetEdiPointToSegmentW(const char *Segm, int Num);
int SetEdiPointToSegment_(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int Num);
int SetEdiPointToSegmentW_(EDI_REAL_MES_STRUCT *pMes, const char *Segm, int Num);

/********************************************************************/
/*      a) �᫨ ���� �� �������� ��. ᥣ�.  㦥 �ந��������,      */
/*          ��࠭�稢��� ���. �������� ��㯯��.� ������� ���. ���.*/
/*         return 1 - Ok                                            */
/*      b) ���᪠ �� �� �뫮.                                      */
/*         �ந������ ����;                                        */
/*         �믮��塞 �㭪� �);                                      */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  0 - ��� ��. ᥣ�. � ������� ���浪��� ����஬          */
/* return  1 - �� Ok                                               */
/********************************************************************/
int SetEdiPointToSegGr(short SegGr, int Num);
int SetEdiPointToSegGr_(EDI_REAL_MES_STRUCT *pMes, short SegGr, int Num);
int SetEdiPointToSegGrW(short SegGr, int Num);
int SetEdiPointToSegGrW_(EDI_REAL_MES_STRUCT *pMes, short SegGr, int Num);

/********************************************************************/
/* �ந������ ����, �᫨ ���᪠ �� ���뫮                         */
/* �����頥� ���-�� ᥣ������ ��㯯 � ᮮ�饭��                   */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  N - ���-��                                               */
/********************************************************************/
int GetNumSegGr(short SegGr);
int GetNumSegGr_(EDI_REAL_MES_STRUCT *pMes, short SegGr);
int GetNumSegGrW(short SegGr);

/********************************************************************/
/* �ந������ ����, �᫨ ���᪠ �� ���뫮                         */
/* �����頥� ���-�� ᥣ���⮢ � ᮮ�饭�� ���뢠� ���. ��������  */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  N - ���-��                                               */
/********************************************************************/
int GetNumSegment(const char *Segm);
int GetNumSegmentW(const char *Segm);
int GetNumSegment_(EDI_REAL_MES_STRUCT *pMes, const char *Segm);


/********************************************************************/
/* �ந������ ����, �᫨ ���᪠ �� ���뫮                         */
/* �����頥� ���-�� ᥣ���⮢ � ᮮ�饭�� ���뢠� ���. ��������  */
/* return -1 - �ணࠬ���� �訡��                                   */
/* return  N - ���-��                                               */
/********************************************************************/
int GetNumComposite(const char *Comp);
int GetNumCompositeW(const char *Comp);
int GetNumComposite_(EDI_REAL_MES_STRUCT *pMes, const char *Comp);

/*
 * �� ⨯� ᮮ�饭�� ���⠥� ��� ⨯ (����� / �⢥�)
 */
edi_msg_req GetEdiMsgQTypeByType(edi_msg_types_t type);

/* �������� ������ EDIFACT ᮮ�饭�� */
int CreateNewEdiMes(edi_msg_types_t type, const char *chset);
/* �������� ������ EDIFACT ᮮ�饭�� */
int CreateNewEdiMes_(EDI_MESSAGES_STRUCT *pTempMes,
		     EDI_REAL_MES_STRUCT *pMes,
		     edi_msg_types_t type, const char *chset);


/*
 �८�ࠧ�� ��-�� ᮮ�饭�� � ⥪�� EDIFACT
 �뤥��� ������
 return len >= 0 -Ok
 return < 0 - Error
 */
int WriteEdiMessage_(EDI_REAL_MES_STRUCT *pMes, char **buff);
int WriteEdiMessage(char **buff);

/*
 �८�ࠧ�� ��-�� ᮮ�饭�� � ⥪�� EDIFACT
 �ᯮ���� ��।���� ����
 return len >= 0 -Ok
 return < 0 - Error ��� �����쪨� ����
 */
int WriteEdiMessageStat_(EDI_REAL_MES_STRUCT *pMes, char *buff, int bsize);
int WriteEdiMessageStat(char *buff, int bsize);

/*
 ���� ᥣ���� �� ��ப� �� �ଠ��
 d1+d2:d3+++d4++...
 */
int SetEdiFullSegmentF(const char *segm, int num, const char *format, ...);
/*
 ���� ᥣ���� �� ��ப�
 d1+d2:d3+++d4++...
 */
int SetEdiFullSegment(const char *segm, int num, const char *seg_str);
int SetEdiFullSegment_(EDI_REAL_MES_STRUCT *pM, const char *segm,
		       int num, const char *seg_str);
int SetEdiFullCurSegment(EDI_REAL_MES_STRUCT *pM, const char *seg_str);

/*
 ���� �������� �� ��ப� �� �ଠ��
 d1::d2:d3::...
 */
int SetEdiFullCompositeF(const char *comp, int num, const char *format, ...);
/*
 ���� �������� �� ��ப�
 d1::d2:d3::...
 */
int SetEdiFullComposite(const char *comp, int num, const char *comp_str);
int SetEdiFullComposite_(EDI_REAL_MES_STRUCT *pM, const char *comp,
			 int num, const char *comp_str);
int SetEdiFullCurComposite(EDI_REAL_MES_STRUCT *pM, const char *comp_str);


int SetEdiSegGr_(EDI_REAL_MES_STRUCT *pM, int SegGr, int num);
int SetEdiSegGr(int SegGr, int num);

int SetEdiSegment_(EDI_REAL_MES_STRUCT *pM, const char *segm, int num);
int SetEdiSegment(const char *segm, int num);

int SetEdiComposite_(EDI_REAL_MES_STRUCT *pM, const char *comp, int num);
int SetEdiComposite(const char *comp, int num);

int SetEdiDataElem(int dataNum, const char *data);
int SetEdiDataElemArr(int dataNum, int num, const char *data);
int SetEdiDataElemLen(int dataNum, const char *data, int len);
int SetEdiDataElemArrLen(int dataNum, int num, const char *data, int len);
int SetEdiDataElemF(int dataNum, const char *format, ...);
int SetEdiDataElemArrF(int dataNum, int num, const char *format, ...);
int SetEdiDataElem_(EDI_REAL_MES_STRUCT *pM, int data, int num,
		    const char *dataStr, int len);
int SetEdiDataElem__(EDI_REAL_MES_STRUCT *pM, int data, int numseq, int num,
            const char *dataStr, int len);

/*
*������� ����� ������ �� ��� ������� �����
*set != 0 - ᮧ������ ����
*�ணࠬ���� �訡�� ��� ���������筮���:
*return -1 - error
*return  0 - Ok
*/
int SetDBFNF(short SegGr , int SegGrNum,
	     const char *Segm, int SegmNum,
	     const char *Comp, int CompNum,
	     int Data  , int DataNum, int set,
	     const char *format, ...);

/*
*������� ����� ������ �� ��� ������� �����
*set != 0 - ᮧ������ ����
*�ணࠬ���� �訡�� ��� ���������筮���:
*return -1 - error
*return  0 - Ok
*/
int SetDBFN_(short SegGr , int SegGrNum,
	     const char *Segm, int SegmNum,
	     const char *Comp, int CompNum,
	     int Data  , int DataNum, int set,
	     const char *dataStr, int dataLen);

/*�� ᮧ������ ����*/
#define SetDBFN(SegGr, SegGrNum, Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((SegGr),(SegGrNum),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), strlen(dataStr))
/*��������� ����*/
#define SetDBFNs(SegGr, SegGrNum, Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((SegGr),(SegGrNum),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), strlen(dataStr))

/*�� ᮧ������ ����, ��।����� ����� ������*/
#define SetDBFNl(SegGr, SegGrNum, Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr, dataLen) \
    SetDBFN_((SegGr),(SegGrNum),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), (dataLen))
/*��������� ����, ��।����� ����� ������*/
#define SetDBFNls(SegGr, SegGrNum, Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((SegGr),(SegGrNum),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), (dataLen))


/*����� ᥣ���⭮� ��㯯� */
#define SetDBFNSegGr(Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), strlen(dataStr))

#define SetDBFNSegGrs(Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), strlen(dataStr))

#define SetDBFNSegGrl(Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr, dataLen) \
    SetDBFN_((0),(0),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), (dataLen))

#define SetDBFNSegGrls(Segm, SegmNum, Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(Segm),(SegmNum),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), (dataLen))

/*����� ᥣ���� */
#define SetDBFNSeg(Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(NULL),(0),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), strlen(dataStr))

#define SetDBFNSegs(Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(NULL),(0),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), strlen(dataStr))

#define SetDBFNSegl(Comp, CompNum, Data, DataNum, dataStr, dataLen) \
    SetDBFN_((0),(0),(NULL),(0),(Comp),(CompNum),(Data),(DataNum), 0, (dataStr), (dataLen))

#define SetDBFNSegls(Comp, CompNum, Data, DataNum, dataStr) \
    SetDBFN_((0),(0),(NULL),(0),(Comp),(CompNum),(Data),(DataNum), 1, (dataStr), (dataLen))


void PrintRealEdiMesW(FILE *fl);
void PrintRealEdiMes (FILE *fl);
void PrintRealEdiMes_(EDI_REAL_MES_STRUCT *pIdeMes, FILE *fl);

const char *EdiErrGetString();
const char *EdiErrGetString_(EDI_REAL_MES_STRUCT *pMes);

void SetEdiErrLang(int lang);
void SetEdiErrLang_(EDI_REAL_MES_STRUCT *pMes, int lang);

const char *EdiErrGetSegName(EDI_REAL_MES_STRUCT *pMes);
int EdiErrGetSegNum(void);
int EdiErrGetSegNum_(EDI_REAL_MES_STRUCT *pMes);
int EdiErrGetCompNum(void);
int EdiErrGetCompNum_(EDI_REAL_MES_STRUCT *pMes);
int EdiErrGetDataNum(void);
int EdiErrGetDataNum_(EDI_REAL_MES_STRUCT *pMes);

int isEdiError(void);
int isEdiError_(EDI_REAL_MES_STRUCT *pMes);

int GetEdiErrNum(void);
int GetEdiErrNum_(EDI_REAL_MES_STRUCT *pMes);

/*const char * get_edi_msg_by_sir(int data_elem, int sir_msg);*/


/*��⠭�������� ᯥ�. ᨬ���� (�� 㬮�砭�� ��� ��⠢�塞��� ᮮ�饭��)*/
int SetEdiCharSet(CharSetType type);
int SetEdiCharSet_(EDI_REAL_MES_STRUCT *pMes, CharSetType type);
int SetCustomCharSet(const char *s);
/*
 For our char set s = ":+,? \"\n"
 comma or full stop
 */
int SetCustomCharSet_(EDI_REAL_MES_STRUCT *pMes, const char *s);


#if 0
/*
 * �� ⨯� ᮮ�饭�� ���⠥� ��� ⨯ (����� / �⢥�)
 */
edi_msg_req GetEdiMsgQTypeByType(edi_msg_types_t type);

#endif
/*
 * �� �������� ᮮ�饭�� ���⠥� ��� ⨯ � �᫠�
 */
int GetEdiMsgTypeByType(edi_msg_types_t type, edi_mes_head *pHead);
int GetEdiMsgTypeByType_(EDI_MESSAGES_STRUCT *pMesTemp,
			 edi_msg_types_t type, edi_mes_head *pHead);

int GetEdiMsgTypeByCode(const char *code, edi_mes_head *pHead);
int GetEdiMsgTypeByCode_(EDI_MESSAGES_STRUCT *pMesTemp,
			 const char *code, edi_mes_head *pHead);

/*
 ������ ᮮ�饭�� � ���� �� 䠩��� ��४�ਨ
 return EDI_MES_OK - ok
 return other - error
 */
int insert_to_ora_from_dir(const char *dir, const char *, const char *);

int insert_to_ora_from_dir_cur(const char *dir);

/*
 ����� 蠡����
 */
void DeleteAllEdiMessages( EDI_MESSAGES_STRUCT **ppEdiMes );

int CreateEdiMesStruct(EDI_REAL_MES_STRUCT **pMes);

/****************************************************************************/
/*���뢠�� ᮮ�饭�� � ��                                                  */
/*Mes -㪠��⥫� �� ��ப�, ᮤ�ঠ��� ������ ᮮ�饭��                     */
/*return EDI_MES_NOT_FND    -�������⭮� ᮮ�饭��                          */
/*return EDI_MES_ERR        -�訡��                                         */
/*return EDI_MES_OK         -���뢠��� ��諮 �ᯥ譮                      */
/*return EDI_MES_STRUCT_ERR -�訡�� � ������� ᮮ�饭��                   */
/****************************************************************************/
int ReadEdiMes(const char *Mes, EDI_MESSAGES_STRUCT *pMes,
                          EDI_REAL_MES_STRUCT *pCurMes);

/*
 �� ���뢠� ᮮ�饭�� 楫����, ��६ ⮫쪮 UNB,UNH
 return EDI_MES_OK
 return EDI_MES_STRUCT_ERR
 */
int FillEdiHeadStr(EDI_REAL_MES_STRUCT *pCurMes, EDI_MESSAGES_STRUCT *pMes,
		    const char *Mes);

int create_edi_empty_files(const char *dir);

void PrintAllEdiTemplateMessages(FILE *fl);

void PrintAllEdiMessages( EDI_MESSAGES_STRUCT *pIdeMes, FILE *fl);

/******************************************************/
/*        ��頥� ���� ������ ᮮ�饭��               */
/* return 0 - SQL ERROR                               */
/* return 1 - Ok                                      */
/******************************************************/
int DeleteDBMesseges();


/*
 ��४������ ᮮ�饭�� EDIFACT �� ��室���� ����� �� ᨢ���� � �������
 ��ࠬ����:
 edi_text - ��室�� ⥪��
 len - ������
 for_coding - ��� ����� �� ᨢ���� (���� IATA)
 dest - ���� �����⥫�
 dest_size - ࠧ��� ���� �����⥫�
 */
int ChangeEdiCharSetStat(const char *edi_text, int len,
			 const char *for_coding, char *dest, size_t dest_size);

/*
 �������� ������!
 ��४������ ᮮ�饭�� EDIFACT �� ��室���� ����� �� ᨢ���� � �������
 ��ࠬ����:
 edi_text - ��室�� ⥪��
 len - ������
 for_coding - ��� ����� �� ᨢ���� (���� IATA)
 dest - ���� �����⥫�
 dest_len - ����� ����� ��ப�
 */
int ChangeEdiCharSet(const char *edi_text, int len,
		     const char *for_coding, char **dest, size_t *dest_len);

int maskSpecialChars_capp(const Edi_CharSet *Chars, const char *instr, char **out);

#endif /*_EDI_USER_FUNC_H_*/
