#ifndef _EDI_SQL_INSERT_H_
#define _EDI_SQL_INSERT_H_
#include <stdio.h>

#define MES_FILE_  "message.edi"
#define SEG_FILE_  "segment.edi"
#define COMP_FILE_ "composite.edi"
#define DATA_FILE_ "dataelement.edi"

#define BUFFER_SIZE 512 /*������ ���� �⥭��*/
#define TEXT_SIZE   256 /*���ᨬ��쭠� ����� ���� TEXT*/
#define COMM_SIZE   15  /*���ᨬ��쭠� ����� ���� (�� ���� ���ᠭ�� TEXT)*/
#define MAX_COM     7   /*���ᨬ� ����� � ��ப� 䠩��*/
#define POS_LEN     4   /*��� � oracle (���ᨬ��쭠� ����� POS & MAXPOS)*/
#define MES_R       5   /*���ᨬ��쭠� ����� R � EDI_STR_MESSAGE(���-�� ����७�� ᥣ����/ᥣ�. ��㯯�)*/
#define COMP_R      3   /*���ᨬ��쭠� ����� R � EDI_STR_COMPOSITE & EDI_STR_SEGMENT(���-�� ����७�� ��������/�.������ )*/
#define DE_LEN      4   /*��� � oracle (���ᨬ��쭠� ����� ����� ������*/
#define FIELD_LEN   4   /*��� � oracle (MaxField & MinField � EDI_DATA_ELEM)*/


#define MESCOMMAND  "Message:"   /* ����뢠�� �� ��砫� ���ᠭ�� ᮮ�饭�� */
#define SGCOMMAND   "Sg"         /* ����뢠�� �� ���ᠠ��� ᥣ�. ��㯯� */
#define SEGCOMMAND  "Segment:"   /* ����뢠�� �� ��砫� ���ᠭ�� ᥣ����*/
#define DECOMMAND   "DE:"        /* ����뢠�� �� ���ᠭ�� ����� ������ � ��⠢� ᥣ����*/
#define COMPCOMMAND "Composite:" /* ����뢠�� �� ��砫� ���ᠭ�� ��������*/

#define TABCHAR 0x9     /* ��� ⠡��樨      */
#define ANYKEYCHAR 0x20 /* ��� �஡���        */
#define COMMENTCHAR '#' /* ������ ��������� */
#define ENTERCHAR   0xA /* ��� Enter          */
#define TEXTCHAR    '"' /* ����窨 ��� ⥪�� */

typedef struct _COMMAND_STRUCT_
{
 unsigned short flag; /* �� ࠧ��ࠥ�:            */
 	              /* MES_FILE                  */
                      /* 0- ���ᠭ�� ᮮ�饭��     */
                      /* 1- ���ᠭ�� ᥣ����      */
                      /* 2- ���ᠭ�� ᥣ����. ��. */
                      /*****************************/
                      /* SEG_FILE                  */
                      /* 3- ���ᠭ�� ᥣ����      */
                      /* 4- ���ᠭ�� ��������     */
                      /* 5- ���ᠭ�� �. ������    */
                      /*****************************/
                      /* COMP_FILE                 */
                      /* 6- ���ᠭ�� ��������     */
                      /* 7- ���ᠭ�� ���-�� ����-�*/
                      /*****************************/
                      /* DATA_FILE                 */
                      /* 8- ���ᠭ�� ��-�� ������*/
                      /*****************************/
 char Command[MAX_COM][COMM_SIZE];
 char Text[TEXT_SIZE];
 int tlen;
}Command_Struct;

#define SQL_DUBLICATE 00001 /*����⪠ ����� � SQL 㦥 ����饩�� ���ଠ樨*/

typedef struct {
    FILE *mes;
    FILE *seg;
    FILE *comp;
    FILE *data;
}EdiFilePoints;

typedef int (*edi_insert_func)(Command_Struct *, char **);

#endif /* _EDI_SQL_INSERT_H_ */
