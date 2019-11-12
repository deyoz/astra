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
  LogTrace(TRACE5) << node->name;
  LogTrace(TRACE5) << node->children->name;
  svc_id = NodeAsString( "svc_id", node );
  doc_id = NodeAsString( "doc_id", node, "" );
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
  svc_id = NodeAsString( "svc_id", node );
  price = NodeAsFloat( "price", node );
  currency = NodeAsString( "currency", node );
  ticket_cpn = NodeAsString( "ticket_cpn", node, "" );
  ticknum = NodeAsString( "ticknum", node, "" );
}

void SvcFromSirena::toContextXML(xmlNodePtr node) const {
  NewTextChild( node, "svc_id", svc_id );
  NewTextChild( node, "doc_id", doc_id );
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
  NewTextChild( node, "svc_id", svc_id );
  NewTextChild( node, "price", price );
  NewTextChild( node, "currency", currency );
  NewTextChild( node, "ticket_cpn", ticket_cpn );
  NewTextChild( node, "ticknum", ticknum );
  NewTextChild(node,"valid", valid());
}

void SvcFromSirena::toDB( TQuery& Qry) const
{
  Qry.SetVariable("svc_id",svc_id);
  Qry.SetVariable("doc_id",doc_id);
  Qry.SetVariable("pass_id",pass_id);
  Qry.SetVariable("seg_id",seg_id);
  Qry.SetVariable("price",price);
  Qry.SetVariable("currency",currency);
  Qry.SetVariable("ticket_cpn",ticket_cpn);
  Qry.SetVariable("ticknum",ticknum);
}

void SvcFromSirena::fromDB(TQuery& Qry,const std::string& lang)
{
  svc_id = Qry.FieldAsString("svc_id");
  doc_id = Qry.FieldAsString("doc_id");
  pass_id = Qry.FieldAsString("pass_id");
  seg_id = Qry.FieldAsString("seg_id");
  price = Qry.FieldAsFloat("price");
  name = (lang==AstraLocale::LANG_EN?Qry.FieldAsString("name_view_lat"):Qry.FieldAsString("name_view"));
  currency = Qry.FieldAsString("currency");
  ticket_cpn = Qry.FieldAsString("ticket_cpn");
  ticknum = Qry.FieldAsString("ticknum");
  LogTrace(TRACE5) << toString();
}

bool SvcFromSirena::valid() const
{
  return price!=ASTRA::NoExists &&
         price>=0.0 &&
         !currency.empty() &&
         status_direct != TPriceRFISCList::STATUS_DIRECT_PAID; //!!!
}

std::string SvcFromSirena::toString() const {
  std::ostringstream s;
  s << "svc_id=" << svc_id << ",price=" << price << ",cuurency=" << currency << ",doc_id=" << doc_id << ",ticknum="<<ticknum<<"/"<<ticket_cpn;
  return s.str();
}

const TPriceServiceItem& TPriceServiceItem::toXML(xmlNodePtr node, const std::string& svc_id) const
{
  if (node==NULL) return *this;
  TPaxSegRFISCKey::toXML(node);
  if ( std::string( NodeAsString( "name_view", node) ).empty() ) {
    ReplaceTextChild(node, "name_view", name_view() ); //!!!
  }
  NewTextChild(node,"pax_name",pax_name);
  if ( svcs.find( svc_id ) == svcs.end() ) {
    throw EXCEPTIONS::Exception( std::string("scvs not found svc_id") + svc_id );
  }
  SvcFromSirena svc = svcs.at(svc_id);
  svc.toXML(node);
  return *this;
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
      svcs.emplace(svc.svc_id,svc);
      n = n->next;
    }
  }
  if ( list_id != ASTRA::NoExists ) {
    getListItem();
  }
  return *this;
}

TPriceServiceItem& TPriceServiceItem::fromXML(xmlNodePtr node)
{
  if (node==NULL) return *this;
  TPaxSegRFISCKey::fromXML(node);
  xmlNodePtr node2 = node->children;
  pax_name = NodeAsStringFast("pax_name",node2,"");
  SvcFromSirena svc;
  svc.fromXML(node);
  svcs.emplace(svc.svc_id,svc);
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

const TPriceServiceItem& TPriceServiceItem::toDB(TQuery &Qry, const std::string& svc_id) const
{
  Qry.SetVariable("name_view", name_view(AstraLocale::LANG_RU));
  Qry.SetVariable("name_view_lat", name_view(AstraLocale::LANG_EN));
  //Qry.SetVariable("list_id",(list_id==ASTRA::NoExists?FNull:list_id));
  if ( svcs.find( svc_id ) == svcs.end() ) {
    throw EXCEPTIONS::Exception( std::string("scvs not found svc_id") + svc_id );
  }
  SvcFromSirena svc = svcs.at(svc_id);
  svc.toDB(Qry);
  return *this;
}

TPriceServiceItem& TPriceServiceItem::fromDB(TQuery &Qry, const std::string& lang)
{
  TPaxSegRFISCKey::fromDB(Qry);
  SvcFromSirena svc;
  svc.fromDB(Qry,lang);
  int svc_id = 100000;
  std::pair<std::map<std::string,SvcFromSirena>::iterator,bool> ret;
  ret = svcs.insert(make_pair(svc.svc_id,svc)); //!!!здесь потеря
  if ( ret.second == false ) { //один и тот же svc_id, но в разных PNR
     svcs.insert(make_pair(IntToString(svc_id),svc)); //псевдо svc_id для печати Дена
     svc_id++;
  }
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
  cost = NodeAsFloat( "cost",node,ASTRA::NoExists);
  if ( cost != ASTRA::NoExists ) {
    currency = ElemToElemId( etCurrency, NodeAsString( "cost/@curr",node), fmt );
  }
}

void SvcEmdCost::toXML(xmlNodePtr node) const {
  if ( cost != ASTRA::NoExists ) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << cost;
    SetProp(NewTextChild(node,"cost",stream.str()),"curr",currency); //ElemIdToElem
  }
}

void SvcEmdSvcsReq::toXML(xmlNodePtr node) const
{
  for ( const auto& svc : svcs ) {
    SetProp( NewTextChild( node, "svc" ), "id", svc );
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
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "INSERT INTO pay_services(grp_id,pax_id,transfer_num,rfisc,service_type,airline,name_view,name_view_lat,list_id,svc_id,doc_id,pass_id,seg_id,price,currency,ticknum,ticket_cpn,time_paid)"
    "  VALUES(:grp_id,:pax_id,:transfer_num,:rfisc,:service_type,:airline,:name_view,:name_view_lat,:list_id,:svc_id,:doc_id,:pass_id,:seg_id,:price,:currency,:ticknum,:ticket_cpn,:time_paid)";
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
  Qry.DeclareVariable("doc_id",otString);
  Qry.DeclareVariable("pass_id",otInteger);
  Qry.DeclareVariable("seg_id",otInteger);
  Qry.DeclareVariable("price",otFloat);
  Qry.DeclareVariable("currency",otString);
  Qry.DeclareVariable("ticknum",otString);
  Qry.DeclareVariable("ticket_cpn",otString);
  Qry.CreateVariable("time_paid",otDate, BASIC::date_time::NowUTC());
  for ( const auto &p : *this ) {
    for ( const auto svc : p.second.svcs ) {
      if ( svc.second.status_direct == STATUS_DIRECT_PAID ) {
        p.first.toDB(Qry);
        p.second.toDB(Qry,svc.first);
        Qry.Execute();
      }
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
  error_code = NodeAsString("error_code",node,"");
  error_message = NodeAsString("error_message",node,"");
  SvcEmdPayDoc::fromXML(node);
  SvcEmdCost::fromXML(node);
  SvcEmdTimeout::fromXML(node);
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
    tst();
    item.fromContextXML(node);
    tst();
    insert( std::make_pair(item,item) );
    node = node->next;
  }
  tst();
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
    item.fromXML(node);
    auto p = find(item);
    if ( p != end() ) {
      auto svc = item.svcs.begin();
      p->second.svcs.emplace( svc->first, svc->second );
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
    for ( const auto &svc : p.second.svcs ) {
/*???      if ( svc.second.status_direct == TPriceRFISCList::STATUS_DIRECT_PAID ) {
        tst();
        continue;
      }
      tst();*/
      xmlNodePtr n = NewTextChild( node2, "item" );
      p.second.toXML( n, svc.first );
      if ( svc.second.valid() ) {
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
        TPriceServiceItem priceItem(tmpItem,paxNames.getPaxName(tmpItem.pax_id),std::map<std::string,SvcFromSirena>());
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
  for ( const auto& nsvc : svcs ) {
    for ( auto& oitem : *this ) {
      if ( oitem.second.svcs.find( nsvc.id ) != oitem.second.svcs.end() ) {
        oitem.second.svcs[ nsvc.id ].status_svc = nsvc.status;
        std::string ticket_cpn = nsvc.ticket_cpn.empty()?oitem.second.svcs[nsvc.id].ticket_cpn:nsvc.ticket_cpn;
        std::string ticknum = nsvc.ticknum.empty()?oitem.second.svcs[nsvc.id].ticknum:nsvc.ticknum;
        oitem.second.svcs[nsvc.id].ticket_cpn = ticket_cpn;
        oitem.second.svcs[nsvc.id].ticknum = ticknum;
        break;
      }
    }
  }
}

bool TPriceRFISCList::synchFromSirena(const TPriceRFISCList& list, bool only_delete)
{
  bool res = true;
  //insert or update
  if ( !only_delete ) {
    for ( const auto& nitem : list ) {
      TPriceRFISCList::iterator oitem;
      if ( (oitem = find( nitem.first )) == end() ) { //new
        LogTrace(TRACE5) << "clear list_id AND view_name!!";
        insert( std::make_pair( nitem.first, nitem.second ) );
      }
      else { //update
        for ( const auto& nsvc : nitem.second.svcs ) {
          if ( oitem->second.svcs.find( nsvc.first ) == oitem->second.svcs.end() ) {
            oitem->second.svcs.insert( std::make_pair(nsvc.first,nsvc.second));
          }
          else {
            LogTrace(TRACE5) << nsvc.second.toString();
            std::string status_direct = oitem->second.svcs[nsvc.first].status_direct;
            std::string ticket_cpn = nsvc.second.ticket_cpn.empty()?oitem->second.svcs[nsvc.first].ticket_cpn:nsvc.second.ticket_cpn;
            std::string ticknum = nsvc.second.ticknum.empty()?oitem->second.svcs[nsvc.first].ticknum:nsvc.second.ticknum;
            oitem->second.svcs[nsvc.first] = nsvc.second;
            oitem->second.svcs[nsvc.first].ticket_cpn = ticket_cpn;
            oitem->second.svcs[nsvc.first].ticknum = ticknum;
            oitem->second.svcs[nsvc.first].status_direct = status_direct;
          }
        }
      }
    }
  }
  //delete
  for ( auto oitem=begin(); oitem!=end(); ) {
    TPriceRFISCList::const_iterator nitem;
    if ( (nitem = list.find(oitem->first)) == list.end()) {
      this->erase(oitem++);
      res = false;
    }
    else {
      for ( auto osvc=oitem->second.svcs.begin(); osvc!=oitem->second.svcs.end(); ) {
        if ( nitem->second.svcs.find(osvc->first) == nitem->second.svcs.end() ) {
          oitem->second.svcs.erase(osvc++);
          res = false;
        }
        else {
          ++osvc;
        }
      }
      ++oitem;
    }
  }
  return res;
}

void TPriceRFISCList::setStatusDirect( const std::string &from, const std::string &to )
{
  for ( auto& oitem : *this ) {
    for ( auto& osvc : oitem.second.svcs ) {
      if ( osvc.second.status_direct == from ) {
        LogTrace(TRACE5)<<"svc_id=" << osvc.first << " change direct status from " << osvc.second.status_direct << " to " << to;
        osvc.second.status_direct = to;
      }
    }
  }
}

bool TPriceRFISCList::haveStatusDirect( const std::string& statusDirect )
{
  std::vector<std::string> svcs;
  return haveStatusDirect( statusDirect, svcs );
}

bool TPriceRFISCList::haveStatusDirect( const std::string& statusDirect, std::vector<std::string> &svcs )
{
  svcs.clear();
  for ( const auto& oitem : *this ) {
    for ( const auto& osvc : oitem.second.svcs ) {
      if ( osvc.second.valid() &&
           osvc.second.status_direct == statusDirect ) {
        svcs.push_back( osvc.first );
      }
    }
  }
  return !svcs.empty();
}

bool TPriceRFISCList::getNextIssueQueryGrpEMD( std::vector<std::string> &svcs)
{
  svcs.clear();
  TServiceType::Enum service_type = TServiceType::Enum::Unknown;
  for ( auto& oitem : *this ) {
    for ( auto& osvc : oitem.second.svcs ) {
      if ( osvc.second.status_direct != TPriceRFISCList::STATUS_DIRECT_SELECTED ) {
        continue;
      }
      if ( service_type == TServiceType::Enum::Unknown ) {
        service_type = oitem.second.service_type;
      }
      //!!!if ( service_type == oitem.second.service_type ) {
        svcs.push_back( osvc.first );
        osvc.second.status_direct = TPriceRFISCList::STATUS_DIRECT_ISSUE_QUERY;
        LogTrace(TRACE5) << __func__ << " " << osvc.first;
       //!!! break; //only one at step
      //!!!}
    }
/*!!!    if ( !svcs.empty() ) { //по одному
      break;
    }*/
  }
  return !svcs.empty();
}

bool TPriceRFISCList::terminalChoiceAny()
{
  for ( auto& nitem : *this ) { //filtered
    for ( const auto& nsvc : nitem.second.svcs ) {
      if ( nsvc.second.valid() ) {
        return true;
      }
    }
  }
  return false;
}

bool TPriceRFISCList::filterFromTerminal(const TPriceRFISCList& list)
{
  bool res = false;
  for ( auto& nitem : list ) { //filtered
    TPriceRFISCList::iterator oitem;
    if ( (oitem = find( nitem.first )) != end() ) { //new
      for ( const auto& nsvc : nitem.second.svcs ) {
        if ( oitem->second.svcs.find( nsvc.first ) != oitem->second.svcs.end() ) {
          LogTrace(TRACE5) << "svc_id=" << nsvc.first << " " << oitem->second.svcs[ nsvc.first ].status_direct;
            if ( oitem->second.svcs[ nsvc.first ].valid() &&
                 oitem->second.svcs[ nsvc.first ].status_direct == TPriceRFISCList::STATUS_DIRECT_ORDER  ) {
              oitem->second.svcs[ nsvc.first ].status_direct = TPriceRFISCList::STATUS_DIRECT_SELECTED;
              res = true;
            }
        }
      }
    }
  }
  return res;
}

int getKeyPaxId( int pax_id, TQuery &Qry )
{
  Qry.SetVariable("pax_id",pax_id);
  Qry.Execute();
  int res = Qry.Eof?ASTRA::NoExists:Qry.FieldAsInteger("pax_id");
  if (res ==ASTRA::NoExists) { //!!!
    res = pax_id;
  }
  return res;
}


void SegsPaxs::fromDB(int grp_id, int point_dep)
{
  segs.clear();
  TCkinRoute tckin_route;
  tckin_route.GetRouteAfter(grp_id, crtNotCurrent, crtOnlyDependent);
  int seg_no = 0;
  segs.insert(std::make_pair(seg_no,PointGrpPaxs(point_dep,grp_id)));
  for ( auto p : tckin_route ) { //пробег по маршруту и группам
    tst();
    seg_no++;
    segs.insert(std::make_pair(seg_no,PointGrpPaxs(p.point_dep,p.grp_id)));
    LogTrace(TRACE5) << seg_no << "=" << p.point_dep << ",grp_id=" << p.grp_id;
  }
  //начитка пассажиров
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT pax.ticket_no, pax.coupon_no, pax.ticket_rem, pax.ticket_confirm, "
    "       pax.grp_id, pax.pax_id "
    "FROM pax, tckin_pax_grp, "
    "     (SELECT tckin_pax_grp.tckin_id, "
    "             tckin_pax_grp.first_reg_no-pax.reg_no AS distance "
    "      FROM pax, tckin_pax_grp "
    "      WHERE pax.grp_id=tckin_pax_grp.grp_id AND pax.pax_id=:pax_id) p "
    "WHERE tckin_pax_grp.tckin_id=p.tckin_id AND "
    "      pax.grp_id=tckin_pax_grp.grp_id AND "
    "      tckin_pax_grp.first_reg_no-pax.reg_no=p.distance AND "
    "      tckin_pax_grp.seg_no=1";
  Qry.DeclareVariable("pax_id",otInteger);
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
      if ( p.trfer_num != 0 ) { // используем только первый пункт, т.к. и так бежим по маршруту
        LogTrace(TRACE5) << "p.trfer_num=" << p.trfer_num << ",point_id =" << s->second.point_id << ",pax_id=" << p.pax_id;
        continue;
      }
    //LogTrace(TRACE5)<<tmpItem.service_quantity<<tmpItem.need;
      if (tmpItem.service_quantity>0 && // service_quantity - общее кол-во услуг
          tmpItem.need != ASTRA::NoExists &&
          tmpItem.need>0) // need - кол-во платить, paid -- paid - платные услуги
        {
          if ( segs.find( p.trfer_num ) == segs.end() ) {
            throw AstraLocale::UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA"); //!!!
          }
          if ( s->second.paxs.find(p.pax_id) == s->second.paxs.end() ) {
            s->second.paxs[ p.pax_id ] = getKeyPaxId( p.pax_id, Qry );
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
