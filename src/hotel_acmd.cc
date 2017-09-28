#include "hotel_acmd.h"
#include "tripinfo.h"
#include "brd.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;

struct THotelAcmdPaxItem {
    int idx;
    int point_id;
    int pax_id;
    string full_name;
    TPerson pers_type;
    int hotel_id;
    int room_type;
    bool breakfast;
    bool dinner;
    bool supper;
    THotelAcmdPaxItem():
        idx(NoExists),
        pax_id(NoExists),
        pers_type(NoPerson),
        hotel_id(NoExists),
        room_type(NoExists),
        breakfast(false),
        dinner(false),
        supper(false)
    {}
};

struct THotelAcmdPax
{
    map<int, THotelAcmdPaxItem> items;
    void fromDB(int point_id);
    void fromXML(xmlNodePtr reqNode);
    void toDB(list<pair<int, int> > &inserted_paxes);
};

struct TPaxListItem {
    int pax_id;
    int reg_no;
    TPerson pers_type;
    string full_name;
    TPaxListItem():
        pax_id(NoExists),
        reg_no(NoExists),
        pers_type(NoPerson)
    {}
};

struct TPaxList {
    list<TPaxListItem> items;
    void fromDB(int point_id);
};

void TPaxList::fromDB(int point_id)
{
    // из PNL
    TCachedQuery crsQry(
            "select "
            "   crs_pax.surname||' '||crs_pax.name full_name, "
            "   pax.reg_no, "
            "   crs_pax.pax_id, "
            "   crs_pax.pers_type "
            "from "
            "   crs_pnr, "
            "   tlg_binding, "
            "   crs_pax, "
            "   pax "
            "where "
            "   crs_pnr.point_id=tlg_binding.point_id_tlg AND "
            "   crs_pnr.system='CRS' AND "
            "   crs_pnr.pnr_id=crs_pax.pnr_id AND "
            "   crs_pax.pr_del=0 and "
            "   tlg_binding.point_id_spp = :point_id and "
            "   crs_pax.pax_id = pax.pax_id(+) ",
            QParams() << QParam("point_id", otInteger, point_id));
    crsQry.get().Execute();
    for(; not crsQry.get().Eof; crsQry.get().Next()) {
        TPaxListItem item;
        item.pax_id = crsQry.get().FieldAsInteger("pax_id");
        if(not crsQry.get().FieldIsNULL("reg_no"))
            item.reg_no = crsQry.get().FieldAsInteger("reg_no");
        item.pers_type = DecodePerson(crsQry.get().FieldAsString("pers_type"));
        item.full_name = crsQry.get().FieldAsString("full_name");
        items.push_back(item);
    }

    // норики
    TQuery Qry(&OraSession);
    BrdInterface::GetPaxQuery(Qry, point_id, NoExists, NoExists, TReqInfo::Instance()->desk.lang, rtNOREC, "", stRegNo);
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) {
        CheckIn::TSimplePaxItem pax;
        pax.fromDB(Qry);
        TPaxListItem item;
        item.pax_id = pax.id;
        item.reg_no = pax.reg_no;
        item.pers_type = pax.pers_type;
        item.full_name = pax.surname + " " + pax.name;
        items.push_back(item);
    }

    // паксы введенные вручную
    TCachedQuery freePaxQry("select * from hotel_acmd_free_pax where point_id = :point_id",
            QParams() << QParam("point_id", otInteger, point_id));
    freePaxQry.get().Execute();
    for(; not freePaxQry.get().Eof; freePaxQry.get().Next()) {
        TPaxListItem item;
        item.pax_id = freePaxQry.get().FieldAsInteger("pax_id");
        item.pers_type = DecodePerson(freePaxQry.get().FieldAsString("pers_type"));
        item.full_name = freePaxQry.get().FieldAsString("full_name");
        items.push_back(item);
    }
}


void THotelAcmdPax::toDB(list<pair<int, int> > &inserted_paxes)
{
    TCachedQuery newPaxQry(
            "begin "
            "   select pax_id.nextval INTO :pax_id FROM dual; "
            "   insert into hotel_acmd_free_pax ( "
            "      pax_id, "
            "      point_id, "
            "      full_name, "
            "      pers_type "
            "   ) values ( "
            "      :pax_id, "
            "      :point_id, "
            "      :full_name, "
            "      :pers_type "
            "   ); "
            "end; ",
            QParams()
            << QParam("pax_id", otInteger)
            << QParam("point_id", otInteger)
            << QParam("full_name", otString)
            << QParam("pers_type", otString));

    TCachedQuery delQry("delete from hotel_acmd_pax where pax_id = :pax_id",
            QParams() << QParam("pax_id", otInteger));

    TCachedQuery insQry(
            "begin "
            "   insert into hotel_acmd_pax ( "
            "       pax_id, "
            "       point_id, "
            "       hotel_id, "
            "       room_type, "
            "       breakfast, "
            "       dinner, "
            "       supper "
            "   ) values ( "
            "       :pax_id, "
            "       :point_id, "
            "       :hotel_id, "
            "       :room_type, "
            "       :breakfast, "
            "       :dinner, "
            "       :supper "
            "   ); "
            "exception "
            "   when dup_val_on_index then "
            "       update hotel_acmd_pax set "
            "           point_id = :point_id, "
            "           hotel_id = :hotel_id, "
            "           room_type = :room_type, "
            "           breakfast = :breakfast, "
            "           dinner = :dinner, "
            "           supper = :supper "
            "       where "
            "           pax_id = :pax_id; "
            "end; ",
        QParams()
            << QParam("pax_id", otInteger)
            << QParam("point_id", otInteger)
            << QParam("hotel_id", otInteger)
            << QParam("room_type", otInteger)
            << QParam("breakfast", otInteger)
            << QParam("dinner", otInteger)
            << QParam("supper", otInteger));
    for(map<int, THotelAcmdPaxItem>::iterator pax = items.begin();
            pax != items.end(); pax++) {
        if(pax->second.pax_id == NoExists) {
            newPaxQry.get().SetVariable("point_id", pax->second.point_id);
            newPaxQry.get().SetVariable("full_name", pax->second.full_name);
            newPaxQry.get().SetVariable("pers_type", EncodePerson(pax->second.pers_type));
            newPaxQry.get().Execute();
            pax->second.pax_id = newPaxQry.get().GetVariableAsInteger("pax_id");
            inserted_paxes.push_back(make_pair(pax->second.idx, pax->second.pax_id));
        }
        if(pax->second.hotel_id == NoExists) {
            delQry.get().SetVariable("pax_id", pax->second.pax_id);
            delQry.get().Execute();
        } else {
            insQry.get().SetVariable("pax_id", pax->second.pax_id);
            insQry.get().SetVariable("point_id", pax->second.point_id);
            insQry.get().SetVariable("hotel_id", pax->second.hotel_id);
            insQry.get().SetVariable("room_type", pax->second.room_type);
            insQry.get().SetVariable("breakfast", pax->second.breakfast);
            insQry.get().SetVariable("dinner", pax->second.dinner);
            insQry.get().SetVariable("supper", pax->second.supper);
            insQry.get().Execute();
        }
    }
}

void THotelAcmdPax::fromXML(xmlNodePtr reqNode)
{
    int point_id = NodeAsInteger("point_id", reqNode);
    xmlNodePtr lstNode = NodeAsNode("PaxList", reqNode);
    xmlNodePtr paxNode = lstNode->children;
    for(; paxNode; paxNode = paxNode->next) {
        xmlNodePtr currNode = paxNode->children;
        THotelAcmdPaxItem item;
        item.idx = NodeAsInteger("@idx", paxNode);
        item.point_id = point_id;
        item.pax_id = NodeAsIntegerFast("pax_id", currNode, NoExists);
        item.full_name = NodeAsStringFast("full_name", currNode);
        item.pers_type = (TPerson)NodeAsIntegerFast("pers_type", currNode);
        item.hotel_id = NodeAsIntegerFast("hotel_id", currNode);
        item.room_type = NodeAsIntegerFast("room_type", currNode);
        item.breakfast = NodeAsIntegerFast("breakfast", currNode);
        item.dinner = NodeAsIntegerFast("dinner", currNode);
        item.supper = NodeAsIntegerFast("supper", currNode);
        items[item.pax_id] = item;
    }
}

void THotelAcmdPax::fromDB(int point_id)
{
    TCachedQuery Qry("select * from hotel_acmd_pax where point_id = :point_id",
            QParams() << QParam("point_id", otInteger, point_id));
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        int col_point_id = Qry.get().FieldIndex("point_id");
        int col_pax_id = Qry.get().FieldIndex("pax_id");
        int col_hotel_id = Qry.get().FieldIndex("hotel_id");
        int col_room_type = Qry.get().FieldIndex("room_type");
        int col_breakfast = Qry.get().FieldIndex("breakfast");
        int col_dinner = Qry.get().FieldIndex("dinner");
        int col_supper = Qry.get().FieldIndex("supper");
        for(; not Qry.get().Eof; Qry.get().Next()) {
            THotelAcmdPaxItem item;
            item.point_id = Qry.get().FieldAsInteger(col_point_id);
            item.pax_id = Qry.get().FieldAsInteger(col_pax_id);
            item.hotel_id = Qry.get().FieldAsInteger(col_hotel_id);
            item.room_type = Qry.get().FieldAsInteger(col_room_type);
            item.breakfast = Qry.get().FieldAsInteger(col_breakfast);
            item.dinner = Qry.get().FieldAsInteger(col_dinner);
            item.supper = Qry.get().FieldAsInteger(col_supper);
            items[item.pax_id] = item;
        }
    }
}

void HotelAcmdInterface::View(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger( "point_id", reqNode );
    TPaxList pax_list;
    pax_list.fromDB(point_id);
    THotelAcmdPax hotel_acmd_pax;
    hotel_acmd_pax.fromDB(point_id);
    xmlNodePtr paxListNode = NULL;
    for(list<TPaxListItem>::iterator pax = pax_list.items.begin();
            pax != pax_list.items.end(); pax++) {
        if(not paxListNode)
            paxListNode = NewTextChild(resNode, "PaxList");
        xmlNodePtr paxNode = NewTextChild(paxListNode, "pax");
        NewTextChild(paxNode, "pax_id", pax->pax_id);
        if(pax->reg_no != NoExists)
            NewTextChild(paxNode, "reg_no", pax->reg_no);
        NewTextChild(paxNode, "pers_type", pax->pers_type);
        NewTextChild(paxNode, "full_name", transliter(pax->full_name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU));
        map<int, THotelAcmdPaxItem>::iterator acmd_pax = hotel_acmd_pax.items.find(pax->pax_id);
        if(acmd_pax != hotel_acmd_pax.items.end()) {
            NewTextChild(paxNode, "hotel_id", acmd_pax->second.hotel_id);
            NewTextChild(paxNode, "room_type", acmd_pax->second.room_type);
            NewTextChild(paxNode, "breakfast", acmd_pax->second.breakfast);
            NewTextChild(paxNode, "dinner", acmd_pax->second.dinner);
            NewTextChild(paxNode, "supper", acmd_pax->second.supper);
        }
    }
    xmlNodePtr defaultsNode = NewTextChild(resNode, "defaults");
    NewTextChild(defaultsNode, "hotel_id", NoExists);
    NewTextChild(defaultsNode, "room_type", 1);
    NewTextChild(defaultsNode, "breakfast", 0);
    NewTextChild(defaultsNode, "dinner", 0);
    NewTextChild(defaultsNode, "supper", 0);

    LogTrace(TRACE5) << GetXMLDocText(resNode->doc);
}

void HotelAcmdInterface::Save(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    THotelAcmdPax acmd_pax;
    acmd_pax.fromXML(reqNode);
    list<pair<int, int> > inserted_paxes;
    acmd_pax.toDB(inserted_paxes);

    xmlNodePtr paxListNode = NULL;
    for(list<pair<int, int> >::iterator i = inserted_paxes.begin();
            i != inserted_paxes.end(); i++) {
        if(not paxListNode)
            paxListNode = NewTextChild(resNode, "PaxList");
        xmlNodePtr paxNode = NewTextChild(paxListNode, "pax");
        NewTextChild(paxNode, "idx", i->first);
        NewTextChild(paxNode, "pax_id", i->second);
    }
}

