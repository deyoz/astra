#include "docs.h"
#include "xml_unit.h"
#include "oralib.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "astra_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;

const string PMTrfer = "PMTrfer";
const string PM = "PM";
const string BMTrfer = "BMTrfer";
const string BM = "BM";
const string ref = "ref";
const string notpres = "notpres";
const string rem = "rem";
const string crs = "crs";
const string crsUnreg = "crsUnreg";
const string exam = "exam";

string get_ckin_zone(int grp_hall_id)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select rpt_grp from halls2 where id = :id";
    Qry.CreateVariable("id", otInteger, grp_hall_id);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("get_ckin_zone: zone not found for %d", grp_hall_id);
    return Qry.FieldAsString("rpt_grp");
}

void DocsInterface::RunReport(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    string name = NodeAsString("name", reqNode);
    xmlNodePtr node = GetNode("target", reqNode);
    string target;
    if(node)
        target = NodeAsString(node);
    node = GetNode("pr_brd_pax", reqNode);
    int pr_brd = 0;
    if(node)
        pr_brd = NodeAsInteger(node);
    node = GetNode("pr_vip", reqNode);
    int grp_hall_id = -1;
    if(node)
        grp_hall_id = NodeAsInteger(node);

    TRptType rpt_type = rtUnknown;
    if(name == PM or name == PMTrfer)
        rpt_type = rtPTM;
    else if(name == BM or name == BMTrfer)
        rpt_type = rtBTM;
    else if(name == ref)
        rpt_type = rtREFUSE;
    else if(name == notpres)
        rpt_type = rtNOTPRES;
    else if(name == rem)
        rpt_type = rtREM;
    else if(name == crs)
        rpt_type = rtCRS;
    else if(name == crsUnreg)
        rpt_type = rtCRSUNREG;
    else if(name == exam)
        rpt_type = rtEXAM;
    else
        throw Exception("Unknown report name: %s", name.c_str());

    bool pr_trfer;
    if(name == PMTrfer or name == BMTrfer)
        pr_trfer = true;
    else
        pr_trfer = false;

    bool pr_et;
    if(
            name == PM and target.empty() or
            name == PMTrfer and target == "etm"
            )
        pr_et = true;
    else
        pr_et = false;

    if(
            name == PM and target == "tot" or
            name == PMTrfer and (target == "tpm" or target == "etm")
            )
        target.erase();

    xmlNodePtr qryNode = reqNode->parent;
    xmlUnlinkNode(reqNode);
    xmlFreeNode(reqNode);
    reqNode = NewTextChild(qryNode, "run_report2");
    NewTextChild(reqNode, "rpt_type", EncodeRptType(rpt_type));
    NewTextChild(reqNode, "airp_arv", target, "");
    NewTextChild(reqNode, "point_id", point_id);
    NewTextChild(reqNode, "pr_trfer", pr_trfer, 0);
    if(grp_hall_id >= 0)
        NewTextChild(reqNode, "ckin_zone", get_ckin_zone(grp_hall_id));
    NewTextChild(reqNode, "pr_brd", pr_brd, 0);
    NewTextChild(reqNode, "pr_et", pr_et, 0);
    ProgTrace(TRACE5, "%s", GetXMLDocText(reqNode->doc).c_str());
    RunReport2(ctxt, reqNode, resNode);
}

int get_grp_hall_id(string airp, string grp)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select id from halls2 where "
        "   airp = :airp and (rpt_grp = :rpt_grp or rpt_grp is null and :rpt_grp is null) and "
        "   rownum < 2";
    Qry.CreateVariable("airp", otString, airp);
    Qry.CreateVariable("rpt_grp", otString, grp);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("get_grp_hall_id: cannot find id for: '%s', '%s'", airp.c_str(), grp.c_str());
    return Qry.FieldAsInteger("id");
}

void DocsInterface::GetSegList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    int get_tranzit = 0;
    string rpType = NodeAsString("rpType", reqNode);

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT airp,point_num, "
        "       DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
        "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if(Qry.Eof) throw EXCEPTIONS::UserException("Рейс не найден. Обновите данные.");

    int first_point = Qry.FieldAsInteger("first_point");
    int point_num = Qry.FieldAsInteger("point_num");
    string own_airp = Qry.FieldAsString("airp");
    string prev_airp, curr_airp;

    vector<string> zone_list = get_grp_zone_list(point_id);
    xmlNodePtr SegListNode = NewTextChild(resNode, "SegList");

    for(int j = -1; j <= 1; j++) {
        if(j == 0) {
            curr_airp = own_airp;
            continue;
        }
        Qry.Clear();
        string SQLText = "select airp from points ";
        if(j == -1)
            SQLText +=
                "where :first_point in(first_point,point_id) and point_num<:point_num and pr_del=0 "
                "order by point_num desc ";
        else
            SQLText +=
                "where first_point=:first_point and point_num>:point_num and pr_del=0 "
                "order by point_num asc ";
        Qry.SQLText = SQLText;
        Qry.CreateVariable("first_point", otInteger, first_point);
        Qry.CreateVariable("point_num", otInteger, point_num);
        Qry.Execute();
        while(!Qry.Eof) {
            string airp = Qry.FieldAsString("airp");
            if(j == -1) {
                if(get_tranzit) prev_airp = airp;
                break;
            }

            for(vector<string>::iterator iv = zone_list.begin(); iv != zone_list.end(); iv++) {
                if(prev_airp.size()) {
                    xmlNodePtr SegNode = NewTextChild(SegListNode, "seg");
                    NewTextChild(SegNode, "status", "T");
                    NewTextChild(SegNode, "airp_dep_code", prev_airp);
                    NewTextChild(SegNode, "airp_arv_code", airp);
                    NewTextChild(SegNode, "pr_vip", 2);
                    if(
                            rpType == "BM" ||
                            rpType == "TBM" ||
                            rpType == "PM" ||
                            rpType == "TPM"
                      ) {
                        string hall;
                        if(iv->empty()) {
                            NewTextChild(SegNode, "zone");
                            hall = " (др. залы)";
                        } else if(*iv != " ") {
                            NewTextChild(SegNode, "zone", *iv);
                            hall = " (" + *iv + ")";
                        }
                        NewTextChild(SegNode, "item", prev_airp + "-" + airp + hall);
                    }
                }
                if(curr_airp.size()) {
                    xmlNodePtr SegNode = NewTextChild(SegListNode, "seg");
                    NewTextChild(SegNode, "status", "N");
                    NewTextChild(SegNode, "airp_dep_code", curr_airp);
                    NewTextChild(SegNode, "airp_arv_code", airp);
                    if(*iv == " ") // Все залы
                        NewTextChild(SegNode, "pr_vip", -1);
                    else
                        NewTextChild(SegNode, "pr_vip", get_grp_hall_id(curr_airp, *iv));
                    if(
                            rpType == "BM" ||
                            rpType == "TBM" ||
                            rpType == "PM" ||
                            rpType == "TPM"
                      ) {
                        string hall;
                        if(iv->empty()) {
                            NewTextChild(SegNode, "zone");
                            hall = " (др. залы)";
                        } else if(*iv != " ") {
                            NewTextChild(SegNode, "zone", *iv);
                            hall = " (" + *iv + ")";
                        }
                        NewTextChild(SegNode, "item", curr_airp + "-" + airp + hall);
                    }
                }
                if(!(rpType == "BM" || rpType == "TBM" || rpType == "PM" || rpType == "TPM"))
                    break;
            }
            Qry.Next();
        }
    }
    if(
            rpType == "PM" ||
            rpType == "TPM" ||
            rpType == "TBM" ||
            rpType == "BM"
      ) {
        xmlNodePtr SegNode;
        if(
                rpType == "PM" ||
                rpType == "TPM"
          ) {
            string item;
            if(rpType == "TPM")
                item = "etm";
            else
                item = "";
            SegNode = NewTextChild(SegListNode, "seg");
            NewTextChild(SegNode, "status");
            NewTextChild(SegNode, "airp_dep_code", curr_airp);
            NewTextChild(SegNode, "airp_arv_code", item);
            NewTextChild(SegNode, "pr_vip", -1);
            NewTextChild(SegNode, "item", "Список ЭБ");
        }

        if(
                rpType == "BM" ||
                rpType == "TBM" ||
                rpType == "PM" ||
                rpType == "TPM"
          ) {
            string item;
            if(rpType == "PM")
                item = "tot";
            else if(rpType == "TPM")
                item = "tpm";
            else
                item = "";
            SegNode = NewTextChild(SegListNode, "seg");
            NewTextChild(SegNode, "status");
            NewTextChild(SegNode, "airp_dep_code", curr_airp);
            NewTextChild(SegNode, "airp_arv_code", item);
            NewTextChild(SegNode, "pr_vip", -1);
            NewTextChild(SegNode, "item", "Общая");
        }
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}
