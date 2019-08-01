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
#include "edi_utils.h" //только ради isTagAddRequestSBDO и т.п
#include "checkin.h"
#include "astra_callbacks.h"
#include "http_main.h"
#include "web_search.h"

#define NICKNAME "GRISHA"
#include <serverlib/slogger.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;

const string STR_INTERNAL_ERROR = "MSG.SBDO.INTERNAL_ERROR";
const string STR_PAX_NO_MATCH = "MSG.SBDO.PAX_NO_MATCH"; // используется и для PaxCtl
const string STR_INCORRECT_DATA_IN_XML = "MSG.SBDO.INCORRECT_DATA_IN_XML";
const string STR_WEIGHT_CONCEPT_NOT_ALLOWED = "WEIGHT CONCEPT IS NOT ALLOWED WHILE TESTING"; // !!!
const string STR_BAG_TYPE_NOT_ALLOWED = "MSG.SBDO.BAG_TYPE_NOT_ALLOWED";
const string STR_NO_ANSWER_FROM_REMOTE_SYSTEM = "MSG.SBDO.NO_ANSWER_FROM_REMOTE_SYSTEM";
const string STR_BAGGAGE_ADD_NOT_ALLOWED = "MSG.SBDO.BAGGAGE_ADD_NOT_ALLOWED";
const string STR_BAGGAGE_DEL_NOT_ALLOWED = "MSG.SBDO.BAGGAGE_DEL_NOT_ALLOWED";
const string STR_BAG_TAG_NOT_FOUND = "MSG.SBDO.BAG_TAG_NOT_FOUND";
const string STR_TAG_WAS_DEACTIVATED = "MSG.SBDO.TAG_WAS_DEACTIVATED";
const string STR_TAG_ALREADY_ACTIVATED = "MSG.SBDO.TAG_ALREADY_ACTIVATED";
const string STR_TAG_ALREADY_DEACTIVATED = "MSG.SBDO.TAG_ALREADY_DEACTIVATED";
const string STR_WRONG_PAX_STATUS = "MSG.SBDO.WRONG_PAX_STATUS";
const string STR_TRFER_NO_CONFIRM = "MSG.SBDO.TRFER_NO_CONFIRM";
const string STR_WRONG_CHECKIN_STATUS = "MSG.SBDO.WRONG_CHECKIN_STATUS";
const string STR_CONCEPT_UNKNOWN = "MSG.SBDO.CONCEPT_UNKNOWN";
const string STR_WEIGHT_EXCEED = "MSG.SBDO.WEIGHT_EXCEED";
const string STR_SERVICES_BECAME_PAID = "MSG.SBDO.SERVICES_BECAME_PAID"; // в checkin.cc ещё есть эта строка

void ZamarException(const AstraLocale::LexemaData& lexemeData, const string& comment, const string& func)
{
  LogTrace(TRACE5) << "ZAMAR EXCEPTION: lexema_id='" << lexemeData.lexema_id << "' comment='" << comment << "' function='" << func << "'";
  if (lexemeData.lexema_id == STR_INCORRECT_DATA_IN_XML)
  {
    ProgError(STDLOG, "ZAMAR incorrect data in XML: %s", comment.c_str());
    throw AstraLocale::UserException("WRAP.QRY_HANDLER_ERR", AstraLocale::LParams() << AstraLocale::LParam("text", comment));
  }
  else
  {
    throw AstraLocale::UserException(lexemeData.lexema_id, lexemeData.lparams);
  }
}

class FinishProcess {};

void ZamarSetPostProcess()
{
  AstraJxtCallbacks* astra_cb_ptr = dynamic_cast<AstraJxtCallbacks*>(jxtlib::JXTLib::Instance()->GetCallbacks());
  if (nullptr == astra_cb_ptr)
    return;
  astra_cb_ptr->SetPostProcessXMLAnswerCallback(AstraHTTP::ZamarPostProcessXMLAnswer);
}

static void ProcessXML(ZamarDataInterface& data, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode, ZamarType type)
{
  try
  {
    ZamarSetPostProcess(); // для kick
    data.fromXML(reqNode, externalSysResNode, type);
    data.toXML(resNode, type);
  }
  catch (const AstraLocale::UserException&)
  {
    throw;
  }
  catch (const std::exception& e)
  {
    ProgError(STDLOG, "ZAMAR std::exception: %s", e.what());
    throw AstraLocale::UserException("WRAP.QRY_HANDLER_ERR", AstraLocale::LParams() << AstraLocale::LParam("text", e.what()));
  }
  catch (const FinishProcess&)
  {
    if (isDoomedToWait())
      AstraLocale::showError(STR_NO_ANSWER_FROM_REMOTE_SYSTEM);
  }
}

//-----------------------------------------------------------------------------------
// PaxCtl

const char* XML_QUERY_TYPE = "queryType";

void ZamarPaxCtlInterface::PassengerSearchPaxCtl(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  SetProp(resNode, XML_QUERY_TYPE, "PassengerSearchPaxCtl");
  PassengerSearchResult result;
  ProcessXML(result, reqNode, nullptr, resNode, ZamarType::PaxCtl);
}

//-----------------------------------------------------------------------------------
// SBDO

void ZamarSBDOInterface::PassengerSearchSBDO(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  SetProp(resNode, XML_QUERY_TYPE, "PassengerSearchSBDO");
  PassengerSearchResult result;
  ProcessXML(result, reqNode, nullptr, resNode, ZamarType::SBDO);
}

void ZamarSBDOInterface::PassengerBaggageTagAdd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ZamarSBDOInterface::PassengerBaggageTagAdd(reqNode, nullptr, resNode);
}

void ZamarSBDOInterface::PassengerBaggageTagAdd(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  SetProp(resNode, XML_QUERY_TYPE, "PassengerBaggageTagAdd");
  ZamarBaggageTagAdd tag_add;
  ProcessXML(tag_add, reqNode, externalSysResNode, resNode, ZamarType::SBDO);
}

void ZamarSBDOInterface::PassengerBaggageTagConfirm(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ZamarSBDOInterface::PassengerBaggageTagConfirm(reqNode, nullptr, resNode);
}

void ZamarSBDOInterface::PassengerBaggageTagConfirm(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  SetProp(resNode, XML_QUERY_TYPE, "PassengerBaggageTagConfirm");
  ZamarBaggageTagConfirm tag_confirm;
  ProcessXML(tag_confirm, reqNode, externalSysResNode, resNode, ZamarType::SBDO);
}

void ZamarSBDOInterface::PassengerBaggageTagRevoke(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ZamarSBDOInterface::PassengerBaggageTagRevoke(reqNode, nullptr, resNode);
}

void ZamarSBDOInterface::PassengerBaggageTagRevoke(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  SetProp(resNode, XML_QUERY_TYPE, "PassengerBaggageTagRevoke");
  ZamarBaggageTagRevoke tag_revoke;
  ProcessXML(tag_revoke, reqNode, externalSysResNode, resNode, ZamarType::SBDO);
}

//-----------------------------------------------------------------------------------
// PassengerSearch

void PassengerSearchResult::fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType type)
{
  if (reqNode == nullptr)
    ZamarException(STR_INTERNAL_ERROR, "reqNode == nullptr", __func__);

  AstraLocale::OutputLang lang("", {AstraLocale::OutputLang::OnlyTrueIataCodes});

  // sessionId
  sessionId = NodeAsString("sessionId", reqNode);
  if (sessionId.empty())
    ZamarException(STR_INCORRECT_DATA_IN_XML, "Empty <sessionId>", __func__);

  // bcbp or paxId
  bool isBoardingPass = false;
  int reg_no = NoExists;
  if (nullptr != GetNode("bcbp", reqNode))
  {
    string bcbp = NodeAsString( "bcbp", reqNode);
    if (bcbp.empty())
      ZamarException(STR_INCORRECT_DATA_IN_XML, "Empty <bcbp>", __func__);
    SearchPaxByScanData(bcbp, point_id, reg_no, pax_id, isBoardingPass);
    if (point_id==NoExists || reg_no==NoExists || pax_id==NoExists || !isBoardingPass)
    {
      LogTrace(TRACE5) << __FUNCTION__
                       << ": found_point_id=" << point_id
                       << ", reg_no=" << reg_no
                       << ", found_pax_id=" << pax_id
                       << boolalpha << ", isBoardingPass=" << isBoardingPass;
      if (!isBoardingPass)
        throw AstraLocale::UserException("MSG.WRONG_DATA_RECEIVED");
      if (point_id==NoExists)
        ZamarException(STR_PAX_NO_MATCH, "", __func__);
      if (pax_id==NoExists)
        ZamarException(STR_PAX_NO_MATCH, "", __func__);
      throw AstraLocale::UserException("MSG.PASSENGER.NOT_CHECKIN_WITH_REG_NO");
    }
  }
  else
  {
    if (nullptr == GetNode("passengerId", reqNode))
      ZamarException(STR_INCORRECT_DATA_IN_XML, "Must provide <bcbp> or <passengerId>", __func__);
    pax_id = NodeAsInteger("passengerId", reqNode);
  }

  // pax
  if (not pax_item.getByPaxId(pax_id))
  {
    stringstream ss;
    ss << "pax_id='" << pax_id << "'";
    ZamarException(STR_PAX_NO_MATCH, ss.str(), __func__);
  }

  // grp
  grp_id = pax_item.grp_id;
  bool get_grp_result = false;
  if (type == ZamarType::PaxCtl)
    get_grp_result = grp_item.getByGrpId(grp_id);
  else if (type == ZamarType::SBDO)
    get_grp_result = grp_item.getByGrpIdWithBagConcepts(grp_id);
  if (not get_grp_result)
  {
    stringstream ss;
    if (type == ZamarType::PaxCtl)
      ss << "Failed grp_item.getByGrpId ";
    else if (type == ZamarType::SBDO)
      ss << "Failed grp_item.getByGrpIdWithBagConcepts ";
    else
      ss << "Unknown grp_id error ";
    ss << grp_id;
    ZamarException(STR_INTERNAL_ERROR, ss.str(), __func__);
  }

  // point
  if (NoExists == point_id)
    point_id = grp_item.point_dep;
  if (not trip_info.getByPointId(point_id))
  {
    stringstream ss;
    ss << "Failed trip_info.getByPointId " << point_id;
    ZamarException(STR_INTERNAL_ERROR, ss.str(), __func__);
  }

  doc_exists = CheckIn::LoadPaxDoc(pax_id, doc);
  doco_exists = CheckIn::LoadPaxDoco(pax_id, doco);

  mkt_flt.getByPaxId(pax_id);
  if (mkt_flt.empty())
  {
    stringstream ss;
    ss << "Failed mkt_flt.getByPaxId " << pax_id;
    ZamarException(STR_INTERNAL_ERROR, ss.str(), __func__);
  }

  // flightStatus
  flightCheckinStage = TTripStages(point_id).getStage( stCheckIn );
  // pnr
  pnrs.getByPaxIdFast(pax_id);

  // baggageTags
  GetTagsByPool(grp_id, pax_item.bag_pool_num, bagTagsExtended, false);
  if (type == ZamarType::SBDO)
  {
    // generated
    TQuery Qry( &OraSession );
    Qry.SQLText="SELECT no, weight FROM sbdo_tags_generated WHERE pax_id = :pax_id AND deactivated = 0";
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next())
    {
      const double no = Qry.FieldAsFloat("no");
      // проверяем что не активирована
      if (find_if(bagTagsExtended.cbegin(), bagTagsExtended.cend(),
                  [no](auto & it){ return no == it.first.numeric_part; })
          == bagTagsExtended.cend())
        bagTagsGenerated.emplace_back(no, Qry.FieldAsInteger("weight"));
    }
  }
}

//-----------------------------------------------------------------------------------

static void BagTagsToZamarXML(xmlNodePtr resNode,
                              const std::multimap<TBagTagNumber, CheckIn::TBagItem>& tags_ext,
                              const std::list<std::pair<double, int>>& tags_gen)
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
    SetProp(tagNode, "activated", 1);
  }
  for (const auto & tag : tags_gen)
  {
    xmlNodePtr tagNode = NewTextChild(tagsNode, "baggageTag");
    TBagTagNumber num("", tag.first);
    SetProp(tagNode, "id", num.str());
    SetProp(tagNode, "weight", tag.second);
    SetProp(tagNode, "unit", "KG");
    SetProp(tagNode, "activated", 0);
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

// TODO сделать функцию, добавляющую в список язык запроса (при отсутствии такового в списке)
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

  // allowBoarding -- PaxCtl
  if (type == ZamarType::PaxCtl)
    NewTextChild(resNode, "allowBoarding", pax_item.allowToBoarding()? 1: 0);

  // paxId
  NewTextChild(resNode, "passengerId", pax_id);
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
    NewTextChild(resNode, "cabinClass", ElemIdToPrefferedElem(etClass, grp_item.cl/*pax_item.getCabinClass()*/, efmtCodeNative, lang.get()));
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
    xmlNodePtr allowanceNode = NewTextChild(resNode, "baggageAllowance");
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

  // baggageTags
  BagTagsToZamarXML(resNode, bagTagsExtended, bagTagsGenerated);
}

//-----------------------------------------------------------------------------------
// ZamarBagTag

std::string ZamarBagTag::tagNumber() const
{
  if (!tagNumber_) return "NUMBER NOT DEFINED";
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
  // вызов функции генерации для существующей бирки - это нештатная ситуация
  if (generated_)
    ZamarException(STR_INTERNAL_ERROR, tagNumber() + " tag number already generated", __func__);
  if (activated_)
    ZamarException(STR_INTERNAL_ERROR, tagNumber() + " tag number already activated", __func__);
  if (deactivated_)
    ZamarException(STR_INTERNAL_ERROR, tagNumber() + " tag number already deactivated", __func__);

  TGeneratedTags generated;
  generated.generate(grp_id, 1);
  if (generated.tags().size()!=1)
    ZamarException(STR_INTERNAL_ERROR, "generated.tags().size()!=1", __func__);
  tagNumber_=*(generated.tags().begin());
  generated_ = true;

  toDB_generated();
}

void ZamarBagTag::fromXML_add(xmlNodePtr reqNode)
{
  if (reqNode == nullptr)
    ZamarException(STR_INTERNAL_ERROR, "reqNode == nullptr", __func__);

  pax_id_ = NodeAsInteger("passengerId", reqNode);
  xmlNodePtr tagNode = NodeAsNode("baggageTag", reqNode);
  bag_.amount = 1;
  bag_.weight = NodeAsInteger("@weight", tagNode);
  string unit_xml = NodeAsString("@unit", tagNode);
  if (unit_xml != "KG")
  {
    stringstream ss;
    ss << "@unit must be 'KG' instead of '" << unit_xml << "'";
    ZamarException(STR_INCORRECT_DATA_IN_XML, ss.str(), __func__);
  }
  xmlNodePtr typeNode = NodeAsNode("baggageType", reqNode);

  string concept = NodeAsString("@type", typeNode);

  TElemFmt airline_fmt = efmtUnknown;
  string airline_xml = NodeAsString("@airline", typeNode);
  string airline = ElemToElemId(etAirline, airline_xml, airline_fmt);
  if (airline.empty())
  {
    stringstream ss;
    ss << "Unknown @airline '" << airline_xml << "'";
    ZamarException(STR_INCORRECT_DATA_IN_XML, ss.str(), __func__);
  }

  if (concept == "piece")
  {
    bag_.pc=boost::in_place();
    TRFISCListKey& key=bag_.pc.get();

    key.airline = airline;
    key.RFISC = NodeAsString("@rfisc", typeNode);
    if (bag_.pc.get().RFISC.empty()) // TODO почему не key.RFISC
      ZamarException(STR_INCORRECT_DATA_IN_XML, "Empty @rfisc", __func__);
    string service_type_xml = NodeAsString("@serviceType", typeNode);
    key.service_type=ServiceTypes().decode(service_type_xml);
    if (key.service_type==TServiceType::Unknown)
    {
      stringstream ss;
      ss << "Unknown @serviceType '" << service_type_xml << "'";
      ZamarException(STR_INCORRECT_DATA_IN_XML, ss.str(), __func__);
    }
  }
  else if (concept == "weight")
  {
    // !!! НА ВРЕМЯ ТЕСТИРОВАНИЯ !!!
    ZamarException(STR_WEIGHT_CONCEPT_NOT_ALLOWED, "", __func__);

    bag_.wt=boost::in_place();
    TBagTypeListKey& key=bag_.wt.get();

    key.airline = airline;
    key.bag_type = NodeAsString( "@bag_type", typeNode);
    if (key.bag_type.empty())
      ZamarException(STR_INCORRECT_DATA_IN_XML, "Empty @bag_type", __func__);
  }
  else
  {
    stringstream ss;
    ss << "Unrecognized baggageType @type '" << concept << "' (must be 'piece' or 'weight')";
    ZamarException(STR_INCORRECT_DATA_IN_XML, ss.str(), __func__);
  }

  SetListId();
}

void ZamarBagTag::fromXML(xmlNodePtr reqNode, ZamarActionType actionType)
{
  if (reqNode == nullptr)
    ZamarException(STR_INTERNAL_ERROR, "reqNode == nullptr", __func__);

  int pax_id = NodeAsInteger("passengerId", reqNode);
  tagNumber_=boost::in_place("", NodeAsFloat("tag", reqNode));

  fromDB(actionType);

  if (pax_id != pax_id_)
    ZamarException(STR_INTERNAL_ERROR, "Passenger ID not match", __func__);
}

void ZamarBagTag::toXML(xmlNodePtr resNode) const
{
  if (resNode == nullptr)
    return;
  if (not generated_)
    ZamarException(STR_INTERNAL_ERROR, "Tag not generated", __func__);
  NewTextChild(resNode, "passengerId", pax_id_);
  NewTextChild(resNode, "tag", tagNumber());
}

void ZamarBagTag::SetListId()
{
  // TODO TRFISCListKey::fromSbdoXML и  TBagTypeListKey::fromSbdoXML
  try
  {
    if (bag_.pc)
    {
      TRFISCKey& key=bag_.pc.get();
      key.getListItemByPaxId(pax_id_, 0, TServiceCategory::BaggageInHold, __FUNCTION__);
      if (!key.list_item || !key.list_item.get().isBaggageInHold())
        ZamarException(STR_BAG_TYPE_NOT_ALLOWED, "", __func__);
    }

    if (bag_.wt)
    {
      TBagTypeKey& key=bag_.wt.get();
      key.getListItemByPaxId(pax_id_, 0, TServiceCategory::BaggageInHold, __FUNCTION__);
      if (!key.list_item || !key.list_item.get().isBaggageInHold())
        ZamarException(STR_BAG_TYPE_NOT_ALLOWED, "", __func__);
    }
  }
  catch(const EConvertError &e)
  {
    LogTrace(TRACE5) << e.what();
    ZamarException(STR_BAG_TYPE_NOT_ALLOWED, "EConvertError", __func__);
  }
}

void ZamarBagTag::toDB_generated() const
{
  if (not generated_)
    ZamarException(STR_INTERNAL_ERROR, "Tag not generated", __func__);
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
  if (bag_.pc)
    bag_.pc.get().toDB(Qry);
  else if (bag_.wt)
    bag_.wt.get().toDB(Qry);

  LogTrace(TRACE5) << __func__
                   << ": no=" << tagNumber()
                   << ", pax_id=" << pax_id_
                   << ", weight=" << bag_.weight
                   << ", "
                   << (bag_.pc?bag_.pc.get().traceStr():"")
                   << (bag_.wt?bag_.wt.get().traceStr():"");
  Qry.Execute();
}

void ZamarBagTag::toDB_activated(xmlNodePtr reqNode, xmlNodePtr externalSysResNode) const
{
  if (not generated_)
    ZamarException(STR_INTERNAL_ERROR, "Tag not generated", __func__);

  if (!tagNumber_)
    ZamarException(STR_INTERNAL_ERROR, "No tag number", __func__);

  RemoveNode(GetNode("TCkinSavePax", reqNode));
  xmlNodePtr emulReqNode=NewTextChild(reqNode, "TCkinSavePax");

  createEmulDocForSBDO(pax_id_,
                       tagNumber_.get(),
                       bag_,
                       emulReqNode);

  CheckInInterface::SavePax(emulReqNode, externalSysResNode, nullptr);

  if (isDoomedToWait())
    throw FinishProcess();
}

void ZamarBagTag::toDB_deactivated(xmlNodePtr reqNode, xmlNodePtr externalSysResNode) const
{
  if (not generated_)
    ZamarException(STR_INTERNAL_ERROR, "Tag not generated", __func__);

  if (!tagNumber_)
    ZamarException(STR_INTERNAL_ERROR, "No tag number", __func__);

  if (activated_)
  {
    RemoveNode(GetNode("TCkinSavePax", reqNode));
    xmlNodePtr emulReqNode=NewTextChild(reqNode, "TCkinSavePax");

    // удаляем из активированных=зарегистрированных
    createEmulDocForSBDO(pax_id_,
                         tagNumber_.get(),
                         boost::none,
                         emulReqNode);

    CheckInInterface::SavePax(emulReqNode, externalSysResNode, nullptr);

    if (isDoomedToWait())
      throw FinishProcess();
  }

  // выставляем статус деактивирована в таблице сгенерированных
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "UPDATE sbdo_tags_generated SET deactivated = 1 WHERE no = :no";
  tagNumberToDB(Qry);
  Qry.Execute();
}

void ZamarBagTag::fromDB(ZamarActionType actionType)
{
  TQuery Qry(&OraSession);
  //сначала всегда ищем в сгенерированных
  Qry.Clear();
  Qry.SQLText =
      "SELECT sbdo_tags_generated.*, 1 AS amount FROM sbdo_tags_generated WHERE no = :no";
  tagNumberToDB(Qry);
  Qry.Execute();
  if (Qry.Eof)
    ZamarException(STR_BAG_TAG_NOT_FOUND, tagNumber(), __func__);

  // сгенерирована
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
    {
      //по какой-то причине бирка есть в группе, но привязана к другому пассажиру (запросто могли перепривязать с терминала)
      //ну или не привязана вообще, но это странно
      if (actionType == ZamarActionType::CONFIRM)
        ZamarException(STR_BAGGAGE_ADD_NOT_ALLOWED, "", __func__);
      if (actionType == ZamarActionType::REVOKE)
        ZamarException(STR_BAGGAGE_DEL_NOT_ALLOWED, "", __func__);
    }
  }
}

void ZamarBagTag::Activate(xmlNodePtr reqNode, xmlNodePtr externalSysResNode)
{
  if (activated_)
    ZamarException(STR_TAG_ALREADY_ACTIVATED, tagNumber(), __func__);
  if (deactivated_)
    ZamarException(STR_TAG_WAS_DEACTIVATED, tagNumber(), __func__);
  if (not generated_)
    ZamarException(STR_INTERNAL_ERROR, "Tag not generated", __func__);

  toDB_activated(reqNode, externalSysResNode);
  activated_ = true;
}

void ZamarBagTag::Deactivate(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, bool force)
{
  if (deactivated_)
    ZamarException(STR_TAG_ALREADY_DEACTIVATED, tagNumber(), __func__);
  if (not generated_)
    ZamarException(STR_INTERNAL_ERROR, "Tag not generated", __func__);

  if (activated_ and not force)
    ZamarException(STR_BAGGAGE_DEL_NOT_ALLOWED, tagNumber(), __func__);

  toDB_deactivated(reqNode, externalSysResNode);
  activated_ = false;
  deactivated_ = true;
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
    ZamarException(STR_INTERNAL_ERROR, "!bag.pc", __func__);
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
      ZamarException(STR_INTERNAL_ERROR, "!item.pc", __func__);
    additionalBaggage.emplace_back(pax_id, item.pc.get());
  }

  TCkinGrpIds tckin_grp_ids;
  TPaidRFISCList paidAfter; //сюда будут входить все зарегистрированные и оцененные услуги группы
                            //(в т.ч. зарегистрированный=активированный багаж)
                            //(в т.ч. сгенерированный багаж=бирки всех пассажиров группы)
                            //(в т.ч. и багаж, который идет на генерацию)
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

      //сравним со всеми зарегистрированными и оцененными раньше услугами группы (в т.ч. зарегистрированный=активированный багаж)
      if (paidAfter.becamePaid(pax.grp_id))
        ZamarException(STR_SERVICES_BECAME_PAID, "", __func__);
    }
  }
  catch(const SvcPaymentStatusNotApplicable&)
  {
    ZamarException(STR_BAGGAGE_ADD_NOT_ALLOWED, "", __func__);
  }

  if (isDoomedToWait())
    throw FinishProcess();
}

void ZamarGetPax(const int pax_id, CheckIn::TSimplePaxItem& pax_item)
{
  stringstream ss_pax_id;
  ss_pax_id << "pax_id='" << pax_id << "'";
  if (not pax_item.getByPaxId(pax_id)) // Если пассажир не найден (ид. неверный)
    ZamarException(STR_PAX_NO_MATCH, ss_pax_id.str(), __func__);
  if (not pax_item.allowToBagCheckIn()) // Если пассажир не в том статусе
    ZamarException(STR_WRONG_PAX_STATUS, ss_pax_id.str(), __func__);
}

void ZamarGetGrp(const int grp_id, CheckIn::TPaxGrpItem& grp_item)
{
  stringstream ss_grp_id;
  ss_grp_id << "grp_id='" << grp_id << "'";
  if (not grp_item.getByGrpIdWithBagConcepts(grp_id))
    ZamarException(STR_INTERNAL_ERROR, ss_grp_id.str(), __func__);
  if (not grp_item.allowToBagCheckIn()) // проанализировать trfer_confirm == true
    ZamarException(STR_TRFER_NO_CONFIRM, ss_grp_id.str(), __func__);
}

void ZamarAllowToBagCheckIn(int point_id)
{
  if (not TTripStages(point_id).allowToBagCheckIn()) // Если этап тех графика "регистрация" не в том статусе
    ZamarException(STR_WRONG_CHECKIN_STATUS, "", __func__);
}

void ZamarBaggageTagAdd::fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType)
{
  if (reqNode == nullptr)
    ZamarException(STR_INTERNAL_ERROR, "reqNode == nullptr", __func__);

  session_id_ = NodeAsString("sessionId", reqNode);
  if (session_id_.empty())
    ZamarException(STR_INCORRECT_DATA_IN_XML, "Empty <sessionId>", __func__);

  tag_.fromXML_add(reqNode);

  CheckIn::TSimplePaxItem pax_item;
  ZamarGetPax(tag_.pax_id_, pax_item); // пассажир зарегистрирован или посажен

  CheckIn::TPaxGrpItem grp_item;
  ZamarGetGrp(pax_item.grp_id, grp_item); // подтвержден маршрут багажа

  ZamarAllowToBagCheckIn(grp_item.point_dep); // открыта регистрация в а/п

  // Нужно проанализировать что concept != unknown
  TBagConcept::Enum bag_concept_grp = grp_item.getBagAllowanceType();
  if (bag_concept_grp == TBagConcept::Unknown)
    ZamarException(STR_CONCEPT_UNKNOWN, "", __func__);

  const int ALLOWED_MIN = 1;
  const int ALLOWED_MAX = 99;
  if (tag_.bag_.weight < ALLOWED_MIN or tag_.bag_.weight > ALLOWED_MAX)
  {
    stringstream ss;
    ss << "weight='" << tag_.bag_.weight << "'";
    ZamarException(STR_WEIGHT_EXCEED, ss.str(), __func__);
  }

  if (tag_.bagConcept() == TBagConcept::Piece)
  {
    CheckPieceConceptAllowance(pax_item, tag_.bag_, reqNode, externalSysResNode);
  }
  else
  {
    // !!! НА ВРЕМЯ ТЕСТИРОВАНИЯ !!!
    ZamarException(STR_WEIGHT_CONCEPT_NOT_ALLOWED, "", __func__);
  }

  tag_.Generate(pax_item.grp_id);
}

void ZamarBaggageTagAdd::toXML(xmlNodePtr resNode, ZamarType) const
{
  if (resNode == nullptr)
    return;
  NewTextChild(resNode, "sessionId", session_id_);
  tag_.toXML(resNode);
}

//-----------------------------------------------------------------------------------
// PassengerBaggageTagConfirm

void ZamarBaggageTagConfirm::fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType)
{
  if (reqNode == nullptr)
    ZamarException(STR_INTERNAL_ERROR, "reqNode == nullptr", __func__);

  session_id_ = NodeAsString("sessionId", reqNode);
  if (session_id_.empty())
    ZamarException(STR_INCORRECT_DATA_IN_XML, "Empty <sessionId>", __func__);

  tag_.fromXML(reqNode, ZamarActionType::CONFIRM);

  CheckIn::TSimplePaxItem pax_item;
  ZamarGetPax(tag_.pax_id_, pax_item); // пассажир зарегистрирован или посажен

  // --- добавлены проверки
  CheckIn::TPaxGrpItem grp_item;
  ZamarGetGrp(pax_item.grp_id, grp_item); // подтвержден маршрут багажа
  ZamarAllowToBagCheckIn(grp_item.point_dep); // открыта регистрация в а/п
  // --- добавлены проверки

  tag_.Activate(reqNode, externalSysResNode);
}

void ZamarBaggageTagConfirm::toXML(xmlNodePtr resNode, ZamarType) const
{
  if (resNode == nullptr)
    return;
  NewTextChild(resNode, "sessionId", session_id_);
  tag_.toXML(resNode);
}

//-----------------------------------------------------------------------------------
// PassengerBaggageTagRevoke

void ZamarBaggageTagRevoke::fromXML(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, ZamarType)
{
  if (reqNode == nullptr)
    ZamarException(STR_INTERNAL_ERROR, "reqNode == nullptr", __func__);

  session_id_ = NodeAsString("sessionId", reqNode);
  if (session_id_.empty())
    ZamarException(STR_INCORRECT_DATA_IN_XML, "Empty <sessionId>", __func__);

  bool force = static_cast<bool>(GetNode("force", reqNode));

  tag_.fromXML(reqNode, ZamarActionType::REVOKE);

  // --- добавлены проверки
  int pax_id = tag_.pax_id_;
  stringstream ss_pax_id;
  ss_pax_id << "pax_id='" << pax_id << "'";
  CheckIn::TSimplePaxItem pax_item;
  if (not pax_item.getByPaxId(pax_id)) // Если пассажир не найден (ид. неверный)
    ZamarException(STR_PAX_NO_MATCH, ss_pax_id.str(), __func__);
  // allowToBagRevoke всегда возвращает true так что пока используем STR_WRONG_PAX_STATUS
  if (not pax_item.allowToBagRevoke()) // зарегистрирован, посажен, разрегистрирован
    ZamarException(STR_WRONG_PAX_STATUS, ss_pax_id.str(), __func__);

  CheckIn::TPaxGrpItem grp_item;
  ZamarGetGrp(pax_item.grp_id, grp_item); // подтвержден маршрут багажа
  ZamarAllowToBagCheckIn(grp_item.point_dep); // открыта регистрация в а/п
  // --- добавлены проверки

  tag_.Deactivate(reqNode, externalSysResNode, force);
}

void ZamarBaggageTagRevoke::toXML(xmlNodePtr resNode, ZamarType) const
{
  if (resNode == nullptr)
    return;
  NewTextChild(resNode, "sessionId", session_id_);
  tag_.toXML(resNode);
}

