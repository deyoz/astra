#include "zamar_dsm.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "term_version.h"
#include "astra_misc.h"
#include "passenger.h"
#include "web_main.h"
#include "baggage_tags.h"
#include "astra_elems.h"
#include "tripinfo.h"
#include "remarks.h"
#include "baggage_calc.h"

#define NICKNAME "GRISHA"
#include <serverlib/slogger.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace EXCEPTIONS;

const string STR_PAX_NO_MATCH = "PAX_NO_MATCH";
const string STR_INTERNAL_ERROR = "INTERNAL_ERROR";
const string STR_UNDER_CONSTRUCTION = "UNDER_CONSTRUCTION";

//-----------------------------------------------------------------------------------

struct ZamarException : public Exception
{
  string cmd;
  ZamarException(string a_cmd, string a_err) : Exception(a_err.c_str()), cmd(a_cmd) {}
};

//-----------------------------------------------------------------------------------

PassengerSearchResult& PassengerSearchResult::fromXML(xmlNodePtr reqNode, ZamarType type)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");

  AstraLocale::OutputLang lang("", {AstraLocale::OutputLang::OnlyTrueIataCodes});
  // sessionId
  sessionId = NodeAsString( "sessionId", reqNode);
  if (sessionId.empty())
    throw Exception("Empty <sessionId>");
  // bcbp
  string bcbp = NodeAsString( "bcbp", reqNode);
  if (bcbp.empty())
    throw Exception("Empty <bcbp>");

  AstraWeb::GetBPPaxFromScanCode(bcbp, bppax); // throws

  if (not bppax.errors.empty())
  {
    for (const auto& err : bppax.errors)
    {
      if (err.lexema_id == "MSG.PASSENGERS.NOT_FOUND" or
          err.lexema_id == "MSG.PASSENGER.NOT_FOUND" or
          err.lexema_id == "MSG.PASSENGERS.FOUND_MORE" or
          err.lexema_id == "MSG.FLIGHT.NOT_FOUND")
      {
        LogTrace(TRACE5) << "error command = '" << STR_PAX_NO_MATCH << "' lexema_id = '" << err.lexema_id << "'";
        throw ZamarException(STR_PAX_NO_MATCH, getLocaleText(err.lexema_id, err.lparams, lang.get()));
      }
    }
    const auto& err = *bppax.errors.cbegin();
    LogTrace(TRACE5) << "error command = '" << STR_INTERNAL_ERROR << "' lexema_id = '" << err.lexema_id << "'";
    throw ZamarException(STR_INTERNAL_ERROR, getLocaleText(err.lexema_id, err.lparams, lang.get()));
  }

  point_id = bppax.point_dep;
  grp_id = bppax.grp_id;
  pax_id = bppax.pax_id;

  if (not trip_info.getByPointId(point_id))
    throw Exception("Failed trip_info.getByPointId %d", point_id);

  bool get_grp_result = false;
  if (type == ZamarType::DSM) get_grp_result = grp_item.getByGrpId(grp_id);
  else if (type == ZamarType::SBDO) get_grp_result = grp_item.getByGrpIdWithBagConcepts(grp_id);
  if (not get_grp_result)
  {
    string err_str = "Unknown grp_id error %d";
    if (type == ZamarType::DSM) err_str = "Failed grp_item.getByGrpId %d";
    else if (type == ZamarType::SBDO) err_str = "Failed grp_item.getByGrpIdWithBagConcepts %d";
    throw Exception(err_str.c_str(), grp_id);
  }

  if (not pax_item.getByPaxId(pax_id))
    throw Exception("Failed pax_item.getByPaxId %d", pax_id);

  doc_exists = CheckIn::LoadPaxDoc(pax_id, doc);
  doco_exists = CheckIn::LoadPaxDoco(pax_id, doco);
  mkt_flt.getByPaxId(pax_id);
  if (mkt_flt.empty())
    throw Exception("Failed mkt_flt.getByPaxId %d", pax_id);

  // flightStatus
  flightCheckinStage = TTripStages(point_id).getStage( stCheckIn );
  // pnr
  pnrs.getByPaxIdFast(pax_id);
  // baggageTags
  if (type == ZamarType::DSM) GetTagsByPool(grp_id, pax_item.bag_pool_num, bagTagsExtended, false);

  return *this;
}

//-----------------------------------------------------------------------------------

static void BagTagsToZamarXML(xmlNodePtr resNode, const std::multimap<TBagTagNumber, CheckIn::TBagItem>& tags_ext)
{
  if (resNode == nullptr) return;
  xmlNodePtr tagsNode = NewTextChild(resNode, "baggageTags");
  for (const auto & tag : tags_ext)
  {
    xmlNodePtr tagNode = NewTextChild(tagsNode, "baggageTag");
    SetProp(tagNode, "id", tag.first.str());
    SetProp(tagNode, "weight", tag.second.weight);
    SetProp(tagNode, "unit", "KG");
  }
}

static void PnrToZamarXML(xmlNodePtr resNode, const TPnrAddrs& pnrs, const boost::optional<AstraLocale::OutputLang>& lang)
{
  if (resNode == nullptr) return;
  xmlNodePtr addrsNode = NewTextChild(resNode, "pnrAddrs");
  for (const TPnrAddrInfo& pnr : pnrs)
  {
    xmlNodePtr pnrNode = NewTextChild(addrsNode, "pnr", lang ? convert_pnr_addr(pnr.addr, lang->isLatin()) : pnr.addr);
    SetProp(pnrNode, "airline", lang ? airlineToPrefferedCode(pnr.airline, lang.get()) : pnr.airline);
  }
}

static void DocToZamarXML(xmlNodePtr resNode, const CheckIn::TPaxDocItem& doc, const boost::optional<AstraLocale::OutputLang>& lang)
{
  if (resNode == nullptr) return;
  xmlNodePtr docNode = NewTextChild(resNode, "document");
  NewTextChild(docNode, "type", lang ? ElemIdToPrefferedElem(etPaxDocType, doc.type, efmtCodeNative, lang->get()) : doc.type);
  NewTextChild(docNode, "issueCountry", CheckIn::paxDocCountryToWebXML(doc.issue_country, lang));
  NewTextChild(docNode, "no", doc.no);
  NewTextChild(docNode, "nationality", CheckIn::paxDocCountryToWebXML(doc.nationality, lang));
  if (doc.birth_date != ASTRA::NoExists)
    NewTextChild(docNode, "birthDate", DateTimeToStr(doc.birth_date, "yyyy-mm-dd"));
  else
    NewTextChild(docNode, "birthDate");
  NewTextChild(docNode, "gender", lang ? ElemIdToPrefferedElem(etGenderType, doc.gender, efmtCodeNative, lang->get()) : doc.gender);
  if (doc.expiry_date != ASTRA::NoExists)
    NewTextChild(docNode, "expiryDate", DateTimeToStr(doc.expiry_date, "yyyy-mm-dd"));
  else
    NewTextChild(docNode, "expiryDate");
  NewTextChild(docNode, "surname", doc.surname);
  NewTextChild(docNode, "firstName", doc.first_name);
  NewTextChild(docNode, "secondName", doc.second_name);
}

static void DocoToZamarXML(xmlNodePtr resNode, const CheckIn::TPaxDocoItem& doco, const boost::optional<AstraLocale::OutputLang>& lang)
{
  if (resNode == nullptr) return;
  xmlNodePtr docNode = NewTextChild(resNode, "doco");
  NewTextChild(docNode, "birthPlace", doco.birth_place);
  NewTextChild(docNode, "type", lang ? ElemIdToPrefferedElem(etPaxDocType, doco.type, efmtCodeNative, lang->get()) : doco.type);
  NewTextChild(docNode, "no", doco.no);
  NewTextChild(docNode, "issuePlace", doco.issue_place);
  if (doco.issue_date != ASTRA::NoExists)
    NewTextChild(docNode, "issueDate", DateTimeToStr(doco.issue_date, "yyyy-mm-dd"));
  else
    NewTextChild(docNode, "issueDate");
  if (doco.expiry_date != ASTRA::NoExists)
    NewTextChild(docNode, "expiryDate", DateTimeToStr(doco.expiry_date, "yyyy-mm-dd"));
  else
    NewTextChild(docNode, "expiryDate");
  NewTextChild(docNode, "applicCountry", CheckIn::paxDocCountryToWebXML(doco.applic_country, lang));
}

static void BaggageListToZamarXML(xmlNodePtr listNode, const TRFISCList& list, const AstraLocale::OutputLang& lang)
{
  if (listNode == nullptr || list.empty()) return;
  for (const auto& item : list)
  {
    const auto& i = item.second;
    xmlNodePtr node = NewTextChild(listNode, "piece_concept");
    NewTextChild(node, "rfisc", i.RFISC);
    NewTextChild(node, "service_type", ServiceTypes().encode(i.service_type));
    NewTextChild(node, "airline", airlineToPrefferedCode(i.airline, lang));
    NewTextChild(node, "name_view", lowerc(i.name_view(lang.get())));
    i.priority? NewTextChild(node, "priority", i.priority.get()): NewTextChild(node, "priority");
  }
}

static void BaggageListToZamarXML(xmlNodePtr listNode, const TBagTypeList& list, const AstraLocale::OutputLang& lang)
{
  if (listNode == nullptr || list.empty()) return;
  for (const auto& item : list)
  {
    const auto& i = item.second;
    xmlNodePtr node = NewTextChild(listNode, "weight_concept");
    NewTextChild(node, "bag_type", i.bag_type);
    NewTextChild(node, "airline", airlineToPrefferedCode(i.airline, lang));
    NewTextChild(node, "name_view", i.name_view(lang.get()));
    NewTextChild(node, "descr_view", i.descr_view(lang.get()));
    i.priority? NewTextChild(node, "priority", i.priority.get()): NewTextChild(node, "priority");
  }
}

//-----------------------------------------------------------------------------------

const PassengerSearchResult& PassengerSearchResult::toXML(xmlNodePtr resNode, ZamarType type) const
{
  if (resNode == nullptr) return *this;

  AstraLocale::OutputLang lang("", {AstraLocale::OutputLang::OnlyTrueIataCodes});
  // lang
  SetProp(resNode, "lang", lang.get());

  // queryType
  // TODO queryType for DSM -- согласовать с клиентом?
  if (type == ZamarType::SBDO) NewTextChild(resNode, "queryType", "PassengerSearchSBDO");

  // sessionId
  NewTextChild(resNode, "sessionId", sessionId);
  // airline
  NewTextChild(resNode, "airline", airlineToPrefferedCode(trip_info.airline, lang));
  // flightCode
  NewTextChild(resNode, "flightCode", trip_info.flight_number(lang));
  // flightStatus
  NewTextChild(resNode, "flightStatus", EncodeStage(flightCheckinStage));
  // flightSTD
  TDateTime flightSTD = UTCToLocal(trip_info.scd_out, AirpTZRegion(trip_info.airp));
  NewTextChild(resNode, "flightSTD", DateTimeToStr(flightSTD, "yyyy-mm-dd'T'hh:nn:00"));
  // srcAirport
  NewTextChild(resNode, "srcAirport", airpToPrefferedCode(trip_info.airp, lang));
  // dstAirport
  NewTextChild(resNode, "dstAirport", airpToPrefferedCode(grp_item.airp_arv, lang));
  // gate
  vector<string> gates;
  TripsInterface::readGates(point_id, gates);
  xmlNodePtr gatesNode = NewTextChild(resNode, "gates");
  for (const auto& gate : gates)
  {
    NewTextChild(gatesNode, "gate", gate);
  }
  // codeshare
  xmlNodePtr codeshareNode = NewTextChild(resNode, "codeshare");
  xmlNodePtr flightNode = NewTextChild(codeshareNode, "flight");
  SetProp(flightNode, "airline", airlineToPrefferedCode(mkt_flt.airline, lang));
  SetProp(flightNode, "code", mkt_flt.flight_number(lang));

  // allowCheckIn -- SBDO
  if (type == ZamarType::SBDO)
  {
    bool allowCheckIn = grp_item.allowToBagCheckIn() &&
                        pax_item.allowToBagCheckIn() &&
                        TTripStages(point_id).allowToBagCheckIn();
    NewTextChild(resNode, "allowCheckIn", allowCheckIn? 1: 0);
  }

  // allowBoarding -- DSM
  if (type == ZamarType::DSM) NewTextChild(resNode, "allowBoarding", pax_item.allowToBoarding()? 1: 0);

  // paxId
  NewTextChild(resNode, "paxId", bppax.pax_id);
  // sequence
  NewTextChild(resNode, "sequence", pax_item.reg_no);
  // lastName
  NewTextChild(resNode, "lastName", pax_item.surname);
  // firstName
  NewTextChild(resNode, "firstName", pax_item.name);
  // document
  if (doc_exists)
    DocToZamarXML(resNode, doc, lang);
  else
    NewTextChild(resNode, "document");
  // doco
  if (doco_exists)
    DocoToZamarXML(resNode, doco, lang);
  else
    NewTextChild(resNode, "doco");
  // group
  NewTextChild(resNode, "group", "");
  // pnr
  PnrToZamarXML(resNode, pnrs, lang);
  // ticket
  NewTextChild(resNode, "ticket", pax_item.tkn.no_str());

  // seats -- SBDO
  if (type == ZamarType::SBDO)
  {
    xmlNodePtr seatsNode = NewTextChild(resNode, "seats");
    NewTextChild(seatsNode, "seat", "5A");
    NewTextChild(seatsNode, "seat", "5B");
  }

  // paxCategory
  NewTextChild(resNode, "paxCategory", ElemIdToPrefferedElem(etPersType, EncodePerson(pax_item.pers_type), efmtCodeNative, lang.get()));
  // status
  NewTextChild(resNode, "status", pax_item.checkInStatus());

  // cabinClass -- SBDO
  if (type == ZamarType::SBDO)
    NewTextChild(resNode, "cabinClass", ElemIdToPrefferedElem(etClass, pax_item.getCabinClass(), efmtCodeNative, lang.get()));
  // bookingClass -- SBDO
  if (type == ZamarType::SBDO)
    NewTextChild(resNode, "bookingClass", ElemIdToPrefferedElem(etSubcls, pax_item.subcl, efmtCodeNative, lang.get()));

  // bonusLevel -- SBDO
  if (type == ZamarType::SBDO)
  {
    set<CheckIn::TPaxFQTItem> fqts;
    if (CheckIn::LoadPaxFQTNotEmptyTierLevel(pax_id, fqts, true))
      NewTextChild(resNode, "bonusLevel", fqts.begin()->tier_level);
    else
      NewTextChild(resNode, "bonusLevel");
  }

  if (type == ZamarType::SBDO)
  {
    TBagTotals totals;
    /*bool totals_result =*/ pax_item.getBaggageInHoldTotals(totals);
    // bagTotalCount -- SBDO
    if (/*totals_result &&*/ totals.amount != ASTRA::NoExists) NewTextChild(resNode, "bagTotalCount", totals.amount);
    else NewTextChild(resNode, "bagTotalCount");
    // bagTotalWeight -- SBDO
    if (/*totals_result &&*/ totals.weight != ASTRA::NoExists) NewTextChild(resNode, "bagTotalWeight", totals.weight);
    else NewTextChild(resNode, "bagTotalWeight");
  }

  if (type == ZamarType::SBDO)
  {
    TBagConcept::Enum bagAllowanceType = grp_item.getBagAllowanceType();
    string bagAllowanceType_str;
    switch (bagAllowanceType)
    {
    case TBagConcept::Unknown: bagAllowanceType_str = "UNKNOWN"; break;
    case TBagConcept::Piece: bagAllowanceType_str = "PIECE"; break;
    case TBagConcept::Weight: bagAllowanceType_str = "WEIGHT"; break;
    default: break;
    }
    // bagAllowanceType -- SBDO
    NewTextChild(resNode, "bagAllowanceType", bagAllowanceType_str);

    boost::optional<TBagTotals> totals = boost::none;
    if (bagAllowanceType == TBagConcept::Piece) totals = PieceConcept::getBagAllowance(pax_item);
    else if (bagAllowanceType == TBagConcept::Weight) totals = WeightConcept::getBagAllowance(pax_item);
    // bagAllowanceCount -- SBDO
    if (totals && totals->amount != ASTRA::NoExists) NewTextChild(resNode, "bagAllowanceCount", totals->amount);
    else NewTextChild(resNode, "bagAllowanceCount");
    // bagAllowanceWeight -- SBDO
    if (totals && totals->weight != ASTRA::NoExists) NewTextChild(resNode, "bagAllowanceWeight", totals->weight);
    else NewTextChild(resNode, "bagAllowanceWeight");
  }

  // baggageRoute -- SBDO
  if (type == ZamarType::SBDO)
  {
    TTrferRoute route;
    route.GetRoute(grp_item.id, trtWithFirstSeg);
    xmlNodePtr routeNode = NewTextChild(resNode, "baggageRoute");
    int segment = 0;
    for (const auto& item : route)
    {
      xmlNodePtr itemNode = NewTextChild(routeNode, "item");
      NewTextChild(itemNode, "segment", ++segment);
      NewTextChild(itemNode, "airline", airlineToPrefferedCode(item.operFlt.airline, lang));
      NewTextChild(itemNode, "flightCode", item.operFlt.flight_number(lang));
      NewTextChild(itemNode, "srcAirport", airpToPrefferedCode(item.operFlt.airp, lang));
      NewTextChild(itemNode, "dstAirport", airpToPrefferedCode(item.airp_arv, lang));
    }
  }

  // baggageTypes -- SBDO
  if (type == ZamarType::SBDO)
  {
    xmlNodePtr listNode = NewTextChild(resNode, "baggageTypes");
    TRFISCList rfiscList;
    pax_item.getBaggageListForSBDO(rfiscList);
    BaggageListToZamarXML(listNode, rfiscList, lang);
    TBagTypeList bagTypeList;
    pax_item.getBaggageListForSBDO(bagTypeList);
    BaggageListToZamarXML(listNode, bagTypeList, lang);
  }

  // baggageTags -- DSM
  if (type == ZamarType::DSM) BagTagsToZamarXML(resNode, bagTagsExtended);

  return *this;
}

//-----------------------------------------------------------------------------------
// DSM

void PassengerSearchResult::errorXML(xmlNodePtr resNode, const std::string& cmd, const std::string &err)
{
  if (resNode == nullptr) return;
  NewTextChild(resNode, "command", cmd);
  NewTextChild(resNode, "error", err);
}

void ZamarDSMInterface::PassengerSearch(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  PassengerSearchResult result;
  try
  {
    result.fromXML(reqNode, ZamarType::DSM);
  }
  catch (ZamarException ZE)
  {
    PassengerSearchResult::errorXML(resNode, ZE.cmd, ZE.what());
    return;
  }
  catch (Exception E)
  {
    PassengerSearchResult::errorXML(resNode, STR_INTERNAL_ERROR, E.what());
    return;
  }
  result.toXML(resNode, ZamarType::DSM);
}

//-----------------------------------------------------------------------------------
// SBDO

void ZamarSBDOInterface::PassengerSearchSBDO(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  // FIXME повторяющийся код -- проанализировать после завершения работ по SBDO
  PassengerSearchResult result;
  try
  {
    result.fromXML(reqNode, ZamarType::SBDO);
  }
  catch (ZamarException ZE)
  {
    PassengerSearchResult::errorXML(resNode, ZE.cmd, ZE.what());
    return;
  }
  catch (Exception E)
  {
    PassengerSearchResult::errorXML(resNode, STR_INTERNAL_ERROR, E.what());
    return;
  }
  result.toXML(resNode, ZamarType::SBDO);
}

void ZamarSBDOInterface::PassengerBaggageTagAdd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (resNode == nullptr) return;
  NewTextChild(resNode, "queryType", "PassengerBaggageTagAdd");
  NewTextChild(resNode, "command", STR_UNDER_CONSTRUCTION);
}

void ZamarSBDOInterface::PassengerBaggageTagConfirm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (resNode == nullptr) return;
  NewTextChild(resNode, "queryType", "PassengerBaggageTagConfirm");
  NewTextChild(resNode, "command", STR_UNDER_CONSTRUCTION);
}

void ZamarSBDOInterface::PassengerBaggageTagRevoke(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (resNode == nullptr) return;
  NewTextChild(resNode, "queryType", "PassengerBaggageTagRevoke");
  NewTextChild(resNode, "command", STR_UNDER_CONSTRUCTION);
}
