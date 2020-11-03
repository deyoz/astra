//
// C++ Interface: RemFoidElem
//
// Description: Ремарка FOID в TypeB телеграмме
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2007
//
//
#ifndef _REM_FQTX_ELEM_H_
#define _REM_FQTX_ELEM_H_
#include "ssr_parser.h"

namespace typeb_parser
{

class RemFoidElem : public Remark {
    RemFoidElem(const std::string &rem)
    :Remark(RemarkTypes::Foid, rem)
    {
    }
public:
    static Remark * parse (const std::string &code, const std::string &rem);
};

}
#endif /*_REM_FQTX_ELEM_H_*/
