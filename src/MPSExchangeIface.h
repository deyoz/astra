#ifndef MPSEXCHANGEIFACE_H
#define MPSEXCHANGEIFACE_H
#include "ExchangeIface.h"
#include "date_time.h"
#include "etick/tick_data.h"

namespace MPS
{

class MPSLogger
{
  private:
    bool isLogging;
  public:
    void toLog( xmlDocPtr doc );
};

class MPSExchange: public SirenaExchange::TExchange
{
  private:
    int clientId;
    std::string Authorization;
    std::string Resource;
  public:
    std::string getAuthorization() {
      return Authorization;
    }
    std::string getResource() {
      return Resource;
    }
    virtual std::string methodName() const = 0;
    void fromDB(int clientId);
    virtual void build(std::string &content) const;
    virtual void errorFromXML(xmlNodePtr node);
    virtual void errorToXML(xmlNodePtr node) const{}
    virtual void parseResponse(xmlNodePtr node);
    bool isEmptyAnswer(xmlNodePtr);
};

class checkEmpty {
  private:
    bool FEmpty;
  public:
    class Stop{};
    checkEmpty() {
      FEmpty = true;
    }
    checkEmpty &operator<<(bool isNottEmpty) {
      FEmpty &= !isNottEmpty;
      return *this;
    }
    bool operator<<(const Stop&) {
      return FEmpty;
    }
};

class SimpleSoapObject {
  private:
    std::string Fname;
  public:
    SimpleSoapObject( std::string Aname ) {
      Fname = Aname;
    }
    virtual ~SimpleSoapObject() {}
    std::string getObjectName() const {
      return Fname;
    }
};

template <typename T>
class SimpleSoapObjectOpt: public SimpleSoapObject {
  private:
    boost::optional<T> Fvalue;
  public:
    void setObject( const T& Avalue ) {
      Fvalue.emplace( Avalue );
    }
    boost::optional<T> getObject() const {
      return Fvalue;
    }
    virtual bool isEmpty() const {
      return ( Fvalue == boost::none );
    }
    void toXML( xmlNodePtr node ) const {
      if ( node == nullptr ) {
        throw EXCEPTIONS::Exception( getObjectName() + "::toXML: node is nullptr");
      }
      xmlNodePtr Fnode = NewTextChild( node, getObjectName().c_str() );
      if ( isEmpty() ) {
        SetProp( Fnode, "xsi:nil", "true" );
      }
      else {
        Fvalue.get().toXML( Fnode );
      }
    }
    SimpleSoapObjectOpt( std::string Aname ):SimpleSoapObject( Aname ){}
    ~SimpleSoapObjectOpt(){}
};

template <typename T>
class SimpleSoapArrayObject: public SimpleSoapObject {
  private:
    std::vector<T> vecSimpleSoapObject;
  public:
    SimpleSoapArrayObject( const std::string& Aname ):SimpleSoapObject( Aname ) {}
    virtual ~SimpleSoapArrayObject() {}
    void toXML( xmlNodePtr node ) const;
    void addItem( const T& item ) {
      vecSimpleSoapObject.emplace_back( item );
    }
};

class TXSDateTime:public SimpleSoapObject {
  private:
    boost::optional<BASIC::date_time::TDateTime> Ftimelimit;
  public:
    TXSDateTime( const std::string& Aname ):SimpleSoapObject( Aname ) {}
    virtual ~TXSDateTime() {}
    void toXML( xmlNodePtr node ) const;
    void setTime( const BASIC::date_time::TDateTime& value ) {
      Ftimelimit = value;
    }
};

class OrderID:public SimpleSoapObject {
  private:
    int Fshop_id;
    std::string Fnumber;
  public:
    void SetShop_Id( int Ashop_id ) {
      Fshop_id = Ashop_id;
    }
    void SetNumber( const std::string& Anumber ) {
      Fnumber = Anumber;
    }
    std::string getNumber() const {
      return Fnumber;
    }
    OrderID():SimpleSoapObject( "order" ) {
      Fshop_id = 0;
      Fnumber.clear();
    }
    virtual ~OrderID() {}
    void toXML( xmlNodePtr node ) const;
    void fromXML( xmlNodePtr node );
};

class Amount:public SimpleSoapObject {
  private:
    Ticketing::TaxAmount::Amount Famount;
    std::string Fcurrency;
  public:
    void SetAmount( Ticketing::TaxAmount::Amount Aamount ) {
      Famount = Aamount;
    }
    void SetCurrency( const std::string &Acurrency );
    Amount(const std::string& Name):SimpleSoapObject( Name ) {}
    virtual ~Amount() {}
    void toXML( xmlNodePtr node ) const;
    void fromXML( xmlNodePtr node );
};

class CustomerInfo:public SimpleSoapObject {
  private:
    std::string Fphone;
    std::string Femail;
    std::string Fname;
    std::string Fid;
  public:
    void Setphone( const std::string& Aphone )  {
      Fphone = Aphone;
    }
    void Setemail( const std::string& Aemail ) {
      Femail = Aemail;
    }
    void Setname( const std::string& Aname ) {
      Fname = Aname;
    }
    void Setid( const std::string& Aid ) {
      Fid = Aid;
    }
    CustomerInfo():SimpleSoapObject( "сustomer" ) {}
    virtual ~CustomerInfo() {}
    void toXML( xmlNodePtr node ) const;
};

class String: public SimpleSoapObject {
  private:
    std::string value;
  public:
    String():SimpleSoapObject( "string" ) {}
    virtual ~String() {}
    void toXML( xmlNodePtr node ) const;
};

class ArrayOfString: public SimpleSoapArrayObject<String> {
  public:
    ArrayOfString( const std::string& Name ):SimpleSoapArrayObject(Name){}
    virtual ~ArrayOfString() {}
};

class AgentInfo: public SimpleSoapObject {
  private:
    std::string Ftype;
  public:
    void Settype( const std::string& Atype )  {
      Ftype = Atype;
    }
    AgentInfo():SimpleSoapObject( "agent_info" ) {}
    virtual ~AgentInfo() {}
    void toXML( xmlNodePtr node ) const;
};

class SupplierInfo:public SimpleSoapObject {
  private:
    std::string Fname;
    std::string Finn;
    std::string Fphone;
  public:
    void Setname( const std::string& Aname ) {
      Fname = Aname;
    }
    void Setinn( const std::string& Ainn ) {
      Finn = Ainn;
    }
    void Setphone( const std::string Aphone ) {
      Fphone = Aphone;
    }
    SupplierInfo():SimpleSoapObject( "supplier_info" ) {}
    virtual ~SupplierInfo() {}
    void toXML( xmlNodePtr node ) const;
};

class Tax:public SimpleSoapObject {
  private:
    std::string Fname;
    std::string Fpercentage;
    std::string Fsource;
    SimpleSoapObjectOpt<Amount> Famount;
  public:
    void Setname( const std::string& Aname ) {
      Fname = Aname;
    }
    void Setpercentage( const std::string& Apercentage ) {
      Fpercentage = Apercentage;
    }
    void Setsource( const std::string& Asource ) {
      Fsource = Asource;
    }
    void Setamount( const Amount& Aamount ) {
      Famount.setObject( Aamount );
    }
    Tax():SimpleSoapObject( "tax" ), Famount( "amount" ) {} //???
    virtual ~Tax() {}
    void toXML( xmlNodePtr node ) const;
};

class taxArray:public SimpleSoapArrayObject<Tax> {
  public:
    taxArray( ):SimpleSoapArrayObject( "taxes" ) {}
    virtual ~taxArray() {}
};


class OrderItem:public SimpleSoapObject {
  private:
    std::string Fname;
    std::string Ftypename;
    std::string Fdescr;
    std::string Fhost;
    std::string Ftaxation_item_settlement_method;
    int Fquantity;
    std::string Fnumber;
    std::string Ftaxation_item_type;
    ArrayOfString Fdocuments;
    SimpleSoapObjectOpt<AgentInfo> Fagent_info;
    std::string Fmeasure;
    SimpleSoapObjectOpt<SupplierInfo> Fsupplier_info;
    std::string Fclearing;
    std::string Faccode;
    std::string Ftaxation_system;
    taxArray Ftaxes;
    SimpleSoapObjectOpt<Amount> Famount;
    std::string Fshopref;
    std::string Fref;
  public:
    OrderItem():SimpleSoapObject( "OrderItem" ), Fdocuments( "documents" ), Fagent_info( "agent_info" ), Fsupplier_info( "supplier_info" ), Famount( "amount" ) {
      Fquantity = 0;
    }
    virtual ~OrderItem() {}
    void toXML( xmlNodePtr node ) const;
    void setName( const std::string& Aname ) {
      Fname = Aname;
    }
    void setTypename( const std::string& Atypename ) {
      Ftypename = Atypename;
    }
    void setDescr( const std::string& Adescr ) {
      Fdescr = Adescr;
    }
    void setHost( const std::string& Ahost ) {
      Fhost = Ahost;
    }
    void setTaxation_item_settlement_method( const std::string& Ataxation_item_settlement_method ) {
      Ftaxation_item_settlement_method = Ataxation_item_settlement_method;
    }
    void setQuantity( int Aquantity ) {
      Fquantity = Aquantity;
    }
    void setNumber( const std::string& Anumber ) {
      Fnumber = Anumber;
    }
    void setTaxation_item_type( const std::string& Ataxation_item_type ) {
      Ftaxation_item_type = Ataxation_item_type;
    }
    void setDocuments( const ArrayOfString& Adocuments ) {
      Fdocuments = Adocuments;
    }
    void setAgent_info( const AgentInfo& Aagent_info ) {
      Fagent_info.setObject( Aagent_info );
    }
    void setMeasure( const std::string& Ameasure ) {
      Fmeasure = Ameasure;
    }
    void setSupplier_info( const SupplierInfo& Asupplier_info ) {
      Fsupplier_info.setObject( Asupplier_info );
    }
    void setClearing( const std::string& Aclearing ) {
      Fclearing = Aclearing;
    }
    void setAccode( const std::string& Aaccode ) {
      Faccode = Aaccode;
    }
    void setTaxation_system( const std::string& Ataxation_system ) {
      Ftaxation_system = Ataxation_system;
    }
    void setTaxes( const taxArray& Ataxes ) {
      Ftaxes = Ataxes;
    }
    void setAmount( const Amount& Aamount ) {
      Famount.setObject( Aamount );
    }
    void setShopref( const std::string& Ashopref ) {
      Fshopref = Ashopref;
    }
    void setRef( const std::string& Aref ) {
      Fref = Aref;
    }
};

class OrderItemArray: public SimpleSoapArrayObject<OrderItem> {
  public:
    OrderItemArray( const std::string& Name ):SimpleSoapArrayObject(Name){}
    virtual ~OrderItemArray() {}
};

class PaymentPart:public SimpleSoapObject {
  private:
    SimpleSoapObjectOpt<Amount> Fsource;
    std::string Fid;
    SimpleSoapObjectOpt<Amount> Famount;
    std::string Ftype;
    std::string Fref;
  public:
    PaymentPart():SimpleSoapObject( "PaymentPart" ), Fsource("source"), Famount("amount") {}
    virtual ~PaymentPart() {}
    void toXML( xmlNodePtr node ) const;
};

class PaymentPartArray:public SimpleSoapArrayObject<PaymentPart> {
  public:
    PaymentPartArray( const std::string& Name ):SimpleSoapArrayObject(Name){}
    virtual ~PaymentPartArray() {}
};

class OrderFull:public SimpleSoapObject {
  private:
    SimpleSoapObjectOpt<TXSDateTime> Ftimelimit;
    PaymentPartArray Fparts;
    OrderItemArray Fsales;
    OrderItemArray Frefunds;
    std::string Fshopref;
  public:
    OrderFull():SimpleSoapObject( "description" ), Ftimelimit( "timelimit" ), Fparts( "parts" ), Fsales( "sales" ), Frefunds( "refunds" ) {}
    virtual ~OrderFull() {}
    void toXML( xmlNodePtr node ) const;
    void setTimelimit( const TXSDateTime& Atimelimit ) {
      Ftimelimit.setObject( Atimelimit );
    }
    void setSales( const OrderItemArray& Asales ) {
      Fsales = Asales;
    }
    void setParts( const PaymentPartArray& Aparts ) {
      Fparts = Aparts;
    }
    void setRefunds( const OrderItemArray& Arefunds ) {
      Frefunds = Arefunds;
    }
};

class PostEntry:public SimpleSoapObject {
  private:
    std::string Fname;
    std::string Fvalue;
  public:
    void Setname( const std::string& Aname ) {
      Fname = Aname;
    }
    void Setvalue( const std::string& Avalue ) {
      Fvalue = Avalue;
    }
    PostEntry():SimpleSoapObject( "PostEntry" ) {}
    virtual ~PostEntry() {}
    void toXML( xmlNodePtr node ) const;
};

class PostEntryArray:public SimpleSoapArrayObject<PostEntry> {
  public:
    PostEntryArray(const std::string& Name):SimpleSoapArrayObject(Name){}
    virtual ~PostEntryArray(){}
};

class Error: public SimpleSoapObject {
  private:
    std::string Fcategory;
    std::string Fcode;
  public:
    bool isOk() {
      return Fcode == "ok";
    }
    void fromXML( xmlNodePtr node );
    Error():SimpleSoapObject( "error" ) {
      Fcategory.clear();
      Fcode.clear();
    }
    std::string toString() {
      std::string s = "category:"+ Fcategory + " code:" + Fcode;
      return s;
    }
    virtual ~Error() {}
};

struct PaySVC {
    std::string rfisc;
    std::string rfic;
    std::string service_type;
    std::string pass_id;
    std::string seg_id;
    std::string name;
    std::string emd_type;
    std::string emd_number;
    std::string emd_coupon;
    std::string emd_status;
    void clear() {
      rfisc.clear();
      rfic.clear();
      service_type.clear();
      pass_id.clear();
      seg_id.clear();
      name.clear();
      emd_type.clear();
      emd_number.clear();
      emd_status.clear ();
    }
    void fromXML( xmlNodePtr node );
    std::string toString() const {
       std::ostringstream buf;
       buf << "rfisc=" << rfisc
           << " rfic=" << rfic
           << " service_type=" << service_type
           << " pass_id=" << pass_id
           << " seg_id=" << seg_id
           << " name=" << name
           << " emd_type=" << emd_type
           << " emd_number=" << emd_number
           << " emd_coupon=" << emd_coupon
           << " emd_status=" << emd_status;
      return buf.str();
    }
};

struct PayItem {
  std::string id;
  std::string ptype; //<type>svc</type>
  std::string owner; // <owner>sirena</owner>
  std::string pay_id; // <rrn>015609428207</rrn>
  std::string order; //number>049GCN</number>
  std::string total; //<total>1490.00</total>
  void fromXML( xmlNodePtr );
};

class NotifyPushEvent: public OrderID, Error {
  private:
    std::string FStatus;
    std::string FOrder;
    std::vector<PayItem> items;
    std::map<std::string, Amount> amounts;
    std::map<std::string, std::vector<PaySVC>> svcs;
    std::map<std::string, std::string> passes;
    std::map<std::string, std::string> segs;
  public:
    enum EnumParse
    {
      onlyStatus,
      Total
    };
    bool isGood() {
      return FStatus == "acknowledged";
    }
    void fromStr( const std::string& notifyXML, EnumParse parseMode = Total );
    void fromXML( xmlNodePtr node, EnumParse parseMode = Total );
    std::string GetError() {
       std::string s = "ERROR " + Error::toString() + " Status=" + FStatus;
       return s;
    }
    void getSVCS( std::vector<PaySVC>& vsvcs ) const;
    std::string getOrder() const{
      return FOrder;
    }

    NotifyPushEvent() {}
};

class RegisterResult: public MPSExchange {
  public:
    virtual std::string exchangeId() const { return std::string("RegisterResult");}
    virtual void clear() {}
    virtual bool isRequest() const {
      return false;
    }
    virtual std::string methodName() const {
      return "registerResponse";
    }
};

class SimpleMethod: public SimpleSoapObject, public MPSExchange {
  protected:
    SimpleSoapObjectOpt<OrderID> Forder;
  public:
    SimpleMethod( ):SimpleSoapObject( methodName() ), Forder( "order" ) {}
    virtual ~SimpleMethod(){}
    virtual std::string methodName() const {
      return "simple";
    }
    virtual bool isRequest() const {
      return true;
    }
    virtual void clear() {}
    std::string getUniqueNumber() const;
    void setOrder( const OrderID& Aorder ) {
      Forder.setObject( Aorder );
    }
    std::string getOrderId() const {
      boost::optional<OrderID> Value = Forder.getObject();
      if ( Value ) {
        return Value.get().getNumber();
      }
      return "";
    }
    void request( xmlNodePtr reqNode );
    void toDB( int grp_id );
    void trace( xmlNodePtr node );
    static int getShopId() {
      return 555;
    }
};


class RegisterMethod: public SimpleMethod {
  private:
    SimpleSoapObjectOpt<Amount> Fcost;
    SimpleSoapObjectOpt<CustomerInfo> Fcustomer;
    SimpleSoapObjectOpt<OrderFull> Fdescription;
    PostEntryArray Fpostdata;
  public:
    RegisterMethod( ):SimpleMethod(), Fcost( "cost" ), Fcustomer( "customer" ), Fdescription( "description" ), Fpostdata( "postdata" ) {}
    virtual ~RegisterMethod(){}
    virtual std::string methodName() const {
      return "register";
    }
    virtual std::string exchangeId() const { return "RegisterMethod";}
    void toXML( xmlNodePtr node ) const;
    //void trace( xmlNodePtr node );
    void setCost( const Amount& Aamount ) {
      Fcost.setObject( Aamount );
    }
    void setCustomer( const CustomerInfo& Acustomer ) {
      Fcustomer.setObject( Acustomer );
    }
    void setDescription( const OrderFull& Adescription ) {
      Fdescription.setObject( Adescription );
    }
    void setPostEntryArray( const PostEntryArray& Apostdata ) {
      Fpostdata = Apostdata;
    }
    std::string getRegnumFromXML( xmlNodePtr node );
};

class CancelMethod:public SimpleMethod {
  public:
    CancelMethod( ):SimpleMethod() {}
    virtual ~CancelMethod(){}
    virtual std::string methodName() const {
      return "cancel";
    }
    virtual std::string exchangeId() const { return "CancelMethod";}
    void toXML( xmlNodePtr node ) const;
};

class CancelResult: public MPSExchange {
  public:
    virtual std::string exchangeId() const { return std::string("CancelResult");}
    virtual void clear() {}
    virtual bool isRequest() const {
      return false;
    }
    virtual std::string methodName() const {
      return "cancelResponse";
    }
};
struct NotifyEvent {
  std::string method;
  std::string order_id;
  std::string xml;
};

typedef std::vector<NotifyEvent> NotifyEvents;

class DBExchanger
{
  public:
    enum EnumStatus
    {
      stSend,
      stProcessed,
      //stError, // относится только к типа request
      stNotify, // относится только к типа request
      stComplete, // относится только к типа notify
      stAny
    };
    std::string EncodeStatus( const EnumStatus& status );
    EnumStatus DecodeStatus( const std::string& status );
    enum EnumRequestType
    {
      rtRequest,
      rtAnswer,
      rtNotify
    };
    std::string EncodeRequestType( const EnumRequestType& request_type );
    EnumRequestType DecodeRequestType( const std::string& request_type );
  private:
    std::string ForderId;
    std::string Fmethod;
    void toDB( int grp_id, const EnumRequestType& msg_type, const std::string& xml, const EnumStatus& status );
    std::string getFirstProcessedMethod();
  public:
    void changeStatus( const EnumRequestType& msg_type, const EnumStatus& status );
    void getNotifyEvents( int grp_id, NotifyEvents& events );
    int getRequestGrpId();
    void request( int grp_id, const std::string& xml );
    void answer( int grp_id, const std::string& xml );
    void notifyError( const std::string& xml );
    void notifyOk( const std::string& xml, const EnumStatus& status );
    bool check_msg( EnumStatus& currStatus, std::string& xml, const EnumRequestType& msg_type, const EnumStatus& status );
    bool check_msg( std::string& xml, const EnumRequestType& msg_type, const EnumStatus& status );
    void stopWaitNotify();
    bool alreadyRequest();
    DBExchanger( ){}
    DBExchanger( const std::string& method, const std::string& orderId ):ForderId(orderId),Fmethod(method) {}
    DBExchanger( const std::string& orderId ):ForderId(orderId) {
      Fmethod = getFirstProcessedMethod();
    }
};

struct TermPos {
  int id;
  std::string name;
  std::string address;
  std::string serial;
  std::string vendor;
  bool inUse;
  void toXML( xmlNodePtr node ) const;
};

class PosAssignator
{
private:
  static const int TIME_OUT = 10; //minutes
private:
  std::vector<TermPos> termPoses;
  int FPosId;
  std::string FAirline;
  std::string FAirp;
public:
  void clear() {
    FPosId = ASTRA::NoExists;
    FAirline.clear();
    FAirp.clear();
    termPoses.clear();
  }
  PosAssignator() {
    clear();
  }
  void fromDB( int point_id );
  void toXML(  xmlNodePtr node );
  void AssignPos( int pos_id );
  void ReleasePos( );
  static void checkTimeout();
  bool inUse( );
  bool isEmpty() {
    return termPoses.empty();
  }
  int getPosId() {
    return FPosId;
  }
};


class PosClient
{
private:
  int sirena_id;
  int mps_shopid;
  std::string vendor;
  std::string serial;
public:
  void clear() {
    sirena_id = ASTRA::NoExists;
    mps_shopid = ASTRA::NoExists;
    vendor.clear();
    serial.clear();
  }
  void fromDB( int posId );
  int getShopId() {
    return mps_shopid;
  }
  int getSirenaId() {
    return sirena_id;
  }
  std::string getVendor() {
    return vendor;
  }
  std::string getSerial() {
    return serial;
  }
};

class MPSExchangeIface: public ExchangeIterface::ExchangeIface
{
private:
protected:
    virtual void BeforeRequest(xmlNodePtr& reqNode, xmlNodePtr externalSysResNode, const SirenaExchange::TExchange& req);
    virtual void BeforeResponseHandle(int reqCtxtId, xmlNodePtr& reqNode, xmlNodePtr& ResNode, std::string& exchangeId);
public:
  static const std::string getServiceName() {
    return "service_eval_mps";
  }
  static void Request(xmlNodePtr reqNode, int clientId, const std::string& ifaceName, MPSExchange& req);
  MPSExchangeIface() : ExchangeIterface::ExchangeIface(getServiceName()) {
    domainName = "ASTRA-MPS";
    Handler *evHandle;
    evHandle=JxtHandler<MPSExchangeIface>::CreateHandler(&MPSExchangeIface::NotifyPushEvents);
    AddEvent("notify",evHandle);
    addResponseHandler("RegisterMethod", response_RegisterResult);
    addResponseHandler("CancelMethod", response_CancelResult);
    AddEvent("mps_register", JXT_HANDLER(MPSExchangeIface, KickHandler));
    //RegisterNotifyParseTest();
  }
  //void RegisterNotifyParseTest();
  void CheckPaid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void StopPaid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void NotifyPushEvents(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static void response_RegisterResult(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  static void response_CancelResult(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  virtual ~MPSExchangeIface(){}
};

int TIMELIMIT_SEC();

}
#endif // MPSEXCHANGEIFACE_H
