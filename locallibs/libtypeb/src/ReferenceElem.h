#ifndef _REFELEM_H_
#define _REFELEM_H_
#include <string>
#include <list>
#include "typeb_message.h"
#include "tb_elements.h"

namespace typeb_parser
{

class ReferenceElem : public TbElement 
{
    std::string date_;
    unsigned int gid_;
    bool last_;
    unsigned int mid_;
    std::string creatorRef_;
    ReferenceElem(const std::string & d, unsigned int gid, bool l, unsigned int mid, const std::string & ref)
    : date_(d), gid_(gid), last_(l), mid_(mid), creatorRef_(ref) {}
public:
    static ReferenceElem * parse(const std::string & text); //DDMON
    const std::string & date() const { return date_; }
    unsigned int gid() const { return gid_; }
    bool isLast() const { return last_; }
    unsigned int mid() const { return mid_; }
    const std::string & creatorRef() const { return creatorRef_; }
};

std::ostream & operator << (std::ostream& os, const ReferenceElem &name);

}
#endif /*_REFELEM_H_*/
