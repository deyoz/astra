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
#include "jxtlib/jxt_cont.h"
#include "serverlib/str_utils.h"
#include "serverlib/cfgproc.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC;

const double WAIT_ANSWER_SEC = 30.0;   // ждем ответа 30 секунд
const string PARAM_FILE_ID = "file_id";
const string PARAM_NEXT_FILE = "NextFile";

void CommitWork( int file_id );

bool deleteFile( int id )
{
    TQuery Qry(&OraSession);
/*    Qry.SQLText=
      " BEGIN "
      " DELETE file_queue WHERE id= :id; "
      " DELETE file_params WHERE id= :id; "
      "END; ";*/
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
    " FROM file_param_sets "
	  " WHERE file_param_sets.own_point_addr=:own_point_addr AND "
	  "       file_param_sets.point_addr=:point_addr AND "
	  "       file_param_sets.type=:type AND "
	  "       file_param_sets.pr_send=:send AND "
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
  // испраывить!!!! ВЛАД АУ!!!
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
    	if ( !ScanQry.FieldAsInteger( "in_order" ) || // не важен порядок отправки
    		   !(find( vecType.begin(), vecType.end(), in_order_key ) != vecType.end()) &&
    		   ( ScanQry.FieldAsString( "status" ) != "SEND" || // или этот файл еще не отправлен и не было перед ним такого же
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
	  "SELECT DISTINCT thread_type FROM file_param_sets, file_types "
	  " WHERE OWN_POINT_ADDR=:own_point_addr AND point_addr=:point_addr AND file_types.code=file_param_sets.type";
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
	 "SELECT pr_send, thread_type FROM file_param_sets, file_types "
	 " WHERE own_point_addr=:own_point_addr AND point_addr=:point_addr AND file_types.code=file_param_sets.type AND rownum<2";
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
  msg.msg = string("Файл доставлен (тип=" + ftype + ", адресат=" + desk_code + ", ид.=") + IntToString( file_id ) + ")";
  TReqInfo::Instance()->MsgToLog( msg );
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

string getFileEncoding( const string &file_type, const string &point_addr )
{
	string res;
  TQuery EncodeQry( &OraSession );
  EncodeQry.SQLText =
      "select encoding from file_encoding where "
      "   own_point_addr = :own_point_addr and "
      "   type = :type and "
      "   point_addr=:point_addr AND "
      "   pr_send = 1";
  EncodeQry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
  EncodeQry.CreateVariable( "type", otString, file_type );
  EncodeQry.CreateVariable( "point_addr", otString, point_addr );
  EncodeQry.Execute();
  if ( !EncodeQry.Eof )
  	res = EncodeQry.FieldAsString( "encoding" );
  return res;
}

bool CreateCommonFileData( int id, const std::string type, const std::string &airp, const std::string &airline,
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
        " FROM file_param_sets "
        " WHERE own_point_addr=:own_point_addr AND "
        "       type=:type AND "
        "       pr_send = 1 AND "
        "       ( airp IS NULL OR airp=:airp ) AND "
        "       ( airline IS NULL OR airline=:airline ) AND "
        "       ( flt_no IS NULL OR flt_no=:flt_no ) "
        " ORDER BY point_addr,priority DESC";
    Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
    Qry.CreateVariable( "type", otString, type );
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
        	   !Qry.Eof && client_canon_name != Qry.FieldAsString( "point_addr" ) ) { /* если нет такого имени */
          if ( master_params ) {
            fds.clear();
            TFileData fd;
            try {
                if (
                        type == FILE_CENT_TYPE && createCentringFile( id, client_canon_name, fds ) ||
                        type == FILE_SOFI_TYPE && createSofiFile( id, inparams, client_canon_name, fds ) ||
                        type == FILE_AODB_OUT_TYPE && createAODBFiles( id, client_canon_name, fds ) ||
                        type == FILE_SPPCEK_TYPE && createSPPCEKFile( id, client_canon_name, fds ) ||
                        type == FILE_1CCEK_TYPE && Sync1C( client_canon_name, fds ) ) {
                    /* теперь в params еще лежит и имя файла */
                    string encoding = getFileEncoding( type, client_canon_name );
                    for ( vector<TFileData>::iterator i=fds.begin(); i!=fds.end(); i++ ) {
                    	i->params[PARAM_CANON_NAME] = client_canon_name;
                      i->params[ NS_PARAM_AIRP ] = airp;
                      i->params[ NS_PARAM_AIRLINE ] = airline;
                      i->params[ NS_PARAM_FLT_NO ] = flt_no;
                    	string str_file = i->file_data;
                      if ( !encoding.empty() )
                          try {
                              str_file = ConvertCodepage( str_file, "CP866", encoding );
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
                    		msg.msg = string("Файл создан (тип=" + type + ", адресат=" + client_canon_name + ", ид.=") + IntToString( file_id ) + ")";
                    		TReqInfo::Instance()->MsgToLog( msg );
                      }
                    }
                }
            }
            /* ну не получилось сформировать файл, остальные файлы имеют тоже право попробовать сформироваться */
            catch( std::exception &e) {
                ProgError(STDLOG, "file_type=%s, id=%d, what=%s", type.c_str(), id, e.what());
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
	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT airp, airline, flt_no FROM points WHERE point_id=:point_id";
	Qry.CreateVariable( "point_id", otInteger, point_id );
	Qry.Execute();
	if ( !Qry.Eof )
		CreateCommonFileData( point_id, FILE_CENT_TYPE, Qry.FieldAsString( "airp" ),
		                      Qry.FieldAsString( "airline" ), Qry.FieldAsString( "flt_no" ) );
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
	Qry.SQLText = "SELECT airp_dep as airp, airline, flt_no FROM bag_receipts WHERE receipt_id=:receipt_id";
	Qry.CreateVariable( "receipt_id", otInteger, receipt_id );
	Qry.Execute();
	if ( !Qry.Eof )
		CreateCommonFileData( receipt_id, FILE_SOFI_TYPE, Qry.FieldAsString( "airp" ),
		                      Qry.FieldAsString( "airline" ), Qry.FieldAsString( "flt_no" ) );
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

void sync_aodb( void )
{
	TQuery Qry( &OraSession );
	Qry.SQLText =
		 "SELECT DISTINCT points.point_id,points.airline,points.flt_no,points.airp "
		 " FROM points, file_param_sets, trip_final_stages "
		 " WHERE points.point_id = trip_final_stages.point_id AND "
		 "       trip_final_stages.stage_type=:ckin_stage_type AND "
		 "       trip_final_stages.stage_id != :prep_checkin_stage_id AND "
		 "       trip_final_stages.stage_id != :no_active_stage_id AND "
		 "       ( file_param_sets.airp IS NULL OR file_param_sets.airp=points.airp ) AND "
		 "       ( file_param_sets.airline IS NULL OR file_param_sets.airline=points.airline ) AND "
		 "       ( file_param_sets.flt_no IS NULL OR file_param_sets.flt_no=points.flt_no ) AND "
		 "       file_param_sets.type=:type AND pr_send=1 AND own_point_addr=:own_point_addr AND "
		 "       points.act_out IS NULL AND points.pr_del=0 ";
	Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "type", otString, FILE_AODB_OUT_TYPE );
	Qry.CreateVariable( "ckin_stage_type", otInteger, stCheckIn );
	Qry.CreateVariable( "no_active_stage_id", otInteger, sNoActive );
	Qry.CreateVariable( "prep_checkin_stage_id", otInteger, sPrepCheckIn );
	Qry.Execute();
	while ( !Qry.Eof ) {
		CreateCommonFileData( Qry.FieldAsInteger( "point_id" ), FILE_AODB_OUT_TYPE, Qry.FieldAsString( "airp" ),
  	                      Qry.FieldAsString( "airline" ), Qry.FieldAsString( "flt_no" ) );
		Qry.Next();
	}
}

void sync_sppcek( void )
{
	TQuery Qry( &OraSession );
	Qry.SQLText =
	 "SELECT DISTINCT points.point_id,points.airline,points.flt_no,points.airp "
	 " FROM points, file_param_sets "
	 " WHERE file_param_sets.type=:file_type AND pr_send=1 AND own_point_addr=:own_point_addr AND "
	 "       ( points.scd_in >= system.UTCSYSDATE-1 OR "
   "         points.scd_out >= system.UTCSYSDATE-1 ) AND "
	 "       ( file_param_sets.airp IS NULL OR file_param_sets.airp=points.airp ) AND "
	 "       ( file_param_sets.airline IS NULL OR file_param_sets.airline=points.airline ) AND "
	 "       ( file_param_sets.flt_no IS NULL OR file_param_sets.flt_no=points.flt_no ) ";
	Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "file_type", otString, FILE_SPPCEK_TYPE );
	Qry.Execute();
	while ( !Qry.Eof ) {
		CreateCommonFileData( Qry.FieldAsInteger( "point_id" ), FILE_SPPCEK_TYPE, Qry.FieldAsString( "airp" ),
  	                      Qry.FieldAsString( "airline" ), Qry.FieldAsString( "flt_no" ) );
		Qry.Next();
	}

}

void sync_1ccek( void )
{
	CreateCommonFileData( -1, FILE_1CCEK_TYPE, "ЧЛБ", "", "" );
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
	TQuery Qry( &OraSession );
  TQuery EncodeQry( &OraSession );
  EncodeQry.SQLText =
      "select encoding from file_encoding where "
      "   own_point_addr = :own_point_addr and "
      "   type = :type AND "
      "   point_addr=:point_addr AND "
      "   pr_send = 0";
  EncodeQry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
  EncodeQry.CreateVariable( "point_addr", otString, fileparams[ "canon_name" ] );
  EncodeQry.CreateVariable( "type", otString, FILE_AODB_IN_TYPE );
  EncodeQry.Execute();

  string convert_aodb;
  if ( !EncodeQry.Eof )
      try {
      	  convert_aodb = EncodeQry.FieldAsString( "encoding" );
          file_data = ConvertCodepage( file_data, convert_aodb, "CP866" );
          convert_aodb.clear();
      }
      catch( EConvertError &E ) {
        ProgError(STDLOG, E.what());
      }
  JxtContext::JxtCont *sysCont = JxtContext::getJxtContHandler()->sysContext();
  string airline = sysCont->read( fileparams[ "canon_name" ] + "_" + OWN_POINT_ADDR() + "_file_param_sets.airline" );
  ParseAndSaveSPP( fileparams[ PARAM_FILE_NAME ], fileparams[ "canon_name" ], airline, file_data, convert_aodb );
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
  string encoding = getFileEncoding(Qry.FieldAsString( "type" ), Qry.FieldAsString( "receiver" ) );
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
