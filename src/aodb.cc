/*
create table aodb_pax( point_id NUMBER(9), pax_id NUMBER(9), record VARCHAR2(2000) );
create table aodb_bag( pax_id NUMBER(9), num NUMBAR(3), record VARCHAR2(2000) );
INSERT INTO file_types(code,name,in_order) VALUES( "AODB", "AODB", 1 );
INSERT INTO file_param_sets
alter table aodb_bag add pr_cabin NUMBER(1) NOT NULL;
*/
#include "aodb.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include "base_tables.h"

#include "exceptions.h"
#include "basic.h"
#include "stl_utils.h"
#include "base_tables.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_service.h"
#include "misc.h"
#include "astra_misc.h"
#include "stages.h"
#include "tripinfo.h"
#include "salons.h"
#include "sopp.h"
#include "points.h"
#include "serverlib/helpcpp.h"
#include "tlg/tlg.h"
#include "flt_binding.h"
#include "astra_misc.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/slogger.h"


#include "serverlib/query_runner.h"
#include "serverlib/posthooks.h"
#include "serverlib/ourtime.h"


using namespace BASIC;
using namespace EXCEPTIONS;

#define WAIT_INTERVAL           60000      //�����ᥪ㭤�


using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

enum TAODBFormat { afDefault, afNewUrengoy };

struct AODB_STRUCT{
	int pax_id;
	int reg_no;
	int num;
	int pr_cabin;
	bool unaccomp;
	bool doit;
	string record;
	int checkin_time_pos;
	int length_checkin_time;
	int brd_time_pos;
  int length_brd_time;
  AODB_STRUCT() {
	  checkin_time_pos = -1;
	  length_checkin_time = 0;
	  brd_time_pos = -1;
    length_brd_time = 0;
  }
};

struct AODB_Dest {
	int num; //1
	std::string airp; //3
	int pr_del; //1
};

struct AODB_Term {
	std::string type; //1
	std::string name; //4
	int pr_del; //1
};

const
  unsigned int REC_NO_IDX = 0;
  unsigned int REC_NO_LEN = 6;
  unsigned int FLT_ID_IDX = 6;
  unsigned int FLT_ID_LEN = 10;
  unsigned int TRIP_IDX = 16;
  unsigned int TRIP_LEN = 10;
  unsigned int LITERA_IDX = 26;
  unsigned int LITERA_LEN = 3;
  unsigned int SCD_IDX = 29;
  unsigned int SCD_LEN = 16;
  unsigned int EST_IDX = 45;
  unsigned int EST_LEN = 16;
  unsigned int ACT_IDX = 61;
  unsigned int ACT_LEN = 16;
  unsigned int HALL_IDX = 77;
  unsigned int HALL_LEN = 2;
  unsigned int PARK_OUT_IDX = 79;
  unsigned int PARK_OUT_LEN = 5;
  unsigned int KRM_IDX = 84;
  unsigned int KRM_LEN = 3;
  unsigned int MAX_LOAD_IDX = 87;
  unsigned int MAX_LOAD_LEN = 6;
  unsigned int CRAFT_IDX = 93;
  unsigned int CRAFT_LEN = 10;
  unsigned int BORT_IDX = 103;
  unsigned int BORT_LEN = 10;
  unsigned int CHECKIN_BEG_IDX = 113;
  unsigned int CHECKIN_BEG_LEN = 16;
  unsigned int CHECKIN_END_IDX = 129;
  unsigned int CHECKIN_END_LEN = 16;
  unsigned int BOARDING_BEG_IDX = 145;
  unsigned int BOARDING_BEG_LEN = 16;
  unsigned int BOARDING_END_IDX = 161;
  unsigned int BOARDING_END_LEN = 16;
  unsigned int PR_CANCEL_IDX = 177;
  unsigned int PR_CANCEL_LEN = 1;
  unsigned int PR_DEL_IDX = 178;
  unsigned int PR_DEL_LEN = 1;

struct AODB_Flight {
	int rec_no; //6
	double id;	//10
	std::string airline; //10
	int flt_no;
	std::string suffix;
	std::string litera; //3
	std::string trip_type;
	TDateTime scd; //16
	TDateTime est; //16
	TDateTime act; //16
	std::string hall; //2
	std::string park_out;
	int krm; //3
	int max_load; //6
	std::string craft; //10
	std::string bort; //10
	TDateTime checkin_beg; //16
	TDateTime checkin_end; //16
	TDateTime boarding_beg; //16
	TDateTime boarding_end; //16
	int pr_cancel; //1
	int pr_del; //1
	vector<AODB_Dest> dests;
	vector<AODB_Term> terms;
	string invalid_field;
};

void getRecord( int pax_id, int reg_no, bool pr_unaccomp, const vector<AODB_STRUCT> &aodb_pax,
                const vector<AODB_STRUCT> &aodb_bag,
                string &res_checkin );
void createRecord( int point_id, int pax_id, int reg_no, const string &point_addr, bool pr_unaccomp,
                   const string unaccomp_header,
                   vector<AODB_STRUCT> &aodb_pax, vector<AODB_STRUCT> &aodb_bag,
                   vector<AODB_STRUCT> &prior_aodb_pax, vector<AODB_STRUCT> &prior_aodb_bag,
                   string &res_checkin );

string getRegion( const string &airp )
{
  string city =((TAirpsRow&)base_tables.get("airps").get_row( "code", airp, true )).city;
  return ((TCitiesRow&)base_tables.get("cities").get_row( "code", city, true )).region;
}

// �ਢ離� � ������ ३��
void bindingAODBFlt( const std::string &point_addr, int point_id, float aodb_point_id )
{
  ProgTrace( TRACE5, "bindingAODBFlt: point_addr=%s, point_id=%d, aodb_point_id=%f",
             point_addr.c_str(), point_id, aodb_point_id );
  vector<string> strs;
  vector<int> points;
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT point_addr,aodb_point_id, point_id FROM aodb_points "
                " WHERE point_addr=:point_addr AND aodb_point_id=:aodb_point_id AND point_id!=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.CreateVariable( "aodb_point_id", otFloat, aodb_point_id );
  Qry.Execute();
  while ( !Qry.Eof ) {
    string str = string( "point_addr=" ) +  Qry.FieldAsString("point_addr") +
                 ",point_id="+Qry.FieldAsString( "point_id" ) +",aodb_point_id=" +  Qry.FieldAsString( "aodb_point_id" );
    points.push_back( Qry.FieldAsInteger( "point_id" ) );
    strs.push_back( str );
    Qry.Next();
  }
	try {
    Qry.Clear();
	  Qry.SQLText =
	    "BEGIN "
	    " DELETE aodb_bag WHERE point_addr=:point_addr AND "
      "   pax_id IN ( SELECT pax_id FROM aodb_pax WHERE point_id=:point_id AND point_addr=:point_addr ); "
      " DELETE aodb_unaccomp WHERE point_addr=:point_addr AND point_id=:point_id; "
	    " DELETE aodb_pax WHERE point_addr=:point_addr AND point_id=:point_id; "
	    " DELETE aodb_points WHERE point_addr=:point_addr AND point_id=:point_id; "
	    "END;";
	  Qry.CreateVariable( "point_addr", otString, point_addr );
	  Qry.DeclareVariable( "point_id", otInteger );
  	for ( vector<int>::iterator i=points.begin(); i!=points.end(); i++ ) {
      Qry.SetVariable( "point_id", *i );
      Qry.Execute();
	  }
    Qry.Clear();
	  Qry.SQLText =
	   "BEGIN "
	   " UPDATE aodb_points "
	   " SET aodb_point_id=:aodb_point_id "
	   " WHERE point_id=:point_id AND point_addr=:point_addr; "
	   " IF SQL%NOTFOUND THEN "
	   "  INSERT INTO aodb_points(aodb_point_id,point_addr,point_id,rec_no_pax,rec_no_bag,rec_no_flt,rec_no_unaccomp,overload_alarm) "
	   "    VALUES(:aodb_point_id,:point_addr,:point_id,-1,-1,-1,-1,0);"
	   " END IF; "
	   "END;";
	  Qry.CreateVariable( "point_id", otInteger, point_id );
	  Qry.CreateVariable( "point_addr", otString, point_addr );
  	Qry.CreateVariable( "aodb_point_id", otFloat, aodb_point_id );
	  Qry.Execute();
  }
  catch(EOracleError &E) {  // deadlock!!!
     ProgError( STDLOG, "bindingAODBFlt EOracleError:" );
     try {
       for ( vector<string>::iterator i=strs.begin(); i!=strs.end(); i++ ) {
         ProgTrace( TRACE5, "%s", i->c_str() );
       }
     }
     catch(...){};
     throw;
  }
}

void bindingAODBFlt( const std::string &airline, const int flt_no, const std::string suffix,
                     const TDateTime locale_scd_out, const std::string airp )
{
  tst();
  TFndFlts flts;
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT point_addr, aodb_point_id FROM aodb_points WHERE point_id=:point_id AND aodb_point_id IS NOT NULL";
  Qry.DeclareVariable( "point_id", otInteger );
  if ( findFlt( airline, flt_no, suffix, locale_scd_out, airp, true, flts ) ) {
    map<string,int> aodb_point_ids;
    int point_id = NoExists;
    for ( TFndFlts::iterator i=flts.begin(); i!=flts.end(); i++ ) {
      ProgTrace( TRACE5, "bindingAODBFlt: i->point_id=%d, i->pr_del=%d", i->point_id, i->pr_del );
      if ( i->pr_del == -1 ) {
        Qry.SetVariable( "point_id", i->point_id );
        Qry.Execute();
        if ( !Qry.Eof ) {
          aodb_point_ids[ Qry.FieldAsString( "point_addr" ) ] = Qry.FieldAsInteger( "aodb_point_id" );
        }
      }
      if ( i->pr_del != -1 && point_id == NoExists ) {
        point_id = i->point_id;
      }
    }
    if ( point_id == NoExists )
      return;
    for ( map<string,int>::iterator i=aodb_point_ids.begin(); i!=aodb_point_ids.end(); i++ ) {
      Qry.Clear();
      Qry.SQLText =
        "BEGIN "
        "DELETE aodb_points WHERE point_id=:point_id AND point_addr=:point_addr AND aodb_point_id!=:aodb_point_id;"
        "UPDATE aodb_points SET point_id=:point_id WHERE aodb_point_id=:aodb_point_id AND point_addr=:point_addr;"
        "END;";
	    Qry.CreateVariable( "point_id", otInteger, point_id );
	    Qry.CreateVariable( "point_addr", otString, i->first );
	    Qry.CreateVariable( "aodb_point_id", otFloat, i->second );
	    Qry.Execute();
    }
  }
}

void createFileParamsAODB( int point_id, map<string,string> &params, bool pr_bag )
{
	TQuery FlightQry( &OraSession );
	FlightQry.SQLText = "SELECT airline,flt_no,suffix,scd_out,airp FROM points WHERE point_id=:point_id";
	FlightQry.CreateVariable( "point_id", otInteger, point_id );
	FlightQry.Execute();
	if ( !FlightQry.RowCount() )
		throw Exception( "Flight not found in createFileParams" );
	string region = getRegion( FlightQry.FieldAsString( "airp" ) );
	TDateTime scd_out = UTCToLocal( FlightQry.FieldAsDateTime( "scd_out" ), region );
	string p = string( FlightQry.FieldAsString( "airline" ) ) +
	           FlightQry.FieldAsString( "flt_no" ) +
	           FlightQry.FieldAsString( "suffix" ) +
	           string( DateTimeToStr( scd_out, "yymmddhhnn" ) );
	if ( pr_bag )
		p += 'b';
  params[ PARAM_FILE_NAME ] =  p + ".txt";
  params[ NS_PARAM_EVENT_TYPE ] = EncodeEventType( ASTRA::evtPax );
  params[ NS_PARAM_EVENT_ID1 ] = IntToString( point_id );
  params[ PARAM_TYPE ] = VALUE_TYPE_FILE; // FILE
}

void DecodeBagType( int bag_type, int &code, string &name, TAODBFormat format )
{
	switch ( bag_type ) {
	   case 1:
	   case 2: // ��������
		 	  code = 1;
		 	  name = "�����.����";
		 	  break;
		 case 3: // ��������
		    code = 2;
		    name = "�����������";
		    break;
		 case 4: // ������
		 	  code = 3;
		 	  name = "��������";
		 	  break;
		 case 5:
		 case 6:
		 case 7: // ������ - ��ॢ��, ����, 梥��
		 	  code = 4;
		 	  name = "������";
		 	  break;
		 case 8: // ᯥ���� - �㦥���� ����
		 	  code = 6;
		 	  name = "���������";
		 	  break;
     case 36:
        code = 5;
        name = "������";
        if ( format == afNewUrengoy )
          break;
		 default:
		 	  code = 0;
		 	  name = "�������";
		 	  break;
		}
}

bool getFlightData( int point_id, const string &point_addr,
                    double &aodb_point_id, string &flight, string &scd_date, string &airp_dep )
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT aodb_point_id,airline||flt_no||suffix trip,scd_out,airp "
	 " FROM points, aodb_points, trip_final_stages "
	 " WHERE points.point_id=:point_id AND "
	 "       trip_final_stages.point_id=points.point_id AND "
	 "       trip_final_stages.stage_type=:ckin_stage_type AND "
	 "       trip_final_stages.stage_id BETWEEN :stage1 AND :stage2 AND "
	 "       points.point_id=aodb_points.point_id(+) AND "
	 "       :point_addr=aodb_points.point_addr(+)";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.CreateVariable( "ckin_stage_type", otInteger, stCheckIn );
	Qry.CreateVariable( "stage1", otInteger, sOpenCheckIn );
	Qry.CreateVariable( "stage2", otInteger, sCloseBoarding );
	Qry.Execute();
	if ( !Qry.RowCount() )
		return false;
	aodb_point_id = Qry.FieldAsFloat( "aodb_point_id" );
	flight = Qry.FieldAsString( "trip" );
	scd_date = DateTimeToStr( UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), getRegion( Qry.FieldAsString("airp") ) ), "dd.mm.yyyy hh:nn" );
	airp_dep = Qry.FieldAsString( "airp" );
	return true;
}

string GetTermInfo( TQuery &Qry, int pax_id, int reg_no, bool pr_tcheckin, const string &client_type,
                    const string &work_mode, const string &airp_dep,
                    int &length_time_value )
{
  ostringstream info;
  string term;
  TDateTime t;
  Qry.SetVariable( "pax_id", pax_id );
  Qry.SetVariable( "reg_no", reg_no );
  Qry.SetVariable( "work_mode", work_mode );
  Qry.Execute();
  if ( !Qry.Eof ) {
 	  if ( DecodeClientType( client_type.c_str() ) == ASTRA::ctWeb )
   		term = "777";
    else    // �⮩��
      if ( string(Qry.FieldAsString( "airp" )) == airp_dep ) {
        term = Qry.FieldAsString( "station" );
        if ( !term.empty() && ( ( work_mode == "�" && term[0] == 'R' ) ||
                                ( work_mode == "�" && term[0] == 'G' ) ) )
          term = term.substr( 1, term.length() - 1 );
      }
      else
        if ( pr_tcheckin )
          term = "999";
  }

  if ( !Qry.Eof )
    t = Qry.FieldAsDateTime( "time" );
  else
   	t = NoExists;
  if ( term.empty() )
    info<<setw(4)<<"";
  else
    info<<setw(4)<<string(term).substr(0,4); // �⮩��
  length_time_value = 16;
  if ( t == NoExists )
   	info<<setw(length_time_value)<<"";
  else
   	info<<setw(length_time_value)<<DateTimeToStr( UTCToLocal( t, getRegion( airp_dep ) ), "dd.mm.yyyy hh:nn" );
  length_time_value += 4;
  return info.str();
}

bool createAODBCheckInInfoFile( int point_id, bool pr_unaccomp, const std::string &point_addr,
                                TAODBFormat format, TFileDatas &fds )
{
  ProgTrace( TRACE5, "createAODBCheckInInfoFile: point_id=%d, point_addr=%s", point_id, point_addr.c_str() );
	TFileData fd;
	TDateTime execTask = NowUTC();
  double aodb_point_id;
	string flight;
	string scd_date;
	string airp_dep;
	AODB_STRUCT STRAO;
	vector<AODB_STRUCT> prior_aodb_pax, aodb_pax;
	vector<AODB_STRUCT> prior_aodb_bag, aodb_bag;
	if ( !getFlightData( point_id, point_addr, aodb_point_id, flight, scd_date, airp_dep ) )
		return false;
	ostringstream heading;
	if ( aodb_point_id )
	  heading<<setfill(' ')<<std::fixed<<setw(10)<<setprecision(0)<<aodb_point_id;
	else
	  heading<<setfill(' ')<<std::fixed<<setw(10)<<"";
	heading<<setfill(' ')<<std::fixed<<setw(10)<<flight;
	heading<<setw(16)<<scd_date;

	TQuery Qry(&OraSession);
	if ( pr_unaccomp )
	  Qry.SQLText =
	   "SELECT DISTINCT grp_id pax_id, 0 reg_no, NULL record FROM aodb_unaccomp WHERE point_id=:point_id AND point_addr=:point_addr";
	else
	  Qry.SQLText =
	   "SELECT pax_id, reg_no, record FROM aodb_pax WHERE point_id=:point_id AND point_addr=:point_addr";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.Execute();
	STRAO.doit = false;
	STRAO.unaccomp = pr_unaccomp;
	while ( !Qry.Eof ) {
  	STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
  	STRAO.reg_no = Qry.FieldAsInteger( "reg_no" );
		STRAO.record = string( Qry.FieldAsString( "record" ) );
		prior_aodb_pax.push_back( STRAO );
		Qry.Next();
	}
	Qry.Clear();
	if ( pr_unaccomp )
	  Qry.SQLText =
	   "SELECT grp_id pax_id,num bag_num,record bag_record,pr_cabin "
	   " FROM aodb_unaccomp "
	   " WHERE point_id=:point_id AND"
	   "       point_addr=:point_addr "
	   " ORDER BY pr_cabin DESC, bag_num ";
	else
	  Qry.SQLText =
	   "SELECT aodb_pax.pax_id,aodb_bag.num bag_num,aodb_bag.record bag_record,aodb_bag.pr_cabin "
	   " FROM aodb_pax,aodb_bag "
	   " WHERE aodb_pax.point_id=:point_id AND"
	   "       aodb_pax.point_addr=:point_addr AND "
	   "       aodb_bag.point_addr=:point_addr AND "
	   "       aodb_pax.pax_id=aodb_bag.pax_id "
	   " ORDER BY pr_cabin DESC, bag_num ";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.Execute();
	STRAO.doit = false;
	STRAO.unaccomp = pr_unaccomp;
	while ( !Qry.Eof ) {
		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  STRAO.num = Qry.FieldAsInteger( "bag_num" );
	  STRAO.record = Qry.FieldAsString( "bag_record" );
	  STRAO.pr_cabin = Qry.FieldAsInteger( "pr_cabin" );
	  prior_aodb_bag.push_back( STRAO );
		Qry.Next();
	}
	// ⥯��� ᮧ����� ��宦�� ���⨭�� �� ����� ३� �� ��
	TQuery BagQry( &OraSession );
	BagQry.Clear();
	BagQry.SQLText =
		 "SELECT bag2.bag_type,bag2.weight,color,no,"
		 "       bag2.num bag_num "
		 " FROM bag2,bag_tags "
		 " WHERE bag2.grp_id=:grp_id AND "
     "       bag2.bag_pool_num=:bag_pool_num AND "
		 "       bag2.grp_id=bag_tags.grp_id(+) AND "
		 "       bag2.num=bag_tags.bag_num(+) AND "
		 "       bag2.pr_cabin=:pr_cabin "
		 " ORDER BY bag2.num, no";
	BagQry.DeclareVariable( "grp_id", otInteger );
  BagQry.DeclareVariable( "bag_pool_num", otInteger );
	BagQry.DeclareVariable( "pr_cabin", otInteger );
	Qry.Clear();
	if ( pr_unaccomp )
	  Qry.SQLText =
	   "SELECT grp_id pax_id, grp_id, 0 reg_no FROM pax_grp "
	   " WHERE point_dep=:point_id AND status NOT IN ('E') AND class IS NULL "
	   " ORDER BY grp_id";
  else
  {
	  Qry.SQLText =
	   "SELECT pax.pax_id,pax.reg_no,pax.surname||RTRIM(' '||pax.name) name,pax_grp.grp_id,"
	   "       pax_grp.airp_arv,pax_grp.class,pax.refuse,"
	   "       pax.pers_type, "
	   "       NVL(pax_doc.gender,'F') as gender, "
	   "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'one',rownum) AS seat_no, "
	   "       pax.seats seats, "
	   "       ckin.get_excess(pax_grp.grp_id,pax.pax_id) excess,"
	   "       ckin.get_rkAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rkamount,"
	   "       ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rkweight,"
	   "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bagamount,"
	   "       ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bagweight,"
     "       ckin.get_bag_pool_pax_id(pax.grp_id,pax.bag_pool_num) AS bag_pool_pax_id, "
     "       pax.bag_pool_num, "
	   "       pax.pr_brd, "
	   "       pax_grp.status, "
	   "       pax_grp.client_type, "
	   "       pax_doc.no document, "
	   "       pax.ticket_no "
	   " FROM pax_grp, pax, pax_doc "
	   " WHERE pax_grp.grp_id=pax.grp_id AND "
	   "       pax_grp.point_dep=:point_id AND "
     "       pax_grp.status NOT IN ('E') AND "
	   "       pax.wl_type IS NULL AND "
	   "       pax.pax_id=pax_doc.pax_id(+) "
	   " ORDER BY pax_grp.grp_id,seats ";
	};
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	TQuery RemQry( &OraSession );
	RemQry.SQLText = "SELECT rem_code FROM pax_rem WHERE pax_id=:pax_id";
	RemQry.DeclareVariable( "pax_id", otInteger );
	TQuery TimeQry( &OraSession );
	if ( !pr_unaccomp ) {
    TimeQry.SQLText =
      "SELECT time,NVL(stations.name,aodb_pax_change.desk) station, client_type, airp "
      " FROM aodb_pax_change,stations "
      " WHERE pax_id=:pax_id AND aodb_pax_change.reg_no=:reg_no AND "
      "       aodb_pax_change.work_mode=:work_mode AND "
      "       aodb_pax_change.desk=stations.desk(+) AND aodb_pax_change.work_mode=stations.work_mode(+)";
    TimeQry.DeclareVariable( "pax_id", otInteger );
    TimeQry.DeclareVariable( "reg_no", otInteger );
    TimeQry.DeclareVariable( "work_mode", otString );
  }
	vector<string> baby_names;
	int length_time_value;

	while ( !Qry.Eof ) {
		if ( !pr_unaccomp && Qry.FieldAsInteger( "seats" ) == 0 ) {
			if ( Qry.FieldIsNULL( "refuse" ) )
			  baby_names.push_back( Qry.FieldAsString( "name" ) );
			Qry.Next();
			continue;
		}
		ostringstream record;
		record<<heading.str();
		int end_checkin_time = -1;
		int end_brd_time = -1;
		length_time_value = 0;
		if ( !pr_unaccomp ) {
		  if ( format == afNewUrengoy ) {
		    vector<TPnrAddrItem> pnrs;
		    GetPaxPnrAddr( Qry.FieldAsInteger( "pax_id" ), pnrs );
		    if ( !pnrs.empty() )
		      record<<setw(20)<<string(pnrs.begin()->addr);
        else
          record<<setw(20)<<"";
        record<<setw(20)<<Qry.FieldAsString( "ticket_no" );
        record<<setw(20)<<Qry.FieldAsString( "document" );
		  }
		  record<<setw(3)<<Qry.FieldAsInteger( "reg_no");
	  	record<<setw(30)<<string(Qry.FieldAsString( "name" )).substr(0,30);
      if ( DecodePerson( Qry.FieldAsString( "pers_type" ) ) == ASTRA::adult ) {
        record<<setw(1)<<string(Qry.FieldAsString( "gender" )).substr(0,1);
      }
      else {
        record<<setw(1)<<string(" ").substr(0,1);
      }
		  TAirpsRow *row=(TAirpsRow*)&base_tables.get("airps").get_row("code",Qry.FieldAsString("airp_arv"));
		  if ( format == afNewUrengoy )
		    record<<setw(3)<<row->code.substr(0,3);
		  else
		    record<<setw(20)<<row->code.substr(0,20);
		  record<<setw(1);
		  switch ( DecodeClass(Qry.FieldAsString( "class")) ) {
		  	case ASTRA::F:
		  		  record<<0;
		  		  break;
		  	case ASTRA::C:
		  		  record<<1;
		  		  break;
		  	case ASTRA::Y:
		  		  record<<2;
		  		  break;
		  	default:;
		  }
		  record<<setw(1);
		  RemQry.SetVariable( "pax_id", Qry.FieldAsInteger( "pax_id" ) );
	    RemQry.Execute();
	    int category = 0;
		  while ( !RemQry.Eof && category == 0 ) {
		  	string rem = RemQry.FieldAsString( "rem_code" );
		  	if ( rem == "VIP" ) {
		  	  category = 2;
		  	}
		  	else
		  		if ( rem == "DIPB" ) {
		  		  category = 3;
		  		}
		  		else
		  			if ( rem == "SPSV" ) {
		  			  category = 4;
		  			}
		  			else
		  				if ( rem == "MEDA" ) {
		  				  category = 5;
		  			  }
		  			  else
		  			  	if ( rem == "UMNR" ) {
		  			  	  category = 9;
		  			  	}
                else
                  if ( rem == "DUTY" ) {
                    category = 10;
                  }
			  RemQry.Next();
		  }
		  record<<setw(2)<<category;
		  record<<setw(1);
		  bool adult = false;
		  switch ( DecodePerson( Qry.FieldAsString( "pers_type" ) ) ) {
		  	case ASTRA::adult:
		  		  adult = true;
		  		  record<<0;
		  		  break;
		    case ASTRA::baby:
		    	  record<<2;
		    	  break;
			  default:
		  		  record<<1;
	  	}
		  record<<setw(5)<<Qry.FieldAsString( "seat_no" );
		  record<<setw(2)<<Qry.FieldAsInteger( "seats" )-1;
		  record<<setw(4)<<Qry.FieldAsInteger( "excess" );
		  record<<setw(3)<<Qry.FieldAsInteger( "rkamount" );
		  record<<setw(4)<<Qry.FieldAsInteger( "rkweight" );
		  record<<setw(3)<<Qry.FieldAsInteger( "bagamount" );
		  record<<setw(4)<<Qry.FieldAsInteger( "bagweight" );
		  record<<setw(10)<<""; // ����� ��ꥤ�������� ३�
		  record<<setw(16)<<""; // ��� ��ꥤ�������� ३�
		  record<<setw(3)<<""; // ���� ॣ. ����� ���ᠦ��
		  if ( Qry.FieldAsInteger( "pr_brd" ) )
		  	record<<setw(1)<<1;
		  else
		  	record<<setw(1)<<0;
		  if ( adult && !baby_names.empty() ) {
		    record<<setw(2)<<1; // �� ������⢮
		    record<<setw(36)<<baby_names.begin()->substr(0,36); // ��� ॡ����
		    baby_names.erase( baby_names.begin() );
	 	  }
		  else {
		    record<<setw(2)<<0; // �� ������⢮
		    record<<setw(36)<<""; // ��� ॡ����
		  }
		  record<<setw(60)<<""; // ���. ���
		  record<<setw(1)<<0; // ����㭠த�� �����
//		record<<setw(1)<<0; // �࠭�⫠���᪨� ����� :)
      // �⮩�� ॣ. + �६� ॣ. + ��室 �� ��ᠤ�� + �६� ��室� �� ��ᠤ��
      // ��।������ ᪢���猪
      TCkinRouteItem item;
      TCkinRoute().GetPriorSeg( Qry.FieldAsInteger( "grp_id" ), crtIgnoreDependent, item );
      bool pr_tcheckin = ( item.grp_id != NoExists );
      record<<GetTermInfo( TimeQry, Qry.FieldAsInteger( "pax_id" ),
                           Qry.FieldAsInteger( "reg_no" ),
                           pr_tcheckin,
                           Qry.FieldAsString( "client_type" ),
                           "�", airp_dep, length_time_value ); // �⮩�� ॣ.
      end_checkin_time = record.str().size();
      record<<GetTermInfo( TimeQry, Qry.FieldAsInteger( "pax_id" ),
                           Qry.FieldAsInteger( "reg_no" ),
                           pr_tcheckin,
                           Qry.FieldAsString( "client_type" ),
                           "�", airp_dep, length_time_value ); // ��室 �� ��ᠤ��
      end_brd_time = record.str().size();
		  if ( Qry.FieldIsNULL( "refuse" ) )
		  	record<<setw(1)<<0<<";";
		  else
    		record<<setw(1)<<1<<";"; // �⪠� �� �����
    } // end if !pr_unaccomp
	 	STRAO.record = record.str();
	 	STRAO.checkin_time_pos = end_checkin_time - length_time_value;
	 	STRAO.length_checkin_time = length_time_value;
	 	STRAO.brd_time_pos = end_brd_time - length_time_value;
	 	STRAO.length_brd_time = length_time_value;
	  STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  STRAO.reg_no = Qry.FieldAsInteger( "reg_no" );
		aodb_pax.push_back( STRAO );
		// ��筠� �����
		if ( pr_unaccomp ||
         ( !Qry.FieldIsNULL( "bag_pool_pax_id" ) && !Qry.FieldIsNULL( "bag_pool_num" ) &&
           Qry.FieldAsInteger( "bag_pool_pax_id" ) == Qry.FieldAsInteger( "pax_id" ) ) ) {
		  BagQry.SetVariable( "grp_id", Qry.FieldAsInteger( "grp_id" ) );
      if (pr_unaccomp)
        BagQry.SetVariable( "bag_pool_num", 1 );
      else
        BagQry.SetVariable( "bag_pool_num", Qry.FieldAsInteger( "bag_pool_num" ) );
		  BagQry.SetVariable( "pr_cabin", 1 );
		  BagQry.Execute();

		  int code;
		  string type_name;
		  while ( !BagQry.Eof ) {
		  	ostringstream record_bag;
		  	record_bag<<setfill(' ')<<std::fixed;
		  	DecodeBagType( BagQry.FieldAsInteger( "bag_type" ), code, type_name, format );
		  	record_bag<<setw(2)<<code<<setw(20)<<type_name.substr(0,20);
	  	  record_bag<<setw(4)<<BagQry.FieldAsInteger( "weight" );
	  		//record<<setw(1)<<0; // ��⨥
	  		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  		STRAO.num = BagQry.FieldAsInteger( "bag_num" );
	  		STRAO.record = record_bag.str();
	  		STRAO.pr_cabin = 1;
		  	aodb_bag.push_back( STRAO );
		  	BagQry.Next();
		  }
		  // �����
		  BagQry.SetVariable( "pr_cabin", 0 );
		  BagQry.Execute();
		  int prior_bag_num = -1;
		  while ( !BagQry.Eof ) {
		  	ostringstream record_bag, numstr;
		  	record_bag<<setfill(' ')<<std::fixed;
		  	DecodeBagType( BagQry.FieldAsInteger( "bag_type" ), code, type_name, format );
		  	record_bag<<setw(2)<<code<<setw(20)<<type_name.substr(0,20);
		  	numstr<<setfill('0')<<std::fixed<<setw(10)<<setprecision(0)<<BagQry.FieldAsFloat( "no" );
		  	record_bag<<setw(10);
        if ( numstr.str().size() <= 10 )
          record_bag<<numstr.str();
        else
          record_bag<<" ";
 	  	  record_bag<<setw(2)<<string(BagQry.FieldAsString( "color" )).substr(0,2);
		  	if ( prior_bag_num == BagQry.FieldAsInteger( "bag_num" ) )
		  		record_bag<<setw(4)<<0;
		  	else
		  		record_bag<<setw(4)<<BagQry.FieldAsInteger( "weight" );
		  	prior_bag_num = BagQry.FieldAsInteger( "bag_num" );
	  		STRAO.pax_id = Qry.FieldAsInteger( "pax_id" );
	  		STRAO.num = BagQry.FieldAsInteger( "bag_num" );
	  		STRAO.record = record_bag.str();
	  		STRAO.pr_cabin = 0;
		  	aodb_bag.push_back( STRAO );
		  	BagQry.Next();
		  }
		}
		else {
	  }
		Qry.Next();
	}
	// ����� �ࠢ����� 2-� ᫥����, ���᭥���, �� ����������, �� 㤠������, �� ����������
	// �ନ஢���� ������ ��� �⯠��� + �� ������ ��� ���� ᫥���?
	if ( NowUTC() - execTask > 1.0/1440.0 ) {
		ProgTrace( TRACE5, "Attention execute task aodb time > 1 min !!!, time=%s", DateTimeToStr( NowUTC() - execTask, "nn:ss" ).c_str() );
	}

  string res_checkin, prior_res_checkin;
  bool ch_pax;
  // ���砫� ���� �஢�ઠ �� 㤠������ � ���������� ���ᠦ�஢
	for ( vector<AODB_STRUCT>::iterator p=prior_aodb_pax.begin(); p!=prior_aodb_pax.end(); p++ ) {
 		getRecord( p->pax_id, p->reg_no, pr_unaccomp, aodb_pax, aodb_bag, res_checkin/*, res_bag*/ );
  	getRecord( p->pax_id, p->reg_no, pr_unaccomp, prior_aodb_pax, prior_aodb_bag, prior_res_checkin/*, prior_res_bag*/ );
	  ch_pax = res_checkin != prior_res_checkin;
  	if ( ch_pax ) {
		  createRecord( point_id, p->pax_id, p->reg_no, point_addr, pr_unaccomp,
		                heading.str(),
		                aodb_pax, aodb_bag, prior_aodb_pax, prior_aodb_bag,
		                res_checkin/*, res_bag*/ );
 			if ( ch_pax )
	  		fd.file_data += res_checkin;
	  }
  }
  // ��⮬ ���� �஢�ઠ �� ����� ���ᠦ�஢
	for ( vector<AODB_STRUCT>::iterator p=aodb_pax.begin(); p!=aodb_pax.end(); p++ ) {
		if ( p->doit )
			continue;
		getRecord( p->pax_id, p->reg_no, pr_unaccomp, aodb_pax, aodb_bag, res_checkin/*, res_bag*/ );
		getRecord( p->pax_id, p->reg_no, pr_unaccomp, prior_aodb_pax, prior_aodb_bag, prior_res_checkin/*, prior_res_bag*/ );
		ch_pax = res_checkin != prior_res_checkin;
		//ch_bag = res_bag != prior_res_bag;
		if ( ch_pax/*|| ch_bag*/ ) {
//		if ( getRecord( p->pax_id, aodb_pax, aodb_bag ) != getRecord( p->pax_id, prior_aodb_pax, prior_aodb_bag ) ) {
//			ProgTrace(TRACE5, "p->doit=%d, pax_id=%d", p->doit, p->pax_id );
			createRecord( point_id, p->pax_id, p->reg_no, point_addr, pr_unaccomp,
			              heading.str(),
			              aodb_pax, aodb_bag, prior_aodb_pax, prior_aodb_bag,
			              res_checkin/*, res_bag*/ );
			if ( ch_pax )
				fd.file_data += res_checkin;
/*			if ( ch_bag ) {
				//ProgTrace( TRACE5, "res_bag=%s, prior_res_bag=%s", res_bag.c_str(), prior_res_bag.c_str() );
				bag_file_data += res_bag;
			}*/
//			ProgTrace(TRACE5, "create record pax_id=%d", p->pax_id );
	  }
  }
  if ( !fd.file_data.empty() ) {
	  createFileParamsAODB( point_id, fd.params, pr_unaccomp );
		fds.push_back( fd );
	}
	return !fds.empty();
}


void getRecord( int pax_id, int reg_no, bool pr_unaccomp, const vector<AODB_STRUCT> &aodb_pax, const vector<AODB_STRUCT> &aodb_bag,
                string &res_checkin/*, string &res_bag*/  )
{
	res_checkin.clear();
//	res_bag.clear();
	for ( vector<AODB_STRUCT>::const_iterator i=aodb_pax.begin(); i!=aodb_pax.end(); i++ ) {
	  if ( i->pax_id == pax_id && i->reg_no == reg_no ) {
	  	if ( !pr_unaccomp )
		    res_checkin = i->record;
		    if ( i->checkin_time_pos >= 0 && i->length_checkin_time > 0 ) {
		      res_checkin.replace( res_checkin.begin() + i->checkin_time_pos, res_checkin.begin() + i->checkin_time_pos + i->length_checkin_time, i->length_checkin_time, ' ' );
        }
		    if ( i->brd_time_pos >= 0 && i->length_brd_time > 0 ) {
		      res_checkin.replace( res_checkin.begin() + i->brd_time_pos, res_checkin.begin() + i->brd_time_pos + i->length_brd_time, i->length_brd_time, ' ' );
        }
		  for ( vector<AODB_STRUCT>::const_iterator b=aodb_bag.begin(); b!=aodb_bag.end(); b++ ) {
		  	if ( b->pax_id != pax_id )
		  		continue;
		  	res_checkin += b->record;
//		  	res_bag += b->record;
	    }
	    break;
	  }
	}
//	ProgTrace( TRACE5, "getRecord pax_id=%d, reg_no=%d,return res=%s", pax_id, reg_no, res_checkin.c_str() );
}

void createRecord( int point_id, int pax_id, int reg_no, const string &point_addr, bool pr_unaccomp,
                   const string unaccomp_header,
                   vector<AODB_STRUCT> &aodb_pax, vector<AODB_STRUCT> &aodb_bag,
                   vector<AODB_STRUCT> &prior_aodb_pax, vector<AODB_STRUCT> &prior_aodb_bag,
                   string &res_checkin/*, string &res_bag*/ )
{
	//ProgTrace( TRACE5, "point_id=%d, pax_id=%d, reg_no=%d, point_addr=%s", point_id, pax_id, reg_no, point_addr.c_str() );
	res_checkin.clear();
	//res_bag.clear();
	TQuery PQry( &OraSession );
 	PQry.SQLText =
 	 "SELECT NVL(MAX(rec_no_pax),-1) r1, "
 	 "       NVL(MAX(rec_no_bag),-1) r2, "
 	 "       NVL(MAX(rec_no_unaccomp),-1) r3 "
 	 " FROM aodb_points "
 	 " WHERE point_id=:point_id AND point_addr=:point_addr";
 	PQry.CreateVariable( "point_id", otInteger, point_id );
 	PQry.CreateVariable( "point_addr", otString, point_addr );
 	PQry.Execute();
 	int bag_num;
 	if ( pr_unaccomp ) {
 		bag_num = PQry.FieldAsInteger( "r3" ) + 1;
  }
 	else {
 	  bag_num = PQry.FieldAsInteger( "r2" );
    ostringstream r;
    r<<setfill(' ')<<std::fixed<<setw(6)<<PQry.FieldAsInteger( "r1" ) + 1;
 	  res_checkin = r.str();
  }
  	// ��࠭塞 ���� ᫥���
 	PQry.Clear();
 	if ( pr_unaccomp )
 	  PQry.SQLText =
 	   " DELETE aodb_unaccomp WHERE grp_id=:pax_id AND point_addr=:point_addr AND point_id=:point_id ";
 	else
 	  PQry.SQLText =
 	   "BEGIN "
 	   " DELETE aodb_bag WHERE pax_id=:pax_id AND point_addr=:point_addr; "
 	   " DELETE aodb_pax WHERE pax_id=:pax_id AND point_addr=:point_addr; "
 	   " UPDATE aodb_points SET rec_no_pax=NVL(rec_no_pax,-1)+1 WHERE point_id=:point_id AND point_addr=:point_addr; "
     "  IF SQL%NOTFOUND THEN "
     "    INSERT INTO aodb_points(point_id,point_addr,aodb_point_id,rec_no_flt,rec_no_pax,rec_no_bag,rec_no_unaccomp) "
     "      VALUES(:point_id,:point_addr,NULL,-1,0,-1,-1); "
     "  END IF; "
 	   "END; ";
 	PQry.CreateVariable( "point_id", otInteger, point_id );
 	PQry.CreateVariable( "pax_id", otInteger, pax_id );
 	PQry.CreateVariable( "point_addr", otString, point_addr );
 	PQry.Execute();
 	vector<AODB_STRUCT>::iterator d, n;
  d=aodb_pax.end();
  n=prior_aodb_pax.end();
	for ( d=aodb_pax.begin(); d!=aodb_pax.end(); d++ ) // �롨ࠥ� �㦭��� ���ᠦ��
 		if ( d->pax_id == pax_id && d->reg_no == reg_no ) {
 			break;
 		}
  for ( n=prior_aodb_pax.begin(); n!=prior_aodb_pax.end(); n++ )
 		if ( n->pax_id == pax_id && n->reg_no == reg_no ) {
 			break;
 		}
 	if ( !pr_unaccomp ) {
    if ( d == aodb_pax.end() ) { // 㤠����� ���ᠦ��
  	  res_checkin += n->record.substr( 0, n->record.length() - 2 );
  	  res_checkin += "1;";
  		ProgTrace( TRACE5, "delete record, record=%s", res_checkin.c_str() );
    }
	  else  {
  	  res_checkin += d->record;
    	PQry.Clear();
       PQry.SQLText =
        "INSERT INTO aodb_pax(point_id,pax_id,reg_no,point_addr,record) "
        " VALUES(:point_id,:pax_id,:reg_no,:point_addr,:record)";
      PQry.CreateVariable( "point_id", otInteger, point_id );
      PQry.CreateVariable( "pax_id", otInteger, pax_id );
      PQry.CreateVariable( "reg_no", otInteger, reg_no );
      PQry.CreateVariable( "point_addr", otString, point_addr );
      string save_record = d->record;
	    if ( d->checkin_time_pos >= 0 && d->length_checkin_time > 0 ) {
		      save_record.replace( save_record.begin() + d->checkin_time_pos, save_record.begin() + d->checkin_time_pos + d->length_checkin_time, d->length_checkin_time, ' ' );
        }
		    if ( d->brd_time_pos >= 0 && d->length_brd_time > 0 ) {
		      save_record.replace( save_record.begin() + d->brd_time_pos, save_record.begin() + d->brd_time_pos + d->length_brd_time, d->length_brd_time, ' ' );
        }
      PQry.CreateVariable( "record", otString, save_record );
      PQry.Execute();
    }
  }
  PQry.Clear();
  if ( pr_unaccomp )
    PQry.SQLText =
     "INSERT INTO aodb_unaccomp(grp_id,point_id,point_addr,num,pr_cabin,record) "
     " VALUES(:pax_id,:point_id,:point_addr,:num,:pr_cabin,:record)";
  else
    PQry.SQLText =
     "INSERT INTO aodb_bag(pax_id,point_addr,num,pr_cabin,record) "
     " VALUES(:pax_id,:point_addr,:num,:pr_cabin,:record)";
  if ( pr_unaccomp )
  	PQry.CreateVariable( "point_id", otInteger, point_id );
  PQry.CreateVariable( "pax_id", otInteger, pax_id );
  PQry.CreateVariable( "point_addr", otString, point_addr );
  PQry.DeclareVariable( "num", otInteger );
  PQry.DeclareVariable( "pr_cabin", otInteger );
  PQry.DeclareVariable( "record", otString );
  int num=0;
  vector<string> bags;
	vector<string> delbags;
	for ( int pr_cabin=1; pr_cabin>=0; pr_cabin-- ) {
		if ( d != aodb_pax.end() ) {
		  PQry.SetVariable( "pr_cabin", pr_cabin );
	    for ( vector<AODB_STRUCT>::iterator i=aodb_bag.begin(); i!= aodb_bag.end(); i++ ) {
	  	  if ( i->pr_cabin != pr_cabin || i->pax_id != pax_id )
  	  		continue;
	    	PQry.SetVariable( "num", num );
	    	PQry.SetVariable( "record", i->record );
	  	  PQry.Execute();
	  	  num++;
	  	  bool f=false;
	  	  for ( vector<AODB_STRUCT>::iterator j=prior_aodb_bag.begin(); j!= prior_aodb_bag.end(); j++ ) {
  	  		if ( j->pr_cabin == pr_cabin && !j->doit &&
	    			   i->pax_id == pax_id && i->pax_id == j->pax_id && i->record == j->record ) {
	  			  f=true;
	  			  i->doit=true;
	  			  j->doit=true;
   			    bags.push_back( i->record + "0;" );
	  		  }
	      }
	      if ( !f ) {
	    	  bags.push_back( i->record + "0;" );
	      }
  	  }
  	}
  	if ( d != aodb_pax.end() || pr_unaccomp ) {
	    for ( vector<AODB_STRUCT>::iterator i=prior_aodb_bag.begin(); i!= prior_aodb_bag.end(); i++ ) {
	    	bool f=false;
	    	if ( i->doit || i->pr_cabin != pr_cabin || i->pax_id != pax_id )
	  	  	continue;
	  	  for ( vector<AODB_STRUCT>::iterator j=aodb_bag.begin(); j!= aodb_bag.end(); j++ ) {
	  		  if ( j->pr_cabin == pr_cabin && i->pax_id == pax_id && i->pax_id == j->pax_id && i->record == j->record ) {
  	  			f=true;
	    			break;
	    		}
	      }
	      if ( !f ) {
	    	  delbags.push_back(i->record + "1;");
	      }
  	  }
  	}
	} // end for
	  if ( d != aodb_pax.end() )
	  	d->doit = true;
	  if ( n != prior_aodb_pax.end() )
	  	n->doit = true;

	for (vector<string>::iterator si=delbags.begin(); si!=delbags.end(); si++ ) {
		if ( pr_unaccomp ) {
      stringstream r;
      r<<setfill(' ')<<std::fixed<<setw(6)<<bag_num++<<unaccomp_header;
      res_checkin += r.str() + *si + "\n";
    }
    else res_checkin += *si;
	}

	for (vector<string>::iterator si=bags.begin(); si!=bags.end(); si++ ) {
		if ( pr_unaccomp ) {
      stringstream r;
      r<<setfill(' ')<<std::fixed<<setw(6)<<bag_num++<<unaccomp_header;
      res_checkin += r.str() + *si + "\n";
    }
    else res_checkin += *si;
	}

	PQry.Clear();
	if ( pr_unaccomp ) {
	  PQry.SQLText =
 	   "BEGIN "
 	   "UPDATE aodb_points SET rec_no_unaccomp=:rec_no_unaccomp WHERE point_id=:point_id AND point_addr=:point_addr; "
     "  IF SQL%NOTFOUND THEN "
     "    INSERT INTO aodb_points(point_id,point_addr,aodb_point_id,rec_no_flt,rec_no_pax,rec_no_bag,rec_no_unaccomp) "
     "      VALUES(:point_id,:point_addr,NULL,-1,-1,-1,0); "
     "  END IF; "
 	   "END; ";
 	  PQry.CreateVariable( "rec_no_unaccomp", otInteger, bag_num - 1 );
 	}
	else {
	  PQry.SQLText =
 	   "BEGIN "
 	   "UPDATE aodb_points SET rec_no_bag=:rec_no_bag WHERE point_id=:point_id AND point_addr=:point_addr; "
     "  IF SQL%NOTFOUND THEN "
     "    INSERT INTO aodb_points(point_id,point_addr,aodb_point_id,rec_no_flt,rec_no_pax,rec_no_bag,rec_no_unaccomp) "
     "      VALUES(:point_id,:point_addr,NULL,-1,0,:rec_no_bag,-1); "
     "  END IF; "
 	   "END; ";
 	  PQry.CreateVariable( "rec_no_bag", otInteger, bag_num );
 	}
 	PQry.CreateVariable( "point_id", otInteger, point_id );
 	PQry.CreateVariable( "point_addr", otString, point_addr );
 	PQry.Execute();
 	if ( !pr_unaccomp )
	  res_checkin += "\n";
}

void ParseFlight( const std::string &point_addr, const std::string &airp, std::string &linestr, AODB_Flight &fl )
{
	int err=0;
try {
	fl.invalid_field.clear();
  fl.rec_no = NoExists;
 	if ( linestr.length() < REC_NO_LEN )
 		throw Exception( "�訡�� �ଠ� ३�, �����=%d, ���祭��=%s, ����� ����� ��ப�", linestr.length(), linestr.c_str() );
  err++;
	TReqInfo *reqInfo = TReqInfo::Instance();
  string region = getRegion( airp );
	TQuery Qry( &OraSession );
	TQuery TIDQry( &OraSession );
	TQuery POINT_IDQry( &OraSession );
	string tmp, tmp1;
	tmp = linestr.substr( REC_NO_IDX, REC_NO_LEN );
	tmp = TrimString( tmp );
	fl.rec_no = NoExists;
	if ( StrToInt( tmp.c_str(), fl.rec_no ) == EOF || fl.rec_no < 0 ||fl.rec_no > 999999 )
		throw Exception( "�訡�� � ����� ��ப�, ���祭��=%s", tmp.c_str() );

 	if ( linestr.length() < 180 + 4 )
 		throw Exception( "�訡�� �ଠ� ३�, �����=%d, ���祭��=%s, ����� ����� ��ப�", linestr.length(), linestr.c_str() );

	tmp = linestr.substr( FLT_ID_IDX, FLT_ID_LEN );
	tmp = TrimString( tmp );
	if ( StrToFloat( tmp.c_str(), fl.id ) == EOF || fl.id < 0 || fl.id > 9999999999.0 )
		throw Exception( "�訡�� �����䨪��� ३�, ���祭��=%s", tmp.c_str() );
	tmp = linestr.substr( TRIP_IDX, TRIP_LEN );
	parseFlt( tmp, fl.airline, fl.flt_no, fl.suffix );
	err++;
	TElemFmt fmt;
	if ( !fl.suffix.empty() ) {
    try {
      fl.suffix = ElemToElemId( etSuffix, fl.suffix, fmt, false );
      if ( fmt == efmtUnknown )
       throw EConvertError("");
    }
    catch( EConvertError &e ) {
    	throw Exception( "�訡�� �ଠ� ����� ३�, ���祭��=%s", tmp.c_str() );
    }
  }
 	try {
    fl.airline = ElemToElemId( etAirline, fl.airline, fmt, false );
    if ( fmt == efmtUnknown )
      throw EConvertError("");
	  if ( fmt == efmtCodeInter || fmt == efmtCodeICAOInter )
		  fl.trip_type = "�";  //!!!vlad � �ࠢ��쭮 �� ⠪ ��।����� ⨯ ३�? �� 㢥७. �஢�ઠ �� ����� �������. �᫨ � ������� �� �.�. �ਭ������� ����� ��࠭� � "�" ���� "�"
    else
  	  fl.trip_type = "�";
  }
  catch( EConvertError &e ) {
  	Qry.Clear();
  	Qry.SQLText =
  	 "SELECT airline as code FROM aodb_airlines WHERE aodb_code=:code";
	  Qry.CreateVariable( "code", otString, fl.airline );
	  err++;
	  Qry.Execute();
	  err++;
	  if ( !Qry.RowCount() )
	  	throw Exception( "�������⭠� ������������, ���祭��=%s", fl.airline.c_str() );
	  fl.airline = Qry.FieldAsString( "code" );
	  fl.trip_type = "�"; //???
  }
  err++;
	tmp = linestr.substr( LITERA_IDX, LITERA_LEN );
	fl.litera = TrimString( tmp );
	if ( !fl.litera.empty() ) {
		Qry.Clear();
		Qry.SQLText =
		 "SELECT code,1 FROM trip_liters WHERE code=:code AND pr_del=0"
		 " UNION "
		 "SELECT litera as code,2 FROM aodb_liters WHERE aodb_code=:code "
		 " ORDER BY 2";
		Qry.CreateVariable( "code", otString, fl.litera );
		err++;
		Qry.Execute();
		err++;
		if ( !Qry.RowCount() )
			throw Exception( "�������⭠� ����, ���祭��=%s", fl.litera.c_str() );
		fl.litera = Qry.FieldAsString( "code" );
	}
	err++;
	TDateTime local_scd_out;
	tmp = linestr.substr( SCD_IDX, SCD_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		throw Exception( "�訡�� �ଠ� ��������� �६��� �뫥�, ���祭��=%s", tmp.c_str() );
	else
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", local_scd_out ) == EOF )
			throw Exception( "�訡�� �ଠ� ��������� �६��� �뫥�, ���祭��=%s", tmp.c_str() );
  try {
	  fl.scd = LocalToUTC( local_scd_out, region );
	}
	catch( boost::local_time::ambiguous_result ) {
		throw Exception( "�������� �६� �믮������ ३� ��।����� �� �������筮" );
  }
  catch( boost::local_time::time_label_invalid ) {
    throw Exception( "�������� �६� �믮������ ३� �� �������" );
  }
	tmp = linestr.substr( EST_IDX, EST_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.est = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.est ) == EOF )
			throw Exception( "�訡�� �ଠ� ���⭮�� �६��� �뫥�, ���祭��=%s", tmp.c_str() );
		try {
		  fl.est = LocalToUTC( fl.est, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
		  throw Exception( "����⭮� �६� �믮������ ३� ��।����� �� �������筮" );
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "����⭮� �६� �믮������ ३� �� �������" );
    }
	}
	tmp = linestr.substr( ACT_IDX, ACT_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.act = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.act ) == EOF )
			throw Exception( "�訡�� �ଠ� 䠪��᪮�� �६��� �뫥�, ���祭��=%s", tmp.c_str() );
		try {
		  fl.act= LocalToUTC( fl.act, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
	  	throw Exception( "�����᪮� �६� �믮������ ३� ��।����� �� �������筮" );
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "�����᪮� �६� �믮������ ३� �� �������" );
    }
	}
	err++;
	tmp = linestr.substr( HALL_IDX, HALL_LEN );
	fl.hall = TrimString( tmp );
	tmp = linestr.substr( PARK_OUT_IDX, PARK_OUT_LEN );
	fl.park_out = TrimString( tmp );
	fl.park_out = fl.park_out.substr( 0, 3 );

	tmp = linestr.substr( KRM_IDX, KRM_LEN );
	tmp = TrimString( tmp );
	if ( tmp.empty() )
		fl.krm = NoExists;
	else
  	if ( StrToInt( tmp.c_str(), fl.krm ) == EOF )
	  	throw Exception( "�訡�� �ଠ� ���, ���祭��=%s", tmp.c_str() );
	tmp = linestr.substr( MAX_LOAD_IDX, MAX_LOAD_LEN );
	tmp = TrimString( tmp );
	if ( tmp.empty() )
		fl.max_load = NoExists;
	else
  	if ( StrToInt( tmp.c_str(), fl.max_load ) == EOF || fl.max_load < 0 || fl.max_load > 999999 )
	  	throw Exception( "�訡�� �ଠ� ���, ���祭��=%s", tmp.c_str() );
	tmp = linestr.substr( CRAFT_IDX, CRAFT_LEN );
	fl.craft = TrimString( tmp );
	ProgTrace( TRACE5, "fl.craft=%s", fl.craft.c_str() );
	bool pr_craft_error = true;
	err++;
	if ( !fl.craft.empty() ) {
 	  try {
      fl.craft = ElemCtxtToElemId( ecDisp, etCraft, fl.craft, fmt, false );
      pr_craft_error = false;
    }
    catch( EConvertError &e ) {
      tst();
  	  Qry.Clear();
    	Qry.SQLText =
	     "SELECT code, 1 FROM crafts WHERE ( name=:code OR name_lat=:code ) AND pr_del=0 "
	     " UNION "
	     "SELECT craft as code, 2 FROM aodb_crafts WHERE aodb_code=:code";
	    Qry.CreateVariable( "code", otString, fl.craft );
	    err++;
	    Qry.Execute();
	    err++;
	    pr_craft_error = !Qry.RowCount();
	    if ( !pr_craft_error )
	      fl.craft = Qry.FieldAsString( "code" );
    }
	}
	err++;
  tmp = linestr.substr( BORT_IDX, BORT_LEN );
  fl.bort = TrimString( tmp );
	tmp = linestr.substr( CHECKIN_BEG_IDX, CHECKIN_BEG_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.checkin_beg = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.checkin_beg ) == EOF )
			throw Exception( "�訡�� �ଠ� ��砫� ॣ����樨, ���祭��=%s", tmp.c_str() );
		try {
		  fl.checkin_beg = LocalToUTC( fl.checkin_beg, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
	  	fl.checkin_beg = LocalToUTC( fl.checkin_beg + 1, region ) - 1;
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "��砫� ॣ����樨 ३� �� �������" );
    }
	}
	tmp = linestr.substr( CHECKIN_END_IDX, CHECKIN_END_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.checkin_end = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.checkin_end ) == EOF )
			throw Exception( "�訡�� �ଠ� ����砭�� ॣ����樨, ���祭��=%s", tmp.c_str() );
		try {
		  fl.checkin_end = LocalToUTC( fl.checkin_end, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
	  	fl.checkin_end = LocalToUTC( fl.checkin_end + 1, region ) - 1;
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "����砭�� ॣ����樨 ३� �� �������" );
    }
	}
	tmp = linestr.substr( BOARDING_BEG_IDX, BOARDING_BEG_LEN );
	tmp = TrimString( tmp );
	err++;
  if ( tmp.empty() )
		fl.boarding_beg = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.boarding_beg ) == EOF )
			throw Exception( "�訡�� �ଠ� ��砫� ��ᠤ��, ���祭��=%s", tmp.c_str() );
		try {
		  fl.boarding_beg = LocalToUTC( fl.boarding_beg, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
	  	fl.boarding_beg = LocalToUTC( fl.boarding_beg + 1, region ) - 1;
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "��砫� ��ᠤ�� ३� �� �������" );
    }
	}
	err++;
	tmp = linestr.substr( BOARDING_END_IDX, BOARDING_END_LEN );
	tmp = TrimString( tmp );
  if ( tmp.empty() )
		fl.boarding_end = NoExists;
	else {
		if ( StrToDateTime( tmp.c_str(), "dd.mm.yyyy hh:nn", fl.boarding_end ) == EOF )
			throw Exception( "�訡�� �ଠ� ����砭�� ��ᠤ��, ���祭��=%s", tmp.c_str() );
		try {
		  fl.boarding_end = LocalToUTC( fl.boarding_end, region );
		}
	  catch( boost::local_time::ambiguous_result ) {
	  	fl.boarding_end = LocalToUTC( fl.boarding_end + 1, region ) - 1;
    }
    catch( boost::local_time::time_label_invalid ) {
      throw Exception( "����砭�� ��ᠤ�� ३� �� �������" );
    }
	}
	err++;
	tmp = linestr.substr( PR_CANCEL_IDX, PR_CANCEL_LEN );
	tmp = TrimString( tmp );
	if ( tmp.empty() )
		fl.pr_cancel = 0;
	else
  	if ( StrToInt( tmp.c_str(), fl.pr_cancel ) == EOF )
	  	throw Exception( "�訡�� �ଠ� �ਧ���� �⬥��, ���祭��=%s", tmp.c_str() );
	tmp = linestr.substr( PR_DEL_IDX, PR_DEL_LEN );
	tmp = TrimString( tmp );
	if ( tmp.empty() )
		fl.pr_del = NoExists;
	else
  	if ( StrToInt( tmp.c_str(), fl.pr_del ) == EOF )
	  	throw Exception( "�訡�� �ଠ� �ਧ���� 㤠�����, ���祭��=%s", tmp.c_str() );
	int len = linestr.length();
	int i = PR_DEL_IDX + PR_DEL_LEN;
	tmp = linestr.substr( i, 1 );
	if ( tmp[ 0 ] != ';' )
		throw Exception( "�訡�� �ଠ� �������. �������� ᨬ��� ';', ����⨫�� ᨬ���	%c", linestr[ i ] );
	i++;
	bool dest_mode = true;
	err++;
	AODB_Dest dest;
	AODB_Term term;
	while ( i < len ) {
		tmp = linestr.substr( i, 1 );
		tmp = TrimString( tmp );
		if ( dest_mode ) {
    	if ( StrToInt( tmp.c_str(), dest.num ) == EOF || dest.num < 0 || dest.num > 9 )
    		if ( fl.dests.empty() )
	    	  throw Exception( "�訡�� �ଠ� ����� �㭪� ��ᠤ��, ���祭��=%s", tmp.c_str() );
	      else
	    	  dest_mode = false;
	    else {
	    	i++;
	    	tmp = linestr.substr( i, 3 );
	    	dest.airp = TrimString( tmp );
	    	if ( dest.airp.empty() )
	    		throw Exception( "�訡�� �ଠ� ���� ��ய���, ���祭��=%s", dest.airp.c_str() );
    	  try {
         dest.airp = ElemCtxtToElemId( ecDisp, etAirp, dest.airp, fmt, false );
        }
        catch( EConvertError &e ) {
	    	  Qry.Clear();
	    	  Qry.SQLText =
	    	   "SELECT code, 1 FROM airps WHERE ( name=:code OR name_lat=:code ) AND pr_del=0 "
	    	   " UNION "
	    	   "SELECT airp as code, 2 FROM aodb_airps WHERE aodb_code=:code "
	    	   " ORDER BY 2 ";
	      	Qry.CreateVariable( "code", otString, dest.airp );
	      	err++;
	      	Qry.Execute();
	      	err++;
	      	if ( !Qry.RowCount() )
	      		throw Exception( "��������� ��� ��ய���, ���祭��=%s", dest.airp.c_str() );
	      	dest.airp = Qry.FieldAsString( "code" );
        }
	    	i += 3;
	    	tmp = linestr.substr( i, 1 );
	    	tmp = TrimString( tmp );
	    	if ( tmp.empty() || StrToInt( tmp.c_str(), dest.pr_del ) == EOF || dest.pr_del < 0 || dest.pr_del > 1 )
	    	  throw Exception( "�訡�� �ଠ� �ਧ���� 㤠����� �㭪�, ���祭��=%s", tmp.c_str() );
	    	fl.dests.push_back( dest );
	    	i++;
	    	tmp = linestr.substr( i, 1 );
	      if ( tmp[ 0 ] != ';' )
		      throw Exception( "�訡�� �ଠ� �������. �������� ᨬ��� ';', ����⨫�� ᨬ���	%c", linestr[ i ] );
		    i++;
	    }
		}
		err++;
		if ( !dest_mode ) {
			int old_i = i;
			try {
			  if ( tmp != "�" && tmp != "�" )
			  	throw Exception( "�訡�� �ଠ� ⨯� �⮩��, ���祭��=%s", tmp.c_str() );
			  term.type = tmp;
			  i++;
    	  tmp = linestr.substr( i, 4 );
	   	  term.name = TrimString( tmp );
			  if ( term.name.empty() )
			  	throw Exception( "�訡�� �ଠ� ����� �⮩��, ���祭��=%s", term.name.c_str() );
			  string term_name;
			  if ( term.type == "�" )
		  		term_name = "G" + term.name;
		  	else
		  		term_name = "R" + term.name;
	  		Qry.Clear();
	  		Qry.SQLText = "SELECT desk FROM stations WHERE airp=:airp AND work_mode=:work_mode AND name=:code";
	  		Qry.CreateVariable( "airp", otString, airp );
	  		Qry.CreateVariable( "work_mode", otString, term.type );
	  		Qry.CreateVariable( "code", otString, term_name );
	  		err++;
	  		Qry.Execute();
	  		err++;
	  		if ( !Qry.RowCount() ) {
    			if ( term.type == "�" )
	    			term_name = "G0" + term.name;
		    	else
		  	  	term_name = "R0" + term.name;
			    Qry.SetVariable( "code", term_name );
			    err++;
			    Qry.Execute();
			    err++;
			    if ( !Qry.RowCount() )
				    throw Exception( "�������⭠� �⮩��, ���祭��=%s", term.name.c_str() );
		  	}
		  	term.name = Qry.FieldAsString( "desk" );
				i += 4;
       	tmp = linestr.substr( i, 1 );
	   	  tmp = TrimString( tmp );
	      if ( tmp.empty() || StrToInt( tmp.c_str(), term.pr_del ) == EOF || term.pr_del < 0 || term.pr_del > 1 )
	        throw Exception( "�訡�� �ଠ� �ਧ���� 㤠����� �⮩��, ���祭��=%s", tmp.c_str() );
	      fl.terms.push_back( term );
		  }
		  catch( Exception &e ) {
		  	i = old_i + 1 + 4;
		  	if ( fl.invalid_field.empty() )
		  		fl.invalid_field = e.what();
		  }
	    i++;
	    tmp = linestr.substr( i, 1 );
	   	if ( tmp[ 0 ] != ';' )
    		throw Exception( "�訡�� �ଠ� �������. �������� ᨬ��� ';', ����⨫�� ᨬ���	%c (2)", linestr[ i ] );
      i++;
		}
	}
	err++;
	bool overload_alarm = false;
	// ������ � ��
	TQuery QrySet(&OraSession);
	QrySet.SQLText =
     "BEGIN "
     " sopp.set_flight_sets(:point_id,:use_seances);"
     " IF :max_commerce IS NOT NULL THEN "
     "  UPDATE trip_sets SET max_commerce=:max_commerce WHERE point_id=:point_id;"
     " END IF; "
     "END;";
	QrySet.DeclareVariable( "point_id", otInteger );
	QrySet.CreateVariable( "use_seances", otInteger, (int)USE_SEANCES() );
	QrySet.DeclareVariable( "max_commerce", otInteger );


  err++;
  TFndFlts pflts;
  int move_id, point_id;
  bool pr_insert = !findFlt( fl.airline, fl.flt_no, fl.suffix, local_scd_out, airp, false, pflts );
  if ( pr_insert ) {
    ProgTrace( TRACE5, "ParseFlight: new flight - return" );
    return;
  }
  err++;
  ProgTrace( TRACE5, "airline=%s, flt_no=%d, suffix=%s, scd_out=%s, insert=%d", fl.airline.c_str(), fl.flt_no,
	           fl.suffix.c_str(), DateTimeToStr( fl.scd ).c_str(), pr_insert );
	int new_tid;
	vector<TTripInfo> flts;
	if ( pr_insert ) {
		if ( fl.craft.empty() )
			throw Exception( "�� ����� ⨯ ��" );
		else
			if ( pr_craft_error )
				throw Exception( "��������� ⨯ ��, ���祭��=%s", fl.craft.c_str() );
	}
	else
		if ( pr_craft_error ) {
		  fl.invalid_field = "��������� ⨯ ��, ���祭��=" + fl.craft;
			fl.craft.clear(); // ��頥� ���祭�� ⨯� �� - �� �� ������ ������� � ��
    }
 	TIDQry.SQLText = "SELECT cycle_tid__seq.nextval n FROM dual ";
	POINT_IDQry.SQLText = "SELECT point_id.nextval point_id FROM dual";
	TDateTime time_in_delay; //��।��塞 �६� ����প�
  TDateTime old_act = NoExists, old_est = NoExists;
  if ( fl.act != NoExists )
    time_in_delay = fl.act - fl.scd;
  else
    time_in_delay = NoExists;
	if ( pr_insert ) { // insert
    Qry.Clear();
    Qry.SQLText =
     "BEGIN "\
     " SELECT move_id.nextval INTO :move_id from dual; "\
     " INSERT INTO move_ref(move_id,reference)  SELECT :move_id, :reference FROM dual; "\
     "END;";
    Qry.DeclareVariable( "move_id", otInteger );
    Qry.CreateVariable( "reference", otString, FNull );
    err++;
    Qry.Execute();
    err++;
    move_id = Qry.GetVariableAsInteger( "move_id" );
    err++;
    TIDQry.Execute();
    err++;
    new_tid = TIDQry.FieldAsInteger( "n" );
    err++;
    POINT_IDQry.Execute();
    err++;
    point_id = POINT_IDQry.FieldAsInteger( "point_id" );
    string lmes = "���� ������ ३� ";
    lmes +=  fl.airline + IntToString( fl.flt_no ) + fl.suffix + ", �������: " + airp + "-";
    for ( vector<AODB_Dest>::iterator it=fl.dests.begin(); it!=fl.dests.end(); it++ ) {
    	if ( it != fl.dests.begin() )
    		lmes += "-";
      lmes += it->airp;
    }
    err++;
    reqInfo->MsgToLog( lmes, evtDisp, move_id, point_id );
    err++;
    reqInfo->MsgToLog( string( "���� ������ �㭪� " ) + airp, evtDisp, move_id, point_id );
    TTripInfo tripInfo;
    tripInfo.airline = fl.airline;
    tripInfo.flt_no = fl.flt_no;
    tripInfo.suffix = fl.suffix;
    tripInfo.airp = airp;
    tripInfo.scd_out = fl.scd;
    flts.push_back( tripInfo );
    err++;
    Qry.Clear();
    Qry.SQLText =
     "INSERT INTO points(move_id,point_id,point_num,airp,pr_tranzit,first_point,airline,flt_no,suffix,"\
     "                   craft,bort,scd_in,est_in,act_in,scd_out,est_out,act_out,trip_type,litera,"\
     "                   park_in,park_out,pr_del,tid,remark,pr_reg,airline_fmt,airp_fmt,craft_fmt,suffix_fmt) "\
     " VALUES(:move_id,:point_id,:point_num,:airp,:pr_tranzit,:first_point,:airline,:flt_no,:suffix,"\
     "        :craft,:bort,:scd_in,:est_in,:act_in,:scd_out,:est_out,:act_out,:trip_type,:litera,"\
     "        :park_in,:park_out,:pr_del,:tid,:remark,:pr_reg,0,0,0,0)";
    Qry.CreateVariable( "move_id", otInteger, move_id );
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "point_num", otInteger, 0 );
    Qry.CreateVariable( "airp", otString, airp );
    Qry.CreateVariable( "pr_tranzit", otInteger, 0 );
    Qry.CreateVariable( "first_point", otInteger, FNull );
    Qry.CreateVariable( "airline", otString, fl.airline );
    Qry.CreateVariable( "flt_no", otInteger, fl.flt_no );
    if ( fl.suffix.empty() )
      Qry.CreateVariable( "suffix", otString, FNull );
    else
    	Qry.CreateVariable( "suffix", otString, fl.suffix );
    Qry.CreateVariable( "craft", otString, fl.craft );
    Qry.CreateVariable( "bort", otString, fl.bort );
    Qry.CreateVariable( "scd_in", otDate, FNull );
    Qry.CreateVariable( "est_in", otDate, FNull );
    Qry.CreateVariable( "act_in", otDate, FNull );
    Qry.CreateVariable( "scd_out", otDate, fl.scd );
    if ( fl.est > NoExists )
      Qry.CreateVariable( "est_out", otDate, fl.est );
    else
    	Qry.CreateVariable( "est_out", otDate, FNull );
    if ( fl.act > NoExists )
      Qry.CreateVariable( "act_out", otDate, fl.act );
    else
    	Qry.CreateVariable( "act_out", otDate, FNull );
    Qry.CreateVariable( "trip_type", otString, fl.trip_type );
    Qry.CreateVariable( "litera", otString, fl.litera );
    Qry.CreateVariable( "park_in", otString, FNull );
    Qry.CreateVariable( "park_out", otString, fl.park_out );
   	if ( fl.pr_cancel )
    	Qry.CreateVariable( "pr_del", otInteger, 1 );
    else
    	Qry.CreateVariable( "pr_del", otInteger, 0 );
    Qry.CreateVariable( "tid", otInteger, new_tid );
    Qry.CreateVariable( "remark", otString, FNull );
    Qry.CreateVariable( "pr_reg", otInteger, fl.scd != NoExists );
    err++;
    Qry.Execute();
    err++;
    // ᮧ���� �६��� �孮�����᪮�� ��䨪� ⮫쪮 ��� �㭪� �뫥� �� ��� � ����� �� ��������
    QrySet.SetVariable( "point_id", point_id );
    if ( fl.max_load != NoExists )
      QrySet.SetVariable( "max_commerce", fl.max_load );
    else
      QrySet.SetVariable( "max_commerce", FNull );
		err++;
		QrySet.Execute();
		err++;
    int num = 0;
    for ( vector<AODB_Dest>::iterator it=fl.dests.begin(); it!=fl.dests.end(); it++ ) {
    	num++;
    	err++;
      POINT_IDQry.Execute();
      err++;
      Qry.SetVariable( "point_id", POINT_IDQry.FieldAsInteger( "point_id" ) );
      Qry.SetVariable( "point_num", num );
      Qry.SetVariable( "airp", it->airp );
      Qry.SetVariable( "pr_tranzit", 0 );
      Qry.SetVariable( "first_point", point_id );
      Qry.SetVariable( "pr_reg", 0 );
      if ( it == fl.dests.end() - 1 ) {
      	Qry.SetVariable( "airline", FNull );
        Qry.SetVariable( "flt_no", FNull );
        Qry.SetVariable( "suffix", FNull );
        Qry.SetVariable( "craft", FNull );
        Qry.SetVariable( "bort", FNull );
        Qry.SetVariable( "park_out", FNull );
        Qry.SetVariable( "trip_type", FNull );
        Qry.SetVariable( "litera", FNull );
      }
      Qry.SetVariable( "scd_out", FNull );
      Qry.SetVariable( "est_out", FNull );
      Qry.SetVariable( "act_out", FNull );
      Qry.SetVariable( "park_out", FNull );
      if ( it->pr_del )
      	Qry.SetVariable( "pr_del", -1 );
      else
    		Qry.SetVariable( "pr_del", 0 );
    	err++;
      TIDQry.Execute();
      err++;
      Qry.SetVariable( "tid", TIDQry.FieldAsInteger( "n" ) );
      err++;
      Qry.Execute();
      err++;
      reqInfo->MsgToLog( string( "���� ������ �㭪� " ) + it->airp, evtDisp, move_id, POINT_IDQry.FieldAsInteger( "point_id" ) );
    }
	}
	else { // update
    move_id = pflts.begin()->move_id;
    point_id = pflts.begin()->point_id;
		bool change_comp=false;
		string remark;
		TPointsDest dest;
		BitSet<TUseDestData> FUseData;
		FUseData.clearFlags();
		dest.Load( point_id, FUseData );
		
		TFlights  flights;
		flights.Get( point_id, ftAll );
		flights.Lock();
    Qry.Clear();
    Qry.SQLText =
     "UPDATE points "
     " SET craft=NVL(craft,:craft),bort=NVL(bort,:bort),est_out=:est_out,act_out=:act_out,litera=:litera, "
     "     park_out=:park_out "
     " WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "craft", otString, fl.craft );
 	  if ( fl.craft != dest.craft ) {
	  	if ( dest.craft.empty() ) {
 	  		reqInfo->MsgToLog( string( "�����祭�� �� " ) + fl.craft + " ���� " + airp , evtDisp, move_id, point_id );
 	  		change_comp = true;
 	  	}
 	  }
 	  Qry.CreateVariable( "bort", otString, fl.bort );
 	  if ( fl.bort != dest.bort ) {
 	  	if ( dest.bort.empty() ) {
 	  		reqInfo->MsgToLog( string( "�����祭�� ���� " ) + fl.bort + " ���� " + airp, evtDisp, move_id, point_id );
 	  		change_comp = true;
 	  	}
 	  }
 	  if ( fl.est > NoExists )
 	    Qry.CreateVariable( "est_out", otDate, fl.est );
 	  else
 	  	Qry.CreateVariable( "est_out", otDate, FNull );
 	  if ( fl.act > NoExists )
 	  	Qry.CreateVariable( "act_out", otDate, fl.act );
 	  else
 	    Qry.CreateVariable( "act_out", otDate, FNull );
 	  Qry.CreateVariable( "litera", otString, fl.litera );
 	  Qry.CreateVariable( "park_out", otString, fl.park_out );
 	  err++;
 	  old_act = dest.act_out;
 	  old_est = dest.est_out;
 	  if ( old_act != NoExists && fl.act == NoExists )
 	    time_in_delay = 0.0;
 	  Qry.Execute();
 	  err++;
 	  if ( change_comp ) {
 	  	SALONS2::AutoSetCraft( point_id );
    }
    bool old_ignore_auto = ( old_act != NoExists || dest.pr_del != 0 );
    bool new_ignore_auto = ( fl.act != NoExists || dest.pr_del != 0 );
    if ( old_ignore_auto != new_ignore_auto ) {
      SetTripStages_IgnoreAuto( point_id, new_ignore_auto );
    }
    overload_alarm = Get_AODB_overload_alarm( point_id, fl.max_load );
    Qry.Clear();
    Qry.SQLText =
     "UPDATE trip_sets SET max_commerce=:max_commerce WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "max_commerce", otInteger, fl.max_load );
    err++;
    Qry.Execute();
    err++;
  	check_overload_alarm( point_id );
	} // end update
	SALONS2::check_diffcomp_alarm( point_id );
	if ( SALONS2::isTranzitSalons( point_id ) ) {
    SALONS2::check_waitlist_alarm_on_tranzit_routes( point_id );
  }
  else {
    check_waitlist_alarm( point_id );
  }
  tst();
  if ( old_est != fl.est ) {
    if ( fl.est != NoExists ) {
      ProgTrace( TRACE5, "events: %s, %d, %d",
                 string(string("���⠢����� ���. �६� �뫥� �/� ") + airp + " " + DateTimeToStr( fl.est, "dd hh:nn" )).c_str(), move_id, point_id );
      reqInfo->MsgToLog( string("���⠢����� ���. �६� �뫥� �/� ") + airp + " " + DateTimeToStr( fl.est, "dd hh:nn" ), evtDisp, move_id, point_id );
    }
    else
      if ( old_est != NoExists ) {
      ProgTrace( TRACE5, "events: %s, %d, %d",
                 string(string("�������� ���. �६��� �뫥� �/� ") + airp).c_str(), move_id, point_id );
        reqInfo->MsgToLog( string("�������� ���. �६��� �뫥� �/� ") + airp, evtDisp, move_id, point_id );
      }
  }
  tst();
  err++;
  if ( old_act != fl.act ) {
    if ( fl.act != NoExists )
      reqInfo->MsgToLog( string("���⠢����� 䠪�. �६��� �뫥� �/� ")  + airp + " " + DateTimeToStr( fl.act, "hh:nn dd.mm.yy" ) + string(" (UTC)"), evtDisp, move_id, point_id );
    else
      if ( old_act != NoExists )
        reqInfo->MsgToLog( string("�⬥�� 䠪� �뫥� �/� ") + airp, evtDisp, move_id, point_id );
  }
  tst();
	//��।��塞 �६� ����প� �� �ਫ��
  if ( time_in_delay != NoExists ) { //���� ����প� �� �ਫ�� � ᫥�. �㭪�
    TTripRoute routes;
    routes.GetRouteAfter( ASTRA::NoExists, point_id, trtNotCurrent, trtWithCancelled );
    if ( routes.size() > 0 && !routes.begin()->pr_cancel ) {
      ProgTrace( TRACE5, "routes.begin()->point_id=%d,time_in_delay=%f", routes.begin()->point_id, time_in_delay );
      Qry.Clear();
      Qry.SQLText =
        "UPDATE points SET est_in=scd_in+:time_diff WHERE point_id=:point_id";
      Qry.CreateVariable( "point_id", otInteger, routes.begin()->point_id );
      if ( time_in_delay > 0 )
        Qry.CreateVariable( "time_diff", otFloat, time_in_delay );
      else
        Qry.CreateVariable( "time_diff", otFloat, 0.0 );
      Qry.Execute();
    }
  }
  err++;
  Set_AODB_overload_alarm( point_id, overload_alarm );
  TTripStages trip_stages( point_id );
	// ���������� �६�� �孮�����᪮�� ��䨪�
  Qry.Clear();
	Qry.SQLText =
    "UPDATE trip_stages SET est=NVL(:scd,est) "
    " WHERE point_id=:point_id AND stage_id=:stage_id AND act IS NULL";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "stage_id", otInteger, sOpenCheckIn );
  if ( fl.checkin_beg != NoExists )
	  Qry.CreateVariable( "scd", otDate, fl.checkin_beg );
  else
    Qry.CreateVariable( "scd", otDate, FNull );
	err++;
	Qry.Execute();
	if ( Qry.RowsProcessed() > 0 ) {
     if ( fl.checkin_beg != NoExists && trip_stages.time( sOpenCheckIn ) != fl.checkin_beg )
       reqInfo->MsgToLog( string( "�⠯ '" ) + TStagesRules::Instance()->stage_name( sOpenCheckIn, airp, false ) + "': " +
                          " ���. �६�=" + DateTimeToStr( fl.checkin_beg, "hh:nn dd.mm.yy" ) + " (UTC)", evtGraph, point_id, sOpenCheckIn );
	}
	err++;
	Qry.SetVariable( "stage_id", sCloseCheckIn );
	if ( fl.checkin_end  != NoExists )
    Qry.SetVariable( "scd", fl.checkin_end );
  else
	  Qry.SetVariable( "scd", FNull );
	err++;
	Qry.Execute();
	if ( Qry.RowsProcessed() > 0 ) {
     if ( fl.checkin_end != NoExists && trip_stages.time( sCloseCheckIn ) != fl.checkin_end )
       reqInfo->MsgToLog( string( "�⠯ '" ) + TStagesRules::Instance()->stage_name( sCloseCheckIn, airp, false ) + "': " +
                          " ���. �६�=" + DateTimeToStr( fl.checkin_end, "hh:nn dd.mm.yy" ) + " (UTC)", evtGraph, point_id, sCloseCheckIn );
	}
	err++;
	Qry.SetVariable( "stage_id", sOpenBoarding );
	if ( fl.boarding_beg  != NoExists )
	  Qry.SetVariable( "scd", fl.boarding_beg );
  else
    Qry.SetVariable( "scd", FNull );
	err++;
	Qry.Execute();
	if ( Qry.RowsProcessed() > 0 ) {
     if ( fl.boarding_beg != NoExists && trip_stages.time( sOpenBoarding ) != fl.boarding_beg )
       reqInfo->MsgToLog( string( "�⠯ '" ) + TStagesRules::Instance()->stage_name( sOpenBoarding, airp, false ) + "': " +
                          " ���. �६�=" + DateTimeToStr( fl.boarding_beg, "hh:nn dd.mm.yy" ) + " (UTC)", evtGraph, point_id, sOpenBoarding );
	}
	err++;
	// ���⠥� �६� ����砭�� ��ᠤ��
	if ( old_est != fl.est ) { // ��������� ���⭮�� �६��� �뫥�
    fl.boarding_end = trip_stages.time_scd( sCloseBoarding );
    if ( fl.est != NoExists && fl.scd != fl.est ) { // ����প� != 0
      fl.boarding_end += fl.est - fl.scd; // ������塞 � ��������� �६��� ����砭�� ��ᠤ�� ����প� �� �뫥��
    }
  	Qry.SetVariable( "stage_id", sCloseBoarding );
  	if ( fl.boarding_end != NoExists )
  	  Qry.SetVariable( "scd", fl.boarding_end );
    else
      Qry.SetVariable( "scd", FNull );
  	err++;
  	Qry.Execute();
  	if ( Qry.RowsProcessed() > 0 ) {
       if ( fl.boarding_end != NoExists && trip_stages.time( sCloseBoarding ) != fl.boarding_end )
         reqInfo->MsgToLog( string( "�⠯ '" ) + TStagesRules::Instance()->stage_name( sCloseBoarding, airp, false ) + "': " +
                            " ���. �६�=" + DateTimeToStr( fl.boarding_end, "hh:nn dd.mm.yy" ) + " (UTC)", evtGraph, point_id, sCloseBoarding );
	  }
  }
	err++;
	// ���������� �⮥� ॣ����樨 � ��室�� �� ��c����
	Qry.Clear();
	Qry.SQLText =
	 "BEGIN "
	 " :pr_change := 0; "
	 " IF :pr_del != 0 THEN "
	 "   DELETE trip_stations	WHERE point_id=:point_id AND desk=:desk AND work_mode=:work_mode; "
	 "   :pr_change := 1; "
	 " ELSE "
	 "  UPDATE trip_stations SET desk=desk WHERE point_id=:point_id AND desk=:desk AND work_mode=:work_mode; "
 	 "  IF SQL%NOTFOUND THEN "
 	 "   INSERT INTO trip_stations(point_id,desk,work_mode,pr_main) "
	 "    VALUES(:point_id,:desk,:work_mode,0); "
	 "   :pr_change := 1; "
 	 "  END IF; "
	 " END IF; "
	 "END;";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.DeclareVariable( "pr_del", otInteger );
	Qry.DeclareVariable( "desk", otString );
	Qry.DeclareVariable( "work_mode", otString );
	Qry.DeclareVariable( "pr_change", otInteger );
//ProgTrace( TRACE5, "fl.terms.size()=%zu, point_id=%d", fl.terms.size(), point_id );
  string reg, reg_del, brd, brd_del;
  bool pr_change_reg = false, pr_change_brd = false;
	for ( vector<AODB_Term>::iterator it=fl.terms.begin(); it!=fl.terms.end(); it++ ) {
		Qry.SetVariable( "desk", it->name );
		Qry.SetVariable( "work_mode", it->type );
		Qry.SetVariable( "pr_del", it->pr_del );
//ProgTrace( TRACE5, "desk=%s, work_mode=%s, pr_del=%d", it->name.c_str(), it->type.c_str(), it->pr_del );
    err++;
		Qry.Execute();
		err++;
		if ( it->type == "�" )
			pr_change_reg = pr_change_reg || Qry.GetVariableAsInteger( "pr_change" );
		else
			pr_change_brd = pr_change_brd || Qry.GetVariableAsInteger( "pr_change" );
		if ( it->pr_del )
		  if ( it->type == "�" )
		  	reg_del = reg_del + " " + it->name;
		  else
		  	brd_del = brd_del + " " + it->name;
		else
		  if ( it->type == "�" )
		  	reg = reg + " " + it->name;
		  else
		  	brd = brd + " " + it->name;
	}
	if ( pr_change_reg ) {
		if ( !reg_del.empty() )
	    reqInfo->MsgToLog( string( "�������� �⮥� ॣ����樨" ) + reg_del, evtDisp, move_id, point_id );
		if ( !reg.empty() )
	    reqInfo->MsgToLog( string( "�����祭�� �⮥� ॣ����樨" ) + reg, evtDisp, move_id, point_id );
	}
	if ( pr_change_brd ) {
		if ( !brd_del.empty() )
		  reqInfo->MsgToLog( string( "�������� ��室�� �� ��ᠤ��" ) + brd_del, evtDisp, move_id, point_id );
		if ( !brd.empty() )
		  reqInfo->MsgToLog( string( "�����祭�� ��室�� �� ��ᠤ��" ) + brd, evtDisp, move_id, point_id );
	}
	if ( pr_change_reg || pr_change_brd ) check_DesksGates( point_id );
	
  bindingAODBFlt( point_addr, point_id, fl.id );
  err++;
  TTlgBinding(true).bind_flt_oper(flts);
  TTrferBinding().bind_flt_oper(flts);
	tst();
	if ( old_act != fl.act ) {
    if ( old_act == NoExists && fl.act > NoExists ) {
      try {
       	exec_stage( point_id, sTakeoff );
      }
      catch(EOracleError &E) {
        ProgError( STDLOG, "EOracleError %d: %s", E.Code, E.what());
        ProgError( STDLOG, "SQL: %s", E.SQLText());
      }
      catch( std::exception &E ) {
        ProgError( STDLOG, "AODB exec_stage: Takeoff. std::exception: %s", E.what() );
      }
      catch( ... ) {
        ProgError( STDLOG, "AODB exec_stage: Unknown error" );
      };
    }
 	  ChangeACT_OUT( point_id, old_act, fl.act );
  }
}
catch(EOracleError &E)
{
  ProgError( STDLOG, "EOracleError %d: %s", E.Code, E.what());
  ProgError( STDLOG, "SQL: %s", E.SQLText());
  throw;
}
catch(Exception &E)
{
  throw;
}
catch(...)
{
  ProgError( STDLOG, "AODB error=%d, what='Unknown error', msg=%s", err, linestr.c_str() );
  throw;
}
}

/*
 create table aodb_spp (
 name varchar2(50) not null,
 receiver varchar2(6) not null,
 rec_no number(9),
 value varchar2(4000),
 error varchar2(4000) );

 create table aodb_airlines (
  code varchar2(5) not null,
  airline varchar2(3) not null );

 create table aodb_airps (
  code varchar2(3) not null,
  airp varchar2(3) not null );


 create table aodb_crafts (
  code varchar2(10) not null,
  craft varchar2(3) not null );

 create table aodb_liters (
  code varchar2(3) not null,
  litera varchar2(3) not null );
 */

void ParseAndSaveSPP( const std::string &filename, const std::string &canon_name,
                      const std::string &airline, const std::string &airp,
	                    std::string &fd, const string &convert_aodb )
{
	TQuery QryLog( &OraSession );
	QryLog.SQLText =
	 "INSERT INTO aodb_events(filename,point_addr,airline,rec_no,record,msg,time,type) "
	 " SELECT :filename,:point_addr,:airline,:rec_no,:record,:msg,system.UTCSYSDATE,:type FROM dual ";
 	QryLog.CreateVariable( "filename", otString, filename );
	QryLog.CreateVariable( "point_addr", otString, canon_name );
	QryLog.CreateVariable( "airline", otString, airline );
	QryLog.DeclareVariable( "rec_no", otInteger );
	QryLog.DeclareVariable( "record", otString );
	QryLog.DeclareVariable( "msg", otString );
	QryLog.DeclareVariable( "type", otString );
	string errs;
	string linestr;
	char c_n = 13, c_a = 10;
	int max_rec_no = -1;
  while ( !fd.empty() ) {
  	std::string::size_type i = fd.find( c_n );
  	if ( i == string::npos ) {
  		linestr = fd;
  		fd.clear();
  	}
  	else {
  	  linestr = fd.substr( 0, i );
  	  if ( i < fd.length() - 1 && fd[ i + 1 ] == c_a ) {
  		  i++;
  		}
  	  fd.erase( 0, i + 1 );
  	}
    AODB_Flight fl;
    try {
    	fl.rec_no = NoExists;
    	if ( !convert_aodb.empty() ) {
    		try {
    		  linestr = ConvertCodepage( linestr, convert_aodb, "CP866" );
    		}
	      catch( EConvertError &E ) {
	      	string l;
	      	try {
	      		l = ConvertCodepage( linestr.substr( REC_NO_IDX, REC_NO_LEN ), convert_aodb, "CP866" );
	      	}
	        catch( EConvertError &E ) {
	        	throw Exception( string("�訡�� ��४���஢�� ३�") );
	        }
	      	throw Exception( string("�訡�� ��४���஢�� ३�, ��ப� ") + l );
	      }
      }
      ParseFlight( canon_name, airp, linestr, fl );
      QryLog.SetVariable( "rec_no", fl.rec_no );
      if ( linestr.empty() )
      	QryLog.SetVariable( "record", "empty line!" );
      else
        QryLog.SetVariable( "record", linestr );
      if ( fl.invalid_field.empty() )
    	  QryLog.SetVariable( "msg", "ok" );
      else
        QryLog.SetVariable( "msg", fl.invalid_field );
    	QryLog.SetVariable( "type", EncodeEventType( ASTRA::evtFlt ) );
      QryLog.Execute();
    }
    catch( Exception &e ) {
      try { OraSession.Rollback(); }catch(...){};
      if ( fl.rec_no == NoExists )
      	QryLog.SetVariable( "rec_no", -1 );
      else
        QryLog.SetVariable( "rec_no", fl.rec_no );
      if ( linestr.empty() )
      	QryLog.SetVariable( "record", "empty line!" );
      else
        QryLog.SetVariable( "record", linestr );
    	QryLog.SetVariable( "msg", e.what() );
    	QryLog.SetVariable( "type", EncodeEventType( ASTRA::evtProgError ) );
      QryLog.Execute();
      if ( !errs.empty() )
      	errs += c_n/* + c_a*/;
      if ( fl.rec_no == NoExists )
        errs += string( "�訡�� ࠧ��� ��ப�: " ) + string(e.what()).substr(0,120);
      else
      	errs += string( "�訡�� ࠧ��� ��ப� " ) + IntToString( fl.rec_no ) + " : " + string(e.what()).substr(0,120);
    }
    if ( fl.rec_no > NoExists )
      max_rec_no = fl.rec_no;
    OraSession.Commit();
  }
	TQuery Qry( &OraSession );
	Qry.SQLText = "UPDATE aodb_spp_files SET rec_no=:rec_no WHERE filename=:filename AND point_addr=:point_addr AND airline=:airline";
	Qry.CreateVariable( "rec_no", otInteger, max_rec_no );
	Qry.CreateVariable( "filename", otString, filename );
	Qry.CreateVariable( "point_addr", otString, canon_name );
	Qry.CreateVariable( "airline", otString, airline );
	Qry.Execute();
/*	if ( !errs.empty() )
	 AstraLocale::showProgError( errs ); !!!*/
}

bool BuildAODBTimes( int point_id, const std::string &point_addr,
                     TAODBFormat format, TFileDatas &fds )
{
	TFileData fd;
	TQuery Qry( &OraSession );
	Qry.SQLText =
	 "SELECT aodb_point_id,airline||flt_no||suffix trip,scd_out,airp,aodb_points.overload_alarm, "
	 "       rec_no_flt "
	 " FROM points, aodb_points "
	 " WHERE points.point_id=:point_id AND "
	 "       points.point_id=aodb_points.point_id(+) AND "
	 "       :point_addr=aodb_points.point_addr(+) ";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.Execute();
	if ( Qry.Eof )
		return false;
		
  string flight = Qry.FieldAsString( "trip" );
	string region = getRegion( Qry.FieldAsString( "airp" ) );
	TDateTime scd_out = UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region );
	double aodb_point_id = Qry.FieldAsFloat( "aodb_point_id" );
	int rec_no;
	string old_record;
	if ( Qry.Eof || Qry.FieldIsNULL( "rec_no_flt" ) )
		rec_no = 0;
	else {
	  rec_no = Qry.FieldAsInteger( "rec_no_flt" ) + 1;
	  get_string_into_snapshot_points( point_id, FILE_AODB_OUT_TYPE, point_addr, old_record );
	}
	int checkin = 0, boarding = 0;
	TMapTripStages stages;
	TTripStages::LoadStages( point_id, stages );
	ostringstream record;
	record<<setfill(' ');
	if ( stages[ sOpenCheckIn ].act > NoExists )
		record<<setw(16)<<DateTimeToStr( UTCToLocal( stages[ sOpenCheckIn ].act, region ), "dd.mm.yyyy hh:nn" );
	else
		record<<setw(16)<<" ";
	if ( stages[ sCloseCheckIn ].act > NoExists )
		record<<setw(16)<<DateTimeToStr( UTCToLocal( stages[ sCloseCheckIn ].act, region ), "dd.mm.yyyy hh:nn" );
	else
		record<<setw(16)<<" ";
	if ( stages[ sOpenBoarding ].act > NoExists )
		record<<setw(16)<<DateTimeToStr( UTCToLocal( stages[ sOpenBoarding ].act, region ), "dd.mm.yyyy hh:nn" );
	else
		record<<setw(16)<<" ";
	if ( stages[ sCloseBoarding ].act > NoExists )
		record<<setw(16)<<DateTimeToStr( UTCToLocal( stages[ sCloseBoarding ].act, region ), "dd.mm.yyyy hh:nn" );
	else
		record<<setw(16)<<" ";
  checkin = ( stages[ sOpenCheckIn ].act > NoExists && stages[ sCloseCheckIn ].act == NoExists );
	boarding = ( stages[ sOpenBoarding ].act > NoExists && stages[ sCloseBoarding ].act == NoExists );
	record<<setw(1)<<checkin<<setw(1)<<boarding<<setw(1)<<Qry.FieldAsInteger( "overload_alarm" );
	TQuery StationsQry( &OraSession );

	StationsQry.Clear();
	StationsQry.SQLText =
	 "SELECT name, start_time FROM trip_stations, stations "
	 " WHERE point_id=:point_id AND trip_stations.work_mode='�' AND "
	 "       trip_stations.desk=stations.desk AND trip_stations.work_mode=stations.work_mode";
	StationsQry.CreateVariable( "point_id", otInteger, point_id );
	StationsQry.Execute();
	while ( !StationsQry.Eof ) {
    string term = StationsQry.FieldAsString( "name" );
    if ( !term.empty() && term[0] == 'R' )
    	term = term.substr( 1, term.length() - 1 );
		record<<";"<<"�"<<setw(4)<<term.substr(0,4)<<setw(1)<<(int)!StationsQry.FieldIsNULL( "start_time" );
		StationsQry.Next();
	}
	if ( Qry.Eof || record.str() != old_record ) {
		ostringstream r;
		if ( aodb_point_id )
		  r<<std::fixed<<setw(6)<<rec_no<<setw(10)<<setprecision(0)<<aodb_point_id;
		else
			r<<std::fixed<<setw(6)<<rec_no<<setw(10)<<"";
		r<<setw(10)<<flight.substr(0,10)<<setw(16)<<DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" )<<record.str();
		Qry.Clear();
  	Qry.SQLText =
  	 "BEGIN "
   	 " UPDATE aodb_points SET rec_no_flt=NVL(rec_no_flt,-1)+1 "
   	 "  WHERE point_id=:point_id AND point_addr=:point_addr; "
   	 "  IF SQL%NOTFOUND THEN "
   	 "    INSERT INTO aodb_points(point_id,point_addr,aodb_point_id,rec_no_flt,rec_no_pax,rec_no_bag,rec_no_unaccomp) "
   	 "      VALUES(:point_id,:point_addr,NULL,0,-1,-1,-1); "
   	 "  END IF; "
   	 "END;";
   	Qry.CreateVariable( "point_id", otInteger, point_id );
   	Qry.CreateVariable( "point_addr", otString, point_addr );
 	  Qry.Execute();
 	  put_string_into_snapshot_points( point_id, FILE_AODB_OUT_TYPE, point_addr, !old_record.empty(), record.str() );
 	  fd.file_data = r.str() + "\n";
	}
  if ( !fd.file_data.empty() ) {
	  string p = flight + DateTimeToStr( scd_out, "yymmddhhnn" );
    fd.params[ PARAM_FILE_NAME ] =  p + "reg.txt";
	  fd.params[ NS_PARAM_EVENT_TYPE ] = EncodeEventType( ASTRA::evtFlt );
	  fd.params[ NS_PARAM_EVENT_ID1 ] = IntToString( point_id );
    fd.params[ PARAM_TYPE ] = VALUE_TYPE_FILE; // FILE
    fds.push_back( fd );
	}
	return !fds.empty();
}

bool createAODBFiles( int point_id, const std::string &point_addr, TFileDatas &fds )
{
  TAODBFormat format;
	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT airp FROM points WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	format = afDefault;
	if ( !Qry.Eof ) {
    string airp = Qry.FieldAsString( "airp" );
    if ( airp == "���" )
      format = afNewUrengoy;
  }
  createAODBCheckInInfoFile( point_id, false, point_addr, format, fds );
  createAODBCheckInInfoFile( point_id, true, point_addr, format, fds );
  BuildAODBTimes( point_id, point_addr, format, fds );
	return !fds.empty();
}
/*
 �����頥� true ⮫쪮 � ��砥, ����� ���� ��ॣ�㧪� � ��� ��������� � max_commerce
*/
bool Get_AODB_overload_alarm( int point_id, int max_commerce )
{
	TQuery Qry( &OraSession );
	Qry.SQLText =
	  "SELECT max_commerce, aodb_points.overload_alarm FROM aodb_points, trip_sets "
	  " WHERE aodb_points.point_id=:point_id AND aodb_points.point_id=trip_sets.point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	return ( !Qry.Eof &&
	         Qry.FieldAsInteger( "max_commerce" ) == max_commerce &&
	         Qry.FieldAsInteger( "overload_alarm" ) );
}

void Set_AODB_overload_alarm( int point_id, bool overload_alarm )
{
	TQuery Qry( &OraSession );
	Qry.SQLText =
	  "UPDATE aodb_points SET overload_alarm=:overload_alarm WHERE point_id=:point_id";
	Qry.CreateVariable( "overload_alarm", otInteger, overload_alarm );
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
}

void parseIncommingAODBData()
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT file_queue.id, data FROM file_queue, files "
    " WHERE file_queue.sender=:sender AND "
    "       file_queue.receiver=:receiver AND "
    "       file_queue.type=:type AND "
    "       file_queue.id=files.id "
    " ORDER BY file_queue.time";
  Qry.CreateVariable( "sender", otString, OWN_POINT_ADDR() );
  Qry.CreateVariable( "receiver", otString, OWN_POINT_ADDR() );
  Qry.CreateVariable( "type", otString, FILE_AODB_IN_TYPE );
  Qry.Execute();
  TQuery QryParams( &OraSession );
  QryParams.SQLText = "SELECT name, value FROM file_params WHERE id=:file_id";
  QryParams.DeclareVariable( "file_id", otInteger );
	Qry.Execute();
  string airline;
  map<string,string> fileparams;
  int len;
  void *p = NULL;
  try {
    while ( !Qry.Eof ) {
     	len = Qry.GetSizeLongField( "data" );
      if ( p )
      	p = (char*)realloc( p, len );
      else
        p = (char*)malloc( len );
      if ( !p )
      	throw Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
      Qry.FieldAsLong( "data", p );
      QryParams.SetVariable( "file_id", Qry.FieldAsInteger( "id" ) );
      QryParams.Execute();
      while ( !QryParams.Eof ) {
        fileparams[ QryParams.FieldAsString( "name" ) ] = QryParams.FieldAsString( "value" );
        QryParams.Next();
      }

      string convert_aodb = getFileEncoding( FILE_AODB_IN_TYPE, fileparams[ PARAM_CANON_NAME ], false );
/*      ProgTrace( TRACE5, "convert_aodb=%s, fileparams[ PARAM_CANON_NAME ]=%s, fileparams[ NS_PARAM_AIRLINE ]=%s",
                 convert_aodb.c_str(), fileparams[ PARAM_CANON_NAME ].c_str(), fileparams[ NS_PARAM_AIRLINE ].c_str( ) );*/
      string str_file( (char*)p, len );
      TReqInfo::Instance()->desk.code = fileparams[ PARAM_CANON_NAME ];
      ParseAndSaveSPP( fileparams[ PARAM_FILE_NAME ], fileparams[ PARAM_CANON_NAME ] ,
                       fileparams[ NS_PARAM_AIRLINE ], fileparams[ NS_PARAM_AIRP ],
	                     str_file, convert_aodb );
      ProgTrace( TRACE5, "deleteFile id=%d", Qry.FieldAsInteger( "id" ) );
      deleteFile( Qry.FieldAsInteger( "id" ) );
      OraSession.Commit();
  	  Qry.Next();
    }
  }
  catch(...) {
   if ( p ) free( p );
   throw;
  }
  if ( p ) free( p );
}

int main_aodb_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(1);
    InitLogTime(NULL);
    OpenLogFile("log1");

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();

     if (init_edifact()<0) throw Exception("'init_edifact' error");
    
    TReqInfo::Instance()->clear();
    char buf[10];
    for(;;)
    {
      emptyHookTables();
      TDateTime execTask = NowUTC();
      InitLogTime(NULL);
      base_tables.Invalidate();
      parseIncommingAODBData();
      OraSession.Commit();
      if ( NowUTC() - execTask > 5.0/(1440.0*60.0) )
      	ProgTrace( TRACE5, "Attention execute task time!!!, name=%s, time=%s","CMD_PARSE_AODB", DateTimeToStr( NowUTC() - execTask, "nn:ss" ).c_str() );
      callPostHooksAfter();
      waitCmd("CMD_PARSE_AODB",WAIT_INTERVAL,buf,sizeof(buf));
    }; // end of loop
  }
  catch(EOracleError &E)
  {
    ProgError( STDLOG, "EOracleError %d: %s", E.Code, E.what());
    ProgError( STDLOG, "SQL: %s", E.SQLText());
  }
  catch(std::exception &E)
  {
    ProgError( STDLOG, "std::exception: %s", E.what() );
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };
  try
  {
    OraSession.Rollback();
    OraSession.LogOff();
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };
  return 0;
};

bool is_sync_aodb_pax( const TTripInfo &tripInfo )
{
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT id FROM file_param_sets "
    " WHERE ( file_param_sets.airp IS NULL OR file_param_sets.airp=:airp ) AND "
		"       ( file_param_sets.airline IS NULL OR file_param_sets.airline=:airline ) AND "
		"       ( file_param_sets.flt_no IS NULL OR file_param_sets.flt_no=:flt_no ) AND "
		"       file_param_sets.type=:type AND pr_send=1 AND own_point_addr=:own_point_addr";
	Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "type", otString, FILE_AODB_OUT_TYPE );
	Qry.CreateVariable( "airline", otString, tripInfo.airline );
	Qry.CreateVariable( "airp", otString, tripInfo.airp );
	Qry.CreateVariable( "flt_no", otInteger, tripInfo.flt_no );
  Qry.Execute();
  return ( !Qry.Eof );
};

void VerifyParseFlight( )
{
	std::string linestr =
"     1    184071     ��437   01.09.2008 21:5001.09.2008 21:5001.09.2008 22:04 2   85134 11474    ��154�     8568101.09.2008 20:2001.09.2008 21:1001.09.2008 21:1001.09.2008 21:3000;1���0;�  130;�  140;�  190;�  200;�  210;�  220;�  230;�  240;�  250;�  260;�  270;�  350;�  360;�  370;� 5010;� 5020;� 5030;� 5040;� 5050;� 5060;�   40;";
  AODB_Flight fl;
  string point_addr = "DJEK";
  ProgTrace( TRACE5, "linestr=%s", linestr.c_str() );
  ParseFlight( point_addr, "���", linestr, fl );
  tst();
}


