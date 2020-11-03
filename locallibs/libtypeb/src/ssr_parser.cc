/*
*  C++ Implementation: ssr_parser
*
* Description: SSR parsers
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/

#include "ssr_parser.h"
#include "ssr_tknx.h"
#include "ssr_asvx.h"
#include "RemTkneElem.h"
#include "RemFoidElem.h"
#include "RemFqtxElem.h"
#include "RemDocsElem.h"

#include "typeb_msg.h"
#include "typeb_parse_exceptions.h"
#include <serverlib/str_utils.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/test.h>

namespace typeb_parser
{
    using namespace std;
    SsrParsers::SsrTypePrsMap *SsrParsers::SsrTypePrs=0;

    const std::string SsrTypes::Tknx = "TKNX";
    const std::string SsrTypes::Asvx = "ASVX";
    const std::string RemarkTypes::Foid = "FOID";
    const std::string RemarkTypes::Tkne = "TKNE";
    const std::string RemarkTypes::Fqtv = "FQTV";
    const std::string RemarkTypes::Fqtu = "FQTU";
    const std::string RemarkTypes::Fqtr = "FQTR";
    const std::string RemarkTypes::Docs = "DOCS";

    const std::string Ssr::SsrTag = "SSR";
    const std::string Remark::RemTag = ".R/";

Ssr * SsrParsers::parse (const std::string &remType, const std::string &txt)
{
    pair<string, string> CodeText = SsrParsers::separateCodeAndText(txt);
    SsrPrsMap::const_iterator i = prsMap(remType).find(CodeText.first);
    if(i == prsMap(remType).end())
    {
        throw typeb_parse_except(STDLOG,TBMsg::UNKNOWN_SSR_CODE,
                           (string("Unknown SSR code ")+CodeText.first));
    }

    pair<string, boost::shared_ptr<NameElem> > TextAndPax;
    if( i->second.parse_pax )
        TextAndPax = SsrParsers::separateTextAndPax(CodeText.second);
    else
        TextAndPax = std::make_pair(CodeText.second, boost::shared_ptr<NameElem> () );

    Ssr * ssr = i->second.parse(CodeText.first, TextAndPax.first);

    if(ssr)
        ssr->setPaxReference(TextAndPax.second);

    return ssr;
}

std::pair<std::string, std::string>
        SsrParsers::separateCodeAndText(const std::string & text)
{
    pair<string, string> ret_pair;

    std::string::size_type nonspace = StrUtils::lNonSpacePos(text);
    if(nonspace == string::npos || nonspace == text.size())
    {
        throw typeb_parse_except(STDLOG, TBMsg::UNKNOWN_SSR_CODE,
                           "Empty SSR body. There is no SSR code!");
    }
    ret_pair.first = StrUtils::delSpaces(text.substr(nonspace, SsrParsers::CodeLen));
    if(ret_pair.first.size() != SsrParsers::CodeLen)
    {
        throw typeb_parse_except(STDLOG, TBMsg::UNKNOWN_SSR_CODE,
                           ("Wrong SSR code! "+ret_pair.first));
    }

    nonspace = StrUtils::lNonSpacePos(text, nonspace+SsrParsers::CodeLen);
    if(nonspace == string::npos || nonspace == text.size())
//         throw typeb_parse_except(STDLOG, TBMsg::EMPTY_SSR_BODY,
//                            "Empty SSR body!");
        ret_pair.second = string();
    else
        ret_pair.second = text.substr(nonspace);

    ProgTrace(TRACE3,"SSR Code: <%s>, SSR Text: '%s'",
              ret_pair.first.c_str(), ret_pair.second.c_str());
    return ret_pair;
}

std::pair<std::string, boost::shared_ptr<NameElem> >
        SsrParsers::separateTextAndPax(const std::string & text)
{
    boost::shared_ptr<NameElem> Pax;
    std::string NewText;
    std::string::size_type PaxPos = text.find("-");
    if(PaxPos != std::string::npos)
    {
        // Name Element Presents
        // .R/TKNE HK1 2621000002513/1-1VETROVA/ELENA
        // ---------------------------^
        NewText = text.substr(0, PaxPos);
        std::string PaxText = text.substr(PaxPos+1);
        Pax.reset( NameElem::parse(PaxText) );
    }
    else
    {
        NewText = text;
    }

    LogTrace(TRACE3) << "SSR Text: <" << NewText << ">";
    if(Pax)
        LogTrace(TRACE3) << "SSR Name: <" << *Pax<< ">";

    return make_pair(NewText, Pax);
}

SsrParsers::SsrParsers()
{
    addParser(Ssr::SsrTag, SsrTypes::Tknx, SsrTknx::parse);
    addParser(Ssr::SsrTag, SsrTypes::Asvx, SsrAsvx::parse);
    addParser(Remark::RemTag, RemarkTypes::Foid, RemFoidElem::parse);
    addParser(Remark::RemTag, RemarkTypes::Tkne, RemTkneElem::parse);
    addParser(Remark::RemTag, RemarkTypes::Fqtv, RemFqtxElem::parse, false /* parse PAX automaticaly */);
    addParser(Remark::RemTag, RemarkTypes::Fqtu, RemFqtxElem::parse, false);
    addParser(Remark::RemTag, RemarkTypes::Fqtr, RemFqtxElem::parse, false);
    addParser(Remark::RemTag, RemarkTypes::Docs, RemDocsElem::parse, false);
}

SsrParsers::SsrPrsMap & SsrParsers::prsMap(const std::string & type)
{
    if(!SsrTypePrs)
        SsrTypePrs = new SsrTypePrsMap();
    SsrTypePrsMap::const_iterator i = SsrTypePrs->find(type);
    SsrPrsMapPtr pPrsMap;
    if(i == SsrTypePrs->end())
    {
        pPrsMap = SsrPrsMapPtr(new SsrPrsMap);
        (*SsrTypePrs)[type] = pPrsMap;
    } else {
        pPrsMap = i->second;
    }

    return *pPrsMap;
}

}
