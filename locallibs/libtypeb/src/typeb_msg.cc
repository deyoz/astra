/*
*  C++ Implementation: typeb_msg
*
* Description: Локализация сообщений парсинга type_b
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/

#include "typeb_msg.h"

#define REGERR(x,e,r) REG_ERR__(TBMsg, x,e,r)

namespace typeb_parser
{
    REGERR(INV_ADDRESSES,
           "Invalid senser/receiver field",
           "Ошибка в поле отправитель/получатель");
    REGERR(UNKNOWN_MSG_TYPE,
           "Unknown type_b message code type",
           "Неизвестный тип type_b сообщения");
    REGERR(UNKNOWN_MESSAGE_ELEM,
           "Unknown message element",
           "Неизвестный элемент сообщения");
    REGERR(MISS_NECESSARY_ELEMENT,
           "Missed necessary element",
           "Пропущен обязательный элемент");
    REGERR(TOO_FEW_ELEMENTS,
           "Too few elements",
           "Слишком мало элементов данного типа");
    REGERR(TOO_LONG_MSG_LINE,
           "Too long TypeB message line",
           "Слишком длинная строка TypeB сообщения");
    REGERR(EMPTY_MSG_LINE,
          "Empty TypeB message line",
          "Пустая строка в TypeB сообщении");
    REGERR(PROG_ERR,
          "Program error at time of parsing",
          "Программная ошибка при разборе сообщения");
    REGERR(INV_DATETIME_FORMAT_z1z,
          "Неверный формат даты/времени '%1%'",
          "Invalid date/time format '%1%'");
    REGERR(INV_FORMAT_z1z,
          "Invalid format of '%1%'",
          "Неверный формат элемента '%1%'");
    REGERR(INV_FORMAT_z1z_FIELD_z2z,
           "Invalid format of '%1%' field %2%",
           "Неверный формат элемента '%1%' в поле %2%");
    REGERR(INV_NAME_ELEMENT,
          "Error in name element",
          "Ошибка в сегменте имен");
    REGERR(WRONG_CHARS_IN_NAME,
          "Wrong characters in name element",
          "Недопустимый набор символов в элементе имен");

    REGERR(INV_REMARK_STATUS,
          "Invalid remark status code",
          "Неверный код статуса ремарки");
    REGERR(INV_TICKET_NUM,
          "Invalid ticket number in remark",
          "Неверный номер билета в ремарке");
    REGERR(MISS_COUPON_NUM,
          "Missing coupon number in remark",
          "Пропущен номер купона в ремарке");
    REGERR(INV_COUPON_NUM,
           "Invalid coupon number",
           "Неверный номер купона");

    REGERR(INV_RL_ELEMENT,
           "Invalid recloc element",
           "Неверный Recloc елемент");
    REGERR(UNKNOWN_SSR_CODE,
           "Unknown SSR code",
           "Неизвестный код SSR");
    REGERR(NO_HANDLER_FOR_THIS_TYPE,
           "There are no handler for this type of airimp message",
           "Нет обработчика для данного типа airimp сообщения");
    REGERR(EMPTY_SSR_BODY,
           "Empty SSR body",
           "Пустой SSR");
    REGERR(PARTLY_HANDLED,
           "Message partly handled",
           "Сообщение было обработано частично");
    REGERR(INV_TICKNUM,
        "INVALID TICKET NUMBER",
        "НЕКОРРЕКТНЫЙ НОМЕР БИЛЕТА");

    REGERR(INV_NUMS_BY_DEST_ELEM,
           "Invalid Numerics by destination element",
           "Неверный елемент 'Numerics by destination'");
    REGERR(INV_CATEGORY_AP_ELEM,
           "Invalid Category by destination element",
           "Неверный елемент 'Category by destination'");
    REGERR(INV_CATEGORY_ELEM,
           "Invalid Category element",
           "Неверный елемент 'Category'");
    REGERR(INV_END_ELEMENT,
           "Invalid END element",
           "Неверный елемент 'End element'");
           
    REGERR(INV_FQTx_FORMAT,
           "Invalid FQTX element format",
           "Неверный элемент FQTX");
}

