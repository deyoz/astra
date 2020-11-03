//
// C++ Interface: AutomPNRAddr
//
// Description: PNR element
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2009
//
//
#ifndef _AUTOMPNRADDR_H_
#define _AUTOMPNRADDR_H_
#include <string>
#include "tb_elements.h"

namespace typeb_parser
{
// .L/06ВФРН/ПО
class AutomPNRAddr : public TbElement {
    std::string Airline; // Авиакомп
    std::string Recloc;  // record locator
public:
    AutomPNRAddr (){}
    static AutomPNRAddr * parse (const std::string &rl);

    /**
     * @brief Авиакомпания
     * @return std::string
     */
    const std::string &airline() const { return Airline; }

    /**
     * @brief record locator
     */
    const std::string &recloc() const { return  Recloc; }

    /**
     * @brief Имя элемента
     * @return const char * - name
     */
    static const char *elemName() { return "Automated PNR address element"; }
};

std::ostream & operator << (std::ostream& os, const AutomPNRAddr &pnr);
}
#endif /*_AUTOMPNRADDR_H_*/
