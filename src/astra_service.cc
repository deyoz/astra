#include <stdlib.h>
#include <string>
#include <map>
#include "astra_service.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "oralib.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "basic.h"
#include "stl_utils.h"
#include "develop_dbf.h"
#include "flight_cent_dbf.h"
#include "sofi.h"
#include "aodb.h"
#include "spp_cek.h"
#include "timer.h"
#include "stages.h"
#include "maindcs.h"
#include "base_tables.h"
#include "astra_misc.h"
#include "jxtlib/jxt_cont.h"
#include "serverlib/str_utils.h"
#include "tlg/tlg.h"
#include "serverlib/posthooks.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC;

const double WAIT_ANSWER_SEC = 30.0;   // ���� �⢥� 30 ᥪ㭤
const string PARAM_FILE_ID = "file_id";
const string PARAM_NEXT_FILE = "NextFile";

void CommitWork( int file_id );

bool createCheckinDataFiles( int point_id, const std::string &point_addr, TFileDatas &fds );
bool CreateCommonFileData( const std::string &point_addr,
                           int id, const std::string type,
                           const std::string &airp, const std::string &airline,
	                         const std::string &flt_no );


bool deleteFile( int id )
{
    TQuery Qry(&OraSession);
    Qry.SQLText=
      " BEGIN "
      " DELETE file_queue WHERE id= :id; "
      "END; ";
    Qry.CreateVariable("id",otInteger,id);
    Qry.Execute();
    return Qry.RowsProcessed()>0;
};

int putFile( const string &receiver,
             const string &sender,
             const string &type,
             map<string,string> &params,
             const string &file_data )
{
	int file_id = ASTRA::NoExists;
    try
    {
        TQuery Qry(&OraSession);
        Qry.SQLText = "SELECT tlgs_id.nextval id FROM dual";
        Qry.Execute();
        file_id = Qry.FieldAsInteger( "id" );
        Qry.Clear();
        Qry.SQLText=
                "INSERT INTO "
                "file_queue(id,sender,receiver,type,status,time) "
                "VALUES"
                "(tlgs_id.currval,:sender,:receiver,"
                ":type,'PUT',system.UTCSYSDATE)";
        Qry.CreateVariable("sender",otString,sender);
        Qry.CreateVariable("receiver",otString,receiver);
        Qry.CreateVariable("type",otString,type);
        Qry.Execute();
        Qry.SQLText=
                "INSERT INTO "
                "files(id,sender,receiver,type,time,data,error) "
                "VALUES"
                "(tlgs_id.currval,:sender,:receiver,"
                ":type,system.UTCSYSDATE,:data,NULL)";
        Qry.DeclareVariable("data",otLongRaw);
        Qry.SetLongVariable("data",(void*)file_data.c_str(),file_data.size());
        Qry.Execute();
        Qry.Close();

        Qry.Clear();
        Qry.SQLText=
                "INSERT INTO file_params(id,name,value) "
                "VALUES(tlgs_id.currval,:name,:value)";
        Qry.DeclareVariable("name",otString);
        Qry.DeclareVariable("value",otString);
        for(map<string,string>::iterator i=params.begin();i!=params.end();i++)
        {
        	if ( i->first.empty() )
        		continue;
          Qry.SetVariable("name",i->first);
          Qry.SetVariable("value",i->second);
          Qry.Execute();
        };
    }
    catch( std::exception &e)
    {
    	try {deleteFile( file_id );} catch(...){};
      ProgError(STDLOG, e.what());
      throw;
    }
    catch(...)
    {
    	try {deleteFile( file_id );} catch(...){};
      ProgError(STDLOG, "putFile: Unknown error while trying to put file");
      throw;
    };
  return file_id;
};

bool errorFile( int id, const string &msg )
{
	TQuery ErrQry(&OraSession);
	ErrQry.SQLText =
	 "SELECT in_order FROM file_types, files "
	 " WHERE files.id=:id AND files.type=file_types.code";
	ErrQry.CreateVariable( "id", otInteger, id );
	ErrQry.Execute();

  if ( ( ErrQry.Eof || !ErrQry.FieldAsInteger( "in_order" ) ) && deleteFile(id) ) {
    ErrQry.Clear();
    ErrQry.SQLText=
     "BEGIN "
     " UPDATE files SET error=:error,time=system.UTCSYSDATE WHERE id=:id; "
     " INSERT INTO file_error(id,msg) VALUES(:id,:msg); "
     "END;";
    ErrQry.CreateVariable( "error", otString, "ERR" );
    ErrQry.CreateVariable( "id", otInteger, id );
    ErrQry.CreateVariable( "msg", otString, msg );
    ErrQry.Execute();
    return ErrQry.RowsProcessed()>0;
  }
  else return false;
};

bool sendFile( int id )
{
	TQuery SendQry(&OraSession);
  SendQry.SQLText="UPDATE file_queue SET status='SEND', time=system.UTCSYSDATE WHERE id= :id";
  SendQry.CreateVariable("id",otInteger,id);
  SendQry.Execute();
  return SendQry.RowsProcessed()>0;
}

bool doneFile( int id )
{
  if ( deleteFile( id ) ) {
    TQuery DoneQry(&OraSession);
    DoneQry.SQLText=" UPDATE files SET time=system.UTCSYSDATE WHERE id=:id ";
    DoneQry.CreateVariable("id",otInteger,id);
    DoneQry.Execute();
    return DoneQry.RowsProcessed()>0;
  }
  else
  	return false;
}

struct TSQLCondDates {
  string sql;
  map<string,TDateTime> dates;
};

class TPointAddr {
  string type;
  bool pr_send;
public:
  TPointAddr( const string &vtype, bool vpr_send ) {
    type = vtype;
    pr_send = vpr_send;
  }
  virtual ~TPointAddr() {};
  virtual bool validateParams( const string &point_addr, const vector<string> &ailines,
                               const vector<string> &airps, const vector<int> &flt_nos ) {
    return true;
  }
  virtual bool validatePoints( int point_id ) {
    return true;
  }
  void createPointSQL( const TSQLCondDates &cond_dates ) {
    TQuery Qry( &OraSession );
    TQuery PointsQry( &OraSession );
    Qry.SQLText =
      "SELECT point_addr,airline,flt_no,airp,param_name FROM file_param_sets, desks "
      " WHERE type=:type AND own_point_addr=:own_point_addr AND pr_send=:pr_send AND "
      "       desks.code=file_param_sets.point_addr "
      "ORDER BY point_addr";
  	string point_addr;
    vector<string> airlines, airps;
	  vector<int> flt_nos;
	  bool pr_empty_airline = false, pr_empty_airp = false, pr_empty_flt_no = false;
	  Qry.CreateVariable( "type", otString, type );
	  Qry.CreateVariable( "pr_send", otInteger, pr_send );
	  Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	  //ProgTrace( TRACE5, "type=%s, pr_send=%d, own_point_addr=%s", type.c_str(), pr_send, OWN_POINT_ADDR() );
	  Qry.Execute();
	  while ( true ) {
      //ProgTrace( TRACE5, "Qry.Eof=%d, point_addr=%s", Qry.Eof, point_addr.c_str() );
      if ( !point_addr.empty() && ( Qry.Eof || point_addr != Qry.FieldAsString( "point_addr" ) ) ) { //���室 � ᫥�. point_addr
        if ( pr_empty_airline )
          airlines.clear();
        if ( pr_empty_airp )
          airps.clear();
        if ( pr_empty_flt_no )
          flt_nos.clear();
        ProgTrace( TRACE5, "point_addr=%s, airlines.size()=%d, airps.size()=%d,flt_nos.size()=%d",
                   point_addr.c_str(), airlines.size(), airps.size(),  flt_nos.size() );
        if ( validateParams( point_addr, airlines, airps, flt_nos ) ) {
          PointsQry.Clear();
          string res = "SELECT point_id, airline, airp, flt_no FROM points WHERE ";
          if ( cond_dates.sql.empty() ) //⠪��� �� ������ ����
            res = " 1=1 AND ";
          else
            res += cond_dates.sql;
          if ( !airps.empty() ) {
            res += " AND airp IN (";
            int idx=0;
            for ( vector<string>::const_iterator istr=airps.begin(); istr!=airps.end(); istr++ ) {
              if ( istr != airps.begin() )
                res += ",";
              res += ":airp"+IntToString(idx);
              PointsQry.CreateVariable( "airp"+IntToString(idx), otString, *istr );
//              ProgTrace( TRACE5, "airp idx=%d, value=%s", idx, istr->c_str() );
              idx++;
            }
            res += ")";
          }
          if ( !airlines.empty() ) {
            res += " AND airline IN (";
            int idx=0;
            for ( vector<string>::const_iterator istr=airlines.begin(); istr!=airlines.end(); istr++ ) {
              if ( istr != airlines.begin() )
                res += ",";
              res += ":airline"+IntToString(idx);
              PointsQry.CreateVariable( "airline"+IntToString(idx), otString, *istr );
  //            ProgTrace( TRACE5, "airline idx=%d, value=%s", idx, istr->c_str() );
              idx++;
            }
            res += ")";
          }
          if ( !flt_nos.empty() ) {
            res += " AND flt_no IN (";
            int idx=0;
            for ( vector<int>::const_iterator iint=flt_nos.begin(); iint!=flt_nos.end(); iint++ ) {
              if ( iint != flt_nos.begin() )
                res += ",";
              res += ":flt_no"+IntToString(idx);
              PointsQry.CreateVariable( "flt_no"+IntToString(idx), otInteger, *iint );
    //          ProgTrace( TRACE5, "flt_no idx=%d, value=%d", idx, *iint );
              idx++;
            }
            res += ")";
          }
          for ( map<string,TDateTime>::const_iterator idate=cond_dates.dates.begin();
                idate!=cond_dates.dates.end(); idate++ ) {
            PointsQry.CreateVariable( idate->first, otDate, idate->second );
          }
          PointsQry.SQLText = res;
          ProgTrace( TRACE5, "type=%s, PointsQry.SQLText=%s, point_addr=%s", type.c_str(), res.c_str(), point_addr.c_str() );
          PointsQry.Execute();
          while ( !PointsQry.Eof ) {
            if ( validatePoints( PointsQry.FieldAsInteger( "point_id" ) ) ) {
            ProgTrace( TRACE5, "type=%s, point_addr=%s, point_id=%d", type.c_str(), point_addr.c_str(), PointsQry.FieldAsInteger( "point_id" ) );
            CreateCommonFileData( point_addr,
                                  PointsQry.FieldAsInteger( "point_id" ),
                                  type,
                                  PointsQry.FieldAsString( "airp" ),
  	                              PointsQry.FieldAsString( "airline" ),
                                  PointsQry.FieldAsString( "flt_no" ) );
/*              ProgTrace( TRACE5, "point_addr=%s, airp=%s, airline=%s, flt_no=%d",
                         point_addr.c_str(), PointsQry.FieldAsString( "airp" ), PointsQry.FieldAsString( "airline" ),
                         PointsQry.FieldAsInteger( "flt_no" ) );*/
            }
            PointsQry.Next();
          }
        }
        airlines.clear();
        airps.clear();
        flt_nos.clear();
        pr_empty_airline = false;
        pr_empty_airp = false;
        pr_empty_flt_no = false;
      }
      if ( Qry.Eof )
        break;
      point_addr = Qry.FieldAsString( "point_addr" );
      if ( Qry.FieldIsNULL( "airline" ) )
        pr_empty_airline = true;
      else
        if ( find( airlines.begin(), airlines.end(), string(Qry.FieldAsString( "airline" )) ) == airlines.end() )
          airlines.push_back( Qry.FieldAsString( "airline" ) );
      if ( Qry.FieldIsNULL( "airp" ) )
        pr_empty_airp = true;
      else
        if ( find( airps.begin(), airps.end(), string(Qry.FieldAsString( "airp" )) ) == airps.end() )
          airps.push_back( Qry.FieldAsString( "airp" ) );
      if ( Qry.FieldIsNULL( "flt_no" ) )
        pr_empty_flt_no = true;
      else
        if ( find( flt_nos.begin(), flt_nos.end(), Qry.FieldAsInteger( "flt_no" ) ) == flt_nos.end() )
          flt_nos.push_back( Qry.FieldAsInteger( "flt_no" ) );
      ProgTrace( TRACE5, "point_addr=%s, param_name=%s,airline=%s,airp=%s,flt_no=%s",
                 point_addr.c_str(), Qry.FieldAsString( "param_name" ), Qry.FieldAsString( "airline" ),
                 Qry.FieldAsString( "airp" ),Qry.FieldAsString( "flt_no" ) );
      Qry.Next();
	  }
  }
};

void getFileParams( const std::string client_canon_name, const std::string &type,
	                  int id, map<string,string> &fileparams, bool send )
{
	fileparams.clear();
	TQuery ParamQry( &OraSession );
	ParamQry.SQLText = "SELECT name,value FROM file_params WHERE id=:id";
	ParamQry.CreateVariable( "id", otInteger, id );
  ParamQry.Execute();
//  ProgTrace( TRACE5, "id=%d", id );
  string airline, airp, flt_no;
	while ( !ParamQry.Eof ) {
		if ( ParamQry.FieldAsString( "name" ) == NS_PARAM_AIRP )
			airp = ParamQry.FieldAsString( "value" );
		else
			if ( ParamQry.FieldAsString( "name" ) == NS_PARAM_AIRLINE )
				airline = ParamQry.FieldAsString( "value" );
			else
				if ( ParamQry.FieldAsString( "name" ) == NS_PARAM_FLT_NO )
					flt_no = ParamQry.FieldAsString( "value" );
				else
					if ( ParamQry.FieldAsString( "name" ) != NS_PARAM_EVENT_TYPE &&
						   ParamQry.FieldAsString( "name" ) != NS_PARAM_EVENT_ID1 &&
						   ParamQry.FieldAsString( "name" ) != NS_PARAM_EVENT_ID2 &&
						   ParamQry.FieldAsString( "name" ) != NS_PARAM_EVENT_ID3 ) {
					  fileparams[ string( ParamQry.FieldAsString( "name" ) ) ] = ParamQry.FieldAsString( "value" );
//		      ProgTrace( TRACE5, "name=%s, value=%s", ParamQry.FieldAsString( "name" ), ParamQry.FieldAsString( "value" ) );
			    }
		ParamQry.Next();
	}
	ParamQry.Clear();
	ParamQry.SQLText =
	  "SELECT param_name, param_value,"
    "       DECODE( file_param_sets.airp, NULL, 0, 4 ) + "
    "       DECODE( file_param_sets.airline, NULL, 0, 2 ) + "
    "       DECODE( file_param_sets.flt_no, NULL, 0, 1 ) AS priority "
    " FROM file_param_sets, desks "
	  " WHERE file_param_sets.own_point_addr=:own_point_addr AND "
	  "       file_param_sets.point_addr=:point_addr AND "
	  "       file_param_sets.type=:type AND "
	  "       file_param_sets.pr_send=:send AND "
	  "       desks.code=file_param_sets.point_addr AND "
	  "       ( file_param_sets.airp IS NULL OR file_param_sets.airp=:airp ) AND "
	  "       ( file_param_sets.airline IS NULL OR file_param_sets.airline=:airline ) AND "
	  "       ( file_param_sets.flt_no IS NULL OR file_param_sets.flt_no=:flt_no ) "
	  " ORDER BY priority DESC";
	if ( airp.empty() )
		ParamQry.CreateVariable( "airp", otString, FNull );
	else
		ParamQry.CreateVariable( "airp", otString, airp );
	if ( airline.empty() )
		ParamQry.CreateVariable( "airline", otString, FNull );
	else
		ParamQry.CreateVariable( "airline", otString, airline );
	if ( flt_no.empty() )
		ParamQry.CreateVariable( "flt_no", otInteger, FNull );
	else
		ParamQry.CreateVariable( "flt_no", otInteger, flt_no );
	ParamQry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	ParamQry.CreateVariable( "point_addr", otString, client_canon_name );
	ParamQry.CreateVariable( "type", otString, type );
  ParamQry.CreateVariable( "send", otInteger, send );
	ParamQry.Execute();
	int priority = -1;
	while ( !ParamQry.Eof ) {
		if ( priority < 0 ) {
			priority = ParamQry.FieldAsInteger( "priority" );
		}
		if ( priority != ParamQry.FieldAsInteger( "priority" ) )
			break;
		fileparams[ ParamQry.FieldAsString( "param_name" ) ] = ParamQry.FieldAsString( "param_value" );
//		ProgTrace( TRACE5, "name=%s, value=%s", ParamQry.FieldAsString( "param_name" ), ParamQry.FieldAsString( "param_value" ) );
		ParamQry.Next();
  }
  // ���뢨��!!!! ���� ��!!!
  if ( type != "BSM" )
    fileparams[ PARAM_FILE_TYPE ] = type;
  else
  	fileparams[ PARAM_TYPE ] = type;
  fileparams[ PARAM_FILE_ID ] = IntToString( id );
	ParamQry.Clear();
	ParamQry.SQLText = "SELECT NVL(in_order,0) as in_order FROM file_types WHERE code=:type";
	ParamQry.CreateVariable( "type", otString, type );
	ParamQry.Execute();
//	ProgTrace( TRACE5, "type=%s", type.c_str() );
	if ( !ParamQry.Eof && ParamQry.FieldAsInteger( "in_order" ) ) {
		fileparams[ PARAM_IN_ORDER ] = "TRUE";
	}
}

void buildFileParams( xmlNodePtr dataNode, const map<string,string> &fileparams )
{
	dataNode = NewTextChild( dataNode, "headers" );
	for ( map<string,string>::const_iterator i=fileparams.begin(); i!=fileparams.end(); i++ ) {
		xmlNodePtr n = NewTextChild( dataNode, "param" );
		NewTextChild( n, "key", i->first );
		NewTextChild( n, "value", i->second );
	}
}

void parseFileParams( xmlNodePtr dataNode, map<string,string> &fileparams )
{
	fileparams.clear();
	if ( !dataNode )
		return;
	xmlNodePtr headersNode = GetNode( "headers", dataNode );
	if ( headersNode ) {
		headersNode = headersNode->children;
		while ( headersNode ) {
			fileparams[ NodeAsString( "key", headersNode ) ] = NodeAsString( "value", headersNode );
			headersNode = headersNode->next;
		}
	}
}

int buildSaveFileData( xmlNodePtr resNode, const std::string &client_canon_name, double &wait_time )
{
	int file_id;
	TQuery ScanQry( &OraSession );
	ScanQry.SQLText =
		"SELECT file_queue.id,file_queue.sender,file_queue.receiver,file_queue.type,"
		"       file_queue.status,file_queue.time,files.time as wait_time,system.UTCSYSDATE AS now, "
		"       files.data, NVL(file_types.in_order,0) in_order "
		" FROM file_queue,files,file_types "
		" WHERE file_queue.id=files.id AND "
    "       file_queue.sender=:sender AND "
    "       file_queue.receiver=:receiver AND "
    "       file_queue.type=file_types.code AND "
    "       ( file_queue.status='PUT' OR NVL(file_types.in_order,0)!=0 OR "
    "         file_queue.status='SEND' AND file_queue.time + :wait_answer_sec/(60*60*24) < system.UTCSYSDATE  ) "
    " ORDER BY DECODE(in_order,1,files.time,file_queue.time),file_queue.id";
	ScanQry.CreateVariable( "sender", otString, OWN_POINT_ADDR() );
	ScanQry.CreateVariable( "receiver", otString, client_canon_name );
	ScanQry.CreateVariable( "wait_answer_sec", otInteger, WAIT_ANSWER_SEC );
	ScanQry.Execute();
	map<string,string> fileparams;
	char *p = NULL;
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  vector<string> vecType;
  string in_order_key;
	while ( !ScanQry.Eof ) {
  	file_id = ScanQry.FieldAsInteger( "id" );
   in_order_key = string( ScanQry.FieldAsString( "type" ) ) + ScanQry.FieldAsString( "receiver" );
    try
    {
    	if ( !ScanQry.FieldAsInteger( "in_order" ) || // �� ����� ���冷� ��ࠢ��
    		   !(find( vecType.begin(), vecType.end(), in_order_key ) != vecType.end()) &&
    		   ( ScanQry.FieldAsString( "status" ) != "SEND" || // ��� ��� 䠩� �� �� ��ࠢ��� � �� �뫮 ��। ��� ⠪��� ��
      		   ScanQry.FieldAsDateTime( "time" ) + WAIT_ANSWER_SEC/(60.0*60.0*24.0) < ScanQry.FieldAsDateTime( "now" )
      		 )
    		  ) {
        getFileParams( client_canon_name, ScanQry.FieldAsString( "type" ), file_id, fileparams, true );
      	int len = ScanQry.GetSizeLongField( "data" );
      	if ( p )
      		p = (char*)realloc( p, len );
      	else
      	  p = (char*)malloc( len );
      	if ( !p )
      		throw Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
        ScanQry.FieldAsLong( "data", p );
        xmlNodePtr fileNode = NewTextChild( dataNode, "file" );
        NewTextChild( fileNode, "data", StrUtils::b64_encode( (const char*)p, len ) );
        wait_time = ScanQry.FieldAsDateTime( "now" ) - ScanQry.FieldAsDateTime( "wait_time" );
        NewTextChild( fileNode, "wait_time", wait_time );
        ScanQry.Next();
        if ( !ScanQry.Eof )
        	fileparams[ PARAM_NEXT_FILE ] = "TRUE";
        else
        	fileparams[ PARAM_NEXT_FILE ] = "FALSE";
        buildFileParams( dataNode, fileparams );
//        ProgTrace( TRACE5, "file_id=%d, msg.size()=%d", file_id, len );
        sendFile( file_id );
        break;
      }
    }
    catch(Exception &E)
    {
    	OraSession.Rollback();
      EOracleError *orae=dynamic_cast<EOracleError*>(&E);
      if (orae!=NULL&&
          (orae->Code==4061||orae->Code==4068)) {
         ;
      }
      else {
      	try {
          errorFile( file_id, string("�訡�� ��ࠢ�� ᮮ�饭��: ") + E.what() );
          OraSession.Commit();
        }
        catch( ... ) {
        	try { OraSession.Rollback(); } catch(...){};
        }
        ProgError( STDLOG, "Exception: %s (file_id=%d)", E.what(), file_id );
        if ( p )
          free( p );
        throw;
      }
    };
    if ( ScanQry.FieldAsInteger( "in_order" ) )
      vecType.push_back( in_order_key );
    ScanQry.Next();
  }	 // end while
  if ( p )
    free( p );
  return file_id;
}

void buildLoadFileData( xmlNodePtr resNode, const std::string &client_canon_name )
{ /* ⥯��� ���� ࠧ������� �� �/� */
	JxtContext::JxtCont *sysCont = JxtContext::getJxtContHandler()->sysContext();
	int prior_id = sysCont->readInt( client_canon_name + "_" + OWN_POINT_ADDR() + "_file_param_sets.id", -1 ); // for sort request
	ProgTrace( TRACE5, "get prior_id=%d", prior_id );
	TQuery Qry( &OraSession );
	Qry.SQLText =
	 "SELECT type,airline,param_name,param_value FROM file_param_sets "
	 " WHERE point_addr=:point_addr AND own_point_addr=:own_point_addr AND pr_send=:send "
	 " ORDER BY type,airline ";
	Qry.CreateVariable( "point_addr", otString, client_canon_name );
	Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "send", otInteger, 0 );
	Qry.Execute();
  if ( Qry.Eof )
		return;
	xmlNodePtr dataNode = NewTextChild( resNode, "data" );
	map<string,string> fileparams, first_fileparams;
	string airline, first_airline;
	int new_id = -1, first_new_id;
	int id=0;
	while ( !Qry.Eof ) {
		ProgTrace( TRACE5, "new_id=%d", id );
		if ( fileparams.find( PARAM_FILE_TYPE ) == fileparams.end() ) {
		  fileparams[ PARAM_FILE_TYPE ] = Qry.FieldAsString( "type" );
			airline = Qry.FieldAsString( "airline" );
			new_id = id;
			ProgTrace( TRACE5, "new_id=%d", new_id  );
		}
		if ( fileparams[ PARAM_FILE_TYPE ] != Qry.FieldAsString( "type" ) ||
			   airline != Qry.FieldAsString( "airline" ) ) {
			//next type or airline
			if ( first_fileparams.empty() ) {
				first_fileparams = fileparams;
				first_new_id = new_id;
				first_airline = airline;
				ProgTrace( TRACE5, "airline=%s, first_new_id=%d", airline.c_str(), first_new_id );
			}
			ProgTrace( TRACE5, "prior_id=%d, new_id=%d", prior_id, new_id );
			if ( prior_id >= new_id || fileparams.find( PARAM_LOAD_DIR ) == fileparams.end() ) {
				tst();
				fileparams.clear();
				continue;
		  }
		  else {
		  	tst();
		  	break;
		  }
		}
		else {
	    fileparams[ Qry.FieldAsString( "param_name" ) ] = Qry.FieldAsString( "param_value" );
	    tst();
	  }
	  id++;
		Qry.Next();
	}
	tst();
	ProgTrace( TRACE5, "new_id=%d", new_id );
	if ( Qry.Eof ) // next find from first row
		new_id = -1;
	ProgTrace( TRACE5, "new_id=%d", new_id );

	if ( fileparams.find( PARAM_LOAD_DIR ) == fileparams.end() && !first_fileparams.empty() ) {
		fileparams = first_fileparams;
		airline = first_airline;
		new_id = first_new_id;
	}

  if ( fileparams.find( PARAM_LOAD_DIR ) == fileparams.end() ) {
  	sysCont->write( client_canon_name + "_" + OWN_POINT_ADDR() + "_file_param_sets.id", -1 );
  	if ( Qry.RowCount() )
  	  ProgError( STDLOG, "AstraService Exception: invalid value of table file_params_sets, param LOADDIR not found" );
  	return;
  }
	ProgTrace( TRACE5, "write prior_id=%d", new_id );
	if ( fileparams[ PARAM_FILE_TYPE ] == FILE_AODB_IN_TYPE ) {
		if ( airline.empty() )
			return;
	  string region = CityTZRegion( "���" );
	  TDateTime d = UTCToLocal( NowUTC(), region );
	  string filename = string( "SPP" ) + DateTimeToStr( d, "yymmdd" ) + ".txt";
	  fileparams[ PARAM_FILE_NAME ] = filename;
	  Qry.Clear();
	  Qry.SQLText =
	   "BEGIN "
	   " SELECT rec_no INTO :rec_no FROM aodb_spp_files "
	   "  WHERE filename=:filename AND point_addr=:point_addr AND NVL(airline,'Z')=NVL(:airline,'Z');"
	   " EXCEPTION WHEN NO_DATA_FOUND THEN "
	   " BEGIN "
	   "  :rec_no := -1; "
	   "  INSERT INTO aodb_spp_files(filename,point_addr,airline,rec_no) VALUES(:filename,:point_addr,:airline,:rec_no); "
	   " END;"
	   "END;";
	  Qry.CreateVariable( "filename", otString, filename );
	  Qry.CreateVariable( "point_addr", otString, client_canon_name );
	  Qry.CreateVariable( "airline", otString, airline );
	  Qry.DeclareVariable( "rec_no", otInteger );
	  Qry.Execute();
	  fileparams[ PARAM_FILE_REC_NO ] = Qry.GetVariableAsString( "rec_no" );
	}
	buildFileParams( dataNode, fileparams );
	sysCont->write( client_canon_name + "_" + OWN_POINT_ADDR() + "_file_param_sets.id", new_id );
	sysCont->write( client_canon_name + "_" + OWN_POINT_ADDR() + "_file_param_sets.airline", airline );
}

void AstraServiceInterface::AstraTasksLogon( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  TQuery Qry( &OraSession );
	Qry.SQLText =
	  "SELECT DISTINCT thread_type FROM file_param_sets, file_types, desks "
	  " WHERE OWN_POINT_ADDR=:own_point_addr AND point_addr=:point_addr AND "
	  "       desks.code=file_param_sets.point_addr AND "
    "       file_types.code=file_param_sets.type";
	Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "point_addr", otString, TReqInfo::Instance()->desk.code );
	Qry.Execute();
	if ( Qry.Eof )
		return;
	showBasicInfo();
	GetDevices( reqNode, resNode );
	xmlNodePtr node = NewTextChild( resNode, "tasks" );
	while ( !Qry.Eof ) {
    xmlNodePtr tNode = NewTextChild( node, "task" );
    NewTextChild( tNode, "thread_type", Qry.FieldAsString( "thread_type" ) );
		Qry.Next();
	}
}

void AstraServiceInterface::authorize( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	xmlNodePtr node = GetNode( "canon_name", reqNode );
	if ( !node )
		throw AstraLocale::UserException( "param canon_name not found" );
	string client_canon_name = NodeAsString( node );
	string curmode = NodeAsString( "curmode", reqNode );
	ProgTrace( TRACE5, "client_canon_name=%s, curmode=%s", client_canon_name.c_str(), curmode.c_str() );
	if ( curmode == "OUT" )	{
		double wait_time;
	  int file_id = buildSaveFileData( resNode, client_canon_name, wait_time );
    string event_type;
    TQuery Qry( &OraSession );
    Qry.SQLText = "SELECT value FROM file_params WHERE id=:id AND name=:name";
    Qry.CreateVariable( "id", otInteger, file_id );
    Qry.CreateVariable( "name", otString, NS_PARAM_EVENT_TYPE );
    Qry.Execute();
    if ( !Qry.Eof ) {
      TLogMsg msg;
    	string desk_code;
    	msg.ev_type = DecodeEventType( Qry.FieldAsString( "value" ) );
      Qry.SetVariable( "name", PARAM_CANON_NAME );
      Qry.Execute();
      if ( !Qry.Eof )
  	    desk_code = Qry.FieldAsString( "value" );
      Qry.SetVariable( "name", NS_PARAM_EVENT_ID1 );
      Qry.Execute();
      if ( !Qry.Eof )
  	    msg.id1 = ToInt( Qry.FieldAsString( "value" ) );
      Qry.SetVariable( "name", NS_PARAM_EVENT_ID2 );
      Qry.Execute();
      if ( !Qry.Eof )
  	    msg.id2 = ToInt( Qry.FieldAsString( "value" ) );
      Qry.SetVariable( "name", NS_PARAM_EVENT_ID3 );
      Qry.Execute();
      if ( !Qry.Eof )
  	    msg.id3 = ToInt( Qry.FieldAsString( "value" ) );
  	  Qry.Clear();
  	  Qry.SQLText = "SELECT type FROM file_queue WHERE id=:id";
  	  Qry.CreateVariable( "id", otInteger, file_id );
  	  Qry.Execute();
  	  msg.msg = string("���� ��ࠢ��� (⨯=" + string( Qry.FieldAsString( "type" ) ) + ", �����=" + desk_code + ", ��.=") +
                IntToString( file_id ) + ", ����প�=" + IntToString( (int)(wait_time*60.0*60.0*24.0)) + "ᥪ.)";
  	  TReqInfo::Instance()->MsgToLog( msg );
    }
	}
	else {
		buildLoadFileData( resNode, client_canon_name );
	}
}

void AstraServiceInterface::ThreadTaskReqData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	string client_canon_name = TReqInfo::Instance()->desk.code;
	TQuery Qry( &OraSession );
	Qry.SQLText =
	 "SELECT pr_send, thread_type FROM file_param_sets, file_types, desks "
	 " WHERE own_point_addr=:own_point_addr AND point_addr=:point_addr AND "
	 "       desks.code=file_param_sets.point_addr AND "
   "       file_types.code=file_param_sets.type AND rownum<2";
	Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "point_addr", otString, client_canon_name );
	Qry.Execute();
	if ( Qry.Eof )
		return;
	string thread_type = Qry.FieldAsString( "thread_type" );
  ProgTrace( TRACE5, "client_canon_name=%s, pr_send=%d", client_canon_name.c_str(), Qry.FieldAsInteger( "pr_send" ) );
  NewTextChild( resNode, "thread_type", thread_type );
	if ( Qry.FieldAsInteger( "pr_send" ) ) {
		double wait_time;
	  int file_id = buildSaveFileData( resNode, client_canon_name, wait_time );
    string event_type;
    Qry.Clear();
    Qry.SQLText = "SELECT value FROM file_params WHERE id=:id AND name=:name";
    Qry.CreateVariable( "id", otInteger, file_id );
    Qry.CreateVariable( "name", otString, NS_PARAM_EVENT_TYPE );
    Qry.Execute();
    if ( !Qry.Eof ) {
      TLogMsg msg;
    	string desk_code;
    	msg.ev_type = DecodeEventType( Qry.FieldAsString( "value" ) );
      Qry.SetVariable( "name", PARAM_CANON_NAME );
      Qry.Execute();
      if ( !Qry.Eof )
  	    desk_code = Qry.FieldAsString( "value" );
      Qry.SetVariable( "name", NS_PARAM_EVENT_ID1 );
      Qry.Execute();
      if ( !Qry.Eof )
  	    msg.id1 = ToInt( Qry.FieldAsString( "value" ) );
      Qry.SetVariable( "name", NS_PARAM_EVENT_ID2 );
      Qry.Execute();
      if ( !Qry.Eof )
  	    msg.id2 = ToInt( Qry.FieldAsString( "value" ) );
      Qry.SetVariable( "name", NS_PARAM_EVENT_ID3 );
      Qry.Execute();
      if ( !Qry.Eof )
  	    msg.id3 = ToInt( Qry.FieldAsString( "value" ) );
  	  Qry.Clear();
  	  Qry.SQLText = "SELECT type FROM file_queue WHERE id=:id";
  	  Qry.CreateVariable( "id", otInteger, file_id );
  	  Qry.Execute();
  	  msg.msg = string("���� ��ࠢ��� (⨯=" + string( Qry.FieldAsString( "type" ) ) + ", �����=" + desk_code + ", ��.=") +
                IntToString( file_id ) + ", ����প�=" + IntToString( (int)(wait_time*60.0*60.0*24.0)) + "ᥪ.)";
  	  TReqInfo::Instance()->MsgToLog( msg );
    }
	}
	else {
		if ( thread_type == "ewAODBLoad" )
		  buildLoadFileData( resNode, client_canon_name );
	}
}

void AstraServiceInterface::ThreadTaskResData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	string thread_type = NodeAsString( "thread_type", reqNode );
	string client_canon_name = TReqInfo::Instance()->desk.code;
	NewTextChild( resNode, "thread_type", thread_type );
	TQuery Qry( &OraSession );
	Qry.SQLText =
	 "SELECT pr_send FROM file_param_sets, file_types "
	 " WHERE own_point_addr=:own_point_addr AND point_addr=:point_addr AND "
	 "       file_types.code=file_param_sets.type AND thread_type=:thread_type AND rownum<2";
	Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "point_addr", otString, client_canon_name );
	Qry.CreateVariable( "thread_type", otString, thread_type );
	Qry.Execute();
	if ( Qry.Eof )
		return;
	map<string,string> params;
	xmlNodePtr n = NodeAsNode( "headers", reqNode );
	if ( !n || !n->children ) {
		AstraLocale::showProgError( "MSG.MSG_PARAMS_NOT_DEFINED.CALL_ADMIN" );
		return;
	}
  n = n->children;
  while ( n ) {
  	params[ NodeAsString( "key", n ) ] = NodeAsString( "value", n );
  	ProgTrace( TRACE5, "key=%s, value=%s",NodeAsString( "key", n ), NodeAsString( "value", n ) );
  	n = n->next;
  }
	if ( Qry.FieldAsInteger( "pr_send" ) ) {
		tst();
		string sfile_id = params[ PARAM_FILE_ID ];
		int file_id;
		if ( sfile_id.empty() || params[ "ANSWER" ].empty() || StrToInt( sfile_id.c_str(), file_id ) == EOF ) {
			AstraLocale::showProgError( "MSG.MSG_PARAMS_NOT_DEFINED.CALL_ADMIN" );
			return;
		}
		if ( params[ "ANSWER" ] == "COMMIT" )
		  CommitWork( file_id );
  }
}

void CommitWork( int file_id )
{
  ProgTrace( TRACE5, "commitFileData param file_id=%d", file_id );
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT value FROM file_params WHERE id=:id AND name=:name";
  Qry.CreateVariable( "id", otInteger, file_id );
  Qry.CreateVariable( "name", otString, NS_PARAM_EVENT_TYPE );
  Qry.Execute();
  TLogMsg msg;
	string desk_code;
  if ( !Qry.Eof )
 	  msg.ev_type = DecodeEventType( Qry.FieldAsString( "value" ) );
  Qry.SetVariable( "name", PARAM_CANON_NAME );
  Qry.Execute();
  if ( !Qry.Eof )
  	desk_code = Qry.FieldAsString( "value" );
  Qry.SetVariable( "name", NS_PARAM_EVENT_ID1 );
  Qry.Execute();
  if ( !Qry.Eof )
  	msg.id1 = ToInt( Qry.FieldAsString( "value" ) );
  Qry.SetVariable( "name", NS_PARAM_EVENT_ID2 );
  Qry.Execute();
  if ( !Qry.Eof )
  	msg.id2 = ToInt( Qry.FieldAsString( "value" ) );
  Qry.SetVariable( "name", NS_PARAM_EVENT_ID3 );
  Qry.Execute();
  if ( !Qry.Eof )
  	msg.id3 = ToInt( Qry.FieldAsString( "value" ) );
  Qry.Clear();
  Qry.SQLText = "SELECT type FROM file_queue WHERE id=:id";
  Qry.CreateVariable( "id", otInteger, file_id );
  Qry.Execute();
  string ftype = Qry.FieldAsString( "type" );
  doneFile( file_id );
  msg.msg = string("���� ���⠢��� (⨯=" + ftype + ", �����=" + desk_code + ", ��.=") + IntToString( file_id ) + ")";
  TReqInfo::Instance()->MsgToLog( msg );
  if ( ftype == "BSM" )
    monitor_idle_zapr_type(1, QUEPOT_TLG_OTH);
}

void AstraServiceInterface::commitFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  CommitWork( NodeAsInteger( "file_id", reqNode ) );
}

void AstraServiceInterface::errorFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	bool pr_error;
	xmlNodePtr n = GetNode( "error", reqNode );
	pr_error = n;
	if ( !pr_error )
		n = GetNode( "mes", reqNode );
	string msg = NodeAsString( n );
	string file_id = NodeAsString( (char*)PARAM_FILE_ID.c_str(), reqNode );
	if ( pr_error )
    ProgError( STDLOG, "AstraService Exception: %s, file_id=%s", msg.c_str(), file_id.c_str() );
  else
  	ProgTrace( TRACE5, "AstraService Message: %s, file_id=%s", msg.c_str(), file_id.c_str() );
  int id;
  if ( StrToInt( file_id.c_str(), id ) != EOF && id > 0 ) {
  	errorFile( id, msg );
  }
}

void AstraServiceInterface::createFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
/*	int point_id = NodeAsInteger( "point_id", reqNode );
	string client_canon_name = NodeAsString( "canon_name", reqNode );
	ProgTrace( TRACE5, "createFileData point_id=%d, client_canon_name=%s", point_id, client_canon_name.c_str() );
	map<string,string> params;
	params[ PARAM_WORK_DIR ] = "C:\\Temp";
	string data;
	createCentringFile( point_id, params, data );*/
}

string getFileEncoding( const string &file_type, const string &point_addr, bool pr_send )
{
	string res;
  TQuery EncodeQry( &OraSession );
  EncodeQry.SQLText =
      "select encoding from file_encoding where "
      "   own_point_addr = :own_point_addr and "
      "   type = :type and "
      "   point_addr=:point_addr AND "
      "   pr_send = :pr_send";
  EncodeQry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
  EncodeQry.CreateVariable( "type", otString, file_type );
  EncodeQry.CreateVariable( "point_addr", otString, point_addr );
  EncodeQry.CreateVariable( "pr_send", otInteger, pr_send );
  EncodeQry.Execute();
  if ( !EncodeQry.Eof )
  	res = EncodeQry.FieldAsString( "encoding" );
  return res;
}

bool isXMLFormat( const std::string type )
{
  return ( type == FILE_CHECKINDATA_TYPE );
}

bool CreateCommonFileData( const std::string &point_addr,
                           int id, const std::string type,
                           const std::string &airp, const std::string &airline,
	                         const std::string &flt_no )
{
	bool res = false;
    TQuery Qry( &OraSession );
    Qry.SQLText =
        "SELECT point_addr, param_name, param_value,"
        " airp, airline, flt_no, "
        " DECODE( airp, NULL, 0, 4 ) + "
        " DECODE( airline, NULL, 0, 2 ) + "
        " DECODE( flt_no, NULL, 0, 1 ) AS priority "
        " FROM file_param_sets, desks "
        " WHERE own_point_addr=:own_point_addr AND "
        "       point_addr=:point_addr AND "
        "       type=:type AND "
        "       pr_send = 1 AND "
        "       desks.code=file_param_sets.point_addr AND "
        "       ( airp IS NULL OR airp=:airp ) AND "
        "       ( airline IS NULL OR airline=:airline ) AND "
        "       ( flt_no IS NULL OR flt_no=:flt_no ) "
        " ORDER BY point_addr,priority DESC";
    Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
    Qry.CreateVariable( "type", otString, type );
    Qry.CreateVariable( "point_addr", otString, point_addr );
    if ( airp.empty() )
        Qry.CreateVariable( "airp", otString, FNull );
    else
        Qry.CreateVariable( "airp", otString, airp );
    if ( airline.empty() )
        Qry.CreateVariable( "airline", otString, FNull );
    else
        Qry.CreateVariable( "airline", otString, airline );
    if ( flt_no.empty() )
        Qry.CreateVariable( "flt_no", otInteger, FNull );
    else
        Qry.CreateVariable( "flt_no", otInteger, flt_no );
    Qry.Execute();
//    map<string,int> cname; /* canon_name, priority */
    map<string,string> inparams;
    TFileDatas fds;
//    int priority;
    string client_canon_name;
    bool master_params = false;
    while ( 1 ) {
        if ( client_canon_name.empty() && !Qry.Eof )
          client_canon_name = Qry.FieldAsString( "point_addr" );
        if ( Qry.Eof && !client_canon_name.empty() ||
        	   !Qry.Eof && client_canon_name != Qry.FieldAsString( "point_addr" ) ) { /* �᫨ ��� ⠪��� ����� */
          if ( master_params ) {
            fds.clear();
            TFileData fd;
            try {
                if (
                        type == FILE_CENT_TYPE && createCentringFile( id, client_canon_name, fds ) ||
                        type == FILE_SOFI_TYPE && createSofiFile( id, inparams, client_canon_name, fds ) ||
                        type == FILE_AODB_OUT_TYPE && createAODBFiles( id, client_canon_name, fds ) ||
                        type == FILE_SPPCEK_TYPE && createSPPCEKFile( id, client_canon_name, fds ) ||
                        type == FILE_1CCEK_TYPE && Sync1C( client_canon_name, fds ) ||
                        type == FILE_CHECKINDATA_TYPE && createCheckinDataFiles( id, client_canon_name, fds ) ) {
                    /* ⥯��� � params �� ����� � ��� 䠩�� */
                    string encoding = getFileEncoding( type, client_canon_name, true );
                    for ( vector<TFileData>::iterator i=fds.begin(); i!=fds.end(); i++ ) {
                    	i->params[PARAM_CANON_NAME] = client_canon_name;
                      i->params[ NS_PARAM_AIRP ] = airp;
                      i->params[ NS_PARAM_AIRLINE ] = airline;
                      i->params[ NS_PARAM_FLT_NO ] = flt_no;
                    	string str_file = i->file_data;
                      if ( !encoding.empty() )
                          try {
                              str_file = ConvertCodepage( str_file, "CP866", encoding );
                              if ( isXMLFormat( type ) ) { //XML UTF-8 convert to encoding
                                str_file.replace( str_file.find( "encoding=\"UTF-8\""), string( "encoding=\"UTF-8\"" ).size(), string("encoding=\"") + encoding + "\"" );
                              }
                          } catch(EConvertError &E) {
                              ProgError(STDLOG, E.what());
                              throw AstraLocale::UserException("MSG.CONVERT_INTO_ERR", LParams() << LParam("enc", encoding));
                          }
                      res = true;
                      int file_id = putFile( client_canon_name, OWN_POINT_ADDR(), type, i->params, str_file );
                      ProgTrace( TRACE5, "file create file_id=%d, type=%s", file_id, type.c_str() );
                      TLogMsg msg;
                      if ( i->params.find( NS_PARAM_EVENT_TYPE ) != i->params.end() ) {
                    	  msg.ev_type = DecodeEventType( i->params[ NS_PARAM_EVENT_TYPE ] );
                    	  if ( i->params.find( NS_PARAM_EVENT_ID1 ) != i->params.end() )
                    		  msg.id1 = ToInt( i->params[ NS_PARAM_EVENT_ID1 ] );
                    	  if ( i->params.find( NS_PARAM_EVENT_ID2 ) != i->params.end() )
                    		  msg.id2 = ToInt( i->params[ NS_PARAM_EVENT_ID2 ] );
                    	  if ( i->params.find( NS_PARAM_EVENT_ID3 ) != i->params.end() )
                    		  msg.id3 = ToInt( i->params[ NS_PARAM_EVENT_ID3 ] );
                    		msg.msg = string("���� ᮧ��� (⨯=" + type + ", �����=" + client_canon_name + ", ��.=") + IntToString( file_id ) + ")";
                    		TReqInfo::Instance()->MsgToLog( msg );
                      }
                    }
                }
            }
            /* �� �� ����稫��� ��ନ஢��� 䠩�, ��⠫�� 䠩�� ����� ⮦� �ࠢ� ���஡����� ��ନ஢����� */
            catch(EOracleError &E)
            {
              ProgError( STDLOG, "EOracleError file_type=%s, %d: %s", type.c_str(), E.Code, E.what());
              ProgError( STDLOG, "SQL: %s", E.SQLText());
            }
            catch( std::exception &e) {
                ProgError(STDLOG, "exception file_type=%s, id=%d, what=%s", type.c_str(), id, e.what());
            }
            catch(...) {
                ProgError(STDLOG, "putFile: Unknown error while trying to put file");
            };
            inparams.clear();
            master_params = false;
          }
         	if ( !Qry.Eof )
            client_canon_name = Qry.FieldAsString( "point_addr" );
        }
        if ( Qry.Eof )
        	break;
        map<string,string>::iterator im=inparams.end();
        for ( im=inparams.begin(); im!=inparams.end(); im++ ) {
        	if ( im->first == Qry.FieldAsString( "param_name" ) )
        		break;
        }
        if ( im == inparams.end() ) {
        	inparams.insert( make_pair( Qry.FieldAsString( "param_name" ), Qry.FieldAsString( "param_value" ) ) );
        	if ( string( Qry.FieldAsString( "param_name" ) ).find( "DEP" ) == std::string::npos ) {
        	 	master_params = true;
        	}
        }
        Qry.Next();
    }
  return res;
}

void CreateCentringFileDATA( int point_id )
{
/*	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT airp, airline, flt_no FROM points WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	if ( !Qry.Eof )
		CreateCommonFileData( point_id, FILE_CENT_TYPE, Qry.FieldAsString( "airp" ),
		                      Qry.FieldAsString( "airline" ), Qry.FieldAsString( "flt_no" ) );*/
	//client_canon_name = "CENTST";
	//createCentringFile( point_id, OWN_POINT_ADDR(), string( "ASWFMG" ) );
	//createCentringFile( point_id, OWN_POINT_ADDR(), string( "GABFMG" ) );
	//createCentringFile( point_id, OWN_POINT_ADDR(), string( "UT_FMG" ) );
	//createCentringFile( point_id, OWN_POINT_ADDR(), string( "TJMFMG" ) );
	//createCentringFile( point_id, OWN_POINT_ADDR(), string( "SGCFMG" ) );
}

void createSofiFileDATA( int receipt_id )
{
	TQuery Qry( &OraSession );
	Qry.SQLText =
    "SELECT airp_dep as airp, airline, flt_no "
    " FROM bag_receipts "
    "WHERE receipt_id=:receipt_id";
  Qry.CreateVariable( "receipt_id", otInteger, receipt_id );
	Qry.Execute();
  string airline, airp;
	int flt_no;
	if ( !Qry.Eof ) {
    airline = Qry.FieldAsString( "airline" );
    airp = Qry.FieldAsString( "airp" );
    if ( Qry.FieldIsNULL( "flt_no" ) )
      flt_no = ASTRA::NoExists;
    else
      flt_no = Qry.FieldAsInteger( "flt_no" );
    Qry.Clear();
    Qry.SQLText =
      "SELECT DISTINCT point_addr FROM file_param_sets, desks "
      " WHERE type=:type AND own_point_addr=:own_point_addr AND pr_send=1 AND "
      "       desks.code=file_param_sets.point_addr AND "
      "       ( airp IS NULL OR airp=:airp ) AND "
		  "       ( airline IS NULL OR airline=:airline ) AND "
		  "       ( flt_no IS NULL OR flt_no=:flt_no ) ";
    Qry.CreateVariable( "type", otString, FILE_SOFI_TYPE );
    Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
    Qry.CreateVariable( "airp", otString, airp );
    Qry.CreateVariable( "airline", otString, airline );
    if ( flt_no == ASTRA::NoExists )
      Qry.CreateVariable( "flt_no", otInteger, FNull );
    else
      Qry.CreateVariable( "flt_no", otInteger, flt_no );
    Qry.Execute();
    string strflt_no;
    if (  flt_no != ASTRA::NoExists )
      strflt_no = IntToString( flt_no );
    while ( !Qry.Eof ) {
  		CreateCommonFileData( Qry.FieldAsString("point_addr"),
                            receipt_id, FILE_SOFI_TYPE, airp,
	  	                      airline, strflt_no );
      Qry.Next();
    }
	}
}

/*void createAODBFileDATA( int point_id )
{
	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT airp, airline, flt_no FROM points WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	if ( !Qry.Eof )
		CreateCommonFileData( point_id, FILE_AODB_TYPE, Qry.FieldAsString( "airp" ),
		                      Qry.FieldAsString( "airline" ), Qry.FieldAsString( "flt_no" ) );
}*/


class TAODBPointAddr: public TPointAddr {
  TQuery *StagesQry;
public:
  TAODBPointAddr( ):TPointAddr( string(FILE_AODB_OUT_TYPE), true ) {
    StagesQry = new TQuery(&OraSession);
  	StagesQry->SQLText =
      "SELECT stage_id FROM trip_final_stages WHERE point_id=:point_id AND stage_type=:ckin_stage_type";
    StagesQry->CreateVariable( "ckin_stage_type", otInteger, stCheckIn );
    StagesQry->DeclareVariable( "point_id", otInteger );
  }
  ~TAODBPointAddr( ) {
    delete StagesQry;
  }
  virtual bool validateParams( const string &point_addr, const vector<string> &ailines,
                               const vector<string> &airps, const vector<int> &flt_nos ) {
    return ( airps.size() == 1 ); // �롨ࠥ� ३��. �� ����室���� �᫮���
  }
  virtual bool validatePoints( int point_id ) {
    StagesQry->SetVariable( "point_id", point_id );
    StagesQry->Execute();
    return ( !StagesQry->Eof &&
              StagesQry->FieldAsInteger( "stage_id" ) != sNoActive &&
              StagesQry->FieldAsInteger( "stage_id" ) != sPrepCheckIn );
  }
};

void sync_aodb( void )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT TRUNC(system.UTCSYSDATE) currdate FROM dual";
  Qry.Execute();
  TDateTime currdate = Qry.FieldAsDateTime( "currdate" );
  
  TAODBPointAddr point_addr;
  TSQLCondDates cond_dates;
  cond_dates.sql = " time_out in (:day1,:day2) AND act_out IS NULL AND pr_del=0 ";
  cond_dates.dates.insert( make_pair( "day1", currdate ) );
  cond_dates.dates.insert( make_pair( "day2", currdate + 1 ) );
  point_addr.createPointSQL( cond_dates );
/*	Qry.Clear();
	Qry.SQLText =
    "SELECT record, rec_no from drop_spp1 WHERE airline='��' AND filename=:filename "
    "ORDER BY rec_no";
  Qry.DeclareVariable( "filename", otString );
  TDateTime d = NowUTC() - 30;
  string filename;
  filename=string("SPP")+string(DateTimeToStr( d, "yymmdd" )) + ".txt";
  Qry.SetVariable( "filename", filename );
  Qry.Execute();
  while ( Qry.Eof && d <= NowUTC() ) {
     d++;
     filename=string(DateTimeToStr( d, "yymmdd" )) + ".txt";
     Qry.SetVariable( "filename", filename );
     Qry.Execute();
  };
  if ( d > NowUTC() ) {
    ProgTrace( TRACE5, "SPP: all record parsed" );
    return;
  }
  int count_line = 15;
  string data;
  vector<int> recs;
  while ( !Qry.Eof && count_line >=0 ) {
    count_line--;
    ProgTrace( TRACE5, "SPP: record=%s", Qry.FieldAsString( "record" ) );
    data += Qry.FieldAsString( "record" );
    recs.push_back( Qry.FieldAsInteger("rec_no") );
    data += 13;
    data += 10;
    Qry.Next();
  }
  ProgTrace( TRACE5, "SPP: parse data=%s", data.c_str() );
  Qry.Clear();
  Qry.SQLText =
	 "BEGIN "
	 " SELECT rec_no INTO :rec_no FROM aodb_spp_files "
	 "  WHERE filename=:filename AND point_addr=:point_addr AND NVL(airline,'Z')=NVL(:airline,'Z');"
	 " EXCEPTION WHEN NO_DATA_FOUND THEN "
	 " BEGIN "
	 "  :rec_no := -1; "
	 "  INSERT INTO aodb_spp_files(filename,point_addr,airline,rec_no) VALUES(:filename,:point_addr,:airline,:rec_no); "
	 " END;"
	 "END;";
  Qry.CreateVariable("filename", otString, filename);
  Qry.CreateVariable("rec_no", otInteger, -1);
  Qry.CreateVariable("point_addr", otString,"RASTRV");
  Qry.CreateVariable("airline", otString,"��");
  Qry.Execute();
  tst();
  Qry.Clear();
  Qry.SQLText =
    "DELETE drop_spp1 WHERE airline='��' AND filename=:filename and rec_no=:rec_no";
  Qry.CreateVariable( "filename", otString, filename );
  Qry.DeclareVariable( "rec_no", otInteger );
  for ( vector<int>::iterator i=recs.begin(); i!=recs.end(); i++ ) {
    Qry.SetVariable( "rec_no", *i );
    Qry.Execute();
    ProgTrace( TRACE5, "SPP: rec_no=%d", *i );
  }
  map<string,string> fileparams;
  fileparams[ NS_PARAM_AIRLINE ] = "��";
  fileparams[ PARAM_FILE_NAME ] = filename;
  fileparams[ PARAM_FILE_TYPE ] = FILE_AODB_IN_TYPE;
  fileparams[ PARAM_CANON_NAME ] = "RASTRV";
  
  putFile( OWN_POINT_ADDR(),
           OWN_POINT_ADDR(),
           FILE_AODB_IN_TYPE,
           fileparams,
           data );
  sendCmd( "CMD_PARSE_AODB", "P" );   */
}

class TSPPCEKPointAddr: public TPointAddr {
public:
  TSPPCEKPointAddr( ):TPointAddr( FILE_SPPCEK_TYPE, true ) {}
  ~TSPPCEKPointAddr( ) {}
  virtual bool validateParams( const string &point_addr, const vector<string> &ailines,
                               const vector<string> &airps, const vector<int> &flt_nos ) {
    return ( airps.size() == 1 ); // �롨ࠥ� ३��. �� ����室���� �᫮���
  }
};

void sync_sppcek( void )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT system.UTCSYSDATE currdate FROM dual";
  Qry.Execute();
  TDateTime currdate = Qry.FieldAsDateTime( "currdate" );
  TSPPCEKPointAddr point_addr;
  TSQLCondDates cond_dates;
  cond_dates.sql = " ( scd_in >= :day1 OR scd_out >= :day1 OR "
                   "   est_in >= :day1 OR est_out >= :day1 OR "
                   "   act_in >= :day1 OR act_out >= :day1 ) AND "
                   " ( time_out < :max_spp_day OR time_in < :max_spp_day ) ";
  cond_dates.dates.insert( make_pair( "day1", currdate - 1 ) );
  TDateTime f;
  modf( currdate, &f );
  cond_dates.dates.insert( make_pair( "max_spp_day", f + CREATE_SPP_DAYS() + 1 ) );
  point_addr.createPointSQL( cond_dates );
}

void sync_1ccek( void )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT DISTINCT point_addr FROM file_param_sets, desks "
    " WHERE type=:type AND pr_send=1 AND "
    "       desks.code=file_param_sets.point_addr";
  Qry.CreateVariable( "type", otString, FILE_1CCEK_TYPE );
  Qry.Execute();
  while ( !Qry.Eof ) {
	  CreateCommonFileData( Qry.FieldAsString( "point_addr" ), -1, FILE_1CCEK_TYPE, "���", "", "" );
	  Qry.Next();
  }
}

void sendCmdParseAODB()
{
  sendCmd( "CMD_PARSE_AODB", "P" );
}

void AstraServiceInterface::saveFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	map<string,string> fileparams;
  xmlNodePtr dataNode = GetNode( "data", reqNode );
	parseFileParams( dataNode, fileparams );
	dataNode = GetNode( "file", dataNode );
	if ( !dataNode ) {
		ProgError( STDLOG, "saveFileData tag file not found!" );
		return;
	}
	dataNode = GetNode( "data", dataNode );
	if ( !dataNode ) {
		ProgError( STDLOG, "saveFileData tag file\\data not found!" );
		return;
	}
	string file_data = NodeAsString( dataNode );
	file_data = StrUtils::b64_decode( file_data.c_str(), file_data.size() );
	fileparams[ PARAM_CANON_NAME ] = fileparams[ "canon_name" ];
	fileparams.erase( "canon_name" );
	fileparams[ NS_PARAM_AIRLINE ] = JxtContext::getJxtContHandler()->sysContext()->read( fileparams[ PARAM_CANON_NAME ] + "_" + OWN_POINT_ADDR() + "_file_param_sets.airline" );
	
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airp FROM file_param_sets, desks "
    " WHERE type=:type AND pr_send=0 AND "
    "       point_addr=:point_addr AND "
    "       airline=:airline AND "
    "       airp IS NOT NULL AND "
    "       desks.code=file_param_sets.point_addr";
  Qry.CreateVariable( "type", otString, FILE_AODB_IN_TYPE );
  Qry.CreateVariable( "point_addr", otString, fileparams[ PARAM_CANON_NAME ] );
  Qry.CreateVariable( "airline", otString, fileparams[ NS_PARAM_AIRLINE ] );
  Qry.Execute();
  if ( Qry.Eof ) {
    ProgError( STDLOG, "straServiceInterface::saveFileData: invalid sync AODB SPP, point_addr=%s, airline=%s",
               fileparams[ PARAM_CANON_NAME ].c_str(), fileparams[ NS_PARAM_AIRLINE ].c_str() );
    return;
  }
  fileparams[ NS_PARAM_AIRP ] = Qry.FieldAsString( "airp" );
  ProgTrace( TRACE5, "fileparams[NS_PARAM_AIRLINE=%s]=%s, airp=%s",
             NS_PARAM_AIRLINE.c_str(), fileparams[ NS_PARAM_AIRLINE ].c_str(),
             fileparams[ NS_PARAM_AIRP ].c_str() );
	putFile( OWN_POINT_ADDR(),
           OWN_POINT_ADDR(),
           FILE_AODB_IN_TYPE,
           fileparams,
           file_data );
  registerHookAfter(sendCmdParseAODB);
}

class TCheckinDataPointAddr: public TPointAddr {
  TQuery *StagesQry;
public:
  TCheckinDataPointAddr( ):TPointAddr( string(FILE_CHECKINDATA_TYPE), true ) {
    StagesQry = new TQuery(&OraSession);
  	StagesQry->SQLText =
      "SELECT stage_id FROM trip_final_stages WHERE point_id=:point_id AND stage_type=:ckin_stage_type";
    StagesQry->CreateVariable( "ckin_stage_type", otInteger, stCheckIn );
    StagesQry->DeclareVariable( "point_id", otInteger );
  }
  ~TCheckinDataPointAddr( ) {
    delete StagesQry;
  }
  virtual bool validateParams( const string &point_addr, const vector<string> &ailines,
                               const vector<string> &airps, const vector<int> &flt_nos ) {
    return ( airps.size() == 1 ); // �롨ࠥ� ३��. �� ����室���� �᫮���
  }
  virtual bool validatePoints( int point_id ) {
    StagesQry->SetVariable( "point_id", point_id );
    StagesQry->Execute();
    return ( !StagesQry->Eof &&
              StagesQry->FieldAsInteger( "stage_id" ) != sNoActive &&
              StagesQry->FieldAsInteger( "stage_id" ) != sPrepCheckIn );
  }
};

struct TCheckInData {
  int f;
  int c;
  int y;
  TCheckInData() {
    f = 0;
    c = 0;
    y = 0;
  }
};

struct TPassTypeData {
  int male;
  int female;
  int chd;
  int inf;
  TPassTypeData() {
    male = 0;
    female = 0;
    chd = 0;
    inf = 0;
  }
};

struct TResaData {
  string code;
  int resa;
  int tranzit;
  TResaData() {
    resa = 0;
    tranzit = 0;
  }
};

struct TBagData {
  int bag_amount;
  int bag_weight;
  int rk_amount;
  int rk_weight;
  TBagData() {
    bag_amount = 0;
    bag_weight = 0;
    rk_amount = 0;
    rk_weight = 0;
  }
};

void sync_checkin_data( void )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT TRUNC(system.UTCSYSDATE) currdate FROM dual";
  Qry.Execute();
  TDateTime currdate = Qry.FieldAsDateTime( "currdate" );

  TCheckinDataPointAddr point_addr;
  TSQLCondDates cond_dates;
  cond_dates.sql = " time_out in (:day1,:day2) AND act_out IS NULL ";
  cond_dates.dates.insert( make_pair( "day1", currdate ) );
  cond_dates.dates.insert( make_pair( "day2", currdate + 1 ) );
  point_addr.createPointSQL( cond_dates );
};

bool createCheckinDataFiles( int point_id, const std::string &point_addr, TFileDatas &fds )
{
  fds.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airline, flt_no, suffix, suffix_fmt, airp, scd_out, est_out, act_out, pr_del FROM points "
    " WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  TQuery StageQry( &OraSession );
  StageQry.SQLText =
    "SELECT stage_id FROM trip_final_stages WHERE point_id=:point_id AND stage_type=:stage_type";
  StageQry.CreateVariable( "point_id", otInteger, point_id );
  StageQry.CreateVariable( "stage_type", otInteger, stCheckIn );
  Qry.Execute();
  if ( Qry.Eof )
    return false;
  TQuery PaxQry( &OraSession );
  //������
  PaxQry.SQLText =
    "SELECT point_arv, "
    "       NVL(SUM(DECODE(pax_grp.class,:f,1,0)),0) AS f, "
    "       NVL(SUM(DECODE(pax_grp.class,:c,1,0)),0) AS c, "
    "       NVL(SUM(DECODE(pax_grp.class,:y,1,0)),0) AS y "
    "  FROM pax_grp, pax "
    " WHERE pax_grp.grp_id=pax.grp_id AND "
    "       point_dep=:point_id AND pr_brd IS NOT NULL "
    "GROUP BY point_arv";
  PaxQry.CreateVariable( "point_id", otInteger, point_id );
  PaxQry.CreateVariable( "f", otString, "�" );
  PaxQry.CreateVariable( "c", otString, "�" );
  PaxQry.CreateVariable( "y", otString, "�" );
  //���� ���ᠦ�஢
  TQuery PassQry( &OraSession );
  PassQry.SQLText =
    "SELECT point_arv, "
    "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:male,1,NULL,1,0),0)),0) AS male, "
    "       NVL(SUM(DECODE(pax.pers_type,:adl,DECODE(pax_doc.gender,:female,1,0),0)),0) AS female, "
    "       NVL(SUM(DECODE(pax.pers_type,:chd,1,0)),0) AS chd, "
    "       NVL(SUM(DECODE(pax.pers_type,:inf,1,0)),0) AS inf "
    " FROM pax_grp, pax, pax_doc "
    " WHERE pax_grp.grp_id=pax.grp_id AND "
    "       pax.pax_id=pax_doc.pax_id(+) AND "
    "       point_dep=:point_id AND pr_brd IS NOT NULL "
    "GROUP BY point_arv";
  PassQry.CreateVariable( "point_id", otInteger, point_id );
  PassQry.CreateVariable( "adl", otString, "��" );
  PassQry.CreateVariable( "male", otString, "M" );
  PassQry.CreateVariable( "female", otString, "F" );
  PassQry.CreateVariable( "chd", otString, "��" );
  PassQry.CreateVariable( "inf", otString, "��" );
  TQuery ResaQry( &OraSession );
  ResaQry.SQLText =
/*    "SELECT COUNT(*) resa, airp_arv, class FROM tlg_binding,crs_pnr,crs_pax "
    " WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "       crs_pax.pr_del=0 AND "
    "       tlg_binding.point_id_spp=:point_id AND "
    "       tlg_binding.point_id_tlg=crs_pnr.point_id AND "
    "       crs_pnr.system='CRS' "
    "GROUP BY airp_arv, class";*/
    "SELECT COUNT(*) resa, target airp_arv, class FROM tlg_binding,crs_pnr,crs_pax "
    " WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "       crs_pax.pr_del=0 AND "
    "       tlg_binding.point_id_spp=:point_id AND "
    "       tlg_binding.point_id_tlg=crs_pnr.point_id "
    "GROUP BY target, class";
  ResaQry.CreateVariable( "point_id", otInteger, point_id );
  TQuery BagQry( &OraSession );
  BagQry.SQLText =
    "SELECT pax_grp.point_arv, "
    "       SUM(DECODE(bag2.pr_cabin, 0, amount, 0)) bag_amount, "
    "       SUM(DECODE(bag2.pr_cabin, 0, weight, 0)) bag_weight, "
    "       SUM(DECODE(bag2.pr_cabin, 0, 0, amount)) rk_amount, "
    "       SUM(DECODE(bag2.pr_cabin, 0, 0, weight)) rk_weight "
    "FROM pax_grp, bag2 "
    " WHERE pax_grp.point_dep = :point_id AND "
    "       pax_grp.grp_id = bag2.grp_id AND "
    "       pax_grp.bag_refuse = 0 "
    " GROUP BY pax_grp.point_arv ";
  BagQry.CreateVariable( "point_id", otInteger, point_id );
  tst();
  string airline = Qry.FieldAsString( "airline" );
  int flt_no = Qry.FieldAsInteger( "flt_no" );
  string suffix = ElemIdToElemCtxt( ecDisp, etSuffix, Qry.FieldAsString( "suffix" ), (TElemFmt)Qry.FieldAsInteger( "suffix_fmt" ) );

  string airp = Qry.FieldAsString( "airp" );
  TDateTime scd_out = Qry.FieldAsDateTime( "scd_out" );
  string prior_record, record;
  get_string_into_snapshot_points( point_id, FILE_CHECKINDATA_TYPE, point_addr, prior_record );
  xmlDocPtr doc = CreateXMLDoc( "UTF-8", "flight" );
  tst();
  try {
    xmlNodePtr node = doc->children;
    xmlNodePtr n = NewTextChild( node, "airline" );
    SetProp( n, "code_zrt", airline );
    SetProp( n, "code_iata", ((TAirlinesRow&)base_tables.get("airlines").get_row( "code", airline, true )).code_lat );
    NewTextChild( node, "flt_no", flt_no );
    if ( !suffix.empty() )
      NewTextChild( node, "suffix", suffix );
    NewTextChild( node, "date_scd", DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ) );
    if ( Qry.FieldAsInteger( "pr_del" ) == -1 )
      NewTextChild( node, "status", "delete" );
    else
      if ( Qry.FieldAsInteger( "pr_del" ) == 1 )
        NewTextChild( node, "status", "cancel" );
      else
        if ( !Qry.FieldIsNULL( "act_out" ) )
          NewTextChild( node ,"status", "close" );
        else {
          tst();
          StageQry.Execute();
          if ( StageQry.FieldAsInteger( "stage_id" ) == sOpenCheckIn )
            NewTextChild( node ,"status", "checkin" );
          else
            NewTextChild( node ,"status", "close" );
        }
    map<string,vector<TResaData> > resaData;
    map<int,TCheckInData> checkinData;
    map<int,TPassTypeData> passtypeData;
    map<int,TBagData> bagData;
    tst();
    ResaQry.Execute();
    tst();
    while ( !ResaQry.Eof ) {
      TResaData resa;
      resa.code = ResaQry.FieldAsString( "class" );
      resa.resa = ResaQry.FieldAsInteger( "resa" );
      if ( resa.resa > 0 )
        resaData[ ResaQry.FieldAsString( "airp_arv" ) ].push_back( resa );
      tst();
      ResaQry.Next();
      tst();
    }
    tst();
    PaxQry.Execute();
    tst();
    while ( !PaxQry.Eof ) {
      if ( PaxQry.FieldAsInteger( "f" ) +
           PaxQry.FieldAsInteger( "c" ) +
           PaxQry.FieldAsInteger( "y" )!= 0 ) {
        checkinData[ PaxQry.FieldAsInteger( "point_arv" ) ].f = PaxQry.FieldAsInteger( "f" );
        checkinData[ PaxQry.FieldAsInteger( "point_arv" ) ].c = PaxQry.FieldAsInteger( "c" );
        checkinData[ PaxQry.FieldAsInteger( "point_arv" ) ].y = PaxQry.FieldAsInteger( "y" );
        ProgTrace( TRACE5, "f=%d, c=%d, y=%d",
                   checkinData[ PaxQry.FieldAsInteger( "point_arv" ) ].f,
                   checkinData[ PaxQry.FieldAsInteger( "point_arv" ) ].c,
                   checkinData[ PaxQry.FieldAsInteger( "point_arv" ) ].y );
      }
      PaxQry.Next();
    }
    tst();
    PassQry.Execute();
    tst();
    while ( !PassQry.Eof ) {
      if ( PassQry.FieldAsInteger( "male" ) +
           PassQry.FieldAsInteger( "female" ) +
           PassQry.FieldAsInteger( "chd" ) +
           PassQry.FieldAsInteger( "inf" ) != 0 ) {
        passtypeData[ PassQry.FieldAsInteger( "point_arv" ) ].male = PassQry.FieldAsInteger( "male" );
        passtypeData[ PassQry.FieldAsInteger( "point_arv" ) ].female = PassQry.FieldAsInteger( "female" );
        passtypeData[ PassQry.FieldAsInteger( "point_arv" ) ].chd = PassQry.FieldAsInteger( "chd" );
        passtypeData[ PassQry.FieldAsInteger( "point_arv" ) ].inf = PassQry.FieldAsInteger( "inf" );
      }
      PassQry.Next();
    }
    BagQry.Execute();
    while ( !BagQry.Eof ) {
      bagData[ BagQry.FieldAsInteger( "point_arv" ) ].bag_amount = BagQry.FieldAsInteger( "bag_amount" );
      bagData[ BagQry.FieldAsInteger( "point_arv" ) ].bag_weight = BagQry.FieldAsInteger( "bag_weight" );
      bagData[ BagQry.FieldAsInteger( "point_arv" ) ].rk_amount = BagQry.FieldAsInteger( "rk_amount" );
      bagData[ BagQry.FieldAsInteger( "point_arv" ) ].rk_weight = BagQry.FieldAsInteger( "rk_weight" );
      BagQry.Next();
    }

    tst();
    int route_num = 1;
    TTripRoute routes;
    routes.GetRouteBefore( ASTRA::NoExists, point_id, trtWithCurrent, trtWithCancelled );
    ProgTrace( TRACE5, "routes.GetRouteBefore=%d", routes.size() );
    for ( vector<TTripRouteItem>::iterator i=routes.begin(); i!=routes.end(); i++ ) {
      xmlNodePtr n1, n = NewTextChild( node, "route" );
      SetProp( n, "num", route_num );
      route_num++;
      n1 = NewTextChild( n, "airp" );
      SetProp( n1, "code_zrt", i->airp );
      SetProp( n1, "code_iata", ((TAirpsRow&)base_tables.get("airps").get_row( "code", i->airp, true )).code_lat );
      if ( i->pr_cancel )
        SetProp( n1, "pr_cancel", i->pr_cancel );
    }
    routes.clear();
    tst();
    routes.GetRouteAfter( ASTRA::NoExists, point_id, trtNotCurrent, trtWithCancelled );
    ProgTrace( TRACE5, "routes.GetRouteAfter=%d", routes.size() );
    for ( vector<TTripRouteItem>::iterator i=routes.begin(); i!=routes.end(); i++ ) {
      xmlNodePtr n1, n = NewTextChild( node, "route" );
      SetProp( n, "num", route_num );
      route_num++;
      n1 = NewTextChild( n, "airp" );
      SetProp( n1, "code_zrt", i->airp );
      SetProp( n1, "code_iata", ((TAirpsRow&)base_tables.get("airps").get_row( "code", i->airp, true )).code_lat );
      if ( i->pr_cancel )
        SetProp( n1, "pr_cancel", i->pr_cancel );
      n1 = NewTextChild( n, "booking" );
      if ( !resaData[i->airp].empty() ) {
        for ( vector<TResaData>::iterator j=resaData[i->airp].begin(); j!=resaData[i->airp].end(); j++ ) {
          SetProp( NewTextChild( n1, "class", j->resa + j->tranzit ), "code", j->code );
        }
      }
      n1 = NewTextChild( n, "checkin" );
      if ( checkinData.find( i->point_id ) != checkinData.end() ) {
        if ( checkinData[ i->point_id ].f != 0 )
          SetProp( NewTextChild( n1, "class", checkinData[ i->point_id ].f ), "code", "�" );
        if ( checkinData[ i->point_id ].c != 0 )
          SetProp( NewTextChild( n1, "class", checkinData[ i->point_id ].c ), "code", "�" );
        if ( checkinData[ i->point_id ].y != 0 )
          SetProp( NewTextChild( n1, "class", checkinData[ i->point_id ].y ), "code", "�" );
      }
      if ( passtypeData.find( i->point_id ) != passtypeData.end() ) {
        if (  passtypeData[ i->point_id ].male != 0 )
          SetProp( NewTextChild( n1, "pass", passtypeData[ i->point_id ].male ), "type", "�" );
        if (  passtypeData[ i->point_id ].female != 0 )
          SetProp( NewTextChild( n1, "pass", passtypeData[ i->point_id ].female ), "type", "�" );
        if (  passtypeData[ i->point_id ].chd != 0 )
          SetProp( NewTextChild( n1, "pass", passtypeData[ i->point_id ].chd ), "type", "��" );
        if (  passtypeData[ i->point_id ].inf != 0 )
          SetProp( NewTextChild( n1, "pass", passtypeData[ i->point_id ].inf ), "type", "��" );
      }
      if ( bagData.find( i->point_id ) != bagData.end() ) {
        xmlNodePtr n2 = NewTextChild( n1, "baggage" );
        NewTextChild( n2,"weight", bagData[ i->point_id ].bag_weight );
        NewTextChild( n2,"amount", bagData[ i->point_id ].bag_amount );
        n2 = NewTextChild( n1, "hand_bag" );
        NewTextChild( n2, "weight", bagData[ i->point_id ].rk_weight );
        NewTextChild( n2, "amount", bagData[ i->point_id ].rk_amount );
      }
    }
    //����� ॣ����樨
    record = XMLTreeToText( doc ).c_str();
    ProgTrace( TRACE5, "sync_checkin_data: point_id=%d, prior_record=%s", point_id, prior_record.c_str() );
    ProgTrace( TRACE5, "sync_checkin_data: point_id=%d, record=%s", point_id, record.c_str() );
    if ( record != prior_record ) {
      put_string_into_snapshot_points( point_id, FILE_CHECKINDATA_TYPE, point_addr, !prior_record.empty(), record );
      TFileData fd;
      fd.file_data = record;
  	  fd.params[ PARAM_FILE_NAME ] = airline + IntToString( flt_no ) + suffix + DateTimeToStr( scd_out, "yymmddhhnn" ) + ".xml";
  	  fd.params[ NS_PARAM_EVENT_TYPE ] = EncodeEventType( ASTRA::evtFlt );
      fd.params[ NS_PARAM_EVENT_ID1 ] = IntToString( point_id );
      fd.params[ PARAM_TYPE ] = VALUE_TYPE_FILE; // FILE
      fds.push_back( fd );
    }
    xmlFreeDoc( doc );
  }
  catch(...) {
    xmlFreeDoc( doc );
    throw;
  }
  return !fds.empty();
}


void AstraServiceInterface::getFileParams( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT code, name from file_types";
	Qry.Execute();
  xmlNodePtr node = NewTextChild( resNode, "file_types" );
	while (!Qry.Eof) {
		xmlNodePtr n = NewTextChild( node, "type" );
		NewTextChild( n, "code", Qry.FieldAsString( "code" ) );
		NewTextChild( n, "name", Qry.FieldAsString( "name" ) );
		Qry.Next();
	}
	Qry.Clear();
	Qry.SQLText = "SELECT DISTINCT type, receiver FROM files";
	Qry.Execute();
	node = NewTextChild( resNode, "point_addrs" );
	while (!Qry.Eof) {
		xmlNodePtr n = NewTextChild( node, "addrs" );
		NewTextChild( n, "type", Qry.FieldAsString( "type" ) );
		NewTextChild( n, "point_addr", Qry.FieldAsString( "receiver" ) );
		Qry.Next();
	}
}

void AstraServiceInterface::viewFileIds( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  string type = NodeAsString( "type", reqNode );
  string receiver = NodeAsString( "receiver", reqNode );
	TDateTime first_day = NodeAsDateTime( "first_day", reqNode );
	TDateTime last_day = NodeAsDateTime( "last_day", reqNode );
	ProgTrace( TRACE5, "type=%s, receiver=%s, first_day=%f, last_day=%f", type.c_str(), receiver.c_str(), first_day, last_day );
	TQuery Qry( &OraSession );
	Qry.SQLText =
	"SELECT files.id, time, value FROM files, file_params "
	" WHERE type=:type AND receiver=:receiver AND time>=:first_day AND time<=:last_day AND "
	"      files.id=file_params.id(+) AND 'FileName'=file_params.name(+) ";
	Qry.CreateVariable( "type", otString, type );
	Qry.CreateVariable( "receiver", otString, receiver );
	Qry.CreateVariable( "first_day", otDate, first_day );
	Qry.CreateVariable( "last_day", otDate, last_day );
	Qry.Execute();
	xmlNodePtr node = NewTextChild( resNode, "ids" );
	while ( !Qry.Eof ) {
		xmlNodePtr n = NewTextChild( node, "ids" );
		NewTextChild( n, "id", Qry.FieldAsInteger( "id" ) );
		NewTextChild( n, "time", DateTimeToStr( Qry.FieldAsDateTime( "time" ), ServerFormatDateTimeAsString ) );
		NewTextChild( n, "filename", Qry.FieldAsString( "value" ) );
		Qry.Next();
	}

}

void AstraServiceInterface::viewFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	int file_id = NodeAsInteger( "file_id", reqNode );
	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT type, receiver, error, data FROM files WHERE id=:file_id";
	Qry.CreateVariable( "file_id", otInteger, file_id );
	Qry.Execute();
	if ( Qry.Eof )
		throw AstraLocale::UserException( "MSG.FILE.NOT_FOUND" );
 	int len = Qry.GetSizeLongField( "data" );
  void *p = (char*)malloc( len );
 	if ( !p )
 		throw Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
  Qry.FieldAsLong( "data", p );
  string encoding = getFileEncoding(Qry.FieldAsString( "type" ), Qry.FieldAsString( "receiver" ), true );
  string str_file( (char*)p, len );
  ProgTrace( TRACE5, "encoding=%s, file_str=%s", encoding.c_str(), str_file.c_str() );
  if ( !encoding.empty() )
  	str_file = ConvertCodepage( str_file, encoding, "CP866" );
  str_file = ConvertCodepage( str_file, "CP866", "WINDOWS-1251" );
  ProgTrace( TRACE5, "file_str=%s", str_file.c_str() );
  NewTextChild( resNode, "data", StrUtils::b64_encode( str_file.c_str(), len ) );
  free( p );
  Qry.SQLText = "SELECT * FROM file_params WHERE id=:file_id";
	Qry.CreateVariable( "file_id", otInteger, file_id );
	Qry.Execute();
	xmlNodePtr paramsN = NewTextChild( resNode, "params" );
  while ( !Qry.Eof ) {
  	xmlNodePtr n = NewTextChild( paramsN, "param" );
  	NewTextChild( n, "name", Qry.FieldAsString( "name" ) );
  	NewTextChild( n, "value", Qry.FieldAsString( "value" ) );
  	Qry.Next();
  }
}

void AstraServiceInterface::getAodbFiles( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT filename, point_addr, airline FROM aodb_spp_files";
	Qry.Execute();
	xmlNodePtr node = NewTextChild( resNode, "files" );
	while ( !Qry.Eof ) {
		xmlNodePtr n = NewTextChild( node, "file" );
		NewTextChild( n, "filename", Qry.FieldAsString( "filename" ) );
		NewTextChild( n, "point_addr", Qry.FieldAsString( "point_addr" ) );
		NewTextChild( n, "airline", Qry.FieldAsString( "airline" ) );
		Qry.Next();
	}
}


void AstraServiceInterface::getAodbData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	string filename = NodeAsString( "filename", reqNode );
	string point_addr = NodeAsString( "point_addr", reqNode );
	string airline = NodeAsString( "airline", reqNode );
	TQuery Qry( &OraSession );
	Qry.SQLText =
	"SELECT rec_no, record, msg, type, time FROM aodb_events "
	" WHERE filename=:filename AND point_addr=:point_addr AND airline=:airline"
	" ORDER BY rec_no ";
	Qry.CreateVariable( "filename", otString, filename );
	Qry.CreateVariable( "point_addr", otString, point_addr );
	Qry.CreateVariable( "airline", otString, airline );
	Qry.Execute();
	ProgTrace( TRACE5, "filename=%s, point_addr=%s, airline=%s", filename.c_str(), point_addr.c_str(), airline.c_str() );
	xmlNodePtr node = NewTextChild( resNode, "records" );
	while ( !Qry.Eof ) {
		xmlNodePtr n = NewTextChild( node, "record" );
		NewTextChild( n, "rec_no", Qry.FieldAsInteger( "rec_no" ) );
		NewTextChild( n, "record", Qry.FieldAsString( "record" ) );
		NewTextChild( n, "msg", Qry.FieldAsString( "msg" ) );
		NewTextChild( n, "type", Qry.FieldAsString( "type" ) );
		NewTextChild( n, "time", DateTimeToStr( Qry.FieldAsDateTime( "time" ), ServerFormatDateTimeAsString ) );
		Qry.Next();
	}
}

void AstraServiceInterface::logFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  xmlNodePtr n = GetNode( "mes", reqNode );
	string msg = NodeAsString( n );
	ProgTrace( TRACE5, "logFileData=%s", msg.c_str() );
}

void AstraServiceInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};

void put_string_into_snapshot_points( int point_id, std::string file_type,
	                                    std::string point_addr, bool pr_old_record, std::string record )
{
	TQuery Qry( &OraSession );
 	Qry.SQLText =
 	  "DELETE snapshot_points "
 	  " WHERE point_id=:point_id AND file_type=:file_type AND point_addr=:point_addr";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "file_type", otString, file_type );
  Qry.CreateVariable( "point_addr", otString, point_addr );
  Qry.Execute();
 	if ( pr_old_record && record.empty() )
 		return;
 	Qry.Clear();
  Qry.SQLText =
    "INSERT INTO snapshot_points(point_id,file_type,point_addr,record,page_no ) "
    "                VALUES(:point_id,:file_type,:point_addr,:record,:page_no) ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "file_type", otString, file_type );
  Qry.CreateVariable( "point_addr", otString, point_addr );
  Qry.DeclareVariable( "record", otString );
  Qry.DeclareVariable( "page_no", otInteger );
  int i=0;
  while ( !record.empty() ) {
  	Qry.SetVariable( "record", record.substr( 0, 1000 ) );
  	Qry.SetVariable( "page_no", i );
  	Qry.Execute();
  	i++;
  	record.erase( 0, 1000 );
  }
}

void get_string_into_snapshot_points( int point_id, const std::string &file_type,
	                                    const std::string &point_addr, std::string &record )
{
	record.clear();
	TQuery Qry( &OraSession );
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
}
