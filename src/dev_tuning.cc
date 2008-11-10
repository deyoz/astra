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
#include "xml_stuff.h"
#include "cache.h"
//!!!
#include <fstream>

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
        "   prn_form_vers.version, "
        "   prn_forms.name||' ('||prn_form_vers.descr||')' prn_form "
        "from "
        "   bp_models, "
        "   prn_forms, "
        "   prn_form_vers, "
        "   dev_models "
        "where "
        "   bp_models.id = prn_forms.id and "
        "   bp_models.dev_model = dev_models.code and "
        "   bp_models.id = prn_form_vers.id and "
        "   bp_models.version = prn_form_vers.version "
        "order by "
        "   bp_models.form_type, "
        "   dev_models.name, "
        "   prn_forms.fmt_type, "
        "   prn_forms.name ";
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
        "   prn_forms.version,  "
        "   prn_forms.fmt_type  "
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
    if(Qry.Eof)
        throw UserException("Форма не найдена. Обновите данные.");
    NewTextChild(prnFormNode, "form", Qry.FieldAsString("form"));
    NewTextChild(prnFormNode, "data", Qry.FieldAsString("data"));
    NewTextChild(prnFormNode, "descr", Qry.FieldAsString("descr"));
    NewTextChild(prnFormNode, "version", Qry.FieldAsString("version"));
    NewTextChild(prnFormNode, "fmt_type", Qry.FieldAsString("fmt_type"));
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
        throw UserException("Информация по форме недоступна. Обновите данные.");
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
    try {
        Qry.Execute();
    } catch(EOracleError E) {
        if(E.Code == 1403)
            throw UserException("Информация по форме недоступна. Обновите данные.");
        else
            throw;
    } catch(...) {
        throw;
    }
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
        string form_type, dev_model, fmt_type;
        int id, version;
        switch(state) {
            case bpAdd:
                form_type = NodeAsStringFast("form_type", fieldNode);
                dev_model = NodeAsStringFast("dev_model", fieldNode);
                fmt_type = NodeAsStringFast("fmt_type", fieldNode);
                id = NodeAsIntegerFast("id", fieldNode);
                version = NodeAsIntegerFast("version", fieldNode);
                {
                    TQuery Qry(&OraSession);
                    Qry.SQLText = "select fmt_type from prn_forms where id = :id and version = :version";
                    Qry.CreateVariable("id", otInteger, id);
                    Qry.CreateVariable("version", otInteger, version);
                    Qry.Execute();
                    if(Qry.Eof)
                        throw Exception("BPListCommit bpAdd: prn_form not found, id: %d, version: %d", id, version);
                    if(Qry.FieldAsString(0) != fmt_type)
                        throw UserException("Формат формы не соответствует формату устройства");
                }
                Qry.SQLText =
                    "insert into bp_models ( "
                    "   form_type, "
                    "   dev_model, "
                    "   fmt_type, "
                    "   id, "
                    "   version "
                    ") values ( "
                    "   :form_type, "
                    "   :dev_model, "
                    "   :fmt_type, "
                    "   :id, "
                    "   :version "
                    ") ";
                Qry.CreateVariable("form_type", otString, form_type);
                Qry.CreateVariable("dev_model", otString, dev_model);
                Qry.CreateVariable("fmt_type", otString, fmt_type);
                Qry.CreateVariable("id", otInteger, id);
                Qry.CreateVariable("version", otInteger, version);
                break;
            case bpDel:
                Qry.SQLText =
                    "delete from bp_models where "
                    "   form_type = :form_type and "
                    "   fmt_type = :fmt_type and "
                    "   dev_model = :dev_model ";
                Qry.CreateVariable("form_type", otString, NodeAsStringFast("form_type", fieldNode));
                Qry.CreateVariable("dev_model", otString, NodeAsStringFast("dev_model", fieldNode));
                Qry.CreateVariable("fmt_type", otString, NodeAsStringFast("fmt_type", fieldNode));
                break;
            case bpUpd:
                Qry.SQLText =
                    "update bp_models set "
                    "   id = :id, "
                    "   version = :version "
                    "where "
                    "   form_type = :form_type and "
                    "   fmt_type = :fmt_type and "
                    "   dev_model = :dev_model ";
                Qry.CreateVariable("form_type", otString, NodeAsStringFast("form_type", fieldNode));
                Qry.CreateVariable("dev_model", otString, NodeAsStringFast("dev_model", fieldNode));
                Qry.CreateVariable("fmt_type", otString, NodeAsStringFast("fmt_type", fieldNode));
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
        "  prn_forms.id, "
        "  prn_forms.fmt_type, "
        "  prn_forms.name, "
        "  prn_form_vers.version, "
        "  prn_form_vers.descr "
        "from "
        "  prn_forms, "
        "  prn_form_vers "
        "where "
        "  prn_forms.op_type = :op_type and "
        "  prn_forms.id = prn_form_vers.id "
        "order by "
        "  prn_forms.fmt_type, "
        "  prn_forms.name, "
        "  prn_form_vers.version ";
    Qry.CreateVariable("op_type", otString, NodeAsString("op_type", reqNode));
    Qry.Execute();
    xmlNodePtr prnFormsNode = NewTextChild(resNode, "prn_forms");
    if(!Qry.Eof) {
        int col_id = Qry.FieldIndex("id");
        int col_fmt_type = Qry.FieldIndex("fmt_type");
        int col_name = Qry.FieldIndex("name");
        int col_version = Qry.FieldIndex("version");
        int col_descr = Qry.FieldIndex("descr");
        string name;
        xmlNodePtr rowNode = NULL;
        xmlNodePtr versNode = NULL;
        for(; !Qry.Eof; Qry.Next()) {
            string tmp_name = Qry.FieldAsString(col_name);
            if(name != tmp_name) {
                name = tmp_name;
                rowNode = NewTextChild(prnFormsNode, "row");
                NewTextChild(rowNode, "id", Qry.FieldAsInteger(col_id));
                NewTextChild(rowNode, "fmt_type", Qry.FieldAsString(col_fmt_type));
                NewTextChild(rowNode, "name", name);
                versNode = NewTextChild(rowNode, "vers");
            }
            xmlNodePtr itemNode = NewTextChild(versNode, "item");
            NewTextChild(itemNode, "vers", Qry.FieldAsInteger(col_version));
            NewTextChild(itemNode, "descr", Qry.FieldAsString(col_descr));
        }
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

/*
<rows tid="-1">
  <row pr_del="0">
    <col>ATB</col>
    <col>ATB-формат</col>
  </row>
  <row pr_del="0">
    <col>BTP</col>
    <col>BTP-формат</col>
  </row>
  <row pr_del="0">
    <col>EPL2</col>
    <col>EPL2-формат</col>
  </row>
  <row pr_del="0">
    <col>ZPL2</col>
    <col>ZPL2-формат</col>
  </row>
  <row pr_del="0">
    <col>TEXT</col>
    <col>Текстовой формат вывода</col>
  </row>
  <row pr_del="0">
    <col>EPSON</col>
    <col>EPSON-формат вывода</col>
  </row>
  <row pr_del="0">
    <col>FRX</col>
    <col>Формат генератора отчетов</col>
  </row>
  <row pr_del="0">
    <col>SCAN1</col>
    <col>Формат чтения штрих-кода</col>
  </row>
</rows>
*/

namespace TUNE {
    struct TField {
        string Name;
        int DataSize;
        TCacheFieldType DataType;
        TField() { DataType = ftUnknown; DataSize = -1; };
    };

    typedef vector<TField> TFields;

    void get_fields(xmlNodePtr ifaceNode, TFields &Fields)
    {
        xmlNodePtr curNode = ifaceNode->children;
        curNode = NodeAsNodeFast("fields", curNode);
        curNode = curNode->children;
        for(; curNode; curNode = curNode->next) {
            TField Field;
            xmlNodePtr paramNode = curNode->children;
            Field.Name = NodeAsStringFast("Name", paramNode);
            Field.DataSize = NodeAsIntegerFast("DataSize", paramNode);
            Field.DataType = TCacheFieldType(NodeAsIntegerFast("DataType", paramNode));
            Fields.push_back(Field);
        }
    }

    TFields CacheHeader(xmlNodePtr reqNode, xmlNodePtr dataNode)
    {
        xmlNodePtr paramsNode = NodeAsNodeFast("params", reqNode->children);
        paramsNode = paramsNode->children;
        string code = NodeAsStringFast("code", paramsNode);
//        int client_data_ver = NodeAsIntegerFast("data_ver", paramsNode);
        int client_interface_ver = NodeAsIntegerFast("interface_ver", paramsNode);

        // общие данные кэша
        NewTextChild(dataNode, "code", code);
        NewTextChild(dataNode, "Forbidden", 0);
        NewTextChild(dataNode, "ReadOnly", 0);
        NewTextChild(dataNode, "Keep_Locally", 0);

        string name = code + ".xml";
        xmlKeepBlanksDefault(0);
        xmlDocPtr ifaceDoc = xmlParseFile(name.c_str());
        int iface_version = -1;
        TFields Fields;
        try {
            if(ifaceDoc == NULL)
                throw Exception(name + " not parsed successfully.");
            xmlNodePtr ifaceNode = xmlDocGetRootElement(ifaceDoc);
            if(!ifaceNode)
                throw Exception(name + " is empty");
            get_fields(ifaceNode, Fields);
            iface_version = NodeAsInteger("/iface/fields/@tid", ifaceNode);
            if(iface_version > client_interface_ver) {
                xml_decode_nodelist(ifaceNode);
                xmlAddChild(dataNode, xmlCopyNode(ifaceNode, 1));
            }
            xmlFreeDoc(ifaceDoc);
        } catch(...) {
            xmlFreeDoc(ifaceDoc);
            throw;
        }
        return Fields;
    }

}

string FieldAsString(TCacheTable &cache, const TRow &row, string name)
{
    int FieldIndex = cache.FieldIndex(name);
    if ( FieldIndex < 0 )
        throw Exception( "FieldAsString: Ошибка при поиске поля " + name);
    return row.cols[FieldIndex];
}

string OldFieldAsString(TCacheTable &cache, const TRow &row, string name)
{
    int FieldIndex = cache.FieldIndex(name);
    if ( FieldIndex < 0 )
        throw Exception( "OldFieldAsString: Ошибка при поиске поля " + name);
    return row.old_cols[FieldIndex];
}

void BeforeApplyUpdates(TCacheTable &cache, const TRow &row)
{
    if(cache.GetCacheCode() == "PRN_FORMS") {
        if(
                row.status == usDeleted
          ) {
            TQuery Qry(&OraSession);
            Qry.SQLText = "select * from prn_form_vers where id = :id";
            Qry.CreateVariable("id", otString, ToInt(OldFieldAsString(cache, row, "id")));
            Qry.Execute();
            if(!Qry.Eof)
                throw UserException("Для формы " + OldFieldAsString(cache, row, "name") + " задан список версий.");
        }
    }
    if(cache.GetCacheCode() == "PRN_FORM_VERS") {
        if(row.status == usInserted) {
            if(FieldAsString(cache, row, "read_only") == "0")
                throw UserException("Версия " + FieldAsString(cache, row, "version") + ". При создании версии, редактирование должно быть включено.");
        }
        if(
                row.status == usModified or
                row.status == usDeleted
          ) {
            if(OldFieldAsString(cache, row, "read_only") == "0")
                throw UserException("Редактирование версии " + OldFieldAsString(cache, row, "version") + " запрещено.");
            if(row.status == usDeleted) {
                TQuery Qry(&OraSession);
                Qry.SQLText =
                    "select * from bp_models where id = :id and version = :version";
                Qry.CreateVariable("id", otString, ToInt(OldFieldAsString(cache, row, "id")));
                Qry.CreateVariable("version", otString, ToInt(OldFieldAsString(cache, row, "version")));
                Qry.Execute();
                if(!Qry.Eof)
                    throw UserException("Для версии " + OldFieldAsString(cache, row, "version") + " задан список бланков.");
            }
        }
    }
    if(
            cache.GetCacheCode() == "BLANK_LIST" or
            cache.GetCacheCode() == "BP_MODELS"
      ) {
        if(
                row.status == usInserted or
                row.status == usModified
          ) {
            string form_type = FieldAsString(cache, row, (cache.GetCacheCode() == "BLANK_LIST" ?  "form_type" : "code"));
            string dev_model = FieldAsString(cache, row, "dev_model");
            string fmt_type = FieldAsString(cache, row, "fmt_type");
            int id = ToInt(FieldAsString(cache, row, "id"));
            int version = ToInt(FieldAsString(cache, row, "version"));
            TQuery Qry(&OraSession);
            Qry.SQLText = "select * from prn_form_vers where id = :id and version = :version and form is null and data is null";
            Qry.CreateVariable("id", otInteger, id);
            Qry.CreateVariable("version", otInteger, version);
            Qry.Execute();
            if(!Qry.Eof) {
                string err;
                if(cache.GetCacheCode() == "BLANK_LIST")
                    err = "Вер. " + IntToString(version) + ". ";
                err += "Форма не заполнена.";
                throw UserException(err);
            }
            if(
                    row.status == usInserted and
                    cache.GetCacheCode() == "BLANK_LIST"
              ) {
                Qry.Clear();
                Qry.SQLText =
                    "select "
                    "   prn_forms.name form_name, "
                    "   bp_models.version "
                    "from "
                    "   prn_forms, "
                    "   bp_models "
                    "where "
                    "   bp_models.form_type = :form_type and "
                    "   bp_models.dev_model = :dev_model and "
                    "   bp_models.fmt_type = :fmt_type and "
                    "   bp_models.id = prn_forms.id ";
                Qry.CreateVariable("form_type", otString, form_type);
                Qry.CreateVariable("dev_model", otString, dev_model);
                Qry.CreateVariable("fmt_type", otString, fmt_type);
                Qry.Execute();
                if(!Qry.Eof) {
                    throw UserException(
                            "На бланк " + FieldAsString(cache, row, "form_type") + " " +
                            dev_model + " " + fmt_type + " уже назначена форма " + 
                            Qry.FieldAsString("form_name") + " Вер. " + IntToString(version) + "."
                            );
                }
            }
        }
    }
}

void DevTuningInterface::ApplyCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgTrace(TRACE2, "DevTuningInterface::ApplyCache");
    TCacheTable cache;
    cache.Init(reqNode);
    if ( cache.changeIfaceVer() )
        throw UserException( "Версия интерфейса изменилась. Обновите данные." );
    cache.OnBeforeApply = BeforeApplyUpdates;
    cache.ApplyUpdates( reqNode );
    cache.refresh();
    tst();
    SetProp(resNode, "handle", "1");
    xmlNodePtr ifaceNode = NewTextChild(resNode, "interface");
    SetProp(ifaceNode, "id", "cache");
    SetProp(ifaceNode, "ver", "1");
    cache.buildAnswer(resNode);
    showMessage( "Изменения успешно сохранены" );
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void DevTuningInterface::Cache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgTrace(TRACE2, "DevTuningInterface::Cache, reqNode->Name=%s, resNode->Name=%s",
            (char*)reqNode->name,(char*)resNode->name);

    TCacheTable cache;
    cache.Init(reqNode);
    cache.refresh();
    SetProp(resNode, "handle", "1");
    xmlNodePtr ifaceNode = NewTextChild(resNode, "interface");
    SetProp(ifaceNode, "id", "cache");
    SetProp(ifaceNode, "ver", "1");
    cache.buildAnswer(resNode);
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

const char *not_avail_err = "Информация по форме недоступна. Обновите данные.";

void DevTuningInterface::Load(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int id = NodeAsInteger("id", reqNode);
    int version = NodeAsInteger("version", reqNode);
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select descr, form, data, read_only from prn_form_vers where "
        "   id = :id and "
        "   version = :version";
    Qry.CreateVariable("id", otInteger, id);
    Qry.CreateVariable("version", otInteger, version);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException(not_avail_err);
    xmlNodePtr prnFormNode = NewTextChild(resNode, "prn_form");
    NewTextChild(prnFormNode, "form", Qry.FieldAsString("form"));
    NewTextChild(prnFormNode, "data", Qry.FieldAsString("data"));
    NewTextChild(prnFormNode, "descr", Qry.FieldAsString("descr"));
    NewTextChild(prnFormNode, "read_only", Qry.FieldAsInteger("read_only"));
    Qry.Clear();
    Qry.SQLText = "select name from prn_forms where id = :id";
    Qry.CreateVariable("id", otInteger, id);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException(not_avail_err);
    NewTextChild(prnFormNode, "name", Qry.FieldAsString("name"));
    Qry.Clear();
    Qry.SQLText = "select id, version, descr, read_only from prn_form_vers where id = :id order by version";
    Qry.CreateVariable("id", otInteger, id);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException(not_avail_err);
    xmlNodePtr verLstNode = NewTextChild(prnFormNode, "verLst");
    for(; not Qry.Eof; Qry.Next()) {
        xmlNodePtr itemNode = NewTextChild(verLstNode, "item");
        NewTextChild(itemNode, "id", Qry.FieldAsInteger("id"));
        NewTextChild(itemNode, "version", Qry.FieldAsInteger("version"));
        NewTextChild(itemNode, "descr", Qry.FieldAsString("descr"));
        NewTextChild(itemNode, "read_only", Qry.FieldAsInteger("read_only"));
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void DevTuningInterface::UpdateCopy(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr node = reqNode->children;
    int id = NodeAsIntegerFast("id", node);
    int vers = NodeAsIntegerFast("vers", node);
    string form = NodeAsStringFast("form", node);
    string data = NodeAsStringFast("data", node);
    string descr = NodeAsStringFast("descr", node);
    TQuery Qry(&OraSession);
    if(*(char*)reqNode->name == 'U') // Update
        Qry.SQLText =
            "update prn_form_vers set form = :form, data = :data, descr = :descr where id = :id and version = :vers";
    else // Copy
        Qry.SQLText =
            "insert into prn_form_vers(id, version, descr, form, data, read_only) values(:id, :vers, :descr, :form, :data, 0)";
    Qry.CreateVariable("id", otInteger, id);
    Qry.CreateVariable("vers", otInteger, vers);
    Qry.CreateVariable("form", otString, form);
    Qry.CreateVariable("data", otString, data);
    Qry.CreateVariable("descr", otString, descr);
    Qry.Execute();
    if(Qry.RowsProcessed() == 0)
        throw UserException(not_avail_err);
}
