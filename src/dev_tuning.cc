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
            if(cache.FieldOldValue("read_only", row) == cache.FieldValue("read_only", row) and
                    cache.FieldOldValue("read_only", row) == "0")
                throw UserException("Редактирование версии " + cache.FieldOldValue("version", row) + " запрещено.");
            if(row.status == usDeleted) {
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
    virtual ~TFormTypes() {};
};

struct TBPTypesItem {
    string code, airline, airp, name;
};

struct TBPTypes:TFormTypes {
    vector<TBPTypesItem> items;
    void add(string type);
};

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
    item.code = Qry.FieldAsString("code");
    item.airline = Qry.FieldAsString("airline");
    item.airp = Qry.FieldAsString("airp");
    item.name = Qry.FieldAsString("name");
    items.push_back(item);
}

struct TTagTypesItem {
    string code, airline, name, airp;
    int no_len, printable;
    TTagTypesItem(): no_len(NoExists), printable(NoExists) {};
};

struct TTagTypes:TFormTypes {
    vector<TTagTypesItem> items;
    void add(string type);
};

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
}

struct TModels {
    virtual void add(string type) = 0;
    virtual ~TModels() {};
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
};

void TPrnForms::add(int id)
{
    if(items.find(id) == items.end()) {
        TQuery PrnFormQry(&OraSession);
        PrnFormQry.SQLText =
            "select "
            "   op_type, "
            "   fmt_type, "
            "   name "
            "from "
            "   items "
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

struct TPrnFormVers {
    map<TPrnFormVersKey, TPrnFormVersRow, TPrnFormVersCmp> items;
    void add(int id, int version);
};

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
            "   items "
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

struct TBPModels:TModels {
    TPrnForms prn_forms;
    TPrnFormVers prn_form_vers;
    vector<TBPModelsItem> items;
    void add(string type);
};

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

struct TBTModels:TModels {
    TPrnForms prn_forms;
    TPrnFormVers prn_form_vers;
    vector<TBTModelsItem> items;
    void add(string type);
};

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
    xmlNodePtr currNode = NodeAsNode("types", reqNode);
    string op_type = NodeAsString("op_type", reqNode);
    TFormTypes *form_types = NULL;
    TModels *models = NULL;
    try {
        if(op_type == "PRINT_BP") {
            form_types = new TBPTypes;
            models = new TBPModels;
        } else if(op_type == "PRINT_BT") {
            form_types = new TTagTypes;
            models = new TBTModels;
        } else
            throw Exception("Unknown type: %s", op_type.c_str());
        map<int, TPrnFormsItem> prn_forms;
        map<TPrnFormVersKey, TPrnFormVersRow, TPrnFormVersCmp> prn_form_vers;
        currNode = currNode->children;
        xmlNodePtr bp_typesNode = NULL;
        xmlNodePtr bp_modelsNode = NULL;
        for(; currNode; currNode = currNode->next) {
            string type = NodeAsString(currNode);
            form_types->add(type);
            models->add(type);




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
            if(not bp_typesNode)
                bp_typesNode = NewTextChild(resNode, "bp_types");
            xmlNodePtr itemNode = NewTextChild(bp_typesNode, "item");
            NewTextChild(itemNode, "code", type);
            NewTextChild(itemNode, "airline", Qry.FieldAsString("airline"));
            NewTextChild(itemNode, "airp", Qry.FieldAsString("airp"));
            NewTextChild(itemNode, "name", Qry.FieldAsString("name"));
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
            Qry.Execute();
            for(; !Qry.Eof; Qry.Next()) {
                if(not bp_modelsNode)
                    bp_modelsNode = NewTextChild(resNode, "bp_models");
                itemNode = NewTextChild(bp_modelsNode, "item");
                int id = Qry.FieldAsInteger("id");
                int version = Qry.FieldAsInteger("version");
                NewTextChild(itemNode, "form_type", Qry.FieldAsString("form_type"));
                NewTextChild(itemNode, "dev_model", Qry.FieldAsString("dev_model"));
                NewTextChild(itemNode, "fmt_type", Qry.FieldAsString("fmt_type"));
                NewTextChild(itemNode, "id", id);
                NewTextChild(itemNode, "version", version);
                if(prn_forms.find(id) == prn_forms.end()) {
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
                    TPrnFormsItem &prn_form = prn_forms[id];
                    prn_form.op_type = PrnFormQry.FieldAsString("op_type");
                    prn_form.fmt_type = PrnFormQry.FieldAsString("fmt_type");
                    prn_form.name = PrnFormQry.FieldAsString("name");
                }
                TPrnFormVersKey vers_key;
                vers_key.id = id;
                vers_key.version = version;
                if(prn_form_vers.find(vers_key) == prn_form_vers.end()) {
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
                    TPrnFormVersRow &row = prn_form_vers[vers_key];
                    row.descr = VersQry.FieldAsString("descr");
                    row.form = VersQry.FieldAsString("form");
                    row.data = VersQry.FieldAsString("data");
                    row.read_only = VersQry.FieldAsInteger("read_only") != 0;
                }
            }
        }
        if(not prn_forms.empty()) {
            xmlNodePtr prn_formsNode = NewTextChild(resNode, "prn_forms");
            for(map<int, TPrnFormsItem>::iterator it = prn_forms.begin(); it != prn_forms.end(); it++) {
                xmlNodePtr itemNode = NewTextChild(prn_formsNode, "item");
                NewTextChild(itemNode, "id", it->first);
                NewTextChild(itemNode, "op_type", it->second.op_type);
                NewTextChild(itemNode, "fmt_type", it->second.fmt_type);
                NewTextChild(itemNode, "name", it->second.name);
            }
        }
        if(not prn_form_vers.empty()) {
            xmlNodePtr prn_form_versNode = NewTextChild(resNode, "prn_form_vers");
            for(map<TPrnFormVersKey, TPrnFormVersRow, TPrnFormVersCmp>::iterator it = prn_form_vers.begin(); it != prn_form_vers.end(); it++) {
                xmlNodePtr itemNode = NewTextChild(prn_form_versNode, "item");
                NewTextChild(itemNode, "id", it->first.id);
                NewTextChild(itemNode, "version", it->first.version);
                NewTextChild(itemNode, "descr", it->second.descr);
                NewTextChild(itemNode, "form", it->second.form);
                NewTextChild(itemNode, "data", it->second.data);
                NewTextChild(itemNode, "read_only", it->second.read_only);
            }
        }
        ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
        delete form_types;
    } catch(...) {
        delete form_types;
        throw;
    }
}
