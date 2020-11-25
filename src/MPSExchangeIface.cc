#include "MPSExchangeIface.h"
#include "xml_unit.h"

#define NICKNAME "DJEK"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>
#include <serverlib/str_utils.h>
#include "date_time.h"
#include "etick/tick_data.h"
#include "astra_elems.h"
#include "astra_locale.h"
#include "ExchangeIface.h"
#include "service_eval.h"
#include "edi_utils.h"
#include "rfisc_price.h"
#include "etick/tick_data.h"
#include "etick.h"
#include "trip_tasks.h"
#include "astra_utils.h"
#include "serverlib/xml_stuff.h"

namespace MPS
{

class MPSClient : public ExchangeIterface::HTTPClient
{
  private:
  public:
    struct ConnectProps {
      std::string addr;
      unsigned port;
      int timeout;
      bool useSSL;
      std::string Resource;
      std::string Authorization;
      void clear() {
        addr.clear();
        port = 0;
        timeout = 0;
        useSSL = true;
        Authorization.clear();
        Resource.clear();
      }
      ConnectProps() {
        clear();
      }
      void fromDB( ) {
        clear();
        addr = getTCLParam("MPS_ENDPOINT","");
        size_t fnd_b = addr.find(":");
        int vport;
        if ( fnd_b != std::string::npos &&
             StrToInt( addr.substr( fnd_b+1 ).c_str(), vport ) != EOF ) {
          addr.erase( addr.begin() + fnd_b, addr.end() );
          port = vport;
        }
        else {
          port = 80;
        }
        timeout = 15000;
      }
    };
    virtual std::string makeHttpPostRequest( const std::string& postbody) const {
      std::stringstream os; //+//not use POST http:// + // + " HTTP/1.1\r\n"
      os << "POST " << resource << " HTTP/1.1\r\n";
      os << "Host: " << m_addr.host << ":" << m_addr.port << "\r\n";
      os << "Content-Type: application/x-www-form-urlencoded\r\n";
      os << authorization << "\r\n";
      os << "Accept: */*" << "\r\n";
      os <<  "Content-Length: " << HelpCpp::string_cast(postbody.size()) << "\r\n";
      os << "\r\n";
      os << postbody;
//      LogTrace(TRACE5) << os.str();
      return os.str();
    }
    MPSClient( const ConnectProps& props ):ExchangeIterface::HTTPClient(props.addr, props.port, props.timeout, props.useSSL, props.Resource, props.Authorization){}
    ~MPSClient(){}
};

void MPSExchange::fromDB(int clientId)
{
  this->clientId = clientId;
  Authorization = "Authorization: Basic " + StrUtils::b64_encode( getTCLParam("MPS_CONNECT","") );
  Resource = "/order/v2/";
  LogTrace(TRACE5) << __func__ << " " << Authorization << ",clientId=" << clientId << ", Resource=" <<Resource;
}

void MPSExchange::build(std::string &content) const
{
  content.clear();
  try
  {
    XMLDoc doc;
    doc.set("soapenv:Envelope");
    if (doc.docPtr()==NULL)
      throw EXCEPTIONS::Exception("toSoapReq: CreateXMLDoc failed");
    xmlNodePtr soapNode=xmlDocGetRootElement(doc.docPtr());
    SetProp(soapNode, "xmlns:soapenv", "http://schemas.xmlsoap.org/soap/envelope/");
    SetProp(soapNode, "xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
    SetProp(soapNode, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    xmlNodePtr bodyNode = NewTextChild(soapNode, "soapenv:Body");
    if (error())
      errorToXML(bodyNode);
    else
      toXML(bodyNode);
    content = ConvertCodepage( XMLTreeToText( doc.docPtr() ), "CP866", "UTF-8" );
  }
  catch(EXCEPTIONS::Exception &e)
  {
    throw EXCEPTIONS::Exception("MPSExchange::build: %s", e.what());
  };
}

void MPSExchange::parseResponse(xmlNodePtr node)
{
  errorFromXML(node);
  node = GetNodeFast( "Body", node->children );
  LogTrace(TRACE5) << node;
  if ( node != nullptr ) {
    xmlNodePtr fnode = GetNodeFast( methodName().c_str(), node->children );
    LogTrace(TRACE5) << fnode;
    if ( fnode != nullptr &&
         GetNodeFast( "retval", fnode->children ) != nullptr &&
         std::string(NodeAsStringFast( "retval", fnode->children )).empty() ) {
      tst();
      error_code.clear();
      error_message.clear();
    }
  }
}

void MPSExchange::errorFromXML(xmlNodePtr node)
{
  error_code.clear();
  error_message.clear();
  error_reference.clear();
  LogTrace(TRACE5) << node->name;
  xmlNodePtr nodeError = GetNodeFast("error", node->children);
  if ( nodeError != nullptr ) {
    error_code = NodeAsStringFast( "code", nodeError->children );
    error_message = NodeAsStringFast( "message", nodeError->children );
  }
  nodeError = GetNodeFast( "Body", node->children );
  if ( nodeError != nullptr ) {
    xmlNodePtr fnode = GetNodeFast( "Fault", nodeError->children ); // ТОлько Fast работает!!!! <soap-env:Fault>
    if ( fnode != nullptr ) {
      tst();
      error_message = NodeAsString( "faultstring", fnode );
    }
  }
}

void MPSExchangeIface::Request(xmlNodePtr reqNode, int clientId, const std::string& ifaceName, MPSExchange& req)
{
  req.fromDB(clientId);
  MPSClient::ConnectProps props;
  props.fromDB();
  props.Authorization = req.getAuthorization();
  props.Resource = req.getResource();
  MPSClient client( props );
  ExchangeIface* iface=dynamic_cast<ExchangeIface*>(JxtInterfaceMng::Instance()->GetInterface(ifaceName));
  iface->DoRequest(reqNode, nullptr, req, client);
}

template <class T>
void SimpleSoapArrayObject<T>::toXML( xmlNodePtr node ) const
{
  xmlNodePtr Fnode = NewTextChild( node, getObjectName().c_str() );
  for ( const auto &v : vecSimpleSoapObject ) {
    v.toXML( Fnode );
  }
}

void OrderID::toXML( xmlNodePtr node ) const {
  NewTextChild( node, "shop_id", Fshop_id );
  NewTextChild( node, "number", Fnumber );
}

void OrderID::fromXML( xmlNodePtr node ) {
  node = node->children;
  Fshop_id = NodeAsIntegerFast( "shop_id", node );
  Fnumber = NodeAsStringFast( "number", node );
}

void Amount::SetCurrency( const std::string &Acurrency ) {
  Fcurrency = ElemIdToPrefferedElem(etCurrency, Acurrency, efmtCodeNative, AstraLocale::LANG_EN);
}

void Amount::toXML( xmlNodePtr node ) const {
  NewTextChild( node, "amount", Famount.amStr(Ticketing::CutZeroFraction(true)) );
  NewTextChild( node, "currency", Fcurrency );
}

void Amount::fromXML( xmlNodePtr node )
{
  SetAmount( Ticketing::TaxAmount::Amount( NodeAsStringFast( "amount", node ) ) );
  SetCurrency( NodeAsStringFast( "currency", node ) );
}

void CustomerInfo::toXML( xmlNodePtr node ) const {
  NewTextChild( node, "phone", Fphone );
  NewTextChild( node, "email", Femail );
  NewTextChild( node, "name", Fname );
  NewTextChild( node, "id", Fid );
}

void String::toXML( xmlNodePtr node ) const {
  NodeSetContent( node, value );
}

void AgentInfo::toXML( xmlNodePtr node ) const {
  NewTextChild( node, "type", Ftype );
}

void SupplierInfo::toXML( xmlNodePtr node ) const {
  NewTextChild( node, "name", Fname );
  NewTextChild( node, "inn", Finn );
  NewTextChild( node, "phone", Fphone );
}
void Tax::toXML( xmlNodePtr node ) const {
  NewTextChild( node, "name", Fname );
  NewTextChild( node, "percentage", Fpercentage );
  NewTextChild( node, "source", Fsource );
  Famount.toXML(node);
}

void OrderItem::toXML( xmlNodePtr node ) const {
  xmlNodePtr Fnode = NewTextChild( node, "OrderItem" );
  NewTextChild( Fnode, "name", Fname );
  NewTextChild( Fnode, "typename", Ftypename );
  NewTextChild( Fnode, "descr", Fdescr );
  NewTextChild( Fnode, "host", Fhost );
  NewTextChild( Fnode, "taxation_item_settlement_method", Ftaxation_item_settlement_method );
  NewTextChild( Fnode, "quantity", Fquantity );
  NewTextChild( Fnode, "number", Fnumber );
  NewTextChild( Fnode, "taxation_item_type", Ftaxation_item_type );
  Fdocuments.toXML( Fnode );
  Fagent_info.toXML( Fnode );
  NewTextChild( Fnode, "measure", Fmeasure );
  Fsupplier_info.toXML( Fnode );
  NewTextChild( Fnode, "clearing", Fclearing );
  NewTextChild( Fnode, "accode", Faccode );
  NewTextChild( Fnode, "taxation_system", Ftaxation_system );
  Ftaxes.toXML( Fnode );
  Famount.toXML( Fnode );
  NewTextChild( Fnode, "shopref", Fshopref );
  NewTextChild( Fnode, "ref", Fref );
}

void PaymentPart::toXML( xmlNodePtr node ) const {
  xmlNodePtr Fnode = NewTextChild( node, "PaymentPart" );
  Fsource.toXML( Fnode );
  NewTextChild( Fnode, "id", Fid );
  Famount.toXML( Fnode );
  NewTextChild( Fnode, "type", Ftype );
  NewTextChild( Fnode, "ref", Fref );
}

void TXSDateTime::toXML( xmlNodePtr node ) const {
  std::string val;
  int Year, Month, Day;
  BASIC::date_time::TDateTime d = Ftimelimit.get();
  BASIC::date_time::DecodeDate(d, Year, Month, Day);
  if ( Year + Month + Day > 0 ) {
    val = BASIC::date_time::DateTimeToStr(d, "yyyy-mm-ddThh:nn:ss:000Z");
  }
  else {
    val = BASIC::date_time::DateTimeToStr(d, "Thh:nn:ss:000Z");
  }
  NodeSetContent( node, val );
}

void OrderFull::toXML( xmlNodePtr node ) const {
  Ftimelimit.toXML( node );
  Fparts.toXML( node );
  Fsales.toXML( node );
  Frefunds.toXML( node );
  NewTextChild( node, "shopref", Fshopref );
}

 void PostEntry::toXML( xmlNodePtr node ) const {
   xmlNodePtr n = NewTextChild( node, "PostEntry" );
   NewTextChild( n, "name", Fname );
   NewTextChild( n, "value", Fvalue );
}

void Error::fromXML( xmlNodePtr node ) {
  node = node->children;
  Fcategory = NodeAsStringFast( "category", node );
  Fcode = NodeAsStringFast( "code", node );
}

void NotifyPushEvent::fromStr( const std::string& notifyXML, EnumParse parseMode ) {
//  LogTrace(TRACE5)<<notifyXML;
  XMLDoc doc(notifyXML);
  LogTrace(TRACE5) << doc.docPtr() << doc.docPtr()->children->name;
  xmlNodePtr node = GetNode( "/term/query/notify/content",doc.docPtr()->children );
  node = GetNodeFast( "Envelope", node->children);
  node = GetNodeFast( "Body", node->children );
  LogTrace(TRACE5) << node;
  node = NodeAsNodeFast( "notify", node->children );
  LogTrace(TRACE5) << node;
  fromXML( node->children, parseMode );
}

void PayItem::fromXML( xmlNodePtr node ) {
  id = NodeAsStringFast( "id", node );
  ptype = NodeAsStringFast( "type", node );
  owner = NodeAsStringFast( "owner", node );
  pay_id = NodeAsStringFast( "rrn", node );
  order = NodeAsStringFast( "number", node );
  total = NodeAsStringFast( "total", node );
}

void PaySVC::fromXML( xmlNodePtr node ) {
  clear();
  xmlNodePtr n = NodeAsNodeFast( "svc_details", node );
  xmlNodePtr nn;
  if ( n ) {
    nn = n->children;
    rfisc = NodeAsStringFast( "rfisc", nn );
    rfic = NodeAsStringFast( "rfic", nn );
     name = NodeAsStringFast( "name", nn );
    service_type = NodeAsStringFast( "service_type", nn );
  }
  n = NodeAsNodeFast( "svc_document", node );
  if ( n ) {
    nn = n->children;
    emd_type = NodeAsStringFast( "emd_type", nn );
    emd_number = NodeAsStringFast( "emd_number", nn );
    emd_coupon = NodeAsStringFast( "emd_coupon", nn );
    emd_status = NodeAsStringFast( "status", nn );
  }
  pass_id = NodeAsStringFast( "passenger_id", n );
  seg_id = NodeAsStringFast( "segment_id", n );
}

void NotifyPushEvent::fromXML( xmlNodePtr node, EnumParse parseMode ) {
  FStatus = NodeAsStringFast( "status", node );
  Error::fromXML( NodeAsNodeFast( "error", node ) );
  OrderID::fromXML( NodeAsNodeFast( "order", node ) );
  if ( parseMode != Total || !isGood() ) {
    return;
  }
  xmlNodePtr n =  GetNodeFast( "items", node );
  items.clear();
  if ( !n ) {
    tst();
    return;
  }
  n = n->children;
  while ( n  && std::string( "item" ) == (const char*)n->name ) {
    PayItem item;
    item.fromXML( n->children );
    items.emplace_back( item );
    if ( FOrder.empty() ) {
      FOrder = item.order;
    }
    n = n->next;
  }
  n = NodeAsNodeFast( "issued_documents", node );
  n = NodeAsNodeFast( "svcs", n->children );
  LogTrace(TRACE5)<< n;
  n = n->children;
  svcs.clear();
  while ( n && std::string( "svc" ) == (const char*)n->name ) {
    PaySVC  svc;
    svc.fromXML( n->children );
    std::string sitem_id = NodeAsStringFast("item_id", n->children );
    std::map<std::string, std::vector<PaySVC>>::iterator pv = svcs.find( sitem_id );
    if ( pv == svcs.end() ) {
      std::vector<PaySVC> v;
      v.emplace_back( svc );
      svcs.emplace( std::make_pair( sitem_id, v ) );
    }
    else {
      pv->second.emplace_back( svc );
    }
    n = n->next;
  }
  n = NodeAsNodeFast( "passengers", node );
  passes.clear();
  n = n->children;
  while ( n && std::string( "passenger" ) == (const char*)n->name ) {
    passes.emplace( std::make_pair( NodeAsStringFast("id", n->children ), NodeAsStringFast("gds_id", n->children ) ) );
    n = n->next;
  }
  n = NodeAsNodeFast( "trip_details", node );
  n = NodeAsNodeFast( "air_trip_details", n->children );
  n = n->children;
  std::string id, gds_id;
  segs.clear();
  while ( n ) {
    if ( std::string( "trip_route" ) == (const char*)n->name ) {
      if ( id.empty() || gds_id.empty() ) {
        LogError(STDLOG) << __func__ << "id=" <<id << " gds_id=" << gds_id;
        throw EXCEPTIONS::Exception(" NotifyPushEvent::fromXML: invalid notify XML from MPS id or gds_id is emtry");;
      }
      segs.emplace( std::make_pair(id, gds_id) );
    }
    if ( std::string( "id" ) == (const char*)n->name ) {
      id = NodeAsString( n );
      LogTrace(TRACE5) << "id=" << id;
    }
    if ( std::string( "gds_id" ) == (const char*)n->name ) {
      gds_id = NodeAsString( n );
      LogTrace(TRACE5) << "gds_id=" << gds_id;
    }
    n = n->next;
  }
  n = NodeAsNodeFast( "payments", node );
  n = n->children;
  amounts.clear();
  while ( n && std::string( "Payment" ) == (const char*)n->name ) {
    Amount amount( "amount" );
    amount.fromXML( NodeAsNodeFast( "amount", n->children )->children );
    amounts.emplace( std::make_pair( NodeAsStringFast( "id", n->children ), amount ) );
    n = n->next;
  }
}

void NotifyPushEvent::getSVCS( std::vector<PaySVC>& vsvcs ) const
{
  vsvcs.clear();
  for ( const auto &vecsvc : svcs ) {
    for ( const auto &svc : vecsvc.second ) {
      PaySVC p = svc;
      const auto pass = passes.find( p.pass_id );
      if ( pass != passes.end()  ) {
        p.pass_id = pass->second;
        LogTrace(TRACE5) << p.pass_id;
      }
      const auto seg = segs.find( p.seg_id );
      if ( seg != segs.end() ) {
        p.seg_id = seg->second;
        LogTrace(TRACE5) << p.seg_id;
      }
      LogTrace(TRACE5) << p.toString();
      vsvcs.emplace_back( p );
    }
  }
}

void RegisterMethod::toXML( xmlNodePtr node ) const {
  xmlNodePtr Fnode = NewTextChild( node, methodName().c_str() );
  SetProp( Fnode, "xmlns", "http://www.sirena-travel.ru" );
  Forder.toXML( Fnode );
  Fcost.toXML( Fnode );
  Fcustomer.toXML( Fnode );
  Fdescription.toXML( Fnode );
  Fpostdata.toXML( Fnode );
}

std::string RegisterMethod::getRegnumFromXML( xmlNodePtr node )
{
  // name space in XML! work only Fast method
  xmlNodePtr n = GetNodeFast( "register", node );
  n = GetNodeFast( "register", n->children );
  n = GetNodeFast( "description", n->children );
  n = GetNodeFast( "sales", n->children );
  n = GetNodeFast( "OrderItem", n->children );
  return std::string( NodeAsStringFast( "number", n->children ) );
}

void CancelMethod::toXML( xmlNodePtr node ) const {
  xmlNodePtr Fnode = NewTextChild( node, methodName().c_str() );
  SetProp( Fnode, "xmlns", "http://www.sirena-travel.ru" );
  Forder.toXML( Fnode );
}

void SimpleMethod::trace( xmlNodePtr node ) {
  LogError(STDLOG) << XMLTreeToText( node->doc );
}

std::string SimpleMethod::getUniqueNumber() const
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
   "SELECT events__seq.nextval AS id FROM dual";
  Qry.Execute();
  return Qry.FieldAsString( "id");
}

void SimpleMethod::toDB( int grp_id )
{
   xmlNodePtr rootNode;
   XMLDoc doc( methodName(),rootNode,__func__);
   toXML( rootNode );
   std::string value = XMLTreeToText( doc.docPtr() );
   DBExchanger(  methodName(), getOrderId() ).request( grp_id, value );
}

void SimpleMethod::request( xmlNodePtr reqNode )
{
  toDB( NodeAsInteger("grp_id",reqNode) );
  MPS::MPSExchangeIface::Request( reqNode, 101, MPSExchangeIface::getServiceName(), *this );
}


/*
  CREATE TABLE mps_exchange (
  request_type VARCHAR2(10) NOT NULL,
  method VARCHAR2(20) NOT NULL,
  order_id VARCHAR2(9) NOT NULL,
  grp_id NUMBER(9) NOT NULL,
  xml VARCHAR2(4000) NOT NULL,
  page_no NUMBER(3) NOT NULL,
  status VARCHAR2(10) NOT NULL, "send","processed","error","notify"
  time_create DATE NOT NULL
);
*/

void MPSExchangeIface::BeforeRequest(xmlNodePtr& reqNode, xmlNodePtr externalSysResNode, const SirenaExchange::TExchange& req)
{
  AstraLocale::showProgError("MSG.MPS_CONNECT_ERROR");
  NewTextChild( reqNode, "exchangeId", req.exchangeId() );
}

void MPSExchangeIface::BeforeResponseHandle(int reqCtxtId, xmlNodePtr& reqNode, xmlNodePtr& ResNode, std::string& exchangeId)
{
  xmlNodePtr exchangeIdNode = NodeAsNode( "exchangeId", reqNode );
  exchangeId = NodeAsString( exchangeIdNode );
  RemoveNode(exchangeIdNode);
  LogTrace(TRACE5) << "exchangeId=" << exchangeId;
}

void MPSExchangeIface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    using namespace AstraEdifact;

    const std::string DefaultAnswer = "<answer/>";
    std::string pult = TReqInfo::Instance()->desk.code;
    LogTrace(TRACE3) << __FUNCTION__ << " for pult [" << pult << "]";
    boost::optional<httpsrv::HttpResp> resp = ExchangeIterface::HTTPClient::receive(pult,domainName);
    if(resp) {
        //LogTrace(TRACE3) << "req:\n" << resp->req.text;
        if(resp->commErr) {
             LogError(STDLOG) << "Http communication error! "
                              << "(" << resp->commErr->code << "/" << resp->commErr->errMsg << ")";
        }
    } else {
        LogError(STDLOG) << "Enter to KickHandler but HttpResponse is empty!";
    }

    if(GetNode("@req_ctxt_id",reqNode) != NULL)
    {
        int req_ctxt_id = NodeAsInteger("@req_ctxt_id", reqNode);
        XMLDoc termReqCtxt;
        getTermRequestCtxt(req_ctxt_id, true, "ExchangeIface::KickHandler", termReqCtxt);
        xmlNodePtr termReqNode = NodeAsNode("/term/query", termReqCtxt.docPtr())->children;
        if(termReqNode == NULL)
          throw EXCEPTIONS::Exception("ExchangeIface::KickHandler: context TERM_REQUEST termReqNode=NULL");;

        std::string answerStr = DefaultAnswer;
        std::string http_code, http_message;
        if(resp) {
            answerStr = resp->text;
            size_t fnd_b = answerStr.find("HTTP"); //!!!<answer>
            fnd_b = answerStr.find(" ",fnd_b+std::string("HTTP").size());
            size_t fnd_e = answerStr.find(" ",fnd_b+1);
            if ( fnd_b != std::string::npos &&
                 fnd_e != std::string::npos ) {
              http_code = answerStr.substr( fnd_b+1, fnd_e-fnd_b-1 );
              fnd_b = fnd_e;
              fnd_e = answerStr.find("\n",fnd_b);
              if ( fnd_b != std::string::npos &&
                   fnd_e != std::string::npos ) {
                http_message = answerStr.substr( fnd_b + 1, fnd_e - fnd_b - std::string("\r\n").size() );
                fnd_e = answerStr.find( "\r\n\r\n", fnd_e + 1 );
                if ( fnd_e != std::string::npos ) {
                  answerStr.erase( answerStr.begin(), answerStr.begin() + fnd_e + std::string("\r\n\r\n").size() );
                }
              }
            }
        }

        XMLDoc answerResDoc;
        try
        {
          answerResDoc = ASTRA::createXmlDoc2(answerStr);
        }
        catch(std::exception &e)
        {
          LogError(STDLOG) << "ASTRA::createXmlDoc2(answerStr) error " << http_code << " " << http_message;
          answerResDoc = ASTRA::createXmlDoc2(DefaultAnswer);
        }
        LogTrace(TRACE5) << answerResDoc.docPtr()->children->name;
        xmlNodePtr errnode = NewTextChild(answerResDoc.docPtr()->children, "error");
        NewTextChild( errnode, "code", http_code );
        NewTextChild( errnode, "message", http_message );
        LogTrace(TRACE5) << answerResDoc.text();
        std::string exchangeId;
        BeforeResponseHandle(req_ctxt_id, termReqNode, answerResDoc.docPtr()->children, exchangeId);
        handleResponse(exchangeId, termReqNode, answerResDoc.docPtr()->children, resNode);
    }
}

void MPSExchangeIface::response_RegisterResult(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << "exchangeId=" << exchangeId;
  RegisterResult res;
  if ( externalSysResNode == nullptr ||
       externalSysResNode->children == nullptr ) {
    throw AstraLocale::UserException("MSG.MPS_CONNECT_ERROR");
  }
  int grp_id = NodeAsInteger("grp_id",reqNode);
  CheckIn::TSimplePaxGrpItem grpItem;
  if ( !grpItem.getByGrpId(grp_id) ) {
    throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
  }
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  LogTrace(TRACE5) << __func__ << " " << prices.getMPSOrderId();
  DBExchanger exchanger( RegisterMethod().methodName(), prices.getMPSOrderId() );
  exchanger.answer( grp_id, XMLTreeToText( externalSysResNode->doc ) );

  res.parseResponse(externalSysResNode);
  if ( res.error() ) {
    exchanger.changeStatus( DBExchanger::rtRequest, DBExchanger::stComplete );
    AstraLocale::showErrorMessage("MSG.MPS_PAYMENT_ERROR", AstraLocale::LParams()
                                      << AstraLocale::LParam("code",res.error_code)
                                      << AstraLocale::LParam("message",res.error_message) );
  }
  else {
    exchanger.changeStatus( DBExchanger::rtRequest, DBExchanger::stProcessed );
    AstraLocale::showMessage("MSG.MPS_PAYMENT");
    TLogLocale tlocale;
    tlocale.lexema_id = "EVT.MPS_DO_PAYMENT";
    tlocale.ev_type=ASTRA::evtPax;
    tlocale.id1 = grpItem.point_dep;
    tlocale.id2 = ASTRA::NoExists;
    tlocale.id3 = grp_id;
    LogTrace(TRACE5) << grp_id << "," << grpItem.point_dep <<',' << prices.getRegnum() << "," << prices.getMPSOrderId();
    tlocale.prms << PrmSmpl<std::string>("pnr", prices.getRegnum())
                        << PrmSmpl<std::string>("order_id", prices.getMPSOrderId() )
                        << PrmSmpl<std::string>("cost", Ticketing::TaxAmount::Amount(FloatToString(prices.getTotalCost())).amStr(Ticketing::CutZeroFraction(true)) )
                        << PrmSmpl<std::string>("currency", prices.getTotalCurrency() );
    TReqInfo::Instance()->LocaleToLog( tlocale );
    NewTextChild( resNode, "pr_wait_paid", 1 );
    NewTextChild( resNode, "timeout", 600 );//!!! 180000
  };
}

void MPSExchangeIface::response_CancelResult(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
  LogTrace(TRACE5) << "exchangeId=" << exchangeId;
  CancelResult res;
  if ( externalSysResNode == nullptr ||
       externalSysResNode->children == nullptr ) {
    throw AstraLocale::UserException("MSG.MPS_CONNECT_ERROR");
  }
  int grp_id = NodeAsInteger("grp_id",reqNode);
  CheckIn::TSimplePaxGrpItem grpItem;
  if ( !grpItem.getByGrpId(grp_id) ) {
    throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");
  }
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  LogTrace(TRACE5) << __func__ << " " << prices.getMPSOrderId();
  DBExchanger exchanger( CancelMethod().methodName(), prices.getMPSOrderId() );
  exchanger.answer( grp_id, XMLTreeToText( externalSysResNode->doc ) );

  res.parseResponse(externalSysResNode);
  if ( res.error() ) {
    exchanger.changeStatus( DBExchanger::rtRequest, DBExchanger::stComplete);
    AstraLocale::LParams lparams;
    lparams << AstraLocale::LParam("code",res.error_code)
            << AstraLocale::LParam("message",res.error_message);
    AstraLocale::showErrorMessage("MSG.MPS_PAYMENT_CANCEL_ERROR", lparams);
    NewTextChild( resNode, "paid_error", AstraLocale::getLocaleText("MSG.MPS_PAYMENT_CANCEL",lparams) );
  }
  else {
    tst();
    exchanger.changeStatus( DBExchanger::rtRequest, DBExchanger::stProcessed );
    AstraLocale::showMessage("MSG.MPS_PAYMENT_CANCEL");
    TLogLocale tlocale;
    tlocale.lexema_id = "EVT.MPS_CANCEL_FINISH";
    tlocale.ev_type=ASTRA::evtPax;
    tlocale.id1 = grpItem.point_dep;
    tlocale.id2 = ASTRA::NoExists;
    tlocale.id3 = grp_id;
    LogTrace(TRACE5) << grp_id << "," << grpItem.point_dep <<',' << prices.getRegnum() << "," << prices.getMPSOrderId();
    tlocale.prms << PrmSmpl<std::string>("pnr", prices.getRegnum())
                        << PrmSmpl<std::string>("order_id", prices.getMPSOrderId() )
                        << PrmSmpl<std::string>("cost", Ticketing::TaxAmount::Amount(FloatToString(prices.getTotalCost())).amStr(Ticketing::CutZeroFraction(true)) )
                        << PrmSmpl<std::string>("currency", prices.getTotalCurrency() );
    TReqInfo::Instance()->LocaleToLog( tlocale );
    NewTextChild( resNode, "paid_error", AstraLocale::getLocaleText("MSG.MPS_PAYMENT_CANCEL") );
  };
}

void SynchMPSEMD( int grp_id, const NotifyPushEvent &p );

void MPSExchangeIface::NotifyPushEvents(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  std::string xml = XMLTreeToText( reqNode->doc );
  LogTrace(TRACE5)<< __func__ << std::endl << xml;
  xmlNodePtr node = GetNode( "content", reqNode );
  node = GetNodeFast( "Envelope", node->children);
  node = GetNodeFast( "Body", node->children );
  node = NodeAsNodeFast( "notify", node->children );
  NotifyPushEvent event;
  event.fromXML( node->children );
  DBExchanger dbChanger( event.getNumber() );
  try {
    std::string notifyXML;
//    std::string method = NodeAsString( "method", reqNode, MPS::RegisterMethod().methodName().c_str() );
    MPS::DBExchanger::EnumStatus currStatus;
    if ( dbChanger.check_msg( currStatus, notifyXML, MPS::DBExchanger::rtNotify, MPS::DBExchanger::stComplete ) ) { // уже приходил notify
      return;
    }
    int grp_id = dbChanger.getRequestGrpId();
    LogTrace(TRACE5) << "grp_id=" << grp_id;
    CheckIn::TSimplePaxGrpItem grpItem;
    if ( !grpItem.getByGrpId(grp_id) ) {
      throw EXCEPTIONS::Exception("MPS notify event grp not found %d", grp_id );
      return;
    }
    //ставим задачу на обновление EMD & change status to F
    add_trip_task( TTripTaskKey(grpItem.point_dep,EMD_REFRESH_BY_GRP,IntToString(grp_id) ) );
    //EMDAutoBoundInterface::EMDRefresh(EMDAutoBoundGrpId(grp_id), reqNode); не работает!
    TLogLocale tlocale;
    NewTextChild( reqNode, "grp_id", grp_id );//!!!
    tlocale.ev_type = ASTRA::evtPax;
    tlocale.id1 = grpItem.point_dep;
    tlocale.id2 = ASTRA::NoExists;
    tlocale.id3 = grp_id;
    tlocale.prms << PrmSmpl<std::string>("pnr", event.getOrder())
                 << PrmSmpl<std::string>("order_id", event.getNumber() );

    if ( event.isGood() ) {
      SynchMPSEMD( grp_id, event );
      dbChanger.notifyOk( xml, DBExchanger::stNotify );
      tlocale.lexema_id = "EVT.MPS_COMPLETED";
    }
    else {
      dbChanger.notifyError( xml );
      tlocale.lexema_id = "EVT.MPS_ERROR";
      tlocale.prms << PrmSmpl<std::string>("error", event.GetError() );
    }
    TReqInfo::Instance()->LocaleToLog( tlocale );
  }
  catch(EXCEPTIONS::Exception &e) {
    LogError(STDLOG) << __func__ << " " << e.what();
    dbChanger.notifyError( xml );
  }
  catch(...) { //надо отвиснуть
    LogError(STDLOG) << __func__ << " unknown error";
    dbChanger.notifyError( xml );
  }
}

void DBExchanger::toDB(  int grp_id, const EnumRequestType& msg_type, const std::string& xml, const EnumStatus& status ) {
  LogTrace(TRACE5) << __func__ << " " << EncodeRequestType( msg_type ) << "," << ForderId << " xml="  << xml << ",status=" << EncodeStatus( status );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "INSERT INTO mps_exchange(request_type,method,grp_id,order_id,xml,page_no,time_create,status) "
    " VALUES(:request_type,:method,:grp_id,:order_id,:xml,:page_no,:time_create,:status)";
  Qry.CreateVariable( "request_type", otString, EncodeRequestType( msg_type ) );
  Qry.CreateVariable( "method", otString, Fmethod );
  Qry.CreateVariable( "grp_id", otInteger, grp_id );
  Qry.CreateVariable( "order_id", otString, ForderId );
  Qry.CreateVariable( "time_create", otDate,BASIC::date_time::NowUTC() );
  Qry.CreateVariable( "status", otString, EncodeStatus( status ) );
  Qry.DeclareVariable( "xml", otString );
  Qry.DeclareVariable( "page_no", otInteger );
  longToDB(Qry, "xml", xml, true);
}

void DBExchanger::changeStatus( const EnumRequestType& msg_type, const EnumStatus& status )
{
  LogTrace(TRACE5) << __func__ << ",ForderId=" << ForderId +  ",msg_type=" << EncodeRequestType( msg_type ) << ",method=" << Fmethod << " status=" <<  EncodeStatus( status );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "UPDATE mps_exchange SET status=:status WHERE order_id=:order_id AND request_type=:request_type AND method=:method";
  Qry.CreateVariable( "request_type", otString, EncodeRequestType( msg_type ) );
  Qry.CreateVariable( "method", otString, Fmethod );
  Qry.CreateVariable( "status", otString, EncodeStatus( status ) );
  Qry.CreateVariable( "order_id", otString, ForderId );
  Qry.Execute();
}

void DBExchanger::getNotifyEvents( int grp_id, NotifyEvents& events )
{
  events.clear();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT method, order_id, xml,page_no FROM mps_exchange WHERE request_type=:request_type AND status=:status AND grp_id=:grp_id "
    "ORDER BY order_id,method,page_no";
  Qry.CreateVariable( "request_type", otString, EncodeRequestType( rtNotify ) );
  Qry.CreateVariable( "status", otString, EncodeStatus( stNotify ) );
  Qry.CreateVariable( "grp_id", otInteger, grp_id );
  Qry.Execute();
  NotifyEvent event;
  for (; !Qry.Eof; Qry.Next() ) {
    LogTrace(TRACE5) << "page_no=" << Qry.FieldAsInteger("page_no");
    if ( event.order_id != Qry.FieldAsString("order_id") ||
         event.method != Qry.FieldAsString("method") ) {
      if ( !event.xml.empty() ) {
        events.emplace_back( event );
      }
      event.xml.clear();
      event.order_id = Qry.FieldAsString("order_id");
      event.method = Qry.FieldAsString("method");
    }
    event.xml.append(Qry.FieldAsString("xml"));
  }
  if ( !event.xml.empty() ) {
    events.emplace_back( event );
  }
}

int DBExchanger::getRequestGrpId()
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT grp_id FROM mps_exchange WHERE order_id=:order_id AND request_type=:request_type AND method=:method AND page_no=1";
  Qry.CreateVariable( "request_type", otString, EncodeRequestType( EnumRequestType::rtRequest ) );
  Qry.CreateVariable( "method", otString, Fmethod );
  Qry.CreateVariable( "order_id", otString, ForderId );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw EXCEPTIONS::Exception("MPS OrderStatusChanger::getGrpId: order not found %s", ForderId.c_str());
  }
  return Qry.FieldAsInteger( "grp_id" );
}

std::string DBExchanger::getFirstProcessedMethod()
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT method FROM mps_exchange WHERE order_id=:order_id AND request_type=:request_type AND page_no=1 "
    " ORDER BY time_create";
  Qry.CreateVariable( "request_type", otString, EncodeRequestType(rtRequest) );
  Qry.CreateVariable( "order_id", otString, ForderId );
  Qry.Execute();
  return Qry.Eof?std::string(""):std::string(Qry.FieldAsString( "method" ));
}
/*
 *
 <soap-env:Envelope xmlns:soap-env="http://schemas.xmlsoap.org/soap/envel
  <soap-env:Body>
    <soap-env:Fault>
      <faultcode>registerFault</faultcode>
      <faultstring>WRONG_AMOUNT</faultstring>
      <detail/>
    </soap-env:Fault>
  </soap-env:Body>
  <soap-env:error>
    <soap-env:code>500</soap-env:code>
    <soap-env:message>WRONG_AMOUNT</soap-env:message>
  </soap-env:error>

<notify xmlns="http://www.sirena-travel.ru">
<order>
<shop_id>555</shop_id>
<number>47070664</number>
</order>
<status>not_authorized</status>
<shopref/>
<error>
<category>user</category>
<code>timeout</code>
</error>
<payments/>
</notify>
*/

std::string DBExchanger::EncodeStatus( const EnumStatus& status ) {
  switch( status ) {
    case stSend: return "send";
    case stProcessed: return "processed";
    case stNotify: return "notify";
    case stComplete: return "complete";
    //case stError: return "error";
    default:
      throw EXCEPTIONS::Exception("Invalid encode DBExchanger::EnumStatus" );
  }
}

DBExchanger::EnumStatus DBExchanger::DecodeStatus( const std::string& status ) {
  if ( status == "send" ) {
    return stSend;
  }
  if ( status == "processed" ) {
    return stProcessed;
  }
  if ( status == "notify" ) {
    return stNotify;
  }
  if ( status == "complete" ) {
    return stComplete;
  }
  //if ( status == "error" ) {
//    return stError;
//  }
  throw EXCEPTIONS::Exception("Invalid decode DBExchanger::EnumStatus %s", status.c_str() );
}

std::string DBExchanger::EncodeRequestType( const EnumRequestType& request_type ) {
  switch( request_type ) {
    case rtRequest: return "request";
    case rtAnswer: return "answer";
    case rtNotify: return "notify";
    default:
      throw EXCEPTIONS::Exception("Invalid encode DBExchanger::EncodeRequestType" );
  }
}

DBExchanger::EnumRequestType DBExchanger::DecodeRequestType( const std::string& request_type ) {
  if ( request_type == "request" ) {
    return rtRequest;
  }
  if ( request_type == "answer" ) {
    return rtAnswer;
  }
  if ( request_type == "notify" ) {
    return rtNotify;
  }
  throw EXCEPTIONS::Exception("Invalid decode DBExchanger::DecodeRequestType %s", request_type.c_str() );
}

void DBExchanger::request( int grp_id, const std::string& xml ) {
  toDB( grp_id, EnumRequestType::rtRequest, xml, stSend );
}

void DBExchanger::answer( int grp_id, const std::string& xml ) {
  toDB( grp_id, EnumRequestType::rtAnswer, xml, EnumStatus::stComplete );
}

void DBExchanger::notifyError( const std::string& xml ) {
  toDB( getRequestGrpId(), rtNotify, xml, stComplete );
  changeStatus( rtRequest, stComplete );
}

void DBExchanger::notifyOk( const std::string& xml, const EnumStatus& status ) {
  toDB( getRequestGrpId(), EnumRequestType::rtNotify, xml, status );
  changeStatus( rtRequest, stComplete );
  changeStatus( rtNotify, MPS::DBExchanger::stComplete );
}

bool DBExchanger::check_msg( std::string& xml, const EnumRequestType& msg_type, const EnumStatus& status ) {
  EnumStatus currStatus;
  return check_msg( currStatus, xml, msg_type, status );
}

bool DBExchanger::check_msg( EnumStatus& currStatus, std::string& xml, const EnumRequestType& msg_type, const EnumStatus& status ) {
  TQuery Qry(&OraSession);
  if ( status == stAny ) {
    Qry.SQLText =
      "SELECT xml, status FROM mps_exchange WHERE order_id=:order_id AND request_type=:request_type AND method=:method "
      "ORDER BY page_no";
  }
  else {
    Qry.SQLText =
      "SELECT xml, status FROM mps_exchange WHERE order_id=:order_id AND request_type=:request_type AND method=:method AND status=:status "
      "ORDER BY page_no";
    Qry.CreateVariable( "status", otString, EncodeStatus( status ) );
  }
  Qry.CreateVariable( "request_type", otString, EncodeRequestType( msg_type ) );
  Qry.CreateVariable( "method", otString, Fmethod );
  Qry.CreateVariable( "order_id", otString, ForderId );
  Qry.Execute();
  xml.clear();
  for ( ;!Qry.Eof; Qry.Next() ) {
     xml.append(Qry.FieldAsString("xml"));
     currStatus = DecodeStatus( Qry.FieldAsString("status") );
  }
  if ( xml.empty() ) {
    return false;
  }
  xml = ConvertCodepage(xml,"CP866", "UTF-8");
  LogTrace(TRACE5) << __func__ << " ok";
  return true;
}

void DBExchanger::stopWaitNotify() {
  tst();
  changeStatus( EnumRequestType::rtNotify,  EnumStatus::stComplete );
}

bool DBExchanger::alreadyRequest() {
  std::string xml;
  return check_msg( xml, EnumRequestType::rtRequest, EnumStatus::stAny );
}

//=================================================
void TermPos::toXML(  xmlNodePtr node ) const {
  NewTextChild( node, "id", id );
  NewTextChild( node, "name", name );
  NewTextChild( node, "address", address );
  NewTextChild( node, "serial", serial );
  NewTextChild( node, "vendor", vendor );
  if ( inUse ) {
    NewTextChild( node, "inUse" );
  }
}


void PosAssignator::checkTimeout( ) {
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "DELETE pos_term_assign "
    " WHERE time_create < :time_create";
  Qry.CreateVariable( "time_create", otDate,  BASIC::date_time::NowUTC() );
  Qry.Execute();
}

void PosAssignator::fromDB( int point_id ) {
  clear();
  TTripInfo fltInfo;
  if (!fltInfo.getByPointId(point_id))
    throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  if ( inUse( ) &&
       (fltInfo.airline != FAirline ||
        fltInfo.airp != FAirp) ) {
    ReleasePos( );
  }
  checkTimeout();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT s.id,s.name,s.address,s.serial,v.code "
    " FROM pos_term_sets s, pos_term_vendors v "
    "WHERE s.vendor_id=v.id AND "
    "      s.airline=:airline AND "
    "      s.airp=:airp AND "
    "      s.pr_denial=0 AND "
    "      s.id NOT IN (SELECT pos_id FROM pos_term_assign WHERE desk<>:desk) "
    "ORDER BY name";
  Qry.CreateVariable( "airline", otString, fltInfo.airline );
  Qry.CreateVariable( "airp", otString, fltInfo.airp );
  Qry.CreateVariable( "desk", otString,  TReqInfo::Instance()->desk.code );
  Qry.Execute();
  for ( ;!Qry.Eof; Qry.Next() ) {
    TermPos t;
    t.id = Qry.FieldAsInteger( "id" );
    t.name = Qry.FieldAsString( "name" );
    t.address = Qry.FieldAsString( "address" );
    t.serial = Qry.FieldAsString( "serial" );
    t.vendor = Qry.FieldAsString( "code" );
    t.inUse = ( t.id == FPosId );
    termPoses.push_back( t );
  }
}

void PosAssignator::toXML( xmlNodePtr node ) {
  for ( const auto& t : termPoses ) {
    xmlNodePtr n = NewTextChild( node, "item" );
    t.toXML( n );
  }
}

void PosAssignator::AssignPos( int pos_id ) {
  checkTimeout();
  if ( pos_id == ASTRA::NoExists ) {
    throw AstraLocale::UserException("MSG.MPS_POS_NOTSELECTED");
  }
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "BEGIN "
    " UPDATE pos_term_assign SET pos_id=:pos_id,time_create=:time_create "
    " WHERE desk=:desk;"
    "  IF SQL%NOTFOUND THEN "
    "    INSERT INTO pos_term_assign(pos_id, time_create, desk) "
    "    VALUES(:pos_id, :time_create, :desk); "
    "  END IF;"
    "END;";
  Qry.CreateVariable( "pos_id", otInteger,  pos_id );
  Qry.CreateVariable( "desk", otString,  TReqInfo::Instance()->desk.code );
  Qry.CreateVariable( "time_create", otDate, BASIC::date_time::NowUTC() + PosAssignator::TIME_OUT/1440.0 );
  try {
    Qry.Execute();
  }
  catch(const EOracleError& E) {
    if (E.Code == 1) {
      throw AstraLocale::UserException("MSG.MPS_POS_INUSE");
    }
  }
}

void PosAssignator::ReleasePos( ) {
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "DELETE pos_term_assign WHERE desk=:desk";
  Qry.CreateVariable( "desk", otString,  TReqInfo::Instance()->desk.code );
  Qry.Execute();
  FPosId = ASTRA::NoExists;
  FAirline.clear();
  FAirp.clear();
}

bool PosAssignator::inUse( )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT a.pos_id, s.airline, s.airp "
    " FROM pos_term_assign a, pos_term_sets s "
    " WHERE a.desk=:desk AND "
    "       s.id=a.pos_id AND "
    "       s.pr_denial=0";
  Qry.CreateVariable( "desk", otString, TReqInfo::Instance()->desk.code );
  Qry.Execute();
  if ( Qry.Eof ) {
    FPosId = ASTRA::NoExists;
    FAirline.clear();
    FAirp.clear();
  }
  else {
    FPosId = Qry.FieldAsInteger( "pos_id" );
    FAirline = Qry.FieldAsString( "airline");
    FAirp = Qry.FieldAsString( "airp");
  }
  return ( FPosId != ASTRA::NoExists );
}

void PosClient::fromDB( int posId )
{
  clear();
  PosAssignator posAssignator;
  posAssignator.AssignPos( posId );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT pay_clients.client_id, shop_id,serial,pos_term_vendors.code vendor "
    " FROM pay_clients, pos_term_sets, pos_term_vendors "
    " WHERE pos_term_sets.id=:pos_id AND "
    "       pos_term_sets.vendor_id=pos_term_vendors.id AND "
    "       pos_term_sets.pr_denial=0 AND "
    "       pay_clients.id=pos_term_sets.client_id AND "
    "       pay_clients.pr_denial=0";
  Qry.CreateVariable( "pos_id", otInteger,  posId );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw AstraLocale::UserException("MSG.MPS_POS_NOTSELECTED_OR_TIMEOUT");
  }
  sirena_id = Qry.FieldAsInteger( "client_id" );
  mps_shopid = Qry.FieldAsInteger( "shop_id" );
  vendor = Qry.FieldAsString( "vendor" );
  serial = Qry.FieldAsString( "serial" );
}

void SynchMPSEMD( int grp_id, const NotifyPushEvent &p )
{
  tst();
  std::vector<PaySVC> mps_svcs;
  p.getSVCS( mps_svcs );
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  prices.setMPSOrderId( p.getNumber() );
  prices.setRegnum( p.getOrder() );
  if ( mps_svcs.empty() ) {
    throw EXCEPTIONS::Exception( "svcs not found at MPS notify answer" );
  }
  for ( auto& item : prices ) {
    SVCS svcs;
    item.second.getSVCS( svcs, TPriceRFISCList::STATUS_DIRECT_SELECTED );
    for ( const auto& svc : svcs ) {
      LogTrace(TRACE5) << " svc orderid=" << svc.first.orderId;
      if ( convert_pnr_addr(svc.first.orderId,true) != p.getOrder() ) {
        continue;
      }
      bool prFind = false;
      //find in mps answer ???
      for ( const auto& mps_svc : mps_svcs ) {
        LogTrace( TRACE5 ) << mps_svc.toString();
        LogTrace( TRACE5 ) << "item.first.RFISC=" << item.first.RFISC;
        LogTrace( TRACE5 ) << "item.first.RFISC=" << ServiceTypes().encode(item.first.service_type);
        LogTrace( TRACE5 ) << " svc.second.pass_id=" <<  svc.second.pass_id;
        LogTrace( TRACE5 ) << " svc.second.seg_id=" <<  svc.second.seg_id;

        if ( mps_svc.rfisc == item.first.RFISC &&
             //mps_svc.rfic == svc.second.rfic &&
             ServiceTypes().decode(mps_svc.service_type) == item.first.service_type &&
             mps_svc.pass_id == svc.second.pass_id &&
             mps_svc.seg_id == svc.second.seg_id ) {
          SVCS::iterator f;
          if ( !item.second.findSVC( svc.first, f ) ) {
            throw EXCEPTIONS::Exception( "svc %d not found at TPriceRFISCList", svc.first.svcId );
          }
          f->second.ticknum = mps_svc.emd_number;
          f->second.ticket_cpn = mps_svc.emd_coupon;
          f->second.status_direct = TPriceRFISCList::STATUS_DIRECT_PAID;
          LogTrace(TRACE5) << "svc_id=" << svc.first.svcId << " emd_no=" << f->second.ticknum << "/" << f->second.ticket_cpn;
          prFind = true;
        }
      }
      if ( !prFind ) {
        throw EXCEPTIONS::Exception( "svc %d not found at MPS notify answer", svc.first.svcId );
      }
    }
  }
  prices.SvcEmdPayDoc::clear();
  //prices.SvcEmdCost::clear();
  prices.SvcEmdTimeout::clear();
  prices.toContextDB(grp_id);
  prices.toDB(grp_id);
}

int TIMELIMIT_SEC()       //секунды
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("MPS_TIMELIMIT",0,1000,0);
  return VAR;
}

/*void MPSExchangeIface::RegisterNotifyParseTest()
{
tst();
  return;
  tst();
  try {
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT order_id,method,xml,page_no FROM mps_exchange WHERE grp_id=2603192 and request_type='notify'  and time_create> sysdate-1 "
      "ORDER BY order_id,method,page_no";
    Qry.Execute();
    tst();
    NotifyEvent event;
    for (; !Qry.Eof; Qry.Next() ) {
      LogTrace(TRACE5) << "page_no=" << Qry.FieldAsInteger("page_no");
      if ( event.order_id != Qry.FieldAsString("order_id") ) {
        if ( !event.xml.empty() ) {
          NotifyPushEvent p;
          tst();
          event.xml = ConvertCodepage(event.xml,"CP866", "UTF-8");
          p.fromStr( event.xml );
          int grp_id = 2603192;
          SynchMPSEMD( grp_id, p );
          EMDAutoBoundInterface::EMDRefresh(EMDAutoBoundGrpId(grp_id), nullptr);
          tst();
        }
         tst();
        event.xml.clear();
        event.order_id = Qry.FieldAsString("order_id");
        event.method = Qry.FieldAsString("method");
      }
      event.xml.append(Qry.FieldAsString("xml"));
    }
        if ( !event.xml.empty() ) {
          NotifyPushEvent p;
          tst();
          event.xml = ConvertCodepage(event.xml,"CP866", "UTF-8");
          p.fromStr( event.xml );
          int grp_id = 2603192;
          SynchMPSEMD( grp_id, p );
          EMDAutoBoundInterface::EMDRefresh(EMDAutoBoundGrpId(grp_id), nullptr);
          tst();
        }
  }

  catch(std::exception &e)
  {
    LogError(STDLOG) << "ASTRA::createXmlDoc2(answerStr) error " << e.what();
  }
}*/

}

/*
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"
xmlns:xs="http://www.w3.org/2001/XMLSchema"
xmlns:xsi="http://www.w3.org/1999/XMLSchema-instance"
xmlns:wsa="http://schemas.xmlsoap.org/ws/2004/08/addressing">
<SOAP-ENV:Body>
<notify xmlns="http://www.sirena-travel.ru">
<order><shop_id>555</shop_id>
<number>47048790</number></order>
<status>not_authorized</status>
<shopref></shopref>
<error>
<category>user</category>
<code>timeout</code>
</error>
<payments></payments>
</notify>
</SOAP-ENV:Body></SOAP-ENV:Envelope>'
*/

