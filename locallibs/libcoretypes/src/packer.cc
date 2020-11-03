#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include "packer.h"

#include <serverlib/json_packer_heavy.h>
#include <serverlib/json_pack_types.h>
#include <serverlib/dates.h>
#include <libnsi/packer.h>
#include <libnsi/exception.h>

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

namespace json_spirit
{

JSON_RIP_PACK(ct::FlightNum);
JSON_RIP_PACK(ct::Suffix);
JSON_RIP_PACK(ct::Recloc);
JSON_RIP_PACK(ct::RemoteRecloc);
JSON_RIP_PACK(ct::SegId);
JSON_RIP_PACK(ct::PassId);
JSON_RIP_PACK(ct::SsrId);
JSON_RIP_PACK(ct::OsiId);
JSON_RIP_PACK(ct::SvcId);
JSON_RIP_PACK(ct::SpaceId);
JSON_RIP_PACK(ct::DocId);
JSON_RIP_PACK(ct::DocNumber);
JSON_RIP_PACK(ct::GroupName);
JSON_RIP_PACK(ct::SpecRes);
JSON_RIP_PACK(ct::TicketSer);
JSON_RIP_PACK(ct::TicketNum);
JSON_RIP_PACK(ct::AccountCode);
JSON_RIP_PACK(ct::Coupon);
JSON_RIP_PACK(ct::LegNum);
JSON_RIP_PACK(ct::SegNum);
JSON_RIP_PACK(ct::Version);
JSON_RIP_PACK(ct::DeiCode);
JSON_RIP_PACK(ct::Agency);
JSON_RIP_PACK(ct::Ppr);
JSON_RIP_PACK(ct::Pult);
JSON_RIP_PACK(ct::Operator);
JSON_PACK_UNPACK_ENUM(ct::PassType);
JSON_PACK_UNPACK_ENUM(ct::Sex);
JSON_PACK_UNPACK_ENUM(ct::EmdType);
JSON_PACK_UNPACK_ENUM(ct::SegStatus::Code);
JSON_PACK_UNPACK_ENUM(ct::ArrStatus::Code);
JSON_PACK_UNPACK_ENUM(ct::SsrStatus::Code);
JSON_PACK_UNPACK_ENUM(ct::SvcStatus::Code);
JSON_RIP_PACK(ct::Carf);
JSON_RIP_PACK(ct::LangCode);
JSON_RIP_PACK(ct::ServiceType);

// Rbd

JSON_RIP_PACK_INT( ct::Rbd );

mValue Traits< ct::Rbd >::packExt( ct::Rbd const &t )
{
    return mValue( ct::rbdCode( t ) );
}

UnpackResult< ct::Rbd > Traits< ct::Rbd >::unpackExt( mValue const &v )
{
    JSON_ASSERT_TYPE(ct::Rbd, v, json_spirit::str_type);
    const boost::optional<ct::Rbd> rbds(ct::rbdFromStr(v.get_str()));
    if (!rbds) {
        return UnpackError{STDLOG, "rbdFromStr failed"};
    }
    return *rbds;
}

// Cabin

JSON_RIP_PACK_INT( ct::Cabin );

mValue Traits< ct::Cabin >::packExt( ct::Cabin const &t )
{
    return mValue( ct::cabinCode( t ) );
}

UnpackResult< ct::Cabin > Traits< ct::Cabin >::unpackExt( mValue const &v )
{
    JSON_ASSERT_TYPE(ct::Cabin, v, json_spirit::str_type);
    const boost::optional<ct::Cabin> cabins(ct::cabinFromStr(v.get_str()));
    if (!cabins) {
        return UnpackError{STDLOG, "cabinFromStr failed"};
    }
    return *cabins;
}

mValue Traits<ct::Seat>::packInt(const ct::Seat& s)
{
    return mValue(s.toString());
}

UnpackResult<ct::Seat> Traits<ct::Seat>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(ct::Seat, v, json_spirit::str_type);
    const boost::optional<ct::Seat> seat(ct::Seat::fromStr(v.get_str()));
    if (!seat) {
        return UnpackError{STDLOG, "Seat::fromStr failed"};
    }
    return *seat;
}

mValue Traits<ct::Seat>::packExt(const ct::Seat& i)
{
    return Traits<ct::Seat>::packInt(i);
}

UnpackResult<ct::Seat> Traits<ct::Seat>::unpackExt(const mValue& v)
{
    return Traits<ct::Seat>::unpackInt(v);
}

mValue Traits<ct::RbdOrder2>::packInt(const ct::RbdOrder2& s)
{
    return mValue(s.toString());
}

UnpackResult<ct::RbdOrder2> Traits<ct::RbdOrder2>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(ct::RbdOrder2, v, json_spirit::str_type);
    const boost::optional<ct::RbdOrder2> order(ct::RbdOrder2::getRbdOrder(v.get_str()));
    if (!order) {
        return UnpackError{STDLOG, "getRbdOrder failed"};
    }
    return *order;
}

mValue Traits<ct::RbdOrder2>::packExt(const ct::RbdOrder2& i)
{
    return Traits<ct::RbdOrder2>::packInt(i);
}

UnpackResult<ct::RbdOrder2> Traits<ct::RbdOrder2>::unpackExt(const mValue& v)
{
    return Traits<ct::RbdOrder2>::unpackInt(v);
}

mValue Traits<ct::RbdLayout>::packInt(const ct::RbdLayout& s)
{
    return mValue(s.toString());
}

UnpackResult<ct::RbdLayout> Traits<ct::RbdLayout>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(ct::RbdLayout, v, json_spirit::str_type);
    const boost::optional<ct::RbdLayout> rl(ct::RbdLayout::fromString(v.get_str()));
    if (!rl) {
        return UnpackError{STDLOG, "RbdLayout::fromString failed"};
    }
    return *rl;
}

mValue Traits<ct::RbdLayout>::packExt(const ct::RbdLayout& i)
{
    return Traits<ct::RbdLayout>::packInt(i);
}

UnpackResult<ct::RbdLayout> Traits<ct::RbdLayout>::unpackExt(const mValue& v)
{
    return Traits<ct::RbdLayout>::unpackInt(v);
}
//#############################################################################
mValue Traits<ct::Cabins>::packInt(const ct::Cabins& s)
{
    return mValue(ct::cabinsCode(s, ENGLISH));
}

UnpackResult<ct::Cabins> Traits<ct::Cabins>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(ct::Cabins, v, json_spirit::str_type);
    const boost::optional<ct::Cabins> cabins(ct::cabinsFromString(v.get_str()));
    if (!cabins) {
        return UnpackError{STDLOG, "cabinsFromString failed"};
    }
    return *cabins;
}

mValue Traits<ct::Cabins>::packExt(const ct::Cabins& i)
{
    return Traits<ct::Cabins>::packInt(i);
}

UnpackResult<ct::Cabins> Traits<ct::Cabins>::unpackExt(const mValue& v)
{
    return Traits<ct::Cabins>::unpackInt(v);
}
//#############################################################################
mValue Traits<ct::Rbds>::packInt(const ct::Rbds& s)
{
    return mValue(ct::rbdsCode(s, ENGLISH));
}

UnpackResult<ct::Rbds> Traits<ct::Rbds>::unpackInt(const mValue& v)
{
    if (v.type() == json_spirit::str_type) {
        const boost::optional<ct::Rbds> rbds(ct::rbdsFromStr(v.get_str()));
        if (!rbds) {
            return UnpackError{STDLOG, "rbdsFromStr failed"};
        }
        return *rbds;
    }
    if (v.type() == json_spirit::array_type) {
        const UnpackResult<std::vector<int> > unpacked = json_spirit::Traits<std::vector<int> >::unpackInt(v);
        if (!unpacked) {
            return UnpackError{STDLOG, "Rbds failed to unpack vector<int>"};
        }
        return ct::Rbds(unpacked->begin(), unpacked->end());
    }
    return UnpackError{STDLOG, "Rbds invalid type"};
}

mValue Traits<ct::Rbds>::packExt(const ct::Rbds& i)
{
    return Traits<ct::Rbds>::packInt(i);
}

UnpackResult<ct::Rbds> Traits<ct::Rbds>::unpackExt(const mValue& v)
{
    return Traits<ct::Rbds>::unpackInt(v);
}
//#############################################################################

json_spirit::mValue Traits<ct::Flight>::packInt(const ct::Flight& v)
{
    json_spirit::mValue value;
    char str[32] = {};
    if (v.suffix.get() == 0) {
        sprintf(str, "%d-%d", v.airline.get(), v.number.get());
    } else {
        sprintf(str, "%d-%d/%d", v.airline.get(), v.number.get(), v.suffix.get());
    }
    value = str;
    return value;
}

UnpackResult<ct::Flight> Traits<ct::Flight>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(ct::Flight, v, json_spirit::str_type);
    std::string flight = v.get_str();
    std::size_t sepPos = flight.find("-");
    if (sepPos == std::string::npos) {
        return UnpackError{STDLOG, "invalid format"};
    }

    std::size_t slashPos = flight.find("/", sepPos);
    int suffix = 0;
    if (slashPos != std::string::npos) {
        std::string suffStr = flight.substr(slashPos + 1);
        suffix = std::stoi(suffStr);
    }

    std::string fltStr = flight.substr(sepPos + 1, slashPos - sepPos - 1);
    int flightCode = std::stoi(fltStr);

    std::string awkStr = flight.substr(0, sepPos);
    int awk = std::stoi(awkStr);

    return ct::Flight(nsi::CompanyId(awk), ct::FlightNum(flightCode), ct::Suffix(suffix));
}

mValue Traits<ct::Flight>::packExt(const ct::Flight& v)
{
    json_spirit::mValue value;
    value = v.toString();
    return value;
}

UnpackResult<ct::Flight> Traits<ct::Flight>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(ct::Flight, v, json_spirit::str_type);
    const boost::optional<ct::Flight> f(ct::Flight::fromStr(EncString::fromUtf(v.get_str()).to866()));
    if (!f) {
        return UnpackError{STDLOG, "Flight::fromStr failed"};
    }
    return *f;
}

mValue Traits<ct::FlightDate>::packInt(const ct::FlightDate& v)
{
    return Traits<std::pair<ct::Flight, boost::gregorian::date>>::packInt({v.flt, v.dt});
}

UnpackResult<ct::FlightDate> Traits<ct::FlightDate>::unpackInt(const mValue& v)
{
    UnpackResult<std::pair<ct::Flight, boost::gregorian::date>> res
        = Traits<std::pair<ct::Flight, boost::gregorian::date>>::unpackInt(v);
    if (!res) {
        return res.err();
    }
    return ct::FlightDate{res->first, res->second};
}

mValue Traits<ct::FlightDate>::packExt(const ct::FlightDate& v)
{
    return Traits<std::pair<ct::Flight, boost::gregorian::date>>::packExt({v.flt, v.dt});
}

UnpackResult<ct::FlightDate> Traits<ct::FlightDate>::unpackExt(const mValue& v)
{
    UnpackResult<std::pair<ct::Flight, boost::gregorian::date>> res
        = Traits<std::pair<ct::Flight, boost::gregorian::date>>::unpackExt(v);
    if (!res) {
        return res.err();
    }
    return ct::FlightDate{res->first, res->second};
}

using ct::FlightSeg;
JSON_BEGIN_DESC_TYPE(FlightSeg)
    DESC_TYPE_FIELD("flt", flt)
    DESC_TYPE_FIELD("seg", seg)
JSON_END_DESC_TYPE(FlightSeg)

using ct::FlightSegDate;
JSON_BEGIN_DESC_TYPE(FlightSegDate)
    DESC_TYPE_FIELD("flt", flt)
    DESC_TYPE_FIELD("seg", seg)
    DESC_TYPE_FIELD("dt", dt)
JSON_END_DESC_TYPE(FlightSegDate)

mValue Traits<Freq>::packInt(const Freq& f)
{
    return mValue(f.str());
}

UnpackResult<Freq> Traits<Freq>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(Freq, v, json_spirit::str_type);
    return Freq(v.get_str());
}

mValue Traits<Freq>::packExt(const Freq& f)
{
    return mValue(f.str());
}

UnpackResult<Freq> Traits<Freq>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(Freq, v, json_spirit::str_type);
    return Freq(v.get_str());
}

json_spirit::mValue Traits<Period>::packInt(const Period& v)
{
    json_spirit::mValue value;
    const std::string freq = v.freq.normalString();
    value = Dates::yyyymmdd(v.start) + "-" + Dates::yyyymmdd(v.end)
        + "/" + (freq == "1234567" ? "8" : freq) + (v.biweekly ? "/2" : "");
    return value;
}

UnpackResult<Period> Traits<Period>::unpackInt(const mValue& v)
{
    std::string periodStr = v.get_str();
    std::size_t delimPos = periodStr.find("-");
    if (delimPos == std::string::npos) {
        return UnpackError{STDLOG, "invalid format"};
    }

    std::size_t freqSlash = periodStr.find("/", delimPos + 1);
    if (freqSlash == std::string::npos) {
        return UnpackError{STDLOG, "invalid format"};
    }

    std::string startStr = periodStr.substr(0, delimPos);
    std::string endStr = periodStr.substr(delimPos + 1, freqSlash - delimPos - 1);

    std::size_t bwSlash = periodStr.find("/", freqSlash + 1);
    std::string freqStr = periodStr.substr(freqSlash + 1
                                           , bwSlash == std::string::npos ? bwSlash : bwSlash - freqSlash - 1);

    std::string bwStr;
    if (bwSlash != std::string::npos) {
        bwStr = periodStr.substr(bwSlash + 1);
    }
    const boost::optional<Period> p(Period::create(Dates::DateFromYYYYMMDD(startStr)
                , Dates::DateFromYYYYMMDD(endStr)
                , Freq(freqStr == "8" ? "1234567" : freqStr)
                , !bwStr.empty()));
    if (!p) {
        return UnpackError{STDLOG, "Period::create failed"};
    }
    return *p;
}

mValue Traits<Period>::packExt(const Period& v)
{
    return packInt(v);
}

UnpackResult<Period> Traits<Period>::unpackExt(const mValue& v)
{
    return unpackInt(v);
}

json_spirit::mValue Traits<DayRange>::packInt(const DayRange& v)
{
    json_spirit::mValue value;
    value = (v.start == v.end) ? Dates::yyyymmdd(v.start) :
                                 Dates::yyyymmdd(v.start) + "-" + Dates::yyyymmdd(v.end);
    return value;
}

static boost::optional<DayRange> unpackDayRangeFromStr(const mValue& v)
{
    std::string rangeStr = v.get_str();
    std::size_t delimPos = rangeStr.find("-");
    if (delimPos == std::string::npos) {
        boost::gregorian::date dt(Dates::DateFromYYYYMMDD(rangeStr));
        return DayRange(dt, dt);
    } else {
        return DayRange(Dates::DateFromYYYYMMDD(rangeStr.substr(0, delimPos)),
                        Dates::DateFromYYYYMMDD(rangeStr.substr(delimPos + 1, rangeStr.size() - delimPos)));
    }
    return boost::none;
}

UnpackResult<DayRange> Traits<DayRange>::unpackInt(const mValue& v)
{
    if (v.type() == json_spirit::str_type) {
        const boost::optional<DayRange> r(unpackDayRangeFromStr(v));
        if (!r) {
            return UnpackError{STDLOG, "invalid format"};
        }
        return *r;
    }
    // old style pack - pair of dates
    UnpackResult<std::pair<boost::gregorian::date,boost::gregorian::date> >
        datesPair(Traits< std::pair<boost::gregorian::date, boost::gregorian::date> >::unpackInt(v));
    if (datesPair) {
        return DayRange(datesPair->first, datesPair->second);
    }
    return datesPair.err();
}

mValue Traits<DayRange>::packExt(const DayRange& v)
{
    return packInt(v);
}

UnpackResult<DayRange> Traits<DayRange>::unpackExt(const mValue& v)
{
    if (v.type() == json_spirit::str_type) {
        const boost::optional<DayRange> r(unpackDayRangeFromStr(v));
        if (!r) {
            return UnpackError{STDLOG, "invalid format"};
        }
        return *r;
    }
    // old style pack - pair of dates
    UnpackResult<std::pair<boost::gregorian::date,boost::gregorian::date> >
        datesPair(Traits< std::pair<boost::gregorian::date, boost::gregorian::date> >::unpackExt(v));
    if (datesPair) {
        return DayRange(datesPair->first, datesPair->second);
    }
    return datesPair.err();
}


json_spirit::mValue Traits<ct::PredPoint>::packInt(const ct::PredPoint& v)
{
    return json_spirit::mValue(v.str());
}

UnpackResult<ct::PredPoint> Traits<ct::PredPoint>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(ct::PredPoint, v, json_spirit::str_type);
    const boost::optional<ct::PredPoint> pp(ct::PredPoint::create(v.get_str()));
    if (!pp) {
        return UnpackError{STDLOG, "PredPoint::create failed"};
    }
    return *pp;
}

mValue Traits<ct::PredPoint>::packExt(const ct::PredPoint& v)
{
    return packInt(v);
}

UnpackResult<ct::PredPoint> Traits<ct::PredPoint>::unpackExt(const mValue& v)
{
    return unpackInt(v);
}

json_spirit::mValue Traits<ct::SegTime>::packInt(const ct::SegTime& v)
{
    return json_spirit::mValue(v.makeText());
}

UnpackResult<ct::SegTime> Traits<ct::SegTime>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(ct::SegTime, v, json_spirit::str_type);
    const boost::optional<ct::SegTime> st(ct::SegTime::create(v.get_str()));
    if (!st) {
        return UnpackError{STDLOG, "SegTime::create failed"};
    }
    return *st;
}

mValue Traits<ct::SegTime>::packExt(const ct::SegTime& v)
{
    return packInt(v);
}

UnpackResult<ct::SegTime> Traits<ct::SegTime>::unpackExt(const mValue& v)
{
    return unpackInt(v);
}

mValue Traits<ct::DocTypeEx>::packInt(const ct::DocTypeEx& v)
{
    if (v.id) {
        return Traits<nsi::DocTypeId>::packInt(*v.id);
    } else {
        return Traits<EncString>::packInt(v.str);
    }
}

UnpackResult<ct::DocTypeEx> Traits<ct::DocTypeEx>::unpackInt(const mValue& v)
{
    if (v.type() == json_spirit::int_type) {
        const UnpackResult<nsi::DocTypeId> retId = Traits<nsi::DocTypeId>::unpackInt(v);
        if (!retId) {
            return retId.err();
        }
        return ct::DocTypeEx(*retId);
    } else if (v.type() == json_spirit::str_type) {
        const UnpackResult<EncString> retStr = Traits<EncString>::unpackInt(v);
        if (!retStr) {
            return retStr.err();
        }
        return ct::DocTypeEx(*retStr);
    }
    return UnpackError{STDLOG, "DocTypeEx: invalid type " + HelpCpp::string_cast(v.type())};
}

mValue Traits<ct::DocTypeEx>::packExt(const ct::DocTypeEx& v)
{
    if (v.id) {
        return Traits<nsi::DocTypeId>::packExt(*v.id);
    } else {
        return Traits<EncString>::packExt(v.str);
    }
}

UnpackResult<ct::DocTypeEx> Traits<ct::DocTypeEx>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(ct::DocTypeEx, v, json_spirit::str_type);
    const UnpackResult<nsi::DocTypeId> retId = Traits<nsi::DocTypeId>::unpackExt(v);
    if (!retId) {
        const UnpackResult<EncString> retStr = Traits<EncString>::unpackExt(v);
        if (!retStr) {
            return retStr.err();
        }
        return ct::DocTypeEx(*retStr);
    }
    return ct::DocTypeEx(*retId);
}

using ct::Checkin;
JSON_BEGIN_DESC_TYPE(Checkin)
    DESC_TYPE_FIELD("time", time)
    DESC_TYPE_FIELD("luggage", luggage)
    DESC_TYPE_FIELD("dt", dt)
    DESC_TYPE_FIELD("at", at)
JSON_END_DESC_TYPE(Checkin)

json_spirit::mValue Traits<ct::SegStatus>::packInt(const ct::SegStatus& v)
{
    return json_spirit::Traits<ct::SegStatus::Code>::packInt(v.code());
}

UnpackResult<ct::SegStatus> Traits<ct::SegStatus>::unpackInt(const mValue& v)
{
    const UnpackResult<ct::SegStatus::Code> c(Traits<ct::SegStatus::Code>::unpackInt(v));
    if (c) {
        return ct::SegStatus(*c);
    }
    return c.err();
}

mValue Traits<ct::SegStatus>::packExt(const ct::SegStatus& v)
{
    return json_spirit::Traits<ct::SegStatus::Code>::packExt(v.code());
}

UnpackResult<ct::SegStatus> Traits<ct::SegStatus>::unpackExt(const mValue& v)
{
    const UnpackResult<ct::SegStatus::Code> c(Traits<ct::SegStatus::Code>::unpackExt(v));
    if (c) {
        return ct::SegStatus(*c);
    }
    return c.err();
}

json_spirit::mValue Traits<ct::ArrStatus>::packInt(const ct::ArrStatus& v)
{
    return json_spirit::Traits<ct::ArrStatus::Code>::packInt(v.code());
}

UnpackResult<ct::ArrStatus> Traits<ct::ArrStatus>::unpackInt(const mValue& v)
{
    const UnpackResult<ct::ArrStatus::Code> c(Traits<ct::ArrStatus::Code>::unpackInt(v));
    if (c) {
        return ct::ArrStatus(*c);
    }
    return c.err();
}

mValue Traits<ct::ArrStatus>::packExt(const ct::ArrStatus& v)
{
    return json_spirit::Traits<ct::ArrStatus::Code>::packExt(v.code());
}

UnpackResult<ct::ArrStatus> Traits<ct::ArrStatus>::unpackExt(const mValue& v)
{
    const UnpackResult<ct::ArrStatus::Code> c(Traits<ct::ArrStatus::Code>::unpackExt(v));
    if (c) {
        return ct::ArrStatus(*c);
    }
    return c.err();
}

json_spirit::mValue Traits<ct::SsrStatus>::packInt(const ct::SsrStatus& v)
{
    return json_spirit::Traits<ct::SsrStatus::Code>::packInt(v.code());
}

UnpackResult<ct::SsrStatus> Traits<ct::SsrStatus>::unpackInt(const mValue& v)
{
    const UnpackResult<ct::SsrStatus::Code> c(Traits<ct::SsrStatus::Code>::unpackInt(v));
    if (c) {
        return ct::SsrStatus(*c);
    }
    return c.err();
}

mValue Traits<ct::SsrStatus>::packExt(const ct::SsrStatus& v)
{
    return json_spirit::Traits<ct::SsrStatus::Code>::packExt(v.code());
}

UnpackResult<ct::SsrStatus> Traits<ct::SsrStatus>::unpackExt(const mValue& v)
{
    const UnpackResult<ct::SsrStatus::Code> c(Traits<ct::SsrStatus::Code>::unpackExt(v));
    if (c) {
        return ct::SsrStatus(*c);
    }
    return c.err();
}

json_spirit::mValue Traits<ct::SvcStatus>::packInt(const ct::SvcStatus& v)
{
    return json_spirit::Traits<ct::SvcStatus::Code>::packInt(v.code());
}

UnpackResult<ct::SvcStatus> Traits<ct::SvcStatus>::unpackInt(const mValue& v)
{
    const UnpackResult<ct::SvcStatus::Code> c(Traits<ct::SvcStatus::Code>::unpackInt(v));
    if (c) {
        return ct::SvcStatus(*c);
    }
    return c.err();
}

mValue Traits<ct::SvcStatus>::packExt(const ct::SvcStatus& v)
{
    return json_spirit::Traits<ct::SvcStatus::Code>::packExt(v.code());
}

UnpackResult<ct::SvcStatus> Traits<ct::SvcStatus>::unpackExt(const mValue& v)
{
    const UnpackResult<ct::SvcStatus::Code> c(Traits<ct::SvcStatus::Code>::unpackExt(v));
    if (c) {
        return ct::SvcStatus(*c);
    }
    return c.err();
}

JSON_RIP_PACK(ct::RfiscSubCode);
JSON_RIP_PACK(ct::RfiscCommercialName);
JSON_RIP_PACK(ct::RfiscGroupCode);
JSON_RIP_PACK(ct::RfiscSubGroupCode);

mValue Traits<ct::Rfic>::packExt(const ct::Rfic& rfic)
{
    return mValue(enumToStr(rfic));
}

mValue Traits<ct::Rfic>::packInt(const ct::Rfic& rfic)
{
    return Traits<ct::Rfic>::packExt(rfic);
}

UnpackResult<ct::Rfic> Traits<ct::Rfic>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(ct::Rfic, v, json_spirit::str_type);
    ct::Rfic e;
    if (!enumFromStr(e, v.get_str())) {
        return UnpackError{STDLOG, "unknown Rfic code"};
    }
    return e;
}

UnpackResult<ct::Rfic> Traits<ct::Rfic>::unpackInt(const mValue& v)
{
    return Traits<ct::Rfic>::unpackExt(v);
}

using ct::RfiscGroup;
JSON_BEGIN_DESC_TYPE(RfiscGroup)
    DESC_TYPE_FIELD("code", code)
    DESC_TYPE_FIELD("name", name)
JSON_END_DESC_TYPE(RfiscGroup)

using ct::RfiscSubGroup;
JSON_BEGIN_DESC_TYPE(RfiscSubGroup)
    DESC_TYPE_FIELD("code", code)
    DESC_TYPE_FIELD("name", name)
    DESC_TYPE_FIELD("groupCode", groupCode)
JSON_END_DESC_TYPE(RfiscSubGroup)

JSON_RIP_PACK(ct::TypebAddress);
JSON_RIP_PACK(ct::EdiAddress);

mValue Traits<ct::SsrCode>::packInt(const ct::SsrCode& ssr)
{
    return mValue(ssr.get());
}

UnpackResult<ct::SsrCode> Traits<ct::SsrCode>::unpackInt(const mValue& v)
{
    if (v.type() == json_spirit::str_type) {
        return unpackExt(v);
    }
    if (v.type() == json_spirit::int_type) {
        try {
            const std::string s = nsi::SsrType(nsi::SsrTypeId(v.get_int())).code(ENGLISH).toUtf();
            if (const auto ssr = ct::SsrCode::create(s)) {
                return *ssr;
            }
            LogError(STDLOG) << "unpack SsrCode " << v.get_int() << " failed";
            return UnpackError{STDLOG, "invalid int for SsrCode"};
        } catch (const nsi::Exception& e) {
            LogError(STDLOG) << "unpack SsrCode " << v.get_int() << " failed: "
                << e.what();
            return UnpackError{STDLOG, "invalid int for SsrCode"};
        }
    }
    return UnpackError{STDLOG, "invalid type"};
}

mValue Traits<ct::SsrCode>::packExt(const ct::SsrCode& ssr)
{
    return mValue(ssr.get());
}

UnpackResult<ct::SsrCode> Traits<ct::SsrCode>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(ct::SsrCode, v, json_spirit::str_type);
    const auto ssr = ct::SsrCode::create(v.get_str());
    if (!ssr) {
        return UnpackError{STDLOG, "invalid string for SsrCode"};
    }
    return *ssr;
}

} // json_spirit

#ifdef XP_TESTING
#include <serverlib/timer.h>
#include <nsi/callbacks.h>


#include <serverlib/checkunit.h>

void init_packer_tests(){}

namespace {

using json_spirit::UnpackResult;
using json_spirit::UnpackError;

START_TEST(check_flight_pack)
{
    boost::optional<ct::Flight> flt(ct::Flight::fromStr("U6-100A"));
    fail_unless(static_cast<bool>(flt), "flight fromStr failed");

    json_spirit::mValue v;
    std::string s;
    s = json_spirit::write(json_spirit::Traits<ct::Flight>::packInt(*flt));
    CHECK_EQUAL_STRINGS(s, "\"122-100/1\"");
    fail_unless(json_spirit::read(s, v));
    UnpackResult<ct::Flight> f(json_spirit::Traits<ct::Flight>::unpackInt(v));
    fail_unless(static_cast<bool>(f), "flight unpackInt failed");
    fail_unless(*flt == *f);

    s = json_spirit::write(json_spirit::Traits<ct::Flight>::packExt(*flt));
    CHECK_EQUAL_STRINGS(s, "\"U6-100A\"");
    fail_unless(json_spirit::read(s, v));
    f = json_spirit::Traits<ct::Flight>::unpackExt(v);
    fail_unless(static_cast<bool>(f), "flight unpackExt failed");
    fail_unless(*flt == *f);

    boost::optional<ct::Flight> flt2(ct::Flight::fromStr("UT-200"));
    fail_unless(static_cast<bool>(flt2), "flight fromStr failed");

    s = json_spirit::write(json_spirit::Traits<ct::Flight>::packInt(*flt2));
    CHECK_EQUAL_STRINGS(s, "\"119-200\"");
    fail_unless(json_spirit::read(s, v));
    f = json_spirit::Traits<ct::Flight>::unpackInt(v);
    fail_unless(static_cast<bool>(f), "flight unpackInt failed");
    fail_unless(*flt2 == *f);

    s = json_spirit::write(json_spirit::Traits<ct::Flight>::packExt(*flt2));
    CHECK_EQUAL_STRINGS(s, "\"UT-200\"");
    fail_unless(json_spirit::read(s, v));
    f = json_spirit::Traits<ct::Flight>::unpackExt(v);
    fail_unless(static_cast<bool>(f), "flight unpackExt failed");
    fail_unless(*flt2 == *f);
} END_TEST

START_TEST(check_period_pack)
{
    boost::gregorian::date start = Dates::ddmmyyyy("12052012");
    boost::gregorian::date end = Dates::ddmmyyyy("12072012");

    const Period p1(start, end, Freq("12345"));

    json_spirit::mValue v;
    std::string s;
    s = json_spirit::write(json_spirit::Traits<Period>::packInt(p1));
    CHECK_EQUAL_STRINGS(s, "\"20120512-20120712/12345\"");
    fail_unless(json_spirit::read(s, v));
    UnpackResult<Period> p(json_spirit::Traits<Period>::unpackInt(v));
    fail_unless(static_cast<bool>(p), "period unpackInt failed");
    fail_unless(p1 == *p);

    const Period p2(start, end, Freq("1234567"));
    s = json_spirit::write(json_spirit::Traits<Period>::packInt(p2));
    CHECK_EQUAL_STRINGS(s, "\"20120512-20120712/8\"");
    fail_unless(json_spirit::read(s, v));
    p = json_spirit::Traits<Period>::unpackInt(v);
    fail_unless(static_cast<bool>(p), "period unpackInt failed");
    fail_unless(p2 == *p);

} END_TEST

START_TEST (check_rbds_unpack)
{
    std::string str1 = "ABCDE";
    UnpackResult<ct::Rbds> r1 = json_spirit::Traits<ct::Rbds>::unpackInt(str1);
    fail_if(!r1, "new format unpack failed");
    // old format
    std::string str2;
    for (size_t i = 0; i < str1.size(); ++i)
        str2 += "," + HelpCpp::string_cast(*ct::rbdFromStr(str1.substr(i, 1)));
    str2 = "[" + str2.substr(1) + "]";
    LogTrace(TRACE5) << str2;
    json_spirit::mValue val;
    json_spirit::read(str2, val);
    UnpackResult<ct::Rbds> r2 = json_spirit::Traits<ct::Rbds>::unpackInt(val);
    fail_if(!r2, "old format unpack failed");
    fail_if(r1 != r2, "different unpack result");
} END_TEST

#define SUITENAME "coretypes"
TCASEREGISTER(nsi::setupTestNsi, 0)
    ADD_TEST(check_flight_pack);
    ADD_TEST(check_period_pack);
    ADD_TEST(check_rbds_unpack);
TCASEFINISH

}

#endif /* XP_TESTING */
