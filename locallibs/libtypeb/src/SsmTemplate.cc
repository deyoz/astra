#include <boost/regex.hpp>
#include "SsmTemplate.h"
#include "SsmElem.h"
#include "SsmStrings.h"
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE

namespace typeb_parser
{

#define SSM_TEMPLATE_PARSE(x) \
    TbElement * Ssm##x##Template::parse(const std::string & text) const { return Ssm##x##Elem::parse(text); } 

SSM_TEMPLATE_PARSE(Flight)
SSM_TEMPLATE_PARSE(RevFlight)
SSM_TEMPLATE_PARSE(LongPeriod)
SSM_TEMPLATE_PARSE(SkdPeriod)
SSM_TEMPLATE_PARSE(ShortFlight)
SSM_TEMPLATE_PARSE(Equipment)
SSM_TEMPLATE_PARSE(Routing)
SSM_TEMPLATE_PARSE(LegChange)
SSM_TEMPLATE_PARSE(Segment)
SSM_TEMPLATE_PARSE(RejectInfo)
SSM_TEMPLATE_PARSE(DontCare)
SSM_TEMPLATE_PARSE(Empty)
SSM_TEMPLATE_PARSE(SupplInfo)

#define SSM_TEMPLATE_OURS(x) \
    bool Ssm##x##Template::isItYours(const std::string & text, std::string::size_type * till) const {\
        static const boost::regex rx(Ssm##x##String); \
        return boost::regex_match(text, rx, boost::match_any);}

SSM_TEMPLATE_OURS(Flight)
SSM_TEMPLATE_OURS(RevFlight)
SSM_TEMPLATE_OURS(LongPeriod)
SSM_TEMPLATE_OURS(SkdPeriod)
SSM_TEMPLATE_OURS(ShortFlight)
SSM_TEMPLATE_OURS(Equipment)
SSM_TEMPLATE_OURS(Routing)
SSM_TEMPLATE_OURS(LegChange)
SSM_TEMPLATE_OURS(Segment)
SSM_TEMPLATE_OURS(RejectInfo)
SSM_TEMPLATE_OURS(DontCare)
SSM_TEMPLATE_OURS(Empty)
SSM_TEMPLATE_OURS(SupplInfo)

} // namespace typeb_parser
