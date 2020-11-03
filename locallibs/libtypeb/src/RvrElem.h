#ifndef _RVRELEM_H_
#define _RVRELEM_H_
#include <string>
#include <list>
#include "typeb_message.h"
#include "tb_elements.h"

namespace typeb_parser
{

class RvrElem : public TbElement 
{
    std::string airline_;
    unsigned int flightNum_;
    char suffix_;
    std::string firstDate_;
    std::string secondDate_;
    std::string freq_;
    RvrElem(std::string & airline, unsigned int flight, char suffix, 
            std::string & d1, std::string & d2, std::string & freq) : 
            airline_(airline), flightNum_(flight), suffix_(suffix), 
            firstDate_(d1), secondDate_(d2), freq_(freq) {}
public:
    const std::string & airline() const { return airline_; }
    unsigned int flight() const { return flightNum_; }
    char suffix() const { return suffix_; }
    const std::string & firstDate() const { return firstDate_; }
    const std::string & secondDate() const { return secondDate_; }
    const std::string & freq() const { return freq_; }
    static RvrElem * parse(const std::string & text);
};

std::ostream & operator << (std::ostream& os, const RvrElem &name);

}
#endif /*_RVRELEM_H_*/
