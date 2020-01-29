#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include "packer.h"

#include <boost/date_time/gregorian/greg_date.hpp>

#include <serverlib/json_packer_heavy.h>
#include <serverlib/json_pack_types.h>
#include <nsi/packer.h>

#include "mct.h"

#define NICKNAME "DAG"
#include <serverlib/slogger.h>

namespace json_spirit {

JSON_PACK_UNPACK_DECL(ssim::mct::AircraftBody);
JSON_PACK_UNPACK_ENUM(ssim::mct::AircraftBody)

JSON_PACK_UNPACK_DECL(ssim::mct::Status);
JSON_PACK_UNPACK_ENUM(ssim::mct::Status)

JSON_PACK_UNPACK_DECL(ssim::mct::SegDesc);

using SegDesc = ssim::mct::SegDesc;
JSON_BEGIN_DESC_TYPE(SegDesc)
    DESC_TYPE_FIELD("status", status)
    DESC_TYPE_FIELD("conxPt", conxStation)
    DESC_TYPE_FIELD("conxTr", conxTerminal)
    DESC_TYPE_FIELD("crr", carrier)
    DESC_TYPE_FIELD("oprCrr", cshOprCarrier)
    DESC_TYPE_FIELD2("csh", cshIndicator, false)
    DESC_TYPE_FIELD("frng", fltRange)
    DESC_TYPE_FIELD("acType", aircraftType)
    DESC_TYPE_FIELD2("acbType", aircraftBodyType, ssim::mct::AircraftBody::Any)
    DESC_TYPE_FIELD("oppG", oppGeozone)
    DESC_TYPE_FIELD("oppC", oppCountry)
    DESC_TYPE_FIELD("oppR", oppRegion)
    DESC_TYPE_FIELD("oppS", oppStation)
JSON_END_DESC_TYPE(SegDesc)

using Record = ssim::mct::Record;
JSON_BEGIN_DESC_TYPE(Record)
    DESC_TYPE_FIELD("arrSg", arrival)
    DESC_TYPE_FIELD("depSg", departure)
    DESC_TYPE_FIELD2("begDt", effectiveFrom, boost::gregorian::date(boost::gregorian::neg_infin))
    DESC_TYPE_FIELD2("endDt", effectiveTo, boost::gregorian::date(boost::gregorian::pos_infin))
    DESC_TYPE_FIELD2("sup", suppressionIndicator, false)
    DESC_TYPE_FIELD("supG", suppressionGeozone)
    DESC_TYPE_FIELD("supC", suppressionCountry)
    DESC_TYPE_FIELD("supR", suppressionRegion)
    DESC_TYPE_FIELD2("mct", time, boost::posix_time::time_duration(boost::posix_time::pos_infin))
JSON_END_DESC_TYPE(Record)

} //json_spirit

namespace ssim { namespace mct {

std::string packRecord(const Record& r)
{
    return json_spirit::write(json_spirit::Traits<Record>::packInt(r));
}

boost::optional<Record> unpackRecord(const std::string& s)
{
    json_spirit::mValue val;
    if (!json_spirit::read(s, val)) {
        LogTrace(TRACE1) << "invalid JSON";
        return boost::none;
    }

    auto v = json_spirit::Traits<Record>::unpackInt(val);
    if (!v) {
        LogTrace(TRACE1) << v.err();
        return boost::none;
    }
    return *v;
}

} } //ssim::mct
