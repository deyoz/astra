#include "loadsheet_parser.h"
#include <utility>
#include <regex>

#define NICKNAME "DEN"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC::date_time;

namespace TypeB
{

TTlgPartInfo ParseLOADSHEETHeading(TTlgPartInfo heading, TUCMHeadingInfo &info)
{
    TTlgPartInfo next;
    const char *p, *line_p;
    TTlgParser tlg;
    line_p=heading.p;
    size_t rownum = 1;
    try
    {
        do
        {
            if ((p=tlg.GetToEOLLexeme(line_p))==NULL) continue;
            switch(rownum) {
                case 1:
                    {
                        static const std::regex e_flt("^" + regex::airline + regex::flt_no + "/" + regex::date + " .*");
                        std::cmatch results;
                        if(std::regex_match(tlg.lex, results, e_flt)) {
                            info.flt_info.airline = GetAirline(results[1]);
                            info.flt_info.flt_no = ToInt(results[2]);
                            info.flt_info.suffix = GetSuffix(results[3]);
                            info.flt_info.date = ParseDate(ToInt(results[4]));
                        } else
                            throw ETlgError(tlgeNotMonitorNotAlarm, "Wrong flight: " + info.flt_info.src);
                        info.flt_info.src = tlg.lex;

                        break;
                    }
                case 2:
                    {
                        static const std::regex e_airp("^" + regex::airp + " .*");
                        std::cmatch results;
                        if (std::regex_match(tlg.lex, results, e_airp)) {
                            info.flt_info.airp = GetAirp(results[1]);
                        } else
                            throw ETlgError(tlgeNotMonitorNotAlarm, "Wrong airp: " + info.flt_info.src);
                        info.flt_info.src += (string)"\n" + tlg.lex;
                        line_p=tlg.NextLine(line_p);
                        return nextPart(heading, line_p);
                    }
            }
        }
        while ((line_p=tlg.NextLine(line_p))!=NULL and rownum++);
    }
    catch(const ETlgError &E)
    {
        throwTlgError(E.what(), heading, line_p);
    };
    return nextPart(heading, line_p);
}

}
