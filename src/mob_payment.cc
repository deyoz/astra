#include "mob_payment.h"
#include "astra_elem_utils.h"
#include "sopp.h"
#include "tripinfo.h"
#include "rfisc_sirena.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

namespace MobilePayment
{

const string dateFmt="dd.mm.yyyy";
const string dateTimeFmt="dd.mm.yyyy hh:nn";

static int TIMEOUT()       //миллисекунды
{
  static boost::optional<int> value;
  if (!value)
    value=getTCLParam("MOB_PAYMENT_SEARCH_TIMEOUT",1,10000,1000);
  return value.get();
}

static std::string getParam(const std::string& name, xmlNodePtr node)
{
  return NodeAsString(name.c_str(), node, "");
}

void Response::errorToXML(const std::exception& e, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (reqNode==nullptr || resNode==nullptr) throw;

  xmlUnlinkNode(resNode->children);
  xmlFreeNode(resNode->children);

  xmlNodePtr errorNode=NewTextChild(NewTextChild(resNode, (const char*)reqNode->name), "error");
  SetProp(errorNode, "code", 0);
  SetProp(errorNode, "message", e.what());
}

SearchPassengersRequest::Depth& SearchPassengersRequest::Depth::fromXML(xmlNodePtr node)
{
  departure=elemIdFromXML(etAirp, "departure", node, cfErrorIfEmpty);

  if (!getParam("search_depth", node).empty())
  {
    hours=NodeAsInteger("search_depth", node);
    if (hours<1 || hours>48)
      throw Exception("Wrong <search_depth>");
  }

  string document_no=upperc(getParam("document_no", node));
  if (!document_no.empty())
  {
    doc=boost::in_place();
    doc.get().no=document_no;
  }

  string lastname=upperc(getParam("lastname", node));
  if (!lastname.empty())
  {
    pax=boost::in_place();
    pax.get().surname=lastname;
  }

  if (hours!=ASTRA::NoExists)
  {
    filter.min_scd_out=NowUTC();
    filter.max_scd_out=filter.min_scd_out+hours/24.0;
    filter.scdOutIsLocal=false;
  }
  filter.airp=departure;

  return *this;
}

SearchPassengersRequest::Segment& SearchPassengersRequest::Segment::fromXML(xmlNodePtr node)
{
  airp=elemIdFromXML(etAirp, "departure", node, cfErrorIfEmpty);

  if (!getParam("carrier", node).empty())
    airline=elemIdFromXML(etAirline, "carrier", node, cfErrorIfEmpty);
  if (!getParam("flight_no", node).empty())
  {
    auto flightNumber=flightNumberFromXML("flight_no", node, cfErrorIfEmpty);
    if (flightNumber)
    {
      flt_no=flightNumber.get().first;
      suffix=flightNumber.get().second;
    }
  }
  if (!getParam("departure_date_scd", node).empty())
    departure_date_scd=dateFromXML("departure_date_scd", node, dateFmt, cfErrorIfEmpty);
  if (!getParam("destination", node).empty())
    destination=elemIdFromXML(etAirp, "destination", node, cfErrorIfEmpty);

  filter=FlightFilter(*this);
  if (departure_date_scd!=ASTRA::NoExists)
  {
    filter.min_scd_out=departure_date_scd;
    filter.max_scd_out=departure_date_scd+1.0;
    filter.scdOutIsLocal=true;
  }
  filter.airp_arv=destination;

  return *this;
}

SearchPassengersRequest::Barcode& SearchPassengersRequest::Barcode::fromXML(xmlNodePtr node)
{
  string::operator = (NodeAsString("barcode", node));
  try
  {
    filter.set(*this);
  }
  catch(const EConvertError& e)
  {
    throw Exception("Wrong <barcode>: %s", e.what());
  }

  return *this;
}

SearchPassengersRequest& SearchPassengersRequest::fromXML(xmlNodePtr node)
{
  departure=elemIdFromXML(etAirp, "departure", node, cfErrorIfEmpty);

  string recloc=upperc(getParam("recloc", node));
  if (!recloc.empty())
  {
    pnr=boost::in_place();
    pnr.get().addr=recloc;
  }

  string ticket_no=upperc(getParam("ticket_no", node));
  if (!ticket_no.empty())
  {
    tkn=boost::in_place();
    tkn.get().no=ticket_no;
  }

  if (!getParam("carrier", node).empty() ||
      !getParam("flight_no", node).empty() ||
      !getParam("departure_date_scd", node).empty() ||
      !getParam("destination", node).empty())
  {
    oper=boost::in_place();
    oper.get().fromXML(node);
  }

  if (!getParam("search_depth", node).empty() ||
      !getParam("document_no", node).empty() ||
      !getParam("lastname", node).empty())
  {
    depth=boost::in_place();
    depth.get().fromXML(node);
  }

  if (!getParam("barcode", node).empty())
  {
    barcode=boost::in_place();
    barcode.get().fromXML(node);
  }

  return *this;
}

SearchFlightsRequest& SearchFlightsRequest::fromXML(xmlNodePtr node)
{
  departure=elemIdFromXML(etAirp, "departure", node, cfErrorIfEmpty);
  depth_hours=NodeAsInteger("search_depth", node);
  if (depth_hours<1 || depth_hours>48)
    throw Exception("Wrong <search_depth>");
  departure_datetime=ASTRA::NoExists;
  if (!getParam("departure_datetime", node).empty())
    departure_datetime=dateFromXML("departure_datetime", node, dateTimeFmt, cfErrorIfEmpty);
  if (!getParam("carrier", node).empty())
    oper_carrier=elemIdFromXML(etAirline, "carrier", node, cfErrorIfEmpty);
  return *this;
}

void Stages::set(int point_id)
{
  TTripStages tripStages(point_id);
  checkin =    tripStages.getStage( stCheckIn )    ==    sOpenCheckIn? Segment::Open:Segment::Close;
  webCheckin = tripStages.getStage( stWEBCheckIn ) == sOpenWEBCheckIn? Segment::Open:Segment::Close;
  boarding =   tripStages.getStage( stBoarding )   ==   sOpenBoarding? Segment::Open:Segment::Close;
}

const Stages& Stages::toXML(xmlNodePtr node) const
{
  if (node==nullptr) return *this;

  SetProp(node, "check_in_status", statusStr(checkin));
  SetProp(node, "web_check_in_status", statusStr(webCheckin));
  SetProp(node, "boarding_status", statusStr(boarding));

  return *this;
}

std::string Stages::statusStr(const StageStatus& status)
{
  return status==Open?"open":"close";
}

const Segment& Segment::toXML(xmlNodePtr node,
                              const AstraLocale::OutputLang& lang) const
{
  if (node==nullptr) return *this;

  SetProp(node, "carrier", airlineToPrefferedCode(departure.airline, lang));
  SetProp(node, "flight_no", departure.flight_number(lang));
  SetProp(node, "departure", airpToPrefferedCode(departure.airp, lang));
  SetProp(node, "destination", airpToPrefferedCode(arrival.airp, lang));
  SetProp(node, "departure_time", departure.scd_out_local(dateTimeFmt));
  SetProp(node, "arrival_time", arrival.scd_in_local(dateTimeFmt));
  Stages::toXML(node);

  return *this;
}

const Passenger& Passenger::toXML(xmlNodePtr node,
                                  const Segment& segment,
                                  const AstraLocale::OutputLang& lang) const
{
  if (node==nullptr) return *this;

  SetProp(node, "lastname", surname);
  SetProp(node, "name", name);
  SetProp(node, "date_of_birth", doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, dateFmt):"");
  SetProp(node, "category", categoryStr());
  SetProp(node, "gender", genderStr());
  SetProp(node, "document_no", doc.no);
  SetProp(node, "pax_id", id);
  SetProp(node, "status", statusStr());

  xmlNodePtr segNode=NewTextChild(node, "flight");
  segment.toXML(segNode, lang);

  NewTextChild(segNode, "ticket", tkn.no);

  pnrAddrs.toSirenaXML(NewTextChild(segNode, "reclocs"), lang);

  return *this;
}

std::string Passenger::categoryStr() const
{
  switch(pers_type)
  {
    case ASTRA::adult: return "adult";
    case ASTRA::child: return "child";
    case ASTRA::baby:  return "infant";
    default:           return "";
  }
}

std::string Passenger::genderStr() const
{
  switch (CheckIn::is_female(doc.gender, name))
  {
    case ASTRA::NoExists: return "unknown";
    case 0:               return "male";
    default:              return "female";
  };
}

std::string Passenger::statusStr() const
{
  //но лучше использовать TSimplePaxItem::checkInStatus()
  if (id==ASTRA::NoExists) return "unknown";
  if (!refuse.empty()) return "refused";
  return grp_id!=ASTRA::NoExists?"checked":"not_checked";
}

const Segment& SegmentCache::getSegment(const CheckIn::TPaxSegmentPair& segmentPair) const
{
  auto i=segments.find(segmentPair);
  if (i!=segments.end()) return i->second;

  TAdvTripRoute route;
  route.getRouteBetween(segmentPair.point_dep, segmentPair.airp_arv);

  if (route.size()<2)
  {
    LogTrace(TRACE5) << "segmentPair(" << segmentPair.point_dep << ", " << segmentPair.airp_arv << "): route.size()=" << route.size();
    throw NotFound();
  }

  Segment& segment=segments.emplace(segmentPair, Segment()).first->second;

  segment.departure=*(route.begin());
  segment.arrival=*(route.rbegin());

  if (segment.departure.act_out==ASTRA::NoExists)
    segment.setStages(segment.departure.point_id);

  return segment;
}

void SearchPassengersResponse::add(const CheckIn::TSimplePaxItem& pax,
                                   const std::string& reqDeparture)
{
  if (pax.id==ASTRA::NoExists) return;
  if (!pax.refuse.empty()) return;

  try
  {
    if (pax.grp_id==ASTRA::NoExists)
    {
      //незарегистрированные пассажиры
      CheckIn::TSimplePnrItem pnr;
      pnr.getByPaxId(pax.id);

      TAdvTripInfoList flts;
      getTripsByCRSPnrId(pnr.id, flts);

      if (flts.size()!=1)
      {
        LogTrace(TRACE5) << "flts.size()=" << flts.size() << " (pnr.id=" << pnr.id << ")";
        return;
      }

      const TAdvTripInfo& flt=flts.front();

      if (flt.airp!=reqDeparture) return;

      CheckIn::TPaxSegmentPair segmentPair(flt.point_id, pnr.airp_arv);
      const Segment& segment=getSegment(segmentPair);
      if (!segment.departure.match(TReqInfo::Instance()->user.access)) return;

      Passenger& passenger=*(passengers.emplace(passengers.end(), pax, segmentPair));
      CheckIn::LoadCrsPaxTkn(pax.id, passenger.tkn);
      CheckIn::LoadCrsPaxDoc(pax.id, passenger.doc);
      passenger.pnrAddrs.getByPnrIdFast(pnr.id);

    }
    else
    {
      //зарегистрированные пассажиры
      CheckIn::TPaxGrpItem grp;
      if (!grp.getByGrpId(pax.grp_id)) return;

      if (grp.airp_dep!=reqDeparture) return;

      CheckIn::TPaxSegmentPair segmentPair=grp.getSegmentPair();
      const Segment& segment=getSegment(segmentPair);
      if (!segment.departure.match(TReqInfo::Instance()->user.access)) return;

      Passenger& passenger=*(passengers.emplace(passengers.end(), pax, segmentPair));
      CheckIn::LoadPaxDoc(pax.id, passenger.doc);
      passenger.pnrAddrs.getByPaxIdFast(pax.id);
    }

  }
  catch(const SegmentCache::NotFound&)
  {
    return;
  }
}

void SearchPassengersResponse::searchPassengers(const SearchPassengersRequest& req)
{
  CheckIn::TSimplePaxList paxs;

  CheckIn::Search search(TIMEOUT());
  if (req.barcode && req.barcode.get().completeForSearch())
  {
    req.barcode.get().getBarcodeFilter().getPassengers(search, paxs, true);
  }
  if (req.tkn)
  {
    search(paxs, req.tkn.get());
  }
  else if (req.pnr)
  {
    search(paxs, req.pnr.get());
  }
  else if (req.oper && req.oper.get().completeForSearch())
  {
    search(paxs, req.oper.get().getFlightFilter());
  }
  else if (req.depth && req.depth.get().completeForSearch())
  {
    const SearchPassengersRequest::Depth& depth=req.depth.get();
    if (depth.doc)
    {
      search(paxs, depth.doc.get(), depth.getFlightFilter());
    }
    else if (depth.pax)
    {
      search(paxs, depth.pax.get(), depth.getFlightFilter());
    }
  }
  else throw Exception("Not enough parameters to search");

  if (search.timeoutIsReached())
  {
    ProgTrace(TRACE5, ">>>> search.timeoutIsReached()=true (%d ms)", TIMEOUT());
    return;
  }

  for(const CheckIn::TSimplePaxItem pax : paxs) add(pax, req.departure);
}

bool SearchPassengersResponse::suitable(const Passenger& passenger,
                                        const SearchPassengersRequest& req) const
{
  const Segment& segment=getSegment(passenger.segmentPair);
  //штрихкод
  if (req.barcode &&
      !req.barcode.get().getBarcodeFilter().suitable(segment.departure,
                                                     segment.arrival,
                                                     passenger,
                                                     passenger.pnrAddrs,
                                                     true)) return false;
  //билет
  if (req.tkn && !req.tkn.get().suitable(passenger.tkn)) return false;
  //номер PNR
  if (req.pnr && !req.pnr.get().suitable(passenger.pnrAddrs)) return false;
  //рейс
  if (req.oper && !req.oper.get().getFlightFilter().suitable(segment.departure,
                                                             segment.arrival))  return false;

  if (req.depth)
  {
    const SearchPassengersRequest::Depth& depth=req.depth.get();
    //рейсы
    if (!depth.getFlightFilter().suitable(segment.departure,
                                          segment.arrival))  return false;
    //документ
    if (depth.doc && !depth.doc.get().suitable(passenger.doc)) return false;
    //фамилия
    if (depth.pax && !depth.pax.get().suitable(passenger)) return false;
  }

  return true;
}


void SearchPassengersResponse::filterPassengers(const SearchPassengersRequest& req)
{
  for(auto p=passengers.begin(); p!=passengers.end();)
    if (!suitable(*p, req)) p=passengers.erase(p); else ++p;
}

const SearchPassengersResponse& SearchPassengersResponse::toXML(xmlNodePtr node) const
{
  if (node==nullptr) return *this;

  for(const Passenger& p : passengers)
    p.toXML(NewTextChild(node, "passenger"), getSegment(p.segmentPair), outputLang);

  return *this;
}


const Flight& Flight::toXML(xmlNodePtr node,
                            const AstraLocale::OutputLang& lang) const
{
  if (node==nullptr) return *this;

  SetProp(node, "carrier", airlineToPrefferedCode(airline, lang));
  SetProp(node, "flight_no", flight_number(lang));
  SetProp(node, "departure", airpToPrefferedCode(airp, lang));
  Stages::toXML(node);
  if (scd_out!=ASTRA::NoExists)
    NewTextChild(node, "departure_datetime_scd", DateTimeToStr(UTCToLocal(scd_out, AirpTZRegion(airp)), dateTimeFmt));
  if (est_out_exists())
    NewTextChild(node, "departure_datetime_est", DateTimeToStr(UTCToLocal(est_out.get(), AirpTZRegion(airp)), dateTimeFmt));
  if (act_out_exists())
    NewTextChild(node, "departure_datetime_act", DateTimeToStr(UTCToLocal(act_out.get(), AirpTZRegion(airp)), dateTimeFmt));

  int id=0;
  xmlNodePtr destsNode=NewTextChild(node, "destinations");
  for(const TAdvTripRouteItem& item : routeAfter)
    SetProp(NewTextChild(destsNode, "destination", airpToPrefferedCode(item.airp, lang)), "id", id++);

  NewTextChild(node, "check_in_desks", check_in_desks);
  NewTextChild(node, "gates", gates);

  return *this;
}

void SearchFlightsResponse::add(const TAdvTripInfo& flt)
{
  auto f=flights.emplace(flights.end(), flt);

  Flight& flight=*f;
  flight.routeAfter.GetRouteAfter(flt, trtNotCurrent, trtNotCancelled);
  if (flight.routeAfter.empty())
  {
    flights.erase(f);
    return;
  }
  if (flight.act_out_exists())
    flight.setStages(flight.point_id);
  get_DesksGates(flight.point_id, flight.check_in_desks, flight.gates);
}

void SearchFlightsResponse::searchFlights(const SearchFlightsRequest& req)
{
  if (req.departure.empty()) return;

  TDateTime min_out=req.departure_datetime==ASTRA::NoExists?
                      NowUTC():
                      LocalToUTC(req.departure_datetime, AirpTZRegion(req.departure));
  TDateTime max_out=min_out+req.depth_hours/24.0;

  if (max_out-min_out<=0.0 || max_out-min_out>2.0) return;

  TTripListSQLParams params;
  params.access=TReqInfo::Instance()->user.access;
  params.access.merge_airps(TAccessElems<std::string>(req.departure, true));
  if (!req.oper_carrier.empty())
    params.access.merge_airlines(TAccessElems<std::string>(req.oper_carrier, true));
  if (params.access.airlines().totally_not_permitted() ||
      params.access.airps().totally_not_permitted())
  {
    LogTrace(TRACE5) << "totally_not_permitted!";
    return;
  }

  params.pr_cancel=false;
  params.pr_takeoff=true;
  params.first_date=min_out;
  params.last_date=max_out;
  params.includeScdIntoDateRange=true;

  TQuery Qry(&OraSession);
  setSQLTripList(Qry, params);
  //LogTrace(TRACE5) << Qry.SQLText.SQLText();
  Qry.Execute();
  FlightProps props(FlightProps::NotCancelled, FlightProps::WithCheckIn);
  for(; !Qry.Eof; Qry.Next())
  {
    TAdvTripInfo flt(Qry);
    if (!flt.match(props))
    {
      LogError(STDLOG) << "strange situation: !flt.match(props)";
      continue;
    }
    if (!flt.match(TReqInfo::Instance()->user.access))
    {
      LogError(STDLOG) << "strange situation: !flt.match(TReqInfo::Instance()->user.access)";
      continue;
    }

    add(flt);
  }
}

const SearchFlightsResponse& SearchFlightsResponse::toXML(xmlNodePtr node) const
{
  if (node==nullptr) return *this;

  for(const Flight& f : flights)
    f.toXML(NewTextChild(node, "flight"), outputLang);

  return *this;
}

const GetClientPermsResponse& GetClientPermsResponse::toXML(xmlNodePtr node) const
{
  if (node==nullptr) return *this;

  xmlNodePtr pointsNode=NewTextChild(node, "points");

  const TAccess& access=TReqInfo::Instance()->user.access;

  if (access.airps().totally_not_permitted() ||
      access.airlines().totally_not_permitted()) return *this;

  if ((access.airps().totally_permitted() || access.airps().elems_permit()) &&
      (access.airlines().totally_permitted() || access.airlines().elems_permit()))
  {
    if (!access.airps().totally_permitted())
    {
      for(const std::string& airp : access.airps().elems())
        NewTextChild(pointsNode, "point", airpToPrefferedCode(airp, outputLang));
    }

    if (!access.airlines().totally_permitted())
    {
      xmlNodePtr carriersNode=NewTextChild(node, "carriers");
      for(const std::string& airline : access.airlines().elems())
        NewTextChild(carriersNode, "carrier", airlineToPrefferedCode(airline, outputLang));
    }
  }

  return *this;
}

} //namespace MobilePayment

void MobilePaymentInterface::searchPassengers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  try
  {
    MobilePayment::SearchPassengersRequest req;
    req.fromXML(reqNode);

    MobilePayment::SearchPassengersResponse res;
    res.searchPassengers(req);
    res.filterPassengers(req);
    res.toXML(NewTextChild(resNode, (const char*)reqNode->name));
  }
  catch(const std::exception& e)
  {
    MobilePayment::Response::errorToXML(e, reqNode, resNode);
  }
}

void MobilePaymentInterface::searchFlights(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  try
  {
    MobilePayment::SearchFlightsRequest req;
    req.fromXML(reqNode);

    MobilePayment::SearchFlightsResponse res;
    res.searchFlights(req);
    res.toXML(NewTextChild(resNode, (const char*)reqNode->name));
  }
  catch(const std::exception& e)
  {
    MobilePayment::Response::errorToXML(e, reqNode, resNode);
  }
}

void MobilePaymentInterface::getClientPerms(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  try
  {
    MobilePayment::GetClientPermsResponse res;
    res.toXML(NewTextChild(resNode, (const char*)reqNode->name));
  }
  catch(const std::exception& e)
  {
    MobilePayment::Response::errorToXML(e, reqNode, resNode);
  }
}

void MobilePaymentInterface::getPassengerInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  try
  {
    SirenaExchange::TEntityList entities;
    entities.emplace(NodeAsInteger("pax_id", reqNode), 0);
    SirenaExchange::TPseudoGroupInfoRes res;
    SirenaExchange::fillPaxsSvcs(entities, res);
    res.toXML(NewTextChild(resNode, (const char*)reqNode->name));
  }
  catch(const std::exception& e)
  {
    MobilePayment::Response::errorToXML(e, reqNode, resNode);
  }

}


