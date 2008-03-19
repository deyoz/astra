#include "design_blank.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_utils.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;

void DesignBlankInterface::PrevNext(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TDocType doc = DecodeDocType(NodeAsString("doc_type", reqNode));
    int delta = NodeAsInteger("delta", reqNode);
    TQuery Qry(&OraSession);        
    switch (doc) {
        case dtBP:
            Qry.SQLText =
                "select "
                "   form, "
                "   data "
                "from "
                "   bp_forms "
                "where "
                "   prn_type = :prn_type and "
                "   bp_type = :bp_type and "
                "   version = :version";
            break;
        case dtBT:
            Qry.SQLText =
                "select "
                "   form, "
                "   data "
                "from "
                "   bt_forms "
                "where "
                "   prn_type = :prn_type and "
                "   tag_type = :bp_type and "
                "   num = :num and "
                "   version = :version";
            Qry.CreateVariable("num", otInteger, NodeAsInteger("num", reqNode));
            break;
        case dtReceipt:
            Qry.SQLText =
                "select "
                "   null form, "
                "   data "
                "from "
                "   br_forms "
                "where "
                "   prn_type = :prn_type and "
                "   form_type = :bp_type and "
                "   version = :version";
            break;
            default:
                throw Exception("DesignBlankInterface::PrevNext: unsupported doc type %d", (int)doc);
                break;
    }
    Qry.CreateVariable("prn_type", otInteger, NodeAsInteger("prn_type", reqNode));
    Qry.CreateVariable("bp_type", otString, NodeAsString("blank_type", reqNode));
    Qry.CreateVariable("version", otInteger, NodeAsInteger("version", reqNode) + delta);
    Qry.Execute();
    if(Qry.Eof) throw Exception("PrevNext: data not found");
    NewTextChild(resNode, "form", Qry.FieldAsString("form"));
    NewTextChild(resNode, "data", Qry.FieldAsString("data"));
}

void DesignBlankInterface::Save(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TDocType doc = DecodeDocType(NodeAsString("doc_type", reqNode));

    xmlNodePtr formNode = GetNode("form", reqNode);

    TQuery Qry(&OraSession);
    string SQLText;
    if(formNode) {
        switch(doc) {
            case dtBP:
                SQLText =
                    "begin "
                    "   delete from bp_forms where "
                    "       bp_type = :bp_type and "
                    "       prn_type = :prn_type and "
                    "       version > :version; "
                    "   insert into bp_forms ( "
                    "       bp_type, "
                    "       prn_type, "
                    "       version, "
                    "       form, "
                    "       data "
                    "   ) values ( "
                    "       :bp_type, "
                    "       :prn_type, "
                    "       :version + 1, "
                    "       :form, "
                    "       :data "
                    "   ); "
                    "end;";
                Qry.CreateVariable("form", otString, NodeAsString("form", reqNode));
                break;
            case dtBT:
                SQLText =
                    "begin "
                    "   delete from bt_forms where "
                    "       tag_type = :bp_type and "
                    "       prn_type = :prn_type and "
                    "       num = :num and "
                    "       version > :version; "
                    "   insert into bt_forms ( "
                    "       tag_type, "
                    "       prn_type, "
                    "       num, "
                    "       version, "
                    "       form, "
                    "       data "
                    "   ) values ( "
                    "       :bp_type, "
                    "       :prn_type, "
                    "       :num, "
                    "       :version + 1, "
                    "       :form, "
                    "       :data "
                    "   ); "
                    "end;";
                Qry.CreateVariable("form", otString, NodeAsString("form", reqNode));
                Qry.CreateVariable("num", otInteger, NodeAsInteger("num", reqNode));
                break;
            case dtReceipt:
                SQLText =
                    "begin "
                    "   delete from br_forms where "
                    "       form_type = :bp_type and "
                    "       prn_type = :prn_type and "
                    "       version > :version; "
                    "   insert into br_forms ( "
                    "       form_type, "
                    "       prn_type, "
                    "       version, "
                    "       data "
                    "   ) values ( "
                    "       :bp_type, "
                    "       :prn_type, "
                    "       :version + 1, "
                    "       :data "
                    "   ); "
                    "end;";
                break;
            default:
                throw Exception("DesignBlankInterface::Save: unsupported doc type %d", (int)doc);
                break;
        }
        Qry.CreateVariable("data", otString, NodeAsString("data", reqNode));
    } else {
        switch(doc) {
            case dtBP:
                SQLText =
                    "delete from bp_forms where "
                    "    bp_type = :bp_type and "
                    "    prn_type = :prn_type and "
                    "    version > :version ";
                break;
            case dtBT:
                SQLText =
                    "delete from bt_forms where "
                    "    tag_type = :bp_type and "
                    "    prn_type = :prn_type and "
                    "    num = :num and "
                    "    version > :version ";
                Qry.CreateVariable("num", otInteger, NodeAsInteger("num", reqNode));
                break;
            case dtReceipt:
                SQLText =
                    "delete from br_forms where "
                    "    form_type = :bp_type and "
                    "    prn_type = :prn_type and "
                    "    version > :version ";
                break;
            default:
                throw Exception("DesignBlankInterface::Save: (1) unsupported doc type %d", (int)doc);
                break;
        }
    }
    Qry.SQLText = SQLText;

    Qry.CreateVariable("bp_type", otString, NodeAsString("blank_type", reqNode));
    Qry.CreateVariable("prn_type", otInteger, NodeAsInteger("prn_type", reqNode));
    Qry.CreateVariable("version", otInteger, NodeAsInteger("version", reqNode));

    Qry.Execute();
}

void DesignBlankInterface::GetBlanksList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TDocType doc = DecodeDocType(NodeAsString("doc_type", reqNode));

    TQuery Qry(&OraSession);        
    switch(doc) {
        case dtBP:
            Qry.SQLText =
                "select  "
                "   bp_forms.version, "
                "   bp_forms.bp_type,  "
                "   0 num, "
                "   bp_forms.form,  "
                "   bp_forms.data  "
                "from  "
                "   bp_forms, "
                "   ( "
                "    select "
                "        bp_type, "
                "        prn_type, "
                "        max(version) version "
                "    from "
                "        bp_forms "
                "    group by "
                "        bp_type, "
                "        prn_type "
                "   ) a "
                "where  "
                "   a.prn_type = :prn_type and "
                "   a.bp_type = bp_forms.bp_type and "
                "   a.prn_type = bp_forms.prn_type and "
                "   a.version = bp_forms.version ";
            break;
        case dtBT:
            Qry.SQLText =
                "select  "
                "   bt_forms.version, "
                "   bt_forms.tag_type bp_type,  "
                "   bt_forms.num, "
                "   bt_forms.form,  "
                "   bt_forms.data  "
                "from  "
                "   bt_forms, "
                "   ( "
                "    select "
                "        tag_type, "
                "        prn_type, "
                "        num, "
                "        max(version) version "
                "    from "
                "        bt_forms "
                "    group by "
                "        tag_type, "
                "        prn_type, "
                "        num "
                "   ) a "
                "where  "
                "   a.prn_type = :prn_type and "
                "   a.tag_type = bt_forms.tag_type and "
                "   a.prn_type = bt_forms.prn_type and "
                "   a.num = bt_forms.num and "
                "   a.version = bt_forms.version "
                "order by "
                "   bt_forms.tag_type, "
                "   bt_forms.num ";
            break;
        case dtReceipt:
            Qry.SQLText =
                "select "
                "   br_forms.version, "
                "   br_forms.form_type bp_type, "
                "   0 num, "
                "   null form, "
                "   br_forms.data "
                "from "
                "   br_forms, "
                "   ( "
                "    select "
                "        form_type, "
                "        prn_type, "
                "        max(version) version "
                "    from "
                "        br_forms "
                "    group by "
                "        form_type, "
                "        prn_type "
                "   ) a "
                "where "
                "   a.prn_type = :prn_type and "
                "   a.form_type = br_forms.form_type and "
                "   a.prn_type = br_forms.prn_type and "
                "   a.version = br_forms.version ";
            break;
        default:
            throw Exception("DesignBlankInterface::GetBlanksList: unsupported doc type %d", (int)doc);
            break;
    }
    Qry.CreateVariable("prn_type", otInteger, NodeAsInteger("prn_type", reqNode));
    Qry.Execute();
    if(Qry.Eof) throw Exception("Нет бланков для редактирования");
    xmlNodePtr blanksNode = NewTextChild(resNode, "blanks");
    string bp_type;
    xmlNodePtr blankNode;
    xmlNodePtr itemsNode;
    while(!Qry.Eof) {
        if(!(
                    Qry.FieldIsNULL("data")
            )
          ) {
            string bp_type_tmp = Qry.FieldAsString("bp_type");
            if(bp_type != bp_type_tmp) {
                bp_type = bp_type_tmp;
                blankNode = NewTextChild(blanksNode, "blank");
                NewTextChild(blankNode, "bp_type", Qry.FieldAsString("bp_type"));
                itemsNode = NewTextChild(blankNode, "items");
            }
            xmlNodePtr itemNode = NewTextChild(itemsNode, "item");
            NewTextChild(itemNode, "version", Qry.FieldAsInteger("version"));
            NewTextChild(itemNode, "form", Qry.FieldAsString("form"));
            NewTextChild(itemNode, "data", Qry.FieldAsString("data"));
        }
        Qry.Next();
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void DesignBlankInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
