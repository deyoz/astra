//
// C++ Interface: typeb_core_parser
//
// Description: ядро typeb парсера
// Здесь должны жить общие алгоритмы для всех типов type B телеграмм
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _TYPEB_CORE_PARSER_H_
#define _TYPEB_CORE_PARSER_H_
#include <vector>
#include "typeb_parse_exceptions.h"
#include "tb_elements.h"
#include "typeb_template.h"

#ifndef _TYPEB_CORE_PARSER_SECRET_
#error Do not include this file! Include typeb_message.h instead!
#endif

/**
 * @file  typeb_core_parser.h Ядро typeb парсера
 * Здесь должны жить общие алгоритмы для всех типов type B телеграмм
*/

namespace typeb_parser
{

struct TBCfg
{
    static const unsigned short MsgTypeLen = 3;
    static const unsigned short MaxLineLen = 69;
};

class TypeBMessage;
class AddrHead
{
    std::string Sender;
    std::list<std::string> Receiver;
    std::string MsgId;

    AddrHead(){}
    friend class TypeBMessage;
    static AddrHead parse(const std::string &first, const std::string &second);

    static const unsigned short AddrLen = 7;
    AddrHead(std::string from, std::list<std::string> to, std::string mId)
    :Sender(from), Receiver(to), MsgId(mId)
    {
    }
public:

    /**
     * Отправитель
     * @return string
     */
    const std::string &sender() const { return Sender; }

    /**
     * Получатель
     * @return string
     */
    const std::list<std::string> &receiver() const { return Receiver; }

    /**
     * Идентификатор телеграммы
     * @return std::string
     */
    const std::string &msgId() const { return MsgId; }
};

struct TypeBHeader
{
    TypeBHeader(const std::string& type_, const std::string& depPt_, const AddrHead& addr_, size_t part_, bool last_, const std::string& uid_)
        : type(type_), depPoint(depPt_), addr(addr_), part(part_), isLastPart(last_), uid(uid_)
    {}
    std::string type;
    std::string depPoint;
    AddrHead addr;
    size_t part;
    bool isLastPart;
    std::string uid;
};

// class typeb_template;
// class base_templ_group;
class TypeBMessage : public TbGroupElement
{
    std::string SrcMessage;
    std::string Type;
    AddrHead Addrs;
    TypeBMessage(std::string msg):SrcMessage(msg), Template(0){}

    struct MsgLines_t
    {
        std::vector <std::string> Msgl;
        std::string &operator[](unsigned i) { return Msgl[i];}
        const std::string &operator[](unsigned i) const { return Msgl[i];}
        unsigned CurrMsgLine;
        std::string::size_type CurrLinePos;
        unsigned CurrPrsLength;
        unsigned totalLines() const { return Msgl.size(); }

        MsgLines_t():CurrMsgLine(0),CurrLinePos(0),CurrPrsLength(0){}
    }Msg;

    /**
     * Разбивает телеграмму на строки
     */
    void split2lines();

    void check_message_general_rules();

    const typeb_template *Template;

    /**
     * Разборка тела телеграммы по шаблону
     * @param templ шаблон
     * @param msgl строки сообщения
     * @return TbGroupElement::ptr_t
     */
    static TbGroupElement::ptr_t parse_group(const base_templ_group &templ,
                                             MsgLines_t &msgl,
                                             TbGroupElement *group_ptr=0);

    /**
     * Разбор одного элемента из группы
     * @param templ Шаблон
     * @param msgl Строки исходного сообщения
     * @return TbListElement::ptr_t - список разобранных эл. сообщения
     */
    static TbElement::ptr_t parse_switch_elem(const templ_switch_group &templ,
                                              MsgLines_t &msgl);

    /**
     * Разбор массива однородных элементов
     * @param templ Шаблон
     * @param msgl Строки исходного сообщения
     * @return TbListElement::ptr_t - список разобранных эл. сообщения
     */
    static TbListElement::ptr_t parse_array(const typeb_template::const_elem_iter &iter_in,
                                            const typeb_template::const_elem_iter &enditer,
                                            const tb_descr_element &templ,
                                            MsgLines_t &msgl);

    /**
     * Разбор одного элемента
     * @param templ Шаблон
     * @param msgl Строка исходного сообщения
     * @return TbElement::ptr_t
     */
    static TbElement::ptr_t
            parse_singleline_element(const typeb_template::const_elem_iter &iter_in,
                                     const typeb_template::const_elem_iter &enditer,
                                     const tb_descr_element & templ,
                                     MsgLines_t & msgl);

    static void addNextFewLines(const tb_descr_element & templ,
                                const base_templ_group::const_elem_iter &iter_in,
                                const base_templ_group::const_elem_iter &enditer,
                                MsgLines_t & msgl, std::string &str, std::string::size_type &end_pos);

    /**
     * Установить шаблон для данного сообщения
     * @param t Шаблон
     */
    inline void setTemplate(const typeb_template *t) { Template = t; }

    static std::string::size_type
            GetNextMsgPosition(const typeb_template::const_elem_iter &iter_in,
                               const typeb_template::const_elem_iter &enditer,
                               const std::string &currline,
                               std::string::size_type st);

public:

    /**
     * Получить шаблон для сообщения
     * @return Шаблон для данного сообщения
     */
    inline const typeb_template *templ() { return Template; }
    
    /**
     * Доступ к строке по номеру
     * @param i номер строки
     * @return строка - string
     */
    const std::string &msgline(unsigned int i) const { return Msg[i]; }
    
    /**
     * @return число строк в сообщении
     */
    unsigned totalLines() const { return Msg.Msgl.size(); }

    /**
     * Разборка телеграммы
     * @param message Исходный текст type b телеграммы
     * @return class TypeBMessage
     */
    static TypeBMessage parse(const std::string &message);

    /**
     * Разборка адресов телеграммы
     * @param message Исходный текст type b телеграммы
     * @return class AddrHead
     */
    static AddrHead parseAddr(const std::string &message);

    /**
     * Разборка заголовка телеграммы
     * @param message Исходный текст type b телеграммы
     * @return class TypeBHeader
     */
    static TypeBHeader parseHeader(const AddrHead &addr, const std::string &message);

    /**
     * Проверка, является ли телеграмма TypeB
     * @param tlg Текст тлг
     * @return true/false
     */
    static bool is_it_typeb(const std::string &tlg);

    /**
     * Исходный текст обработанной тлг
     * @return SrcMessage
     */
    const std::string &msgSource() const { return SrcMessage; }

    /**
     * Заголовок телеграммы - адреса отправителя и получателя
     * @return AddrHead
     */
    const AddrHead &addrs() const { return Addrs; }

    /**
     * 3-х буквенный тип телеграммы
     * @return string
     */
    const std::string &type() const { return Type; }
};

std::ostream & operator << (std::ostream& os, const TypeBMessage &msg);

std::ostream & operator << (std::ostream& os, const AddrHead &addr);

}

#endif /*_TYPEB_CORE_PARSER_H_*/
