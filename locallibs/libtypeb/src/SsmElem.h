#ifndef _SSMFLELEM_H_
#define _SSMFLELEM_H_
#include <string>
#include <vector>
#include "typeb_message.h"
#include "tb_elements.h"

namespace typeb_parser
{

class SsmFlightElem : public TbElement
{
    std::string fl_;
    std::vector<std::string> dei_;
    SsmFlightElem(const std::string & fl, const std::vector<std::string> & v) 
        : fl_(fl), dei_(v) {}
public:
    static SsmFlightElem * parse(const std::string & text);
    const std::string & flight() const { return fl_; }
    const std::vector< std::string > & dei() const { return dei_; }
};

class SsmRevFlightElem : public TbElement 
{
    std::string fl_; //su
    std::string dt1_; //23may(99)
    std::string dt2_;
    std::string freq_; //124
    std::string freqRate_; // /W2
    SsmRevFlightElem(const std::string & a, const std::string & dt1, const std::string & dt2,
                  const std::string & fq, const std::string & fqR) 
        : fl_(a), dt1_(dt1), dt2_(dt2), freq_(fq), freqRate_(fqR) {}
public:
    static SsmRevFlightElem * parse(const std::string & text);
    const std::string & flight() const { return fl_; }
    const std::string & date1() const { return dt1_; }
    const std::string & date2() const { return dt2_; }
    const std::string & freq() const { return freq_; }
    const std::string & freqRate() const { return freqRate_; }
};

class SsmLongPeriodElem : public TbElement 
{
    //all dates in format 23may(99) 
    std::string dt1_; 
    std::string dt2_;
    std::string freq_; //124
    std::string freqRate_; // /W2
    std::vector< std::string > dei_; // 2/AS 3/X ... 1/AS/DV(/ER)
    SsmLongPeriodElem(const std::string & dt1, const std::string & dt2,
                  const std::string & fq, const std::string & fqR, const std::vector<std::string> & v)
        : dt1_(dt1), dt2_(dt2), freq_(fq), freqRate_(fqR), dei_(v) {}
public:
    static SsmLongPeriodElem * parse(const std::string & text);
    const std::string & date1() const { return dt1_; }
    const std::string & date2() const { return dt2_; }
    const std::string & freq() const { return freq_; }
    const std::string & freqRate() const { return freqRate_; }
    const std::vector< std::string > & dei() const { return dei_; }
};

class SsmSkdPeriodElem : public TbElement
{
    std::string effectDt_;
    std::string discontDt_;
    SsmSkdPeriodElem(const std::string & dt1, const std::string & dt2) : effectDt_(dt1), discontDt_(dt2) {}
public:
    static SsmSkdPeriodElem * parse(const std::string & text);
    const std::string & effectDt() const { return effectDt_; }
    const std::string & discontDt() const { return discontDt_; }
};

class SsmShortFlightElem : public TbElement 
{
    std::string fl_;
    SsmShortFlightElem(const std::string & a) : fl_(a) {}
public:
    static SsmShortFlightElem * parse(const std::string & text);
    const std::string & flight() const { return fl_; }
};

class SsmEquipmentElem : public TbElement 
{
    char serviceType_;
    std::string craftType_;
    std::string aircraftConfig_;
    std::string aircraftRegistration_;
    std::vector<std::string> dei_;
    SsmEquipmentElem(char s, const std::string & ct, 
            const std::string & ac, const std::string & ar, const std::vector<std::string> & v)
        : serviceType_(s), craftType_(ct), aircraftConfig_(ac), aircraftRegistration_(ar), dei_(v) {}
public:
    static SsmEquipmentElem * parse(const std::string & text);
    char serviceType() const { return serviceType_; }
    const std::string & craftType() const { return craftType_; }
    const std::string & aircraftConfig() const { return aircraftConfig_; }
    const std::string & aircraftRegistration() const { return aircraftRegistration_; }
    const std::vector<std::string> & dei() const { return dei_; }
};

class SsmRoutingElem : public TbElement 
{
    std::string dep_; //departure
    std::string airSTD_; //scheduled aircraft departure
    std::string varSTD_; //date variation of std
    std::string pasSTD_; //passenger std
    std::string arr_; //arrival
    std::string airSTA_; //scheduler aircraft arrival
    std::string varSTA_; //date variation of sta
    std::string pasSTA_; //passenger sta
    std::vector< std::string > dei_;
    SsmRoutingElem(const std::string & d, 
                   const std::string & astd, const std::string & vstd, const std::string & pstd,
                   const std::string & a, 
                   const std::string & asta, const std::string & vsta, const std::string & psta,
                   const std::vector< std::string > & v)
    : dep_(d), airSTD_(astd), varSTD_(vstd), pasSTD_(pstd),
      arr_(a), airSTA_(asta), varSTA_(vsta), pasSTA_(psta), dei_(v) {}   
public:
    static SsmRoutingElem * parse(const std::string & text);
    const std::string & dep() const { return dep_; }
    const std::string & airSTD() const { return airSTD_; }
    const std::string & varSTD() const { return varSTD_; }
    const std::string & pasSTD() const { return pasSTD_; }
    const std::string & arr() const { return arr_; }
    const std::string & airSTA() const { return airSTA_; }
    const std::string & varSTA() const { return varSTA_; }
    const std::string & pasSTA() const { return pasSTA_; }
    const std::vector< std::string > & dei() const { return dei_; }
};

class SsmLegChangeElem : public TbElement
{
    std::string legsChange_; //flight leg change identifier
    std::vector< std::string > dei_;
    SsmLegChangeElem(const std::string & lc, const std::vector<std::string> & v) 
        : legsChange_(lc), dei_(v) {}
public:
    static SsmLegChangeElem * parse(const std::string & text);
    const std::string & legsChange() const { return legsChange_; }
    const std::vector< std::string > & dei() const { return dei_; }
};

class SsmSegmentElem : public TbElement 
{
    std::string seg_;
    std::string dei_;
    SsmSegmentElem(const std::string & s, const std::string & d) : seg_(s), dei_(d) {}
public:
    static SsmSegmentElem * parse(const std::string & text);
    const std::string & seg() const { return seg_; }
    const std::string & dei() const { return dei_; }
};

class SsmSupplInfoElem : public TbElement
{
    std::string txt_;
    SsmSupplInfoElem(const std::string & s) : txt_(s) {}
public:
    static SsmSupplInfoElem * parse(const std::string & text);
    const std::string & txt() const { return txt_; }
};

class SsmDontCareElem : public TbElement 
{
    SsmDontCareElem() {}
public:
    static SsmDontCareElem * parse(const std::string & text);
};

class SsmRejectInfoElem : public TbElement
{
    std::string txt_;
    SsmRejectInfoElem(const std::string & t) : txt_(t) {}
public:
    static SsmRejectInfoElem * parse(const std::string & text);
    const std::string & txt() const { return txt_; }
};

class SsmEmptyElem : public TbElement
{
    SsmEmptyElem() {}
public:
    static SsmEmptyElem * parse(const std::string & text);
};

std::ostream & operator << (std::ostream& os, const SsmFlightElem &name);
std::ostream & operator << (std::ostream& os, const SsmRevFlightElem &name);
std::ostream & operator << (std::ostream& os, const SsmLongPeriodElem &name);
std::ostream & operator << (std::ostream& os, const SsmSkdPeriodElem &name);
std::ostream & operator << (std::ostream& os, const SsmShortFlightElem &name);
std::ostream & operator << (std::ostream& os, const SsmEquipmentElem &name);
std::ostream & operator << (std::ostream& os, const SsmRoutingElem &name);
std::ostream & operator << (std::ostream& os, const SsmLegChangeElem &name);
std::ostream & operator << (std::ostream& os, const SsmSegmentElem &name);
std::ostream & operator << (std::ostream& os, const SsmSupplInfoElem &name);
std::ostream & operator << (std::ostream& os, const SsmDontCareElem &name);
std::ostream & operator << (std::ostream& os, const SsmRejectInfoElem &name);
std::ostream & operator << (std::ostream& os, const SsmEmptyElem &name);

}
#endif /*_SSMFLELEM_H_*/
