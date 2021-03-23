#include "stat_unacc.h"
#include "qrys.h"
#include "report_common.h"
#include "astra_misc.h"
#include "stat/stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;

bool TUNACCStatRow::operator < (const TUNACCStatRow &_row) const
{
    if(point_id == _row.point_id)
        if(grp_id == _row.grp_id)
            if(num == _row.num)
                if(no == _row.no)
                    return time_create < _row.time_create;
                else
                    return no < _row.no;
            else
                return num < _row.num;
        else
            return grp_id < _row.grp_id;
    else
        return point_id < _row.point_id;
}

void TUNACCFullStat::add(const TUNACCStatRow &row)
{
    prn_airline.check(row.airline);
    insert(row);
    FRowCount++;
}

void ArxRunUNACCStat(
        const TStatParams &params,
        TUNACCAbstractStat &UNACCFullStat
        )
{
    tst();
    for(int pass = 1; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        QryParams << QParam("arx_trip_date_range", otDate, params.LastDate+ARX_TRIP_DATE_RANGE());
        string SQLText =
            "select \n"
            "   arx_points.point_id, \n"
            "   arx_points.craft, \n"
            "   arx_points.airline, \n"
            "   arx_points.flt_no, \n"
            "   arx_points.suffix, \n"
            "   arx_points.scd_out, \n"
            "   arx_points.airp, \n"
            "   arx_pax_grp.airp_arv, \n"
            "   arx_unaccomp.original_tag_no, \n"
            "   arx_unaccomp.surname, \n"
            "   arx_unaccomp.name, \n"
            "   arx_unaccomp.airline prev_airline, \n"
            "   arx_unaccomp.flt_no prev_flt_no, \n"
            "   arx_unaccomp.suffix prev_suffix, \n"
            "   arx_unaccomp.scd prev_scd, \n"
            "   arx_pax_grp.grp_id, \n"
            "   arx_pax_grp.desk, \n"
            "   arx_pax_grp.time_create, \n"
            "   arx_pax_grp.user_id, \n" //for join with users2
            "   arx_bag2.bag_type, \n"
            "   arx_bag2.num, \n"
            "   arx_bag2.amount, \n"
            "   arx_bag2.weight, \n"
            "   arx_bag_tags.no, \n"
            "   arx_transfer.airline trfer_airline, \n"
            "   arx_transfer.airp_dep trfer_airp_dep, \n"
            "   arx_transfer.flt_no trfer_flt_no, \n"
            "   arx_transfer.suffix trfer_suffix, \n"
            "   arx_transfer.scd trfer_scd, \n"
            "   arx_transfer.airp_arv trfer_airp_arv \n"
            "from \n"
            "   arx_points , \n"
            "   arx_bag_tags , \n"
            "   arx_pax_grp "
            "       LEFT JOIN arx_transfer ON arx_pax_grp.part_key = arx_transfer.part_key AND "
            "                                 arx_pax_grp.grp_id   = arx_transfer.grp_id AND"
            "                                 arx_transfer.pr_final <> 0,  \n"
            "   arx_unaccomp_bag_info arx_unaccomp "
            "       RIGHT JOIN arx_bag2 ON arx_unaccomp.part_key = arx_bag2.part_key AND "
            "                              arx_unaccomp.grp_id   = arx_bag2.grp_id AND "
            "                              arx_unaccomp.num      = arx_bag2.num \n";
        if(pass == 2) {
            SQLText += getMoveArxQuery();
        }
        SQLText +=
            "where \n"
            "   arx_bag2.part_key    = arx_bag_tags.part_key and \n"
            "   arx_bag2.part_key    = arx_pax_grp.part_key and \n"
            "   arx_bag2.grp_id      = arx_bag_tags.grp_id and \n"
            "   arx_bag2.num         = arx_bag_tags.bag_num and \n"
            "   arx_pax_grp.part_key = arx_points.part_key and \n"
            "   arx_points.scd_out  >= :FirstDate AND arx_points.scd_out < :LastDate and \n";
        if(pass == 1)
            SQLText += " arx_points.part_key >= :FirstDate AND arx_points.part_key < :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " arx_points.part_key=arx_ext.part_key AND arx_points.move_id=arx_ext.move_id AND \n";

        params.AccessClause(SQLText, "arx_points");
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and \n";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        SQLText +=
            "   arx_pax_grp.point_dep = arx_points.point_id and \n"
            "   arx_pax_grp.class is null and \n"
            "   arx_bag2.grp_id = arx_pax_grp.grp_id \n";

        UsersReader::Instance().updateUsers();

        DB::TCachedQuery Qry(PgOra::getROSession("ARX_UNACCOMP_BAG_INFO"), SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            TAirpArvInfo airp_arv_info;
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_craft = Qry.get().FieldIndex("craft");
            int col_airline = Qry.get().FieldIndex("airline");
            int col_flt_no = Qry.get().FieldIndex("flt_no");
            int col_suffix = Qry.get().FieldIndex("suffix");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_airp = Qry.get().FieldIndex("airp");
            int col_original_tag_no = Qry.get().FieldIndex("original_tag_no");
            int col_surname = Qry.get().FieldIndex("surname");
            int col_name = Qry.get().FieldIndex("name");
            int col_prev_airline = Qry.get().FieldIndex("prev_airline");
            int col_prev_flt_no = Qry.get().FieldIndex("prev_flt_no");
            int col_prev_suffix = Qry.get().FieldIndex("prev_suffix");
            int col_prev_scd = Qry.get().FieldIndex("prev_scd");
            int col_grp_id = Qry.get().FieldIndex("grp_id");
            int col_desk = Qry.get().FieldIndex("desk");
            int col_time_create = Qry.get().FieldIndex("time_create");
            int col_user_id = Qry.get().FieldIndex("user_id");
            int col_bag_type = Qry.get().FieldIndex("bag_type");
            int col_num = Qry.get().FieldIndex("num");
            int col_amount = Qry.get().FieldIndex("amount");
            int col_weight = Qry.get().FieldIndex("weight");
            int col_no = Qry.get().FieldIndex("no");

            int col_trfer_airline = Qry.get().FieldIndex("trfer_airline");
            int col_trfer_airp_dep = Qry.get().FieldIndex("trfer_airp_dep");
            int col_trfer_flt_no = Qry.get().FieldIndex("trfer_flt_no");
            int col_trfer_suffix = Qry.get().FieldIndex("trfer_suffix");
            int col_trfer_scd = Qry.get().FieldIndex("trfer_scd");
            int col_trfer_airp_arv = Qry.get().FieldIndex("trfer_airp_arv");

            for(; not Qry.get().Eof; Qry.get().Next()) {
                TUNACCStatRow row;
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.craft = Qry.get().FieldAsString(col_craft);
                row.airline = Qry.get().FieldAsString(col_airline);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.airp = Qry.get().FieldAsString(col_airp);
                row.airp_arv = airp_arv_info.get(Qry.get());
                row.original_tag_no = Qry.get().FieldAsString(col_original_tag_no);
                row.surname = Qry.get().FieldAsString(col_surname);
                row.name = Qry.get().FieldAsString(col_name);
                row.prev_airline = Qry.get().FieldAsString(col_prev_airline);
                if(not Qry.get().FieldIsNULL(col_prev_flt_no))
                    row.prev_flt_no = Qry.get().FieldAsInteger(col_prev_flt_no);
                row.prev_suffix = Qry.get().FieldAsString(col_prev_suffix);
                if(not Qry.get().FieldIsNULL(col_prev_scd))
                    row.prev_scd = Qry.get().FieldAsDateTime(col_prev_scd);
                row.grp_id = Qry.get().FieldAsInteger(col_grp_id);
                int pax_grp_user_id = Qry.get().FieldAsInteger(col_user_id);
                row.descr = UsersReader::Instance().getDescr(pax_grp_user_id).value_or("");
                row.desk = Qry.get().FieldAsString(col_desk);
                if(not Qry.get().FieldIsNULL(col_time_create))
                    row.time_create = Qry.get().FieldAsDateTime(col_time_create);
                if(not Qry.get().FieldIsNULL(col_bag_type))
                    row.bag_type = Qry.get().FieldAsInteger(col_bag_type);
                row.num = Qry.get().FieldAsInteger(col_num);
                row.amount = Qry.get().FieldAsInteger(col_amount);
                row.weight = Qry.get().FieldAsInteger(col_weight);
                row.no = Qry.get().FieldAsFloat(col_no);

                row.trfer_airline = Qry.get().FieldAsString(col_trfer_airline);
                row.trfer_airp_dep = Qry.get().FieldAsString(col_trfer_airp_dep);
                if(not Qry.get().FieldIsNULL(col_trfer_flt_no))
                    row.trfer_flt_no = Qry.get().FieldAsInteger(col_trfer_flt_no);
                row.trfer_suffix = Qry.get().FieldAsString(col_trfer_suffix);
                if(not Qry.get().FieldIsNULL(col_trfer_scd))
                    row.trfer_scd = Qry.get().FieldAsDateTime(col_trfer_scd);
                row.trfer_airp_arv = Qry.get().FieldAsString(col_trfer_airp_arv);

                UNACCFullStat.add(row);
                params.overflow.check(UNACCFullStat.RowCount());
            }
        }
    }
}


void RunUNACCStat(
        const TStatParams &params,
        TUNACCAbstractStat &UNACCFullStat
        )
{
    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, params.FirstDate)
        << QParam("LastDate", otDate, params.LastDate);
    string SQLText =
        "select \n"
        "   points.point_id, \n"
        "   points.craft, \n"
        "   points.airline, \n"
        "   points.flt_no, \n"
        "   points.suffix, \n"
        "   points.scd_out, \n"
        "   points.airp, \n"
        "   pax_grp.airp_arv, \n"
        "   unaccomp.original_tag_no, \n"
        "   unaccomp.surname, \n"
        "   unaccomp.name, \n"
        "   unaccomp.airline prev_airline, \n"
        "   unaccomp.flt_no prev_flt_no, \n"
        "   unaccomp.suffix prev_suffix, \n"
        "   unaccomp.scd prev_scd, \n"
        "   pax_grp.grp_id, \n"
        "   users2.descr, \n"
        "   pax_grp.desk, \n"
        "   pax_grp.time_create, \n"
        "   bag2.bag_type, \n"
        "   bag2.num, \n"
        "   bag2.amount, \n"
        "   bag2.weight, \n"
        "   bag_tags.no, \n"
        "   trfer_trips.airline trfer_airline, \n"
        "   trfer_trips.airp_dep trfer_airp_dep, \n"
        "   trfer_trips.flt_no trfer_flt_no, \n"
        "   trfer_trips.suffix trfer_suffix, \n"
        "   trfer_trips.scd trfer_scd, \n"
        "   transfer.airp_arv trfer_airp_arv \n"
        "from \n"
        "   points, \n"
        "   pax_grp, \n"
        "   bag2, \n"
        "   bag_tags, \n"
        "   users2, \n"
        "   unaccomp_bag_info unaccomp, \n"
        "   transfer, \n"
        "   trfer_trips \n"
        "where \n"
        "   points.scd_out >= :FirstDate AND points.scd_out < :LastDate and \n"
        "   pax_grp.user_id = users2.user_id(+) and \n"
        "   bag2.grp_id = bag_tags.grp_id and \n"
        "   bag2.num = bag_tags.bag_num and \n";
    params.AccessClause(SQLText);
    if(params.flt_no != NoExists) {
        SQLText += " points.flt_no = :flt_no and \n";
        QryParams << QParam("flt_no", otInteger, params.flt_no);
    }
    SQLText +=
        "   pax_grp.point_dep = points.point_id and \n"
        "   pax_grp.class is null and \n"
        "   bag2.grp_id = pax_grp.grp_id and \n"
        "   unaccomp.grp_id(+) = bag2.grp_id and \n"
        "   unaccomp.num(+) = bag2.num and \n"
        "   pax_grp.grp_id = transfer.grp_id(+) and \n"
        "   transfer.pr_final(+) <> 0 \n"
        "   and transfer.point_id_trfer = trfer_trips.point_id(+) \n";

    TCachedQuery Qry(SQLText, QryParams);
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        TAirpArvInfo airp_arv_info;
        int col_point_id = Qry.get().FieldIndex("point_id");
        int col_craft = Qry.get().FieldIndex("craft");
        int col_airline = Qry.get().FieldIndex("airline");
        int col_flt_no = Qry.get().FieldIndex("flt_no");
        int col_suffix = Qry.get().FieldIndex("suffix");
        int col_scd_out = Qry.get().FieldIndex("scd_out");
        int col_airp = Qry.get().FieldIndex("airp");
        int col_original_tag_no = Qry.get().FieldIndex("original_tag_no");
        int col_surname = Qry.get().FieldIndex("surname");
        int col_name = Qry.get().FieldIndex("name");
        int col_prev_airline = Qry.get().FieldIndex("prev_airline");
        int col_prev_flt_no = Qry.get().FieldIndex("prev_flt_no");
        int col_prev_suffix = Qry.get().FieldIndex("prev_suffix");
        int col_prev_scd = Qry.get().FieldIndex("prev_scd");
        int col_grp_id = Qry.get().FieldIndex("grp_id");
        int col_descr = Qry.get().FieldIndex("descr");
        int col_desk = Qry.get().FieldIndex("desk");
        int col_time_create = Qry.get().FieldIndex("time_create");
        int col_bag_type = Qry.get().FieldIndex("bag_type");
        int col_num = Qry.get().FieldIndex("num");
        int col_amount = Qry.get().FieldIndex("amount");
        int col_weight = Qry.get().FieldIndex("weight");
        int col_no = Qry.get().FieldIndex("no");

        int col_trfer_airline = Qry.get().FieldIndex("trfer_airline");
        int col_trfer_airp_dep = Qry.get().FieldIndex("trfer_airp_dep");
        int col_trfer_flt_no = Qry.get().FieldIndex("trfer_flt_no");
        int col_trfer_suffix = Qry.get().FieldIndex("trfer_suffix");
        int col_trfer_scd = Qry.get().FieldIndex("trfer_scd");
        int col_trfer_airp_arv = Qry.get().FieldIndex("trfer_airp_arv");

        for(; not Qry.get().Eof; Qry.get().Next()) {
            TUNACCStatRow row;
            row.point_id = Qry.get().FieldAsInteger(col_point_id);
            row.craft = Qry.get().FieldAsString(col_craft);
            row.airline = Qry.get().FieldAsString(col_airline);
            row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
            row.suffix = Qry.get().FieldAsString(col_suffix);
            row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
            row.airp = Qry.get().FieldAsString(col_airp);
            row.airp_arv = airp_arv_info.get(Qry.get());
            row.original_tag_no = Qry.get().FieldAsString(col_original_tag_no);
            row.surname = Qry.get().FieldAsString(col_surname);
            row.name = Qry.get().FieldAsString(col_name);
            row.prev_airline = Qry.get().FieldAsString(col_prev_airline);
            if(not Qry.get().FieldIsNULL(col_prev_flt_no))
                row.prev_flt_no = Qry.get().FieldAsInteger(col_prev_flt_no);
            row.prev_suffix = Qry.get().FieldAsString(col_prev_suffix);
            if(not Qry.get().FieldIsNULL(col_prev_scd))
                row.prev_scd = Qry.get().FieldAsDateTime(col_prev_scd);
            row.grp_id = Qry.get().FieldAsInteger(col_grp_id);
            row.descr = Qry.get().FieldAsString(col_descr);
            row.desk = Qry.get().FieldAsString(col_desk);
            if(not Qry.get().FieldIsNULL(col_time_create))
                row.time_create = Qry.get().FieldAsDateTime(col_time_create);
            if(not Qry.get().FieldIsNULL(col_bag_type))
                row.bag_type = Qry.get().FieldAsInteger(col_bag_type);
            row.num = Qry.get().FieldAsInteger(col_num);
            row.amount = Qry.get().FieldAsInteger(col_amount);
            row.weight = Qry.get().FieldAsInteger(col_weight);
            row.no = Qry.get().FieldAsFloat(col_no);

            row.trfer_airline = Qry.get().FieldAsString(col_trfer_airline);
            row.trfer_airp_dep = Qry.get().FieldAsString(col_trfer_airp_dep);
            if(not Qry.get().FieldIsNULL(col_trfer_flt_no))
                row.trfer_flt_no = Qry.get().FieldAsInteger(col_trfer_flt_no);
            row.trfer_suffix = Qry.get().FieldAsString(col_trfer_suffix);
            if(not Qry.get().FieldIsNULL(col_trfer_scd))
                row.trfer_scd = Qry.get().FieldAsDateTime(col_trfer_scd);
            row.trfer_airp_arv = Qry.get().FieldAsString(col_trfer_airp_arv);

            UNACCFullStat.add(row);
            params.overflow.check(UNACCFullStat.RowCount());
        }
    }

    ArxRunUNACCStat(params, UNACCFullStat);
}

string TUNACCFullStat::getBagWeight(TUNACCFullStat::const_iterator &row) const
{
    ostringstream result;
    if(not empty()) {
        int grp_id = NoExists;
        int num = NoExists;
        if(row != begin()) {
            grp_id = prev(row)->grp_id;
            num = prev(row)->num;
        }
        if(not (grp_id == row->grp_id and num == row->num))
            result << row->weight;
    }
    return result.str();
}

void createXMLUNACCFullStat(
        const TStatParams &params,
        const TUNACCFullStat &UNACCFullStat,
        xmlNodePtr resNode)
{
    if(UNACCFullStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", UNACCFullStat.prn_airline.get(), "");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ॣ. ������"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�⮩��"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ��ଫ����"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ��"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�६� � ���"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 35);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ����"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���� ����"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ����"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortFloat);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ����"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ������"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��ઠ"));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 35);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortInteger);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ���ᠦ��"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��ࢨ筠� ��ઠ"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��ࢨ筠� ��"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��ࢨ�� ३�"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� �뫥�"));
    SetProp(colNode, "width", 105);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TUNACCFullStat::const_iterator i = UNACCFullStat.begin(); i != UNACCFullStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // �� ॣ. ������
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        //�⮩��
        NewTextChild(rowNode, "col", i->desk);
        //�����
        NewTextChild(rowNode, "col", i->descr);
        //��� ��ଫ����
        NewTextChild(rowNode, "col", (i->time_create == NoExists ? "" : DateTimeToStr(i->time_create, "dd.mm.yyyy")));
        //AK
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, i->airline));
        //����
        ostringstream buf;
        buf << setw(3) << setfill('0') << i->flt_no << ElemIdToCodeNative(etSuffix, i->suffix);
        NewTextChild(rowNode, "col", buf.str());
        //��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        //��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_arv));
        //��� ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etCraft, i->craft));
        //�६� � ���
        TDateTime time_travel = getTimeTravel(i->craft, i->airp, i->airp_arv);
        if(time_travel != NoExists)
            NewTextChild(rowNode, "col", DateTimeToStr(time_travel, "hh:nn"));
        else
            NewTextChild(rowNode, "col");
        //����
        NewTextChild(rowNode, "col", getLocaleText((i->trfer_flt_no == NoExists ? "���" : "��")));
        if(i->trfer_flt_no == NoExists) {
            //�� ����
            NewTextChild(rowNode, "col");
            //���� ����
            NewTextChild(rowNode, "col");
            //�� ����
            NewTextChild(rowNode, "col");
            //�� ����
            NewTextChild(rowNode, "col");
        } else {
            //�� ����
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, i->trfer_airline));
            //���� ����
            buf.str("");
            buf << setw(3) << setfill('0') << i->trfer_flt_no << ElemIdToCodeNative(etSuffix, i->trfer_suffix);
            NewTextChild(rowNode, "col", buf.str());
            //�� ����
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->trfer_airp_dep));
            //�� ����
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->trfer_airp_arv));
        }
        //��� ������
        if(i->bag_type == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col",  ElemIdToNameLong(etBagType, i->bag_type));
        //� ���. ��ન
        buf.str("");
        buf << fixed << setprecision(0) << setw(10) << setfill('0') << i->no;
        NewTextChild(rowNode, "col", buf.str());
        // ��� ������
        NewTextChild(rowNode, "col", UNACCFullStat.getBagWeight(i));
        //��� ���ᠦ��
        buf.str("");
        if(not i->surname.empty())
            buf << i->surname;
        if(not i->name.empty()) {
            if(not buf.str().empty())
                buf << " ";
            buf << i->name;
        }
        NewTextChild(rowNode, "col", buf.str());
        //��ࢨ�� � ��ન
        NewTextChild(rowNode, "col", i->original_tag_no);
        //��ࢨ筠� ��
        if(i->prev_airline.empty())
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, i->prev_airline));
        //��ࢨ�� ३�
        buf.str("");
        if(i->prev_flt_no != NoExists)
            buf << setw(3) << setfill('0') << i->prev_flt_no << ElemIdToCodeNative(etSuffix, i->prev_suffix);
        NewTextChild(rowNode, "col", buf.str());
        //��� �뫥� ३�
        if(i->prev_scd == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->prev_scd, "dd.mm.yyyy"));
    }
    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("��ᮯ�. �����"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("���஡���"));
}

void TUNACCStatRow::add_data(ostringstream &buf) const
{
        // �� ॣ. ������
        buf << ElemIdToCodeNative(etAirp, airp) << delim;
        //�⮩��
        buf << desk << delim;
        //�����
        buf << descr << delim;
        //��� ��ଫ����
        buf << (time_create == NoExists ? "" : DateTimeToStr(time_create, "dd.mm.yyyy")) << delim;
        //AK
        buf << ElemIdToCodeNative(etAirline, airline) << delim;
        //����
        ostringstream tmp_s;
        tmp_s << setw(3) << setfill('0') << flt_no << ElemIdToCodeNative(etSuffix, suffix);
        buf << tmp_s.str() << delim;
        //��
        buf << ElemIdToCodeNative(etAirp, airp) << delim;
        //��
        buf << ElemIdToCodeNative(etAirp, airp_arv) << delim;
        //��� ��
        buf << ElemIdToCodeNative(etCraft, craft) << delim;
        //�६� � ���
        TDateTime time_travel = getTimeTravel(craft, airp, airp_arv);
        if(time_travel != NoExists)
            buf << DateTimeToStr(time_travel, "hh:nn") << delim;
        else
            buf << delim;
        //����
        buf << getLocaleText((trfer_flt_no == NoExists ? "���" : "��")) << delim;
        if(trfer_flt_no == NoExists) {
            //�� ����
            buf << delim;
            //���� ����
            buf << delim;
            //�� ����
            buf << delim;
            //�� ����
            buf << delim;
        } else {
            //�� ����
            buf << ElemIdToCodeNative(etAirline, trfer_airline) << delim;
            //���� ����
            tmp_s.str("");
            tmp_s << setw(3) << setfill('0') << trfer_flt_no << ElemIdToCodeNative(etSuffix, trfer_suffix);
            buf << tmp_s.str() << delim;
            //�� ����
            buf << ElemIdToCodeNative(etAirp, trfer_airp_dep) << delim;
            //�� ����
            buf << ElemIdToCodeNative(etAirp, trfer_airp_arv) << delim;
        }
        //��� ������
        buf <<  ElemIdToNameLong(etBagType, bag_type) << delim;
        //� ���. ��ન
        tmp_s.str("");
        tmp_s << fixed << setprecision(0) << setw(10) << setfill('0') << no;
        buf << tmp_s.str() << delim;
        //���
        buf << view_weight << delim;
        //��� ���ᠦ��
        tmp_s.str("");
        if(not surname.empty())
            tmp_s << surname;
        if(not name.empty()) {
            if(not tmp_s.str().empty())
                tmp_s << " ";
            tmp_s << name;
        }
        buf << tmp_s.str() << delim;
        //��ࢨ�� � ��ન
        buf << original_tag_no << delim;
        //��ࢨ筠� ��
        if(prev_airline.empty())
            buf << delim;
        else
            buf << ElemIdToCodeNative(etAirline, prev_airline) << delim;
        //��ࢨ�� ३�
        tmp_s.str("");
        if(prev_flt_no != NoExists)
            tmp_s << setw(3) << setfill('0') << prev_flt_no << ElemIdToCodeNative(etSuffix, prev_suffix);
        buf << tmp_s.str() << delim;
        //��� �뫥� ३�
        if(prev_scd != NoExists)
            buf << DateTimeToStr(prev_scd, "dd.mm.yyyy");
        buf << endl;
}

void TUNACCStatRow::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("�� ॣ. ������") << delim
        << getLocaleText("�⮩��") << delim
        << getLocaleText("�����") << delim
        << getLocaleText("��� ��ଫ����") << delim
        << getLocaleText("��") << delim
        << getLocaleText("����") << delim
        << getLocaleText("��") << delim
        << getLocaleText("��") << delim
        << getLocaleText("��� ��") << delim
        << getLocaleText("�६� � ���") << delim
        << getLocaleText("����") << delim
        << getLocaleText("�� ����") << delim
        << getLocaleText("���� ����") << delim
        << getLocaleText("�� ����") << delim
        << getLocaleText("�� ����") << delim
        << getLocaleText("��� ������") << delim
        << getLocaleText("��ઠ") << delim
        << getLocaleText("���") << delim
        << getLocaleText("��� ���ᠦ��") << delim
        << getLocaleText("��ࢨ筠� ��ઠ") << delim
        << getLocaleText("��ࢨ筠� ��") << delim
        << getLocaleText("��ࢨ�� ३�") << delim
        << getLocaleText("��� �뫥�") << endl;
}

void RunUNACCFullFile(const TStatParams &params, TOrderStatWriter &writer)
{
    TUNACCFullStat UNACCFullStat;
    RunUNACCStat(params, UNACCFullStat);
    for(TUNACCFullStat::const_iterator i = UNACCFullStat.begin(); i != UNACCFullStat.end(); i++) {
        // ����஢���� ��ꥪ� ⮫쪮 ��-�� ⮣�, �⮡� �ந��樠����஢��� ����.
        TUNACCStatRow row = *i;
        row.view_weight = UNACCFullStat.getBagWeight(i);
        writer.insert(row);
    }
}
