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
            int tst = 0;
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

struct TFormTypes {
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
    void add(xmlNodePtr reqNode);
    void ToXML(xmlNodePtr resNode);
    void add(string type);
    void ToBase();
    void PrnFormsToBase();
    void copy(string src, string dest);
};

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
    void add(xmlNodePtr reqNode);
    void ToXML(xmlNodePtr resNode);
    void add(string type);
    void ToBase();
    void PrnFormsToBase();
    void copy(string src, string dest);
};

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
    void add(xmlNodePtr reqNode);
    void ToXML(xmlNodePtr resNode);
    void add(string type);
    void ToBase();
    void PrnFormsToBase();
    void copy(string src, string dest);
};

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
        if(op_type == "PRINT_BP") {
            form_types = new TBPTypes;
        } else if(op_type == "PRINT_BT") {
            form_types = new TTagTypes;
        } else if(op_type == "PRINT_BR") {
            form_types = new TBRTypes;
        } else
            throw Exception("Unknown type: %s", op_type.c_str());
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

void DevTuningInterface::Import(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgTrace(TRACE5, "%s", GetXMLDocText(reqNode->doc).c_str());
    string op_type = NodeAsString("op_type", reqNode);
    TFormTypes *form_types = NULL;
    try {
        if(op_type == "PRINT_BP") {
            form_types = new TBPTypes;
        } else if(op_type == "PRINT_BT") {
            form_types = new TTagTypes;
        } else if(op_type == "PRINT_BR") {
            form_types = new TBRTypes;
        } else
            throw Exception("Unknown type: %s", op_type.c_str());
        form_types->add(reqNode);
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
        delete form_types;
    } catch(...) {
        delete form_types;
        throw;
    }
}
