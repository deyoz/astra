#include <string>
#include <map>
#include <set>
#include <algorithm>

#include "file_queue.h"
#include "exceptions.h"
#include "basic.h"
#include "astra_utils.h"
#include "stl_utils.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

void TFilterQueue::createQuery( TQuery &Qry ) const
{
  Qry.Clear();
  string sql;
  sql = "SELECT file_queue.id,"
         "      file_queue.time,"
         "      files.data, "
         "      files.time AS puttime ";
  if ( pr_first_order ) { //важен порядок отправки - вычисляем самый приоритетный файл для отправки
    sql += ", file_queue.type, file_types.in_order "
           " FROM file_queue, files, file_types ";
  }
  else {
    sql += " FROM file_queue, files ";
  }
  sql += "WHERE file_queue.sender=:sender AND "
         "      file_queue.receiver=:receiver AND "
         "      file_queue.id=files.id ";
  if ( !type.empty() ) {
    sql += "AND file_queue.type=:type ";
    Qry.CreateVariable( "type", otString, type );
  }
  if ( last_time != ASTRA::NoExists ) {
    sql += " AND file_queue.time < :last_time ";
    Qry.CreateVariable( "last_time", otDate, last_time );
  }
  if ( first_id != ASTRA::NoExists ) {
    sql += " AND file_queue.id != :first_id ";
    Qry.CreateVariable( "first_id", otInteger, first_id );
  }
  if ( pr_first_order ) { //важен порядок отправки
    sql += " AND file_queue.type=file_types.code ";
    sql += " ORDER BY DECODE(in_order,1,files.time,file_queue.time), file_queue.id";
  }
  else {
    sql += " ORDER BY file_queue.time, file_queue.id";
  }
  Qry.SQLText = sql;
  Qry.CreateVariable( "sender", otString, OWN_POINT_ADDR() );
  Qry.CreateVariable( "receiver", otString, receiver );
  ProgTrace( TRACE5, "TFilterQueue::createQuery: pr_first_order=%d, type=%s, last_time=%f, first_id=%d",
             pr_first_order, type.c_str(), last_time, first_id );
  ProgTrace( TRACE5, "TFilterQueue::createQuery: sql=%s", sql.c_str() );
}

std::string TFileQueue::getstatus( int id )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT status FROM file_queue "
    " WHERE id=:id";
  Qry.CreateVariable( "id", otInteger, id );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw Exception( "TFileQueue::getstatus(%d): file not found", id );
  }
  std::string res = Qry.FieldAsString( "status" );
  return res;
}

std::string TFileQueue::gettype( int id )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT type FROM file_queue "
    " WHERE id=:id";
  Qry.CreateVariable( "id", otInteger, id );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw Exception( "TFileQueue::getstatus(%d): file not found", id );
  }
  std::string res = Qry.FieldAsString( "type" );
  return res;
}

BASIC::TDateTime TFileQueue::getwait_time( int id )
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT time AS puttime FROM files "
    " WHERE files.id=:id";
  Qry.CreateVariable( "id", otInteger, id );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw Exception( "TFileQueue::getstatus(%d): file not found", id );
  }
  BASIC::TDateTime res = NowUTC() - Qry.FieldAsDateTime( "puttime" );
  return res;
}

bool TFileQueue::getparam_value( int id, const std::string &param_name, std::string &param_value )
{
  param_value.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT value FROM file_params WHERE id=:id AND name=:name";
  Qry.CreateVariable( "id", otInteger, id );
  Qry.CreateVariable( "name", otString, param_name );
  Qry.Execute();
  if ( Qry.Eof ) {
    return false;
  }
  param_value = Qry.FieldAsString( "value" );
  return true;
}

void TFileQueue::getparams( int id, std::map<std::string, std::string> &params )
{
  params.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT name,value FROM file_params WHERE id=:id";
  Qry.CreateVariable( "id", otInteger, id );
  Qry.Execute();
  for( ; !Qry.Eof; Qry.Next() ) {
    params[ string( Qry.FieldAsString( "name" ) ) ] = Qry.FieldAsString( "value" );
  }
};

void TFileQueue::add_sets_params( const std::string &airp,
                                  const std::string &airline,
                                  const std::string &flt_no,
                                  const std::string &client_canon_name,
                                  const std::string &type,
                                  bool send,
                                  std::map<std::string,std::string> &params)
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
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
  Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
  Qry.CreateVariable( "point_addr", otString, client_canon_name );
  Qry.CreateVariable( "type", otString, type );
  Qry.CreateVariable( "send", otInteger, send );

  ProgTrace( TRACE5, "airp: %s, airline %s, flt_no %s, point_addr %s, type %s, send %d",
             airp.c_str(),
             airline.c_str(),
             flt_no.c_str(),
             client_canon_name.c_str(),
             type.c_str(),
             send );
  Qry.Execute();
  int priority = -1;
  for ( ;!Qry.Eof; Qry.Next() ) {
    if ( priority < 0 ) {
      priority = Qry.FieldAsInteger( "priority" );
    }
    if ( priority != Qry.FieldAsInteger( "priority" ) ) {
      break;
    }
    params[ Qry.FieldAsString( "param_name" ) ] = Qry.FieldAsString( "param_value" );
  }
}

void TFileQueue::get( const TFilterQueue &filter,
                      std::vector<TQueueItem> &items )
{
  items.clear();
  pr_last_file = true;
  TQuery Qry( &OraSession );
  filter.createQuery( Qry );
  Qry.Execute();
  BASIC::TDateTime UTCSysdate = NowUTC();
  std::set<std::string> file_keys;
  for ( ; !Qry.Eof; Qry.Next() ) {
    if ( filter.pr_first_order ) { //нужна последовательность посылки данных
      string status = getstatus( Qry.FieldAsInteger( "id" ) );
      bool pr_need_send = ( status ==  "PUT" || //не отправлялся
                            ( status == "SEND" && Qry.FieldAsDateTime( "time" ) + filter.timeout_sec/(60.0*60.0*24.0) < UTCSysdate ) );  //время ответа истекло
      if ( Qry.FieldAsInteger( "in_order" ) != 0 ) { //последовательная отправка
        ProgTrace( TRACE5, "status=%s, id=%d, pr_need_send=%d", status.c_str(), Qry.FieldAsInteger( "id" ), pr_need_send );
        string in_order_key = string( Qry.FieldAsString( "type" ) ) + filter.receiver;
        if ( pr_need_send ) { //файл нужно отправлять
          //проверим есть ли в очереди
          if ( file_keys.find( in_order_key ) != file_keys.end() ) { //такой тип + отправитель файла ждет ответа
            tst();
            pr_need_send = false;
          }
        }
        else { //не надо отправлять - ждем ответа
          /* для улучшения производительности будем считать, что для одного receiver будет одна очередь.
             Т.е. если в очереди есть сообщение, которые ждет факта доставки, то все другие сообщения, даже с
             другим type, будут ждать нашего сообщения, пока оно не доставится */
          file_keys.insert( in_order_key );
          break; //поэтому ждем
        }
      }
      if ( !pr_need_send ) { //файл не нужно отправлять - переход к следующему
        tst();
        continue;
      }
    }
    //create TQueueItem
    TQueueItem item;
    item.id = Qry.FieldAsInteger( "id" );
    item.receiver = filter.receiver;
    item.type = filter.pr_first_order?Qry.FieldAsString( "type" ):filter.type;
    item.time = Qry.FieldAsDateTime( "time" );
    item.wait_time =UTCSysdate-Qry.FieldAsDateTime( "puttime" );
    
    if ( filter.pr_first_order  &&
         filter.first_id != ASTRA::NoExists ) {
      items.push_back( item );
      return;
    }
    
    int len = Qry.GetSizeLongField( "data" );
    if ( pdata )
    	pdata = (char*)realloc( pdata, len );
    else
      pdata = (char*)malloc( len );
    if ( !pdata )
      throw Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
    Qry.FieldAsLong( "data", pdata );
    item.data = string( pdata, len );
    getparams( item.id, item.params );
    items.push_back( item );
    if ( filter.pr_first_order ) { //нужна последовательность посылки данных - выбираем только первый + признак того, что есть еще в очереди
      //есть ли еще в очереди файлы стакими же параметрами receiver_type и определяем признак pr_last_file
      TFilterQueue nfilter( filter );
      nfilter.first_id = item.id;
      nfilter.receiver = item.receiver;
      nfilter.type = item.type;
      tst();
      std::vector<TQueueItem> vitems;
      get( nfilter, vitems );
      pr_last_file = vitems.empty();
      break;
    }
  }
  tst();
}

bool TFileQueue::in_order( int id )
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT in_order FROM file_types, files "
	 " WHERE files.id=:id AND files.type=file_types.code";
	Qry.CreateVariable( "id", otInteger, id );
	Qry.Execute();
  return ( !Qry.Eof && Qry.FieldAsInteger( "in_order" ) );
}

bool TFileQueue::in_order( const std::string &type )
{
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT in_order FROM file_types "
	 " WHERE code=:type";
	Qry.CreateVariable( "type", otString, type );
	Qry.Execute();
  return ( !Qry.Eof && Qry.FieldAsInteger( "in_order" ) );
}

std::string TFileQueue::getEncoding( const std::string &type,
                                     const std::string &point_addr,
                                     bool pr_send )
{
	string res;
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT encoding "
    " FROM file_encoding "
    " WHERE own_point_addr = :own_point_addr and "
    "       type = :type AND "
    "       point_addr=:point_addr AND "
    "       pr_send = :pr_send";
  Qry.CreateVariable( "own_point_addr", otString, OWN_POINT_ADDR() );
  Qry.CreateVariable( "type", otString, type );
  Qry.CreateVariable( "point_addr", otString, point_addr );
  Qry.CreateVariable( "pr_send", otInteger, pr_send );
  Qry.Execute();
  if ( !Qry.Eof ) {
  	res = Qry.FieldAsString( "encoding" );
  }
  return res;
}

bool TFileQueue::errorFile( int id, const std::string &msg )
{
  if ( !in_order( id ) && deleteFile( id ) ) {
	  TQuery Qry(&OraSession);
    Qry.SQLText =
     "BEGIN "
     " UPDATE files SET error=:error,time=system.UTCSYSDATE WHERE id=:id; "
     " INSERT INTO file_error(id,msg) VALUES(:id,:msg); "
     "END;";
    Qry.CreateVariable( "error", otString, "ERR" );
    Qry.CreateVariable( "id", otInteger, id );
    Qry.CreateVariable( "msg", otString, msg );
    Qry.Execute();
    bool res = ( Qry.RowsProcessed()>0 );
    if ( res ) {
      ProgTrace( TRACE5, "errorFile id=%d", id );
    }
    return res;
  }
  else
    return false;
};

bool TFileQueue::sendFile( int id )
{
	TQuery Qry(&OraSession);
  Qry.SQLText =
    "UPDATE file_queue SET status='SEND', time=system.UTCSYSDATE WHERE id= :id";
  Qry.CreateVariable("id",otInteger,id);
  Qry.Execute();
  bool res = ( Qry.RowsProcessed()>0 );
  if ( res ) {
    ProgTrace( TRACE5, "sendFile id=%d", id );
  }
  return res;
}

bool TFileQueue::doneFile( int id )
{
  if ( deleteFile( id ) ) {
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "UPDATE files SET time=system.UTCSYSDATE WHERE id=:id";
    Qry.CreateVariable("id",otInteger,id);
    Qry.Execute();
    bool res = ( Qry.RowsProcessed()>0 );
    if ( res ) {
      ProgTrace( TRACE5, "doneFile id=%d", id );
    }
    return res;
  }
  else
  	return false;
}

bool TFileQueue::deleteFile( int id )
{
    TQuery Qry(&OraSession);
    Qry.SQLText=
      " BEGIN "
      " DELETE file_queue WHERE id= :id; "
      "END; ";
    Qry.CreateVariable("id",otInteger,id);
    Qry.Execute();
    bool res = ( Qry.RowsProcessed()>0 );
    if ( res ) {
      ProgTrace( TRACE5, "deleteFile id=%d", id );
    }
    return res;
};

int TFileQueue::putFile( const std::string &receiver,
                         const std::string &sender,
                         const std::string &type,
                         const std::map<std::string,std::string> &params,
                         const std::string &file_data )
{
	int file_id = ASTRA::NoExists;
  try {
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT tlgs_id.nextval id FROM dual";
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
  ProgTrace( TRACE5, "putFile id=%d", file_id );
  return file_id;
};

int file_by_id(int argc,char **argv)
{
    if(argc < 2 or argc > 2) {
        cout << "only file id allowed" << endl;
        return 1;
    }

    int id = ASTRA::NoExists;
    try {
        id = ToInt(argv[1]);
    } catch(Exception &e) {
        cout << e.what() << endl;
        return 1;
    }

    TQuery Qry(&OraSession);
    Qry.SQLText = "select data from files where id = :id";
    Qry.CreateVariable("id", otInteger, id);
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) {
        int len = Qry.GetSizeLongField( "data" );
        char *pdata = (char*)malloc( len );
        if ( !pdata )
            throw Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
        Qry.FieldAsLong( "data", pdata );
        cout << string(pdata, len).c_str();
        free( pdata );
    }
    return 1;
}
