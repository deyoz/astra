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
#include "sofi.h"
#include "aodb.h"
#include "cent.h"
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

const double WAIT_ANSWER_SEC = 30.0;   // ждем ответа 30 секунд
const string PARAM_FILE_ID = "file_id";
const string PARAM_NEXT_FILE = "NextFile";
#define ENDL "\r\n"

void CommitWork( int file_id );

bool createCheckinDataFiles( int point_id, const std::string &point_addr, TFileDatas &fds );
bool CreateCommonFileData( bool pr_commit, const std::string &point_addr,
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
             const map<string,string> &params,
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
        for(map<string,string>::const_iterator i=params.begin();i!=params.end();++i)
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

int putMail( const string &receiver,
             const string &sender,
             const string &type,
             map<string,string> &params,
             const string &file_data )
{
	int file_id = ASTRA::NoExists;
  ofstream f;
  string filename;
  if ( params.find( PARAM_FILE_NAME ) == params.end() )
    throw Exception( "Can't find param FileName" );
  filename = params[ PARAM_FILE_NAME ];
  TQuery FilesQry(&OraSession);
  FilesQry.Clear();
  FilesQry.SQLText=
    "SELECT name,dir,last_create,airp "
    "FROM file_sets "
    "WHERE code=:code AND pr_denial=0";
  FilesQry.CreateVariable( "code", otString, type );
  FilesQry.Execute();
  if ( FilesQry.Eof )
    throw Exception( "Can't find file type in file_sets" );
  filename = string( FilesQry.FieldAsString( "dir" ) ) + filename;
  f.open( filename.c_str() );
  if (!f.is_open()) throw Exception( "Can't open file '%s'", filename.c_str() );
  try {
    TQuery Qry(&OraSession);
    Qry.SQLText = "SELECT tlgs_id.nextval id FROM dual";
    Qry.Execute();
    file_id = Qry.FieldAsInteger( "id" );
    f << file_data;
    f << ENDL;
    f.close();
  }
  catch(...)
  {
    try { f.close(); } catch( ... ) { };
    try
    {
      //в случае ошибки запишем пустой файл
      f.open( filename.c_str() );
      if ( f.is_open() ) f.close();
    }
    catch( ... ) { };
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
  int point_id;
  TSQLCondDates() {
    point_id = ASTRA::NoExists;
  }
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
                               const vector<string> &airps, const vector<int> &flt_nos,
                               const map<string,string> &params, int point_id ) {
    return true;
  }
  virtual bool validatePoints( int point_id ) {
    return true;
  }
  void createPointSQL( const TSQLCondDates &cond_dates ) {
    TQuery Qry( &OraSession );
    TQuery PointsQry( &OraSession );
    Qry.SQLText =
      "SELECT point_addr,airline,flt_no,airp,param_name,param_value FROM file_param_sets, desks "
      " WHERE type=:type AND own_point_addr=:own_point_addr AND pr_send=:pr_send AND "
      "       desks.code=file_param_sets.point_addr "
      "ORDER BY point_addr";
  	string point_addr;
    vector<string> airlines, airps;
	  vector<int> flt_nos;
    map<string,string> params;
	  bool pr_empty_airline = false, pr_empty_airp = false, pr_empty_flt_no = false;
	  Qry.CreateVariable( "type", otString, type );
	  Qry.CreateVariable( "pr_send", otInteger, pr_send );
	  Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	  //ProgTrace( TRACE5, "type=%s, pr_send=%d, own_point_addr=%s", type.c_str(), pr_send, OWN_POINT_ADDR() );
	  Qry.Execute();
	  while ( true ) {
      //ProgTrace( TRACE5, "Qry.Eof=%d, point_addr=%s", Qry.Eof, point_addr.c_str() );
      if ( !point_addr.empty() && ( Qry.Eof || point_addr != Qry.FieldAsString( "point_addr" ) ) ) { //переход к след. point_addr
        if ( pr_empty_airline )
          airlines.clear();
        if ( pr_empty_airp )
          airps.clear();
        if ( pr_empty_flt_no )
          flt_nos.clear();
        ProgTrace( TRACE5, "point_addr=%s, airlines.size()=%zu, airps.size()=%zu,flt_nos.size()=%zu",
                   point_addr.c_str(), airlines.size(), airps.size(),  flt_nos.size() );
        if ( validateParams( point_addr, airlines, airps, flt_nos, params, cond_dates.point_id ) ) {
          ProgTrace( TRACE5, "validateparams" );
          PointsQry.Clear();
          string res = "SELECT point_id, airline, airp, flt_no FROM points WHERE ";
          if ( cond_dates.sql.empty() ) //такого не должно быть
            res = " 1=1 ";
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
              idx++;
            }
            res += ")";
          }
          for ( map<string,TDateTime>::const_iterator idate=cond_dates.dates.begin();
                idate!=cond_dates.dates.end(); idate++ ) {
            PointsQry.CreateVariable( idate->first, otDate, idate->second );
          }
          if ( cond_dates.point_id != ASTRA::NoExists )
            PointsQry.CreateVariable( "point_id", otInteger, cond_dates.point_id );
          PointsQry.SQLText = res;
          ProgTrace( TRACE5, "type=%s, PointsQry.SQLText=%s, point_addr=%s", type.c_str(), res.c_str(), point_addr.c_str() );
          PointsQry.Execute();
          while ( !PointsQry.Eof ) {
            if ( validatePoints( PointsQry.FieldAsInteger( "point_id" ) ) ) {
            ProgTrace( TRACE5, "type=%s, point_addr=%s, point_id=%d", type.c_str(), point_addr.c_str(), PointsQry.FieldAsInteger( "point_id" ) );
            CreateCommonFileData( cond_dates.point_id == ASTRA::NoExists,
                                  point_addr,
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
        params.clear();
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
      params[ Qry.FieldAsString( "param_name" ) ] = Qry.FieldAsString( "param_value" );
      ProgTrace( TRACE5, "point_addr=%s, param_name=%s,param_value=%s,airline=%s,airp=%s,flt_no=%s",
                 point_addr.c_str(), Qry.FieldAsString( "param_name" ),
                 Qry.FieldAsString( "param_value" ), Qry.FieldAsString( "airline" ),
                 Qry.FieldAsString( "airp" ),Qry.FieldAsString( "flt_no" ) );
      Qry.Next();
	  }
  }
};

void getFileParams(
        const std::string &airp,
        const std::string &airline,
        const std::string &flt_no,
        const std::string &client_canon_name,
        const std::string &type,
        bool send,
        map<string,string> &fileparams)
{
    TQuery ParamQry(&OraSession);
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

    ProgTrace(TRACE5, "airp: %s", airp.c_str());
    ProgTrace(TRACE5, "airline: %s", airline.c_str());
    ProgTrace(TRACE5, "flt_no: %s", flt_no.c_str());
    ProgTrace(TRACE5, "own_point_addr: %s", OWN_POINT_ADDR());
    ProgTrace(TRACE5, "point_addr: %s", client_canon_name.c_str());
    ProgTrace(TRACE5, "type: %s", type.c_str());
    ProgTrace(TRACE5, "send: %d", send);

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
}

void getFileParams( int id, map<string, string> &fileparams)
{
  fileparams.clear();
  TQuery ParamQry( &OraSession );
  ParamQry.SQLText = "SELECT name,value FROM file_params WHERE id=:id";
  ParamQry.CreateVariable( "id", otInteger, id );
  ParamQry.Execute();
  for(;!ParamQry.Eof;ParamQry.Next())
    fileparams[ string( ParamQry.FieldAsString( "name" ) ) ] = ParamQry.FieldAsString( "value" );
};

void getFileParams( const std::string client_canon_name, const std::string &type,
	                  int id, map<string, string> &fileparams, bool send )
{
    fileparams.clear();
    map<string, string> fp;
    getFileParams(id, fp);
    //ProgTrace( TRACE5, "id=%d", id );
    string airline, airp, flt_no;
    for(map<string, string>::const_iterator p=fp.begin(); p!=fp.end(); ++p)
        if ( p->first == NS_PARAM_AIRP )
            airp = p->second;
        else
            if ( p->first == NS_PARAM_AIRLINE )
                airline = p->second;
            else
                if ( p->first == NS_PARAM_FLT_NO )
                    flt_no = p->second;
                else
                    if ( p->first != NS_PARAM_EVENT_TYPE &&
                         p->first != NS_PARAM_EVENT_ID1 &&
                         p->first != NS_PARAM_EVENT_ID2 &&
                         p->first != NS_PARAM_EVENT_ID3 ) {
                      fileparams.insert(*p);
                      //ProgTrace( TRACE5, "name=%s, value=%s", p->first.c_str(), p->second.c_str() );
                    }

    getFileParams(
            airp,
            airline,
            flt_no,
            client_canon_name,
            type,
            send,
            fileparams);
    // испраывить!!!! ВЛАД АУ!!!
    if ( type != "BSM" )
        fileparams[ PARAM_FILE_TYPE ] = type;
    else
        fileparams[ PARAM_TYPE ] = type;
    fileparams[ PARAM_FILE_ID ] = IntToString( id );
    TQuery ParamQry( &OraSession );
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
    	if ( !ScanQry.FieldAsInteger( "in_order" ) || // не важен порядок отправки
    		   ( !(find( vecType.begin(), vecType.end(), in_order_key ) != vecType.end()) &&
    		     ( string(ScanQry.FieldAsString( "status" )) != string("SEND") || // или этот файл еще не отправлен и не было перед ним такого же
      		     ScanQry.FieldAsDateTime( "time" ) + WAIT_ANSWER_SEC/(60.0*60.0*24.0) < ScanQry.FieldAsDateTime( "now" )
      		   )
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
          errorFile( file_id, string("Ошибка отправки сообщения: ") + E.what() );
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
{ /* теперь есть разделение по а/к */
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
	  string region = CityTZRegion( "МОВ" );
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
  	  msg.msg = string("Файл отправлен (тип=" + string( Qry.FieldAsString( "type" ) ) + ", адресат=" + desk_code + ", ид.=") +
                IntToString( file_id ) + ", задержка=" + IntToString( (int)(wait_time*60.0*60.0*24.0)) + "сек.)";
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
  	  msg.msg = string("Файл отправлен (тип=" + string( Qry.FieldAsString( "type" ) ) + ", адресат=" + desk_code + ", ид.=") +
                IntToString( file_id ) + ", задержка=" + IntToString( (int)(wait_time*60.0*60.0*24.0)) + "сек.)";
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
  ProgTrace( TRACE5, "CommitWork: param file_id=%d", file_id );
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT type FROM file_queue WHERE id=:id";
  Qry.CreateVariable( "id", otInteger, file_id );
  Qry.Execute();
  if ( Qry.Eof ) {
    ProgTrace( TRACE5, ">>>commitFileData: already commited file_id=%d", file_id );
    return;
  }
  string ftype = Qry.FieldAsString( "type" );
  Qry.Clear();
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
  doneFile( file_id );
  msg.msg = string("Файл доставлен (тип=" + ftype + ", адресат=" + desk_code + ", ид.=") + IntToString( file_id ) + ")";
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

bool CreateCommonFileData( bool pr_commit,
                           const std::string &point_addr,
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
        if ( ( Qry.Eof && !client_canon_name.empty() ) ||
        	   ( !Qry.Eof && client_canon_name != Qry.FieldAsString( "point_addr" ) ) ) { /* если нет такого имени */
          if ( master_params ) {
            fds.clear();
            TFileData fd;
            try {
                if (
                        ( type == FILE_SOFI_TYPE && createSofiFile( id, inparams, client_canon_name, fds ) ) ||
                        ( type == FILE_AODB_OUT_TYPE && createAODBFiles( id, client_canon_name, fds ) ) ||
                        ( type == FILE_SPPCEK_TYPE && createSPPCEKFile( id, client_canon_name, fds ) ) ||
                        ( type == FILE_1CCEK_TYPE && Sync1C( client_canon_name, fds ) ) ||
                        ( type == FILE_CHECKINDATA_TYPE && createCheckinDataFiles( id, client_canon_name, fds ) ) ) {
                    /* теперь в params еще лежит и имя файла */
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
                      int file_id;
                      if ( inparams.find( PARAM_MAIL_INTERVAL ) != inparams.end() )
                        file_id = putMail( client_canon_name, OWN_POINT_ADDR(), type, i->params, str_file );
                      else
                        file_id = putFile( client_canon_name, OWN_POINT_ADDR(), type, i->params, str_file );
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
                    		msg.msg = string("Файл создан (тип=" + type + ", адресат=" + client_canon_name + ", ид.=") + IntToString( file_id ) + ")";
                    		TReqInfo::Instance()->MsgToLog( msg );
                      }
                    }
                    if ( pr_commit ) {
                      OraSession.Commit();
                    }
                }
            }
            /* ну не получилось сформировать файл, остальные файлы имеют тоже право попробовать сформироваться */
            catch(EOracleError &E)
            {
              if ( pr_commit ) {
                try { OraSession.Rollback(); }catch(...){};
              }
              ProgError( STDLOG, "EOracleError file_type=%s, %d: %s", type.c_str(), E.Code, E.what());
              ProgError( STDLOG, "SQL: %s", E.SQLText());
            }
            catch( std::exception &e) {
              if ( pr_commit ) {
                try { OraSession.Rollback(); }catch(...){};
              }
              ProgError(STDLOG, "exception file_type=%s, id=%d, what=%s", type.c_str(), id, e.what());
            }
            catch(...) {
              if ( pr_commit ) {
                try { OraSession.Rollback(); }catch(...){};
              }
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
  		CreateCommonFileData( false,
                            Qry.FieldAsString("point_addr"),
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
                               const vector<string> &airps, const vector<int> &flt_nos,
                               const map<string,string> &params, int point_id ) {
    return ( airps.size() == 1 ); // выбираем рейсы. Это необходимое условие
  }
  virtual bool validatePoints( int point_id ) {
    StagesQry->SetVariable( "point_id", point_id );
    StagesQry->Execute();
    return ( !StagesQry->Eof &&
              StagesQry->FieldAsInteger( "stage_id" ) != sNoActive &&
              StagesQry->FieldAsInteger( "stage_id" ) != sPrepCheckIn );
  }
};

void sync_aodb( int point_id )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT TRUNC(system.UTCSYSDATE) currdate FROM dual";
  Qry.Execute();
  TDateTime currdate = Qry.FieldAsDateTime( "currdate" );
  
  TAODBPointAddr point_addr;
  TSQLCondDates cond_dates;
  cond_dates.sql = " time_out in (:day1,:day2) AND pr_del=0 ";
  if ( point_id == ASTRA::NoExists )
    cond_dates.sql += " AND act_out IS NULL ";
  else
    cond_dates.sql += " AND point_id=:point_id ";
  cond_dates.point_id = point_id;
  cond_dates.dates.insert( make_pair( "day1", currdate ) );
  cond_dates.dates.insert( make_pair( "day2", currdate + 1 ) );
  point_addr.createPointSQL( cond_dates );
}

void sync_aodb( void )
{
  sync_aodb( ASTRA::NoExists );
}

class TSPPCEKPointAddr: public TPointAddr {
public:
  TSPPCEKPointAddr( ):TPointAddr( FILE_SPPCEK_TYPE, true ) {}
  ~TSPPCEKPointAddr( ) {}
  virtual bool validateParams( const string &point_addr, const vector<string> &ailines,
                               const vector<string> &airps, const vector<int> &flt_nos,
                               const map<string,string> &params, int point_id ) {
    return ( airps.size() == 1 ); // выбираем рейсы. Это необходимое условие
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
	  CreateCommonFileData( true, Qry.FieldAsString( "point_addr" ), -1, FILE_1CCEK_TYPE, "ЧЛБ", "", "" );
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
                               const vector<string> &airps, const vector<int> &flt_nos,
                               const map<string,string> &params, int point_id ) {

    if ( airps.size() != 1 ) // выбираем рейсы. Это необходимое условие
      return false;
    if ( params.find( PARAM_MAIL_INTERVAL ) != params.end() && point_id == ASTRA::NoExists ) { // проверка на то, что пора создавать файл
      int interval;
      if ( StrToInt( params.find( PARAM_MAIL_INTERVAL )->second.c_str(), interval ) == EOF ) {
        interval = 30;
        ProgError( STDLOG, "createCheckinDataFiles: mail interval not set, default = 30 min" );
      }
      ProgTrace( TRACE5, "TCheckinDataPointAddr->validateParams: interval=%d", interval );
      TQuery QryFileSets( &OraSession );
      QryFileSets.SQLText =
        "UPDATE file_sets SET last_create=system.UTCSYSDATE"
        " WHERE code=:code AND pr_denial=0 AND airp=:airp AND NVL(last_create+:interval/(24*60),system.UTCSYSDATE)<=system.UTCSYSDATE";
      QryFileSets.CreateVariable( "code", otString, FILE_CHECKINDATA_TYPE );
      QryFileSets.CreateVariable( "airp", otString, *airps.begin() );
      QryFileSets.CreateVariable( "interval", otInteger, interval );
      QryFileSets.Execute();
      ProgTrace( TRACE5, "TCheckinDataPointAddr->validateParams return %d", QryFileSets.RowsProcessed() );
      return QryFileSets.RowsProcessed();
    }
    return true;
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

void sync_checkin_data( int point_id )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT TRUNC(system.UTCSYSDATE) currdate FROM dual";
  Qry.Execute();
  TDateTime currdate = Qry.FieldAsDateTime( "currdate" );

  TCheckinDataPointAddr point_addr;
  TSQLCondDates cond_dates;
  cond_dates.sql = " time_out in (:day1,:day2) AND pr_del=0 ";
  if ( point_id ==  ASTRA::NoExists )
    cond_dates.sql += " AND act_out IS NULL ";
  else
    cond_dates.sql += " AND point_id=:point_id ";
  cond_dates.point_id = point_id;
  cond_dates.dates.insert( make_pair( "day1", currdate ) );
  cond_dates.dates.insert( make_pair( "day2", currdate + 1 ) );
  point_addr.createPointSQL( cond_dates );
};

void sync_checkin_data( )
{
  sync_checkin_data( ASTRA::NoExists );
}


bool createCheckinDataFiles( int point_id, const std::string &point_addr, TFileDatas &fds )
{
  fds.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT point_num,first_point,pr_tranzit,airline, flt_no, suffix,suffix_fmt,airp,scd_out,est_out,act_out,pr_del FROM points "
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
    
  string airline = Qry.FieldAsString( "airline" );
  string airline_lat = ElemIdToElem( etAirline, airline, efmtCodeInter, AstraLocale::LANG_EN );
  if ( airline_lat.empty()) {
    ProgError( STDLOG, "createCheckinDataFiles: airline_lat empty (code=%s)",airline.c_str() );
    return false;
  }
  int flt_no = Qry.FieldAsInteger( "flt_no" );
  string suffix = ElemIdToElemCtxt( ecDisp, etSuffix, Qry.FieldAsString( "suffix" ), (TElemFmt)Qry.FieldAsInteger( "suffix_fmt" ) );
  string suffix_lat = ElemIdToElem( etSuffix, suffix, efmtCodeInter, AstraLocale::LANG_EN );
  if ( !suffix.empty() && suffix_lat.empty() ) {
    ProgError( STDLOG, "createCheckinDataFiles: sufix_lat empty (code=%s)", suffix.c_str() );
    return false;
  }

  string airp = Qry.FieldAsString( "airp" );
  TDateTime scd_out = Qry.FieldAsDateTime( "scd_out" );
  TTripRoute routesB, routesA;
  routesA.GetRouteAfter( ASTRA::NoExists,
                         point_id,
                         Qry.FieldAsInteger( "point_num" ),
                         Qry.FieldIsNULL("first_point")?ASTRA::NoExists:Qry.FieldAsInteger("first_point"),
                         Qry.FieldAsInteger( "pr_tranzit" ),
                         trtNotCurrent,
                         trtNotCancelled );
  ProgTrace( TRACE5, "point_id=%d, routesA.size()=%zu", point_id, routesA.size() );
  if ( routesA.empty() ) {
      return false;
  }
  routesB.GetRouteBefore( ASTRA::NoExists,
                          point_id,
                          Qry.FieldAsInteger( "point_num" ),
                          Qry.FieldIsNULL("first_point")?ASTRA::NoExists:Qry.FieldAsInteger("first_point"),
                          Qry.FieldAsInteger( "pr_tranzit" ),
                          trtWithCurrent,
                          trtNotCancelled );

  TBalanceData balanceData;
  balanceData.get( point_id, Qry.FieldAsInteger( "pr_tranzit" ), routesB, routesA, true );
 TQuery ResaQry( &OraSession );
  ResaQry.SQLText =
    "SELECT COUNT(*) resa, airp_arv, class FROM tlg_binding,crs_pnr,crs_pax "
    " WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
    "       crs_pax.pr_del=0 AND "
    "       tlg_binding.point_id_spp=:point_id AND "
    "       tlg_binding.point_id_tlg=crs_pnr.point_id AND "
    "       crs_pnr.system='CRS' "
    "GROUP BY airp_arv, class";
  ResaQry.CreateVariable( "point_id", otInteger, point_id );
  tst();
  string prior_record, record;
  get_string_into_snapshot_points( point_id, FILE_CHECKINDATA_TYPE, point_addr, prior_record );
  xmlDocPtr doc = CreateXMLDoc( "UTF-8", "flight" );
  tst();
  try {
    xmlNodePtr node = doc->children;
    xmlNodePtr n = NewTextChild( node, "airline" );
    SetProp( n, "code_zrt", airline );
    SetProp( n, "code_iata", airline_lat );
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
    int route_num = 1;
    TTripRoute routes;
    routes.GetRouteBefore( ASTRA::NoExists, point_id, trtWithCurrent, trtWithCancelled );
    ProgTrace( TRACE5, "routes.GetRouteBefore=%zu", routes.size() );
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
    ProgTrace( TRACE5, "routes.GetRouteAfter=%zu", routes.size() );
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
      for ( vector<TDestBalance>::iterator ibal=balanceData.balances.begin(); ibal!=balanceData.balances.end(); ibal++ ) {
        if ( ibal->point_id == i->point_id ) {
          // включая транзитных пассажиров
          xmlNodePtr n2 = NULL;
          for( map<string,TBalance>::iterator iclass=ibal->tranzit_classbal.begin(); iclass!=ibal->tranzit_classbal.end(); iclass++ ) {
            if ( n2 == NULL )
              n2 = NewTextChild( n1, "tranzit" );
            xmlNodePtr n3 = NewTextChild( n2, "class" );
            SetProp( n3, "code", iclass->first );
            if ( iclass->second.male > 0 )
              SetProp( NewTextChild( n3, "male", iclass->second.male ), "seats", iclass->second.male_seats );
            if ( iclass->second.female > 0 )
              SetProp( NewTextChild( n3, "female", iclass->second.female ), "seats", iclass->second.female_seats );
            if ( iclass->second.chd > 0 )
              SetProp( NewTextChild( n3, "chd", iclass->second.chd ), "seats", iclass->second.chd_seats );
            if ( iclass->second.inf > 0 )
              SetProp( NewTextChild( n3, "inf", iclass->second.inf ), "seats", iclass->second.inf_seats );
            if ( iclass->second.rk_weight > 0 )
              NewTextChild( n3, "rk_weight", iclass->second.rk_weight );
            if ( iclass->second.bag_amount > 0 )
              NewTextChild( n3, "bag_amount", iclass->second.bag_amount );
            if ( iclass->second.bag_weight > 0 )
              NewTextChild( n3, "bag_weight", iclass->second.bag_weight );
            if ( iclass->second.paybag_weight > 0 )
              NewTextChild( n3, "paybag_weight", iclass->second.paybag_weight );

          }
          n2 = NULL;
          for( map<string,TBalance>::iterator iclass=ibal->goshow_classbal.begin(); iclass!=ibal->goshow_classbal.end(); iclass++ ) {
            if ( n2 == NULL )
              n2 = NewTextChild( n1, "goshow" );
            xmlNodePtr n3 = NewTextChild( n2, "class" );
            SetProp( n3, "code", iclass->first );
            if ( iclass->second.male > 0 )
              SetProp( NewTextChild( n3, "male", iclass->second.male ), "seats", iclass->second.male_seats );
            if ( iclass->second.female > 0 )
              SetProp( NewTextChild( n3, "female", iclass->second.female ), "seats", iclass->second.female_seats );
            if ( iclass->second.chd > 0 )
              SetProp( NewTextChild( n3, "chd", iclass->second.chd ), "seats", iclass->second.chd_seats );
            if ( iclass->second.inf > 0 )
              SetProp( NewTextChild( n3, "inf", iclass->second.inf ), "seats", iclass->second.inf_seats );
            if ( iclass->second.rk_weight > 0 )
              NewTextChild( n3, "rk_weight", iclass->second.rk_weight );
            if ( iclass->second.bag_amount > 0 )
              NewTextChild( n3, "bag_amount", iclass->second.bag_amount );
            if ( iclass->second.bag_weight > 0 )
              NewTextChild( n3, "bag_weight", iclass->second.bag_weight );
            if ( iclass->second.paybag_weight > 0 )
              NewTextChild( n3, "paybag_weight", iclass->second.paybag_weight );
          }
        }
      }
    }
    //данные регистрации
    record = XMLTreeToText( doc ).c_str();
//    ProgTrace( TRACE5, "sync_checkin_data: point_id=%d, prior_record=%s", point_id, prior_record.c_str() );
//    ProgTrace( TRACE5, "sync_checkin_data: point_id=%d, record=%s", point_id, record.c_str() );
    if ( record != prior_record ) {
      put_string_into_snapshot_points( point_id, FILE_CHECKINDATA_TYPE, point_addr, !prior_record.empty(), record );
      TFileData fd;
      fd.file_data = record;
  	  fd.params[ PARAM_FILE_NAME ] = airline_lat + IntToString( flt_no ) + suffix_lat + DateTimeToStr( scd_out, "yymmddhhnn" ) + ".xml";
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
  ProgTrace( TRACE5, "createCheckinDataFiles return %d", !fds.empty() );
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

void AstraServiceInterface::getTcpClientData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  string client_type = NodeAsString( "client_type", reqNode );
  int last_id = NodeAsInteger( "last_id", reqNode, ASTRA::NoExists );
  int minutes = NodeAsInteger( "minutes", reqNode, ASTRA::NoExists );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT id, file_in_id, file_out_id, time "
    " FROM tcp_data_events "
    " WHERE client_type=:client_type AND "
    "       ( :minutes IS NULL OR time>system.UTCSYSDATE() - :minutes/24 ) AND "
    "       ( :last_id IS NULL OR id > :last_id ) "
    "ORDER BY id ";
  Qry.CreateVariable( "client_type", otString, client_type );
  if ( last_id == ASTRA::NoExists )
    Qry.CreateVariable( "last_id", otInteger, FNull );
  else
    Qry.CreateVariable( "last_id", otInteger, last_id );
  if ( minutes == ASTRA::NoExists )
    Qry.CreateVariable( "minutes", otInteger, FNull );
  else
    Qry.CreateVariable( "minutes", otInteger, minutes );
  TQuery FilesQry( &OraSession );
  FilesQry.SQLText =
    "SELECT data from files WHERE id=:id";
  FilesQry.DeclareVariable( "id", otInteger );
  Qry.Execute();
  xmlNodePtr node = NewTextChild( resNode, "datas" );
  char *p = NULL;
  while ( !Qry.Eof ) {
    xmlNodePtr nodeItem = NewTextChild( node, "item" );
    NewTextChild( nodeItem, "id", Qry.FieldAsInteger( "id" ) );
    NewTextChild( nodeItem, "time", DateTimeToStr( Qry.FieldAsDateTime( "time" ), ServerFormatDateTimeAsString ) );
    if ( Qry.FieldIsNULL( "file_in_id" ) )
      NewTextChild( nodeItem, "data_in" );
    else {
      FilesQry.SetVariable( "id", Qry.FieldAsInteger( "file_in_id" ) );
      FilesQry.Execute();
      int len = FilesQry.GetSizeLongField( "data" );
     	if ( p )
     		p = (char*)realloc( p, len );
     	else
     	  p = (char*)malloc( len );
     	if ( !p )
     		throw Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
      FilesQry.FieldAsLong( "data", p );
      string strbuf( (const char*)p, len );
      string sss;
      StringToHex( strbuf, sss );
      NewTextChild( nodeItem, "data_in", sss );
    }
    if ( Qry.FieldIsNULL( "file_out_id" ) )
      NewTextChild( nodeItem, "data_out" );
    else {
      FilesQry.SetVariable( "id", Qry.FieldAsInteger( "file_out_id" ) );
      FilesQry.Execute();
      int len = FilesQry.GetSizeLongField( "data" );
     	if ( p )
     		p = (char*)realloc( p, len );
     	else
     	  p = (char*)malloc( len );
     	if ( !p )
     		throw Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
      FilesQry.FieldAsLong( "data", p );
      string strbuf( (const char*)p, len );
      string sss;
      StringToHex( strbuf, sss );
      NewTextChild( nodeItem, "data_out", sss );
    }
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
