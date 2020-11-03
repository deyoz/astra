#ifndef _ASMFLTEMPLATE_H_
#define _ASMFLTEMPLATE_H_
#include <string>
#include "typeb/typeb_template.h"
#include "typeb/SsmElem.h"
#include "typeb/SsmTemplate.h"
#include "typeb/AsmStrings.h"
namespace typeb_parser {

typedef SsmEquipmentTemplate  AsmEquipmentTemplate;
typedef SsmSegmentTemplate    AsmSegmentTemplate;
typedef SsmRejectInfoTemplate AsmRejectInfoTemplate;
typedef SsmDontCareTemplate   AsmDontCareTemplate;
typedef SsmEmptyTemplate      AsmEmptyTemplate;

typedef SsmEquipmentElem  AsmEquipmentElem;
typedef SsmSegmentElem    AsmSegmentElem;
typedef SsmSupplInfoElem  AsmSupplInfoElem;
typedef SsmRejectInfoElem AsmRejectInfoElem;
typedef SsmDontCareElem   AsmDontCareElem;
typedef SsmEmptyElem      AsmEmptyElem;

class AsmFlightTemplate : public tb_descr_nolexeme_element 
{
public:
    virtual const char * name() const { return " asm flight element";}
    virtual const char * accessName() const { return "asm flight"; }
    virtual TbElement * parse(const std::string & text) const;
    virtual bool isItYours(const std::string &, std::string::size_type *till) const;
};

class AsmFlightElem : public TbElement 
{
    std::string flightDate_; //mandatory flight info
    std::string legChange_; //conditional; makes sense for some msgs only; not supported
    std::string newFlightDate_; //mandatory for flight str FLT
    std::vector<std::string> dei_;
    AsmFlightElem(const std::string & fd, const std::string & lc, const std::string & nfd,
            const std::vector<std::string> & vc) : flightDate_(fd), legChange_(lc), newFlightDate_(nfd), dei_(vc) {}
public:
    static AsmFlightElem * parse(const std::string & text);
    const std::string & flightDate() const { return flightDate_; }
    const std::string & legChange() const { return legChange_; } 
    const std::string & newFlightDate() const { return newFlightDate_; } 
    const std::vector<std::string> & dei() const { return dei_; }

};

class AsmRoutingTemplate : public tb_descr_nolexeme_element 
{
public:
    virtual const char * name() const { return " asm routing element";}
    virtual const char * accessName() const { return "asm routing"; }
    virtual TbElement * parse(const std::string & text) const;
    virtual bool isItYours(const std::string &, std::string::size_type *till) const;
};

class AsmRoutingElem : public TbElement 
{
    std::string dep_; //departure
    std::string dateD_; //dep date (if any)
    std::string airSTD_; //scheduled aircraft departure
    std::string pasSTD_; //passenger std
    std::string arr_; //arrival
    std::string dateA_; //arr date (if any)
    std::string airSTA_; //scheduler aircraft arrival
    std::string pasSTA_; //passenger sta
    std::vector<std::string> dei_;
    AsmRoutingElem(const std::string & dep, const std::string & dD,
                   const std::string & astd, const std::string & pstd,
                   const std::string & arr, const std::string & dA,
                   const std::string & asta, const std::string & psta,
                   const std::vector< std::string > & v)
    : dep_(dep), dateD_(dD), airSTD_(astd), pasSTD_(pstd),
      arr_(arr), dateA_(dA), airSTA_(asta), pasSTA_(psta), dei_(v) {}   
public:
    static AsmRoutingElem * parse(const std::string & text);
    const std::string & dep() const { return dep_; }
    const std::string & dateD() const { return dateD_; }
    const std::string & airSTD() const { return airSTD_; }
    const std::string & pasSTD() const { return pasSTD_; }
    const std::string & arr() const { return arr_; }
    const std::string & dateA() const { return dateA_; }
    const std::string & airSTA() const { return airSTA_; }
    const std::string & pasSTA() const { return pasSTA_; }
    const std::vector<std::string> & dei() const { return dei_; }
};

std::ostream & operator << (std::ostream& os, const AsmFlightElem &name);
std::ostream & operator << (std::ostream& os, const AsmRoutingElem &name);

} // namespace typeb_parser
#endif /*_ASMFLTEMPLATE_H_*/
