/*
*  C++ Implementation: FlightELem
*
* Description: FlightElem � ETL ⥫��ࠬ��
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
    std::string Airline; // ��������
    unsigned FlightNum;  // ����� ३�

    boost::gregorian::date DepDate; // ��� �뫥�

    std::string DepPoint; // ��த/���� �뫥�
    size_t Part; // ����� ��� ⫣
public:
    FlightElem(const std::string &Airl, unsigned FlNum,
               const boost::gregorian::date &depd, std::string  &depp, size_t part)
    :Airline(Airl), FlightNum(FlNum),DepDate(depd), DepPoint(depp), Part(part)
    {
    }

    /**
     * ������������
     * @return std::string
     */
    const std::string &airline() const { return Airline; }

    /**
     * ����� ३�
     * @return unsigned
     */
    unsigned flightNum() const { return  FlightNum; }

    /**
     * ��� �뫥�
     * @return boost::gregorian::date
     */
    const boost::gregorian::date &depDate() const { return DepDate; }
    /**
     * ��த/���� �뫥�
     * @return std::string
     */
    const std::string &depPoint() const { return DepPoint; }

    /**
     * @return part number
     */
    size_t part() const { return Part; }

    /**
     * ������
     * @param txt text of Flight Element
     * @return FlightElem *
     */
    static FlightElem * parse (const std::string &txt);
};

}
#endif /*_FLIGHT_ELEM_H_*/
