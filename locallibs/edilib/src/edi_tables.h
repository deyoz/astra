#ifndef __EDI_TABLES_H__
#define __EDI_TABLES_H__
#include "edi_all.h"
#include "edilib/edi_user_func.h"

/*typedef struct _EDI_MESSAGES_STRUCT_      EDI_MESSAGES_STRUCT;*/
/*typedef struct _EDI_TEMP_MESSAGE_         EDI_TEMP_MESSAGE;*/
typedef struct _TAG_STRUCT_               TAG_STRUCT;
typedef struct _COMPOSITE_STRUCT_         COMPOSITE_STRUCT;
typedef struct _DATA_ELEM_STRUCT_         DATA_ELEM_STRUCT;

/*typedef struct _EDI_REAL_MES_STRUCT_      EDI_REAL_MES_STRUCT;*/
typedef struct _TAG_REAL_STRUCT_          TAG_REAL_STRUCT;
typedef struct _COMP_REAL_STRUCT_         COMP_REAL_STRUCT;
typedef struct _DATA_REAL_STRUCT_         DATA_REAL_STRUCT;

typedef struct _EDI_TABLES_STRUCT_        EDI_TABLES_STRUCT;
typedef struct _MESSAGE_TABLE_STRUCT_     MESSAGE_TABLE_STRUCT;
typedef struct _MES_STRUCT_TABLE_STRUCT_  MES_STRUCT_TABLE_STRUCT;
typedef struct _SEG_STRUCT_TABLE_STRUCT_  SEG_STRUCT_TABLE_STRUCT;
typedef struct _COMP_STRUCT_TABLE_STRUCT_ COMP_STRUCT_TABLE_STRUCT;
typedef struct _DATA_ELEM_TABLE_STRUCT_   DATA_ELEM_TABLE_STRUCT;
typedef struct _SEGMENT_TABLE_STRUCT_     SEGMENT_TABLE_STRUCT;
typedef struct _COMPOSITE_TABLE_STRUCT_   COMPOSITE_TABLE_STRUCT;

/*-------------------------------------------------------*/
/****************************************************************************/
/*-------------------------------------------------------*/

typedef struct {
    char code[MESSAGE_LEN+1];  /*�������� ᮮ�饭��*/
    edi_msg_types_t type;
}EDI_MESSAGE_TYPE;

/*-------------------------------------------------------*/
struct _EDI_TEMP_MESSAGE_ /*����뢠�� ᮮ�饭��*/
{
    EDI_MESSAGE_TYPE Type;        /* ��� ᮮ�饭�� */
    char Text[256+1];             /* ���ᠭ�� */
    TAG_STRUCT *Tag;              /* �����⥫� �� ᥣ���� */
    EDI_TEMP_MESSAGE *Next;       /* �����⥫� �� ᫥��騩 �. */
};

struct _EDI_MESSAGES_STRUCT_
{
    EDI_TEMP_MESSAGE *pTemps;

    EDI_MSG_TYPE *HBNTypes; /*Hash By Name*/
    EDI_MSG_TYPE *HBTTypes; /*Hash By Type*/
    size_t tnum;

    fp_edi_after_parse_err	f_after_parse_err;
    fp_edi_before_proc		f_before_proc;
    fp_edi_after_proc_err       f_after_proc_err;
    fp_edi_after_proc		f_after_proc;
    fp_edi_before_send          f_before_send;
    fp_edi_after_send 		f_after_send;
    fp_edi_after_send_err       f_after_send_err;

    edi_loaded_char_sets *char_set_arr;
    size_t chsnum;
};

/*-------------------------------------------------------*/
struct _TAG_STRUCT_ /*����뢠�� ᥣ����(��㯯� ᥣ���⮢)*/
{
    char TagName[TAG_LEN+1];/*��� ᥣ����('\0' �᫨ ����뢠���� ��㯯� ᥣ���⮢)*/
    char Text[256+1];       /*���ᠭ�� ᥣ����(��㯯� ᥣ.)*/
    int Pos;                /*������ ᥣ����(��㯯� ᥣ.) � ᮮ�饭��*/
    short Num;              /*����� ᥣ���⭮� ��㯯� (0 - �᫨ ᥣ����)*/
    char S;                 /*�ਧ��� ��易⥫쭮�� ᥣ����(��㯯� ᥣ.)*/
    int MaxR;              /*���ᨬ��쭮� ���-�� ����७�� ᥣ����(ᥣ���⭮� ��㯯�)*/
    TAG_STRUCT *NextTag;    /*�����⥫� �� ᫥��騩 ᥣ����(ᥣ. ��.)*/
    TAG_STRUCT *GrStr  ;    /*�����⥫� �� ��⠢���騥 ��㯯� ᥣ���⮢(NULL - �᫨ ᥣ����!)*/
    COMPOSITE_STRUCT *Comp; /*�����⥫� �� ��������, �᫨ ��ᬠ�ਢ����� ᥣ����*/
};
/*-------------------------------------------------------*/
struct _COMPOSITE_STRUCT_ /*����뢠�� ��������*/
{
    char Composite[COMP_LEN+1]; /*��� �������� ('\0' �᫨ ��. ������ ��� ��������)*/
    char Text[256+1];           /*���ᠭ�� (�.���� NULL)*/
    int Pos;                    /*������*/
    char S;                     /*�ਧ��� ��易⥫쭮�� ��������*/
    int MaxR;                  /*���ᨬ��쭮� ���-�� ����७�� ��������*/
    int Num;    	      	     /*����� � ᥣ����, �᫨ ��������� �� � ��㯯� */
    /*  ��. edilib.txt �ਬ�砭��1 */
    COMPOSITE_STRUCT *Next;     /*�����⥫� �� ᫥��騩 ��������*/
    DATA_ELEM_STRUCT *DataElem; /*�����⥫� �� ����� ������*/
};
/*-------------------------------------------------------*/
struct _DATA_ELEM_STRUCT_ /*����뢠�� ����� ������*/
{
    int DataElem;           /*�������� ����� ������*/
    char Text[256+1];       /*���ᠭ�� ����� ������*/
    char S;                 /*�ਧ��� ����室����� �. ������*/
    int  Pos;               /*������ �. ������ � ��������*/
    char Format;            /*��ଠ� ������ */
    /*!!!!!*/               /*(A - Alpha-Numeric, N - Numeric, a - Alphabetic)*/
    int MaxField;           /*���ᨬ���� ࠧ��� ����� ������*/
    int MinField;           /*���������  ࠧ��� ����� ������*/
    int MaxR;
    int Num; 		 /*����� � ��������, ᥣ����, �᫨ ��������� �� � ��㯯� */
    /*  ��. edilib.txt �ਬ�砭��1 */
    DATA_ELEM_STRUCT *Next; /*�����⥫� �� ᫥�. ����� ������*/
};
/*-------------------------------------------------------*/
/****************************************************************************/
/*-------------------------------------------------------*/
typedef struct {
    char *mes;
    int size;
    int len;
    unsigned index; /*������ � ��ப� ᮮ�饭�� �� ���뢠���*/
}MesEdiStruct;

typedef struct _Edi_Error_
{
    unsigned ErrNum;
    short ErrLang;

    int SegGr;
    int SegGr_num;

    char Segment[TAG_LEN+1];
    int  Segm_num;

    char Composite[COMP_LEN+1];
    int  Comp_num;

    int DataElem;
    int DataElemNum;

    /* Counter */
    int SegmCount;
    int CompCount;
    int DataCount;
}Edi_Error;

typedef struct _Edi_Point_ /*��࠭�祭�� ������ ��������*/
{
    TAG_REAL_STRUCT     *pGrStr;/*���. ���. ���. ��㯯�� ᥣ���⮢*/
    TAG_REAL_STRUCT     *pTag  ;/*���. ���. ���. ᥣ���⮬*/
    COMP_REAL_STRUCT    *pComp ;
    /* DATA_REAL_STRUCT    *pData ;*/
}Edi_Point;

typedef struct _Edi_WrPoint_
{ /*�����⥫� �� �����, ��᫥���� ����ᠭ�� � ��ॢ� ᮮ�饭��.*/
    TAG_REAL_STRUCT     *pGrStrArr; /*��᫥���� � ���ᨢ�*/

    TAG_REAL_STRUCT     *pTag  ;
    TAG_REAL_STRUCT     *pTagArr;

    COMP_REAL_STRUCT    *pComp ;
    COMP_REAL_STRUCT    *pCompArr ;

    DATA_REAL_STRUCT    *pData ;
    DATA_REAL_STRUCT    *pDataArr ;
}Edi_WrPoint;

typedef struct _Edi_Stack_Point_ /*���࠭���� ������ �������� � �⥪*/
{
    TAG_REAL_STRUCT          *pGrStr;/*���. ���. ���. ��㯯�� ᥣ���⮢*/
    TAG_REAL_STRUCT          *pTag  ;/*���. ���. ���. ᥣ���⮬*/
    COMP_REAL_STRUCT         *pComp ;
    struct _Edi_Stack_Point_ *Next  ;
}Edi_Stack_Point;

#define edi_max2  ( ( (TAG_LEN) > (COMP_LEN) ) ? (TAG_LEN) : (COMP_LEN) )
#define edi_max ( ( (edi_max2+1) > (sizeof(int)) ) ? (edi_max2+1) : (sizeof(int)) )

typedef struct _Find_Data_  /*����� ��� ���᪠ ������*/
{
    char     *pData        ;/*��� ����� ᮤ�ঠ�� ���ᨢ 㪠��⥫�� �� �.*/
    int      NumData       ;/*��饥 ���-�� ��������� ������*/
    char     Data[edi_max] ;/*�᪮�� �����*/
    short    FindFlag      ;/*            0- ���᪠ ���뫮*/
    /*�� �饬:   1- �. ������,  */
    /*	       2- ��������,    */
    /*  	       3- ᥣ����      */
    /*            4- ᥣ�. ��.    */
    int      MaxNum        ;/*���ᨬ� ��࠭����� ������ */
    int      NumSeq        ;/*����� �. ������ � ��᫥����⥫쭮��*/
    Edi_Point *pPoint; /* �� �� ��⠭������ ������� �������� ��� ���᪠ */
}Find_Data;

typedef struct _edi_save_found_
{ /*���࠭塞 �������� ���. ���ᮬ ���., ���뢠� ���. ��������*/
    Edi_Point *pPoint;
    Find_Data *pFind;
}edi_save_found;


/*-------------------------------------------------------*/
/****************************************************************************/
/*-------------------------------------------------------*/

struct _EDI_REAL_MES_STRUCT_ /*����뢠�� ॠ�쭮�(��⠭���) ᮮ�饭��*/
{
    char Message[MESSAGE_LEN+1];  /*�������� ᮮ�饭��*/
    char Text[256+1];             /*���ᠭ��*/
    MesEdiStruct MesTxt;          /*�����⥫� �� ॠ�쭮� ᮮ�饭��(��襤襥 � ���� ��ப�)*/
    /*��� ��᪨ ⥪�� �� ��⠢����� ᮮ�饭��*/
    TAG_REAL_STRUCT *Tag;         /*�����⥫� �� ᥣ����*/

    EDI_TEMP_MESSAGE *pTempMes;/*�����⥫� �� 蠡��� ᮮ�饭��*/
    Edi_Point       *pPoint;      /*�����. ��� ��࠭�祭�� ��� ��������*/
    Edi_Stack_Point *pStack;      /*�����. ��� ��࠭���� ���. �������� � �⥪*/
    Find_Data       *pFind;       /*�����. ��� ���᪠ ������*/
    edi_save_found  *pFound[MAX_SAVE_FOUND]; /*�����. ��� ��࠭���� ��室�� */

    Edi_Error       ErrInfo;      /*��� � ᨭ⠪��. �訡��*/
    Edi_WrPoint     WrPoint;      /*�����⥫� ����� ��� �������� (�� ��⠢����� ᮮ�饭��)*/

    Edi_CharSet     *pCharSet;    /*��ࠢ���騥 ᨬ����*/
};
/*-------------------------------------------------------*/
struct _TAG_REAL_STRUCT_    /*����뢠�� ᥣ����(��㯯� ᥣ���⮢)*/
{
    int R;                    /*���-�� ����७�� ᥣ����(ᥣ���⭮� ��㯯�)*/
    TAG_STRUCT *TemplateTag  ; /*�����⥫� �� 蠡���.*/
    TAG_REAL_STRUCT  *NextTag; /*�����⥫� �� ᫥��騩 ᥣ����(ᥣ. ��.)*/
    TAG_REAL_STRUCT  *NextArr; /*�����⥫� �� ᫥��騩 ᥣ����(ᥣ. ��.) � ���ᨢ�*/
    TAG_REAL_STRUCT  *GrStr  ; /*�����⥫� �� ��⠢���騥 ��㯯� ᥣ���⮢(NULL - �᫨ ᥣ����!)*/
    COMP_REAL_STRUCT *Comp   ; /*�����⥫� �� ��������, �᫨ ��ᬠ�ਢ����� ᥣ����*/

    TAG_REAL_STRUCT *ParTag  ; /*�����⥫� �� த�⥫� - ������⭠� ��㯯� ��� NULL*/
};
/*-------------------------------------------------------*/
struct _COMP_REAL_STRUCT_ /*����뢠�� ��������*/
{
    int  R;                         /*���-�� ����७�� ��������*/
    int SingleData;                 /*������� ������ (��� ��������)*/
    COMPOSITE_STRUCT *TemplateComp; /*�����⥫� �� 蠡���.*/
    COMP_REAL_STRUCT *Next;         /*�����⥫� �� ᫥��騩 ��������*/
    COMP_REAL_STRUCT *NextArr;      /*�����⥫� �� ᫥��騩 �������� � ���ᨢ� ����.*/
    DATA_REAL_STRUCT *DataElem;     /*�����⥫� �� ����� ������*/

    TAG_REAL_STRUCT  *ParTag;       /*�����⥫� �� த�⥫� - ᥣ����*/
};
/*-------------------------------------------------------*/
struct _DATA_REAL_STRUCT_ /*����뢠�� ����� ������*/
{
    int  DataElem;                  /*�������� ����� ������*/
    const char *Data;               /*�����⥫� �� �����*/
    int  len;                       /*����� ������*/
    int  R;                         /*���-�� ����७�� �. ������*/
    int  Num;                       /*���浪��� ����� � ⥪�饩 ������ ��������*/
    int  malloc;                    /*㪠�뢠��, �� ⥪�� �࠭���� � �뤥������ �����*/
    DATA_ELEM_STRUCT *TemplateData; /*�����⥫� �� 蠡���.*/
    DATA_REAL_STRUCT *NextArr;      /*�����⥫� �� ᫥��騩 �. ������ � ���ᨢ�*/
    DATA_REAL_STRUCT *Next;         /*�����⥫� �� ᫥�. ����� ������*/

    COMP_REAL_STRUCT *ParComp; 	 /*�����⥫� �� த�⥫� - ��������*/
};
/*-------------------------------------------------------*/
/****************************************************************************/
/*-------------------------------------------------------*/
struct _EDI_TABLES_STRUCT_
{
    MESSAGE_TABLE_STRUCT      *MesTable;
    MES_STRUCT_TABLE_STRUCT   *MesStrTable;
    SEG_STRUCT_TABLE_STRUCT   *SegStrTable;
    COMP_STRUCT_TABLE_STRUCT  *CompStrTable;
    DATA_ELEM_TABLE_STRUCT    *DataTable;
    SEGMENT_TABLE_STRUCT      *SegTable;
    COMPOSITE_TABLE_STRUCT    *CompTable;
};
/*-------------------------------------------------------*/
struct _MESSAGE_TABLE_STRUCT_
{
    char Message[MESSAGE_LEN+1];
    char Text[256+1];
    MES_STRUCT_TABLE_STRUCT *First;/*�����⥫� �� ����� ���-��, ᮤ�ঠ��� ���� � ������ ᮮ�饭��*/
    MESSAGE_TABLE_STRUCT *Next;
    MESSAGE_TABLE_STRUCT *Prev;
};
/*-------------------------------------------------------*/
struct _MES_STRUCT_TABLE_STRUCT_
{
    char Message[MESSAGE_LEN+1];
    char Tag[TAG_LEN+1]; /*'\0' ,�᫨ ����뢠���� ᥣ���⭠� ��㯯�*/
    char Text[256+1];  /*���ᠭ�� ������� ᥣ����*/
    int Pos;
    int MaxPos;
    char S[1+1];
    int R;
    int GrpNum;
    SEG_STRUCT_TABLE_STRUCT *First;/*�����⥫� �� ����� ���-��, ᮤ�ঠ��� ���� � ������ ᥣ����*/
    MES_STRUCT_TABLE_STRUCT *Next;
    MES_STRUCT_TABLE_STRUCT *Prev;
};
/*-------------------------------------------------------*/
struct _SEG_STRUCT_TABLE_STRUCT_
{
    char Tag[TAG_LEN+1];
    int Pos;
    char Text[256+1];
    char Composite[COMP_LEN+1];
    int  DataElem;
    char S[1+1];
    int  R;
    COMP_STRUCT_TABLE_STRUCT *FirstComp;
    DATA_ELEM_TABLE_STRUCT   *FirstElem;
    SEG_STRUCT_TABLE_STRUCT *Next;
    SEG_STRUCT_TABLE_STRUCT *Prev;
};
/*-------------------------------------------------------*/
struct _COMP_STRUCT_TABLE_STRUCT_
{
    char Composite[COMP_LEN+1];
    int Pos;
    int DataElem;
    char Text[256+1];
    char S[1+1];
    int R;
    DATA_ELEM_TABLE_STRUCT *First;
    COMP_STRUCT_TABLE_STRUCT *Next;
    COMP_STRUCT_TABLE_STRUCT *Prev;
};
/*-------------------------------------------------------*/
struct _DATA_ELEM_TABLE_STRUCT_
{
    int DataElem;
    char Text[256+1];
    char Format[1+1];
    int MaxField;
    int MinField;
    DATA_ELEM_TABLE_STRUCT *Next;
    DATA_ELEM_TABLE_STRUCT *Prev;
};
/*-------------------------------------------------------*/
struct _SEGMENT_TABLE_STRUCT_
{
    char Tag[TAG_LEN+1];
    char Text[256+1];
    SEGMENT_TABLE_STRUCT *Next;
    SEGMENT_TABLE_STRUCT *Prev;
};
/*-------------------------------------------------------*/
struct _COMPOSITE_TABLE_STRUCT_
{
    char Composite[COMP_LEN+1];
    char Text[256+1];
    COMPOSITE_TABLE_STRUCT *Next;
    COMPOSITE_TABLE_STRUCT *Prev;
};
/*-------------------------------------------------------*/
/****************************************************************************/
#endif
