#include "stat/stat_utils.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "qrys.h"
#include "baggage_ckin.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace ASTRA;

bool STAT::bad_client_img_version()
{
    return TReqInfo::Instance()->desk.compatible("201101-0117116") and not TReqInfo::Instance()->desk.compatible("201101-0118748");
}


xmlNodePtr STAT::getVariablesNode(xmlNodePtr resNode)
{
    xmlNodePtr formDataNode = GetNode("form_data", resNode);
    if(!formDataNode)
        formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = GetNode("variables", formDataNode);
    if(!variablesNode)
        variablesNode = NewTextChild(formDataNode, "variables");
    return variablesNode;
}

xmlNodePtr STAT::set_variables(xmlNodePtr resNode, string lang)
{
    if(lang.empty())
        lang = TReqInfo::Instance()->desk.lang;

    xmlNodePtr variablesNode = getVariablesNode(resNode);

    TReqInfo *reqInfo = TReqInfo::Instance();
    TDateTime issued = NowUTC();
    string tz;
    if(reqInfo->user.sets.time == ustTimeUTC)
        tz = "(GMT)";
    else if(
            reqInfo->user.sets.time == ustTimeLocalDesk ||
            reqInfo->user.sets.time == ustTimeLocalAirp
           ) {
        issued = UTCToLocal(issued,reqInfo->desk.tz_region);
        tz = "(" + ElemIdToCodeNative(etCity, reqInfo->desk.city) + ")";
    }

    NewTextChild(variablesNode, "print_date",
            DateTimeToStr(issued, "dd.mm.yyyy hh:nn:ss ") + tz);
    NewTextChild(variablesNode, "print_oper", reqInfo->user.login);
    NewTextChild(variablesNode, "print_term", reqInfo->desk.code);
    NewTextChild(variablesNode, "use_seances", false); //!!!потом убрать
    NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
    if(bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "cap_test", getLocaleText("CAP.TEST", lang));
    NewTextChild(variablesNode, "page_number_fmt", getLocaleText("CAP.PAGE_NUMBER_FMT", lang));
    NewTextChild(variablesNode, "short_page_number_fmt", getLocaleText("CAP.SHORT_PAGE_NUMBER_FMT", lang));
    NewTextChild(variablesNode, "oper_info", getLocaleText("CAP.DOC.OPER_INFO", LParams()
                << LParam("date", DateTimeToStr(issued, "dd.mm.yyyy hh:nn:ss ") + tz)
                << LParam("oper", reqInfo->user.login)
                << LParam("term", reqInfo->desk.code),
                lang
                ));
    NewTextChild(variablesNode, "skip_header", 0);
    return variablesNode;
}

void TStatPlaces::set(string aval, bool pr_locale)
{
    if(not result.empty())
        throw Exception("TStatPlaces::set(): already set");
    if(pr_locale) {
        vector<string> tokens;
        while(true) {
            size_t idx = aval.find('-');
            if(idx == string::npos) break;
            tokens.push_back(aval.substr(0, idx));
            aval.erase(0, idx + 1);
        }
        tokens.push_back(aval);
        for(vector<string>::iterator is = tokens.begin(); is != tokens.end(); is++)
            result += (result.empty() ? "" : "-") + ElemIdToCodeNative(etAirp, *is);
    } else
        result = aval;
}

string TStatPlaces::get() const
{
    return result;
}

bool TPointsRow::operator == (const TPointsRow &item) const
{
    return part_key==item.part_key &&
        name==item.name &&
        point_id==item.point_id;
};

bool lessPointsRow(const TPointsRow& item1,const TPointsRow& item2)
{
    bool result;
    if(item1.real_out_client == item2.real_out_client) {
        if(item1.flt_no == item2.flt_no) {
            if(item1.airline == item2.airline) {
                if(item1.suffix == item2.suffix) {
                    if(item1.move_id == item2.move_id) {
                        result = item1.point_num < item2.point_num;
                    } else
                        result = item1.move_id < item2.move_id;
                } else
                    result = item1.suffix < item2.suffix;
            } else
                result = item1.airline < item2.airline;
        } else
            result = item1.flt_no < item2.flt_no;
    } else
        result = item1.real_out_client > item2.real_out_client;
    return result;
};

bool ArxValidateByAirp(const TDateTime& part_key, const int point_id,
                      const std::string& airp, int point_num)
{
    LogTrace5 << __func__ << " part_key: " << part_key << " point_id: " << point_id << " airp: " << airp;
    TReqInfo &reqInfo = *(TReqInfo::Instance());
    if (!reqInfo.user.access.airps().elems().empty()) {
        std::optional<std::string> next_airp = CKIN::next_airp(point_id, point_num, DateTimeToBoost(part_key));
        const auto& airps = reqInfo.user.access.airps().elems();
        if (reqInfo.user.access.airps().elems_permit())
        {
            return (airps.count(airp)>0 || airps.count(next_airp.value_or(""))>0);
//            sql << "AND (airp IN " << GetSQLEnum(reqInfo.user.access.airps().elems()) << " OR \n";
//            sql << "arch.next_airp(arx_points.part_key, (CASE WHEN pr_tranzit=0 THEN point_id ELSE first_point END), point_num) IN \n";
//            sql << GetSQLEnum(reqInfo.user.access.airps().elems()) << ") \n";
        }
        else
        {
            return (airps.count(airp)==0 || airps.count(next_airp.value_or(""))==0);
//            sql << "AND (airp NOT IN " << GetSQLEnum(reqInfo.user.access.airps().elems()) << " OR \n";
//            sql << "arch.next_airp(arx_points.part_key, (CASE WHEN pr_tranzit=0 THEN point_id ELSE first_point END), point_num) NOT IN \n";
//            sql << GetSQLEnum(reqInfo.user.access.airps().elems()) << ") \n";
        }
    }
    return true;
}

void FltCBoxListRead(DB::TQuery& Qry, vector<TPointsRow> &points, int& count)
{
    try {
        Qry.Execute();
    } catch (EOracleError &E) {
        if(E.Code == 376)
            throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
        else
            throw;
    }
    string trip_name;
    if(!Qry.Eof) {
        int col_move_id=Qry.FieldIndex("move_id");
        int col_point_num=Qry.FieldIndex("point_num");
        int col_point_id=Qry.FieldIndex("point_id");
        int col_part_key=Qry.FieldIndex("part_key");

        for( ; !Qry.Eof; Qry.Next()) {
            TTripInfo tripInfo(Qry);
            try
            {
                trip_name = GetTripName(tripInfo,ecCkin,false,true);
            }
            catch(AstraLocale::UserException &E)
            {
                AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
                continue;
            };
            TPointsRow pointsRow;
            TDateTime scd_out_client;
            if(Qry.FieldIsNULL(col_part_key))
                pointsRow.part_key = NoExists;
            else {
                pointsRow.part_key =  Qry.FieldAsDateTime(col_part_key);
            }
            pointsRow.point_id = Qry.FieldAsInteger(col_point_id);
            tripInfo.get_client_dates(scd_out_client, pointsRow.real_out_client);
            pointsRow.airline = tripInfo.airline;
            pointsRow.suffix = tripInfo.suffix;
            pointsRow.name = trip_name;
            pointsRow.flt_no = tripInfo.flt_no;
            pointsRow.move_id = Qry.FieldAsInteger(col_move_id);
            pointsRow.point_num = Qry.FieldAsInteger(col_point_num);
            if( pointsRow.part_key != NoExists) {
                int first_point = Qry.FieldAsInteger("first_point");
                int pr_tranzit = Qry.FieldAsInteger("pr_tranzit");
                std::string airp = Qry.FieldAsString("airp");
                int point_id = pr_tranzit==0 ? pointsRow.point_id : first_point;

                if(ArxValidateByAirp(pointsRow.part_key, point_id, airp, pointsRow.point_num)) {
                    points.push_back(pointsRow);
                }
            } else {
                points.push_back(pointsRow);
            }

            count++;
            if(count >= MAX_STAT_ROWS()) {
                AstraLocale::showErrorMessage("MSG.TOO_MANY_FLIGHTS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                        LParams() << LParam("num", MAX_STAT_ROWS()));
                break;
            }
        }
    }
}

void ArxGetFltCBoxList(TScreenState scr, TDateTime first_date, TDateTime last_date, bool pr_show_del,
                       vector<TPointsRow> &points, int &count)
{
    LogTrace5 << __func__ << " first_date: " << DateTimeToBoost(first_date)
              << " last_date: " << DateTimeToBoost(last_date);
    TReqInfo &reqInfo = *(TReqInfo::Instance());
    DB::TQuery Qry(PgOra::getROSession("ARX_POINTS"), STDLOG);
    Qry.CreateVariable("FirstDate", otDate, first_date);
    Qry.CreateVariable("LastDate", otDate, last_date);
    TPerfTimer tm;
    tm.Init();
    if (!reqInfo.user.access.totally_not_permitted())
    {
        for(int pass=1; pass<=2; pass++)
        {
            ostringstream sql;
            sql << "SELECT \n"
                "    arx_points.part_key, \n"
                "    arx_points.move_id, \n"
                   " arx_points.airp, \n"
                   " arx_points.pr_tranzit, \n"
                   " arx_points.first_point, \n";
            sql << "    " << TTripInfo::selectedFields() << ", \n"
                   "    point_num \n"
                   "FROM arx_points \n";
            if (pass==1) {
                sql << "WHERE arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND \n"
                       "      arx_points.part_key >= :FirstDate AND arx_points.part_key < :arx_trip_date_range \n";
            }
            if (pass==2) {
                sql << getMoveArxQuery();
                sql<< " WHERE arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND \n"
                      "      arx_points.part_key=arx_ext.part_key AND arx_points.move_id=arx_ext.move_id \n";
            }
            if(scr == ssPaxList)
                sql << " AND pr_del = 0 \n";
            if((scr == ssFltLog or scr == ssFltTaskLog) and !pr_show_del)
                sql << " AND pr_del >= 0 \n";
            if (!reqInfo.user.access.airlines().elems().empty()) {
                if (reqInfo.user.access.airlines().elems_permit())
                    sql << " AND airline IN " << GetSQLEnum(reqInfo.user.access.airlines().elems()) << "\n";
                else
                    sql << " AND airline NOT IN " << GetSQLEnum(reqInfo.user.access.airlines().elems()) << "\n";
            }
            Qry.CreateVariable("arx_trip_date_range", otDate, last_date + ARX_TRIP_DATE_RANGE());

            //ProgTrace(TRACE5, "FltCBoxDropDown: pass=%d SQL=\n%s", pass, sql.str().c_str());
            Qry.SQLText = sql.str().c_str();
            FltCBoxListRead(Qry, points, count);
        }
    }
}

void GetFltCBoxList(TScreenState scr, TDateTime first_date, TDateTime last_date, bool pr_show_del, vector<TPointsRow> &points)
{
    TReqInfo &reqInfo = *(TReqInfo::Instance());
    DB::TQuery Qry(PgOra::getROSession("POINTS"), STDLOG);
    Qry.CreateVariable("FirstDate", otDate, first_date);
    Qry.CreateVariable("LastDate", otDate, last_date);
    TPerfTimer tm;
    tm.Init();
    int count = 0;
    if (!reqInfo.user.access.totally_not_permitted())
    {
        ostringstream sql;
        sql << "SELECT \n"
                "    NULL part_key, \n"
                "    move_id, \n";
        sql << "    " << TTripInfo::selectedFields() << ", \n"
               "    point_num \n";
        sql << "FROM points \n"
                "WHERE points.scd_out >= :FirstDate AND points.scd_out < :LastDate \n";
        if(scr == ssPaxList)
            sql << " AND pr_del = 0 \n";
        if((scr == ssFltLog or scr == ssFltTaskLog) and !pr_show_del)
            sql << " AND pr_del >= 0 \n";
        if (!reqInfo.user.access.airlines().elems().empty()) {
            if (reqInfo.user.access.airlines().elems_permit())
                sql << " AND airline IN " << GetSQLEnum(reqInfo.user.access.airlines().elems()) << "\n";
            else
                sql << " AND airline NOT IN " << GetSQLEnum(reqInfo.user.access.airlines().elems()) << "\n";
        }
        if (!reqInfo.user.access.airps().elems().empty()) {
            if (reqInfo.user.access.airps().elems_permit())
            {
                sql << "AND (airp IN " << GetSQLEnum(reqInfo.user.access.airps().elems()) << " OR \n";
                sql << "ckin.next_airp(DECODE(pr_tranzit,0,point_id,first_point), point_num) IN \n";
                sql << GetSQLEnum(reqInfo.user.access.airps().elems()) << ") \n";
            }
            else
            {
                sql << "AND (airp NOT IN " << GetSQLEnum(reqInfo.user.access.airps().elems()) << " OR \n";
                sql << "ckin.next_airp(DECODE(pr_tranzit,0,point_id,first_point), point_num) NOT IN \n";
                sql << GetSQLEnum(reqInfo.user.access.airps().elems()) << ") \n";
            };
        };

        //ProgTrace(TRACE5, "FltCBoxDropDown: pass=%d SQL=\n%s", pass, sql.str().c_str());
        Qry.SQLText = sql.str().c_str();
        FltCBoxListRead(Qry, points, count);
        ArxGetFltCBoxList(scr, first_date, last_date, pr_show_del, points, count);
    };
    ProgTrace(TRACE5, "FltCBoxDropDown EXEC QRY: %s", tm.PrintWithMessage().c_str());
    if(count == 0)
        throw AstraLocale::UserException("MSG.FLIGHTS_NOT_FOUND");
    tm.Init();
    sort(points.begin(), points.end(), lessPointsRow);
    ProgTrace(TRACE5, "FltCBoxDropDown SORT: %s", tm.PrintWithMessage().c_str());
};

void TPeriods::dump(TItems::iterator i)
{
    if(i == items.end())
        LogTrace(TRACE5) << "at the end.";
    else
        LogTrace(TRACE5)
            << DateTimeToStr(i->first, ServerFormatDateTimeAsString)
            << "-"
            << DateTimeToStr(i->second, ServerFormatDateTimeAsString);
}

void TPeriods::dump()
{
    for(TItems::iterator i = items.begin(); i != items.end(); i++) dump(i);
}

void TPeriods::get(TDateTime FirstDate, TDateTime LastDate)
{
    items.clear();
    TCachedQuery Qry("select trunc(:FirstDate, 'month'), add_months(trunc(:FirstDate, 'month'), 1) from dual",
            QParams() << QParam("FirstDate", otDate));
    TDateTime tmp_begin = FirstDate;
    while(true) {
        Qry.get().SetVariable("FirstDate", tmp_begin);
        Qry.get().Execute();
        TDateTime begin = Qry.get().FieldAsDateTime(0);
        TDateTime end = Qry.get().FieldAsDateTime(1);
        items.push_back(make_pair(begin, end));
        if(LastDate < end) break;
        tmp_begin = end;
    }
    items.front().first = FirstDate;
    if(items.back().first == LastDate)
        items.pop_back();
    else
        items.back().second = LastDate;
}

int nosir_months(int argc,char **argv)
{
    TDateTime FirstDate, LastDate;
    StrToDateTime("12.04.2014 00:00:00", ServerFormatDateTimeAsString, FirstDate);
    StrToDateTime("13.04.2018 00:00:00", ServerFormatDateTimeAsString, LastDate);
    TPeriods periods;
    periods.get(FirstDate, LastDate);
    periods.dump();
    return 1;
}

void GetMinMaxPartKey(const string &where, TDateTime &min_part_key, TDateTime &max_part_key)
{
  LogTrace5 << __func__ ;
  DB::TQuery Qry(PgOra::getROSession("ARX_POINTS"), STDLOG);
  if(ARX::READ_PG()) {
    Qry.SQLText="SELECT DATE_TRUNC('day',MIN(part_key)) AS min_part_key FROM arx_points";
  } else {
    Qry.SQLText="SELECT TRUNC(MIN(part_key)) AS min_part_key FROM arx_points";
  }
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_part_key"))
    min_part_key=NoExists;
  else
    min_part_key=Qry.FieldAsDateTime("min_part_key");

  Qry.SQLText="SELECT MAX(part_key) AS max_part_key FROM arx_points";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_part_key"))
    max_part_key=NoExists;
  else
    max_part_key=Qry.FieldAsDateTime("max_part_key");

  ProgTrace(TRACE5, "%s: min_part_key=%s max_part_key=%s",
                    where.c_str(),
                    DateTimeToStr(min_part_key, ServerFormatDateTimeAsString).c_str(),
                    DateTimeToStr(max_part_key, ServerFormatDateTimeAsString).c_str());
};

template <class T>
bool EqualCollections(const string &where, const T &c1, const T &c2, const string &err1, const string &err2,
                      pair<typename T::const_iterator, typename T::const_iterator> &diff)
{
  diff.first=c1.end();
  diff.second=c2.end();
  if (!err1.empty() || !err2.empty())
  {
    if (err1!=err2)
    {
      ProgError(STDLOG, "%s: err1=%s err2=%s", where.c_str(), err1.c_str(), err2.c_str());
      return false;
    };
  }
  else
  {
    //ошибок нет
    if (c1.size() != c2.size())
    {
      ProgError(STDLOG, "%s: c1.size()=%zu c2.size()=%zu", where.c_str(), c1.size(), c2.size());
      diff.first=c1.begin();
      diff.second=c2.begin();
      for(;diff.first!=c1.end() && diff.second!=c2.end(); diff.first++,diff.second++)
      {
        if (*(diff.first)==*(diff.second)) continue;
        break;
      };
      return false;
    }
    else
    {
      //размер векторов совпадает
      if (!c1.empty() && !c2.empty())
      {
        diff=mismatch(c1.begin(),c1.end(),c2.begin());
        if (diff.first!=c1.end() || diff.second!=c2.end()) return false;
      };
    };
  };
  return true;
};
