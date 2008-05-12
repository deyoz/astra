#include "spp_cek.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

#include "exceptions.h"
#include "basic.h"
#include "stl_utils.h"
#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "slogger.h"
#include "base_tables.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "develop_dbf.h"
#include "helpcpp.h"
#include "misc.h"
#include "sopp.h"
#include "timer.h"
#include "xml_unit.h"
#include "xml_stuff.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

enum TSQLTYPE { sql_date, sql_number, sql_char };

enum TModify { sppnochange, sppinsert, sppupdate, sppdelete };

const string spp__a_dbf_fields =
"AA CHAR(2),"
"ZADERGKA CHAR(34),"
"PNR CHAR(7),"
"PNRS CHAR(7),"
"PR CHAR(3),"
"AR NUMBER(10),"
"VRD CHAR(3),"
"VDV CHAR(3),"
"KUG CHAR(3),"
"BL CHAR(3),"
"NR CHAR(5),"
"DR CHAR(1),"
"LR CHAR(3),"
"KR CHAR(3),"
"RAS NUMBER(5),"
"TOR NUMBER(1),"
"TVC CHAR(4),"
"VLD CHAR(3),"
"BNP CHAR(5),"
"BNF CHAR(5),"
"FAM CHAR(12),"
"RZ CHAR(1),"
"RPVSN CHAR(3),"
"ABEK CHAR(3),"
"TKABEK CHAR(4),"
"TKABSM CHAR(4),"
"ABSM CHAR(3),"
"DN DATE,"
"ZA CHAR(1),"
"RPVS CHAR(3),"
"OT CHAR(1),"
"RPVSP CHAR(3),"
"TKAO CHAR(4),"
"PRIZ NUMBER(1),"
"BLS CHAR(3),"
"NRS CHAR(5),"
"RDS CHAR(1),"
"PRUG CHAR(1),"
"MSK CHAR(1),"
"MMIN CHAR(3),"
"NMSP CHAR(3),"
"NMSF CHAR(3),"
"OST NUMBER(5),"
"TRZ NUMBER(5),"
"TRDZ NUMBER(5),"
"FKZ NUMBER(5),"
"OR1 NUMBER(1),"
"OR2 NUMBER(1),"
"KRE NUMBER(3),"
"LKRE NUMBER(3),"
"BOP NUMBER(1),"
"PKZ NUMBER(5),"
"LPC NUMBER(5),"
"EKP NUMBER(1),"
"NSOP CHAR(1),"
"BP NUMBER(3),"
"MBCF NUMBER(6),"
"CBCF NUMBER(4,2),"
"RVC NUMBER(4,2),"
"TST NUMBER(4),"
"TVZ NUMBER(5),"
"TRS NUMBER(5),"
"MVZD NUMBER(6),"
"FZAG NUMBER(5),"
"S1 CHAR(3),"
"S2 NUMBER(4),"
"S3 NUMBER(4),"
"S4 CHAR(3),"
"S5 CHAR(3),"
"S6 NUMBER(4),"
"S7 CHAR(3),"
"S8 NUMBER(4),"
"S10 NUMBER(4),"
"KUR NUMBER(2),"
"S2R NUMBER(4),"
"S6R NUMBER(4),"
"T2 NUMBER(3),"
"T3 NUMBER(3),"
"T4 NUMBER(3),"
"T6 NUMBER(5),"
"T7 NUMBER(5),"
"T9 NUMBER(5),"
"T11 NUMBER(5),"
"F2 NUMBER(3),"
"F3 NUMBER(3),"
"F4 NUMBER(3),"
"F6 NUMBER(5),"
"F7 NUMBER(5),"
"F8 NUMBER(5),"
"F9 NUMBER(5),"
"F11 NUMBER(5),"
"TO NUMBER(6),"
"TO1 NUMBER(6),"
"TO2 NUMBER(6),"
"TO3 NUMBER(6),"
"TO4 NUMBER(6),"
"TO5 NUMBER(6),"
"FKZG NUMBER(6),"
"IZG NUMBER(6),"
"ZGO NUMBER(6),"
"ORVE NUMBER(3),"
"C1T NUMBER(4)";

const string spp__d_dbf_fields =
"AA CHAR(2),"
"ZADERGKA CHAR(34),"
"PNR CHAR(7),"
"PNRS CHAR(7),"
"PR CHAR(3),"
"AR NUMBER(10),"
"VRD CHAR(3),"
"VDV CHAR(3),"
"KUG CHAR(3),"
"BL CHAR(3),"
"NR CHAR(5),"
"DR CHAR(1),"
"LR CHAR(3),"
"KR CHAR(3),"
"RAS NUMBER(5),"
"TOR NUMBER(1),"
"TVC CHAR(4),"
"VLD CHAR(3),"
"BNP CHAR(5),"
"BNF CHAR(5),"
"FAM CHAR(12),"
"RZ CHAR(1),"
"RPVSN CHAR(3),"
"ABEK CHAR(3),"
"TKABEK CHAR(4),"
"TKABSM CHAR(4),"
"ABSM CHAR(3),"
"DN DATE,"
"ZA CHAR(1),"
"RPVS CHAR(3),"
"OT CHAR(1),"
"RPVSP CHAR(3),"
"TKAO CHAR(4),"
"PRIZ NUMBER(1),"
"BLS CHAR(3),"
"NRS CHAR(5),"
"RDS CHAR(1),"
"PRUG CHAR(1),"
"MSK CHAR(1),"
"MMIN CHAR(3),"
"NMSP CHAR(3),"
"NMSF CHAR(3),"
"OST NUMBER(5),"
"TRZ NUMBER(5),"
"TRDZ NUMBER(5),"
"FKZ NUMBER(5),"
"OR1 NUMBER(1),"
"OR2 NUMBER(1),"
"KRE NUMBER(3),"
"LKRE NUMBER(3),"
"BOP NUMBER(1),"
"PKZ NUMBER(5),"
"LPC NUMBER(5),"
"EKP NUMBER(1),"
"NSOP CHAR(1),"
"BP NUMBER(3),"
"MBCF NUMBER(6),"
"CBCF NUMBER(4,2),"
"RVC NUMBER(4,2),"
"TST NUMBER(4),"
"TVZ NUMBER(5),"
"TRS NUMBER(5),"
"MVZD NUMBER(6),"
"FZAG NUMBER(5),"
"S1 CHAR(3),"
"S2 NUMBER(4),"
"S3 NUMBER(4),"
"S4 CHAR(3),"
"S5 CHAR(3),"
"S6 NUMBER(4),"
"S7 CHAR(3),"
"S8 NUMBER(4),"
"S10 NUMBER(4),"
"KUR NUMBER(2),"
"S2R NUMBER(4),"
"S6R NUMBER(4),"
"T2 NUMBER(3),"
"T3 NUMBER(3),"
"T4 NUMBER(3),"
"T6 NUMBER(5),"
"T7 NUMBER(5),"
"T8 NUMBER(4),"
"T9 NUMBER(5),"
"T11 NUMBER(5),"
"F2 NUMBER(3),"
"F3 NUMBER(3),"
"F4 NUMBER(3),"
"F6 NUMBER(5),"
"F7 NUMBER(5),"
"F8 NUMBER(5),"
"F9 NUMBER(5),"
"F11 NUMBER(5),"
"TO NUMBER(6),"
"TO1 NUMBER(6),"
"TO2 NUMBER(6),"
"TO3 NUMBER(6),"
"TO4 NUMBER(6),"
"TO5 NUMBER(6),"
"FKZG NUMBER(6),"
"IZG NUMBER(6),"
"ZGO NUMBER(6),"
"ORVE NUMBER(3),"
"C1T NUMBER(4)";

const string spp__ak_dbf_fields =
"PER CHAR(7),"
"AR NUMBER(4),"
"PR CHAR(3),"
"BL CHAR(3),"
"PNR CHAR(7),"
"NR CHAR(5),"
"DR CHAR(1),"
"AV CHAR(3),"
"TKV CHAR(4),"
"AP CHAR(3),"
"TKP CHAR(4),"
"AZ CHAR(3),"
"TKZ CHAR(4),"
"RPVSZ CHAR(3),"
"FDPZ DATE,"
"FVPZ NUMBER(4),"
"RDVZ DATE,"
"RVVZ NUMBER(4),"
"FDVZ DATE,"
"FVVZ NUMBER(4),"
"PZP NUMBER(4),"
"PVV NUMBER(4),"
"PDV DATE,"
"RDV DATE,"
"RVV NUMBER(4),"
"PRZR CHAR(3),"
"FDV DATE,"
"FVV NUMBER(4),"
"PRZK CHAR(3),"
"VPOL NUMBER(4),"
"TP NUMBER(5),"
"DPP DATE,"
"VPP NUMBER(4),"
"VPP1 NUMBER(4),"
"DPR DATE,"
"VPR NUMBER(4),"
"DPF DATE,"
"VPF NUMBER(4),"
"VST NUMBER(4),"
"TPS NUMBER(3),"
"TRRB NUMBER(3),"
"TRM NUMBER(2),"
"TRUK NUMBER(4),"
"TBAG NUMBER(5),"
"TGRU NUMBER(5),"
"TPC NUMBER(5),"
"DPS NUMBER(3),"
"DRB NUMBER(3),"
"DRM NUMBER(3),"
"RUK NUMBER(4),"
"BAG NUMBER(5),"
"PBAG NUMBER(4),"
"GRU NUMBER(5),"
"PC NUMBER(5),"
"PB NUMBER(3),"
"TRB NUMBER(3),"
"PUR NUMBER(2),"
"TAR_P NUMBER(5) ";

const string spp__dk_dbf_fields =
"PER CHAR(7),"
"AR NUMBER(4),"
"PR CHAR(3),"
"BL CHAR(3),"
"PNR CHAR(7),"
"NR CHAR(5),"
"DR CHAR(1),"
"AV CHAR(3),"
"TKV CHAR(4),"
"AP CHAR(3),"
"TKP CHAR(4),"
"AZ CHAR(3),"
"TKZ CHAR(4),"
"RPVSZ CHAR(3),"
"FDPZ DATE,"
"FVPZ NUMBER(4),"
"RDVZ DATE,"
"RVVZ NUMBER(4),"
"FDVZ DATE,"
"FVVZ NUMBER(4),"
"PZP NUMBER(4),"
"PVV NUMBER(4),"
"PDV DATE,"
"RDV DATE,"
"RVV NUMBER(4),"
"PRZR CHAR(3),"
"FDV DATE,"
"FVV NUMBER(4),"
"PRZK CHAR(3),"
"VPOL NUMBER(4),"
"TP NUMBER(5),"
"DPP DATE,"
"VPP NUMBER(4),"
"VPP1 NUMBER(4),"
"DPR DATE,"
"VPR NUMBER(4),"
"DPF DATE,"
"VPF NUMBER(4),"
"VST NUMBER(4),"
"TPS NUMBER(3),"
"TRRB NUMBER(3),"
"TRM NUMBER(2),"
"TRUK NUMBER(4),"
"TBAG NUMBER(5),"
"TPBAG NUMBER(4),"
"TGRU NUMBER(5),"
"TPC NUMBER(5),"
"DPS NUMBER(3),"
"DRB NUMBER(3),"
"DRM NUMBER(3),"
"RUK NUMBER(4),"
"BAG NUMBER(5),"
"PBAG NUMBER(5),"
"GRU NUMBER(5),"
"PC NUMBER(5),"
"PB NUMBER(3),"
"TRB NUMBER(5),"
"PUR NUMBER(2),"
"TAR_P NUMBER(5)";

inline string GetMinutes( TDateTime d1, TDateTime d2 )
{
	if ( d2 > NoExists ) {
    int Hour, Min, Sec, Year, Month, Day;
    TDateTime d = d2-d1;
    if ( d >= 1 )
    	DecodeDate( d, Year, Month, Day );
    else
    	Day = 0;
    DecodeTime( d, Hour, Min, Sec );
    return IntToString( Day*24*60 + Hour*60 + Min );
	}
	else
		return "0";
}

inline string GetStrDate( TDateTime d1 )
{
  if ( d1 > NoExists )
    return DateTimeToStr( d1, "yyyy-mm-dd" );
 	else
 		return "";
}

bool createSPPCEK( TDateTime sppdate, const string &file_type, const string &point_addr, TFileDatas &fds )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
	ProgTrace( TRACE5, "sppdate=%s, file_type=%s, point_addr=%s", DateTimeToStr( sppdate, "dd.mm.yy" ).c_str(), file_type.c_str(), point_addr.c_str() );
	TQuery Qry( &OraSession );
	Qry.SQLText =
	"SELECT * FROM snapshot_params "
	" WHERE name=:sppdate AND file_type=:file_type AND point_addr=:point_addr";
	Qry.CreateVariable( "sppdate", otString, DateTimeToStr( sppdate, "SPPdd.mm.yy" ) );
	Qry.CreateVariable( "file_type", otString, file_type );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.Execute();
	if ( !Qry.Eof )
		return false;
	TFileData fd;
	string sql_str;
	xmlDocPtr doc = CreateXMLDoc( "UTF-8", "sqls" );	
	try {
	  xmlNodePtr queryNode = NewTextChild( doc->children, "query" );
  //  file_data = string("DELETE TABLE SPP") + DateTimeToStr( sppdate, "dd" ) + "A";
  //  file_data += VALUE_END_SQL;
    sql_str = string("CREATE TABLE SPP") + DateTimeToStr( sppdate, "dd" ) + "A(";
    sql_str += spp__a_dbf_fields + ")";
    xmlNodePtr sqlNode = NewTextChild( queryNode, "sql", sql_str );  
  //  file_data += string("DELETE TABLE SPP") + DateTimeToStr( sppdate, "dd" ) + "D";
  //  file_data += VALUE_END_SQL;
	  queryNode = NewTextChild( doc->children, "query" );
    sql_str = string("CREATE TABLE SPP") + DateTimeToStr( sppdate, "dd" ) + "D(";
    sql_str += spp__d_dbf_fields + ")";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );    
  //  file_data += string("DELETE TABLE SPP") + DateTimeToStr( sppdate, "dd" ) + "AK";
  //  file_data += VALUE_END_SQL;
    queryNode = NewTextChild( doc->children, "query" );
    sql_str = string("CREATE TABLE SPP") + DateTimeToStr( sppdate, "dd" ) + "AK(";
    sql_str += spp__ak_dbf_fields + ")";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );      
	  queryNode = NewTextChild( doc->children, "query" );  
    sql_str = string("CREATE INDEX SPP") + DateTimeToStr( sppdate, "dd" ) + "AK ON ";
    sql_str += string("SPP") + DateTimeToStr( sppdate, "dd" ) + "AK(PNR+STR(PUR,2))";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );      
	  queryNode = NewTextChild( doc->children, "query" );    
    sql_str = string("CREATE INDEX SPP") + DateTimeToStr( sppdate, "dd" ) + "AK1 ON ";
    sql_str += string("SPP") + DateTimeToStr( sppdate, "dd" ) + "AK(PNR+STR(PUR,2))"; //DESCEND(STR(PUR,2))
    sqlNode = NewTextChild( queryNode, "sql", sql_str );      
	  queryNode = NewTextChild( doc->children, "query" );      
  //  file_data += string("DELETE TABLE SPP") + DateTimeToStr( sppdate, "dd" ) + "DK";
  //  file_data += VALUE_END_SQL;
    sql_str = string("CREATE TABLE SPP") + DateTimeToStr( sppdate, "dd" ) + "DK(";
    sql_str += spp__dk_dbf_fields + ")";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );      
	  queryNode = NewTextChild( doc->children, "query" );      
    sql_str = string("CREATE INDEX SPP") + DateTimeToStr( sppdate, "dd" ) + "DK ON ";
    sql_str += string("SPP") + DateTimeToStr( sppdate, "dd" ) + "DK(PNR+STR(PUR,2))";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );      
	  queryNode = NewTextChild( doc->children, "query" );      
    sql_str = "INSERT INTO SPPCIKL(DADP,DFSPP,VFSPP,DSPP) VALUES( '" +
               reqInfo->desk.city + "',{d'" + DateTimeToStr( reqInfo->desk.time, "yyyy.mm.dd" ) +
               "'},'" + DateTimeToStr( reqInfo->desk.time, "hh:nn" ) + "',{d'" + DateTimeToStr( sppdate, "yyyy.mm.dd" ) + "'})";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );      
	  if ( doc ) {
	  	string encoding = getFileEncoding( FILE_SPPCEK_TYPE, point_addr );
  	  fd.params[ PARAM_FILE_NAME ] =  "SPP" + DateTimeToStr( sppdate, "dd" ) + "A.DBF";
   		fd.params[ PARAM_TYPE ] = VALUE_TYPE_SQL; // SQL
    	string s = XMLTreeToText( doc );
    	s.replace( s.find( "encoding=\"UTF-8\""), string( "encoding=\"UTF-8\"" ).size(), string("encoding=\"") + encoding + "\"" );
    	fd.file_data = s;
    	fds.push_back( fd );
    	xmlFreeDoc( doc );
	  }
	}
	catch ( ... ) {
		xmlFreeDoc( doc );
		throw;
	}  
  Qry.Clear();
  Qry.SQLText =
  "INSERT INTO snapshot_params(file_type,point_addr,name,value) VALUES(:file_type,:point_addr,:sppdate,'SPP')";
	Qry.CreateVariable( "sppdate", otString, DateTimeToStr( sppdate, "SPPdd.mm.yy" ) );
	Qry.CreateVariable( "file_type", otString, file_type );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.Execute();
	return !fds.empty();
}

void createParam( xmlNodePtr paramsNode, const string &name, const string &value, const string &type )
{
  	xmlNodePtr Nparam = NewTextChild( paramsNode, "param" );
  	NewTextChild( Nparam, "name", name );
  	NewTextChild( Nparam, "value", value );
  	NewTextChild( Nparam, "type", type );
}

xmlDocPtr createXMLTrip( TSOPPTrips::iterator tr, xmlDocPtr &doc )
{
	TDateTime tm;
	Luggage lug_in, lug_out;  
  int cargo_in=0, cargo_out=0, mail_in=0, mail_out=0;	
  lug_in.max_commerce = 0; lug_out.max_commerce = 0;
	if ( !doc )
	  doc = CreateXMLDoc( "UTF-8", "xmltrip" );		
	xmlNodePtr tripNode = NewTextChild( doc->children, "trip" );
	NewTextChild( tripNode, "point_id", tr->point_id );
	if ( !tr->places_in.empty() ) {
		xmlNodePtr NodeA = NewTextChild( tripNode, "A" );
  	NewTextChild( NodeA, "PNR", tr->airline_in + IntToString( tr->flt_no_in ) + tr->suffix_in ); 	
  	NewTextChild( NodeA, "DN", GetStrDate( tr->scd_in ) );
  	NewTextChild( NodeA, "KUG", tr->airline_in );
  	NewTextChild( NodeA, "TVC", tr->craft_in );
    if ( tr->pr_del_in == 1 )
    	NewTextChild( NodeA, "BNP", "ОТМЕН" );
    else
    	NewTextChild( NodeA, "BNP", tr->bort_in );
    NewTextChild( NodeA, "NMSF", tr->park_in );
    NewTextChild( NodeA, "RPVSN", GetMinutes( tr->scd_in, tr->est_in ) );
 	  if ( tr->airline_in + IntToString( tr->flt_no_in ) + tr->suffix_in == tr->airline_out + IntToString( tr->flt_no_out ) + tr->suffix_out )
 	    NewTextChild( NodeA, "PRIZ", IntToString( 2 ) );
 	  else 
 	  	NewTextChild( NodeA, "PRIZ", IntToString( 1 ) );
    NewTextChild( NodeA, "KUR", IntToString( tr->places_in.size() ) );
    if ( tr->triptype_in == "м" )
    	NewTextChild( NodeA, "VDV", "МЕЖ" );
    else
      NewTextChild( NodeA, "VDV", "ПАСС" );    
    int k = 0;
    int prior_point_id = ASTRA::NoExists;
    for ( TSOPPDests::iterator d=tr->places_in.begin(); d!= tr->places_in.end(); d++,k++ ) {
  	  if ( d->pr_del == 0 )
  			prior_point_id = d->point_id;    	 
      xmlNodePtr NodeAK = NewTextChild( NodeA, "AK" );  			 			
    	NewTextChild( NodeAK, "PNR", tr->airline_in + IntToString( tr->flt_no_in ) + tr->suffix_in );
    	NewTextChild( NodeAK, "AV", d->airp );
     /* аэропорт прилета(AP), плановая дата прилета(DPP), плановое время прилета(VPP),
        расчетная дата прилета(DPR), расчетное время прилета(VPR), фактическая дата прилета(DPF),
        фактическое время прилета(VPF), плановя дата вылета(PDV), плановое время вылета(PVV),
        расчетная дата вылета(RDV), расчетное время вылета(RVV), фактическая дата вылета(FDV),
        фактическое время вылета(FVV), номер участка(PUR), признак...???(PR), почта по плеча(PC),
        транзитная почта по плеча(TPC), груз по плеча(GRU), транзитный груз по плечам(TGRU)
      */
      TSOPPDests::iterator n = d;
      n++;
      if ( n != tr->places_in.end() ) {
      	NewTextChild( NodeAK, "AP", n->airp );
        NewTextChild( NodeAK, "DPP", GetStrDate( n->scd_in ) );
        modf( n->scd_in, &tm );
        NewTextChild( NodeAK, "VPP", GetMinutes( tm, n->scd_in ) );
        NewTextChild( NodeAK, "DPR", GetStrDate( n->est_in ) );
       	modf( n->est_in, &tm );
       	NewTextChild( NodeAK, "VPR", GetMinutes( tm, n->est_in ) );
       	NewTextChild( NodeAK, "DPF", GetStrDate( n->act_in ) );
       	modf( n->act_in, &tm );
       	NewTextChild( NodeAK, "VPF", GetMinutes( tm, n->act_in ) );

        NewTextChild( NodeAK, "PDV", GetStrDate( d->scd_out ) );
       	modf( d->scd_out, &tm );
       	NewTextChild( NodeAK, "PVV", GetMinutes( tm, d->scd_out ) );
       	NewTextChild( NodeAK, "RDV", GetStrDate( d->est_out ) );
       	modf( d->est_out, &tm );
       	NewTextChild( NodeAK, "RVV", GetMinutes( tm, d->est_out ) );
       	NewTextChild( NodeAK, "FDV", GetStrDate( d->act_out ) );
       	modf( d->act_out, &tm );
      	NewTextChild( NodeAK, "FVV", GetMinutes( tm, d->act_out ) );
      }
      else {
      	NewTextChild( NodeAK, "AP", tr->airp );
      	NewTextChild( NodeAK, "DPP", GetStrDate( tr->scd_in ) );
       	modf( tr->scd_in, &tm );
       	NewTextChild( NodeAK, "VPP", GetMinutes( tm, tr->scd_in ) );
       	NewTextChild( NodeAK, "DPR", GetStrDate( tr->est_in ) );
       	modf( tr->est_in, &tm );
       	NewTextChild( NodeAK, "VPR", GetMinutes( tm, tr->est_in ) );
       	NewTextChild( NodeAK, "DPF", GetStrDate( tr->act_in ) );
       	modf( tr->act_in, &tm );
       	NewTextChild( NodeAK, "VPF", GetMinutes( tm, tr->act_in ) );
       	
       	NewTextChild( NodeAK, "PDV", GetStrDate( d->scd_out ) );
        modf( d->scd_out, &tm );
        NewTextChild( NodeAK, "PVV", GetMinutes( tm, d->scd_out ) );
        NewTextChild( NodeAK, "RDV", GetStrDate( d->est_out ) );
       	modf( d->est_out, &tm );
       	NewTextChild( NodeAK, "RVV", GetMinutes( tm, d->est_out ) );
       	NewTextChild( NodeAK, "FDV", GetStrDate( d->act_out ) );
       	modf( d->act_out, &tm );
       	NewTextChild( NodeAK, "FVV", GetMinutes( tm, d->act_out ) );
      }
      NewTextChild( NodeAK, "PUR", IntToString( k + 1 ) );
      NewTextChild( NodeAK, "PR", IntToString( 0 ) );
      NewTextChild( NodeAK, "PC", IntToString( 0 ) );
      NewTextChild( NodeAK, "TPC", IntToString( 0 ) );
      NewTextChild( NodeAK, "GRU", IntToString( 0 ) );
      NewTextChild( NodeAK, "TGRU", IntToString( 0 ) );
    };      
		if ( prior_point_id > ASTRA::NoExists ) {
 			GetLuggage( prior_point_id, lug_in );
 		}
 		else lug_in.max_commerce = 0;
    for ( vector<Cargo>::iterator wc=lug_in.vcargo.begin(); wc!=lug_in.vcargo.end(); wc++ ) {
    	if ( wc->point_arv == tr->point_id ) {
     	  cargo_in += wc->cargo;
       	mail_in += wc->mail;
       }
     }    
    NewTextChild( NodeA, "PKZ", IntToString( lug_in.max_commerce ) );
    NewTextChild( NodeA, "F9", IntToString( cargo_in ) );    
    NewTextChild( NodeA, "F11", IntToString( mail_in ) );
	} // end if !place_in.empty()	
	if ( !tr->places_out.empty() ) {
  	GetLuggage( tr->point_id, lug_out );
		xmlNodePtr NodeD = NewTextChild( tripNode, "D" );
		NewTextChild( NodeD, "PNR", tr->airline_out + IntToString( tr->flt_no_out ) + tr->suffix_out );
		NewTextChild( NodeD, "DN", GetStrDate( tr->scd_out ) );
  	NewTextChild( NodeD, "KUG", tr->airline_out );
  	NewTextChild( NodeD, "TVC", tr->craft_out );
    if ( tr->pr_del_out == 1 )
    	NewTextChild( NodeD, "BNP", "ОТМЕН" );
    else
 	  	NewTextChild( NodeD, "BNP", tr->bort_out );
 	  NewTextChild( NodeD, "NMSF", tr->park_out );
 	  NewTextChild( NodeD, "RPVSN", GetMinutes( tr->scd_out, tr->est_out ) );
 	  if ( tr->airline_in + IntToString( tr->flt_no_in ) + tr->suffix_in == tr->airline_out + IntToString( tr->flt_no_out ) + tr->suffix_out )
 	    NewTextChild( NodeD, "PRIZ", IntToString( 2 ) );
 	  else 
 	  	NewTextChild( NodeD, "PRIZ", IntToString( 1 ) );
    if ( tr->triptype_out == "м" )
    	NewTextChild( NodeD, "VDV", "МЕЖ" );
    else
    	NewTextChild( NodeD, "VDV", "ПАСС" );
    // теперь создание записей по плечам        
    int k = 0;
    for ( TSOPPDests::iterator d=tr->places_out.begin(); d!= tr->places_out.end(); d++, k++ ) {
      vector<Cargo>::iterator c=lug_out.vcargo.end();            	
      xmlNodePtr NodeDK = NewTextChild( NodeD, "DK" );
      NewTextChild( NodeDK, "PNR", tr->airline_out + IntToString( tr->flt_no_out ) + tr->suffix_out );
      NewTextChild( NodeDK, "AP", d->airp );
      /* аэропорт прилета(AP), плановая дата прилета(DPP), плановое время прилета(VPP),
         расчетная дата прилета(DPR), расчетное время прилета(VPR), фактическая дата прилета(DPF),
         фактическое время прилета(VPF), плановя дата вылета(PDV), плановое время вылета(PVV),
         расчетная дата вылета(RDV), расчетное время вылета(RVV), фактическая дата вылета(FDV),
         фактическое время вылета(FVV), номер участка(PUR), признак...???(PR), почта по плеча(PC),
         транзитная почта по плеча(TPC), груз по плеча(GRU), транзитный груз по плечам(TGRU)
      */
      TDateTime tm;
      if ( k > 0 ) {
      	TSOPPDests::iterator n = d - 1;
      	NewTextChild( NodeDK, "AV", n->airp );
      	NewTextChild( NodeDK, "DPP", GetStrDate( d->scd_in ) );
       	modf( d->scd_in, &tm );
      	NewTextChild( NodeDK, "VPP", GetMinutes( tm, d->scd_in ) );
      	NewTextChild( NodeDK, "DPR", GetStrDate( d->est_in ) );
       	modf( d->est_in, &tm );
        NewTextChild( NodeDK, "VPR", GetMinutes( tm, d->est_in ) );
        NewTextChild( NodeDK, "DPF", GetStrDate( d->act_in ) );
       	modf( d->act_in, &tm );
        NewTextChild( NodeDK, "VPF", GetMinutes( tm, d->act_in ) );

        NewTextChild( NodeDK, "PDV", GetStrDate( n->scd_out ) );
       	modf( n->scd_out, &tm );
        NewTextChild( NodeDK, "PVV", GetMinutes( tm, n->scd_out ) );
        NewTextChild( NodeDK, "RDV", GetStrDate( n->est_out ) );
       	modf( n->est_out, &tm );
        NewTextChild( NodeDK, "RVV", GetMinutes( tm, n->est_out ) );
        NewTextChild( NodeDK, "FDV", GetStrDate( n->act_out ) );
       	modf( n->act_out, &tm );
        NewTextChild( NodeDK, "FVV", GetMinutes( tm, n->act_out ) );
        for ( c=lug_out.vcargo.begin(); c!=lug_out.vcargo.end(); c++ ) {
   	      if ( c->point_arv == n->point_id ) //???
     	      break;
        }                    	
       	d++;
      }
      else {
        NewTextChild( NodeDK, "AV", tr->airp );
        NewTextChild( NodeDK, "DPP", GetStrDate( d->scd_in ) );
       	modf( d->scd_in, &tm );
       	NewTextChild( NodeDK, "VPP", GetMinutes( tm, d->scd_in ) );
       	NewTextChild( NodeDK, "DPR", GetStrDate( d->est_in ) );
       	modf( d->est_in, &tm );
       	NewTextChild( NodeDK, "VPR", GetMinutes( tm, d->est_in ) );
       	NewTextChild( NodeDK, "DPF", GetStrDate( d->act_in ) );
       	modf( d->act_in, &tm );          	
       	NewTextChild( NodeDK, "VPF", GetMinutes( tm, d->act_in ) );

        NewTextChild( NodeDK, "PDV", GetStrDate( tr->scd_out ) );
       	modf( tr->scd_out, &tm );
       	NewTextChild( NodeDK, "PVV", GetMinutes( tm, tr->scd_out ) );
       	NewTextChild( NodeDK, "RDV", GetStrDate( tr->est_out ) );
       	modf( tr->est_out, &tm );
       	NewTextChild( NodeDK, "RVV", GetMinutes( tm, tr->est_out ) );
       	NewTextChild( NodeDK, "FDV", GetStrDate( tr->act_out ) );
       	modf( tr->act_out, &tm );
       	NewTextChild( NodeDK, "FVV", GetMinutes( tm, tr->act_out ) );
        for ( c=lug_in.vcargo.begin(); c!=lug_in.vcargo.end(); c++ ) {
  	      if ( c->point_arv == tr->point_id )
     	      break;
        }                    	
      }
      NewTextChild( NodeDK, "PUR", IntToString( k + 1 ) );
      NewTextChild( NodeDK, "PR", IntToString( 0 ) );
      if ( c!=lug_out.vcargo.end() && c!=lug_in.vcargo.end() ) {
      	NewTextChild( NodeDK, "PC", IntToString( c->mail ) );
      	NewTextChild( NodeDK, "GRU", IntToString( c->cargo ) );
      }
      else {
      	NewTextChild( NodeDK, "PC", IntToString( 0 ) );
      	NewTextChild( NodeDK, "GRU", IntToString( 0 ) );
      }
      NewTextChild( NodeDK, "TPC", IntToString( 0 ) );
      NewTextChild( NodeDK, "TGRU", IntToString( 0 ) );
    }; // end for
    for ( vector<Cargo>::iterator wc=lug_out.vcargo.begin(); wc!=lug_out.vcargo.end(); wc++ ) {
    	cargo_out += wc->cargo;
    	mail_out += wc->mail;
    }  				    
 	  NewTextChild( NodeD, "PKZ", IntToString( lug_out.max_commerce ) );
 	  NewTextChild( NodeD, "F9", IntToString( cargo_out ) );
 	  NewTextChild( NodeD, "F11", IntToString( mail_out ) );
 	  NewTextChild( NodeD, "KUR", IntToString( tr->places_out.size() ) );    
	}   	
	return doc;  
}

string GetXMLRow( xmlNodePtr node )
{
	string res;
	node = node->children;
	while ( node ) {
		res += NodeAsString( node );
		node = node->next;
	}
	return res;
}

void createDBF( xmlDocPtr &sqldoc, xmlDocPtr old_doc, xmlDocPtr doc, const string &day, bool pr_land )
{
  xmlNodePtr nodeP, nodeN;
  string sql_str;  
  xmlNodePtr paramsNode;
  xmlNodePtr queryNode, sqlNode;
  string dbf_type;
  if ( pr_land )
  	dbf_type = "A";
  else
  	dbf_type = "D";
  if ( old_doc )
    nodeP = GetNode( (char*)dbf_type.c_str(), old_doc, old_doc->children->children );
  else
  	nodeP = 0;
  if ( doc )
    nodeN = GetNode( (char*)dbf_type.c_str(), doc, doc->children->children );
  else
  	nodeN = 0;
  bool pr_insert = ( !nodeP && nodeN ||
                     nodeP && nodeN && 
                     string(NodeAsString( "PNR", nodeP )) + NodeAsString( "DN", nodeP ) != 
                     string(NodeAsString( "PNR", nodeN )) + NodeAsString( "DN", nodeN ) );
  bool pr_delete = ( nodeP && !nodeN || 
                     nodeP && nodeN && 
                     string(NodeAsString( "PNR", nodeP )) + NodeAsString( "DN", nodeP ) != 
                     string(NodeAsString( "PNR", nodeN )) + NodeAsString( "DN", nodeN ) );
  bool pr_update = false; 
  if ( nodeP && nodeN && !pr_delete ) {
   string old_val = GetXMLRow( nodeP );
   string new_val = GetXMLRow( nodeN );
   pr_update = ( old_val != new_val );
  }
  ProgTrace( TRACE5, "pr_insert=%d, pr_delete=%d, pr_update=%d", pr_insert, pr_delete, pr_update );
  
  if ( pr_insert || pr_update || pr_delete ) {
  	if ( !sqldoc )
  	  sqldoc = CreateXMLDoc( "UTF-8", "sqls" );
  }
  
  if ( pr_delete ) {
  	// delete прилет
   	sql_str =
      string("DELETE FROM SPP") + day + dbf_type +
      " WHERE PNR=:PNR AND DN=:DN";    	  	
 	  queryNode = NewTextChild( sqldoc->children, "query" );
 	  sqlNode = NewTextChild( queryNode, "sql", sql_str );
 	  paramsNode = NewTextChild( queryNode, "params" );      
 	  createParam( paramsNode, "PNR", NodeAsString( "PNR", nodeP ), DBF_TYPE_CHAR );
 	  createParam( paramsNode, "DN", NodeAsString( "DN", nodeP ), DBF_TYPE_DATE );  	 	  
  };
 	if ( pr_insert )  {
		//insert прилет
    sql_str =
    string("INSERT INTO SPP") + day + dbf_type +
    "(PNR,KUG,TVC,BNP,NMSF,RPVSN,DN,PRIZ,PKZ,F9,F11,KUR,VDV) VALUES"
    "(:PNR,:KUG,:TVC,:BNP,:NMSF,:RPVSN,:DN,:PRIZ,:PKZ,:F9,:F11,:KUR,:VDV)";  		
  };
  if ( pr_update ) {
		// update if change
		sql_str =
      string("UPDATE SPP") + day + dbf_type +
      " SET KUG=:KUG,TVC=:TVC,BNP=:BNP,NMSF=:NMSF,RPVSN=:RPVSN,"
      "    PRIZ=:PRIZ,PKZ=:PKZ,F9=:F9,F11=:F11,KUR=:KUR,VDV=:VDV "
      " WHERE PNR=:PNR AND DN=:DN";    	  		
  };
  if ( pr_insert || pr_update ) {
    queryNode = NewTextChild( sqldoc->children, "query" );
    sqlNode = NewTextChild( queryNode, "sql", sql_str );
    paramsNode = NewTextChild( queryNode, "params" );      
    if ( pr_update ) {
 	    createParam( paramsNode, "PNR", NodeAsString( "PNR", nodeP ), DBF_TYPE_CHAR );
 	    createParam( paramsNode, "DN", NodeAsString( "DN", nodeP ), DBF_TYPE_DATE );  	    
 	  }
 	  else {
 	    createParam( paramsNode, "PNR", NodeAsString( "PNR", nodeN ), DBF_TYPE_CHAR );
 	    createParam( paramsNode, "DN", NodeAsString( "DN", nodeN ), DBF_TYPE_DATE );  	     	  	
 	  }
    createParam( paramsNode, "PNR", NodeAsString( "PNR", nodeN ), DBF_TYPE_CHAR );
    createParam( paramsNode, "DN", NodeAsString( "DN", nodeN ), DBF_TYPE_DATE );  		
	  createParam( paramsNode, "KUG", NodeAsString( "KUG", nodeN ), DBF_TYPE_CHAR );
    createParam( paramsNode, "TVC", NodeAsString( "TVC", nodeN ), DBF_TYPE_CHAR );
   	createParam( paramsNode, "BNP", NodeAsString( "BNP", nodeN ), DBF_TYPE_CHAR );
    createParam( paramsNode, "NMSF", NodeAsString( "NMSF", nodeN ), DBF_TYPE_CHAR );
    createParam( paramsNode, "RPVSN", NodeAsString( "RPVSN", nodeN ), DBF_TYPE_CHAR );
   	createParam( paramsNode, "PRIZ", NodeAsString( "PRIZ", nodeN ), DBF_TYPE_NUMBER );
    createParam( paramsNode, "PKZ", NodeAsString( "PKZ", nodeN ), DBF_TYPE_NUMBER );
    createParam( paramsNode, "F9", NodeAsString( "F9", nodeN ),  DBF_TYPE_NUMBER );
    createParam( paramsNode, "F11", NodeAsString( "F11", nodeN ), DBF_TYPE_NUMBER );
    createParam( paramsNode, "KUR", NodeAsString( "KUR", nodeN ), DBF_TYPE_NUMBER );
   	createParam( paramsNode, "VDV", NodeAsString( "VDV", nodeN ), DBF_TYPE_CHAR );
  }
  if ( pr_land )
  	dbf_type = "AK";
  else
  	dbf_type = "DK";  
  // маршрут на прилет
  vector<xmlNodePtr>::iterator nodePK, nodeNK;
  vector<xmlNodePtr> nodesPK, nodesNK;
  if ( nodeP )
    GetNodes( (char*)dbf_type.c_str(), nodesPK, nodeP );
  if ( nodeN )
    GetNodes( (char*)dbf_type.c_str(), nodesNK, nodeN );
  nodePK = nodesPK.begin();
  nodeNK = nodesNK.begin();
  while ( nodePK != nodesPK.end() || nodeNK != nodesNK.end() ) {
    pr_insert = ( nodePK == nodesPK.end() && nodeNK !=nodesNK.end() ||
                  nodePK != nodesPK.end() && nodeNK != nodesNK.end() && 
                  string(NodeAsString( "PNR", *nodePK )) + NodeAsString( "DPP", *nodePK ) != 
                  string(NodeAsString( "PNR", *nodeNK )) + NodeAsString( "DPP", *nodeNK ) );
    pr_delete = ( nodePK != nodesPK.end() && nodeNK == nodesNK.end() ||
                  nodePK != nodesPK.end() && nodeNK != nodesNK.end() &&  
                  string(NodeAsString( "PNR", *nodePK )) + NodeAsString( "DPP", *nodePK ) != 
                  string(NodeAsString( "PNR", *nodeNK )) + NodeAsString( "DPP", *nodeNK ) );
    pr_update = false; 
    if ( nodePK != nodesPK.end() && nodeNK != nodesNK.end() && !pr_delete ) {
      string old_val = GetXMLRow( *nodePK );
      string new_val = GetXMLRow( *nodeNK );
      pr_update = ( old_val != new_val );
    }
    if ( pr_insert || pr_update || pr_delete ) {
  	  if ( !sqldoc )
  	    sqldoc = CreateXMLDoc( "UTF-8", "sqls" );
    }    
    if ( pr_delete ) {
    	// delete прилет
     	sql_str =
        string("DELETE FROM SPP") + day + dbf_type +
        " WHERE PNR=:PNR AND DPP=:DPP AND PUR=:PUR";    	  	
 	    queryNode = NewTextChild( sqldoc->children, "query" );
 	    sqlNode = NewTextChild( queryNode, "sql", sql_str );
 	    paramsNode = NewTextChild( queryNode, "params" );      
 	    createParam( paramsNode, "PNR", NodeAsString( "PNR", *nodePK ), DBF_TYPE_CHAR );
 	    createParam( paramsNode, "DPP", NodeAsString( "DPP", *nodePK ), DBF_TYPE_DATE );
      createParam( paramsNode, "PUR", NodeAsString( "PUR", *nodePK ), DBF_TYPE_NUMBER ); 	    
    }
    if ( pr_insert ) {
      sql_str = 
        string("INSERT INTO SPP") + day + dbf_type +
        "(PNR,AV,AP,DPP,VPP,DPR,VPR,DPF,VPF,PDV,PVV,RDV,RVV,FDV,FVV,PUR,PR,PC,TPC,GRU,TGRU) "
        " VALUES(:PNR,:AV,:AP,:DPP,:VPP,:DPR,:VPR,:DPF,:VPF,:PDV,:PVV,:RDV,:RVV,:FDV,:FVV,:PUR,:PR,:PC,:TPC,:GRU,:TGRU)";   	
    }
    if ( pr_update ) {
      sql_str =
        string("UPDATE SPP") + day + dbf_type +
        " SET "
        "AV=:AV,AP=:AP,DPP=:DPP,VPP=:VPP,DPR=:DPR,VPR=:VPR,DPF=:DPF,VPF=:VPF,PDV=:PDV,PVV=:PVV,RDV=:RDV,"
        "RVV=:RVV,FDV=:FDV,FVV=:FVV,PUR=:PUR,PR=:PR,PC=:PC,TPC=:TPC,GRU=:GRU,TGRU=:TGRU "
        " WHERE PNR=:PNR AND DPP=:DPP AND PUR=:PUR";    	  		        
    }
    if ( pr_insert || pr_update ) {
      queryNode = NewTextChild( sqldoc->children, "query" );
      sqlNode = NewTextChild( queryNode, "sql", sql_str );
      paramsNode = NewTextChild( queryNode, "params" );          	
      if ( pr_update ) {
 	      createParam( paramsNode, "PNR", NodeAsString( "PNR", *nodePK ), DBF_TYPE_CHAR );
 	      createParam( paramsNode, "DPP", NodeAsString( "DPP", *nodePK ), DBF_TYPE_DATE );    	
 	      createParam( paramsNode, "PUR", NodeAsString( "PUR", *nodePK ), DBF_TYPE_NUMBER ); 	    
 	    }
 	    else {
 	      createParam( paramsNode, "PNR", NodeAsString( "PNR", *nodeNK ), DBF_TYPE_CHAR );
 	      createParam( paramsNode, "DPP", NodeAsString( "DPP", *nodeNK ), DBF_TYPE_DATE );    	 	
 	      createParam( paramsNode, "PUR", NodeAsString( "PUR", *nodeNK ), DBF_TYPE_NUMBER ); 	        	
 	    }
      createParam( paramsNode, "AV", NodeAsString( "AV", *nodeNK ), DBF_TYPE_CHAR );
      createParam( paramsNode, "AP", NodeAsString( "AP", *nodeNK ), DBF_TYPE_CHAR );
     	createParam( paramsNode, "VPP", NodeAsString( "VPP", *nodeNK ), DBF_TYPE_NUMBER );
     	createParam( paramsNode, "DPR", NodeAsString( "DPR", *nodeNK ), DBF_TYPE_DATE );
     	createParam( paramsNode, "VPR", NodeAsString( "VPR", *nodeNK ), DBF_TYPE_NUMBER );
     	createParam( paramsNode, "DPF", NodeAsString( "DPF", *nodeNK ), DBF_TYPE_DATE );
     	createParam( paramsNode, "VPF", NodeAsString( "VPF", *nodeNK ), DBF_TYPE_NUMBER );
          	
     	createParam( paramsNode, "PDV", NodeAsString( "PDV", *nodeNK ), DBF_TYPE_DATE );
     	createParam( paramsNode, "PVV", NodeAsString( "PVV", *nodeNK ), DBF_TYPE_NUMBER );
     	createParam( paramsNode, "RDV", NodeAsString( "RDV", *nodeNK ), DBF_TYPE_DATE );
     	createParam( paramsNode, "RVV", NodeAsString( "RVV", *nodeNK ), DBF_TYPE_NUMBER );
     	createParam( paramsNode, "FDV", NodeAsString( "FDV", *nodeNK ), DBF_TYPE_DATE );
     	createParam( paramsNode, "FVV", NodeAsString( "FVV", *nodeNK ), DBF_TYPE_NUMBER );
      createParam( paramsNode, "PR", NodeAsString( "PR", *nodeNK ), DBF_TYPE_CHAR );
      createParam( paramsNode, "PC", NodeAsString( "PC", *nodeNK ), DBF_TYPE_NUMBER );
      createParam( paramsNode, "TPC", NodeAsString( "TPC", *nodeNK ), DBF_TYPE_NUMBER );
      createParam( paramsNode, "GRU", NodeAsString( "GRU", *nodeNK ), DBF_TYPE_NUMBER );
      createParam( paramsNode, "TGRU", NodeAsString( "TGRU", *nodeNK ), DBF_TYPE_NUMBER );
    }
    
    if ( nodePK != nodesPK.end() )
  		nodePK++;
  	if ( nodeNK != nodesNK.end() )
  		nodeNK++;
  } // end while 
}

bool createSPPCEKFile( int point_id, const string &point_addr, TFileDatas &fds )
{
	ProgTrace( TRACE5, "CEK point_id=%d", point_id );
	TReqInfo *reqInfo = TReqInfo::Instance();
	reqInfo->Initialize("ЧЛБ");
  reqInfo->user.sets.time = ustTimeUTC;
  reqInfo->user.user_type = utAirport;
  reqInfo->user.access.airps.push_back( "ЧЛБ" );
  reqInfo->user.access.airps_permit = true;

	string file_type = FILE_SPPCEK_TYPE;
	string record;
	TQuery Qry( &OraSession );
	Qry.Clear();
	Qry.SQLText = "SELECT TRUNC(system.UTCSYSDATE) d FROM dual";
 	Qry.Execute();
 	/* проверка на существование таблиц */
 	for ( int max_day=0; max_day<=CREATE_SPP_DAYS(); max_day++ ) {
	  createSPPCEK( UTCToLocal( Qry.FieldAsDateTime( "d" ), reqInfo->desk.tz_region ) + max_day, file_type, point_addr, fds );
	}
	TFileData fd;
	Qry.Clear();
	Qry.SQLText =
	 "SELECT record FROM points, snapshot_points "
	 " WHERE points.point_id=:point_id AND "
	 "       points.point_id=snapshot_points.point_id AND "
	 "       snapshot_points.point_addr=:point_addr AND "
	 "       snapshot_points.file_type=:file_type ";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.CreateVariable( "file_type", otString, file_type );
	Qry.Execute();
	// в полу Record есть инфа на прилет и вылет, надо ее разделить по 2-м переменным in_record, out_record
	if ( !Qry.Eof ) {
		record = Qry.FieldAsString( "record" );
	}
  TSOPPTrips trips;
  createSOPPTrip( point_id, trips );
  TSOPPTrips::iterator tr = trips.end();
  xmlDocPtr doc = 0, old_doc = 0;
  for ( tr=trips.begin(); tr!=trips.end(); tr++ ) {
  	if ( tr->point_id != point_id )
  		continue;
  	createXMLTrip( tr, doc );
  	break;  	
  }
  if ( !doc )
  	return false;
  if ( XMLTreeToText( doc ) == record )
  	return false;
  // есть изменения
  if ( !record.empty() ) {
 		record.replace( record.find( "encoding=\"UTF-8\""), string( "encoding=\"UTF-8\"" ).size(), string("encoding=\"") + "CP866" + "\"" );
    old_doc = TextToXMLTree( record );
	  if ( old_doc ) {
      xmlFree(const_cast<xmlChar *>(old_doc->encoding));
      xml_decode_nodelist( old_doc->children );
		}
  }
    
  xmlDocPtr sqldoc = 0;
  if ( tr->scd_in > NoExists )
    createDBF( sqldoc, old_doc, doc, DateTimeToStr( tr->scd_in, "dd" ), true ); // на прилет  
  if ( tr->scd_out > NoExists )  
    createDBF( sqldoc, old_doc, doc, DateTimeToStr( tr->scd_out, "dd" ), false ); // на вылет
  
  string sres;
  if ( sqldoc ) { // CP-866
  	string encoding = getFileEncoding( FILE_SPPCEK_TYPE, point_addr );
  	if ( encoding.empty() )
  		encoding = "CP866";
  	fd.params[ PARAM_TYPE ] = VALUE_TYPE_SQL; // SQL  	  	  	
  	sres = XMLTreeToText( sqldoc );
  	sres.replace( sres.find( "encoding=\"UTF-8\""), string( "encoding=\"UTF-8\"" ).size(), string("encoding=\"") + encoding + "\"" );
  	fd.file_data = sres;
  	ProgTrace( TRACE5, "fd.file_data=%s", fd.file_data.c_str() );
  	fds.push_back( fd );
  	xmlFreeDoc( sqldoc );
  }
  //CREATE INSERT UPDATE DELETE Querys
  Qry.Clear();
  if ( !old_doc && doc ) { // новая запись
    ProgTrace( TRACE5, "insert snapshot_points" );
    Qry.SQLText =
      "INSERT INTO snapshot_points(point_id,file_type,point_addr,record ) "
      "                VALUES(:point_id,:file_type,:point_addr,:record) ";
  }
  else
    if ( old_doc && doc ) {  // изменения
    	ProgTrace( TRACE5, "update snapshot_points" );
      Qry.SQLText =
        "UPDATE snapshot_points SET record=:record "
        " WHERE point_id=:point_id AND file_type=:file_type AND point_addr=:point_addr ";
    }
    else 
    	if ( old_doc && !doc ) { // удаление
      	ProgTrace( TRACE5, "delete snapshot_points" );
      	Qry.SQLText =
      	  "DELETE snapshot_points "
      	  " WHERE point_id=:point_id AND file_type=:file_type AND point_addr=:point_addr AND :record=:record";
      }
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "file_type", otString, FILE_SPPCEK_TYPE );
  Qry.CreateVariable( "point_addr", otString, point_addr );
 	sres = XMLTreeToText( doc );
  Qry.CreateVariable( "record", otString, sres );
  Qry.Execute();
  if ( doc )
  	xmlFreeDoc( doc );
  if ( old_doc )
  	xmlFreeDoc( old_doc );  
	return !fds.empty();
}

