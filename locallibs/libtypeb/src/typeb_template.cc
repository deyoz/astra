/*
*  C++ Implementation: typeb_template
*
* Description: Шаблоны разборщиков сообщений
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include <limits>

#include "typeb_template.h"
#include "typeb_parse_exceptions.h"
#include "serverlib/str_utils.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser{

using namespace boost;

std::map< std::string, typeb_template::ptr_t > typeb_template::Templates;


tb_descr_element::tb_descr_element()
    :Necessary(false), MayBeInline(false), MaxNumOfRepetition(1), MinNumOfRepetition(0)
{
}

tb_descr_element * base_templ_group::addElement(tb_descr_element::ptr_t elem)
{
    Elements.push_back(elem);
    return Elements.back().get();
}

tb_descr_element * base_templ_group::addElement(tb_descr_element * elem)
{
    return addElement(tb_descr_element::ptr_t(elem));
}

tb_descr_element *tb_descr_element::setNecessary()
{
    Necessary = true;
    return this;
}

bool tb_descr_element::necessaty() const
{
    return Necessary;
}

tb_descr_element * tb_descr_element::setMayBeInline()
{
    MayBeInline = true;
    return this;
}

bool tb_descr_element::mayBeInline() const
{
    return MayBeInline;
}

unsigned tb_descr_element::maxNumOfRepetition() const
{
    return MaxNumOfRepetition;
}

unsigned tb_descr_element::minNumOfRepetition() const
{
    return MinNumOfRepetition;
}


tb_descr_element * tb_descr_element::setMaxNumOfRepetition(unsigned num)
{
    if(num == 0)
    {
        throw typeb_except(STDLOG, "Invalid max number or repetition");
    }
    MaxNumOfRepetition = num;
    return this;
}

tb_descr_element * tb_descr_element::setMinNumOfRepetition(unsigned num)
{
    if(num == 0)
    {
        throw typeb_except(STDLOG, "Invalid min number or repetition 0. "
                           "Use Necessary=false instead.");
    }
    if(num && this->necessaty()==false)
    {
        throw typeb_except(STDLOG,
                           "Invalid combination of min number or repetition >0 "
                           "and Necessary=false.");
    }

    MinNumOfRepetition = num;
    return this;
}

tb_descr_element * tb_descr_element::setUnlimRep()
{
    MaxNumOfRepetition = std::numeric_limits<unsigned>::max();
    return this;
}

//=====================
typeb_template::typeb_template(const std::string & ID, const std::string & descr)
    :MessageID(ID),Description(descr)
{
    if(ID.size() != IdLength && ID.size() > 0)
    {
        throw typeb_except(STDLOG, "Invalid MessageID element");
    }
}

void typeb_template::addTemplate(typeb_template * templ)
{
    if(templ->elements().empty())
    {
        ProgError(STDLOG, "Empty template for %s", templ->MessageID.c_str());
        throw typeb_except(STDLOG, "You are trying to add an empty template!");
    }
    Templates.insert(make_pair(templ->MessageID, typeb_template::ptr_t(templ)));
}

const typeb_template * typeb_template::findTemplate(const std::string &msg_type)
{
    TemplHash_t::const_iterator i = Templates.find(msg_type);
    if(i == Templates.end())
    {
        WriteLog(STDLOG,"Template for typeb message '%s' not found", msg_type.c_str());
        return 0;
    }
    return (*i).second.get();
}

bool tb_descr_element::isSingle() const
{
    return ( type() == SingleElement );
}

bool tb_descr_element::isMultiline() const
{
    return ( type() == SingleMultilineElement );
}

bool tb_descr_element::isGroup() const
{
    return ( type() == GroupOfElements );
}

bool tb_descr_element::isSwitch() const
{
    return ( type() == SwitchElement );
}

bool tb_descr_element::isArray() const
{
    return (maxNumOfRepetition()>1);
}

bool tb_descr_element::hasLexeme() const
{
    return true;
}

bool tb_descr_nolexeme_element::hasLexeme() const
{
    return false;
}

bool tb_descr_element::isItYours(const std::string &, std::string::size_type *till) const
{
    return false;
}

bool tb_descr_element::check_nextline(const std::string &, const std::string &, size_t *start, size_t *till) const
{
    return false;
}

const char * templ_group::accessName() const
{
    if(!acc_name.empty())
    {
        return acc_name.c_str();
    }
    if(!elements().empty())
    {
        acc_name = std::string(elements().front()->accessName()) + "Gr";
    }
    else
    {
        LogError(STDLOG) << "Template group has no elements";
        throw typeb_except(STDLOG, "Add elements into group first");
    }

    return acc_name.c_str();
}

void templ_group::setAccessName(const char * accN)
{
    acc_name = accN;
}

base_templ_group* base_templ_group::setMayMixElements()
{
    MayMixElements=true;
    return this;
}

bool tb_descr_multiline_element::check_nextline(const std::string &whole_str, const std::string &curr_line,
                                                size_t *start, size_t *till) const
{
    std::string nextlineLexeme = nextline_lexeme();
    std::string currLine = StrUtils::ltrim(curr_line);

    if (currLine.substr(0, nextlineLexeme.length()) == nextlineLexeme) {
        *start = nextlineLexeme.length();
        return true;
    }

    return false;
}

} // namespace typeb_parser
