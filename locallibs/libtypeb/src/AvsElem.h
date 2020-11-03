#ifndef _AVSELEM_H_
#define _AVSELEM_H_
#include <string>
#include "typeb/typeb_message.h"
#include "typeb/tb_elements.h"

namespace typeb_parser
{

class AvsElem : public TbElement 
{
    std::string airline_;
    unsigned int flightNum_;
    char suffix_;
    char subcls_;
    bool stForAllRbd_;
    std::string date_;
    std::string status_;
    std::string leg_;
    AvsElem(std::string & airline, unsigned int flight, char suffix, char rbd,
                    bool forAll, std::string & d, std::string & status, std::string & leg) : 
                    airline_(airline), flightNum_(flight), suffix_(suffix), 
                    subcls_(rbd), stForAllRbd_(forAll), date_(d), status_(status), leg_(leg){}
public:
    const std::string & airline() const { return airline_; }
    unsigned int flight() const { return flightNum_; }
    char suffix() const { return suffix_; }
    char rbd() const { return subcls_; }
    const std::string & date1() const { return date_; }
    const std::string & status() const { return status_; }
    bool statusForAllRbd() const { return stForAllRbd_; }
    const std::string & leg() const { return leg_; }
    static AvsElem * parse(const std::string & text);
};

std::ostream & operator << (std::ostream& os, const AvsElem & av);

}
#endif /*_AVSELEM_H_*/
