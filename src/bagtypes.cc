#include "bagtypes.h"
#include "astra_consts.h"
#include "astra_locale.h"
#include "astra_utils.h"
#include "term_version.h"
#include "astra_misc.h"
#include "basic.h"
#include "httpClient.h"
#include <boost/crc.hpp>

#define NICKNAME "DJEK"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace BASIC;


namespace BagPayment
{


void rficsItem::fromXML( xmlNodePtr node ) { // svc
  if ( string( "svc" ) != (char*)node->name ) {
    throw EXCEPTIONS::Exception("invalid xml: svc tag not found" );
  }
  xmlNodePtr n = GetNode( "@service_type", node );
  if ( n == NULL ) {
    throw EXCEPTIONS::Exception("invalid xml: @service_type tag not found" );
  }
  service_type = NodeAsString( n );
  if ( service_type.empty() ) {
    throw EXCEPTIONS::Exception("invalid xml: service_type is empty" );
  }
  n = GetNode( "@rfic", node );
  if ( n != NULL ) {
    rfic = NodeAsString( n );
  }
  if ( rfic.length() > 1 ) {
    throw EXCEPTIONS::Exception("invalid xml: @rfic tag invalid value" );
  }
  n = GetNode( "@rfisc", node );
  if ( n == NULL ) {
    throw EXCEPTIONS::Exception("invalid xml: @rfisc tag not found" );
  }
  rfisc = NodeAsString( n );
  if ( rfisc.empty() ) {
    throw EXCEPTIONS::Exception("invalid xml: rfisc is empty" );
  }
  n = GetNode( "@emd_type", node );
  if ( n != NULL ) {
    emd_type = NodeAsString( n );
  }
  if ( emd_type == "EMD-A" ) {
    emd_type = "A";
  }
  else
    if ( emd_type == "EMD-S" ) {
      emd_type = "S";
    }
    else
      if ( emd_type == "OTHR" ) {
        emd_type = "O";
      }
      else
        if ( !emd_type.empty() ) {
          throw EXCEPTIONS::Exception("invalid emd_type is invalid" );
        }
  ProgTrace( TRACE5, "emd_type=%s", emd_type.c_str() );
  node = node->children;
  ProgTrace( TRACE5, "node->name=%s", (char*)node->name);
  for ( ;node != NULL;node=node->next ) {
    if ( string( "name" ) != (char*)node->name ) {
      throw EXCEPTIONS::Exception("invalid xml: name tag not found" );
    }
    n = GetNode( "@language", node );
    if ( n == NULL ) {
      throw EXCEPTIONS::Exception("invalid xml: language tag not found" );
    }
    string lang = NodeAsString( n );
    if ( lang != "en" && lang != "ru" ) {
      throw EXCEPTIONS::Exception("invalid xml: language tag invalid value" );
    }
    if ( lang == "ru" ) {
      name = NodeAsString( node );
    }
    else {
      name_lat = NodeAsString( node );
    }
  }
  if ( name.empty() ) {
    throw EXCEPTIONS::Exception("invalid xml: name tag is empty" );
  }
  if ( name_lat.empty() ) {
    throw EXCEPTIONS::Exception("invalid xml: name_lat tag is empty" );
  }
}

 void grpRFISC::fromDB( int list_id ) { // загрузка данных по группе
   RFISCList.clear();
   TQuery Qry(&OraSession);
   Qry.Clear();
   Qry.SQLText =
     "SELECT service_type, rfic, rfisc, emd_type, name, name_lat "
     " FROM grp_rfisc_lists "
     "WHERE list_id=:list_id";
   Qry.CreateVariable( "list_id", otInteger, list_id );
   Qry.Execute();
/*!!!   if ( Qry.Eof ) {
     throw EXCEPTIONS::Exception("rfics not found %d", list_id );
   }*/
   for ( ;!Qry.Eof;Qry.Next()) {
     rficsItem item;
     item.rfisc = Qry.FieldAsString( "rfisc" );
     item.rfic = Qry.FieldAsString( "rfic" );
     item.service_type = Qry.FieldAsString( "service_type" );
     item.emd_type = Qry.FieldAsString( "emd_type" );
     item.name = Qry.FieldAsString( "name" );
     item.name_lat = Qry.FieldAsString( "name_lat" );
     RFISCList.insert( make_pair(item.rfisc,item));
   }
 }

 //1. Если он пустой
 //3. Если совпали CRC32 и содержимое
 //4. Если совпали CRS32 и не совпало содержимое
 //5. Если нет таких данных
 int grpRFISC::toDB(  ) {
    //сохраняем только новые
   if ( empty() ) { //1
     return ASTRA::NoExists;
   }
   TQuery Qry(&OraSession);
   Qry.Clear();
   Qry.SQLText =
     "SELECT id FROM  bag_types_lists "
     " WHERE crc=:crs";
   Qry.CreateVariable( "crc", otInteger, getCRC32() );
   Qry.Execute();
   for ( ;!Qry.Eof; Qry.Next() ) { //3+4
     grpRFISC rfsc;
     rfsc.fromDB( Qry.FieldAsInteger( "id" ) );
     if ( *this == rfsc ) { //3 !!!
       return Qry.FieldAsInteger( "id" );
     }
   }
   //4+5
   Qry.Clear();
   Qry.SQLText =
     "SELECT id__seq.nextval AS id FROM dual"; //!!! отдельный выдуманный
   Qry.Execute();
   int id = Qry.FieldAsInteger( "id" );
   Qry.Clear();
   Qry.SQLText =
     "INSERT INTO bag_types_lists(id,crc) VALUES(:id,crc)";
   Qry.CreateVariable( "id", otInteger, id );
   Qry.Execute();
   Qry.Clear();
   Qry.SQLText =
     "INSERT INTO grp_rfisc_lists(list_id,rfisc,service_type,rfic,emd_type,name,name_lat) "
     "VALUES(:list_id,:rfisc,:service_type,:rfic,:emd_type,:name,:name_lat)";
   Qry.CreateVariable( "list_id", otInteger, id );
   Qry.DeclareVariable( "rfisc", otString );
   Qry.DeclareVariable( "service_type", otString );
   Qry.DeclareVariable( "rfic", otString );
   Qry.DeclareVariable( "emd_type", otString );
   Qry.DeclareVariable( "name", otString );
   Qry.DeclareVariable( "name_lat", otString );
   for ( map<std::string,rficsItem>::iterator ifrsc=RFISCList.begin(); ifrsc!=RFISCList.end(); ifrsc++ ) {
     Qry.SetVariable( "rfisc", ifrsc->first );
     Qry.SetVariable( "service_type", ifrsc->second.service_type );
     Qry.SetVariable( "rfic", ifrsc->second.rfic );
     Qry.SetVariable( "emd_type", ifrsc->second.emd_type );
     Qry.SetVariable( "name", ifrsc->second.name );
     Qry.SetVariable( "name_lat", ifrsc->second.name_lat );
     Qry.Execute();
   }
   return id;
 }

bool grpRFISC::fromXML( xmlNodePtr node ) { //разбор node=svc_availability
/*!!!   if (node==NULL)
     throw EXCEPTIONS::Exception("invalid xml: svc_availability tag not found" );*/
   RFISCList.clear();
   if (node!=NULL) { //!!!
     node = node->children;
  /*!!!   if (node==NULL)
       throw EXCEPTIONS::Exception("invalid xml: svc tag not found" );*/
     for (;node!=NULL;node=node->next) {
       rficsItem item;
       item.fromXML(node);
       if ( RFISCList.insert(make_pair(item.rfisc,item)).second ) {
         throw EXCEPTIONS::Exception("invalid xml: rfisc already exists" );
       }
     }
   }
   return empty();
}

 std::string grpRFISC::normilizeData( ) {
   string res;
   for ( map<std::string,rficsItem>::iterator ifrsc=RFISCList.begin(); ifrsc!=RFISCList.end(); ifrsc++ ) {
     res += ifrsc->first + ";";
   }
   return res;
 }

 int grpRFISC::getCRC32() {
   if ( empty() ) {
     return 0; //!!!
   }
   boost::crc_basic<32> crc32( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true );
   crc32.reset();
   string buf = normilizeData();
   crc32.process_bytes( buf.c_str(), buf.size() );
   int crc_key = crc32.checksum();
   return crc_key;
 }

void paxNormsPC::from_BD( int pax_id )
{
  free_baggage_norm.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT lang,text FROM  pax_norms_pc "
    " WHERE pax_id=:pax_id"
    " ORDER BY lang, page_no";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  string text;
  string lang;
  for ( ;!Qry.Eof; Qry.Next() ) {
    if ( lang != Qry.FieldAsString( "lang" ) ) {
      if ( !text.empty() ) {
        free_baggage_norm.insert( make_pair( lang, text ) );
      }
      lang = Qry.FieldAsString( "lang" );
      text.clear();
    }
    text += Qry.FieldAsString( "text" );
  }
}

void paxNormsPC::toDB( int pax_id ) {
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "DELETE pax_norms_pc WHERE pax_id=:pax_id";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  if ( free_baggage_norm.empty() ) {
    return;
  }
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO pax_norms_pc(pax_id,lang,page_no,text) "
    " VALUES(:pax_id,:lang,:page_no,:text) ";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.DeclareVariable( "lang", otString );
  Qry.DeclareVariable( "page_no", otInteger );
  Qry.DeclareVariable( "text", otString );
  for ( std::map<std::string,std::string>::iterator inorm=free_baggage_norm.begin(); inorm!=free_baggage_norm.end(); inorm++ ) {
    int i=0;
    string text = inorm->second;
    Qry.SetVariable( "lang", inorm->first );
    while ( !text.empty() ) {
      Qry.SetVariable( "text", text.substr( 0, page_size ) );
      Qry.SetVariable( "page_no", i );
      Qry.Execute();
      i++;
      text.erase( 0, page_size );
    }
  }
}

bool paxNormsPC::from_XML( xmlNodePtr node ) //tag free_baggage_norm
{
  if ( string( "free_baggage_norm" ) != (char*)node->name ) {
    throw EXCEPTIONS::Exception("invalid xml: 'free_baggage_norm' tag not found" );
  }
  xmlNodePtr n = node->children;
  while ( n != NULL &&  string( "text" ) == (char*)n->name ) {
    string text = NodeAsString( n );
    if ( text.empty() ) {
      continue;
    }
    xmlNodePtr langNode = GetNode( "@lang", n );
    if ( langNode == NULL ) {
      throw EXCEPTIONS::Exception("invalid xml: '@lang' tag not founs" );
    }
    string lang = NodeAsString( langNode );
    if ( lang != "en" && lang != "ru" ) {
      throw EXCEPTIONS::Exception("invalid xml: '@lang' tag  invaliud value '" + lang + "'" );
    }
    free_baggage_norm.insert( make_pair( lang, text ));
    n = n->next;
  }
  return !free_baggage_norm.empty(); //???
}

void requestRFISC::fillTESTDATA()
{
  clear();
  passenger pass;
  pass.id = 1;
  pass.ticket = "2986112345678";
  pass.category = "ADT";
  StrToDateTime( "1975-07-15", "yyyy-mm-dd", pass.birthdate );
  pass.sex = "male";
  addPassenger( pass );
  pass.id = 2;
  pass.ticket = "2986112345679";
  pass.category = "INS";
  pass.birthdate = ASTRA::NoExists;
  pass.sex = "";
  addPassenger( pass );
  segment seg;
  seg.id = 1;
  seg.company = "UT"; //???
  seg.flight = "123A";
  seg.departure = "SVO";
  seg.arrival = "TJM";
  BASIC::StrToDateTime( "2015-11-25T17:20:00", "yyyy-mm-ddThh:mm:ss", seg.departure_time );
  seg.equipment = "320";
  addSegment( seg );
  seg.id = 2;
  seg.company = "ЮТ"; //???
  seg.flight = "321";
  seg.departure = "TJM";
  seg.arrival = "ШРМ";
  StrToDateTime( "2015-11-26T06:35:00", "yyyy-mm-ddThh:mm:ss", seg.departure_time );
  seg.equipment = "";
  addSegment( seg );
}

xmlDocPtr requestRFISC::createRequest()
{
  if ( passengers.empty() || segments.empty() ) {
    throw EXCEPTIONS::Exception("requestRFISC::createRequest: no data for request" );
  }
  xmlDocPtr res = CreateXMLDoc( "query");
  xmlNodePtr node = NewTextChild( res->children, "svc_availability" );
  for ( vector<passenger>::iterator ipass=passengers.begin(); ipass!=passengers.end(); ipass++ ) {
    xmlNodePtr passNode = NewTextChild( node, "passenger" );
    SetProp( passNode, "id", ipass->id );
    SetProp( passNode, "ticket_number", ipass->ticket );
    SetProp( passNode, "category", ipass->category );
    if ( ipass->birthdate != ASTRA::NoExists ) {
      SetProp( passNode, "birthdate", BASIC::DateTimeToStr( ipass->birthdate, "yyyy-mm-dd" ) );
    }
    if ( !ipass->sex.empty() ) {
      SetProp( passNode, "sex", ipass->sex );
    }
  }
  for ( vector<segment>::iterator iseg=segments.begin(); iseg!=segments.end(); iseg++ ) {
    xmlNodePtr segNode = NewTextChild( node, "segment" );
    SetProp( segNode, "id", iseg->id );
    SetProp( segNode, "company", iseg->company );
    SetProp( segNode, "flight", iseg->flight );
    SetProp( segNode, "departure", iseg->departure );
    SetProp( segNode, "arrival", iseg->arrival );
    SetProp( segNode, "departure_time", BASIC::DateTimeToStr( iseg->departure_time, "yyyy-mm-ddThh:mm:ss" ) );
    if ( !iseg->equipment.empty() ) {
      SetProp( segNode, "equipment", iseg->equipment );
    }
  }
  return res;
}

std::string requestRFISC::createRequestStr()
{
  string res = XMLTreeToText( createRequest() );
  res = ConvertCodepage( res, "CP866", "UTF-8" );
  return res;
}


void sendRequestTESTRFISC()
{
  requestRFISC r;
  r.fillTESTDATA();
  RequestInfo request;
  request.action = "";
  request.login = "";
  request.pswd = "";
  request.content = r.createRequestStr();
  ProgTrace( TRACE5, "sendRequestTESTRFISC: request.content=%s", r.createRequestStr().c_str() );
  request.using_ssl = false;
  request.host = "test.sirena-travel.ru";
  request.port = 9000;
  request.path = "/astra";
  request.sirena_exch = true;
  ResponseInfo response;
  httpClient_main(request, response);
  if (response.content.size()>200)
  {
    ProgTrace(TRACE5, "%s: %s", __FUNCTION__, response.content.substr(0,200).c_str());
    ProgTrace(TRACE5, "%s: %s", __FUNCTION__, response.content.substr(response.content.size()-200).c_str());
  }
  else
    ProgTrace(TRACE5, "%s: %s", __FUNCTION__, response.content.c_str());
  XMLDoc doc(response.content);
  if (doc.docPtr()==NULL)
    ProgError(STDLOG, "%s: %s", __FUNCTION__, "Wrong XML!");
  else
    ProgTrace(TRACE5, "%s: %s", __FUNCTION__, "Good XML!");
}

}// end namespace

int test_rfisc(int argc,char **argv)
{
  BagPayment::sendRequestTESTRFISC();
  return 0;
}

