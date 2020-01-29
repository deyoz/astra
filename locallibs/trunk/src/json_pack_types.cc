#include "json_pack_types.h"

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "json_packer_heavy.h"
#include "helpcpp.h"
#include "str_utils.h"
#include "string_cast.h"
#include "dates.h"
#include "lngv_user.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

namespace json_spirit
{

mValue Traits<int>::packInt(const int& i)
{
    return mValue(i);
}

UnpackResult<int> Traits<int>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(int, v, json_spirit::int_type);
    return v.get_int();
}

mValue Traits<int>::packExt(const int& i)
{
    return mValue(i);
}

UnpackResult<int> Traits<int>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(int, v, json_spirit::int_type);
    return v.get_int();
}

mValue Traits<unsigned int>::packInt(const unsigned int& i)
{
    return mValue( static_cast< int >( i ) );
}

UnpackResult<unsigned int> Traits<unsigned int>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(unsigned int, v, json_spirit::int_type);
    return static_cast<unsigned int>(v.get_int());
}

mValue Traits<unsigned int>::packExt(const unsigned int& i)
{
    return mValue( static_cast< int >( i ) );
}

UnpackResult<unsigned int> Traits<unsigned int>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(unsigned int, v, json_spirit::int_type);
    return static_cast<unsigned int>(v.get_int());
}

mValue Traits<short>::packInt(const short& i)
{
    return mValue(i);
}
UnpackResult<short> Traits<short>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(short, v, json_spirit::int_type);
    return static_cast<short>(v.get_int());
}

mValue Traits<short>::packExt(const short& i)
{
    return mValue(i);
}

UnpackResult<short> Traits<short>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(short, v, json_spirit::int_type);
    return static_cast<short>(v.get_int());
}

mValue Traits<bool>::packInt(const bool& i)
{
    return mValue(i);
}

UnpackResult<bool> Traits<bool>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(bool, v, json_spirit::bool_type);
    return v.get_bool();
}

mValue Traits<bool>::packExt(const bool& i)
{
    return mValue(i);
}

UnpackResult<bool> Traits<bool>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(bool, v, json_spirit::bool_type);
    return v.get_bool();
}

mValue Traits<int64_t>::packInt(const int64_t& i)
{
    return mValue(i);
}

UnpackResult<int64_t> Traits<int64_t>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(int64_t, v, json_spirit::int_type);
    return v.get_int64();
}

mValue Traits<int64_t>::packExt(const int64_t& i)
{
    return mValue(i);
}

UnpackResult<int64_t> Traits<int64_t>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(int64_t, v, json_spirit::int_type);
    return v.get_int64();
}

mValue Traits<uint64_t>::packInt(const uint64_t& i)
{
    return mValue(i);
}

UnpackResult<uint64_t> Traits<uint64_t>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(uint64_t, v, json_spirit::int_type);
    return v.get_uint64();
}

mValue Traits<uint64_t>::packExt(const uint64_t& i)
{
    return mValue(i);
}

UnpackResult<uint64_t> Traits<uint64_t>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(uint64_t, v, json_spirit::int_type);
    return v.get_uint64();
}
mValue Traits<uint8_t>::packInt(const uint8_t& i)
{
    return mValue(std::string(1, i));
}

UnpackResult<uint8_t> Traits<uint8_t>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(uint8_t, v, json_spirit::str_type);
    std::string tmp(v.get_str());
    if (tmp.size() != 1) {
        return UnpackError{"invalid char str length"};
    }
    return static_cast<uint8_t>(tmp[0]);
}

mValue Traits<uint8_t>::packExt(const uint8_t& i)
{
    return mValue(std::string(1, i));
}

UnpackResult<uint8_t> Traits<uint8_t>::unpackExt( const mValue& v)
{
    return unpackInt(v);
}

mValue Traits<double>::packInt(const double& d)
{
    return mValue(d);
}

UnpackResult<double> Traits<double>::unpackInt(const mValue& v)
{
    if (v.type() == json_spirit::real_type) {
        return v.get_real();
    } else if (v.type() == json_spirit::int_type) {
        return v.get_int();
    }
    return UnpackError{"double invalid type " + HelpCpp::string_cast(v.type()) + " expected int or real"};
}

mValue Traits<double>::packExt(const double& d)
{
    return mValue(d);
}

UnpackResult<double> Traits<double>::unpackExt(const mValue& v)
{
    return unpackInt(v);
}

mValue Traits<std::string>::packInt(const std::string& s)
{
    return mValue(s);
}

UnpackResult<std::string> Traits<std::string>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(std::string, v, json_spirit::str_type);
    return v.get_str();
}

mValue Traits<std::string>::packExt(const std::string& s)
{
    return mValue(s);
}

UnpackResult<std::string> Traits<std::string>::unpackExt(const mValue& v)
{
    return unpackInt(v);
}

//
// boost::gregorian::date
//

mValue Traits<boost::gregorian::date>::packInt(const boost::gregorian::date& d)
{
    boost::gregorian::date tmp = !d.is_pos_infinity() ? d : boost::gregorian::date(4712, 12, 31);
    ASSERT(!tmp.is_not_a_date());
    return mValue(Dates::to_iso_string(tmp));
}

UnpackResult<boost::gregorian::date> Traits<boost::gregorian::date>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(boost::gregorian::date, v, json_spirit::str_type);
    boost::gregorian::date d(Dates::date_from_iso_string(v.get_str()));
    if (d == boost::gregorian::date(4712, 12, 31)) {
        d = boost::gregorian::date(boost::gregorian::pos_infin);
    }
    if (d.is_not_a_date()) {
        return UnpackError{"not a date"};
    }
    return d;
}

mValue Traits<boost::gregorian::date>::packExt(const boost::gregorian::date& d)
{
    return packInt(d);
}

UnpackResult<boost::gregorian::date> Traits<boost::gregorian::date>::unpackExt(const mValue& v)
{
    return unpackInt(v);
}

//
// boost::gregorian::date_duration
//

mValue Traits< boost::gregorian::date_duration >::packInt( boost::gregorian::date_duration const &d )
{
    return mValue( static_cast< int >( d.days() ) );
}

mValue Traits< boost::gregorian::date_duration >::packExt( boost::gregorian::date_duration const &d )
{
    return packInt(d);
}

UnpackResult< boost::gregorian::date_duration > Traits< boost::gregorian::date_duration >::unpackInt( mValue const &v )
{
    const UnpackResult<int> iopt(Traits<int>::unpackInt(v));
    if (!iopt) {
        return iopt.err();
    }
    return boost::gregorian::date_duration(*iopt);
}

UnpackResult< boost::gregorian::date_duration > Traits< boost::gregorian::date_duration >::unpackExt( mValue const &v )
{
    return unpackInt(v);
}

//
// boost::posix_time::time_duration
//

mValue Traits<boost::posix_time::time_duration>::packInt(const boost::posix_time::time_duration& td)
{
    ASSERT(!td.is_not_a_date_time());
    return mValue( Dates::timeDurationToStr( td ) );
}

UnpackResult<boost::posix_time::time_duration> Traits<boost::posix_time::time_duration>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(boost::posix_time::time_duration, v, json_spirit::str_type);
    const std::string timeVal = v.get_str();
    if (timeVal.find(':') == std::string::npos) {
        return UnpackError{"separator not found"};
    }
    const boost::posix_time::time_duration td(Dates::duration_from_string(v.get_str()));
    if (td.is_not_a_date_time()) {
        return UnpackError{"not a time duration"};
    }
    return td;
}

mValue Traits<boost::posix_time::time_duration>::packExt(const boost::posix_time::time_duration& td)
{
    return packInt(td);
}

UnpackResult<boost::posix_time::time_duration> Traits<boost::posix_time::time_duration>::unpackExt(const mValue& v)
{
    return unpackInt(v);
}

mValue Traits<boost::posix_time::ptime>::packInt(const boost::posix_time::ptime& t)
{
    ASSERT(!t.is_not_a_date_time());
    return mValue(Dates::to_iso_string(t));
}

UnpackResult<boost::posix_time::ptime> Traits<boost::posix_time::ptime>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(boost::posix_time::ptime, v, json_spirit::str_type);
    const boost::posix_time::ptime t(Dates::time_from_iso_string(v.get_str()));
    if (t.is_special()) {
        return UnpackError{"not a datetime"};
    }
    return t;
}

mValue Traits<boost::posix_time::ptime>::packExt(const boost::posix_time::ptime& t)
{
    return packInt(t);
}

UnpackResult<boost::posix_time::ptime> Traits<boost::posix_time::ptime>::unpackExt(const mValue& v)
{
    return unpackInt(v);
}


JSON_RIP_PACK(ct::UserId);
//JSON_RIP_PACK(bgnd::BgndReqId);
JSON_RIP_PACK(ct::ObjectId);
JSON_RIP_PACK(ct::CommandId);

//-----------------------------------------------------------------------

mValue Traits<Language>::packExt(const Language& l)
{
    return packInt(l);
}

mValue Traits<Language>::packInt(const Language& l)
{
    return mValue(l == RUSSIAN ? "ru" : "en");
}

UnpackResult<Language> Traits<Language>::unpackExt(const mValue& v)
{
    return unpackInt(v);
}

UnpackResult<Language> Traits<Language>::unpackInt(const mValue& v)
{
    JSON_ASSERT_TYPE(Language, v, json_spirit::str_type);
    const std::string timeVal = v.get_str();
    if(timeVal == "en")
        return ENGLISH;
    if(timeVal == "ru")
        return RUSSIAN;
    return UnpackError{STDLOG, "unknown Language value " + timeVal};
}

//-----------------------------------------------------------------------

static mValue packUserLanguage(const UserLanguage& userLangauge, bool external)
{
    UserLanguagePackerHelper ph;
    return mValue (
        external ? Traits<std::string>::packExt(ph.pack(userLangauge))
                 : Traits<std::string>::packInt(ph.pack(userLangauge))
    );
}

static UnpackResult<UserLanguage> unpackUserLanguage(const mValue& v, bool external)
{
    if (v.type() == int_type) {
        auto lang = external ? Traits<Language>::unpackExt(v) : Traits<Language>::unpackInt(v);
        if (!lang) {
            return lang.err();
        }
        UserLanguagePackerHelper ph;
        return UserLanguage(ph.unpack(*lang));
    } else {
        auto lang = external ? Traits<std::string>::unpackExt(v) : Traits<std::string>::unpackInt(v);
        if (!lang) {
            return lang.err();
        }
        return UserLanguage(*lang);
    }
}

mValue Traits< UserLanguage >::packInt( UserLanguage const &content )
{
    return packUserLanguage(content, false);
}

mValue Traits< UserLanguage >::packExt( UserLanguage const &content )
{
    return packUserLanguage(content, true);
}

UnpackResult< UserLanguage > Traits< UserLanguage >::unpackExt( mValue const &v )
{
    return unpackUserLanguage(v, true);
}

UnpackResult< UserLanguage > Traits< UserLanguage >::unpackInt( mValue const &v )
{
    return unpackUserLanguage(v, false);
}

} // json_spirit

