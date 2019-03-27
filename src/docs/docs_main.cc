#include <set>
#include "docs_main.h"
#include "docs_common.h"
#include "stat/stat_utils.h"
#include "oralib.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_date_time.h"
#include "base_tables.h"
#include "season.h"
#include "brd.h"
#include "aodb.h"
#include "astra_misc.h"
#include "term_version.h"
#include "load_fr.h"
#include "passenger.h"
#include "remarks.h"
#include "telegram.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/str_utils.h"
#include "baggage_calc.h"
#include "salons.h"
#include "franchise.h"
#include "docs_ptm.h"
#include <boost/algorithm/string.hpp>
#include "stat/stat_annul_bt.h"
#include "docs_exam.h"
#include "docs_notpres.h"
#include "docs_refuse.h"
#include "docs_rem.h"
#include "docs_services.h"
#include "docs_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace ASTRA::date_time;
using namespace boost;


struct TBag2PK {
    int grp_id, num;
    bool operator==(const TBag2PK op)
    {
        return (grp_id == op.grp_id) && (num == op.num);
    }
};

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

const int TO_RAMP_PRIORITY = 1000;

void t_rpt_bm_bag_name::get(string class_code, TBagTagRow &bag_tag_row, TRptParams &rpt_params)
{
    string &result = bag_tag_row.bag_name;
    for(vector<TBagNameRow>::iterator iv = bag_names.begin(); iv != bag_names.end(); iv++) {
        bool eval = false;
        if(class_code == iv->class_code) {
            if(bag_tag_row.rfisc.empty()) {
                if(bag_tag_row.bag_type != NoExists and bag_tag_row.bag_type == iv->bag_type) {
                    eval = true;
                }
            } else if(bag_tag_row.rfisc == iv->rfisc) {
                eval = true;
            }
        }
        if(eval) {
            result = rpt_params.IsInter() ? iv->name_lat : iv->name;
            if(result.empty())
                result = iv->name;
            break;
        }
    }
    if(not result.empty())
        bag_tag_row.bag_name_priority = bag_tag_row.bag_type;
}

void t_rpt_bm_bag_name::init(const string &airp, const string &airline, bool pr_stat_fv)
{
    bag_names.clear();
    TQuery Qry(&OraSession);
    string SQLText = (string)
        "select "
        "   bag_type, "
        "   rfisc, "
        "   class, "
        "   airp, "
        "   airline, "
        "   name, "
        "   name_lat "
        "from " +
        (pr_stat_fv ? "stat_fv_bag_names" : "rpt_bm_bag_names ") +
        " where "
        "   (airp is null or "
        "   airp = :airp) and "
        "   (airline is null or "
        "   airline = :airline) "
        "order by "
        "   airline nulls last, "
        "   airp nulls last, "
        "   name, name_lat ";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("airp", otString, airp);
    Qry.CreateVariable("airline", otString, airline);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        TBagNameRow bag_name_row;
        if(not Qry.FieldIsNULL("bag_type"))
            bag_name_row.bag_type = Qry.FieldAsInteger("bag_type");
        bag_name_row.rfisc = Qry.FieldAsString("rfisc");
        bag_name_row.class_code = Qry.FieldAsString("class");
        bag_name_row.airp = Qry.FieldAsString("airp");
        bag_name_row.airline = Qry.FieldAsString("airline");
        bag_name_row.name = Qry.FieldAsString("name");
        bag_name_row.name_lat = Qry.FieldAsString("name_lat");
        bag_names.push_back(bag_name_row);
    }
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

void BTM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
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
    Qry.Clear();
    Qry.SQLText = "select airp, airline from points where point_id = :point_id AND pr_del>=0";
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("RunBMNew: point_id %d not found", rpt_params.point_id);
    string airp = Qry.FieldAsString(0);
    string airline = Qry.FieldAsString(1);
    bag_names.init(airp, airline);
    vector<TBagTagRow> bag_tags;
    Qry.Clear();
    string SQLText =
        "select ";
    if(rpt_params.pr_trfer)
        SQLText +=
            "    nvl2(transfer.grp_id, 1, 0) pr_trfer, \n"
            "    trfer_trips.airline trfer_airline, \n"
            "    trfer_trips.flt_no trfer_flt_no, \n"
            "    trfer_trips.suffix trfer_suffix, \n"
            "    transfer.airp_arv trfer_airp_arv, \n"
            "    trfer_trips.scd trfer_scd, \n";
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
        "    pax_grp.class, "
        "    pax_grp.status, "
        "    bag2.bag_type, "
        "    bag2.rfisc, "
        "    bag2.num bag_num, "
        "    bag2.amount, "
        "    bag2.weight, "
        "    bag2.pr_liab_limit, "
        "    bag2.to_ramp "
        "from "
        "    pax_grp, "
        "    points, "
        "    bag2, "
        "    halls2, "
        "    pax ";
    if(rpt_params.pr_trfer)
        SQLText += ", transfer, trfer_trips ";
    SQLText +=
        "where "
        "    points.pr_del>=0 AND "
        "    pax_grp.point_dep = :point_id and "
        "    pax_grp.point_arv = points.point_id and "
        "    pax_grp.grp_id = bag2.grp_id and ";

    if (rpt_params.pr_brd)
      SQLText +=
          "    ckin.bag_pool_boarded(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)<>0 and ";
    else
      SQLText +=
          "    ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 and ";
    SQLText +=
        "    bag2.pr_cabin = 0 and "
        "    bag2.hall = halls2.id(+) and "
        "    ckin.get_bag_pool_pax_id(bag2.grp_id, bag2.bag_pool_num) = pax.pax_id(+) ";
    if(!rpt_params.airp_arv.empty()) {
        SQLText += " and pax_grp.airp_arv = :target ";
        Qry.CreateVariable("target", otString, rpt_params.airp_arv);
    }
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        SQLText +=
            "   and nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and bag2.hall IS NOT NULL ";
        Qry.CreateVariable("zone", otString, rpt_params.ckin_zone);
    }
    if(rpt_params.pr_trfer)
        SQLText +=
            " and pax_grp.grp_id=transfer.grp_id(+) and \n"
            " transfer.pr_final(+) <> 0 and \n"
            " transfer.point_id_trfer = trfer_trips.point_id(+) \n";

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
        bag_tag_row.last_target = REPORTS::get_last_target(Qry, rpt_params);
        bag_tag_row.point_num = Qry.FieldAsInteger("point_num");
        bag_tag_row.grp_id = cur_grp_id;
        bag_tag_row.airp_arv = Qry.FieldAsString("airp_arv");
        if(not Qry.FieldIsNULL("bag_type"))
            bag_tag_row.bag_type = Qry.FieldAsInteger("bag_type");
        bag_tag_row.to_ramp = Qry.FieldAsInteger("to_ramp");
        if(bag_tag_row.to_ramp)
            bag_tag_row.to_ramp_str = getLocaleText("У ТРАПА", rpt_params.GetLang());

        string class_code = Qry.FieldAsString("class");
        bag_tag_row.rfisc = Qry.FieldAsString("rfisc");
        bag_names.get(class_code, bag_tag_row, rpt_params);
        if(Qry.FieldIsNULL("class")) {
            if((string)Qry.FieldAsString("status") == "E") {
                bag_tag_row.class_priority = 50;
                bag_tag_row.class_code = "1";
                bag_tag_row.class_name = getLocaleText("Баг. экипажа", rpt_params.GetLang());
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

        TQuery tagsQry(&OraSession);
        tagsQry.SQLText =
            "select "
            "   bag_tags.tag_type, "
            "   bag_tags.color, "
            "   to_char(bag_tags.no) no "
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
            // ищем непривязанные бирки для каждой группы
            TQuery tagsQry(&OraSession);
            string SQLText =
                "select "
                "   bag_tags.tag_type, "
                "   bag_tags.color, "
                "   to_char(bag_tags.no) no "
                "from ";
            if(rpt_params.ckin_zone != ALL_CKIN_ZONES)
                SQLText += " halls2, pax_grp, ";
            SQLText +=
                "   bag_tags "
                "where "
                "   bag_tags.grp_id = :grp_id and "
                "   bag_tags.bag_num is null";
            if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
                SQLText +=
                    "   and nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and pax_grp.hall IS NOT NULL and "
                    "   pax_grp.grp_id = bag_tags.grp_id and "
                    "   pax_grp.hall = halls2.id(+) ";
                tagsQry.CreateVariable("zone", otString, rpt_params.ckin_zone);
            }
            tagsQry.SQLText = SQLText;
            tagsQry.CreateVariable("grp_id", otInteger, cur_grp_id);
            tagsQry.Execute();
            for(; !tagsQry.Eof; tagsQry.Next()) {
                bag_tags.push_back(bag_tag_row);
                bag_tags.back().bag_num = -1;
                bag_tags.back().amount = 0;
                bag_tags.back().weight = 0;
                bag_tags.back().bag_name_priority = -1;
                bag_tags.back().bag_name = "";
                bag_tags.back().tag_type = tagsQry.FieldAsString("tag_type");
                bag_tags.back().color = tagsQry.FieldAsString("color");
                bag_tags.back().no = tagsQry.FieldAsFloat("no");
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
                bm_table.back().class_name = getLocaleText("Несопровождаемый багаж", rpt_params.GetLang());
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
    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "TotPcs", TotAmount);
    NewTextChild(variablesNode, "TotWeight", TotWeight);
    NewTextChild(variablesNode, "Tot", vs_number(TotAmount, rpt_params.IsInter()));
    Qry.Clear();
    SQLText =
        "select "
        "  sum(bag2.amount) amount, "
        "  sum(bag2.weight) weight "
        "from "
        "  pax_grp, "
        "  bag2, "
        "  transfer ";
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES)
        SQLText += ", halls2 ";
    SQLText +=
        "where "
        "  pax_grp.point_dep = :point_id and "
        "  pax_grp.grp_id = bag2.grp_id and "
        "  pax_grp.grp_id = transfer.grp_id and "
        "  pax_grp.status NOT IN ('E') and "
        "  ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 and "
        "  bag2.pr_cabin = 0 and "
        "  transfer.pr_final <> 0 ";
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        SQLText +=
            "   and bag2.hall = halls2.id(+) "
            "   and nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and bag2.hall IS NOT NULL ";
        Qry.CreateVariable("zone", otString, rpt_params.ckin_zone);
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();
    int trfer_amount = 0;
    int trfer_weight = 0;
    if(not Qry.Eof) {
        trfer_amount = Qry.FieldAsInteger("amount");
        trfer_weight = Qry.FieldAsInteger("weight");
    }
    NewTextChild(variablesNode, "TotTrferPcs", trfer_amount);
    NewTextChild(variablesNode, "TotTrferWeight", trfer_weight);
    Qry.Clear();
    Qry.SQLText =
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
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    ProgTrace(TRACE5, "SQLText: %s", Qry.SQLText.SQLText());
    Qry.Execute();
    if(Qry.Eof) throw Exception("RunBM: variables fetch failed for point_id " + IntToString(rpt_params.point_id));

    TElemFmt airline_fmt = (TElemFmt)Qry.FieldAsInteger("airline_fmt");
    TElemFmt suffix_fmt = (TElemFmt)Qry.FieldAsInteger("suffix_fmt");
    TElemFmt craft_fmt = (TElemFmt)Qry.FieldAsInteger("craft_fmt");

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
            airline = Qry.FieldAsString("airline");
            flt_no = Qry.FieldAsInteger("flt_no");
            suffix = Qry.FieldAsString("suffix");
        }
    } else {
        airline = rpt_params.mkt_flt.airline;
        flt_no = rpt_params.mkt_flt.flt_no;
        suffix = rpt_params.mkt_flt.suffix;
    }
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(Qry.FieldAsString("airp"));

    //    TCrafts crafts;

    NewTextChild(variablesNode, "own_airp_name", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, rpt_params.dup_lang())), rpt_params.dup_lang()));
    NewTextChild(variablesNode, "own_airp_name_lat", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, AstraLocale::LANG_EN)), AstraLocale::LANG_EN));
    NewTextChild(variablesNode, "airp_dep_name", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong));
    NewTextChild(variablesNode, "airline_name", rpt_params.ElemIdToReportElem(etAirline, airline, efmtNameLong));
    ostringstream flt;
    flt
        << rpt_params.ElemIdToReportElem(etAirline, airline, airline_fmt)
        << setw(3) << setfill('0') << flt_no
        << rpt_params.ElemIdToReportElem(etSuffix, suffix, suffix_fmt);
    NewTextChild(variablesNode, "flt", flt.str());
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", rpt_params.ElemIdToReportElem(etCraft, craft, craft_fmt));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
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
    string pr_brd_pax_str = (string)"Номера багажных бирок " + (rpt_params.pr_brd ? "(посаж)" : "(зарег)");
    NewTextChild(variablesNode, "pr_brd_pax", getLocaleText(pr_brd_pax_str, rpt_params.dup_lang()));
    NewTextChild(variablesNode, "pr_brd_pax_lat", getLocaleText(pr_brd_pax_str, AstraLocale::LANG_EN));
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        NewTextChild(variablesNode, "zone", get_hall_list(airp, rpt_params));
    } else
        NewTextChild(variablesNode, "zone"); // пустой тег - нет детализации по залу
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    STAT::set_variables(resNode, rpt_params.GetLang());
    trip_rpt_person(resNode, rpt_params);
    TDateTime takeoff = NoExists;
    if(not Qry.FieldIsNULL("act_out"))
        takeoff = UTCToLocal(Qry.FieldAsDateTime("act_out"), tz_region);
    NewTextChild(variablesNode, "takeoff", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm.yy hh:nn")));
    NewTextChild(variablesNode, "takeoff_date", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm")));
    NewTextChild(variablesNode, "takeoff_time", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "hh:nn")));
}

void PTMBTMTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (rpt_params.rpt_type==rtPTMTXT)
    REPORTS::PTM(rpt_params, reqNode, resNode);
  else
    BTM(rpt_params, reqNode, resNode);

  xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
  xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);

  string str;
  ostringstream s;
  //текстовый формат
  int page_width=75;
  //специально вводим для кириллических символов, так как в терминале при экспорте проблемы
  //максимальная длина строки при экспорте в байтах! не должна превышать ~147 (65 рус + 15 лат)
  int max_symb_count= rpt_params.IsInter() ? page_width : 60;
  NewTextChild(variablesNode, "page_width", page_width);
  NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
  if(STAT::bad_client_img_version())
      NewTextChild(variablesNode, "doc_cap_test", " ");

  s.str("");
  s << get_test_str(page_width, rpt_params.GetLang());
  NewTextChild(variablesNode, "test_str", s.str());


  s.str("");
  if (rpt_params.rpt_type==rtPTMTXT)
  {
      ProgTrace(TRACE5, "'%s'", NodeAsString((rpt_params.IsInter() ? "ptm_lat" : "ptm"), variablesNode)); //!!!
      str.assign(NodeAsString((rpt_params.IsInter() ? "ptm_lat" : "ptm"), variablesNode));
  }
  else
    str.assign(getLocaleText("БАГАЖНАЯ ВЕДОМОСТЬ", rpt_params.GetLang()));

  s << setfill(' ')
    << str
    << right << setw(page_width-str.size())
    << string(NodeAsString((rpt_params.IsInter() ? "own_airp_name_lat" : "own_airp_name"),variablesNode)).substr(0,max_symb_count-str.size());
  NewTextChild(variablesNode, "page_header_top", s.str());


  s.str("");
  str.assign(getLocaleText("Владелец или Оператор: ", rpt_params.GetLang()));
  s << left
    << str
    << string(NodeAsString("airline_name",variablesNode)).substr(0,max_symb_count-str.size()) << endl
    << setw(10) << getLocaleText("№ рейса", rpt_params.GetLang());
  if (rpt_params.IsInter())
    s << setw(19) << "Aircraft";
  else
    s << setw(9)  << "№ ВС"
      << setw(10) << "ТипВС Ст. ";

  if (!NodeIsNULL("airp_arv_name",variablesNode))
    s << setw(15) << getLocaleText("А/п вылета", rpt_params.GetLang())
      << setw(20) << getLocaleText("А/п назначения", rpt_params.GetLang());
  else
    s << setw(35) << getLocaleText("А/п вылета", rpt_params.GetLang());
  s << setw(6)  << getLocaleText("Дата", rpt_params.GetLang())
    << setw(5)  << getLocaleText("Время", rpt_params.GetLang()) << endl;

  s << setw(10) << NodeAsString("flt",variablesNode)
    << setw(11) << NodeAsString("bort",variablesNode)
    << setw(4)  << NodeAsString("craft",variablesNode)
    << setw(4)  << NodeAsString("park",variablesNode);

  if (!NodeIsNULL("airp_arv_name",variablesNode))
    s << setw(15) << string(NodeAsString("airp_dep_name",variablesNode)).substr(0,20-1)
      << setw(20) << string(NodeAsString("airp_arv_name",variablesNode)).substr(0,20-1);
  else
    s << setw(35) << string(NodeAsString("airp_dep_name",variablesNode)).substr(0,40-1);

  s << setw(6) << NodeAsString("scd_date",variablesNode)
    << setw(5) << NodeAsString("scd_time",variablesNode);
  string departure = NodeAsString("takeoff", variablesNode);
  if(not departure.empty())
      s << endl << getLocaleText("Вылет", rpt_params.GetLang()) << ": " << departure;
  NewTextChild(variablesNode, "page_header_center", s.str() );

  s.str("");
  str.assign(NodeAsString((rpt_params.IsInter()?"pr_brd_pax_lat":"pr_brd_pax"),variablesNode));
  if (!NodeIsNULL("zone",variablesNode))
  {
    unsigned int zone_len=max_symb_count-str.size()-1;
    string zone;
    zone.assign(getLocaleText("CAP.DOC.ZONE", rpt_params.GetLang()) + ": ");
    zone.append(NodeAsString("zone",variablesNode));
    if (zone_len<zone.size())
      s << str << right << setw(page_width-str.size()) << zone.substr(0,zone_len-3).append("...") << endl;
    else
      s << str << right << setw(page_width-str.size()) << zone << endl;
  }
  else
    s << str << endl;

  if (rpt_params.rpt_type==rtPTMTXT)
    s << left
      << setw(4)  << (getLocaleText("CAP.DOC.REG", rpt_params.GetLang()))
      << setw(rpt_params.IsInter()?13:14) << (getLocaleText("Фамилия", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("Пол", rpt_params.GetLang()))
      << setw(rpt_params.IsInter()?4:3)   << (getLocaleText("Кл", rpt_params.GetLang()))
      << setw(5)  << (getLocaleText("CAP.DOC.SEAT_NO", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("РБ", rpt_params.GetLang()))
      << setw(4)  << (getLocaleText("РМ", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("CAP.DOC.BAG", rpt_params.GetLang()))
      << setw(6)  << (getLocaleText("Р/кл", rpt_params.GetLang()))
      << setw(15) << (getLocaleText("CAP.DOC.BAG_TAG_NOS", rpt_params.GetLang()))
      << setw(9)  << (getLocaleText("Ремарки", rpt_params.GetLang()));
  else
    s << left
      << setw(29) << (getLocaleText("Номера багажных бирок", rpt_params.GetLang()))
      << setw(10) << (getLocaleText("Цвет", rpt_params.GetLang()))
      << setw(5)  << (getLocaleText("Мест", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("Вес", rpt_params.GetLang()))
      << setw(8)  << (getLocaleText("№ Конт.", rpt_params.GetLang()))
      << setw(10) << (getLocaleText("CAP.DOC.HOLD", rpt_params.GetLang()))
      << setw(11) << (getLocaleText("Отсек", rpt_params.GetLang()));

  NewTextChild(variablesNode, "page_header_bottom", s.str() );

  if (rpt_params.rpt_type==rtPTMTXT)
  {
    s.str("");
    s
        << setw(17)
        << (getLocaleText("Всего в классе", rpt_params.GetLang()))
        << setw(9)
        << "M/F"
        << setw(4)
        << getLocaleText("Крс", rpt_params.GetLang())
        << right
        << setw(3)
        << getLocaleText("РБ", rpt_params.GetLang()) << " "
        << setw(3)
        << getLocaleText("РМ", rpt_params.GetLang()) << " "
        << left
        << setw(7)
        << getLocaleText("Баг.", rpt_params.GetLang())
        << right
        << setw(5)
        << getLocaleText("Р/кл", rpt_params.GetLang())
        << setw(7) << " " // заполнение 7 пробелов (обязат. д.б. вкл. флаг right, см. выше)
        << "XCR DHC MOS JMP"
        << endl
        // Здесь видно, что Багаж и р/кл (%2u/%-4u%5u) расположены вплотную, что не есть хорошо.
        << "%-16s %-7s  %3u %3u %3u %2u/%-4u%5u       %3u %3u %3u %3u";
    NewTextChild(variablesNode, "total_in_class_fmt", s.str());

    s.str("");
    if (!NodeIsNULL("airp_arv_name",variablesNode))
    {
      str.assign(NodeAsString("airp_dep_name",variablesNode)).append("-");
      str.append(NodeAsString("airp_arv_name",variablesNode));

      s << left
        << setw(6) << (getLocaleText("Всего", rpt_params.GetLang()))
        << setw(50) << str.substr(0,50-1)
        << (getLocaleText("Подпись", rpt_params.GetLang())) << endl;
    }
    else
      s << left
        << (getLocaleText("Всего", rpt_params.GetLang())) << endl;

    s << setw(7) << (getLocaleText("Кресел", rpt_params.GetLang()))
      << setw(8) << (getLocaleText("ВЗ/Ж", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("РБ", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("РМ", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("Мест", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("Вес", rpt_params.GetLang()))
      << setw(7) << (getLocaleText("Р/кл", rpt_params.GetLang()))
      << setw(8) << (getLocaleText("CAP.DOC.EX_BAG", rpt_params.GetLang()))
      << setw(16) << "XCR DHC MOS JMP" << endl
      << "%-6u %-7s %-6u %-6u %-6u %-6u %-6u %-6s  %-3u %-3u %-3u %-3u" << endl
      << (getLocaleText("Подпись", rpt_params.GetLang())) << endl
      << setw(30) << string(NodeAsString("pts_agent", variablesNode)).substr(0, 30) << endl
      << (getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));

    NewTextChild(variablesNode, "page_footer_top", s.str() );


    xmlNodePtr dataSetNode = NodeAsNode("v_pm_trfer", dataSetsNode);
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    for(;rowNode!=NULL;rowNode=rowNode->next)
    {
      str=NodeAsString("airp_arv_name",rowNode);
      ReplaceTextChild(rowNode,"airp_arv_name",str.substr(0,max_symb_count));
      if (!NodeIsNULL("last_target",rowNode))
      {
        str.assign(getLocaleText("ДО", rpt_params.GetLang()) + ": ").append(NodeAsString("last_target",rowNode));
        ReplaceTextChild(rowNode,"last_target",str.substr(0,max_symb_count));
      };

      //рабиваем фамилию, бирки, ремарки
      SeparateString(NodeAsString("full_name",rowNode),13,rows);
      fields["full_name"]=rows;
      SeparateString(NodeAsString("tags",rowNode),15,rows);
      fields["tags"]=rows;
      SeparateString(NodeAsString("remarks",rowNode),9,rows);
      fields["remarks"]=rows;

      string gender = NodeAsString("gender",rowNode);

      row=0;
      string pers_type=NodeAsString("pers_type",rowNode);
      s.str("");
      do
      {
        if (row!=0) s << endl;
        s << right << setw(3) << (row==0?NodeAsString("reg_no",rowNode):"") << " "
          << left << setw(13) << (!fields["full_name"].empty()?*(fields["full_name"].begin()):"") << " "
          << left <<  setw(4) << (row==0?gender:"")
          << left <<  setw(3) << (row==0?NodeAsString("class",rowNode):"")
          << right << setw(4) << (row==0?NodeAsString("seat_no",rowNode):"") << " "
          << left <<  setw(4) << (row==0&&pers_type=="CHD"?" X ":"")
          << left <<  setw(4) << (row==0&&pers_type=="INF"?" X ":"");
        if (row!=0 ||
            (NodeAsInteger("bag_amount",rowNode)==0 &&
            NodeAsInteger("bag_weight",rowNode)==0))
          s << setw(7) << "";
        else
          s << right << setw(2) << NodeAsInteger("bag_amount",rowNode) << "/"
            << left << setw(4) << NodeAsInteger("bag_weight",rowNode);
        if (row!=0 ||
            NodeAsInteger("rk_weight",rowNode)==0)
          s << setw(5) << "";
        else
          s << right << setw(4) << NodeAsInteger("rk_weight",rowNode) << " ";
        s << left << setw(15) << (!fields["tags"].empty()?*(fields["tags"].begin()):"") << " "
          << left << setw(9) << (!fields["remarks"].empty()?*(fields["remarks"].begin()):"") << " ";

        for(map< string, vector<string> >::iterator f=fields.begin();f!=fields.end();f++)
          if (!f->second.empty()) f->second.erase(f->second.begin());
        row++;
      }
      while(!fields["full_name"].empty() ||
            !fields["tags"].empty() ||
            !fields["remarks"].empty());

      NewTextChild(rowNode,"str",s.str());
    };

    for(int k=(int)rpt_params.pr_trfer;k>=0;k--)
    {
      s.str("");
      if (!rpt_params.pr_trfer)
        s << setw(15) << (getLocaleText("Всего багажа", rpt_params.GetLang()));
      else
      {
        if (k==0)
          s << setw(19) << (getLocaleText("Всего нетр. баг.", rpt_params.GetLang()));
        else
          s << setw(19) << (getLocaleText("Всего тр. баг.", rpt_params.GetLang()));
      };

      s << setw(7) << (getLocaleText("Кресел", rpt_params.GetLang()))
        << setw(8) << (getLocaleText("ВЗ/Ж", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("РБ", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("РМ", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("Мест", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("Вес", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("Р/кл", rpt_params.GetLang()))
        << setw(7) << (getLocaleText("CAP.DOC.EX_BAG", rpt_params.GetLang()));

      if (!rpt_params.pr_trfer)
        NewTextChild(variablesNode, "subreport_header", s.str() );
      else
      {
        if (k==0)
          NewTextChild(variablesNode, "subreport_header", s.str() );
        else
          NewTextChild(variablesNode, "subreport_header_trfer", s.str() );
      };
    };

    dataSetNode = NodeAsNode(rpt_params.pr_trfer ? "v_pm_trfer_total" : "v_pm_total", dataSetsNode);

    rowNode=dataSetNode->children;
    for(;rowNode!=NULL;rowNode=rowNode->next)
    {
      ostringstream adl_fem;
      adl_fem << NodeAsInteger("adl", rowNode) << '/' << NodeAsInteger("adl_f", rowNode);

      s.str("");
      s << setw(rpt_params.pr_trfer?19:15) << NodeAsString("class_name",rowNode)
        << setw(7) << NodeAsInteger("seats",rowNode)
        << setw(8) << adl_fem.str()
        << setw(7) << NodeAsInteger("chd",rowNode)
        << setw(7) << NodeAsInteger("inf",rowNode)
        << setw(7) << NodeAsInteger("bag_amount",rowNode)
        << setw(7) << NodeAsInteger("bag_weight",rowNode)
        << setw(7) << NodeAsInteger("rk_weight",rowNode)
        << setw(7) << NodeAsString("excess",rowNode) << endl
        << "XCR/DHC/MOS/JMP: "
        << NodeAsInteger("xcr",rowNode) << "/"
        << NodeAsInteger("dhc",rowNode) << "/"
        << NodeAsInteger("mos",rowNode) << "/"
        << NodeAsInteger("jmp",rowNode);

      NewTextChild(rowNode,"str",s.str());
    };
  }
  else
  {
    s.str("");
    s << "%-39s%4u %6u";
    NewTextChild(variablesNode, "total_in_class_fmt", s.str());

    xmlNodePtr dataSetNode = NodeAsNode(rpt_params.pr_trfer ? "v_bm_trfer" : "v_bm", dataSetsNode);
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    for(;rowNode!=NULL;rowNode=rowNode->next)
    {
      str=NodeAsString("airp_arv_name",rowNode);
      ReplaceTextChild(rowNode,"airp_arv_name",str.substr(0,max_symb_count));
      if (!NodeIsNULL("last_target",rowNode))
      {
        str.assign(getLocaleText("ДО", rpt_params.GetLang()) + ": ").append(NodeAsString("last_target",rowNode));
        ReplaceTextChild(rowNode,"last_target",str.substr(0,max_symb_count));
      };

      //разбиваем диапазоны бирок, цвет
      int offset = 2;
      if(not NodeIsNULL("bag_name", rowNode))
          offset += 2;
      if(not NodeIsNULL("to_ramp", rowNode))
          offset += 2;

      SeparateString(NodeAsString("birk_range",rowNode),28 - offset,rows);
      fields["birk_range"]=rows;
      SeparateString(NodeAsString("color",rowNode),9,rows);
      fields["color"]=rows;

      row=0;
      s.str("");
      do
      {
        if (row!=0) s << endl;
        s << setw(offset) << "" //отступ
          << left << setw(28 - offset) << (!fields["birk_range"].empty()?*(fields["birk_range"].begin()):"") << " "
          << left << setw(9) << (!fields["color"].empty()?*(fields["color"].begin()):"") << " "
          << right << setw(4) << (row==0?NodeAsString("num",rowNode):"") << " ";

        for(map< string, vector<string> >::iterator f=fields.begin();f!=fields.end();f++)
          if (!f->second.empty()) f->second.erase(f->second.begin());
        row++;
      }
      while(!fields["birk_range"].empty() ||
            !fields["color"].empty());

      NewTextChild(rowNode,"str",s.str());
    };

    if (rpt_params.pr_trfer)
    {
      for(int k=(int)rpt_params.pr_trfer;k>=0;k--)
      {
        s.str("");
        s << left;
        if (k==0)
          s << setw(39) << (getLocaleText("Всего багажа, исключая трансферный", rpt_params.GetLang()));
        else
          s << setw(39) << (getLocaleText("Всего трансферного багажа", rpt_params.GetLang()));
        s << "%4u %6u";
        if (k==0)
          NewTextChild(variablesNode, "total_not_trfer_fmt", s.str() );
        else
          NewTextChild(variablesNode, "total_trfer_fmt", s.str() );
      };

      s.str("");
      s << setw(39) << (getLocaleText("Всего багажа", rpt_params.GetLang()))
        << "%4u %6u" << endl
        << setw(39) << (getLocaleText("Трансферного багажа", rpt_params.GetLang()))
        << "%4u %6u";
      NewTextChild(variablesNode, "report_footer", s.str() );
    };

    s.str("");
    s << (getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    NewTextChild(variablesNode, "page_footer_top", s.str() );



    s.str("");
    if (!NodeIsNULL("airp_arv_name",variablesNode))
    {
      str.assign(NodeAsString("airp_dep_name",variablesNode)).append("-");
      str.append(NodeAsString("airp_arv_name",variablesNode));

      s << left
        << setw(6) << (getLocaleText("Всего", rpt_params.GetLang()))
        << setw(50) << str.substr(0,50-1)
        << (getLocaleText("Подпись агента СОПП", rpt_params.GetLang())) << endl;
    }
    else
      s << left
        << setw(56) << (getLocaleText("Всего", rpt_params.GetLang()))
        << (getLocaleText("Подпись агента СОПП", rpt_params.GetLang())) << endl;

    s << setw(6)  << (getLocaleText("Мест", rpt_params.GetLang()))
      << setw(7)  << (getLocaleText("Вес", rpt_params.GetLang()))
      << setw(43) << (getLocaleText("Количество мест прописью", rpt_params.GetLang()))
      << setw(24) << string(NodeAsString("pts_agent", variablesNode)).substr(0, 24) << endl;

    SeparateString(NodeAsString("Tot",variablesNode),42,rows);
    row=0;
    do
    {
      if (row!=0) s << endl;
      s << setw(6)  << (row==0?NodeAsString("TotPcs",variablesNode):"")
        << setw(7)  << (row==0?NodeAsString("TotWeight",variablesNode):"")
        << setw(42) << (!rows.empty()?*(rows.begin()):"");
      if (!rows.empty()) rows.erase(rows.begin());
      row++;
    }
    while(!rows.empty());
    NewTextChild(variablesNode,"report_summary",s.str());
  };
};

void EMD(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtEMDTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("EMD", reqNode, resNode);
    std::map<int, std::vector<std::string> > tab;
    size_t total = 0;
    EMDReport(rpt_params.point_id, tab, total);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_emd");

    if(tab.size() != 0) {
        for( std::map<int, std::vector<std::string> >::iterator i = tab.begin(); i != tab.end(); i++) {
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            NewTextChild(rowNode, "reg_no", i->first);
            const vector<string> &fields = i->second;
            for(size_t j = 0; j < fields.size(); j++) {
                switch(j) {
                    case 0:
                        NewTextChild(rowNode, "full_name", transliter(fields[j], 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
                        break;
                    case 1:
                        NewTextChild(rowNode, "etkt_no", fields[j]);
                        break;
                    case 2:
                        NewTextChild(rowNode, "emd_no", fields[j]);
                        break;
                }
            }
        }
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no");
        NewTextChild(rowNode, "full_name", getLocaleText("Итого:", rpt_params.GetLang()));
        NewTextChild(rowNode, "etkt_no");
        NewTextChild(rowNode, "emd_no", (int)total);
    }

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.EMD",
                LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void EMDTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    EMD(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    ostringstream s;
    s.str("");
    s << NodeAsString("caption", variablesNode);
    string str = s.str().substr(0, max_symb_count);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << getLocaleText("№", rpt_params.GetLang()) << " "
        << left
        << setw(37) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(20)  << getLocaleText("№ Билета", rpt_params.GetLang())
        << setw(20)  << getLocaleText("№ EMD", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_emd", dataSetsNode);
    xmlNodeSetName(dataSetNode, "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("full_name", rowNode), 36, rows);
        fields["full_name"]=rows;

        SeparateString(NodeAsString("emd_no", rowNode), 19, rows);
        fields["emd_no"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(36) << (!fields["full_name"].empty() ? *(fields["full_name"].begin()) : "") << col_sym
                << left <<  setw(19) << (row == 0 ? NodeAsString("etkt_no", rowNode) : "") << " " << col_sym
                << left << setw(19) << (!fields["emd_no"].empty() ? *(fields["emd_no"].begin()) : "");

            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["surname"].empty() or
                !fields["emd_no"].empty()
                );
        NewTextChild(rowNode,"str",s.str());
    }
}

void WB_MSG(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("WB_msg", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "msg");

    TCachedQuery Qry("select id, time_receive from wb_msg where point_id = :point_id and msg_type = :msg_type order by id desc",
            QParams()
            << QParam("point_id", otInteger, rpt_params.point_id)
            << QParam("msg_type", otString, EncodeRptType(rpt_params.rpt_type))
            );
    Qry.get().Execute();
    if(Qry.get().Eof)
        throw UserException("MSG.NOT_DATA");

    xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
    int id = Qry.get().FieldAsInteger("id");
    // TDateTime time_receive = Qry.get().FieldAsDateTime("time_receive");
    TCachedQuery txtQry("select text from wb_msg_text where id = :id order by page_no",
            QParams() << QParam("id", otInteger, id));
    txtQry.get().Execute();
    string text;
    for(; not txtQry.get().Eof; txtQry.get().Next())
        text += txtQry.get().FieldAsString("text");
    NewTextChild(rowNode, "text", text);

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "caption", EncodeRptType(rpt_params.rpt_type));
}

void CRS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtCRSTXT or rpt_params.rpt_type == rtCRSUNREGTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form((rpt_params.rpt_type == rtBDOCS ? "bdocs" : "crs"), reqNode, resNode);
    bool pr_unreg = rpt_params.rpt_type == rtCRSUNREG or rpt_params.rpt_type == rtCRSUNREGTXT;
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT "
        "      crs_pax.pax_id, "
        "      crs_pax.surname||' '||crs_pax.name family ";
    if(rpt_params.rpt_type != rtBDOCS) {
        SQLText +=
            "      , tlg_binding.point_id_spp AS point_id, "
            "      crs_pax.pers_type, "
            "      crs_pnr.class, "
            "      salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'_seats',rownum) AS seat_no, "
            "      crs_pnr.airp_arv AS target, "
            "      crs_pnr.airp_arv_final AS last_target, "
            "      crs_pnr.pnr_id, "
            "      report.get_TKNO(crs_pax.pax_id) ticket_no, "
            "      report.get_PSPT(crs_pax.pax_id, 1, :lang) AS document ";
        Qry.CreateVariable("lang", otString, rpt_params.GetLang());
    }
    SQLText +=
        "FROM crs_pnr,tlg_binding,crs_pax ";
    if(pr_unreg)
        SQLText += " , pax ";
    SQLText +=
        "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
        "      crs_pnr.system='CRS' AND "
        "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pax.pr_del=0 and "
        "      tlg_binding.point_id_spp = :point_id ";
    if(pr_unreg)
        SQLText +=
            "    and crs_pax.pax_id = pax.pax_id(+) and "
            "    pax.pax_id is null ";
    SQLText +=
        "order by ";
    switch(rpt_params.sort) {
        case stServiceCode:
        case stRegNo:
        case stSurname:
            SQLText += " family ";
            break;
        case stSeatNo:
            if(rpt_params.rpt_type != rtBDOCS)
                SQLText += " seat_no,";
            SQLText += " family ";
            break;
    }
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_crs");

    TQuery docsQry(&OraSession);
    docsQry.SQLText = "select * from crs_pax_doc where pax_id = :pax_id and rem_code = 'DOCS'";
    docsQry.DeclareVariable("pax_id", otInteger);
    //ремарки пассажиров

    TRemGrp rem_grp;
    if(rpt_params.rpt_type != rtBDOCS)
      rem_grp.Load(retPNL_SEL, rpt_params.point_id);
    for(; !Qry.Eof; Qry.Next()) {
        int pax_id=Qry.FieldAsInteger("pax_id");
        if(rpt_params.rpt_type == rtBDOCS) {
            docsQry.SetVariable("pax_id", pax_id);
            docsQry.Execute();
            if (!docsQry.Eof)
            {
                if (
                        !docsQry.FieldIsNULL("type") &&
                        !docsQry.FieldIsNULL("issue_country") &&
                        !docsQry.FieldIsNULL("no") &&
                        !docsQry.FieldIsNULL("nationality") &&
                        !docsQry.FieldIsNULL("birth_date") &&
                        !docsQry.FieldIsNULL("gender") &&
                        !docsQry.FieldIsNULL("expiry_date") &&
                        !docsQry.FieldIsNULL("surname") &&
                        !docsQry.FieldIsNULL("first_name")
                   )
                    continue;

                xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
                NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
                NewTextChild(rowNode, "type", rpt_params.ElemIdToReportElem(etPaxDocType, docsQry.FieldAsString("type"), efmtCodeNative));
                NewTextChild(rowNode, "issue_country", rpt_params.ElemIdToReportElem(etPaxDocCountry, docsQry.FieldAsString("issue_country"), efmtCodeNative));
                NewTextChild(rowNode, "no", docsQry.FieldAsString("no"));
                NewTextChild(rowNode, "nationality", rpt_params.ElemIdToReportElem(etPaxDocCountry, docsQry.FieldAsString("nationality"), efmtCodeNative));
                if (!docsQry.FieldIsNULL("birth_date"))
                    NewTextChild(rowNode, "birth_date", DateTimeToStr(docsQry.FieldAsDateTime("birth_date"), "dd.mm.yyyy"));
                NewTextChild(rowNode, "gender", rpt_params.ElemIdToReportElem(etGenderType, docsQry.FieldAsString("gender"), efmtCodeNative));
                if (!docsQry.FieldIsNULL("expiry_date"))
                    NewTextChild(rowNode, "expiry_date", DateTimeToStr(docsQry.FieldAsDateTime("expiry_date"), "dd.mm.yyyy"));
                NewTextChild(rowNode, "surname", docsQry.FieldAsString("surname"));
                NewTextChild(rowNode, "first_name", docsQry.FieldAsString("first_name"));
                NewTextChild(rowNode, "second_name", docsQry.FieldAsString("second_name"));
            } else {
                xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
                NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            }
        } else {
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            NewTextChild(rowNode, "family", transliter(Qry.FieldAsString("family"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger("point_id"));
            string pnr_addr=TPnrAddrs().firstAddrByPnrId(Qry.FieldAsInteger("pnr_id"), TPnrAddrInfo::AddrOnly); //пока не надо выводить компанию, может быть потом...
            NewTextChild(rowNode, "pnr_ref", pnr_addr);
            NewTextChild(rowNode, "pers_type", rpt_params.ElemIdToReportElem(etPersType, Qry.FieldAsString("pers_type"), efmtCodeNative));
            NewTextChild(rowNode, "class", rpt_params.ElemIdToReportElem(etClass, Qry.FieldAsString("class"), efmtCodeNative));
            NewTextChild(rowNode, "seat_no", Qry.FieldAsString("seat_no"));
            NewTextChild(rowNode, "target", rpt_params.ElemIdToReportElem(etAirp, Qry.FieldAsString("target"), efmtCodeNative));
            string last_target = Qry.FieldAsString("last_target");
            TElemFmt fmt;
            string last_target_id = ElemToElemId(etAirp, last_target, fmt);
            if(not last_target_id.empty())
                last_target = rpt_params.ElemIdToReportElem(etAirp, last_target_id, efmtCodeNative);

            NewTextChild(rowNode, "last_target", last_target);
            NewTextChild(rowNode, "ticket_no", Qry.FieldAsString("ticket_no"));
            NewTextChild(rowNode, "document", Qry.FieldAsString("document"));
            NewTextChild(rowNode, "remarks", GetCrsRemarkStr(rem_grp, pax_id));
        }
    }

    // Теперь переменные отчета
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    if(pr_unreg)
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.CRSUNREG",
                    LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    else
        NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.CRS",
                    LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
}

void CRSTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    CRS(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    int max_symb_count=rpt_params.IsInter()?page_width:60;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    ostringstream s;
    vector<string> rows;
    string str;
    SeparateString(NodeAsString("caption", variablesNode), max_symb_count, rows);
    s.str("");
    for(vector<string>::iterator iv = rows.begin(); iv != rows.end(); iv++) {
        if(iv != rows.begin())
            s << endl;
        s << right << setw(((page_width - iv->size()) / 2) + iv->size()) << *iv;
    }
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << right << setw(3)  << getLocaleText("№", rpt_params.GetLang()) << " "
        << left
        << setw(7)  << "PNR"
        << setw(22) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(5)  << getLocaleText("Пас", rpt_params.GetLang())
        << setw(3) << getLocaleText("Кл", rpt_params.GetLang())
        << setw(8)  << getLocaleText("№ м", rpt_params.GetLang())
        << setw(4)  << getLocaleText("CAP.DOC.AIRP_ARV", rpt_params.GetLang())
        << setw(7)  << getLocaleText("CAP.DOC.TO", rpt_params.GetLang())
        << setw(10) << getLocaleText("Билет", rpt_params.GetLang())
        << setw(10) << getLocaleText("Документ", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_crs", dataSetsNode);
    xmlNodeSetName(dataSetNode, "table");
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    int row_num = 1;
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("pnr_ref", rowNode), 6, rows);
        fields["pnrs"]=rows;

        SeparateString(NodeAsString("family", rowNode), 21, rows);
        fields["surname"]=rows;

        SeparateString(NodeAsString("ticket_no", rowNode), 9, rows);
        fields["tkts"]=rows;

        SeparateString(NodeAsString("document", rowNode), 10, rows);
        fields["docs"]=rows;


        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(3) << (row == 0 ? IntToString(row_num++) : "") << col_sym
                << left << setw(6) << (!fields["pnrs"].empty() ? *(fields["pnrs"].begin()) : "") << col_sym
                << left << setw(21) << (!fields["surname"].empty() ? *(fields["surname"].begin()) : "") << col_sym
                << right <<  setw(4) << (row == 0 ? NodeAsString("pers_type", rowNode, "ВЗ") : "") << col_sym
                << left << setw(2) << (row == 0 ? NodeAsString("class", rowNode) : "") << col_sym
                << left <<  setw(7) << (row == 0 ? NodeAsString("seat_no", rowNode, "") : "") << col_sym
                << left << setw(3) << (row == 0 ? NodeAsString("target", rowNode) : "") << col_sym
                << left << setw(6) << (row == 0 ? NodeAsString("last_target", rowNode) : "") << col_sym
                << left << setw(9) << (!fields["tkts"].empty() ? *(fields["tkts"].begin()) : "") << col_sym
                << left << setw(10) << (!fields["docs"].empty() ? *(fields["docs"].begin()) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(
                !fields["pnrs"].empty() ||
                !fields["surname"].empty() ||
                !fields["tkts"].empty() ||
                !fields["docs"].empty()
             );
        NewTextChild(rowNode,"str",s.str());
    }
}

void WEB(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    NewTextChild(reqNode, "client_type", rpt_params.client_type);
    EXAM(rpt_params, reqNode, resNode);
}

void WEBTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    NewTextChild(reqNode, "client_type", rpt_params.client_type);
    EXAMTXT(rpt_params, reqNode, resNode);
}

// VOUCHERS BEGIN

void VOUCHERS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("vouchers", reqNode, resNode);

    TCachedQuery Qry(
            "select "
            "    confirm_print.pax_id, "
            "    pax.surname||' '||pax.name AS full_name, "
            "    pax.pers_type, "
            "    pax.reg_no, "
            "    pax.ticket_no, "
            "    pax.coupon_no, "
            "    pax.ticket_rem, "
            "    pax.ticket_confirm, "
            "    pax.pers_type, "
            "    pax.seats, "
            "    confirm_print.voucher, "
            "    count(*) total "
            "from "
            "    pax_grp, "
            "    pax, "
            "    confirm_print "
            "where "
            "    pax_grp.point_dep = :point_id and "
            "    pax_grp.grp_id = pax.grp_id and "
            "    pax.pax_id = confirm_print.pax_id and "
            "    confirm_print.voucher is not null and "
            "    confirm_print.pr_print <> 0 "
            "group by "
            "    confirm_print.pax_id, "
            "    pax.surname||' '||pax.name, "
            "    pax.pers_type, "
            "    pax.reg_no, "
            "    pax.ticket_no, "
            "    pax.coupon_no, "
            "    pax.ticket_rem, "
            "    pax.ticket_confirm, "
            "    pax.pers_type, "
            "    pax.seats, "
            "    confirm_print.voucher "
            "order by "
            "    confirm_print.voucher, "
            "    pax.reg_no ",
            QParams()
            << QParam("point_id", otInteger, rpt_params.point_id));
    Qry.get().Execute();

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    // переменные отчёта
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // заголовок отчёта
    NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.VOUCHERS",
                LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());

    if(not Qry.get().Eof) {
        xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_vouchers");
        // строки отчёта

        map<string, int> totals;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            string voucher = Qry.get().FieldAsString("voucher");
            int reg_no = Qry.get().FieldAsInteger("reg_no");
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            string category = rpt_params.ElemIdToReportElem(etVoucherType, voucher, efmtNameLong);
            NewTextChild(rowNode, "category", category);
            NewTextChild(rowNode, "reg_no", reg_no);
            NewTextChild(rowNode, "fio", transliter(Qry.get().FieldAsString("full_name"), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
            NewTextChild(rowNode, "type", getLocaleText(Qry.get().FieldAsString("pers_type"), rpt_params.GetLang()));

            string ticket_no = Qry.get().FieldAsString("ticket_no");
            string coupon_no = Qry.get().FieldAsString("coupon_no");
            ostringstream ticket;
            if(not ticket_no.empty())
                ticket << ticket_no << (coupon_no.empty() ? "" : "/") << coupon_no;

            NewTextChild(rowNode, "tick_no", ticket.str());

            int quantity = Qry.get().FieldAsInteger("total");
            totals[category] += quantity;
            NewTextChild(rowNode, "quantity", quantity);

            set<string> rems;
            REPORT_PAX_REMS::get_rem_codes(Qry.get(), rpt_params.GetLang(), rems);
            ostringstream rems_list;
            for(set<string>::iterator i = rems.begin(); i != rems.end(); i++) {
                if(rems_list.str().size() != 0)
                    rems_list << " ";
                rems_list << *i;
            }
            NewTextChild(rowNode, "rem", rems_list.str());
        }
        dataSetNode = NewTextChild(dataSetsNode, "totals");
        int total_sum = 0;
        for(map<string, int>::iterator i = totals.begin(); i != totals.end(); i++) {
            xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
            ostringstream buf;
            buf << i->first << " " << i->second;
            total_sum += i->second;
            NewTextChild(rowNode, "item", buf.str());
        }
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        ostringstream buf;
        buf << getLocaleText("Всего:", rpt_params.GetLang()) << " " << total_sum;
        NewTextChild(rowNode, "item", buf.str());
    }
}

// VOUCHERS END

void SPPCentrovka(TDateTime date, xmlNodePtr resNode)
{
}

void SPPCargo(TDateTime date, xmlNodePtr resNode)
{
}

void SPPCex(TDateTime date, xmlNodePtr resNode)
{
}

void  DocsInterface::RunSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr node = reqNode->children;
    string name = NodeAsStringFast("name", node);
    get_new_report_form(name, reqNode, resNode);
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
}

void RESEAT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(rpt_params.rpt_type == rtRESEATTXT)
        get_compatible_report_form("docTxt", reqNode, resNode);
    else
        get_compatible_report_form("reseat", reqNode, resNode);

    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_reseat");
    // переменные отчёта
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // заголовок отчёта
    NewTextChild(variablesNode, "caption",
        getLocaleText("CAP.DOC.RESEAT", LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());

    map<bool,map <int,TSeatRanges> > seats; // true - старые, false - новые места
    SALONS2::getPaxSeatsWL(rpt_params.point_id, seats);

    // приведем seats к нормальному виду prepared_seats
    map<int, pair<string, string> > prepared_seats;

    // Пробег по паксам со старыми местами (в мэпе seats[true])
    for(map <int,TSeatRanges>::iterator old_seats = seats[true].begin();
            old_seats != seats[true].end(); old_seats++) {

        pair<string, string> &pair_seats = prepared_seats[old_seats->first];

        // Старые места выводим
        pair_seats.first = GetSeatRangeView(old_seats->second, "list", rpt_params.GetLang() != AstraLocale::LANG_RU);

        // Ищем, есть ли у пакса новые места (в мэпе seats[false])
        // Если есть, выводим
        map<int,TSeatRanges>::iterator new_seats = seats[false].find(old_seats->first);
        if(new_seats != seats[false].end())
            pair_seats.second = GetSeatRangeView(new_seats->second, "list", rpt_params.GetLang() != AstraLocale::LANG_RU);
    }

    struct TSortedPax {
        CheckIn::TSimplePaxItem pax;
        pair<string, string> seats;
    };

    map<int, TSortedPax> sorted_pax;
    for(map<int, pair<string, string> >::iterator i = prepared_seats.begin();
            i != prepared_seats.end(); i++) {
        CheckIn::TSimplePaxItem pax;
        pax.getByPaxId(i->first);
        sorted_pax[pax.reg_no].pax = pax;
        sorted_pax[pax.reg_no].seats = i->second;
    }

    // строки отчёта
    map<int, string> classes;
    for(map<int, TSortedPax>::iterator i = sorted_pax.begin();
            i != sorted_pax.end(); i++) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no", i->second.pax.reg_no);
        NewTextChild(rowNode, "full_name", transliter(i->second.pax.full_name(), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        NewTextChild(rowNode, "pr_brd", i->second.pax.pr_brd);
        NewTextChild(rowNode, "seats", i->second.pax.seats);
        NewTextChild(rowNode, "old_seat_no", i->second.seats.first);
        NewTextChild(rowNode, "new_seat_no", i->second.seats.second);

        map<int, string>::iterator i_cls = classes.find(i->second.pax.grp_id);
        if(i_cls == classes.end()) {
            CheckIn::TSimplePaxGrpItem grp;
            grp.getByGrpId(i->second.pax.grp_id);
            pair<map<int, string>::iterator, bool> ret;
            ret = classes.insert(make_pair(i->second.pax.grp_id, grp.cl));
            i_cls = ret.first;
        }
        NewTextChild(rowNode, "cls", rpt_params.ElemIdToReportElem(etClass, i_cls->second, efmtCodeNative));

        NewTextChild(rowNode, "document", CheckIn::GetPaxDocStr(NoExists, i->second.pax.id, false, rpt_params.GetLang()));

        ostringstream ticket_no;
        ticket_no << i->second.pax.tkn.no;
        if(i->second.pax.tkn.coupon != NoExists)
            ticket_no << "/" << i->second.pax.tkn.coupon;

        NewTextChild(rowNode, "ticket_no", ticket_no.str());
    }
}

void RESEATTXT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    RESEAT(rpt_params, reqNode, resNode);

    xmlNodePtr variablesNode=NodeAsNode("form_data/variables",resNode);
    xmlNodePtr dataSetsNode=NodeAsNode("form_data/datasets",resNode);
    int page_width=80;
    NewTextChild(variablesNode, "page_width", page_width);
    NewTextChild(variablesNode, "test_server", STAT::bad_client_img_version() ? 2 : get_test_server());
    if(STAT::bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "test_str", get_test_str(page_width, rpt_params.GetLang()));
    ostringstream s;
    s.str("");
    s << NodeAsString("caption", variablesNode);
    string str = s.str().substr(0, page_width);
    s.str("");
    s << right << setw(((page_width - str.size()) / 2) + str.size()) << str;
    NewTextChild(variablesNode, "page_header_top", s.str());
    s.str("");
    s
        << left
        << setw(3)  << getLocaleText("№", rpt_params.GetLang())
        << setw(26) << getLocaleText("Ф.И.О.", rpt_params.GetLang())
        << setw(4)  << getLocaleText("Пс", rpt_params.GetLang())
        << setw(3)  << (getLocaleText("Кл", rpt_params.GetLang()))
        << setw(4)  << (getLocaleText("Крс", rpt_params.GetLang()))
        << setw(6)  << (getLocaleText("Стар.", rpt_params.GetLang()))
        << setw(8)  << (getLocaleText("Нов.", rpt_params.GetLang()))
        << setw(11) << getLocaleText("Документ", rpt_params.GetLang())
        << setw(15) << getLocaleText("Билет", rpt_params.GetLang());
    NewTextChild(variablesNode, "page_header_bottom", s.str() );
    NewTextChild(variablesNode, "page_footer_top",
            getLocaleText("CAP.ISSUE_DATE", LParams() << LParam("date", NodeAsString("date_issue",variablesNode)), rpt_params.GetLang()));
    xmlNodePtr dataSetNode = NodeAsNode("v_reseat", dataSetsNode);
    xmlNodeSetName(dataSetNode, "table");
    vector<string> rows;
    map< string, vector<string> > fields;
    int row;
    xmlNodePtr rowNode=dataSetNode->children;
    const char col_sym = ' ';
    for(; rowNode != NULL; rowNode = rowNode->next)
    {
        SeparateString(NodeAsString("full_name", rowNode), 25, rows);
        fields["full_name"]=rows;

        row=0;
        s.str("");
        do
        {
            if (row != 0) s << endl;
            s
                << right << setw(2) << (row == 0 ? NodeAsString("reg_no", rowNode) : "") << col_sym
                << left << setw(25) << (!fields["full_name"].empty() ? *(fields["full_name"].begin()) : "") << col_sym
                << left <<  setw(3) << (row == 0 ? (strcmp(NodeAsString("pr_brd", rowNode, "0"), "0") == 0 ? "-" : "+") : "") << col_sym
                << left <<  setw(2) << (row == 0 ? NodeAsString("cls", rowNode) : "") << col_sym
                << left <<  setw(3) << (row == 0 ? NodeAsString("seats", rowNode) : "") << col_sym
                << left <<  setw(5) << (row == 0 ? NodeAsString("old_seat_no", rowNode) : "") << col_sym
                << left <<  setw(7) << (row == 0 ? NodeAsString("new_seat_no", rowNode) : "") << col_sym
                << left <<  setw(10) << (row == 0 ? NodeAsString("document", rowNode) : "") << col_sym
                << left <<  setw(15) << (row == 0 ? NodeAsString("ticket_no", rowNode) : "");
            for(map< string, vector<string> >::iterator f = fields.begin(); f != fields.end(); f++)
                if (!f->second.empty()) f->second.erase(f->second.begin());
            row++;
        }
        while(!fields["full_name"].empty());
        NewTextChild(rowNode,"str",s.str());
    }
}

void KOMPLEKT(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    if(not TReqInfo::Instance()->desk.compatible(KOMPLEKT_VERSION))
        throw UserException("MSG.KOMPLEKT_SUPPORT",
                LParams()
                << LParam("rpt", ElemIdToNameLong(etReportType, EncodeRptType(rpt_params.rpt_type)))
                << LParam("vers", KOMPLEKT_VERSION));
  struct TReportItem {
      string code;
      int num;
      int sort_order;
      bool operator < (const TReportItem &other) const
      {
          return sort_order < other.sort_order;
      }
      TReportItem(const TReportItem &other)
      {
          code = other.code;
          num = other.num;
          sort_order = other.sort_order;
      }
      TReportItem(
              const string &_code,
              int _num,
              int _sort_order):
          code(_code),
          num(_num),
          sort_order(_sort_order)
      {}
  };

  TTripInfo info;
  info.getByPointId(rpt_params.point_id);
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT report_type, "
    "       num, "
    "       report_types.sort_order, "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(flt_no,NULL,0,2)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM doc_num_copies, report_types "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) and "
    "      doc_num_copies.report_type = report_types.code ";

  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.Execute();

  // получаем для каждого типа отчёта все варианты количества
  map< TReportItem, map<int, int> > temp;
  while (!Qry.Eof)
  {
      TReportItem item(
              Qry.FieldAsString("report_type"),
              Qry.FieldAsInteger("num"),
              Qry.FieldAsInteger("sort_order"));
      int priority = Qry.FieldAsInteger("priority");
//    LogTrace(TRACE5) << "report_type=" << report_type
//                     << " priority=" << priority
//                     << " num=" << num;
      temp[item][priority] = item.num;
      Qry.Next();
  }


  // выбираем для каждого типа отчёта количество с наивысшим приоритетом
  set<TReportItem> nums;
  for (auto r : temp) {
      TReportItem item(r.first);
      item.num = r.second.rbegin()->second;
      nums.insert(item);
  }
  // формирование отчёта
  get_compatible_report_form("komplekt", reqNode, resNode);
  xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
  xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
  xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_komplekt");
  // переменные отчёта
  xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
  PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
  // заголовки отчёта
  NewTextChild(variablesNode, "caption", getLocaleText("CAP.DOC.KOMPLEKT", rpt_params.GetLang()));
  populate_doc_cap(variablesNode, rpt_params.GetLang());
  NewTextChild(variablesNode, "doc_cap_komplekt",
      getLocaleText("CAP.DOC.KOMPLEKT_HEADER",
                    LParams() << LParam("airline", NodeAsString("airline_name", variablesNode))
                              << LParam("flight", get_flight(variablesNode))
                              << LParam("route", NodeAsString("long_route", variablesNode)),
                    rpt_params.GetLang()));
  NewTextChild(variablesNode, "komplekt_empty", (nums.empty() ? getLocaleText("MSG.KOMPLEKT_EMPTY", rpt_params.GetLang()) : ""));
  for (auto r : nums)
  {
    xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
    NewTextChild(rowNode, "code", r.code);
    NewTextChild(rowNode, "name", ElemIdToNameLong(etReportType, r.code));
    NewTextChild(rowNode, "num", r.num);
  }
}

int GetNumCopies(TRptParams rpt_params)
{
  TTripInfo info;
  info.getByPointId(rpt_params.point_id);
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT num, "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(flt_no,NULL,0,2)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM doc_num_copies "
    "WHERE report_type = :report_type AND "
    "      (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("report_type",otString,EncodeRptType(rpt_params.orig_rpt_type));
  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("flt_no",otInteger,info.flt_no);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.Execute();
  return Qry.Eof ? NoExists : Qry.FieldAsInteger("num");
}

void ANNUL_TAGS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("annul_tags", reqNode, resNode);
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_annul");
    // переменные отчёта
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // заголовок отчёта
    NewTextChild(variablesNode, "caption",
            getLocaleText("CAP.DOC.ANNUL_TAGS", LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());

    NewTextChild(variablesNode, "doc_cap_annul_reg_no", getLocaleText("№"));
    NewTextChild(variablesNode, "doc_cap_annul_fio", getLocaleText("Ф.И.О."));
    NewTextChild(variablesNode, "doc_cap_annul_no", getLocaleText("№№ баг. бирок"));
    NewTextChild(variablesNode, "doc_cap_annul_weight", getLocaleText("БГ вес"));
    NewTextChild(variablesNode, "doc_cap_annul_bag_type", getLocaleText("Тип багажа/RFISC"));
    NewTextChild(variablesNode, "doc_cap_annul_trfer", getLocaleText("Трфр"));
    NewTextChild(variablesNode, "doc_cap_annul_trfer_dir", getLocaleText("До трфр"));

    TAnnulBTStat AnnulBTStat;
    RunAnnulBTStat(AnnulBTStat, rpt_params.point_id);

    TCachedQuery paxQry("select reg_no, name, surname from pax where pax_id = :pax_id",
            QParams() << QParam("pax_id", otInteger));

    struct TPaxInfo {
        int reg_no;
        string surname;
        string name;
        TPaxInfo(): reg_no(NoExists) {}
    };

    map<int, TPaxInfo> pax_map;

    for(list<TAnnulBTStatRow>::const_iterator i = AnnulBTStat.rows.begin(); i != AnnulBTStat.rows.end(); i++) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        map<int, TPaxInfo>::iterator iPax = pax_map.find(i->pax_id);

        if(iPax == pax_map.end()) {
            TPaxInfo pax;
            if(i->pax_id != NoExists) {
                paxQry.get().SetVariable("pax_id", i->pax_id);
                paxQry.get().Execute();
                if(not paxQry.get().Eof) {
                    pax.reg_no = paxQry.get().FieldAsInteger("reg_no");
                    pax.name = paxQry.get().FieldAsString("name");
                    pax.surname = paxQry.get().FieldAsString("surname");
                }
            }
            pair<map<int, TPaxInfo>::iterator, bool> ret =
                pax_map.insert(make_pair(i->pax_id, pax));
            iPax = ret.first;
        }

        //  Рег№
        if(iPax->second.reg_no == NoExists)
            NewTextChild(rowNode, "reg_no");
        else
            NewTextChild(rowNode, "reg_no", iPax->second.reg_no);
        //  пассажира ФИО
        ostringstream buf;
        if(iPax->second.reg_no != NoExists)
            buf
                << transliter(iPax->second.surname, 1, rpt_params.GetLang() != AstraLocale::LANG_RU) << " "
                << transliter(iPax->second.name, 1, rpt_params.GetLang() != AstraLocale::LANG_RU);
        NewTextChild(rowNode, "fio", buf.str());
        //  номер бирки
        NewTextChild(rowNode, "no", get_tag_range(i->tags, LANG_EN));
        //  значение по весу
        if (i->weight != NoExists)
            NewTextChild(rowNode, "weight", i->weight);
        else
            NewTextChild(rowNode, "weight");

        //  тип багажа
        buf.str("");
        if(not i->rfisc.empty())
            buf << i->rfisc;
        else if(i->bag_type != NoExists)
            buf << ElemIdToNameLong(etBagType, i->bag_type);
        NewTextChild(rowNode, "bag_type", buf.str());

        if(i->trfer_airline.empty()) {
            //  призн.трансфера
            NewTextChild(rowNode, "pr_trfer", getLocaleText("НЕТ"));
            //  направление трфр
            NewTextChild(rowNode, "trfer_airp_arv");
        } else {
            NewTextChild(rowNode, "pr_trfer", getLocaleText("ДА"));
            NewTextChild(rowNode, "trfer_airp_arv", rpt_params.ElemIdToReportElem(etAirp, i->trfer_airp_arv, efmtCodeNative));
        }
    }
}

void  DocsInterface::RunReport2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr node = reqNode->children;
    TRptParams rpt_params;
    rpt_params.Init(node);
    switch(rpt_params.rpt_type) {
        case rtPTM:
            REPORTS::PTM(rpt_params, reqNode, resNode);
            break;
        case rtPTMTXT:
            PTMBTMTXT(rpt_params, reqNode, resNode);
            break;
        case rtBTM:
            BTM(rpt_params, reqNode, resNode);
            break;
        case rtBTMTXT:
            PTMBTMTXT(rpt_params, reqNode, resNode);
            break;
        case rtWEB:
            WEB(rpt_params, reqNode, resNode);
            break;
        case rtWEBTXT:
            WEBTXT(rpt_params, reqNode, resNode);
            break;
        case rtREFUSE:
            REFUSE(rpt_params, reqNode, resNode);
            break;
        case rtREFUSETXT:
            REFUSETXT(rpt_params, reqNode, resNode);
            break;
        case rtNOTPRES:
            NOTPRES(rpt_params, reqNode, resNode);
            break;
        case rtNOTPRESTXT:
            NOTPRESTXT(rpt_params, reqNode, resNode);
            break;
        case rtSPEC:
        case rtREM:
            REM(rpt_params, reqNode, resNode);
            break;
        case rtSPECTXT:
        case rtREMTXT:
            REMTXT(rpt_params, reqNode, resNode);
            break;
        case rtCRS:
        case rtCRSUNREG:
        case rtBDOCS:
            CRS(rpt_params, reqNode, resNode);
            break;
        case rtCRSTXT:
        case rtCRSUNREGTXT:
            CRSTXT(rpt_params, reqNode, resNode);
            break;
        case rtEXAM:
        case rtNOREC:
        case rtGOSHO:
            EXAM(rpt_params, reqNode, resNode);
            break;
        case rtEXAMTXT:
        case rtNORECTXT:
        case rtGOSHOTXT:
            EXAMTXT(rpt_params, reqNode, resNode);
            break;
        case rtEMD:
            EMD(rpt_params, reqNode, resNode);
            break;
        case rtEMDTXT:
            EMDTXT(rpt_params, reqNode, resNode);
            break;
        case rtLOADSHEET:
        case rtNOTOC:
        case rtLIR:
            WB_MSG(rpt_params, reqNode, resNode);
            break;
        case rtANNUL_TAGS:
            ANNUL_TAGS(rpt_params, reqNode, resNode);
            break;
        case rtVOUCHERS:
            VOUCHERS(rpt_params, reqNode, resNode);
            break;
        case rtSERVICES:
            SERVICES(rpt_params, reqNode, resNode);
            break;
        case rtSERVICESTXT:
            SERVICESTXT(rpt_params, reqNode, resNode);
            break;
        case rtRESEAT:
            RESEAT(rpt_params, reqNode, resNode);
            break;
        case rtRESEATTXT:
            RESEATTXT(rpt_params, reqNode, resNode);
            break;
        case rtKOMPLEKT:
            KOMPLEKT(rpt_params, reqNode, resNode);
            break;
        default:
            throw AstraLocale::UserException("MSG.TEMPORARILY_NOT_SUPPORTED");
    }
    NewTextChild(resNode, "copies", GetNumCopies(rpt_params), NoExists);
}

void  DocsInterface::LogPrintEvent(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int copies = NodeAsInteger("copies", reqNode, NoExists);
    int printed_copies = NodeAsInteger("printed_copies", reqNode, NoExists);
    if(printed_copies == NoExists) { // старая версия терминала не присылает тег printed_copies
        TReqInfo::Instance()->LocaleToLog("EVT.PRINT_REPORT", LEvntPrms()
                << PrmElem<std::string>("report", etReportType, NodeAsString("rpt_type", reqNode), efmtNameLong)
                << PrmSmpl<std::string>("copies", ""),
                ASTRA::evtPrn, NodeAsInteger("point_id", reqNode));
    } else {
        ostringstream str;
        str << getLocaleText("Напечатано копий") << ": " << printed_copies;
        if(copies != NoExists and copies != printed_copies)
            str << "; " << getLocaleText("Задано копий") << ": " << copies;
        TReqInfo::Instance()->LocaleToLog("EVT.PRINT_REPORT",
                LEvntPrms()
                << PrmElem<std::string>("report", etReportType, NodeAsString("rpt_type", reqNode), efmtNameLong)
                << PrmSmpl<std::string>("copies", str.str()),
                ASTRA::evtPrn, NodeAsInteger("point_id", reqNode));
    }
}

void  DocsInterface::LogExportEvent(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo::Instance()->LocaleToLog("EVT.EXPORT_REPORT", LEvntPrms()
                                       << PrmElem<std::string>("report", etReportType, NodeAsString("rpt_type", reqNode), efmtNameLong)
                                       << PrmSmpl<std::string>("fmt", NodeAsString("export_name", reqNode)),
                                       ASTRA::evtPrn, NodeAsInteger("point_id", reqNode));
}

void  DocsInterface::SaveReport(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    if(NodeIsNULL("name", reqNode))
        throw UserException("Form name can't be null");
    string name = NodeAsString("name", reqNode);
    string version = NodeAsString("version", reqNode, "");
    ProgTrace(TRACE5, "VER. %s", version.c_str());
    if(version == "")
        version = get_report_version(name);

    string form = NodeAsString("form", reqNode);
    Qry.SQLText = "update fr_forms2 set form = :form where name = :name and version = :version";
    Qry.CreateVariable("version", otString, version);
    Qry.CreateVariable("name", otString, name);
    Qry.CreateLongVariable("form", otLong, (void *)form.c_str(), form.size());
    Qry.Execute();
    if(!Qry.RowsProcessed())
      throw UserException("MSG.REPORT_UPDATE_FAILED.NOT_FOUND", LParams() << LParam("report_name", name));
    TReqInfo::Instance()->LocaleToLog("EVT.UPDATE_REPORT", LEvntPrms() << PrmSmpl<std::string>("name", name), evtSystem);
}

void DocsInterface::print_komplekt(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr codesNode = NodeAsNode("codes", reqNode);
    codesNode = codesNode->children;
    xmlNodePtr reportListNode = NULL;
    for(; codesNode; codesNode = codesNode->next) {
        xmlNodePtr currNode = codesNode->children;
        string code = NodeAsStringFast("code", currNode);

        xmlNodePtr LoadFormNode = GetNodeFast("LoadForm", currNode);
        xmlNodePtr reqLoadFormNode = GetNode("LoadForm", reqNode);

        if(reqLoadFormNode) {
            if(not LoadFormNode)
                RemoveNode(reqLoadFormNode);
        } else {
            if(LoadFormNode)
                NewTextChild(reqNode, "LoadForm");
        }

        xmlNodePtr rptTypeNode = GetNode("rpt_type", reqNode);
        if(rptTypeNode)
            NodeSetContent(rptTypeNode, code);
        else
            NewTextChild(reqNode, "rpt_type", code);

        // Некоторые отчеты, напр. GOSHO требуют, чтобы структура
        // XML была следующая: /term/answer
        XMLDoc doc("term");
        xmlNodePtr reportNode = doc.docPtr()->children;
        xmlNodePtr answerNode = NewTextChild(reportNode, "answer");
        SetProp(reportNode, "code", code);

        try {
            RunReport2(ctxt, reqNode, answerNode);

            if(not reportListNode)
                reportListNode = NewTextChild(resNode, "report_list");

            CopyNode(reportListNode, reportNode);
        } catch(Exception &E) {
            LogTrace(TRACE5) << __FUNCTION__ << ": " << code << " failed: " << E.what();
        }

        // break;
    }
}

void DocsInterface::GetFonts(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    tst();
  NewTextChild(resNode,"fonts","");
  tst();
}

int testbm(int argc,char **argv)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize("МОВ");
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

