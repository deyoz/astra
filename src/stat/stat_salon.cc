#include "stat_salon.h"
#include "astra_misc.h"
#include "qrys.h"
#include "report_common.h"
#include "stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;

void createXMLSalonStat(const TStatParams &params, const TSalonStat &SalonStat, xmlNodePtr resNode)
{
    if(SalonStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата вылета"));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Агент"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время операции"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDateTime);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Операция"));
    SetProp(colNode, "width", 200);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    ostringstream buf;
    for(const auto &row: SalonStat) {
        rowNode = NewTextChild(rowsNode, "row");
        // Рейс
        ostringstream buf;
        buf
            << ElemIdToCodeNative(etAirline, row.airline)
            << setw(3) << setfill('0') << row.flt_no << ElemIdToCodeNative(etSuffix, row.suffix);
        NewTextChild(rowNode, "col", buf.str());
        // Дата вылета
        NewTextChild(rowNode, "col", DateTimeToStr(row.scd_out, "dd.mm.yy"));
        // Агент
        NewTextChild(rowNode, "col", row.login);
        // Время операции
        NewTextChild(rowNode, "col", DateTimeToStr(row.time, "dd.mm.yy hh:nn"));
        // Операция
        NewTextChild(rowNode, "col", row.msg);
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Изменения салона"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

void get_points(const TStatParams &params, list<pair<TDateTime, TTripInfo>> &points)
{
    points.clear();
    for(int pass = 0; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText = "select * from ";
        if(pass != 0) {
            SQLText += " arx_points points ";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else
            SQLText += " points ";
        SQLText +=
            "where "
            "   pr_del >= 0 and ";
        params.AccessClause(SQLText);
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        SQLText +=
            "   points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";

        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();

        if(not Qry.get().Eof) {
            int col_part_key = NoExists;
            if(pass != 0)
                col_part_key = Qry.get().FieldIndex("part_key");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                points.push_back(
                        make_pair(
                            (pass != 0 ? Qry.get().FieldAsDateTime(col_part_key) : NoExists),
                            TTripInfo(Qry.get())
                            )
                        );
            }
        }
    }
}

void RunSalonStat(
        const TStatParams &params,
        TSalonStat &SalonStat
        )
{
    list<pair<TDateTime, TTripInfo>> points;
    get_points(params, points);
    boost::optional<TCachedQuery> operQry;
    boost::optional<TCachedQuery> arxQry;
    for(const auto &point: points) {
        auto &Qry = (point.first == NoExists ? operQry : arxQry);
        if(not Qry) {
            QParams QryParams;
            QryParams
                << QParam("lang", otString, TReqInfo::Instance()->desk.lang)
                << QParam("type", otString, EncodeEventType(evtFlt))
                << QParam("point_id", otInteger);
            string SQLText = "select * from ";
            if(point.first != NoExists)
                SQLText += "arx_events ";
            else
                SQLText += "events_bilingual ";
            SQLText += "where ";
            if(point.first != NoExists) {
                SQLText += "part_key = :part_key and ";
                QryParams << QParam("part_key", otDate);
            }
            if(not params.salon_op_type.empty()) {
                SQLText += " sub_type = :op_type and \n";
                QryParams << QParam("op_type", otString, params.salon_op_type);
            } else
                SQLText += " sub_type is not null and \n";
            SQLText +=
                " lang = :lang and "
                " type = :type and "
                " id1 = :point_id ";
            Qry = boost::in_place(SQLText, QryParams);
        }

        Qry->get().SetVariable("point_id", point.second.point_id);
        if(point.first != NoExists)
            Qry->get().SetVariable("part_key", point.first);

        Qry->get().Execute();
        if(not Qry->get().Eof) {
            int col_ev_order = Qry->get().FieldIndex("ev_order");
            int col_time = Qry->get().FieldIndex("time");
            int col_login = Qry->get().FieldIndex("ev_user");
            int col_op_type = Qry->get().FieldIndex("sub_type");
            int col_msg = Qry->get().FieldIndex("msg");
            for(; not Qry->get().Eof; Qry->get().Next()) {
                TSalonStatRow row;
                row.point_id = point.second.point_id;
                row.ev_order = Qry->get().FieldAsInteger(col_ev_order);
                row.scd_out = point.second.scd_out;
                row.time = Qry->get().FieldAsDateTime(col_time);
                row.login = Qry->get().FieldAsString(col_login);
                row.op_type = Qry->get().FieldAsString(col_op_type);
                try {
                    row.op_type = SalonOpTypes().encode(row.op_type);
                } catch(EConvertError &) {
                }
                row.msg = Qry->get().FieldAsString(col_msg);
                row.airline = point.second.airline;
                row.flt_no = point.second.flt_no;
                row.suffix = point.second.suffix;
                SalonStat.insert(row);
                params.overflow.check(SalonStat.size());
            }
        }
    }
}

const TSalonOpTypes &SalonOpTypes()
{
    static TSalonOpTypes opTypes;
    return opTypes;
}

struct TSalonStatCombo: public TOrderStatItem {
    const TSalonStatRow &row;
    TSalonStatCombo(const TSalonStatRow &row): row(row) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TSalonStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("Рейс") << delim
        << getLocaleText("Дата вылета") << delim
        << getLocaleText("Агент") << delim
        << getLocaleText("Время операции") << delim
        << getLocaleText("Операция") << endl;
}

void TSalonStatCombo::add_data(ostringstream &buf) const
{
    ostringstream flt;
    flt
        << ElemIdToCodeNative(etAirline, row.airline)
        << setw(3) << setfill('0') << row.flt_no << ElemIdToCodeNative(etSuffix, row.suffix);

    buf
        << flt.str() << delim
        << DateTimeToStr(row.scd_out, "dd.mm.yy") << delim
        << row.login << delim
        << DateTimeToStr(row.time, "dd.mm.yy hh:nn") << delim
        << row.msg << endl;
}

void RunSalonStatFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TSalonStat stat;
    RunSalonStat(params, stat);
    for(const auto &row: stat) writer.insert(TSalonStatCombo(row));
}
