#include "stat_layout.h"
#include "xml_unit.h"
#include "stat_common.h"
#include "qrys.h"
#include "term_version.h"
#include "stat_salon.h"

using namespace std;
using namespace AstraLocale;

void TParamItem::toXML(xmlNodePtr resNode)
{
    xmlNodePtr itemNode = NewTextChild(resNode, "item");
    NewTextChild(itemNode, "code", code);
    NewTextChild(itemNode, "visible", visible);
    NewTextChild(itemNode, "label", label);
    NewTextChild(itemNode, "caption", caption);
    NewTextChild(itemNode, "ctype", ctype);
    NewTextChild(itemNode, "name", name);
    NewTextChild(itemNode, "width", width);
    NewTextChild(itemNode, "len", len);
    NewTextChild(itemNode, "isalnum", isalnum);
    NewTextChild(itemNode, "ref", ref);
    NewTextChild(itemNode, "ref_field", ref_field);
    NewTextChild(itemNode, "tag", tag);
    NewTextChild(itemNode, "edit_fmt", edit_fmt);
    NewTextChild(itemNode, "filter", filter);

    if(code == "SegCategory") {
        xmlNodePtr dataNode = NewTextChild(itemNode, "data");
        xmlNodePtr CBoxItemNode = NewTextChild(dataNode, "item");
        NewTextChild(CBoxItemNode, "code");
        NewTextChild(CBoxItemNode, "caption");
        for(list<pair<TSegCategories::Enum, string> >::const_iterator
                i = TSegCategories::pairsCodes().begin();
                i != TSegCategories::pairsCodes().end(); i++) {
            if(i->first == TSegCategories::Unknown) continue;
            CBoxItemNode = NewTextChild(dataNode, "item");
            NewTextChild(CBoxItemNode, "code", i->second);
            NewTextChild(CBoxItemNode, "caption", getLocaleText(i->second));
        }
    }

    if(code == "SalonOpType") {
        xmlNodePtr dataNode = NewTextChild(itemNode, "data");
        xmlNodePtr CBoxItemNode = NewTextChild(dataNode, "item");
        NewTextChild(CBoxItemNode, "code");
        NewTextChild(CBoxItemNode, "caption");
        for(const auto &i: TSalonOpType::pairs()) {
            CBoxItemNode = NewTextChild(dataNode, "item");
            NewTextChild(CBoxItemNode, "code", i.first);
            NewTextChild(CBoxItemNode, "caption", getLocaleText(i.second));
        }
    }

    if(code == "Seance") {
        static const char *seances[] = {"", "Ää", "Äè"};
        xmlNodePtr dataNode = NewTextChild(itemNode, "data");
        for(size_t i = 0; i < sizeof(seances) / sizeof(seances[0]); i++) {
            xmlNodePtr CBoxItemNode = NewTextChild(dataNode, "item");
            NewTextChild(CBoxItemNode, "code", seances[i]);
            NewTextChild(CBoxItemNode, "caption", seances[i]);
        }
    }
}

void TParamItem::fromDB(TQuery &Qry)
{
    code = Qry.FieldAsString("code");
    visible = Qry.FieldAsInteger("visible");
    label = code + "L";
    caption = Qry.FieldAsString("caption");
    ctype = Qry.FieldAsString("ctype");
    name = code + ctype;
    width = Qry.FieldAsInteger("width");
    len = Qry.FieldAsInteger("len");
    isalnum = Qry.FieldAsInteger("isalnum");
    ref = Qry.FieldAsString("ref");
    ref_field = Qry.FieldAsString("ref_field");
    tag = Qry.FieldAsString("tag");
    edit_fmt = Qry.FieldAsString("edit_fmt");
    filter = Qry.FieldAsString("filter");
}

void TLayout::get_params()
{
    if(params.empty()) {
        TCachedQuery Qry("select * from stat_params");
        Qry.get().Execute();
        for(; not Qry.get().Eof; Qry.get().Next()) {
            TParamItem item;
            item.fromDB(Qry.get());
            if(
                    (item.ctype != "CkBox" or
                     TReqInfo::Instance()->desk.compatible(STAT_CKBOX_VERSION)) and

                    (TReqInfo::Instance()->desk.compatible(BI_STAT_VERSION) or
                     (Qry.get().FieldIsNULL("edit_fmt") and
                      Qry.get().FieldIsNULL("filter")))
              )
                params.insert(make_pair(item.visible, item));
        }
    }
}

void TLayout::toXML(xmlNodePtr resNode, const string &tag, const string &qry)
{
    TCachedQuery Qry(qry);
    Qry.get().Execute();
    xmlNodePtr listNode = NewTextChild(resNode, tag.c_str());
    for(; not Qry.get().Eof; Qry.get().Next()) {
        xmlNodePtr itemNode = NewTextChild(listNode, "item");
        NewTextChild(itemNode, "type", Qry.get().FieldAsString("code"));
        NewTextChild(itemNode, "visible", Qry.get().FieldAsString("visible"));
        if(tag == "types") {
            int item_params = Qry.get().FieldAsInteger("params");
            xmlNodePtr paramsNode = NULL;
            for(map<int, TParamItem>::iterator i = params.begin(); i != params.end(); i++) {
                if((item_params & i->first) == i->first) {
                    if(not paramsNode) paramsNode = NewTextChild(itemNode, "params");
                    NewTextChild(paramsNode, "item", i->second.code);
                }
            }
        }
    }
}

void TLayout::toXML(xmlNodePtr resNode)
{
    get_params();
    xmlNodePtr paramsNode = NewTextChild(resNode, "params");
    for(map<int, TParamItem>::iterator i = params.begin(); i != params.end(); i++)
        i->second.toXML(paramsNode);
    toXML(resNode, "types", "select * from stat_types order by view_order");
    toXML(resNode, "levels", "select * from stat_levels order by view_order");
}

