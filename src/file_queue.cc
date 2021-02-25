#include <string>
#include <map>
#include <set>
#include <algorithm>

#include "file_queue.h"
#include "exceptions.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "qrys.h"
#include "astra_dates.h"
#include "db_tquery.h"
#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/dbcpp_session.h>
#include <serverlib/pg_cursctl.h>
#include "hooked_session.h"
#include "PgOraConfig.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

static std::string getFileDataPg(int id)
{
  std::string data;
  PgCpp::BinaryDefHelper<std::string> defdata{data};
  auto cur = DbCpp::mainPgReadOnlySession(STDLOG).createPgCursor(
      STDLOG,
      "select data from files where id = :id",
      true);
  cur.bind(":id", id).def(defdata).EXfet();
  return data;
}

static std::string getFileDataOra(int id)
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "select data from files where id = :id";
  Qry.CreateVariable("id", otInteger, id);
  Qry.Execute();

  std::string file_data;
  if (!Qry.Eof) {
      int len = Qry.GetSizeLongField( "data" );
      char *pdata = (char*)malloc( len );
      if ( !pdata )
          throw Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
      Qry.FieldAsLong( "data", pdata );
      file_data = string(pdata, len);
      free( pdata );
  }
  return file_data;
}

std::string TFileQueue::getFileData(int id)
{
  if(PgOra::supportsPg("FILES")) {
    return getFileDataPg(id);
  } else {
    return getFileDataOra(id);
  }
}

static std::vector<TQueueItem> getFileQueue(const TFilterQueue &filter)
{
  DB::TQuery Qry( PgOra::getROSession("FILE_QUEUE") );

  string sql;
  sql = "SELECT id, time, type, status "
        " FROM file_queue "
        "WHERE sender=:sender AND "
        "      receiver=:receiver ";

  if ( !filter.type.empty() ) {
    sql += " AND type=:type ";
    Qry.CreateVariable( "type", otString, filter.type );
  }
  if ( filter.last_time != ASTRA::NoExists ) {
    sql += " AND time < :last_time ";
    Qry.CreateVariable( "last_time", otDate, filter.last_time );
  }
  if ( filter.first_id != ASTRA::NoExists ) {
    sql += " AND id != :first_id ";
    Qry.CreateVariable( "first_id", otInteger, filter.first_id );
  }
  sql += " ORDER BY time, id";

  Qry.SQLText = sql;
  Qry.CreateVariable( "sender", otString, OWN_POINT_ADDR() );
  Qry.CreateVariable( "receiver", otString, filter.receiver );
  ProgTrace( TRACE5, "TFilterQueue::getFileQueue: pr_first_order=%d, type=%s, last_time=%f, first_id=%d",
             filter.pr_first_order, filter.type.c_str(), filter.last_time, filter.first_id );
  ProgTrace( TRACE5, "TFilterQueue::getFileQueue: sql=%s", sql.c_str() );
  Qry.Execute();

  std::vector<TQueueItem> queue;
  for (; !Qry.Eof; Qry.Next()) {
    TQueueItem item;
    item.id = Qry.FieldAsInteger("id");
    item.receiver = filter.receiver;
    item.type = Qry.FieldAsString("type");
    item.time = Qry.FieldAsDateTime("time");
    const auto wait_put = TFileQueue::getwait_time(item.id);
    item.wait_time = wait_put.first;
    item.put_time = wait_put.second;
    item.status = Qry.FieldAsString("status");
    item.in_order = TFileQueue::in_order(item.type);
    queue.push_back(item);
  }

  // ORDER BY DECODE(in_order,1,files.time,file_queue.time), file_queue.id"
  LogTrace(TRACE3) << "queue size: " << queue.size();
  return algo::sort(queue, [filter](const TQueueItem &i1, const TQueueItem &i2) {
    if(filter.pr_first_order) {
      return i1.put_time <= i2.put_time && i1.id < i2.id;
    } else {
      return i1.time <= i2.time && i1.id < i2.id;
    }
  });
}

std::string TFileQueue::getstatus( int id )
{
  DB::TQuery Qry(PgOra::getROSession("FILE_QUEUE"));
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
  DB::TQuery Qry(PgOra::getROSession("FILE_QUEUE"));
  Qry.SQLText =
    "SELECT type FROM file_queue "
    " WHERE id=:id";
  Qry.CreateVariable( "id", otInteger, id );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw Exception( "TFileQueue::gettype(%d): file not found", id );
  }
  std::string res = Qry.FieldAsString( "type" );
  return res;
}

std::pair<TDateTime, TDateTime> TFileQueue::getwait_time(int id)
{
  DB::TQuery Qry( PgOra::getROSession("FILES") );
  Qry.SQLText =
    "SELECT time AS puttime FROM files "
    " WHERE files.id=:id";
  Qry.CreateVariable( "id", otInteger, id );
  Qry.Execute();
  if ( Qry.Eof ) {
    throw Exception("TFileQueue::getwait_time(%d): file not found", id);
  }
  const auto puttime = Qry.FieldAsDateTime( "puttime" );
  TDateTime waittime = NowUTC() - puttime;
  return std::make_pair(waittime, puttime);
}

bool TFileQueue::getparam_value( int id, const std::string &param_name, std::string &param_value )
{
  param_value.clear();
  DB::TQuery Qry( PgOra::getROSession("FILE_PARAMS") );
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
  DB::TQuery Qry( PgOra::getROSession("FILE_PARAMS") );
  Qry.SQLText = "SELECT name, value FROM file_params WHERE id=:id";
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
                      std::vector<TQueueItem> &items_out )
{
  items_out.clear();
  pr_last_file = true;
  const auto items = getFileQueue(filter);

  TDateTime UTCSysdate = NowUTC();
  std::set<std::string> file_keys;
  for (const auto &item: items) {
    ProgTrace(TRACE5, "item.status=%s, id=%d, filter.pr_first_order=%d", item.status.c_str(), item.id, filter.pr_first_order);
    if ( filter.pr_first_order ) { //нужна последовательность посылки данных
      bool pr_need_send = ( item.status ==  "PUT" || //не отправлялся
                            ( item.status == "SEND" && item.time + filter.timeout_sec/(60.0*60.0*24.0) < UTCSysdate ) );  //время ответа истекло
      if ( item.in_order != 0 ) { //последовательная отправка
        ProgTrace( TRACE5, "item.status=%s, id=%d, pr_need_send=%d", item.status.c_str(), item.id, pr_need_send );
        string in_order_key = item.type + filter.receiver;
        if ( pr_need_send ) { //файл нужно отправлять
          //проверим есть ли в очереди
          if ( file_keys.find( in_order_key ) != file_keys.end() ) { //такой тип + отправитель файла ждет ответа
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
        continue;
      }
    }
    //create TQueueItem
    TQueueItem item_out = item;

    if ( filter.pr_first_order  &&
         filter.first_id != ASTRA::NoExists ) {
      items_out.push_back(item_out);
      return;
    }
    item_out.data = getFileData(item.id);
    getparams( item.id, item_out.params );
    items_out.push_back(item_out);

    if ( filter.pr_first_order ) { //нужна последовательность посылки данных - выбираем только первый + признак того, что есть еще в очереди
      //есть ли еще в очереди файлы стакими же параметрами receiver_type и определяем признак pr_last_file
      TFilterQueue nfilter( filter );
      nfilter.first_id = item.id;
      nfilter.receiver = item.receiver;
      nfilter.type = item.type;
      std::vector<TQueueItem> vitems;
      get( nfilter, vitems );
      pr_last_file = vitems.empty();
      break;
    }
  }
}

bool TFileQueue::in_order( int id )
{
	DB::TQuery Qry(PgOra::getROSession("FILES"));
	Qry.SQLText =
	 "SELECT type FROM files "
	 " WHERE id=:id";
	Qry.CreateVariable( "id", otInteger, id );
	Qry.Execute();
  return ( !Qry.Eof && in_order(Qry.FieldAsString( "type" )) );
}

bool TFileQueue::in_order( const std::string &type )
{
	DB::TQuery Qry(PgOra::getROSession("FILE_TYPES"));
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
  DB::TQuery Qry( PgOra::getROSession("FILE_ENCODING") );
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
    const auto systime = BoostToDateTime(Dates::second_clock::universal_time());
	  DB::TQuery Qry(PgOra::getRWSession("FILES"));
    Qry.SQLText =
     "UPDATE files SET error=:error,time=:time WHERE id=:id ";
    Qry.CreateVariable( "error", otString, "ERR" );
    Qry.CreateVariable( "id", otInteger, id );
    Qry.CreateVariable( "msg", otString, msg );
    Qry.CreateVariable( "time", otDate, systime );
    Qry.Execute();
    bool res = ( Qry.RowsProcessed()>0 );
    if ( res ) {
      ProgTrace( TRACE5, "errorFile id=%d", id );
    }
    Qry.SQLText = "INSERT INTO file_error(id,msg) VALUES(:id,:msg) ";
    Qry.Execute();

    return res;
  }
  else
    return false;
};

bool TFileQueue::sendFile( int id )
{
  const auto systime = BoostToDateTime(Dates::second_clock::universal_time());
  DB::TQuery Qry(PgOra::getRWSession("FILE_QUEUE"));
  Qry.SQLText =
    "UPDATE file_queue SET status='SEND', time=:time WHERE id= :id";
  Qry.CreateVariable("id",otInteger,id);
  Qry.CreateVariable("time", otDate, systime);
  Qry.Execute();
  bool res = ( Qry.RowsProcessed()>0 );
  if ( res ) {
    ProgTrace( TRACE5, "sendFile id=%d", id );
  }
  return res;
}

bool TFileQueue::unsendFile( int id )
{
  const auto systime = BoostToDateTime(Dates::second_clock::universal_time());
  DB::TQuery Qry(PgOra::getRWSession("FILE_QUEUE"));
  Qry.SQLText = "UPDATE file_queue SET status='PUT', time=:time WHERE id= :id";
  Qry.CreateVariable("id", otInteger, id);
  Qry.CreateVariable("time", otDate, systime);
  Qry.Execute();
  bool res = (Qry.RowsProcessed() > 0);
  if (res)
  {
    ProgTrace(TRACE5, "unSendFile id=%d", id);
  }
  return res;
}

bool TFileQueue::doneFile( int id )
{
  if ( deleteFile( id ) ) {
    const auto systime = BoostToDateTime(Dates::second_clock::universal_time());
    DB::TQuery Qry(PgOra::getRWSession("FILES"));
    Qry.SQLText =
      "UPDATE files SET time=:time WHERE id=:id";
    Qry.CreateVariable("id",otInteger,id);
    Qry.CreateVariable("time", otDate, systime);
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
    auto cur = make_db_curs("DELETE from file_queue WHERE id= :id", PgOra::getRWSession("FILE_QUEUE"));
    cur.bind(":id", id).exec();
    bool res = ( cur.rowcount() > 0 );
    if ( res ) {
      ProgTrace( TRACE5, "deleteFile id=%d", id );
    }
    return res;
};

static void insert_file_queue(int file_id, const std::string &sender, const std::string &receiver,
                              const std::string &type,
                              const Dates::DateTime_t &utc)
{
  auto cur = make_db_curs(
      "INSERT INTO "
      "file_queue(id,sender,receiver,type,status,time) "
      "VALUES"
      "(:id, :sender, :receiver, :type, 'PUT', :utc_dt)", PgOra::getRWSession("FILE_QUEUE"));

  cur.
    bind(":id", file_id).
    bind(":sender", sender).
    bind(":receiver", receiver).
    bind(":type", type).
    bind(":utc_dt", utc).
    exec();
}

static void insert_files_pg(int file_id, const std::string &sender, const std::string &receiver,
                            const std::string &type,
                            const Dates::DateTime_t &utc,
                            const std::string &file_data)
{
    auto cur = DbCpp::mainPgManagedSession(STDLOG).createPgCursor(STDLOG,
      "INSERT INTO "
      "files(id,sender,receiver,type,time,data,error) "
      "VALUES"
      "(:id,:sender,:receiver,:type,:utc_dt,:data,NULL)", true);

    cur.
      bind(":id", file_id).
      bind(":sender", sender).
      bind(":receiver", receiver).
      bind(":type", type).
      bind(":utc_dt", utc).
      bind(":data", PgCpp::BinaryBindHelper({file_data.data(), file_data.size()})).
      exec();
}

static void insert_files_ora(int file_id, const std::string &sender, const std::string &receiver,
                            const std::string &type,
                            const Dates::DateTime_t &utc,
                            const std::string &file_data)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "INSERT INTO "
    "files(id,sender,receiver,type,time,data,error) "
    "VALUES"
    "(:id,:sender,:receiver,"
    ":type,system.UTCSYSDATE,:data,NULL)";
  Qry.CreateVariable("id", otInteger, file_id);
  Qry.CreateVariable("sender", otString, sender);
  Qry.CreateVariable("receiver", otString, receiver);
  Qry.CreateVariable("type", otString, type);
  Qry.DeclareVariable("data",otLongRaw);
  Qry.SetLongVariable("data",(void*)file_data.c_str(),file_data.size());
  Qry.Execute();
  Qry.Close();
}

static void insert_files(int file_id, const std::string &sender, const std::string &receiver,
                            const std::string &type,
                            const Dates::DateTime_t &utc,
                            const std::string &file_data)
{
  if(PgOra::supportsPg("FILES")) {
    insert_files_pg(file_id, sender, receiver, type, utc, file_data);
  } else {
    insert_files_ora(file_id, sender, receiver, type, utc, file_data);
  }
}

static void insert_file_params(int file_id, const std::map<std::string, std::string> &params)
{
  for(map<string,string>::const_iterator i=params.begin();i!=params.end();++i)
  {
  	if ( i->first.empty() )
   		continue;
    auto cur = make_db_curs(
        "INSERT INTO file_params(id,name,value) "
        "VALUES(:id,:name,:value)",
        PgOra::getRWSession("FILE_PARAMS"));

    cur.bind(":id", file_id).bind(":name", i->first).bind(":value", i->second).exec();
  };
}

int TFileQueue::putFile( const std::string &receiver,
                         const std::string &sender,
                         const std::string &type,
                         const std::map<std::string,std::string> &params,
                         const std::string &file_data )
{
  const auto file_id = PgOra::getSeqNextVal("TLGS_ID");
  const auto utc = Dates::second_clock::universal_time();
  try
  {
    insert_file_queue(file_id, sender, receiver, type, utc);
    insert_files(file_id, sender, receiver, type, utc, file_data);
    insert_file_params(file_id, params);
  }
  catch( std::exception &e)
  {
  	try {deleteFile( file_id );} catch(...){};
    ProgError(STDLOG, "%s", e.what());
    throw;
  }
  catch(...)
  {
  	try {deleteFile( file_id );} catch(...){};
    ProgError(STDLOG, "putFile: Unknown error while trying to put file");
    throw;
  };
  LogTrace( TRACE5 ) << "putFile id=" << file_id;
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

    std::string data = TFileQueue::getFileData(id);
    cout << data;
    return 1;
}

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef XP_TESTING

#include "xp_testing.h"
START_TEST(check_file_queue)
{
  const auto utc = Dates::second_clock::universal_time();
  insert_file_queue(1, "snd", "rcv", "FIDS", utc);
  insert_files(1, "snd", "rcv", "FIDS", utc, std::string(10 * 1024, 'F'));
  fail_unless(TFileQueue::deleteFile(10) == false);
  fail_unless(TFileQueue::getstatus(1) == "PUT");
  fail_unless(TFileQueue::gettype(1) == "FIDS");
  fail_unless(TFileQueue::in_order("FIDS") == true);
  fail_unless(TFileQueue::getwait_time(1).first < 1);
  fail_unless(TFileQueue::in_order(1) == true);
  TFileQueue::errorFile(1, "ERROR !!!");
  fail_unless(TFileQueue::sendFile(1) == true);
  fail_unless(TFileQueue::getstatus(1) == "SEND");
  fail_unless(TFileQueue::unsendFile(1) == true);
  fail_unless(TFileQueue::getstatus(1) == "PUT");
  fail_unless(TFileQueue::deleteFile(1) == true);

  fail_unless(TFileQueue::doneFile(1) == false);
  insert_file_queue(2, "snd", "rcv", "FIDS", utc);
  insert_files(2, "snd", "rcv", "FIDS", utc, std::string(10 * 1024, 'F'));
  fail_unless(TFileQueue::doneFile(2) == true);

  fail_unless(TFileQueue::sendFile(1) == false);
}
END_TEST;

START_TEST(check_file_encoding)
{
  fail_unless(TFileQueue::getEncoding("FIDS", "MOWDT") == "");
}
END_TEST;

START_TEST(check_put_file)
{
  std::map<std::string, std::string> params;
  params["param"] = "pam-pam";
  const auto file_id = TFileQueue::putFile("rcv", "snd", "file", params,
                                           std::string(10 * 1024, 'F'));

  std::string value;
  std::map<std::string, std::string> params_out;
  fail_unless(TFileQueue::getparam_value(file_id, "param", value) == true);
  TFileQueue::getparams(file_id, params_out);
  fail_unless(params_out == params);
  fail_unless(value == "pam-pam");

}
END_TEST;

static void sleep_filequeue(int id1, int id2, int id3)
{
  DB::TQuery Qry(PgOra::getRWSession("FILE_QUEUE"));
  if (PgOra::supportsPg("FILE_QUEUE"))
  {
    Qry.SQLText = "update file_queue set time = time - interval '2 seconds' where id in (:id1, :id2, :id3)";
    Qry.CreateVariable("id1", otInteger, id1);
    Qry.CreateVariable("id2", otInteger, id2);
    Qry.CreateVariable("id3", otInteger, id3);
    Qry.Execute();
    Qry.SQLText = "update files set time = time - interval '2 seconds' where id in (:id1, :id2, :id3)";
    Qry.Execute();
  }
  else
  {
    Qry.SQLText = "update file_queue set time = time - 2/3600 where id in (:id1, :id2, :id3)";
    Qry.CreateVariable("id1", otInteger, id1);
    Qry.CreateVariable("id2", otInteger, id2);
    Qry.CreateVariable("id3", otInteger, id3);
    Qry.Execute();
    Qry.SQLText = "update files set time = time - 2/3600 where id in (:id1, :id2, :id3)";
    Qry.Execute();
  }
}

START_TEST(check_old_tests_file_queue)
{
   int res = 0;
   DB::TQuery Qry(PgOra::getRWSession("FILE_QUEUE"));
   Qry.SQLText =
     "DELETE from file_queue";
   Qry.Execute();

   std::map<std::string,std::string> params;
   params[ "PARAM_FILE_TYPE" ] = "AODB";
   params[ "PARAM_CANON_NAME" ] = "RASTRV";
   params[ "PARAM_IN_ORDER" ] = "TRUE";
   string sender = OWN_POINT_ADDR();
   string type = string("AODBO");
   std::string receiver = string("RASTRV");
   int id1 = TFileQueue::putFile( receiver,
                                  sender,
                                  type,
                                  params,
                                  string("TEST1") );
   int id2 = TFileQueue::putFile( receiver,
                                  sender,
                                  type,
                                  params,
                                  string("TEST2") );
   int id3 = TFileQueue::putFile( receiver,
                                  sender,
                                  type,
                                  params,
                                  string("TEST3") );
   TFileQueue file_queue;
   file_queue.get( TFilterQueue(receiver, 5) );
   tst();
   res++;
   if ( file_queue.empty() ||
        file_queue.begin()->id != id1||
        file_queue.getstatus(id1) != string( "PUT" ) ||
        !file_queue.in_order( id1 ) ||
        !TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.isLastFile=%d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, isLastFile=%d",
                  res,
                  file_queue.begin()->id,
                  id1,
                  file_queue.getstatus(id1).c_str(),
                  id1,
                  file_queue.in_order( id1 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  file_queue.isLastFile() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   tst();
   res++;
   TFileQueue::sendFile( id1 );
   file_queue.get( TFilterQueue(receiver, 5) );
   if ( !file_queue.empty() ) {
     ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, isLastFile=%d",
                res,
                file_queue.begin()->id,
                id1,
                file_queue.getstatus(id1).c_str(),
                id1,
                file_queue.in_order( id1 ),
                type.c_str(),
                TFileQueue::in_order( type ),
                file_queue.size(),
                file_queue.isLastFile() );
     fail_unless(false, "file_queue error. see log");
   }
   res++;
   tst();


   sleep_filequeue(id1, id2, id3);

   file_queue.get( TFilterQueue(receiver, 1) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id1||
        file_queue.getstatus(id1) != string( "SEND" ) ||
        !file_queue.in_order( id1 ) ||
        !TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.isLastFile=%d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, isLastFile=%d",
                  res,
                  file_queue.begin()->id,
                  id1,
                  file_queue.getstatus(id1).c_str(),
                  id1,
                  file_queue.in_order( id1 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  file_queue.isLastFile() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   res++;
   tst();
   TFileQueue::sendFile( id2 );
   file_queue.get( TFilterQueue(receiver, 1) );

   ProgError(STDLOG, "error%d: %d, file_queue.isLastFile=%d", res, file_queue.empty(), file_queue.isLastFile());
   if (!file_queue.empty())
   {
     ProgError(STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, isLastFile=%d",
               res,
               file_queue.begin()->id,
               id1,
               file_queue.getstatus(id1).c_str(),
               id1,
               file_queue.in_order(id1),
               type.c_str(),
               TFileQueue::in_order(type),
               file_queue.size(),
               file_queue.isLastFile());
   }

   fail_unless(!file_queue.empty());
   fail_unless(file_queue.begin()->id == id1);
   fail_unless(file_queue.getstatus(id1) == string( "SEND" ));
   fail_unless(file_queue.in_order( id1 ) == 1);
   fail_unless(TFileQueue::in_order( type ) == 1);
   fail_unless(file_queue.size() == 1);
   fail_unless(file_queue.isLastFile() == true);
   res++;
   sleep_filequeue(id1, id2, id3);
   tst();
   file_queue.get( TFilterQueue(receiver, 1) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id1||
        file_queue.getstatus(id1) != string( "SEND" ) ||
        !file_queue.in_order( id1 ) ||
        !TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.pr_next_file=%d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, pr_next_file=%d",
                  res,
                  file_queue.begin()->id,
                  id1,
                  file_queue.getstatus(id1).c_str(),
                  id1,
                  file_queue.in_order( id1 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  file_queue.isLastFile() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   res++;
   tst();
   file_queue.doneFile( id1 );
   file_queue.get( TFilterQueue(receiver, 1) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id2 ||
        file_queue.getstatus(id2) != string( "SEND" ) ||
        !file_queue.in_order( id2 ) ||
        !TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.pr_next_file=%d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id2=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, pr_next_file=%d",
                  res,
                  file_queue.begin()->id,
                  id2,
                  file_queue.getstatus(id2).c_str(),
                  id2,
                  file_queue.in_order( id2 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  file_queue.isLastFile() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   res++;
   tst();
   file_queue.doneFile( id2 );
   file_queue.get( TFilterQueue(receiver, 1) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id3 ||
        file_queue.getstatus(id3) != string( "PUT" ) ||
        !file_queue.in_order( id3 ) ||
        !TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        !file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.pr_next_file=%d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id3=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, pr_next_file=%d",
                  res,
                  file_queue.begin()->id,
                  id3,
                  file_queue.getstatus(id3).c_str(),
                  id3,
                  file_queue.in_order( id3 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  file_queue.isLastFile() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   ///
   file_queue.doneFile( id3 );
   ////////////////////////////////////
   type = "SOFI"; //не важна сортировка
   id1 = TFileQueue::putFile( receiver,
                              sender,
                              type,
                              params,
                              string("TEST1") );
   id2 = TFileQueue::putFile( receiver,
                              sender,
                              type,
                              params,
                              string("TEST2") );
   id3 = TFileQueue::putFile( receiver,
                              sender,
                              type,
                              params,
                              string("TEST3") );
   res++;
   file_queue.get( TFilterQueue(receiver, 5) );
   tst();
   if ( file_queue.empty() ||
        file_queue.begin()->id != id1||
        file_queue.getstatus(id1) != string( "PUT" ) ||
        file_queue.in_order( id1 ) ||
        TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.isLastFile=%d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, isLastFile=%d",
                  res,
                  file_queue.begin()->id,
                  id1,
                  file_queue.getstatus(id1).c_str(),
                  id1,
                  file_queue.in_order( id1 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  file_queue.isLastFile() );
     }
    fail_unless(false, "file_queue error. see log");
   }
   res++;
   TFileQueue::sendFile( id1 );
   tst();
   file_queue.get( TFilterQueue(receiver, 5) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id2||
        file_queue.getstatus(id1) != string( "SEND" ) ||
        file_queue.getstatus(id2) != string( "PUT" ) ||
        file_queue.in_order( id2 ) ||
        TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.isLastFile=%d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id2=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, isLastFile=%d",
                  res,
                  file_queue.begin()->id,
                  id2,
                  file_queue.getstatus(id2).c_str(),
                  id2,
                  file_queue.in_order( id2 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  file_queue.isLastFile() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   res++;
   tst();
   sleep_filequeue(id1, id2, id3);
   file_queue.get( TFilterQueue(receiver, 1) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id1||
        file_queue.getstatus(id1) != string( "SEND" ) ||
        file_queue.getstatus(id2) != string( "PUT" ) ||
        file_queue.in_order( id1 ) ||
        file_queue.in_order( id2 ) ||
        TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.isLastFile=%d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, isLastFile=%d",
                  res,
                  file_queue.begin()->id,
                  id1,
                  file_queue.getstatus(id1).c_str(),
                  id1,
                  file_queue.in_order( id1 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  file_queue.isLastFile() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   tst();
   file_queue.doneFile( id1 );
   tst();
   sleep_filequeue(id1, id2, id3);
   res++;
   file_queue.get( TFilterQueue(receiver, 1) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id2||
        file_queue.getstatus(id2) != string( "PUT" ) ||
        file_queue.in_order( id2 ) ||
        TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.isLastFile=%d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, i2=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, isLastFile=%d",
                  res,
                  file_queue.begin()->id,
                  id2,
                  file_queue.getstatus(id2).c_str(),
                  id2,
                  file_queue.in_order( id2 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  file_queue.isLastFile() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   res++;
   tst();
   file_queue.doneFile( id2 );
   file_queue.get( TFilterQueue(receiver, 1) );
     if ( file_queue.empty() ||
        file_queue.begin()->id != id3||
        file_queue.getstatus(id3) != string( "PUT" ) ||
        file_queue.in_order( id3 ) ||
        TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        !file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.isLastFile=%d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu, isLastFile=%d",
                  res,
                  file_queue.begin()->id,
                  id3,
                  file_queue.getstatus(id3).c_str(),
                  id3,
                  file_queue.in_order( id3 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  file_queue.isLastFile() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   tst();
   res++;
   file_queue.doneFile( id3 );
   file_queue.get( TFilterQueue(receiver, 1) );
     if ( !file_queue.empty() ||
          !file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.isLastFile=%d", res, file_queue.empty(), file_queue.isLastFile() );
     fail_unless(false, "file_queue error. see log");
   }
   tst();
   type = "MINTRANS"; //не важна сортировка
   params[ "WORKDIR" ] = "c:\\work";
   TDateTime UTCSysdate = NowUTC() + 5.0/1440.0;
   id1 = TFileQueue::putFile( receiver,
                              sender,
                              type,
                              params,
                              string("TEST1") );
   id2 = TFileQueue::putFile( receiver,
                              sender,
                              type,
                              params,
                              string("TEST2") );
   id3 = TFileQueue::putFile( receiver,
                              sender,
                              type,
                              params,
                              string("TEST3") );
   file_queue.get( TFilterQueue(receiver, type, UTCSysdate) );
   string param_value;
   res++;
   if ( file_queue.empty() ||
        file_queue.begin()->id != id1||
        file_queue.getstatus(id2) != string( "PUT" ) ||
        file_queue.in_order( id1 ) ||
        file_queue.in_order( id2 ) ||
        TFileQueue::in_order( type ) ||
        file_queue.size() != 3 ||
        !TFileQueue::getparam_value( id1, "WORKDIR", param_value ) ||
        param_value != "c:\\work" ) {
     ProgError( STDLOG, "error%d: %d", res, file_queue.empty() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu,param_value=%s",
                  res,
                  file_queue.begin()->id,
                  id1,
                  file_queue.getstatus(id1).c_str(),
                  id1,
                  file_queue.in_order( id1 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  param_value.c_str() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   res++;
   tst();
   file_queue.get( TFilterQueue(receiver, type) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id1||
        file_queue.getstatus(id2) != string( "PUT" ) ||
        file_queue.in_order( id1 ) ||
        file_queue.in_order( id2 ) ||
        TFileQueue::in_order( type ) ||
        file_queue.size() != 3 ||
        !file_queue.getparam_value( id1, "WORKDIR", param_value ) ||
        param_value != "c:\\work" ) {
     ProgError( STDLOG, "error%d: %d", res, file_queue.empty() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu,param_value=%s",
                  res,
                  file_queue.begin()->id,
                  id1,
                  file_queue.getstatus(id1).c_str(),
                  id1,
                  file_queue.in_order( id1 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  param_value.c_str() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   TFileQueue::getEncoding( std::string( "UTG" ),
                            std::string("BETADC"),
                            true );
   res++;
   TFilterQueue filter2( receiver,1 );
   filter2.receiver = receiver;
   filter2.type = type;
   filter2.last_time = ASTRA::NoExists;
   filter2.first_id = ASTRA::NoExists;
   filter2.pr_first_order = true;
   filter2.timeout_sec = 5;

   file_queue.get( TFilterQueue(filter2) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id1||
        file_queue.getstatus(id2) != string( "PUT" ) ||
        file_queue.in_order( id1 ) ||
        file_queue.in_order( id2 ) ||
        TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        !file_queue.getparam_value( id1, "WORKDIR", param_value ) ||
        param_value != "c:\\work" ||
        file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, %d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu,param_value=%s, isLastFile=%d",
                  res,
                  file_queue.begin()->id,
                  id1,
                  file_queue.getstatus(id1).c_str(),
                  id1,
                  file_queue.in_order( id1 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  param_value.c_str(),
                  file_queue.isLastFile() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   res++;
   file_queue.sendFile( id1 );
   file_queue.get( TFilterQueue(filter2) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id2||
        file_queue.getstatus(id2) != string( "PUT" ) ||
        file_queue.in_order( id1 ) ||
        file_queue.in_order( id2 ) ||
        TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        !file_queue.getparam_value( id1, "WORKDIR", param_value ) ||
        param_value != "c:\\work" ||
        file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, %d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu,param_value=%s, isLastFile=%d",
                  res,
                  file_queue.begin()->id,
                  id2,
                  file_queue.getstatus(id1).c_str(),
                  id2,
                  file_queue.in_order( id2 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  param_value.c_str(),
                  file_queue.isLastFile() );
     }
     fail_unless(false, "file_queue error. see log");
   }
   res++;
   file_queue.sendFile( id2 );
   file_queue.get( TFilterQueue(filter2) );
   fail_unless(!file_queue.empty());
   fail_unless(file_queue.begin()->id == id3);
   fail_unless(file_queue.getstatus(id2) == string( "SEND" ));
   fail_unless(!file_queue.in_order( id1 ));
   fail_unless(!file_queue.in_order( id3 ));
   fail_unless(!TFileQueue::in_order( type ));
   fail_unless(file_queue.size() == 1);
   fail_unless(file_queue.getparam_value( id1, "WORKDIR", param_value ));
   fail_unless(file_queue.begin()->params["WORKDIR"] == param_value);
   fail_unless(param_value == "c:\\work");
   fail_unless(file_queue.isLastFile());
}
END_TEST;

START_TEST(check_get_file_data)
{
  const auto utc = Dates::second_clock::universal_time();
  insert_files(1, "snd", "rcv", "file", utc, std::string(10 * 1024, '\x4'));
  fail_unless(TFileQueue::getFileData(1) == std::string(10 * 1024, '\x4'));
  fail_unless(TFileQueue::getFileData(2) == "");
  fail_unless(TFileQueue::getwait_time(1).first < 1);
}
END_TEST;

#define SUITENAME "file_queue"
TCASEREGISTER(testInitDB, testShutDBConnection)
{
  ADD_TEST(check_file_queue);
  ADD_TEST(check_put_file);
  ADD_TEST(check_get_file_data);
  ADD_TEST(check_file_encoding);
  ADD_TEST(check_old_tests_file_queue);
}
TCASEFINISH;
#undef SUITENAME
#endif /*XP_TESTING*/
