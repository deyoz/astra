#include <stdlib.h>
#include "astra_service.h"
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "oralib.h"
#include "str_utils.h"
#include "astra_utils.h"
#include "basic.h"
#include "stl_utils.h"
//#include "flight_cent_dbf.h" //???
#include "cfgproc.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

const double WAIT_ANSWER_SEC = 30;   // ждем ответа 30 секунд
const string PARAM_TYPE = "type";
const string PARAM_FILE_ID = "file_id";
const string PARAM_NEXT_FILE = "NextFile";

const char* OWN_POINT_ADDR()
{
  static string OWNADDR;
  if ( OWNADDR.empty() ) {
    char r[100];
    r[0]=0;
    if ( get_param( "OWN_POINT_ADDR", r, sizeof( r ) ) < 0 )
      throw EXCEPTIONS::Exception( "Can't read param OWN_POINT_ADDR" );
    OWNADDR = r;
    ProgTrace( TRACE5, "OWN_POINT_ADDR=%s", OWNADDR.c_str() );
  }
  return OWNADDR.c_str();
};

bool deleteFile( int id )
{
    TQuery Qry(&OraSession);
    Qry.SQLText=
      " BEGIN "
      " DELETE file_queue WHERE id= :id; "
      " DELETE file_params WHERE id= :id; "
      "END; ";
    Qry.CreateVariable("id",otInteger,id);
    Qry.Execute();
    return Qry.RowsProcessed()>0;
};

void putFile(const char* receiver,
             const char* sender,
             const char* type,
             map<string,string> &params,
             int data_len,
             const void* data)
{
    try
    {
        TQuery Qry(&OraSession);
        Qry.SQLText=
                "INSERT INTO "
                "file_queue(id,sender,receiver,type,status,time) "
                "VALUES"
                "(tlgs_id.nextval,:sender,:receiver,"
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
        Qry.SetLongVariable("data",(void*)data,data_len);
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
          Qry.SetVariable("name",i->first);
          Qry.SetVariable("value",i->second);
          Qry.Execute();
        };
    }
    catch( std::exception &e)
    {
        ProgError(STDLOG, e.what());
        throw;
    }
    catch(...)
    {
        ProgError(STDLOG, "putFile: Unknown error while trying to put file");
        throw;
    };
};

bool errorFile( int id, const string &msg )
{
    if (deleteFile(id))
    {
      TQuery ErrQry(&OraSession);
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

void getFileParams( int id, map<string,string> &fileparams )
{
	fileparams.clear();
	TQuery ParamQry( &OraSession );
	ParamQry.SQLText = "SELECT name,value FROM file_params WHERE id=:id";
	ParamQry.CreateVariable( "id", otInteger, id );
  ParamQry.Execute();
	while ( !ParamQry.Eof ) {
		fileparams[ string( ParamQry.FieldAsString( "name" ) ) ] = ParamQry.FieldAsString( "value" );
		ParamQry.Next();
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

void buildFileData( xmlNodePtr resNode, const std::string &client_canon_name )
{
	TQuery ScanQry( &OraSession );
	ScanQry.SQLText =
		"SELECT file_queue.id,file_queue.sender,file_queue.receiver,file_queue.type,"
		"       file_queue.status,file_queue.time,system.UTCSYSDATE AS now, "
		"       files.data "
		" FROM file_queue,files "
		" WHERE file_queue.id=files.id AND "
    "       file_queue.sender=:sender AND "
    "       ( file_queue.status='PUT' OR file_queue.status='SEND' AND file_queue.time + :wait_answer_sec/(60*60*24) < system.UTCSYSDATE  ) "
    " ORDER BY file_queue.time,file_queue.id";
	ScanQry.CreateVariable( "sender", otString, OWN_POINT_ADDR() );
	ScanQry.CreateVariable( "wait_answer_sec", otInteger, WAIT_ANSWER_SEC );
	ScanQry.Execute();
	map<string,string> fileparams;
	char *p = NULL;
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );

	while ( !ScanQry.Eof ) {
  	int file_id = ScanQry.FieldAsInteger( "id" );
    try
    {
      if ( client_canon_name == ScanQry.FieldAsString( "receiver" ) ) {
      	tst();
        getFileParams( file_id, fileparams );
      	int len = ScanQry.GetSizeLongField( "data" );
      	if ( p )
      		p = (char*)realloc( p, len );
      	else
      	  p = (char*)malloc( len );
      	if ( !p )
      		throw Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
        ScanQry.FieldAsLong( "data", p );
        xmlNodePtr fileNode = NewTextChild( dataNode, "file" );
        NewTextChild( fileNode, "data", b64_encode( (const char*)p, len ) );
        fileparams[ PARAM_TYPE ] = ScanQry.FieldAsString( "type" );
        ScanQry.Next();
        if ( !ScanQry.Eof )
        	fileparams[ PARAM_NEXT_FILE ] = "TRUE";
        else
        	fileparams[ PARAM_NEXT_FILE ] = "FALSE";
        fileparams[ PARAM_FILE_ID ] = IntToString( file_id );
        buildFileParams( dataNode, fileparams );
        ProgTrace( TRACE5, "file_id=%d, msg.size()=%d", file_id, len );
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
      	tst();
      	try {
          errorFile( file_id, string("Ошибка отправки сообщения: ") + E.what() );
          OraSession.Commit();
        }
        catch( ... ) {
        	tst();
        	try { OraSession.Rollback(); } catch(...){};
        }
        ProgError( STDLOG, "Exception: %s (file_id=%d)", E.what(), file_id );
        if ( p )
          free( p );
        throw;
      }
    };
    ScanQry.Next();
  }	 // end while
  if ( p )
    free( p );
}

void AstraServiceInterface::authorize( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	xmlNodePtr node = GetNode( "canon_name", reqNode );
	if ( !node )
		throw UserException( "Не найдено canon_name" );
	string client_canon_name = NodeAsString( node );
	ProgTrace( TRACE5, "client_canon_name=%s", client_canon_name.c_str() );
  buildFileData( resNode, client_canon_name );
}

void AstraServiceInterface::commitFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  int file_id = NodeAsInteger( "file_id", reqNode );
  ProgTrace( TRACE5, "commitFileData param file_id=%d", file_id );
  doneFile( file_id );
}

void AstraServiceInterface::errorFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	string msg = NodeAsString( "error", reqNode );
	string file_id = NodeAsString( (char*)PARAM_FILE_ID.c_str(), reqNode );
  ProgError( STDLOG, "AstraService Exception: %s, file_id=%s", msg.c_str(), file_id.c_str() );
  int id;
  if ( StrToInt( file_id.c_str(), id ) != EOF && id > 0 ) {
  	errorFile( id, msg );
  }
}

void AstraServiceInterface::createFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	int point_id = NodeAsInteger( "point_id", reqNode );
	string client_canon_name = NodeAsString( "canon_name", reqNode );
	ProgTrace( TRACE5, "createFileData point_id=%d, client_canon_name=%s", point_id, client_canon_name.c_str() );
//	createCentringFile( point_id, OWN_POINT_ADDR(), client_canon_name );
	tst();
}


void AstraServiceInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};



