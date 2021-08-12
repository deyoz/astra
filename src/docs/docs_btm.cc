#include "docs_btm.h"
#include "docs_utils.h"
#include "franchise.h"
#include "stat/stat_utils.h"
#include "baggage_ckin.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace AstraLocale;

bool lessBagTagRow(const TBagTagRow &p1, const TBagTagRow &p2)
{
    bool result;
    if(p1.point_num == p2.point_num) {
        if(p1.pr_trfer == p2.pr_trfer) {
            if(p1.last_target == p2.last_target) {
                if(p1.class_priority == p2.class_priority) {
                    if(p1.to_ramp == p2.to_ramp) {
                        if(p1.bag_name_priority == p2.bag_name_priority) {
                            if(p1.tag_type == p2.tag_type) {
                                result = p1.color < p2.color;
                            } else
                                result = p1.tag_type > p2.tag_type;
                        } else
                            result = p1.bag_name_priority < p2.bag_name_priority;
                    } else
                        result = p1.to_ramp < p2.to_ramp;
                } else
                    result = p1.class_priority < p2.class_priority;
            } else
                result = p1.last_target > p2.last_target;
        } else
            result = p1.pr_trfer < p2.pr_trfer;
    } else
        result = p1.point_num > p2.point_num;
    return result;
}

void dump_tag_row(TBagTagRow &tag, bool hdr = true)
{
    ostringstream log;
    if(hdr) {
        log
            << setw(9)  << "pr_trfer"
            << setw(12) << "last_target"
            << setw(10) << "grp_id"
            << setw(9)  << "airp_arv"
            << setw(11) << "class_name"
            << setw(12) << "to_ramp_str"
            << setw(9)  << "bag_type"
            << setw(18) << "bag_name_priority"
            << setw(23) << "bag_name"
            << setw(8)  << "bag_num"
            << setw(7)  << "amount"
            << setw(7)  << "weight"
            << setw(9)  << "tag_type"
            << setw(6)  << "color"
            << setw(10) << "no"
            << setw(30) << "tag_range"
            << setw(4)  << "num"
            << endl;
        ProgTrace(TRACE5, "%s", log.str().c_str());
        log.str("");
    }
    log
        << setw(9)  << tag.pr_trfer
        << setw(12) << tag.last_target
        << setw(10) << tag.grp_id
        << setw(9)  << tag.airp_arv
        << setw(11) << tag.class_name
        << setw(12)  << tag.to_ramp_str
        << setw(9)  << tag.bag_type
        << setw(18) << tag.bag_name_priority
        << setw(23) << tag.bag_name
        << setw(8)  << tag.bag_num
        << setw(7)  << tag.amount
        << setw(7)  << tag.weight
        << setw(9)  << tag.tag_type
        << setw(6)  << tag.color
        << setw(10) << fixed << setprecision(0) << tag.no
        << setw(30) << tag.tag_range
        << setw(4)  << tag.num
        << endl;
    ProgTrace(TRACE5, "%s", log.str().c_str());
}

void dump_bag_tags(vector<TBagTagRow> &bag_tags)
{
    ostringstream log;
    bool hdr = true;
    for(vector<TBagTagRow>::iterator iv = bag_tags.begin(); iv != bag_tags.end(); iv++) {
        dump_tag_row(*iv, hdr);
        if(hdr) hdr = false;
    }
    ProgTrace(TRACE5, "%s", log.str().c_str());
}

struct TBag2PK {
    int grp_id, num;
    bool operator==(const TBag2PK op)
    {
        return (grp_id == op.grp_id) && (num == op.num);
    }
};

void BTM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string rpt_name;
    if(rpt_params.airp_arv.empty() ||
            rpt_params.rpt_type==rtBTMTXT) {
        if(rpt_params.pr_trfer)
            rpt_name="BMTrferTotal";
        else
            rpt_name="BMTotal";
    } else {
        if(rpt_params.pr_trfer)
            rpt_name="BMTrfer";
        else
            rpt_name="BM";
    };
    if (rpt_params.rpt_type==rtBTMTXT) rpt_name=rpt_name+"Txt";
    get_compatible_report_form(rpt_name, reqNode, resNode);

    t_rpt_bm_bag_name bag_names;
    DB::TQuery QryPoints(PgOra::getROSession("POINTS"),STDLOG);
    QryPoints.SQLText = "select airp, airline from points where point_id = :point_id AND pr_del>=0";
    QryPoints.CreateVariable("point_id", otInteger, rpt_params.point_id);
    QryPoints.Execute();
    if(QryPoints.Eof)
        throw Exception("RunBMNew: point_id %d not found", rpt_params.point_id);
    string airp = QryPoints.FieldAsString(0);
    string airline = QryPoints.FieldAsString(1);
    bag_names.init(airp, airline);
    vector<TBagTagRow> bag_tags;

    std::list<std::string> involvedTables({"PAX_GRP","POINTS","BAG2","HALLS2","PAX"});
    string SQLText =
        "SELECT ";
    if(rpt_params.pr_trfer)
        SQLText +=
            "    (CASE WHEN transfer.grp_id IS NOT NULL THEN 1 ELSE 0 END) pr_trfer, "
            "    trfer_trips.airline trfer_airline, "
            "    trfer_trips.flt_no trfer_flt_no, "
            "    trfer_trips.suffix trfer_suffix, "
            "    transfer.airp_arv trfer_airp_arv, "
            "    trfer_trips.scd trfer_scd, ";
    else
        SQLText +=
            "    0 pr_trfer, "
            "    null trfer_airline, "
            "    null trfer_flt_no, "
            "    null trfer_suffix, "
            "    null trfer_airp_arv, "
            "    null trfer_scd, ";
    SQLText +=
        "    points.point_num, "
        "    pax.pax_id, "
        "    pax_grp.grp_id, "
        "    pax_grp.airp_arv, "
        "    COALESCE(pax.cabin_class, pax_grp.class) class, "
        "    pax_grp.status, "
        "    bag2.bag_type, "
        "    bag2.rfisc, "
        "    bag2.num bag_num, "
        "    bag2.amount, "
        "    bag2.weight, "
        "    bag2.pr_liab_limit, "
        "    bag2.to_ramp "
        "FROM pax_grp "
        "JOIN points ON pax_grp.point_arv = points.point_id "
        "JOIN (bag2 "
        "    LEFT OUTER JOIN halls2 ON bag2.hall = halls2.id "
        "    LEFT OUTER JOIN pax ON bag2.grp_id = pax.grp_id "
        ") ON pax_grp.grp_id = bag2.grp_id ";
    if(rpt_params.pr_trfer)
    {
        SQLText +=
            "LEFT OUTER JOIN (transfer "
            "    LEFT OUTER JOIN trfer_trips ON transfer.point_id_trfer = trfer_trips.point_id "
            ") ON pax_grp.grp_id = transfer.grp_id AND transfer.pr_final <> 0 ";
        involvedTables.push_back("TRANSFER");
        involvedTables.push_back("TRFER_TRIPS");
    }
    if(rpt_params.trzt_autoreg != TRptParams::TrztAutoreg::All)
    {
        SQLText += "LEFT OUTER JOIN tckin_pax_grp "
                   "ON pax_grp.grp_id = tckin_pax_grp.grp_id AND tckin_pax_grp.transit_num <> 0 ";
        if(rpt_params.trzt_autoreg == TRptParams::TrztAutoreg::Auto) {
          SQLText +=
              "AND pax_grp.status IN ('T') "
              "AND tckin_pax_grp.grp_id is NOT NULL ";
        } else {
          SQLText +=
              "AND NOT (pax_grp.status IN ('T') "
              "AND tckin_pax_grp.grp_id IS NOT NULL) ";
        }
        involvedTables.push_back("TCKIN_PAX_GRP");
    }
    SQLText +=
        "WHERE "
        "    points.pr_del >= 0 AND "
        "    pax_grp.point_dep = :point_id AND ";

    if (rpt_params.pr_brd) {
      SQLText += CKIN::bag_pool_boarded_query();
    } else {
      SQLText += CKIN::bag_pool_not_refused_query();
    }
    SQLText +=
        "    AND bag2.pr_cabin = 0 ";

    DB::TQuery Qry(PgOra::getROSession(involvedTables),STDLOG);
    if(!rpt_params.airp_arv.empty()) {
        SQLText += " AND pax_grp.airp_arv = :target ";
        Qry.CreateVariable("target", otString, rpt_params.airp_arv);
    }
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        SQLText +=
            "   and COALESCE(NULLIF(halls2.rpt_grp,''), ' ') = COALESCE(NULLIF(:zone,''), ' ') and bag2.hall IS NOT NULL ";
        Qry.CreateVariable("zone", otString, rpt_params.ckin_zone);
    }

    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    ProgTrace(TRACE5, "SQLText: %s", SQLText.c_str());
    Qry.Execute();
    set<int> grps;
    for(; !Qry.Eof; Qry.Next()) {
        if(not rpt_params.mkt_flt.empty()) {
            TMktFlight mkt_flt;
            mkt_flt.getByPaxId(Qry.FieldAsInteger("pax_id"));
            if(mkt_flt.empty() or not(mkt_flt == rpt_params.mkt_flt))
                continue;
        }

        int cur_grp_id = Qry.FieldAsInteger("grp_id");
        int cur_bag_num = Qry.FieldAsInteger("bag_num");

        TBagTagRow bag_tag_row;
        bag_tag_row.pr_trfer = Qry.FieldAsInteger("pr_trfer");
        bag_tag_row.last_target = get_last_target(Qry, rpt_params);
        bag_tag_row.point_num = Qry.FieldAsInteger("point_num");
        bag_tag_row.grp_id = cur_grp_id;
        bag_tag_row.airp_arv = Qry.FieldAsString("airp_arv");
        if(not Qry.FieldIsNULL("bag_type"))
            bag_tag_row.bag_type = Qry.FieldAsInteger("bag_type");
        bag_tag_row.to_ramp = Qry.FieldAsInteger("to_ramp");
        if(bag_tag_row.to_ramp)
            bag_tag_row.to_ramp_str = getLocaleText("� �����", rpt_params.GetLang());

        string class_code = Qry.FieldAsString("class");
        bag_tag_row.rfisc = Qry.FieldAsString("rfisc");
        bag_names.get(class_code, bag_tag_row, rpt_params);
        if(class_code.empty()) {
            if((string)Qry.FieldAsString("status") == "E") {
                bag_tag_row.class_priority = 50;
                bag_tag_row.class_code = "1";
                bag_tag_row.class_name = getLocaleText("���. �����", rpt_params.GetLang());
            } else
                bag_tag_row.class_priority = 100;
        } else {
            bag_tag_row.class_priority = ((const TClassesRow&)base_tables.get("classes").get_row( "code", class_code)).priority;
            bag_tag_row.class_code = rpt_params.ElemIdToReportElem(etClass, class_code, efmtCodeNative);
            bag_tag_row.class_name = rpt_params.ElemIdToReportElem(etClass, class_code, efmtNameLong);
        }
        bag_tag_row.bag_num = cur_bag_num;
        bag_tag_row.amount = Qry.FieldAsInteger("amount");
        bag_tag_row.weight = Qry.FieldAsInteger("weight");
        bag_tag_row.pr_liab_limit = Qry.FieldAsInteger("pr_liab_limit");

        DB::TQuery tagsQry(PgOra::getROSession("BAG_TAGS"),STDLOG);
        tagsQry.SQLText =
            "select "
            "   bag_tags.tag_type, "
            "   bag_tags.color, "
            //"   to_char(bag_tags.no) no " // oracle version
            "   bag_tags.no "
            "from "
            "   bag_tags "
            "where "
            "   bag_tags.grp_id = :grp_id and "
            "   bag_tags.bag_num = :bag_num ";
        tagsQry.CreateVariable("grp_id", otInteger, cur_grp_id);
        tagsQry.CreateVariable("bag_num", otInteger, cur_bag_num);
        tagsQry.Execute();
        if(tagsQry.Eof) {
            bag_tags.push_back(bag_tag_row);
            bag_tags.back().bag_name_priority = -1;
            bag_tags.back().bag_name.clear();
        } else
            for(; !tagsQry.Eof; tagsQry.Next()) {
                bag_tags.push_back(bag_tag_row);
                bag_tags.back().tag_type = tagsQry.FieldAsString("tag_type");
                bag_tags.back().color = tagsQry.FieldAsString("color");
                bag_tags.back().no = tagsQry.FieldAsFloat("no");
            }

        if(grps.find(cur_grp_id) == grps.end()) {
            grps.insert(cur_grp_id);
            // �饬 ���ਢ易��� ��ન ��� ������ ��㯯�
            std::list<std::string> tables;
            string SQLText =
                "SELECT "
                "   bag_tags.tag_type, "
                "   bag_tags.color, "
                "   bag_tags.no "
                "FROM bag_tags ";
            if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
                tables.push_back("PAX_GRP");
                tables.push_back("HALLS2");
                SQLText += "JOIN (pax_grp "
                           "      LEFT OUTER JOIN halls2 ON pax_grp.hall = halls2.id ";
                           "      AND COALESCE(halls2.rpt_grp, ' ') = COALESCE(:zone, ' ') "
                           ") ON pax_grp.grp_id = bag_tags.grp_id AND pax_grp.hall IS NOT NULL ";
            }
            SQLText +=
                "WHERE "
                "   bag_tags.grp_id = :grp_id AND "
                "   bag_tags.bag_num IS NULL ";
            DB::TQuery unlinkTagsQry(PgOra::getROSession(tables), STDLOG);
            unlinkTagsQry.SQLText = SQLText;
            if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
                unlinkTagsQry.CreateVariable("zone", otString, rpt_params.ckin_zone);
            }
            unlinkTagsQry.CreateVariable("grp_id", otInteger, cur_grp_id);
            unlinkTagsQry.Execute();
            for(; !unlinkTagsQry.Eof; unlinkTagsQry.Next()) {
                bag_tags.push_back(bag_tag_row);
                bag_tags.back().bag_num = -1;
                bag_tags.back().amount = 0;
                bag_tags.back().weight = 0;
                bag_tags.back().bag_name_priority = -1;
                bag_tags.back().bag_name = "";
                bag_tags.back().tag_type = unlinkTagsQry.FieldAsString("tag_type");
                bag_tags.back().color = unlinkTagsQry.FieldAsString("color");
                bag_tags.back().no = unlinkTagsQry.FieldAsFloat("no");
            }
        }
    }
    sort(bag_tags.begin(), bag_tags.end(), lessBagTagRow);
    dump_bag_tags(bag_tags);

    TBagTagRow bag_tag_row;
    TBagTagRow bag_sum_row;
    bag_sum_row.amount = 0;
    bag_sum_row.weight = 0;
    vector<t_tag_nos_row> tag_nos;
    vector<TBagTagRow> bm_table;
    int bag_sum_idx = -1;
    vector<TBag2PK> bag2_pks;
    for(vector<TBagTagRow>::iterator iv = bag_tags.begin(); iv != bag_tags.end(); iv++) {
        if(
                !(
                    bag_tag_row.last_target == iv->last_target &&
                    bag_tag_row.airp_arv == iv->airp_arv &&
                    bag_tag_row.class_code == iv->class_code &&
                    bag_tag_row.to_ramp == iv->to_ramp &&
                    bag_tag_row.bag_name == iv->bag_name &&
                    bag_tag_row.tag_type == iv->tag_type &&
                    bag_tag_row.color == iv->color
                 )
          ) {
            if(!tag_nos.empty()) {
                bm_table.back().tag_range = get_tag_range(tag_nos, rpt_params.GetLang());
                bm_table.back().num = tag_nos.size();
                tag_nos.clear();
            }
            bag_tag_row.last_target = iv->last_target;
            bag_tag_row.airp_arv = iv->airp_arv;
            bag_tag_row.class_code = iv->class_code;
            bag_tag_row.to_ramp = iv->to_ramp;
            bag_tag_row.bag_name = iv->bag_name;
            bag_tag_row.tag_type = iv->tag_type;
            bag_tag_row.color = iv->color;
            bm_table.push_back(*iv);
            if(iv->class_code.empty())
                bm_table.back().class_name = getLocaleText("��ᮯ஢������� �����", rpt_params.GetLang());
            if(iv->color.empty())
                bm_table.back().color = "-";
            else
                bm_table.back().color = rpt_params.ElemIdToReportElem(etTagColor, iv->color, efmtNameLong);
            bm_table.back().amount = 0;
            bm_table.back().weight = 0;
            if(
                    !(
                        bag_sum_row.last_target == iv->last_target &&
                        bag_sum_row.airp_arv == iv->airp_arv &&
                        bag_sum_row.class_code == iv->class_code
                     )
              ) {
                if(bag_sum_row.amount != 0) {
                    bm_table[bag_sum_idx].amount = bag_sum_row.amount;
                    bm_table[bag_sum_idx].weight = bag_sum_row.weight;
                    bag_sum_row.amount = 0;
                    bag_sum_row.weight = 0;
                    bag2_pks.clear();
                }
                bag_sum_row.last_target = iv->last_target;
                bag_sum_row.airp_arv = iv->airp_arv;
                bag_sum_row.class_code = iv->class_code;
                bag_sum_idx = bm_table.size() - 1;
            }
        }
        TBag2PK bag2__pk;
        bag2__pk.grp_id = iv->grp_id;
        bag2__pk.num = iv->bag_num;
        if(find(bag2_pks.begin(), bag2_pks.end(), bag2__pk) == bag2_pks.end()) {
            bag2_pks.push_back(bag2__pk);
            bag_sum_row.amount += iv->amount;
            bag_sum_row.weight += iv->weight;
        }
        t_tag_nos_row tag_nos_row;
        tag_nos_row.pr_liab_limit = iv->pr_liab_limit;
        tag_nos_row.no = iv->no;
        tag_nos.push_back(tag_nos_row);
    }
    if(!tag_nos.empty()) {
        bm_table.back().tag_range = get_tag_range(tag_nos, rpt_params.GetLang());
        bm_table.back().num = tag_nos.size();
        tag_nos.clear();
    }
    if(bag_sum_row.amount != 0) {
        bm_table[bag_sum_idx].amount = bag_sum_row.amount;
        bm_table[bag_sum_idx].weight = bag_sum_row.weight;
        bag_sum_row.amount = 0;
        bag_sum_row.weight = 0;
    }
    dump_bag_tags(bm_table);


    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, rpt_params.pr_trfer ? "v_bm_trfer" : "v_bm");

    int TotAmount = 0;
    int TotWeight = 0;
    for(vector<TBagTagRow>::iterator iv = bm_table.begin(); iv != bm_table.end(); iv++) {
        if(iv->tag_type.empty()) continue;
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        string airp_arv = iv->airp_arv;
        NewTextChild(rowNode, "airp_arv", airp_arv);
        NewTextChild(rowNode, "airp_arv_name", rpt_params.ElemIdToReportElem(etAirp, airp_arv, efmtNameLong));
        NewTextChild(rowNode, "pr_trfer", iv->pr_trfer);
        NewTextChild(rowNode, "last_target", iv->last_target);
        NewTextChild(rowNode, "to_ramp", iv->to_ramp_str);
        NewTextChild(rowNode, "bag_name", iv->bag_name);
        NewTextChild(rowNode, "birk_range", iv->tag_range);
        NewTextChild(rowNode, "color", iv->color);
        NewTextChild(rowNode, "num", iv->num);
        NewTextChild(rowNode, "pr_vip", 2);

        NewTextChild(rowNode, "class", iv->class_code);
        NewTextChild(rowNode, "class_name", iv->class_name);
        NewTextChild(rowNode, "amount", iv->amount);
        NewTextChild(rowNode, "weight", iv->weight);
        TotAmount += iv->amount;
        TotWeight += iv->weight;
        Qry.Next();
    }
    // ������ ��६���� ����
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "TotPcs", TotAmount);
    NewTextChild(variablesNode, "TotWeight", TotWeight);
    NewTextChild(variablesNode, "Tot", vs_number(TotAmount, rpt_params.IsInter()));

    auto &dbSession = rpt_params.ckin_zone != ALL_CKIN_ZONES
                        ? PgOra::getROSession({"PAX_GRP","BAG2","TRANSFER","PAX"})
                        : PgOra::getROSession({"PAX_GRP","BAG2","TRANSFER","HALLS2","PAX"});
    DB::TQuery Qry2(dbSession,STDLOG);
    SQLText =
        "select "
        "  sum(bag2.amount) amount, "
        "  sum(bag2.weight) weight "
        "from "
        "  pax_grp "
        "    join TRANSFER "
        "       on PAX_GRP.GRP_ID = TRANSFER.GRP_ID ";
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES)
        SQLText += "  join ( "
                   "    BAG2 "
                   "      left outer join HALLS2 "
                   "        on BAG2.HALL = HALLS2.ID "
                   "    ) "
                   "    on PAX_GRP.GRP_ID = BAG2.GRP_ID ";

    else
        SQLText += "  join BAG2 "
                   "    on PAX_GRP.GRP_ID = BAG2.GRP_ID ";
    SQLText +=
        "where "
        "  pax_grp.point_dep = :point_id and "
        "  pax_grp.grp_id = bag2.grp_id and "
        "  pax_grp.grp_id = transfer.grp_id and "
        "  pax_grp.status NOT IN ('E') and " +
        CKIN::bag_pool_not_refused_query() +
        "  and bag2.pr_cabin = 0 and "
        "  transfer.pr_final <> 0 ";
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        SQLText +=
            "   and COALESCE(halls2.rpt_grp, ' ') = COALESCE(:zone, ' ') and bag2.hall IS NOT NULL ";
        Qry2.CreateVariable("zone", otString, rpt_params.ckin_zone);
    }
    Qry2.SQLText = SQLText;
    Qry2.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry2.Execute();
    int trfer_amount = 0;
    int trfer_weight = 0;
    if(not Qry2.Eof) {
        trfer_amount = Qry2.FieldAsInteger("amount");
        trfer_weight = Qry2.FieldAsInteger("weight");
    }
    NewTextChild(variablesNode, "TotTrferPcs", trfer_amount);
    NewTextChild(variablesNode, "TotTrferWeight", trfer_weight);

    DB::TQuery QryPoints2(PgOra::getROSession("POINTS"),STDLOG);

    QryPoints2.SQLText =
        "select "
        "   airp, "
        "   airline, "
        "   flt_no, "
        "   suffix, "
        "   craft, "
        "   bort, "
        "   park_out park, "
        "   scd_out, "
        "   act_out, "
        "   airp_fmt, "
        "   airline_fmt, "
        "   suffix_fmt, "
        "   craft_fmt "
        "from "
        "   points "
        "where "
        "   point_id = :point_id AND pr_del>=0";
    QryPoints2.CreateVariable("point_id", otInteger, rpt_params.point_id);
    ProgTrace(TRACE5, "SQLText: %s", QryPoints2.SQLText.c_str());
    QryPoints2.Execute();
    if(QryPoints2.Eof) throw Exception("RunBM: variables fetch failed for point_id " + IntToString(rpt_params.point_id));

    TElemFmt airline_fmt = (TElemFmt)QryPoints2.FieldAsInteger("airline_fmt");
    TElemFmt suffix_fmt = (TElemFmt)QryPoints2.FieldAsInteger("suffix_fmt");
    TElemFmt craft_fmt = (TElemFmt)QryPoints2.FieldAsInteger("craft_fmt");

    string suffix;
    int flt_no = NoExists;
    if(rpt_params.mkt_flt.empty()) {
        Franchise::TProp franchise_prop;
        if(
                franchise_prop.get(rpt_params.point_id, Franchise::TPropType::bagManifest) and
                franchise_prop.val == Franchise::pvNo
          ) {
            airline = franchise_prop.franchisee.airline;
            flt_no = franchise_prop.franchisee.flt_no;
            suffix = franchise_prop.franchisee.suffix;
        } else {
            airline = QryPoints2.FieldAsString("airline");
            flt_no = QryPoints2.FieldAsInteger("flt_no");
            suffix = QryPoints2.FieldAsString("suffix");
        }
    } else {
        airline = rpt_params.mkt_flt.airline;
        flt_no = rpt_params.mkt_flt.flt_no;
        suffix = rpt_params.mkt_flt.suffix;
    }
    string craft = QryPoints2.FieldAsString("craft");
    string tz_region = AirpTZRegion(QryPoints2.FieldAsString("airp"));

    //    TCrafts crafts;

    NewTextChild(variablesNode, "own_airp_name", getLocaleText("CAP.DOC.AIRP_NAME",
       LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, rpt_params.dup_lang())), rpt_params.dup_lang()));
    NewTextChild(variablesNode, "own_airp_name_lat", getLocaleText("CAP.DOC.AIRP_NAME",
       LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, AstraLocale::LANG_EN)), AstraLocale::LANG_EN));
    NewTextChild(variablesNode, "airp_dep_name", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong));
    NewTextChild(variablesNode, "airline_name", rpt_params.ElemIdToReportElem(etAirline, airline, efmtNameLong));
    ostringstream flt;
    flt
        << rpt_params.ElemIdToReportElem(etAirline, airline, airline_fmt)
        << setw(3) << setfill('0') << flt_no
        << rpt_params.ElemIdToReportElem(etSuffix, suffix, suffix_fmt);
    NewTextChild(variablesNode, "flt", flt.str());
    NewTextChild(variablesNode, "bort", QryPoints2.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", rpt_params.ElemIdToReportElem(etCraft, craft, craft_fmt));
    NewTextChild(variablesNode, "park", QryPoints2.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(getReportSCDOut(rpt_params.point_id), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", rpt_params.IsInter()));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh:nn", rpt_params.IsInter()));
    string airp_arv_name;
    if(rpt_params.airp_arv.size())
        airp_arv_name = rpt_params.ElemIdToReportElem(etAirp, rpt_params.airp_arv, efmtNameLong);
    NewTextChild(variablesNode, "airp_arv_name", airp_arv_name);

    {
        // delete in future 14.10.07 !!!
        NewTextChild(variablesNode, "DosKwit");
        NewTextChild(variablesNode, "DosPcs");
        NewTextChild(variablesNode, "DosWeight");
    }

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", rpt_params.IsInter()));
    string pr_brd_pax_str = (string)"����� �������� ��ப " + (rpt_params.pr_brd ? "(��ᠦ)" : "(��ॣ)");
    NewTextChild(variablesNode, "pr_brd_pax", getLocaleText(pr_brd_pax_str, rpt_params.dup_lang()));
    NewTextChild(variablesNode, "pr_brd_pax_lat", getLocaleText(pr_brd_pax_str, AstraLocale::LANG_EN));
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        NewTextChild(variablesNode, "zone", get_hall_list(airp, rpt_params));
    } else
        NewTextChild(variablesNode, "zone"); // ���⮩ ⥣ - ��� ��⠫���樨 �� ����
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    STAT::set_variables(resNode, rpt_params.GetLang());
    trip_rpt_person(resNode, rpt_params);
    TDateTime takeoff = NoExists;
    if(not QryPoints2.FieldIsNULL("act_out"))
        takeoff = UTCToLocal(QryPoints2.FieldAsDateTime("act_out"), tz_region);
    NewTextChild(variablesNode, "takeoff", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm.yy hh:nn")));
    NewTextChild(variablesNode, "takeoff_date", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm")));
    NewTextChild(variablesNode, "takeoff_time", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "hh:nn")));
}

int testbm(int argc,char **argv)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize("���");
    xmlDocPtr resDoc=xmlNewDoc("1.0");
    xmlNodePtr rootNode=xmlNewDocNode(resDoc,NULL,"query",NULL);
    TRptParams rpt_params;
    rpt_params.point_id = 603906;
    rpt_params.ckin_zone = "area bis";
    rpt_params.pr_et = false;
    rpt_params.pr_trfer = false;
    rpt_params.pr_brd = false;
    BTM(rpt_params, rootNode, rootNode);
    return 0;
}

