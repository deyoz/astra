#include "stat_annul_bt.h"
#include "report_common.h"
#include "stat/stat_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;

void TAnnulBTStatRow::get_tags(TDateTime part_key, int id)
{
    QParams QryParams;
    QryParams << QParam("id", otInteger, id);
    string SQLText = "select no from ";
    std::string table = (part_key != NoExists) ? "arx_annul_tags" : "annul_tags";
    SQLText += table + " where ";
    if(part_key != NoExists) {
        SQLText += " part_key = :part_key and ";
        QryParams << QParam("part_key", otDate, part_key);
    }
    SQLText += " id = :id order by no";
    DB::TCachedQuery Qry(PgOra::getROSession(table), SQLText, QryParams, STDLOG);
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        t_tag_nos_row tag;
        tag.pr_liab_limit = false;
        tag.no = Qry.get().FieldAsFloat("no");
        tags.push_back(tag);
    }
}

void RunAnnulBTStat(TAnnulBTStat &AnnulBTStat, int point_id)
{
    TStatParams params(TStatOverflow::ignore);
    TPrintAirline airline;
    return RunAnnulBTStat(params, AnnulBTStat, airline, point_id);
}

void ArxRunAnnulBTStat(
        const TStatParams &params,
        TAnnulBTStat &AnnulBTStat,
        TPrintAirline &prn_airline)
{
    LogTrace5 << __func__;
    for(int pass = 1; pass <= 2; pass++) {
        QParams QryParams;
        QryParams << QParam("FirstDate", otDate, params.FirstDate)
                  << QParam("LastDate", otDate, params.LastDate)
                  << QParam("arx_trip_date_range", otDate, params.LastDate+ARX_TRIP_DATE_RANGE());

        string SQLText =
            "select arx_points.part_key, "
            "   arx_points.airline, "
            "   arx_points.airp, "
            "   arx_points.flt_no, "
            "   arx_points.suffix, "
            "   arx_annul_bag.id, "
            "   arx_pax_grp.airp_dep, "
            "   arx_pax_grp.airp_arv, "
            "   arx_pax.surname||' '||arx_pax.name full_name, "
            "   arx_annul_bag.pax_id, "
            "   arx_annul_bag.bag_type, "
            "   arx_annul_bag.rfisc, "
            "   arx_annul_bag.time_create, "
            "   arx_annul_bag.time_annul, "
            "   arx_annul_bag.amount, "
            "   arx_annul_bag.weight, "
            "   arx_annul_bag.user_id, "
            "   arx_transfer.airline trfer_airline, \n"
            "   arx_transfer.airp_dep trfer_airp_dep, \n"
            "   arx_transfer.flt_no trfer_flt_no, \n"
            "   arx_transfer.suffix trfer_suffix, \n"
            "   arx_transfer.scd trfer_scd, \n"
            "   arx_transfer.airp_arv trfer_airp_arv \n"
            "from "
            "   arx_pax_grp "
                "LEFT OUTER JOIN arx_transfer ON arx_pax_grp.part_key = arx_transfer.part_key "
                    " AND arx_pax_grp.grp_id = arx_transfer.grp_id AND  arx_transfer.pr_final <> 0 , "
            "   arx_annul_bag "
                "LEFT OUTER JOIN arx_pax ON  arx_annul_bag.part_key = arx_pax.part_key "
                    " AND  arx_annul_bag.pax_id = arx_pax.pax_id , "
            "   arx_points ";
        if(pass == 2) {
            SQLText += getMoveArxQuery();
        }
        SQLText +=
            "where "
            " arx_points.part_key = arx_annul_bag.part_key   and \n"
            " arx_points.part_key = arx_pax_grp.part_key     and \n"
            " arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and \n"
            " arx_points.point_id = arx_pax_grp.point_dep and \n"
            " arx_pax_grp.grp_id = arx_annul_bag.grp_id and ";
        params.AccessClause(SQLText, "arx_points");
        if(pass == 1) {
            SQLText += " arx_points.part_key >= :FirstDate AND arx_points.part_key < :arx_trip_date_range \n";
        }
        if(pass == 2) {
            SQLText += " arx_points.part_key = arx_ext.part_key AND arx_points.move_id = arx_ext.move_id \n";
        }

        if(params.flt_no != NoExists) {
            SQLText += " and arx_points.flt_no = :flt_no \n";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        UsersReader::Instance().updateUsers();
        DB::TCachedQuery Qry(PgOra::getROSession("ARX_POINTS"), SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().FieldIndex("part_key");
            int col_airline = Qry.get().FieldIndex("airline");
            int col_airp = Qry.get().FieldIndex("airp");
            int col_flt_no = Qry.get().FieldIndex("flt_no");
            int col_suffix = Qry.get().FieldIndex("suffix");
            int col_id = Qry.get().FieldIndex("id");
            int col_airp_dep = Qry.get().FieldIndex("airp_dep");
            int col_airp_arv = Qry.get().FieldIndex("airp_arv");
            int col_full_name = Qry.get().FieldIndex("full_name");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_bag_type = Qry.get().FieldIndex("bag_type");
            int col_rfisc = Qry.get().FieldIndex("rfisc");
            int col_time_create = Qry.get().FieldIndex("time_create");
            int col_time_annul = Qry.get().FieldIndex("time_annul");
            int col_amount = Qry.get().FieldIndex("amount");
            int col_weight = Qry.get().FieldIndex("weight");
            int col_user_id = Qry.get().FieldIndex("user_id");
            int col_trfer_airline = Qry.get().FieldIndex("trfer_airline");
            int col_trfer_flt_no = Qry.get().FieldIndex("trfer_flt_no");
            int col_trfer_suffix = Qry.get().FieldIndex("trfer_suffix");
            int col_trfer_scd = Qry.get().FieldIndex("trfer_scd");
            int col_trfer_airp_arv = Qry.get().FieldIndex("trfer_airp_arv");

            for(; not Qry.get().Eof; Qry.get().Next()) {
                prn_airline.check(Qry.get().FieldAsString(col_airline));

                TDateTime part_key = NoExists;
                if(not Qry.get().FieldIsNULL(col_part_key))
                    part_key = Qry.get().FieldAsDateTime(col_part_key);

                TAnnulBTStatRow row;
                row.airline = Qry.get().FieldAsString(col_airline);
                row.airp = Qry.get().FieldAsString(col_airp);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                row.id = Qry.get().FieldAsInteger(col_id);
                row.airp_dep = Qry.get().FieldAsString(col_airp_dep);
                row.airp_arv = Qry.get().FieldAsString(col_airp_arv);
                if(not Qry.get().FieldIsNULL(col_pax_id)) {
                    row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
                    row.full_name = Qry.get().FieldAsString(col_full_name);
                }
                if(not Qry.get().FieldIsNULL(col_bag_type))
                    row.bag_type = Qry.get().FieldAsInteger(col_bag_type);
                row.rfisc = Qry.get().FieldAsString(col_rfisc);
                if(not Qry.get().FieldIsNULL(col_time_create))
                    row.time_create = Qry.get().FieldAsDateTime(col_time_create);
                if(not Qry.get().FieldIsNULL(col_time_annul))
                    row.time_annul = Qry.get().FieldAsDateTime(col_time_annul);
                if(not Qry.get().FieldIsNULL(col_amount))
                    row.amount = Qry.get().FieldAsInteger(col_amount);
                if(not Qry.get().FieldIsNULL(col_weight))
                    row.weight = Qry.get().FieldAsInteger(col_weight);
                if(not Qry.get().FieldIsNULL(col_user_id))
                    row.user_id = Qry.get().FieldAsInteger(col_user_id);
                if(row.user_id != NoExists) {
                    row.agent = UsersReader::Instance().getDescr(row.user_id).value_or("");
                }
                row.trfer_airline = Qry.get().FieldAsString(col_trfer_airline);
                if(not Qry.get().FieldIsNULL(col_trfer_flt_no))
                    row.trfer_flt_no = Qry.get().FieldAsInteger(col_trfer_flt_no);
                row.trfer_suffix = Qry.get().FieldAsString(col_trfer_suffix);
                if(not Qry.get().FieldIsNULL(col_trfer_scd))
                    row.trfer_scd = Qry.get().FieldAsDateTime(col_trfer_scd);
                row.trfer_airp_arv = Qry.get().FieldAsString(col_trfer_airp_arv);

                LogTrace(TRACE5) << "trfer_airp_arv: " << row.trfer_airp_arv;

                row.get_tags(part_key, row.id);
                AnnulBTStat.rows.push_back(row);
                params.overflow.check(AnnulBTStat.rows.size());
            }
        }
    }
}

void RunAnnulBTStat(
        const TStatParams &params,
        TAnnulBTStat &AnnulBTStat,
        TPrintAirline &prn_airline,
        int point_id
        )
{
    LogTrace(TRACE6) << __func__;
    map<int, string> agents;
    DB::TCachedQuery agentQry(
          PgOra::getROSession("USERS2"),
          "SELECT descr FROM users2 WHERE user_id = :user_id",
          QParams() << QParam("user_id", otInteger), STDLOG);

    QParams QryParams;
    if(point_id == NoExists) {
        QryParams << QParam("first_date", otDate, params.FirstDate)
                       << QParam("last_date", otDate, params.LastDate);
    } else {
        QryParams << QParam("point_id", otInteger, point_id);
    }
    std::string SQLText =
        "SELECT "
        "   id, "
        "   pax_id, "
        "   grp_id, "
        "   point_id, "
        "   bag_type, "
        "   rfisc, "
        "   time_create, "
        "   time_annul, "
        "   amount, "
        "   weight, "
        "   user_id "
        "FROM annul_bag ";

    LogTrace5 << __func__ << " point_id: " << point_id;
    if(point_id == ASTRA::NoExists) {
        params.AccessClause(SQLText);
        SQLText += "WHERE scd_out >= :first_date AND scd_out < :last_date ";
    } else {
        SQLText += "WHERE point_id = :point_id ";
    }

    DB::TCachedQuery Qry(
          PgOra::getROSession("ANNUL_BAG"),
          SQLText,
          QryParams, STDLOG);

    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int col_id = Qry.get().FieldIndex("id");
        int col_pax_id = Qry.get().FieldIndex("pax_id");
        int col_grp_id = Qry.get().FieldIndex("grp_id");
        int col_point_id = Qry.get().FieldIndex("point_id");
        int col_bag_type = Qry.get().FieldIndex("bag_type");
        int col_rfisc = Qry.get().FieldIndex("rfisc");
        int col_time_create = Qry.get().FieldIndex("time_create");
        int col_time_annul = Qry.get().FieldIndex("time_annul");
        int col_amount = Qry.get().FieldIndex("amount");
        int col_weight = Qry.get().FieldIndex("weight");
        int col_user_id = Qry.get().FieldIndex("user_id");

        DB::TCachedQuery trferQry(
              PgOra::getROSession("TRANSFER-TRFER_TRIPS"),
              "SELECT \n"
              "   trfer_trips.airline trfer_airline, \n"
              "   trfer_trips.airp_dep trfer_airp_dep, \n"
              "   trfer_trips.flt_no trfer_flt_no, \n"
              "   trfer_trips.suffix trfer_suffix, \n"
              "   trfer_trips.scd trfer_scd, \n"
              "   transfer.airp_arv trfer_airp_arv \n"
              "FROM \n"
              "   transfer, \n"
              "   trfer_trips \n"
              "WHERE \n"
              "   transfer.grp_id = :grp_id AND \n"
              "   transfer.pr_final <> 0 AND \n"
              "   transfer.point_id_trfer = trfer_trips.point_id(+) ",
              QParams() << QParam("grp_id", otInteger), STDLOG);

        for(; not Qry.get().Eof; Qry.get().Next()) {
            TTripInfo fltInfo;
            if (!fltInfo.getByPointId(Qry.get().FieldAsInteger(col_point_id))) {
              continue;
            }
            CheckIn::TSimplePaxGrpItem grpItem;
            if (!grpItem.getByGrpId(Qry.get().FieldAsInteger(col_grp_id))) {
              continue;
            }

            prn_airline.check(fltInfo.airline);

            const TDateTime part_key = ASTRA::NoExists;

            TAnnulBTStatRow row;
            row.airline = fltInfo.airline;
            row.airp = fltInfo.airp;
            row.flt_no = fltInfo.flt_no;
            row.suffix = fltInfo.suffix;
            row.id = Qry.get().FieldAsInteger(col_id);
            row.airp_dep = grpItem.airp_dep;
            row.airp_arv = grpItem.airp_arv;

            CheckIn::TSimplePaxItem paxItem;
            if (paxItem.getByPaxId(Qry.get().FieldAsInteger(col_pax_id))) {
                row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
                row.full_name = paxItem.full_name();
            }

            if(not Qry.get().FieldIsNULL(col_bag_type))
                row.bag_type = Qry.get().FieldAsInteger(col_bag_type);
            row.rfisc = Qry.get().FieldAsString(col_rfisc);
            if(not Qry.get().FieldIsNULL(col_time_create))
                row.time_create = Qry.get().FieldAsDateTime(col_time_create);
            if(not Qry.get().FieldIsNULL(col_time_annul))
                row.time_annul = Qry.get().FieldAsDateTime(col_time_annul);
            if(not Qry.get().FieldIsNULL(col_amount))
                row.amount = Qry.get().FieldAsInteger(col_amount);
            if(not Qry.get().FieldIsNULL(col_weight))
                row.weight = Qry.get().FieldAsInteger(col_weight);
            if(not Qry.get().FieldIsNULL(col_user_id))
                row.user_id = Qry.get().FieldAsInteger(col_user_id);
            if(row.user_id != NoExists) {
                map<int, string>::iterator agent = agents.find(row.user_id);
                if(agent == agents.end()) {
                    agentQry.get().SetVariable("user_id", row.user_id);
                    agentQry.get().Execute();
                    string buf;
                    if(not agentQry.get().Eof)
                        buf = agentQry.get().FieldAsString("descr");
                    auto ret = agents.insert(make_pair(row.user_id, buf));
                    agent = ret.first;
                }
                row.agent = agent->second;
            }

            trferQry.get().SetVariable("grp_id", Qry.get().FieldAsInteger(col_grp_id));
            trferQry.get().Execute();
            if (!trferQry.get().Eof) {
                int col_trfer_airline = Qry.get().FieldIndex("trfer_airline");
                int col_trfer_flt_no = Qry.get().FieldIndex("trfer_flt_no");
                int col_trfer_suffix = Qry.get().FieldIndex("trfer_suffix");
                int col_trfer_scd = Qry.get().FieldIndex("trfer_scd");
                int col_trfer_airp_arv = Qry.get().FieldIndex("trfer_airp_arv");

                for(; not trferQry.get().Eof; trferQry.get().Next()) {
                    row.trfer_airline = Qry.get().FieldAsString(col_trfer_airline);
                    if(not Qry.get().FieldIsNULL(col_trfer_flt_no))
                        row.trfer_flt_no = Qry.get().FieldAsInteger(col_trfer_flt_no);
                    row.trfer_suffix = Qry.get().FieldAsString(col_trfer_suffix);
                    if(not Qry.get().FieldIsNULL(col_trfer_scd))
                        row.trfer_scd = Qry.get().FieldAsDateTime(col_trfer_scd);
                    row.trfer_airp_arv = Qry.get().FieldAsString(col_trfer_airp_arv);
                    LogTrace(TRACE5) << "trfer_airp_arv: " << row.trfer_airp_arv;

                    row.get_tags(part_key, row.id);
                    AnnulBTStat.rows.push_back(row);
                    params.overflow.check(AnnulBTStat.rows.size());
                }
            } else {
                row.get_tags(part_key, row.id);
                AnnulBTStat.rows.push_back(row);
                params.overflow.check(AnnulBTStat.rows.size());
            }
        }
    }

    if(point_id == NoExists) {
        ArxRunAnnulBTStat(params, AnnulBTStat, prn_airline);
    }
}

void createXMLAnnulBTStat(
        const TStatParams &params,
        const TAnnulBTStat &AnnulBTStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode)
{
    if(AnnulBTStat.rows.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���ᠦ��"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ���. ��ப"));
    SetProp(colNode, "width", 90);
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ����"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ���"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ������/RFISC"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ���᪠"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�६� ���᪠"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� 㤠�����"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�६� 㤠�����"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(list<TAnnulBTStatRow>::const_iterator i = AnnulBTStat.rows.begin(); i != AnnulBTStat.rows.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, i->airline));
        // ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        //����
        ostringstream buf;
        buf << setw(3) << setfill('0') << i->flt_no << ElemIdToCodeNative(etSuffix, i->suffix);
        NewTextChild(rowNode, "col", buf.str());
        // �����
        NewTextChild(rowNode, "col", i->agent);
        // ���ᠦ��
        NewTextChild(rowNode, "col", transliter(i->full_name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU));
        // �� ���. ��ப
        NewTextChild(rowNode, "col", get_tag_range(i->tags, LANG_EN));
        // ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_dep));
        // ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_arv));
        // ����
        if(i->amount != NoExists)
            NewTextChild(rowNode, "col", i->amount);
        else
            NewTextChild(rowNode, "col");
        // ���
        if(i->weight != NoExists)
            NewTextChild(rowNode, "col", i->weight);
        else
            NewTextChild(rowNode, "col");
        // ��� ������/RFISC
        buf.str("");
        if(not i->rfisc.empty())
            buf << i->rfisc;
        else if(i->bag_type != NoExists)
            buf << ElemIdToNameLong(etBagType, i->bag_type);
        NewTextChild(rowNode, "col", buf.str());
        if(i->time_create != NoExists) {
            // ��� ���᪠
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_create, "dd.mm.yyyy"));
            // �६� ���᪠
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_create, "hh:nn"));
        } else {
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        }
        if(i->time_create != NoExists) {
            // ��� 㤠�����
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_annul, "dd.mm.yyyy"));
            // �६� 㤠�����
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_annul, "hh:nn"));
        } else {
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        }
        // ����
        if(i->trfer_airline.empty()) {
            NewTextChild(rowNode, "col", getLocaleText("���"));
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        } else {
            // ����
            NewTextChild(rowNode, "col", getLocaleText("��"));
            //����
            buf.str("");
            buf
                << ElemIdToCodeNative(etAirline, i->trfer_airline)
                << setw(3) << setfill('0') << i->trfer_flt_no << ElemIdToCodeNative(etSuffix, i->trfer_suffix);
            NewTextChild(rowNode, "col", buf.str());
            // ���
            NewTextChild(rowNode, "col", DateTimeToStr(i->trfer_scd, "dd.mm.yyyy"));
        }
    }
    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("����. ��ન"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("���஡���"));
}

void TAnnulBTStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("��") << delim
        << getLocaleText("��") << delim
        << getLocaleText("����") << delim
        << getLocaleText("�����") << delim
        << getLocaleText("���ᠦ��") << delim
        << getLocaleText("�� ���. ��ப") << delim
        << getLocaleText("��") << delim
        << getLocaleText("��") << delim
        << getLocaleText("�� ����") << delim
        << getLocaleText("�� ���") << delim
        << getLocaleText("��� ������/RFISC") << delim
        << getLocaleText("��� ���᪠") << delim
        << getLocaleText("�६� ���᪠") << delim
        << getLocaleText("��� 㤠�����") << delim
        << getLocaleText("�६� 㤠�����") << delim
        << getLocaleText("����") << delim
        << getLocaleText("����") << delim
        << getLocaleText("���") << endl;
}

void TAnnulBTStatCombo::add_data(ostringstream &buf) const
{
        // ��
    buf <<  ElemIdToCodeNative(etAirline, data.airline) << delim
        // ��
        <<  ElemIdToCodeNative(etAirp, data.airp) << delim;
        //����
    ostringstream oss1;
    oss1 << setw(3) << setfill('0') << data.flt_no
            << ElemIdToCodeNative(etSuffix, data.suffix);
    buf <<  oss1.str() << delim
        // �����
        <<  data.agent << delim
        // ���ᠦ��
        <<  transliter(data.full_name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU) << delim
        // �� ���. ��ப
        <<  get_tag_range(data.tags, LANG_EN) << delim
        // ��
        << ElemIdToCodeNative(etAirp, data.airp_dep) << delim
        // ��
        <<  ElemIdToCodeNative(etAirp, data.airp_arv) << delim;
    // ����
    if (data.amount != NoExists) buf << data.amount;
    buf << delim;
    // ���
    if (data.weight != NoExists) buf << data.weight;
    buf << delim;
    // ��� ������/RFISC
    oss1.str("");
    if (not data.rfisc.empty())
        oss1 << data.rfisc;
    else if (data.bag_type != NoExists)
        oss1 << ElemIdToNameLong(etBagType, data.bag_type);
    buf <<  oss1.str() << delim;
    if (data.time_create != NoExists)
    {
        // ��� ���᪠
        buf <<  DateTimeToStr(data.time_create, "dd.mm.yyyy") << delim
        // �६� ���᪠
            <<  DateTimeToStr(data.time_create, "hh:nn") << delim;
    } else buf << delim << delim;
    if (data.time_create != NoExists)
    {
        // ��� 㤠�����
        buf <<  DateTimeToStr(data.time_annul, "dd.mm.yyyy") << delim
        // �६� 㤠�����
            <<  DateTimeToStr(data.time_annul, "hh:nn") << delim;
    } else buf << delim << delim;
    // ����
    if (data.trfer_airline.empty())
    {
        buf << getLocaleText("���") << delim << delim;
    } else {
        // ����
        buf << getLocaleText("��") << delim;
        //����
        oss1.str("");
        oss1 << ElemIdToCodeNative(etAirline, data.trfer_airline)
                << setw(3) << setfill('0') << data.trfer_flt_no
                << ElemIdToCodeNative(etSuffix, data.trfer_suffix);
        buf <<  oss1.str() << delim;
        // ���
        buf << DateTimeToStr(data.trfer_scd, "dd.mm.yyyy");
    }
    buf << endl;
}

void RunAnnulBTStatFile(const TStatParams &params, TOrderStatWriter &writer, TPrintAirline &prn_airline)
{
    TAnnulBTStat AnnulBTStat;
    RunAnnulBTStat(params, AnnulBTStat, prn_airline);
    for (std::list<TAnnulBTStatRow>::const_iterator i = AnnulBTStat.rows.begin(); i != AnnulBTStat.rows.end(); ++i)
        writer.insert(TAnnulBTStatCombo(*i));
}

