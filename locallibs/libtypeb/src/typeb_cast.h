//
// C++ Interface: typeb_cast
//
// Description: Приведение типов в typeb сообщениях
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _TYPEB_CAST_H_
#define _TYPEB_CAST_H_
#include <string>
#include "tb_elements.h"
namespace typeb_parser
{
class TbElem_badcast : public std::bad_cast
{
    const std::string What;
    std::string makeWhat(const std::string &str)
    {
        return std::string(typeid(*this).name()) + ": " + str;
    }
public:
    TbElem_badcast(const char *str) throw ()
    :What(makeWhat(str))
    {
    }
    TbElem_badcast(const std::string &str) throw ()
    :What(makeWhat(str))
    {
    }

    TbElem_badcast(const char *nick, const char *file, int line, const char *str) throw ()
    :What(makeWhat(str))
    {
    }
    TbElem_badcast(const char *nick, const char *file, int line, const std::string &str) throw ()
    :What(makeWhat(str))
    {
    }

    virtual const char *what() const throw ()
    {
        return What.c_str();
    }

    virtual ~TbElem_badcast() throw () {}
};

template <class T>
         const T *TbElem_cast (const TbElement *elem)
{
    const T *res = dynamic_cast<const T *>(elem);
    return res;
}

template <class T>
         const T *TbElem_cast (const char *nick, const char *file, int line,
                               const TbElement *elem)
{
    T *res = dynamic_cast<T *>(elem);
    return res;
}

template <class T>
         const T &TbElem_cast (const TbElement &elem)
{
    const T *res = TbElem_cast<T> (&elem);
    if (!res)
    {
        const char *ElemName = elem.templ()?elem.templ()->name():"Unknown element";
        throw TbElem_badcast("ROMAN",__FILE__,__LINE__, ElemName);
    }
    return *res;
}

template <class T>
         const T & TbElem_cast (const char *nick, const char *file, int line,
                                      const TbElement &elem)
{
    const T *res = TbElem_cast<T> (&elem);
    if (!res)
    {
        const char *ElemName = elem.templ()?elem.templ()->name():"Unknown element";
        throw TbElem_badcast(nick, file, line, ElemName);
    }
    return *res;
}

} // namespace typeb_parser
#endif /*_TYPEB_CAST_H_*/
