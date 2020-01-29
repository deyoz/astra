//
// C++ Interface: tb_elements
//
// Description: ������ typeb ᮮ�饭��
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
    // �����⥫� �� 蠡���
    const tb_descr_element *Templ;

    unsigned Line;
    std::string::size_type Position;
    unsigned Length;
    std::string LineSrc;
protected:
    /**
     * @brief ��⠭�������� 㪠��⥫� �� 蠡��� �����
     * @param tmpl 蠡��� tb_descr_element*
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
     * @brief �����⥫� �� 蠡��� �����
     * @return tb_descr_element *
     */
    inline const tb_descr_element *templ() const
    {
        return Templ;
    }

    /**
     * @brief ��� �. ������
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
     * �������� �����
     * @param elem TbElement::ptr_t
     */
    inline void addElement(TbElement::ptr_t p)
    {
        Elements.push_back(p);
    }
    /**
     * �������� �����
     * @param elem TbElement *
     */
    inline void addElement(TbElement *elem)
    {
        addElement(TbElement::ptr_t(elem));
    }
    /**
     * �������� ��㯯� ����⮢
     * @param elem TbListElement *
     */
    inline void addGroupElement(TbListElement *elem)
    {
        addElement(TbElement::ptr_t(elem));
    }

    /**
     * ���⮩ �� ���⥩��� ?
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
     * ��砫� ᯨ᪠ ����⮢
     * @return const_iterator
     */
    inline const_iterator begin() const { return Elements.begin(); }
    /**
     * ����� ᯨ᪠ ����⮢
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
     * ��饥 ���-�� ������⮢ � ������ ���⥩���
     * @return unsigned
     */
    inline unsigned total() const { return Elements.size(); }

    /**
     * ��� �. ������
     * @return string - name
     */
    virtual std::string name() const;

    /**
     * ���� ����� � ᯨ᪥/��㯯�
     * @return TbElement reference
     */
    virtual const TbElement & front() const;
    /**
     * ���� ����� � ᯨ᪥/��㯯�
     * @return TbElement pointer
     */
    virtual const TbElement * frontp() const;


    /**
     * ���� ����� "���᮪" � ᯨ᪥/��㯯�
     * @return TbListElement reference
     */
    virtual const TbListElement & front_list() const;
    /**
     * ���� ����� "���᮪" � ᯨ᪥/��㯯�
     * @return TbListElement pointer
     */
    virtual const TbListElement * front_listp() const;

    /**
     * ���� ����� "��㯯�" � ᯨ᪥/��㯯�
     * @return TbGroupElement reference
     */
    virtual const TbGroupElement & front_grp() const;
    /**
     * ���� ����� "��㯯�" � ᯨ᪥/��㯯�
     * @return TbGroupElement pointer
     */
    virtual const TbGroupElement * front_grpp() const;

    /**
     * ��᫥���� ����� � ᯨ᪥/��㯯�
     * @return TbElement reference
     */
    virtual const TbElement & back() const;
    /**
     * ��᫥���� ����� � ᯨ᪥/��㯯�
     * @return TbElement pointer
     */
    virtual const TbElement * backp() const;


    /**
     * ��᫥���� ����� "���᮪" � ᯨ᪥/��㯯�
     * @return TbListElement reference
     */
    virtual const TbListElement & back_list() const;
    /**
     * ��᫥���� ����� "���᮪" � ᯨ᪥/��㯯�
     * @return TbListElement pointer
     */
    virtual const TbListElement * back_listp() const;

    /**
     * ��᫥���� ����� "��㯯�" � ᯨ᪥/��㯯�
     * @return TbGroupElement reference
     */
    virtual const TbGroupElement & back_grp() const;
    /**
     * ��᫥���� ����� "��㯯�" � ᯨ᪥/��㯯�
     * @return TbGroupElement pointer
     */
    virtual const TbGroupElement * back_grpp() const;


    /**
     * ������� �� ������
     * @return TbElement reference
     */
    virtual const TbElement & byNum(unsigned num) const;
    /**
     * ������� �� ������
     * @return TbElement pointer
     */
    virtual const TbElement * byNump(unsigned num) const;

    /**
     * ���᮪ �� ������
     * @return TbListElement reference
     */
    virtual const TbListElement & byNum_list(unsigned num) const;
    /**
     * ���᮪ �� ������
     * @return TbListElement pointer
     */
    virtual const TbListElement * byNum_listp(unsigned num) const;

    /**
     * ��㯯� �� ������
     * @return TbGroupElement reference
     */
    virtual const TbGroupElement & byNum_grp(unsigned num) const;
    /**
     * ��㯯� �� ������
     * @return TbGroupElement pointer
     */
    virtual const TbGroupElement * byNum_grpp(unsigned num) const;

    virtual TbElemType_t elemType() const { return TbListElem; }

    virtual void print(std::ostream& os, int level) const;

    /**
     * ���� ������⮢ �� �᫮���
     * @param it1 ��砫�
     * @param it2 �����
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
     * ��� �. ������
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
