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
#include "seats_utils.h"

#define NICKNAME "GRISHA"
#include <serverlib/slogger.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace EXCEPTIONS;

const string STR_PAX_NO_MATCH = "PAX_NO_MATCH";
const string STR_PAX_REFUSE = "PAX_REFUSE";
const string STR_TRFER_NO_CONFIRM = "TRFER_NO_CONFIRM";
const string STR_WRONG_CHECKIN_STATUS = "WRONG_CHECKIN_STATUS";
const string STR_CONCEPT_UNKNOWN = "CONCEPT_UNKNOWN";
const string STR_PIECE_EXCEED = "PIECE_EXCEED";
const string STR_WEIGHT_EXCEED = "WEIGHT_EXCEED";
const string STR_BAG_TYPE_NOT_ALLOWED = "BAG_TYPE_NOT_ALLOWED";
//const string STR_CONCEPT_NO_MATCH = "CONCEPT_NO_MATCH";
const string STR_BAG_TAG_PAID = "BAG_TAG_PAID";

const string STR_INTERNAL_ERROR = "INTERNAL_ERROR";
const string STR_UNDER_CONSTRUCTION = "UNDER_CONSTRUCTION";


struct ZamarException : public Exception
{
  string cmd;
  ZamarException(string a_cmd, string a_err) : Exception(a_err.c_str()), cmd(a_cmd) {}
};

void ZamarErrorXML(xmlNodePtr resNode, const std::string& cmd, const std::string &err)
{
  if (resNode == nullptr)
    return;
  NewTextChild(resNode, "command", cmd);
  NewTextChild(resNode, "error", err);
}

void ProcessXML(ZamarDataInterface& data, xmlNodePtr reqNode, xmlNodePtr resNode, ZamarType type)
{
  try
  {
    data.fromXML(reqNode, type);
  }
  catch (ZamarException& ZE)
  {
    ZamarErrorXML(resNode, ZE.cmd, ZE.what());
    return;
  }
  catch (Exception& E)
  {
    ZamarErrorXML(resNode, STR_INTERNAL_ERROR, E.what());
    return;
  }
  data.toXML(resNode, type);
}

//-----------------------------------------------------------------------------------
// DSM

void ZamarDSMInterface::PassengerSearch(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  PassengerSearchResult result;
  ProcessXML(result, reqNode, resNode, ZamarType::DSM);
}

//-----------------------------------------------------------------------------------
// SBDO

void ZamarSBDOInterface::PassengerSearchSBDO(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  PassengerSearchResult result;
  ProcessXML(result, reqNode, resNode, ZamarType::SBDO);
}

void ZamarSBDOInterface::PassengerBaggageTagAdd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ZamarBaggageTagAdd tag_add;
  ProcessXML(tag_add, reqNode, resNode, ZamarType::SBDO);
}

void ZamarSBDOInterface::PassengerBaggageTagConfirm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ZamarBaggageTagConfirm tag_confirm;
  ProcessXML(tag_confirm, reqNode, resNode, ZamarType::SBDO);
}

void ZamarSBDOInterface::PassengerBaggageTagRevoke(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ZamarBaggageTagRevoke tag_revoke;
  ProcessXML(tag_revoke, reqNode, resNode, ZamarType::SBDO);
}

//-----------------------------------------------------------------------------------

bool getBaggageInHoldTotalsDummy(int pax_id, TBagTotals& totals)
{
  totals.clear();
  if (pax_id == ASTRA::NoExists)
    return false;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
      "SELECT COUNT(*) AS amount_total, SUM(weight) AS weight_total "
      " FROM sbdo_tags_activated WHERE pax_id = :pax_id";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.Execute();
  if (not Qry.Eof)
  {
    totals.amount = Qry.FieldAsInteger("amount_total");
    totals.weight = Qry.FieldAsInteger("weight_total");
  }
  return !Qry.Eof;
}

//-----------------------------------------------------------------------------------
// PassengerSearch

void PassengerSearchResult::fromXML(xmlNodePtr reqNode, ZamarType type)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");

  AstraLocale::OutputLang lang("", {AstraLocale::OutputLang::OnlyTrueIataCodes});
  // sessionId
  sessionId = NodeAsString("sessionId", reqNode);
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
  if (type == ZamarType::DSM)
    get_grp_result = grp_item.getByGrpId(grp_id);
  else if (type == ZamarType::SBDO)
    get_grp_result = grp_item.getByGrpIdWithBagConcepts(grp_id);
  if (not get_grp_result)
  {
    string err_str = "Unknown grp_id error %d";
    if (type == ZamarType::DSM)
      err_str = "Failed grp_item.getByGrpId %d";
    else if (type == ZamarType::SBDO)
      err_str = "Failed grp_item.getByGrpIdWithBagConcepts %d";
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
  if (type == ZamarType::DSM)
    GetTagsByPool(grp_id, pax_item.bag_pool_num, bagTagsExtended, false);
}

//-----------------------------------------------------------------------------------

static void BagTagsToZamarXML(xmlNodePtr resNode, const std::multimap<TBagTagNumber, CheckIn::TBagItem>& tags_ext)
{
  if (resNode == nullptr)
    return;
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
  if (resNode == nullptr)
    return;
  xmlNodePtr addrsNode = NewTextChild(resNode, "pnrAddrs");
  for (const TPnrAddrInfo& pnr : pnrs)
  {
    xmlNodePtr pnrNode = NewTextChild(addrsNode, "pnr", lang ? convert_pnr_addr(pnr.addr, lang->isLatin()) : pnr.addr);
    SetProp(pnrNode, "airline", lang ? airlineToPrefferedCode(pnr.airline, lang.get()) : pnr.airline);
  }
}

static void DocToZamarXML(xmlNodePtr resNode, const CheckIn::TPaxDocItem& doc, const boost::optional<AstraLocale::OutputLang>& lang)
{
  if (resNode == nullptr)
    return;
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
  if (resNode == nullptr)
    return;
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

static void BaggageListToZamarXML(xmlNodePtr allowanceNode, const TRFISCList& list, const AstraLocale::OutputLang& lang)
{
  if (allowanceNode == nullptr || list.empty())
    return;
  for (const auto& item : list)
  {
    const auto& i = item.second;
    xmlNodePtr typeNode = NewTextChild(allowanceNode, "type");
    SetProp(typeNode, "airline", airlineToPrefferedCode(i.airline, lang));
    SetProp(typeNode, "rfisc", i.RFISC);
    SetProp(typeNode, "serviceType", ServiceTypes().encode(i.service_type));
    SetProp(typeNode, "displayName", lowerc(i.name_view(lang.get())));
    if(i.priority)
      SetProp(typeNode, "priority", i.priority.get());
  }
}

static void BaggageListToZamarXML(xmlNodePtr allowanceNode, const TBagTypeList& list, const AstraLocale::OutputLang& lang)
{
  if (allowanceNode == nullptr || list.empty())
    return;
  for (const auto& item : list)
  {
    const auto& i = item.second;
    xmlNodePtr typeNode = NewTextChild(allowanceNode, "type");
    SetProp(typeNode, "airline", airlineToPrefferedCode(i.airline, lang));
    SetProp(typeNode, "bagType", i.bag_type);
    SetProp(typeNode, "displayName", i.name_view(lang.get()));
    SetProp(typeNode, "displayDescr", i.descr_view(lang.get()));
    if(i.priority)
      SetProp(typeNode, "priority", i.priority.get());
  }
}

static void GetSeats(int point_id, int pax_id, vector<string>& seats, const AstraLocale::OutputLang& lang)
{
  seats.clear();
  TQuery SeatsQry(&OraSession);
  SeatsQry.SQLText=
    "SELECT yname AS seat_row, xname AS seat_column "
    "FROM pax_seats "
    "WHERE pax_id=:pax_id AND point_id=:point_id AND NVL(pr_wl,0)=0";
  SeatsQry.CreateVariable("point_id",otInteger,point_id);
  SeatsQry.CreateVariable("pax_id",otInteger,pax_id);
  SeatsQry.Execute();
  for(; !SeatsQry.Eof; SeatsQry.Next())
  {
    TSeat seat(IntToString(SeatsQry.FieldAsInteger("seat_row")), SeatsQry.FieldAsString("seat_column"));
    seats.push_back(seat.denorm_view(lang.isLatin()));
  }
}

//-----------------------------------------------------------------------------------

void PassengerSearchResult::toXML(xmlNodePtr resNode, ZamarType type) const
{
  if (resNode == nullptr)
    return;

  AstraLocale::OutputLang lang("", {AstraLocale::OutputLang::OnlyTrueIataCodes});
  // lang
  SetProp(resNode, "lang", lang.get());

  // queryType
  // TODO queryType for DSM -- согласовать с клиентом?
  if (type == ZamarType::SBDO)
    NewTextChild(resNode, "queryType", "PassengerSearchSBDO");

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
  if (type == ZamarType::DSM)
    NewTextChild(resNode, "allowBoarding", pax_item.allowToBoarding()? 1: 0);

  // paxId
  NewTextChild(resNode, "paxId", pax_id);
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
    vector<string> seats;
    GetSeats(point_id, pax_id, seats, lang);
    for (const auto& seat : seats)
    {
      xmlNodePtr seatNode = NewTextChild(seatsNode, "seat");
      SetProp(seatNode, "n", seat);
    }
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
    // pax_item.getBaggageInHoldTotals(totals); // возвращаемое значение не используется, нули - валидные значения
    getBaggageInHoldTotalsDummy(pax_id, totals);
    // bagTotalCount -- SBDO
    if (totals.amount != ASTRA::NoExists)
      NewTextChild(resNode, "bagTotalCount", totals.amount);
    else
      NewTextChild(resNode, "bagTotalCount");
    // bagTotalWeight -- SBDO
    if (totals.weight != ASTRA::NoExists)
    {
      xmlNodePtr nodeBagTotalWeight = NewTextChild(resNode, "bagTotalWeight", totals.weight);
      SetProp(nodeBagTotalWeight, "unit", "KG");
    }
    else
      NewTextChild(resNode, "bagTotalWeight");
  }

  if (type == ZamarType::SBDO)
  {
    TBagConcept::Enum bagAllowanceType = grp_item.getBagAllowanceType();
    string bagAllowanceType_str;
    switch (bagAllowanceType)
    {
    case TBagConcept::Unknown:
      bagAllowanceType_str = "unknown";
      break;
    case TBagConcept::Piece:
      bagAllowanceType_str = "piece";
      break;
    case TBagConcept::Weight:
      bagAllowanceType_str = "weight";
      break;
    default:
      break;
    }
    // bagAllowanceType -- SBDO
    xmlNodePtr allowanceNode = NewTextChild(resNode, "bagAllowance");
    SetProp(allowanceNode, "type", bagAllowanceType_str);

    boost::optional<TBagTotals> totals = boost::none;
    if (bagAllowanceType == TBagConcept::Piece)
      totals = PieceConcept::getBagAllowance(pax_item);
    else if (bagAllowanceType == TBagConcept::Weight)
      totals = WeightConcept::getBagAllowance(pax_item);
    // bagAllowanceCount -- SBDO
    if (totals && totals->amount != ASTRA::NoExists)
      SetProp(allowanceNode, "pks", totals->amount);
    // bagAllowanceWeight -- SBDO
    if (totals && totals->weight != ASTRA::NoExists)
    {
      SetProp(allowanceNode, "weight", totals->weight);
      SetProp(allowanceNode, "unit", "KG");
    }
    
    // baggageTypes -- SBDO
    TRFISCList rfiscList;
    pax_item.getBaggageListForSBDO(rfiscList);
    BaggageListToZamarXML(allowanceNode, rfiscList, lang);
    TBagTypeList bagTypeList;
    pax_item.getBaggageListForSBDO(bagTypeList);
    BaggageListToZamarXML(allowanceNode, bagTypeList, lang);
  }

  // baggageRoute -- SBDO
  if (type == ZamarType::SBDO)
  {
    TTrferRoute route;
    route.GetRoute(grp_item.id, trtWithFirstSeg);
    xmlNodePtr routeNode = NewTextChild(resNode, "baggageRoute");
    int id = 0;
    for (const auto& item : route)
    {
      xmlNodePtr segmentNode = NewTextChild(routeNode, "segment");
      SetProp(segmentNode, "id", ++id);
      SetProp(segmentNode, "airline", airlineToPrefferedCode(item.operFlt.airline, lang));
      SetProp(segmentNode, "flightCode", item.operFlt.flight_number(lang));
      SetProp(segmentNode, "src", airpToPrefferedCode(item.operFlt.airp, lang));
      SetProp(segmentNode, "dst", airpToPrefferedCode(item.airp_arv, lang));
    }
  }

  // baggageTags -- DSM
  if (type == ZamarType::DSM)
    BagTagsToZamarXML(resNode, bagTagsExtended);
}

//-----------------------------------------------------------------------------------
// ZamarBagTag

std::string ZamarBagTag::NoToStr() const
{
  TBagTagNumber tag("", no_dbl_);
  return tag.str();
}

void ZamarBagTag::Generate(int grp_id)
{
  if (generated_)
    throw Exception("Tag number already generated %s", NoToStr().c_str());
  if (activated_)
    throw Exception("Cannot generate number for activated tag %s", NoToStr().c_str());
  if (deactivated_)
    throw Exception("Cannot generate number for deactivated tag %s", NoToStr().c_str());
  
  TGeneratedTags gen_tags;
  gen_tags.generate(grp_id, 1);
  const auto& tags = gen_tags.tags();
  if (tags.empty())
    throw Exception("TGeneratedTags empty");
  const TBagTagNumber& tag = *tags.begin();
  no_dbl_ = tag.numeric_part;
  generated_ = true;
  
  toDB_generated();
}

void ZamarBagTag::fromXML_add(xmlNodePtr reqNode)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");
  
  pax_id_ = NodeAsInteger("passengerId", reqNode);
  xmlNodePtr tagNode = NodeAsNode("baggageTag", reqNode);
  weight_ = NodeAsInteger("@weight", tagNode); // TODO unit
  xmlNodePtr typeNode = NodeAsNode("baggageType", reqNode);
  TElemFmt airline_fmt = efmtUnknown;
  airline_ = ElemToElemId(etAirline, NodeAsString("@airline", typeNode), airline_fmt);
  if (airline_fmt == efmtUnknown)
    throw Exception("Unknown airline");
  
  string concept = NodeAsString("@type", typeNode);
  if (concept == "piece")
  {
    bag_concept_ = TBagConcept::Piece;
    rfisc_ = NodeAsString("@RFISC", typeNode);
    if (rfisc_.empty())
      throw Exception("Empty <RFISC>");
    string service_type = NodeAsString( "@service_type", typeNode);
    if (service_type.empty())
      throw Exception("Empty <service_type>");
    service_type_ = ServiceTypes().decode(service_type);
  }
  else if (concept == "weight")
  {
    // НА ВРЕМЯ ТЕСТИРОВАНИЯ ЗАГЛУШЕК
    throw Exception("WEIGHT CONCEPT IS NOT ALLOWED WHILE TESTING");
    bag_concept_ = TBagConcept::Weight;
    bag_type_ = NodeAsString( "@bag_type", typeNode);
    if (bag_type_.empty())
      throw Exception("Empty <bag_type>");
  }
  else
    throw Exception("Unrecognized <concept>");
  
  GetListId();
}

void ZamarBagTag::fromXML(xmlNodePtr reqNode)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");
  
  int pax_id = NodeAsInteger("passengerId", reqNode);
  no_dbl_ = NodeAsFloat("tag", reqNode);
  
  fromDB();
  
  if (pax_id != pax_id_)
    throw Exception("Passenger ID is wrong for this tag");
}

void ZamarBagTag::toXML(xmlNodePtr resNode) const
{
  if (resNode == nullptr)
    return;
  if (not generated_)
    throw Exception("ZamarBagTag::toXML: tag not generated");
  NewTextChild(resNode, "passengerId", pax_id_);
  NewTextChild(resNode, "tag", NoToStr());
}

void ZamarBagTag::GetListId()
{
  // TODO TRFISCListKey::fromSbdoXML и  TBagTypeListKey::fromSbdoXML
  if (bag_concept_ == TBagConcept::Piece)
  {
    TRFISCKey key;
    key.RFISC = rfisc_;
    key.service_type = service_type_;
    key.airline = airline_;
    key.getListItemByPaxId(pax_id_, 0, TServiceCategory::BaggageInHold, __FUNCTION__);
    list_id_ = key.list_id;
  }
  else if (bag_concept_ == TBagConcept::Weight)
  {
    TBagTypeKey key;
    key.bag_type = bag_type_;
    key.airline = airline_;
    key.getListItemByPaxId(pax_id_, 0, TServiceCategory::BaggageInHold, __FUNCTION__);
    list_id_ = key.list_id;
  }
  else
    throw Exception("GetListId() bag_concept_ unknown");
}

void ZamarBagTag::toDB_generated()
{
  if (not generated_)
    throw Exception("ZamarBagTag::toDB_generated: tag not generated");
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.CreateVariable("no", otFloat, no_dbl_);
  Qry.CreateVariable("pax_id", otInteger, pax_id_);
  Qry.CreateVariable("weight", otInteger, weight_);
  Qry.CreateVariable("list_id", otInteger, list_id_);
  Qry.CreateVariable("airline", otString, airline_);
  Qry.CreateVariable("deactivated", otInteger, 0);
  
  LogTrace(TRACE5) << "ZamarBagTag::toDB_generated() no='" << NoToStr() << "' pax_id_='" << pax_id_ <<
                      "' weight_='" << weight_ << "' list_id_='" << list_id_ << "' airline_='" << airline_ << "'";
  
  if (bag_concept_ == TBagConcept::Piece)
  {
    LogTrace(TRACE5) << "ZamarBagTag::toDB_generated() rfisc_='" << rfisc_ << "' service_type_='" << ServiceType() << "'";
    Qry.SQLText =
        "INSERT INTO sbdo_tags_generated(no, pax_id, weight, list_id, airline, RFISC, service_type, deactivated) "
        " VALUES(:no, :pax_id, :weight, :list_id, :airline, :RFISC, :service_type, :deactivated) ";
    Qry.CreateVariable("RFISC", otString, rfisc_);
    Qry.CreateVariable("service_type", otString, ServiceType());
  }
  else if (bag_concept_ == TBagConcept::Weight)
  {
    LogTrace(TRACE5) << "ZamarBagTag::toDB_generated() bag_type_='" << bag_type_ << "'";
    Qry.SQLText =
        "INSERT INTO sbdo_tags_generated(no, pax_id, weight, list_id, airline, bag_type, deactivated) "
        " VALUES(:no, :pax_id, :weight, :list_id, :airline, :bag_type, :deactivated) ";
    Qry.CreateVariable("bag_type", otString, bag_type_);
  }
  Qry.Execute();
}

void ZamarBagTag::toDB_activated() // ПОВТОРЯЮЩИЙСЯ КОД!!!
{
  if (not generated_)
    throw Exception("ZamarBagTag::toDB_activated: tag not generated");
  if (not activated_)
    throw Exception("ZamarBagTag::toDB_activated: tag not activated");
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.CreateVariable("no", otFloat, no_dbl_);
  Qry.CreateVariable("pax_id", otInteger, pax_id_);
  Qry.CreateVariable("weight", otInteger, weight_);
  Qry.CreateVariable("list_id", otInteger, list_id_);
  Qry.CreateVariable("airline", otString, airline_);
  
  LogTrace(TRACE5) << "ZamarBagTag::toDB_activated() no='" << NoToStr() << "' pax_id_='" << pax_id_ <<
                      "' weight_='" << weight_ << "' list_id_='" << list_id_ << "' airline_='" << airline_ << "'";
  
  if (bag_concept_ == TBagConcept::Piece)
  {
    LogTrace(TRACE5) << "ZamarBagTag::toDB_activated() rfisc_='" << rfisc_ << "' service_type_='" << ServiceType() << "'";
    Qry.SQLText =
        "INSERT INTO sbdo_tags_activated(no, pax_id, weight, list_id, airline, RFISC, service_type) "
        " VALUES(:no, :pax_id, :weight, :list_id, :airline, :RFISC, :service_type) ";
    Qry.CreateVariable("RFISC", otString, rfisc_);
    Qry.CreateVariable("service_type", otString, ServiceType());
  }
  else if (bag_concept_ == TBagConcept::Weight)
  {
    LogTrace(TRACE5) << "ZamarBagTag::toDB_activated() bag_type_='" << bag_type_ << "'";
    Qry.SQLText =
        "INSERT INTO sbdo_tags_activated(no, pax_id, weight, list_id, airline, bag_type) "
        " VALUES(:no, :pax_id, :weight, :list_id, :airline, :bag_type) ";
    Qry.CreateVariable("bag_type", otString, bag_type_);
  }
  Qry.Execute();
}

void ZamarBagTag::toDB_deactivated()
{
  if (not generated_)
    throw Exception("ZamarBagTag::toDB_deactivated: tag not generated");
  if (not deactivated_)
    throw Exception("ZamarBagTag::toDB_deactivated: tag not deactivated");
  // удаляем из активированных
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "DELETE FROM sbdo_tags_activated WHERE no = :no";
  Qry.CreateVariable("no", otFloat, no_dbl_);
  Qry.Execute();
  // выставляем статус деактивирована в таблице сгенерированных
  Qry.Clear();
  Qry.SQLText = "UPDATE sbdo_tags_generated SET deactivated = 1 WHERE no = :no";
  Qry.CreateVariable("no", otFloat, no_dbl_);
  Qry.Execute();
}

void ZamarBagTag::fromDB()
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  // ищем в активированных
  Qry.SQLText =
      "SELECT pax_id, weight, list_id, bag_type, RFISC, service_type, airline "
      " FROM sbdo_tags_activated WHERE no = :no";
  Qry.CreateVariable("no", otFloat, no_dbl_);
  Qry.Execute();
  if (not Qry.Eof) // активирована
  {
    generated_ = true;
    activated_ = true;
  }
  else // в активированных не нашли - ищем в сгенерированных
  {
    activated_ = false;
    Qry.Clear();
    Qry.SQLText =
        "SELECT pax_id, weight, list_id, bag_type, RFISC, service_type, airline, deactivated "
        " FROM sbdo_tags_generated WHERE no = :no";
    Qry.CreateVariable("no", otFloat, no_dbl_);
    Qry.Execute();
    if (not Qry.Eof) // сгенерирована
    {
      generated_ = true;
      deactivated_ = bool(Qry.FieldAsInteger("deactivated"));
    }
    else
      throw Exception("Bag tag not found %s", NoToStr().c_str());
  }
    
  pax_id_ = Qry.FieldAsInteger("pax_id");
  weight_ = Qry.FieldAsInteger("weight");
  list_id_ = Qry.FieldAsInteger("list_id");
  airline_ = Qry.FieldAsString("airline");
  
  if (not Qry.FieldIsNULL("RFISC"))
  {
    bag_concept_ = TBagConcept::Piece;
    rfisc_ = Qry.FieldAsString("RFISC");
    service_type_ = ServiceTypes().decode(Qry.FieldAsString("service_type"));
  }
  else
  {
    bag_concept_ = TBagConcept::Weight;
    bag_type_ = Qry.FieldAsString("bag_type");;
  }
}

void ZamarBagTag::Activate()
{
  if (activated_)
    throw Exception("ZamarBagTag::Activate: TAG ALREADY ACTIVATED");
  if (deactivated_)
    throw Exception("ZamarBagTag::Activate: TAG WAS DEACTIVATED");
  if (not generated_)
    throw Exception("ZamarBagTag::Activate: TAG NOT GENERATED");

  activated_ = true;
  toDB_activated();
}

void ZamarBagTag::Deactivate()
{
  if (deactivated_)
    throw Exception("ZamarBagTag::Deactivate: TAG ALREADY DEACTIVATED");
  if (not activated_)
    throw Exception("ZamarBagTag::Deactivate: TAG NOT ACTIVATED");
  if (not generated_)
    throw Exception("ZamarBagTag::Deactivate: TAG NOT GENERATED");

  activated_ = false;
  deactivated_ = true;
  toDB_deactivated();
}

//-----------------------------------------------------------------------------------
// PassengerBaggageTagAdd

const vector<TRFISCListKey>& GetAllowedRFISCs()
{
  static vector<TRFISCListKey> v;
  if (v.empty())
  {
    TServiceType::Enum s = TServiceType::BaggageCharge;
    v.emplace_back("0GO", s, "ЮТ");
    v.emplace_back("03C", s, "ЮТ");
    v.emplace_back("0M6", s, "ЮТ");
    v.emplace_back("0C2", s, "ЮТ");
    v.emplace_back("0DD", s, "ЮТ");
  }
  return v;
}

void CheckPieceConceptAllowance(const ZamarBagTag& tag, int pieces, int allowed)
{
  // 1) проверка платности багажа
  TRFISCListKey key(tag.rfisc_, tag.service_type_, tag.airline_);
  auto v = GetAllowedRFISCs();
  if (find(v.begin(), v.end(), key) == v.end())
    throw ZamarException(STR_BAG_TAG_PAID, "Not found in allowed");
  // 2) проверка превышения нормы
  TBagTotals totals;
  getBaggageInHoldTotalsDummy(tag.pax_id_, totals);
  if ((totals.amount + pieces) > allowed)
  {
    stringstream ss;
    ss << totals.amount << " pieces already activated when " << allowed << " allowed";
    throw ZamarException(STR_BAG_TAG_PAID, ss.str());
  }
}

void ZamarBaggageTagAdd::fromXML(xmlNodePtr reqNode, ZamarType)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");
  session_id_ = NodeAsString("sessionId", reqNode);
  if (session_id_.empty())
    throw Exception("Empty <sessionId>");
  
  tag_.fromXML_add(reqNode);
  
  const int pax_id = tag_.pax_id_;
  const TBagConcept::Enum bag_concept = tag_.bag_concept_;

  CheckIn::TSimplePaxItem pax_item;
  stringstream ss_pax_id_err;
  ss_pax_id_err << "pax_id='" << pax_id << "'";
  if (not pax_item.getByPaxId(pax_id)) // Если пассажир не найден (ид. неверный)
  {
    ss_pax_id_err << " Failed pax_item.getByPaxId";
    throw ZamarException(STR_PAX_NO_MATCH, ss_pax_id_err.str());
  }
  if (not pax_item.allowToBagCheckIn()) // Если пассажир не в том статусе
    throw ZamarException(STR_PAX_REFUSE, ss_pax_id_err.str());

  int grp_id = pax_item.grp_id;
  CheckIn::TPaxGrpItem grp_item;
  stringstream ss_grp_id_err;
  ss_grp_id_err << "grp_id='" << grp_id << "'";
  if (not grp_item.getByGrpIdWithBagConcepts(grp_id))
  {
    ss_grp_id_err << " Failed grp_item.getByGrpId";
    throw ZamarException(STR_INTERNAL_ERROR, ss_grp_id_err.str());
  }
  if (not grp_item.allowToBagCheckIn()) // проанализировать trfer_confirm == true
    throw ZamarException(STR_TRFER_NO_CONFIRM, ss_grp_id_err.str());

  // Если этап тех графика "регистрация" не в том статусе
  if (not TTripStages(grp_item.point_dep).allowToBagCheckIn())
    throw ZamarException(STR_WRONG_CHECKIN_STATUS, "");

  // Нужно проанализировать что concept != unknown
  TBagConcept::Enum bag_concept_grp = grp_item.getBagAllowanceType();
  if (bag_concept_grp == TBagConcept::Unknown)
    throw ZamarException(STR_CONCEPT_UNKNOWN, "");
//  if (bag_concept_grp != bag_concept) throw ZamarException(STR_CONCEPT_NO_MATCH, ""); // ???
  
  boost::optional<TBagTotals> totals = boost::none;
  if (bag_concept == TBagConcept::Piece)
  {
    totals = PieceConcept::getBagAllowance(pax_item);
    int pieces = 1; // 1 бирка = 1 место багажа
    int allowed = 1; // TODO УТОЧНИТЬ
    if (totals and totals->amount != ASTRA::NoExists)
      allowed = totals->amount;
    // ПЕРЕДЕЛАТЬ vvvvv
    if (pieces > allowed)
    {
      stringstream ss;
      ss << "pieces='" << pieces << "' when allowed='" << allowed << "'";
      throw ZamarException(STR_PIECE_EXCEED, ss.str());
    }
    // ПЕРЕДЕЛАТЬ ^^^^^
    // !!! ПРОВЕРКА ПЛАТНОСТИ ДОБАВЛЯЕМОГО БАГАЖА !!!
    CheckPieceConceptAllowance(tag_, pieces, allowed);
  }
  else if (bag_concept == TBagConcept::Weight)
  {
    // Что вес багажа в пределах допустимого
    totals = WeightConcept::getBagAllowance(pax_item);
    const int ALLOWED_MIN = 1;
    const int ALLOWED_MAX_DEFAULT = 99;
    int weight = tag_.weight_;
    int allowed = ALLOWED_MAX_DEFAULT;
    if (totals and totals->weight != ASTRA::NoExists)
      allowed = totals->weight;
    // ПЕРЕДЕЛАТЬ vvvvv
    if (weight < ALLOWED_MIN or weight > allowed)
    {
      stringstream ss;
      ss << "weight='" << weight << "' when allowed='" << allowed << "'";
      throw ZamarException(STR_WEIGHT_EXCEED, ss.str());
    }
    // ПЕРЕДЕЛАТЬ ^^^^^
  }

  // Что тип багажа есть среди типов багажа пассажира
  if (bag_concept == TBagConcept::Piece)
  {
    TRFISCList rfiscList;
    pax_item.getBaggageListForSBDO(rfiscList);
    auto found = find_if(rfiscList.cbegin(), rfiscList.cend(),
                         [this](const auto& i)
                         { return i.second.RFISC == tag_.rfisc_ && i.second.service_type == tag_.service_type_; });
    if (found == rfiscList.cend())
      throw ZamarException(STR_BAG_TYPE_NOT_ALLOWED, "");
  }
  else if (bag_concept == TBagConcept::Weight)
  {
    TBagTypeList bagTypeList;
    pax_item.getBaggageListForSBDO(bagTypeList);
    auto found = find_if(bagTypeList.cbegin(), bagTypeList.cend(),
                         [this](const auto& i)
                         { return i.second.bag_type == tag_.bag_type_; });
    if (found == bagTypeList.cend())
      throw ZamarException(STR_BAG_TYPE_NOT_ALLOWED, "");
  }

  tag_.Generate(grp_id);
}

void ZamarBaggageTagAdd::toXML(xmlNodePtr resNode, ZamarType) const
{
  if (resNode == nullptr)
    return;
  NewTextChild(resNode, "queryType", "PassengerBaggageTagAdd");
  NewTextChild(resNode, "sessionId", session_id_);
  tag_.toXML(resNode);
}

//-----------------------------------------------------------------------------------
// PassengerBaggageTagConfirm

void ZamarBaggageTagConfirm::fromXML(xmlNodePtr reqNode, ZamarType)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");
  session_id_ = NodeAsString("sessionId", reqNode);
  if (session_id_.empty())
    throw Exception("Empty <sessionId>");
  
  tag_.fromXML(reqNode);
  
  const int pax_id = tag_.pax_id_;
  const TBagConcept::Enum bag_concept = tag_.bag_concept_;
  CheckIn::TSimplePaxItem pax_item; // ПОВТОРЯЮЩИЙСЯ КОД !!!
  stringstream ss_pax_id_err;
  ss_pax_id_err << "pax_id='" << pax_id << "'";
  if (not pax_item.getByPaxId(pax_id)) // Если пассажир не найден (ид. неверный)
  {
    ss_pax_id_err << " Failed pax_item.getByPaxId";
    throw ZamarException(STR_PAX_NO_MATCH, ss_pax_id_err.str());
  }
  if (not pax_item.allowToBagCheckIn()) // Если пассажир не в том статусе
    throw ZamarException(STR_PAX_REFUSE, ss_pax_id_err.str());
  
  boost::optional<TBagTotals> totals = boost::none;
  if (bag_concept == TBagConcept::Piece)
  {
    totals = PieceConcept::getBagAllowance(pax_item);
    int pieces = 1; // 1 бирка = 1 место багажа
    int allowed = 1; // TODO УТОЧНИТЬ
    if (totals and totals->amount != ASTRA::NoExists)
      allowed = totals->amount;
    // ПЕРЕДЕЛАТЬ vvvvv
    if (pieces > allowed)
    {
      stringstream ss;
      ss << "pieces='" << pieces << "' when allowed='" << allowed << "'";
      throw ZamarException(STR_PIECE_EXCEED, ss.str());
    }
    // ПЕРЕДЕЛАТЬ ^^^^^
    // !!! ПРОВЕРКА ПЛАТНОСТИ ДОБАВЛЯЕМОГО БАГАЖА !!!
    CheckPieceConceptAllowance(tag_, pieces, allowed);
  }
  
  tag_.Activate();
}

void ZamarBaggageTagConfirm::toXML(xmlNodePtr resNode, ZamarType) const
{
  if (resNode == nullptr)
    return;
  NewTextChild(resNode, "queryType", "PassengerBaggageTagConfirm");
  NewTextChild(resNode, "sessionId", session_id_);
  tag_.toXML(resNode);
}

//-----------------------------------------------------------------------------------
// PassengerBaggageTagRevoke

void ZamarBaggageTagRevoke::fromXML(xmlNodePtr reqNode, ZamarType)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");
  session_id_ = NodeAsString("sessionId", reqNode);
  if (session_id_.empty())
    throw Exception("Empty <sessionId>");
  
  tag_.fromXML(reqNode);
  tag_.Deactivate();
}

void ZamarBaggageTagRevoke::toXML(xmlNodePtr resNode, ZamarType) const
{
  if (resNode == nullptr)
    return;
  NewTextChild(resNode, "queryType", "PassengerBaggageTagRevoke");
  NewTextChild(resNode, "sessionId", session_id_);
  tag_.toXML(resNode);
}

