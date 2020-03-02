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
    boost::optional<T> getObject() {
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
    std::string getNumber() {
      return Fnumber;
    }
    OrderID():SimpleSoapObject( "order" ) {
      Fshop_id = 0;
      Fnumber.clear();
    }
    virtual ~OrderID() {}
    void toXML( xmlNodePtr node ) const;
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
    CustomerInfo():SimpleSoapObject( "áustomer" ) {}
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

class RegisterResult: public MPSExchange {
  public:
    virtual std::string exchangeId() const { return std::string("");}
    virtual void clear() {}
    virtual bool isRequest() const {
      return false;
    }
};

class RegisterMethod:public SimpleSoapObject, public MPSExchange {
  private:
    SimpleSoapObjectOpt<OrderID> Forder;
    SimpleSoapObjectOpt<Amount> Fcost;
    SimpleSoapObjectOpt<CustomerInfo> Fcustomer;
    SimpleSoapObjectOpt<OrderFull> Fdescription;
    PostEntryArray Fpostdata;
  public:
    RegisterMethod( ):SimpleSoapObject( "register" ), Forder( "order" ), Fcost( "cost" ), Fcustomer( "customer" ), Fdescription( "description" ), Fpostdata( "postdata" ) {}
    virtual ~RegisterMethod(){}
    virtual std::string exchangeId() const { return SimpleSoapObject::getObjectName();}
    virtual bool isRequest() const {
      return true;
    }
    virtual void clear() {}
    void toXML( xmlNodePtr node ) const;
    void trace( xmlNodePtr node );
    void setOrder( const OrderID& Aorder ) {
      Forder.setObject( Aorder );
    }
    std::string getOrderId() {
      boost::optional<OrderID> Value = Forder.getObject();
      if ( Value ) {
        return Value.get().getNumber();
      }
      return "";
    }
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
    std::string getUniqueNumber() const;

    void request( xmlNodePtr reqNode );
    void toDB( int grp_id, const std::string& request_type );
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
    evHandle=JxtHandler<MPSExchangeIface>::CreateHandler(&MPSExchangeIface::CheckPaid);
    AddEvent("check_paid",evHandle);
    evHandle=JxtHandler<MPSExchangeIface>::CreateHandler(&MPSExchangeIface::StopPaid);
    AddEvent("stop_paid",evHandle);
    addResponseHandler("register", response_RegisterResult);
    AddEvent("mps_register", JXT_HANDLER(MPSExchangeIface, KickHandler));
  }
  void CheckPaid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void StopPaid(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  static void response_RegisterResult(const std::string& exchangeId, xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode);
  //static bool isResponseHandler( const std::string& name, xmlNodePtr node );
  virtual ~MPSExchangeIface(){}
};


class PushEvents: public MPSExchange {
  virtual std::string exchangeId() const { return std::string("");}
  virtual void clear() {}
  virtual bool isRequest() const {
    return false;
  }
  virtual void parseResponse(xmlNodePtr node);
};

void parseMPS_PUSH_Events( xmlNodePtr node );
}

#endif // MPSEXCHANGEIFACE_H
