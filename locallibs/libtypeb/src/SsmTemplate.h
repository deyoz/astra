#ifndef _SSMFLTEMPLATE_H_
#define _SSMFLTEMPLATE_H_
#include "typeb/typeb_template.h"
namespace typeb_parser
{

#define SSM_CLASS_TEMPLATE(x) \
    class Ssm##x##Template : public tb_descr_nolexeme_element \
    { public: virtual const char * name() const { return #x" element";} \
        virtual const char * accessName() const { return #x; } \
        virtual TbElement * parse(const std::string & text) const; \
        virtual bool isItYours(const std::string &, std::string::size_type *till) const; };


    SSM_CLASS_TEMPLATE(Flight)
    SSM_CLASS_TEMPLATE(RevFlight)
    SSM_CLASS_TEMPLATE(LongPeriod)
    SSM_CLASS_TEMPLATE(SkdPeriod)
    SSM_CLASS_TEMPLATE(ShortFlight)
    SSM_CLASS_TEMPLATE(Equipment)
    SSM_CLASS_TEMPLATE(Routing)
    SSM_CLASS_TEMPLATE(LegChange)
    SSM_CLASS_TEMPLATE(Segment)
    SSM_CLASS_TEMPLATE(RejectInfo)
    SSM_CLASS_TEMPLATE(DontCare)
    SSM_CLASS_TEMPLATE(Empty)
    SSM_CLASS_TEMPLATE(SupplInfo)

} // namespace typeb_parser
#endif /*_SSMFLTEMPLATE_H_*/
