#include "stat_limited_capab.h"
#include "qrys.h"
#include "report_common.h"
#include "stat_utils.h"
#include "points.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;

void RunLimitedCapabStat(
        const TStatParams &params,
        TLimitedCapabStat &LimitedCapabStat,
        TPrintAirline &prn_airline
        )
{
    for(int pass = 0; pass <= 1; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        string SQLText =
            "select "
            "   points.point_id, "
            "   points.airline, "
            "   points.airp, "
            "   stat.airp_arv, "
            "   points.flt_no, "
            "   points.suffix, "
            "   points.scd_out, "
            "   stat.rem_code, "
            "   stat.pax_amount "
            "from ";
        if(pass != 0) {
            SQLText +=
                "   arx_limited_capability_stat stat, "
                "   arx_points points ";
        } else {
            SQLText +=
                "   limited_capability_stat stat, "
                "   points ";
        }
        SQLText +=
            "where "
            "   stat.point_id = points.point_id and ";
        params.AccessClause(SQLText);
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(pass != 0)
            SQLText +=
                " points.part_key >= :FirstDate and points.part_key < :FirstDate and "
                " stat.part_key >= :FirstDate and stat.part_key < :LastDate ";
        else
            SQLText +=
                "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_airline = Qry.get().FieldIndex("airline");
            int col_airp = Qry.get().FieldIndex("airp");
            int col_airp_arv = Qry.get().FieldIndex("airp_arv");
            int col_flt_no = Qry.get().FieldIndex("flt_no");
            int col_suffix = Qry.get().FieldIndex("suffix");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_rem_code = Qry.get().FieldIndex("rem_code");
            int col_pax_amount = Qry.get().FieldIndex("pax_amount");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                prn_airline.check(Qry.get().FieldAsString(col_airline));
                TFlight row;
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.airline = Qry.get().FieldAsString(col_airline);
                row.airp = Qry.get().FieldAsString(col_airp);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);

                string airp_arv = Qry.get().FieldAsString(col_airp_arv);
                string rem_code = Qry.get().FieldAsString(col_rem_code);
                int pax_amount = Qry.get().FieldAsInteger(col_pax_amount);


                LimitedCapabStat.total[rem_code] += pax_amount;
                LimitedCapabStat.rows[row][airp_arv][rem_code] = pax_amount;
            }
        }
    }
}

void createXMLLimitedCapabStat(const TStatParams &params, const TLimitedCapabStat &LimitedCapabStat, const TPrintAirline &prn_airline, xmlNodePtr resNode)
{
    if(LimitedCapabStat.rows.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    if (LimitedCapabStat.rows.size() >= (size_t)MAX_STAT_ROWS())
        throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
    NewTextChild(resNode, "airline", prn_airline.get(), "");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    for(TLimitedCapabStat::TRems::const_iterator rem_col = LimitedCapabStat.total.begin();
            rem_col != LimitedCapabStat.total.end(); rem_col++)
    {
        colNode = NewTextChild(headerNode, "col", rem_col->first);
        SetProp(colNode, "width", 40);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
    }

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TLimitedCapabStat::TRows::const_iterator row = LimitedCapabStat.rows.begin();
            row != LimitedCapabStat.rows.end(); row++)
    {
        for(TLimitedCapabStat::TAirpArv::const_iterator airp = row->second.begin();
                airp != row->second.end(); airp++)
        {
            rowNode = NewTextChild(rowsNode, "row");
            // АК
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, row->first.airline));
            // АП
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, row->first.airp));
            // Рейс
            ostringstream buf;
            buf << setw(3) << setfill('0') << row->first.flt_no << ElemIdToCodeNative(etSuffix, row->first.suffix);
            NewTextChild(rowNode, "col", buf.str());
            // Дата
            NewTextChild(rowNode, "col", DateTimeToStr(row->first.scd_out, "dd.mm.yyyy"));
            // До
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, airp->first));

            const TLimitedCapabStat::TRems &rems = airp->second;

            for(TLimitedCapabStat::TRems::const_iterator rem_col = LimitedCapabStat.total.begin();
                    rem_col != LimitedCapabStat.total.end(); rem_col++)
            {
                TLimitedCapabStat::TRems::const_iterator rems_idx = rems.find(rem_col->first);
                int pax_count = 0;
                if(rems_idx != rems.end()) pax_count = rems_idx->second;
                NewTextChild(rowNode, "col", pax_count);
            }
        }
    }
    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    for(TLimitedCapabStat::TRems::const_iterator rem_col = LimitedCapabStat.total.begin();
            rem_col != LimitedCapabStat.total.end(); rem_col++)
        NewTextChild(rowNode, "col", rem_col->second);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "rem_col_num", (int)LimitedCapabStat.total.size());
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Пассажиры с ограниченными возможностями"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

struct TLimitedCapabStatCombo : public TOrderStatItem
{
    typedef std::pair<TFlight, TLimitedCapabStat::TAirpArv> Tdata;
    Tdata data;
    TLimitedCapabStat::TRems total;
    TLimitedCapabStatCombo(const Tdata &aData, TLimitedCapabStat::TRems &aTotal)
        : data(aData), total(aTotal) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TLimitedCapabStatCombo::add_header(ostringstream &buf) const
{
    buf << "АК" << delim;
    buf << "АП" << delim;
    buf << "Рейс" << delim;
    buf << "Дата" << delim;
    buf << "До" << delim;
    for (TLimitedCapabStat::TRems::const_iterator rem_col = total.begin();;)
    {
        if (rem_col != total.end()) buf << rem_col->first;
        else { buf << endl; break; }
        if (++rem_col != total.end()) buf << delim;
        else { buf << endl; break; }
    }
}

void TLimitedCapabStatCombo::add_data(ostringstream &buf) const
{
    for(TLimitedCapabStat::TAirpArv::const_iterator airp = data.second.begin();
                airp != data.second.end(); airp++)
    {
        // АК
        buf << ElemIdToCodeNative(etAirline, data.first.airline) << delim;
        // АП
        buf << ElemIdToCodeNative(etAirp, data.first.airp) << delim;
        // Рейс
        ostringstream oss1;
        oss1 << setw(3) << setfill('0') << data.first.flt_no << ElemIdToCodeNative(etSuffix, data.first.suffix);
        buf << oss1.str() << delim;
        // Дата
        buf << DateTimeToStr(data.first.scd_out, "dd.mm.yyyy") << delim;
        // До
        buf << ElemIdToCodeNative(etAirp, airp->first) << delim;

        const TLimitedCapabStat::TRems &rems = airp->second;
        for(TLimitedCapabStat::TRems::const_iterator rem_col = total.begin(); rem_col != total.end();)
        {
            TLimitedCapabStat::TRems::const_iterator rems_idx = rems.find(rem_col->first);
            int pax_count = 0;
            if (rems_idx != rems.end()) pax_count = rems_idx->second;
            buf << pax_count;
            if (++rem_col != total.end()) buf << delim;
        }
        buf << endl;
    }
}

void RunLimitedCapabStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline)
{
    TLimitedCapabStat LimitedCapabStat;
    RunLimitedCapabStat(params, LimitedCapabStat, prn_airline);
    for(TLimitedCapabStat::TRows::const_iterator row = LimitedCapabStat.rows.begin();
            row != LimitedCapabStat.rows.end(); row++)
        writer.insert(TLimitedCapabStatCombo(*row, LimitedCapabStat.total));
}

void nosir_lim_capab_stat_point(int point_id)
{
    TFlights flightsForLock;
    flightsForLock.Get( point_id, ftTranzit );
    flightsForLock.Lock(__FUNCTION__);

    TQuery Qry(&OraSession);
    Qry.SQLText = "SELECT count(*) from points where point_id=:point_id AND pr_del=0";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if (Qry.Eof || Qry.FieldAsInteger(0) == 0)
    {
        OraSession.Rollback();
        return;
    }

    bool pr_stat = false;
    Qry.SQLText = "SELECT pr_stat FROM trip_sets WHERE point_id=:point_id";
    Qry.Execute();
    if(not Qry.Eof) pr_stat = Qry.FieldAsInteger(0) != 0;

    int count = 0;
    Qry.SQLText = "select count(*) from limited_capability_stat where point_id=:point_id";
    Qry.Execute();
    if(not Qry.Eof) count = Qry.FieldAsInteger(0);

    if(pr_stat and count == 0)
        get_limited_capability_stat(point_id);

    OraSession.Commit();
}

int nosir_lim_capab_stat(int argc,char **argv)
{
    cout << "start time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    list<int> point_ids;
    TQuery Qry(&OraSession);
    Qry.SQLText = "select point_id from trip_sets";
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) point_ids.push_back(Qry.FieldAsInteger(0));
    OraSession.Rollback();
    cout << point_ids.size() << " points to process." << endl;
    int count = 0;
    for(list<int>::iterator i = point_ids.begin(); i != point_ids.end(); i++, count++) {
        nosir_lim_capab_stat_point(*i);
        if(not (count % 1000))
            cout << count << endl;
    }
    cout << "end time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    return 0;
}

void get_limited_capability_stat(int point_id)
{
    TCachedQuery delQry("delete from limited_capability_stat where point_id = :point_id", QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();

    TRemGrp rem_grp;
    rem_grp.Load(retLIMITED_CAPAB_STAT, point_id);
    if(rem_grp.empty()) return;

    TCachedQuery Qry(
            "select pax.grp_id, pax_grp.airp_arv, rem_code "
            "from pax_grp, pax, pax_rem where "
            "   pax_grp.point_dep = :point_id and "
            "   pax.grp_id = pax_grp.grp_id and "
            "   pax_rem.pax_id = pax.pax_id ",
            QParams() << QParam("point_id", otInteger, point_id)
            );
    Qry.get().Execute();
    if(not Qry.get().Eof) {

        TAirpArvInfo airp_arv_info;

        map<string, map<string, int> > rems;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            string airp_arv = airp_arv_info.get(Qry.get());
            string rem = Qry.get().FieldAsString("rem_code");
            if(rem_grp.exists(rem)) rems[airp_arv][rem]++;
        }

        if(not rems.empty()) {
            TCachedQuery insQry(
                    "insert into limited_capability_stat ( "
                    "   point_id, "
                    "   airp_arv, "
                    "   rem_code, "
                    "   pax_amount "
                    ") values ( "
                    "   :point_id, "
                    "   :airp_arv, "
                    "   :rem_code, "
                    "   :pax_amount "
                    ") ",
                    QParams()
                    << QParam("point_id", otInteger, point_id)
                    << QParam("airp_arv", otString)
                    << QParam("rem_code", otString)
                    << QParam("pax_amount", otInteger)
                    );
            for(map<string, map<string, int> >::iterator airp_arv = rems.begin(); airp_arv != rems.end(); airp_arv++) {
                for(map<string, int>::iterator rem = airp_arv->second.begin(); rem != airp_arv->second.end(); rem++) {
                    insQry.get().SetVariable("airp_arv", airp_arv->first);
                    insQry.get().SetVariable("rem_code", rem->first);
                    insQry.get().SetVariable("pax_amount", rem->second);
                    insQry.get().Execute();
                }
            }
        }
    }
}

