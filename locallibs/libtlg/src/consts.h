#ifndef LIBTLG_CONSTS_H
#define LIBTLG_CONSTS_H

#ifdef IN
#error IN already defined
#endif

#define MAX_TLG_SIZE 10240 // максимальный размер тлг в UDP-сообщении
#define MAX_TLG_LEN 900000 // максимальный размер телеграмы в C-структурах, в C++ используется string

/**
 * @brief управление очередью телеграм
 * значение поля sys_q из air_q 
 * */
typedef enum Direction
{
    IN = 1,                         ///< достать для обработки или отправки
    TYPEA_OUT = 2,                  ///< поместить в выходную (Edifact на отправку)
    REPEAT,                         ///< очередь телеграмм на ручную обработку
    OTHER_OUT,                      ///< поместить в выходную (не Edifact на отправку)
    WAIT_DELIV,                     ///< ожидает подтверждения от адресата
    INCOMING = 6,                   ///< поместить во входную на обработку
    DEL_ = 7,                       ///< удалить телеграмму
    LONG_PART,                      ///< очередь кусочков длинных телеграмм
    DISPATCH                        ///< очередь тлг для диспетчеризации
}Direction;



#endif /* LIBTLG_CONSTS_H */

