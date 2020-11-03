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
    char code[MESSAGE_LEN+1];  /*Название сообщения*/
    edi_msg_types_t type;
}EDI_MESSAGE_TYPE;

/*-------------------------------------------------------*/
struct _EDI_TEMP_MESSAGE_ /*Описывает сообщение*/
{
    EDI_MESSAGE_TYPE Type;        /* Тип сообщения */
    char Text[256+1];             /* Описание */
    TAG_STRUCT *Tag;              /* Указатель на сегмент */
    EDI_TEMP_MESSAGE *Next;       /* Указатель на следущий эл. */
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
struct _TAG_STRUCT_ /*Описывает сегмент(группу сегментов)*/
{
    char TagName[TAG_LEN+1];/*Имя сегмента('\0' если описывается группа сегментов)*/
    char Text[256+1];       /*Описание сегмента(группы сег.)*/
    int Pos;                /*Позиция сегмента(группы сег.) в сообщении*/
    short Num;              /*Номер сегментной группы (0 - если сегмент)*/
    char S;                 /*Признак обязательности сегмента(группы сег.)*/
    int MaxR;              /*Максимальное кол-во повторений сегмента(сегментной группы)*/
    TAG_STRUCT *NextTag;    /*Указатель на следущий сегмент(сег. гр.)*/
    TAG_STRUCT *GrStr  ;    /*Указатель на составляющие группы сегментов(NULL - если сегмент!)*/
    COMPOSITE_STRUCT *Comp; /*Указатель на композит, если рассматривается сегмент*/
};
/*-------------------------------------------------------*/
struct _COMPOSITE_STRUCT_ /*Описывает композит*/
{
    char Composite[COMP_LEN+1]; /*Имя композита ('\0' если элем. данных без композита)*/
    char Text[256+1];           /*Описание (м.быть NULL)*/
    int Pos;                    /*Позиция*/
    char S;                     /*Признак обязательности композита*/
    int MaxR;                  /*Максимальное кол-во повторений композита*/
    int Num;    	      	     /*Номер в сегменте, если повторяется не в группе */
    /*  См. edilib.txt Примечание1 */
    COMPOSITE_STRUCT *Next;     /*Указатель на следущий композит*/
    DATA_ELEM_STRUCT *DataElem; /*Указатель на элемент данных*/
};
/*-------------------------------------------------------*/
struct _DATA_ELEM_STRUCT_ /*Описывает элемент данных*/
{
    int DataElem;           /*Название элемента данных*/
    char Text[256+1];       /*Описание элемента данных*/
    char S;                 /*Признак необходимости эл. данных*/
    int  Pos;               /*Позиция эл. данных в композите*/
    char Format;            /*Формат данных */
    /*!!!!!*/               /*(A - Alpha-Numeric, N - Numeric, a - Alphabetic)*/
    int MaxField;           /*Максимальный размер элемента данных*/
    int MinField;           /*Минимальный  размер элемента данных*/
    int MaxR;
    int Num; 		 /*Номер в композите, сегменте, если повторяется не в группе */
    /*  См. edilib.txt Примечание1 */
    DATA_ELEM_STRUCT *Next; /*Указатель на след. элемент данных*/
};
/*-------------------------------------------------------*/
/****************************************************************************/
/*-------------------------------------------------------*/
typedef struct {
    char *mes;
    int size;
    int len;
    unsigned index; /*Индекс в строке сообщения при считывании*/
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

typedef struct _Edi_Point_ /*Ограничение области видимости*/
{
    TAG_REAL_STRUCT     *pGrStr;/*Обл. вид. огр. группой сегментов*/
    TAG_REAL_STRUCT     *pTag  ;/*Обл. вид. огр. сегментом*/
    COMP_REAL_STRUCT    *pComp ;
    /* DATA_REAL_STRUCT    *pData ;*/
}Edi_Point;

typedef struct _Edi_WrPoint_
{ /*Указатель на элемент, последним записанный в дерево сообщения.*/
    TAG_REAL_STRUCT     *pGrStrArr; /*Последний в массиве*/

    TAG_REAL_STRUCT     *pTag  ;
    TAG_REAL_STRUCT     *pTagArr;

    COMP_REAL_STRUCT    *pComp ;
    COMP_REAL_STRUCT    *pCompArr ;

    DATA_REAL_STRUCT    *pData ;
    DATA_REAL_STRUCT    *pDataArr ;
}Edi_WrPoint;

typedef struct _Edi_Stack_Point_ /*Сохранение области видимости в стек*/
{
    TAG_REAL_STRUCT          *pGrStr;/*Обл. вид. огр. группой сегментов*/
    TAG_REAL_STRUCT          *pTag  ;/*Обл. вид. огр. сегментом*/
    COMP_REAL_STRUCT         *pComp ;
    struct _Edi_Stack_Point_ *Next  ;
}Edi_Stack_Point;

#define edi_max2  ( ( (TAG_LEN) > (COMP_LEN) ) ? (TAG_LEN) : (COMP_LEN) )
#define edi_max ( ( (edi_max2+1) > (sizeof(int)) ) ? (edi_max2+1) : (sizeof(int)) )

typedef struct _Find_Data_  /*Необх для поиска данных*/
{
    char     *pData        ;/*Обл памяти содержащая массив указателей на д.*/
    int      NumData       ;/*Общее кол-во найденных данных*/
    char     Data[edi_max] ;/*Искомые данные*/
    short    FindFlag      ;/*            0- поиска небыло*/
    /*Что ищем:   1- эл. данных,  */
    /*	       2- композит,    */
    /*  	       3- сегмент      */
    /*            4- сегм. гр.    */
    int      MaxNum        ;/*Максимум сохраненных данных */
    int      NumSeq        ;/*Номер эл. данных в последовательности*/
    Edi_Point *pPoint; /* На что установлена область видимости для поиска */
}Find_Data;

typedef struct _edi_save_found_
{ /*Сохраняем найденные посл. поисом дан., учитывая обл. видимости*/
    Edi_Point *pPoint;
    Find_Data *pFind;
}edi_save_found;


/*-------------------------------------------------------*/
/****************************************************************************/
/*-------------------------------------------------------*/

struct _EDI_REAL_MES_STRUCT_ /*Описывает реальное(считанное) сообщение*/
{
    char Message[MESSAGE_LEN+1];  /*Название сообщения*/
    char Text[256+1];             /*Описание*/
    MesEdiStruct MesTxt;          /*Указатель на реальное сообщение(Пришедшее в виде строки)*/
    /*Или куски текста при составлении сообщения*/
    TAG_REAL_STRUCT *Tag;         /*Указатель на сегмент*/

    EDI_TEMP_MESSAGE *pTempMes;/*Указатель на шаблон сообщения*/
    Edi_Point       *pPoint;      /*Необх. для ограничения обл видимости*/
    Edi_Stack_Point *pStack;      /*Необх. для сохранения обл. видимости в стек*/
    Find_Data       *pFind;       /*Необх. для поиска данных*/
    edi_save_found  *pFound[MAX_SAVE_FOUND]; /*Необх. для сохранения находок */

    Edi_Error       ErrInfo;      /*Инфо о синтаксич. ошибке*/
    Edi_WrPoint     WrPoint;      /*Указатель внутри обл видимости (при составлении сообщения)*/

    Edi_CharSet     *pCharSet;    /*Управляющие символы*/
};
/*-------------------------------------------------------*/
struct _TAG_REAL_STRUCT_    /*Описывает сегмент(группу сегментов)*/
{
    int R;                    /*Кол-во повторений сегмента(сегментной группы)*/
    TAG_STRUCT *TemplateTag  ; /*Указатель на шаблон.*/
    TAG_REAL_STRUCT  *NextTag; /*Указатель на следущий сегмент(сег. гр.)*/
    TAG_REAL_STRUCT  *NextArr; /*Указатель на следущий сегмент(сег. гр.) в массиве*/
    TAG_REAL_STRUCT  *GrStr  ; /*Указатель на составляющие группы сегментов(NULL - если сегмент!)*/
    COMP_REAL_STRUCT *Comp   ; /*Указатель на композит, если рассматривается сегмент*/

    TAG_REAL_STRUCT *ParTag  ; /*Указатель на родителя - Сегментная группа или NULL*/
};
/*-------------------------------------------------------*/
struct _COMP_REAL_STRUCT_ /*Описывает композит*/
{
    int  R;                         /*Кол-во повторений композита*/
    int SingleData;                 /*Элемент данных (без композита)*/
    COMPOSITE_STRUCT *TemplateComp; /*Указатель на шаблон.*/
    COMP_REAL_STRUCT *Next;         /*Указатель на следущий композит*/
    COMP_REAL_STRUCT *NextArr;      /*Указатель на следущий композит в массиве комп.*/
    DATA_REAL_STRUCT *DataElem;     /*Указатель на элемент данных*/

    TAG_REAL_STRUCT  *ParTag;       /*Указатель на родителя - сегмент*/
};
/*-------------------------------------------------------*/
struct _DATA_REAL_STRUCT_ /*Описывает элемент данных*/
{
    int  DataElem;                  /*Название элемента данных*/
    const char *Data;               /*Указатель на данные*/
    int  len;                       /*Длина данных*/
    int  R;                         /*Кол-во повторений эл. данных*/
    int  Num;                       /*Порядковый номер в текущей области видимости*/
    int  malloc;                    /*указывает, что текст хранится в выделенной памяти*/
    DATA_ELEM_STRUCT *TemplateData; /*Указатель на шаблон.*/
    DATA_REAL_STRUCT *NextArr;      /*Указатель на следущий эл. данных в массиве*/
    DATA_REAL_STRUCT *Next;         /*Указатель на след. элемент данных*/

    COMP_REAL_STRUCT *ParComp; 	 /*Указатель на родителя - композит*/
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
    MES_STRUCT_TABLE_STRUCT *First;/*Указатель на первую стр-ру, содержащую инфу о данном сообщении*/
    MESSAGE_TABLE_STRUCT *Next;
    MESSAGE_TABLE_STRUCT *Prev;
};
/*-------------------------------------------------------*/
struct _MES_STRUCT_TABLE_STRUCT_
{
    char Message[MESSAGE_LEN+1];
    char Tag[TAG_LEN+1]; /*'\0' ,если описывается сегментная группа*/
    char Text[256+1];  /*Описание данного сегмента*/
    int Pos;
    int MaxPos;
    char S[1+1];
    int R;
    int GrpNum;
    SEG_STRUCT_TABLE_STRUCT *First;/*Указатель на первую стр-ру, содержащую инфу о данном сегменте*/
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
