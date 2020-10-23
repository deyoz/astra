#include "checkin_segments.h"
#include "astra_locale_adv.h"
#include "flt_settings.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>


using namespace ASTRA;
using namespace AstraLocale;
using namespace std;

namespace CheckIn
{

boost::optional<Segment> Segment::fromDB(const PointId_t& pointDep,
                                         const AirportCode_t& airpDep,
                                         const boost::optional<PointId_t>& pointArv,
                                         const AirportCode_t& airpArv)
{
  Segment segment;

  if (!segment.flt.getByPointId(pointDep.get(), FlightProps(FlightProps::WithCancelled,
                                                            FlightProps::WithOrWithoutCheckIn))) return {};
  if (segment.flt.airp!=airpDep.get()) return {};

  if (pointArv)
  {
    segment.route.getRouteBetween(segment.flt, pointArv.get());
    if (!segment.route.empty() && segment.route.back().airp!=airpArv.get())
      segment.route.clear();
  }
  else
  {
    segment.route.getRouteBetween(segment.flt, airpArv);
  }

  if (segment.route.size()==1) segment.route.clear(); //вылет и прилет один и тот же пункт

  segment.pointArv_=pointArv;
  segment.airpArv_=airpArv;

  return segment;
}

Segment Segment::fromDB(xmlNodePtr segNode)
{
  boost::optional<CheckIn::Segment> seg=
      fromDB(PointId_t(NodeAsInteger("point_dep",segNode)),
             AirportCode_t(NodeAsString("airp_dep",segNode)),
             PointId_t(NodeAsInteger("point_arv",segNode)),
             AirportCode_t(NodeAsString("airp_arv",segNode)));
  if (!seg)
    throw UserException("MSG.FLIGHT.CANCELED.REFRESH_DATA");

  if (!seg.get().flt.match(FlightProps(FlightProps::NotCancelled,
                                       FlightProps::WithOrWithoutCheckIn)))
    throw UserException("MSG.FLIGHT.CANCELED_NAME.REFRESH_DATA",
                        LParams()<<LParam("flight",GetTripName(seg.get().flt,ecCkin,true,false)));

  if (!seg.get().flt.match(FlightProps(FlightProps::NotCancelled,
                                       FlightProps::WithCheckIn)) ||
      seg.get().route.empty())
    throw UserException("MSG.FLIGHT.CHANGED_NAME.REFRESH_DATA",
                        LParams()<<LParam("flight",GetTripName(seg.get().flt,ecCkin,true,false)));

  return seg.get();
}

bool Segment::isNewCheckin(xmlNodePtr segNode)
{
  return NodeIsNULL("grp_id", segNode, true);
}


bool Segment::isSuitableForTransitLeg(const CheckIn::TPaxRemItem& rem)
{
  if (isReadonlyRem(rem)) return false;

  TRemCategory cat=getRemCategory(rem);
  if (cat==remCREW || cat==remUnknown) return true;

  return false;
}

void Segment::transformXmlForTransitLeg(xmlNodePtr legNode, const TAdvTripRouteItem& leg) const
{
  if (legNode==nullptr) return;

  ReplaceTextChild(legNode, "point_dep", leg.point_id);
  ReplaceTextChild(legNode, "airp_dep", leg.airp);
  ReplaceTextChild(legNode, "status", EncodePaxStatus(psTransit));
  RemoveNode(GetNode("mark_flight", legNode));
  xmlNodePtr paxNode=GetNode("passengers", legNode);
  if (paxNode==nullptr) return;

  for(paxNode=paxNode->children; paxNode!=nullptr; paxNode=paxNode->next)
  {
    if (string((const char*)paxNode->name)!="pax") continue;

    xmlNodePtr node2=paxNode->children;

    if (!NodeIsNULLFast("pax_id", node2))
      NewTextChild(paxNode, "original_crs_pax_id", NodeAsIntegerFast("pax_id", node2)); //сохраняем pax_id из PNL
    ReplaceTextChild(paxNode, "pax_id");  //делаем NOREC для транзитных паксов

    RemoveChildNodes(GetNodeFast("fqt_rems", node2));

    xmlNodePtr remNode=GetNodeFast("rems", node2);
    if (remNode==nullptr) continue;

    for(remNode=remNode->children; remNode!=nullptr;)
    {
      if (string((const char*)remNode->name)!="rem")
      {
        remNode=remNode->next;
        continue;
      }

      xmlNodePtr nodeToRemove=nullptr;
      if (!isSuitableForTransitLeg(CheckIn::TPaxRemItem().fromXML(remNode))) //оставляем только ничего не значащие ремарки или CREW
        nodeToRemove=remNode;

      remNode=remNode->next;

      RemoveNode(nodeToRemove);
    }
  }
}

bool Segment::addTransitLegsIfNeeded(xmlNodePtr segNode) const
{
  bool result=false;
  if (!isNewCheckin(segNode)) return result;

  boost::optional<XMLDoc> transitSegmentsDoc;

  for(TAdvTripRoute::const_iterator i=route.begin(); i!=route.end();)
  {
    if (i==route.begin())
    {
      ++i;
      continue;
    }

    const TAdvTripRouteItem& leg=*i;

    ++i;

    if (i!=route.end())
    {
      if (TTripInfo().getByPointId(leg.point_id, FlightProps(FlightProps::NotCancelled,
                                                             FlightProps::WithCheckIn)))
      {
        if (TTripSetList().fromDB(leg.point_id).value<bool>(tsTransitBortChanging))
        {
          if (!transitSegmentsDoc)
            transitSegmentsDoc=boost::in_place("transit_segments");

          xmlNodePtr transitSegsNode=GetNode("/transit_segments", transitSegmentsDoc.get().docPtr());
          transformXmlForTransitLeg(CopyNode(transitSegsNode, segNode), leg);
          result=true;
        }
      }
    }
  }

  if (transitSegmentsDoc)
    CopyNode(segNode, NodeAsNode("/transit_segments", transitSegmentsDoc.get().docPtr()));

  return result;
}

bool Segment::transitBortChangingExists() const
{
  for(TAdvTripRoute::const_iterator i=route.begin(); i!=route.end();)
  {
    if (i==route.begin())
    {
      ++i;
      continue;
    }

    const TAdvTripRouteItem& leg=*i;

    ++i;

    if (i!=route.end())
    {
      if (TTripInfo().getByPointId(leg.point_id, FlightProps(FlightProps::NotCancelled,
                                                             FlightProps::WithCheckIn)))
      {
        if (TTripSetList().fromDB(leg.point_id).value<bool>(tsTransitBortChanging)) return true;
      }
    }
  }

  return false;
}

PointId_t Segment::pointDep() const
{
  return PointId_t(flt.point_id);
}

PointId_t Segment::pointArv() const
{
  if (pointArv_) return pointArv_.get();

  if (route.size()<2)
    throw EXCEPTIONS::Exception("%s: route.size()=%zu", __func__, route.size());

  return PointId_t(route.back().point_id);
}

AirportCode_t Segment::airpDep() const
{
  return AirportCode_t(flt.airp);
}

AirportCode_t Segment::airpArv() const
{
  if (airpArv_) return airpArv_.get();

  if (route.size()<2)
    throw EXCEPTIONS::Exception("%s: route.size()=%zu", __func__, route.size());

  return AirportCode_t(route.back().airp);
}

TDateTime Segment::scdOutLocal() const
{
  return BASIC::date_time::UTCToLocal(flt.scd_out, AirpTZRegion(flt.airp));
}

const Segment& SegmentMap::get(const PointId_t& pointId, const std::string &whence) const
{
  SegmentMap::const_iterator i=find(pointId);
  if (i==end())
    throw EXCEPTIONS::Exception("%s: pointId=%d not found in SegmentMap", whence.c_str(), pointId.get());
  return i->second;
}

std::set<PointId_t> SegmentMap::getPointIds()
{
  std::set<PointId_t> pointIds;
  for(setFirstSeg(); !noMoreSeg(); setNextSeg())
    if (!pointIds.insert(PointId_t(NodeAsInteger("point_dep", segNode()))).second)
      throw UserException("MSG.CHECKIN.DUPLICATED_FLIGHT_IN_ROUTE");

  return pointIds;
}

SegmentMap::SegmentMap(xmlNodePtr segmentsParentNode)
{
  firstSegNode=NodeAsNode("segments/segment", segmentsParentNode);
  setFirstGrp();
}

void SegmentMap::setFirstGrp()
{
  currSegNode=firstSegNode;
  currTransitNode=nullptr;
  currSegNo=1;
  currTransitNum=0;
  currGrpNum=1;
}

void SegmentMap::nextSeg()
{
  for(currSegNode=currSegNode->next;
      currSegNode!=nullptr;
      currSegNode=currSegNode->next)
    if (string((const char*)currSegNode->name)=="segment") break;

  currSegNo++;
  currTransitNum=0;
}

void SegmentMap::setNextGrp()
{
  if (currSegNode==nullptr)
    throw EXCEPTIONS::Exception("%s: no more group", __func__);

  if (currTransitNode==nullptr)
    currTransitNode=GetNode("transit_segments/segment", currSegNode);
  else
  {
    for(currTransitNode=currTransitNode->next;
        currTransitNode!=nullptr;
        currTransitNode=currTransitNode->next)
      if (string((const char*)currTransitNode->name)=="segment") break;
  }

  if (currTransitNode==nullptr)
    nextSeg();
  else
    currTransitNum++;

  if (currGrpNum) currGrpNum.get()++;
}

bool SegmentMap::noMoreGrp() const
{
  return noMoreSeg();
}

void SegmentMap::setFirstSeg()
{
  setFirstGrp();
}

void SegmentMap::setNextSeg()
{
  if (currSegNode==nullptr)
    throw EXCEPTIONS::Exception("%s: no more segment", __func__);

  nextSeg();
  currGrpNum=boost::none;
}

bool SegmentMap::noMoreSeg() const
{
  return currSegNode==nullptr;
}

int SegmentMap::grpNum() const
{
  if (!currGrpNum)
    throw EXCEPTIONS::Exception("%s: unacceptable in this context");
  return currGrpNum.get();
}

bool SegmentMap::isFirstSeg() const
{
  return currSegNo==1;
}

bool SegmentMap::isFirstGrp() const
{
  if (!currGrpNum)
    throw EXCEPTIONS::Exception("%s: unacceptable in this context");
  return currGrpNum.get()==1;
}

bool SegmentMap::isTransitGrp() const
{
  if (!currGrpNum)
    throw EXCEPTIONS::Exception("%s: unacceptable in this context");
  return currTransitNum!=0;
}

void Segment::formatToLog(PrmEnum& routeFmt) const
{
  TDateTime local_scd = BASIC::date_time::UTCToLocal(flt.scd_out, AirpTZRegion(flt.airp));
  modf(local_scd, &local_scd);

  routeFmt.prms << PrmSmpl<string>("", " -> ")
                << PrmFlight("", flt.airline, flt.flt_no, flt.suffix)
                << PrmSmpl<string>("", "/")
                << PrmDate("", local_scd, "dd") << PrmSmpl<string>("", ":")
                << PrmElem<string>("", etAirp, airpDep().get())
                << PrmSmpl<string>("", "-")
                << PrmElem<string>("", etAirp, airpArv().get());
}

void IatciSegment::formatToLog(PrmEnum& routeFmt) const
{
  routeFmt.prms << PrmSmpl<string>("", " -> ")
                << PrmFlight("", trip_header.airline, trip_header.flt_no, trip_header.suffix)
                << PrmSmpl<string>("", "/")
                << PrmDate("", trip_header.scd_out_local, "dd") << PrmSmpl<string>("", ":")
                << PrmElem<string>("", etAirp, seg_info.airp_dep)
                << PrmSmpl<string>("", "-")
                << PrmElem<string>("", etAirp, seg_info.airp_arv);
}

}
