#include "stat_trfer_pax.h"
#include "astra_misc.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "qrys.h"
#include "passenger.h"
#include "report_common.h"
#include "stat_utils.h"

template void RunTrferPaxStat(TStatParams const&, TOrderStatWriter&, TPrintAirline&);
template void RunTrferPaxStat(TStatParams const&, TTrferPaxStat&, TPrintAirline&);

using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace BASIC::date_time;

void TTrferPaxStatItem::clear()
{
    airline.clear();
    airp.clear();
    flt_no1 = NoExists;
    suffix1.clear();

    date1 = 0;

    trfer_airp.clear();
    flt_no2 = NoExists;
    suffix2.clear();

    date2 = 0;

    airp_arv.clear();
    seg_category = TSegCategories::Unknown;
    pax_name.clear();
    pax_doc.clear();
    pax_amount = 0;
    adult = 0;
    child = 0;
    baby = 0;

    rk_weight = 0;
    bag_amount = 0;
    bag_weight = 0;
}

void TTrferPaxStatItem::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("АК") << delim
        << getLocaleText("АПВ") << delim
        << getLocaleText("Сег.1") << delim
        << getLocaleText("Дата") << delim
        << getLocaleText("АПТ") << delim
        << getLocaleText("Сег.2") << delim
        << getLocaleText("Дата") << delim
        << getLocaleText("АПП") << delim
        << getLocaleText("Категория") << delim
        << getLocaleText("ФИО пассажира") << delim
        << getLocaleText("Документ") << delim
        << getLocaleText("CAP.DOC.PAX") << delim
        << getLocaleText("ВЗ") << delim
        << getLocaleText("РБ") << delim
        << getLocaleText("РМ") << delim
        << getLocaleText("Р/к") << delim
        << getLocaleText("БГ мест") << delim
        << getLocaleText("БГ вес") << delim
        << getLocaleText("Бирки") << endl;
}

void TTrferPaxStatItem::add_data(ostringstream &buf) const
{
    if(airline.empty()) {
        buf
            << getLocaleText("Итого:") << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim;
    } else {
        // АК
        buf << ElemIdToCodeNative(etAirline, airline) << delim;
        // АП
        buf << ElemIdToCodeNative(etAirp, airp) << delim;
        //Сег.1
        ostringstream tmp;
        tmp << setw(3) << setfill('0') << flt_no1 << ElemIdToCodeNative(etSuffix, suffix1);
        buf << tmp.str() << delim;
        //Дата
        buf << DateTimeToStr(date1, "dd.mm.yyyy") << delim;
        //Трансфер
        buf << ElemIdToCodeNative(etAirp, trfer_airp) << delim;
        //Сег.2
        tmp.str("");
        tmp
            << ElemIdToCodeNative(etAirline, airline2)
            << setw(3) << setfill('0') << flt_no2 << ElemIdToCodeNative(etSuffix, suffix2);
        buf << tmp.str() << delim;
        //Дата
        buf << DateTimeToStr(date2, "dd.mm.yyyy") << delim;
        //А/п прилета
        buf << ElemIdToCodeNative(etAirp, airp_arv) << delim;
        //Категория
        buf << getLocaleText(TSegCategory().encode(seg_category)) << delim;
        //ФИО
        buf << pax_name << delim;
        //Паспорт
        buf << pax_doc << delim;
    }
    //Кол-во пасс.
    buf << pax_amount << delim;
    //ВЗ
    buf << adult << delim;
    //РБ
    buf << child << delim;
    //РМ
    buf << baby << delim;
    //Р/кладь(вес)
    buf << rk_weight << delim;
    //БГ мест
    buf << bag_amount << delim;
    //БГ вес
    buf << bag_weight << delim;
    //Бирки
    buf << tags << endl;
}

void getSegList(const string &segments, list<pair<TTripInfo, string> > &seg_list)
{
    vector<string> tokens;
    boost::split(tokens, segments, boost::is_any_of(";"));
    for(vector<string>::iterator i = tokens.begin();
            i != tokens.end(); i++) {
        vector<string> flt_info;
        boost::split(flt_info, *i, boost::is_any_of(","));
        TTripInfo flt;
        flt.airline = flt_info[0];
        flt.airp = flt_info[1];
        flt.flt_no = ToInt(flt_info[2]);
        flt.suffix = flt_info[3];
        StrToDateTime(flt_info[4].c_str(), ServerFormatDateTimeAsString, flt.scd_out);

        string airp_arv;
        if(flt_info.size() > 5)
            airp_arv = flt_info[5];
        seg_list.push_back(make_pair(flt, airp_arv));
    }
}

template <class T>
void RunTrferPaxStat(
        const TStatParams &params,
        T &TrferPaxStat,
        TPrintAirline &prn_airline
        )
{
    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, params.FirstDate)
        << QParam("LastDate", otDate, params.LastDate)
        << QParam("pr_lat", otInteger, TReqInfo::Instance()->desk.lang!=AstraLocale::LANG_RU);
    TTrferPaxStatItem totals;
    for(int pass = 0; pass <= 2; pass++) {
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText =
            "select ";
        if(pass)
            SQLText += "points.part_key, ";
        else
            SQLText += "null part_key, ";
        SQLText +=
            "   trfer_pax_stat.*, "
            "   pax.surname||' '||pax.name full_name, "
            "   pax.pers_type, ";
        if (pass!=0)
            SQLText +=
                " arch.get_birks2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags ";
        else
            SQLText +=
                " ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags ";
        SQLText +=
            "from ";

        if (pass!=0)
        {
            SQLText +=
            "   arx_trfer_pax_stat trfer_pax_stat, "
            "   arx_pax pax, "
            "   arx_points points ";
            if (pass==2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        }
        else
            SQLText +=
            "   trfer_pax_stat, "
            "   pax, "
            "   points ";

        SQLText +=
            "where ";
        if(pass != 0)
            SQLText +=
                "   points.part_key = trfer_pax_stat.part_key and "
                "   pax.part_key = trfer_pax_stat.part_key and ";

        if (pass==1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if (pass==2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        SQLText +=
            "   trfer_pax_stat.point_id = points.point_id and ";
        params.AccessClause(SQLText);
        SQLText +=
            "   trfer_pax_stat.scd_out>=:FirstDate AND trfer_pax_stat.scd_out<:LastDate and "
            "   trfer_pax_stat.pax_id = pax.pax_id ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().FieldIndex("part_key");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_rk_weight = Qry.get().FieldIndex("rk_weight");
            int col_bag_weight = Qry.get().FieldIndex("bag_weight");
            int col_bag_amount = Qry.get().FieldIndex("bag_amount");
            int col_segments = Qry.get().FieldIndex("segments");
            int col_full_name = Qry.get().FieldIndex("full_name");
            int col_pers_type = Qry.get().FieldIndex("pers_type");
            int col_tags = Qry.get().FieldIndex("tags");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TDateTime part_key = NoExists;
                if(not Qry.get().FieldIsNULL(col_part_key))
                    part_key = Qry.get().FieldAsDateTime(col_part_key);
                int pax_id = Qry.get().FieldAsInteger(col_pax_id);
                int rk_weight = Qry.get().FieldAsInteger(col_rk_weight);
                int bag_weight = Qry.get().FieldAsInteger(col_bag_weight);
                int bag_amount = Qry.get().FieldAsInteger(col_bag_amount);
                string segments = Qry.get().FieldAsString(col_segments);
                string full_name = Qry.get().FieldAsString(col_full_name);
                string pers_type = Qry.get().FieldAsString(col_pers_type);
                string tags = Qry.get().FieldAsString(col_tags);

                list<pair<TTripInfo, string> > seg_list;
                getSegList(segments, seg_list);

                TTrferPaxStat tmp_stat;
                TTrferPaxStatItem item;
                for(list<pair<TTripInfo, string> >::iterator flt = seg_list.begin();
                        flt != seg_list.end(); flt++) {
                    if(item.airline.empty()) {
                        prn_airline.check(flt->first.airline);
                        item.airline = flt->first.airline;
                        item.airp = flt->first.airp;
                        item.flt_no1 = flt->first.flt_no;
                        item.suffix1 = flt->first.suffix;
                        item.date1 = flt->first.scd_out;
                    } else {
                        item.trfer_airp = flt->first.airp;
                        item.airline2 = flt->first.airline;
                        item.flt_no2 = flt->first.flt_no;
                        item.suffix2 = flt->first.suffix;
                        item.date2 = flt->first.scd_out;
                        item.airp_arv = flt->second;
                        item.pax_name = transliter(full_name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
                        item.pax_doc = CheckIn::GetPaxDocStr(part_key, pax_id, false);

                        typedef map<bool, TSegCategories::Enum> TSeg2Map;
                        typedef map<bool, TSeg2Map> TCategoryMap;

                        static const TCategoryMap category_map =
                        {
                            {
                                false,
                                {
                                    {false, TSegCategories::IntInt},
                                    {true, TSegCategories::IntFor}
                                }
                            },
                            {
                                true,
                                {
                                    {false, TSegCategories::ForInt},
                                    {true, TSegCategories::ForFor}
                                }
                            }
                        };

                        string country1 = get_airp_country(item.airp);
                        string country2 = get_airp_country(item.trfer_airp);
                        string country3 = get_airp_country(item.airp_arv);

                        bool is_inter1 = country1 != country2;
                        bool is_inter2 = country2 != country3;
                        item.seg_category = category_map.at(is_inter1).at(is_inter2);

                        TSegCategories::Enum seg_category = TSegCategories::Unknown;
                        if(params.seg_category != TSegCategories::Unknown) {
                            seg_category = item.seg_category;
                        }

                        if(
                                params.seg_category == seg_category and
                                (params.trfer_airp.empty() or params.trfer_airp == item.trfer_airp) and
                                (params.trfer_airline.empty() or params.trfer_airline == item.airline)
                          )
                            tmp_stat.push_back(item);

                        item.clear();
                        item.airline = flt->first.airline;
                        item.airp = flt->first.airp;
                        item.flt_no1 = flt->first.flt_no;
                        item.suffix1 = flt->first.suffix;
                        item.date1 = flt->first.scd_out;
                    }
                }

                if(tmp_stat.begin() != tmp_stat.end()) {
                    tmp_stat.begin()->pax_amount = 1;
                    tmp_stat.begin()->adult = pers_type == "ВЗ";
                    tmp_stat.begin()->child = pers_type == "РБ";
                    tmp_stat.begin()->baby = pers_type == "РМ";
                    tmp_stat.begin()->rk_weight = rk_weight;
                    tmp_stat.begin()->bag_amount = bag_amount;
                    tmp_stat.begin()->bag_weight = bag_weight;
                    tmp_stat.begin()->tags = tags;

                    totals.pax_amount++;
                    totals.adult += tmp_stat.begin()->adult;
                    totals.child += tmp_stat.begin()->child;
                    totals.baby += tmp_stat.begin()->baby;
                    totals.rk_weight += rk_weight;
                    totals.bag_amount += bag_amount;
                    totals.bag_weight += bag_weight;
                }

                for(TTrferPaxStat::iterator i = tmp_stat.begin();
                        i != tmp_stat.end(); i++) TrferPaxStat.push_back(*i);

                params.overflow.check(TrferPaxStat.size());
            }
        }
    }
    if(totals.pax_amount != 0)
        TrferPaxStat.push_back(totals);
}

void createXMLTrferPaxStat(
        const TStatParams &params,
        TTrferPaxStat &TrferPaxStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode)
{
    if(TrferPaxStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", prn_airline.get(), "");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АПВ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Сег.1"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 55);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АПТ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Сег.2"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 55);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АПП"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Категория"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("ФИО пассажира"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Документ"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("CAP.DOC.PAX"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("ВЗ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("РБ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("РМ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Р/к"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ мест"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ вес"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Бирки"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(TTrferPaxStat::iterator stat = TrferPaxStat.begin();
            stat != TrferPaxStat.end(); stat++) {
        rowNode = NewTextChild(rowsNode, "row");
        if(stat->airline.empty()) {
            NewTextChild(rowNode, "col", getLocaleText("Итого:"));
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        } else {
            // АК
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, stat->airline));
            // АП
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->airp));
            //Сег.1
            ostringstream buf;
            buf << setw(3) << setfill('0') << stat->flt_no1 << ElemIdToCodeNative(etSuffix, stat->suffix1);
            NewTextChild(rowNode, "col", buf.str());
            //Дата
            NewTextChild(rowNode, "col", DateTimeToStr(stat->date1, "dd.mm.yy"));
            //Трансфер
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->trfer_airp));
            //Сег.2
            buf.str("");
            buf
                << ElemIdToCodeNative(etAirline, stat->airline2)
                << setw(3) << setfill('0') << stat->flt_no2 << ElemIdToCodeNative(etSuffix, stat->suffix2);
            NewTextChild(rowNode, "col", buf.str());
            //Дата
            NewTextChild(rowNode, "col", DateTimeToStr(stat->date2, "dd.mm.yy"));
            //А/п прилета
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->airp_arv));
            //Категория
            NewTextChild(rowNode, "col", getLocaleText(TSegCategory().encode(stat->seg_category)));
            //ФИО
            NewTextChild(rowNode, "col", stat->pax_name);
            //Паспорт
            NewTextChild(rowNode, "col", stat->pax_doc);
        }
        //Кол-во пасс.
        NewTextChild(rowNode, "col", stat->pax_amount);
        //ВЗ
        NewTextChild(rowNode, "col", stat->adult);
        //РБ
        NewTextChild(rowNode, "col", stat->child);
        //РМ
        NewTextChild(rowNode, "col", stat->baby);
        //Р/кладь(вес)
        NewTextChild(rowNode, "col", stat->rk_weight);
        //БГ мест
        NewTextChild(rowNode, "col", stat->bag_amount);
        //БГ вес
        NewTextChild(rowNode, "col", stat->bag_weight);
        //Бирки
        NewTextChild(rowNode, "col", stat->tags);
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Трансфер"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

string segListFromDB(int tckin_id)
{
    TCachedQuery Qry(
            "select grp_id from tckin_pax_grp where tckin_id = :tckin_id AND tckin_pax_grp.transit_num=0 order by seg_no",
            QParams() << QParam("tckin_id", otInteger, tckin_id));
    Qry.get().Execute();
    ostringstream result;
    for(; not Qry.get().Eof; Qry.get().Next()) {
        int grp_id = Qry.get().FieldAsInteger("grp_id");

        if(not result.str().empty()) result << ";";
        TTripInfo info;
        if(info.getByGrpId(grp_id)) {
            result
                << info.airline << ","
                << info.airp << ","
                << info.flt_no << ","
                << info.suffix << ","
                << DateTimeToStr(info.scd_out) << ",";
            CheckIn::TSimplePaxGrpItem grp;
            if(grp.getByGrpId(grp_id))
                result << grp.airp_arv;
        }
    }
    return result.str();
}

void get_trfer_pax_stat(int point_id)
{
    TCachedQuery delQry("delete from trfer_pax_stat where point_id = :point_id",
            QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();

    TTripInfo info;
    info.getByPointId(point_id);

    TCachedQuery insQry(
            "insert into trfer_pax_stat( "
            "   point_id, "
            "   scd_out, "
            "   pax_id, "
            "   rk_weight, "
            "   bag_weight, "
            "   bag_amount, "
            "   segments "
            ") values( "
            "   :point_id, "
            "   :scd_out, "
            "   :pax_id, "
            "   :rk_weight, "
            "   :bag_weight, "
            "   :bag_amount, "
            "   :segments "
            ")",
            QParams()
            << QParam("point_id", otInteger, point_id)
            << QParam("scd_out", otDate, info.scd_out)
            << QParam("pax_id", otInteger)
            << QParam("rk_weight", otInteger)
            << QParam("bag_weight", otInteger)
            << QParam("bag_amount", otInteger)
            << QParam("segments", otString)
            );

    TCachedQuery selQry(
            "select "
            "    pax.pax_id, "
            "    tckin_pax_grp.tckin_id, "
            "    NVL(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS rk_weight, "
            "    NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_amount, "
            "    NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_weight "
            "from "
            "    pax_grp, "
            "    tckin_pax_grp, "
            "    pax "
            "where "
            "    pax_grp.point_dep = :point_id and "
            "    pax.refuse is null and "
            "    pax_grp.grp_id = tckin_pax_grp.grp_id and  "
            "    tckin_pax_grp.seg_no = 1 and tckin_pax_grp.transit_num=0 AND "
            "    pax_grp.grp_id = pax.grp_id ",
            QParams() << QParam("point_id", otInteger, point_id));
    selQry.get().Execute();
    if(not selQry.get().Eof) {
        int col_pax_id = selQry.get().FieldIndex("pax_id");
        int col_tckin_id = selQry.get().FieldIndex("tckin_id");
        int col_rk_weight = selQry.get().FieldIndex("rk_weight");
        int col_bag_amount = selQry.get().FieldIndex("bag_amount");
        int col_bag_weight = selQry.get().FieldIndex("bag_weight");
        for(; not selQry.get().Eof; selQry.get().Next()) {

            insQry.get().SetVariable("pax_id", selQry.get().FieldAsInteger(col_pax_id));
            insQry.get().SetVariable("rk_weight", selQry.get().FieldAsInteger(col_rk_weight));
            insQry.get().SetVariable("bag_weight", selQry.get().FieldAsInteger(col_bag_weight));
            insQry.get().SetVariable("bag_amount", selQry.get().FieldAsInteger(col_bag_amount));
            insQry.get().SetVariable("segments", segListFromDB(selQry.get().FieldAsInteger(col_tckin_id)));

            insQry.get().Execute();
        }
    }
}

