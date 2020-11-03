#ifndef __RAS_ELEMENT_H__
#define __RAS_ELEMENT_H__

#include <string>
#include "typeb/typeb_message.h"
#include "typeb/tb_elements.h"

namespace typeb_parser {

class RasElem : public TbElement
{
    std::string flight_;
    std::string startDate_;
    std::string endDate_;
    std::string segment_;
    bool closedOnly_;

    RasElem(const std::string& flt, const std::string& startDt, const std::string& endDt, const std::string& seg, bool);
public:
    static RasElem * parse(const std::string & text);

    const std::string& flight() const { return flight_; }
    const std::string& startDate() const { return startDate_; }
    const std::string& endDate() const { return endDate_; }
    const std::string& segment() const { return segment_; }
    bool isClosedOnly() const { return closedOnly_; }
};

std::ostream & operator << (std::ostream& os, const RasElem & av);

} //namespace typeb_parser

#endif //__RAS_ELEMENT_H__
