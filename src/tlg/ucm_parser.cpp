#include "ucm_parser.h"
#include "franchise.h"
#include <utility>
#include <boost/regex.hpp>

#define NICKNAME "DEN"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC::date_time;

namespace TypeB
{

TTlgPartInfo ParseUCMHeading(TTlgPartInfo heading, TUCMHeadingInfo &info, TFlightsForBind &flts)
{
    TTlgPartInfo next;
    const char *p, *line_p;
    TTlgParser tlg;
    line_p=heading.p;
    try
    {
        do
        {
            if ((p=tlg.GetToEOLLexeme(line_p))==NULL) continue;
            info.flt_info.parse(tlg.lex,flts,info.tlg_cat);
            line_p=tlg.NextLine(line_p);
            return nextPart(heading, line_p);
        }
        while ((line_p=tlg.NextLine(line_p))!=NULL);
    }
    catch(ETlgError E)
    {
        throwTlgError(E.what(), heading, line_p);
    };
    return nextPart(heading, line_p);
}

void TUCMFltInfo::clear()
{
    src.clear();
    airline.clear();
    airp.clear();
    flt_no = NoExists;
    suffix.clear();
    date = NoExists;
}

void TUCMFltInfo::parse(const char *val, TFlightsForBind &flts, TTlgCategory tlg_cat)
{
    clear();
    src = val; // это важно: boost::regex_match(strin(val), results, e2) не прокатит, непредсказуемо.

    static const boost::regex e_ntm("^" + regex::airline + regex::flt_no + "/" + regex::date_month + " " + regex::airp + ".*");
    static const boost::regex e_sls_ucm(
            "^" + regex::airline + regex::flt_no + "/" + regex::date +
            regex::full_stop + regex::bort +
            regex::full_stop + regex::aircraft_vers +
            regex::full_stop + regex::airp +
            ".*");
    static const boost::regex e1("^" + regex::airline + regex::flt_no + "/" + regex::flt_no + "/" + regex::date + ".*");
    static const boost::regex e2("^" + regex::airline + regex::flt_no + "/" + regex::date + ".*");

    boost::match_results<std::string::const_iterator> results;

    // для LDM и CPM АП вылета неизвестен

    if (boost::regex_match(src, results, e_ntm)) {
        // NTM
        airline = GetAirline(results[1]);
        flt_no = ToInt(results[2]);
        suffix = GetSuffix(results[3]);
        date = ParseDate(results[4]);
        airp = GetAirp(results[5]);
    } else if (boost::regex_match(src, results, e_sls_ucm)) {
        // SLS & UCM
        airline = GetAirline(results[1]);
        flt_no = ToInt(results[2]);
        suffix = GetSuffix(results[3]);
        date = ParseDate(ToInt(results[4]));
        airp = GetAirp(results[7]);
    } else if (boost::regex_match(src, results, e1)) {
        // OTHER 1 (unknown airp)
        airline = GetAirline(results[1]);
        flt_no = ToInt(results[2]);
        suffix = GetSuffix(results[3]);
        date = ParseDate(ToInt(results[6]));
    } else if (boost::regex_match(src, results, e2)) {
        // OTHER 2 (unknown airp)
        airline = GetAirline(results[1]);
        flt_no = ToInt(results[2]);
        suffix = GetSuffix(results[3]);
        date = ParseDate(ToInt(results[4]));
    } else
        throw ETlgError(tlgeNotMonitorNotAlarm, "Wrong flight: " + src);

    TTripInfo trip_info;
    trip_info.airline = airline;
    trip_info.airp = airp;
    trip_info.flt_no = flt_no;
    trip_info.suffix = suffix;
    trip_info.scd_out = date;

    vector<TTripInfo> franchise_flts;
    get_wb_franchise_flts(trip_info, franchise_flts);

    // привязка к рейсы
    for(const auto &flt: franchise_flts)
        flts.push_back(TFltForBind(TFltInfo(flt),  btFirstSeg, TSearchFltInfoPtr()));
}

TFltInfo TUCMFltInfo::toFltInfo()
{
    TFltInfo result;
    strcpy(result.airline, airline.c_str());
    strcpy(result.airp_dep, airp.c_str());
    result.flt_no=flt_no;
    result.suffix[0] = (suffix.empty() ? 0 : suffix[0]);
    result.suffix[1] = 0;
    result.scd=date;
    result.pr_utc=true;
    *result.airp_arv = 0;
    result.dump();
    return result;
}

string TUCMFltInfo::toString()
{
    ostringstream result;
    result
        << "src: '" << src << "': "
        << airline
        << setw(3) << setfill('0') << flt_no
        << suffix << ' '
        << airp << ' '
        << DateTimeToStr(date, ServerFormatDateTimeAsString);
    return result.str();
}

}
