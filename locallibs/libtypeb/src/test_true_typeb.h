//
// C++ Interface: test_true_typeb
//
// Description:
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
//
//
#ifndef _AIRIMP_TEMPLATE_H_
#define _AIRIMP_TEMPLATE_H_
#include "typeb_template.h"
#include "tb_elements.h"

namespace typeb_parser
{

class AIRIMP_template : public typeb_template
{
public:
    AIRIMP_template();
};

class SimpleElem : public TbElement
{
    std::string Source;
    public:
        SimpleElem(const std::string &src) { Source = src; }

        const std::string &source() const { return Source; }
};

typedef SimpleElem ReclocElem;
typedef SimpleElem NameElem;
typedef SimpleElem SegmElem;
typedef SimpleElem SsrElem;

class ReclocTemplate : public tb_descr_nolexeme_element
{
    virtual const char * name() const { return "recloc"; }
    virtual bool isItYours(const std::string &, std::string::size_type *till) const;
    virtual const char * accessName() const { return "recloc"; }
    virtual TbElement * parse(const std::string &text) const;
};

class Name2Template : public tb_descr_nolexeme_element
{
    virtual const char * name() const { return "name"; }
    virtual bool isItYours(const std::string &, std::string::size_type *till) const;
    virtual const char * accessName() const { return "name"; }
    virtual TbElement * parse(const std::string &text) const;
};

class SegmTemplate : public tb_descr_nolexeme_element
{
    virtual const char * name() const { return "itin"; }
    virtual bool isItYours(const std::string &, std::string::size_type *till) const;
    virtual const char * accessName() const { return "itin"; }
    virtual TbElement * parse(const std::string &text) const;
};

class SsrTemplate : public tb_descr_multiline_element
{
    virtual const char * name() const { return "ssr"; }
    virtual const char * lexeme() const { return "SSR"; }
    virtual const char * nextline_lexeme() const { return "SSR"; }
//     virtual bool isItYours(const std::string &) const;
    virtual const char * accessName() const { return "ssr"; }
    virtual TbElement * parse(const std::string &text) const;
};

}

#endif /*_AIRIMP_TEMPLATE_H_*/
