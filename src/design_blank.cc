#include "design_blank.h"
#define NICKNAME "DENIS"
#include "test.h"
#include "oralib.h"
#include "xml_unit.h"

using namespace std;
using namespace EXCEPTIONS;

void DesignBlankInterface::PrevNext(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int delta = NodeAsInteger("delta", reqNode);
    TQuery Qry(&OraSession);        
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
    xmlNodePtr formNode = GetNode("form", reqNode);

    TQuery Qry(&OraSession);
    string SQLText;
    if(formNode) {
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
        Qry.CreateVariable("data", otString, NodeAsString("data", reqNode));
    } else
        SQLText =
            "delete from bp_forms where "
            "    bp_type = :bp_type and "
            "    prn_type = :prn_type and "
            "    version > :version ";
    Qry.SQLText = SQLText;

    Qry.CreateVariable("bp_type", otString, NodeAsString("blank_type", reqNode));
    Qry.CreateVariable("prn_type", otInteger, NodeAsInteger("prn_type", reqNode));
    Qry.CreateVariable("version", otInteger, NodeAsInteger("version", reqNode));

    Qry.Execute();
}

void DesignBlankInterface::GetBlanksList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);        
    Qry.SQLText =
        "select  "
        "   bp_forms.version, "
        "   bp_forms.bp_type,  "
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
    Qry.CreateVariable("prn_type", otInteger, NodeAsInteger("prn_type", reqNode));
    Qry.Execute();
    if(Qry.Eof) throw Exception("��� ������� ��� ।���஢����");
    xmlNodePtr blanksNode = NewTextChild(resNode, "blanks");
    while(!Qry.Eof) {
        if(!(
                    Qry.FieldIsNULL("form") ||
                    Qry.FieldIsNULL("data")
            )
          ) {
            xmlNodePtr blankNode = NewTextChild(blanksNode, "blank");
            NewTextChild(blankNode, "bp_type", Qry.FieldAsString("bp_type"));
            xmlNodePtr itemsNode = NewTextChild(blankNode, "items");
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
