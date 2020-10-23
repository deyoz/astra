#pragma once

#include "astra_misc.h"
#include "astra_api.h"
#include "remarks.h"

namespace CheckIn
{

class Segment
{
  private:
    boost::optional<PointId_t> pointArv_;
    boost::optional<AirportCode_t> airpArv_;
  public:
    TAdvTripInfo flt;
    TAdvTripRoute route;

    static boost::optional<Segment> fromDB(const PointId_t& pointDep,
                                           const AirportCode_t& airpDep,
                                           const boost::optional<PointId_t>& pointArv,
                                           const AirportCode_t& airpArv);
    static Segment fromDB(xmlNodePtr segNode);
    static bool isNewCheckin(xmlNodePtr segNode);
    static bool isSuitableForTransitLeg(const CheckIn::TPaxRemItem& rem);

    void formatToLog(PrmEnum& routeFmt) const;

    PointId_t pointDep() const;
    PointId_t pointArv() const;
    AirportCode_t airpDep() const;
    AirportCode_t airpArv() const;
    TDateTime scdOutLocal() const;

    void transformXmlForTransitLeg(xmlNodePtr legNode, const TAdvTripRouteItem& leg) const;
    bool addTransitLegsIfNeeded(xmlNodePtr segNode) const;
    bool transitBortChangingExists() const;
};

class IatciSegment
{
  private:
    astra_api::xml_entities::XmlTripHeader trip_header;
    astra_api::xml_entities::XmlSegmentInfo seg_info;
  public:
    IatciSegment(const astra_api::xml_entities::XmlSegment& xmlSeg) :
      trip_header(xmlSeg.trip_header),
      seg_info(xmlSeg.seg_info) {}

    void formatToLog(PrmEnum& routeFmt) const;
};

class Segments
{
  public:
    std::vector<Segment> segs;
    bool is_edi;
};

class SegmentMap : public std::map<PointId_t, Segment>
{
  private:
    xmlNodePtr firstSegNode;
    xmlNodePtr currSegNode;
    xmlNodePtr currTransitNode;
    int currSegNo;
    int currTransitNum;
    boost::optional<int> currGrpNum;

    void nextSeg();
  public:
    SegmentMap(xmlNodePtr segmentsParentNode);

    void add(const Segment& segment)
    {
      emplace(PointId_t(segment.flt.point_id), segment);
    }

    const Segment& get(const PointId_t& pointId, const std::string& whence) const;

    std::set<PointId_t> getPointIds();

    xmlNodePtr segNode() const { return currSegNode; }
    xmlNodePtr grpNode() const { return currTransitNode!=nullptr?currTransitNode:currSegNode; }
    int segNo() const { return currSegNo; }
    int transitNum() const { return currTransitNum; }
    int grpNum() const;

    void setFirstGrp();
    void setNextGrp();
    bool noMoreGrp() const;

    void setFirstSeg();
    void setNextSeg();
    bool noMoreSeg() const;

    bool isFirstSeg() const;
    bool isFirstGrp() const;
    bool isTransitGrp() const;
};

} //namespace CheckIn

