#ifndef _EDI_TYPES_H_
#define _EDI_TYPES_H_

#include "stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int edi_msg_types_t;
typedef unsigned int edi_msg_flags_t;

typedef enum {
    DefaultEdiCharSet = 0,
    PresetEdiCharSet = 1,
    LoadedEdiCharSet = 2,
}CharSetType;

/* ���-� ��⠭����/��⠢������� ᮮ�饭�� */
typedef struct _EDI_REAL_MES_STRUCT_ EDI_REAL_MES_STRUCT;
typedef struct _EDI_MESSAGES_STRUCT_ EDI_MESSAGES_STRUCT;
typedef struct _EDI_TEMP_MESSAGE_    EDI_TEMP_MESSAGE;

typedef enum EDI_MSG_TYPES_REQ  {
    ERRQUERY=0, QUERY, RESPONSE,
}edi_msg_req;

#define EDINOMES   (edi_msg_types_t) 0

#define MESSAGE_LEN 6  /*����� ᮮ�饭��*/
#define TAG_LEN     3  /*����� ᥣ���� */
#define COMP_LEN    4  /*����� ��������*/

#define EDI_CHSET_STR_LEN       6

#define EDI_CHSET_LEN           4
#define EDI_SENDER_LEN 		35
#define EDI_RECIPIENT_LEN 	35
#define EDI_DATE_LEN            6
#define EDI_TIME_LEN            4
#define EDI_REFERENCE_LEN       14
#define EDI_REF_PASS_LEN        14
#define EDI_ASSOC_LEN           1

#define EDI_ACCREF_LEN          35
#define EDI_MESNUM_LEN          14
#define EDI_VER_NUM_LEN         3
#define EDI_REL_NUM_LEN         3
#define EDI_CONTR_AGENCY_LEN    2
#define EDI_ADDRESING_VER_LEN   4
#define EDI_FSE_ID_LEN          14
#define EDI_ADDRESING_EXT_LEN          14


typedef struct _edi_mes_head_ edi_mes_head;

/*
 udate - ��।����� �� ���譨� �㭪権
 data   - ��।��� ���� �� ��ࠡ��稪� � ��ࠢ�⥫�
 */
typedef int (*fp_edi_proc) (edi_mes_head *pHead, void *udata, void *data, int *err);
typedef int (*fp_edi_send) (edi_mes_head *pHead, void *udata, void *data, int *err);
/*�������� ��� "data" ���-��*/
typedef void (*fp_destuct) (void *pLS);

typedef struct _EDI_MSG_TYPE_
{
    const char *code; /*�������� ᮮ�饭��*/
    edi_msg_types_t type;
    edi_msg_types_t answer_type;
    edi_msg_req query_type;
    fp_edi_proc f_proc;
    fp_edi_send f_send;
    size_t str_sz;
    fp_destuct destruct;
    edi_msg_flags_t flags;

    EDI_TEMP_MESSAGE *pTemp;
} EDI_MSG_TYPE;


#define MAX_END_SEG_LEN 2 /*���ᨬ��쭠� �����  END_SEG/END_SEG1*/
typedef struct { /*����� �ࠢ����� ᨬ�����*/
    char EndSegStr[MAX_END_SEG_LEN+1];
    short EndSegLen;
    char EndComp;
    char EndData;
    char Release;
}Edi_CharSet;

struct _edi_mes_head_
{
    /*�᭮��� ����� �� UNB*/
    char chset[EDI_CHSET_LEN+1];                /*Syntax edentifier*/
    int  syntax_ver;                            /*Syntax version number*/

    char from [EDI_SENDER_LEN+1];      	        /* ���� "��ࠢ�⥫�" */
    char fromAddVer[EDI_ADDRESING_VER_LEN+1];
    char fromAddExt[EDI_ADDRESING_EXT_LEN+1];   /* ���७�� ���� ��ࠢ�⥫� */
    char to   [EDI_RECIPIENT_LEN+1];     	/* ���� "�����⥫�" */
    char toAddVer[EDI_ADDRESING_VER_LEN+1];
    char toAddExt[EDI_ADDRESING_EXT_LEN+1]; /* ���७�� ���� �����⥫� */
    char date [EDI_DATE_LEN +1];   		/* ���� "���" */
    char time [EDI_TIME_LEN +1];   		/* ���� "�६�" */
    char other_ref [EDI_REFERENCE_LEN+1];	/* ���� "�ࠢ����� ��뫪� ������" */
    /* ��� ����� � ��஭� ��ࠢ�⥫� */
    char our_ref   [EDI_REF_PASS_LEN+1]; 	/* ���� "��஫� �����⥫�" */
    /* ��� ����� ⮣�, �� �� �⢥砥� */
    char assoc_code[EDI_ASSOC_LEN +1]; 		/* ���� "⨯ ᮮ�饭��" */

    /*�᭮��� ����� �� UNH*/
    char code	 [MESSAGE_LEN +1];     		/* ���� "⨯ ᮮ�饭��" */
    char acc_ref   [EDI_ACCREF_LEN +1];  	/* ���� "��뫪� ��饣� ����㯠 */
    char unh_number[EDI_MESNUM_LEN +1];		/* 㭨����� ����� �� UNH */
    unsigned  mes_num;
    char ver_num   [EDI_VER_NUM_LEN+1];         /* ����� ���ᨨ ᮮ�饭�� */
    char rel_num   [EDI_REL_NUM_LEN+1];         /* */
    char cntrl_agn [EDI_CONTR_AGENCY_LEN+1];    /* Controlling agency */
    char FseId     [EDI_FSE_ID_LEN+1];

    edi_msg_types_t  msg_type;		      	/* ����� ⨯� ᮮ�饭�� */
    edi_msg_types_t  answer_type;		      	/* ����� ⨯� �⢥⭮�� ᮮ�饭�� */
    edi_msg_req    msg_type_req;		/* �ਧ��� "�����/�⢥�" */
    EDI_MSG_TYPE   *msg_type_str;

    Edi_CharSet    CharSet;
};

typedef struct {
    char name[EDI_CHSET_LEN+1];
    char chset_str[EDI_CHSET_STR_LEN+2];
}edi_loaded_char_sets;

/* Service function: */
/*1) Obr edifact     */
/* Func before proc  */
typedef int (*fp_edi_after_parse_err)(int parse_ret, void *udata, int *err);
typedef int (*fp_edi_before_proc)(edi_mes_head *pHead, void *udata, int *err);
typedef int (*fp_edi_after_proc_err)(edi_mes_head *pHead, int procf_ret, void *udata, int *err);
typedef int (*fp_edi_after_proc)(edi_mes_head *pHead, void *udata, int *err);
typedef int (*fp_edi_before_send)(edi_mes_head *pHead, void *udata, int *err);
/*�८�ࠧ����� ���-�� ᮮ�饭�� � ⥪�� � ��ࠢ��� �� ��*/
typedef int (*fp_edi_after_send )(edi_mes_head *pHead, void *udata, int *err);
typedef int (*fp_edi_after_send_err)(edi_mes_head *pHead, int sendf_ret, void *udata, int *err);

/*
 �� ����� ᮮ�饭�� ���⠥� ��� ��������
 return NULL - not found
 return EDI_MSG_TYPE * - Ok
 */
EDI_MSG_TYPE *GetEdiMsgTypeStrByName(const char *mname);
EDI_MSG_TYPE *GetEdiMsgTypeStrByName_(EDI_MESSAGES_STRUCT *pMesTemp,
                                      const char *mname);
/*
 �� ⨯� ᮮ�饭�� ���⠥� ��� ��������
 return NULL - not found
 return EDI_MSG_TYPE * - Ok
 */
EDI_MSG_TYPE *GetEdiMsgTypeStrByType(edi_msg_types_t type);
EDI_MSG_TYPE *GetEdiMsgTypeStrByType_(EDI_MESSAGES_STRUCT *pMesTemp,
                                      edi_msg_types_t type);


/*
 �� ����� ᮮ�饭�� ���⠥� ��� ��������
 return NULL - not found
 return EDI_MSG_TYPE * - Ok
 */
EDI_MSG_TYPE *GetEdiMsgTypeStrByName(const char *mname);
EDI_MSG_TYPE *GetEdiMsgTypeStrByName_(EDI_MESSAGES_STRUCT *pMesTemp,
                                      const char *mname);

const char * GetEdiMsgNameByType(edi_msg_types_t type);
const char * GetEdiMsgNameByType_(EDI_MESSAGES_STRUCT *pMesTemp, edi_msg_types_t type);

/*
 �� ⨯� ᮮ�饭�� ���⠥� ��� ⨯ �⢥�
    return EDI_MSG_TYPE * - Ok
 */
edi_msg_types_t GetEdiAnswerByType(edi_msg_types_t type);
edi_msg_types_t GetEdiAnswerByType_(EDI_MESSAGES_STRUCT *pMesTemp,
                                    edi_msg_types_t type);

/*
 ��ࠢ�� ᮮ�饭�� ��������� ⨯�
*/
int SendEdiMessage(edi_msg_types_t type, edi_mes_head *pHead,
				   void *udata, void *data, int *err);
int SendEdiMessage_(EDI_MESSAGES_STRUCT *pMesTemp, EDI_REAL_MES_STRUCT *pMesStr,
					edi_msg_types_t type, edi_mes_head *pHead,
					void *udata, void *data, int *err);
int SendEdiMessage__(EDI_MESSAGES_STRUCT *pMesTemp, EDI_REAL_MES_STRUCT *pMesStr,
					 EDI_MSG_TYPE *pEType, edi_mes_head *pHead,
					 void *udata, void *data, int *err);


/*
 ��ࠡ�⪠ ᮮ�饭�� ��������� ⨯�
 */
int ProcEdiMessage(edi_msg_types_t type, edi_mes_head *pHead,
                   void *udata, void *data, int *err);
int ProcEdiMessage_(EDI_MESSAGES_STRUCT *pMesTemp,EDI_REAL_MES_STRUCT *pMesStr,
					edi_msg_types_t type, edi_mes_head *pHead,
					void *udata, void *data, int *err);
int ProcEdiMessage__(EDI_MESSAGES_STRUCT *pMesTemp,EDI_REAL_MES_STRUCT *pMesStr,
					 EDI_MSG_TYPE *pEType, edi_mes_head *pHead,
					 void *udata, void *data, int *err);

int FullObrEdiMessage(const char *edi_tlg_text, edi_mes_head *pHead,
					  void *udata, int *err);
int FullObrEdiMessage_(EDI_MESSAGES_STRUCT *pMesTemp,EDI_REAL_MES_STRUCT *pMesStr,
					   const char *edi_tlg_text, edi_mes_head *pHead,
					   void *udata, int *err);


/*
 ��ࢨ�� �㭪樨 �⠯�� ��ࠡ�⪨
 */
void SetEdiTempServiceFunc(fp_edi_after_parse_err f_ape,
                           fp_edi_before_proc     f_bp,
                           fp_edi_after_proc      f_ap,
                           fp_edi_after_proc_err  f_apre,
                           fp_edi_before_send 	  f_bs,
                           fp_edi_after_send	  f_as,
                           fp_edi_after_send_err  f_ase);

void SetEdiTempServiceFunc_(EDI_MESSAGES_STRUCT *pMesTemp,
							fp_edi_after_parse_err f_ape,
							fp_edi_before_proc     f_bp,
                            fp_edi_after_proc      f_ap,
                            fp_edi_after_proc_err  f_apre,
                            fp_edi_before_send 	  f_bs,
                            fp_edi_after_send	  f_as,
                            fp_edi_after_send_err  f_ase);

#ifdef __cplusplus
}

#endif

#endif /*_EDI_TYPES_H_*/

