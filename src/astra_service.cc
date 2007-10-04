#include <stdlib.h>
#include <string>
#include <map>
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
#include "develop_dbf.h"
#include "flight_cent_dbf.h"
#include "sofi.h"
#include "aodb.h"
#include "stages.h"
#include "cfgproc.h"


using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

const double WAIT_ANSWER_SEC = 30.0;   // ждем ответа 30 секунд
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

void putFile( const string &receiver,
              const string &sender,
              const string &type,
              map<string,string> &params,
              const string &file_data )
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
        	if ( i->first.empty() || i->second.empty() )
        		continue;
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
	TQuery ErrQry(&OraSession);
	ErrQry.SQLText = 
	 "SELECT in_order FROM file_types, files "
	 " WHERE files.id=:id AND files.type=file_types.code AND files.type=:BSMtype";
	ErrQry.CreateVariable( "id", otInteger, id );
	ErrQry.CreateVariable( "BSMtype", otString, "BSM" );
	ErrQry.Execute();
	
  if ( ( !ErrQry.RowCount() || !ErrQry.FieldAsInteger( "in_order" ) ) && deleteFile(id) ) {
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
				else {
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

void buildSaveFileData( xmlNodePtr resNode, const std::string &client_canon_name )
{
	TQuery ScanQry( &OraSession );
	ScanQry.SQLText =
		"SELECT file_queue.id,file_queue.sender,file_queue.receiver,file_queue.type,"
		"       file_queue.status,file_queue.time,system.UTCSYSDATE AS now, "
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
  	int file_id = ScanQry.FieldAsInteger( "id" );
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
        NewTextChild( fileNode, "data", b64_encode( (const char*)p, len ) );
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
}

void buildLoadFileData( xmlNodePtr resNode, const std::string &client_canon_name )
{
	TQuery Qry( &OraSession );
	Qry.SQLText = 
	 "SELECT type,param_name, param_value FROM file_param_sets "
	 " WHERE point_addr=:point_addr AND own_point_addr=:own_point_addr AND pr_send=:send";
	Qry.CreateVariable( "point_addr", otString, client_canon_name );
	Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "send", otInteger, 0 );
	Qry.Execute();
	if ( !Qry.RowCount() )
		return;		
	xmlNodePtr dataNode = NewTextChild( resNode, "data" );
	map<string,string> fileparams;
	while ( !Qry.Eof ) {
		if ( Qry.FieldAsString( "type" ) == FILE_AODB_TYPE ) {
			fileparams[ Qry.FieldAsString( "param_name" ) ] = Qry.FieldAsString( "param_value" );
		}
		Qry.Next();
	}	
	fileparams[ PARAM_FILE_TYPE ] = FILE_AODB_TYPE;
	string region = CityTZRegion( "МОВ" );
	TDateTime d = UTCToLocal( NowUTC(), region );	 
	string filename = string( "SPP" ) + DateTimeToStr( d, "yymmdd" ) + ".txt";
	fileparams[ PARAM_FILE_NAME ] = filename;
	Qry.Clear();
	Qry.SQLText = 
	 "BEGIN "
	 " SELECT rec_no INTO :rec_no FROM aodb_spp_files WHERE filename=:filename AND point_addr=:point_addr;"
	 " EXCEPTION WHEN NO_DATA_FOUND THEN "
	 " BEGIN "
	 "  :rec_no := -1; "
	 "  INSERT INTO aodb_spp_files(filename,point_addr,rec_no) VALUES(:filename,:point_addr,:rec_no); "
	 " END;"
	 "END;";	 
	Qry.CreateVariable( "filename", otString, filename );
	Qry.CreateVariable( "point_addr", otString, client_canon_name );
	Qry.DeclareVariable( "rec_no", otInteger );
	Qry.Execute();
	fileparams[ PARAM_FILE_REC_NO ] = Qry.GetVariableAsString( "rec_no" );
	buildFileParams( dataNode, fileparams );	
}

void AstraServiceInterface::authorize( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
	xmlNodePtr node = GetNode( "canon_name", reqNode );
	if ( !node )
		throw UserException( "param canon_name not found" );
	string client_canon_name = NodeAsString( node );
	string curmode = NodeAsString( "curmode", reqNode );	
	ProgTrace( TRACE5, "client_canon_name=%s, curmode=%s", client_canon_name.c_str(), curmode.c_str() );
	if ( curmode == "OUT" )	
	  buildSaveFileData( resNode, client_canon_name );
	else
		buildLoadFileData( resNode, client_canon_name );
}

void AstraServiceInterface::commitFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  int file_id = NodeAsInteger( "file_id", reqNode );
  ProgTrace( TRACE5, "commitFileData param file_id=%d", file_id );
  doneFile( file_id );
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
	int point_id = NodeAsInteger( "point_id", reqNode );
	string client_canon_name = NodeAsString( "canon_name", reqNode );
	ProgTrace( TRACE5, "createFileData point_id=%d, client_canon_name=%s", point_id, client_canon_name.c_str() );
	map<string,string> params;
	params[ PARAM_WORK_DIR ] = "C:\\Temp";
	string data;
	createCentringFile( point_id, params, data );
}

void CreateCommonFileData( int id, const std::string type, const std::string &airp, const std::string &airline, 
	                         const std::string &flt_no )
{
    TQuery EncodeQry( &OraSession );
    EncodeQry.SQLText =
        "select encoding from file_encoding where "
        "   own_point_addr = :own_point_addr and "
        "   type = :type and "
        "   point_addr=:point_addr AND "
        "   pr_send = 1";
    EncodeQry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
    EncodeQry.CreateVariable( "type", otString, type );
    EncodeQry.DeclareVariable( "point_addr", otString );
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
    map<string,string> params, inparams;
    string file_data;
//    int priority;
    string client_canon_name; 
    bool master_params = false;
    while ( 1 ) {
//        priority = Qry.FieldAsInteger( "priority" );
        if ( client_canon_name.empty() && !Qry.Eof )
          client_canon_name = Qry.FieldAsString( "point_addr" );
/*???        string airp = Qry.FieldAsString( "airp" );
        string airline = Qry.FieldAsString( "airline" );
        string flt_no = Qry.FieldAsString( "flt_no" );
        map<string,int>::iterator r=cname.end();
        for ( r=cname.begin(); r!=cname.end(); r++ ) {
            if ( r->first == client_canon_name )
                break;
        }*/
        if ( Qry.Eof && !client_canon_name.empty() ||
        	   !Qry.Eof && client_canon_name != Qry.FieldAsString( "point_addr" ) ) { /* если нет такого имени */
//        	ProgTrace( TRACE5, "client_canon_name=%s", client_canon_name.c_str() );            
          if ( master_params ) {
            params.clear();
            file_data.clear();
            try {
                params[PARAM_CANON_NAME] = client_canon_name;
                if ( 
                        type == FILE_CENT_TYPE && createCentringFile( id, params, file_data ) || 
                        type == FILE_SOFI_TYPE && createSofiFile( id, inparams, params, file_data ) ||
                        type == FILE_AODB_TYPE && BuildAODBTimes( id, params, file_data ) ||
                        type == FILE_AODB_TYPE && createAODBCheckInInfoFile( id, params, file_data )
                   ) {
                    ProgTrace( TRACE5, "createFiledata type=%s, id=%d", type.c_str(), id );
                    /* теперь в params еще лежит и имя файла */
                    params[ NS_PARAM_AIRP ] = airp;
                    params[ NS_PARAM_AIRLINE ] = airline;
                    params[ NS_PARAM_FLT_NO ] = flt_no;
                    params[ PARAM_TYPE ] = VALUE_TYPE_FILE; // FILE
                    EncodeQry.SetVariable( "point_addr", client_canon_name );
                    EncodeQry.Execute();
                    if ( !EncodeQry.Eof )              
                        try {
                            file_data = ConvertCodePage( EncodeQry.FieldAsString( "encoding" ), "CP866", file_data );
                        } catch(EConvertError &E) {
                            ProgError(STDLOG, E.what());
                            throw UserException("Ошибка конвертации в %s", EncodeQry.FieldAsString( "encoding" ));
                        }
                    putFile( client_canon_name, OWN_POINT_ADDR(), type, params, file_data );
                }
            }
            /* ну не получилось сформировать файл, остальные файлы имеют тоже право попробовать сформироваться */
            catch( std::exception &e) {
                ///try OraSession.RollBack(); catch(...){};//!!!
                ProgError(STDLOG, e.what());
            }
            catch(...) {
                ///try OraSession.RollBack(); catch(...){}; //!!!
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
	 " FROM points, file_param_sets, aodb_points "
	 " WHERE file_param_sets.type=:type AND pr_send=1 AND own_point_addr=:own_point_addr AND "
	 "       points.point_id=aodb_points.point_id AND "
	 "       points.act_out IS NULL AND points.pr_del=0 AND "
	 "       gtimer.get_stage(points.point_id,1) BETWEEN :stage1 AND :stage2 AND "
	 "       ( file_param_sets.airp IS NULL OR file_param_sets.airp=points.airp ) AND "
	 "       ( file_param_sets.airline IS NULL OR file_param_sets.airline=points.airline ) AND "
	 "       ( file_param_sets.flt_no IS NULL OR file_param_sets.flt_no=points.flt_no ) ";
	Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "type", otString, FILE_AODB_TYPE );
	Qry.CreateVariable( "stage1", otInteger, sOpenCheckIn );
	Qry.CreateVariable( "stage2", otInteger, sCloseBoarding );
	Qry.Execute();
	while ( !Qry.Eof ) {
		CreateCommonFileData( Qry.FieldAsInteger( "point_id" ), FILE_AODB_TYPE, Qry.FieldAsString( "airp" ),
  	                      Qry.FieldAsString( "airline" ), Qry.FieldAsString( "flt_no" ) );	
		Qry.Next();
	}
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
	file_data = b64_decode( file_data.c_str(), file_data.size() ); 
	TQuery Qry( &OraSession );
  TQuery EncodeQry( &OraSession );
  EncodeQry.SQLText =
      "select encoding from file_encoding where "
      "   own_point_addr = :own_point_addr and "
      "   type = 'AODB' and "
      "   point_addr=:point_addr AND "
      "   pr_send = 0";
  EncodeQry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
  EncodeQry.CreateVariable( "point_addr", otString, fileparams[ "canon_name" ] );
  EncodeQry.Execute();    	

  if ( !EncodeQry.Eof )
      try {
          file_data = ConvertCodePage( "CP866", EncodeQry.FieldAsString( "encoding" ), file_data );
      } catch(EConvertError &E) {
          ProgError(STDLOG, E.what());
          throw UserException("Ошибка конвертации из %s", EncodeQry.FieldAsString( "encoding" ));
      }

  ParseAndSaveSPP( fileparams[ PARAM_FILE_NAME ], fileparams[ "canon_name" ], file_data );
}

void AstraServiceInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
