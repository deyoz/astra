#include <fstream>
#include "dev_tuning.h"
#include "basic.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "print.h"
#include "cache.h"
#include "jxtlib/xml_stuff.h"
#include "dev_utils.h"

#define NICKNAME "DEN"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

void BeforeApplyUpdates(TCacheTable &cache, const TRow &row, TQuery &applyQry, const TCacheQueryType qryType)
{
    if(cache.code() == "PRN_FORMS") {
        if(
                row.status == usDeleted
          ) {
            TQuery Qry(&OraSession);
            Qry.SQLText = "select * from prn_form_vers where id = :id";
            Qry.CreateVariable("id", otString, ToInt(cache.FieldOldValue("id", row)));
            Qry.Execute();
            if(!Qry.Eof)
                throw UserException("Для формы " + cache.FieldOldValue("name", row) + " задан список версий.");
        }
    }
    if(cache.code() == "PRN_FORM_VERS") {
        if(row.status == usInserted) {
            if(cache.FieldValue("read_only", row) == "0")
                throw UserException("Версия " + cache.FieldValue("version", row) + ". При создании версии, редактирование должно быть включено.");
        }
        if(
                row.status == usModified or
                row.status == usDeleted
          ) {
            if(cache.FieldOldValue("version", row) == "0")
                throw UserException("Редактирование версии 0 запрещено.");
            if(row.status == usModified)
                if(cache.FieldOldValue("read_only", row) == cache.FieldValue("read_only", row) and
                        cache.FieldOldValue("read_only", row) == "0")
                    throw UserException("Редактирование версии " + cache.FieldOldValue("version", row) + " запрещено.");
            if(row.status == usDeleted) {
                if(cache.FieldOldValue("read_only", row) == "0")
                    throw UserException("Редактирование версии " + cache.FieldOldValue("version", row) + " запрещено.");
                TQuery Qry(&OraSession);
                Qry.SQLText =
                    "select * from bp_models where id = :id and version = :version";
                Qry.CreateVariable("id", otString, ToInt(cache.FieldOldValue("id", row)));
                Qry.CreateVariable("version", otString, ToInt(cache.FieldOldValue("version", row)));
                Qry.Execute();
                if(!Qry.Eof)
                    throw UserException("Для версии " + cache.FieldOldValue("version", row) + " задан список бланков.");
            }
        }
    }
    if(
            cache.code() == "BLANK_LIST" or
            cache.code() == "BP_MODELS"
      ) {
        if(
                row.status == usInserted or
                row.status == usModified
          ) {
            string form_type = cache.FieldValue((cache.code() == "BLANK_LIST" ?  "form_type" : "code"), row);
            string dev_model = cache.FieldValue("dev_model", row);
            string fmt_type = cache.FieldValue("fmt_type", row);
            int id = ToInt(cache.FieldValue("id", row));
            int version = ToInt(cache.FieldValue("version", row));
            TQuery Qry(&OraSession);
            Qry.SQLText = "select * from prn_form_vers where id = :id and version = :version and form is null and data is null";
            Qry.CreateVariable("id", otInteger, id);
            Qry.CreateVariable("version", otInteger, version);
            Qry.Execute();
            if(!Qry.Eof) {
                string err;
                if(cache.code() == "BLANK_LIST")
                    err = "Вер. " + IntToString(version) + ". ";
                err += "Форма не заполнена.";
                throw UserException(err);
            }
            if(
                    row.status == usInserted and
                    cache.code() == "BLANK_LIST"
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
                            "На бланк " + cache.FieldValue("form_type", row) + " " +
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
        "select "
        "   prn_forms.fmt_type, "
        "   prn_form_vers.descr, "
        "   prn_form_vers.form, "
        "   prn_form_vers.data, "
        "   prn_form_vers.read_only "
        "from "
        "   prn_form_vers, "
        "   prn_forms "
        "where "
        "   prn_forms.id = :id and "
        "   prn_form_vers.id = prn_forms.id and "
        "   prn_form_vers.version = :version";
    Qry.CreateVariable("id", otInteger, id);
    Qry.CreateVariable("version", otInteger, version);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException(not_avail_err);
    xmlNodePtr prnFormNode = NewTextChild(resNode, "prn_form");
    NewTextChild(prnFormNode, "fmt_type", Qry.FieldAsString("fmt_type"));
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

///////////////////////////////////// Export stuff /////////////////////////////

struct TPrnFormsItem {
    string op_type, fmt_type, name;
};

struct TPrnFormVersRow {
    string descr, form, data;
    bool read_only;
    TPrnFormVersRow(): read_only(false) {};
};

struct TPrnFormVersKey {
    int id, version;
    TPrnFormVersKey(): id(NoExists), version(NoExists) {};
};

struct TPrnFormVersCmp {
    bool operator() (const TPrnFormVersKey &lr, const TPrnFormVersKey &rr) const
    {
        if(lr.id == rr.id)
            return lr.version < rr.version;
        else
            return lr.id < rr.id;
    }
};

struct TPectabItem {
    TDevOperType op_type;
    int num;
    string form_type, dev_model, fmt_type, form, data;
    TPectabItem(): op_type(dotUnknown), num(0) {};
};


struct TFormTypes {
    virtual void get_pectabs(vector<TPectabItem> &pectabs) = 0;
    virtual void add(string type) = 0;
    virtual void add(xmlNodePtr reqNode) = 0;
    virtual void ToXML(xmlNodePtr resNode) = 0;
    virtual void ToBase() = 0;
    virtual void PrnFormsToBase() = 0;
    virtual void copy(string src, string dest) = 0;
    virtual ~TFormTypes() {};
};

struct TBRTypesItem {
    string code, name, validator, basic_type;
    int series_len, no_len;
    bool pr_check_bit;
    TBRTypesItem(): series_len(NoExists), no_len(NoExists), pr_check_bit(false) {};
};

struct TBPTypesItem {
    string code, airline, airp, name;
};

struct TBRModelsItem {
    string form_type, dev_model, fmt_type;
    int id, version;
    TBRModelsItem(): id(NoExists), version(NoExists) {};
};

struct TBPModelsItem {
    string form_type, dev_model, fmt_type;
    int id, version;
    TBPModelsItem(): id(NoExists), version(NoExists) {};
};

struct TBTModelsItem {
    string form_type, dev_model, fmt_type;
    int id, version, num;
    TBTModelsItem(): id(NoExists), version(NoExists), num(NoExists) {};
};

struct TPrnForms {
    map<int, TPrnFormsItem> items;
    void add(int id);
    void add(xmlNodePtr reqNode);
    void ToXML(xmlNodePtr resNode);
    void ToBase();
};

void TPrnForms::ToBase()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  update prn_forms set "
        "    op_type = :op_type, "
        "    fmt_type = :fmt_type, "
        "    name = :name "
        "  where id = :id; "
        "  if sql%notfound then "
        "    insert into prn_forms ( "
        "      id, "
        "      op_type, "
        "      fmt_type, "
        "      name "
        "    ) values ( "
        "      :id, "
        "      :op_type, "
        "      :fmt_type, "
        "      :name "
        "    ); "
        "  end if; "
        "end; ";
    Qry.DeclareVariable("id", otInteger);
    Qry.DeclareVariable("op_type", otString);
    Qry.DeclareVariable("fmt_type", otString);
    Qry.DeclareVariable("name", otString);
    for(map<int, TPrnFormsItem>::iterator im = items.begin(); im != items.end(); im++) {
        Qry.SetVariable("id", im->first);
        Qry.SetVariable("op_type", im->second.op_type);
        Qry.SetVariable("fmt_type", im->second.fmt_type);
        Qry.SetVariable("name", im->second.name);
        Qry.Execute();
    }
}

void TPrnForms::add(xmlNodePtr reqNode)
{
    xmlNodePtr currNode = NodeAsNode("prn_forms", reqNode)->children;
    for(; currNode != NULL; currNode = currNode->next) {
        xmlNodePtr fastNode = currNode->children;
        int id = NodeAsIntegerFast("id", fastNode);
        items[id].op_type = NodeAsStringFast("op_type", fastNode);
        items[id].fmt_type = NodeAsStringFast("fmt_type", fastNode);
        items[id].name = NodeAsStringFast("name", fastNode);
    }
}

void TPrnForms::ToXML(xmlNodePtr resNode)
{
    if(not items.empty()) {
        xmlNodePtr prn_formsNode = NewTextChild(resNode, "prn_forms");
        for(map<int, TPrnFormsItem>::iterator it = items.begin(); it != items.end(); it++) {
            xmlNodePtr itemNode = NewTextChild(prn_formsNode, "item");
            NewTextChild(itemNode, "id", it->first);
            NewTextChild(itemNode, "op_type", it->second.op_type);
            NewTextChild(itemNode, "fmt_type", it->second.fmt_type);
            NewTextChild(itemNode, "name", it->second.name);
        }
    }
}

struct TPrnFormVers {
    map<TPrnFormVersKey, TPrnFormVersRow, TPrnFormVersCmp> items;
    void add(int id, int version);
    void add(xmlNodePtr resNode);
    void ToXML(xmlNodePtr resNode);
    void ToBase();
};

void TPrnFormVers::ToBase()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  update prn_form_vers set "
        "    descr = :descr, "
        "    form = :form, "
        "    data = :data, "
        "    read_only = :read_only "
        "  where "
        "    id = :id and "
        "    version = :version; "
        "  if sql%notfound then "
        "    insert into prn_form_vers ( "
        "      id, "
        "      version, "
        "      descr, "
        "      form, "
        "      data, "
        "      read_only "
        "    ) values ( "
        "      :id, "
        "      :version, "
        "      :descr, "
        "      :form, "
        "      :data, "
        "      :read_only "
        "    ); "
        "  end if; "
        "end; ";
    Qry.DeclareVariable("id", otInteger);
    Qry.DeclareVariable("version", otInteger);
    Qry.DeclareVariable("descr", otString);
    Qry.DeclareVariable("form", otString);
    Qry.DeclareVariable("data", otString);
    Qry.DeclareVariable("read_only", otString);
    for(map<TPrnFormVersKey, TPrnFormVersRow, TPrnFormVersCmp>::iterator im = items.begin(); im != items.end(); im++) {
        Qry.SetVariable("id", im->first.id);
        Qry.SetVariable("version", im->first.version);
        Qry.SetVariable("descr", im->second.descr);
        Qry.SetVariable("form", im->second.form);
        Qry.SetVariable("data", im->second.data);
        Qry.SetVariable("read_only", im->second.read_only);
        Qry.Execute();
    }
}

void TPrnFormVers::add(xmlNodePtr resNode)
{
    xmlNodePtr currNode = NodeAsNode("prn_form_vers", resNode)->children;
    for(; currNode != NULL; currNode = currNode->next) {
        xmlNodePtr fastNode = currNode->children;
        TPrnFormVersKey key;
        key.id = NodeAsIntegerFast("id", fastNode);
        key.version = NodeAsIntegerFast("version", fastNode);
        items[key].descr = NodeAsStringFast("descr", fastNode);
        items[key].form = NodeAsStringFast("form", fastNode);
        items[key].data = NodeAsStringFast("data", fastNode);
        items[key].read_only = NodeAsIntegerFast("read_only", fastNode) != 0;
    }
}

void TPrnFormVers::ToXML(xmlNodePtr resNode)
{
    if(not items.empty()) {
        xmlNodePtr prn_form_versNode = NewTextChild(resNode, "prn_form_vers");
        for(map<TPrnFormVersKey, TPrnFormVersRow, TPrnFormVersCmp>::iterator it = items.begin(); it != items.end(); it++) {
            xmlNodePtr itemNode = NewTextChild(prn_form_versNode, "item");
            NewTextChild(itemNode, "id", it->first.id);
            NewTextChild(itemNode, "version", it->first.version);
            NewTextChild(itemNode, "descr", it->second.descr);
            NewTextChild(itemNode, "form", it->second.form);
            NewTextChild(itemNode, "data", it->second.data);
            NewTextChild(itemNode, "read_only", it->second.read_only);
        }
    }
}

struct TBRModels {
    TPrnForms prn_forms;
    TPrnFormVers prn_form_vers;
    vector<TBRModelsItem> items;
    void add(string type);
    void add(xmlNodePtr reqNode);
    void ToXML(xmlNodePtr resNode);
    void ToBase();
};

struct TBPModels {
    TPrnForms prn_forms;
    TPrnFormVers prn_form_vers;
    vector<TBPModelsItem> items;
    void add(string type);
    void add(xmlNodePtr reqNode);
    void ToXML(xmlNodePtr resNode);
    void ToBase();
};

void TBRModels::ToBase()
{
    prn_forms.ToBase();
    prn_form_vers.ToBase();
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  update br_models set "
        "    id = :id, "
        "    version = :version "
        "  where "
        "    form_type = :form_type and "
        "    dev_model = :dev_model and "
        "    fmt_type = :fmt_type; "
        "  if sql%notfound then "
        "    insert into br_models ( "
        "      form_type, "
        "      dev_model, "
        "      fmt_type, "
        "      id, "
        "      version "
        "    ) values ( "
        "      :form_type, "
        "      :dev_model, "
        "      :fmt_type, "
        "      :id, "
        "      :version "
        "    ); "
        "  end if; "
        "end; ";
    Qry.DeclareVariable("form_type", otString);
    Qry.DeclareVariable("dev_model", otString);
    Qry.DeclareVariable("fmt_type", otString);
    Qry.DeclareVariable("id", otInteger);
    Qry.DeclareVariable("version", otInteger);
    for(vector<TBRModelsItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
        Qry.SetVariable("form_type", iv->form_type);
        Qry.SetVariable("dev_model", iv->dev_model);
        Qry.SetVariable("fmt_type", iv->fmt_type);
        Qry.SetVariable("id", iv->id);
        Qry.SetVariable("version", iv->version);
        Qry.Execute();
    }
}

void TBPModels::ToBase()
{
    prn_forms.ToBase();
    prn_form_vers.ToBase();
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  update bp_models set "
        "    id = :id, "
        "    version = :version "
        "  where "
        "    form_type = :form_type and "
        "    dev_model = :dev_model and "
        "    fmt_type = :fmt_type; "
        "  if sql%notfound then "
        "    insert into bp_models ( "
        "      form_type, "
        "      dev_model, "
        "      fmt_type, "
        "      id, "
        "      version "
        "    ) values ( "
        "      :form_type, "
        "      :dev_model, "
        "      :fmt_type, "
        "      :id, "
        "      :version "
        "    ); "
        "  end if; "
        "end; ";
    Qry.DeclareVariable("form_type", otString);
    Qry.DeclareVariable("dev_model", otString);
    Qry.DeclareVariable("fmt_type", otString);
    Qry.DeclareVariable("id", otInteger);
    Qry.DeclareVariable("version", otInteger);
    for(vector<TBPModelsItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
        Qry.SetVariable("form_type", iv->form_type);
        Qry.SetVariable("dev_model", iv->dev_model);
        Qry.SetVariable("fmt_type", iv->fmt_type);
        Qry.SetVariable("id", iv->id);
        Qry.SetVariable("version", iv->version);
        Qry.Execute();
    }
}

void TBRModels::add(xmlNodePtr reqNode)
{
    xmlNodePtr currNode = NodeAsNode("models", reqNode)->children;
    for(; currNode != NULL; currNode = currNode->next) {
        xmlNodePtr fastNode = currNode->children;
        TBRModelsItem item;
        item.form_type = NodeAsStringFast("form_type", fastNode);
        item.dev_model = NodeAsStringFast("dev_model", fastNode);
        item.fmt_type = NodeAsStringFast("fmt_type", fastNode);
        item.id = NodeAsIntegerFast("id", fastNode);
        item.version = NodeAsIntegerFast("version", fastNode);
        items.push_back(item);
    }
    prn_forms.add(reqNode);
    prn_form_vers.add(reqNode);
}

void TBPModels::add(xmlNodePtr reqNode)
{
    xmlNodePtr currNode = NodeAsNode("models", reqNode)->children;
    for(; currNode != NULL; currNode = currNode->next) {
        xmlNodePtr fastNode = currNode->children;
        TBPModelsItem item;
        item.form_type = NodeAsStringFast("form_type", fastNode);
        item.dev_model = NodeAsStringFast("dev_model", fastNode);
        item.fmt_type = NodeAsStringFast("fmt_type", fastNode);
        item.id = NodeAsIntegerFast("id", fastNode);
        item.version = NodeAsIntegerFast("version", fastNode);
        items.push_back(item);
    }
    prn_forms.add(reqNode);
    prn_form_vers.add(reqNode);
}

void TBRModels::ToXML(xmlNodePtr resNode)
{
    if(not items.empty()) {
        xmlNodePtr bp_modelsNode = NewTextChild(resNode, "models");
        for(vector<TBRModelsItem>::iterator it = items.begin(); it != items.end(); it++) {
            xmlNodePtr itemNode = NewTextChild(bp_modelsNode, "item");
            NewTextChild(itemNode, "form_type", it->form_type);
            NewTextChild(itemNode, "dev_model", it->dev_model);
            NewTextChild(itemNode, "fmt_type", it->fmt_type);
            NewTextChild(itemNode, "id", it->id);
            NewTextChild(itemNode, "version", it->version);
        }
    }
    prn_forms.ToXML(resNode);
    prn_form_vers.ToXML(resNode);
}

void TBPModels::ToXML(xmlNodePtr resNode)
{
    if(not items.empty()) {
        xmlNodePtr bp_modelsNode = NewTextChild(resNode, "models");
        for(vector<TBPModelsItem>::iterator it = items.begin(); it != items.end(); it++) {
            xmlNodePtr itemNode = NewTextChild(bp_modelsNode, "item");
            NewTextChild(itemNode, "form_type", it->form_type);
            NewTextChild(itemNode, "dev_model", it->dev_model);
            NewTextChild(itemNode, "fmt_type", it->fmt_type);
            NewTextChild(itemNode, "id", it->id);
            NewTextChild(itemNode, "version", it->version);
        }
    }
    prn_forms.ToXML(resNode);
    prn_form_vers.ToXML(resNode);
}

struct TBRTypes:TFormTypes {
    TBRModels br_models;
    vector<TBRTypesItem> items;
    void get_pectabs(vector<TPectabItem> &pectabs);
    void add(xmlNodePtr reqNode);
    void ToXML(xmlNodePtr resNode);
    void add(string type);
    void ToBase();
    void PrnFormsToBase();
    void copy(string src, string dest);
};

void TBRTypes::get_pectabs(vector<TPectabItem> &pectabs)
{
}

void TBRTypes::PrnFormsToBase()
{
    br_models.prn_forms.ToBase();
    br_models.prn_form_vers.ToBase();
}

void TBRTypes::copy(string src, string dest)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "delete from br_models where form_type = :dest";
    Qry.CreateVariable("dest", otString, dest);
    Qry.Execute();
    Qry.Clear();
    Qry.SQLText =
        "insert into br_models ( "
        "  form_type, "
        "  dev_model, "
        "  fmt_type, "
        "  id, "
        "  version "
        ") values ( "
        "  :form_type, "
        "  :dev_model, "
        "  :fmt_type, "
        "  :id, "
        "  :version "
        ") ";
    Qry.DeclareVariable("form_type", otString);
    Qry.DeclareVariable("dev_model", otString);
    Qry.DeclareVariable("fmt_type", otString);
    Qry.DeclareVariable("id", otInteger);
    Qry.DeclareVariable("version", otInteger);
    for(vector<TBRModelsItem>::iterator iv = br_models.items.begin(); iv != br_models.items.end(); iv++) {
        if(iv->form_type != src)
            continue;
        Qry.SetVariable("form_type", dest);
        Qry.SetVariable("dev_model", iv->dev_model);
        Qry.SetVariable("fmt_type", iv->fmt_type);
        Qry.SetVariable("id", iv->id);
        Qry.SetVariable("version", iv->version);
        Qry.Execute();
    }
}

struct TBPTypes:TFormTypes {
    TBPModels bp_models;
    vector<TBPTypesItem> items;
    void get_pectabs(vector<TPectabItem> &pectabs);
    void add(xmlNodePtr reqNode);
    void ToXML(xmlNodePtr resNode);
    void add(string type);
    void ToBase();
    void PrnFormsToBase();
    void copy(string src, string dest);
};

void TBPTypes::get_pectabs(vector<TPectabItem> &pectabs)
{
    for(vector<TBPModelsItem>::iterator iv = bp_models.items.begin(); iv != bp_models.items.end(); iv++) {
        TPectabItem pectab;
        pectab.form_type = iv->form_type;
        pectab.dev_model = iv->dev_model;
        pectab.fmt_type = iv->fmt_type;
        TPrnFormVersKey vers_k;
        vers_k.id = iv->id;
        vers_k.version = iv->version;
        map<TPrnFormVersKey, TPrnFormVersRow, TPrnFormVersCmp>::iterator im = bp_models.prn_form_vers.items.find(vers_k);
        if(im != bp_models.prn_form_vers.items.end()) {
            map<int, TPrnFormsItem>::iterator prn_forms_i = bp_models.prn_forms.items.find(iv->id);
            if(prn_forms_i == bp_models.prn_forms.items.end()) throw Exception("TBPTypes::get_pectabs: prn_form not found %d", iv->id);
            pectab.op_type = DecodeDevOperType(prn_forms_i->second.op_type);
            pectab.form = im->second.form;
            pectab.data = im->second.data;
            pectabs.push_back(pectab);
        }
    }
}

void TBPTypes::PrnFormsToBase()
{
    bp_models.prn_forms.ToBase();
    bp_models.prn_form_vers.ToBase();
}

void TBPTypes::copy(string src, string dest)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "  delete from bp_models where form_type = :dest ";
    Qry.CreateVariable("dest", otString, dest);
    Qry.Execute();
    Qry.Clear();
    Qry.SQLText =
        "insert into bp_models ( "
        "  form_type, "
        "  dev_model, "
        "  fmt_type, "
        "  id, "
        "  version "
        ") values ( "
        "  :form_type, "
        "  :dev_model, "
        "  :fmt_type, "
        "  :id, "
        "  :version "
        ") ";
    Qry.DeclareVariable("form_type", otString);
    Qry.DeclareVariable("dev_model", otString);
    Qry.DeclareVariable("fmt_type", otString);
    Qry.DeclareVariable("id", otInteger);
    Qry.DeclareVariable("version", otInteger);
    for(vector<TBPModelsItem>::iterator iv = bp_models.items.begin(); iv != bp_models.items.end(); iv++) {
        if(iv->form_type != src)
            continue;
        Qry.SetVariable("form_type", dest);
        Qry.SetVariable("dev_model", iv->dev_model);
        Qry.SetVariable("fmt_type", iv->fmt_type);
        Qry.SetVariable("id", iv->id);
        Qry.SetVariable("version", iv->version);
        Qry.Execute();
    }
}

void TBRTypes::ToBase()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  update form_types set "
        "    name = :name, "
        "    series_len = :series_len, "
        "    no_len = :no_len, "
        "    pr_check_bit = :pr_check_bit, "
        "    validator = :validator, "
        "    basic_type = :basic_type "
        "  where code = :code; "
        "  if sql%notfound then "
        "    insert into form_types( "
        "      code, "
        "      name, "
        "      series_len, "
        "      no_len, "
        "      pr_check_bit, "
        "      validator, "
        "      basic_type "
        "    ) values ( "
        "      :code, "
        "      :name, "
        "      :series_len, "
        "      :no_len, "
        "      :pr_check_bit, "
        "      :validator, "
        "      :basic_type "
        "    ); "
        "  end if; "
        "end; ";
    Qry.DeclareVariable("code", otString);
    Qry.DeclareVariable("name", otString);
    Qry.DeclareVariable("series_len", otInteger);
    Qry.DeclareVariable("no_len", otInteger);
    Qry.DeclareVariable("pr_check_bit", otInteger);
    Qry.DeclareVariable("validator", otString);
    Qry.DeclareVariable("basic_type", otString);
    for(vector<TBRTypesItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
        Qry.SetVariable("code", iv->code);
        Qry.SetVariable("name", iv->name);
        Qry.SetVariable("series_len", iv->series_len);
        Qry.SetVariable("no_len", iv->no_len);
        Qry.SetVariable("pr_check_bit", iv->pr_check_bit);
        Qry.SetVariable("validator", iv->validator);
        Qry.SetVariable("basic_type", iv->basic_type);
        Qry.Execute();
    }
    br_models.ToBase();
}

void TBPTypes::ToBase()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  update bp_types set "
        "    airline = :airline, "
        "    airp = :airp, "
        "    name = :name "
        "  where code = :code; "
        "  if sql%notfound then "
        "    insert into bp_types( "
        "      code, "
        "      airline, "
        "      airp, "
        "      name "
        "    ) values ( "
        "      :code, "
        "      :airline, "
        "      :airp, "
        "      :name "
        "    ); "
        "  end if; "
        "end; ";
    Qry.DeclareVariable("code", otString);
    Qry.DeclareVariable("airline", otString);
    Qry.DeclareVariable("airp", otString);
    Qry.DeclareVariable("name", otString);
    for(vector<TBPTypesItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
        Qry.SetVariable("code", iv->code);
        Qry.SetVariable("airline", iv->airline);
        Qry.SetVariable("airp", iv->airp);
        Qry.SetVariable("name", iv->name);
        Qry.Execute();
    }
    bp_models.ToBase();
}

void TBRTypes::add(xmlNodePtr reqNode)
{
    xmlNodePtr currNode = NodeAsNode("blank_types", reqNode)->children;
    for(; currNode != NULL; currNode = currNode->next) {
        xmlNodePtr fastNode = currNode->children;
        TBRTypesItem item;
        item.code = NodeAsStringFast("code", fastNode);
        item.name = NodeAsStringFast("name", fastNode);
        item.series_len = NodeAsIntegerFast("series_len", fastNode);
        item.no_len = NodeAsIntegerFast("no_len", fastNode);
        item.pr_check_bit = NodeAsIntegerFast("pr_check_bit", fastNode) != 0;
        item.validator = NodeAsStringFast("validator", fastNode);
        item.basic_type = NodeAsStringFast("basic_type", fastNode);
        items.push_back(item);
    }
    br_models.add(reqNode);
}

void TBPTypes::add(xmlNodePtr reqNode)
{
    xmlNodePtr currNode = NodeAsNode("blank_types", reqNode)->children;
    for(; currNode != NULL; currNode = currNode->next) {
        xmlNodePtr fastNode = currNode->children;
        TBPTypesItem item;
        item.code = NodeAsStringFast("code", fastNode);
        item.airline = NodeAsStringFast("airline", fastNode);
        item.airp = NodeAsStringFast("airp", fastNode);
        item.name = NodeAsStringFast("name", fastNode);
        items.push_back(item);
    }
    bp_models.add(reqNode);
}

void TBRTypes::ToXML(xmlNodePtr resNode)
{
    if(not items.empty()) {
        xmlNodePtr bp_typesNode = NewTextChild(resNode, "blank_types");
        for(vector<TBRTypesItem>::iterator it = items.begin(); it != items.end(); it++) {
            xmlNodePtr itemNode = NewTextChild(bp_typesNode, "item");
            NewTextChild(itemNode, "code", it->code);
            NewTextChild(itemNode, "name", it->name);
            NewTextChild(itemNode, "series_len", it->series_len);
            NewTextChild(itemNode, "no_len", it->no_len);
            NewTextChild(itemNode, "pr_check_bit", it->pr_check_bit);
            NewTextChild(itemNode, "validator", it->validator);
            NewTextChild(itemNode, "basic_type", it->basic_type);
        }
    }
    br_models.ToXML(resNode);
}

void TBPTypes::ToXML(xmlNodePtr resNode)
{
    if(not items.empty()) {
        xmlNodePtr bp_typesNode = NewTextChild(resNode, "blank_types");
        for(vector<TBPTypesItem>::iterator it = items.begin(); it != items.end(); it++) {
            xmlNodePtr itemNode = NewTextChild(bp_typesNode, "item");
            NewTextChild(itemNode, "code", it->code);
            NewTextChild(itemNode, "airline", it->airline);
            NewTextChild(itemNode, "airp", it->airp);
            NewTextChild(itemNode, "name", it->name);
        }
    }
    bp_models.ToXML(resNode);
}

void TBRTypes::add(string type)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   name, "
        "   series_len, "
        "   no_len, "
        "   pr_check_bit, "
        "   validator, "
        "   basic_type "
        "from "
        "   form_types "
        "where "
        "   code = :code ";
    Qry.CreateVariable("code", otString, type);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Информация по типу бланка %s недоступна. Обновите данные.", type.c_str());
    TBRTypesItem item;
    item.code = type;
    item.name = Qry.FieldAsString("name");
    item.series_len = Qry.FieldAsInteger("series_len");
    item.no_len = Qry.FieldAsInteger("no_len");
    item.pr_check_bit = Qry.FieldAsInteger("pr_check_bit") != 0;
    item.validator = Qry.FieldAsString("validator");
    item.basic_type = Qry.FieldAsString("basic_type");
    items.push_back(item);
    br_models.add(type);
}

void TBPTypes::add(string type)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   airline, "
        "   airp, "
        "   name "
        "from "
        "   bp_types "
        "where "
        "   code = :code ";
    Qry.CreateVariable("code", otString, type);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Информация по типу бланка %s недоступна. Обновите данные.", type.c_str());
    TBPTypesItem item;
    item.code = type;
    item.airline = Qry.FieldAsString("airline");
    item.airp = Qry.FieldAsString("airp");
    item.name = Qry.FieldAsString("name");
    items.push_back(item);
    bp_models.add(type);
}

struct TTagTypesItem {
    string code, airline, name, airp;
    int no_len, printable;
    TTagTypesItem(): no_len(NoExists), printable(NoExists) {};
};

struct TBTModels {
    TPrnForms prn_forms;
    TPrnFormVers prn_form_vers;
    vector<TBTModelsItem> items;
    void add(xmlNodePtr reqNode);
    void add(string type);
    void ToXML(xmlNodePtr resNode);
    void ToBase();
};

void TBTModels::ToBase()
{
    prn_forms.ToBase();
    prn_form_vers.ToBase();
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  update bt_models set "
        "    id = :id, "
        "    version = :version "
        "  where "
        "    form_type = :form_type and "
        "    dev_model = :dev_model and "
        "    num = :num and "
        "    fmt_type = :fmt_type; "
        "  if sql%notfound then "
        "    insert into bt_models ( "
        "      form_type, "
        "      dev_model, "
        "      num, "
        "      fmt_type, "
        "      id, "
        "      version "
        "    ) values ( "
        "      :form_type, "
        "      :dev_model, "
        "      :num, "
        "      :fmt_type, "
        "      :id, "
        "      :version "
        "    ); "
        "  end if; "
        "end; ";
    Qry.DeclareVariable("form_type", otString);
    Qry.DeclareVariable("dev_model", otString);
    Qry.DeclareVariable("num", otInteger);
    Qry.DeclareVariable("fmt_type", otString);
    Qry.DeclareVariable("id", otInteger);
    Qry.DeclareVariable("version", otInteger);
    for(vector<TBTModelsItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
        Qry.SetVariable("form_type", iv->form_type);
        Qry.SetVariable("dev_model", iv->dev_model);
        Qry.SetVariable("num", iv->num);
        Qry.SetVariable("fmt_type", iv->fmt_type);
        Qry.SetVariable("id", iv->id);
        Qry.SetVariable("version", iv->version);
        Qry.Execute();
    }
}

void TBTModels::add(xmlNodePtr reqNode)
{
    xmlNodePtr currNode = NodeAsNode("models", reqNode)->children;
    for(; currNode != NULL; currNode = currNode->next) {
        xmlNodePtr fastNode = currNode->children;
        TBTModelsItem item;
        item.form_type = NodeAsStringFast("form_type", fastNode);
        item.dev_model = NodeAsStringFast("dev_model", fastNode);
        item.fmt_type = NodeAsStringFast("fmt_type", fastNode);
        item.id = NodeAsIntegerFast("id", fastNode);
        item.version = NodeAsIntegerFast("version", fastNode);
        item.num = NodeAsIntegerFast("num", fastNode);
        items.push_back(item);
    }
    prn_forms.add(reqNode);
    prn_form_vers.add(reqNode);
}

void TBTModels::ToXML(xmlNodePtr resNode)
{
    if(not items.empty()) {
        xmlNodePtr bt_modelsNode = NewTextChild(resNode, "models");
        for(vector<TBTModelsItem>::iterator it = items.begin(); it != items.end(); it++) {
            xmlNodePtr itemNode = NewTextChild(bt_modelsNode, "item");
            NewTextChild(itemNode, "form_type", it->form_type);
            NewTextChild(itemNode, "dev_model", it->dev_model);
            NewTextChild(itemNode, "fmt_type", it->fmt_type);
            NewTextChild(itemNode, "id", it->id);
            NewTextChild(itemNode, "version", it->version);
            NewTextChild(itemNode, "num", it->num);
        }
    }
    prn_forms.ToXML(resNode);
    prn_form_vers.ToXML(resNode);
}

struct TTagTypes:TFormTypes {
    TBTModels bt_models;
    vector<TTagTypesItem> items;
    void get_pectabs(vector<TPectabItem> &pectabs);
    void add(xmlNodePtr reqNode);
    void ToXML(xmlNodePtr resNode);
    void add(string type);
    void ToBase();
    void PrnFormsToBase();
    void copy(string src, string dest);
};

void TTagTypes::get_pectabs(vector<TPectabItem> &pectabs)
{
}

void TTagTypes::PrnFormsToBase()
{
    bt_models.prn_forms.ToBase();
    bt_models.prn_form_vers.ToBase();
}

void TTagTypes::copy(string src, string dest)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "delete from bt_models where form_type = :dest";
    Qry.CreateVariable("dest", otString, dest);
    Qry.Execute();
    Qry.Clear();
    Qry.SQLText =
        "insert into bt_models ( "
        "  form_type, "
        "  dev_model, "
        "  num, "
        "  fmt_type, "
        "  id, "
        "  version "
        ") values ( "
        "  :form_type, "
        "  :dev_model, "
        "  :num, "
        "  :fmt_type, "
        "  :id, "
        "  :version "
        ") ";
    Qry.DeclareVariable("form_type", otString);
    Qry.DeclareVariable("dev_model", otString);
    Qry.DeclareVariable("num", otInteger);
    Qry.DeclareVariable("fmt_type", otString);
    Qry.DeclareVariable("id", otInteger);
    Qry.DeclareVariable("version", otInteger);
    for(vector<TBTModelsItem>::iterator iv = bt_models.items.begin(); iv != bt_models.items.end(); iv++) {
        if(iv->form_type != src)
            continue;
        Qry.SetVariable("form_type", dest);
        Qry.SetVariable("dev_model", iv->dev_model);
        Qry.SetVariable("num", iv->num);
        Qry.SetVariable("fmt_type", iv->fmt_type);
        Qry.SetVariable("id", iv->id);
        Qry.SetVariable("version", iv->version);
        Qry.Execute();
    }
}

void TTagTypes::ToBase()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  update tag_types set "
        "    airline = :airline, "
        "    name = :name, "
        "    no_len = :no_len, "
        "    printable = :printable, "
        "    airp = :airp "
        "  where code = :code; "
        "  if sql%notfound then "
        "    insert into tag_types ( "
        "      code, "
        "      airline, "
        "      name, "
        "      no_len, "
        "      printable, "
        "      airp "
        "    ) values ( "
        "      :code, "
        "      :airline, "
        "      :name, "
        "      :no_len, "
        "      :printable, "
        "      :airp "
        "    ); "
        "  end if; "
        "end; ";
    Qry.DeclareVariable("code", otString);
    Qry.DeclareVariable("airline", otString);
    Qry.DeclareVariable("name", otString);
    Qry.DeclareVariable("no_len", otInteger);
    Qry.DeclareVariable("printable", otInteger);
    Qry.DeclareVariable("airp", otString);
    for(vector<TTagTypesItem>::iterator iv = items.begin(); iv != items.end(); iv++) {
        Qry.SetVariable("code", iv->code);
        Qry.SetVariable("airline", iv->airline);
        Qry.SetVariable("name", iv->name);
        Qry.SetVariable("no_len", iv->no_len);
        Qry.SetVariable("printable", iv->printable);
        Qry.SetVariable("airp", iv->airp);
        Qry.Execute();
    }
    bt_models.ToBase();
}

void TTagTypes::add(xmlNodePtr reqNode)
{
    xmlNodePtr currNode = NodeAsNode("blank_types", reqNode)->children;
    for(; currNode != NULL; currNode = currNode->next) {
        xmlNodePtr fastNode = currNode->children;
        TTagTypesItem item;
        item.code = NodeAsStringFast("code", fastNode);
        item.airline = NodeAsStringFast("airline", fastNode);
        item.name = NodeAsStringFast("name", fastNode);
        item.airp = NodeAsStringFast("airp", fastNode);
        item.no_len = NodeAsIntegerFast("no_len", fastNode);
        item.printable = NodeAsIntegerFast("printable", fastNode);
        items.push_back(item);
    }
    bt_models.add(reqNode);
}

void TTagTypes::ToXML(xmlNodePtr resNode)
{
    if(not items.empty()) {
        xmlNodePtr tag_typesNode = NewTextChild(resNode, "blank_types");
        for(vector<TTagTypesItem>::iterator it = items.begin(); it != items.end(); it++) {
            xmlNodePtr itemNode = NewTextChild(tag_typesNode, "item");
            NewTextChild(itemNode, "code", it->code);
            NewTextChild(itemNode, "airline", it->airline);
            NewTextChild(itemNode, "name", it->name);
            NewTextChild(itemNode, "airp", it->airp);
            NewTextChild(itemNode, "no_len", it->no_len);
            NewTextChild(itemNode, "printable", it->printable);
        }
    }
    bt_models.ToXML(resNode);
}

void TTagTypes::add(string type)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   code, "
        "   airline, "
        "   name, "
        "   no_len, "
        "   printable, "
        "   airp "
        "from "
        "   tag_types "
        "where "
        "   code = :code ";
    Qry.CreateVariable("code", otString, type);
    Qry.Execute();
    if(Qry.Eof)
        throw UserException("Информация по типу бирки %s недоступна. Обновите данные.", type.c_str());
    TTagTypesItem item;
    item.code = Qry.FieldAsString("code");
    item.airline = Qry.FieldAsString("airline");
    item.name = Qry.FieldAsString("name");
    item.no_len = Qry.FieldAsInteger("no_len");
    item.printable = Qry.FieldAsInteger("printable");
    item.airp = Qry.FieldAsString("airp");
    items.push_back(item);
    bt_models.add(type);
}

void TPrnForms::add(int id)
{
    ProgTrace(TRACE5, "TPrnForms::add: id %d", id);
    if(items.find(id) == items.end()) {
        ProgTrace(TRACE5, "TPrnForms::add: id %d not found", id);
        TQuery PrnFormQry(&OraSession);
        PrnFormQry.SQLText =
            "select "
            "   op_type, "
            "   fmt_type, "
            "   name "
            "from "
            "   prn_forms "
            "where "
            "   id = :id ";
        PrnFormQry.CreateVariable("id", otInteger, id);
        PrnFormQry.Execute();
        if(PrnFormQry.Eof)
            throw UserException("Информация по форме недоступна. Обновите данные.");
        TPrnFormsItem &prn_form = items[id];
        prn_form.op_type = PrnFormQry.FieldAsString("op_type");
        prn_form.fmt_type = PrnFormQry.FieldAsString("fmt_type");
        prn_form.name = PrnFormQry.FieldAsString("name");
    }
}

void TPrnFormVers::add(int id, int version)
{
    TPrnFormVersKey vers_key;
    vers_key.id = id;
    vers_key.version = version;
    if(items.find(vers_key) == items.end()) {
        TQuery VersQry(&OraSession);
        VersQry.SQLText =
            "select "
            "   descr, "
            "   form, "
            "   data, "
            "   read_only "
            "from "
            "   prn_form_vers "
            "where "
            "   id = :id and "
            "   version = :version ";
        VersQry.CreateVariable("id", otInteger, id);
        VersQry.CreateVariable("version", otInteger, version);
        VersQry.Execute();
        if(VersQry.Eof)
            throw UserException("Информация по форме недоступна. Обновите данные.");
        TPrnFormVersRow &row = items[vers_key];
        row.descr = VersQry.FieldAsString("descr");
        row.form = VersQry.FieldAsString("form");
        row.data = VersQry.FieldAsString("data");
        row.read_only = VersQry.FieldAsInteger("read_only") != 0;
    }
}

void TBRModels::add(string type)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   form_type, "
        "   dev_model, "
        "   fmt_type, "
        "   id, "
        "   version "
        "from "
        "   br_models "
        "where "
        "   form_type = :code ";
    Qry.CreateVariable("code", otString, type);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        TBRModelsItem item;
        item.form_type = Qry.FieldAsString("form_type");
        item.dev_model = Qry.FieldAsString("dev_model");
        item.fmt_type = Qry.FieldAsString("fmt_type");
        item.id = Qry.FieldAsInteger("id");
        item.version = Qry.FieldAsInteger("version");
        items.push_back(item);
        prn_forms.add(item.id);
        prn_form_vers.add(item.id, item.version);
    }
}

void TBPModels::add(string type)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   form_type, "
        "   dev_model, "
        "   fmt_type, "
        "   id, "
        "   version "
        "from "
        "   bp_models "
        "where "
        "   form_type = :code ";
    Qry.CreateVariable("code", otString, type);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        TBPModelsItem item;
        item.form_type = Qry.FieldAsString("form_type");
        item.dev_model = Qry.FieldAsString("dev_model");
        item.fmt_type = Qry.FieldAsString("fmt_type");
        item.id = Qry.FieldAsInteger("id");
        item.version = Qry.FieldAsInteger("version");
        items.push_back(item);
        prn_forms.add(item.id);
        prn_form_vers.add(item.id, item.version);
    }
}

void TBTModels::add(string type)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "   form_type, "
        "   dev_model, "
        "   num, "
        "   fmt_type, "
        "   id, "
        "   version "
        "from "
        "   bt_models "
        "where "
        "   form_type = :code ";
    Qry.CreateVariable("code", otString, type);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next()) {
        TBTModelsItem item;
        item.form_type = Qry.FieldAsString("form_type");
        item.dev_model = Qry.FieldAsString("dev_model");
        item.num = Qry.FieldAsInteger("num");
        item.fmt_type = Qry.FieldAsString("fmt_type");
        item.id = Qry.FieldAsInteger("id");
        item.version = Qry.FieldAsInteger("version");
        items.push_back(item);
        prn_forms.add(item.id);
        prn_form_vers.add(item.id, item.version);
    }
}
void DevTuningInterface::Export(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string op_type = NodeAsString("op_type", reqNode);
    TFormTypes *form_types = NULL;
    try {
        switch(DecodeDevOperType(op_type)) {
            case dotPrnBP:
                form_types = new TBPTypes;
                break;
            case dotPrnBT:
                form_types = new TTagTypes;
                break;
            case dotPrnBR:
                form_types = new TBRTypes;
                break;
            default:
                throw Exception("Unknown type: %s", op_type.c_str());
        }
        xmlNodePtr currNode = NodeAsNode("types", reqNode);
        currNode = currNode->children;
        for(; currNode; currNode = currNode->next)
            form_types->add(NodeAsString(currNode));
        form_types->ToXML(resNode);
        NewTextChild(resNode, "op_type", op_type);
        ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
        delete form_types;
    } catch(...) {
        delete form_types;
        throw;
    }
}

//////////// new import classes ////////////

enum TVersAction {vaInsert, vaUpdate, vaNone};

struct TFormType;

struct TVersionType {
    TVersAction action;
    string form, data, fmt_type, name;
    TDevOperType op_type;
    int id, version;
    map<string, TFormType * > forms;
    string str();
    void insert();
    void update();
    void del();
    void delete_blank(TFormType &form);
    void delete_dst_vers();
    string find_form(TVersionType &vers);
    void delete_blanks(TVersionType &vers);
    TVersionType *get_forms_vers();
    void get_form_list(vector<TFormType *> form_list);
    TVersionType(): action(vaNone), op_type(dotUnknown), id(0), version(0) {};
    ~TVersionType();
};

struct TFormType {
    string form_type, dev_model, fmt_type;
    int num;
    TVersionType *dest_version;
    void init(TFormType *val);
    virtual string str();
    virtual void insert(TVersionType &vers) = 0;
    virtual void del() = 0;
    virtual void get_version(TVersionType &ver) = 0;
    virtual TFormType *Copy() = 0;
    TFormType(): num(0), dest_version(NULL) {};
    virtual ~TFormType() {};
};

void TVersionType::insert()
{
    ProgTrace(TRACE5, "insert version");
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "declare "
        "  new_name prn_forms.name%type; "
        "  copy_num number; "
        "  names_count number; "
        "begin "
        "  new_name := :name; "
        "  copy_num := 1; "
        "  loop "
        "    select count(*) into names_count from prn_forms where name = new_name; "
        "    if names_count = 0 then exit; end if; "
        "    new_name := :name || '' копия '' || copy_num; "
        "    copy_num := copy_num + 1; "
        "  end loop; "
        "  :name := new_name; "
        "  insert into prn_forms( id, op_type, fmt_type, name) "
        "    values(id__seq.nextval, :op_type, :fmt_type, :name) "
        "  returning id into :id; "
        "  insert into prn_form_vers(id, version, descr, form, data, read_only) "
        "    values(:id, 0, ''Inserted by prnc'', :form, :data, 1); "
        "end; ";
    Qry.CreateVariable("op_type", otString, EncodeDevOperType(op_type));
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.CreateVariable("name", otString, name);
    Qry.CreateVariable("form", otString, form);
    Qry.CreateVariable("data", otString, data);
    Qry.DeclareVariable("id", otInteger);
    try {
        Qry.Execute();
    } catch(Exception E) {
        throw Exception("Не могу вставить версию %s: %s", name.c_str(), E.what());
    }
    id = Qry.GetVariableAsInteger("id");
    version = 0;
    for(map<string, TFormType *>::iterator i_form = forms.begin(); i_form != forms.end(); i_form++)
        i_form->second->insert(*this);
}

void TVersionType::update()
{
    TVersionType *dst_vers = NULL;
    for(map<string, TFormType *>::iterator i_form = forms.begin(); i_form != forms.end(); i_form++) {
        if(i_form->second->dest_version != NULL) {
            dst_vers = i_form->second->dest_version;
            break;
        }
    }
    if(dst_vers == NULL)
        throw Exception("TVersionType::update: destination vers not found");
    if(
            form != dst_vers->form or
            data != dst_vers->data or
            fmt_type != dst_vers->fmt_type or
            name != dst_vers->name
      ) {

        TQuery Qry(&OraSession);
        Qry.SQLText =
            "declare "
            "  new_name prn_forms.name%type; "
            "  copy_num number; "
            "  names_count number; "
            "begin "
            "  new_name := :name; "
            "  copy_num := 1; "
            "  loop "
            "    select count(*) into names_count from prn_forms where name = new_name and id <> :id; "
            "    if names_count = 0 then exit; end if; "
            "    new_name := :name || '' копия '' || copy_num; "
            "    copy_num := copy_num + 1; "
            "  end loop; "
            "  :name := new_name; "
            "  update prn_forms set "
            "    fmt_type = :fmt_type, "
            "    name = :name "
            "  where id = :id; "
            "  update prn_form_vers set "
            "    form = :form, "
            "    data = :data "
            "  where id = :id and version = :version; "
            "end; ";
        Qry.CreateVariable("form", otString, form);
        Qry.CreateVariable("data", otString, data);
        Qry.CreateVariable("fmt_type", otString, fmt_type);
        Qry.CreateVariable("name", otString, name);
        Qry.CreateVariable("id", otInteger, dst_vers->id);
        Qry.CreateVariable("version", otInteger, dst_vers->version);
        try {
            Qry.Execute();
        } catch(Exception E) {
            throw Exception("Не могу сапдейтить версию %s: %s", name.c_str(), E.what());
        }
    }
}

void TVersionType::del()
{
    ProgTrace(TRACE5, "delete version");
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin"
        "  delete from prn_form_vers where id = :id and version = :version; "
        "  delete from prn_forms where id = :id; "
        "end; ";
    Qry.CreateVariable("id", otInteger, id);
    Qry.CreateVariable("version", otInteger, version);
    try {
        Qry.Execute();
    } catch(Exception E) {
        throw Exception("Не могу удалить версию %s: %s", name.c_str(), E.what());
    }
}

void TVersionType::delete_blank(TFormType &form)
{
    if(forms.find(form.str()) != forms.end()) {
        forms[form.str()]->del();
        delete forms[form.str()];
        forms.erase(form.str());
    }
}

void TVersionType::delete_dst_vers()
{
    for(map<string, TFormType *>::iterator i_form = forms.begin(); i_form != forms.end(); i_form++) {
        if(i_form->second->dest_version != NULL) {
            i_form->second->dest_version->delete_blank(*i_form->second);
            i_form->second->dest_version = NULL;
        }
    }
}

void TVersionType::delete_blanks(TVersionType &vers)
{
    string i_form = find_form(vers);
    while(not i_form.empty()) {
        forms[i_form]->del();
        delete forms[i_form];
        forms.erase(i_form);
        i_form = find_form(vers);
    }
}

string TVersionType::find_form(TVersionType &vers)
{
    string result;
    for(map<string, TFormType *>::iterator i_form = forms.begin(); i_form != forms.end(); i_form++) {
        TFormType &form = *i_form->second;
        if(form.dest_version == &vers) {
            result = form.str();
            break;
        }
    }
    return result;
}

TVersionType *TVersionType::get_forms_vers()
{
    TVersionType *result = NULL;
    for(map<string, TFormType *>::iterator i_forms = forms.begin(); i_forms != forms.end(); i_forms++) {
        TFormType &form = *i_forms->second;
        if(form.dest_version == NULL)
            continue;
        if(result == NULL)
            result = form.dest_version;
        else if(result != form.dest_version) {
            result = NULL;
            break;
        }
    }
    return result;
}

void TFormType::init(TFormType *val)
{
    form_type = val->form_type;
    dev_model = val->dev_model;
    fmt_type = val->fmt_type;
    num = val->num;
    dest_version = val->dest_version;
}

struct TBRFormType:TFormType {
    void insert(TVersionType &vers);
    void del();
    void get_version(TVersionType &ver);
    TFormType *Copy();
};

struct TBPFormType:TFormType {
    void insert(TVersionType &vers);
    void del();
    void get_version(TVersionType &ver);
    TFormType *Copy();
};

struct TBTFormType:TFormType {
    void insert(TVersionType &vers);
    string str();
    void del();
    void get_version(TVersionType &ver);
    TFormType *Copy();
};

void TBPFormType::insert(TVersionType &vers)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "insert into bp_models ( "
        "  form_type,  "
        "  dev_model,  "
        "  fmt_type,  "
        "  id,  "
        "  version "
        ") values ( "
        "  :form_type,  "
        "  :dev_model,  "
        "  :fmt_type,  "
        "  :id,  "
        "  :version "
        ") ";
    Qry.CreateVariable("form_type", otString, form_type);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.CreateVariable("id", otInteger, vers.id);
    Qry.CreateVariable("version", otInteger, vers.version);
    try {
        Qry.Execute();
    } catch(Exception E) {
        throw Exception("Не могу назначить пектаб на пос. талон %s: %s", str().c_str(), E.what());
    }
}

void TBRFormType::insert(TVersionType &vers)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "insert into br_models ( "
        "  form_type,  "
        "  dev_model,  "
        "  fmt_type,  "
        "  id,  "
        "  version "
        ") values ( "
        "  :form_type,  "
        "  :dev_model,  "
        "  :fmt_type,  "
        "  :id,  "
        "  :version "
        ") ";
    Qry.CreateVariable("form_type", otString, form_type);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.CreateVariable("id", otInteger, vers.id);
    Qry.CreateVariable("version", otInteger, vers.version);
    try {
        Qry.Execute();
    } catch(Exception E) {
        throw Exception("Не могу назначить пектаб на баг. квитанцию %s: %s", str().c_str(), E.what());
    }
}

void TBTFormType::insert(TVersionType &vers)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "insert into bt_models ( "
        "  form_type,  "
        "  dev_model,  "
        "  num,  "
        "  fmt_type,  "
        "  id,  "
        "  version "
        ") values ( "
        "  :form_type,  "
        "  :dev_model,  "
        "  :num,  "
        "  :fmt_type,  "
        "  :id,  "
        "  :version "
        ") ";
    Qry.CreateVariable("form_type", otString, form_type);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.CreateVariable("num", otInteger, num);
    Qry.CreateVariable("id", otInteger, vers.id);
    Qry.CreateVariable("version", otInteger, vers.version);
    try {
        Qry.Execute();
    } catch(Exception E) {
        throw Exception("Не могу назначить пектаб на баг. бирку %s: %s", str().c_str(), E.what());
    }
}

void TBPFormType::del()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "delete from bp_models where "
        "  form_type = :form_type and "
        "  dev_model = :dev_model and "
        "  fmt_type = :fmt_type ";
    Qry.CreateVariable("form_type", otString, form_type);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    try {
        Qry.Execute();
    } catch(Exception E) {
        throw Exception("Не могу удалить бланк пос. талона %s: %s", str().c_str(), E.what());
    }
}

void TBRFormType::del()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "delete from br_models where "
        "  form_type = :form_type and "
        "  dev_model = :dev_model and "
        "  fmt_type = :fmt_type ";
    Qry.CreateVariable("form_type", otString, form_type);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    try {
        Qry.Execute();
    } catch(Exception E) {
        throw Exception("Не могу удалить бланк баг. квитанции %s: %s", str().c_str(), E.what());
    }
}

void TBTFormType::del()
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "delete from bt_models where "
        "  form_type = :form_type and "
        "  dev_model = :dev_model and "
        "  fmt_type = :fmt_type and "
        "  num = :num ";
    Qry.CreateVariable("form_type", otString, form_type);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.CreateVariable("num", otInteger, num);
    try {
        Qry.Execute();
    } catch(Exception E) {
        throw Exception("Не могу удалить бланк баг. бирки %s: %s", str().c_str(), E.what());
    }
}

void TVersionType::get_form_list(vector<TFormType *> form_list)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select"
        "  form_type, "
        "  dev_model, "
        "  fmt_type "
        "from "
        "  bp_models "
        "where "
        "  id = :id and "
        "  version = :version ";
    Qry.CreateVariable("id", otInteger, id);
    Qry.CreateVariable("version", otInteger, version);
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) {
        TFormType *form = new TBPFormType;
        form->form_type = Qry.FieldAsString("form_type");
        form->dev_model = Qry.FieldAsString("dev_model");
        form->fmt_type = Qry.FieldAsString("fmt_type");
        form_list.push_back(form);
    }
    Qry.SQLText =
        "select"
        "  form_type, "
        "  dev_model, "
        "  num, "
        "  fmt_type "
        "from "
        "  bt_models "
        "where "
        "  id = :id and "
        "  version = :version ";
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) {
        TFormType *form = new TBTFormType;
        form->form_type = Qry.FieldAsString("form_type");
        form->dev_model = Qry.FieldAsString("dev_model");
        form->fmt_type = Qry.FieldAsString("fmt_type");
        form->num = Qry.FieldAsInteger("num");
        form_list.push_back(form);
    }
    Qry.SQLText =
        "select"
        "  form_type, "
        "  dev_model, "
        "  fmt_type "
        "from "
        "  br_models "
        "where "
        "  id = :id and "
        "  version = :version ";
    Qry.CreateVariable("id", otInteger, id);
    Qry.CreateVariable("version", otInteger, version);
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) {
        TFormType *form = new TBRFormType;
        form->form_type = Qry.FieldAsString("form_type");
        form->dev_model = Qry.FieldAsString("dev_model");
        form->fmt_type = Qry.FieldAsString("fmt_type");
        form_list.push_back(form);
    }
}

TFormType *TBTFormType::Copy()
{
    TFormType *result = new TBTFormType;
    result->init(this);
    return result;
}

TFormType *TBPFormType::Copy()
{
    TFormType *result = new TBPFormType;
    result->init(this);
    return result;
}

TFormType *TBRFormType::Copy()
{
    TFormType *result = new TBRFormType;
    result->init(this);
    return result;
}

void TBRFormType::get_version(TVersionType &ver)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "  br_models.id, "
        "  br_models.version, "
        "  prn_forms.name, "
        "  prn_form_vers.form, "
        "  prn_form_vers.data "
        "from "
        "  br_models, "
        "  prn_forms, "
        "  prn_form_vers "
        "where "
        "  form_type = :form_type and "
        "  dev_model = :dev_model and "
        "  br_models.fmt_type = :fmt_type and "
        "  prn_forms.id = prn_form_vers.id and "
        "  br_models.id = prn_form_vers.id and "
        "  br_models.version = prn_form_vers.version ";
    Qry.CreateVariable("form_type", otString, form_type);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.Execute();
    if(not Qry.Eof) {
        ver.id = Qry.FieldAsInteger("id");
        ver.version = Qry.FieldAsInteger("version");
        ver.name = Qry.FieldAsString("name");
        ver.form = Qry.FieldAsString("form");
        ver.data = Qry.FieldAsString("data");
    }
}

void TBPFormType::get_version(TVersionType &ver)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select  "
        "  bt_models.id,  "
        "  bt_models.version,  "
        "  prn_forms.name,  "
        "  prn_form_vers.form,  "
        "  prn_form_vers.data  "
        "from  "
        "  bt_models,  "
        "  prn_forms,  "
        "  prn_form_vers  "
        "where  "
        "  form_type = :form_type and  "
        "  dev_model = :dev_model and  "
        "  bt_models.fmt_type = :fmt_type and  "
        "  bt_models.num = :num and  "
        "  prn_forms.id = prn_form_vers.id and  "
        "  bt_models.id = prn_form_vers.id and  "
        "  bt_models.version = prn_form_vers.version  ";
    Qry.CreateVariable("form_type", otString, form_type);
    Qry.CreateVariable("dev_model", otString, dev_model);
    Qry.CreateVariable("fmt_type", otString, fmt_type);
    Qry.CreateVariable("num", otInteger, num);
    Qry.Execute();
    if(not Qry.Eof) {
        ver.id = Qry.FieldAsInteger("id");
        ver.version = Qry.FieldAsInteger("version");
        ver.name = Qry.FieldAsString("name");
        ver.form = Qry.FieldAsString("form");
        ver.data = Qry.FieldAsString("data");
    }
}

void TBTFormType::get_version(TVersionType &ver)
{
}

string TBTFormType::str()
{
    return form_type + "." + dev_model + "." + IntToString(num) + "." + fmt_type;
}

string TFormType::str()
{
    return form_type + "." + dev_model + "." + fmt_type;
}

TVersionType::~TVersionType()
{
    for(map<string, TFormType * >::iterator forms_i = forms.begin(); forms_i != forms.end(); forms_i++)
        delete forms_i->second;
}

string TVersionType::str()
{
    ostringstream buf;
    buf << id << "." << version;
    return buf.str();
}

struct TVersList {
    map<string, TVersionType> items;
    string add_new(TPectabItem &pectab);
    string  add_to_vers(string vers_i, TPectabItem &pectab);
    void update();
    void insert();
    void cleanup();
    void fix();
    void fill_dest_list(TVersList &dst_vers_list);
    void dump();
};

void TVersList::update()
{
    for(map<string, TVersionType>::iterator vers_i = items.begin(); vers_i != items.end(); vers_i++) {
        if(vers_i->second.action == vaUpdate)
            vers_i->second.update();
    }
}

void TVersList::insert()
{
    for(map<string, TVersionType>::iterator vers_i = items.begin(); vers_i != items.end(); vers_i++) {
        if(vers_i->second.action == vaInsert)
            vers_i->second.insert();
    }
}

void TVersList::cleanup()
{
    for(map<string, TVersionType>::iterator vers_i = items.begin(); vers_i != items.end(); vers_i++) {
        TVersionType vers = vers_i->second;
        if(vers.forms.empty())
            vers.del();
    }
}

void TVersList::fix()
{
    for(map<string, TVersionType>::iterator i_vers = items.begin(); i_vers != items.end(); i_vers++) {
        TVersionType &vers = i_vers->second;
        TVersionType *single_dst_vers = vers.get_forms_vers();
        if(single_dst_vers != NULL) {
            TVersionType *single_src_vers = single_dst_vers->get_forms_vers();
            if(single_src_vers != NULL) {
                ProgTrace(TRACE5, "simple update of current version");
                vers.action = vaUpdate;
            } else {
                //Удаляет бланки из целевой версии, которые нашлись в исходной версии
                ProgTrace(TRACE5, "delete blanks from destination");
                single_dst_vers->delete_blanks(vers);
                vers.action = vaInsert;
            }
        } else {
            ProgTrace(TRACE5, "delete_dst_vers");
            vers.delete_dst_vers();
            vers.action = vaInsert;
        }
    }
}

void TVersList::fill_dest_list(TVersList &dst_vers_list)
{
    for(map<string, TVersionType>::iterator i_vers = items.begin(); i_vers != items.end(); i_vers++) {
        TVersionType &version = i_vers->second;
        for(map<string, TFormType *>::iterator i_forms = version.forms.begin(); i_forms != version.forms.end(); i_forms++) {
            TFormType &form_type = *i_forms->second;
            TVersionType dst_vers;
            form_type.get_version(dst_vers);
            if(dst_vers.name.empty())
                continue;
            if(dst_vers_list.items.find(dst_vers.str()) == dst_vers_list.items.end()) {
                dst_vers.fmt_type = form_type.fmt_type;
                dst_vers_list.items[dst_vers.str()] = dst_vers;
            }
            TFormType *dest_form_type = form_type.Copy();
            dest_form_type->dest_version = &version; // ссылка на версию исходника
            form_type.dest_version = &(dst_vers_list.items[dst_vers.str()]); // ссылка на версию цели
            dst_vers_list.items[dst_vers.str()].forms[dest_form_type->str()] = dest_form_type;

        }
    }
    // Теперь надо проверить списки бланков для каждой версии и дополнить их
    // в случае надобности.
    for(map<string, TVersionType>::iterator i_vers = dst_vers_list.items.begin(); i_vers != dst_vers_list.items.end(); i_vers++) {
        TVersionType &version = i_vers->second;
        vector<TFormType *> form_list;
        version.get_form_list(form_list);
        for(vector<TFormType *>::iterator i_list = form_list.begin(); i_list != form_list.end(); i_list++) {
            if(version.forms.find((*i_list)->str()) == version.forms.end()) {
                version.forms[(*i_list)->str()] = *i_list;
                *i_list = NULL;
            }
        }
        for(vector<TFormType *>::iterator i_list = form_list.begin(); i_list != form_list.end(); i_list++)
            delete *i_list;
    }
}

void TVersList::dump()
{
    ProgTrace(TRACE5, "-----------TVersList::dump()-----------");
    for(map<string, TVersionType>::iterator i_items = items.begin(); i_items != items.end(); i_items++) {
        ProgTrace(TRACE5, "%s %s", i_items->second.name.c_str(), i_items->second.str().c_str());
        map<string, TFormType * > &forms = i_items->second.forms;
        for(map<string, TFormType * >::iterator forms_i = forms.begin(); forms_i != forms.end(); forms_i++) {
            string buf;
            if(forms_i->second->dest_version != NULL)
                buf = forms_i->second->dest_version->name + " " + forms_i->second->dest_version->str();
            ProgTrace(TRACE5, "        %s %s", forms_i->second->str().c_str(), buf.c_str());
        }
    }
    ProgTrace(TRACE5, "-----------END OF TVersList::dump()-----------");
}

string TVersList::add_to_vers(string vers_i, TPectabItem &pectab)
{
    TFormType *form;
    switch(pectab.op_type) {
        case dotPrnBP:
            form = new TBPFormType;
            break;
        case dotPrnBT:
            form = new TBTFormType;
            break;
        case dotPrnBR:
            form = new TBRFormType;
            break;
        default:
            throw Exception("TVersList::add_to_vers: unknown op_type %d", pectab.op_type);
    }
    form->form_type = pectab.form_type;
    form->dev_model = pectab.dev_model;
    form->fmt_type = pectab.fmt_type;
    form->num = pectab.num;
    if(items[vers_i].forms.find(form->str()) != items[vers_i].forms.end()) {
        string buf = form->str();
        delete form;
        throw Exception("TVersList::add_to_vers: duplicate form found: %s", buf.c_str());
    }
    items[vers_i].forms[form->str()] = form;
    items[vers_i].fmt_type = form->fmt_type;
    return form->str();
}

string TVersList::add_new(TPectabItem &pectab)
{
    TVersionType vers;
    vers.op_type = pectab.op_type;
    vers.form = pectab.form;
    vers.data = pectab.data;
    vers.id = items.size();
    if(items.find(vers.str()) != items.end())
        throw Exception("TVersList::add_new: duplicate version found: %s", vers.str().c_str());
    items[vers.str()] = vers;
    string form_i = add_to_vers(vers.str(), pectab);
    items[vers.str()].name = (items[vers.str()].forms[form_i])->str();
    return vers.str();
}

void process(vector<TPectabItem> &pectabs)
{
    if(pectabs.empty()) return;
    TVersList src_vers_list;
    // Сгруппировать по версиям
    if(pectabs.size() == 1)
        src_vers_list.add_new(pectabs[0]);
    else {
        for(size_t p_i = 0; p_i < pectabs.size() - 1; p_i++) {
            TPectabItem &pectab_i = pectabs[p_i];
            if(pectab_i.form_type.empty())
                continue;
            string vers_i = src_vers_list.add_new(pectab_i);
            pectab_i.form_type.erase();
            for(size_t p_j = p_i + 1; p_j < pectabs.size(); p_j++) {
                TPectabItem &pectab_j = pectabs[p_j];
                if(pectab_j.form_type.empty())
                    continue;
                if(
                        pectab_i.form == pectab_j.form and
                        pectab_i.data == pectab_j.data
                  ) {
                    src_vers_list.add_to_vers(vers_i, pectab_j);
                    pectab_j.form_type.erase();
                }
            }
        }
        TPectabItem &pectab = pectabs[pectabs.size() - 1];
        if(not pectab.form_type.empty())
            src_vers_list.add_new(pectab);
    }
    // Теперь нужно сформировать список версий целевой базы
    // Для этого нужно пробежать по всем типам бланков источника
    // и выяснить какие версии (id, version) им соответствуют в
    // целевой базе.
    TVersList dst_vers_list;
    src_vers_list.fill_dest_list(dst_vers_list);
    src_vers_list.dump();
    dst_vers_list.dump();
    src_vers_list.fix();
    dst_vers_list.cleanup();
    src_vers_list.update();
    src_vers_list.insert();
}

void DevTuningInterface::Import(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgTrace(TRACE5, "%s", GetXMLDocText(reqNode->doc).c_str());
    string op_type = NodeAsString("op_type", reqNode);
    TFormTypes *form_types = NULL;
    try {
        switch(DecodeDevOperType(op_type)) {
            case dotPrnBP:
                form_types = new TBPTypes;
                break;
            case dotPrnBT:
                form_types = new TTagTypes;
                break;
            case dotPrnBR:
                form_types = new TBRTypes;
                break;
            default:
                throw Exception("Unknown type: %s", op_type.c_str());
        }
        form_types->add(reqNode);
        vector<TPectabItem> pectabs;
        form_types->get_pectabs(pectabs);
        ProgTrace(TRACE5, "pectabs.size(): %d", pectabs.size());
        process(pectabs);


        /*
        form_types->PrnFormsToBase();
        xmlNodePtr cfgNode = NodeAsNode("cfg", reqNode)->children;
        for(; cfgNode; cfgNode = cfgNode->next) {
            xmlNodePtr fastNode = cfgNode->children;
            string src = NodeAsStringFast("src", fastNode);
            ProgTrace(TRACE5, "src: %s", src.c_str());
            xmlNodePtr destsNode = NodeAsNodeFast("dests", fastNode)->children;
            for(; destsNode; destsNode = destsNode->next) {
                string dest = NodeAsString(destsNode);
                ProgTrace(TRACE5, "    dest: %s", dest.c_str());
                form_types->copy(src, dest);
            }
        }
        */
        delete form_types;
    } catch(...) {
        delete form_types;
        throw;
    }
}
