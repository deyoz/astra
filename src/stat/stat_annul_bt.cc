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
    if(part_key != NoExists)
        SQLText += "arx_annul_tags annul_tags ";
    else
        SQLText += "annul_tags ";
    SQLText += "where ";
    if(part_key != NoExists) {
        SQLText += " part_key = :part_key and ";
        QryParams << QParam("part_key", otDate, part_key);
    }
    SQLText += " id = :id order by no";
    TCachedQuery Qry(SQLText, QryParams);
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

void RunAnnulBTStat(
        const TStatParams &params,
        TAnnulBTStat &AnnulBTStat,
        TPrintAirline &prn_airline,
        int point_id
        )
{
    map<int, string> agents;
    TCachedQuery agentQry("select descr from users2 where user_id = :user_id",
            QParams() << QParam("user_id", otInteger));

    int pass_count = (point_id == NoExists ? 2 : 0);
    for(int pass = 0; pass <= pass_count; pass++) {
        QParams QryParams;
        if(point_id == NoExists)
            QryParams
                << QParam("FirstDate", otDate, params.FirstDate)
                << QParam("LastDate", otDate, params.LastDate);
        else
            QryParams << QParam("point_id", otInteger, point_id);
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText =
            "select ";
        if(pass != 0)
            SQLText += "    points.part_key, ";
        else
            SQLText += "    null part_key, ";
        SQLText +=
            "   points.airline, "
            "   points.airp, "
            "   points.flt_no, "
            "   points.suffix, "
            "   annul_bag.id, "
            "   pax_grp.airp_dep, "
            "   pax_grp.airp_arv, "
            "   pax.surname||' '||pax.name full_name, "
            "   annul_bag.pax_id, "
            "   annul_bag.bag_type, "
            "   annul_bag.rfisc, "
            "   annul_bag.time_create, "
            "   annul_bag.time_annul, "
            "   annul_bag.amount, "
            "   annul_bag.weight, "
            "   annul_bag.user_id, ";
        if(pass != 0)
            SQLText +=
                "   transfer.airline trfer_airline, \n"
                "   transfer.airp_dep trfer_airp_dep, \n"
                "   transfer.flt_no trfer_flt_no, \n"
                "   transfer.suffix trfer_suffix, \n"
                "   transfer.scd trfer_scd, \n"
                "   transfer.airp_arv trfer_airp_arv \n";
        else
            SQLText +=
                "   trfer_trips.airline trfer_airline, \n"
                "   trfer_trips.airp_dep trfer_airp_dep, \n"
                "   trfer_trips.flt_no trfer_flt_no, \n"
                "   trfer_trips.suffix trfer_suffix, \n"
                "   trfer_trips.scd trfer_scd, \n"
                "   transfer.airp_arv trfer_airp_arv \n";
        SQLText +=
            "from ";
        if(pass != 0) {
            SQLText +=
                "   arx_pax_grp pax_grp, \n"
                "   arx_pax pax, \n"
                "   arx_annul_bag annul_bag,"
                "   arx_points points, \n "
                "   arx_transfer transfer \n";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                "   annul_bag, "
                "   pax_grp, \n"
                "   pax, \n"
                "   transfer, \n"
                "   trfer_trips, \n"
                "   points ";
        }
        SQLText += "where ";
        if(pass != 0)
            SQLText +=
                "   points.part_key = annul_bag.part_key and "
                "   pax_grp.part_key = points.part_key and \n"
                "   annul_bag.part_key = pax.part_key(+) and \n"
                "   pax_grp.part_key = transfer.part_key(+) and \n";
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        if(point_id == NoExists) {
            params.AccessClause(SQLText);
            SQLText +=
                "   points.scd_out >= :FirstDate AND points.scd_out < :LastDate and \n";
        } else
            SQLText +=
                "   points.point_id = :point_id and ";
        SQLText +=
            "   points.point_id = pax_grp.point_dep and "
            "   pax_grp.grp_id = annul_bag.grp_id and "
            "   annul_bag.pax_id = pax.pax_id(+) and "
            "   pax_grp.grp_id = transfer.grp_id(+) and \n"
            "   transfer.pr_final(+) <> 0 \n";
        if(pass == 0)
            SQLText +=
                "   and transfer.point_id_trfer = trfer_trips.point_id(+) \n";
        if(point_id == NoExists and params.flt_no != NoExists) {
            SQLText += " and points.flt_no = :flt_no \n";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        TCachedQuery Qry(SQLText, QryParams);
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
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Агент"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Пассажир"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("№№ баг. бирок"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("От"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ мест"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ вес"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип багажа/RFISC"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата выпуска"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время выпуска"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата удаления"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время удаления"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Трфр"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(list<TAnnulBTStatRow>::const_iterator i = AnnulBTStat.rows.begin(); i != AnnulBTStat.rows.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // АК
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, i->airline));
        // АП
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        //Рейс
        ostringstream buf;
        buf << setw(3) << setfill('0') << i->flt_no << ElemIdToCodeNative(etSuffix, i->suffix);
        NewTextChild(rowNode, "col", buf.str());
        // Агент
        NewTextChild(rowNode, "col", i->agent);
        // Пассажир
        NewTextChild(rowNode, "col", transliter(i->full_name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU));
        // №№ баг. бирок
        NewTextChild(rowNode, "col", get_tag_range(i->tags, LANG_EN));
        // От
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_dep));
        // До
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_arv));
        // Мест
        if(i->amount != NoExists)
            NewTextChild(rowNode, "col", i->amount);
        else
            NewTextChild(rowNode, "col");
        // Вес
        if(i->weight != NoExists)
            NewTextChild(rowNode, "col", i->weight);
        else
            NewTextChild(rowNode, "col");
        // Тип багажа/RFISC
        buf.str("");
        if(not i->rfisc.empty())
            buf << i->rfisc;
        else if(i->bag_type != NoExists)
            buf << ElemIdToNameLong(etBagType, i->bag_type);
        NewTextChild(rowNode, "col", buf.str());
        if(i->time_create != NoExists) {
            // Дата выпуска
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_create, "dd.mm.yyyy"));
            // Время выпуска
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_create, "hh:nn"));
        } else {
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        }
        if(i->time_create != NoExists) {
            // Дата удаления
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_annul, "dd.mm.yyyy"));
            // Время удаления
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_annul, "hh:nn"));
        } else {
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        }
        // Трфр
        if(i->trfer_airline.empty()) {
            NewTextChild(rowNode, "col", getLocaleText("НЕТ"));
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        } else {
            // Трфр
            NewTextChild(rowNode, "col", getLocaleText("ДА"));
            //Рейс
            buf.str("");
            buf
                << ElemIdToCodeNative(etAirline, i->trfer_airline)
                << setw(3) << setfill('0') << i->trfer_flt_no << ElemIdToCodeNative(etSuffix, i->trfer_suffix);
            NewTextChild(rowNode, "col", buf.str());
            // Дата
            NewTextChild(rowNode, "col", DateTimeToStr(i->trfer_scd, "dd.mm.yyyy"));
        }
    }
    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Аннул. бирки"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

void TAnnulBTStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("АК") << delim
        << getLocaleText("АП") << delim
        << getLocaleText("Рейс") << delim
        << getLocaleText("Агент") << delim
        << getLocaleText("Пассажир") << delim
        << getLocaleText("№№ баг. бирок") << delim
        << getLocaleText("От") << delim
        << getLocaleText("До") << delim
        << getLocaleText("БГ мест") << delim
        << getLocaleText("БГ вес") << delim
        << getLocaleText("Тип багажа/RFISC") << delim
        << getLocaleText("Дата выпуска") << delim
        << getLocaleText("Время выпуска") << delim
        << getLocaleText("Дата удаления") << delim
        << getLocaleText("Время удаления") << delim
        << getLocaleText("Трфр") << delim
        << getLocaleText("Рейс") << delim
        << getLocaleText("Дата") << endl;
}

void TAnnulBTStatCombo::add_data(ostringstream &buf) const
{
        // АК
    buf <<  ElemIdToCodeNative(etAirline, data.airline) << delim
        // АП
        <<  ElemIdToCodeNative(etAirp, data.airp) << delim;
        //Рейс
    ostringstream oss1;
    oss1 << setw(3) << setfill('0') << data.flt_no
            << ElemIdToCodeNative(etSuffix, data.suffix);
    buf <<  oss1.str() << delim
        // Агент
        <<  data.agent << delim
        // Пассажир
        <<  transliter(data.full_name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU) << delim
        // №№ баг. бирок
        <<  get_tag_range(data.tags, LANG_EN) << delim
        // От
        << ElemIdToCodeNative(etAirp, data.airp_dep) << delim
        // До
        <<  ElemIdToCodeNative(etAirp, data.airp_arv) << delim;
    // Мест
    if (data.amount != NoExists) buf << data.amount;
    buf << delim;
    // Вес
    if (data.weight != NoExists) buf << data.weight;
    buf << delim;
    // Тип багажа/RFISC
    oss1.str("");
    if (not data.rfisc.empty())
        oss1 << data.rfisc;
    else if (data.bag_type != NoExists)
        oss1 << ElemIdToNameLong(etBagType, data.bag_type);
    buf <<  oss1.str() << delim;
    if (data.time_create != NoExists)
    {
        // Дата выпуска
        buf <<  DateTimeToStr(data.time_create, "dd.mm.yyyy") << delim
        // Время выпуска
            <<  DateTimeToStr(data.time_create, "hh:nn") << delim;
    } else buf << delim << delim;
    if (data.time_create != NoExists)
    {
        // Дата удаления
        buf <<  DateTimeToStr(data.time_annul, "dd.mm.yyyy") << delim
        // Время удаления
            <<  DateTimeToStr(data.time_annul, "hh:nn") << delim;
    } else buf << delim << delim;
    // Трфр
    if (data.trfer_airline.empty())
    {
        buf << getLocaleText("НЕТ") << delim << delim;
    } else {
        // Трфр
        buf << getLocaleText("ДА") << delim;
        //Рейс
        oss1.str("");
        oss1 << ElemIdToCodeNative(etAirline, data.trfer_airline)
                << setw(3) << setfill('0') << data.trfer_flt_no
                << ElemIdToCodeNative(etSuffix, data.trfer_suffix);
        buf <<  oss1.str() << delim;
        // Дата
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

