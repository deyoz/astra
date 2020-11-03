//
// C++ Interface: TotalsByDestElem
//
// Description: Описание TotalsByDestElem ETL сообщения
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//
#ifndef _TOTALS_BY_DEST_ELEM_H_
#define _TOTALS_BY_DEST_ELEM_H_
#include <string>

#include "tb_elements.h"
namespace typeb_parser
{

    //-DME008Y
class TotalsByDestElem : public TbElement
{
    std::string DestPoint;
    unsigned Npassengers;
    std::string Class;
public:
    TotalsByDestElem(const std::string &dest, unsigned npass, const std::string & Cls)
    :DestPoint(dest), Npassengers(npass), Class(Cls)
    {
    }

    const std::string &destPoint() const { return DestPoint; }
    unsigned npass() const { return Npassengers; }
    const std::string & classCh() const { return Class; }

    static TotalsByDestElem * parse (const std::string &txt);
};

}

#endif /*_TOTALS_BY_DEST_ELEM_H_*/
