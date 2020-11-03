//
// C++ Interface: RemDocsElem
//
// Description: ����ઠ DOCS � TypeB ⥫��ࠬ���
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2010
//
//
#ifndef _REM_DOCS_ELEM_H_
#define _REM_DOCS_ELEM_H_
#include "ssr_parser.h"

namespace typeb_parser
{

class RemDocsElem : public Remark
{

    RemDocsElem(const std::string &code, const std::string &rem)
        :Remark(code, rem)
    {
    }
public:
    static Remark * parse (const std::string &code, const std::string &rem);
};

}
#endif /*_REM_DOCS_ELEM_H_*/
