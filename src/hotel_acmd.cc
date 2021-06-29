#include "hotel_acmd.h"
#include "tripinfo.h"
#include "brd.h"
#include "stat/stat_utils.h"
#include "qrys.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;
using namespace AstraLocale;

const string HotelAcmdDateFormat = "dd.mm.yyyy hh:nn";

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
        items[item.pax_id] = item;
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
        items[item.pax_id] = item;
    }

    // паксы введенные вручную
    DB::TCachedQuery freePaxQry(
          PgOra::getROSession("HOTEL_ACMD_FREE_PAX"),
          "SELECT * FROM hotel_acmd_free_pax "
          "WHERE point_id = :point_id",
          QParams() << QParam("point_id", otInteger, point_id),
          STDLOG);
    freePaxQry.get().Execute();
    for(; not freePaxQry.get().Eof; freePaxQry.get().Next()) {
        TPaxListItem item;
        item.pax_id = freePaxQry.get().FieldAsInteger("pax_id");
        item.pers_type = DecodePerson(freePaxQry.get().FieldAsString("pers_type").c_str());
        item.full_name = freePaxQry.get().FieldAsString("full_name");
        items[item.pax_id] = item;
    }
}


void THotelAcmdPax::SetBoolLogParam(LEvntPrms &params, const string &name, int val)
{
    if(val == NoExists) {
        params << PrmSmpl<string>(name, "N/A");
    } else
        params << PrmBool(name, val);
}

static int GetVar(DB::TQuery &Qry, int idx)
{
    int result = NoExists;
    if(not Qry.FieldIsNULL(idx))
        result = Qry.FieldAsInteger(idx);
    return result;
}

PaxId_t saveHotelAcmdFreePax(const PointId_t& point_id, const std::string& full_name,
                             const ASTRA::TPerson pers_type)
{
    const PaxId_t new_pax_id(PgOra::getSeqNextVal_int("PAX_ID"));
    DB::TCachedQuery Qry(
        PgOra::getRWSession("HOTEL_ACMD_FREE_PAX"),
        "INSERT INTO hotel_acmd_free_pax ( "
        "   pax_id, "
        "   point_id, "
        "   full_name, "
        "   pers_type "
        ") VALUES ( "
        "   :pax_id, "
        "   :point_id, "
        "   :full_name, "
        "   :pers_type "
        ") ",
        QParams()
        << QParam("pax_id", otInteger, new_pax_id.get())
        << QParam("point_id", otInteger, point_id.get())
        << QParam("full_name", otString, full_name)
        << QParam("pers_type", otString, EncodePerson(pers_type)),
        STDLOG);
    Qry.get().Execute();
    return new_pax_id;
}

std::optional<HotelId_t> getHotelId(const PaxId_t& pax_id)
{
  DB::TCachedQuery Qry(
        PgOra::getROSession("HOTEL_ACMD_PAX"),
        "SELECT hotel_id FROM hotel_acmd_pax "
        "WHERE pax_id = :pax_id ",
        QParams() << QParam("pax_id", otInteger, pax_id.get()),
        STDLOG);
  Qry.get().Execute();
  if (Qry.get().Eof) {
    return {};
  }
  return HotelId_t(Qry.get().FieldAsInteger("hotel_id"));
}

void deleteHotelAcmdPax(const PaxId_t& pax_id)
{
    DB::TCachedQuery Qry(
        PgOra::getRWSession("HOTEL_ACMD_PAX"),
        "DELETE FROM hotel_acmd_pax "
        "WHERE pax_id = :pax_id ",
        QParams() << QParam("pax_id", otInteger, pax_id.get()),
        STDLOG);
    Qry.get().Execute();
}

void saveHotelAcmdPax(const PaxId_t& pax_id, const PointId_t& point_id, const HotelId_t& hotel_id,
                      int room_type, int breakfast, int dinner, int supper)
{
    QParams params;
    params << QParam("pax_id", otInteger, pax_id.get())
           << QParam("point_id", otInteger, point_id.get())
           << QParam("hotel_id", otInteger, hotel_id.get())
           << QParam("room_type", otInteger, room_type, QParam::NullOnEmpty)
           << QParam("breakfast", otInteger, breakfast, QParam::NullOnEmpty)
           << QParam("dinner", otInteger, dinner, QParam::NullOnEmpty)
           << QParam("supper", otInteger, supper, QParam::NullOnEmpty);

    DB::TCachedQuery updQry(
        PgOra::getRWSession("HOTEL_ACMD_PAX"),
        "UPDATE hotel_acmd_pax SET "
        "    point_id = :point_id, "
        "    hotel_id = :hotel_id, "
        "    room_type = :room_type, "
        "    breakfast = :breakfast, "
        "    dinner = :dinner, "
        "    supper = :supper "
        "WHERE "
        "    pax_id = :pax_id ",
        params,
        STDLOG);
    updQry.get().Execute();
    if (updQry.get().RowsProcessed() == 0) {
        DB::TCachedQuery insQry(
            PgOra::getRWSession("HOTEL_ACMD_PAX"),
            "INSERT INTO hotel_acmd_pax ( "
            "    pax_id, "
            "    point_id, "
            "    hotel_id, "
            "    room_type, "
            "    breakfast, "
            "    dinner, "
            "    supper "
            ") VALUES ( "
            "    :pax_id, "
            "    :point_id, "
            "    :hotel_id, "
            "    :room_type, "
            "    :breakfast, "
            "    :dinner, "
            "    :supper "
            ") ",
            params,
            STDLOG);
        insQry.get().Execute();
    }
}

void THotelAcmdPax::toDB(list<pair<int, int> > &inserted_paxes)
{
    TPaxList pax_list;
    pax_list.fromDB(point_id);
    for(map<int, THotelAcmdPaxItem>::iterator pax = items.begin();
            pax != items.end(); pax++) {
        if(pax->second.pax_id == NoExists) {
            pax->second.pax_id = saveHotelAcmdFreePax(PointId_t(pax->second.point_id),
                                                      pax->second.full_name,
                                                      pax->second.pers_type).get();
            inserted_paxes.push_back(make_pair(pax->second.idx, pax->second.pax_id));
        }

        CheckIn::TSimplePaxItem ckin_pax;
        int reg_no = NoExists;
        int grp_id = NoExists;
        if(ckin_pax.getByPaxId(pax->second.pax_id)) {
            reg_no = ckin_pax.reg_no;
            grp_id = ckin_pax.grp_id;
        }

        if(pax->second.hotel_id == NoExists) {
            const std::optional<HotelId_t> hotel_id = getHotelId(PaxId_t(pax->second.pax_id));
            if (hotel_id) {
                deleteHotelAcmdPax(PaxId_t(pax->second.pax_id));
                LEvntPrms params;
                params << PrmSmpl<std::string>("full_name", pax->second.full_name);
                params << PrmSmpl<string>("hotel_name", ElemIdToNameLong(etHotel, hotel_id->get()));
                TReqInfo::Instance()->LocaleToLog("EVT.HOTEL_ACMD_ANNUL", params, ASTRA::evtPax, point_id, reg_no, grp_id);
            }
        } else {
            saveHotelAcmdPax(PaxId_t(pax->second.pax_id), PointId_t(pax->second.point_id),
                             HotelId_t(pax->second.hotel_id), pax->second.room_type,
                             pax->second.breakfast, pax->second.dinner, pax->second.supper);
            LEvntPrms params;
            params << PrmSmpl<std::string>("full_name", pax->second.full_name);
            params << PrmSmpl<string>("hotel_name", ElemIdToNameLong(etHotel, pax->second.hotel_id));
            params << PrmSmpl<string>("room_type",
                    (pax->second.room_type == NoExists ? "N/A" :
                    ElemIdToNameLong(etHotelRoomType, pax->second.room_type)));
            SetBoolLogParam(params, "breakfast", pax->second.breakfast);
            SetBoolLogParam(params, "dinner", pax->second.dinner);
            SetBoolLogParam(params, "supper", pax->second.supper);

            TReqInfo::Instance()->LocaleToLog("EVT.HOTEL_ACMD", params, ASTRA::evtPax, point_id, reg_no, grp_id);
        }
    }
}

void THotelAcmdPax::fromXML(xmlNodePtr reqNode)
{
    point_id = NodeAsInteger("point_id", reqNode);
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
        item.room_type = NodeAsIntegerFast("room_type", currNode, NoExists);
        item.breakfast = NodeAsIntegerFast("breakfast", currNode, NoExists);
        item.dinner = NodeAsIntegerFast("dinner", currNode, NoExists);
        item.supper = NodeAsIntegerFast("supper", currNode, NoExists);
        items[item.pax_id] = item;
    }
}

void THotelAcmdPax::fromDB(int point_id)
{
    this->point_id = point_id;
    DB::TCachedQuery Qry(
          PgOra::getROSession("HOTEL_ACMD_PAX"),
          "SELECT * FROM hotel_acmd_pax "
          "WHERE point_id = :point_id",
          QParams() << QParam("point_id", otInteger, point_id),
          STDLOG);
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
            item.room_type = GetVar(Qry.get(), col_room_type);
            item.breakfast = GetVar(Qry.get(), col_breakfast);
            item.dinner = GetVar(Qry.get(), col_dinner);
            item.supper = GetVar(Qry.get(), col_supper);
            items[item.pax_id] = item;
        }
    }
}

struct TAcmdDate {
    int point_id;
    TDateTime acmd_date_from;
    TDateTime acmd_date_to;

    TAcmdDate():
        point_id(NoExists),
        acmd_date_from(NoExists),
        acmd_date_to(NoExists)
    {}

    void fromDB(int point_id);
    void fromXML(xmlNodePtr node);

    void toXML(xmlNodePtr node);
    void toDB();
};

void TAcmdDate::toDB()
{
    if(acmd_date_from == NoExists) return;
    TTripInfo info;
    info.getByPointId(point_id);

    QParams params;
    params << QParam("point_id", otInteger, point_id)
           << QParam("acmd_date_from", otDate, LocalToUTC(acmd_date_from, AirpTZRegion(info.airp)))
           << QParam("acmd_date_to", otDate, LocalToUTC(acmd_date_to, AirpTZRegion(info.airp)));

    DB::TCachedQuery updQry(
          PgOra::getRWSession("HOTEL_ACMD_DATES"),
          "UPDATE hotel_acmd_dates SET "
          "    acmd_date_from = :acmd_date_from, "
          "    acmd_date_to = :acmd_date_to "
          "WHERE "
          "    point_id = :point_id ",
          params,
          STDLOG);
    updQry.get().Execute();

    if (updQry.get().RowsProcessed() == 0) {
      DB::TCachedQuery insQry(
            PgOra::getRWSession("HOTEL_ACMD_DATES"),
            "INSERT INTO hotel_acmd_dates ( "
            "    point_id, "
            "    acmd_date_from, "
            "    acmd_date_to "
            ") VALUES ( "
            "    :point_id, "
            "    :acmd_date_from, "
            "    :acmd_date_to "
            ") ",
            params,
            STDLOG);
      insQry.get().Execute();
    }
}

void TAcmdDate::fromXML(xmlNodePtr node)
{
    point_id = NodeAsInteger("point_id", node);
    acmd_date_from = NodeAsDateTime("acmd_date_from", node, NoExists);
    acmd_date_to = NodeAsDateTime("acmd_date_to", node, NoExists);
    if(acmd_date_to < acmd_date_from)
        throw AstraLocale::UserException("MSG.INVALID_RANGE");
}

void TAcmdDate::toXML(xmlNodePtr node)
{
    TTripInfo info;
    info.getByPointId(point_id);
    if(acmd_date_from == NoExists) {
        acmd_date_from = NowUTC();
        acmd_date_to = acmd_date_from;
        NewTextChild(node, "acmd_date_from",
                DateTimeToStr(UTCToLocal(acmd_date_from, AirpTZRegion(info.airp)),
                    "dd.mm.yyyy"));
        NewTextChild(node, "acmd_date_to",
                DateTimeToStr(UTCToLocal(acmd_date_to, AirpTZRegion(info.airp)),
                    "dd.mm.yyyy"));
    } else {
        NewTextChild(node, "acmd_date_from",
                DateTimeToStr(UTCToLocal(acmd_date_from, AirpTZRegion(info.airp)),
                    HotelAcmdDateFormat));
        NewTextChild(node, "acmd_date_to",
                DateTimeToStr(UTCToLocal(acmd_date_to, AirpTZRegion(info.airp)),
                    HotelAcmdDateFormat));
    }
}

void TAcmdDate::fromDB(int apoint_id)
{
    point_id = apoint_id;
    DB::TCachedQuery Qry(
          PgOra::getROSession("HOTEL_ACMD_DATES"),
          "SELECT * FROM hotel_acmd_dates "
          "WHERE point_id = :point_id",
          QParams() << QParam("point_id", otInteger, point_id),
          STDLOG);
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        acmd_date_from = Qry.get().FieldAsDateTime("acmd_date_from");
        acmd_date_to = Qry.get().FieldAsDateTime("acmd_date_to");
    }
}

void HotelAcmdInterface::Print(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger( "point_id", reqNode );

    string hotel_name = NodeAsString("hotel_name", reqNode, "");
    int total_pax = NodeAsInteger("total_pax", reqNode, NoExists);
    int breakfast = NodeAsInteger("breakfast", reqNode, NoExists);
    int dinner = NodeAsInteger("dinner", reqNode, NoExists);
    int supper = NodeAsInteger("supper", reqNode, NoExists);

    if(not hotel_name.empty()) {
        LEvntPrms params;
        params << PrmSmpl<string>("hotel_name", hotel_name);
        params << PrmSmpl<int>("total_pax", total_pax);
        params << PrmSmpl<int>("breakfast", breakfast);
        params << PrmSmpl<int>("dinner", dinner);
        params << PrmSmpl<int>("supper", supper);
        TReqInfo::Instance()->LocaleToLog("EVT.HOTEL_ACMD.PRINT_CLAIM", params, evtFlt, point_id);
    } else
        TReqInfo::Instance()->LocaleToLog("EVT.HOTEL_ACMD.PRINT_LIST", evtFlt, point_id);
}

void HotelAcmdInterface::HotelAcmdClaim(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger( "point_id", reqNode );
    get_compatible_report_form("HotelAcmdClaim", reqNode, resNode);

    xmlNodePtr formDataNode = STAT::set_variables(resNode);
    TRptParams rpt_params(TReqInfo::Instance()->desk.lang);
    PaxListVars(point_id, rpt_params, formDataNode);
    NewTextChild(formDataNode, "doc_hotel_acmd_caption", getLocaleText("DOC.HOTEL_ACMD.CAPTION"));

    TTripInfo info;
    info.getByPointId(point_id);
    TDateTime claim_date = UTCToLocal(NowUTC(),AirpTZRegion(info.airp));
    bool pr_lat = TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU;
    NewTextChild(formDataNode, "claim_day", DateTimeToStr(claim_date, "dd"));
    NewTextChild(formDataNode, "claim_month", getDocMonth(claim_date, pr_lat));
    NewTextChild(formDataNode, "claim_year", DateTimeToStr(claim_date, "yyyy"));
}

void HotelAcmdInterface::View(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int point_id = NodeAsInteger( "point_id", reqNode );

    get_compatible_report_form("HotelAcmdList", reqNode, resNode);

    xmlNodePtr formDataNode = STAT::set_variables(resNode);
    TRptParams rpt_params(TReqInfo::Instance()->desk.lang);
    PaxListVars(point_id, rpt_params, formDataNode);

    string real_out = NodeAsString("real_out", formDataNode);
    string scd_out = NodeAsString("scd_out", formDataNode);
    string date = real_out + (real_out == scd_out ? "" : "(" + scd_out + ")");
    NewTextChild(formDataNode, "caption", getLocaleText("CAP.DOC.HOTEL_ACMD_LIST",
                LParams() << LParam("trip", NodeAsString("trip", formDataNode))
                << LParam("date", date)
                ));

    TPaxList pax_list;
    pax_list.fromDB(point_id);
    THotelAcmdPax hotel_acmd_pax;
    hotel_acmd_pax.fromDB(point_id);
    xmlNodePtr paxListNode = NULL;
    for(map<int, TPaxListItem>::iterator pax = pax_list.items.begin();
            pax != pax_list.items.end(); pax++) {
        if(not paxListNode)
            paxListNode = NewTextChild(resNode, "PaxList");
        xmlNodePtr paxNode = NewTextChild(paxListNode, "pax");
        NewTextChild(paxNode, "pax_id", pax->second.pax_id);
        if(pax->second.reg_no != NoExists)
            NewTextChild(paxNode, "reg_no", pax->second.reg_no);
        NewTextChild(paxNode, "pers_type", pax->second.pers_type);
        NewTextChild(paxNode, "full_name", transliter(pax->second.full_name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU));
        map<int, THotelAcmdPaxItem>::iterator acmd_pax = hotel_acmd_pax.items.find(pax->second.pax_id);
        if(acmd_pax != hotel_acmd_pax.items.end()) {
            NewTextChild(paxNode, "hotel_id", acmd_pax->second.hotel_id);
            NewTextChild(paxNode, "room_type", acmd_pax->second.room_type, NoExists);
            NewTextChild(paxNode, "breakfast", acmd_pax->second.breakfast, NoExists);
            NewTextChild(paxNode, "dinner", acmd_pax->second.dinner, NoExists);
            NewTextChild(paxNode, "supper", acmd_pax->second.supper, NoExists);
        }
    }
    xmlNodePtr defaultsNode = NewTextChild(resNode, "defaults");
    NewTextChild(defaultsNode, "hotel_id", NoExists);
    NewTextChild(defaultsNode, "room_type", NoExists);
    NewTextChild(defaultsNode, "breakfast", NoExists);
    NewTextChild(defaultsNode, "dinner", NoExists);
    NewTextChild(defaultsNode, "supper", NoExists);

    TAcmdDate acmd_date;
    acmd_date.fromDB(point_id);
    acmd_date.toXML(resNode);
}

void HotelAcmdInterface::Save(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TAcmdDate acmd_date;
    acmd_date.fromXML(reqNode);
    acmd_date.toDB();

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

