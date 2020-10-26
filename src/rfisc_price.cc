#include "rfisc_price.h"
#define NICKNAME "DJEK"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>
#include "xml_unit.h"
#include "oralib.h"
#include "date_time.h"
#include "serverlib/xml_stuff.h" // для xml_decode_nodelist
#include "passenger.h"

void SvcFromSirena::fromContextXML(xmlNodePtr node) {
  clear();
  svcKey.svcId = NodeAsString( "svc_id", node );
  svcKey.orderId = NodeAsString( "orderId", node );
  doc = PriceDoc( NodeAsString( "doc_id", node, "" ),NodeAsString( "doc_type", node, "" ),NodeAsBoolean( "unpoundable", node, false ) );
  LogTrace(TRACE5) << svcKey.svcId  << " " << doc.doc_id;
  pass_id = NodeAsString( "pass_id", node );
  seg_id = NodeAsString( "seg_id", node );
  name = NodeAsString( "name", node );
  price = NodeAsFloat( "price", node );
  currency = NodeAsString( "currency", node );
  ticket_cpn = NodeAsString( "ticket_cpn", node, "" );
  ticknum = NodeAsString( "ticknum", node, "" );
  status_svc = NodeAsString( "status_svc", node );
  status_direct = NodeAsString( "status_direct", node );
}

void SvcFromSirena::fromXML(xmlNodePtr node) {
  clear();
  svcKey.svcId = NodeAsString( "svc_id", node );
  price = NodeAsFloat( "price", node );
  currency = NodeAsString( "currency", node );
  ticket_cpn = NodeAsString( "ticket_cpn", node, "" );
  ticknum = NodeAsString( "ticknum", node, "" );
}

void SvcFromSirena::toContextXML(xmlNodePtr node) const {
  NewTextChild( node, "svc_id", svcKey.svcId );
  NewTextChild( node, "orderId", svcKey.orderId );
  NewTextChild( node, "doc_id", doc.doc_id );
  NewTextChild( node, "doc_type", doc.doc_type );
  NewTextChild( node, "unpoundable", doc.unpoundable );
  NewTextChild( node, "pass_id", pass_id );
  NewTextChild( node, "seg_id", seg_id );
  NewTextChild( node, "name", name );
  NewTextChild( node, "price", price );
  NewTextChild( node, "currency", currency );
  NewTextChild( node, "ticket_cpn", ticket_cpn );
  NewTextChild( node, "ticknum", ticknum );
  NewTextChild( node, "status_svc", status_svc );
  NewTextChild( node, "status_direct", status_direct );
}

void SvcFromSirena::toXML(xmlNodePtr node) const {
  NewTextChild( node, "svc_id", svcKey.svcId );
  NewTextChild( node, "price", price );
  NewTextChild( node, "currency", currency );
  NewTextChild( node, "ticket_cpn", ticket_cpn );
  NewTextChild( node, "ticknum", ticknum );
  NewTextChild(node,"valid", clientValid());
}

void SvcFromSirena::toDB( TQuery& Qry) const
{
  Qry.SetVariable("svc_id",svcKey.svcId);
  Qry.SetVariable("order_id",svcKey.orderId);
  Qry.SetVariable("doc_id",doc.doc_id);
  Qry.SetVariable("pass_id",pass_id);
  Qry.SetVariable("seg_id",seg_id);
  Qry.SetVariable("price",price);
  Qry.SetVariable("currency",currency);
  Qry.SetVariable("ticket_cpn",ticket_cpn);
  Qry.SetVariable("ticknum",ticknum);
}

void SvcFromSirena::fromDB(TQuery& Qry,const std::string& lang)
{
  svcKey.svcId = Qry.FieldAsString("svc_id");
  svcKey.orderId = Qry.FieldAsString("order_id");
  doc = PriceDoc(Qry.FieldAsString("doc_id"),"",false);
  pass_id = Qry.FieldAsString("pass_id");
  seg_id = Qry.FieldAsString("seg_id");
  price = Qry.FieldAsFloat("price");
  name = (lang==AstraLocale::LANG_EN?Qry.FieldAsString("name_view_lat"):Qry.FieldAsString("name_view"));
  currency = Qry.FieldAsString("currency");
  ticket_cpn = Qry.FieldAsString("ticket_cpn");
  ticknum = Qry.FieldAsString("ticknum");
  LogTrace(TRACE5) << toString();
}

bool SvcFromSirena::only_for_cost() const
{
  return price == -1.0;
}

bool SvcFromSirena::clientValid() const
{
  return price!=ASTRA::NoExists &&
         price>=0.0 &&
         !currency.empty() &&
         status_direct != TPriceRFISCList::STATUS_DIRECT_PAID; //!!!
}

std::string SvcFromSirena::toString() const {
  std::ostringstream s;
  s << "svc_id=" << svcKey.svcId << ",orderId=" << svcKey.orderId << ",price=" << price << ",cuurency=" << currency
    << ",doc_id=" << doc.doc_id << ",doc_type=" << doc.doc_type << ",unpoundable=" << doc.unpoundable
    << ",ticknum="<<ticknum<<"/"<<ticket_cpn;
  return s.str();
}

const TPriceServiceItem& TPriceServiceItem::toContextXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TPaxSegRFISCKey::toXML(node);
  NewTextChild(node,"pax_name",pax_name);
  if ( list_id != ASTRA::NoExists ) {
    NewTextChild(node,"list_id",list_id);
  }
  xmlNodePtr n = NewTextChild(node,"svcs");
  for ( const auto &svc : svcs ) {
    svc.second.toContextXML( NewTextChild(n,"item"));
  }
  return *this;
}

TPriceServiceItem& TPriceServiceItem::fromContextXML(xmlNodePtr node)
{
  if (node==NULL) return *this;
  TPaxSegRFISCKey::fromXML(node);
  xmlNodePtr node2 = node->children;
  pax_name = NodeAsStringFast("pax_name",node2,"");
  list_id = NodeAsIntegerFast("list_id",node2,ASTRA::NoExists);
  xmlNodePtr n =  GetNodeFast( "svcs", node2 );
  if ( n != nullptr ) {
    n = n->children;
    while ( n != nullptr && std::string("item") == (const char*)n->name ) {
      SvcFromSirena svc;
      svc.fromContextXML(n);
      svcs.emplace(svc.svcKey,svc);
      n = n->next;
    }
  }
  if ( list_id != ASTRA::NoExists ) {
    getListItem();
  }
  return *this;
}

TPriceServiceItem& TPriceServiceItem::fromXML(xmlNodePtr node, const std::string& orderId)
{
  if (node==NULL) return *this;
  TPaxSegRFISCKey::fromXML(node);
  xmlNodePtr node2 = node->children;
  pax_name = NodeAsStringFast("pax_name",node2,"");
  SvcFromSirena svc;
  svc.fromXML(node);
  svc.svcKey.orderId = orderId;
  svcs.emplace(svc.svcKey,svc);
  return *this;
}

const TPriceServiceItem& TPriceServiceItem::toXML(xmlNodePtr node, const SVCKey& svcKey) const
{
  if (node==NULL) return *this;
  TPaxSegRFISCKey::toXML(node);
  if ( std::string( NodeAsString( "name_view", node) ).empty() ) {
    ReplaceTextChild(node, "name_view", name_view() ); //!!!
  }
  NewTextChild(node,"pax_name",pax_name);
  if ( svcs.find( svcKey ) == svcs.end() ) {
    throw EXCEPTIONS::Exception( "svc not found svc_id %s", svcKey.svcId.c_str() );
  }
  SvcFromSirena svc = svcs.at(svcKey);
  svc.toXML(node);
  return *this;
}

std::string TPriceServiceItem::name_view(const std::string& lang) const
{
  std::string name = list_item?lowerc(list_item.get().name_view(lang)):"";
  if ( name.empty() && !svcs.empty() ) {
    name = svcs.begin()->second.name;
  }
  return name;
}

void TPriceServiceItem::changeStatus( const std::string& from, const std::string& to ) {
  for ( auto& osvc : svcs ) {
    if ( osvc.second.status_direct == from ) {
      LogTrace(TRACE5)<<"svcKey(" << osvc.first.svcId << ","<< osvc.first.orderId << ") change direct status from " << osvc.second.status_direct << " to " << to;
      osvc.second.status_direct = to;
    }
  }
}

const TPriceServiceItem& TPriceServiceItem::toDB(TQuery &Qry, const SVCKey& svcKey) const
{
  Qry.SetVariable("name_view", name_view(AstraLocale::LANG_RU));
  Qry.SetVariable("name_view_lat", name_view(AstraLocale::LANG_EN));
  //Qry.SetVariable("list_id",(list_id==ASTRA::NoExists?FNull:list_id));
  if ( svcs.find( svcKey ) == svcs.end() ) {
    throw EXCEPTIONS::Exception( "svc not found svc_id %d", svcKey.svcId );
  }
  SvcFromSirena svc = svcs.at(svcKey);
  svc.toDB(Qry);
  return *this;
}

TPriceServiceItem& TPriceServiceItem::fromDB(TQuery &Qry, const std::string& lang)
{
  TPaxSegRFISCKey::fromDB(Qry);
  SvcFromSirena svc;
  svc.fromDB(Qry,lang);
  //int svc_id = 100000;
  //std::pair<std::map<std::string,SvcFromSirena>::iterator,bool> ret;
  svcs.insert(std::make_pair(svc.svcKey,svc)); //!!!здесь потеря
  LogTrace(TRACE5) << svcs.size();
/*  ret = svcs.insert(make_pair(svc.svcKey,svc)); //!!!здесь потеря
  if ( ret.second == false ) { //один и тот же svc_id, но в разных PNR
     svcs.insert(make_pair(IntToString(svc_id),svc)); //псевдо svc_id для печати Дена
     svc_id++;
  }*/
  PaxsNames p;
  pax_name = p.getPaxName(pax_id);
  return *this;
}

std::string TPriceServiceItem::traceStr() const
{
  std::ostringstream s;
  s << TPaxSegRFISCKey::traceStr() << ", svcs:";
  for ( const auto &id : svcs ) {
    s << id.second.toString() << " ";
  }
  return s.str();
}

void SvcEmdPayDoc::toXML(xmlNodePtr node) const {
  if ( !empty() ) {
    node = NewTextChild( node, "paydoc" );
    NewTextChild( node, "formpay", formpay );
    if ( !type.empty() ) {
      NewTextChild( node, "type", type );
    }
  }
}

void SvcEmdPayDoc::fromXML(xmlNodePtr reqNode) {
  clear();
  formpay = NodeAsString( "paydoc/formpay", reqNode, "" );
  if ( GetNode( "paydoc/type", reqNode ) != nullptr ) {
    type = NodeAsString("paydoc/type",reqNode);
  }
}

void SvcEmdTimeout::fromXML(xmlNodePtr node) {
  clear();
  timeout = NodeAsInteger( "timeout",node,ASTRA::NoExists);
  if ( timeout != ASTRA::NoExists){
    utc_deadline = NodeAsDateTime( "timeout/@utc_deadline","hh:nn dd.mm.yyyy",node);
  }
}

void SvcEmdTimeout::toXML(xmlNodePtr node) const {
  if ( timeout != ASTRA::NoExists ) {
    SetProp( NewTextChild(node,"timeout",timeout), "utc_deadline", BASIC::date_time::DateTimeToStr(utc_deadline,"hh:nn dd.mm.yyyy") );
  }
}

void SvcEmdRegnum::fromXML(xmlNodePtr node,SvcEmdRegnum::Enum style) {
  clear();
  std::string sversion;
  if ( style == SvcEmdRegnum::Enum::propStyle ) {
    sversion = NodeAsString( "@version", NodeAsNode( "regnum", node), "ignore" );
  }
  else {
    sversion =  NodeAsString( "version",node,"ignore" );
  }
  if ( sversion == "ignore" ) {
    version = ASTRA::NoExists;
  }
  else
    if ( StrToInt( sversion.c_str(), version ) == EOF ) {
      throw EXCEPTIONS::Exception( "invalid version value " + sversion );
    }
  regnum = NodeAsString( "regnum",node );
}


void SvcEmdRegnum::toXML(xmlNodePtr node,SvcEmdRegnum::Enum style) const {
  xmlNodePtr n = NewTextChild(node,"regnum",regnum);
  if ( style == SvcEmdRegnum::Enum::propStyle ) {
    SetProp( n, "version", version==ASTRA::NoExists?"ignore":IntToString(version));
  }
  if ( style == SvcEmdRegnum::Enum::nodeStyle) {
    NewTextChild(node,"regnum",version==ASTRA::NoExists?"ignore":IntToString(version));
  }
}

void SvcEmdCost::fromXML(xmlNodePtr node) {
  clear();
  cost = NodeAsString( "cost",node,"");
  if ( !cost.empty() ) {
    currency = ElemToElemId( etCurrency, NodeAsString( "cost/@curr",node), fmt );
  }
}

void SvcEmdCost::toXML(xmlNodePtr node) const {
  SetProp(NewTextChild(node,"cost",cost),"curr",currency); //ElemIdToElem
}

void SvcEmdSvcsReq::toXML(xmlNodePtr node) const
{
  for ( const auto& svc : svcs ) {
    SetProp( NewTextChild( node, "svc" ), "id", svc.svcId );
  }
}

void SvcValue::fromXML( xmlNodePtr node ) {
  id = NodeAsString( "@id", node );
  name = NodeAsString( "@name", node );
  pass_id = NodeAsString( "@pass_id", node );
  rfic = NodeAsString( "@rfic", node );
  rfisc = NodeAsString( "@rfisc", node );
  seg_id = NodeAsString( "@seg_id", node );
  service_type = NodeAsString( "@service_type", node );
  status = NodeAsString( "@status", node );
}

void SvcEmdSvcsAns::fromXML(xmlNodePtr node) {
  clear();
  node = GetNode( "svcs", node );
  if ( node != nullptr ) {
    node = node->children;
    while ( node != nullptr &&
            std::string("svc") == (const char*)node->name ) {
      SvcValue s;
      s.fromXML(node);
      emplace_back(s);
      node = node->next;
    }
  }
}

void SvcEmdSvcsAns::getSvcValue( const std::string &id, SvcValue &vsvc  ) {
  for ( const auto &p : *this ) {
    if ( p.id == id ) {
      vsvc = p;
      return;
    }
  }
  throw EXCEPTIONS::Exception( "svc not found, id=" + id );
}


const std::string TPriceRFISCList::STATUS_DIRECT_ORDER = "order";
const std::string TPriceRFISCList::STATUS_DIRECT_SELECTED = "selected";
const std::string TPriceRFISCList::STATUS_DIRECT_ISSUE_QUERY = "issue_query";
const std::string TPriceRFISCList::STATUS_DIRECT_ISSUE_ANSWER = "issue_answer";
const std::string TPriceRFISCList::STATUS_DIRECT_ISSUE_CONFIRM = "issue_confirm";
const std::string TPriceRFISCList::STATUS_DIRECT_REFUND = "refund";
const std::string TPriceRFISCList::STATUS_DIRECT_PAID = "paid";

TPriceRFISCList::TPriceRFISCList()
{
  time_create = ASTRA::NoExists;
}

void TPriceRFISCList::Lock(int grp_id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT * FROM svc_prices WHERE grp_id=:grp_id FOR UPDATE";
  Qry.CreateVariable( "grp_id",otInteger,grp_id);
  Qry.Execute();
}

void TPriceRFISCList::fromContextDB(int grp_id)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT * FROM svc_prices WHERE grp_id=:grp_id ORDER BY page_no";
  Qry.CreateVariable( "grp_id",otInteger,grp_id);
  Qry.Execute();
  std::string value;
  for ( ;!Qry.Eof; Qry.Next() ) {
     value.append(Qry.FieldAsString("xml"));
     time_create=Qry.FieldAsDateTime("time_create");
  }
  if ( value.empty() ) {
    return;
  }
  value = ConvertCodepage(value,"CP866", "UTF-8");
  LogTrace(TRACE5) << __func__ << std::endl << value;
  XMLDoc doc(value);
  xml_decode_nodelist(doc.docPtr()->children);
  fromContextXML(doc.docPtr()->children);
}

void TPriceRFISCList::toContextDB(int grp_id,bool pr_only_del) const
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
  "DELETE svc_prices WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if ( pr_only_del ) {
    return;
  }
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO svc_prices(grp_id,xml,page_no,time_create) "
    " VALUES(:grp_id,:xml,:page_no,:time_create)";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.DeclareVariable("page_no",otInteger);
  Qry.DeclareVariable("xml",otString);
  Qry.CreateVariable("time_create",otDate,BASIC::date_time::NowUTC());
  xmlNodePtr rootNode;
  XMLDoc doc("TPriceRFISCList",rootNode,__func__);
  toContextXML( rootNode );
  std::string value = XMLTreeToText( doc.docPtr() );
  LogTrace(TRACE5) << __func__ << std::endl << value;
  longToDB(Qry, "xml", value, true);
}

void TPriceRFISCList::toDB(int grp_id) const
{
  //добавляем только новые записи
  TPriceRFISCList r;
  r.fromDB(grp_id);
  std::vector<SVCKey> bd_svcs;
  r.haveStatusDirect( "", bd_svcs ); //просто выбрали все те, которые записаны в БД, у них статусы пустые, т.к. не храняться в БД
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "INSERT INTO pay_services(grp_id,pax_id,transfer_num,rfisc,service_type,airline,name_view,name_view_lat,list_id,svc_id,order_id,doc_id,pass_id,seg_id,price,currency,ticknum,ticket_cpn,time_paid)"
    "  VALUES(:grp_id,:pax_id,:transfer_num,:rfisc,:service_type,:airline,:name_view,:name_view_lat,:list_id,:svc_id,:order_id,:doc_id,:pass_id,:seg_id,:price,:currency,:ticknum,:ticket_cpn,:time_paid)";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.DeclareVariable("pax_id",otInteger);
  Qry.DeclareVariable("transfer_num",otInteger);
  Qry.DeclareVariable("rfisc",otString);
  Qry.DeclareVariable("service_type",otString);
  Qry.DeclareVariable("airline",otString);
  Qry.DeclareVariable("name_view",otString);
  Qry.DeclareVariable("name_view_lat",otString);
  Qry.DeclareVariable("list_id",otInteger);
  Qry.DeclareVariable("svc_id",otInteger);
  Qry.DeclareVariable("order_id",otString);
  Qry.DeclareVariable("doc_id",otString);
  Qry.DeclareVariable("pass_id",otInteger);
  Qry.DeclareVariable("seg_id",otInteger);
  Qry.DeclareVariable("price",otFloat);
  Qry.DeclareVariable("currency",otString);
  Qry.DeclareVariable("ticknum",otString);
  Qry.DeclareVariable("ticket_cpn",otString);
  Qry.CreateVariable("time_paid",otDate, BASIC::date_time::NowUTC());
  for ( const auto &p : *this ) {
    SVCS svcs;
    p.second.getSVCS( svcs, STATUS_DIRECT_PAID );
    for ( const auto svc : svcs ) {
      if ( std::find( bd_svcs.begin(), bd_svcs.end(), svc.first ) != bd_svcs.end() ||
           svc.second.ticknum.empty()  ) { //уже есть в БД или еще не выписан
        continue;
      }
      p.first.toDB(Qry);
      p.second.toDB(Qry,svc.first);
      Qry.Execute();
    }
  }
}

void TPriceRFISCList::fromDB(int grp_id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
   "SELECT * FROM pay_services "
   " WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  for (; !Qry.Eof; Qry.Next() ) {
    TPaxSegRFISCKey key;
    key.fromDB(Qry);
    (*this)[key].fromDB(Qry);
    LogTrace(TRACE5) << (*this)[key].traceStr();
  }
}

void TPriceRFISCList::fromContextXML(xmlNodePtr node)
{
  clear();
  if ( node == nullptr ) {
    return;
  }
  SvcEmdRegnum::fromXML(node,SvcEmdRegnum::propStyle);
  surname = NodeAsString("surname",node);
  mps_order_id = NodeAsString("mps_order_id",node,"");
  FPosId = NodeAsInteger( "pos_id", node, ASTRA::NoExists );
  error_code = NodeAsString("error_code",node,"");
  error_message = NodeAsString("error_message",node,"");
  SvcEmdPayDoc::fromXML(node);
  SvcEmdCost::fromXML(node);
  SvcEmdTimeout::fromXML(node);
  node = GetNode("items",node);
  if ( node == nullptr || node->children == nullptr ) {
    return;
  }
  node = node->children;
  while ( node != nullptr &&
          std::string("item") == (const char*)node->name ) {
    TPriceServiceItem item;
    item.fromContextXML(node);
    insert( std::make_pair(item,item) );
    node = node->next;
  }
}

void TPriceRFISCList::toContextXML(xmlNodePtr node,bool checkFormPay) const
{
  xmlNodePtr node2 = NewTextChild(node,"items");
  for ( const auto &p : *this ) {
    xmlNodePtr n = NewTextChild( node2, "item" );
    p.second.toContextXML( n );
  }
  SvcEmdRegnum::toXML(node,SvcEmdRegnum::propStyle);
  NewTextChild(node, "surname", surname);
  if ( !mps_order_id.empty() ) {
    NewTextChild(node, "mps_order_id", mps_order_id);
  }
  if ( FPosId != ASTRA::NoExists ) {
    NewTextChild( node, "pos_id", FPosId );
  }
  if ( !error_code.empty() ) {
    NewTextChild(node, "error_code", error_code);
  }
  if ( !error_message.empty() ) {
    NewTextChild(node, "error_message", error_message);
  }
  SvcEmdPayDoc::toXML(node);
  SvcEmdCost::toXML(node);
  SvcEmdTimeout::toXML(node);
}

void TPriceRFISCList::fromXML(xmlNodePtr node)
{
  clear();
  if ( node == nullptr ) {
    return;
  }
  SvcEmdRegnum::fromXML(node,SvcEmdRegnum::Enum::noneStyle);
  node = GetNode("items",node);
  if ( node == nullptr || node->children == nullptr ) {
    return;
  }
  tst();
  node = node->children;
  tst();
  while ( node != nullptr &&
          std::string("item") == (const char*)node->name ) {
    TPriceServiceItem item;
    item.fromXML( node, SvcEmdRegnum::getRegnum() );
    auto p = find(item);
    if ( p != end() ) {
      SVCS svcs;
      item.getSVCS(svcs,TPriceServiceItem::EnumSVCS::all);
      p->second.addSVCS( svcs.begin()->first, svcs.begin()->second );
    }
    else {
      insert( std::make_pair(item,item) );
    }
    node = node->next;
  }
}

void TPriceRFISCList::toXML(xmlNodePtr node) const
{
  float total = 0.0;
  std::string currency;
  xmlNodePtr node2 = NewTextChild(node,"items");
  for ( const auto &p : *this ) {
    SVCS svcs;
    p.second.getSVCS(svcs,TPriceServiceItem::EnumSVCS::only_for_pay);
    for ( const auto &svc : svcs ) {
      if ( svc.second.status_svc == "HI" ) {
        LogTrace(TRACE5) << svc.second.toString();
        continue;
      }
      xmlNodePtr n = NewTextChild( node2, "item" );
      p.second.toXML( n, svc.first );
      if ( svc.second.clientValid() ) {
        total += svc.second.price;
        if ( currency.empty() ) {
          currency = ElemIdToElemCtxt( ecCkin, etCurrency, svc.second.currency, efmtCodeNative );
        }
      }
    }
  }
  SvcEmdRegnum::toXML(node,SvcEmdRegnum::Enum::noneStyle);
  NewTextChild(node, "total", total);
  NewTextChild(node, "currency", currency);
}

void TPriceRFISCList::setServices( const TPaidRFISCList& list )
{
  std::map<TPaxSegRFISCKey, TPriceServiceItem>::clear();
  PaxsNames paxNames;
  for ( auto item : list ) {
    if (!item.second.service_quantity_valid()) {
      continue;
    }
    TPaidRFISCItem tmpItem=item.second;
    TPaxSegRFISCKey p=item.first;
    if (tmpItem.service_quantity>0 && // service_quantity - общее кол-во услуг
        tmpItem.need != ASTRA::NoExists &&
        tmpItem.need>0) // need - кол-во платить, paid -- paid - платные услуги
      {
        TPriceServiceItem priceItem(tmpItem,paxNames.getPaxName(tmpItem.pax_id),SVCS());
        LogTrace(TRACE5) << priceItem.list_id;
        insert( std::make_pair(p,priceItem) );
      }
    }
  if ( std::map<TPaxSegRFISCKey, TPriceServiceItem>::empty() ) {
    throw AstraLocale::UserException("MSG.EMD.SERVICES_ALREADY_PAID");
  }
}

void TPriceRFISCList::synchFromSirena(const SvcEmdSvcsAns& svcs)
{
  SVCKey svcKey("",SvcEmdRegnum::getRegnum());
  for ( const auto& nsvc : svcs ) {
    for ( auto& oitem : *this ) {
      SVCS::iterator f;
      svcKey.svcId = nsvc.id;
      if ( oitem.second.findSVC( svcKey, f ) ) {
        f->second.status_svc = nsvc.status;
        f->second.ticket_cpn = nsvc.ticket_cpn.empty()?f->second.ticket_cpn:nsvc.ticket_cpn;
        f->second.ticknum = nsvc.ticknum.empty()?f->second.ticknum:nsvc.ticknum;
        break;
      }
    }
  }
}

bool TPriceRFISCList::synchFromSirena(const TPriceRFISCList& list, bool only_delete)
{
  bool res = true;

  std::set<SVCKey> svc_ids, doc_ids;
  std::set<TServiceType::Enum> sevice_types;

  for ( const auto& nitem : list ) { //filter
    sevice_types.insert( nitem.first.service_type );
  //  LogTrace(TRACE5) << nitem.first.service_type;
    SVCS svcs;
    nitem.second.getSVCS(svcs,TPriceServiceItem::EnumSVCS::all);
    for ( const auto nsvc : svcs ) {
    //  LogTrace(TRACE5) << nsvc.second.toString();
      svc_ids.insert( nsvc.first );
    }
  }

  for ( const auto& s : svc_ids ) {
    for ( auto& oitem : *this ) {
      SVCS::iterator f;
      if ( oitem.second.findSVC(s,f) &&
           f->second.doc.unpoundable ) {
        doc_ids.insert(SVCKey(f->second.doc.doc_id,getRegnum()));
        LogTrace(TRACE5) << f->second.doc.doc_id;
      }
    }
  }
  //insert or update
  if ( !only_delete ) {
    for ( const auto& nitem : list ) {
      TPriceRFISCList::iterator oitem;
      if ( (oitem = find( nitem.first )) == end() ) { //new
        LogTrace(TRACE5) << "clear list_id AND view_name!!";
        insert( std::make_pair( nitem.first, nitem.second ) );
      }
      else { //update
        SVCS svcs;
        nitem.second.getSVCS(svcs,TPriceServiceItem::EnumSVCS::all);
        for ( const auto& nsvc : svcs ) {
          SVCS::iterator f;
          if ( !oitem->second.findSVC( nsvc.first, f ) ) {
            oitem->second.addSVCS( nsvc.first,nsvc.second );
          }
          else {
            LogTrace(TRACE5) << nsvc.second.toString();
            std::string status_direct = f->second.status_direct;
            std::string ticket_cpn = nsvc.second.ticket_cpn.empty()?f->second.ticket_cpn:nsvc.second.ticket_cpn;
            std::string ticknum = nsvc.second.ticknum.empty()?f->second.ticknum:nsvc.second.ticknum;
            f->second = nsvc.second;
            f->second.ticket_cpn = ticket_cpn;
            f->second.ticknum = ticknum;
            f->second.status_direct = status_direct;
          }
        }
      }
    }
  }
  //delete
  for ( auto oitem=begin(); oitem!=end(); ) {
    TPriceRFISCList::const_iterator nitem;
    SVCS osvcs, nsvcs;
    oitem->second.getSVCS(osvcs,TPriceServiceItem::EnumSVCS::all);
    if ( (nitem = list.find(oitem->first)) != list.end()) {
      nitem->second.getSVCS(nsvcs,TPriceServiceItem::EnumSVCS::all);
    }
    for ( const auto &osvc : osvcs ) {
      if ( nsvcs.find(osvc.first)==nsvcs.end() ) {
        //LogTrace(TRACE5) << oitem->second.traceStr();
        if ( doc_ids.find( SVCKey(osvc.second.doc.doc_id,getRegnum()) ) == doc_ids.end() &&
             sevice_types.find( oitem->first.service_type ) == sevice_types.end() ) {
          //LogTrace(TRACE5) << osvc.second.doc.doc_id;
          oitem->second.eraseSVC(osvc.first);
          res = false;
        }
//        LogTrace(TRACE5) << oitem->second.traceStr();
      }
    }
    oitem->second.getSVCS(osvcs,TPriceServiceItem::EnumSVCS::all);
    if ( osvcs.empty() ) {
      this->erase(oitem++);
      res = false;
    }
    else
      ++oitem;
  }
  tst();
  return res;
}

void TPriceRFISCList::setStatusDirect( const std::string &from, const std::string &to )
{
  for ( auto& oitem : *this ) {
    oitem.second.changeStatus( from, to );
  }
}

bool TPriceRFISCList::haveStatusDirect( const std::string& statusDirect )
{
  std::vector<SVCKey> svcs;
  return haveStatusDirect( statusDirect, svcs );
}

bool TPriceRFISCList::haveStatusDirect( const std::string& statusDirect, std::vector<SVCKey> &svcs )
{
  svcs.clear();
  for ( const auto& oitem : *this ) {
    SVCS nsvcs;
    oitem.second.getSVCS( nsvcs, statusDirect );
    for ( const auto& osvc : nsvcs ) {
      if ( osvc.second.clientValid() ) {
        svcs.push_back( osvc.first );
      }
    }
  }
  return !svcs.empty();
}

bool TPriceRFISCList::getNextIssueQueryGrpEMD( std::vector<SVCKey> &svcs)
{
  bool res = false;
  svcs.clear();
  TServiceType::Enum service_type = TServiceType::Enum::Unknown;
  for ( auto& oitem : *this ) {
    SVCS osvcs;
    oitem.second.getSVCS( osvcs, TPriceServiceItem::EnumSVCS::all);
    for ( auto& osvc : osvcs ) {
      res |= (osvc.second.status_direct == TPriceRFISCList::STATUS_DIRECT_SELECTED);
      if ( osvc.second.status_direct != TPriceRFISCList::STATUS_DIRECT_SELECTED &&
           !osvc.second.only_for_cost() ) {
        continue;
      }
      if ( service_type == TServiceType::Enum::Unknown ) {
        service_type = oitem.second.service_type;
      }
      //!!!if ( service_type == oitem.second.service_type ) {
        svcs.push_back( osvc.first );
        SVCS::iterator f;
        if ( !oitem.second.findSVC( osvc.first, f ) ) {
          throw EXCEPTIONS::Exception( "svc not found %s", osvc.first.svcId.c_str() );
        }
        f->second.status_direct = TPriceRFISCList::STATUS_DIRECT_ISSUE_QUERY;
        LogTrace(TRACE5) << __func__ << " " << osvc.first.svcId;
       //!!! break; //only one at step
      //!!!}
    }
/*!!!    if ( !svcs.empty() ) { //по одному
      break;
    }*/
  }
  if ( !res ) {
    svcs.clear();
  }
  return !svcs.empty();
}

bool TPriceRFISCList::terminalChoiceAny()
{
  for ( auto& nitem : *this ) { //filtered
    SVCS svcs;
    nitem.second.getSVCS( svcs, TPriceServiceItem::EnumSVCS::only_for_pay );
    for ( const auto& nsvc : svcs ) {
      if ( nsvc.second.clientValid() ) {
        return true;
      }
    }
  }
  return false;
}

float TPriceRFISCList::getTotalCost()
{
  float res = 0.0;
  for ( auto& nitem : *this ) { //filtered
    SVCS svcs;
    nitem.second.getSVCS( svcs, TPriceServiceItem::EnumSVCS::only_for_pay );
    for ( const auto& nsvc : svcs ) {
      if ( nsvc.second.clientValid() ) {
        res += nsvc.second.price;
      }
    }
  }
  return res;
}

std::string TPriceRFISCList::getTotalCurrency()
{
  for ( auto& nitem : *this ) { //filtered
    SVCS svcs;
    nitem.second.getSVCS( svcs, TPriceServiceItem::EnumSVCS::only_for_pay );
    for ( const auto& nsvc : svcs ) {
      if ( nsvc.second.clientValid() &&
           !nsvc.second.currency.empty() ) {
        return nsvc.second.currency;
      }
    }
  }
  return std::string("");
}


bool TPriceRFISCList::filterFromTerminal(const TPriceRFISCList& list)
{
  bool res = false;
  for ( auto& nitem : list ) { //filtered
    TPriceRFISCList::iterator oitem;
    if ( (oitem = find( nitem.first )) != end() ) { //new
      SVCS nsvcs;
      nitem.second.getSVCS( nsvcs, TPriceServiceItem::EnumSVCS::only_for_pay );
      for ( const auto& nsvc : nsvcs ) {
        SVCS::iterator f;
        if ( oitem->second.findSVC( nsvc.first, f ) ) {
          LogTrace(TRACE5) << "svc_id=" << nsvc.first.svcId << " " << f->second.status_direct;
            if ( f->second.clientValid() &&
                 f->second.status_direct == TPriceRFISCList::STATUS_DIRECT_ORDER  ) {
              f->second.status_direct = TPriceRFISCList::STATUS_DIRECT_SELECTED;
              res = true;
            }
        }
      }
    }
  }
  return res;
}

int getKeyPaxId( int pax_id )
{
  std::map<int, CheckIn::TCkinPaxTknItem> tkns;
  GetTCkinTickets(pax_id, tkns);
  boost::optional<CheckIn::TCkinPaxTknItem> tkn=algo::find_opt<boost::optional>(tkns, 1);

  return tkn?tkn.get().pax_id:pax_id;
}


void SegsPaxs::fromDB(int grp_id, int point_dep)
{
  segs.clear();
  TCkinRoute tckin_route;
  tckin_route.getRouteAfter(GrpId_t(grp_id),
                            TCkinRoute::NotCurrent,
                            TCkinRoute::OnlyDependent,
                            TCkinRoute::WithoutTransit);
  int seg_no = 0;
  segs.insert(std::make_pair(seg_no,PointGrpPaxs(point_dep,grp_id)));
  for ( auto p : tckin_route ) { //пробег по маршруту и группам
    tst();
    seg_no++;
    segs.insert(std::make_pair(seg_no,PointGrpPaxs(p.point_dep,p.grp_id)));
    LogTrace(TRACE5) << seg_no << "=" << p.point_dep << ",grp_id=" << p.grp_id;
  }
  //начитка пассажиров
  for ( auto s=segs.begin(); s!=segs.end(); ) {
    LogTrace(TRACE5) << s->first << "=(point_id=" << s->second.point_id << ",grp_id=" << s->second.grp_id << ")";
    TPaidRFISCList PaidRFISCList;
    PaidRFISCList.fromDB(s->second.grp_id,true);
    for ( auto item : PaidRFISCList ) {
      if (!item.second.service_quantity_valid()) {
        continue;
      }
      TPaidRFISCItem tmpItem=item.second;
      TPaxSegRFISCKey p=item.first;
/*!!!      if ( p.trfer_num != 0 ) { // используем только первый пункт, т.к. и так бежим по маршруту
        LogTrace(TRACE5) << "p.trfer_num=" << p.trfer_num << ",point_id =" << s->second.point_id << ",pax_id=" << p.pax_id;
        continue;
      }*/
    //LogTrace(TRACE5)<<tmpItem.service_quantity<<tmpItem.need;
      if (tmpItem.service_quantity>0 && // service_quantity - общее кол-во услуг
          tmpItem.need != ASTRA::NoExists &&
          tmpItem.need>0) // need - кол-во платить, paid -- paid - платные услуги
        {
          if ( segs.find( p.trfer_num ) == segs.end() ) {
            //!!!throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA"); //!!!
            continue;
          }
          if ( s->second.paxs.find(p.pax_id) == s->second.paxs.end() ) {
            s->second.paxs[ p.pax_id ] = getKeyPaxId( p.pax_id );
            LogTrace(TRACE5) << "p.trfer_num=" << p.trfer_num << ",point_id =" << s->second.point_id << ",pax_id=" << p.pax_id;
          }
        }
      }

      if ( s->second.paxs.empty() ) {
        segs.erase(s++);
      }
      else {
        ++s;
      }
  }
  if ( segs.empty() ) {
    tst();
    throw AstraLocale::UserException("MSG.EMD.SERVICES_ALREADY_PAID");
  }
}

std::string PaxsNames::getPaxName( int pax_id ) {
  if ( items.find( pax_id ) == items.end() ) {
    Qry.SetVariable( "pax_id", pax_id );
    Qry.Execute();
    CheckIn::TSimplePaxItem p;
    p.fromDB(Qry);
    std::string name = p.surname;
    if ( !p.name.empty() && !name.empty() ) {
      name += " ";
    }
    name += p.name;
    items.insert(make_pair(pax_id,name));
  }
  return items[pax_id];
}
