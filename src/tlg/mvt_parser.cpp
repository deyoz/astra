#include "mvt_parser.h"

#define NICKNAME "DEN"
#include <serverlib/slogger.h>
#include "sopp.h"

using namespace std;
using namespace BASIC;
using namespace ASTRA;

namespace TypeB
{
    namespace MVTParser
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

        TDateTime TAD::nearest_date(TDateTime time, int day)
        {
            TDateTime result = NoExists;
            TDateTime back_date = DayToDate(day, time, true);
            if(back_date != time) {
                TDateTime forward_date = DayToDate(day, time, false);
                TDateTime back_interval = time - back_date;
                TDateTime forward_interval = forward_date - time;
                if(back_interval < forward_interval)
                    result = back_date;
                else
                    result = forward_date;
            } else
                result = back_date;
            return result;
        }

        TDateTime TAD::fetch_time(TDateTime scd, const string &val)
        {
            TDateTime result = NoExists;
            TDateTime day_create, time_create;
            int year, mon, day;
            int hour;
            int minute;
            if(val.size() == 6) {
                day = ToInt(val.substr(0, 2));
                hour = ToInt(val.substr(2, 2));
                minute = ToInt(val.substr(4, 2));

                result = nearest_date(scd, day);

                DecodeDate( result, year, mon, day );

                EncodeDate(year,mon,day,day_create);
                EncodeTime(hour, minute, 0, time_create);
                result = day_create + time_create;
            } else if(val.size() == 4) {
                DecodeDate( scd, year, mon, day );
                hour = ToInt(val.substr(0, 2));
                minute = ToInt(val.substr(2, 2));
                EncodeDate(year,mon,day,day_create);
                EncodeTime(hour, minute, 0, time_create);
                result = day_create + time_create;
            } else
                throw ETlgError("wrong date format: '%s'", val.c_str());
            return result;
        }

        void TAD::parse(TDateTime scd, const string &val)
        {
            if(val.substr(0, 2) == "AD") {
                string buf =  val.substr(2);
                vector<string> items;
                split(items, buf, ' ');
                if(items.size() == 3 or items.size() == 0) {
                    string str_ad = (items.size() == 0 ? buf : items[0]);
                    vector<string> ad_times;
                    split(ad_times, str_ad, '/');
                    if(ad_times.size() == 2) {
                        off_block_time = fetch_time(scd, ad_times[0]);
                        airborne_time = fetch_time(scd, ad_times[1]);
                    } else if(ad_times.size() == 0) {
                        off_block_time = fetch_time(scd, str_ad);
                    } else
                        throw ETlgError("wrong AD times format: '%s'", val.c_str());
                    if(items.size() == 3) {
                        if(items[1].substr(0, 2) == "EA") {
                            ea = fetch_time(scd, items[1].substr(2));
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
                            con.ad.parse(info.flt.scd, tlg.lex);
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
                TDateTime utc_act_out = (con.ad.airborne_time == NoExists ? con.ad.off_block_time : con.ad.airborne_time);
                if(utc_act_out != NoExists) {
                    // forcibly set time to be used as UTC
                    TReqInfo::Instance()->user.sets.time = ustTimeUTC;
                    SetFlightFact(point_id_spp, utc_act_out);
                }
            }
        }
    }
}
