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
#include "serverlib/xml_stuff.h" // ��� xml_decode_nodelist

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;

void cacheBack();

bool isPaymentAtDesk( int point_id )
{
  TTripInfo fltInfo;
  if (!fltInfo.getByPointId(point_id))
    throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  if ( GetTripSets(tsPayAtDesk, fltInfo) ) {
     TQuery Qry(&OraSession);
     Qry.SQLText =
       "SELECT method_type, "
       " DECODE(airline,:airline,100,NULL,50,0) + "
       " DECODE(airp_dep,:airp_dep,100,NULL,50,0) AS priority "
       " FROM pay_methods_set "
       " WHERE (airline=:airline OR airline IS NULL) AND "
       "       (airp_dep=:airp_dep OR airp_dep IS NULL) "
       "ORDER BY priority DESC";
     Qry.CreateVariable( "airline", otString, fltInfo.airline );
     Qry.CreateVariable( "airp_dep", otString, fltInfo.airp );
     Qry.Execute();
     return ( !Qry.Eof && Qry.FieldAsInteger( "priority" ) != 0 );
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
      throw EXCEPTIONS::Exception( "passenger not found, id=" + id );
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
        while ( node != nullptr &&
                std::string("segment") == (const char*)node->name ) {
          Segment s;
          s.fromXML(node,trfer_num);
          emplace_back(s);
          trfer_num++;
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
      throw EXCEPTIONS::Exception( "segment not found, id=" + id );
    }
};

class Ticket {
  public:
    std::string pass_id;
    std::string seg_id;
    std::string svc_id;
    std::string ticket_cpn;
    std::string ticknum;
    void fromXML( xmlNodePtr node ) {
      pass_id = NodeAsString( "@pass_id", node );
      seg_id = NodeAsString( "@seg_id", node, "" ); // ����� ���� ���⮩ � ��砥 ���⢥ত���� ������
      svc_id = NodeAsString( "@svc_id", node, "" ); // ����� ���� ���⮩ � ��砥 ����� �⮨����
      ticket_cpn = NodeAsString( "@ticket_cpn", node );
      ticknum = NodeAsString( "@ticknum", node );
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
      for ( const auto &p : *this ) {
        if ( p.pass_id == pass_id &&
             p.seg_id == seg_id ) {
          ticket = p;
          return;
        }
      }
      throw EXCEPTIONS::Exception( "ticket not found, pass_id=" + pass_id + ",seg_id=" + seg_id );
    }
};

class Price {
  public:
    std::string accode;
    std::string baggage;
    std::string code;
    std::string currency;
    std::string doc_id;
    std::string doc_type;
    std::string passenger_id;
    std::string svc_id;
    std::string ticket;
    std::string ticket_cpn;
    std::string validating_company;
    float total;
    void fromXML( xmlNodePtr node ) {
      accode = NodeAsString( "@accode", node );
      baggage = NodeAsString( "@baggage", node );
      code = NodeAsString( "@code", node );
      currency = NodeAsString( "@currency", node );
      doc_id = NodeAsString( "@doc_id", node );
      doc_type = NodeAsString( "@doc_type", node );
      passenger_id = NodeAsString( "@passenger-id", node );
      svc_id = NodeAsString( "@svc-id", node, "" ); // ��䨪��� �������⭠
      ticket = NodeAsString( "@ticket", node );
      ticket_cpn = NodeAsString( "@ticket_cpn", node, "" );
      validating_company = NodeAsString( "@validating_company", node );
      total = NodeAsFloat( "total", node );
    }
};

class Prices: public std::vector<Price> {
  public:
    void fromXML( xmlNodePtr node ) {
      node = GetNode( "prices", node );
      if ( node != nullptr ) {
        node = node->children;
        while ( node != nullptr &&
                std::string("price") == (const char*)node->name ) {
          Price s;
          s.fromXML(node);
          emplace_back(s);
          node = node->next;
        }
      }
    }
    std::string getDocId(const std::string& svc_id) {
      for ( const auto &p : *this ) {
        if ( p.svc_id == svc_id ) {
          return p.doc_id;
        }
      }
      return "";
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
      SegsPaxs segPaxs;
      segPaxs.fromDB(grp_id,point_dep);
      for ( auto &p : prices ) {
        if ( p.svc_id.empty() ) { //��䨪��� �� ������ - �� ᬮ��� �業���
          continue;
        }
        LogTrace(TRACE5) << "pass_id=" << p.passenger_id << ",svc_id=" << p.svc_id << ",total=" << p.total << ",currency=" << p.currency;
        SvcValue svc;
        SvcEmdSvcsAns::getSvcValue( p.svc_id, svc );
        LogTrace(TRACE5) << "svc_id=" << svc.id << ",pass_id=" << svc.pass_id << ",seg_id=" << svc.seg_id;
        if ( svc.pass_id !=  p.passenger_id )
          throw EXCEPTIONS::Exception( "Invalid data XML pass_id from passengers not equal pass_id from svcs, passengers.pass_id=" + p.passenger_id + ",svcs.pass_id=" + svc.pass_id );
        Ticket ticket;
        tickets.get(svc.pass_id,svc.seg_id,ticket);
        if ( ticket.pass_id !=  p.passenger_id )
          throw EXCEPTIONS::Exception( "Invalid data XML pass_id from passengers not equal pass_id from tickets, passengers.pass_id=" + p.passenger_id + ",tickets.pass_id=" + ticket.pass_id );
        if ( ticket.seg_id !=  svc.seg_id )
          throw EXCEPTIONS::Exception( "Invalid data XML seg_id from svcs not equal seg_id from tickets, svcs.seg_id=" + svc.seg_id + ",tickets.seg_id=" + ticket.seg_id );
      }
      for ( auto &p : *this ) {
        if ( std::string("HD") != p.status ) {
          continue;
        }
        Ticket t;
        tickets.get( p.pass_id, p.seg_id, t );
        CheckIn::TSimplePaxList paxs_list;
        CheckIn::Search search(paxCheckIn);
        int coupon;
        if ( StrToInt(t.ticket_cpn.c_str(), coupon) == EOF ) {
          throw EXCEPTIONS::Exception( "Invalid data XML counpon no=" + t.ticket_cpn );
        }
        search(paxs_list, CheckIn::TPaxTknItem(t.ticknum,coupon));
        if ( paxs_list.size() != 1 ) {
           throw EXCEPTIONS::Exception( "Invalid data XML pax not found ticknum=" + t.ticknum + ",ticket_cpn=" + t.ticket_cpn );
        }
        if ( paxs.find(p.pass_id) != paxs.end() ) {
          if ( paxs[p.pass_id] != paxs_list.begin()->id ) {
            throw EXCEPTIONS::Exception( "Invalid data XML astra pax_id not identical sirena pax_id" );
          }
        }
        else {
          paxs.insert( make_pair(p.pass_id,paxs_list.begin()->id) );
          LogTrace(TRACE5) << "sirena pax_id=" << p.pass_id << ",astra pax_id=" << paxs_list.begin()->id;
        }
      }
      for ( auto &p : segments ) {
        if ( !segPaxs.checkTrferNum(p.trfer_num) ) {
          throw EXCEPTIONS::Exception( "Invalid data XML sirena trfer_num not found in astra routes, trfer_num=" + p.trfer_num );
        }
        trfer_nums[p.id] = p.trfer_num;
      }
      for ( auto &p : prices ) {
        if ( p.svc_id.empty() ) { //��䨪��� �� ������ - �� ᬮ��� �業���
          continue;
        }
        TPaxSegRFISCKey key;
        TElemFmt fmt;
        key.airline = ElemToElemId( etAirline, p.validating_company, fmt );
        SvcValue svc;
        SvcEmdSvcsAns::getSvcValue(p.svc_id,svc);
        if ( paxs.find(svc.pass_id) == paxs.end() ) {
          throw EXCEPTIONS::Exception( "Invalid data XML astra pax_id not identical sirena pax_id=" + svc.pass_id );
        }
        key.pax_id = paxs[svc.pass_id];
        if ( trfer_nums.find(svc.seg_id) == trfer_nums.end() ) {
          throw EXCEPTIONS::Exception( "Invalid data XML sirena seg_num not found in astra routes, seg_num=" + svc.seg_id );
        }
        key.trfer_num = trfer_nums[svc.seg_id];
        key.RFISC = svc.rfisc;
        key.service_type = ServiceTypes().decode(svc.service_type);
        TPriceServiceItem price(key,paxsNames.getPaxName(key.pax_id),std::map<std::string,SvcFromSirena>());
        SvcFromSirena svcSirena;
        svcSirena.doc_id = prices.getDocId(p.svc_id);
        svcSirena.pass_id = svc.pass_id;
        svcSirena.price = p.total;
        svcSirena.currency =  ElemToElemId( etCurrency, p.currency, fmt );
        svcSirena.seg_id = svc.seg_id;
        svcSirena.name = svc.name;
        svcSirena.status_svc = svc.status;
        svcSirena.status_direct = TPriceRFISCList::STATUS_DIRECT_ORDER;
        svcSirena.svc_id = p.svc_id;
        svcSirena.time_change = BASIC::date_time::NowUTC();
        if ( svcList.find(key) == svcList.end() ) {
          svcList.emplace( key, price );
        }
        svcList[key].svcs.emplace( svcSirena.svc_id, svcSirena );
        for ( auto &l : svcList ) {
          LogTrace(TRACE5) << l.second.traceStr();
        }
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
          service.status_direct = "order";mnm,//㦥 ���� �����-� �����
          astraPrices.svcs.insert(make_pair(service,service));
        }
      }
      //astraPrices.PriceRFISCList = PriceRFISCList;
    }*/

};


// �������� PNR � �����⥬�묨 ᥣ���⠬�, ᮮ⢥�����饣� ��㯯� ॣ����樨 � ���� (� ������� - "+��") - check_in_get_pnr
class CheckInGetPNRReq: public SWC::SWCExchange
{
  private:
    std::map<int,std::set<int>> params; // point_id,pax_id
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
        xmlNodePtr segNode = NewTextChild(n,"segment");
        SetProp( segNode, "point_id", sp.first );
        for ( const auto &p : sp.second ) {
          SetProp( NewTextChild(segNode,"passenger"), "crs_pax_id", p );
        }
      }
    }
    CheckInGetPNRReq( const std::map<int,std::set<int>>& _params ) : params(_params) {}
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
void ServiceEvalInterface::Evaluation(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
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
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  if (prices.notInit()) {
    std::map<int,std::set<int>> params;
    CheckIn::TSimplePaxGrpItem grpItem;
    if ( !grpItem.getByGrpId(grp_id) ) {
      tst();
      throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
    }
    if ( !isPaymentAtDesk(grpItem.point_dep) ) {
      throw AstraLocale::UserException("MSG.EMD.NOT_COST_SETS");
    }
    SegsPaxs segsPaxs;
    segsPaxs.fromDB(grp_id,grpItem.point_dep);
    segsPaxs.getPaxs(params);
    CheckInGetPNRReq req(params);
    req.fromDB();
    Request( reqNode, getServiceName(), req );
  }
  else {
    OrderReq orderReq( prices.getSvcEmdRegnum(), prices.getSurname() );
    orderReq.fromDB();
    tst();
    Request( reqNode, getServiceName(), orderReq );
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
  orderReq.fromDB();
  Request( reqNode, getServiceName(), orderReq );
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
  tst();
  TPriceRFISCList list;
  res.toPrice(grp_id,grpItem.point_dep,list);
  prices.synchFromSirena(list);
  tst();
  prices.toContextDB(grp_id);
  tst();
  xmlNodePtr node = NewTextChild(resNode,"prices");
  NewTextChild(node,"grp_id",grp_id);
  tst();
  prices.toXML(NewTextChild( node, "services" ));
  tst();
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
  TPriceRFISCList PriceRFISCList;
  PriceRFISCList.fromXML(servicesNode);
  xmlNodePtr node = NewTextChild(resNode,"prices");
  NewTextChild(node,"grp_id",grp_id);
  PriceRFISCList.toXML(NewTextChild( node, "services" ));
}

void ServiceEvalInterface::backPaid(const std::string& exchangeId,xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << __func__ << ",exchangeId=" << exchangeId <<",externalSysResNode=" << externalSysResNode;
  int grp_id=NodeAsInteger("grp_id",reqNode);
  TPriceRFISCList prices;
  bool inRequest = false;

  if ( externalSysResNode == nullptr ) { // �⪠�
    prices.fromContextDB(grp_id);
    std::vector<std::string> svcs;
    if ( prices.haveStatusDirect( TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM, svcs ) ) {
      SvcEmdVoidReq req(prices.getSvcEmdRegnum(),SvcEmdSvcsReq(svcs));
      req.fromDB();
      Request( reqNode, getServiceName(), req );
      inRequest = true;
    }
    if ( prices.haveStatusDirect( TPriceRFISCList::STATUS_DIRECT_ISSUE_ANSWER, svcs ) ) {
      SvcEmdIssueCancelReq req(prices.getSvcEmdRegnum());
      req.fromDB();
      Request( reqNode, getServiceName(), req );
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
    prices.Lock(grp_id); //� ࠧ��� ��ࠡ��稪�� ��ࠫ����� ������???
    try {
      prices.fromContextDB(grp_id);

      try {
        LogTrace(TRACE5) << "parse " << res->exchangeId();
        res->parseResponse(externalSysResNode);
        if ( res->error() ) {
          throw EXCEPTIONS::Exception("error_message=%s, error_code=%s", res->error_message,res->error_code);
        }
        tst();
      }
      catch(EXCEPTIONS::Exception &e) {
        LogError(STDLOG) << e.what();
      }
      catch(...) {
        LogError(STDLOG) << __func__ <<  " some error";
      }
      prices.setStatusDirect(res->exchangeId() == SvcEmdVoidRes::getExchangeId()?TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM:TPriceRFISCList::STATUS_DIRECT_ISSUE_ANSWER,TPriceRFISCList::STATUS_DIRECT_ORDER);
      ASTRA::commit(); //!!!
    }
    catch(...) {
      prices.setStatusDirect(res->exchangeId() == SvcEmdVoidRes::getExchangeId()?TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM:TPriceRFISCList::STATUS_DIRECT_ISSUE_ANSWER,TPriceRFISCList::STATUS_DIRECT_ORDER);
      ASTRA::commit(); //!!!
    }
  }
  //�� �⪠⨫� ��� ��祣� �뫮 �⪠�뢠��
  //���� ����� +�� ������, �⮡� ����⠭����� ��㣨 �� �����, ��� �⮣� ���⨬ ���� �� ��㯯�, �뤠�� ᮮ�饭�� �������
  prices.fromContextDB(grp_id);
  if ( !prices.haveStatusDirect( TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM ) &&
       !prices.haveStatusDirect( TPriceRFISCList::STATUS_DIRECT_ISSUE_ANSWER ) ) {
    LogTrace(TRACE5) << __func__ << " rollback STATUS_DIRECT_ISSUE_CONFIRM and STATUS_DIRECT_ISSUE_ANSWER";
    prices.toContextDB(grp_id,true);
    prices.fromContextDB(grp_id);
    xmlNodePtr node = NewTextChild(resNode,"prices");
    NewTextChild(node,"grp_id",grp_id);
    prices.toXML(NewTextChild( node, "services" ));
    AstraLocale::showErrorMessage("MSG.EMD.PAID_ERROR_REFRESH_DATA");
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
// �� �ॡ���� �⪠� - ���室�� � �����
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
  prices.setSvcEmdPayDoc( SvcEmdPayDoc( "CA", "" ) );
  prices.toContextDB(grp_id);
  std::vector<std::string> svcs;
  prices.haveStatusDirect(TPriceRFISCList::STATUS_DIRECT_SELECTED,svcs);
  if ( svcs.empty() ) {
    throw AstraLocale::UserException("MSG.EMD.NOT_SERVICES_CHOICE");
  }
  continuePaidRequest(reqNode, resNode);
//  backPaid(reqNode,NULL,resNode); // �⪠� ��� �����, ����� �� �����訫���
}

void ServiceEvalInterface::continuePaidRequest(xmlNodePtr reqNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << __func__;
  int grp_id=NodeAsInteger("grp_id",reqNode);
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  std::vector<std::string> svcs;
  if ( !prices.getNextIssueQueryGrpEMD( svcs ) ) {
    //EMD_REFRESH!!!
    prices.setStatusDirect(TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM,TPriceRFISCList::STATUS_DIRECT_PAID);
    prices.SvcEmdPayDoc::clear();
    prices.SvcEmdCost::clear();
    prices.SvcEmdTimeout::clear();
    prices.toContextDB(grp_id);
    tst();
    EMDAutoBoundInterface::EMDRefresh(EMDAutoBoundGrpId(grp_id), reqNode);
    tst();
    xmlNodePtr node = NewTextChild(resNode,"prices");
    NewTextChild(node,"grp_id",grp_id);
    prices.toXML(NewTextChild( node, "services" ));
    tst();
    AstraLocale::showMessage( "MSG.EMD.PAID_FINISHED" );
    return;
  }
  LogTrace(TRACE5) << "svcs.size=" << svcs.size();
  SvcEmdIssueReq req(prices.getSvcEmdRegnum(), prices.getSvcEmdPayDoc(), SvcEmdSvcsReq(svcs));
  req.fromDB();
  tst();
  prices.toContextDB(grp_id);
  ASTRA::commit();
  Request( reqNode, getServiceName(), req );
  tst();
}

void ServiceEvalInterface::response_svc_emd_issue_query(const std::string& exchangeId,xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  SvcEmdIssueRes res;
  /*if ( !isResponseHandler( res.exchangeId(), externalSysResNode ) ) {
     return;
  }*/
  try {
    LogTrace(TRACE5) << __func__;
    res.parseResponse(externalSysResNode);
    if ( res.error() ) {
      //res.errorToXML(resNode); // ᮮ�饭�� �� ��࠭
      throw EXCEPTIONS::Exception("error_message=%s, error_code=%s", res.error_message,res.error_code);
    }
    int grp_id=NodeAsInteger("grp_id",reqNode);
    TPriceRFISCList prices;
    prices.fromContextDB(grp_id);
    prices.setStatusDirect(TPriceRFISCList::STATUS_DIRECT_ISSUE_QUERY,TPriceRFISCList::STATUS_DIRECT_ISSUE_ANSWER);
    prices.toContextDB(grp_id);
    SvcEmdIssueConfirmReq reqConfirm( SvcEmdRegnum(res.getRegnum()),  prices.getSvcEmdPayDoc(), res.getSvcEmdCost() );
    reqConfirm.fromDB();
    ASTRA::commit(); //!!!
    Request( reqNode, getServiceName(), reqConfirm );
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
    if ( res.error() ) {
      //res.errorToXML(resNode);
      throw EXCEPTIONS::Exception("error_message=%s, error_code=%s", res.error_message,res.error_code);
    }
    int grp_id=NodeAsInteger("grp_id",reqNode);
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

 *
 */
