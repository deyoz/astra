#include "ucm_parser.h"
#include "franchise.h"
#include "astra_misc.h" // for DayToDate
#include "date_time.h" // for NowUTC()
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

namespace new_breed {
    string TlgElemToElemId(TElemType type, const string &elem, bool with_icao = false)
    {
        TElemFmt fmt;
        string id2;

        id2=ElemToElemId(type, elem, fmt, false);
        if (fmt==efmtUnknown)
            throw EBaseTableError("TlgElemToElemId: elem not found (type=%s, elem=%s)",
                    EncodeElemType(type),elem.c_str());
        if (id2.empty())
            throw EBaseTableError("TlgElemToElemId: id is empty (type=%s, elem=%s)",
                    EncodeElemType(type),elem.c_str());
        if (!with_icao && (fmt==efmtCodeICAONative || fmt==efmtCodeICAOInter))
            throw EBaseTableError("TlgElemToElemId: ICAO only elem found (type=%s, elem=%s)",
                    EncodeElemType(type),elem.c_str());

        return id2;
    }

    string GetAirline(const string &airline, bool with_icao = true)
    {
        try
        {
            return TlgElemToElemId(etAirline,airline,with_icao);
        }
        catch (EBaseTableError)
        {
            throw ETlgError("Unknown airline code '%s'", airline.c_str());
        };
    };

    string GetSuffix(const string &suffix)
    {
        string result;
        if (not suffix.empty())
        {
            try
            {
                result = TlgElemToElemId(etSuffix,suffix);
            }
            catch (EBaseTableError)
            {
                throw ETlgError("Unknown flight number suffix '%s'", suffix.c_str());
            };
        };
        return result;
    };
}

namespace regex {
    static const string  m = "[А-ЯЁA-Z0-9]";
    static const string a = "[А-ЯЁA-Z]";

    static const string airline = "(" + m + "{2}" + a + "?)";
    static const string airp = "(" + a + "{3})";
    static const string flt_no = "(\\d{1,4})(" + a + "?)";
    static const string date = "(\\d{2})";
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

    static const boost::regex e1("^" + regex::airline + regex::flt_no + "/" + regex::flt_no + "/" + regex::date + ".*");
    static const boost::regex e2("^" + regex::airline + regex::flt_no + "/" + regex::date + ".*");

    boost::match_results<std::string::const_iterator> results;

    if (boost::regex_match(src, results, e1))
    {
        airline = new_breed::GetAirline(results[1]);
        flt_no = ToInt(results[2]);
        suffix = new_breed::GetSuffix(results[3]);
        date = DayToDate(ToInt(results[6]), NowUTC() + 1, true);
    } else if (boost::regex_match(src, results, e2))
    {
        airline = new_breed::GetAirline(results[1]);
        flt_no = ToInt(results[2]);
        suffix = new_breed::GetSuffix(results[3]);
        date = DayToDate(ToInt(results[4]), NowUTC() + 1, true);
    } else
        throw ETlgError(tlgeNotMonitorNotAlarm, "Wrong flight: " + src);

    /*
    // Применение франчайз
    TTripInfo trip_info;
    trip_info.airline = airline;
    trip_info.airp = airp;
    trip_info.flt_no = flt_no;
    trip_info.suffix = suffix;
    trip_info.scd_out = date;
    Franchise::TProp franchise_prop;
    if(franchise_prop.get_franchisee(trip_info, Franchise::TPropType::wb) and franchise_prop.val == Franchise::pvYes) {
        airline = franchise_prop.oper.airline;
        flt_no = franchise_prop.oper.flt_no;
        suffix = franchise_prop.oper.suffix;
    }
    */

    // привязка к рейсы
    flts.push_back(TFltForBind(toFltInfo(),  btFirstSeg, TSearchFltInfoPtr()));
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
