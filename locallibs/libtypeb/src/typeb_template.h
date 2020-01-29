//
// C++ Interface: typeb_template
//
// Description: Шаблоны разборщиков сообщений
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _TYPEB_TEMPLATE_H_
#define _TYPEB_TEMPLATE_H_

#include <string>
#include <list>
#include <map>
#include <boost/shared_ptr.hpp>
#include "tb_elements.h"

namespace typeb_parser
{
enum TemplElemType
{
    SingleElement,
    SingleMultilineElement,
    GroupOfElements,
    SwitchElement,
};


/**
 * @class tb_descr_element Базовый элемент эелемента шаблона
 */
class tb_descr_element {
    // Позарез нежен
    bool Necessary;
    // Может начаться с середины строки сообщения
    // Или за ним могут начаться другие элементы (на этой же строке)
    bool MayBeInline;

    // Максим. кол-во повторений
    unsigned MaxNumOfRepetition;
    // Минимум повторений
    unsigned MinNumOfRepetition;
public:
    typedef boost::shared_ptr<tb_descr_element> ptr_t;

    tb_descr_element();

    tb_descr_element *setNecessary();
    tb_descr_element *setMayBeInline();

    bool necessaty() const;
    bool mayBeInline() const;

    virtual const char * name () const = 0;
    virtual const char * accessName() const = 0;
    virtual const char * lexeme() const = 0;

    /**
     * set maximum number of repetition
     * @param num tb_descr_element::unlimited == unlimited
     * @return this
     */
    tb_descr_element *setMaxNumOfRepetition(unsigned num);

    /**
     * set minimum number of repetition
     * @param num minimum number of repetition
     * @return this
     */
    tb_descr_element *setMinNumOfRepetition(unsigned num);

    /**
     * Установить неогранич кол-во повторений (max unsigned int)
     * @return this
     */
    tb_descr_element *setUnlimRep();

    /**
     * @return maximum number of repetition
     */
    unsigned maxNumOfRepetition() const;
    /**
     * @return minimum number of repetition
     */
    unsigned minNumOfRepetition() const;

    /**
     * Разбор элемента сообщения
     * @param text текст элемента
     * @return TbElement. Можно вернуть 0, чтобы пропустить элемент
     */
    virtual TbElement * parse(const std::string &text) const = 0;

    /**
     * Если обычный элемент
     * @return bool
     */
    virtual bool isSingle() const;
    /**
     * Элемент может иметь продолжение на следующей строке
     * @return bool
     */
    virtual bool isMultiline() const;
    /**
     * Группа элементов
     * @return bool
     */
    virtual bool isGroup() const;

    /**
     * Выбор из елементов
     * @return bool
    */
    virtual bool isSwitch() const;

    /**
     * Массив однородных элементов
     * @return bool
     */
    virtual bool isArray() const;

    /**
     * Имеет ли лексему ?
     * @return true
     */
    virtual bool hasLexeme() const;

    virtual bool isItYours(const std::string &, std::string::size_type *till) const;

    virtual bool check_nextline(const std::string &, const std::string &, size_t *start, size_t *till) const;

    virtual TemplElemType type () const { return SingleElement; }
    virtual ~tb_descr_element(){}
};

/**
 * @class tb_descr_multiline_element Базовый элемент эелемента шаблона
 * Может иметь продолжение на следующей строке
 */
class tb_descr_multiline_element : public tb_descr_element
{
public:
    virtual const char * nextline_lexeme() const = 0;
    virtual TemplElemType type() const { return SingleMultilineElement; }
    virtual bool check_nextline(const std::string &, const std::string &, size_t *start, size_t *till) const;
    virtual ~tb_descr_multiline_element() {}
};

/**
 * @class tb_descr_nolexeme_element Описывает элемент не имеющий лексемы
 * Должен иметь функцию "virtual bool isItYours(const std::string &) const"
*/
class tb_descr_nolexeme_element : public tb_descr_element
{
public:
    /**
    * Имеет ли лексему ?
    * @return false
    */
    virtual bool hasLexeme() const;
    virtual const char *lexeme() const { return ""; }
    virtual bool isItYours(const std::string &, std::string::size_type *till) const = 0;
    virtual ~tb_descr_nolexeme_element(){}
};

/**
 * @class tb_descr_nolexeme_multiline_element Базовый элемент эелемента шаблона
 * Может иметь продолжение на следующей строке, для определения след строки зовется ф-я
 */
class tb_descr_nolexeme_multiline_element : public tb_descr_nolexeme_element
{
public:
    virtual bool check_nextline(const std::string &, const std::string &, size_t *start, size_t *till) const = 0;
    virtual TemplElemType type() const { return SingleMultilineElement; }
    virtual ~tb_descr_nolexeme_multiline_element(){};
};

class TypeBMessage;
/**
 * @class base_templ_group Базовый класс описывающий группу
 * элементов шаблона
*/
class base_templ_group : public tb_descr_element
{
    virtual TbElement * parse(const std::string &text) const {return 0;}
    virtual const char * lexeme() const {return "";}
protected:
    typedef std::list< tb_descr_element::ptr_t > tb_descr_element_t;
    friend class TypeBMessage;
    typedef tb_descr_element_t::const_iterator const_elem_iter;
    typedef tb_descr_element_t::iterator elem_iter;
    inline const tb_descr_element_t &elements() const
    {
        return Elements;
    }

private:
    // Собстна элементы
    tb_descr_element_t Elements;
    bool MayMixElements;
public:
    base_templ_group() :
        MayMixElements(false)
    {
    }
    /**
    * добавить элемент в шаблон
    * @param elem эелемент шаблона
    * @return *tb_descr_element
    */
    tb_descr_element * addElement(tb_descr_element *elem);
    /**
     * добавить элемент в шаблон
     * @param elem эелемент шаблона завернутый в tb_descr_element::ptr_t
     * @return *tb_descr_element
     */
    tb_descr_element * addElement(tb_descr_element::ptr_t elem);

    /**
     * Элементы в этой группе могут следовать др. за другом в произвольном
     * порядке, но исключая первый элемент группы
     * @return this
     */
    base_templ_group *setMayMixElements();

    bool mayMixElements() const { return MayMixElements; }

    virtual const char * accessName() const { return ""; }
    virtual const char * name () const { return "Base Group of elements"; }

    virtual ~base_templ_group(){}
};

/**
 * @class templ_switch_group
 * Предназначен для случаев, когда нужно сделать выбор
 * из элементов
 * Примером может служить NIL элемент: должен быть либо NIL либо
 * все остальное: `Name`, `Remark`, etc
*/
class templ_switch_group : public base_templ_group
{
public:
    typedef boost::shared_ptr<templ_switch_group> ptr_t;
    virtual const char * name () const { return "Switch element"; }
    virtual TemplElemType type() const { return SwitchElement; }
    virtual const char * accessName() const { return ""; }
};

/**
 * @class templ_group Группа элементов
 */
class templ_group : public base_templ_group
{
    mutable std::string acc_name;
public:
    typedef boost::shared_ptr<templ_group> ptr_t;

    virtual const char * name () const { return "Group of elements"; }
    virtual TemplElemType type() const { return GroupOfElements; }
    virtual const char * accessName() const;
    void setAccessName(const char *accN);
    virtual ~templ_group(){}
};

class TypeBHeader;
class AddrHead;
/**
 * @class typeb_template Шаблон сообщения
 */
class typeb_template : public base_templ_group
{
    typedef boost::shared_ptr<typeb_template> ptr_t;
protected:
    friend class TypeBMessage;
    typedef std::map< std::string, typeb_template::ptr_t > TemplHash_t;
private:
    std::string MessageID;
    std::string Description;
    static const std::string::size_type IdLength = 3;
    static TemplHash_t Templates;
public:
    typeb_template(const std::string &ID, const std::string &descr);

    static const typeb_template *findTemplate(const std::string &);
    static void addTemplate(typeb_template *templ);

    const std::string &description() const { return Description; }
    virtual bool multipart() const { return false; }
    virtual void fillHeader(TypeBHeader& hdr, const AddrHead &addr, size_t flightRowIdx, const TypeBMessage& msg) const {}
    virtual ~typeb_template(){}
};
}//namespace typeb_parser

#endif /*_TYPEB_TEMPLATE_H_*/
