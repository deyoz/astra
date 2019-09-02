#include "stat_reprint.h"
#include "qrys.h"
#include "astra_misc.h"
#include "report_common.h"
#include "stat/stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;

void cleanForeignScan(int days)
{
    TCachedQuery Qry("delete from foreign_scan where time_print <= :time",
            QParams() << QParam("time", otDate, NowUTC() - days));
    Qry.get().Execute();
}

void get_stat_reprint(int point_id)
{
    TCachedQuery delQry("delete from stat_reprint where point_id = :point_id", QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();

    TCachedQuery Qry(
            "select "
            "    points.scd_out, "
            "    confirm_print.desk, "
            "    pax_grp.client_type ckin_type, "
            "    count(*) amount "
            "from "
            "    points, "
            "    pax_grp, "
            "    pax, "
            "    confirm_print "
            "where "
            "    points.point_id = :point_id and "
            "    points.point_id = pax_grp.point_dep and "
            "    pax_grp.grp_id = pax.grp_id and "
            "    pax.pax_id = confirm_print.pax_id and "
            "    confirm_print.from_scan_code <> 0 "
            "group by "
            "    points.scd_out, "
            "    confirm_print.desk, "
            "    pax_grp.client_type ",
            QParams() << QParam("point_id", otInteger, point_id));
    Qry.get().Execute();

    TCachedQuery insQry(
            "insert into stat_reprint ( "
            "   point_id, "
            "   scd_out, "
            "   desk, "
            "   ckin_type, "
            "   amount "
            ") values ( "
            "   :point_id, "
            "   :scd_out, "
            "   :desk, "
            "   :ckin_type, "
            "   :amount) ",
            QParams()
            << QParam("point_id", otInteger, point_id)
            << QParam("scd_out", otDate)
            << QParam("desk", otString)
            << QParam("ckin_type", otString)
            << QParam("amount", otInteger));

    for(; not Qry.get().Eof; Qry.get().Next()) {
        insQry.get().SetVariable("scd_out", Qry.get().FieldAsDateTime("scd_out"));
        insQry.get().SetVariable("desk", Qry.get().FieldAsString("desk"));
        insQry.get().SetVariable("ckin_type", Qry.get().FieldAsString("ckin_type"));
        insQry.get().SetVariable("amount", Qry.get().FieldAsInteger("amount"));
        insQry.get().Execute();
    }
}

void TReprintFullStat::add(const TReprintStatRow &row)
{
    prn_airline.check(row.airline);

    int &amount = (*this)[row.desk][row.view_airline][row.flt][row.scd_out][row.route][row.ckin_type];
    if(amount == 0) FRowCount++;
    amount += row.amount;
    totals += row.amount;
}

void TReprintShortStat::add(const TReprintStatRow &row)
{
    prn_airline.check(row.airline);

    int &amount = (*this)[row.desk][row.view_airline];
    if(amount == 0) FRowCount++;
    amount += row.amount;
    totals += row.amount;
}

void RunReprintStat(
        const TStatParams &params,
        TReprintAbstractStat &ReprintStat,
        bool full
        )
{
    TFltInfoCache flt_cache;
    TDeskAccess desk_access;
    for(int pass = 0; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText = "select stat_reprint.* from ";
        if(pass != 0) {
            SQLText +=
                "   arx_stat_reprint stat_reprint, "
                "   arx_points points ";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                "   stat_reprint, "
                "   points ";
        }
        SQLText +=
            "where "
            "   stat_reprint.point_id = points.point_id and "
            "   points.pr_del >= 0 and ";

        if(not params.ap.empty()) {
            SQLText += " points.airp = :airp and ";
            QryParams << QParam("airp", otString, params.ap);
        }

        if(not params.ak.empty()) {
            SQLText += " points.airline = :airline and ";
            QryParams << QParam("airline", otString, params.ak);
        }

        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(pass != 0)
            SQLText +=
                " points.part_key = stat_reprint.part_key and ";
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        SQLText += "   stat_reprint.scd_out >= :FirstDate AND stat_reprint.scd_out < :LastDate ";
        TCachedQuery Qry(SQLText, QryParams);

        LogTrace(TRACE5) << "reprint SQLText: " << SQLText;
        for(int i = 0; i < Qry.get().VariablesCount(); i++)
            LogTrace(TRACE5) << Qry.get().VariableName(i) << " = " << Qry.get().GetVariableAsString(i);

        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().GetFieldIndex("part_key");
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_desk = Qry.get().FieldIndex("desk");
            int col_ckin_type = Qry.get().FieldIndex("ckin_type");
            int col_amount = Qry.get().FieldIndex("amount");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TReprintStatRow row;
                TDateTime part_key = NoExists;
                if(col_part_key >= 0)
                    part_key = Qry.get().FieldAsDateTime(col_part_key);
                row.desk = Qry.get().FieldAsString(col_desk);

                if(not desk_access.get(row.desk)) continue;

                int point_id = Qry.get().FieldAsInteger(col_point_id);
                const TFltInfoCacheItem &info = flt_cache.get(point_id, part_key);
                row.airline = info.airline;
                row.view_airline = info.view_airline;
                row.flt = info.view_flt_no;
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.route = GetRouteAfterStr(part_key, point_id, trtWithCurrent, trtNotCancelled);
                row.ckin_type = Qry.get().FieldAsString(col_ckin_type);
                row.amount = Qry.get().FieldAsInteger(col_amount);
                ReprintStat.add(row);

                if ((not full) and (ReprintStat.RowCount() > (size_t)MAX_STAT_ROWS()))
                    throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
            }
        }
    }
    // Вытаскиваем репринт сторонних ПТ
    {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        string SQLText = "select * from foreign_scan where ";

        if(not params.ap.empty()) {
            SQLText += " foreign_scan.airp_dep = :airp and ";
            QryParams << QParam("airp", otString, params.ap);
        }

        if(not params.ak.empty()) {
            SQLText += " foreign_scan.airline = :airline and ";
            QryParams << QParam("airline", otString, params.ak);
        }

        if(params.flt_no != NoExists) {
            SQLText += " flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        SQLText += " scd_out >= :FirstDate and scd_out < :LastDate ";
        TCachedQuery Qry(SQLText, QryParams);
        LogTrace(TRACE5) << "SQLText: " << SQLText;
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_desk = Qry.get().GetFieldIndex("desk");
            int col_airline = Qry.get().GetFieldIndex("airline");
            int col_flt_no = Qry.get().GetFieldIndex("flt_no");
            int col_suffix = Qry.get().GetFieldIndex("suffix");
            int col_scd_out = Qry.get().GetFieldIndex("scd_out");
            int col_airp_dep = Qry.get().GetFieldIndex("airp_dep");
            int col_airp_arv = Qry.get().GetFieldIndex("airp_arv");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TReprintStatRow row;
                row.desk = Qry.get().FieldAsString(col_desk);

                if(not desk_access.get(row.desk)) continue;

                row.airline = Qry.get().FieldAsString(col_airline);
                row.view_airline = ElemIdToCodeNative(etAirline, row.airline);
                ostringstream flt_no_str;
                flt_no_str
                    << setw(3) << setfill('0')
                    << Qry.get().FieldAsInteger(col_flt_no)
                    << ElemIdToCodeNative(etSuffix, Qry.get().FieldAsString(col_suffix));
                row.flt = flt_no_str.str();
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                ostringstream route;
                route
                    << ElemIdToCodeNative(etAirp, Qry.get().FieldAsString(col_airp_dep))
                    << "-"
                    << ElemIdToCodeNative(etAirp, Qry.get().FieldAsString(col_airp_arv));
                row.route = route.str();
                row.amount = 1;
                ReprintStat.add(row);

                if ((not full) and (ReprintStat.RowCount() > (size_t)MAX_STAT_ROWS()))
                    throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
            }
        }
    }
}

void createXMLReprintFullStat(
        const TStatParams &params,
        const TReprintFullStat &ReprintFullStat,
        xmlNodePtr resNode)
{
    if(ReprintFullStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", ReprintFullStat.prn_airline.get(), "");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Пульт"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Номер рейса"));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Направление"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип рег."));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Репринт"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(const auto &desk: ReprintFullStat) {
        for(const auto &airline: desk.second) {
            for(const auto &flt: airline.second) {
                for(const auto &scd: flt.second) {
                    for(const auto &route: scd.second) {
                        for(const auto &ckin_type: route.second) {
                            rowNode = NewTextChild(rowsNode, "row");
                            // Пульт
                            NewTextChild(rowNode, "col", desk.first);
                            // АК
                            NewTextChild(rowNode, "col", airline.first);
                            // Рейс
                            NewTextChild(rowNode, "col", flt.first);
                            // Дата вылета
                            NewTextChild(rowNode, "col", DateTimeToStr(scd.first, "dd.mm.yy"));
                            // Направление
                            NewTextChild(rowNode, "col", route.first);
                            // Тип рег.
                            NewTextChild(rowNode, "col", ckin_type.first);
                            // Репринт
                            NewTextChild(rowNode, "col", ckin_type.second);
                        }
                    }
                }
            }
        }
    }

    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", ReprintFullStat.totals);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Репринт"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

void createXMLReprintShortStat(
        const TStatParams &params,
        const TReprintShortStat &ReprintShortStat,
        xmlNodePtr resNode)
{
    if(ReprintShortStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", ReprintShortStat.prn_airline.get(), "");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Пульт"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Репринт"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(const auto &desk: ReprintShortStat) {
        for(const auto &airline: desk.second) {
            rowNode = NewTextChild(rowsNode, "row");
            // Пульт
            NewTextChild(rowNode, "col", desk.first);
            // АК
            NewTextChild(rowNode, "col", airline.first);
            // Репринт
            NewTextChild(rowNode, "col", airline.second);
        }
    }

    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", ReprintShortStat.totals);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Репринт"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Общая"));
}

struct TReprintFullStatCombo : public TOrderStatItem
{
    string desk;
    string airline;
    string flt_no;
    TDateTime scd;
    string route;
    string ckin_type;
    int amount;

    TReprintFullStatCombo(
            const string &_desk,
            const string &_airline,
            const string &_flt_no,
            TDateTime _scd,
            const string &_route,
            const string &_ckin_type,
            int _amount):
        desk(_desk),
        airline(_airline),
        flt_no(_flt_no),
        scd(_scd),
        route(_route),
        ckin_type(_ckin_type),
        amount(_amount)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TReprintFullStatCombo::add_data(ostringstream &buf) const
{
    buf
        << desk << delim
        << airline << delim
        << flt_no << delim
        << DateTimeToStr(scd, "dd.mm.yy") << delim
        << route << delim
        << ckin_type << delim
        << amount << endl;
}

void TReprintFullStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("Пульт") << delim
        << getLocaleText("АК") << delim
        << getLocaleText("Номер рейса") << delim
        << getLocaleText("Дата") << delim
        << getLocaleText("Направление") << delim
        << getLocaleText("Тип рег.") << delim
        << getLocaleText("Репринт") << endl;
}

struct TReprintShortStatCombo : public TOrderStatItem
{
    string desk;
    string airline;
    int amount;

    TReprintShortStatCombo(
            const string &_desk,
            const string &_airline,
            int _amount):
        desk(_desk),
        airline(_airline),
        amount(_amount)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TReprintShortStatCombo::add_data(ostringstream &buf) const
{
    buf
        << desk << delim
        << airline << delim
        << amount << endl;
}

void TReprintShortStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("Пульт") << delim
        << getLocaleText("АК") << delim
        << getLocaleText("Репринт") << endl;
}

void RunReprintFullFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TReprintFullStat ReprintFullStat;
    RunReprintStat(params, ReprintFullStat, true);
    for(const auto &desk: ReprintFullStat) {
        for(const auto &airline: desk.second) {
            for(const auto &flt: airline.second) {
                for(const auto &scd: flt.second) {
                    for(const auto &route: scd.second) {
                        for(const auto &ckin_type: route.second) {
                            writer.insert(TReprintFullStatCombo(
                                        desk.first,
                                        airline.first,
                                        flt.first,
                                        scd.first,
                                        route.first,
                                        ckin_type.first,
                                        ckin_type.second
                                        ));
                        }
                    }
                }
            }
        }
    }
}

void RunReprintShortFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TReprintShortStat ReprintShortStat;
    RunReprintStat(params, ReprintShortStat, true);
    for(const auto &desk: ReprintShortStat) {
        for(const auto &airline: desk.second) {
            writer.insert(TReprintShortStatCombo(
                        desk.first,
                        airline.first,
                        airline.second
                        ));
        }
    }
}
