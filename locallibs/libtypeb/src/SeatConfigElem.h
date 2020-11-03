/*
*  C++ Implementation: SeatConfigElem
*
* Description: SeatConfig в ETL телеграмме
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#ifndef _SEATCFGELEM_H_
#define _SEATCFGELEM_H_
#include <string>
#include <list>

#include "tb_elements.h"
namespace typeb_parser
{

// CFG/008C060Y/TU3
class SeatConfigElem : public TbElement
{
    const std::string FlightType;
    class Total2Class
    {
        unsigned Total;
        char Class;
    public:
        Total2Class(unsigned t, unsigned char cls)
            :Total(t), Class(cls)
        {
        }

        inline unsigned total() const { return Total; }
        inline unsigned char classCh() const   { return Class; }
    };
    
    std::list<Total2Class> Totals;
public:
    SeatConfigElem(const std::list<Total2Class> &ttls, const std::string &ftype)
    :FlightType(ftype), Totals(ttls)
    {
    }
    
    static SeatConfigElem * parse (const std::string &rem);
    
    const std::string &flightType() const { return FlightType; }
    const std::list <Total2Class> & totals() const { return Totals; }
};

} /*namespace typeb_parser*/
#endif /*_SEATCFGELEM_H_*/

