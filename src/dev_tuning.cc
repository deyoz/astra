#include "dev_tuning.h"
#include "basic.h"
#define NICKNAME "DEN"
#include "setup.h"
#include "test.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "print.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

typedef struct {
    int id, version;
    string descr;
} t_descr;

void make_descr_list(xmlNodePtr rowNode, vector<t_descr> &descr)
{
    if(rowNode) {
        xmlNodePtr descrListNode = NULL;
        for(size_t i = 0; i < descr.size(); i++) {
            if(!descrListNode)
                descrListNode = NewTextChild(rowNode, "descr_list");
            xmlNodePtr itemNode = NewTextChild(descrListNode, "item");
            NewTextChild(itemNode, "id", descr[i].id);
            NewTextChild(itemNode, "version", descr[i].version);
            NewTextChild(itemNode, "descr", descr[i].descr);
        }
    }
}

void DevTuningInterface::LoadBTList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   bt_models.form_type, "
        "   bt_models.dev_model dev_code, "
        "   bt_models.num, "
        "   dev_models.name dev_model, "
        "   dev_models.fmt_type, " // !!! missing
        "   prn_forms.id, "
        "   prn_forms.version, "
        "   prn_forms.descr prn_form "
        "from "
        "   bt_models, "
        "   prn_forms, "
        "   dev_models "
        "where "
        "   bt_models.id = prn_forms.id and "
        "   bt_models.version = prn_forms.version and "
        "   bt_models.dev_model = dev_models.code "
        "order by "
        "   bt_models.form_type, "
        "   bt_models.dev_model, "
        "   bt_models.num ";
    Qry.Execute();
    xmlNodePtr operTypesNode = NewTextChild(resNode, "bp_list");
    string form_type, dev_code;
    vector<t_descr> descr;
    xmlNodePtr rowNode = NULL;
    for(; !Qry.Eof; Qry.Next()) {
        string tmp_dev_code = Qry.FieldAsString("dev_code");
        string tmp_form_type = Qry.FieldAsString("form_type");
        if(tmp_form_type != form_type || tmp_dev_code != dev_code) {
            make_descr_list(rowNode, descr);
            rowNode = NewTextChild(operTypesNode, "row");
            NewTextChild(rowNode, "form_type", Qry.FieldAsString("form_type"));
            NewTextChild(rowNode, "dev_code", Qry.FieldAsString("dev_code"));
            NewTextChild(rowNode, "dev_model", Qry.FieldAsString("dev_model"));
            NewTextChild(rowNode, "fmt_type", Qry.FieldAsString("fmt_type"));
            dev_code = tmp_dev_code;
            form_type = tmp_form_type;
            descr.clear();
        }

        t_descr idescr;
        idescr.id = Qry.FieldAsInteger("id");
        idescr.version = Qry.FieldAsInteger("version");
        idescr.descr = Qry.FieldAsString("prn_form");
        descr.push_back(idescr);
    }
    make_descr_list(rowNode, descr);
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void DevTuningInterface::LoadBPList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   bp_models.form_type, "
        "   bp_models.dev_model dev_code, "
        "   dev_models.name dev_model, "
        "   prn_forms.fmt_type, "
        "   prn_forms.id, "
        "   prn_forms.version, "
        "   prn_forms.descr prn_form "
        "from "
        "   bp_models, "
        "   prn_forms, "
        "   dev_models "
        "where "
        "   bp_models.id = prn_forms.id and "
        "   bp_models.version = prn_forms.version and "
        "   bp_models.dev_model = dev_models.code "
        "order by "
        "   bp_models.form_type, "
        "   dev_models.name, "
        "   prn_forms.fmt_type, "
        "   prn_forms.descr ";
    Qry.Execute();
    xmlNodePtr operTypesNode = NewTextChild(resNode, "bp_list");
    for(; !Qry.Eof; Qry.Next()) {
        xmlNodePtr rowNode = NewTextChild(operTypesNode, "row");
        NewTextChild(rowNode, "id", Qry.FieldAsString("id"));
        NewTextChild(rowNode, "ver", Qry.FieldAsString("version"));
        NewTextChild(rowNode, "form_type", Qry.FieldAsString("form_type"));
        NewTextChild(rowNode, "dev_code", Qry.FieldAsString("dev_code"));
        NewTextChild(rowNode, "dev_model", Qry.FieldAsString("dev_model"));
        NewTextChild(rowNode, "fmt_type", Qry.FieldAsString("fmt_type"));
        NewTextChild(rowNode, "prn_form", Qry.FieldAsString("prn_form"));
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void DevTuningInterface::LoadOperTypes(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   code, "
        "   nvl(name, code) oper_type "
        "from "
        "   dev_oper_types "
        "where "
        "   code in ('PRINT_BP', 'PRINT_BT')";
    Qry.Execute();
    xmlNodePtr operTypesNode = NewTextChild(resNode, "oper_types");
    for(; !Qry.Eof; Qry.Next()) {
        xmlNodePtr rowNode = NewTextChild(operTypesNode, "row");
        NewTextChild(rowNode, "code", Qry.FieldAsString("code"));
        NewTextChild(rowNode, "oper_type", Qry.FieldAsString("oper_type"));
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void DevTuningInterface::LoadPrnForm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   prn_forms.form, "
        "   prn_forms.data, "
        "   prn_forms.descr, "
        "   prn_forms.version  "
        "from "
        "   prn_forms, "
        "   ( "
        "       select "
        "           id, "
        "           max(version) version "
        "       from "
        "           prn_forms "
        "       group by "
        "           id "
        "   ) a "
        "where "
        "   prn_forms.id = :id and "
        "   prn_forms.id = a.id and "
        "   prn_forms.version = a.version ";
    Qry.CreateVariable("id", otInteger, NodeAsInteger("id", reqNode));
    Qry.Execute();
    xmlNodePtr prnFormNode = NewTextChild(resNode, "prn_form");
    if(!Qry.Eof) {
        NewTextChild(prnFormNode, "form", Qry.FieldAsString("form"));
        NewTextChild(prnFormNode, "data", Qry.FieldAsString("data"));
        NewTextChild(prnFormNode, "descr", Qry.FieldAsString("descr"));
        NewTextChild(prnFormNode, "version", Qry.FieldAsString("version"));
    }
}

void DevTuningInterface::PrevNext(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int id = NodeAsInteger("id", reqNode);
    int version = NodeAsInteger("version", reqNode);
    int delta = NodeAsInteger("delta", reqNode);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   form, "
        "   data, "
        "   descr "
        "from "
        "   prn_forms "
        "where "
        "   id = :id and "
        "   version = :version";
    Qry.CreateVariable("id", otInteger, id);
    Qry.CreateVariable("version", otInteger, version + delta);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("DevTuningInterface::PrevNext: data not found");
    xmlNodePtr prnFormNode = NewTextChild(resNode, "prn_form");
    NewTextChild(prnFormNode, "form", Qry.FieldAsString("form"));
    NewTextChild(prnFormNode, "data", Qry.FieldAsString("data"));
    NewTextChild(prnFormNode, "descr", Qry.FieldAsString("descr"));
}

void DevTuningInterface::Save(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int id = NodeAsInteger("id", reqNode);
    int version = NodeAsInteger("version", reqNode);
    xmlNodePtr formNode = GetNode("form", reqNode);
    xmlNodePtr dataNode = GetNode("data", reqNode);
    xmlNodePtr descrNode = GetNode("descr", reqNode);
    string SQLText;
    TQuery Qry(&OraSession);
    if(formNode) {
        SQLText =
            "declare "
            "   aop_type prn_forms.op_type%type; "
            "   afmt_type prn_forms.fmt_type%type; "
            "   aform prn_forms.form%type; "
            "   adata prn_forms.data%type; "
            "   bp_cnt integer; "
            "   bt_cnt integer; "
            "begin "
            "   select "
            "       op_type, "
            "       fmt_type, "
            "       form, "
            "       data "
            "   into "
            "       aop_type, "
            "       afmt_type, "
            "       aform, "
            "       adata "
            "   from "
            "       prn_forms "
            "   where "
            "       id = :id and "
            "       version = :version; "
            "   if :version = 0 and aform is null and adata is null then "
            "       update prn_forms set "
            "           form = :form, "
            "           data = :data "
            "       where "
            "           id = :id and "
            "           version = :version; "
            "   else "
            "     select count(*) into bp_cnt from bp_models where id = :id and version > :version; "
            "     select count(*) into bt_cnt from bp_models where id = :id and version > :version; "
            "     if bp_cnt = 0 and bt_cnt = 0 then "
            "       insert into prn_forms ( "
            "           id, "
            "           version, "
            "           op_type, "
            "           descr, "
            "           form, "
            "           data, "
            "           fmt_type "
            "       ) values ( "
            "           :id, "
            "           :version + 1, "
            "           aop_type, "
            "           :descr, "
            "           :form, "
            "           :data, "
            "           afmt_type "
            "       ); "
            "         update bp_models set version = :version + 1 where id = :id; "
            "         update bt_models set version = :version + 1 where id = :id; "
            "     else "
            "       update prn_forms set "
            "         descr = :descr, "
            "         form = :form, "
            "         data = :data "
            "       where "
            "         id = :id and "
            "         version = :version + 1; "
            "       if bp_cnt <> 0 then "
            "         update bp_models set version = :version + 1 where id = :id; "
            "       end if; "
            "       if bt_cnt <> 0 then "
            "         update bt_models set version = :version + 1 where id = :id; "
            "       end if; "
            "       delete from prn_forms where "
            "           id = :id and version > :version + 1; "
            "     end if; "
            "   end if; "
            "end; ";
        Qry.CreateVariable("form", otString, NodeAsString(formNode));
        Qry.CreateVariable("data", otString, NodeAsString(dataNode));
        Qry.CreateVariable("descr", otString, NodeAsString(descrNode));
    } else if(descrNode) {
        SQLText =
            "update prn_forms set descr = :descr where "
            "   id = :id and version = :version ";
        Qry.CreateVariable("descr", otString, NodeAsString(descrNode));
    }else {
        SQLText =
            "begin "
            "   update bp_models set version = :version where id = :id; "
            "   update bt_models set version = :version where id = :id; "
            "   delete from prn_forms where "
            "       id = :id and version > :version; "
            "end; ";
    }
    Qry.CreateVariable("id", otInteger, id);
    Qry.CreateVariable("version", otInteger, version);
    Qry.SQLText = SQLText;
    Qry.Execute();
}

void DevTuningInterface::GetPrintData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
/*    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select form, data from "
        "   prn_forms, "
        "   ( "
        "       select "
        "           id, "
        "           max(version) version "
        "       from "
        "           prn_forms "
        "       group by "
        "           id "
        "   ) a "
        "where "
        "   prn_forms.id = :id and "
        "   prn_forms.id = a.id and "
        "   prn_forms.version = a.version ";
    int id = NodeAsInteger("id", reqNode);
    Qry.CreateVariable("id", otInteger, id);
    Qry.Execute();
    if(Qry.Eof)
        throw Exception("DevTuningInterface::GetPrintData: prn_form not found for id %d", id);
    xmlNodePtr printNode = NewTextChild(resNode, "print");
    NewTextChild(printNode, "form", Qry.FieldAsString("form"));
    PrintDataParser parser(NodeAsInteger("pr_lat", reqNode));
    string data = Qry.FieldAsString("data");
    try {
        NewTextChild(printNode, "data", parser.parse(data));
    } catch (CodeException E) {
        if(E.Code() == 666)
            throw UserException(E.what());
        else
            throw;
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
    */
}

enum TBPState {bpAdd, bpDel, bpUpd, bpNone};

void DevTuningInterface::PrnFormsCommit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr listNode = NodeAsNode("list", reqNode);
    xmlNodePtr currNode = listNode->children;
    xmlNodePtr addListNode = NULL;
    for(; currNode; currNode = currNode->next) {
        xmlNodePtr fieldNode = currNode->children;
        TBPState state = (TBPState)NodeAsIntegerFast("state", fieldNode);
        TQuery Qry(&OraSession);
        string form_type, dev_model;
        switch(state) {
            case bpAdd:
                Qry.SQLText =
                    "begin "
                    "  select id__seq.nextval into :id from dual; "
                    "insert into prn_forms ("
                    "   id, "
                    "   version, "
                    "   op_type, "
                    "   descr, "
                    "   fmt_type "
                    ") values ( "
                    "   :id, "
                    "   0, "
                    "   :op_type, "
                    "   :descr, "
                    "   :fmt_type "
                    "); "
                    "end; ";
                Qry.DeclareVariable("id", otString);
                Qry.CreateVariable("op_type", otString, NodeAsStringFast("op_type", fieldNode));
                Qry.CreateVariable("descr", otString, NodeAsStringFast("descr", fieldNode));
                Qry.CreateVariable("fmt_type", otString, NodeAsStringFast("fmt_type", fieldNode));
                break;
            case bpDel:
                Qry.SQLText =
                    "delete from prn_forms where "
                    "   id = :id and "
                    "   version = :version ";
                Qry.CreateVariable("id", otInteger, NodeAsIntegerFast("id", fieldNode));
                Qry.CreateVariable("version", otInteger, NodeAsIntegerFast("version", fieldNode));
                break;
            case bpUpd:
                Qry.SQLText =
                    "update bp_models set "
                    "   id = :id, "
                    "   version = :version "
                    "where "
                    "   form_type = :form_type and "
                    "   dev_model = :dev_model ";
                Qry.CreateVariable("form_type", otString, NodeAsStringFast("form_type", fieldNode));
                Qry.CreateVariable("dev_model", otString, NodeAsStringFast("dev_model", fieldNode));
                Qry.CreateVariable("id", otInteger, NodeAsIntegerFast("id", fieldNode));
                Qry.CreateVariable("version", otInteger, NodeAsIntegerFast("version", fieldNode));
                break;
            default:
                throw Exception("DevTuningInterface::BPListCommit: Unknown BPState %d", (int)state);
        }
        try {
            Qry.Execute();
            if(state == bpAdd) {
                if(!addListNode)
                    addListNode = NewTextChild(resNode, "list");
                NewTextChild(addListNode, "id", Qry.GetVariableAsInteger("id"));
            }
        } catch( EOracleError E ) {
            switch( E.Code ) {
              case 1: throw UserException("Нарушена уникальность данных");
              case 1400:
              case 1407: throw UserException("Не указано значение в одном из обязательных для заполнения полей");
              case 2291: throw UserException("Значение одного из полей ссылается на несуществующие данные");
              case 2292: throw UserException("Невозможно изменить/удалить значение, на которое ссылаются другие данные");
              default: throw;
            }
        }
    }
}

void DevTuningInterface::BTListCommit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr listNode = NodeAsNode("list", reqNode);
    xmlNodePtr currNode = listNode->children;
    for(; currNode; currNode = currNode->next) {
        xmlNodePtr fieldNode = currNode->children;
        TBPState state = (TBPState)NodeAsIntegerFast("state", fieldNode);
        TQuery Qry(&OraSession);
        string form_type, dev_model;
        switch(state) {
            case bpAdd:
                Qry.SQLText =
                    "insert into bt_models ( "
                    "   form_type, "
                    "   dev_model, "
                    "   num, "
                    "   id, "
                    "   version "
                    ") values ( "
                    "   :form_type, "
                    "   :dev_model, "
                    "   :num, "
                    "   :id, "
                    "   :version "
                    ") ";
                Qry.CreateVariable("form_type", otString, NodeAsStringFast("form_type", fieldNode));
                Qry.CreateVariable("dev_model", otString, NodeAsStringFast("dev_model", fieldNode));
                Qry.DeclareVariable("num", otInteger);
                Qry.DeclareVariable("id", otInteger);
                Qry.DeclareVariable("version", otInteger);
                break;
            case bpDel:
                Qry.SQLText =
                    "delete from bt_models where "
                    "   form_type = :form_type and "
                    "   dev_model = :dev_model ";
                Qry.CreateVariable("form_type", otString, NodeAsStringFast("form_type", fieldNode));
                Qry.CreateVariable("dev_model", otString, NodeAsStringFast("dev_model", fieldNode));
                break;
            case bpUpd:
                Qry.SQLText =
                    "declare "
                    "   btRow bt_models%rowtype; "
                    "begin "
                    "   begin "
                    "     select * into btRow from bt_models where "
                    "         form_type = :form_type and "
                    "         num = :num and "
                    "         dev_model = :dev_model for update; "
                    "     update bt_models set "
                    "        id = :id, "
                    "        version = :version "
                    "     where "
                    "        form_type = :form_type and "
                    "        num = :num and "
                    "        dev_model = :dev_model; "
                    "   exception "
                    "     when no_data_found then "
                    "       insert into bt_models ( "
                    "          form_type, "
                    "          dev_model, "
                    "          num, "
                    "          id, "
                    "          version "
                    "       ) values ( "
                    "          :form_type, "
                    "          :dev_model, "
                    "          :num, "
                    "          :id, "
                    "          :version "
                    "       ); "
                    "     when others then raise; "
                    "   end; "
                    "end; ";
                Qry.CreateVariable("form_type", otString, NodeAsStringFast("form_type", fieldNode));
                Qry.CreateVariable("dev_model", otString, NodeAsStringFast("dev_model", fieldNode));
                Qry.DeclareVariable("num", otInteger);
                Qry.DeclareVariable("id", otInteger);
                Qry.DeclareVariable("version", otInteger);
                break;
            default:
                throw Exception("DevTuningInterface::BPListCommit: Unknown BPState %d", (int)state);
        }
        try {
            xmlNodePtr descrListNode;
            int num = 0;
            switch(state) {
                case bpAdd:
                    descrListNode = NodeAsNodeFast("descr_list", fieldNode);
                    descrListNode = descrListNode->children;
                    while(descrListNode) {
                        Qry.SetVariable("num", num);
                        Qry.SetVariable("id", NodeAsInteger("id", descrListNode));
                        Qry.SetVariable("version", NodeAsInteger("version", descrListNode));
                        Qry.Execute();
                        num++;
                        descrListNode = descrListNode->next;
                    }
                    break;
                case bpUpd:
                    descrListNode = NodeAsNodeFast("descr_list", fieldNode);
                    descrListNode = descrListNode->children;
                    while(descrListNode) {
                        Qry.SetVariable("num", num);
                        Qry.SetVariable("id", NodeAsInteger("id", descrListNode));
                        Qry.SetVariable("version", NodeAsInteger("version", descrListNode));
                        Qry.Execute();
                        num++;
                        descrListNode = descrListNode->next;
                    }
                    Qry.DeleteVariable("id");
                    Qry.DeleteVariable("version");
                    Qry.SQLText =
                        "delete from bt_models where "
                        "   form_type = :form_type and "
                        "   dev_model = :dev_model and "
                        "   num = :num ";
                    for(int i = num; i < 4; i++) {
                        Qry.SetVariable("num", i);
                        Qry.Execute();
                    }
                    break;
                default:
                    Qry.Execute();
                    break;
            }
        } catch( EOracleError E ) {
            switch( E.Code ) {
              case 1: throw UserException("Нарушена уникальность данных");
              case 1400:
              case 1407: throw UserException("Не указано значение в одном из обязательных для заполнения полей");
              case 2291: throw UserException("Значение одного из полей ссылается на несуществующие данные");
              case 2292: throw UserException("Невозможно изменить/удалить значение, на которое ссылаются другие данные");
              default: throw;
            }
        }
    }
}

void DevTuningInterface::BPListCommit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr listNode = NodeAsNode("list", reqNode);
    xmlNodePtr currNode = listNode->children;
    for(; currNode; currNode = currNode->next) {
        xmlNodePtr fieldNode = currNode->children;
        TBPState state = (TBPState)NodeAsIntegerFast("state", fieldNode);
        TQuery Qry(&OraSession);
        string form_type, dev_model;
        switch(state) {
            case bpAdd:
                Qry.SQLText =
                    "insert into bp_models ( "
                    "   form_type, "
                    "   dev_model, "
                    "   id, "
                    "   version "
                    ") values ( "
                    "   :form_type, "
                    "   :dev_model, "
                    "   :id, "
                    "   :version "
                    ") ";
                Qry.CreateVariable("form_type", otString, NodeAsStringFast("form_type", fieldNode));
                Qry.CreateVariable("dev_model", otString, NodeAsStringFast("dev_model", fieldNode));
                Qry.CreateVariable("id", otInteger, NodeAsIntegerFast("id", fieldNode));
                Qry.CreateVariable("version", otInteger, NodeAsIntegerFast("version", fieldNode));
                break;
            case bpDel:
                Qry.SQLText =
                    "delete from bp_models where "
                    "   form_type = :form_type and "
                    "   dev_model = :dev_model ";
                Qry.CreateVariable("form_type", otString, NodeAsStringFast("form_type", fieldNode));
                Qry.CreateVariable("dev_model", otString, NodeAsStringFast("dev_model", fieldNode));
                break;
            case bpUpd:
                Qry.SQLText =
                    "update bp_models set "
                    "   id = :id, "
                    "   version = :version "
                    "where "
                    "   form_type = :form_type and "
                    "   dev_model = :dev_model ";
                Qry.CreateVariable("form_type", otString, NodeAsStringFast("form_type", fieldNode));
                Qry.CreateVariable("dev_model", otString, NodeAsStringFast("dev_model", fieldNode));
                Qry.CreateVariable("id", otInteger, NodeAsIntegerFast("id", fieldNode));
                Qry.CreateVariable("version", otInteger, NodeAsIntegerFast("version", fieldNode));
                break;
            default:
                throw Exception("DevTuningInterface::BPListCommit: Unknown BPState %d", (int)state);
        }
        try {
            Qry.Execute();
        } catch( EOracleError E ) {
            switch( E.Code ) {
              case 1: throw UserException("Нарушена уникальность данных");
              case 1400:
              case 1407: throw UserException("Не указано значение в одном из обязательных для заполнения полей");
              case 2291: throw UserException("Значение одного из полей ссылается на несуществующие данные");
              case 2292: throw UserException("Невозможно изменить/удалить значение, на которое ссылаются другие данные");
              default: throw;
            }
        }
    }
}

void DevTuningInterface::LoadPrnForms(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   prn_forms.id, "
        "   prn_forms.descr, "
        "   prn_forms.version, "
        "   prn_forms.fmt_type "
        "from "
        "   prn_forms, "
        "   ( "
        "       select "
        "           id, "
        "           max(version) version "
        "       from "
        "           prn_forms "
        "       group by "
        "           id "
        "   ) a "
        "where "
        "   prn_forms.op_type = :op_type and "
        "   nvl2(:fmt_type, prn_forms.fmt_type, ' ') = nvl(:fmt_type, ' ') and "
        "   prn_forms.id = a.id and "
        "   prn_forms.version = a.version "
        "order by "
        "   prn_forms.descr, "
        "   prn_forms.version, "
        "   prn_forms.fmt_type ";
    Qry.CreateVariable("op_type", otString, NodeAsString("op_type", reqNode));
    string fmt_type;
    if(xmlNodePtr node = GetNode("fmt_type", reqNode))
        fmt_type = NodeAsString(node);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.Execute();
    xmlNodePtr prnFormsNode = NewTextChild(resNode, "prn_forms");
    for(; !Qry.Eof; Qry.Next()) {
        xmlNodePtr rowNode = NewTextChild(prnFormsNode, "row");
        NewTextChild(rowNode, "id", Qry.FieldAsString("id"));
        NewTextChild(rowNode, "descr", Qry.FieldAsString("descr"));
        NewTextChild(rowNode, "version", Qry.FieldAsString("version"));
        NewTextChild(rowNode, "fmt_type", Qry.FieldAsString("fmt_type"));
    }
}
