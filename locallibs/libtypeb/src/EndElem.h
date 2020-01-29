#ifndef LIBTYPEB_ENDELEM_H
#define LIBTYPEB_ENDELEM_H

#include "tb_elements.h"
namespace typeb_parser
{

//     ENDETL or ENDPART2
class EndElem : public TbElement
{
    bool IsLastPart; // Признак последней части
public:
    EndElem(bool isLastPart)
    : IsLastPart(isLastPart)
    {
    }

    /**
     * @return true for last part
     */
    bool isLastPart() const { return IsLastPart; }

    static EndElem * parse (const std::string &txt);
};

}
#endif /* LIBTYPEB_ENDELEM_H */

