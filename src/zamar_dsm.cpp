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

#define NICKNAME "GRISHA"
#include <serverlib/slogger.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace EXCEPTIONS;

PassengerSearchResult& PassengerSearchResult::fromXML(xmlNodePtr reqNode)
{
  if (reqNode == nullptr)
    throw Exception("reqNode == nullptr");
  
  // sessionId
  sessionId = NodeAsString( "sessionId", reqNode);
  if (sessionId.empty())
    throw Exception("Empty <sessionId>");
  // bcbp
  string bcbp = NodeAsString( "bcbp", reqNode);
  if (bcbp.empty())
    throw Exception("Empty <bcbp>");
  
  AstraWeb::GetBPPaxFromScanCode(bcbp, bppax); // throws
  point_id = bppax.point_dep;
  int grp_id = bppax.grp_id;
  int pax_id = bppax.pax_id;
  
  if (not trip_info.getByPointId(point_id))
    throw Exception("Failed trip_info.getByPointId %d", point_id);
  if (not grp_item.getByGrpId(grp_id))
    throw Exception("Failed grp_item.getByGrpId %d", grp_id);
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
  pnr.getByPaxIdFast(pax_id);
  // baggageTags
  GetTagsByPool(grp_id, pax_item.bag_pool_num, baggageTags, false);
  
  return *this;
}

const PassengerSearchResult& PassengerSearchResult::toXML(xmlNodePtr resNode) const
{
  if (resNode == nullptr) return *this;
  
  AstraLocale::OutputLang lang("", {AstraLocale::OutputLang::OnlyTrueIataCodes});
  // lang
  SetProp(resNode, "lang", lang.get());
  // sessionId
  NewTextChild(resNode, "sessionId", sessionId);
  // airline
  NewTextChild(resNode, "airline", airlineToPrefferedCode(trip_info.airline, lang));
  // flightCode
  NewTextChild(resNode, "flightCode", trip_info.flight_number());
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
  ostringstream ss_gates;
  string separator_gates;
  for (const auto& gate : gates)
  {
    ss_gates << separator_gates << gate;
    separator_gates = " ";
  }
  NewTextChild(resNode, "gate", ss_gates.str());
  // codeshare
  ostringstream ss_mkt;
  ss_mkt << airlineToPrefferedCode(mkt_flt.airline, lang)
         << "/"
         << mkt_flt.flight_number(lang);
  NewTextChild(resNode, "codeshare", ss_mkt.str());
  // allowBoarding
  NewTextChild(resNode, "allowBoarding", pax_item.allowToBoarding()? 1: 0);
  // passengerId
  NewTextChild(resNode, "passengerId", bppax.pax_id);
  // sequence
  NewTextChild(resNode, "sequence", pax_item.reg_no);
  // document
  if (doc_exists)
    doc.toWebXML(resNode, lang);
  else
    NewTextChild(resNode, "document");
  // doco
  if (doco_exists)
    doco.toWebXML(resNode, lang);
  else
    NewTextChild(resNode, "doco");
  // group
  NewTextChild(resNode, "group", "");
  // pnr
  NewTextChild(resNode, "pnr", pnr.str(TPnrAddrInfo::AddrAndAirline, lang));
  // ticket
  NewTextChild(resNode, "ticket", pax_item.tkn.no_str());
  // type
  NewTextChild(resNode, "type",
      ElemIdToPrefferedElem(etPersType, EncodePerson(pax_item.pers_type), efmtCodeNative, lang.get()));
  // status
  NewTextChild(resNode, "status", pax_item.checkInStatus());
  // baggageTags // ��筨�� � ����� ���஢���� !!!
  set<string> flatten_tags;
  FlattenBagTags(baggageTags, flatten_tags);
  ostringstream ss_tags;
  string separator_tags;
  for (const auto& tag : flatten_tags)
  {
    ss_tags << separator_tags << tag;
    separator_tags = ",";
  }
  NewTextChild(resNode, "baggageTags", ss_tags.str());
  
  return *this;
}

void PassengerSearchResult::errorXML(xmlNodePtr resNode, const std::string& msg )
{
  if (resNode == nullptr) return;
  NewTextChild(resNode, "error", msg);
}

void ZamarDSMInterface::PassengerSearch(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{ 
//  NewTextChild( resNode, "sessionId", string(NodeAsString( "sessionId", reqNode, "empty sessionid" )) + ", Hello world!!!" );
  PassengerSearchResult result;
  try
  {
    result.fromXML(reqNode);
  }
  catch (Exception E)
  {
    PassengerSearchResult::errorXML(resNode, E.what());
    return;
  }
  result.toXML(resNode);
}
