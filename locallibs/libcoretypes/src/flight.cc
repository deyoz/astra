#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <serverlib/helpcpp.h>
#include <serverlib/str_utils.h>
#include <serverlib/string_cast.h>
#include <serverlib/dates_io.h>
#include <serverlib/a_ya_a_z_0_9.h>
#include <serverlib/enum.h>

#include "flight.h"

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

namespace ct
{

Flight::Flight(nsi::CompanyId a_, FlightNum f_) : airline(a_), number(f_), suffix(Suffix(0))
{
}

Flight::Flight(nsi::CompanyId a_, FlightNum f_, Suffix s_)
    : airline(a_), number(f_), suffix(s_)
{
}


std::string Flight::toString(Language lang) const
{
    std::string s(nsi::Company(airline).code(lang).toDb());
    s += '-' + HelpCpp::string_cast(number.get());
    if (suffix.get() != 0) {
        s += suffixToString(suffix, lang);
    }
    return s;
}

std::string Flight::toStringUtf(Language lang) const
{
    std::string s(nsi::Company(airline).code(lang).toUtf());
    s += '-' + HelpCpp::string_cast(number.get());
    if (suffix.get() != 0) {
        s += EncString::from866(suffixToString(suffix, lang)).toUtf();
    }
    return s;
}

std::string Flight::toTlgString(Language lang) const
{
    std::string sfx;
    if (suffix.get() != 0) {
        sfx = suffixToString(suffix, lang);
    }
    return (boost::format("%s%03d%s") % nsi::Company(airline).code(lang).to866() % number % sfx).str();
}

std::string Flight::toTlgStringUtf(Language lang) const
{
    std::string sfx;
    if (suffix.get() != 0) {
        sfx = EncString::from866(suffixToString(suffix, lang)).toUtf();
    }
    return (boost::format("%s%03d%s") % nsi::Company(airline).code(lang).toUtf() % number % sfx).str();
}

boost::optional<Flight> Flight::fromStr(const std::string& s)
{
    const boost::optional<Flight> empty;
    static const boost::regex rx(
                          "(([" A_YA A_Z "[:digit:]]{1,3})-([[:digit:]]{1,4})([" A_YA A_Z "]?))|"
                          "(([" A_YA A_Z "[:digit:]]{1,2}[" A_YA A_Z "]?)([[:digit:]]{3,4})([" A_YA A_Z "]?))");
    boost::smatch m;
    if (boost::regex_match(s, m, rx)) {
        const int start_index = m[5].matched ? 5 : 1;
        if (!nsi::Company::find(EncString::from866(m[1 + start_index]))) {
            LogTrace(TRACE5) << "Flight::fromStr(" + s + ") failed: invalid airline [" << m[1 + start_index] << "]";
            return empty;
        }
        const nsi::Company awk(EncString::from866(m[1 + start_index]));
        const FlightNum flight_num(std::stoi(m[2 + start_index]));
        if (m[3 + start_index].matched) {
            const boost::optional<Suffix> sfx = suffixFromString(m[3 + start_index]);
            if (!sfx) {
                LogTrace(TRACE5) << "Flight::fromStr(" + s + ") failed: invalid suffix [" << m[3 + start_index] << "]";
                return empty;
            }
            return Flight(awk.id(), flight_num, *sfx);
        }
        return Flight(awk.id(), flight_num);
    } else {
        LogTrace(TRACE5) << "Flight::fromStr(" + s + ") failed: not matched";
        return empty;
    }
}

bool operator==(const Flight& lhs, const Flight& rhs)
{
    return lhs.airline == rhs.airline && lhs.number == rhs.number && lhs.suffix == rhs.suffix;
}

bool operator!=(const Flight& lhs, const Flight& rhs)
{
    return !(lhs == rhs);
}

bool operator<(const Flight& lhs, const Flight& rhs)
{
    FIELD_LESS(lhs.airline, rhs.airline);
    FIELD_LESS(lhs.number, rhs.number);
    return lhs.suffix < rhs.suffix;
}

bool operator>(const Flight& lhs, const Flight& rhs)
{
    return !(lhs == rhs || lhs < rhs);
}

std::ostream& operator<<(std::ostream& s, const Flight& flt)
{
    return s << flt.airline << '-' << flt.number << '/' << flt.suffix;
}

boost::optional<Suffix> suffixFromString(const std::string& s)
{
    if (s.empty()) {
        return Suffix(0);
    }
    if (s.length() != 1) {
        return boost::optional<Suffix>();
    }
    return suffixFromString(s.at(0));
}

static std::map<unsigned char, int> setupSuffixes(const char* s)
{
    std::map<unsigned char, int> vals;
    for (unsigned i = 0; i < 26; ++i) {
        vals[s[i]] = i + 1;
    }
    return vals;
}

static const char* suffixes[2] = {
    "ABCDPEFGHIJKLMNOQRSTUVWXYZ",
    "€‚–„…”†•ˆ‰Š‹ŒŽŸ‘’“ž˜œ›‡"
};

static const std::map<unsigned char, int> latSuffixes = setupSuffixes(suffixes[0]);
static const std::map<unsigned char, int> cp866Suffixes = setupSuffixes(suffixes[1]);

boost::optional<Suffix> suffixFromString(unsigned char c)
{
    std::map<unsigned char, int>::const_iterator it(latSuffixes.find(c));
    if (it != latSuffixes.end()) {
        return Suffix(it->second);
    }
    it = cp866Suffixes.find(c);
    if (it != cp866Suffixes.end()) {
        return Suffix(it->second);
    }
    return boost::optional<Suffix>();
}

std::string suffixToString(Suffix s, Language lang)
{
    if (s.get() == 0) {
        return std::string();
    } else {
        return std::string(1, lang == ENGLISH ? suffixes[0][s.get()-1] : suffixes[1][s.get()-1]);
    }
}

bool operator==(const FlightDate& lhs, const FlightDate& rhs)
{
    return lhs.flt == rhs.flt
        && lhs.dt == rhs.dt;
}

bool operator!=(const FlightDate& lhs, const FlightDate& rhs)
{
    return !(lhs == rhs);
}

bool operator<(const FlightDate& lhs, const FlightDate& rhs)
{
    FIELD_LESS(lhs.flt, rhs.flt);
    return lhs.dt < rhs.dt;
}

bool operator>(const FlightDate& lhs, const FlightDate& rhs)
{
    return !(lhs == rhs || lhs < rhs);
}

std::ostream& operator<<(std::ostream& os, const FlightDate& fd)
{
    return os << fd.flt << ' ' << fd.dt;
}

FlightSeg::FlightSeg(const Flight& f, const nsi::DepArrPoints& s) : flt(f), seg(s)
{
}

bool operator==(const FlightSeg& lhs, const FlightSeg& rhs)
{
    return lhs.flt == rhs.flt && lhs.seg == rhs.seg;
}

bool operator!=(const FlightSeg& lhs, const FlightSeg& rhs)
{
    return !(lhs == rhs);
}

bool operator<(const FlightSeg& lhs, const FlightSeg& rhs)
{
    FIELD_LESS(lhs.flt, rhs.flt);
    return lhs.seg < rhs.seg;
}

bool operator>(const FlightSeg& lhs, const FlightSeg& rhs)
{
    return !(lhs == rhs || lhs < rhs);
}

std::ostream& operator<<(std::ostream& s, const FlightSeg& fs)
{
    return s << fs.flt << ' ' << fs.seg;
}

FlightSegDate::FlightSegDate(const Flight& f, const nsi::DepArrPoints& s,
                             const boost::gregorian::date& d)
    : flt(f), seg(s), dt(d)
{
}

bool operator==(const FlightSegDate& lhs, const FlightSegDate& rhs)
{
    return lhs.flt == rhs.flt && lhs.seg == rhs.seg && lhs.dt == rhs.dt;
}

bool operator!=(const FlightSegDate& lhs, const FlightSegDate& rhs)
{
    return !(lhs == rhs);
}

bool operator<(const FlightSegDate& lhs, const FlightSegDate& rhs)
{
    FIELD_LESS(lhs.flt, rhs.flt);
    FIELD_LESS(lhs.seg, rhs.seg);
    return lhs.dt < rhs.dt;
}

bool operator>(const FlightSegDate& lhs, const FlightSegDate& rhs)
{
    return !(lhs == rhs || lhs < rhs);
}

std::ostream& operator<<(std::ostream& s, const FlightSegDate& fs)
{
    return s << fs.flt << ' ' << fs.seg << ' ' << fs.dt;
}

FlightPointDate::FlightPointDate(const Flight& f, const nsi::PointId& p, const boost::gregorian::date& d)
    : flt(f), dep(p), dt(d)
{
}

bool operator==(const FlightPointDate& lhs, const FlightPointDate& rhs)
{
    return lhs.flt == rhs.flt && lhs.dep == rhs.dep && lhs.dt == rhs.dt;
}

std::ostream& operator<<(std::ostream& s, const FlightPointDate& fpd)
{
    return s << fpd.flt << ' ' << fpd.dep << ' ' << fpd.dt;
}

} // ct

#ifdef XP_TESTING
#include <serverlib/checkunit.h>
void init_flight_tests(){}
namespace
{
    using namespace StrUtils;

START_TEST(test_suffix)
{
    fail_unless(ct::suffixToString(ct::Suffix(0)) == "");
    fail_unless(ct::suffixToString(ct::Suffix(1)) == "A");
    fail_unless(ct::suffixToString(ct::Suffix(26)) == "Z");
    fail_unless(ct::suffixFromString("Z").get().get() == 26);
    fail_unless(ct::suffixFromString("").get().get() == 0);
    fail_if(ct::suffixFromString(" "));
}
END_TEST





#define SUITENAME "coretypes"
TCASEREGISTER(0,0)
{
    ADD_TEST(test_suffix);
}
TCASEFINISH;
} /* namespace */
#endif /*XP_TESTING*/


