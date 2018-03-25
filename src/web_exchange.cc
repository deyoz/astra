#include "web_exchange.h"
#include "points.h"
#include "astra_locale_adv.h"
#include "qrys.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;
using namespace ASTRA;

namespace AstraWeb
{

int getPointIdsSpp(int id, bool is_pnr_id, set<int>& point_ids_spp)
{
  point_ids_spp.clear();
  int result=ASTRA::NoExists;

  TQuery Qry(&OraSession);
  if (!isTestPaxId(id))
  {
    ostringstream sql;
    sql << "SELECT point_id_spp, crs_pnr.pnr_id "
           "FROM crs_pnr,crs_pax,tlg_binding "
           "WHERE crs_pax.pnr_id=crs_pnr.pnr_id AND "
           "      crs_pnr.point_id=tlg_binding.point_id_tlg(+) AND ";
    if (is_pnr_id)
      sql << "      crs_pnr.pnr_id=:id AND ";
    else
      sql << "      crs_pax.pax_id=:id AND ";
    sql << "      crs_pax.pr_del=0";

    Qry.SQLText = sql.str().c_str();
    Qry.CreateVariable( "id", otInteger, id );
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next())
    {
      if (!Qry.FieldIsNULL( "point_id_spp" ))
        point_ids_spp.insert(Qry.FieldAsInteger("point_id_spp"));
      result=Qry.FieldAsInteger("pnr_id");
    }
  }
  else
  {
    Qry.SQLText =
      "SELECT id FROM test_pax WHERE id=:id";
    Qry.CreateVariable( "id", otInteger, id );
    Qry.Execute();
    if ( !Qry.Eof ) result=id;
  };

  return result;
}

int getPointIdsSppByPaxId(int pax_id, set<int>& point_ids_spp)
{
  return getPointIdsSpp(pax_id, false, point_ids_spp);
}

int getPointIdsSppByPnrId(int pnr_id, set<int>& point_ids_spp)
{
  return getPointIdsSpp(pnr_id, true, point_ids_spp);
}


namespace ProtLayerRequest
{

static const std::map<RequestType, std::string>& requestTypes()
{
  static std::map<RequestType, std::string> m={ {AddProtPaidLayer,    "AddProtPaidLayer"},
                                                {AddProtLayer,        "AddProtLayer"},
                                                {RemoveProtPaidLayer, "RemoveProtPaidLayer"},
                                                {RemoveProtLayer,     "RemoveProtLayer"} };
  return m;
}

Pax& Pax::fromXML(xmlNodePtr node)
{
  clear();
  if (node==nullptr) return *this;
  xmlNodePtr node2=node->children;

  id=NodeAsIntegerFast("crs_pax_id", node2);
  seat_no=NodeAsStringFast("seat_no", node2, "");

  return *this;
}

PaxList& PaxList::fromXML(const bool& isLayerAdded, xmlNodePtr paxsParentNode)
{
  clear();
  if (paxsParentNode==nullptr) return *this;

  if (isLayerAdded)
  {
    point_id=NodeAsInteger("point_id", paxsParentNode);
  }

  xmlNodePtr paxsNode=NodeAsNode("passengers", paxsParentNode);
  for(xmlNodePtr paxNode=paxsNode->children; paxNode!=nullptr; paxNode=paxNode->next)
  {
    if (string((const char*)paxNode->name)!="pax") continue;
    Pax pax;
    pax.fromXML(paxNode);
    if (isLayerAdded)
    {
      //проверим на дублирование
      for(const ProtLayerRequest::Pax& existingPax : *this)
      {
        if (existingPax.id==pax.id)
          throw EXCEPTIONS::Exception("%s: crs_pax_id duplicated (crs_pax_id=%d)",
                                      __FUNCTION__, pax.id);
        if (!pax.seat_no.empty() && existingPax.seat_no==pax.seat_no)
          throw EXCEPTIONS::Exception("%s: seat_no duplicated (crs_pax_id=%d, seat_no=%s)",
                                      __FUNCTION__, pax.id, pax.seat_no.c_str());
      }
    }

    this->push_back(pax);
  }

  return *this;
}

RequestType SegList::setRequestType(xmlNodePtr reqNode)
{
  requestType=Unknown;

  if (reqNode==nullptr) return requestType;

  for( const auto& requestTypePair: requestTypes())
    if (requestTypePair.second==(const char*)reqNode->name)
    {
      requestType=requestTypePair.first;
      break;
    }

  return requestType;
}

std::string SegList::getRequestName() const
{
  std::map<RequestType, std::string>::const_iterator i=requestTypes().find(requestType);
  if (i==requestTypes().end()) return "";
  return i->second;
}

SegList& SegList::fromXML(xmlNodePtr reqNode)
{
  clear();
  if (reqNode==nullptr) return *this;

  setRequestType(reqNode);

  if (requestType==Unknown)
    throw EXCEPTIONS::Exception("%s: wrong request type", __FUNCTION__);


  if (isLayerAdded())
  {
    xmlNodePtr node=GetNode("time_limit",reqNode);
    if (node!=NULL && !NodeIsNULL(node))
    {
      time_limit=NodeAsInteger(node);
      if (time_limit<=0 || time_limit>999)
        throw EXCEPTIONS::Exception("%s: wrong time_limit %d min", getRequestName().c_str(), time_limit);
    }
  }

  if (requestType == AddProtPaidLayer ||
      requestType == RemoveProtPaidLayer)
    layer_type=ASTRA::cltProtBeforePay;

  if (requestType == AddProtLayer ||
      requestType == RemoveProtLayer)
  {
    layer_type=ASTRA::cltProtSelfCkin;

    xmlNodePtr node=GetNode("layer_type",reqNode);
    if (node!=NULL && !NodeIsNULL(node))
    {
      layer_type = DecodeCompLayerType(NodeAsString(node));
      if (!(layer_type == ASTRA::cltProtBeforePay ||
            layer_type == ASTRA::cltProtAfterPay ||
            layer_type == ASTRA::cltProtSelfCkin))
        throw EXCEPTIONS::Exception("%s: wrong layer_type %s ", getRequestName().c_str(), NodeAsString(node) );
    }
  }

  if (isLayerAdded())
  {

    xmlNodePtr segsNode=NodeAsNode("segments", reqNode);
    for(xmlNodePtr segNode=segsNode->children; segNode!=nullptr; segNode=segNode->next)
    {
      if (string((const char*)segNode->name)!="segment") continue;
      this->push_back(PaxList().fromXML(isLayerAdded(), segNode));
    }
  }
  else
  {
    this->push_back(PaxList().fromXML(isLayerAdded(), reqNode));
  }


  return *this;
}

void SegList::lockFlights() const
{
  TFlights flights;

  for(const PaxList& paxList : *this)
  {
    if (paxList.point_id!=ASTRA::NoExists)
      flights.Get(paxList.point_id, ftTranzit);
    for(const Pax& pax : paxList)
    {
      set<int> point_ids_spp;
      getPointIdsSppByPaxId(pax.id, point_ids_spp);
      flights.Get(point_ids_spp, ftTranzit);
    }
  }

  flights.Lock(getRequestName()+"::"+__FUNCTION__);
}

} //namespace ProtLayerRequest

namespace ProtLayerResponse
{

bool Pax::fromDB()
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  if (!isTestPaxId(id))
    Qry.SQLText=
        "SELECT crs_pnr.pnr_id, crs_pnr.status AS pnr_status, "
        "       crs_pnr.subclass, crs_pnr.class, "
        "       crs_pax.seats, "
        "       crs_pnr.tid AS crs_pnr_tid, "
        "       crs_pax.tid AS crs_pax_tid, "
        "       pax.tid AS pax_tid "
        "FROM crs_pnr, crs_pax, pax "
        "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pax.pax_id=:crs_pax_id AND "
        "      crs_pax.pax_id=pax.pax_id(+) AND "
        "      crs_pax.pr_del=0";
  else
    Qry.SQLText=
        "SELECT id AS pnr_id, NULL AS pnr_status, "
        "       subclass, subcls.class, "
        "       1 AS seats, "
        "       id AS crs_pnr_tid, "
        "       id AS crs_pax_tid "
        "FROM test_pax, subcls "
        "WHERE test_pax.subclass=subcls.code AND test_pax.id=:crs_pax_id";
  Qry.CreateVariable("crs_pax_id", otInteger, id);
  Qry.Execute();
  if (Qry.Eof) return false;

  TWebTids::fromDB(Qry);
  seats=Qry.FieldAsInteger("seats");
  pnr_id=Qry.FieldAsInteger("pnr_id");
  pnr_status=Qry.FieldAsString("pnr_status");
  pnr_class=Qry.FieldAsString("class");
  pnr_subclass=Qry.FieldAsString("subclass");

  return true;
}

const Pax& Pax::toXML(xmlNodePtr node) const
{
  if (node==nullptr) return *this;

  NewTextChild(node, "crs_pax_id", id);
  TWebTids::toXML(node);
  if (!seatTariff.empty())
  {
    xmlNodePtr rateNode = NewTextChild( node, "rate" );
    NewTextChild( rateNode, "color", seatTariff.color );
    NewTextChild( rateNode, "value", seatTariff.rateView() );
    NewTextChild( rateNode, "currency", seatTariff.currencyView(TReqInfo::Instance()->desk.lang) );
  }

  return *this;
}

void PaxList::checkFlight() const
{
  TTripInfo fltInfo;
  if (!fltInfo.getByPointId(point_id))
    throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  if (!fltInfo.match(FlightProps(FlightProps::NotCancelled)))
    throw UserException( "MSG.FLIGHT.CANCELED" );
  if (!fltInfo.match(FlightProps(FlightProps::NotCancelled, FlightProps::WithCheckIn)))
    throw UserException( "MSG.FLIGHT.CHECKIN_CANCELED" );
}

void PaxList::complete(const ProtLayerRequest::SegList& segListReq)
{
  if (time_limit==ASTRA::NoExists) time_limit=segListReq.time_limit;

  if (segListReq.layer_type==cltProtBeforePay ||
      segListReq.layer_type==cltProtAfterPay)
  {
    //разметка платными слоями
    TCachedQuery Qry("SELECT pr_permit, prot_timeout FROM trip_paid_ckin WHERE point_id=:point_id",
                     QParams() << QParam("point_id", otInteger, point_id));
    Qry.get().Execute();
    if ( Qry.get().Eof || Qry.get().FieldAsInteger("pr_permit")==0 )
      throw UserException( "MSG.CHECKIN.NOT_PAID_CHECKIN_MODE" );

    if (time_limit==ASTRA::NoExists && !Qry.get().FieldIsNULL("prot_timeout"))
      time_limit=Qry.get().FieldAsInteger("prot_timeout");
  }
  else
  {
    if (time_limit==ASTRA::NoExists) time_limit=10; //потому что не храним настройку в БД
  }

  if (time_limit==NoExists)
    throw UserException( "MSG.PROT_TIMEOUT_NOT_DEFINED" );
}

const PaxList& PaxList::toXML(const bool& isLayerAdded, xmlNodePtr paxsParentNode) const
{
  if (paxsParentNode==nullptr) return *this;

  if (isLayerAdded)
  {
    NewTextChild(paxsParentNode, "point_id", point_id);
    NewTextChild(paxsParentNode, "time_limit", time_limit);
  }

  xmlNodePtr paxsNode=NewTextChild(paxsParentNode, "passengers");
  for(const Pax& pax : *this)
    pax.toXML(NewTextChild(paxsNode, "pax"));

  return *this;
}

const SegList& SegList::toXML(const bool& isLayerAdded, xmlNodePtr resNode) const
{
  if (resNode==nullptr) return *this;

  NewTextChild( resNode, "layer_type", EncodeCompLayerType(layer_type) );
  if (isLayerAdded)
  {
    xmlNodePtr segsNode=NewTextChild(resNode, "segments");
    for(const PaxList& paxList : *this)
      paxList.toXML(isLayerAdded, NewTextChild(segsNode, "segment"));
  }
  else
  {
    for(const PaxList& paxList : *this)
      paxList.toXML(isLayerAdded, resNode);
  }

  return *this;
}

} //namespace ProtLayerResponse

} //namespace AstraWeb
