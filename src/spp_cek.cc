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
#include "base_tables.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "develop_dbf.h"
#include "misc.h"
#include "sopp.h"
#include "timer.h"
#include "xml_unit.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/helpcpp.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

enum TSQLTYPE { sql_date, sql_NUMERIC, sql_char };

enum TModify { sppnochange, sppinsert, sppupdate, sppdelete };

const string spp__a_dbf_fields =
"AA CHAR(2),"
"ZADERGKA CHAR(34),"
"PNR CHAR(7),"
"PNRS CHAR(7),"
"PR CHAR(3),"
"AR NUMERIC(10,0),"
"VRD CHAR(3),"
"VDV CHAR(3),"
"KUG CHAR(3),"
"BL CHAR(3),"
"NR CHAR(5),"
"DR CHAR(1),"
"LR CHAR(3),"
"KR CHAR(3),"
"RAS NUMERIC(5,0),"
"TOR NUMERIC(1,0),"
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
"PRIZ NUMERIC(1,0),"
"BLS CHAR(3),"
"NRS CHAR(5),"
"RDS CHAR(1),"
"PRUG CHAR(1),"
"MSK CHAR(1),"
"MMIN CHAR(3),"
"NMSP CHAR(3),"
"NMSF CHAR(3),"
"OST NUMERIC(5,0),"
"TRZ NUMERIC(5,0),"
"TRDZ NUMERIC(5,0),"
"FKZ NUMERIC(5,0),"
"OR1 NUMERIC(1,0),"
"OR2 NUMERIC(1,0),"
"KRE NUMERIC(3,0),"
"LKRE NUMERIC(3,0),"
"BOP NUMERIC(1,0),"
"PKZ NUMERIC(5,0),"
"LPC NUMERIC(5,0),"
"EKP NUMERIC(1,0),"
"NSOP CHAR(1),"
"BP NUMERIC(3,0),"
"MBCF NUMERIC(6,0),"
"CBCF NUMERIC(5,2),"
"RVC NUMERIC(5,2),"
"TST NUMERIC(4,0),"
"TVZ NUMERIC(5,0),"
"TRS NUMERIC(5,0),"
"MVZD NUMERIC(6,0),"
"FZAG NUMERIC(5,0),"
"S1 CHAR(3),"
"S2 NUMERIC(4,0),"
"S3 NUMERIC(4,0),"
"S4 CHAR(3),"
"S5 CHAR(3),"
"S6 NUMERIC(4,0),"
"S7 CHAR(3),"
"S8 NUMERIC(4,0),"
"S10 NUMERIC(4,0),"
"KUR NUMERIC(2,0),"
"S2R NUMERIC(4,0),"
"S6R NUMERIC(4,0),"
"T2 NUMERIC(3,0),"
"T3 NUMERIC(3,0),"
"T4 NUMERIC(3,0),"
"T6 NUMERIC(5,0),"
"T7 NUMERIC(5,0),"
"T9 NUMERIC(5,0),"
"T11 NUMERIC(5,0),"
"F2 NUMERIC(3,0),"
"F3 NUMERIC(3,0),"
"F4 NUMERIC(3,0),"
"F6 NUMERIC(5,0),"
"F7 NUMERIC(5,0),"
"F8 NUMERIC(5,0),"
"F9 NUMERIC(5,0),"
"F11 NUMERIC(5,0),"
"\"TO\" NUMERIC(6,0),"
"TO1 NUMERIC(6,0),"
"TO2 NUMERIC(6,0),"
"TO3 NUMERIC(6,0),"
"TO4 NUMERIC(6,0),"
"TO5 NUMERIC(6,0),"
"FKZG NUMERIC(6,0),"
"IZG NUMERIC(6,0),"
"ZGO NUMERIC(6,0),"
"ORVE NUMERIC(3,0),"
"C1T NUMERIC(4,0)";

const string spp__d_dbf_fields =
"AA CHAR(2),"
"ZADERGKA CHAR(34),"
"PNR CHAR(7),"
"PNRS CHAR(7),"
"PR CHAR(3),"
"AR NUMERIC(10,0),"
"VRD CHAR(3),"
"VDV CHAR(3),"
"KUG CHAR(3),"
"BL CHAR(3),"
"NR CHAR(5),"
"DR CHAR(1),"
"LR CHAR(3),"
"KR CHAR(3),"
"RAS NUMERIC(5,0),"
"TOR NUMERIC(1,0),"
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
"PRIZ NUMERIC(1,0),"
"BLS CHAR(3),"
"NRS CHAR(5),"
"RDS CHAR(1),"
"PRUG CHAR(1),"
"MSK CHAR(1),"
"MMIN CHAR(3),"
"NMSP CHAR(3),"
"NMSF CHAR(3),"
"OST NUMERIC(5,0),"
"TRZ NUMERIC(5,0),"
"TRDZ NUMERIC(5,0),"
"FKZ NUMERIC(5,0),"
"OR1 NUMERIC(1,0),"
"OR2 NUMERIC(1,0),"
"KRE NUMERIC(3,0),"
"LKRE NUMERIC(3,0),"
"BOP NUMERIC(1,0),"
"PKZ NUMERIC(5,0),"
"LPC NUMERIC(5,0),"
"EKP NUMERIC(1,0),"
"NSOP CHAR(1),"
"BP NUMERIC(3,0),"
"MBCF NUMERIC(6,0),"
"CBCF NUMERIC(5,2),"
"RVC NUMERIC(5,2),"
"TST NUMERIC(4,0),"
"TVZ NUMERIC(5,0),"
"TRS NUMERIC(5,0),"
"MVZD NUMERIC(6,0),"
"FZAG NUMERIC(5,0),"
"S1 CHAR(3),"
"S2 NUMERIC(4,0),"
"S3 NUMERIC(4,0),"
"S4 CHAR(3),"
"S5 CHAR(3),"
"S6 NUMERIC(4,0),"
"S7 CHAR(3),"
"S8 NUMERIC(4,0),"
"S10 NUMERIC(4,0),"
"KUR NUMERIC(2,0),"
"S2R NUMERIC(4,0),"
"S6R NUMERIC(4,0),"
"T2 NUMERIC(3,0),"
"T3 NUMERIC(3,0),"
"T4 NUMERIC(3,0),"
"T6 NUMERIC(5,0),"
"T7 NUMERIC(5,0),"
"T8 NUMERIC(4,0),"
"T9 NUMERIC(5,0),"
"T11 NUMERIC(5,0),"
"F2 NUMERIC(3,0),"
"F3 NUMERIC(3,0),"
"F4 NUMERIC(3,0),"
"F6 NUMERIC(5,0),"
"F7 NUMERIC(5,0),"
"F8 NUMERIC(5,0),"
"F9 NUMERIC(5,0),"
"F11 NUMERIC(5,0),"
"\"TO\" NUMERIC(6,0),"
"TO1 NUMERIC(6,0),"
"TO2 NUMERIC(6,0),"
"TO3 NUMERIC(6,0),"
"TO4 NUMERIC(6,0),"
"TO5 NUMERIC(6,0),"
"FKZG NUMERIC(6,0),"
"IZG NUMERIC(6,0),"
"ZGO NUMERIC(6,0),"
"ORVE NUMERIC(3,0),"
"C1T NUMERIC(4,0)";

const string spp__ak_dbf_fields =
"PER CHAR(7),"
"AR NUMERIC(4,0),"
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
"FVPZ NUMERIC(4,0),"
"RDVZ DATE,"
"RVVZ NUMERIC(4,0),"
"FDVZ DATE,"
"FVVZ NUMERIC(4,0),"
"PZP NUMERIC(4,0),"
"PVV NUMERIC(4,0),"
"PDV DATE,"
"RDV DATE,"
"RVV NUMERIC(4,0),"
"PRZR CHAR(3),"
"FDV DATE,"
"FVV NUMERIC(4,0),"
"PRZK CHAR(3),"
"VPOL NUMERIC(4,0),"
"TP NUMERIC(5,0),"
"DPP DATE,"
"VPP NUMERIC(4,0),"
"VPP1 NUMERIC(4,0),"
"DPR DATE,"
"VPR NUMERIC(4,0),"
"DPF DATE,"
"VPF NUMERIC(4,0),"
"VST NUMERIC(4,0),"
"TPS NUMERIC(3,0),"
"TRRB NUMERIC(3,0),"
"TRM NUMERIC(2,0),"
"TRUK NUMERIC(4,0),"
"TBAG NUMERIC(5,0),"
"TGRU NUMERIC(5,0),"
"TPC NUMERIC(5,0),"
"DPS NUMERIC(3,0),"
"DRB NUMERIC(3,0),"
"DRM NUMERIC(3,0),"
"RUK NUMERIC(4,0),"
"BAG NUMERIC(5,0),"
"PBAG NUMERIC(4,0),"
"GRU NUMERIC(5,0),"
"PC NUMERIC(5,0),"
"PB NUMERIC(3,0),"
"TRB NUMERIC(3,0),"
"PUR NUMERIC(2,0),"
"TAR_P NUMERIC(5,0),"
"SPUR CHAR(2)";

const string spp__dk_dbf_fields =
"PER CHAR(7),"
"AR NUMERIC(4,0),"
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
"FVPZ NUMERIC(4,0),"
"RDVZ DATE,"
"RVVZ NUMERIC(4,0),"
"FDVZ DATE,"
"FVVZ NUMERIC(4,0),"
"PZP NUMERIC(4,0),"
"PVV NUMERIC(4,0),"
"PDV DATE,"
"RDV DATE,"
"RVV NUMERIC(4,0),"
"PRZR CHAR(3),"
"FDV DATE,"
"FVV NUMERIC(4,0),"
"PRZK CHAR(3),"
"VPOL NUMERIC(4,0),"
"TP NUMERIC(5,0),"
"DPP DATE,"
"VPP NUMERIC(4,0),"
"VPP1 NUMERIC(4,0),"
"DPR DATE,"
"VPR NUMERIC(4,0),"
"DPF DATE,"
"VPF NUMERIC(4,0),"
"VST NUMERIC(4,0),"
"TPS NUMERIC(3,0),"
"TRRB NUMERIC(3,0),"
"TRM NUMERIC(2,0),"
"TRUK NUMERIC(4,0),"
"TBAG NUMERIC(5,0),"
"TPBAG NUMERIC(4,0),"
"TGRU NUMERIC(5,0),"
"TPC NUMERIC(5,0),"
"DPS NUMERIC(3,0),"
"DRB NUMERIC(3,0),"
"DRM NUMERIC(3,0),"
"RUK NUMERIC(4,0),"
"BAG NUMERIC(5,0),"
"PBAG NUMERIC(5,0),"
"GRU NUMERIC(5,0),"
"PC NUMERIC(5,0),"
"PB NUMERIC(3,0),"
"TRB NUMERIC(5,0),"
"PUR NUMERIC(2,0),"
"TAR_P NUMERIC(5,0),"
"SPUR CHAR(2)";

inline string GetMinutes( TDateTime d1, TDateTime d2 )
{
	if ( d2 > NoExists ) {
    int Hour, Min, Sec, Year, Month, Day1, Day2;
   	DecodeDate( d1, Year, Month, Day1 );
   	DecodeDate( d2, Year, Month, Day2 );
    DecodeTime( d2-d1, Hour, Min, Sec );
    return IntToString( (Day2-Day1)*24*60 + Hour*60 + Min );
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
	xmlNodePtr rollbackNode;
	string tablename;
	try {
	  xmlNodePtr queryNode = NewTextChild( doc->children, "query" );
	  tablename = string("SPP") + DateTimeToStr( sppdate, "dd" ) + "A";
    sql_str = string("DROP TABLE ") + tablename;
    xmlNodePtr sqlNode = NewTextChild( queryNode, "sql", sql_str );
    NewTextChild( queryNode, "ignoreErrorCode", 5004 );
    rollbackNode = NewTextChild( queryNode, "table_rollback" );
    NewTextChild( rollbackNode, "table", tablename + ".DBF" );
    NewTextChild( rollbackNode, "tmp_table", string("T") + tablename.substr(1,6) + ".DBF" );
    queryNode = NewTextChild( doc->children, "query" );
    tablename = string("SPP") + DateTimeToStr( sppdate, "dd" ) + "A";
    sql_str = string("CREATE TABLE ") + tablename + "(";
    sql_str += spp__a_dbf_fields + ") IN DATABASE";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );

    tablename = string("SPP") + DateTimeToStr( sppdate, "dd" ) + "D";
	  queryNode = NewTextChild( doc->children, "query" );
    sql_str = string("DROP TABLE ") + tablename;
    sqlNode = NewTextChild( queryNode, "sql", sql_str );
    NewTextChild( queryNode, "ignoreErrorCode", 5004 );
    rollbackNode = NewTextChild( queryNode, "table_rollback" );
    NewTextChild( rollbackNode, "table", tablename + ".DBF" );
    NewTextChild( rollbackNode, "tmp_table", string("T") + tablename.substr(1,6) + ".DBF" );

    queryNode = NewTextChild( doc->children, "query" );
    tablename = string("SPP") + DateTimeToStr( sppdate, "dd" ) + "D";
    sql_str = string("CREATE TABLE ") + tablename + "(";
    sql_str += spp__d_dbf_fields + ") IN DATABASE";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );

	  queryNode = NewTextChild( doc->children, "query" );
	  tablename = string("SPP") + DateTimeToStr( sppdate, "dd" ) + "AK";
    sql_str = string("DROP TABLE ") + tablename;
    sqlNode = NewTextChild( queryNode, "sql", sql_str );
    NewTextChild( queryNode, "ignoreErrorCode", 5004 );
    rollbackNode = NewTextChild( queryNode, "table_rollback" );
    NewTextChild( rollbackNode, "table", tablename + ".DBF" );
    NewTextChild( rollbackNode, "tmp_table", string("T") + tablename.substr(1,6) + ".DBF" );

    queryNode = NewTextChild( doc->children, "query" );
    tablename = string("SPP") + DateTimeToStr( sppdate, "dd" ) + "AK";
    sql_str = string("CREATE TABLE ") + tablename + "(";
    sql_str += spp__ak_dbf_fields + ") IN DATABASE";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );

/*    queryNode = NewTextChild( doc->children, "query" );
	  tablename = string("SPP") + DateTimeToStr( sppdate, "dd" ) + "AK";
    sql_str = string("CREATE INDEX ") + tablename + " ON ";
    sql_str += string("SPP") + DateTimeToStr( sppdate, "dd" ) + "AK(PNR ASC,SPUR ASC)";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );

	  queryNode = NewTextChild( doc->children, "query" );
	  tablename = string("SPP") + DateTimeToStr( sppdate, "dd" ) + "AK1";
    sql_str = string("CREATE INDEX ") + tablename + " ON ";
    sql_str += string("SPP") + DateTimeToStr( sppdate, "dd" ) + "AK(PNR ASC,SPUR DESC)";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );*/

	  queryNode = NewTextChild( doc->children, "query" );
	  tablename =  string("SPP") + DateTimeToStr( sppdate, "dd" ) + "DK";
    sql_str = string("DROP TABLE ") + tablename;
    sqlNode = NewTextChild( queryNode, "sql", sql_str );
    NewTextChild( queryNode, "ignoreErrorCode", 5004 );
    rollbackNode = NewTextChild( queryNode, "table_rollback" );
    NewTextChild( rollbackNode, "table", tablename + ".DBF" );
    NewTextChild( rollbackNode, "tmp_table", string("T") + tablename.substr(1,6) + ".DBF" );

    queryNode = NewTextChild( doc->children, "query" );
    tablename = string("SPP") + DateTimeToStr( sppdate, "dd" ) + "DK";
    sql_str = string("CREATE TABLE ") + tablename + "(";
    sql_str += spp__dk_dbf_fields + ") IN DATABASE";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );

/*	  queryNode = NewTextChild( doc->children, "query" );
	  tablename = string("SPP") + DateTimeToStr( sppdate, "dd" ) + "DK";
    sql_str = string("CREATE INDEX ") + tablename + " ON ";
    sql_str += string("SPP") + DateTimeToStr( sppdate, "dd" ) + "DK(PNR ASC,SPUR ASC)";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );*/

	  queryNode = NewTextChild( doc->children, "query" );
    sql_str = "DELETE FROM SPPCIKL WHERE DSPP={d'" + DateTimeToStr( sppdate, "yyyy-mm-dd" ) + "'}";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );


	  queryNode = NewTextChild( doc->children, "query" );
    sql_str = "INSERT INTO SPPCIKL(DADP,DFSPP,VFSPP,DSPP) VALUES( '" +
               reqInfo->desk.city + "',{d'" + DateTimeToStr( reqInfo->desk.time, "yyyy-mm-dd" ) +
               "'},'" + DateTimeToStr( reqInfo->desk.time, "hh:nn" ) + "',{d'" + DateTimeToStr( sppdate, "yyyy-mm-dd" ) + "'})";
    sqlNode = NewTextChild( queryNode, "sql", sql_str );

	  if ( doc ) {
	  	string encoding = getFileEncoding( FILE_SPPCEK_TYPE, point_addr );
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

string getPNR( const string airline, int airline_fmt, int flt_no, const string suffix, int suffix_fmt )
{
	string pnr;
	if ( airline == "ЬЬ" )
		pnr = IntToString( flt_no ) +
		      ElemIdToElemCtxt( ecDisp, etSuffix, suffix, suffix_fmt );
	else
		pnr = ElemIdToElemCtxt( ecDisp, etAirline, airline, airline_fmt ) +
		      IntToString( flt_no ) +
		      ElemIdToElemCtxt( ecDisp, etSuffix, suffix, suffix_fmt );
	return pnr;
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
  	NewTextChild( NodeA, "PNR", getPNR( tr->airline_in, tr->airline_in_fmt, tr->flt_no_in, tr->suffix_in, tr->suffix_in_fmt ) );
  	NewTextChild( NodeA, "DN", GetStrDate( tr->scd_in ) );
  	NewTextChild( NodeA, "KUG", tr->airline_in );
  	NewTextChild( NodeA, "TVC", tr->craft_in );
    if ( tr->pr_del_in == 1 )
    	NewTextChild( NodeA, "BNP", "ОТМЕН" );
    else
    	NewTextChild( NodeA, "BNP", tr->bort_in.substr(0,5) );
    NewTextChild( NodeA, "NMSF", tr->park_in );
    NewTextChild( NodeA, "RPVSN", GetMinutes( tr->scd_in, tr->est_in ).substr(0,3) );
 	  if ( tr->airline_in + IntToString( tr->flt_no_in ) + tr->suffix_in == tr->airline_out + IntToString( tr->flt_no_out ) + tr->suffix_out )
 	    NewTextChild( NodeA, "PRIZ", IntToString( 2 ) );
 	  else
 	  	NewTextChild( NodeA, "PRIZ", IntToString( 1 ) );
    NewTextChild( NodeA, "KUR", IntToString( tr->places_in.size() ) );
    if ( tr->triptype_in == "м" )
    	NewTextChild( NodeA, "VDV", "МЕЖ" );
    else
      NewTextChild( NodeA, "VDV", "ПАС" );
    int k = 0;
    int prior_point_id = ASTRA::NoExists;
    for ( TSOPPDests::iterator d=tr->places_in.begin(); d!= tr->places_in.end(); d++,k++ ) {
  	  if ( d->pr_del == 0 )
  			prior_point_id = d->point_id;
      xmlNodePtr NodeAK = NewTextChild( NodeA, "AK" );
    	NewTextChild( NodeAK, "PNR", getPNR( tr->airline_in, tr->airline_in_fmt, tr->flt_no_in, tr->suffix_in, tr->suffix_in_fmt ) );
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
      //NewTextChild( NodeAK, "PC", IntToString( 0 ) );
      NewTextChild( NodeAK, "TPC", IntToString( 0 ) );
      //NewTextChild( NodeAK, "GRU", IntToString( 0 ) );
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
    //NewTextChild( NodeA, "F9", IntToString( cargo_in ) );
    //NewTextChild( NodeA, "F11", IntToString( mail_in ) );
	} // end if !place_in.empty()
	if ( !tr->places_out.empty() ) {
  	GetLuggage( tr->point_id, lug_out );
		xmlNodePtr NodeD = NewTextChild( tripNode, "D" );
		NewTextChild( NodeD, "PNR", getPNR( tr->airline_out, tr->airline_out_fmt, tr->flt_no_out, tr->suffix_out, tr->suffix_out_fmt ) );
		NewTextChild( NodeD, "DN", GetStrDate( tr->scd_out ) );
  	NewTextChild( NodeD, "KUG", tr->airline_out );
  	NewTextChild( NodeD, "TVC", tr->craft_out );
    if ( tr->pr_del_out == 1 )
    	NewTextChild( NodeD, "BNP", "ОТМЕН" );
    else
 	  	NewTextChild( NodeD, "BNP", tr->bort_out.substr(0,5) );
 	  NewTextChild( NodeD, "NMSF", tr->park_out );
 	  NewTextChild( NodeD, "RPVSN", GetMinutes( tr->scd_out, tr->est_out ).substr(0,3) );
 	  if ( tr->airline_in + IntToString( tr->flt_no_in ) + tr->suffix_in == tr->airline_out + IntToString( tr->flt_no_out ) + tr->suffix_out )
 	    NewTextChild( NodeD, "PRIZ", IntToString( 2 ) );
 	  else
 	  	NewTextChild( NodeD, "PRIZ", IntToString( 1 ) );
    if ( tr->triptype_out == "м" )
    	NewTextChild( NodeD, "VDV", "МЕЖ" );
    else
    	NewTextChild( NodeD, "VDV", "ПАС" );
    // теперь создание записей по плечам
    int k = 0;
    for ( TSOPPDests::iterator d=tr->places_out.begin(); d!=tr->places_out.end(); d++, k++ ) {
    	ProgTrace( TRACE5, "k=%d, end=%d", k, (d==tr->places_out.end()) );
      vector<Cargo>::iterator c=lug_out.vcargo.end();
      xmlNodePtr NodeDK = NewTextChild( NodeD, "DK" );
      NewTextChild( NodeDK, "PNR", getPNR( tr->airline_out, tr->airline_out_fmt, tr->flt_no_out, tr->suffix_out, tr->suffix_out_fmt ) );
      ProgTrace( TRACE5, "tr->places_out.size=%d, d->airp=%s", tr->places_out.size(), d->airp.c_str() );
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
      	//NewTextChild( NodeDK, "PC", IntToString( c->mail ) );
      	//NewTextChild( NodeDK, "GRU", IntToString( c->cargo ) );
      }
      else {
      	//NewTextChild( NodeDK, "PC", IntToString( 0 ) );
      	//NewTextChild( NodeDK, "GRU", IntToString( 0 ) );
      }
      NewTextChild( NodeDK, "TPC", IntToString( 0 ) );
      NewTextChild( NodeDK, "TGRU", IntToString( 0 ) );
    }; // end for
    for ( vector<Cargo>::iterator wc=lug_out.vcargo.begin(); wc!=lug_out.vcargo.end(); wc++ ) {
    	cargo_out += wc->cargo;
    	mail_out += wc->mail;
    }
 	  NewTextChild( NodeD, "PKZ", IntToString( lug_out.max_commerce ) );
 	  //NewTextChild( NodeD, "F9", IntToString( cargo_out ) );
 	  //NewTextChild( NodeD, "F11", IntToString( mail_out ) );
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
  xmlNodePtr queryNode, sqlNode, rollbackNode;
  string dbf_type;
  if ( pr_land )
  	dbf_type = "A";
  else
  	dbf_type = "D";
  string tablename = string("SPP") + day + dbf_type;
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
  	string deltablename = string("SPP") + string(NodeAsString( "DN", nodeP )).substr(8,2) + dbf_type;
   	sql_str = string("DELETE FROM ") + deltablename + " WHERE PNR=:PNR AND DN=:DN";
 	  queryNode = NewTextChild( sqldoc->children, "query" );
 	  sqlNode = NewTextChild( queryNode, "sql", sql_str );
    rollbackNode = NewTextChild( queryNode, "table_rollback" );
    NewTextChild( rollbackNode, "table", deltablename + ".DBF" );
    NewTextChild( rollbackNode, "tmp_table", string("T") + deltablename.substr(1,6) + ".DBF" );
 	  paramsNode = NewTextChild( queryNode, "params" );
 	  createParam( paramsNode, "PNR", NodeAsString( "PNR", nodeP ), DBF_TYPE_CHAR );
 	  createParam( paramsNode, "DN", NodeAsString( "DN", nodeP ), DBF_TYPE_DATE );
  };
 	if ( pr_insert )  {
		//insert прилет
    sql_str =
    string("INSERT INTO ") + tablename +
    "(PNR,KUG,TVC,BNP,NMSF,RPVSN,DN,PRIZ,PKZ,KUR,VDV) VALUES"
    "(:PNR,:KUG,:TVC,:BNP,:NMSF,:RPVSN,:DN,:PRIZ,:PKZ,:KUR,:VDV)";
  };
  if ( pr_update ) {
		// update if change
		sql_str =
      string("UPDATE ") + tablename +
      " SET KUG=:KUG,TVC=:TVC,BNP=:BNP,NMSF=:NMSF,RPVSN=:RPVSN,"
      "    PRIZ=:PRIZ,PKZ=:PKZ,KUR=:KUR,VDV=:VDV "
      " WHERE PNR=:PNR AND DN=:DN";
  };
  if ( pr_insert || pr_update ) {
    queryNode = NewTextChild( sqldoc->children, "query" );
    sqlNode = NewTextChild( queryNode, "sql", sql_str );
    rollbackNode = NewTextChild( queryNode, "table_rollback" );
    NewTextChild( rollbackNode, "table", tablename + ".DBF" );
    NewTextChild( rollbackNode, "tmp_table", string("T") + tablename.substr(1,6) + ".DBF" );
    paramsNode = NewTextChild( queryNode, "params" );
    if ( pr_update ) {
 	    createParam( paramsNode, "PNR", NodeAsString( "PNR", nodeP ), DBF_TYPE_CHAR );
 	    createParam( paramsNode, "DN", NodeAsString( "DN", nodeP ), DBF_TYPE_DATE );
 	  }
 	  else {
 	    createParam( paramsNode, "PNR", NodeAsString( "PNR", nodeN ), DBF_TYPE_CHAR );
 	    createParam( paramsNode, "DN", NodeAsString( "DN", nodeN ), DBF_TYPE_DATE );
 	  }
	  createParam( paramsNode, "KUG", NodeAsString( "KUG", nodeN ), DBF_TYPE_CHAR );
    createParam( paramsNode, "TVC", NodeAsString( "TVC", nodeN ), DBF_TYPE_CHAR );
   	createParam( paramsNode, "BNP", NodeAsString( "BNP", nodeN ), DBF_TYPE_CHAR );
    createParam( paramsNode, "NMSF", NodeAsString( "NMSF", nodeN ), DBF_TYPE_CHAR );
    createParam( paramsNode, "RPVSN", NodeAsString( "RPVSN", nodeN ), DBF_TYPE_CHAR );
   	createParam( paramsNode, "PRIZ", NodeAsString( "PRIZ", nodeN ), DBF_TYPE_NUMBER );
    createParam( paramsNode, "PKZ", NodeAsString( "PKZ", nodeN ), DBF_TYPE_NUMBER );
    //createParam( paramsNode, "F9", NodeAsString( "F9", nodeN ),  DBF_TYPE_NUMBER );
    //createParam( paramsNode, "F11", NodeAsString( "F11", nodeN ), DBF_TYPE_NUMBER );
    createParam( paramsNode, "KUR", NodeAsString( "KUR", nodeN ), DBF_TYPE_NUMBER );
   	createParam( paramsNode, "VDV", NodeAsString( "VDV", nodeN ), DBF_TYPE_CHAR );
  }
  if ( pr_land )
  	dbf_type = "AK";
  else
  	dbf_type = "DK";
  tablename = string("SPP") + day + dbf_type;
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
    	string deltablename = string("SPP") + string(NodeAsString( "DPP", *nodePK )).substr(8,2) + dbf_type;
     	sql_str =
        string("DELETE FROM ") + deltablename +
        " WHERE PNR=:PNR AND DPP=:DPP AND PUR=:PUR";
 	    queryNode = NewTextChild( sqldoc->children, "query" );
 	    sqlNode = NewTextChild( queryNode, "sql", sql_str );
      rollbackNode = NewTextChild( queryNode, "table_rollback" );
      NewTextChild( rollbackNode, "table", deltablename + ".DBF" );
      NewTextChild( rollbackNode, "tmp_table", string("T") + deltablename.substr(1,6) + ".DBF" );
 	    paramsNode = NewTextChild( queryNode, "params" );
 	    createParam( paramsNode, "PNR", NodeAsString( "PNR", *nodePK ), DBF_TYPE_CHAR );
 	    createParam( paramsNode, "DPP", NodeAsString( "DPP", *nodePK ), DBF_TYPE_DATE );
      createParam( paramsNode, "PUR", NodeAsString( "PUR", *nodePK ), DBF_TYPE_NUMBER );
    }
    if ( pr_insert ) {
      sql_str =
        string("INSERT INTO ") + tablename +
        "(PNR,AV,AP,DPP,VPP,DPR,VPR,DPF,VPF,PDV,PVV,RDV,RVV,FDV,FVV,PUR,SPUR,PR,TPC,TGRU) "
        " VALUES(:PNR,:AV,:AP,:DPP,:VPP,:DPR,:VPR,:DPF,:VPF,:PDV,:PVV,:RDV,:RVV,:FDV,:FVV,:PUR,:SPUR,:PR,:TPC,:TGRU)";
    }
    if ( pr_update ) {
      sql_str =
        string("UPDATE ") + tablename +
        " SET "
        "AV=:AV,AP=:AP,DPP=:DPP,VPP=:VPP,DPR=:DPR,VPR=:VPR,DPF=:DPF,VPF=:VPF,PDV=:PDV,PVV=:PVV,RDV=:RDV,"
        "RVV=:RVV,FDV=:FDV,FVV=:FVV,PUR=:PUR,SPUR=:SPUR,PR=:PR,TPC=:TPC,TGRU=:TGRU "
        " WHERE PNR=:PNR AND DPP=:DPP AND PUR=:PUR";
    }
    if ( pr_insert || pr_update ) {
      queryNode = NewTextChild( sqldoc->children, "query" );
      sqlNode = NewTextChild( queryNode, "sql", sql_str );
      rollbackNode = NewTextChild( queryNode, "table_rollback" );
      NewTextChild( rollbackNode, "table", tablename + ".DBF" );
      NewTextChild( rollbackNode, "tmp_table", string("T") + tablename.substr(1,6) + ".DBF" );
      paramsNode = NewTextChild( queryNode, "params" );
      if ( pr_update ) {
 	      createParam( paramsNode, "PNR", NodeAsString( "PNR", *nodePK ), DBF_TYPE_CHAR );
 	      createParam( paramsNode, "DPP", NodeAsString( "DPP", *nodePK ), DBF_TYPE_DATE );
 	      createParam( paramsNode, "PUR", NodeAsString( "PUR", *nodePK ), DBF_TYPE_NUMBER );
 	      createParam( paramsNode, "SPUR", (string)" "+NodeAsString( "PUR", *nodePK ), DBF_TYPE_CHAR );
 	    }
 	    else {
 	      createParam( paramsNode, "PNR", NodeAsString( "PNR", *nodeNK ), DBF_TYPE_CHAR );
 	      createParam( paramsNode, "DPP", NodeAsString( "DPP", *nodeNK ), DBF_TYPE_DATE );
 	      createParam( paramsNode, "PUR", NodeAsString( "PUR", *nodeNK ), DBF_TYPE_NUMBER );
 	      createParam( paramsNode, "SPUR", (string)" "+NodeAsString( "PUR", *nodeNK ), DBF_TYPE_CHAR );
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
     	//!!!createParam( paramsNode, "FDV", NodeAsString( "FDV", *nodeNK ), DBF_TYPE_DATE );
     	createParam( paramsNode, "FDV", NodeAsString( "DPP", *nodeNK ), DBF_TYPE_DATE );
     	createParam( paramsNode, "FVV", NodeAsString( "FVV", *nodeNK ), DBF_TYPE_NUMBER );
      createParam( paramsNode, "PR", NodeAsString( "PR", *nodeNK ), DBF_TYPE_CHAR );
      //createParam( paramsNode, "PC", NodeAsString( "PC", *nodeNK ), DBF_TYPE_NUMBER );
      createParam( paramsNode, "TPC", NodeAsString( "TPC", *nodeNK ), DBF_TYPE_NUMBER );
      //createParam( paramsNode, "GRU", NodeAsString( "GRU", *nodeNK ), DBF_TYPE_NUMBER );
      createParam( paramsNode, "TGRU", NodeAsString( "TGRU", *nodeNK ), DBF_TYPE_NUMBER );
    }

    if ( nodePK != nodesPK.end() )
  		nodePK++;
  	if ( nodeNK != nodesNK.end() )
  		nodeNK++;
  } // end while
}

void put_string_into_snapshot( int point_id, string type, string point_addr, xmlDocPtr old_doc, xmlDocPtr doc )
{
	TQuery Qry( &OraSession );
 	Qry.SQLText =
 	  "DELETE snapshot_points "
 	  " WHERE point_id=:point_id AND file_type=:file_type AND point_addr=:point_addr";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "file_type", otString, type );
  Qry.CreateVariable( "point_addr", otString, point_addr );
  Qry.Execute();
 	if ( old_doc && !doc )
 		return;
 	Qry.Clear();
  Qry.SQLText =
    "INSERT INTO snapshot_points(point_id,file_type,point_addr,record,page_no ) "
    "                VALUES(:point_id,:file_type,:point_addr,:record,:page_no) ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "file_type", otString, FILE_SPPCEK_TYPE );
  Qry.CreateVariable( "point_addr", otString, point_addr );
  Qry.DeclareVariable( "record", otString );
  Qry.DeclareVariable( "page_no", otInteger );
  string sres = XMLTreeToText( doc );
  int i=0;
  while ( !sres.empty() ) {
  	Qry.SetVariable( "record", sres.substr( 0, 1000 ) );
  	Qry.SetVariable( "page_no", i );
  	Qry.Execute();
  	i++;
  	sres.erase( 0, 1000 );
  }
}

bool createSPPCEKFile( int point_id, const string &point_addr, TFileDatas &fds )
{
	ProgTrace( TRACE5, "CEK point_id=%d", point_id );
	TReqInfo *reqInfo = TReqInfo::Instance();
	reqInfo->user.sets.time = ustTimeLocalDesk;
	reqInfo->Initialize("ЧЛБ");
  reqInfo->user.user_type = utAirport;
  reqInfo->user.access.airps.push_back( "ЧЛБ" );
  reqInfo->user.access.airps_permit = true;

	string file_type = FILE_SPPCEK_TYPE;
	string record;
	TQuery Qry( &OraSession );
 	TDateTime UTCNow = NowUTC();
 	TDateTime LocalNow;
 	modf( UTCToClient( UTCNow, reqInfo->desk.tz_region ), &LocalNow );
 	modf( UTCNow, &UTCNow );

 	/* проверка на существование таблиц */
 	for ( int max_day=0; max_day<=CREATE_SPP_DAYS(); max_day++ ) {
	  createSPPCEK( (int)LocalNow + max_day, file_type, point_addr, fds );
	}
	TFileData fd;
  TSOPPTrips trips;
  createSOPPTrip( point_id, trips );
  TSOPPTrips::iterator tr = trips.end();
  xmlDocPtr doc = 0, old_doc = 0;
  std::string errcity;
  for ( tr=trips.begin(); tr!=trips.end(); tr++ ) {
  	if ( tr->point_id != point_id ) continue;
  	bool res;
  	try {
  	  res = FilterFlightDate( *tr, UTCNow, UTCNow + CREATE_SPP_DAYS() + 1, /*true,*/ errcity, false ); // фильтр по датам прилета-вылета рейса
  	}
  	catch(...) {
  		res = false;
  	}
    if ( !res ) continue;
    ProgTrace( TRACE5, "CEK point_id=%d, res=%d, CREATE_SPP_DAYS()=%d", point_id, res, CREATE_SPP_DAYS() );
  	createXMLTrip( tr, doc );
  	break;
  }
  if ( !doc && !trips.empty() ) // рейс не удален
  	return fds.size();
  ProgTrace( TRACE5, "CEK point_id=%d", point_id );
	Qry.Clear();
	Qry.SQLText =
	 "SELECT record FROM points, snapshot_points "
	 " WHERE points.point_id=:point_id AND "
	 "       points.point_id=snapshot_points.point_id AND "
	 "       snapshot_points.point_addr=:point_addr AND "
	 "       snapshot_points.file_type=:file_type "
	 " ORDER BY page_no";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.CreateVariable( "file_type", otString, file_type );
	Qry.Execute();
	while ( !Qry.Eof ) {
		record += Qry.FieldAsString( "record" );
		Qry.Next();
	}
  string strdoc;
  if ( doc )
  	strdoc = XMLTreeToText( doc );
  if ( strdoc == record ) {
    if ( doc )
    	xmlFreeDoc( doc );
  	return fds.size();
  }
  // есть изменения
  if ( !record.empty() ) {
 		record.replace( record.find( "encoding=\"UTF-8\""), string( "encoding=\"UTF-8\"" ).size(), string("encoding=\"") + "CP866" + "\"" );
    old_doc = TextToXMLTree( record );
	  if ( old_doc ) {
      xmlFree(const_cast<xmlChar *>(old_doc->encoding));
      old_doc->encoding = 0;
      xml_decode_nodelist( old_doc->children );
		}
  }

  xmlDocPtr sqldoc = 0;
  TDateTime scd_in, scd_out;
  if ( tr != trips.end() ) {
  	scd_in = tr->scd_in;
  	scd_out = tr->scd_out;
  }
  else {
  	Qry.Clear();
  	Qry.SQLText = "SELECT scd_in, scd_out FROM points WHERE point_id=:point_id";
  	Qry.CreateVariable( "point_id", otInteger, point_id );
  	Qry.Execute();
  	if ( Qry.FieldIsNULL( "scd_in" ) )
  	  scd_in = NoExists;
  	else
  		scd_in = Qry.FieldAsDateTime( "scd_in" );
  	if ( Qry.FieldIsNULL( "scd_out" ) )
  	  scd_out = NoExists;
  	else
  		scd_out = Qry.FieldAsDateTime( "scd_out" );
  }
  createDBF( sqldoc, old_doc, doc, DateTimeToStr( scd_in, "dd" ), true ); // на прилет
  createDBF( sqldoc, old_doc, doc, DateTimeToStr( scd_out, "dd" ), false ); // на вылет

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
  put_string_into_snapshot( point_id, FILE_SPPCEK_TYPE, point_addr, old_doc, doc );
  //ProgTrace( TRACE5, "doc=%p, old_doc=%p", doc, old_doc );
  if ( doc )
  	xmlFreeDoc( doc );
  if ( old_doc ) {
  	xmlFreeDoc( old_doc );
  }
	return !fds.empty();
}

/*
drop table sppcikl
create table sppcikl(
DADP CHAR(3),
DFSPP DATE,
VFSPP CHAR(5),
DSPP DATE,
NSPPA CHAR(6),
NSPPD CHAR(6),
NSPPAK CHAR(7),
NSPPDK CHAR(7) ) IN DATABASE
*/
