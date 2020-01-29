//
// C++ Interface: tb_elements
//
// Description: элементы typeb сообщения
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _TB_ELEMENTS_H_
#define _TB_ELEMENTS_H_
#include <string>
#include <list>
#include <exception>
#include <boost/shared_ptr.hpp>
//#include "typeb_cast.h"

namespace typeb_parser
{
class tb_descr_element;
class TypeBMessage;
class TbGroupElement;
class TbListElement;
class TbElement;
template <class T> const T & TbElem_cast (const char *nick, const char *file, int line,
                                          const TbElement &elem);
template <class T> const T * TbElem_cast (const char *nick, const char *file, int line,
                                          const TbElement *elem);

enum TbElemType_t { TbSimpleElem = 0, TbListElem, TbGroupElem };

/**
 * @class TbElement
 * @brief Typeb Message Element
*/
class TbElement
{
    friend class TypeBMessage;
    // Указатель на шаблон
    const tb_descr_element *Templ;

    unsigned Line;
    std::string::size_type Position;
    unsigned Length;
    std::string LineSrc;
protected:
    /**
     * @brief Устанавливает указатель на шаблон элемента
     * @param tmpl шаблон tb_descr_element*
     */
    inline void setTemplate(const tb_descr_element *tmpl)
    {
        Templ = tmpl;
    }

    inline void setLine  (unsigned l) { Line = l; }
    inline void setPos   (std::string::size_type p) { Position = p; }
    inline void setLength(unsigned l) { Length = l; }
    inline void setLineSrc(const std::string &src) { LineSrc = src; }
public:
    typedef boost::shared_ptr<TbElement> ptr_t;
    /**
     * @brief Указатель на шаблон элемента
     * @return tb_descr_element *
     */
    inline const tb_descr_element *templ() const
    {
        return Templ;
    }

    /**
     * @brief Имя эл. данных
     * @return string - name
     */
    virtual std::string name() const;

    /**
     * @brief Line in source message this element has gotten
     * @return line position
     */
    virtual unsigned line() const;
    /**
     * @brief Line position in source message this element has gotten
     * @return line position
     */
    virtual std::string::size_type position() const;

    /**
     * @brief Element length
     * @return
     */
    virtual unsigned length() const;

    /**
     * @brief Return an element as pointer of Group Elements
     * @return TbGroupElement *
     */
    virtual const TbGroupElement * elemAsGroupp() const;

    /**
     * @brief Return an element as reference of Group Elements
     * @return TbGroupElement &
     */
    virtual const TbGroupElement & elemAsGroup() const;

    /**
     * @brief Return an element as pointer of List Elements
     * @return TbListElement *
     */
    virtual const TbListElement * elemAsListp() const;

    /**
     * @brief Return an element as reference of List Elements
     * @return TbListElement &
     */
    virtual const TbListElement & elemAsList() const;

    virtual TbElemType_t elemType() const { return TbSimpleElem; }

    template<class T> const T &cast(const char *nick, const char *file, int line) const
    {
        return TbElem_cast<T>(nick, file, line, *this);
    }

    template<class T> const T *castp(const char *nick, const char *file, int line) const
    {
        return TbElem_cast<T>(nick, file, line, this);
    }

    virtual void print(std::ostream& os, int level) const;

    virtual const std::string &lineSrc() const { return LineSrc; }

    virtual ~TbElement()
    {
    }
    TbElement():Templ(0), Line(0), Position(0), Length(0) {}
};

class TbListElement : public TbElement
{
    friend class TypeBMessage;
    typedef std::list<TbElement::ptr_t> ElemsList;
    ElemsList Elements;
protected:
    /**
     * Добавляет элемент
     * @param elem TbElement::ptr_t
     */
    inline void addElement(TbElement::ptr_t p)
    {
        Elements.push_back(p);
    }
    /**
     * Добавляет элемент
     * @param elem TbElement *
     */
    inline void addElement(TbElement *elem)
    {
        addElement(TbElement::ptr_t(elem));
    }
    /**
     * Добавляет группу элементов
     * @param elem TbListElement *
     */
    inline void addGroupElement(TbListElement *elem)
    {
        addElement(TbElement::ptr_t(elem));
    }

    /**
     * Пустой ли контейнер ?
     * @return bool
     */
    inline bool empty() const { return Elements.empty(); }

    typedef ElemsList::iterator iterator;
    typedef ElemsList::reverse_iterator reverse_iterator;
public:
    typedef boost::shared_ptr<TbListElement> ptr_t;
    typedef ElemsList::const_iterator const_iterator;
    typedef ElemsList::const_reverse_iterator const_reverse_iterator;

    /**
     * Начало списка элементов
     * @return const_iterator
     */
    inline const_iterator begin() const { return Elements.begin(); }
    /**
     * Конец списка элементов
     * @return const_iterator
     */
    inline const_iterator end()   const { return Elements.end(); }

    /**
     * Reference to list.rbegin()
     * @return const_reverse_iterator
     */
    inline const_reverse_iterator rbegin() const { return Elements.rbegin(); }

    /**
     * Reference to list.rend()
     * @return const_reverse_iterator
     */
    inline const_reverse_iterator rend()   const { return Elements.rend(); }

    /**
     * Общее кол-во елементов в данном контейнере
     * @return unsigned
     */
    inline unsigned total() const { return Elements.size(); }

    /**
     * Имя эл. данных
     * @return string - name
     */
    virtual std::string name() const;

    /**
     * Первый элемент в списке/группе
     * @return TbElement reference
     */
    virtual const TbElement & front() const;
    /**
     * Первый элемент в списке/группе
     * @return TbElement pointer
     */
    virtual const TbElement * frontp() const;


    /**
     * Первый элемент "Список" в списке/группе
     * @return TbListElement reference
     */
    virtual const TbListElement & front_list() const;
    /**
     * Первый элемент "Список" в списке/группе
     * @return TbListElement pointer
     */
    virtual const TbListElement * front_listp() const;

    /**
     * Первый элемент "Группа" в списке/группе
     * @return TbGroupElement reference
     */
    virtual const TbGroupElement & front_grp() const;
    /**
     * Первый элемент "Группа" в списке/группе
     * @return TbGroupElement pointer
     */
    virtual const TbGroupElement * front_grpp() const;

    /**
     * Последний элемент в списке/группе
     * @return TbElement reference
     */
    virtual const TbElement & back() const;
    /**
     * Последний элемент в списке/группе
     * @return TbElement pointer
     */
    virtual const TbElement * backp() const;


    /**
     * Последний элемент "Список" в списке/группе
     * @return TbListElement reference
     */
    virtual const TbListElement & back_list() const;
    /**
     * Последний элемент "Список" в списке/группе
     * @return TbListElement pointer
     */
    virtual const TbListElement * back_listp() const;

    /**
     * Последний элемент "Группа" в списке/группе
     * @return TbGroupElement reference
     */
    virtual const TbGroupElement & back_grp() const;
    /**
     * Последний элемент "Группа" в списке/группе
     * @return TbGroupElement pointer
     */
    virtual const TbGroupElement * back_grpp() const;


    /**
     * Элемент по номеру
     * @return TbElement reference
     */
    virtual const TbElement & byNum(unsigned num) const;
    /**
     * Элемент по номеру
     * @return TbElement pointer
     */
    virtual const TbElement * byNump(unsigned num) const;

    /**
     * Список по номеру
     * @return TbListElement reference
     */
    virtual const TbListElement & byNum_list(unsigned num) const;
    /**
     * Список по номеру
     * @return TbListElement pointer
     */
    virtual const TbListElement * byNum_listp(unsigned num) const;

    /**
     * Группа по номеру
     * @return TbGroupElement reference
     */
    virtual const TbGroupElement & byNum_grp(unsigned num) const;
    /**
     * Группа по номеру
     * @return TbGroupElement pointer
     */
    virtual const TbGroupElement * byNum_grpp(unsigned num) const;

    virtual TbElemType_t elemType() const { return TbListElem; }

    virtual void print(std::ostream& os, int level) const;

    /**
     * Поиск елементов по условию
     * @param it1 начало
     * @param it2 конец
     * @param pred Predicate
     * @return const_iterator
     */
    template <class Predicate>
        const_iterator find_if(const_iterator it1, const_iterator it2, Predicate pred) const
    {
        return std::find_if(it1, it2, pred);
    }

    virtual ~TbListElement(){}
};

class TbGroupElement : public TbListElement
{
protected:
    friend class TypeBMessage;
    TbGroupElement(){}
    inline void addGroupElement(TbGroupElement *elem)
    {
        addElement(elem);
    }

    static std::list<std::string> split_spath(const std::string &spath);
    static const char path_separator = '/';
    const TbElement * findp(std::list< std::string > &lpath) const;
public:
    /**
     * Имя эл. данных
     * @return string - name
     */
    virtual std::string name() const;

    /**
    * Find element by path name (use access names for search)
    * @param path something like "NameGr/Recloc"
    * @return return TbElement pointer, 0 if not found
    */
    const TbElement * findp(const std::string &path) const;

    /**
     * Find element by path name (use access names for search)
     * @param path something like "NameGr/Recloc"
     * @return return TbElement reference, throws if not found
     */
    const TbElement & find (const std::string &path) const;

    /**
     * Find TbListElement by path name (use access names for search)
     * @param path something like "NameGr/Name"
     * @return return TbListElement pointer, 0 if not found
     */
    const TbListElement *find_listp(const std::string &path) const;

    /**
     * Find TbListElement by path name (use access names for search)
     * @param path something like "NameGr/Name"
     * @return return TbListElement reference, throws if not found
     */
    const TbListElement &find_list (const std::string &path) const;

    /**
     * Find TbGroupElement by path name (use access names for search)
     * @param path something like "NameGr/SomeThingElse/SomeGr"
     * @return return TbGroupElement pointer, 0 if not found
     */
    const TbGroupElement *find_grpp(const std::string &path) const;

    /**
     * Find TbGroupElement by path name (use access names for search)
     * @param path something like "NameGr/SomeThingElse/SomeGr"
     * @return return TbGroupElement reference, throws if not found
     */
    const TbGroupElement &find_grp (const std::string &path) const;

    virtual TbElemType_t elemType() const { return TbGroupElem; }

    virtual void print(std::ostream& os, int level) const;

    typedef boost::shared_ptr<TbGroupElement> ptr_t;
    virtual ~TbGroupElement(){}
};

} //namespace typeb_parser

#endif /*_TB_ELEMENTS_H_*/
