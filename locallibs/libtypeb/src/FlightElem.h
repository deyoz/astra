/*
*  C++ Implementation: FlightELem
*
* Description: FlightElem в ETL телеграмме
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#ifndef _FLIGHT_ELEM_H_
#define _FLIGHT_ELEM_H_
#include <string>
#include <boost/date_time/gregorian/gregorian_types.hpp>

#include "tb_elements.h"
namespace typeb_parser
{

//     P2452/24OCT TJM PART1
class FlightElem : public TbElement
{
    std::string Airline; // Авиакомп
    unsigned FlightNum;  // Номер рейса

    boost::gregorian::date DepDate; // Дата вылета

    std::string DepPoint; // Город/порт вылета
    size_t Part; // Номер части тлг
public:
    FlightElem(const std::string &Airl, unsigned FlNum,
               const boost::gregorian::date &depd, std::string  &depp, size_t part)
    :Airline(Airl), FlightNum(FlNum),DepDate(depd), DepPoint(depp), Part(part)
    {
    }

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
     * @return boost::gregorian::date
     */
    const boost::gregorian::date &depDate() const { return DepDate; }
    /**
     * Город/порт вылета
     * @return std::string
     */
    const std::string &depPoint() const { return DepPoint; }

    /**
     * @return part number
     */
    size_t part() const { return Part; }

    /**
     * Разбор
     * @param txt text of Flight Element
     * @return FlightElem *
     */
    static FlightElem * parse (const std::string &txt);
};

}
#endif /*_FLIGHT_ELEM_H_*/
