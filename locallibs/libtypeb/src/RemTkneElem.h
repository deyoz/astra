//
// C++ Interface: remark_element
//
// Description: Ремарка TKNE в TypeB телеграмме
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _REM_TKNE_ELEM_H_
#define _REM_TKNE_ELEM_H_
#include <string>
#include "ssr_parser.h"

namespace typeb_parser
{
class RemTkneElem: public Remark
{
    std::string Ticknum;
    unsigned Coupon;

    bool Infant;

    RemTkneElem(const std::string whole_txt, const std::string t, unsigned c, bool inf)
    : Remark(RemarkTypes::Tkne, whole_txt),
        Ticknum(t), Coupon(c), Infant(inf)
    {
    }
public:
    static Remark * parse (const std::string &code, const std::string &rem);

    const std::string &ticknum() const { return Ticknum; }
    unsigned coupon() const { return Coupon; }
    bool infant() const { return Infant; }
    const std::string &code() const { return RemarkTypes::Tkne; }
};

std::ostream &operator << (std::ostream &os, const RemTkneElem &tkne);
}

#endif /*_REM_TKNE_ELEM_H_*/
