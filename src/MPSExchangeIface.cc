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
#include "edi_utils.h"
#include "rfisc_price.h"
#include "etick/tick_data.h"
#include "etick.h"

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
        useSSL = false;
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
      LogTrace(TRACE5) << os.str();
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
    xmlNodePtr fnode = GetNodeFast( "registerResponse", node->children );
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

void PushEvents::parseResponse(xmlNodePtr node)
{
  errorFromXML(node);
  xmlNodePtr fnode = GetNodeFast( "Body", node->children );
/*<notify xmlns="http://www.sirena-travel.ru">
<order><shop_id>555</shop_id>
<number>47048790</number></order>
<status>not_authorized</status>
<shopref></shopref>
<error>
<category>user</category>
<code>timeout</code>
</error>
<payments></payments>
  */
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

void Amount::SetCurrency( const std::string &Acurrency ) {
  Fcurrency = ElemIdToPrefferedElem(etCurrency, Acurrency, efmtCodeNative, AstraLocale::LANG_EN);
}

void Amount::toXML( xmlNodePtr node ) const {
  NewTextChild( node, "amount", Famount.amStr(Ticketing::CutZeroFraction(true)) );
  NewTextChild( node, "currency", Fcurrency );
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
    val = BASIC::date_time::StrToDateTime( "YYYY-MM-DDTHH24:MI:SS:000",d);
  }
  else {
    val = BASIC::date_time::StrToDateTime( "THH24:MI:SS:000",d);
  }
   NewTextChild( node, "timelimit", val );
}

void OrderFull::toXML( xmlNodePtr node ) const {
  Ftimelimit.toXML( node );
  Fparts.toXML( node );
  Fsales.toXML( node );
  Frefunds.toXML( node );
  NewTextChild( node, "shopref", Fshopref );
}

 void PostEntry::toXML( xmlNodePtr node ) const {
   NewTextChild( node, "name", Fname );
   NewTextChild( node, "value", Fvalue );
}

void RegisterMethod::toXML( xmlNodePtr node ) const {
  xmlNodePtr Fnode = NewTextChild( node, getObjectName().c_str() );
  SetProp( Fnode, "xmlns", "http://www.sirena-travel.ru" );
  Forder.toXML( Fnode );
  Fcost.toXML( Fnode );
  Fcustomer.toXML( Fnode );
  Fdescription.toXML( Fnode );
  Fpostdata.toXML( Fnode );
}

void RegisterMethod::trace( xmlNodePtr node ) {
  LogError(STDLOG) << XMLTreeToText( node->doc );
}

std::string RegisterMethod::getUniqueNumber() const
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
   "SELECT events__seq.nextval AS id FROM dual";
  Qry.Execute();
  return Qry.FieldAsString( "id");
}

/*
 * CREATE TABLE mps_requests (
  request_type VARCHAR2(10) NOT NULL,
  order_id VARCHAR2(9) NOT NULL,
  grp_id NUMBER(9) NOT NULL,
  xml VARCHAR2(4000) NOT NULL,
  page_no NUMBER(3) NOT NULL,
  time_create DATE NOT NULL
);
*/

void XMLMPS_ToDB( int grp_id, const std::string& request_type, const std::string& orderId, const std::string xml, const std::string& status )
{
  LogTrace(TRACE5) << __func__ << " " << request_type << "," << orderId << " xml="  << xml << ",status=" << status;
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "INSERT INTO mps_requests(request_type,order_id,grp_id,xml,page_no,time_create,status) "
    " VALUES(:request_type,:order_id,:grp_id,:xml,:page_no,:time_create,:status)";
  Qry.CreateVariable( "grp_id", otInteger, grp_id );
  Qry.CreateVariable( "order_id", otString, orderId );
  Qry.CreateVariable( "request_type", otString, request_type );
  Qry.CreateVariable( "time_create", otDate,BASIC::date_time::NowUTC() );
  Qry.CreateVariable( "status", otString, status );
  Qry.DeclareVariable( "xml", otString );
  Qry.DeclareVariable( "page_no", otInteger );
  LogTrace(TRACE5) << __func__ << std::endl << xml;
  longToDB(Qry, "xml", xml, true);
}

void RegisterMethod::toDB( int grp_id, const std::string& request_type )
{
   xmlNodePtr rootNode;
   XMLDoc doc("RegisterMethod",rootNode,__func__);
   toXML( rootNode );
   std::string value = XMLTreeToText( doc.docPtr() );
   XMLMPS_ToDB( grp_id, "request", getOrderId(), value, "send" );
}

void RegisterMethod::request( xmlNodePtr reqNode )
{
  int grp_id = NodeAsInteger("grp_id",reqNode);
  toDB( grp_id, "request" );
  MPS::MPSExchangeIface::Request( reqNode, 101, MPSExchangeIface::getServiceName(), *this );
}

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
        tst();
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
                tst();
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

void SetMPSReuqestsStatus( int grp_id, const std::string& status, const std::string& new_status, const std::string& order_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "UPDATE mps_requests SET status=:new_status WHERE order_id=:order_id AND request_type=:request_type AND grp_id=:grp_id AND status=:status AND page_no=1";
  Qry.CreateVariable( "grp_id", otInteger, grp_id );
  Qry.CreateVariable( "request_type", otString, "request" );
  Qry.CreateVariable( "status", otString, status );
  Qry.CreateVariable( "new_status", otString, new_status );
  Qry.CreateVariable( "order_id", otString, order_id );
  Qry.Execute();
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
  SetMPSReuqestsStatus( grp_id, "send", "receive",  prices.getMPSOrderId() );
  LogTrace(TRACE5) << __func__ << " " << prices.getMPSOrderId();
  XMLMPS_ToDB( grp_id, "answer", prices.getMPSOrderId(), XMLTreeToText( externalSysResNode->doc ), "" );

  res.parseResponse(externalSysResNode);
  if ( res.error() ) {
    AstraLocale::showErrorMessage("MSG.MPS_PAYMENT_ERROR", AstraLocale::LParams()
                                      << AstraLocale::LParam("code",res.error_code)
                                      << AstraLocale::LParam("message",res.error_message) );
  }
  else {
    tst();
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
  };
}

void MPSExchangeIface::StopPaid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int grp_id = NodeAsInteger("grp_id",reqNode);
  TPriceRFISCList prices;
  prices.fromContextDB(grp_id);
  SetMPSReuqestsStatus( grp_id, "send", "error",  prices.getMPSOrderId() );
  SetMPSReuqestsStatus( grp_id, "receive", "error",  prices.getMPSOrderId() );
  NewTextChild( resNode, "paid_finish" );
}

void parseMPS_PUSH_Events( xmlNodePtr node )
{
  tst();
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
}
