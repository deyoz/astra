#include "mvt_parser.h"
#include <boost/regex.hpp>

#define NICKNAME "DEN"
#include <serverlib/slogger.h>
#include "sopp.h"

using namespace std;
using namespace BASIC;

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

    void TAD::parse(const string &val)
    {
        boost::regex pattern("^AD(\\d{2})(\\d{2})/(\\d{2})(\\d{2}) EA(\\d{2})(\\d{2}) (\\w{3})$");
//        boost::regex pattern("^AD(\\d{4})/(\\d{4}) EA(\\d{4}) (\\w{3})$");
        boost::smatch result;
        if(boost::regex_search(val, result, pattern)) {
            int year, mon, day;
            DecodeDate( NowUTC(), year, mon, day );
            TDateTime day_create, time_create;
            EncodeDate(year,mon,day,day_create);

            EncodeTime(
                    ToInt(result[1]),
                    ToInt(result[2]),
                    0,
                    time_create);
            off_block_time = day_create + time_create;

            EncodeTime(
                    ToInt(result[3]),
                    ToInt(result[4]),
                    0,
                    time_create);
            airborne_time = day_create + time_create;

            EncodeTime(
                    ToInt(result[5]),
                    ToInt(result[6]),
                    0,
                    time_create);
            ea = day_create + time_create;

            TElemFmt fmt;
            airp_arv = ElemToElemId(etAirp, result[7], fmt);
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
                        con.ad.parse(tlg.lex);
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
            SetFlightFact(point_id_spp, con.ad.airborne_time);  //UTC???s
        }
    }
}
