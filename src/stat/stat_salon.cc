#include "stat_salon.h"
#include "astra_misc.h"
#include "qrys.h"
#include "report_common.h"
#include "stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

template void RunSalonStat(TStatParams const&, TOrderStatWriter&, TPrintAirline&);
template void RunSalonStat(TStatParams const&, TSalonStat&, TPrintAirline&);

using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;

string strip_op_type(const string &op_type)
{
    string result = op_type;
    string prefix = result.substr(0, 4);
    if(
            prefix == "DEL_" or
            prefix == "ADD_"
      )
        result.erase(0, 4);

    for(const auto i: TSalonOpType::pairs()) {
        if(result.size() != i.first.size() and result.substr(0, i.first.size()) == i.first)
            result = i.first;
    }
    return result;
}

void to_stat_salon(int point_id, const PrmEnum &msg, const string &op_type)
{
    TTripInfo flt;
    flt.getByPointId(point_id);
    TCachedQuery Qry(
            "insert into stat_salon ( "
            "   scd_out, "
            "   point_id, "
            "   time, "
            "   login, "
            "   op_type, "
            "   msg, "
            "   lang "
            ") values ( "
            "   :scd_out, "
            "   :point_id, "
            "   :time, "
            "   :login, "
            "   :op_type, "
            "   :msg, "
            "   :lang "
            ") ",
            QParams()
            << QParam("scd_out", otDate, flt.scd_out)
            << QParam("point_id", otInteger, point_id)
            << QParam("time", otDate, NowUTC())
            << QParam("login", otString, TReqInfo::Instance()->user.login)
            << QParam("op_type", otString, strip_op_type(op_type))
            << QParam("msg", otString)
            << QParam("lang", otString));
    for(int i = 0; i < 2; i++) {
        string lang = (i ? LANG_RU : LANG_EN);
        Qry.get().SetVariable("msg", msg.GetMsg(lang).substr(0, 250));
        Qry.get().SetVariable("lang", lang);
        Qry.get().Execute();
    }
}

void createXMLSalonStat(const TStatParams &params, const TSalonStat &SalonStat, const TPrintAirline &prn_airline, xmlNodePtr resNode)
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
    colNode = NewTextChild(headerNode, "col", "LOGIN");
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время операции"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDateTime);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип операции"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
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
        // LOGIN
        NewTextChild(rowNode, "col", row.login);
        // Время операции
        NewTextChild(rowNode, "col", DateTimeToStr(row.time, "dd.mm.yy hh:nn"));
        // Тип операции
        NewTextChild(rowNode, "col", getLocaleText(row.op_type));
        // Операция
        NewTextChild(rowNode, "col", row.msg);
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Изменения салона"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

template <class T>
void RunSalonStat(
        const TStatParams &params,
        T &SalonStat,
        TPrintAirline &prn_airline
        )
{
    for(int pass = 0; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate)
            << QParam("lang", otString, TReqInfo::Instance()->desk.lang);
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText =
            "select "
            "   stat_salon.*, "
            "   points.point_id, "
            "   points.airline, "
            "   points.flt_no, "
            "   points.suffix "
            "from ";
        if(pass != 0) {
            SQLText +=
                "   arx_stat_salon stat_salon, "
                "   arx_points points ";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                "   stat_salon, "
                "   points ";
        }
        SQLText +=
            "where "
            "   stat_salon.point_id = points.point_id and "
            "   points.pr_del >= 0 and ";
        params.AccessClause(SQLText);
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(pass != 0)
            SQLText +=
                " points.part_key = stat_salon.part_key and ";
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        if(not params.salon_op_type.empty()) {
            SQLText += " stat_salon.op_type = :op_type and \n";
            QryParams << QParam("op_type", otString, params.salon_op_type);
        }
        SQLText +=
            "   stat_salon.scd_out >= :FirstDate AND stat_salon.scd_out < :LastDate and "
            "   stat_salon.lang = :lang ";

        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_time = Qry.get().FieldIndex("time");
            int col_login = Qry.get().FieldIndex("login");
            int col_op_type = Qry.get().FieldIndex("op_type");
            int col_msg = Qry.get().FieldIndex("msg");
            int col_airline = Qry.get().FieldIndex("airline");
            int col_flt_no = Qry.get().FieldIndex("flt_no");
            int col_suffix = Qry.get().FieldIndex("suffix");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TSalonStatRow row;
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.time = Qry.get().FieldAsDateTime(col_time);
                row.login = Qry.get().FieldAsString(col_login);
                row.op_type = Qry.get().FieldAsString(col_op_type);
                try {
                    row.op_type = SalonOpTypes().encode(row.op_type);
                } catch(EConvertError &) {
                }
                row.msg = Qry.get().FieldAsString(col_msg);
                row.airline = Qry.get().FieldAsString(col_airline);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                SalonStat.insert(row);
                params.overflow.check(SalonStat.size());
            }
        }
    }
}

void TSalonStatRow::add_header(ostringstream &buf) const
{
}

void TSalonStatRow::add_data(ostringstream &buf) const
{
}

const TSalonOpTypes &SalonOpTypes()
{
    static TSalonOpTypes opTypes;
    return opTypes;
}

