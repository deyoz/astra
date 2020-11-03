/*
*  C++ Implementation: FlightELem
*
* Description: Frequent Traveller element  � PFS ⥫��ࠬ��
*
*
*/

#ifndef _FREQ_TR_ELEM_H_
#define _FREQ_TR_ELEM_H_
#include <string>
#include <boost/date_time/gregorian/gregorian_types.hpp>

#include "tb_elements.h"
namespace typeb_parser
{

//     P2452/24OCT TJM PART1
class FreqTrElem : public TbElement
{
    std::string Airline; // ��������
    std::string FTNumber; // ����� ����
public:
    FreqTrElem(const std::string &Airl, const std::string & FTNum)
    :Airline(Airl), FTNumber(FTNum)
    {
    }

    /**
     * ������������
     * @return std::string
     */
    const std::string &airline() const { return Airline; }

    /**
     * ����� ����
     */
    const std::string &FTNum() const { return  FTNumber; }

    /**
     * ������
     * @param txt text of Flight Element
     * @return FlightElem *
     */
    static FreqTrElem * parse (const std::string &txt);
    static const char *elemName() { return "Frequent Traveller Number Element"; }
};

}
#endif /* _FREQ_TR_ELEM_H_ */
