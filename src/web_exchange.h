#pragma once

#include <map>

#include "astra_consts.h"
#include "checkin_utils.h"
#include "xml_unit.h"
#include "salons.h"

namespace AstraWeb
{

int getPointIdsSppByPaxId(int pax_id, std::set<int>& point_ids_spp); //возвращает pnr_id
int getPointIdsSppByPnrId(int pnr_id, std::set<int>& point_ids_spp); //возвращает pnr_id
int getPointIdsSpp(int id, bool is_pnr_id, std::set<int>& point_ids_spp);

namespace ProtLayerRequest
{

enum RequestType {AddProtPaidLayer, AddProtLayer, RemoveProtPaidLayer, RemoveProtLayer, Unknown};

class Pax
{
  public:
    int id;
    std::string seat_no;

    Pax() { clear(); }

    void clear()
    {
      id=ASTRA::NoExists;
      seat_no.clear();
    }

    Pax& fromXML(xmlNodePtr node);
};

class PaxList : public std::list<Pax>
{
  public:
    int point_id;

    PaxList() { clear(); }

    void clear()
    {
      point_id=ASTRA::NoExists;
      std::list<Pax>::clear();
    }

    PaxList& fromXML(const bool& isLayerAdded, xmlNodePtr paxsParentNode);
};

class SegList : public std::list<PaxList>
{
  public:
    RequestType requestType;
    int time_limit;
    ASTRA::TCompLayerType layer_type;

    SegList() { clear(); }

    void clear()
    {
      requestType=Unknown;
      time_limit=ASTRA::NoExists;
      layer_type=ASTRA::cltUnknown;
      std::list<PaxList>::clear();
    }
    RequestType setRequestType(xmlNodePtr reqNode);
    std::string getRequestName() const;
    bool isLayerAdded() const
    {
      return requestType == AddProtPaidLayer ||
             requestType == AddProtLayer;
    }

    SegList& fromXML(xmlNodePtr reqNode);
    void lockFlights() const;
    void checkAndComplete();
};

} //namespace ProtLayerRequest

namespace ProtLayerResponse
{

class Pax : public TWebTids
{
  public:
    int id;
    std::string seat_no;
    int seats;
    int pnr_id;
    std::string pnr_status;
    std::string pnr_class;
    std::string pnr_subclass;
    SALONS2::TSeatTariff seatTariff;

    Pax(const ProtLayerRequest::Pax& paxReq)
    {
      clear();
      id=paxReq.id;
      seat_no=paxReq.seat_no;
    }

    void clear()
    {
      TWebTids::clear();
      id=ASTRA::NoExists;
      seat_no.clear();
      seats=ASTRA::NoExists;
      pnr_id=ASTRA::NoExists;
      pnr_status.clear();
      pnr_class.clear();
      pnr_subclass.clear();
      seatTariff.clear();
    }

    bool fromDB();
    const Pax& toXML(xmlNodePtr node) const;
};

class PaxList : public std::list<Pax>
{
  public:
    int point_id;
    int time_limit;

    PaxList() { clear(); }

    void clear()
    {
      point_id=ASTRA::NoExists;
      time_limit=ASTRA::NoExists;
      std::list<Pax>::clear();
    }

    void checkFlight() const;
    void complete(const ProtLayerRequest::SegList& segListReq);

    const PaxList& toXML(const bool& isLayerAdded, xmlNodePtr paxsParentNode) const;
};

class SegList : public std::list<PaxList>
{
  public:
    ASTRA::TCompLayerType layer_type;
    int curr_tid;
    CheckIn::UserException ue;

    SegList() { clear(); }

    void clear()
    {
      layer_type=ASTRA::cltUnknown;
      curr_tid=ASTRA::NoExists;
      std::list<PaxList>::clear();
    }

    const SegList& toXML(const bool& isLayerAdded, xmlNodePtr resNode) const;
};

} //namespace ProtLayerResponse

} //namespace AstraWeb
