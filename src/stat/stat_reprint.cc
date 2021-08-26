#include "stat_reprint.h"
#include "qrys.h"
#include "astra_misc.h"
#include "report_common.h"
#include "passenger.h"
#include "stat/stat_utils.h"
#include "cache_access.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;

void cleanForeignScan(int days)
{
    DB::TCachedQuery Qry(PgOra::getRWSession("FOREIGN_SCAN"),
                         "delete from foreign_scan where time_print <= :time",
                         QParams() << QParam("time", otDate, NowUTC() - days),
                         STDLOG);
    Qry.get().Execute();
}

namespace {

struct ReprintMapKey
{
    TDateTime scd_out;
    std::string desk;
    std::string ckin_type;

    bool operator <(const ReprintMapKey& key) const;
};

bool ReprintMapKey::operator <(const ReprintMapKey& key) const
{
    if (scd_out != key.scd_out) {
        return scd_out < key.scd_out;
    }
    if (desk != key.desk) {
        return desk < key.desk;
    }
    return ckin_type < key.ckin_type;
}

} // namespace

void get_stat_reprint(int point_id)
{
    DB::TCachedQuery delQry(
        PgOra::getRWSession("STAT_REPRINT"),
        "DELETE FROM stat_reprint WHERE point_id = :point_id",
        QParams() << QParam("point_id", otInteger, point_id),
        STDLOG);
    delQry.get().Execute();

    std::map<ReprintMapKey,int> reprint_map;
    TTripInfo flt;
    flt.getByPointId(point_id);
    const std::list<CheckIn::TSimplePaxGrpItem> groups
        = CheckIn::TSimplePaxGrpItem::getByDepPointId(PointId_t(point_id));
    for (const CheckIn::TSimplePaxGrpItem& grp: groups) {
        DB::TCachedQuery Qry(
              PgOra::getROSession("CONFIRM_PRINT"),
              "SELECT desk "
              "FROM confirm_print "
              "WHERE grp_id = :grp_id "
              "AND from_scan_code <> 0 ",
              QParams() << QParam("grp_id", otInteger, grp.id),
              STDLOG);
        Qry.get().Execute();
        for (; not Qry.get().Eof; Qry.get().Next()) {
            const ReprintMapKey key = {
              flt.scd_out,
              Qry.get().FieldAsString("desk"),
              EncodeClientType(grp.client_type)
            };
            auto res = reprint_map.emplace(key, 1);
            const bool reprint_exists = !res.second;
            if (reprint_exists) {
              int& amount = res.first->second;
              amount += 1;
            }
        }

        DB::TCachedQuery insQry(
              PgOra::getRWSession("STAT_REPRINT"),
              "INSERT INTO stat_reprint ( "
              "    point_id, "
              "    scd_out, "
              "    desk, "
              "    ckin_type, "
              "    amount "
              ") VALUES ( "
              "    :point_id, "
              "    :scd_out, "
              "    :desk, "
              "    :ckin_type, "
              "    :amount) ",
              QParams()
              << QParam("point_id", otInteger, point_id)
              << QParam("scd_out", otDate)
              << QParam("desk", otString)
              << QParam("ckin_type", otString)
              << QParam("amount", otInteger),
              STDLOG);

        for (const auto& item: reprint_map) {
            const ReprintMapKey& key = item.first;
            const int amount = item.second;
            insQry.get().SetVariable("scd_out", key.scd_out);
            insQry.get().SetVariable("desk", key.desk);
            insQry.get().SetVariable("ckin_type", key.ckin_type);
            insQry.get().SetVariable("amount", amount);
            insQry.get().Execute();
        }
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

void ArxRunReprintStat(
        const TStatParams &params,
        TReprintAbstractStat &ReprintStat
        )
{
    LogTrace5 << __func__;
    TFltInfoCache flt_cache;
    ViewAccess<DeskCode_t> deskViewAccess;
    for(int pass = 1; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
            QryParams << QParam("arx_trip_date_range", otDate, params.LastDate+ARX_TRIP_DATE_RANGE());
        string SQLText = "select arx_stat_reprint.* from ";
        SQLText +=
            "   arx_stat_reprint , "
            "   arx_points  ";
        if(pass == 2) {
            SQLText += getMoveArxQuery();
        }
        SQLText +=
            "where "
            "   arx_stat_reprint.point_id = arx_points.point_id and "
            "   arx_points.pr_del >= 0 and ";

        if(not params.ap.empty()) {
            SQLText += " arx_points.airp = :airp and ";
            QryParams << QParam("airp", otString, params.ap);
        }

        if(not params.ak.empty()) {
            SQLText += " arx_points.airline = :airline and ";
            QryParams << QParam("airline", otString, params.ak);
        }

        if(params.flt_no != NoExists) {
            SQLText += " arx_points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        SQLText += " arx_points.part_key = arx_stat_reprint.part_key and ";
        if(pass == 1)
            SQLText += " arx_points.part_key >= :FirstDate AND arx_points.part_key < :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " arx_points.part_key = arx_ext.part_key AND arx_points.move_id = arx_ext.move_id AND \n";
        SQLText += "   arx_stat_reprint.scd_out >= :FirstDate AND arx_stat_reprint.scd_out < :LastDate ";
        DB::TCachedQuery Qry(PgOra::getROSession("ARX_POINTS"), SQLText, QryParams, STDLOG);

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

                if(not deskViewAccess.check(DeskCode_t(row.desk))) continue;

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
                params.overflow.check(ReprintStat.RowCount());
            }
        }
    }
}

void RunReprintStat(
        const TStatParams &params,
        TReprintAbstractStat &ReprintStat
        )
{
    TFltInfoCache flt_cache;
    ViewAccess<DeskCode_t> deskViewAccess;
    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, params.FirstDate)
        << QParam("LastDate", otDate, params.LastDate);

    DB::TCachedQuery Qry(
          PgOra::getROSession("STAT_REPRINT"),
          "SELECT * FROM stat_reprint "
          "WHERE scd_out >= :FirstDate "
          "AND scd_out < :LastDate ",
          QryParams,
          STDLOG);

    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int col_part_key = Qry.get().GetFieldIndex("part_key");
        int col_point_id = Qry.get().FieldIndex("point_id");
        int col_scd_out = Qry.get().FieldIndex("scd_out");
        int col_desk = Qry.get().FieldIndex("desk");
        int col_ckin_type = Qry.get().FieldIndex("ckin_type");
        int col_amount = Qry.get().FieldIndex("amount");
        for(; not Qry.get().Eof; Qry.get().Next()) {
            int point_id = Qry.get().FieldAsInteger(col_point_id);
            TTripInfo flt;
            flt.getByPointId(point_id);
            if (flt.pr_del >= 0) {
                continue;
            }
            if(not params.ap.empty() && flt.airp != params.ap) {
                continue;
            }

            if(not params.ak.empty() && flt.airline != params.ak) {
                continue;
            }

            if(params.flt_no != NoExists && flt.flt_no != params.flt_no) {
                continue;
            }

            TReprintStatRow row;
            TDateTime part_key = NoExists;
            if(col_part_key >= 0)
                part_key = Qry.get().FieldAsDateTime(col_part_key);
            row.desk = Qry.get().FieldAsString(col_desk);

            if(not deskViewAccess.check(DeskCode_t(row.desk))) continue;

            const TFltInfoCacheItem &info = flt_cache.get(point_id, part_key);
            row.airline = info.airline;
            row.view_airline = info.view_airline;
            row.flt = info.view_flt_no;
            row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
            row.route = GetRouteAfterStr(part_key, point_id, trtWithCurrent, trtNotCancelled);
            row.ckin_type = Qry.get().FieldAsString(col_ckin_type);
            row.amount = Qry.get().FieldAsInteger(col_amount);
            ReprintStat.add(row);
            params.overflow.check(ReprintStat.RowCount());
        }
    }

    ArxRunReprintStat(params, ReprintStat);

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
        DB::TCachedQuery Qry(PgOra::getROSession("FOREIGN_SCAN"), SQLText, QryParams, STDLOG);
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

                if(not deskViewAccess.check(DeskCode_t(row.desk))) continue;

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
                params.overflow.check(ReprintStat.RowCount());
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
    RunReprintStat(params, ReprintFullStat);
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
    RunReprintStat(params, ReprintShortStat);
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
