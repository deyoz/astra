//
// C++ Interface: ItinElem
//
// Description: ������� � ETH ᮮ�饭��
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
// -ITIN/U6123YDMESVX14MAR07�OK/0700
class ItinElem : public TbElement {
    std::string Airline; // ��������
    unsigned FlightNum;  // ����� ३�

    boost::posix_time::ptime DepDateTime; // ��� �뫥�
    char ClsLeter;

    std::string DepPoint; // ��த/���� �뫥�
    std::string ArrPoint; // ��த/���� �ਡ���
public:
    ItinElem (){}
    static ItinElem * parse (const std::string &itin);

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
     * @return boost::posix_time::ptime
     */
    const boost::posix_time::ptime &depDateTime() const { return DepDateTime; }

    /**
     * ��த/���� �뫥�
     * @return std::string
     */
    const std::string &depPoint() const { return DepPoint; }

    /**
     * ��த/���� �ਡ���
     * @return std::string
     */
    const std::string &arrPoint() const { return ArrPoint; }

    /**
     * �������� ���㦨�����
     * @return char
     */
    char clsLeter() const { return ClsLeter; }

    /**
     * ��� �����
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
     * ��� �������� �������饩 �� ������ ���⪥
     * @return std::string
     */
    const std::string &airline () const { return Airline; }

    /**
     * ����� ३�
     * @return unsigned flight number
     */
    unsigned flightNum () const { return FlightNum; }
    /**
     * ��� �����
     * @return const char * - name
     */
    static const char *elemName() { return "Operating carrier"; }
};

std::ostream & operator << (std::ostream& os, const ItinElem &itin);
std::ostream & operator << (std::ostream& os, const OperCarrierElem &oper);
}
#endif /*_ITINELEM_H_*/
