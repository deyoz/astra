#include "service_eval.h"
#include <stdlib.h>
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "stl_utils.h"
#include "astra_misc.h"
#include "rfisc_price.h"
#include "checkin.h"
#include "ckin_search.h"
#include "SWCExchangeIface.h"
#include "astra_context.h"
#include "term_version.h"
#include "date_time.h"
#include "serverlib/xml_stuff.h" // для xml_decode_nodelist
//#include "sberbank.h"

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;

void cacheBack();

const int MANUAL_PAY_METHOD = 1;

bool isPaymentAtDesk( int point_id )
{
  int method_type;
  return isPaymentAtDesk( point_id, method_type );
}

bool isManualPayAtDest( int point_id )
{
  int method_type;
  return ( isPaymentAtDesk( point_id,method_type) &&
           method_type == MANUAL_PAY_METHOD );
}

bool isPaymentAtDesk( int point_id, int &method_type )
{
  method_type = ASTRA::NoExists;
  TTripInfo fltInfo;
  if (!fltInfo.getByPointId(point_id))
    throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  if ( GetTripSets(tsPayAtDesk, fltInfo) ) {
     TQuery Qry(&OraSession);
     Qry.SQLText =
       "SELECT method_type, "
       " DECODE(airline,:airline,100,NULL,50,0) + "
       " DECODE(airp_dep,:airp_dep,100,NULL,50,0)+ "
       " DECODE(desk,:desk,1000,NULL,50,0) + "
       " DECODE(desk_grp_id,:desk_grp_id,500,NULL,50,0) AS priority "
       " FROM pay_methods_set "
       " WHERE (airline=:airline OR airline IS NULL) AND "
       "       (airp_dep=:airp_dep OR airp_dep IS NULL) AND "
       "       (desk=:desk OR desk IS NULL) AND "
       "       (desk_grp_id=:desk_grp_id OR desk_grp_id IS NULL) "
       "ORDER BY priority DESC";
     Qry.CreateVariable( "airline", otString, fltInfo.airline );
     Qry.CreateVariable( "airp_dep", otString, fltInfo.airp );
     Qry.CreateVariable( "desk", otString, TReqInfo::Instance()->desk.code );
     Qry.CreateVariable( "desk_grp_id", otInteger, TReqInfo::Instance()->desk.grp_id );
     Qry.Execute();
     bool res = ( !Qry.Eof && Qry.FieldAsInteger( "priority" ) != 0 );
     if ( res ) {
       method_type = Qry.FieldAsInteger( "method_type" );
     }
     return res;
  }
  return false;
}

class Passenger {
  public:
    std::string id;
    bool lead_pass;
    std::string name;
    std::string surname;
    std::string doccode;
    std::string doc;
    void fromXML( xmlNodePtr node ) {
      id = NodeAsString( "@id", node );
      lead_pass = NodeAsBoolean( "@lead_pass", node, false );
      name = NodeAsString( "name", node );
      surname = NodeAsString( "surname", node );
      doccode = NodeAsString( "doccode", node );
      doc = NodeAsString( "doc", node );
    }
};

class Passengers: public std::vector<Passenger> {
  public:
    void fromXML( xmlNodePtr node ) {
      clear();
      node = GetNode( "passengers", node );
      if ( node != nullptr ) {
        node = node->children;
        while ( node != nullptr &&
                std::string("passenger") == (const char*)node->name ) {
          Passenger s;
          s.fromXML(node);
          emplace_back(s);
          node = node->next;
        }
      }
    }
    void get( const std::string &id, Passenger &pass ) {
      for ( const auto &p : *this ) {
        if ( p.id == id ) {
          pass = p;
          return;
        }
      }
      throw EXCEPTIONS::Exception( "passenger not found, id=%s", id );
    }
};

class Segment {
  public:
    int trfer_num;
    std::string id;
    std::string company;
    std::string flight;
    std::string departure_airport;
    void fromXML( xmlNodePtr node, int vtrfer_num ) {
      trfer_num = vtrfer_num;
      id = NodeAsString( "@id", node );
      company = NodeAsString( "company", node );
      flight = NodeAsString( "flight", node );
      xmlNodePtr n1 = GetNode( "departure/airport", node );
      departure_airport = n1?NodeAsString( n1 ):NodeAsString( "departure/city", node );
    }
};

class Segments: public std::vector<Segment> {
  public:
    void fromXML( xmlNodePtr node ) {
      clear();
      node = GetNode( "segments", node );
      if ( node != nullptr ) {
        node = node->children;
        int trfer_num = 0;
        while ( node != nullptr ) {
          if ( std::string("segment") == (const char*)node->name ) {
            Segment s;
            s.fromXML(node,trfer_num);
            emplace_back(s);
            trfer_num++;
          }
          node = node->next;
        }
      }
    }
    void get( const std::string &id, Segment &seg ) {
      for ( const auto &p : *this ) {
        if ( p.id == id ) {
          seg = p;
          return;
        }
      }
      throw EXCEPTIONS::Exception( "segment not found, id=%s", id );
    }
};

class Ticket {
  public:
    std::string pass_id;
    std::string seg_id;
    std::string svc_id;
    std::string ticket_cpn;
    std::string ticknum;
    bool is_etick;
    void fromXML( xmlNodePtr node ) {
      pass_id = NodeAsString( "@pass_id", node );
      seg_id = NodeAsString( "@seg_id", node, "" ); // может быть пустой в случае подтверждения оплаты
      svc_id = NodeAsString( "@svc_id", node, "" ); // может быть пустой в случае запроса стоимости
      ticket_cpn = NodeAsString( "@ticket_cpn", node );
      ticknum = NodeAsString( "@ticknum", node );
      is_etick = NodeAsBoolean( "@is_etick", node, false );
    }
    bool isEMD() const {
      return !is_etick;
    }
    void clear(){
      pass_id.clear();
      seg_id.clear();
      svc_id.clear();
      ticket_cpn.clear();
      ticknum.clear();
      is_etick = true;
    }
};

class Tickets: public std::vector<Ticket> {
  public:
    void fromXML( xmlNodePtr node ) {
      clear();
      node = node->children;
      while ( node != nullptr ) {
        if ( string("tickinfo") == (const char*)node->name &&
             string("ticket") == NodeAsString( node ) ) {
          Ticket t;
          t.fromXML( node );
          emplace_back(t);
          LogTrace(TRACE5) << "pass_id=" << t.pass_id << ",seg_id=" << t.seg_id << ",ticket=" << t.ticknum + "/" + t.ticket_cpn;
        }
        node = node->next;
      }
    }
    void get( const std::string &pass_id, const std::string &seg_id, Ticket &ticket ) {
      ticket.clear();
      for ( const auto &p : *this ) {
        if ( p.pass_id == pass_id &&
             p.seg_id == seg_id &&
             !p.isEMD() ) {
          ticket = p;
          return;
        }
      }
      throw EXCEPTIONS::Exception( "ticket not found, pass_id=%s, seg_id=%s", pass_id, seg_id );
    }
    bool get( const std::string& svc_id, Ticket &ticket ) {
      ticket.clear();
      for ( const auto &p : *this ) {
        if ( p.svc_id == svc_id ) {
           ticket = p;
           return true;
        }
      }
      return false;
    }
};

class Price {
  public:
    std::string accode;
    std::string baggage;
    std::string code;
    std::string currency;
    PriceDoc doc;
    std::string passenger_id;
    std::string svc_id;
    std::string ticket;
    std::string ticket_cpn;
    std::string validating_company;
    float total;
    std::string is_reason;
    bool is_bagnorm() {
      return ( std::string("is_bagnorm") == is_reason );
    }

    void fromXML( xmlNodePtr node ) {
      is_reason = NodeAsString( "@reason", node, "");
      LogTrace(TRACE5) << "is_reason=" << is_reason;
      if ( is_reason.empty() ) {
        baggage = NodeAsString( "@baggage", node );
        doc = PriceDoc( NodeAsString( "@doc_id", node ), NodeAsString( "@doc_type", node ), NodeAsBoolean( "@unpoundable", NodeAsNode("fare",node), false ) );
      }
      code = NodeAsString( "@code", node );
      accode = NodeAsString( "@accode", node, "" );
      ticket = NodeAsString( "@ticket", node, "" );
      ticket_cpn = NodeAsString( "@ticket_cpn", node, "" );
      validating_company = NodeAsString( "@validating_company", node, "" );
      passenger_id = NodeAsString( "@passenger-id", node );
      svc_id = NodeAsString( "@svc-id", node, "" ); // тарификация неизвестна
      total = is_bagnorm()?-1.0:NodeAsFloat( "total", node );
      currency = !is_reason.empty()?std::string("РУБ"):NodeAsString( "@currency", node );
    }
};

class Prices: public std::vector<Price> {
  public:
    void fromXML( xmlNodePtr node ) {
      node = GetNode( "prices", node );
      if ( node != nullptr ) {
        node = node->children;
        while ( node != nullptr &&
                ((std::string("price") == (const char*)node->name )||
                 (std::string("no_price") == (const char*)node->name )) ) {
          Price s;
          s.fromXML(node);
          emplace_back(s);
          node = node->next;
        }
      }
    }
    PriceDoc getDoc(const std::string& svc_id) {
      for ( const auto &p : *this ) {
        if ( p.svc_id == svc_id ) {
          return p.doc;
        }
      }
      return PriceDoc("","",false);
    }
};


class SvcEmdCommonRes: public SWC::SWCExchange
{
  private:
    std::string sexchangeId;
  protected:
    virtual bool isRequest() const {
      return false;
    }
  public:
    virtual std::string exchangeId() const {
      return sexchangeId;
    }
    SvcEmdCommonRes( const std::string& _sexchangeId):sexchangeId(_sexchangeId){}
    virtual void clear() {}
    virtual void toXML(xmlNodePtr node) const {}
    ~SvcEmdCommonRes(){}
};

class Order: public SvcEmdRegnum, public SvcEmdSvcsAns
{
  public:
    int grp_id;
    //TPriceRFISCList PriceSirenaRFISCList;
    std::map<std::string,int> paxs;
    std::map<std::string,int> trfer_nums;
    std::string agency;
    Passengers passengers;
    Segments segments;
    Prices prices;
    Tickets tickets;
    void fromXML( xmlNodePtr node ) {
      clear();
      agency = NodeAsString( "@agency", node, "" );
      LogTrace(TRACE5) << agency;
      tickets.fromXML( node );
      node = GetNode( "pnr", node );
      SvcEmdRegnum::fromXML(node,SvcEmdRegnum::Enum::nodeStyle);
      passengers.fromXML( node );
      segments.fromXML( node );
      prices.fromXML( node );
      SvcEmdSvcsAns::fromXML( node );
    }
    void clear() {
      //PaidRFISCListEvaluation::clear();
      SvcEmdRegnum::clear();
      agency.clear();
      passengers.clear();
      segments.clear();
      prices.clear();
      tickets.clear();
      paxs.clear();
      trfer_nums.clear();
      SvcEmdSvcsAns::clear();
    }
    void toPrice( int grp_id, int point_dep, TPriceRFISCList &svcList ) {
      PaxsNames paxsNames;
  //    SegsPaxs segPaxs;
//      segPaxs.fromDB(grp_id,point_dep);
      for ( auto &p : prices ) {
        if ( p.svc_id.empty() ) { //тарфикация не задана - не смогли оценить
          continue;
        }
        LogTrace(TRACE5) << "pass_id=" << p.passenger_id << ",svc_id=" << p.svc_id << ",total=" << p.total << ",currency=" << p.currency;
        SvcValue svc;
        SvcEmdSvcsAns::getSvcValue( p.svc_id, svc );
        LogTrace(TRACE5) << "svc_id=" << svc.id << ",pass_id=" << svc.pass_id << ",seg_id=" << svc.seg_id;
        if ( svc.pass_id !=  p.passenger_id )
          throw EXCEPTIONS::Exception( "Invalid data XML pass_id from passengers not equal pass_id from svcs, passengers.pass_id=%s,svcs.pass_id=%s", p.passenger_id,svc.pass_id );
        Ticket ticket;
        tickets.get(svc.pass_id,svc.seg_id,ticket);
        if ( ticket.pass_id !=  p.passenger_id )
          throw EXCEPTIONS::Exception( "Invalid data XML pass_id from passengers not equal pass_id from tickets, passengers.pass_id=%s,tickets.pass_id=%s", p.passenger_id, ticket.pass_id );
        if ( ticket.seg_id !=  svc.seg_id )
          throw EXCEPTIONS::Exception( "Invalid data XML seg_id from svcs not equal seg_id from tickets, svcs.seg_id=%s,tickets.seg_id=%s", svc.seg_id, ticket.seg_id );
      }
      for ( auto &p : *this ) {
        LogTrace(TRACE5) << p.pass_id << " " << p.status;
/*  !!!      if ( std::string("HD") != p.status ) {
          continue;
        }*/
        Ticket t;
        tickets.get( p.pass_id, p.seg_id, t );
        CheckIn::TSimplePaxList paxs_list;
        CheckIn::Search search(paxPnl);//!!!paxCheckIn);
        int coupon;
        if ( StrToInt(t.ticket_cpn.c_str(), coupon) == EOF ) {
          throw EXCEPTIONS::Exception( "Invalid data XML counpon no=%s", t.ticket_cpn );
        }
        tst();
        search(paxs_list, CheckIn::TPaxTknItem(t.ticknum,coupon));
        LogTrace(TRACE5)<<paxs_list.size() << ",ticknum=" <<t.ticknum<<",coupon="<<coupon;

        if ( paxs_list.size() != 1 ) {
           throw AstraLocale::UserException("MSG.ETICK.ERROR.TICKET_NOT FOUND");
        }
        if ( paxs.find(p.pass_id) != paxs.end() ) {
          /*
           * !!!
           * if ( paxs[p.pass_id] != paxs_list.begin()->id ) {
            LogTrace(TRACE5)<<p.pass_id<<"="<<paxs[p.pass_id]<<",paxs_list.begin()->id="<<paxs_list.begin()->id;
            throw EXCEPTIONS::Exception( "Invalid data XML astra pax_id not identical sirena pax_id" );
          }*/
        }
        else {
          paxs.insert( make_pair(p.pass_id,paxs_list.begin()->id) );
          LogTrace(TRACE5) << "sirena pax_id=" << p.pass_id << ",astra pax_id=" << paxs_list.begin()->id;
        }
      }
      for ( auto &p : segments ) {
/*
 * !!!        if ( !segPaxs.checkTrferNum(p.trfer_num) ) { //pfhtubcnhbhjdfyyst
          throw EXCEPTIONS::Exception( "Invalid data XML sirena trfer_num not found in astra routes, trfer_num=%d", p.trfer_num );
        }*/
        LogTrace(TRACE5) << "id="<<p.id <<",trfer_num=" << p.trfer_num;
        trfer_nums[p.id] = p.trfer_num;
      }
      tst();
      for ( auto &p : prices ) {
        if ( p.svc_id.empty()/* || p.validating_company.empty()*/ ) { //тарфикация не задана - не смогли оценить !!!p.validating_company
          continue;
        }
        TPaxSegRFISCKey key;
        TElemFmt fmt;
        key.airline = ElemToElemId( etAirline, p.validating_company, fmt );
        SvcValue svc;
        SvcEmdSvcsAns::getSvcValue(p.svc_id,svc);
        if ( paxs.find(svc.pass_id) == paxs.end() ) {
          throw EXCEPTIONS::Exception( "Invalid data XML astra pax_id not identical sirena pax_id=%s", svc.pass_id.c_str() );
        }
        tst();
        key.pax_id = paxs[svc.pass_id];
        if ( trfer_nums.find(svc.seg_id) == trfer_nums.end() ) {
          throw EXCEPTIONS::Exception( "Invalid data XML sirena seg_num not found in astra routes, seg_num=%s", svc.seg_id.c_str() );
        }
        key.trfer_num = trfer_nums[svc.seg_id];
        key.RFISC = svc.rfisc;
        key.service_type = ServiceTypes().decode(svc.service_type);
        TPriceServiceItem price(key,paxsNames.getPaxName(key.pax_id),std::map<std::string,SvcFromSirena>());
        SvcFromSirena svcSirena;
        svcSirena.doc = prices.getDoc(p.svc_id);
        svcSirena.pass_id = svc.pass_id;
        svcSirena.price = p.total;
        svcSirena.currency =  ElemToElemId( etCurrency, p.currency, fmt );
        svcSirena.seg_id = svc.seg_id;
        svcSirena.name = svc.name;
        svcSirena.status_svc = svc.status;
        svcSirena.status_direct = TPriceRFISCList::STATUS_DIRECT_ORDER;
        svcSirena.svc_id = p.svc_id;

        SvcValue svcVal;
        SvcEmdSvcsAns::getSvcValue( p.svc_id, svcVal );
        Ticket t;
        tst();
        tickets.get( svcVal.pass_id, svcVal.seg_id, t );
        tst();
        svcSirena.ticknum = t.ticknum;
        svcSirena.ticket_cpn = t.ticket_cpn;
        LogTrace(TRACE5) << "svc_id=" << svcSirena.svc_id << ",ticket=" << svcSirena.ticknum << "/" << svcSirena.ticket_cpn;

        svcSirena.time_change = BASIC::date_time::NowUTC();
        if ( svcList.find(key) == svcList.end() ) {
          svcList.emplace( key, price );
        }
        svcList[key].addSVCS( svcSirena.svc_id, svcSirena );
      }
      for ( auto &l : svcList ) {
        LogTrace(TRACE5) << l.second.traceStr();
      }
    }

/*    void convertFromSirena( int grp_id ) {
//      get(grp_id);
      tst();
      //fromSirena(grp_id);
    }*/

/*    void toAstraSVCPrices( AstraSVCPrices &astraPrices ) {
      astraPrices.svcs.clear();
      for ( const auto &p : PriceRFISCList ) {
        for ( const auto &svc_id : p.second.svcs ) {
          AstraSVCPrices::AstraService service(p.first);
          service.svc_id = svc_id;
          service.doc_id = prices.getDocId(svc_id);
          Svc svc;
          svcs.get(svc_id,svc);
          service.pass_id = svc.pass_id;
          service.seg_id = svc.seg_id;
          service.price = p.second.price;
          service.currency = p.second.currency;
          service.status_svc = svc.status;
          service.status_direct = "order";mnm,//уже есть какой-то статус
          astraPrices.svcs.insert(make_pair(service,service));
        }
      }
      //astraPrices.PriceRFISCList = PriceRFISCList;
    }*/

};


// Создание PNR с внесистемными сегментами, соответствующего группе регистрации в Астре (в просторечьи - "+ПБ") - check_in_get_pnr
class CheckInGetPNRReq: public SWC::SWCExchange
{
  private:
    std::map<int,PointGrpPaxs> params; // seg_no,pax_id
    int version;
  public:
    virtual std::string exchangeId() const {
      return "check_in_get_pnr";
    }
  protected:
    virtual bool isRequest() const {
      return true;
    }

  public:
    void clear() {
      params.clear();
    }
    virtual void fromXML(xmlNodePtr reqNode) {}

    virtual void toXML(xmlNodePtr node) const {
      if ( params.empty() ) {
        tst();
        throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA"); //!!!
      }
      xmlNodePtr n = NewTextChild( node, exchangeId().c_str() );
      for ( const auto &sp : params ) {
        if ( version == 0 ) {
          xmlNodePtr segNode = NewTextChild(n,"segment");
          SetProp( segNode, "point_id", sp.second.point_id );
          for ( const auto &p : sp.second.paxs ) {
            xmlNodePtr n = NewTextChild(segNode,"passenger");
            SetProp( n, "crs_pax_id", p.first );
            SetProp( n, "pax_key_id", p.second );
          }
        }
        else {
          NewTextChild( n, "group_id", sp.second.grp_id );
        }
      }
    }
    CheckInGetPNRReq( const std::map<int,PointGrpPaxs>& _params, int _version=0 ) : params(_params), version(_version) {}
    ~CheckInGetPNRReq() {}
};


class CheckInGetPNRRes: public SWC::SWCExchange, public SvcEmdRegnum
{
  private:
    std::string surname;
  protected:
    virtual bool isRequest() const {
      return false;
    }
  public:
    void clear() {
      SvcEmdRegnum::clear();
      surname.clear();
    }
    virtual std::string exchangeId() const {
      return "check_in_get_pnr";
    }
    virtual void fromXML(xmlNodePtr reqNode) {
      SvcEmdRegnum::fromXML(reqNode,SvcEmdRegnum::Enum::noneStyle);
      surname = NodeAsString( "surname", reqNode );
    }
    std::string &getSurname() {
      return surname;
    }
    ~CheckInGetPNRRes() {}
};

class OrderReq: public SWC::SWCExchange, public SvcEmdRegnum
{
  public:
    virtual std::string exchangeId() const {
      return "order";
    }
    virtual void clear() {
       SvcEmdRegnum::clear();
    }
  protected:
    virtual bool isRequest() const {
      return true;
    }
  private:
    std::string surname;
  public:
     OrderReq( const SvcEmdRegnum& _regnum,
               const std::string& _surname ) : SvcEmdRegnum(_regnum), surname(_surname) {}
    virtual void toXML(xmlNodePtr node) const {
      if ( SvcEmdRegnum::empty() ) {
        throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA"); //!!!
      }
      xmlNodePtr n = NewTextChild( node, exchangeId().c_str() );
      SvcEmdRegnum::toXML(n,SvcEmdRegnum::Enum::noneStyle);
      NewTextChild( n, "surname", surname );
      n = NewTextChild( n, "answer_params" );
      NewTextChild( n, "tickinfo", "true" );
    }
    virtual void fromXML(xmlNodePtr reqNode) {}
    ~OrderReq(){}
};


class OrderRes: public SWC::SWCExchange, public Order
{
  public:
    virtual std::string exchangeId() const {
      return "order";
    }
    virtual void clear() {
       Order::clear();
    }
  protected:
    virtual bool isRequest() const {
      return false;
    }
  public:
    virtual void toXML(xmlNodePtr node) const {}
    virtual void fromXML(xmlNodePtr reqNode) {
      Order::fromXML(reqNode);
    }
    ~OrderRes(){}
};

/*class ContextManagement {
  private:
    std::string getContextKey( ) {
      return TReqInfo::Instance()->desk.code + "_SWC_" + getState();
    }
  public:
    enum Enum
    {
       svc_emd_issue_query = 0,
       svc_emd_issue_confirm = 1,
       svc_emd_void = 2,
       unknown = ASTRA::NoExists
    };

    std::string getState() const {
     return IntToString((int)Decode(exchangeId()));
    }
    virtual std::string exchangeId() const = 0;
    virtual void toXML( xmlNodePtr rootNode ) const = 0;
    virtual bool isRequest() const = 0;
    virtual void fromXML(xmlNodePtr rootNode) = 0;

    static Enum Decode( const std::string &stat ) {
      if ( stat == "svc_emd_issue_query" )
        return svc_emd_issue_query;
      if ( stat == "svc_emd_issue_confirm" )
        return svc_emd_issue_confirm;
      if ( stat == "svc_emd_void" ) {
        return svc_emd_void;
      }
      return unknown;
    }

    void loadXML() {
      std::string value;
      AstraContext::GetContext( getContextKey(), 0, value );
      LogTrace(TRACE5) << value;
      value = ConvertCodepage(value,"CP866", "UTF-8");
      LogTrace(TRACE5) << value;
      XMLDoc doc(value);
      xml_decode_nodelist(doc.docPtr()->children);
      std::string root = string("/") + std::string(isRequest()?"request":"answer") + "/" + exchangeId();
      xmlNodePtr rootNode = NodeAsNode(root.c_str(), doc.docPtr());
      fromXML(rootNode);
      tst();
    }
    void saveXML() {
      saveXML(nullptr);
    }
    void saveXML(xmlDocPtr doc) {
      std::string value;
      if ( doc == nullptr ) {
        xmlNodePtr rootNode;
        XMLDoc _doc(isRequest()?"request":"answer",rootNode,__func__);
        tst();
        toXML( rootNode );
        value = XMLTreeToText(_doc.docPtr() );
      }
      else {
        tst();
        value = XMLTreeToText(doc);
      }
      LogTrace(TRACE5) << value;
      //value = ConvertCodepage(value,"UTF-8","CP866");
      //LogTrace(TRACE5) << value;
      AstraContext::ClearContext(getContextKey(),0);
      AstraContext::SetContext(getContextKey(),0,value);
    }
    virtual ~ContextManagement(){}
};*/

class SvcEmdIssueReq: public SWC::SWCExchange, public SvcEmdRegnum, public SvcEmdPayDoc, public SvcEmdSvcsReq
{
  public:
    virtual std::string exchangeId() const {
      return "svc_emd_issue_query";
    }
    virtual void clear() {
       SvcEmdRegnum::clear();
       SvcEmdPayDoc::clear();
       SvcEmdSvcsReq::clear();
    }
  protected:
    virtual bool isRequest() const {
      return true;
    }
  public:
    SvcEmdIssueReq() {
      clear();
    }
    SvcEmdIssueReq(const SvcEmdRegnum& _regnum,
                   const SvcEmdPayDoc& _paydoc,
                   const SvcEmdSvcsReq& _svcs ) : SvcEmdRegnum(_regnum), SvcEmdPayDoc(_paydoc), SvcEmdSvcsReq(_svcs){}
    virtual void toXML(xmlNodePtr node) const {
      if ( SvcEmdRegnum::empty()  ) {
        throw AstraLocale::UserException("MSG.REGNUM_NOT_SET_REPEAT_AVALUATION");
      }
      if ( SvcEmdPayDoc::empty() ) {
        throw AstraLocale::UserException("MSG.EMD.PAYDOC_NOT_SET");
      }
      node = NewTextChild( node, exchangeId().c_str() );
      SvcEmdRegnum::toXML(node,SvcEmdRegnum::Enum::propStyle);
      SvcEmdPayDoc::toXML(node);
      SvcEmdSvcsReq::toXML(node);
    }
    virtual void fromXML(xmlNodePtr reqNode) {}
    ~SvcEmdIssueReq(){}
};

class SvcEmdIssueRes: public SvcEmdCommonRes, public SvcEmdCost, public SvcEmdTimeout
{
  public:
    virtual void clear() {
       regnum.clear();
       SvcEmdCost::clear();
       SvcEmdTimeout::clear();
    }
  private:
    std::string regnum;
  public:
    SvcEmdIssueRes():SvcEmdCommonRes("svc_emd_issue_query"){}
    virtual void fromXML(xmlNodePtr reqNode) {
      clear();
      regnum = NodeAsString( "@regnum",reqNode );
      SvcEmdTimeout::fromXML(reqNode);
      SvcEmdCost::fromXML(reqNode);
    }
    std::string getRegnum() {
      return regnum;
    }

    ~SvcEmdIssueRes(){}
};

class SvcEmdIssueConfirmReq: public SWC::SWCExchange, public SvcEmdRegnum, public SvcEmdPayDoc, public SvcEmdCost
{
  public:
    virtual std::string exchangeId() const {
      return "svc_emd_issue_confirm";
    }
    virtual void clear() {
      SvcEmdPayDoc::clear();
      SvcEmdRegnum::clear();
      SvcEmdCost::clear();
    }
  protected:
    virtual bool isRequest() const {
      return true;
    }
  public:
    virtual void toXML(xmlNodePtr node) const {
      node = NewTextChild( node, exchangeId().c_str() );
      SvcEmdPayDoc::toXML(node);
      SvcEmdRegnum::toXML(node, SvcEmdRegnum::Enum::propStyle);
      SvcEmdCost::toXML(node);
    }
    virtual void fromXML(xmlNodePtr reqNode) {}
    SvcEmdIssueConfirmReq( const SvcEmdRegnum &_regnum, const SvcEmdPayDoc& _paydoc, const SvcEmdCost& _cost ) : SvcEmdRegnum(_regnum), SvcEmdPayDoc(_paydoc), SvcEmdCost(_cost){}
    ~SvcEmdIssueConfirmReq(){}
};

class SvcEmdIssueConfirmRes: public SWC::SWCExchange, public SvcEmdRegnum, public SvcEmdTimeout, public SvcEmdSvcsAns
{
  public:
    virtual std::string exchangeId() const {
      return "svc_emd_issue_confirm";
    }
    virtual void clear() {
       SvcEmdRegnum::clear();
       SvcEmdTimeout::clear();
       book_time = ASTRA::NoExists;
       agn.clear();
       SvcEmdSvcsAns::clear();
       tickets.clear();
    }
  protected:
    virtual bool isRequest() const {
      return false;
    }
  private:
    TDateTime book_time;
    std::string agn;
    Tickets tickets;
  public:
    virtual void toXML(xmlNodePtr node) const {}
    virtual void fromXML(xmlNodePtr reqNode) {
      clear();
      SvcEmdRegnum::fromXML(reqNode,SvcEmdRegnum::Enum::propStyle);
      SvcEmdTimeout::fromXML(reqNode);
      book_time = NodeAsDateTime( "book_time","dd.mm.yyyy hh:nn",reqNode);
      agn = NodeAsString("agn",reqNode,"");
      SvcEmdSvcsAns::fromXML(reqNode);
      tickets.fromXML(reqNode);
      for ( auto &s : *this ) {
        Ticket t;
        tickets.get( s.id, t );
        s.ticket_cpn = t.ticket_cpn;
        s.ticknum = t.ticknum;
      }
    }
    ~SvcEmdIssueConfirmRes(){}
};

class SvcEmdVoidReq: public SWC::SWCExchange, public SvcEmdRegnum, public SvcEmdSvcsReq
{
  public:
    virtual std::string exchangeId() const {
      return "svc_emd_void";
    }
    virtual void clear() {
      SvcEmdRegnum::clear();
      SvcEmdSvcsReq::clear();
    }
  protected:
    virtual bool isRequest() const {
      return true;
    }
  public:
    SvcEmdVoidReq(const SvcEmdRegnum& _regnum,
                  const SvcEmdSvcsReq& _svcs) : SvcEmdRegnum(_regnum),SvcEmdSvcsReq(_svcs){}
    virtual void toXML(xmlNodePtr node) const {
      node = NewTextChild( node, exchangeId().c_str() );
      SvcEmdRegnum::toXML(node, SvcEmdRegnum::Enum::propStyle);
      SvcEmdSvcsReq::toXML(node);
    }
    virtual void fromXML(xmlNodePtr reqNode) {
    }
    ~SvcEmdVoidReq(){}
};


class SvcEmdVoidRes: public SvcEmdCommonRes, public SvcEmdRegnum
{
  private:
    TDateTime utc_timelimit;
    TDateTime book_time;
    std::string agn;
  public:
    const static std::string getExchangeId() {
      return "svc_emd_void";
    }
    virtual std::string exchangeId() const {
      return getExchangeId();
    }
    SvcEmdVoidRes():SvcEmdCommonRes(SvcEmdVoidRes::getExchangeId()){}
    virtual void fromXML(xmlNodePtr reqNode) {
      SvcEmdRegnum::fromXML(reqNode,SvcEmdRegnum::Enum::propStyle);
      utc_timelimit = NodeAsDateTime( "utc_timelimit","hh:nn dd.mm.yyyy",reqNode);
      book_time = NodeAsDateTime( "book_time","dd.mm.yyyy hh:nn",reqNode);
      agn = NodeAsString("agn",reqNode,"");
    }
    ~SvcEmdVoidRes(){}
};


class SvcEmdIssueCancelReq: public SWC::SWCExchange, public SvcEmdRegnum
{
  public:
    virtual std::string exchangeId() const {
      return "svc_emd_issue_cancel";
    }
    virtual void clear() {
      SvcEmdRegnum::clear();
    }
  protected:
    virtual bool isRequest() const {
      return true;
    }
  public:
    SvcEmdIssueCancelReq(const SvcEmdRegnum& _regnum) : SvcEmdRegnum(_regnum){}
    virtual void toXML(xmlNodePtr node) const {
      node = NewTextChild( node, exchangeId().c_str() );
      SvcEmdRegnum::toXML(node, SvcEmdRegnum::Enum::propStyle);
    }
    virtual void fromXML(xmlNodePtr reqNode) {
    }
    ~SvcEmdIssueCancelReq(){}
};

class SvcEmdIssueCancelRes: public SvcEmdCommonRes, public SvcEmdRegnum, public SvcEmdSvcsAns
{
  public:
    TDateTime utc_timelimit;
    TDateTime book_time;
    std::string agn;
  public:
    const static std::string getExchangeId() {
      return "svc_emd_issue_cancel";
    }
    virtual std::string exchangeId() const {
      return getExchangeId();
    }
    SvcEmdIssueCancelRes():SvcEmdCommonRes(SvcEmdIssueCancelRes::getExchangeId()){}
    virtual void fromXML(xmlNodePtr reqNode) {
      SvcEmdRegnum::fromXML(reqNode,SvcEmdRegnum::Enum::propStyle);
      utc_timelimit = NodeAsDateTime( "utc_timelimit","hh:nn dd.mm.yyyy",reqNode);
      book_time = NodeAsDateTime( "book_time","dd.mm.yyyy hh:nn",reqNode);
      agn = NodeAsString("agn",reqNode,"");
      SvcEmdSvcsAns::fromXML(reqNode);
    }
    ~SvcEmdIssueCancelRes(){}
};

class SvcEmdRefundQryReq: public SWC::SWCExchange, public SvcEmdRegnum, SvcEmdSvcsReq
{
  public:
    virtual std::string exchangeId() const {
      return "svc_emd_refund_query";
    }
    virtual void clear() {
      SvcEmdRegnum::clear();
      SvcEmdSvcsReq::clear();
    }
  protected:
    virtual bool isRequest() const {
      return true;
    }
  public:
    SvcEmdRefundQryReq( const SvcEmdRegnum& _regnum, const SvcEmdSvcsReq& _svcs ) : SvcEmdRegnum(_regnum), SvcEmdSvcsReq(_svcs) {}
    virtual void toXML(xmlNodePtr node) const {
      node = NewTextChild( node, exchangeId().c_str() );
      SvcEmdRegnum::toXML(node,SvcEmdRegnum::Enum::propStyle);
      SvcEmdSvcsReq::toXML(node);
    }
    virtual void fromXML(xmlNodePtr reqNode) {}
    ~SvcEmdRefundQryReq(){}
};

class SvcEmdRefundQryRes: public SvcEmdCommonRes, public SvcEmdCost, public SvcEmdTimeout
{
  private:
    std::string regnum;
  public:
    const static std::string getExchangeId() {
      return "svc_emd_refund_query";
    }
    virtual std::string exchangeId() const {
      return getExchangeId();
    }
    virtual void clear() {
       regnum.clear();
       SvcEmdCost::clear();
       SvcEmdTimeout::clear();
    }
  public:
    SvcEmdRefundQryRes():SvcEmdCommonRes(SvcEmdRefundQryRes::getExchangeId()){}
    virtual void fromXML(xmlNodePtr reqNode) {
      clear();
      regnum = NodeAsString( "@regnum",reqNode );
      SvcEmdCost::fromXML(reqNode);
      SvcEmdTimeout::fromXML(reqNode);
    }
    ~SvcEmdRefundQryRes(){}
};

class SvcEmdRefundConfirmReq: public SWC::SWCExchange, public SvcEmdRegnum, public SvcEmdCost, SvcEmdSvcsReq
{
  protected:
    virtual bool isRequest() const {
      return true;
    }
  public:
    const static std::string getExchangeId() {
      return "svc_emd_refund_confirm";
    }
    virtual void clear() {
      SvcEmdRegnum::clear();
      SvcEmdCost::clear();
      SvcEmdSvcsReq::clear();
    }
    virtual std::string exchangeId() const {
      return getExchangeId();
    }
    SvcEmdRefundConfirmReq( const SvcEmdRegnum& _regnum, const SvcEmdCost& _cost, const SvcEmdSvcsReq& _svcs ) : SvcEmdRegnum(_regnum), SvcEmdCost(_cost), SvcEmdSvcsReq(_svcs) {}
    virtual void toXML(xmlNodePtr node) const {
      node = NewTextChild( node, exchangeId().c_str() );
      SvcEmdRegnum::toXML(node,SvcEmdRegnum::Enum::propStyle);
      SvcEmdCost::toXML(node);
      SvcEmdSvcsReq::toXML(node);
    }
    virtual void fromXML(xmlNodePtr reqNode) {}
    ~SvcEmdRefundConfirmReq(){}
};

class SvcEmdRefundConfirmRes: public SvcEmdCommonRes, public SvcEmdRegnum
{
  public:
    const static std::string getExchangeId() {
      return "svc_emd_refund_confirm";
    }
    virtual std::string exchangeId() const {
      return getExchangeId();
    }
    SvcEmdRefundConfirmRes():SvcEmdCommonRes(SvcEmdRefundConfirmRes::getExchangeId()){}
    virtual void fromXML(xmlNodePtr reqNode) {
      SvcEmdRegnum::fromXML(reqNode,SvcEmdRegnum::Enum::propStyle);
    }
    ~SvcEmdRefundConfirmRes(){}
};


//-----------------------------------------------------------------------------------------------
void ServiceEvalInterface::RequestFromGrpId(xmlNodePtr reqNode, int grp_id, SWC::SWCExchange& req)
{
  TTripInfo info;
  if (!info.getByGrpId(grp_id))
    throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText=
    "SELECT client_id, pr_denial, "
    "    DECODE(airline,NULL,0,8)+ "
    "    DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM pay_clients "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("airline",otString,info.airline);
  Qry.CreateVariable("airp_dep",otString,info.airp);
  Qry.Execute();
  if (Qry.Eof || Qry.FieldAsInteger("pr_denial")!=0) {
    throw AstraLocale::UserException("MSG.CLIENT_ID.NOT_DEFINE");
  }
  Request( reqNode, Qry.FieldAsInteger("client_id"), getServiceName(), req );
}



void ServiceEvalInterface::Evaluation(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << "ServiceEvalInterface::" << __func__;
  int grp_id=NodeAsInteger("grp_id",reqNode);
  bool pr_reset = (NodeAsInteger("pr_reset",reqNode,0) != 0);
  CheckIn::TSimplePaxGrpItem grpItem;
  if ( !grpItem.getByGrpId(grp_id) ) {
    throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
  }
  if ( !isPaymentAtDesk(grpItem.point_dep) ) {
    throw AstraLocale::UserException("MSG.EMD.NOT_EVALUATION_SETS");
  }
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  if ( prices.notInit() || pr_reset ) {
    std::map<int,PointGrpPaxs> params;
    if ( !isPaymentAtDesk(grpItem.point_dep) ) {
      throw AstraLocale::UserException("MSG.EMD.NOT_COST_SETS");
    }
    SegsPaxs segsPaxs;
    try {
      segsPaxs.fromDB(grp_id,grpItem.point_dep);
    }
    catch( AstraLocale::UserException& ex ) {
      if ( ex.getLexemaData().lexema_id == "MSG.EMD.SERVICES_ALREADY_PAID" &&
           NodeAsInteger("pr_after_print",reqNode,0) != 0 ) {
        NewTextChild(resNode, "finish");
        return;
      }
      throw;
    }
    segsPaxs.getPaxs(params);
    CheckInGetPNRReq req(params,1);
    RequestFromGrpId( reqNode, grp_id, req );
  }
  else {
    OrderReq orderReq( prices.getSvcEmdRegnum(), prices.getSurname() );
    tst();
    RequestFromGrpId( reqNode, grp_id, orderReq );
  }
}

void ServiceEvalInterface::response_check_in_get_pnr(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  CheckInGetPNRRes res;
  if ( res.isEmptyAnswer(externalSysResNode)) {
    AstraLocale::showProgError("MSG.SWC_CONNECT_ERROR");
    return;
  }
  res.parseResponse(externalSysResNode);
  if ( res.error() ) {
    res.errorToXML(resNode);
    return;
  }
  int grp_id=NodeAsInteger("grp_id",reqNode);
  TPaidRFISCList PaidRFISCList;
  PaidRFISCList.fromDB(grp_id,true);
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  prices.setSvcEmdRegnum(res.getSvcEmdRegnum());
  LogTrace(TRACE5)<< res.getSvcEmdRegnum().toString();
  LogTrace(TRACE5)<< prices.getSvcEmdRegnum().toString();
  prices.setSurname(res.getSurname());
  prices.setServices(PaidRFISCList);
  prices.toContextDB(grp_id);

  OrderReq orderReq(res.getSvcEmdRegnum(), res.getSurname());
  RequestFromGrpId( reqNode, grp_id, orderReq );
  tst();
}

void ServiceEvalInterface::response_order(const std::string& exchangeId,xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  OrderRes res;
  if ( res.isEmptyAnswer(externalSysResNode)) {
    AstraLocale::showProgError("MSG.SWC_CONNECT_ERROR");
    return;
  }
  res.parseResponse(externalSysResNode);
  if ( res.error() ) {
    res.errorToXML(resNode);
    return;
  }
  int grp_id=NodeAsInteger("grp_id",reqNode);
  CheckIn::TSimplePaxGrpItem grpItem;
  if ( !grpItem.getByGrpId(grp_id) ) {
    throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
  }
  if ( !isPaymentAtDesk(grpItem.point_dep) ) {
    throw AstraLocale::UserException("MSG.EMD.NOT_COST_SETS");
  }

  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  TPriceRFISCList list;
  res.toPrice(grp_id,grpItem.point_dep,list);
  prices.synchFromSirena(list);
  prices.toContextDB(grp_id);
  xmlNodePtr node = NewTextChild(resNode,"prices");
  NewTextChild(node,"grp_id",grp_id);
  prices.toXML(NewTextChild( node, "services" ));
  if ( GetNode("prices/services/items/item",resNode) == nullptr ) { //а есть ли значения
    tst();
    if ( NodeAsInteger("pr_after_print",reqNode,0) != 0 ) {
      NewTextChild(resNode, "finish");
      return;
    }
    throw AstraLocale::UserException("MSG.EMD.SERVICES_ALREADY_PAID");
  }
}

void ServiceEvalInterface::Filtered(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << "ServiceEvalInterface::" << __func__;
  int grp_id=NodeAsInteger("grp_id",reqNode);
  CheckIn::TSimplePaxGrpItem grpItem;
  if ( !grpItem.getByGrpId(grp_id) ) {
    throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
  }
  if ( !isPaymentAtDesk(grpItem.point_dep) ) {
    throw AstraLocale::UserException("MSG.EMD.NOT_EVALUATION_SETS");
  }
  xmlNodePtr servicesNode = GetNode( "services",reqNode);
  if ( servicesNode == nullptr || servicesNode->children == nullptr ) {
    throw AstraLocale::UserException("MSG.EMD.NOT_SERVICES_CHOICE");
  }
  TPriceRFISCList FilteredPriceRFISCList, PriceRFISCList;
  FilteredPriceRFISCList.fromXML(servicesNode);
  PriceRFISCList.fromContextDB(grp_id);
  PriceRFISCList.synchFromSirena(FilteredPriceRFISCList,true);
  tst();
  xmlNodePtr node = NewTextChild(resNode,"prices");
  NewTextChild(node,"grp_id",grp_id);
  tst();
  PriceRFISCList.toXML(NewTextChild( node, "services" ));
  tst();
}

void ServiceEvalInterface::backPaid(const std::string& exchangeId,xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << __func__ << ",exchangeId=" << exchangeId <<",externalSysResNode=" << externalSysResNode;
  int grp_id=NodeAsInteger("grp_id",reqNode);
  TPriceRFISCList prices;
  bool inRequest = false;

  if ( externalSysResNode == nullptr ) { // откат
    prices.fromContextDB(grp_id);
    std::vector<std::string> svcs;
    bool cancelRequest;
    std::shared_ptr<SWC::SWCExchange> req;
    if ( (cancelRequest=prices.haveStatusDirect( TPriceRFISCList::STATUS_DIRECT_ISSUE_ANSWER, svcs )) ||
         prices.haveStatusDirect( TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM, svcs ) ) {
      if (cancelRequest) {
        req = std::make_shared<SvcEmdIssueCancelReq>(prices.getSvcEmdRegnum());
      }
      else {
        req = make_shared<SvcEmdVoidReq>(prices.getSvcEmdRegnum(),SvcEmdSvcsReq(svcs));
      }
      //???req->fromDB();
      RequestFromGrpId( reqNode, grp_id, *req.get() );
      inRequest = true;
    }
    if ( inRequest ) {
      return;
    }
  }
  else { //answer from SvcEmdVoidReq OR SvcEmdIssueCancelReq
    std::shared_ptr<SvcEmdCommonRes> res;
    if ( SvcEmdVoidRes::getExchangeId() == exchangeId ) {
      res = std::make_shared<SvcEmdVoidRes>();
    }
    if ( SvcEmdIssueCancelRes::getExchangeId() == exchangeId ) {
      res = std::make_shared<SvcEmdIssueCancelRes>();
    }
    if ( res == nullptr ) {
      return;
    }
    prices.Lock(grp_id); //в разных обработчиках параллельные запросы???
    prices.fromContextDB(grp_id);

    try {
      LogTrace(TRACE5) << "parse " << res->exchangeId();
      res->parseResponse(externalSysResNode);
      if ( res->error() ) {
        throw EXCEPTIONS::Exception("error_message=%s, error_code=%s", res->error_message.c_str(),res->error_code.c_str());
      }
      tst();
    }
    catch(EXCEPTIONS::Exception &e) {
      LogError(STDLOG) << e.what();
      res->error_message = e.what(); //!!! зачем пользователю знать текст ошибки, наверное надо: Ошибка работы программы
      prices.setError(*res);
    }
    catch(...) {
      res->error_message = AstraLocale::getLocaleText( "MSG.SWC_CONNECT_ERROR" ); //???
      LogError(STDLOG) << __func__ <<  " some error";
      prices.setError(*res);
    }
    prices.setStatusDirect(res->exchangeId() == SvcEmdVoidRes::getExchangeId()?TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM:TPriceRFISCList::STATUS_DIRECT_ISSUE_ANSWER,TPriceRFISCList::STATUS_DIRECT_ORDER);
    prices.toContextDB(grp_id);
    ASTRA::commit(); //!!!
    std::vector<std::string> svcs;
    if ( SvcEmdIssueCancelRes::getExchangeId() == exchangeId &&
         prices.haveStatusDirect( TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM, svcs ) ) {
      SvcEmdVoidReq req(prices.getSvcEmdRegnum(),SvcEmdSvcsReq(svcs));
      RequestFromGrpId( reqNode, grp_id, req );
      return;
    }
  }
  //все откатили или нечего было откатывать
  //надо взять +ПБ заново, чтобы восстановить услуги из Астры, для этого очистим инфу по группе, выдаем сообщение клиенту
  prices.fromContextDB(grp_id);
  if ( !prices.haveStatusDirect( TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM ) &&
       !prices.haveStatusDirect( TPriceRFISCList::STATUS_DIRECT_ISSUE_ANSWER ) ) {
    LogTrace(TRACE5) << __func__ << " rollback STATUS_DIRECT_ISSUE_CONFIRM and STATUS_DIRECT_ISSUE_ANSWER";
    prices.toContextDB(grp_id,true);
    TPriceRFISCList prices1;
    //prices.fromContextDB(grp_id);
    xmlNodePtr node = NewTextChild(resNode,"prices");
    NewTextChild(node,"grp_id",grp_id);
    prices1.toXML(NewTextChild( node, "services" )); //пустой запрос
    AstraLocale::showErrorMessage("MSG.EMD.PAID_ERROR_REFRESH_DATA",
                                  LParams() << LParam("error", prices.getErrorMessage()) << LParam("code",prices.getErrorCode()) );
  }
}

void ServiceEvalInterface::Paid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << "ServiceEvalInterface::" << __func__;
  int grp_id=NodeAsInteger("grp_id",reqNode);
  CheckIn::TSimplePaxGrpItem grpItem;
  if ( !grpItem.getByGrpId(grp_id) ) {
    throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
  }
  if ( !isPaymentAtDesk(grpItem.point_dep) ) {
    throw AstraLocale::UserException("MSG.EMD.NOT_EVALUATION_SETS");
  }
// не требуется отката - переходим к оплате
  xmlNodePtr servicesNode = GetNode( "services",reqNode);
  if ( servicesNode == nullptr || servicesNode->children == nullptr ) {
    throw AstraLocale::UserException("MSG.EMD.NOT_SERVICES_CHOICE");
  }
  TPriceRFISCList termPriceRFISCList;
  tst();
  termPriceRFISCList.fromXML(servicesNode);
  if ( !termPriceRFISCList.terminalChoiceAny() ) {
    throw AstraLocale::UserException("MSG.EMD.NOT_SERVICES_CHOICE");
  }
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  tst();
  prices.setStatusDirect( TPriceRFISCList::STATUS_DIRECT_SELECTED, TPriceRFISCList::STATUS_DIRECT_ORDER );
  tst();
  prices.setStatusDirect( TPriceRFISCList::STATUS_DIRECT_ISSUE_QUERY, TPriceRFISCList::STATUS_DIRECT_ORDER );
  tst();
  if ( !prices.filterFromTerminal(termPriceRFISCList) ) {
    throw EXCEPTIONS::Exception( "filtered cvs from terminal not found in prices context" );
  }
  tst();
  prices.setSvcEmdPayDoc( SvcEmdPayDoc( "IN", "" ) );
  prices.toContextDB(grp_id);
  std::vector<std::string> svcs;
  prices.haveStatusDirect(TPriceRFISCList::STATUS_DIRECT_SELECTED,svcs);
  if ( svcs.empty() ) {
    throw AstraLocale::UserException("MSG.EMD.NOT_SERVICES_CHOICE");
  }
  continuePaidRequest(reqNode, resNode);
//  backPaid(reqNode,NULL,resNode); // откат всех оплат, которые не завершилось
}

void ServiceEvalInterface::PayDocParamsRequest(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id=NodeAsInteger("point_id",reqNode);
  if ( !isManualPayAtDest( point_id ) ) {
    return;
  }
  xmlNodePtr node = NewTextChild( resNode, "items" );
  xmlNodePtr itemNode = NewTextChild( node, "item" );
  NewTextChild( itemNode, "Caption", "Номер чека" );
  NewTextChild( itemNode, "MaxLength", 10 );
  NewTextChild( itemNode, "CharCase", "UpperCase" );
  NewTextChild( itemNode, "Width", 120 );
  NewTextChild( itemNode, "Requred", 1 );
  NewTextChild( itemNode, "Name", "Edit1" );

  itemNode = NewTextChild( node, "item" );
  NewTextChild( itemNode, "Caption", "Номер карты" );
  NewTextChild( itemNode, "MaxLength", 4 );
  NewTextChild( itemNode, "CharCase", "UpperCase" );
  NewTextChild( itemNode, "Width", 40 );
  NewTextChild( itemNode, "Requred", 1 );
  NewTextChild( itemNode, "Name", "Edit2" );

  itemNode = NewTextChild( node, "item" );
  NewTextChild( itemNode, "Caption", "Номер транзакции" );
  NewTextChild( itemNode, "MaxLength", 10 );
  NewTextChild( itemNode, "CharCase", "UpperCase" );
  NewTextChild( itemNode, "Width", 120 );
  NewTextChild( itemNode, "Requred", 1 );
  NewTextChild( itemNode, "Name", "Edit3" );

}

void ServiceEvalInterface::PayDocParamsAnswer(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  tst();

  //проверка и запись введенных данных по оплате
}


void ServiceEvalInterface::continuePaidRequest(xmlNodePtr reqNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << __func__;
  int grp_id=NodeAsInteger("grp_id",reqNode);
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  std::vector<std::string> svcs;
  if ( !prices.getNextIssueQueryGrpEMD( svcs ) ) {
    prices.setStatusDirect(TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM,TPriceRFISCList::STATUS_DIRECT_PAID);
    prices.toDB(grp_id);
    prices.SvcEmdPayDoc::clear();
    //prices.SvcEmdCost::clear();
    prices.SvcEmdTimeout::clear();
    prices.toContextDB(grp_id);
    EMDAutoBoundInterface::EMDRefresh(EMDAutoBoundGrpId(grp_id), reqNode);
    return;
  }
  LogTrace(TRACE5) << "svcs.size=" << svcs.size();
  SvcEmdIssueReq req(prices.getSvcEmdRegnum(), prices.getSvcEmdPayDoc(), SvcEmdSvcsReq(svcs));
  tst();
  prices.toContextDB(grp_id);
  ASTRA::commit();
  RequestFromGrpId( reqNode, grp_id, req );
  tst();
}

void ServiceEvalInterface::AfterPaid(xmlNodePtr reqNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << __func__;
  int grp_id=NodeAsInteger("grp_id",reqNode);
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id); //???
  xmlNodePtr node = NewTextChild(resNode,"prices");
  NewTextChild(node,"grp_id",grp_id);
  prices.toXML(NewTextChild( node, "services" ));
  NewTextChild(resNode,"pr_print");
  SetProp( NewTextChild(resNode,"POSExchange"), "key", grp_id );

  AstraLocale::showMessage( "MSG.EMD.PAID_FINISHED" );
}

void checkRequestErrorAndRollback( int grp_id, const SirenaExchange::TExchange& ex )
{
  if ( !ex.error() ) {
    return;
  }
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  prices.setError(ex);
  prices.toContextDB(grp_id);
  throw EXCEPTIONS::Exception("rollback pay error_message=%s, error_code=%s", ex.error_message.c_str(),ex.error_code.c_str());
}

void ServiceEvalInterface::response_svc_emd_issue_query(const std::string& exchangeId,xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  SvcEmdIssueRes res;

  try {
    LogTrace(TRACE5) << __func__;
    res.parseResponse(externalSysResNode);
    int grp_id=NodeAsInteger("grp_id",reqNode);
    checkRequestErrorAndRollback( grp_id, res );
    TPriceRFISCList prices;
    prices.fromContextDB(grp_id);
    prices.setStatusDirect(TPriceRFISCList::STATUS_DIRECT_ISSUE_QUERY,TPriceRFISCList::STATUS_DIRECT_ISSUE_ANSWER);
    prices.setSvcEmdCost( res.getSvcEmdCost() );
    prices.toContextDB(grp_id);
    SvcEmdIssueConfirmReq reqConfirm( SvcEmdRegnum(res.getRegnum()),  prices.getSvcEmdPayDoc(), res.getSvcEmdCost() );

    ASTRA::commit(); //!!!
    RequestFromGrpId( reqNode, grp_id, reqConfirm );
    tst();
  }
  catch(EXCEPTIONS::Exception &e) {
    LogError(STDLOG) << e.what();
    backPaid("",reqNode,nullptr,resNode);
  }
  catch(...) {
    LogError(STDLOG) << __func__ <<  " some error";
    backPaid("",reqNode,nullptr,resNode);
  }
}

void ServiceEvalInterface::response_svc_emd_issue_confirm(const std::string& exchangeId,xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  SvcEmdIssueConfirmRes res;
  LogTrace(TRACE5) << __func__;
  try {
/*    if ( !isResponseHandler( res.exchangeId(), externalSysResNode ) ) {
     return;
    }*/
    res.parseResponse(externalSysResNode);
    int grp_id=NodeAsInteger("grp_id",reqNode);
    checkRequestErrorAndRollback( grp_id, res );
    TPriceRFISCList prices;
    prices.fromContextDB(grp_id);
    prices.synchFromSirena(res.getSvcEmdSvcsAns());
    prices.setStatusDirect(TPriceRFISCList::STATUS_DIRECT_ISSUE_ANSWER,TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM);
    prices.toContextDB(grp_id);
    ASTRA::commit(); //!!!
    tst();
    continuePaidRequest(reqNode, resNode);
  }
  catch(EXCEPTIONS::Exception &e) {
    LogError(STDLOG) << e.what();
    backPaid("",reqNode,nullptr,resNode);
  }
  catch(...) {
    LogError(STDLOG) << __func__ <<  " some error";
    backPaid("",reqNode,nullptr,resNode);
  }
}

void ServiceEvalInterface::BeforeRequest(xmlNodePtr& reqNode, xmlNodePtr externalSysResNode, const SirenaExchange::TExchange& req)
{
  AstraLocale::showProgError("MSG.SWC_CONNECT_ERROR");
  NewTextChild( reqNode, "exchangeId", req.exchangeId() );
}

void ServiceEvalInterface::BeforeResponseHandle(int reqCtxtId, xmlNodePtr& reqNode, xmlNodePtr& ResNode, std::string& exchangeId)
{
  xmlNodePtr exchangeIdNode = NodeAsNode( "exchangeId", reqNode );
  exchangeId = NodeAsString( exchangeIdNode );
  RemoveNode(exchangeIdNode);
}

/*
void ServiceEvalInterface::BeforePaid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //NewTextChild(resNode,"POSExchange"); //!!!
}

void ServiceEvalInterface::AfterPaid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  tst();
}

void ServiceEvalInterface::exchange(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << reqNode->name;
  sberbank s;
  xmlNodePtr node = NewTextChild(resNode,"exchange");
  std::string modeStr = NodeAsString( "mode", reqNode );
  int grp_id = NodeAsInteger("@key",reqNode);
  LogTrace(TRACE5) << "grp_id=" << grp_id << ",mode=" << modeStr;
  if ( modeStr == "posProcess" ) {
    LogTrace(TRACE5) << "pos finished : request=???";
    NewTextChild(node,"mode","posStop");
    return;
  }
  if ( modeStr == "posStart" ) {
    TPriceRFISCList prices;
    prices.fromContextDB(grp_id);
    SvcEmdCost cost  = prices.getSvcEmdCost();
    LogTrace(TRACE5) << "cost=" << cost.getCost();
    node = NewTextChild(node,"commands");
    node = NewTextChild(node,"command");
    NewTextChild( node, "name", "pay" );
    NewTextChild( node, "request", s.pay(cost.getCost()));
    prices.SvcEmdCost::clear();
    prices.toContextDB(grp_id);
    return;
  }
}
*/

/*
 *
 *
 DROP TABLE SVC_PRICES;
 CREATE TABLE SVC_PRICES (
 GRP_ID NUMBER(9) NOT NULL,
 XML VARCHAR2(4000) NOT NULL,
 PAGE_NO NUMBER(9) NOT NULL,
 TIME_CREATE DATE NOT NULL
 );

 CREATE TABLE EVALUATIONS_LOGS (
   DESK VARCHAR2(6) NOT NULL,
   PROC VARCHAR2(100) NOT NULL,
   XML VARCHAR2(4000) NOT NULL,
   PAGE_NO NUMBER(9) NOT NULL,
   TIME DATETIME NOT NULL
 );

 ALTER TABLE svc_prices
       ADD  ( CONSTRAINT svc_prices__PK PRIMARY KEY (grp_id,page_no) ) ;


CREATE INDEX svc_prices__AK ON svc_prices
(
       grp_id                         ASC
);

ALTER TABLE svc_prices
       ADD  ( CONSTRAINT svc_prices__pax_grp__FK
              FOREIGN KEY (grp_id)
                             REFERENCES pax_grp ) ;


DROP TABLE PAY_SERVICES;

CREATE TABLE PAY_SERVICES (
  GRP_ID NUMBER(9) NOT NULL,
  PAX_ID NUMBER(9) NOT NULL,
  TRANSFER_NUM NUMBER(1) NOT NULL,
  RFISC VARCHAR2(15) NOT NULL,
  SERVICE_TYPE VARCHAR2(1) NOT NULL,
  AIRLINE VARCHAR2(3) NOT NULL,
  NAME_VIEW VARCHAR2(100) NOT NULL,
  NAME_VIEW_LAT VARCHAR2(100) NOT NULL,
  LIST_ID NUMBER(9),
  SVC_ID NUMBER(9) NOT NULL,
  DOC_ID VARCHAR2(100) NOT NULL,
  PASS_ID NUMBER(9) NOT NULL,
  SEG_ID NUMBER(9) NOT NULL,
  PRICE FLOAT NOT NULL,
  CURRENCY VARCHAR2(3) NOT NULL,
  TICKNUM VARCHAR2(15) NOT NULL,
  TICKET_CPN VARCHAR2(1) NOT NULL,
  TIME_PAID DATE NOT NULL
);



 *
 */
