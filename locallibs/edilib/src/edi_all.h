#ifndef _EDI_ALL_H_
#define _EDI_ALL_H_
/*#define EDI_DEBUG*/ 	/*не открывать в реальной системе */
			/*только для тестирования airimp.log*/
#define EDI_READ_BASE_OK       0
#define EDI_READ_BASE_ERR     -1
#define EDI_READ_BASE_NOT_FND -2

#ifdef TEST_150
#define EDI_CHNG_SIZE  10 /*dx для realloc*/
#else
#define EDI_CHNG_SIZE  100 /*dx для realloc*/
#endif

#define EDI_SQL_NOT_FOUND      1403

    /*Сегмент, в котором находиться имя сообщения*/
#define TAG_MES_NAME "UNH"

    /*Сегмент, в * передаются управляющие символы для данного сообщения*/
#define CUSTOM_CHAR_SET_TAG "UNA"
#define CUSTOM_CHAR_SET_LEN 6

#define CHSET_POSITION 1

#define END_SEG      "'"  /*Конец сегмента   */
#define END_SEG1     "\"\n"/*Конец сегмента   */
#define END_COMP     '+'   /*Конец композита  */
#define END_EL_D     ':'   /*Конец эл. данных */
#define EDI_RELEASE  '?'   /*Пропуск управляющих символов*/
#define EDI_COMMA    ','   /* Запятая в числах */
#define EDI_FULL_STOP '.'  /* Точка а числах */

#define CONDITIONAL   'C' /*Символы, указывающие на обязат/необязат элемента*/
#define MANDATORY     'M' /**/
#define DATA_FORMAT1  'A' /*A - Alpha-Numeric*/
#define DATA_FORMAT2  'N' /*N - Numeric      */
#define DATA_FORMAT3  'a' /*a - Alphabetic   */

#define MAX_FIND  5 /*Максимальное кол-во найденных эл. данных */
#define MAX_SAVE_FOUND 5 /*Максимальное кол-во сохраненных находок*/

#endif
