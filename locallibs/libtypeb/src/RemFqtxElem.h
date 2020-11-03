//
// C++ Interface: RemFqtxElem
//
// Description: Ремарка FQTV/R/U в TypeB телеграммее
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2010
//
//
#ifndef _REM_FOID_ELEM_H_
#define _REM_FOID_ELEM_H_
#include "ssr_parser.h"

namespace typeb_parser
{

class RemFqtxElem : public Remark
{
    std::string Airline;
    std::string Number;
    std::string FreeText;


    RemFqtxElem(const std::string &code, const std::string &rem,
                const std::string &awk, const std::string &num, const std::string &freet)
        :Remark(code, rem), Airline(awk), Number(num), FreeText(freet)
    {
    }
public:
    static Remark * parse (const std::string &code, const std::string &rem);

    const std::string &airline () const { return Airline; }
    const std::string &number  () const { return Number; }
    const std::string &freeText() const { return FreeText; }
};

}
#endif /*_REM_FOID_ELEM_H_*/
