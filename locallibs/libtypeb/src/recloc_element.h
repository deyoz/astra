//
// C++ Interface: recloc_element
//
// Description: Разбор Recloc элемента телеграммы
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
//
//

#ifndef _RECLOC_ELEMENT_H_
#define _RECLOC_ELEMENT_H_

#include <string>
#define _TYPEB_CORE_PARSER_SECRET_ "ПРЕВЕД"
#include "typeb_core_parser.h"
#include "tb_elements.h"

namespace typeb_parser
{
    class RecordLocatorElem : public TbElement{
        std::string PredP;
        std::string LocationPoint;
        std::string Airline;

        std::string Recloc;

        RecordLocatorElem(){}
        RecordLocatorElem( const std::string &predp, const std::string & location,
                           const std::string &Air, const std::string &rl)
            :PredP(predp), LocationPoint(location), Airline(Air), Recloc(rl)
        {
        }

    public:
        static RecordLocatorElem *parse(const std::string &rl_elem);
        static void Trace(int level, const char *nick, const char *file, int line,
                          const RecordLocatorElem &si);

        const std::string &recloc() const { return Recloc; }
        const std::string &predP() const  { return PredP; }
        const std::string &airline() const { return Airline; }
        const std::string &locationPoint() const { return LocationPoint; }
    };
}

#endif /*_RECLOC_ELEMENT_H_*/
