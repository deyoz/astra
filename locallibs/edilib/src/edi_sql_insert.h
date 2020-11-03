#ifndef _EDI_SQL_INSERT_H_
#define _EDI_SQL_INSERT_H_
#include <stdio.h>

#define MES_FILE_  "message.edi"
#define SEG_FILE_  "segment.edi"
#define COMP_FILE_ "composite.edi"
#define DATA_FILE_ "dataelement.edi"

#define BUFFER_SIZE 512 /*Размер буфера чтения*/
#define TEXT_SIZE   256 /*Максимальная длина поля TEXT*/
#define COMM_SIZE   15  /*Максимальная длина поля (не поля описания TEXT)*/
#define MAX_COM     7   /*Максимум полей в строке файла*/
#define POS_LEN     4   /*Как в oracle (максимальная длина POS & MAXPOS)*/
#define MES_R       5   /*Максимальная длина R в EDI_STR_MESSAGE(кол-во повторений сегмента/сегм. группы)*/
#define COMP_R      3   /*Максимальная длина R в EDI_STR_COMPOSITE & EDI_STR_SEGMENT(кол-во повторений композита/эл.данных )*/
#define DE_LEN      4   /*Как в oracle (максимальная длина элемента данных*/
#define FIELD_LEN   4   /*Как в oracle (MaxField & MinField в EDI_DATA_ELEM)*/


#define MESCOMMAND  "Message:"   /* Указывает на начало описания сообщения */
#define SGCOMMAND   "Sg"         /* Указывает на описаание сегм. группы */
#define SEGCOMMAND  "Segment:"   /* Указывает на начало описания сегмента*/
#define DECOMMAND   "DE:"        /* Указывает на описание элемента данных в составе сегмента*/
#define COMPCOMMAND "Composite:" /* Указывает на начало описания композита*/

#define TABCHAR 0x9     /* код табуляции      */
#define ANYKEYCHAR 0x20 /* код пробела        */
#define COMMENTCHAR '#' /* Символ комментария */
#define ENTERCHAR   0xA /* Код Enter          */
#define TEXTCHAR    '"' /* Кавычки для текста */

typedef struct _COMMAND_STRUCT_
{
 unsigned short flag; /* Что разбираем:            */
 	              /* MES_FILE                  */
                      /* 0- описание сообщения     */
                      /* 1- описание сегмента      */
                      /* 2- описание сегментн. гр. */
                      /*****************************/
                      /* SEG_FILE                  */
                      /* 3- описание сегмента      */
                      /* 4- описание композита     */
                      /* 5- описание эл. данных    */
                      /*****************************/
                      /* COMP_FILE                 */
                      /* 6- описание композита     */
                      /* 7- описание стр-ры комп-та*/
                      /*****************************/
                      /* DATA_FILE                 */
                      /* 8- описание элем-ов данных*/
                      /*****************************/
 char Command[MAX_COM][COMM_SIZE];
 char Text[TEXT_SIZE];
 int tlen;
}Command_Struct;

#define SQL_DUBLICATE 00001 /*Попытка записи в SQL уже имеющейся информации*/

typedef struct {
    FILE *mes;
    FILE *seg;
    FILE *comp;
    FILE *data;
}EdiFilePoints;

typedef int (*edi_insert_func)(Command_Struct *, char **);

#endif /* _EDI_SQL_INSERT_H_ */
