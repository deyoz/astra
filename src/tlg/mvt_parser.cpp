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
        const string MVTMsgS[] = {
            "AD",
            "AA"
        };


        TMVTMsg DecodeMVTMsg(const std::string s)
        {
            int i;
            for( i=0; i<(int)msgNum; i++ )
                if ( s == MVTMsgS[ i ] )
                    break;
            return (TMVTMsg)i;
        }
        const std::string EncodeMVTMsg(TMVTMsg s)
        {
            return MVTMsgS[s];
        }

        enum TMVTElem {
            mvtCaptionElement,
            mvtFlightElement,
            mvtMsg,
            mvtFLD,
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

        TDateTime nearest_date(TDateTime time, int day)
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

        TDateTime fetch_time(TDateTime scd, const string &val)
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

        void TAA::parse(TDateTime scd, const string &val)
        {
            vector<string> items;
            split(items, val, '/');
            if(items.size() == 1) {
                touch_down_time = fetch_time(scd, items[0]);
            } else if(items.size() == 2) {
                if(not items[0].empty())
                    touch_down_time = fetch_time(scd, items[0]);
                on_block_time = fetch_time(scd, items[1]);
            } else
                throw ETlgError("wrong AA format");
        }

        void TAD::parse(TDateTime scd, const string &val)
        {
            if(val.substr(0, 2) == "AD") {
                string buf =  val.substr(2);
                vector<string> items;
                split(items, buf, ' ');
                if(items.size() == 3 or items.size() == 1) {
                    string str_ad = (items.size() == 1 ? buf : items[0]);
                    vector<string> ad_times;
                    split(ad_times, str_ad, '/');
                    if(ad_times.size() == 2) {
                        off_block_time = fetch_time(scd, ad_times[0]);
                        airborne_time = fetch_time(scd, ad_times[1]);
                    } else if(ad_times.size() == 1) {
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
            TMVTElem e = mvtCaptionElement;
            try
            {
                do {
                    tlg.GetToEOLLexeme(line_p);
                    if(not *tlg.lex)
                        throw ETlgError("blank line not allowed");
                    switch(e) {
                        case mvtCaptionElement: // Not AHM standard. Used by Cobra.
                            e = mvtFlightElement;
                            if(strcmp(tlg.lex, "MVT") == 0)
                                break;
                        case mvtFlightElement:
                            {
                                TTlgPartInfo tmp_body;
                                tmp_body.p = line_p;
                                ParseAHMFltInfo(tmp_body,info,info.flt,info.bind_type);
                                e = mvtMsg;
                            }
                            break;
                        case mvtMsg:
                            {
                                const string &msg_typeS = string(tlg.lex).substr(0, 2);
                                con.msg_type = DecodeMVTMsg(msg_typeS);
                                switch(con.msg_type) {
                                    case msgAD:
                                        con.ad.parse(info.flt.scd, tlg.lex);
                                        e = mvtOthers;
                                        break;
                                    case msgAA:
                                        con.aa.parse(info.flt.scd, string(tlg.lex).substr(2));
                                        e = mvtFLD;
                                        break;
                                    default:
                                        throw ETlgError("unknown MVT message: '%s'", msg_typeS.c_str());
                                }
                                break;
                            }
                        case mvtFLD:
                            {
                                if(string(tlg.lex).substr(0, 3) != "FLD")
                                    throw ETlgError("unexpected lexeme");
                                const string &fldS = string(tlg.lex).substr(3);
                                if(fldS.size() != 2)
                                    throw ETlgError("wrong fld string length: %s", fldS.c_str());
                                con.aa.fld_time = nearest_date(info.flt.scd, ToInt(fldS));
                                e = mvtOthers;
                                break;
                            }
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
            if(con.msg_type == msgAD) { // �����
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
            } else if(con.msg_type == msgAA) { // ��ᠤ��
                TDateTime utc_arv = con.aa.touch_down_time == NoExists ? con.aa.on_block_time : con.aa.touch_down_time;
                LogTrace(TRACE5) << "touch_down_time: " << DateTimeToStr(utc_arv, ServerFormatDateTimeAsString);
            }
        }
    }
}
