#include "bagtypes.h"
#include "astra_consts.h"
#include "astra_locale.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "basic.h"
#include "term_version.h"
#include "astra_misc.h"
#include "oralib.h"
#include <boost/crc.hpp>

#define NICKNAME "DJEK"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;


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

}// end namespace
