#include "ucm_parser.h"
#include "ssm_parser.h" // for ParseDate
#include <utility>
#include <boost/regex.hpp>

#define NICKNAME "DEN"
#include "serverlib/slogger.h"

using namespace std;

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
            info.flt_info.parse(tlg.lex,flts);
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

void TUCMFltInfo::parse(const char *val, TFlightsForBind &flts)
{
    src = val; // это важно: boost::regex_match(strin(val), results, e2) не прокатит, непредсказуемо.

    static const boost::regex e1("^" + regex::airline + regex::flt_no + "/" + regex::flt_no + "/" + regex::date + ".*");
    static const boost::regex e2("^" + regex::airline + regex::flt_no + "/" + regex::date + ".*");

    boost::match_results<std::string::const_iterator> results;

    if (boost::regex_match(src, results, e1))
    {
        airline = new_breed::GetAirline(results[1]);
        flt_no = ToInt(results[2]);
        suffix = new_breed::GetSuffix(results[3]);
        date = ParseDate(results[6]);
    } else if (boost::regex_match(src, results, e2))
    {
        airline = new_breed::GetAirline(results[1]);
        flt_no = ToInt(results[2]);
        suffix = new_breed::GetSuffix(results[3]);
        date = ParseDate(results[4]);
    } else
        throw ETlgError(tlgeNotMonitorNotAlarm, "Wrong flight: " + src);

    // привязка к рейсы
    flts.push_back(make_pair(toFltInfo(), btFirstSeg));
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

}
