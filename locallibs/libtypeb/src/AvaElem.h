#ifndef _AVAELEM_H_
#define _AVAELEM_H_
#include <string>
#include <list>
#include "typeb_message.h"
#include "tb_elements.h"

namespace typeb_parser
{

struct RawRbd {
    char rbd;
    int avail;
};

class AvaElem : public TbElement 
{
    std::string airline_;
    unsigned int flightNum_;
    char suffix_;
    char compartment_;
    std::string date_;
    std::string leg_;
    std::list< RawRbd > rbds_;
    bool cancel_;//if this is cancellation element
    AvaElem(std::string & airline, unsigned int flight, char suffix, char comp,
                    std::string & d, std::string & leg, std::list< RawRbd > & lst, bool isCancel = false) : 
                    airline_(airline), flightNum_(flight), suffix_(suffix), 
                    compartment_(comp), date_(d), leg_(leg), rbds_(lst), cancel_(isCancel) {}
    static AvaElem * parseCancellation(const std::string & text);
    static AvaElem * parseUsual(const std::string & text);
public:
    const std::string & airline() const { return airline_; }
    int flight() const { return flightNum_; }
    char suffix() const { return suffix_; }
    char compartment() const { return compartment_; }
    const std::string & date1() const { return date_; }
    const std::string & leg() const { return leg_; }
    const std::list< RawRbd > & rbds() const { return rbds_; }
    bool isCancellation() const { return cancel_; }
    static AvaElem * parse(const std::string & text);
};

std::ostream & operator << (std::ostream& os, const AvaElem &name);

}
#endif /*_AVAELEM_H_*/
