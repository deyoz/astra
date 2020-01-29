#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include "packer.h"

#include <serverlib/json_packer_heavy.h>
#include <serverlib/json_pack_types.h>
#include <serverlib/string_cast.h>
#include <serverlib/str_utils.h>
#include <serverlib/text_codec.h>


#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

#define JSON_RIP_NSI_DESC(RipType, ObjType) \
mValue Traits<RipType>::packExt(const RipType& s) \
{ return mValue(ObjType(s).code(ENGLISH).toDb()); } \
UnpackResult<RipType> Traits<RipType>::unpackExt(const mValue& v) \
{ \
    JSON_ASSERT_TYPE(RipType, v, json_spirit::str_type); \
    EncString s; \
    try { \
        s = EncString::fromUtf(v.get_str()); \
    } catch (const HelpCpp::ConvertException&) { \
        s = EncString::fromDb(v.get_str()); \
    } \
    if (!ObjType::find(s)) { \
        return UnpackError{STDLOG, #ObjType " code not found: " + v.get_str()}; \
    } \
    return ObjType(s).id(); \
} \
JSON_RIP_PACK_INT(RipType)

namespace json_spirit
{

JSON_RIP_NSI_DESC(nsi::DocTypeId, nsi::DocType);
JSON_RIP_NSI_DESC(nsi::SsrTypeId, nsi::SsrType);
JSON_RIP_NSI_DESC(nsi::GeozoneId, nsi::Geozone);
JSON_RIP_NSI_DESC(nsi::CountryId, nsi::Country);
JSON_RIP_NSI_DESC(nsi::RegionId, nsi::Region);
JSON_RIP_NSI_DESC(nsi::CityId, nsi::City);
JSON_RIP_NSI_DESC(nsi::PointId, nsi::Point);
JSON_RIP_PACK(nsi::TermId);
JSON_RIP_NSI_DESC(nsi::CompanyId, nsi::Company);
JSON_RIP_NSI_DESC(nsi::AircraftTypeId, nsi::AircraftType);
JSON_RIP_NSI_DESC(nsi::RouterId, nsi::Router);
JSON_RIP_NSI_DESC(nsi::RestrictionId, nsi::Restriction);
JSON_RIP_NSI_DESC(nsi::MealServiceId, nsi::MealService);
JSON_RIP_NSI_DESC(nsi::InflServiceId, nsi::InflService);
JSON_RIP_NSI_DESC(nsi::ServiceTypeId, nsi::ServiceType);
JSON_RIP_NSI_DESC(nsi::CurrencyId, nsi::Currency);

JSON_PACK_UNPACK_ENUM(nsi::FltServiceType);

template<typename DepArr, typename T>
mValue depArrPack(const DepArr& depArr, bool internal)
{
    std::ostringstream os;
    if (internal) {
        os << depArr.dep << "-" << depArr.arr;
    } else {
        os << T(depArr.dep).code(ENGLISH).toUtf() << "-" << T(depArr.arr).code(ENGLISH).toUtf();
    }
    return mValue(os.str());
}


mValue Traits<nsi::DepArrPoints>::packInt(const nsi::DepArrPoints& dap)
{
    return depArrPack<nsi::DepArrPoints, nsi::Point>(dap, true);
}

template<typename DepArr, typename T>
UnpackResult<DepArr> unpackObsoleteDap(const mValue& v, bool internal)
{
    JSON_ASSERT_TYPE(Type, v, json_spirit::obj_type);
    const mObject& o( v.get_obj() );
    mObject::const_iterator tmp, oend = o.end();
    tmp = o.find( "from" );
    if (tmp == oend) {
        return UnpackError{STDLOG, "tag 'from' not found"};
    }
    const UnpackResult<T> dep = internal
        ? Traits<T>::unpackInt(tmp->second)
        : Traits<T>::unpackExt(tmp->second);
    if (!dep) {
        return UnpackError{STDLOG, dep.err().text, {"from"}};
    }
    tmp = o.find( "to" );
    if (tmp == oend) {
        return UnpackError{STDLOG, "tag 'to' not found"};
    }
    const UnpackResult<T> arr = internal
        ? Traits<T>::unpackInt(tmp->second)
        : Traits<T>::unpackExt(tmp->second);
    if (!arr) {
        return UnpackError{STDLOG, dep.err().text, {"to"}};
    }
    return DepArr(*dep,*arr);
}

template<typename DepArr, typename T>
UnpackResult<DepArr> depArrUnpack(const mValue& v, bool internal)
{
    typedef typename T::id_type IdType;
    if (v.type() == json_spirit::str_type) {
        if (internal) {
            std::vector<int> dapIds;
            StrUtils::ParseStringToVecInt(dapIds, v.get_str(), '-');
            if (dapIds.size() != 2) {
                return UnpackError{STDLOG, "invalid string [" + v.get_str() + "]"};
            }
            return DepArr(IdType(dapIds.front()), IdType(dapIds.back()));
        } else {
            std::vector<std::string> dapIds;
            StrUtils::split_string(dapIds, v.get_str(), '-', false);
            if (dapIds.size() != 2) {
                return UnpackError{STDLOG, "invalid string [" + v.get_str() + "]"};
            }
            EncString dep(EncString::fromUtf(dapIds.front()));
            EncString arr(EncString::fromUtf(dapIds.back()));
            return DepArr(T(dep).id(), T(arr).id());
        }
    }
    // packed in old style
    return unpackObsoleteDap<DepArr, IdType>(v, internal);
}


UnpackResult<nsi::DepArrPoints> Traits<nsi::DepArrPoints>::unpackInt(const mValue& v)
{
    return depArrUnpack<nsi::DepArrPoints, nsi::Point>(v, true);
}

mValue Traits<nsi::DepArrPoints>::packExt(const nsi::DepArrPoints& dap)
{
    return depArrPack<nsi::DepArrPoints, nsi::Point>(dap, false);
}

UnpackResult<nsi::DepArrPoints> Traits<nsi::DepArrPoints>::unpackExt(const mValue& v)
{
    return depArrUnpack<nsi::DepArrPoints, nsi::Point>(v, false);
}

mValue Traits<nsi::DepArrCities>::packInt(const nsi::DepArrCities& dac)
{
    return depArrPack<nsi::DepArrCities, nsi::City>(dac, true);
}

mValue Traits<nsi::DepArrCities>::packExt(const nsi::DepArrCities& dac)
{
    return depArrPack<nsi::DepArrCities, nsi::City>(dac, false);
}

UnpackResult<nsi::DepArrCities> Traits<nsi::DepArrCities>::unpackInt(const mValue& v)
{
    return depArrUnpack<nsi::DepArrCities, nsi::City>(v, true);
}

UnpackResult<nsi::DepArrCities> Traits<nsi::DepArrCities>::unpackExt(const mValue& v)
{
    return depArrUnpack<nsi::DepArrCities, nsi::City>(v, false);
}

mValue Traits<nsi::Router>::packInt(const nsi::Router& r)
{
    ASSERT(false);
    return mValue();
}

mValue Traits<nsi::Router>::packExt(const nsi::Router& r)
{
    mObject obj;

    obj["id"] = json_spirit::Traits<int>::packExt(r.id().get());
    obj["blockAnswers"] = json_spirit::Traits<bool>::packExt(r.blockAnswers());
    obj["blockRequests"] = json_spirit::Traits<bool>::packExt(r.blockRequests());
    obj["canonName"] = json_spirit::Traits<std::string>::packExt(r.canonName());
    obj["ediTOnO"] = json_spirit::Traits<bool>::packExt(r.ediTOnO());
    obj["hth"] = json_spirit::Traits<bool>::packExt(r.hth());
    obj["hthAddress"] = json_spirit::Traits<std::string>::packExt(r.hthAddress());
    obj["ipAddress"] = json_spirit::Traits<std::string>::packExt(r.ipAddress());
    obj["ipPort"] = json_spirit::Traits<short>::packExt(r.ipPort());
    obj["loopback"] = json_spirit::Traits<bool>::packExt(r.loopback());
    obj["maxHthPartSize"] = json_spirit::Traits<unsigned>::packExt(r.maxHthPartSize());
    obj["maxPartSize"] = json_spirit::Traits<unsigned>::packExt(r.maxPartSize());
    obj["maxTypebPartSize"] = json_spirit::Traits<unsigned>::packExt(r.maxTypebPartSize());
    obj["ourHthAddres"] = json_spirit::Traits<std::string>::packExt(r.ourHthAddres());
    obj["remAddrNum"] = json_spirit::Traits<unsigned>::packExt(r.remAddrNum());
    obj["responseTimeout"] = json_spirit::Traits<unsigned>::packExt(r.responseTimeout());
    obj["censor"] = json_spirit::Traits<bool>::packExt(r.censor());
    obj["translit"] = json_spirit::Traits<bool>::packExt(r.translit());
    obj["sendContrl"] = json_spirit::Traits<bool>::packExt(r.sendContrl());
    obj["senderName"] = json_spirit::Traits<std::string>::packExt(r.senderName());
    obj["tprLen"] = json_spirit::Traits<unsigned>::packExt(r.tprLen());
    obj["trueTypeB"] = json_spirit::Traits<bool>::packExt(r.trueTypeB());
    obj["tpb"] = json_spirit::Traits<bool>::packExt(r.tpb());
    obj["email"] = json_spirit::Traits<bool>::packExt(r.email());

    return mValue(obj);
}

UnpackResult<nsi::Router> Traits<nsi::Router>::unpackInt(const mValue& v)
{
    return UnpackError{STDLOG, "not supported"};
}

UnpackResult<nsi::Router> Traits<nsi::Router>::unpackExt(const mValue& v)
{
    return UnpackError{STDLOG, "not supported"};
}

mValue Traits<EncString>::packInt(const EncString& str)
{
    return mValue(str.toDb());
}

UnpackResult<EncString> Traits<EncString>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(EncString, v, json_spirit::str_type);
    return EncString::fromDb(v.get_str());
}

mValue Traits<EncString>::packExt(const EncString& str)
{
    return Traits<EncString>::packInt(str);
}

UnpackResult<EncString> Traits<EncString>::unpackExt(const mValue& v)
{
    return Traits<EncString>::unpackInt(v);
}

//
// CityPort
//

mValue Traits<nsi::CityPoint>::packInt(const nsi::CityPoint& v)
{
    if (v.pointId()) {
        return mValue("p:" + HelpCpp::string_cast(v.pointId()->get()));
    }
    return mValue("c:" + HelpCpp::string_cast(v.cityId()));
}

UnpackResult<nsi::CityPoint> Traits<nsi::CityPoint>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(nsi::CityPoint, v, json_spirit::str_type);
    const std::string& cityPointCode = v.get_str();
    char type = 0;
    int code = 0;
    if (sscanf(cityPointCode.c_str(), "%c:%d", &type, &code) != 2) {
        return UnpackError{STDLOG, "CityPort invalid string: " + cityPointCode};
    }
    if (type == 'c') {
        return nsi::CityPoint(nsi::CityId(code));
    } else if (type == 'p') {
        return nsi::CityPoint(nsi::PointId(code));
    }
    return UnpackError{STDLOG, "CityPort invalid type"};
}

mValue Traits<nsi::CityPoint>::packExt(const nsi::CityPoint& v)
{
    return Traits<EncString>::packExt(v.code(ENGLISH));
}

UnpackResult<nsi::CityPoint> Traits<nsi::CityPoint>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(nsi::CityPoint, v, json_spirit::str_type);
    const UnpackResult<EncString> cityPort = Traits<EncString>::unpackExt(v);
    if (!cityPort) {
        return UnpackError{STDLOG, "CityPort invalid EncString"};
    }
    if (!nsi::CityPoint::find(*cityPort)) {
        return UnpackError{STDLOG, "CityPort code not found: " + cityPort->toUtf()};
    }
    return nsi::CityPoint(*cityPort);
}

mValue Traits<nsi::PointOrCode>::packInt(const nsi::PointOrCode& v)
{
    if (v.id()) {
        return Traits<nsi::PointId>::packInt(*v.id());
    } else {
        return Traits<EncString>::packInt(v.code(ENGLISH));
    }
}

UnpackResult<nsi::PointOrCode> Traits<nsi::PointOrCode>::unpackInt(const mValue& v)
{
    if (v.type() == json_spirit::int_type) {
        const UnpackResult<nsi::PointId> retId = Traits<nsi::PointId>::unpackInt(v);
        if (!retId) {
            return retId.err();
        }
        return nsi::PointOrCode(*retId);
    } else if (v.type() == json_spirit::str_type) {
        const UnpackResult<EncString> retStr = Traits<EncString>::unpackInt(v);
        if (!retStr) {
            return retStr.err();
        }
        return nsi::PointOrCode(*retStr);
    }
    return UnpackError{STDLOG, "PointOrCode: invalid type " + HelpCpp::string_cast(v.type())};
}

mValue Traits<nsi::PointOrCode>::packExt(const nsi::PointOrCode& v)
{
    if (v.id()) {
        return Traits<nsi::PointId>::packExt(*v.id());
    } else {
        return Traits<EncString>::packExt(v.code(ENGLISH));
    }
}

UnpackResult<nsi::PointOrCode> Traits<nsi::PointOrCode>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(nsi::PointOrCode, v, json_spirit::str_type);
    const UnpackResult<nsi::PointId> retId = Traits<nsi::PointId>::unpackExt(v);
    if (!retId) {
        const UnpackResult<EncString> retStr = Traits<EncString>::unpackExt(v);
        if (!retStr) {
            return retStr.err();
        }
        return nsi::PointOrCode(*retStr);
    }
    return nsi::PointOrCode(*retId);
}

} // json_spirit

