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
#include "serverlib/logger.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/test.h"


//#include "serverlib/perfom.h"
//#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

enum TSQLTYPE { sql_date, sql_NUMERIC, sql_char };

enum TModify { sppnochange, sppinsert, sppupdate, sppdelete };

const string spp__a_dbf_fields =
"AA CHAR(2),"
"ZADERGKA CHAR(34),"
//"PNR CHAR(8),"
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
//"PNR CHAR(8),"
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
//"PNR CHAR(8),"
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
//"PNR CHAR(8),"
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

void createSQLParam( xmlNodePtr paramsNode, const string &name, const string &value, const string &type )
{
  	xmlNodePtr Nparam = NewTextChild( paramsNode, "param" );
  	NewTextChild( Nparam, "name", name );
  	NewTextChild( Nparam, "value", value );
  	NewTextChild( Nparam, "type", type );
}

string getRemoveSuffix( TDateTime spp_date, TDateTime scd )
{
  TDateTime d;
  modf( scd, &d );
  string res;
  if ( spp_date != d )
    res = "М";
  return res;
}

string getPNRParam( const string airline, TElemFmt airline_fmt, int flt_no,
                    const string suffix, TElemFmt suffix_fmt, const string &remove_suffix )
{
	string pnr;
	if ( airline == "ЬЬ" )
		pnr = IntToString( flt_no ) +
		      ElemIdToElemCtxt( ecDisp, etSuffix, suffix, suffix_fmt );
	else
		pnr = ElemIdToElemCtxt( ecDisp, etAirline, airline, airline_fmt ) +
		      IntToString( flt_no ) +
		      ElemIdToElemCtxt( ecDisp, etSuffix, suffix, suffix_fmt );
  pnr += remove_suffix;
	return pnr;
}

string getAirline_CityOnwerCode( const string triptype, const string airline )
{
	if ( triptype == "м" )
		return string("М");
	else {
		TQuery Qry( &OraSession );
		Qry.SQLText =
		  "SELECT * FROM area_countries, airlines, cities "
		  " WHERE airlines.code=:airline AND "
		  "       cities.code=airlines.city AND "
		  "       cities.country=area_countries.country AND cities.country<>'РФ'";
		Qry.CreateVariable( "airline", otString, airline );
		Qry.Execute();
		if ( Qry.Eof )
			return string( "Р" );
		else return string( "С" );
  }
}

void createXMLTrips( const string &dbf_type, bool pr_remove, TDateTime spp_date, const TSOPPTrips::iterator tr, const string &commander, xmlDocPtr &doc )
{
  if ( doc )
    ProgTrace( TRACE5, "createXMLTrips: XMLTreeToText=%s", XMLTreeToText( doc ).c_str() );
  ProgTrace( TRACE5, "CEK point_id=%d, spp_date=%s", tr->point_id, DateTimeToStr( spp_date, ServerFormatDateTimeAsString ).c_str() );
  xmlNodePtr tripNode;
	TDateTime tm;
	/*Luggage lug_in, lug_out;
  int cargo_in=0, cargo_out=0, mail_in=0, mail_out=0;
  lug_in.max_commerce = 0; lug_out.max_commerce = 0; */
  if ( !doc ) {
    doc = CreateXMLDoc( "UTF-8", "trips" );
	  SetProp( doc->children, "point_id", tr->point_id );
  }

  tripNode = NewTextChild( doc->children, "trip" );
  SetProp( tripNode, "spp_date",  DateTimeToStr( spp_date, ServerFormatDateTimeAsString ) );
  SetProp( tripNode, "type", dbf_type );

	ProgTrace( TRACE5, "createXMLTrips: XMLTreeToText=%s", XMLTreeToText( doc ).c_str() );
	if ( dbf_type == "A" ) {
		xmlNodePtr NodeA = NewTextChild( tripNode, dbf_type.c_str() );
  	NewTextChild( NodeA, "PNR", getPNRParam( tr->airline_in, tr->airline_in_fmt, tr->flt_no_in, tr->suffix_in, tr->suffix_in_fmt, getRemoveSuffix( spp_date, tr->scd_in ) ) );
  	NewTextChild( NodeA, "DN", GetStrDate( tr->scd_in ) );
  	NewTextChild( NodeA, "KUG", tr->airline_in );
  	NewTextChild( NodeA, "TVC", ElemIdToElemCtxt( ecDisp, etCraft, tr->craft_in, tr->craft_in_fmt ) );
    if ( tr->pr_del_in == 1 )
    	NewTextChild( NodeA, "BNP", "ОТМЕН" );
    else {
      if ( pr_remove )
        NewTextChild( NodeA, "BNP", "ПЕРЕН" );
      else
    	  NewTextChild( NodeA, "BNP", tr->bort_in.substr(0,5) );
    }
    NewTextChild( NodeA, "NMSF", tr->park_in );
    NewTextChild( NodeA, "RPVSN", GetMinutes( tr->scd_in, tr->est_in ).substr(0,3) );
 	  if ( tr->airline_in + IntToString( tr->flt_no_in ) + tr->suffix_in == tr->airline_out + IntToString( tr->flt_no_out ) + tr->suffix_out )
 	    NewTextChild( NodeA, "PRIZ", IntToString( 2 ) );
 	  else
 	  	NewTextChild( NodeA, "PRIZ", IntToString( 1 ) );
    NewTextChild( NodeA, "KUR", IntToString( tr->places_in.size() ) );
   	NewTextChild( NodeA, "VDV", tr->triptype_in );
    NewTextChild( NodeA, "VRD", tr->litera_in );
    NewTextChild( NodeA, "ABSM", getAirline_CityOnwerCode( tr->triptype_in, tr->airline_in ) );
    NewTextChild( NodeA, "FAM", string(""));
    int k = 0;
    int prior_point_id = ASTRA::NoExists;
    for ( TSOPPDests::iterator d=tr->places_in.begin(); d!= tr->places_in.end(); d++,k++ ) {
  	  if ( d->pr_del == 0 )
  			prior_point_id = d->point_id;
      xmlNodePtr NodeAK = NewTextChild( NodeA, "AK" );
    	NewTextChild( NodeAK, "PNR", getPNRParam( tr->airline_in, tr->airline_in_fmt, tr->flt_no_in, tr->suffix_in, tr->suffix_in_fmt, getRemoveSuffix( spp_date, tr->scd_in ) ) );
    	string airp_tmp = ElemIdToElemCtxt( ecDisp, etAirp, d->airp, d->airp_fmt );
    	if ( airp_tmp.size() > 3 )
    	  NewTextChild( NodeAK, "AV", d->airp );
    	else
    		NewTextChild( NodeAK, "AV", airp_tmp );
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
      	string airp_tmp = ElemIdToElemCtxt( ecDisp, etAirp, n->airp, n->airp_fmt );
      	if ( airp_tmp.size() > 3 )
      		NewTextChild( NodeAK, "AP", n->airp );
      	else
      		NewTextChild( NodeAK, "AP", airp_tmp );
        NewTextChild( NodeAK, "DPP", GetStrDate( n->scd_in ) );
        modf( n->scd_in, &tm );
        NewTextChild( NodeAK, "VPP", GetMinutes( tm, n->scd_in ) );
        NewTextChild( NodeAK, "DPR", GetStrDate( n->est_in ) );
       	modf( n->est_in, &tm );
       	NewTextChild( NodeAK, "VPR", GetMinutes( tm, n->est_in ) );
       	if ( n->act_in != NoExists )
       	  NewTextChild( NodeAK, "DPF", GetStrDate( n->act_in ) );
        else
          NewTextChild( NodeAK, "DPF", GetStrDate( n->scd_in ) );
       	modf( n->act_in, &tm );
       	NewTextChild( NodeAK, "VPF", GetMinutes( tm, n->act_in ) );

        NewTextChild( NodeAK, "PDV", GetStrDate( d->scd_out ) );
       	modf( d->scd_out, &tm );
       	NewTextChild( NodeAK, "PVV", GetMinutes( tm, d->scd_out ) );
       	NewTextChild( NodeAK, "RDV", GetStrDate( d->est_out ) );
       	modf( d->est_out, &tm );
       	NewTextChild( NodeAK, "RVV", GetMinutes( tm, d->est_out ) );
       	if ( d->act_out != NoExists )
       	  NewTextChild( NodeAK, "FDV", GetStrDate( d->act_out ) );
        else
          NewTextChild( NodeAK, "FDV", GetStrDate( d->scd_out ) );
       	modf( d->act_out, &tm );
      	NewTextChild( NodeAK, "FVV", GetMinutes( tm, d->act_out ) );
      }
      else {
      	string airp_tmp = ElemIdToElemCtxt( ecDisp, etAirp, tr->airp, tr->airp_fmt );
      	if ( airp_tmp.size() > 3 )
      	  NewTextChild( NodeAK, "AP", tr->airp );
      	else
      		NewTextChild( NodeAK, "AP", airp_tmp );
      	NewTextChild( NodeAK, "DPP", GetStrDate( tr->scd_in ) );
       	modf( tr->scd_in, &tm );
       	NewTextChild( NodeAK, "VPP", GetMinutes( tm, tr->scd_in ) );
       	NewTextChild( NodeAK, "DPR", GetStrDate( tr->est_in ) );
       	modf( tr->est_in, &tm );
       	NewTextChild( NodeAK, "VPR", GetMinutes( tm, tr->est_in ) );
       	if ( tr->act_in != NoExists )
       	  NewTextChild( NodeAK, "DPF", GetStrDate( tr->act_in ) );
        else
          NewTextChild( NodeAK, "DPF", GetStrDate( tr->scd_in ) );
       	modf( tr->act_in, &tm );
       	NewTextChild( NodeAK, "VPF", GetMinutes( tm, tr->act_in ) );

       	NewTextChild( NodeAK, "PDV", GetStrDate( d->scd_out ) );
        modf( d->scd_out, &tm );
        NewTextChild( NodeAK, "PVV", GetMinutes( tm, d->scd_out ) );
        NewTextChild( NodeAK, "RDV", GetStrDate( d->est_out ) );
       	modf( d->est_out, &tm );
       	NewTextChild( NodeAK, "RVV", GetMinutes( tm, d->est_out ) );
       	if ( d->act_out != NoExists )
       	  NewTextChild( NodeAK, "FDV", GetStrDate( d->act_out ) );
        else
          NewTextChild( NodeAK, "FDV", GetStrDate( d->scd_out ) );
       	modf( d->act_out, &tm );
       	NewTextChild( NodeAK, "FVV", GetMinutes( tm, d->act_out ) );
      }
      NewTextChild( NodeAK, "PUR", IntToString( k + 1 ) );
      NewTextChild( NodeAK, "PR", IntToString( 0 ) );
      //NewTextChild( NodeAK, "PC", IntToString( 0 ) );
      //NewTextChild( NodeAK, "TPC", IntToString( 0 ) );
      //NewTextChild( NodeAK, "GRU", IntToString( 0 ) );
      //NewTextChild( NodeAK, "TGRU", IntToString( 0 ) );
    };
/*		if ( prior_point_id > ASTRA::NoExists ) {
 			GetLuggage( prior_point_id, lug_in );
 		}
 		else lug_in.max_commerce = 0;
    for ( vector<Cargo>::iterator wc=lug_in.vcargo.begin(); wc!=lug_in.vcargo.end(); wc++ ) {
    	if ( wc->point_arv == tr->point_id ) {
     	  cargo_in += wc->cargo;
       	mail_in += wc->mail;
       }
     }*/
    //NewTextChild( NodeA, "PKZ", IntToString( lug_in.max_commerce ) );
    //NewTextChild( NodeA, "F9", IntToString( cargo_in ) );
    //NewTextChild( NodeA, "F11", IntToString( mail_in ) );
	} // end if !place_in.empty()
	if ( dbf_type == "D" ) {
  	//GetLuggage( tr->point_id, lug_out );
		xmlNodePtr NodeD = NewTextChild( tripNode, dbf_type.c_str() );
		NewTextChild( NodeD, "PNR", getPNRParam( tr->airline_out, tr->airline_out_fmt, tr->flt_no_out, tr->suffix_out, tr->suffix_out_fmt, getRemoveSuffix( spp_date, tr->scd_out ) ) );
		NewTextChild( NodeD, "DN", GetStrDate( tr->scd_out ) );
  	NewTextChild( NodeD, "KUG", tr->airline_out );
  	NewTextChild( NodeD, "TVC", ElemIdToElemCtxt( ecDisp, etCraft, tr->craft_out, tr->craft_out_fmt ) );
    if ( tr->pr_del_out == 1 )
    	NewTextChild( NodeD, "BNP", "ОТМЕН" );
    else {
      if ( pr_remove )
        NewTextChild( NodeD, "BNP", "ПЕРЕН" );
      else
        NewTextChild( NodeD, "BNP", tr->bort_out.substr(0,5) );
    }
 	  NewTextChild( NodeD, "NMSF", tr->park_out );
 	  NewTextChild( NodeD, "RPVSN", GetMinutes( tr->scd_out, tr->est_out ).substr(0,3) );
 	  if ( tr->airline_in + IntToString( tr->flt_no_in ) + tr->suffix_in == tr->airline_out + IntToString( tr->flt_no_out ) + tr->suffix_out )
 	    NewTextChild( NodeD, "PRIZ", IntToString( 2 ) );
 	  else
 	  	NewTextChild( NodeD, "PRIZ", IntToString( 1 ) );
   	NewTextChild( NodeD, "VDV", tr->triptype_out );
    NewTextChild( NodeD, "VRD", tr->litera_out );
    NewTextChild( NodeD, "ABSM", getAirline_CityOnwerCode( tr->triptype_out, tr->airline_out ) );
    NewTextChild( NodeD, "FAM", commander.substr(0,12) );
    // теперь создание записей по плечам
    int k = 0;
    for ( TSOPPDests::iterator d=tr->places_out.begin(); d!=tr->places_out.end(); d++, k++ ) {
    	//ProgTrace( TRACE5, "k=%d, end=%d", k, (d==tr->places_out.end()) );
      //vector<Cargo>::iterator c=lug_out.vcargo.end();
      xmlNodePtr NodeDK = NewTextChild( NodeD, "DK" );
      NewTextChild( NodeDK, "PNR", getPNRParam( tr->airline_out, tr->airline_out_fmt, tr->flt_no_out, tr->suffix_out, tr->suffix_out_fmt, getRemoveSuffix( spp_date, tr->scd_out ) ) );
      ProgTrace( TRACE5, "tr->places_out.size=%zu, d->airp=%s", tr->places_out.size(), d->airp.c_str() );
      string airp_tmp = ElemIdToElemCtxt( ecDisp, etAirp, d->airp, d->airp_fmt );
      if ( airp_tmp.size() > 3 )
        NewTextChild( NodeDK, "AP", d->airp );
      else
      	NewTextChild( NodeDK, "AP", airp_tmp );
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
      	string airp_tmp = ElemIdToElemCtxt( ecDisp, etAirp, n->airp, n->airp_fmt );
      	if ( airp_tmp.size() > 3 )
      	  NewTextChild( NodeDK, "AV", n->airp );
      	else
      		NewTextChild( NodeDK, "AV", airp_tmp );
      	NewTextChild( NodeDK, "DPP", GetStrDate( d->scd_in ) );
       	modf( d->scd_in, &tm );
      	NewTextChild( NodeDK, "VPP", GetMinutes( tm, d->scd_in ) );
      	NewTextChild( NodeDK, "DPR", GetStrDate( d->est_in ) );
       	modf( d->est_in, &tm );
        NewTextChild( NodeDK, "VPR", GetMinutes( tm, d->est_in ) );
        if ( d->act_in != NoExists )
          NewTextChild( NodeDK, "DPF", GetStrDate( d->act_in ) );
        else
          NewTextChild( NodeDK, "DPF", GetStrDate( d->scd_in ) );
       	modf( d->act_in, &tm );
        NewTextChild( NodeDK, "VPF", GetMinutes( tm, d->act_in ) );

        NewTextChild( NodeDK, "PDV", GetStrDate( n->scd_out ) );
       	modf( n->scd_out, &tm );
        NewTextChild( NodeDK, "PVV", GetMinutes( tm, n->scd_out ) );
        NewTextChild( NodeDK, "RDV", GetStrDate( n->est_out ) );
       	modf( n->est_out, &tm );
        NewTextChild( NodeDK, "RVV", GetMinutes( tm, n->est_out ) );
        if ( n->act_out != NoExists )
          NewTextChild( NodeDK, "FDV", GetStrDate( n->act_out ) );
        else
          NewTextChild( NodeDK, "FDV", GetStrDate( n->scd_out ) );
       	modf( n->act_out, &tm );
        NewTextChild( NodeDK, "FVV", GetMinutes( tm, n->act_out ) );
/*        for ( c=lug_out.vcargo.begin(); c!=lug_out.vcargo.end(); c++ ) {
   	      if ( c->point_arv == n->point_id ) //???
     	      break;
        }*/
      }
      else {
      	string airp_tmp = ElemIdToElemCtxt( ecDisp, etAirp, tr->airp, tr->airp_fmt );
      	if ( airp_tmp.size() > 3 )
          NewTextChild( NodeDK, "AV", tr->airp );
        else
        	NewTextChild( NodeDK, "AV", airp_tmp );
        NewTextChild( NodeDK, "DPP", GetStrDate( d->scd_in ) );
       	modf( d->scd_in, &tm );
       	NewTextChild( NodeDK, "VPP", GetMinutes( tm, d->scd_in ) );
       	NewTextChild( NodeDK, "DPR", GetStrDate( d->est_in ) );
       	modf( d->est_in, &tm );
       	NewTextChild( NodeDK, "VPR", GetMinutes( tm, d->est_in ) );
       	if ( d->act_in != NoExists )
       	  NewTextChild( NodeDK, "DPF", GetStrDate( d->act_in ) );
        else
          NewTextChild( NodeDK, "DPF", GetStrDate( d->scd_in ) );
       	modf( d->act_in, &tm );
       	NewTextChild( NodeDK, "VPF", GetMinutes( tm, d->act_in ) );

        NewTextChild( NodeDK, "PDV", GetStrDate( tr->scd_out ) );
       	modf( tr->scd_out, &tm );
       	NewTextChild( NodeDK, "PVV", GetMinutes( tm, tr->scd_out ) );
       	NewTextChild( NodeDK, "RDV", GetStrDate( tr->est_out ) );
       	modf( tr->est_out, &tm );
       	NewTextChild( NodeDK, "RVV", GetMinutes( tm, tr->est_out ) );
       	if ( tr->act_out != NoExists )
       	  NewTextChild( NodeDK, "FDV", GetStrDate( tr->act_out ) );
        else
          NewTextChild( NodeDK, "FDV", GetStrDate( tr->scd_out ) );
       	modf( tr->act_out, &tm );
       	NewTextChild( NodeDK, "FVV", GetMinutes( tm, tr->act_out ) );
/*        for ( c=lug_in.vcargo.begin(); c!=lug_in.vcargo.end(); c++ ) {
  	      if ( c->point_arv == tr->point_id )
     	      break;
        }*/
      }
      NewTextChild( NodeDK, "PUR", IntToString( k + 1 ) );
      NewTextChild( NodeDK, "PR", IntToString( 0 ) );
/*      if ( c!=lug_out.vcargo.end() && c!=lug_in.vcargo.end() ) {
      	//NewTextChild( NodeDK, "PC", IntToString( c->mail ) );
      	//NewTextChild( NodeDK, "GRU", IntToString( c->cargo ) );
      }
      else {
      	//NewTextChild( NodeDK, "PC", IntToString( 0 ) );
      	//NewTextChild( NodeDK, "GRU", IntToString( 0 ) );
      }*/
      //NewTextChild( NodeDK, "TPC", IntToString( 0 ) );
      //NewTextChild( NodeDK, "TGRU", IntToString( 0 ) );
    }; // end for
/*    for ( vector<Cargo>::iterator wc=lug_out.vcargo.begin(); wc!=lug_out.vcargo.end(); wc++ ) {
    	cargo_out += wc->cargo;
    	mail_out += wc->mail;
    } */
 	  //NewTextChild( NodeD, "PKZ", IntToString( lug_out.max_commerce ) );
 	  //NewTextChild( NodeD, "F9", IntToString( cargo_out ) );
 	  //NewTextChild( NodeD, "F11", IntToString( mail_out ) );
 	  NewTextChild( NodeD, "KUR", IntToString( tr->places_out.size() ) );
	}
}

string GetXMLRowValue( xmlNodePtr node )
{
	string res;
	node = node->children;
	while ( node ) {
		res += NodeAsString( node );
		node = node->next;
	}
	return res;
}

void createDBF( xmlDocPtr &sqldoc, xmlNodePtr xml_oldtrip, xmlNodePtr xml_newtrip, TDateTime iday, const string &type )
{
  string day = DateTimeToStr( iday, "dd" );
  ProgTrace( TRACE5, "xml_oldtrip=%p, xml-newtrip=%p, iday=%f, type=%s",xml_oldtrip,xml_newtrip, iday, type.c_str() );
  xmlNodePtr nodeP, nodeN;
  string sql_str;
  xmlNodePtr paramsNode;
  xmlNodePtr queryNode, sqlNode, rollbackNode;
  string dbf_type = type;
  tst();
  string tablename = string("SPP") + day + dbf_type;
  if ( xml_oldtrip )
    nodeP = GetNode( (char*)dbf_type.c_str(), xml_oldtrip );
  else
  	nodeP = 0;
  if ( xml_newtrip )
    nodeN = GetNode( (char*)dbf_type.c_str(), xml_newtrip );
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
   string old_val = GetXMLRowValue( nodeP );
   string new_val = GetXMLRowValue( nodeN );
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
 	  createSQLParam( paramsNode, "PNR", NodeAsString( "PNR", nodeP ), DBF_TYPE_CHAR );
 	  createSQLParam( paramsNode, "DN", NodeAsString( "DN", nodeP ), DBF_TYPE_DATE );
  };
 	if ( pr_insert )  {
		//insert прилет
    sql_str =
    string("INSERT INTO ") + tablename +
    "(PNR,KUG,TVC,BNP,NMSF,RPVSN,DN,PRIZ,KUR,VDV,VRD,ABSM,FAM) VALUES"
    "(:PNR,:KUG,:TVC,:BNP,:NMSF,:RPVSN,:DN,:PRIZ,:KUR,:VDV,:VRD,:ABSM,:FAM)";
  };
  if ( pr_update ) {
		// update if change
		sql_str =
      string("UPDATE ") + tablename +
      " SET KUG=:KUG,TVC=:TVC,BNP=:BNP,NMSF=:NMSF,RPVSN=:RPVSN,"
      "    PRIZ=:PRIZ,KUR=:KUR,VDV=:VDV,VRD=:VRD,ABSM=:ABSM,FAM=:FAM "
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
 	    createSQLParam( paramsNode, "PNR", NodeAsString( "PNR", nodeP ), DBF_TYPE_CHAR );
 	    createSQLParam( paramsNode, "DN", NodeAsString( "DN", nodeP ), DBF_TYPE_DATE );
 	  }
 	  else {
 	    createSQLParam( paramsNode, "PNR", NodeAsString( "PNR", nodeN ), DBF_TYPE_CHAR );
 	    createSQLParam( paramsNode, "DN", NodeAsString( "DN", nodeN ), DBF_TYPE_DATE );
 	  }
	  createSQLParam( paramsNode, "KUG", NodeAsString( "KUG", nodeN ), DBF_TYPE_CHAR );
    createSQLParam( paramsNode, "TVC", NodeAsString( "TVC", nodeN ), DBF_TYPE_CHAR );
   	createSQLParam( paramsNode, "BNP", NodeAsString( "BNP", nodeN ), DBF_TYPE_CHAR );
    createSQLParam( paramsNode, "NMSF", NodeAsString( "NMSF", nodeN ), DBF_TYPE_CHAR );
    createSQLParam( paramsNode, "RPVSN", NodeAsString( "RPVSN", nodeN ), DBF_TYPE_CHAR );
   	createSQLParam( paramsNode, "PRIZ", NodeAsString( "PRIZ", nodeN ), DBF_TYPE_NUMBER );
    //createSQLParam( paramsNode, "PKZ", NodeAsString( "PKZ", nodeN ), DBF_TYPE_NUMBER );
    //createSQLParam( paramsNode, "F9", NodeAsString( "F9", nodeN ),  DBF_TYPE_NUMBER );
    //createSQLParam( paramsNode, "F11", NodeAsString( "F11", nodeN ), DBF_TYPE_NUMBER );
    createSQLParam( paramsNode, "KUR", NodeAsString( "KUR", nodeN ), DBF_TYPE_NUMBER );
   	createSQLParam( paramsNode, "VDV", NodeAsString( "VDV", nodeN ), DBF_TYPE_CHAR );
   	createSQLParam( paramsNode, "VRD", NodeAsString( "VRD", nodeN ), DBF_TYPE_CHAR );
   	createSQLParam( paramsNode, "ABSM", NodeAsString( "ABSM", nodeN ), DBF_TYPE_CHAR );
   	createSQLParam( paramsNode, "FAM", NodeAsString( "FAM", nodeN ), DBF_TYPE_CHAR );
  }
  if ( type == "A" )
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
      string old_val = GetXMLRowValue( *nodePK );
      string new_val = GetXMLRowValue( *nodeNK );
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
 	    createSQLParam( paramsNode, "PNR", NodeAsString( "PNR", *nodePK ), DBF_TYPE_CHAR );
 	    createSQLParam( paramsNode, "DPP", NodeAsString( "DPP", *nodePK ), DBF_TYPE_DATE );
      createSQLParam( paramsNode, "PUR", NodeAsString( "PUR", *nodePK ), DBF_TYPE_NUMBER );
    }
    if ( pr_insert ) {
      sql_str =
        string("INSERT INTO ") + tablename +
        "(PNR,AV,AP,DPP,VPP,DPR,VPR,DPF,VPF,PDV,PVV,RDV,RVV,FDV,FVV,PUR,SPUR,PR) "
        " VALUES(:PNR,:AV,:AP,:DPP,:VPP,:DPR,:VPR,:DPF,:VPF,:PDV,:PVV,:RDV,:RVV,:FDV,:FVV,:PUR,:SPUR,:PR)";
    }
    if ( pr_update ) {
      sql_str =
        string("UPDATE ") + tablename +
        " SET "
        "AV=:AV,AP=:AP,DPP=:DPP,VPP=:VPP,DPR=:DPR,VPR=:VPR,DPF=:DPF,VPF=:VPF,PDV=:PDV,PVV=:PVV,RDV=:RDV,"
        "RVV=:RVV,FDV=:FDV,FVV=:FVV,PUR=:PUR,SPUR=:SPUR,PR=:PR "
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
 	      createSQLParam( paramsNode, "PNR", NodeAsString( "PNR", *nodePK ), DBF_TYPE_CHAR );
 	      createSQLParam( paramsNode, "DPP", NodeAsString( "DPP", *nodePK ), DBF_TYPE_DATE );
 	      createSQLParam( paramsNode, "PUR", NodeAsString( "PUR", *nodePK ), DBF_TYPE_NUMBER );
 	      createSQLParam( paramsNode, "SPUR", (string)" "+NodeAsString( "PUR", *nodePK ), DBF_TYPE_CHAR );
 	    }
 	    else {
 	      createSQLParam( paramsNode, "PNR", NodeAsString( "PNR", *nodeNK ), DBF_TYPE_CHAR );
 	      createSQLParam( paramsNode, "DPP", NodeAsString( "DPP", *nodeNK ), DBF_TYPE_DATE );
 	      createSQLParam( paramsNode, "PUR", NodeAsString( "PUR", *nodeNK ), DBF_TYPE_NUMBER );
 	      createSQLParam( paramsNode, "SPUR", (string)" "+NodeAsString( "PUR", *nodeNK ), DBF_TYPE_CHAR );
 	    }
      createSQLParam( paramsNode, "AV", NodeAsString( "AV", *nodeNK ), DBF_TYPE_CHAR );
      createSQLParam( paramsNode, "AP", NodeAsString( "AP", *nodeNK ), DBF_TYPE_CHAR );
     	createSQLParam( paramsNode, "VPP", NodeAsString( "VPP", *nodeNK ), DBF_TYPE_NUMBER );
     	createSQLParam( paramsNode, "DPR", NodeAsString( "DPR", *nodeNK ), DBF_TYPE_DATE );
     	createSQLParam( paramsNode, "VPR", NodeAsString( "VPR", *nodeNK ), DBF_TYPE_NUMBER );
     	createSQLParam( paramsNode, "DPF", NodeAsString( "DPF", *nodeNK ), DBF_TYPE_DATE );
     	createSQLParam( paramsNode, "VPF", NodeAsString( "VPF", *nodeNK ), DBF_TYPE_NUMBER );

     	createSQLParam( paramsNode, "PDV", NodeAsString( "PDV", *nodeNK ), DBF_TYPE_DATE );
     	createSQLParam( paramsNode, "PVV", NodeAsString( "PVV", *nodeNK ), DBF_TYPE_NUMBER );
     	createSQLParam( paramsNode, "RDV", NodeAsString( "RDV", *nodeNK ), DBF_TYPE_DATE );
     	createSQLParam( paramsNode, "RVV", NodeAsString( "RVV", *nodeNK ), DBF_TYPE_NUMBER );
     	createSQLParam( paramsNode, "FDV", NodeAsString( "FDV", *nodeNK ), DBF_TYPE_DATE );
     	createSQLParam( paramsNode, "FVV", NodeAsString( "FVV", *nodeNK ), DBF_TYPE_NUMBER );
      createSQLParam( paramsNode, "PR", NodeAsString( "PR", *nodeNK ), DBF_TYPE_CHAR );
      //createSQLParam( paramsNode, "PC", NodeAsString( "PC", *nodeNK ), DBF_TYPE_NUMBER );
      //createSQLParam( paramsNode, "TPC", NodeAsString( "TPC", *nodeNK ), DBF_TYPE_NUMBER );
      //createSQLParam( paramsNode, "GRU", NodeAsString( "GRU", *nodeNK ), DBF_TYPE_NUMBER );
      //createSQLParam( paramsNode, "TGRU", NodeAsString( "TGRU", *nodeNK ), DBF_TYPE_NUMBER );
    }

    if ( nodePK != nodesPK.end() )
  		nodePK++;
  	if ( nodeNK != nodesNK.end() )
  		nodeNK++;
  } // end while
}

bool validateFlight( bool pr_in, TDateTime spp_date, const TSOPPTrips::iterator &tr )
{
 	try {
 	if ( pr_in && getPNRParam( tr->airline_in, tr->airline_in_fmt, tr->flt_no_in, tr->suffix_in, tr->suffix_in_fmt, getRemoveSuffix( spp_date, tr->scd_in ) ).size() > 7 ||
  	  !pr_in && getPNRParam( tr->airline_out, tr->airline_out_fmt, tr->flt_no_out, tr->suffix_out, tr->suffix_out_fmt, getRemoveSuffix( spp_date, tr->scd_out ) ).size() > 7 )
      return false;
  }
  catch(...) {
   return false;
  }
  return true;
}

struct TTripDay {
  string type;
  TDateTime spp_date;
  xmlNodePtr xml_oldtrip;
  xmlNodePtr xml_newtrip;
  TTripDay() {
    spp_date = NoExists;
    xml_oldtrip = 0;
    xml_newtrip = 0;
  }
};

bool equalTrip( const xmlNodePtr &xml_oldtrip, const xmlNodePtr &xml_newtrip )
{
  bool res = true;
  xmlDocPtr doc1, doc2;
  doc1 = CreateXMLDoc( "UTF-8", "trip" );
  doc2 = CreateXMLDoc( "UTF-8", "trip" );
  tst();
  try {
    CopyNode( doc1->children, xml_oldtrip  );
    CopyNode( doc2->children, xml_newtrip );
    res = ( XMLTreeToText( doc1 ) == XMLTreeToText( doc2 ) );
  }
  catch( ... ) {
    tst();
    xmlFreeDoc( doc1 );
    xmlFreeDoc( doc2 );
    throw;
  }
  ProgTrace( TRACE5, "res=%d", res );
  xmlFreeDoc( doc1 );
  xmlFreeDoc( doc2 );
  return res;
}

void GetNotEqualTrips( const xmlNodePtr &xml_oldtrip, const xmlNodePtr &xml_newtrip, bool pr_revert, vector<TTripDay> &xmltrips )
{
  xmlNodePtr xml_old = xml_oldtrip, xml_new;
  while ( xml_old != NULL ) {
    TTripDay trip;
    trip.spp_date = NodeAsDateTime( "@spp_date", xml_old );
    trip.type = NodeAsString( "@type", xml_old );
    trip.xml_oldtrip = xml_old;
    xml_new = xml_newtrip;
    while ( xml_new != NULL ) {
      if ( trip.type == NodeAsString( "@type", xml_new ) &&
           trip.spp_date == NodeAsDateTime( "@spp_date", xml_new ) ) {
        if ( !pr_revert && !equalTrip( xml_old, xml_new ) )
          trip.xml_newtrip = xml_new;
        else
          trip.xml_oldtrip = NULL;
        break;
      }
      xml_new = xml_new->next;
    }
    if ( trip.xml_oldtrip != NULL ) {
      vector<TTripDay>::iterator i=xmltrips.end();
      for ( i=xmltrips.begin(); i!=xmltrips.end(); i++ ) {
        if ( !pr_revert &&
             i->xml_oldtrip == trip.xml_oldtrip &&
             i->xml_newtrip == trip.xml_newtrip ||
             pr_revert &&
             i->xml_oldtrip == trip.xml_newtrip &&
             i->xml_newtrip == trip.xml_oldtrip )
         break;
      }
      if ( i == xmltrips.end() ) {
        if ( pr_revert ) {
          xml_new = trip.xml_oldtrip;
          trip.xml_oldtrip = trip.xml_newtrip;
          trip.xml_newtrip = xml_new;
        }
        xmltrips.push_back( trip );
      }
    }
    xml_old = xml_old->next;
  }
}

bool equalTrips( const xmlDocPtr &old_doc, const xmlDocPtr &new_doc, vector<TTripDay> &xmltrips )
{
  xmltrips.clear();
  xmlNodePtr xml_oldtrip, xml_newtrip;
  if ( old_doc ) {
    ProgTrace( TRACE5, "old_doc=%p", old_doc );
    xml_oldtrip = GetNode( "trip", old_doc->children );
    if ( !xml_oldtrip )
      return false;
  }
  else {
    tst();
    xml_oldtrip = NULL;
  }
  if ( new_doc ) {
    tst();
    xml_newtrip = GetNode( "trip", new_doc->children );
  }
  else
    xml_newtrip = NULL;
  tst();
  GetNotEqualTrips( xml_oldtrip, xml_newtrip, false, xmltrips );
  GetNotEqualTrips( xml_newtrip, xml_oldtrip, true, xmltrips );
  // xmltrips содержит только изменения
  ProgTrace( TRACE5, "xmltrips.size()=%zu", xmltrips.size() );
  return xmltrips.empty();
}

void createSQLs( const vector<TTripDay> &xmltrips, xmlDocPtr &sqldoc )
{
  for( vector<TTripDay>::const_iterator iv=xmltrips.begin(); iv!=xmltrips.end(); iv++ ) {
    ProgTrace( TRACE5, "iv->xml_oldtrip=%p, iv->xml_newtrip=%p", iv->xml_oldtrip, iv->xml_newtrip );
    if ( iv->xml_oldtrip )
      ProgTrace( TRACE5, "iv->xml_oldtrip->name=%s", (char*)iv->xml_oldtrip->name );
    if ( iv->xml_newtrip )
      ProgTrace( TRACE5, "iv->xml_newtrip->name=%s", (char*)iv->xml_newtrip->name );
    createDBF( sqldoc, iv->xml_oldtrip, iv->xml_newtrip, iv->spp_date, iv->type );
  }
}

bool createSPPCEKFile( int point_id, const string &point_addr, TFileDatas &fds )
{
	ProgTrace( TRACE5, "CEK point_id=%d", point_id );
	TReqInfo *reqInfo = TReqInfo::Instance();
	reqInfo->user.sets.time = ustTimeUTC;
	reqInfo->Initialize("ЧЛБ");
  reqInfo->user.user_type = utAirport;
  reqInfo->user.access.airps.push_back( "ЧЛБ" );
  reqInfo->user.access.airps_permit = true;

	string file_type = FILE_SPPCEK_TYPE;
	string record;
	TQuery Qry( &OraSession );
	TQuery COMMANDERQry( &OraSession );
	COMMANDERQry.SQLText =
	  "SELECT commander from trip_crew WHERE point_id=:point_id";
	COMMANDERQry.DeclareVariable( "point_id", otInteger );
 	TDateTime UTCNow = NowUTC();
 	modf( UTCNow, &UTCNow );
 	vector<TDateTime> spp_days;

 	/* проверка на существование таблиц */
 	for ( int max_day=0; max_day<=CREATE_SPP_DAYS(); max_day++ ) {
	  createSPPCEK( (int)UTCNow + max_day, file_type, point_addr, fds );
	  spp_days.push_back( UTCNow + max_day );
	}
	TFileData fd;
  TSOPPTrips trips;
  createSOPPTrip( point_id, trips );
  tst();
  xmlDocPtr doc = 0, old_doc = 0;
  std::string errcity;
  //создаем рейс по датам
  bool pr_remove_in = false, pr_remove_out = false;
  for ( TSOPPTrips::iterator tr=trips.begin(); tr!=trips.end(); tr++ ) {
    if ( tr->point_id != point_id ) continue;
    COMMANDERQry.SetVariable( "point_id", point_id );
    COMMANDERQry.Execute();
    string commander;
    if ( !COMMANDERQry.Eof )
      commander = COMMANDERQry.FieldAsString( "commander" );
    tst();
    for ( vector<TDateTime>::reverse_iterator iday=spp_days.rbegin(); iday!=spp_days.rend(); iday++ ) { // пробег по всем дням СПП
      //!!!error
      ProgTrace( TRACE5, "tr->scd_out=%f, tr->est_out=%f, iday=%f", tr->scd_out, tr->est_out, *iday );
      bool canuseTR = false;
      if ( !canuseTR && tr->act_in != NoExists )
      	canuseTR = filter_time( tr->act_in, *tr, *iday, *iday + 1, errcity );
/*      if ( !canuseTR && tr->est_in != NoExists )
        canuseTR = filter_time( tr->est_in, *tr, *iday, *iday + 1, errcity );
*/
      if ( !canuseTR && tr->scd_in != NoExists )
        canuseTR = filter_time( tr->scd_in, *tr, *iday, *iday + 1, errcity );
      if ( canuseTR &&
           !tr->places_in.empty() &&
           tr->scd_in != NoExists &&
           validateFlight( true, *iday, tr ) ) { // фильтр по датам прилета-вылета рейса
        createXMLTrips( "A", pr_remove_in, *iday, tr, commander, doc );
        pr_remove_in = true;
      }
      canuseTR = false;
      if ( !canuseTR && tr->act_out != NoExists )
        canuseTR = filter_time( tr->act_out, *tr, *iday, *iday + 1, errcity );
/*      if ( !canuseTR && tr->est_out != NoExists )
        canuseTR = filter_time( tr->est_out, *tr, *iday, *iday + 1, errcity
);
*/
      if ( !canuseTR && tr->scd_out != NoExists )
        canuseTR = filter_time( tr->scd_out, *tr, *iday, *iday + 1, errcity );
      if ( canuseTR &&
           !tr->places_out.empty() &&
           tr->scd_out != NoExists &&
           validateFlight( false, *iday, tr ) ) { // фильтр по датам прилета-вылета рейса
        createXMLTrips( "D", pr_remove_out, *iday, tr, commander, doc );
        pr_remove_out = true;
      }
    }
    break;
  }
  // если ничего не создали, то считаем, что рейс неправильный и его не надо передавать
  if ( !doc && !trips.empty() )
  	return !fds.empty();
  get_string_into_snapshot_points( point_id, file_type, point_addr, record );
  ProgTrace( TRACE5, "get_string_into_snapshot_points: point_id=%d,record=%s", point_id, record.c_str() );
  if ( !record.empty() ) {
 		record.replace( record.find( "encoding=\"UTF-8\""), string( "encoding=\"UTF-8\"" ).size(), string("encoding=\"") + "CP866" + "\"" );
    old_doc = TextToXMLTree( record );
	  if ( old_doc ) {
      xmlFree(const_cast<xmlChar *>(old_doc->encoding));
      old_doc->encoding = 0;
      xml_decode_nodelist( old_doc->children );
		}
  }

  vector<TTripDay> xmltrips;
  if ( equalTrips( old_doc, doc, xmltrips ) ) { // нет изменений по рейсу
    tst();
    if ( doc )
      xmlFreeDoc( doc );
    if ( old_doc )
      xmlFreeDoc( old_doc );
    return !fds.empty();
  }

  xmlDocPtr sqldoc = 0;
  createSQLs( xmltrips, sqldoc );

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
  string snapshot;
  if ( doc )
   snapshot = XMLTreeToText( doc );
  put_string_into_snapshot_points( point_id, FILE_SPPCEK_TYPE, point_addr, old_doc, snapshot );
  tst();
  //ProgTrace( TRACE5, "doc=%p, old_doc=%p", doc, old_doc );
  if ( doc )
  	xmlFreeDoc( doc );
  if ( old_doc ) {
  	xmlFreeDoc( old_doc );
  }
	return !fds.empty();
}

struct TField1C {
	string field_name;
	string dbf_field_name;
	string dbf_field_type;
	TField1C(string vfield_name, string vdbf_field_name, string vdbf_field_type) {
	  field_name = vfield_name;
	  dbf_field_name =vdbf_field_name;
	  dbf_field_type = vdbf_field_type;
	};
	TField1C(string vfield_name, string vdbf_field_type) {
	  field_name = vfield_name;
	  dbf_field_name = vfield_name;
	  dbf_field_type = vdbf_field_type;
	};
};

struct Table1C{
	string name;
	vector<TField1C> fields;
	string create_sql;
	string delete_sql;
	string insert_sql;
	void clear() {
		name.clear();
		fields.clear();
		create_sql.clear();
		delete_sql.clear();
		insert_sql.clear();
	}
};

bool Sync1C( const string &point_addr, TFileDatas &fds )
{
	vector<Table1C> tables;
	Table1C tab;
	tab.name = "airps";
	tab.create_sql =
	  "CREATE TABLE AIRPS(ID NUMERIC(9,0),CODE CHAR(3),CODE_LAT CHAR(3),CITY CHAR(3),NAME CHAR(50),NAME_LAT CHAR(50),ICAO CHAR(4),ICAO_LAT CHAR(4))";
	tab.delete_sql =
	  "DELETE FROM AIRPS WHERE ID=:ID";
	tab.insert_sql =
	  "INSERT INTO AIRPS(ID,CODE,CODE_LAT,CITY,NAME,NAME_LAT,ICAO,ICAO_LAT) "
	  " VALUES (:ID,:CODE,:CODE_LAT,:CITY,:NAME,:NAME_LAT,:ICAO,:ICAO_LAT) ";
  tab.fields.push_back(TField1C("ID",DBF_TYPE_NUMBER));
	tab.fields.push_back(TField1C("CODE",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CITY",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("NAME",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("NAME_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_ICAO","ICAO",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_ICAO_LAT","ICAO_LAT",DBF_TYPE_CHAR));
	tables.push_back( tab );
	tab.clear();
	tab.name = "cities";
	tab.create_sql =
	  "CREATE TABLE CITIES(ID NUMERIC(9,0),CODE CHAR(3),CODE_LAT CHAR(3),COUNTRY CHAR(2),NAME CHAR(50),NAME_LAT CHAR(50),TZ NUMERIC(3,0))";
	tab.delete_sql =
	  "DELETE FROM CITIES WHERE ID=:ID";
	tab.insert_sql =
	  "INSERT INTO CITIES(ID,CODE,CODE_LAT,COUNTRY,NAME,NAME_LAT,TZ) "
	  " VALUES (:ID,:CODE,:CODE_LAT,:COUNTRY,:NAME,:NAME_LAT,:TZ) ";
  tab.fields.push_back(TField1C("ID",DBF_TYPE_NUMBER));
	tab.fields.push_back(TField1C("CODE",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("COUNTRY",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("NAME",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("NAME_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("TZ",DBF_TYPE_NUMBER));
	tables.push_back( tab );
	tab.clear();
	tab.name = "crafts";
	tab.create_sql =
	  "CREATE TABLE CRAFTS(ID NUMERIC(9,0),CODE CHAR(3),CODE_LAT CHAR(3),NAME CHAR(20),NAME_LAT CHAR(20),ICAO CHAR(4), ICAO_LAT CHAR(4))";
	tab.delete_sql =
	  "DELETE FROM CRAFTS WHERE ID=:ID";
	tab.insert_sql =
	  "INSERT INTO CRAFTS(ID,CODE,CODE_LAT,NAME,NAME_LAT,ICAO,ICAO_LAT) "
	  " VALUES (:ID,:CODE,:CODE_LAT,:NAME,:NAME_LAT,:ICAO,:ICAO_LAT) ";
  tab.fields.push_back(TField1C("ID",DBF_TYPE_NUMBER));
	tab.fields.push_back(TField1C("CODE",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("NAME",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("NAME_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_ICAO","ICAO",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_ICAO_LAT","ICAO_LAT",DBF_TYPE_CHAR));
	tables.push_back( tab );
	tab.clear();
	tab.name = "airlines";
	tab.create_sql =
	  "CREATE TABLE AIRLINES(ID NUMERIC(9,0),CODE CHAR(3),CODE_LAT CHAR(3),NAME CHAR(50),NAME_LAT CHAR(50),SHORT CHAR(50),SHORT_LAT CHAR(50),ICAO CHAR(4),ICAO_LAT CHAR(4),AIRCODE CHAR(3),CITY CHAR(3))";
	tab.delete_sql =
	  "DELETE FROM AIRLINES WHERE ID=:ID";
	tab.insert_sql =
	  "INSERT INTO AIRLINES(ID,CODE,CODE_LAT,NAME,NAME_LAT,SHORT,SHORT_LAT,ICAO,ICAO_LAT,AIRCODE,CITY) "
	  " VALUES (:ID,:CODE,:CODE_LAT,:NAME,:NAME_LAT,:SHORT,:SHORT_LAT,:ICAO,:ICAO_LAT,:AIRCODE,:CITY) ";
  tab.fields.push_back(TField1C("ID",DBF_TYPE_NUMBER));
	tab.fields.push_back(TField1C("CODE",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("NAME",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("NAME_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("SHORT_NAME","SHORT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("SHORT_NAME_LAT","SHORT_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_ICAO","ICAO",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_ICAO_LAT","ICAO_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("AIRCODE",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CITY",DBF_TYPE_CHAR));
	tables.push_back( tab );
	tab.clear();
	tab.name = "countries";
	tab.create_sql =
	  "CREATE TABLE COUNTRIES(ID NUMERIC(9,0),CODE CHAR(2),CODE_LAT CHAR(2),NAME CHAR(50),NAME_LAT CHAR(50),CODE_ISO CHAR(3))";
	tab.delete_sql =
	  "DELETE FROM COUNTRIES WHERE ID=:ID";
	tab.insert_sql =
	  "INSERT INTO COUNTRIES(ID,CODE,CODE_LAT,NAME,NAME_LAT,CODE_ISO) "
	  " VALUES (:ID,:CODE,:CODE_LAT,:NAME,:NAME_LAT,:CODE_ISO) ";
  tab.fields.push_back(TField1C("ID",DBF_TYPE_NUMBER));
	tab.fields.push_back(TField1C("CODE",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("NAME",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("NAME_LAT",DBF_TYPE_CHAR));
	tab.fields.push_back(TField1C("CODE_ISO",DBF_TYPE_CHAR));
	tables.push_back( tab );


  TQuery Qry( &OraSession );
  for ( vector<Table1C>::iterator i = tables.begin(); i!=tables.end(); i++ ) {
  	ProgTrace( TRACE5, "table name=%s", i->name.c_str() );
  	Qry.Clear();
    Qry.SQLText =
      "SELECT value FROM snapshot_params "
      " WHERE file_type=:file_type AND point_addr=:point_addr AND name=:table_name";
    Qry.CreateVariable( "file_type", otString, FILE_1CCEK_TYPE );
    Qry.CreateVariable( "point_addr", otString, point_addr );
    Qry.DeclareVariable( "table_name", otString );
  	Qry.SetVariable( "table_name", i->name );
    Qry.Execute();
    int tid = -1, id = -1;
    bool pr_new_table = false;
    if ( Qry.Eof ) {  // создаем таблицу
    	pr_new_table = true;
    	TFileData fd;
	    xmlDocPtr doc = CreateXMLDoc( "UTF-8", "sqls" );
    	try {
	      xmlNodePtr queryNode = NewTextChild( doc->children, "query" );
        NewTextChild( queryNode, "sql", i->create_sql );
        NewTextChild( queryNode, "ignoreErrorCode", 5004 );
  	    string encoding = getFileEncoding( FILE_1CCEK_TYPE, point_addr );
 		    fd.params[ PARAM_TYPE ] = VALUE_TYPE_SQL; // SQL
   	    string s = XMLTreeToText( doc );
   	    s.replace( s.find( "encoding=\"UTF-8\""), string( "encoding=\"UTF-8\"" ).size(), string("encoding=\"") + encoding + "\"" );
   	    fd.file_data = s;
   	    fds.push_back( fd );
   	    xmlFreeDoc( doc );
	    }
    	catch ( ... ) {
		    xmlFreeDoc( doc );
		    throw;
	    }
    }
    else {
    	string param = Qry.FieldAsString( "value" );
    	StrToInt( param.substr( 0, param.find( "id" ) ).c_str(), tid );
    	StrToInt( param.substr( param.find( "id" ) + 2 ).c_str(), id );
    }
    TFileData fd;
    Qry.Clear();
    // передача обновлений
    Qry.SQLText = string(string( "SELECT * FROM " ) + i->name + " WHERE tid>:tid OR tid=:tid AND id>:id ORDER BY tid,id ").c_str();
    Qry.CreateVariable( "tid", otInteger, tid );
    Qry.CreateVariable( "id", otInteger, id );
    Qry.Execute();
    if ( Qry.Eof )
    	continue;
    xmlDocPtr doc = CreateXMLDoc( "UTF-8", "sqls" );
    xmlNodePtr queryNode;
    xmlNodePtr paramsNode;
    try {
      while ( !Qry.Eof ) { // считали все изменение из таблицы
      	if ( !pr_new_table ) {
      	  queryNode = NewTextChild( doc->children, "query" );
      	  NewTextChild( queryNode, "sql", i->delete_sql );
      	  paramsNode = NewTextChild( queryNode, "params" );
          createSQLParam( paramsNode, "ID", Qry.FieldAsString( "ID" ), DBF_TYPE_NUMBER );
        }
        if ( Qry.FieldAsInteger( "pr_del" ) != -1 ) {
      	  queryNode = NewTextChild( doc->children, "query" );
      	  NewTextChild( queryNode, "sql", i->insert_sql );
        	paramsNode = NewTextChild( queryNode, "params" );
        	for ( vector<TField1C>::iterator ifield=i->fields.begin(); ifield!=i->fields.end(); ifield++ ) {
            createSQLParam( paramsNode, ifield->dbf_field_name, Qry.FieldAsString( ifield->field_name ), ifield->dbf_field_type );
          }
        }
        if ( tid < Qry.FieldAsInteger( "tid" ) ) {
        	tid = Qry.FieldAsInteger( "tid" );
        }
        if ( id < Qry.FieldAsInteger( "id" ) ) {
        	id = Qry.FieldAsInteger( "id" );
        }
        if ( Qry.RowCount() > 1000 )
        	break;
      	Qry.Next();
      }
  	  string encoding = getFileEncoding( FILE_1CCEK_TYPE, point_addr );
 		  fd.params[ PARAM_TYPE ] = VALUE_TYPE_SQL; // SQL
   	  string s = XMLTreeToText( doc );
   	  s.replace( s.find( "encoding=\"UTF-8\""), string( "encoding=\"UTF-8\"" ).size(), string("encoding=\"") + encoding + "\"" );
   	  fd.file_data = s;
   	  fds.push_back( fd );
   	  xmlFreeDoc( doc );
    }
    catch(...) {
		  xmlFreeDoc( doc );
		  throw;
    }
    Qry.Clear();
    if ( pr_new_table )
      Qry.SQLText =
        "INSERT INTO snapshot_params(file_type,point_addr,name,value) VALUES(:file_type,:point_addr,:name,:value)";
    else
      Qry.SQLText =
        "UPDATE snapshot_params SET value=:value "
        " WHERE file_type=:file_type AND point_addr=:point_addr AND name=:name";
    Qry.CreateVariable( "file_type", otString, FILE_1CCEK_TYPE );
    Qry.CreateVariable( "point_addr", otString, point_addr );
    Qry.CreateVariable( "name", otString, i->name );
    Qry.CreateVariable( "value", otString, IntToString( tid ) + "id" + IntToString( id ) );
    Qry.Execute();
  } // end for
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
