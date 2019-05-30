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
#include "rfisc_calc.h"
#include "edi_utils.h" //⮫쪮 ࠤ� isTagAddRequestSBDO � �.�
#include "checkin.h"

#define NICKNAME "GRISHA"
#include <serverlib/slogger.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;

const string STR_PAX_NO_MATCH = "PAX_NO_MATCH";
const string STR_PAX_REFUSE = "PAX_REFUSE";
const string STR_TRFER_NO_CONFIRM = "TRFER_NO_CONFIRM";
const string STR_WRONG_CHECKIN_STATUS = "WRONG_CHECKIN_STATUS";
const string STR_CONCEPT_UNKNOWN = "CONCEPT_UNKNOWN";
//const string STR_PIECE_EXCEED = "PIECE_EXCEED";
const string STR_WEIGHT_EXCEED = "WEIGHT_EXCEED";
const string STR_BAG_TYPE_NOT_ALLOWED = "BAG_TYPE_NOT_ALLOWED";
//const string STR_CONCEPT_NO_MATCH = "CONCEPT_NO_MATCH";
const string STR_BAG_TAG_PAID = "BAG_TAG_PAID";
// TODO INVALID XML

const string STR_BAGGAGE_NOT_ALLOWED = "BAGGAGE_NOT_ALLOWED";

const string STR_INTERNAL_ERROR = "INTERNAL_ERROR"; // TODO �ᯮ�짮���� �� �������
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

class FinishProcessXML {};

static void ProcessXML(ZamarDataInterface& data, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode, ZamarType type)
{
  try
  {
    data.fromXML(reqNode, externalSysResNode, type);
  }
  catch (FinishProcessXML)
  {
    return;
  }
  catch (ZamarException& ZE)
  {
    ASTRA::rollback(); //!!!����� ���ᬮ���� ��ࠡ��� �訡��
    ZamarErrorXML(resNode, ZE.cmd, ZE.what());
    return;
  }
  catch (Exception& E)
  {
    ASTRA::rollback(); //!!!����� ���ᬮ���� ��ࠡ��� �訡��
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
  ProcessXML(result, reqNode, nullptr, resNode, ZamarType::DSM);
}

//-----------------------------------------------------------------------------------
// SBDO

void ZamarSBDOInterface::PassengerSearchSBDO(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  PassengerSearchResult result;
  ProcessXML(result, reqNode, nullptr, resNode, ZamarType::SBDO);
}

void ZamarSBDOInterface::PassengerBaggageTagAdd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ZamarSBDOInterface::PassengerBaggageTagAdd(reqNode, nullptr, resNode);
}

void ZamarSBDOInterface::PassengerBaggageTagAdd(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  ZamarBaggageTagAdd tag_add;
  ProcessXML(tag_add, reqNode, externalSysResNode, resNode, ZamarType::SBDO);
}

void ZamarSBDOInterface::PassengerBaggageTagConfirm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ZamarSBDOInterface::PassengerBaggageTagConfirm(reqNode, nullptr, resNode);
}

void ZamarSBDOInterface::PassengerBaggageTagConfirm(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  ZamarBaggageTagConfirm tag_confirm;
  ProcessXML(tag_confirm, reqNode, externalSysResNode, resNode, ZamarType::SBDO);
}

void ZamarSBDOInterface::PassengerBaggageTagRevoke(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ZamarSBDOInterface::PassengerBaggageTagRevoke(reqNode, nullptr, resNode);
}

void ZamarSBDOInterface::PassengerBaggageTagRevoke(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  ZamarBaggageTagRevoke tag_revoke;
  ProcessXML(tag_revoke, reqNode, externalSysResNode, resNode, ZamarType::SBDO);
}

//-----------------------------------------------------------------------------------
// PassengerSearch

void PassengerSearchResult::fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType type)
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

// TODO ᤥ���� �㭪��, ����������� � ᯨ᮪ �� ����� (�� ������⢨� ⠪����� � ᯨ᪥)
const list<string> LANG_LIST = {AstraLocale::LANG_RU, AstraLocale::LANG_EN};

static void BaggageListToZamarXML(xmlNodePtr allowanceNode, const TRFISCListWithProps& list, const AstraLocale::OutputLang& lang)
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
//    SetProp(typeNode, "displayName", i.name_view(lang.get()));
    for (auto lang_str : LANG_LIST)
    {
      xmlNodePtr displayNameNode = NewTextChild(typeNode, "displayName", i.name_view(lang_str));
      SetProp(displayNameNode, "lang", lang_str);
    }
    if(i.priority)
      SetProp(typeNode, "priority", i.priority.get());
    boost::optional<TRFISCBagProps> props=list.getBagProps(i);
    if (props)
    {
      SetProp(typeNode, "minWeight", props.get().min_weight, ASTRA::NoExists);
      SetProp(typeNode, "maxWeight", props.get().max_weight, ASTRA::NoExists);
    }
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
//    SetProp(typeNode, "displayName", i.name_view(lang.get()));
    for (auto lang_str : LANG_LIST)
    {
      xmlNodePtr displayNameNode = NewTextChild(typeNode, "displayName", i.name_view(lang_str));
      SetProp(displayNameNode, "lang", lang_str);
    }
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
  // TODO queryType for DSM -- ᮣ��ᮢ��� � �����⮬?
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
    pax_item.getBaggageInHoldTotals(totals); // �����頥��� ���祭�� �� �ᯮ������, �㫨 - ������� ���祭��
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
    TRFISCListWithProps rfiscList;
    pax_item.getBaggageListForSBDO(rfiscList);
    rfiscList.setPriority(); //������塞 �ਮ��� �뢮��
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

std::string ZamarBagTag::tagNumber() const
{
  if (!tagNumber_) return "";
  return tagNumber_.get().str();
}

void ZamarBagTag::tagNumberToDB(TQuery &Qry) const
{
  tagNumber_?Qry.CreateVariable("no", otFloat, tagNumber_.get().numeric_part):
             Qry.CreateVariable("no", otFloat, FNull);
}

void ZamarBagTag::paxIdToDB(TQuery &Qry) const
{
  pax_id_!=NoExists?Qry.CreateVariable("pax_id", otInteger, pax_id_):
                    Qry.CreateVariable("pax_id", otInteger, FNull);
}

TBagConcept::Enum ZamarBagTag::bagConcept() const
{
  return bag_.pc?(bag_.wt?TBagConcept::Unknown:
                          TBagConcept::Piece):
                 (bag_.wt?TBagConcept::Weight:
                          TBagConcept::Unknown);
}

void ZamarBagTag::Generate(int grp_id)
{
  if (generated_)
    throw Exception("Tag number already generated %s", tagNumber().c_str());
  if (activated_)
    throw Exception("Cannot generate number for activated tag %s", tagNumber().c_str());
  if (deactivated_)
    throw Exception("Cannot generate number for deactivated tag %s", tagNumber().c_str());

  TGeneratedTags generated;
  generated.generate(grp_id, 1);
  if (generated.tags().size()!=1)
    throw Exception("ZamarBagTag::Generate: generated.tags().size()!=1");
  tagNumber_=*(generated.tags().begin());
  generated_ = true;

  toDB_generated();
}

void ZamarBagTag::fromXML_add(xmlNodePtr reqNode)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");

  pax_id_ = NodeAsInteger("passengerId", reqNode);
  xmlNodePtr tagNode = NodeAsNode("baggageTag", reqNode);
  bag_.amount = 1;
  bag_.weight = NodeAsInteger("@weight", tagNode); // TODO unit
  xmlNodePtr typeNode = NodeAsNode("baggageType", reqNode);

  string concept = NodeAsString("@type", typeNode);

  TElemFmt airline_fmt = efmtUnknown;
  string airline = ElemToElemId(etAirline, NodeAsString("@airline", typeNode), airline_fmt);
  if (airline.empty())
    throw Exception("Unknown @airline");

  if (concept == "piece")
  {
    bag_.pc=boost::in_place();
    TRFISCListKey& key=bag_.pc.get();

    key.airline = airline;
    key.RFISC = NodeAsString("@rfisc", typeNode);
    if (bag_.pc.get().RFISC.empty())
      throw Exception("Empty @rfisc");
    key.service_type=ServiceTypes().decode(NodeAsString( "@serviceType", typeNode));
    if (key.service_type==TServiceType::Unknown)
      throw Exception("Unknown @serviceType");
  }
  else if (concept == "weight")
  {
    // �� ����� ������������ ��������
    throw Exception("WEIGHT CONCEPT IS NOT ALLOWED WHILE TESTING");
    bag_.wt=boost::in_place();
    TBagTypeListKey& key=bag_.wt.get();

    key.airline = airline;
    key.bag_type = NodeAsString( "@bag_type", typeNode);
    if (key.bag_type.empty())
      throw Exception("Empty @bag_type");
  }
  else
    throw Exception("Unrecognized <concept>");

  SetListId();
}

void ZamarBagTag::fromXML(xmlNodePtr reqNode)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");

  int pax_id = NodeAsInteger("passengerId", reqNode);
  tagNumber_=boost::in_place("", NodeAsFloat("tag", reqNode));

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
  NewTextChild(resNode, "tag", tagNumber());
}

void ZamarBagTag::SetListId()
{
  // TODO TRFISCListKey::fromSbdoXML �  TBagTypeListKey::fromSbdoXML
  try
  {
    if (bag_.pc)
    {
      TRFISCKey& key=bag_.pc.get();
      key.getListItemByPaxId(pax_id_, 0, TServiceCategory::BaggageInHold, __FUNCTION__);
      if (!key.list_item || !key.list_item.get().isBaggageInHold())
        throw ZamarException(STR_BAG_TYPE_NOT_ALLOWED, "");
    }

    if (bag_.wt)
    {
      TBagTypeKey& key=bag_.wt.get();
      key.getListItemByPaxId(pax_id_, 0, TServiceCategory::BaggageInHold, __FUNCTION__);
      if (!key.list_item || !key.list_item.get().isBaggageInHold())
        throw ZamarException(STR_BAG_TYPE_NOT_ALLOWED, "");
    }
  }
  catch(EConvertError &e)
  {
    LogTrace(TRACE5) << e.what();
    throw ZamarException(STR_BAG_TYPE_NOT_ALLOWED, "");
  }
}

void ZamarBagTag::toDB_generated()
{
  if (not generated_)
    throw Exception("ZamarBagTag::toDB_generated: tag not generated");
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO sbdo_tags_generated(no, pax_id, weight, list_id, bag_type, rfisc, service_type, airline, deactivated) "
    "VALUES(:no, :pax_id, :weight, :list_id, :bag_type, :rfisc, :service_type, :airline, 0) ";

  tagNumberToDB(Qry);
  paxIdToDB(Qry);
  Qry.CreateVariable("weight", otInteger, bag_.weight);
  Qry.DeclareVariable("list_id", otInteger);
  Qry.DeclareVariable("bag_type", otString);
  Qry.DeclareVariable("rfisc", otString);
  Qry.DeclareVariable("service_type", otString);
  Qry.DeclareVariable("airline", otString);
  if (bag_.pc) bag_.pc.get().toDB(Qry);
  else if (bag_.wt) bag_.wt.get().toDB(Qry);

  LogTrace(TRACE5) << __func__
                   << ": no=" << tagNumber()
                   << ", pax_id=" << pax_id_
                   << ", weight=" << bag_.weight
                   << ", "
                   << (bag_.pc?bag_.pc.get().traceStr():"")
                   << (bag_.wt?bag_.wt.get().traceStr():"");
  Qry.Execute();
}

void ZamarBagTag::toDB_activated(xmlNodePtr reqNode, xmlNodePtr externalSysResNode) // ������������� ���!!!
{
  if (not generated_)
    throw Exception("ZamarBagTag::toDB_activated: tag not generated");
  if (not activated_)
    throw Exception("ZamarBagTag::toDB_activated: tag not activated");

  RemoveNode(GetNode("TCkinSavePax", reqNode));
  xmlNodePtr emulReqNode=NewTextChild(reqNode, "TCkinSavePax");

  if (!tagNumber_)
    throw Exception("%s: !tagNumber_", __FUNCTION__);

  createEmulDocForSBDO(pax_id_,
                       tagNumber_.get(),
                       bag_,
                       emulReqNode);

  CheckInInterface::SavePax(emulReqNode, externalSysResNode, nullptr);

  if (isDoomedToWait())
    throw FinishProcessXML();
}

void ZamarBagTag::toDB_deactivated(xmlNodePtr reqNode, xmlNodePtr externalSysResNode)
{
  if (not generated_)
    throw Exception("ZamarBagTag::toDB_deactivated: tag not generated");
  if (not deactivated_)
    throw Exception("ZamarBagTag::toDB_deactivated: tag not deactivated");

  RemoveNode(GetNode("TCkinSavePax", reqNode));
  xmlNodePtr emulReqNode=NewTextChild(reqNode, "TCkinSavePax");

  if (!tagNumber_)
    throw Exception("%s: !tagNumber_", __FUNCTION__);

  // 㤠�塞 �� ��⨢�஢�����=��ॣ����஢�����
  createEmulDocForSBDO(pax_id_,
                       tagNumber_.get(),
                       boost::none,
                       emulReqNode);

  CheckInInterface::SavePax(emulReqNode, externalSysResNode, nullptr);

  if (isDoomedToWait())
    throw FinishProcessXML();

  // ���⠢�塞 ����� ����⨢�஢��� � ⠡��� ᣥ���஢�����
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "UPDATE sbdo_tags_generated SET deactivated = 1 WHERE no = :no";
  tagNumberToDB(Qry);
  Qry.Execute();
}

void ZamarBagTag::fromDB()
{
  TQuery Qry(&OraSession);
  //᭠砫� �ᥣ�� �饬 � ᣥ���஢�����
  Qry.Clear();
  Qry.SQLText =
      "SELECT sbdo_tags_generated.*, 1 AS amount FROM sbdo_tags_generated WHERE no = :no";
  tagNumberToDB(Qry);
  Qry.Execute();
  if (Qry.Eof) throw Exception("Bag tag not found %s", tagNumber().c_str());

  // ᣥ���஢���
  generated_ = true;
  activated_ = false;
  deactivated_ = Qry.FieldAsInteger("deactivated")!=0;
  pax_id_ = Qry.FieldAsInteger("pax_id");
  bag_.fromDB(Qry);

  if (generated_ && !deactivated_ && tagNumber_)
  {
    int tagOwner=ASTRA::NoExists;
    activated_=CheckIn::TGroupBagItem::tagNumberUsedInGroup(pax_id_, tagNumber_.get(), tagOwner);

    if (activated_ && (tagOwner==ASTRA::NoExists || tagOwner!=pax_id_))
      //�� �����-� ��稭� ��ઠ ���� � ��㯯�, �� �ਢ易�� � ��㣮�� ���ᠦ��� (������ ����� ��९ਢ易�� � �ନ����)
      //�� ��� �� �ਢ易�� �����, �� �� ��࠭��
      throw ZamarException(STR_BAGGAGE_NOT_ALLOWED, "Not allowed");
  }
}

void ZamarBagTag::Activate(xmlNodePtr reqNode, xmlNodePtr externalSysResNode)
{
  if (activated_)
    throw Exception("ZamarBagTag::Activate: TAG ALREADY ACTIVATED");
  if (deactivated_)
    throw Exception("ZamarBagTag::Activate: TAG WAS DEACTIVATED");
  if (not generated_)
    throw Exception("ZamarBagTag::Activate: TAG NOT GENERATED");

  activated_ = true;
  toDB_activated(reqNode, externalSysResNode);
}

void ZamarBagTag::Deactivate(xmlNodePtr reqNode, xmlNodePtr externalSysResNode)
{
  if (deactivated_)
    throw Exception("ZamarBagTag::Deactivate: TAG ALREADY DEACTIVATED");
  if (not activated_)
    throw Exception("ZamarBagTag::Deactivate: TAG NOT ACTIVATED");
  if (not generated_)
    throw Exception("ZamarBagTag::Deactivate: TAG NOT GENERATED");

  activated_ = false;
  deactivated_ = true;
  toDB_deactivated(reqNode, externalSysResNode);
}

//-----------------------------------------------------------------------------------
// PassengerBaggageTagAdd

static void CheckPieceConceptAllowance(const CheckIn::TSimplePaxItem& pax,
                                       const CheckIn::TSimpleBagItem& bag,
                                       xmlNodePtr reqNode,
                                       xmlNodePtr externalSysResNode)
{
  list< pair<int/*pax_id*/, TRFISCKey> > additionalBaggage;

  if (!bag.pc)
    throw Exception("%s: !bag.pc", __FUNCTION__);
  additionalBaggage.emplace_back(pax.id, bag.pc.get());

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "SELECT sbdo_tags_generated.*, 1 AS amount FROM sbdo_tags_generated, pax "
                "WHERE sbdo_tags_generated.pax_id=pax.pax_id AND pax.grp_id=:grp_id AND "
                "      pax.refuse IS NULL AND deactivated=0 AND "
                "      NOT EXISTS (SELECT * FROM bag_tags WHERE bag_tags.grp_id=:grp_id AND bag_tags.no=sbdo_tags_generated.no)";
  Qry.CreateVariable("grp_id", otInteger, pax.grp_id);
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    int pax_id=Qry.FieldAsInteger("pax_id");
    CheckIn::TSimpleBagItem item;
    item.fromDB(Qry);
    if (!item.pc)
      throw Exception("%s: !item.pc", __FUNCTION__);
    additionalBaggage.emplace_back(pax_id, item.pc.get());
  }

  TCkinGrpIds tckin_grp_ids;
  TPaidRFISCList paidAfter; //� ���� �室��� �� ��ॣ����஢���� � �業���� ��㣨 ��㯯�
                            //(� �.�. ��ॣ����஢����=��⨢�஢���� �����)
                            //(� �.�. ᣥ���஢���� �����=��ન ��� ���ᠦ�஢ ��㯯�)
                            //(� �.�. � �����, ����� ���� �� �������)
  bool httpWasSent=false;
  SirenaExchange::TLastExchangeList SirenaExchangeList;
  try
  {
    if (getSvcPaymentStatus(pax.grp_id,
                            additionalBaggage,
                            reqNode,
                            externalSysResNode,
                            ASTRA::rollback,
                            SirenaExchangeList,
                            tckin_grp_ids,
                            paidAfter,
                            httpWasSent))
    {
      SirenaExchangeList.handle(__FUNCTION__);

      if (paidAfter.becamePaid(pax.grp_id)) //�ࠢ��� � �ᥬ� ��ॣ����஢���묨 � �業���묨 ࠭�� ��㣠�� ��㯯� (� �.�. ��ॣ����஢����=��⨢�஢���� �����)
        throw ZamarException(STR_BAG_TAG_PAID, "Not allowed"); //!!!UserException("MSG.SBDO.SERVICES_BECAME_PAID")
    }
  }
  catch(SvcPaymentStatusNotApplicable)
  {
    throw ZamarException(STR_BAGGAGE_NOT_ALLOWED, "Not allowed");
  }

  if (isDoomedToWait())
    throw FinishProcessXML();
}

void ZamarBaggageTagAdd::fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");
  session_id_ = NodeAsString("sessionId", reqNode);
  if (session_id_.empty())
    throw Exception("Empty <sessionId>");

  tag_.fromXML_add(reqNode);

  const int pax_id = tag_.pax_id_;

  CheckIn::TSimplePaxItem pax_item;
  stringstream ss_pax_id_err;
  ss_pax_id_err << "pax_id='" << pax_id << "'";
  if (not pax_item.getByPaxId(pax_id)) // �᫨ ���ᠦ�� �� ������ (��. ������)
  {
    ss_pax_id_err << " Failed pax_item.getByPaxId";
    throw ZamarException(STR_PAX_NO_MATCH, ss_pax_id_err.str());
  }
  if (not pax_item.allowToBagCheckIn()) // �᫨ ���ᠦ�� �� � ⮬ �����
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
  if (not grp_item.allowToBagCheckIn()) // �஠������஢��� trfer_confirm == true
    throw ZamarException(STR_TRFER_NO_CONFIRM, ss_grp_id_err.str());

  // �᫨ �⠯ �� ��䨪� "ॣ������" �� � ⮬ �����
  if (not TTripStages(grp_item.point_dep).allowToBagCheckIn())
    throw ZamarException(STR_WRONG_CHECKIN_STATUS, "");

  // �㦭� �஠������஢��� �� concept != unknown
  TBagConcept::Enum bag_concept_grp = grp_item.getBagAllowanceType();
  if (bag_concept_grp == TBagConcept::Unknown)
    throw ZamarException(STR_CONCEPT_UNKNOWN, "");
//  if (bag_concept_grp != bag_concept) throw ZamarException(STR_CONCEPT_NO_MATCH, ""); // ???

  const int ALLOWED_MIN = 1;
  const int ALLOWED_MAX = 99;
  if (tag_.bag_.weight < ALLOWED_MIN or tag_.bag_.weight > ALLOWED_MAX)
  {
    stringstream ss;
    ss << "weight='" << tag_.bag_.weight << "' when allowed='" << ALLOWED_MAX << "'";
    throw ZamarException(STR_WEIGHT_EXCEED, ss.str());
  }

  if (tag_.bagConcept() == TBagConcept::Piece)
  {
    try
    {
      CheckPieceConceptAllowance(pax_item, tag_.bag_, reqNode, externalSysResNode);
    }
    catch(AstraLocale::UserException &e)
    {
      LogTrace(TRACE5) << e.what();
      throw ZamarException(STR_BAGGAGE_NOT_ALLOWED, e.what());
    }
  }
  else
    throw Exception("WEIGHT CONCEPT IS NOT ALLOWED WHILE TESTING");

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

void ZamarBaggageTagConfirm::fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");
  session_id_ = NodeAsString("sessionId", reqNode);
  if (session_id_.empty())
    throw Exception("Empty <sessionId>");

  tag_.fromXML(reqNode);

  const int pax_id = tag_.pax_id_;
  CheckIn::TSimplePaxItem pax_item; // ������������� ��� !!!
  stringstream ss_pax_id_err;
  ss_pax_id_err << "pax_id='" << pax_id << "'";
  if (not pax_item.getByPaxId(pax_id)) // �᫨ ���ᠦ�� �� ������ (��. ������)
  {
    ss_pax_id_err << " Failed pax_item.getByPaxId";
    throw ZamarException(STR_PAX_NO_MATCH, ss_pax_id_err.str());
  }
  if (not pax_item.allowToBagCheckIn()) // �᫨ ���ᠦ�� �� � ⮬ �����
    throw ZamarException(STR_PAX_REFUSE, ss_pax_id_err.str());

// CheckPieceConceptAllowance TODO

  tag_.Activate(reqNode, externalSysResNode);
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

void ZamarBaggageTagRevoke::fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");
  session_id_ = NodeAsString("sessionId", reqNode);
  if (session_id_.empty())
    throw Exception("Empty <sessionId>");

  tag_.fromXML(reqNode);
  tag_.Deactivate(reqNode, externalSysResNode);
}

void ZamarBaggageTagRevoke::toXML(xmlNodePtr resNode, ZamarType) const
{
  if (resNode == nullptr)
    return;
  NewTextChild(resNode, "queryType", "PassengerBaggageTagRevoke");
  NewTextChild(resNode, "sessionId", session_id_);
  tag_.toXML(resNode);
}

