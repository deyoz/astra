#include "json_packer.h"

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "json_packer_heavy.h"
#include "dates_io.h"
#include "cc_censor.h"
#include "text_codec.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

namespace json_spirit {

void maskCardNums(mValue& v)
{
    if (v.type() == obj_type) {
        mObject& o = v.get_obj();
        for (auto& p : o) {
            maskCardNums(p.second);
        }
    } else if (v.type() == str_type) {
        std::string s(v.get_str());
        maskString(s, true, '0',
                [](const std::string& os, const std::string& ns) {
                    LogTrace(TRACE0) << "MASK TEXT: old [" << os << " ] new [" << ns << ']'; 
                });
        v = mValue(s);
    } else if (v.type() == array_type) {
        mArray& a = v.get_array();
        for (auto& v : a) {
            maskCardNums(v);
        }
    }
}

static void convertToUTF8(mArray& ma);
static void convertToUTF8(mObject& ma);

static void convertToUTF8(mValue& mv)
{
    static HelpCpp::TextCodec codec(HelpCpp::TextCodec::Charset::CP866,
                                    HelpCpp::TextCodec::Charset::UTF8);

    switch (mv.type()) {
    case json_spirit::array_type:
        convertToUTF8(mv.get_array());
        break;
    case json_spirit::obj_type:
        convertToUTF8(mv.get_obj());
        break;
    case json_spirit::str_type:
    {
        const auto str = mv.get_str();
        if (!HelpCpp::isUTF8Encoded(str)) {
            try {
                mv = codec.encode(mv.get_str());
            } catch (const std::exception& e) {
                LogError(STDLOG) << mv.get_str() << " : " << e.what();
                throw;
            }
        }
        break;
    }
    default:
        // do nothing
        break;
    }
}

static void convertToUTF8(mArray& ma)
{
    for (auto& mv: ma) {
        convertToUTF8(mv);
    }
}

static void convertToUTF8(mObject& mo)
{
    for (auto& kv: mo) {
        convertToUTF8(kv.second);
    }
}

std::string convertObjToUTF8(const std::string& obj)
{
    mValue mv;
    if (!json_spirit::read(obj, mv)) {
        LogError(STDLOG) << __FUNCTION__ << " : invalid JSON " << obj;
        throw std::runtime_error("Invalid JSON");
    }
    convertToUTF8(mv);

    return json_spirit::write(mv, json_spirit::remove_trailing_zeros | json_spirit::raw_utf8);
}

UnpackError UnpackError::propagate(const std::string& tag) const
{
    UnpackError err(*this);
    err.path.push_front(tag);
    return err;
}

std::ostream& operator<<(std::ostream& os, const UnpackError& e)
{
    return os << '[' << e.nick << ':' << e.file << ':' << e.line
        << " at [" << LogCont("-", e.path) << "] " << e.text;
}

bool operator==( UnpackError const &lhs, UnpackError const &rhs )
{
    return lhs.text == rhs.text && lhs.path == rhs.path;
}

} // json_spirit

#ifdef XP_TESTING
#include <map>
#include "str_utils.h"
#include "int_parameters.h"
#include "rip.h"
#include "json_pack_types.h"
#include "value_or_null.h"
#include "value_or_null_io.h"
#include "checkunit.h"

namespace {

using json_spirit::UnpackResult;
using json_spirit::UnpackError;

struct SimpleTypes
{
    int intVal;
    unsigned int uintVal;
    uint8_t ubVal;
    std::string strVal;
    short shortVal;
    bool boolVal;
    boost::gregorian::date dateVal;
    boost::posix_time::time_duration duraVal;
    boost::posix_time::ptime timeVal;
    boost::posix_time::ptime microTimeVal;
};
static std::ostream& operator<<(std::ostream& s, const SimpleTypes& st)
{
    s << "intVal=" << st.intVal
        << " uintVal=" << st.uintVal
        << " ubVal=" << st.ubVal
        << " strVal=" << st.strVal
        << " shortVal=" << st.shortVal
        << " boolVal=" << st.boolVal
        << " dateVal=" << st.dateVal
        << " duraVal=" << st.duraVal
        << " timeVal=" << st.timeVal
        << " microTimeVal=" << st.microTimeVal;
    return s;
}

MakeIntParamType(CompanyType, int);
MakeIntParamType(FlightType, short);
MakeIntParamType(SuffixType, short);
MakeStrParamType(StrType1, 3);

DECL_RIP(RipInt, int);
DECL_RIP(RipBool, bool);
DECL_RIP(RipStr, std::string);

struct FlightStruct
{
    CompanyType comp;
    FlightType number;
    SuffixType suffix;
    StrType1 strId;
    FlightStruct() {}
    FlightStruct(CompanyType c, FlightType f, SuffixType s, StrType1 st = StrType1("123"))
        : comp(c), number(f), suffix(s), strId(st)
    {}
};
static std::ostream& operator<<(std::ostream& s, const FlightStruct& f)
{
    s << f.comp << "-" << f.number;
    if (f.suffix.valid()) {
        s << "/" << f.suffix;
    }
    if (f.strId.valid()) {
        s << " " << f.strId;
    }

    return s;
}
static bool areEqual(const FlightStruct& lv, const FlightStruct& rv)
{
    LogTrace(TRACE5) << lv;
    LogTrace(TRACE5) << rv;
    return lv.comp == rv.comp && lv.number == rv.number
        && ((lv.suffix.valid() && rv.suffix.valid() && lv.suffix == rv.suffix)
                || (!lv.suffix.valid() && !rv.suffix.valid()))
        && ((lv.strId.valid() && rv.strId.valid() && lv.strId == rv.strId)
                || (!lv.strId.valid() && !rv.strId.valid()));
}

struct CompoundType
{
    boost::gregorian::date d1;
    boost::gregorian::date d2;
    std::vector<CompanyType> comps;
    boost::optional<FlightStruct> flt;
    boost::optional<std::string> optStr; 
    std::vector<SimpleTypes> arr;
};
static bool areEqual(const CompoundType& lv, const CompoundType& rv)
{
    return lv.comps == rv.comps
        && ((lv.flt && rv.flt && areEqual(*lv.flt, *rv.flt))
                || (!lv.flt && !rv.flt))
        && ((lv.optStr && rv.optStr && *lv.optStr == *rv.optStr)
                || (!lv.optStr && !rv.optStr))
        && lv.d1 == rv.d1 && lv.d2 == rv.d2;
}
static std::ostream& operator<<(std::ostream& s, const CompoundType& ct)
{
    s << "comps=" << LogCont(" ", ct.comps);
    if (ct.flt) {
        s << " flt=" << *ct.flt;
    }
    s << " d1=" << ct.d1 << " d2=" << ct.d2;
    if (ct.optStr) {
        s << " optStr=" << *ct.optStr;
    }
    return s;
}

struct RecursiveStruct
{
    int f1;
    std::vector<RecursiveStruct> foos;
};
static bool operator==(const RecursiveStruct& lhs, const RecursiveStruct& rhs)
{
    return lhs.f1 == rhs.f1
        && lhs.foos == rhs.foos;
}
static std::ostream& operator<<(std::ostream& s, const RecursiveStruct& v)
{
    return s << v.f1 << ' ' << LogCont(" ", v.foos);
}

struct ValOrEmptyStruct
{
    boost::optional<ValOrNull<std::string> > fld;
    ValOrEmptyStruct() {};
    ValOrEmptyStruct(const boost::optional<ValOrNull<std::string> >& i_fld) : fld(i_fld) {};
};
bool operator==(const ValOrEmptyStruct& lhs, const ValOrEmptyStruct& rhs)
{
    return lhs.fld == rhs.fld;
}
std::ostream& operator<<(std::ostream& s, const ValOrEmptyStruct& val)
{
    if (val.fld)
        s << *val.fld;
    return s;
}

struct TwoFormats
{
    int f1, f2;
    TwoFormats(int f1_, int f2_)
        : f1(f1_), f2(f2_)
    {}
};
bool operator==(const TwoFormats& lhs, const TwoFormats& rhs)
{
    return lhs.f1 == rhs.f1 && lhs.f2 == rhs.f2;
}

struct IgnoreTag
{
    int f;
};

struct IgnoreTagList
{
    std::vector<IgnoreTag> vals;
};

struct MaskCc
{
    std::string f;
};

struct MaskCcObj
{
    std::vector<MaskCc> vals;
};

struct PackOnly
{
    int fld1;
    std::string fld2;
};

struct NotPackOnly
{
    PackOnly fld1;
    int fld2;
};

struct Foo3
{
    int f1;
    std::string f2;
    RipInt f3;
};

} // namespace

namespace json_spirit
{
JSON_DESC_TYPE_DECL(SimpleTypes);
JSON_BEGIN_DESC_TYPE(SimpleTypes)
    DESC_TYPE_FIELD("intVal", intVal)
    DESC_TYPE_FIELD("uintVal", uintVal)
    DESC_TYPE_FIELD("uint8_t", ubVal)
    DESC_TYPE_FIELD("strVal", strVal)
    DESC_TYPE_FIELD("shortVal", shortVal)
    DESC_TYPE_FIELD("boolVal", boolVal)
    DESC_TYPE_FIELD("date", dateVal)
    DESC_TYPE_FIELD("dura", duraVal)
    DESC_TYPE_FIELD("ptime", timeVal)
    DESC_TYPE_FIELD("mptime", microTimeVal)
JSON_END_DESC_TYPE(SimpleTypes)

JSON_PACK_UNPACK_DECL(CompanyType);
JSON_PARINT_PACK_INT(CompanyType);

mValue Traits<CompanyType>::packExt(const CompanyType& c)
{
    LogTrace(TRACE5) << "packExt CompanyType:" << c;
    return mValue(c.get() == 119 ? "UT" : "??");
}

UnpackResult<CompanyType> Traits<CompanyType>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(CompanyType, v, json_spirit::str_type);
    std::string s(v.get_str());
    if (s == "UT") {
        return CompanyType(119);
    } else {
        return UnpackError{"unknown CompanyType:" + v.get_str()};
    }
}

JSON_PACK_UNPACK_DECL(FlightType);
JSON_PARINT_PACK(FlightType);

JSON_PACK_UNPACK_DECL(SuffixType);
JSON_PARINT_PACK_INT(SuffixType);

mValue Traits<SuffixType>::packExt(const SuffixType& s)
{
    return mValue(s.get() == 3 ? "C" : "");
}

UnpackResult<SuffixType> Traits<SuffixType>::unpackExt(const mValue& v)
{
    JSON_ASSERT_TYPE(SuffixType, v, json_spirit::str_type);
    LogTrace(TRACE5) << "unpackExt SuffixType:" << v.get_str();
    std::string str(v.get_str());
    if (str == "C") {
        return SuffixType(3);
    } else {
        return UnpackError{"unknown SuffixType:" + v.get_str()};
    }
}

JSON_PACK_UNPACK_DECL(StrType1);
JSON_PARINT_PACK(StrType1);

JSON_PACK_UNPACK_DECL(RipInt);
JSON_RIP_PACK(RipInt);
JSON_PACK_UNPACK_DECL(RipBool);
JSON_RIP_PACK(RipBool);
JSON_PACK_UNPACK_DECL(RipStr);
JSON_RIP_PACK(RipStr);

JSON_DESC_TYPE_DECL(FlightStruct);
JSON_BEGIN_DESC_TYPE(FlightStruct)
    DESC_TYPE_FIELD("airline", comp)
    DESC_TYPE_FIELD("number", number)
    DESC_TYPE_FIELD2("suffix", suffix, SuffixType(0))
    DESC_TYPE_FIELD2("strId", strId, StrType1("123"))
JSON_END_DESC_TYPE(FlightStruct)

JSON_DESC_TYPE_DECL(CompoundType);
JSON_BEGIN_DESC_TYPE(CompoundType)
    DESC_TYPE_FIELD("d1", d1)
    DESC_TYPE_FIELD2("d2", d2, boost::gregorian::date())
    DESC_TYPE_FIELD("airlines", comps)
    DESC_TYPE_FIELD("flight", flt)
    DESC_TYPE_FIELD("optStr", optStr)
    DESC_TYPE_FIELD("arr", arr)
JSON_END_DESC_TYPE(CompoundType)

JSON_DESC_TYPE_DECL(RecursiveStruct);
JSON_BEGIN_DESC_TYPE(RecursiveStruct)
    DESC_TYPE_FIELD("f1", f1)
    DESC_TYPE_FIELD("foos", foos)
JSON_END_DESC_TYPE(RecursiveStruct)

JSON_DESC_TYPE_DECL(ValOrEmptyStruct);
JSON_BEGIN_DESC_TYPE(ValOrEmptyStruct)
    DESC_TYPE_FIELD("fld", fld)
JSON_END_DESC_TYPE(ValOrEmptyStruct)

typedef std::pair<int, int> TwoFormatsOld;
boost::optional<TwoFormats> oldToNew(const TwoFormatsOld& p)
{
    return TwoFormats(p.first, p.second);
}

JSON_DESC_TYPE_DECL(TwoFormats);
JSON_BEGIN_DESC_TYPE2(TwoFormats, TwoFormatsOld, oldToNew)
    DESC_TYPE_FIELD("f1", f1)
    DESC_TYPE_FIELD("f2", f2)
JSON_END_DESC_TYPE(TwoFormats)

JSON_DESC_TYPE_DECL(IgnoreTag);
JSON_BEGIN_DESC_TYPE(IgnoreTag)
    DESC_TYPE_FIELD("f", f)
JSON_END_DESC_TYPE(IgnoreTag)

JSON_DESC_TYPE_DECL(IgnoreTagList);
JSON_BEGIN_DESC_TYPE(IgnoreTagList)
    DESC_TYPE_FIELD_IGNORE_BAD("vals", vals)
JSON_END_DESC_TYPE(IgnoreTagList)

JSON_DESC_TYPE_DECL(MaskCc);
JSON_BEGIN_DESC_TYPE(MaskCc)
    DESC_TYPE_FIELD("f", f)
JSON_END_DESC_TYPE(MaskCc)

JSON_DESC_TYPE_DECL(MaskCcObj);
JSON_BEGIN_DESC_TYPE(MaskCcObj)
    DESC_TYPE_FIELD("vals", vals)
JSON_END_DESC_TYPE(MaskCcObj)
// This shouldn't compile:
// can't use ..DESC_TYPE2 when old and new types are the same
/*
struct SameTypeConversionTest {
    int fld1;
};
boost::optional<SameTypeConversionTest> testConv(const SameTypeConversionTest& t) {return t;}
JSON_DESC_TYPE_DECL(SameTypeConversionTest);
NEW_JSON_BEGIN_DESC_TYPE2(SameTypeConversionTest, SameTypeConversionTest, testConv)
    NEW_DESC_TYPE_FIELD("fld1", fld1)
NEW_JSON_END_DESC_TYPE(SameTypeConversionTest)
*/

// This shouldn't compile:
// can't use ..IGNORE_BAD with a non-container field type
/*
struct NonContainerIgnoreBadTest {
    int fld1;
};

JSON_DESC_TYPE_DECL(NonContainerIgnoreBadTest);
NEW_JSON_BEGIN_DESC_TYPE(NonContainerIgnoreBadTest)
    NEW_DESC_TYPE_FIELD_IGNORE_BAD("fld1", fld1)
NEW_JSON_END_DESC_TYPE(NonContainerIgnoreBadTest)
*/

// this is in the header file
JSON_DESC_TYPE_DECL(PackOnly);
JSON_DESC_TYPE_DECL(NotPackOnly);

// and this is in the implementation file
JSON_DISABLE_OP(PackOnly, unpackInt)
JSON_DISABLE_OP(PackOnly, unpackExt)
JSON_BEGIN_DESC_TYPE(PackOnly)
    DESC_TYPE_FIELD("fld1", fld1)
    DESC_TYPE_FIELD("fld2", fld2)
JSON_END_DESC_TYPE(PackOnly)

// whoops, forgot to disable unpackInt
JSON_DISABLE_OP(NotPackOnly, unpackExt)
JSON_BEGIN_DESC_TYPE(NotPackOnly)
    DESC_TYPE_FIELD("fld1", fld1)
    DESC_TYPE_FIELD("fld2", fld2)
JSON_END_DESC_TYPE(NotPackOnly)

JSON_PACK_UNPACK_DECL(Foo3);
JSON_BEGIN_DESC_TYPE(Foo3)
    DESC_TYPE_FIELD("f1", f1)
    DESC_TYPE_FIELD("f2", f2)
    DESC_TYPE_FIELD("f3", f3)
JSON_END_DESC_TYPE(Foo3)
#if 0
template<>
auto getDesc<Foo3>()
{
    using obj_t = Foo3;
    static auto p = details::makeDesc<Foo3>(std::make_tuple(
        declTag("f1", &Foo3::f1),
        declTag("f2", &Foo3::f2),
        declTag("f3", &Foo3::f3),
        0));
    return &p;
}
mValue Traits<Foo3>::packInt(const Foo3& v)
{
    return getDesc<Foo3>()->packInt(v);
}
Expected<Foo3> Traits<Foo3>::unpackInt(const mValue& v)
{
    return getDesc<Foo3>()->unpackInt(v);
}
mValue Traits<Foo3>::packExt(const Foo3& v)
{
    return getDesc<Foo3>()->packExt(v);
}
Expected<Foo3> Traits<Foo3>::unpackExt(const mValue& v)
{
    return getDesc<Foo3>()->unpackExt(v);
}
#endif
} // json_spirit

namespace {

START_TEST(check_json_spirit)
{
    using namespace json_spirit;
    int flags[6] = {
        0,
        pretty_print,
        raw_utf8,
        remove_trailing_zeros,
        pretty_print|raw_utf8,
        pretty_print|raw_utf8|remove_trailing_zeros};
#define CHECK_JSON_READ_WRITE(val, getter) { \
    for (int i = 0; i < 6; ++i) { \
        mObject o; \
        o["v"] = mValue(val); \
        std::string jsonStr(json_spirit::write(o, flags[i])); \
        LogTrace(TRACE5) << "jsonStr: [" << jsonStr << "]"; \
        mValue v; \
        fail_unless(json_spirit::read(jsonStr, v) == true, \
                "failed to read: [%s]", jsonStr.c_str()); \
        fail_unless(v.get_obj()["v"].getter() == val); \
    } \
}
    const std::string str1("simple string"),
        str2("not so simple}"),
        str3("more\\ complicated\""),
        str4("\\}even more complicated\\{"),
        str5("\\\\} brackets\\\\{");

    CHECK_JSON_READ_WRITE(str1, get_str);
    CHECK_JSON_READ_WRITE(str2, get_str);
    CHECK_JSON_READ_WRITE(str3, get_str);
    CHECK_JSON_READ_WRITE(str4, get_str);
    CHECK_JSON_READ_WRITE(str5, get_str);
} END_TEST

START_TEST(string_with_zeroes)
{
    using namespace json_spirit;
    char buffWithZeroes[] = {'a', 'b', '\0', 'c'};
    std::string strWithZeroes(buffWithZeroes, 4);
    try {
        std::string s = json_spirit::write(mValue(strWithZeroes), raw_utf8);
        fail_if(1, "we must throw when trying to write zero char in raw mode");
    } catch (const std::runtime_error& e) {
        LogTrace(TRACE5) << e.what();
    }
    std::string jsonStr = json_spirit::write(mValue(strWithZeroes));
    CHECK_EQUAL_STRINGS(jsonStr, "\"ab\\u0000c\"");

    mValue v;
    fail_unless(json_spirit::read(jsonStr, v) == true, "failed to read: [%s]", jsonStr.c_str());
    CHECK_EQUAL_STRINGS(v.get_str(), strWithZeroes);
} END_TEST

START_TEST(primitives)
{
#define CHECK_PACK_UNPACK_EXACT(Type, Value, PackedInt, PackedExt) { \
    const Type v1(Value); \
    json_spirit::mValue val1(json_spirit::Traits< Type >::packInt(v1)); \
    std::string s1 = json_spirit::write(val1); \
    LogTrace(TRACE5) << #Type << " packInt [" << s1 << "]"; \
    fail_unless(s1 == PackedInt, "packInt: %s != %s", s1.c_str(), PackedInt); \
    const Type v2 = *json_spirit::Traits< Type >::unpackInt(val1); \
    LogTrace(TRACE5) << "v1=" << v1 << " v2=" << v2; \
    fail_unless(v1 == v2, "internal pack/unpack failed for " #Type); \
    fail_unless((bool)json_spirit::Traits< Type >::unpackInt(json_spirit::mValue()) == false); \
    \
    json_spirit::mValue val2(json_spirit::Traits< Type >::packExt(v1)); \
    std::string s2 = json_spirit::write(val2); \
    LogTrace(TRACE5) << #Type << " packExt [" << s2 << "]"; \
    fail_unless(s2 == PackedExt, "packInt: %s != %s", s2.c_str(), PackedExt); \
    const Type v3 = *json_spirit::Traits< Type >::unpackExt(val2); \
    LogTrace(TRACE5) << "v1=" << v1 << " v3=" << v3; \
    fail_unless(v1 == v3, "external pack/unpack failed for " #Type " value: " #Value); \
    fail_unless((bool)json_spirit::Traits< Type >::unpackExt(json_spirit::mValue()) == false); \
}
    CHECK_PACK_UNPACK_EXACT(int, 1, "1", "1");
    CHECK_PACK_UNPACK_EXACT(std::string, "lolka", "\"lolka\"", "\"lolka\"");
    CHECK_PACK_UNPACK_EXACT(unsigned int, 987, "987", "987");

} END_TEST

START_TEST(parints)
{
    CHECK_PACK_UNPACK_EXACT(CompanyType, 119, "119", "\"UT\"");
    CHECK_PACK_UNPACK_EXACT(FlightType, 10, "10", "10");
    CHECK_PACK_UNPACK_EXACT(SuffixType, 3, "3", "\"C\"");
    CHECK_PACK_UNPACK_EXACT(StrType1, "VAL", "\"VAL\"", "\"VAL\"");
} END_TEST

START_TEST(rips)
{
    CHECK_PACK_UNPACK_EXACT(RipInt, 3, "3", "3");
    CHECK_PACK_UNPACK_EXACT(RipBool, true, "true", "true");
    CHECK_PACK_UNPACK_EXACT(RipStr, "VAL", "\"VAL\"", "\"VAL\"");
} END_TEST

START_TEST(val_or_empty)
{
    json_spirit::mValue v;

#define CHECK_VAL_OR_EMPTY(inStr, cond, outStr) { \
    fail_unless(json_spirit::read(inStr, v)); \
    UnpackResult<ValOrEmptyStruct> s = json_spirit::Traits<ValOrEmptyStruct>::unpackInt(v); \
    if (static_cast<bool>(s) != true) { \
        LogTrace(TRACE5) << "unpack failed: " << s.err(); \
        fail_if(true, "unpack failed [%s]", inStr); \
    } \
    fail_unless(cond, "%s failed", #cond); \
    std::string str = json_spirit::write(json_spirit::Traits<ValOrEmptyStruct>::packInt(*s)); \
    fail_unless(str == outStr, "[%s] != [%s]", str.c_str(), outStr); \
}
    CHECK_VAL_OR_EMPTY("{\"fld\":\"text\"}", s->fld->val.get() == "text", "{\"fld\":\"text\"}");
    CHECK_VAL_OR_EMPTY("{\"fld\":\"-\"}", static_cast<bool>(s->fld->val) == false, "{\"fld\":\"-\"}");
    CHECK_VAL_OR_EMPTY("{\"another_field\":\"text\"}", static_cast<bool>(s->fld) == false, "{}");

    ValOrEmptyStruct undef;
    ValOrEmptyStruct empty((ValOrNull<std::string>()));
    ValOrEmptyStruct value(ValOrNull<std::string>("lolka"));

    CHECK_PACK_UNPACK_EXACT(ValOrEmptyStruct, undef, "{}", "{}");
    CHECK_PACK_UNPACK_EXACT(ValOrEmptyStruct, empty, "{\"fld\":\"-\"}", "{\"fld\":\"-\"}")
    CHECK_PACK_UNPACK_EXACT(ValOrEmptyStruct, value, "{\"fld\":\"lolka\"}", "{\"fld\":\"lolka\"}")

#undef CHECK_PACK_UNPACK_EXACT
} END_TEST

START_TEST(containers)
{
    {
        std::vector<int> ints; ints.push_back(1); ints.push_back(2); ints.push_back(3);
        json_spirit::mValue val(json_spirit::Traits< std::vector<int> >::packInt(ints));
        std::string s = json_spirit::write(val);
        LogTrace(TRACE5) << s;
        fail_unless(s == "[1,2,3]");
        fail_unless(json_spirit::read("[3,2,1]", val));
        std::vector<int> unpacked(*json_spirit::Traits< std::vector<int> >::unpackInt(val));
        fail_unless(unpacked != ints);
        ints.clear(); ints.push_back(3); ints.push_back(2); ints.push_back(1);
        fail_unless(unpacked == ints);
    }

    {
        std::set<int> ints; ints.insert(1); ints.insert(2); ints.insert(3);
        json_spirit::mValue val(json_spirit::Traits< std::set<int> >::packInt(ints));
        std::string s = json_spirit::write(val);
        LogTrace(TRACE5) << s;
        fail_unless(s == "[1,2,3]");
        fail_unless(json_spirit::read("[3,2,1]", val));
        std::set<int> unpacked(*json_spirit::Traits< std::set<int> >::unpackInt(val));
        fail_unless(unpacked == ints);
        fail_unless(json_spirit::read("[3,2,2,1,1]", val));
        UnpackResult< std::set<int> > wrong(json_spirit::Traits< std::set<int> >::unpackInt(val));
        fail_unless((bool)wrong == false, "set with duplicate values must return an error");
    }

    {
        std::list<int> ints; ints.push_back(1); ints.push_back(2); ints.push_back(3);
        json_spirit::mValue val(json_spirit::Traits< std::list<int> >::packInt(ints));
        std::string s = json_spirit::write(val);
        LogTrace(TRACE5) << s;
        fail_unless(s == "[1,2,3]");
        fail_unless(json_spirit::read("[3,2,1]", val));
        std::list<int> unpacked(*json_spirit::Traits< std::list<int> >::unpackInt(val));
        fail_unless(unpacked != ints);
        ints.clear(); ints.push_back(3); ints.push_back(2); ints.push_back(1);
        fail_unless(unpacked == ints);
    }

    {
        typedef std::map<int, int> map_t;
        map_t ints; ints[1] = 10; ints[2] = 20; ints[3] = 30;
        json_spirit::mValue val(json_spirit::Traits< map_t >::packInt(ints));
        std::string s = json_spirit::write(val);
        LogTrace(TRACE5) << s;
        fail_unless(s == "[[1,10],[2,20],[3,30]]");
        fail_unless(json_spirit::read("[3,2,1]", val));
        UnpackResult<map_t> res = json_spirit::Traits< map_t >::unpackInt(val);
        fail_unless((bool)res == false);
        fail_unless(json_spirit::read("[[3,30],[2,20],[1,10]]", val));
        map_t unpacked(*json_spirit::Traits< map_t >::unpackInt(val));
        fail_unless(unpacked == ints);
        fail_unless(json_spirit::read("[[3,30],[2,20],[2,23],[1,10]]", val));
        UnpackResult< map_t > wrong(json_spirit::Traits< map_t >::unpackInt(val));
        fail_unless((bool)wrong == false, "map with duplicate values must return an error");
    }

    {
        boost::optional<std::string> optStr;
        fail_unless("null" == json_spirit::write( json_spirit::Traits< boost::optional<std::string> >::packInt( optStr ) ) );
        optStr = std::string("Hi");
        fail_unless("\"Hi\"" == json_spirit::write( json_spirit::Traits< boost::optional<std::string> >::packInt( optStr ) ) );
    }

} END_TEST

START_TEST(simple_types)
{
    SimpleTypes st{-10, 543, 7, "I am String", 33, true,
        boost::gregorian::day_clock::local_day(),
        boost::posix_time::second_clock::local_time().time_of_day(),
        boost::posix_time::second_clock::local_time(),
        boost::posix_time::microsec_clock::local_time()
    };
    json_spirit::mValue val1(json_spirit::Traits<SimpleTypes>::packInt(st));
    std::string str = json_spirit::write(val1);
    LogTrace(TRACE5) << str;
    UnpackResult<SimpleTypes> ost(json_spirit::Traits<SimpleTypes>::unpackInt(val1));
    fail_unless((bool)ost == true, "unpackInt failed");
    LogTrace(TRACE5) << st;
    LogTrace(TRACE5) << *ost;
    fail_unless(st.intVal == ost->intVal);
    fail_unless(st.uintVal == ost->uintVal);
    fail_unless(st.ubVal == ost->ubVal);
    fail_unless(st.strVal == ost->strVal);
    fail_unless(st.shortVal == ost->shortVal);
    fail_unless(st.boolVal == ost->boolVal);
    fail_unless(st.dateVal == ost->dateVal);
    fail_unless(st.duraVal.hours() == ost->duraVal.hours());
    fail_unless(st.duraVal.minutes() == ost->duraVal.minutes());
    fail_unless(st.timeVal == ost->timeVal);
    fail_unless(st.microTimeVal == ost->microTimeVal);

    json_spirit::mValue val2(json_spirit::Traits<SimpleTypes>::packExt(st));
    str = json_spirit::write(val2);
    LogTrace(TRACE5) << str;
    ost = json_spirit::Traits<SimpleTypes>::unpackExt(val2);
    if (!ost) {
        fail_if(true, "unpackExt failed: %s", ost.err().text.c_str());
    }
    LogTrace(TRACE5) << st;
    LogTrace(TRACE5) << *ost;
    fail_unless(st.intVal == ost->intVal);
    fail_unless(st.uintVal == ost->uintVal);
    fail_unless(st.ubVal == ost->ubVal);
    fail_unless(st.strVal == ost->strVal);
    fail_unless(st.shortVal == ost->shortVal);
    fail_unless(st.boolVal == ost->boolVal);
    fail_unless(st.dateVal == ost->dateVal);
    fail_unless(st.duraVal.hours() == ost->duraVal.hours());
    fail_unless(st.duraVal.minutes() == ost->duraVal.minutes());
    fail_unless(st.timeVal == ost->timeVal);
    fail_unless(st.microTimeVal == ost->microTimeVal);
} END_TEST

START_TEST(simple_json_packer)
{
    const std::string flightJsonIntStr("{\"airline\":119,\"number\":100,\"suffix\":3}");
    json_spirit::mValue v;
    const FlightStruct defFlt(CompanyType(119), FlightType(100), SuffixType(3));

    //check internal
    fail_unless(json_spirit::read(flightJsonIntStr, v) == true);
    UnpackResult<FlightStruct> flt(json_spirit::Traits<FlightStruct>::unpackInt(v));
    fail_unless((bool)flt == true, "unpack failed");
    LogTrace(TRACE5) << "unpacked: " << *flt;
    fail_unless(areEqual(*flt, defFlt));
    std::string fltStr(json_spirit::write(json_spirit::Traits<FlightStruct>::packInt(*flt)));
    fail_unless(fltStr == flightJsonIntStr, "[%s] != [%s]", fltStr.c_str(), flightJsonIntStr.c_str());

    //check external
    const std::string flightJsonExtStr("{\"airline\":\"UT\",\"number\":100,\"suffix\":\"C\"}");
    fail_unless(json_spirit::read(flightJsonExtStr, v) == true);
    flt = json_spirit::Traits<FlightStruct>::unpackExt(v);
    fail_unless((bool)flt == true, "unpack failed");
    LogTrace(TRACE5) << "unpacked: " << *flt;
    fail_unless(areEqual(*flt, defFlt));
    fltStr = json_spirit::write(json_spirit::Traits<FlightStruct>::packExt(*flt));
    fail_unless(fltStr == flightJsonExtStr, "[%s] != [%s]", fltStr.c_str(), flightJsonExtStr.c_str());
} END_TEST

START_TEST(default_json_packer)
{
    const std::string flightJsonIntStr("{\"airline\":119,\"number\":100}");
    json_spirit::mValue v;
    fail_unless(json_spirit::read(flightJsonIntStr, v) == true);

    const FlightStruct defFlt(CompanyType(119), FlightType(100), SuffixType(0));
    LogTrace(TRACE5) << "unpack: " << flightJsonIntStr;
    UnpackResult<FlightStruct> flt(json_spirit::Traits<FlightStruct>::unpackInt(v));
    fail_unless((bool)flt == true, "unpack failed");
    LogTrace(TRACE5) << *flt;
    LogTrace(TRACE5) << defFlt;
    fail_unless(areEqual(*flt, defFlt));
    const std::string fltStr(json_spirit::write(json_spirit::Traits<FlightStruct>::packInt(*flt)));
    LogTrace(TRACE5) << fltStr;
    LogTrace(TRACE5) << flightJsonIntStr;
    fail_unless(fltStr == flightJsonIntStr, "[%s] != [%s]", fltStr.c_str(), flightJsonIntStr.c_str());
} END_TEST

START_TEST(compound_json_packer)
{
    const std::string jsonIntStr("{\"d1\":\"20120520\",\"flight\":{\"airline\":119,\"number\":200}}");
    json_spirit::mValue v;
    fail_unless(json_spirit::read(jsonIntStr, v) == true);
    CompoundType ct1{boost::gregorian::date(2012, 5, 20),
        boost::gregorian::date(),
        {},
        FlightStruct(CompanyType(119), FlightType(200), SuffixType(0))};
    LogTrace(TRACE5) << "unpack: " << jsonIntStr;

    UnpackResult<CompoundType> ct2(json_spirit::Traits<CompoundType>::unpackInt(v));
    if (!ct2) {
        fail_if(true, "unpack failed: %s", ct2.err().text.c_str());
    }
    LogTrace(TRACE5) << ct1;
    LogTrace(TRACE5) << *ct2;
    fail_unless(areEqual(ct1, *ct2));
    std::string jsonStr(json_spirit::write(json_spirit::Traits<CompoundType>::packInt( ct1 ) ) );
    fail_unless(jsonStr == jsonIntStr, "[%s] != [%s]", jsonStr.c_str(), jsonIntStr.c_str());

    const std::string jsonExtStrNoFlt("{\"airlines\":[\"UT\",\"UT\"],\"d1\":\"20120520\",\"optStr\":\"A\"}");
    fail_unless(json_spirit::read(jsonExtStrNoFlt, v) == true);
    ct2 = UnpackError{"stub"};
    ct1.d1 = boost::gregorian::date(2012, 5, 20);
    ct1.comps.push_back(CompanyType(119));
    ct1.comps.push_back(CompanyType(119));
    ct1.flt = boost::optional<FlightStruct>();
    ct1.optStr = std::string("A");
    LogTrace(TRACE5) << "unpack: " << jsonExtStrNoFlt;

    ct2 = json_spirit::Traits<CompoundType>::unpackExt(v);
    if (!ct2) {
        fail_if(true, "unpack failed: %s", ct2.err().text.c_str());
    }
    LogTrace(TRACE5) << ct1;
    LogTrace(TRACE5) << *ct2;
    fail_unless(areEqual(ct1, *ct2));
    jsonStr = json_spirit::write(json_spirit::Traits<CompoundType>::packExt( ct1 ) );
    fail_unless(jsonStr == jsonExtStrNoFlt, "[%s] != [%s]", jsonStr.c_str(), jsonExtStrNoFlt.c_str());
} END_TEST

START_TEST(pair_json_packer)
{
    json_spirit::mValue v;
    fail_unless( json_spirit::read( "[ 10, 20 ]", v ) );
    {
        typedef std::pair<int, std::string> wrong_pair_t;
        fail_unless( (bool)json_spirit::Traits< wrong_pair_t >::unpackInt( v ) == false );
        UnpackResult< std::pair< CompanyType, FlightType > > unpacked(json_spirit::Traits< std::pair<CompanyType, FlightType> >::unpackInt( v ) );
        fail_unless( *unpacked == std::make_pair( CompanyType( 10 ), FlightType( 20 ) ) );
        fail_unless( json_spirit::write( json_spirit::Traits< std::pair<CompanyType, FlightType> >::packInt( *unpacked ) ) == "[10,20]" );
    }
    fail_unless( json_spirit::read( "[ \"ururu\", 777 ]", v ) );
    {
        typedef std::pair<int, std::string> wrong_pair_t;
        fail_unless( (bool)json_spirit::Traits< wrong_pair_t >::unpackInt( v ) == false );
        UnpackResult< std::pair<std::string, int> > unpacked(json_spirit::Traits< std::pair<std::string, int> >::unpackInt( v ));
        fail_unless( *unpacked == std::make_pair( std::string( "ururu" ), 777 ) );
        fail_unless( json_spirit::write( json_spirit::Traits< std::pair<std::string, int> >::packInt( *unpacked ) ) == "[\"ururu\",777]" );
    }

    typedef std::pair<int, int> pair_t;
    fail_unless( json_spirit::read( "[]", v ) );
    fail_unless( (bool)json_spirit::Traits<pair_t>::unpackInt( v ) == false);
    fail_unless( json_spirit::read( "[ 1 ]", v ) );
    fail_unless( (bool)json_spirit::Traits<pair_t>::unpackInt( v ) == false);
    fail_unless( json_spirit::read( "[ 1, 2, 3 ]", v ) );
    fail_unless( (bool)json_spirit::Traits<pair_t>::unpackInt( v ) == false);
    fail_unless( json_spirit::read( "[ 1, 2 ]", v ) );
    UnpackResult<pair_t> w = json_spirit::Traits<pair_t>::unpackInt( v );
    fail_unless( (bool)w == true );
    fail_unless( *w == std::make_pair( 1, 2 ) );
    fail_unless( json_spirit::write( json_spirit::Traits<pair_t>::packInt( *w ) ) == "[1,2]" );
} END_TEST

START_TEST( pair_opt_vector )
{
    // порядок вложенности типов обратный определению соотв. шаблонов pack / unpack в json_packer.h 
    typedef std::pair< std::string, CompanyType > pair_t;
    typedef boost::optional< pair_t > opt_t;
    typedef std::vector< opt_t > vec_t;

    vec_t sample;
    sample.push_back( opt_t() );
    sample.push_back( pair_t( "ururu", CompanyType( 10 ) ) );
    sample.push_back( opt_t() );
    sample.push_back( pair_t( "ololo", CompanyType( 20 ) ) );
    fail_unless( "[null,[\"ururu\",10],null,[\"ololo\",20]]" == json_spirit::write( json_spirit::Traits<vec_t>::packInt( sample ) ) );

    json_spirit::mValue val;
    json_spirit::read( "[ null, [ \"ururu\", 10 ], null, [ \"ololo\", 20 ] ]", val );
    UnpackResult<vec_t> v(json_spirit::Traits<vec_t>::unpackInt( val ));
    fail_unless( (bool)v == true );
    fail_unless( *v == sample );
} END_TEST

START_TEST(unpack_error_message)
{
    typedef std::pair< std::string, int > pair_t;
    typedef boost::optional< pair_t > opt_t;
    typedef std::vector< opt_t > vec_t;
    json_spirit::mValue val;
    json_spirit::read( "[ null, [ \"ururu\", 10 ], null, [ \"ololo\", \"20\" ] ]", val );
    UnpackResult<vec_t> v(json_spirit::Traits<vec_t>::unpackInt( val ));
    fail_unless( (bool)v == false );
    CHECK_EQUAL_STRINGS(v.err().text, "int got invalid type(2) expected json_spirit::int_type(4)");
    CHECK_EQUAL_STRINGS(v.err().path[0], "[3]");
    CHECK_EQUAL_STRINGS(v.err().path[1], "second");
} END_TEST

START_TEST(date_duration)
{
    boost::gregorian::date_duration const dd( 20 );
    std::string const istr( json_spirit::write( json_spirit::Traits< boost::gregorian::date_duration >::packInt( dd ) ) );
    json_spirit::mValue val;
    json_spirit::read( istr, val );
    boost::gregorian::date_duration const d1( *json_spirit::Traits< boost::gregorian::date_duration >::unpackInt( val ) );
    fail_unless( dd == d1 );
    std::string const estr( json_spirit::write( json_spirit::Traits< boost::gregorian::date_duration >::packExt( dd ) ) );
    json_spirit::read( estr, val );
    boost::gregorian::date_duration const d2( *json_spirit::Traits< boost::gregorian::date_duration >::unpackExt( val ) );
    fail_unless( dd == d2 );
}
END_TEST

void tstTimeDuration( int hr, int mi, int sc, int fr )
{
    boost::posix_time::time_duration const td( hr, mi, sc, fr );
    boost::posix_time::time_duration const ntd( hr, mi, 0 );
    std::string const estr( json_spirit::write( json_spirit::Traits< boost::posix_time::time_duration >::packExt( td ) ) );
    json_spirit::mValue val;
    json_spirit::read( estr, val );
    boost::posix_time::time_duration const etd( *json_spirit::Traits< boost::posix_time::time_duration >::unpackExt( val ) );
    fail_unless( ntd == etd );
    std::string const istr( json_spirit::write( json_spirit::Traits< boost::posix_time::time_duration >::packInt( td ) ) );
    json_spirit::read( istr, val );
    boost::posix_time::time_duration const itd( *json_spirit::Traits< boost::posix_time::time_duration >::unpackInt( val ) );
    fail_unless( ntd == itd );
}

void tstTimeDurationFormat()
{
    json_spirit::mValue val1("12:00");
    json_spirit::mValue val2("1200");
    boost::posix_time::time_duration const dur1( *json_spirit::Traits< boost::posix_time::time_duration >::unpackExt( val1 ) );
    UnpackResult<boost::posix_time::time_duration> const dur2( json_spirit::Traits< boost::posix_time::time_duration >::unpackExt( val2 ) );
    fail_unless( !dur1.is_not_a_date_time() );
    fail_unless( !dur2 );
}

START_TEST( time_duration )
{
    tstTimeDuration( -100, 10, 20, 0 );
    tstTimeDuration( 100, 10, 20, 0 );
    tstTimeDuration( -100, 10, 20, 33 );
    tstTimeDuration( 100, 10, 20, 33 );
    tstTimeDuration( 12, 0, 0, 0 );
    tstTimeDurationFormat();
}
END_TEST

void tstBadTime()
{
    boost::posix_time::ptime const not_time;
    try {
        std::string const notTimeStrExt( json_spirit::write( json_spirit::Traits< boost::posix_time::ptime >::packExt( not_time ) ) );
        fail_unless( false );
    } catch(ServerFramework::Exception& e) {
    }

    try {
        std::string const notTimeStrInt( json_spirit::write( json_spirit::Traits< boost::posix_time::ptime >::packInt( not_time ) ) );
        fail_unless( false );
    } catch(ServerFramework::Exception& e) {
    }
}

void tstBadDateTime()
{
    boost::gregorian::date not_time;
    try {
        std::string const notTimeStrExt( json_spirit::write( json_spirit::Traits< boost::gregorian::date >::packExt( not_time ) ) );
        fail_unless( false );
    } catch(ServerFramework::Exception& e) {
    }
    try {
        std::string const notTimeStrInt( json_spirit::write( json_spirit::Traits< boost::gregorian::date >::packInt( not_time ) ) );
        fail_unless( false );
    } catch(ServerFramework::Exception& e) {
    }
}

START_TEST( bad_time )
{
    tstBadTime();
    tstBadDateTime();
}
END_TEST


START_TEST(recursive_types)
{
    RecursiveStruct foo3{3};
    foo3.foos.push_back(RecursiveStruct{4});
    RecursiveStruct foo2{2};
    foo2.foos.push_back(foo3);
    RecursiveStruct foo1{1};
    foo1.foos.push_back(foo2);
    foo1.foos.push_back(RecursiveStruct{22});
    foo1.foos.push_back(RecursiveStruct{23});

    json_spirit::mValue v = json_spirit::Traits<RecursiveStruct>::packInt(foo1);
    std::string str = json_spirit::write(v);
    LogTrace(TRACE5) << str;
    UnpackResult<RecursiveStruct> resultFoo(json_spirit::Traits<RecursiveStruct>::unpackInt(v));
    if (!resultFoo) {
        LogTrace(TRACE5) << resultFoo.err();
        fail_if(true, "unpackInt failed: %s", str.c_str());
    }
    LogTrace(TRACE5) << *resultFoo;
    fail_unless(*resultFoo == foo1, "unpack failed");
} END_TEST

START_TEST(unicode_pack)
{
    const std::string ustring("\"\\u0415\\u0432\\u0433\\u0435\\u043d\\u0438\\u0439\"");
    json_spirit::mValue v;
    fail_unless(json_spirit::read(ustring, v));
    const std::string result(json_spirit::write(v, json_spirit::raw_utf8));
    fail_unless(result == EncString::from866("\"Евгений\"").toUtf(), "read/write failed");
} END_TEST

START_TEST(two_formats)
{
    const std::string str1 = "{\"f1\": 2, \"f2\": 4}";
    const std::string str2 = "[2, 4]";
    json_spirit::mValue v;
    fail_unless(json_spirit::read(str1, v));
    UnpackResult<TwoFormats> res1(json_spirit::Traits<TwoFormats>::unpackInt(v));
    fail_unless(static_cast<bool>(res1), "str1 unpack failed");
    fail_unless(json_spirit::read(str2, v));
    UnpackResult<TwoFormats> res2(json_spirit::Traits<TwoFormats>::unpackInt(v));
    fail_unless(static_cast<bool>(res2), "str2 unpack failed");
    fail_unless(*res1 == *res2);

    fail_unless(json_spirit::read(str1, v));
    res1 = json_spirit::Traits<TwoFormats>::unpackExt(v);
    fail_unless(static_cast<bool>(res1), "str1 unpack failed");
    fail_unless(json_spirit::read(str2, v));
    res2 = json_spirit::Traits<TwoFormats>::unpackExt(v);
    fail_unless(static_cast<bool>(res2), "str2 unpack failed");
    fail_unless(*res1 == *res2);
} END_TEST

START_TEST(ignore_tags)
{
    const std::string s = "{\"vals\":[{\"f\":321}, \"321\", {\"f\":\"321\"}]}";
    json_spirit::mValue v;
    fail_unless(json_spirit::read(s, v));
    UnpackResult<IgnoreTagList> res(json_spirit::Traits<IgnoreTagList>::unpackInt(v));
    if (!res) {
        fail_if(true, "s unpack failed: %s", res.err().text.c_str());
    }
    fail_unless(res->vals.size() == 1, "invalid vals size %zd != 1", res->vals.size());
    fail_unless(res->vals[0].f == 321);
} END_TEST

START_TEST(mask_cc)
{
    const std::string s = "{\"vals\":[{\"f\":\"BA4024007178915186\"}, {\"f\":\"wow\"}, {\"f\":\"wow BA4024007178915186 wow\"}]}";
    json_spirit::mValue v;
    fail_unless(json_spirit::read(s, v));
    maskCardNums(v);
    UnpackResult<MaskCcObj> res(json_spirit::Traits<MaskCcObj>::unpackInt(v));
    fail_unless(static_cast<bool>(res), "s unpack failed");
    fail_unless(res->vals.size() == 3, "invalid vals size %zd != 1", res->vals.size());
    CHECK_EQUAL_STRINGS("BA0024000000005186", res->vals[0].f);
    CHECK_EQUAL_STRINGS("wow", res->vals[1].f);
    CHECK_EQUAL_STRINGS("wow BA0024000000005186 wow", res->vals[2].f);
} END_TEST

START_TEST(unpacker)
{
    json_spirit::mObject obj;
    obj["f1"] = 123;
    obj["f2"] = "STR2";
    obj["f3"] = 2;
    json_spirit::mValue values(obj);
    UnpackResult<Foo3> f3 = json_spirit::Traits<Foo3>::unpackExt(values);
    if (!f3) {
        LogTrace(TRACE5) << f3.err();
    }
    fail_unless(f3->f1 == 123, "bad f1=%d\n", f3->f1);
    fail_unless(f3->f2 == "STR2", "bad f2=%s\n", f3->f2.c_str());
    fail_unless(f3->f3.get() == 2, "bad f3=%d\n", f3->f3.get());

    json_spirit::mValue newValues = json_spirit::Traits<Foo3>::packExt(*f3);
    const json_spirit::mObject& newObj(newValues.get_obj());
    fail_unless(newObj.find("f1") != newObj.end());
    fail_unless(newObj.find("f1")->second.get_int() == 123);
    fail_unless(newObj.find("f2") != newObj.end());
    fail_unless(newObj.find("f2")->second.get_str() == "STR2");
    fail_unless(newObj.find("f3") != newObj.end());
    fail_unless(newObj.find("f3")->second.get_int() == 2);
} END_TEST

START_TEST(pack_only)
{
    PackOnly po{5, "Hello"};
    auto mv = json_spirit::Traits<PackOnly>::packInt(po);
    LogTrace(TRACE5) << json_spirit::write(mv);

    try {
        json_spirit::Traits<PackOnly>::unpackInt(mv);
        fail_unless(false, "ASSERT failure expected");
    } catch (const comtech::AssertionFailed& e) {
        LogTrace(TRACE5) << e.what();
        // do nothing
    }

    NotPackOnly npo{po, 10};
    mv = json_spirit::Traits<NotPackOnly>::packInt(npo);
    LogTrace(TRACE5) << json_spirit::write(mv);

    // but this will still fail
    try {
        json_spirit::Traits<NotPackOnly>::unpackInt(mv);
        fail_unless(false, "ASSERT failure expected");
    } catch (const comtech::AssertionFailed& e) {
        LogTrace(TRACE5) << e.what();
        // do nothing
    }
} END_TEST

START_TEST(convert_to_utf8)
{
    struct {
        int id;
        std::string from;
        std::string to;
    } testcases[] = {
        // empty
        {1, "{}", "{}"},
        // ASCII only
        {2, R"({"key":"value"})",
            R"({"key":"value"})"},
        // UTF text only
        {3, "{\"key\":\"\u042e\u0442\"}",
            "{\"key\":\"\u042e\u0442\"}"},
        // combined: no double encoding
        {4, "{\"key\":[\"\u042e\u0442\",\"\x9E\xE2\"]}",
            "{\"key\":[\"\u042e\u0442\",\"\u042e\u0442\"]}"}
    };

    for (const auto& tc: testcases) {
        const auto res = json_spirit::convertObjToUTF8(tc.from);
        if (res != tc.to) {
            LogTrace(TRACE5) << "testcase " << tc.id;
            LogTrace(TRACE5) << "expected: " << tc.to;
            LogTrace(TRACE5) << "got: " << res;
            fail_unless(false);
        }
    }

    try {
        json_spirit::convertObjToUTF8("this is invalid json");
        fail_unless(false, "exception expected");
    } catch (std::exception& e) {
        LogTrace(TRACE5) << e.what();
    }
} END_TEST

#define SUITENAME "JsonPacker"

TCASEREGISTER(0, 0)
    ADD_TEST(check_json_spirit)
    ADD_TEST(primitives)
    ADD_TEST(parints)
    ADD_TEST(rips)
    ADD_TEST(val_or_empty)
    ADD_TEST(containers)
    ADD_TEST(simple_types)
    ADD_TEST(simple_json_packer)
    ADD_TEST(default_json_packer)
    ADD_TEST(compound_json_packer)
    ADD_TEST(pair_json_packer)
    ADD_TEST(pair_opt_vector)
    ADD_TEST(unpack_error_message)
    ADD_TEST(date_duration)
    ADD_TEST(time_duration)
    ADD_TEST(recursive_types)
    ADD_TEST(unicode_pack)
    ADD_TEST(two_formats)
    ADD_TEST(bad_time)
    ADD_TEST(string_with_zeroes)
    ADD_TEST(ignore_tags)
    ADD_TEST(mask_cc)
    ADD_TEST(unpacker)
    ADD_TEST(pack_only)
    ADD_TEST(convert_to_utf8)
TCASEFINISH

} // namespace

void init_json_tests() {}

#endif /* XP_TESTING */
