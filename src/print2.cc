#include "print2.h"
#include "astra_consts.h"
#include "xml_unit.h"
#include "oralib.h"
#include "exceptions.h"
#include "print.h"
#include "str_utils.h"

using namespace ASTRA;
using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

void Print2Interface::GetPrintDataBP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr currNode = reqNode->children;
    int grp_id = NodeAsIntegerFast("grp_id", currNode, NoExists);
    int pax_id = NodeAsIntegerFast("pax_id", currNode, NoExists);
    int pr_all = NodeAsIntegerFast("pr_all", currNode, NoExists);
    int prn_type = NodeAsIntegerFast("prn_type", currNode, NoExists);
    string dev_model = NodeAsStringFast("dev_model", currNode);
    string fmt_type = NodeAsStringFast("fmt_type", currNode);
    int pr_lat = NodeAsIntegerFast("pr_lat", currNode, NoExists);
    xmlNodePtr clientDataNode = NodeAsNodeFast("clientData", currNode);

    /* !!! temporarily commented
    if(prn_type != NoExists) {
        if(prn_type == 90) {
            dev_model = "ATB CUTE";
            fmt_type = "ATB";
        } else if(prn_type == 91) {
            dev_model = "BTP CUTE";
            fmt_type = "BTP";
        } else
            throw UserException("Версия терминала устарела. Обновите терминал.");
        prn_type = NoExists;
    }
    */

    TQuery Qry(&OraSession);

    if(grp_id == NoExists) {
        Qry.Clear();
        Qry.SQLText="SELECT grp_id from pax where pax_id = :pax_id";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
        Qry.Execute();
        if(Qry.Eof)
            throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
        grp_id = Qry.FieldAsInteger("grp_id");
    }
    Qry.Clear();
    Qry.SQLText="SELECT point_dep, class FROM pax_grp WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    if(Qry.FieldIsNULL("class"))
        throw UserException("Для багажа без сопровождения посадочный талон не печатается.");
    int point_id = Qry.FieldAsInteger("point_dep");
    string cl = Qry.FieldAsString("class");
    Qry.Clear();
    Qry.SQLText =
        "SELECT bp_type FROM trip_bp "
        "WHERE point_id=:point_id AND (class IS NULL OR class=:class) "
        "ORDER BY class ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("class", otString, cl);
    Qry.Execute();
    if(Qry.Eof) throw UserException("На рейс или класс не назначен бланк посадочных талонов");
    string form_type = Qry.FieldAsString("bp_type");
    Qry.Clear();
    Qry.SQLText =
        "select "
        "   prn_form_vers.form, "
        "   prn_form_vers.data "
        "from "
        "   bp_models, "
        "   prn_form_vers "
        "where "
        "   bp_models.form_type = :form_type and "
        "   bp_models.dev_model = :dev_model and "
        "   bp_models.fmt_type = :fmt_type and "
        "   bp_models.id = prn_form_vers.id and "
        "   bp_models.version = prn_form_vers.version ";
    Qry.CreateVariable("form_type", otString, form_type);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.Execute();
    if(Qry.Eof||Qry.FieldIsNULL("data")||
    	 Qry.FieldIsNULL( "form" ) && (fmt_type == "BTP" || fmt_type == "ATB" || fmt_type == "EPL2")
    	)
      throw UserException("Печать пос. талона на выбранный принтер не производится");
    string form = Qry.FieldAsString("form");
    string data = Qry.FieldAsString("data");
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr BPNode = NewTextChild(dataNode, "printBP");
    NewTextChild(BPNode, "pectab", form);
    Qry.Clear();
    if(pax_id == NoExists) {
        if(pr_all)
            Qry.SQLText =
                "select pax_id, grp_id, reg_no from pax where grp_id = :grp_id and refuse is null order by reg_no";
        else
            Qry.SQLText =
                "select pax.pax_id, pax.grp_id, pax.reg_no  from "
                "   pax, bp_print "
                "where "
                "   pax.grp_id = :grp_id and "
                "   pax.refuse is null and "
                "   pax.pax_id = bp_print.pax_id(+) and "
                "   bp_print.pr_print(+) <> 0 and "
                "   bp_print.pax_id IS NULL "
                "order by "
                "   pax.reg_no";
        Qry.CreateVariable("grp_id", otInteger, grp_id);
    } else {
        Qry.SQLText = "select grp_id, pax_id, reg_no from pax where pax_id = :pax_id";
        Qry.CreateVariable("pax_id", otInteger, pax_id);
    }
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Изменения в группе производились с другой стойки. Обновите данные");
    xmlNodePtr passengersNode = NewTextChild(BPNode, "passengers");
    while(!Qry.Eof) {
        int pax_id = Qry.FieldAsInteger("pax_id");
        int reg_no = Qry.FieldAsInteger("reg_no");
        int grp_id = Qry.FieldAsInteger("grp_id");
        PrintDataParser parser(grp_id, pax_id, pr_lat, clientDataNode);
        xmlNodePtr paxNode = NewTextChild(passengersNode, "pax");
        string prn_form = parser.parse(data);
        if(fmt_type == "EPSON") {
            TPrnType convert_prn_type;
            if(dev_model == "OLIVETTI")
                convert_prn_type = ptOLIVETTI;
            else if(dev_model == "ML390")
                convert_prn_type = ptOKIML390;
            else if(dev_model == "ML3310")
                convert_prn_type = ptOKIML3310;
            else
                throw Exception(dev_model + " not supported by to_esc::convert");
            to_esc::convert(prn_form, convert_prn_type, NULL);
            prn_form = b64_encode(prn_form.c_str(), prn_form.size());
        }
        NewTextChild(paxNode, "prn_form", prn_form);
        {
            TQuery *Qry = parser.get_prn_qry();
            TDateTime time_print = NowUTC();
            Qry->CreateVariable("now_utc", otDate, time_print);
            Qry->Execute();
            SetProp(paxNode, "pax_id", pax_id);
            SetProp(paxNode, "reg_no", reg_no);
            SetProp(paxNode, "time_print", DateTimeToStr(time_print));
        }
        Qry.Next();
    }
}
