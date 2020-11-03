//
// C++ Interface: ssr_parser
//
// Description: SSR parsers
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _SSR_PARSER_H_
#define _SSR_PARSER_H_

#include <string>
#include <map>
#include <utility>
#include <boost/shared_ptr.hpp>
#include "tb_elements.h"
#include "NameElem.h"

namespace typeb_parser
{
    struct SsrTypes
    {
        static const std::string Tknx; // ET
        static const std::string Asvx; // EMD
    };

    struct RemarkTypes
    {
        static const std::string Foid; // Form of identification
        static const std::string Tkne; // Electr ticket
        static const std::string Fqtv;
        static const std::string Fqtu;
        static const std::string Fqtr;
        static const std::string Docs;
    };

    class Ssr : public TbElement
    {
        std::string Code;
        std::string Text;

        boost::shared_ptr<NameElem> PaxReference;
    public:
        static const std::string SsrTag;
        static const std::string RemTag;
        typedef boost::shared_ptr<Ssr> SharedPtr;

        Ssr(const std::string &code, const std::string &text)
            :Code(code), Text(text)
        {
        }
        void setPaxReference(const boost::shared_ptr<NameElem> &pax) { PaxReference = pax; }
        const std::string &code() const { return Code; }
        const std::string &text() const { return Text; }
        const NameElem * paxReference() const { return PaxReference.get(); }
        virtual ~Ssr() {}
    };
    typedef Ssr Remark;

    class SsrParsers
    {
        typedef Ssr * (*fparse) (const std::string &, const std::string &);
        struct prs_struct
        {
            fparse parse;
            bool parse_pax;
            prs_struct(fparse fp, bool parse_pax_)
                :parse(fp), parse_pax(parse_pax_)
            {
            }
        };
        typedef std::map <std::string, prs_struct> SsrPrsMap;
        typedef boost::shared_ptr<SsrPrsMap> SsrPrsMapPtr;
        typedef std::map <std::string, SsrParsers::SsrPrsMapPtr> SsrTypePrsMap;
        static SsrTypePrsMap *SsrTypePrs;
        static SsrPrsMap & prsMap(const std::string &type);
        static std::pair<std::string, std::string>
                separateCodeAndText(const std::string &text);
        static std::pair<std::string, boost::shared_ptr<NameElem> >
                separateTextAndPax(const std::string &text);

        static const unsigned CodeLen = 4;
        public:
            static void addParser(const std::string &type,
                                  const std::string &code,
                                  fparse prs, bool parse_pax_ = true)
            {
                prsMap(type).insert(std::make_pair(code, prs_struct(prs, parse_pax_)));
            }
            SsrParsers();

            static Ssr * parse (const std::string &type, const std::string &txt);
    };

    typedef SsrParsers RemarkParsers;

}
#endif /*_SSR_PARSER_H_*/
