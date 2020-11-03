#ifndef _EDI_ALL_H_
#define _EDI_ALL_H_
/*#define EDI_DEBUG*/ 	/*�� ���뢠�� � ॠ�쭮� ��⥬� */
			/*⮫쪮 ��� ���஢���� airimp.log*/
#define EDI_READ_BASE_OK       0
#define EDI_READ_BASE_ERR     -1
#define EDI_READ_BASE_NOT_FND -2

#ifdef TEST_150
#define EDI_CHNG_SIZE  10 /*dx ��� realloc*/
#else
#define EDI_CHNG_SIZE  100 /*dx ��� realloc*/
#endif

#define EDI_SQL_NOT_FOUND      1403

    /*�������, � ���஬ ��室����� ��� ᮮ�饭��*/
#define TAG_MES_NAME "UNH"

    /*�������, � * ��।����� �ࠢ���騥 ᨬ���� ��� ������� ᮮ�饭��*/
#define CUSTOM_CHAR_SET_TAG "UNA"
#define CUSTOM_CHAR_SET_LEN 6

#define CHSET_POSITION 1

#define END_SEG      "'"  /*����� ᥣ����   */
#define END_SEG1     "\"\n"/*����� ᥣ����   */
#define END_COMP     '+'   /*����� ��������  */
#define END_EL_D     ':'   /*����� �. ������ */
#define EDI_RELEASE  '?'   /*�ய�� �ࠢ����� ᨬ�����*/
#define EDI_COMMA    ','   /* ������ � �᫠� */
#define EDI_FULL_STOP '.'  /* ��窠 � �᫠� */

#define CONDITIONAL   'C' /*�������, 㪠�뢠�騥 �� ��易�/����易� �����*/
#define MANDATORY     'M' /**/
#define DATA_FORMAT1  'A' /*A - Alpha-Numeric*/
#define DATA_FORMAT2  'N' /*N - Numeric      */
#define DATA_FORMAT3  'a' /*a - Alphabetic   */

#define MAX_FIND  5 /*���ᨬ��쭮� ���-�� ��������� �. ������ */
#define MAX_SAVE_FOUND 5 /*���ᨬ��쭮� ���-�� ��࠭����� ��室��*/

#endif
