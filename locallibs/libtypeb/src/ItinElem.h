//
// C++ Interface: ItinElem
//
// Description: Маршрут в ETH сообщении
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2007
//
//
#ifndef _ITINELEM_H_
#define _ITINELEM_H_
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <string>
#include "tb_elements.h"

namespace typeb_parser
{
// -ITIN/U6123YDMESVX14MAR07 OK/0700
class ItinElem : public TbElement {
    std::string Airline; // Авиакомп
    unsigned FlightNum;  // Номер рейса

    boost::posix_time::ptime DepDateTime; // Дата вылета
    char ClsLeter;

    std::string DepPoint; // Город/порт вылета
    std::string ArrPoint; // Город/порт прибытия
public:
    ItinElem (){}
    static ItinElem * parse (const std::string &itin);

    /**
     * Авиакомпания
     * @return std::string
     */
    const std::string &airline() const { return Airline; }

    /**
     * Номер рейса
     * @return unsigned
     */
    unsigned flightNum() const { return  FlightNum; }

    /**
     * Дата вылета
     * @return boost::posix_time::ptime
     */
    const boost::posix_time::ptime &depDateTime() const { return DepDateTime; }

    /**
     * Город/порт вылета
     * @return std::string
     */
    const std::string &depPoint() const { return DepPoint; }

    /**
     * Город/порт прибытия
     * @return std::string
     */
    const std::string &arrPoint() const { return ArrPoint; }

    /**
     * Подкласс обслуживания
     * @return char
     */
    char clsLeter() const { return ClsLeter; }

    /**
     * Имя элемента
     * @return const char * - name
     */
    static const char *elemName() { return "Itin element"; }
};

class OperCarrierElem : public TbElement {
    std::string Airline;
    unsigned FlightNum;
public:
    OperCarrierElem (){}
    static OperCarrierElem * parse (const std::string &carr);
    /**
     * Код компании оперирующей на данном участке
     * @return std::string
     */
    const std::string &airline () const { return Airline; }

    /**
     * Номер рейса
     * @return unsigned flight number
     */
    unsigned flightNum () const { return FlightNum; }
    /**
     * Имя элемента
     * @return const char * - name
     */
    static const char *elemName() { return "Operating carrier"; }
};

std::ostream & operator << (std::ostream& os, const ItinElem &itin);
std::ostream & operator << (std::ostream& os, const OperCarrierElem &oper);
}
#endif /*_ITINELEM_H_*/
