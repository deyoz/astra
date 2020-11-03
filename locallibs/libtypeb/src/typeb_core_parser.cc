/*
*  C++ Implementation: typeb_core_parser
*
* Description: ядро typeb парсера
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include <iomanip>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/regex.hpp>
#include <boost/scoped_ptr.hpp>
#include <serverlib/string_cast.h>
#include <serverlib/str_utils.h>
#include <serverlib/string_cast.h>
#include <serverlib/dates_io.h>

#define _TYPEB_CORE_PARSER_SECRET_ "ПРЕВЕД"
#include "typeb_core_parser.h"
#include "typeb_template.h"
#include "typeb_msg.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
    using namespace std;
    using namespace Ticketing;


TypeBMessage TypeBMessage::parse(const std::string & message)
{
    TypeBMessage tb_msg(message);
    tb_msg.split2lines();

    // Адреса
    tb_msg.Addrs = parseAddr(message);

    if(tb_msg.msgline(2).length() != TBCfg::MsgTypeLen) {
        tb_msg.Type = "";
        tb_msg.Msg.CurrMsgLine = 2;
    } else {
        tb_msg.Type = tb_msg.msgline(2);
        tb_msg.Msg.CurrMsgLine = 3;
    }
    tb_msg.Template = typeb_template::findTemplate(tb_msg.Type);

    if(tb_msg.Template == 0)
    {
        //Maybe COR or PDM spoils our tlg? Check next line
        if(tb_msg.msgline(3).length() != TBCfg::MsgTypeLen) {
            tb_msg.Type = "";
            tb_msg.Msg.CurrMsgLine = 3;
        } else {
            tb_msg.Type = tb_msg.msgline(3);
            tb_msg.Msg.CurrMsgLine = 4;
        }
        tb_msg.Template = typeb_template::findTemplate(tb_msg.Type);

        if(tb_msg.Template == 0) {
            //damn.. this one is wrong too
            throw typeb_parse_except(STDLOG, TBMsg::UNKNOWN_MSG_TYPE,
                    "There is no parser for message '" + tb_msg.Type + "'",
                    tb_msg.Msg.CurrMsgLine+1,1);
        }
    }

    LogTrace(TRACE3) << "Found parser for message '" <<
            tb_msg.Type << "' - " << tb_msg.Template->description();

    tb_msg.check_message_general_rules();

    try
    {
        TypeBMessage::parse_group(*tb_msg.Template, tb_msg.Msg, &tb_msg);
    }
    catch(typeb_parse_except &e)
    {
        e.setErrLine(tb_msg.Msg.CurrMsgLine+1);
        e.setErrPos(tb_msg.Msg.CurrLinePos+1);
        e.setErrLength(tb_msg.Msg.CurrPrsLength);
        throw;
    }

    if (tb_msg.Msg.CurrMsgLine < tb_msg.Msg.totalLines())
    {
        throw typeb_parse_except(STDLOG, TBMsg::UNKNOWN_MESSAGE_ELEM, tb_msg.Msg.CurrMsgLine+1);
    }
    return tb_msg;
}

AddrHead TypeBMessage::parseAddr(const std::string &message)
{
    TypeBMessage tb_msg(message);

    tb_msg.split2lines();
    if (tb_msg.Msg.totalLines()<2) {
        throw typeb_parse_except(STDLOG, TBMsg::INV_ADDRESSES,
                           "Too few lines in a message");
    }

    return AddrHead::parse(tb_msg.Msg[0], tb_msg.Msg[1]);
}

TypeBHeader TypeBMessage::parseHeader(const AddrHead &addr, const std::string &message)
{
    TypeBMessage tb_msg(message);

    tb_msg.split2lines();
    if (tb_msg.Msg.totalLines()<4) {
        throw typeb_parse_except(STDLOG, TBMsg::INV_ADDRESSES,
                           "Too few lines in a message");
    }

    TypeBHeader hdr("", "", addr, 0, false, "");

    size_t msgTypeIdx = 0;
    for (size_t i = 0; i < tb_msg.Msg.totalLines(); ++i) {
        static const boost::regex rex(" *[A-Z]{3} *");
        if (boost::regex_match(tb_msg.Msg[i], rex)) {
            msgTypeIdx = i;
        }
    }
    const typeb_template* tpl = NULL;
    if (msgTypeIdx) {
        tpl = typeb_template::findTemplate(StrUtils::trim(tb_msg.Msg[msgTypeIdx]));
        if (tpl) {
            if (tpl->multipart()) {
                tpl->fillHeader(hdr, addr, msgTypeIdx + 1, tb_msg);
                return hdr;
            }
        }
    }
    return hdr;
}

AddrHead AddrHead::parse(const std::string & first, const std::string & second)
{
    std::list<string> address_arr;
    boost::split(address_arr, first, boost::algorithm::is_any_of(" "));
    if(!address_arr.empty() && address_arr.back().empty())
    {
        address_arr.pop_back();
    }

    if(!address_arr.empty() && address_arr.front().size() == 2)
    {
        LogTrace(TRACE3) << "Priorety code presents: " << address_arr.front();
        address_arr.pop_front();
    }

    if(address_arr.empty())
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_ADDRESSES,
                                 "Empty destination address element");
    }

    for(std::list<string>::iterator i = address_arr.begin(); i != address_arr.end(); ++i)
    {
        if(i->size() != AddrHead::AddrLen)
        {
            throw typeb_parse_except(STDLOG, TBMsg::INV_ADDRESSES,
                                 "Invalid destination address element");
        }
    }

    if(second.empty() || second.at(0) != '.' || second.length() < 1+AddrHead::AddrLen)
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_ADDRESSES,
                               "Sender field is invalid; <" +
                                       second
                                       +"> length=%d" +
                                       HelpCpp::string_cast(second.length()));
    }

    LogTrace(TRACE3) << "from field is <" << second << ">";
    string Sender;
    string MsgId;
    if(second.length()-1 == AddrHead::AddrLen){
        Sender = second.substr(1);
    } else {
        vector<string> splt;
        boost::split(splt, second, boost::is_any_of(" "));
        if (splt.empty())
            throw typeb_parse_except(STDLOG, TBMsg::INV_ADDRESSES, second);
        if (splt[0].size() != AddrHead::AddrLen + 1 && splt[0].size() != AddrHead::AddrLen + 2)
            throw typeb_parse_except(STDLOG, TBMsg::INV_ADDRESSES, second);
        Sender = splt[0].substr(1);
        for(size_t i = 1; i < splt.size(); ++i)
            MsgId += splt[i];
        MsgId =  StrUtils::trim(MsgId);
    }

    return AddrHead(Sender,address_arr,MsgId);
}


bool TypeBMessage::is_it_typeb(const std::string & tlg)
{
    return true;
}

void TypeBMessage::split2lines()
{
    boost::split(Msg.Msgl, SrcMessage, boost::algorithm::is_any_of("\n"));

    if(!Msg.Msgl.empty() && Msg.Msgl[Msg.Msgl.size()-1].empty())
    {
        Msg.Msgl.pop_back();
    }

    LogTrace(TRACE3) << Msg.totalLines() << " lines in the message";
}

void TypeBMessage::check_message_general_rules()
{
    for(unsigned i=0; i<Msg.Msgl.size(); ++i)
    {
        LogTrace(TRACE2) << "Msg["<< setw(3) << i << "]:" << Msg.Msgl[i];
        if(Msg.Msgl[i].size() > TBCfg::MaxLineLen)
        {
            throw typeb_parse_except(STDLOG, TBMsg::TOO_LONG_MSG_LINE,
                               "Maximum typeb message line length exceed", i+1);
        }
        else if (Msg.Msgl[i].empty())
        {
            throw typeb_parse_except(STDLOG, TBMsg::EMPTY_MSG_LINE,
                                     "Empty msg line", i+1);
        }
    }
}

std::string::size_type
        TypeBMessage::GetNextMsgPosition(const typeb_template::const_elem_iter &iter_in,
                                         const typeb_template::const_elem_iter &enditer,
                                         const std::string &currline,
                                         std::string::size_type start_from)
{
    typeb_template::const_elem_iter iter = iter_in;
    string::size_type min_pos, pos = min_pos = string::npos;

    if(!(*iter)->hasLexeme() && iter != enditer) {
        ++iter;
    }
    while( iter != enditer && (*iter)->mayBeInline() && (*iter)->hasLexeme())
    {
        const std::string lex = (*iter)->lexeme();
        pos = currline.find(lex, start_from);
        if(pos != string::npos && pos < min_pos)
        {
            LogTrace(TRACE5) << "Pos == " << pos;
            min_pos = pos;
        }
        ++iter;
    }
    LogTrace(TRACE5) << "min_pos == " <<
            (min_pos == std::string::npos ? "string::npos" : HelpCpp::string_cast (min_pos));
    return min_pos;
}

/// @todo handle multiline elements
/// @todo handle inline elements
/// @todo throw if mandatory element missed in group with mix elements
TbGroupElement::ptr_t TypeBMessage::parse_group(const base_templ_group &templ,
                                          MsgLines_t &Msg,
                                          TbGroupElement *group_ptr)
{
    TbGroupElement::ptr_t group_ptr_inner;
    TbGroupElement *pgroup;
    if(!group_ptr){
        group_ptr_inner.reset( new TbGroupElement );
        pgroup = group_ptr_inner.get();
    } else {
        pgroup = group_ptr;
    }
    TbGroupElement &group = *pgroup;
    group.setTemplate(&templ);

    typeb_template::tb_descr_element_t elems = templ.elements();
    typeb_template::elem_iter i = elems.begin();

    bool end_of_the_group=false;
    unsigned count=0;
    for (; (i!=elems.end() && Msg.CurrMsgLine < Msg.totalLines()); ++count)
    {
        unsigned msgline = Msg.CurrMsgLine;
        std::string::size_type msglinepos = Msg.CurrLinePos;

        const tb_descr_element * const elem = (*i).get();
        LogTrace(TRACE3) << "Current template element is " <<
                "[" << elem->lexeme() << "] " << elem->name();

        LogTrace(TRACE5) <<
                "mayBeInline: " << elem->mayBeInline()  << "; " <<
                "isArray: "     << elem->isArray()      << "; " <<
                "isSingle: "    << elem->isSingle()     << "; " <<
                "isGroup: "     << elem->isGroup()      << "; " <<
                "isMultiline: " << elem->isMultiline()  << ";";

        LogTrace(TRACE3) << "mayMixElements: " << templ.mayMixElements();

        typeb_template::elem_iter iter_search = i;
        if(templ.mayMixElements())
            iter_search = elems.begin();

        if(elem->isArray())
        {
            TbListElement::ptr_t arr_ptr = parse_array(iter_search, elems.end(), *elem, Msg);
            if(arr_ptr.get()) {
                LogTrace(TRACE3) << "Add Array of Elements";
                group.addElement(arr_ptr);
            }
        }
        else if(elem->isSingle() || elem->isMultiline())
        {
            TbElement::ptr_t prs_elem =
                    parse_singleline_element(iter_search, elems.end(), *elem, Msg);
            if(prs_elem.get()) {
                LogTrace(TRACE3) << "Add Element";
                group.addElement(prs_elem);
            }
        }
        else if(elem->isGroup())
        {
            const base_templ_group &templ_grp_elem =
                    dynamic_cast<const base_templ_group &>(*elem);
            TbGroupElement::ptr_t prs_elem = parse_group(templ_grp_elem, Msg);
            if(prs_elem.get()) {
                LogTrace(TRACE3) << "Add Group of Elements";
                group.addElement(prs_elem);
            }
        }
        else if (elem->isSwitch())
        {
            const templ_switch_group &templ_sw =
                    dynamic_cast<const templ_switch_group &>(*elem);
            TbElement::ptr_t prs_elem = parse_switch_elem(templ_sw, Msg);
            if(prs_elem.get())
            {
                LogTrace(TRACE3) << "Add switched element {" << prs_elem->name() << "}";
                group.addElement(prs_elem);
            }
        }
        else
        {
            throw typeb_parse_except(STDLOG,TBMsg::PROG_ERR,
                               "Unknown element type in template");
        }

        if(msgline == Msg.CurrMsgLine && msglinepos == Msg.CurrLinePos)
        {
            if (!count && !group_ptr) // если это не корневая группа == сообщение
            {
                // Пропущен первый элемент группы
                // верный признак того что пора сваливать...
                end_of_the_group = true;
                break;
            }
            else if (elem->necessaty() == true && !templ.mayMixElements())
            {
                // Пропущен обязательный элемент
                throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT,
                                   elem->name(), Msg.CurrMsgLine+1);
            }
            else
            {
                LogTrace(TRACE3) << "Skip unnecessary element";
            }
            ++i;
        }
        else
        {
            if(templ.mayMixElements())
            {
                if(elem->maxNumOfRepetition()==1)
                    elems.erase(i);
                i = elems.begin();
            }
            else
            {
                ++i;
            }
        }
    }

    // Проверка, не пропущен ли обязательный элемент
    if(!end_of_the_group){
        for (; i!=elems.end(); ++i)
        {
            if((*i)->necessaty())
            {
                throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT,
                                   string("Missed necessaty element: ") +
                                           (*i)->name(), Msg.CurrMsgLine+1, Msg.CurrLinePos);
            }
        }
    }

    LogTrace(TRACE3) << "Group done.";
    return group_ptr_inner;
}

TbElement::ptr_t TypeBMessage::parse_switch_elem(const templ_switch_group & templ,
        MsgLines_t & Msg)
{
    TbElement::ptr_t elem_ptr;
    typeb_template::const_elem_iter i = templ.elements().begin();

    unsigned msgline = Msg.CurrMsgLine;
    std::string::size_type msglinepos = Msg.CurrLinePos;

    for (; (i!=templ.elements().end() && Msg.CurrMsgLine < Msg.totalLines()); ++i)
    {
        const tb_descr_element * const elem = (*i).get();
        LogTrace(TRACE3) << "Current template element is " <<
                "[" << elem->lexeme() << "] " << elem->name();

        LogTrace(TRACE5) <<
                "mayBeInline: " << elem->mayBeInline()  << "; " <<
                "isArray: "     << elem->isArray()      << "; " <<
                "isSingle: "    << elem->isSingle()     << "; " <<
                "isGroup: "     << elem->isGroup()      << "; " <<
                "isMultiline: " << elem->isMultiline()  << ";";

        if(elem->isArray())
        {
            elem_ptr = parse_array(i,templ.elements().end(),
                            *elem, Msg);
            if(elem_ptr.get()) {
                LogTrace(TRACE3) << "Add Array of Elements";
            }
        }
        else if(elem->isSingle() || elem->isMultiline())
        {
            elem_ptr =
                    parse_singleline_element(i,templ.elements().end(), *elem, Msg);
            if(elem_ptr.get()) {
                LogTrace(TRACE3) << "Add Element";
            }
        }
        else if(elem->isGroup())
        {
            const base_templ_group &templ_grp_elem =
                    dynamic_cast<const base_templ_group &>(*elem);
            elem_ptr = parse_group(templ_grp_elem, Msg);
            if(elem_ptr.get()) {
                LogTrace(TRACE3) << "Add Group of Elements";
            }
        }
        else
        {
            throw typeb_parse_except(STDLOG,TBMsg::PROG_ERR,
                               "Unknown element type in template");
        }
        if(msgline != Msg.CurrMsgLine ||
           msglinepos != Msg.CurrLinePos)
        {
            break;
        }
    }

    LogTrace(TRACE3) << "Switch done.";
    return elem_ptr;
}

TbListElement::ptr_t
        TypeBMessage::parse_array(const typeb_template::const_elem_iter &iter_in,
                                  const typeb_template::const_elem_iter &enditer,
                                  const tb_descr_element &templ,
                                  MsgLines_t &msgl)
{
    TbListElement::ptr_t array(new TbListElement);
    LogTrace(TRACE3) << "Parsing array of elements - " << templ.name();

    array->setTemplate(&templ);

    ///@todo delete code duplication
    if(templ.isGroup())
    {
        const base_templ_group &templ_grp_elem =
                dynamic_cast<const base_templ_group &>(templ);

        for( unsigned count = 0; (count < templ.maxNumOfRepetition() &&
             msgl.CurrMsgLine < msgl.totalLines());
             count ++)
        {
            unsigned l = msgl.CurrMsgLine;
            std::string::size_type p = msgl.CurrLinePos;

            LogTrace(TRACE5) << "Trying add " << templ.name() << "[" << count << "]";

            TbGroupElement::ptr_t prs_elem = parse_group(templ_grp_elem, msgl);

            if (l==msgl.CurrMsgLine && p==msgl.CurrLinePos)
            {
                // Ничего не проинсертили
                break;
            } else
            {
                if(prs_elem.get()) {
                    LogTrace(TRACE3) << "Add Group of Element";
                    array->addElement(prs_elem);
                }
            }
        }
    }
    else if( templ.isSingle() || templ.isMultiline())
    {
        for( unsigned count = 0; (count < templ.maxNumOfRepetition() &&
             msgl.CurrMsgLine < msgl.totalLines());
             count ++)
        {
            unsigned l = msgl.CurrMsgLine;
            std::string::size_type p = msgl.CurrLinePos;

            LogTrace(TRACE3) << "Trying add " << templ.name() << "[" << count << "]";

            TbElement::ptr_t prs_elem = parse_singleline_element(iter_in, enditer,
                    templ, msgl);

            if(prs_elem.get()) {
                LogTrace(TRACE3) << "Add Element";
                array->addElement(prs_elem);
            }
            else if (l==msgl.CurrMsgLine && p==msgl.CurrLinePos)
            {
                // Ничего не проинсертили
                break;
            } /*else {
               Просто в парсере не захотели вставлять этот элемент в
               сообщение
            }*/
        }
    }
    else
    {
        throw typeb_parse_except(STDLOG, TBMsg::PROG_ERR, "Array of arrays?!");
    }

    if(array->total() < templ.minNumOfRepetition())
    {
        throw typeb_parse_except(STDLOG, TBMsg::TOO_FEW_ELEMENTS,
                           HelpCpp::string_cast(templ.minNumOfRepetition()) +
                                   " as a minimum");
    }
    else if(array->empty())
    {
        if(templ.necessaty())
        {
            throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT,
                               string("Missed necessaty element: ") + templ.name());
        }
        array.reset();
    }
    LogTrace(TRACE3) << "Array Done.";

    return array;
}

void TypeBMessage::addNextFewLines(const tb_descr_element & templ,
                                   const typeb_template::const_elem_iter &iter_in,
                                   const typeb_template::const_elem_iter &enditer,
                                   MsgLines_t & msgl, std::string &str, string::size_type &end_pos)
{
    string::size_type start_pos = 0;
    unsigned currMsgLine = msgl.CurrMsgLine;
    string::size_type end_pos_tmp = std::string::npos;

    while(msgl.totalLines() > currMsgLine+1
          && templ.check_nextline(str, msgl[currMsgLine + 1], &start_pos, &end_pos_tmp))
    {
        end_pos = end_pos_tmp;
        currMsgLine ++;
        if(end_pos == std::string::npos) {
            // если нам не обозначили конец элемента. или если он действительно до конца строки
            end_pos = GetNextMsgPosition(iter_in, enditer, msgl[currMsgLine], 0);
        }
        if(end_pos >= msgl[currMsgLine].length()) {
            end_pos = std::string::npos;
        }
        str += StrUtils::trim(msgl[currMsgLine].substr(start_pos, end_pos - start_pos));

        start_pos = 0;
        end_pos_tmp = std::string::npos;
        if(end_pos != std::string::npos)
            break;
    }

    msgl.CurrMsgLine = currMsgLine;
    LogTrace(TRACE3) << "result multiline element: " << str;
}

TbElement::ptr_t
        TypeBMessage::parse_singleline_element(const typeb_template::const_elem_iter &iter_in,
                                 const typeb_template::const_elem_iter &enditer,
                                 const tb_descr_element & templ,
                                 MsgLines_t & msgl)
{
    TbElement::ptr_t prs_elem;
    const string &currLine = msgl[msgl.CurrMsgLine];
    const string currLexeme= templ.lexeme();
    bool isItOurElem = false;
    string::size_type pos;
    string currParsedStr;

    if(templ.hasLexeme())
    {
        LogTrace(TRACE5) << "currTempl Lexeme: [" <<
                currLexeme << "] currline Lexeme: [" <<
                currLine.substr(msgl.CurrLinePos, currLexeme.size()) << "]";

        if(currLine.substr(msgl.CurrLinePos, currLexeme.size()) == currLexeme)
        {
            isItOurElem = true;

            pos = GetNextMsgPosition(iter_in, enditer,
                                     currLine, msgl.CurrLinePos+currLexeme.size());
            currParsedStr =
                    StrUtils::rtrim(currLine.substr(msgl.CurrLinePos+currLexeme.size(),
                                    pos - currLexeme.size() - msgl.CurrLinePos));

            // В случае если текущий эл-т может иметь продолжение на след строке
            if(templ.isMultiline() && pos == string::npos)
                addNextFewLines(templ, iter_in, enditer, msgl, currParsedStr, pos);
        }
    }
    else
    {
        std::string::size_type till=0;
        pos = GetNextMsgPosition(iter_in, enditer,
                                 currLine, msgl.CurrLinePos);
        currParsedStr =
                StrUtils::trim(currLine.substr(msgl.CurrLinePos+currLexeme.size(),
                               pos - currLexeme.size() - msgl.CurrLinePos));
        isItOurElem = templ.isItYours(currParsedStr, &till);

        if(isItOurElem) {
            if(till && till != string::npos)
            {
                currParsedStr = currParsedStr.substr(0,till);
                pos = till+msgl.CurrLinePos;
                if(pos == currLine.size())
                    pos = string::npos;
            }

            if(templ.isMultiline() && pos == string::npos)
                addNextFewLines(templ, iter_in, enditer, msgl, currParsedStr, pos);
        }
    }

    if(isItOurElem)
    {
        LogTrace(TRACE5) << "Parsing string: {" << currParsedStr << "}";

        msgl.CurrPrsLength = // length of parsed string
                    (((pos == string::npos || pos < msgl.CurrLinePos)?currLine.length():pos) - msgl.CurrLinePos);

        while(msgl.CurrPrsLength > 0 && currLine[msgl.CurrPrsLength-1] == ' ')
            msgl.CurrPrsLength --;

        prs_elem.reset(templ.parse(currParsedStr));

        if(prs_elem)
        {
            prs_elem->setTemplate(&templ);
            prs_elem->setLine(msgl.CurrMsgLine+1);
            prs_elem->setPos(msgl.CurrLinePos+1);
            prs_elem->setLength(msgl.CurrPrsLength);
            prs_elem->setLineSrc(currParsedStr);

            LogTrace(TRACE5) <<
                    "Elem: "   << prs_elem->name() <<
                    "; Line: " << prs_elem->line() <<
                    "; Pos: "  << prs_elem->position() <<
                    "; Length:"<< prs_elem->length();
        }
        if(pos != string::npos)
        {
            msgl.CurrLinePos = pos;
        }
        else
        {
            ++msgl.CurrMsgLine;
            msgl.CurrLinePos = 0;
        }
        msgl.CurrPrsLength = 0;
    }// По лексеме не подошел
    else
    {
        LogTrace(TRACE3) << "No";
    }
    return prs_elem;
}

std::ostream & operator << (std::ostream& os, const AddrHead &addr)
{
    bool first = true;
    os << "src: " << addr.sender();
    os << " rcv: ";
    for(std::list<std::string>::const_iterator iter = addr.receiver().begin();
        iter != addr.receiver().end();
        iter ++, first = false)
        os << (first ? "" : "/") << *iter;

    if(!addr.msgId().empty())
        os << "  id: " << addr.msgId();

    return os;
}

std::ostream & operator << (std::ostream& os, const TypeBMessage &msg)
{
    os << msg.addrs() << "\n";
    os << msg.type() << "\n";

    for(TbListElement::const_iterator iter = msg.begin(); iter != msg.end(); iter ++)
    {
        (*iter)->print(os, 0);
    }

    return os;
}

}
