#include "mvt_parser.h"

#define NICKNAME "DEN"
#include <serverlib/slogger.h>
#include "sopp.h"

using namespace std;
using namespace BASIC;
using namespace ASTRA;

namespace TypeB
{
    enum TMVTElem {
        mvtFlightElement,
        mvtADElement,
        mvtOthers
    };

    void TAD::dump()
    {
        LogTrace(TRACE5) << "------------TAD::dump()------------------";
        if(off_block_time != ASTRA::NoExists) {
            LogTrace(TRACE5) << "off_block_time: " << DateTimeToStr(off_block_time, BASIC::ServerFormatDateTimeAsString);
            LogTrace(TRACE5) << "airborne_time: " << DateTimeToStr(airborne_time, BASIC::ServerFormatDateTimeAsString);
            LogTrace(TRACE5) << "ea: " << DateTimeToStr(ea, BASIC::ServerFormatDateTimeAsString);
            LogTrace(TRACE5) << "airp_arv: " << airp_arv;
        }
        LogTrace(TRACE5) << "-----------------------------------------";
    }

    TDateTime TAD::fetch_time(const string &val)
    {
        TDateTime result = NoExists;
        int year, mon, day;
        DecodeDate( NowUTC(), year, mon, day );
        TDateTime day_create, time_create;
        if(val.size() == 6) {
            EncodeDate(year,mon,ToInt(val.substr(0, 2)),day_create);
            EncodeTime(
                    ToInt(val.substr(2, 2)),
                    ToInt(val.substr(3, 2)),
                    0,
                    time_create);
            result = day_create + time_create;
        } else if(val.size() == 4) {
            EncodeDate(year,mon,day,day_create);
            EncodeTime(
                    ToInt(val.substr(0, 2)),
                    ToInt(val.substr(3, 2)),
                    0,
                    time_create);
            result = day_create + time_create;
        } else
            throw ETlgError("wrong date format: '%s'", val.c_str());
        return result;
    }

    void TAD::parse(const string &val)
    {
        if(val.substr(0, 2) == "AD") {
            vector<string> items;
            split(items, val.substr(2), ' ');
            if(items.size() == 3 or items.size() == 1) {
                vector<string> ad_times;
                split(ad_times, items[0], '/');
                if(ad_times.size() == 2) {
                    off_block_time = fetch_time(ad_times[0]);
                    airborne_time = fetch_time(ad_times[1]);
                } else if(ad_times.size() == 1) {
                    off_block_time = fetch_time(ad_times[0]);
                } else
                    throw ETlgError("wrong AD times format: '%s'", val.c_str());
                if(items.size() == 3) {
                    if(items[1].substr(0, 2) == "EA") {
                        string ea_time = items[1].substr(2);
                        if(ea_time.size() != 4)
                            throw ETlgError("wrong EA time size: '%s'", ea_time.c_str());
                        ea = fetch_time(ea_time);
                    } else
                        throw ETlgError("wrong EA format: '%s'", items[1].c_str());
                    TElemFmt fmt;
                    airp_arv = ElemToElemId(etAirp, items[2], fmt);
                }
            } else
                throw ETlgError("wrong AD format");
        }
    }

    void ParseMVTContent(TTlgPartInfo body, TAHMHeadingInfo& info, TMVTContent& con, TMemoryManager &mem) {
        con.Clear();
        TTlgParser tlg;
        const char *line_p=body.p;
        TMVTElem e = mvtFlightElement;
        try
        {
            do {
                tlg.GetToEOLLexeme(line_p);
                if(not *tlg.lex)
                    throw ETlgError("blank line not allowed");
                switch(e) {
                    case mvtFlightElement:
                        e = mvtADElement;
                        break;
                    case mvtADElement:
                        LogTrace(TRACE5) << "'" << tlg.lex << "'";
                        con.ad.parse(tlg.lex);
                        con.ad.dump();
                        e = mvtOthers;
                        break;
                    default:
                        break;
                }
                line_p=tlg.NextLine(line_p);
            } while (line_p and *line_p != 0);
        }
        catch (ETlgError E)
        {
            throwTlgError(E.what(), body, line_p);
        };
    }

    void SaveMVTContent(int tlg_id, TAHMHeadingInfo& info, TMVTContent& con) {
        int point_id_tlg=SaveFlt(tlg_id,info.flt,info.bind_type);
        TTripInfo t;
        t.airline = info.flt.airline;
        t.flt_no = info.flt.flt_no;
        t.airp = info.flt.airp_dep;
        if(GetTripSets(tsSetDepTimeByMVT, t)) {
            TQuery Qry(&OraSession);
            Qry.SQLText =
                "SELECT point_id_spp FROM tlg_binding WHERE point_id_tlg=:point_id";
            Qry.CreateVariable("point_id", otInteger, point_id_tlg);
            Qry.Execute();
            if ( Qry.Eof ) {
                throw EXCEPTIONS::Exception( "Flight not found, point_id_tlg=%d", point_id_tlg );
            }
            int point_id_spp = Qry.FieldAsInteger( "point_id_spp" );
            SetFlightFact(point_id_spp, (con.ad.airborne_time == NoExists ? con.ad.off_block_time : con.ad.airborne_time));  //UTC???s
        }
    }
}
