//---------------------------------------------------------------------------
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "basic.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "arx_daily.h"
#include "checkin.h"
#include "passenger.h"
#include "telegram.h"
#include "empty_proc.h"
#include "tlg/tlg_parser.h"
#include "tlg/tlg.h"
#include "tclmon/tcl_utils.h"
#include "serverlib/ourtime.h"
#include <set>
#include "season.h"
#include "events.h"
#include "serverlib/posthooks.h"



#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

const int sleepsec = 25;
#define WAIT_INTERVAL           60000      //миллисекунды
const unsigned int SERVER_HEADER_SIZE = 100;
const unsigned int MAX_INCOMMING_BUF_SIZE = 1000000;
const int SOCKET_ERROR = -1;

using namespace std;

void TestInterface::TestRequestDup(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  BASIC::TDateTime start_time = BASIC::NowUTC();
  for(;;)
  {
    if ((BASIC::NowUTC()-start_time)*86400000>200) break;  //задержка на 50 мсек
  };
  NewTextChild(resNode, "iteration", NodeAsInteger("iteration", reqNode) );
};

int GetLastError1( int Ret, int errCode )
{
  if ( Ret != errCode )
    return 0;
  else {
    ProgTrace( TRACE5, "GetLastError, ret=%d", Ret );
    return errno;
  }
}

int GetErr1( int code, const char *func )
{
  if ( code != 0 ) {
    throw EXCEPTIONS::Exception("TCPSession error:%s, error=%d, func=%s", strerror( code ), code, func);
  }
  return code;
}

int init_socket_grp( const string &var_addr_grp, const string &var_port_grp )
{
  string addr_grp = getTCLParam( var_addr_grp.c_str(), "" );
  string port_grp = getTCLParam( var_port_grp.c_str(), "" );
  ProgTrace( TRACE5, "%s, %s, %s:%s", var_addr_grp.c_str(), var_port_grp.c_str(),
             addr_grp.c_str(), port_grp.c_str() );

  int handle = socket( AF_INET, SOCK_STREAM, 0 );
  GetErr1( GetLastError1( handle, SOCKET_ERROR ), "socket()" );
  GetErr1( GetLastError1( fcntl( handle, F_SETFL, O_NONBLOCK ), SOCKET_ERROR ), "fcntl" );
  sockaddr_in addr_in;
  addr_in.sin_family = AF_INET;
  int port;
  if ( BASIC::StrToInt( port_grp.c_str(), port ) == EOF )
    port = -1;
  addr_in.sin_port = htons( port );
  addr_in.sin_addr.s_addr = inet_addr( addr_grp.c_str() );
  int val = 1;
  GetErr1( GetLastError1( setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &val, sizeof (val)), SOCKET_ERROR ), "setsockopt" );
  int one = 1;
  GetErr1( GetLastError1( setsockopt( handle, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one) ), SOCKET_ERROR ), "setsockopt" );
  int err = connect( handle, (struct sockaddr *)&addr_in, sizeof(addr_in) );
  err = GetLastError1( err, SOCKET_ERROR );
  if ( err != 0 && err != EINPROGRESS )
    GetErr1( err, "connect" );
  return handle;
}

int exec_grp( int handle, const char *buf, int len )
{
  fd_set rsets, wsets, esets;
  FD_ZERO( &rsets );
  FD_ZERO( &wsets );
  FD_ZERO( &esets );
  FD_SET( handle, &rsets );
  FD_SET( handle, &wsets );
  FD_SET( handle, &esets );

  timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  GetErr1( GetLastError1( select( handle + 1, &rsets, &wsets, &esets, &timeout ), SOCKET_ERROR ), "select" );
  socklen_t err_len;
  int error, received = 0, sended = 0;
  err_len = sizeof( error );
  if ( FD_ISSET( handle, &esets ) ||
       GetErr1( GetLastError1( getsockopt( handle, SOL_SOCKET, SO_ERROR, &error, &err_len ), -1 ), "getsockopt" ) || error != 0 ) {
    throw EXCEPTIONS::Exception("exec_grp: invalid socket stat");
    return 0;
  }
  if ( FD_ISSET( handle, &rsets ) ) {
    int BLen = 0;
    GetErr1( GetLastError1( ioctl( handle, FIONREAD, &BLen ), SOCKET_ERROR ), "ioctl" );
    if ( BLen == 0 )
      throw EXCEPTIONS::Exception("Connect aborted");
    char recbuf[1000000];
    received = recv( handle, recbuf, sizeof(recbuf), 0 );
    GetErr1( GetLastError1( received, SOCKET_ERROR ), "recv" );
  }
  if ( !FD_ISSET( handle, &wsets ) ) {
    ProgError( STDLOG, "send_grp - cannot write to socket" );
  }
  else
    if ( len > 0 ) {
      sended = send( handle, buf, len, 0 ); // отправляем сколько сможем
      GetErr1( GetLastError1( sended, SOCKET_ERROR ), "send" );
    }
  if ( received + sended > 0 )
    ProgTrace( TRACE5, "received=%d, sended=%d", received, sended );
  return sended;
}

int main_empty_proc_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try {
    sleep(1);
    InitLogTime(NULL);
    OpenLogFile("log1");
    
    int handle_grp2 = init_socket_grp( "DUB_ADDR_GRP2", "DUB_PORT_GRP2" );
    int handle_grp3 = init_socket_grp( "DUB_ADDR_GRP3", "DUB_PORT_GRP3" );

    char buf[100000];
    vector<string> bufs;
    for( ;; )
    {
      InitLogTime(NULL);
      int len = waitCmd("REQUEST_DUP",10,buf,sizeof(buf));
      if ( len > 0 ) {
        ProgTrace( TRACE5, "incomming msg, size()=%d", len - 1 );
        if ( bufs.size() < 1000 )
          bufs.push_back( string( buf, len ) );
        else
          ProgError( STDLOG, "incomming buffers more then 1000 msgs" );
      }
      int handle;
      for ( int igrp=2; igrp<4; igrp++ ) {
        if ( igrp == 2  )
          handle = handle_grp2;
         else
          handle = handle_grp3;
        for ( vector<string>::iterator i=bufs.begin(); i!=bufs.end();  ) {
          if ( i->c_str()[ 0 ] != igrp  ) {
            i++;
            continue;
          }
          int sended = exec_grp( handle, i->data() + 1, i->size() - 1 );
          if ( sended ) {
            i->erase( 1, sended );
          }
          if ( i->size() == 1 ) {
            i = bufs.erase( i );
          }
          else
            break;
        }
        exec_grp( handle, NULL, 0 );
      }
      //sleep(30); расскоментарив эту строку можно создать ситуацию переполнения sendto,
      //           которая однако не приводит к плохим последствиям для сервера, перенаправляющего поток запросов
    }
  }
  catch( EXCEPTIONS:: Exception &e ) {
    ProgError( STDLOG, "Exception: %s", e.what() );
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
  };
  return 0;
}

#include "oralib.h"
#include "basic.h"
#include "astra_utils.h"

using namespace ASTRA;
using namespace BASIC;
using namespace std;

void alter_wait(int processed, bool commit_before_sleep=false, int work_secs=5, int sleep_secs=5)
{
  static time_t start_time=time(NULL);
  if (time(NULL)-start_time>=work_secs)
  {
    if (commit_before_sleep) OraSession.Commit();
    printf("%d iterations processed. sleep...", processed);
    fflush(stdout);
    sleep(sleep_secs);
    printf("go!\n");
    start_time=time(NULL);
  };
};

/*
CREATE TABLE drop_events_stat
(
  month DATE NOT NULL,
  event_type VARCHAR2(3) NOT NULL,
  event_count NUMBER NOT NULL
);
*/

int get_events_stat(int argc,char **argv)
{
  TQuery Qry(&OraSession);

  Qry.Clear();
  Qry.SQLText="SELECT TO_DATE('01.10.2006','DD.MM.YYYY') AS min_date FROM dual";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_date")) return 0;
  TDateTime min_date=Qry.FieldAsDateTime("min_date");

  Qry.Clear();
  Qry.SQLText="SELECT TO_DATE('01.05.2012','DD.MM.YYYY') AS max_date FROM dual";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_date")) return 0;
  TDateTime max_date=Qry.FieldAsDateTime("max_date");

  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO drop_events_stat(month, event_type, event_count) "
    "SELECT :low_date, type, COUNT(*) FROM events "
    "WHERE time>=:low_date AND time<:high_date "
    "GROUP BY type";
  Qry.DeclareVariable("low_date", otDate);
  Qry.DeclareVariable("high_date", otDate);


  int processed=0;
  for(TDateTime curr_date=min_date; curr_date<=max_date; curr_date=IncMonth(curr_date, 1), processed++)
  {
    alter_wait(processed, false, 10, 5);
    Qry.SetVariable("low_date",curr_date);
    Qry.SetVariable("high_date",IncMonth(curr_date, 1));
    Qry.Execute();
    OraSession.Commit();
  };

  return 0;
};

int get_events_stat2(int argc,char **argv)
{
  TQuery Qry(&OraSession);

  Qry.Clear();
  Qry.SQLText="SELECT TO_DATE('01.10.2006','DD.MM.YYYY') AS min_date FROM dual";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_date")) return 0;
  TDateTime min_date=Qry.FieldAsDateTime("min_date");

  Qry.Clear();
  Qry.SQLText="SELECT TO_DATE('17.08.2012','DD.MM.YYYY') AS max_date FROM dual";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_date")) return 0;
  TDateTime max_date=Qry.FieldAsDateTime("max_date");

  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO drop_events(type, time, ev_order, msg, screen, ev_user, station, id1, id2, id3) "
    "SELECT type, time, ev_order, msg, screen, ev_user, station, id1, id2, id3 FROM events "
    "WHERE time>=:low_date AND time<:high_date AND type<>'СЕЗ' ";
  Qry.DeclareVariable("low_date", otDate);
  Qry.DeclareVariable("high_date", otDate);


  int processed=0;
  for(TDateTime curr_date=min_date; curr_date<max_date; curr_date+=1.0, processed++)
  {
    alter_wait(processed, false, 10, 1);
    Qry.SetVariable("low_date",curr_date);
    Qry.SetVariable("high_date",curr_date+1.0);
    Qry.Execute();
    OraSession.Commit();
  };


  return 0;
};
/*
CREATE UNIQUE INDEX drop_events__AK ON drop_events(ev_order);

DECLARE
  CURSOR cur IS
    SELECT * FROM drop_events WHERE pr_del IS NULL;
vid NUMBER(9);
i   BINARY_INTEGER;
BEGIN
  FOR curRow IN cur LOOP
    IF curRow.type='РЕЙ' OR curRow.type='ГРФ' THEN
      SELECT COUNT(*) INTO i FROM points WHERE point_id=curRow.id1;
      IF i>0 THEN
        UPDATE drop_events SET pr_del=0 WHERE ev_order=curRow.ev_order;
        COMMIT;
      END IF;
    END IF;
    IF curRow.type='ДИС' THEN
      SELECT COUNT(*) INTO i FROM points WHERE move_id=curRow.id1;
      IF i>0 THEN
        UPDATE drop_events SET pr_del=0 WHERE ev_order=curRow.ev_order;
        COMMIT;
      ELSE
        SELECT COUNT(*) INTO i FROM points WHERE move_id=curRow.id2;
        IF i>0 THEN
          UPDATE drop_events SET pr_del=9 WHERE ev_order=curRow.ev_order;
          COMMIT;
        END IF;
      END IF;
    END IF;
  END LOOP;
END;
/

DECLARE
  CURSOR cur IS
    SELECT * FROM drop_events WHERE pr_del IS NOT NULL AND pr_del<0;
BEGIN
  FOR curRow IN cur LOOP
    DELETE FROM events WHERE time=curRow.time AND ev_order=curRow.ev_order;
    IF SQL%ROWCOUNT=1 THEN
      DELETE FROM drop_events WHERE time=curRow.time AND ev_order=curRow.ev_order;
    END IF;
  END LOOP;
END;
/

SELECT type,
       DECODE(id1,NULL,'NULL','NOT NULL') AS id1,
       DECODE(id2,NULL,'NULL','NOT NULL') AS id2,
       DECODE(id3,NULL,'NULL','NOT NULL') AS id3,
       COUNT(*) AS num,
       MIN(time) AS min_time,
       MAX(time) AS max_time,
       SUBSTR(MIN(msg),1,50) AS msg
FROM drop_events WHERE pr_del IS NULL AND type IN ('РЕЙ','ГРФ','ДИС')
GROUP BY type,
         SUBSTR(msg,1,5),
         DECODE(id1,NULL,'NULL','NOT NULL'),
         DECODE(id2,NULL,'NULL','NOT NULL'),
         DECODE(id3,NULL,'NULL','NOT NULL')
ORDER BY max_time;
*/
int get_sirena_rozysk_stat(int argc,char **argv)
{
  string country="ТД";

  TQuery PaxDocQry(&OraSession);
  TQuery PaxDocoQry(&OraSession);
  TQuery Qry(&OraSession);
  
  const char* filename="TJ.txt";
  ofstream f;
  f.open(filename);
  if (!f.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",filename);
  try
  {
    for(int pass=1; pass<=2; pass++)
    {
      Qry.Clear();
      if (pass==1)
        Qry.SQLText="SELECT TO_DATE('01.01.2012','DD.MM.YYYY') AS min_date FROM dual";
      else
        Qry.SQLText="SELECT TO_DATE('01.01.2012','DD.MM.YYYY') AS min_date FROM dual";
      Qry.Execute();
      if (Qry.Eof || Qry.FieldIsNULL("min_date")) return 0;
      TDateTime min_date=Qry.FieldAsDateTime("min_date");
    
      Qry.Clear();
      if (pass==1)
        Qry.SQLText="SELECT MAX(part_key) /*TO_DATE('02.01.2012','DD.MM.YYYY')*/ AS max_date FROM /*dual*/arx_points";
      else
        Qry.SQLText="SELECT MAX(time_out) /*TO_DATE('03.07.2012','DD.MM.YYYY')*/ AS max_date FROM /*dual*/points";
      Qry.Execute();
      if (Qry.Eof || Qry.FieldIsNULL("max_date")) return 0;
      TDateTime max_date=Qry.FieldAsDateTime("max_date");

      Qry.Clear();
      if (pass==1)
        Qry.SQLText=
          "SELECT part_key, point_id, point_num, first_point, pr_tranzit "
          "FROM arx_points, airps, cities "
          "WHERE arx_points.part_key>=:low_date AND arx_points.part_key<:high_date AND "
          "      arx_points.airp=airps.code AND airps.city=cities.code AND cities.country=:country AND arx_points.pr_del=0";
      else
        Qry.SQLText=
          "SELECT point_id, point_num, first_point, pr_tranzit "
          "FROM points, airps, cities "
          "WHERE points.time_out>=:low_date AND points.time_out<:high_date AND "
          "      points.airp=airps.code AND airps.city=cities.code AND cities.country=:country AND points.pr_del=0 "
          "UNION "
          "SELECT point_id, point_num, first_point, pr_tranzit "
          "FROM points, airps, cities "
          "WHERE points.time_in>=:low_date AND points.time_in<:high_date AND "
          "      points.airp=airps.code AND airps.city=cities.code AND cities.country=:country AND points.pr_del=0 ";
      Qry.CreateVariable("country", otString, country);
      Qry.DeclareVariable("low_date", otDate);
      Qry.DeclareVariable("high_date", otDate);

      TQuery PaxQry(&OraSession);
      PaxQry.Clear();
      if (pass==1)
      {
        PaxQry.SQLText=
          "SELECT arx_mark_trips.point_id AS point_id_mark, "
          "       DECODE(arx_mark_trips.point_id,NULL,arx_points.airline,arx_mark_trips.airline) AS airline, "
          "       arx_pax.ticket_no, "
          "       arx_points.scd_out, "
          "       DECODE(arx_mark_trips.point_id,NULL,arx_points.flt_no,arx_mark_trips.flt_no) AS flt_no, "
          "       DECODE(arx_mark_trips.point_id,NULL,arx_points.suffix,arx_mark_trips.suffix) AS suffix, "
          "       arx_pax_grp.class, "
          "       TRIM(arx_pax.surname||' '||arx_pax.name) AS full_name, "
          "       arx_pax.pr_brd, arx_pax.reg_no, arx_pax.pax_id, "
          "       arx_points.act_out "
          "FROM arx_points, arx_pax_grp, arx_pax, arx_mark_trips "
          "WHERE arx_points.part_key=arx_pax_grp.part_key AND "
          "      arx_points.point_id=arx_pax_grp.point_dep AND "
          "      arx_pax_grp.part_key=arx_pax.part_key AND "
          "      arx_pax_grp.grp_id=arx_pax.grp_id AND "
          "      arx_pax_grp.part_key=arx_mark_trips.part_key(+) AND "
          "      arx_pax_grp.point_id_mark=arx_mark_trips.point_id(+) AND "
          "      arx_pax_grp.part_key=:part_key AND "
          "      arx_pax_grp.point_dep=:point_dep AND "
          "      arx_pax_grp.point_arv=:point_arv AND "
          "      arx_pax.pr_brd IS NOT NULL ";
        PaxQry.DeclareVariable("part_key", otDate);
      }
      else
      {
        PaxQry.SQLText=
          "SELECT mark_trips.point_id AS point_id_mark, "
          "       DECODE(mark_trips.point_id,NULL,points.airline,mark_trips.airline) AS airline, "
          "       pax.ticket_no, "
          "       points.scd_out, "
          "       DECODE(mark_trips.point_id,NULL,points.flt_no,mark_trips.flt_no) AS flt_no, "
          "       DECODE(mark_trips.point_id,NULL,points.suffix,mark_trips.suffix) AS suffix, "
          "       pax_grp.class, "
          "       TRIM(pax.surname||' '||pax.name) AS full_name, "
          "       pax.pr_brd, pax.reg_no, pax.pax_id, "
          "       points.act_out "
          "FROM points, pax_grp, pax, mark_trips "
          "WHERE points.point_id=pax_grp.point_dep AND "
          "      pax_grp.grp_id=pax.grp_id AND "
          "      pax_grp.point_id_mark=mark_trips.point_id(+) AND "
          "      pax_grp.point_dep=:point_dep AND "
          "      pax_grp.point_arv=:point_arv AND "
          "      pax.pr_brd IS NOT NULL ";
      }
      PaxQry.DeclareVariable("point_dep", otInteger);
      PaxQry.DeclareVariable("point_arv", otInteger);

      TQuery EventsQry(&OraSession);
      EventsQry.Clear();
      if (pass==1)
      {
        EventsQry.SQLText=
          "SELECT MIN(time) AS time FROM arx_events "
          "WHERE part_key=:part_key AND type=:evtPax AND id1=:point_dep AND id2=:reg_no";
        EventsQry.DeclareVariable("part_key", otDate);
      }
      else
      {
        EventsQry.SQLText=
          "SELECT MIN(time) AS time FROM events "
          "WHERE type=:evtPax AND id1=:point_dep AND id2=:reg_no";
      };
      EventsQry.CreateVariable("evtPax", otString, EncodeEventType(ASTRA::evtPax));
      EventsQry.DeclareVariable("point_dep", otInteger);
      EventsQry.DeclareVariable("reg_no", otInteger);

      set< pair<TDateTime, int> > point_ids;
      int processed=0;
      for(TDateTime curr_date=min_date; curr_date<=max_date; curr_date+=1.0, processed++)
      {
        alter_wait(processed, false, 10, 5);
        Qry.SetVariable("low_date",curr_date);
        Qry.SetVariable("high_date",curr_date+1.0);
        Qry.Execute();
        for(;!Qry.Eof;Qry.Next())
        {
          TDateTime part_key=pass==1?Qry.FieldAsDateTime("part_key"):NoExists;
          int point_id=Qry.FieldAsInteger("point_id");
          int point_num=Qry.FieldAsInteger("point_num");
          int first_point=Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point");
          bool pr_tranzit=Qry.FieldAsInteger("pr_tranzit")!=0;
          for(int pass2=1; pass2<=2; pass2++)
          {
            TTripRoute route;
            if (pass2==1)
            {
              route.GetRouteBefore(part_key,
                                   point_id,
                                   point_num,
                                   first_point,
                                   pr_tranzit,
                                   trtWithCurrent,
                                   trtNotCancelled);
            }
            else
            {
              TTripRouteItem item;
              route.GetNextAirp(part_key,
                                point_id,
                                point_num,
                                first_point,
                                pr_tranzit,
                                trtNotCancelled,
                                item);
              if (item.point_id!=NoExists)
              {
                route.GetRouteBefore(item.part_key, item.point_id, trtNotCurrent, trtNotCancelled);
                //printf("routeBefore: %s\n", route.GetStr().c_str());
              };
            };
            //printf("routeBefore(point_num=%d): %s\n", Qry.FieldAsInteger("point_num"), route.GetStr().c_str());
            if (!route.empty())
              point_ids.insert( make_pair(route.begin()->part_key, route.begin()->point_id) );
          };
        };
      };
      printf("point_ids.size()=%d\n", point_ids.size());
      processed=0;
      for(set< pair<TDateTime, int> >::const_iterator i=point_ids.begin(); i!=point_ids.end(); i++, processed++)
      {
        alter_wait(processed, false, 10, 5);
        TTripRoute route;
        route.GetRouteAfter(i->first, i->second, trtWithCurrent, trtNotCancelled);
        //printf("routeAfter: %s\n", route.GetStr().c_str());
        for(TTripRoute::const_iterator r1=route.begin(); r1!=route.end(); r1++)
        {
          string city_dep, country_dep;
          try
          {
            TAirpsRow &airpRow = (TAirpsRow&)base_tables.get("airps").get_row("code",r1->airp);
            city_dep=airpRow.city;
            TCitiesRow &cityRow = (TCitiesRow&)base_tables.get("cities").get_row("code",airpRow.city);
            country_dep=cityRow.country;
          }
          catch(EBaseTableError) {};

          for(TTripRoute::const_iterator r2=r1; r2!=route.end(); r2++)
          {
            if (r1->part_key==r2->part_key &&
                r1->point_id==r2->point_id) continue;
            string city_arv, country_arv;
            try
            {
              TAirpsRow &airpRow = (TAirpsRow&)base_tables.get("airps").get_row("code",r2->airp);
              city_arv=airpRow.city;
              TCitiesRow &cityRow = (TCitiesRow&)base_tables.get("cities").get_row("code",airpRow.city);
              country_arv=cityRow.country;
            }
            catch(EBaseTableError) {};
            if (country_dep==country || country_arv==country)
            {
              //alter_wait(processed);
              if (pass==1)
                PaxQry.SetVariable("part_key", r1->part_key);
              PaxQry.SetVariable("point_dep", r1->point_id);
              PaxQry.SetVariable("point_arv", r2->point_id);
              if (pass==1)
                EventsQry.SetVariable("part_key", r1->part_key);
              EventsQry.SetVariable("point_dep", r1->point_id);
              PaxQry.Execute();
              /*printf("load pax: part_key=%s point_dep=%d point_arv=%d EOF=%d\n",
                     (pass==1?DateTimeToStr(r1->part_key, "dd.mm.yy hh:nn:ss").c_str():""),
                     r1->point_id, r2->point_id, (int)PaxQry.Eof);*/

              for(;!PaxQry.Eof;PaxQry.Next())
              {
                if (PaxQry.FieldIsNULL("point_id_mark"))
                  printf("empty airline_mark (part_key=%s, point_dep=%d)",
                         (pass==1?DateTimeToStr(r1->part_key, "dd.mm.yy hh:nn:ss").c_str():""),
                         r1->point_id);

                TDateTime scd_out_local=NoExists;
                if (!PaxQry.FieldIsNULL("scd_out"))
                {
                  scd_out_local=UTCToLocal(PaxQry.FieldAsDateTime("scd_out"),
                                           AirpTZRegion(r1->airp));
                };
                if (scd_out_local==NoExists || scd_out_local<min_date) continue;


                f << "ASTRA;;;"
                  << PaxQry.FieldAsString("airline") << ";;"
                  << PaxQry.FieldAsString("ticket_no") << ";";

                if (!PaxQry.FieldIsNULL("scd_out"))
                {
                  TDateTime scd_out_local=UTCToLocal(PaxQry.FieldAsDateTime("scd_out"),
                                                     AirpTZRegion(r1->airp));
                  f << DateTimeToStr(scd_out_local, "dd.mm.yyyy") << ";"
                    << DateTimeToStr(scd_out_local, "hh:nn") << ";";
                }
                else
                {
                  f << ";;";
                };

                if (!PaxQry.FieldIsNULL("flt_no"))
                {
                  f << setw(3) << setfill('0')
                    << PaxQry.FieldAsInteger("flt_no")
                    << PaxQry.FieldAsString("suffix") << ";";
                }
                else
                {
                  f << ";";
                };

                f << city_dep << ";" << city_arv << ";"
                  << r1->airp << ";" << r2->airp << ";";
                switch( DecodeClass( PaxQry.FieldAsString( "class" ) ) ) {
              		case F: f << ";";
              			break;
              		case C: f << "Бизнес-класс;";
              			break;
              		case Y: f << "Эконом-класс;";
              			break;
              		default: f << ";";
              	};

              	f << ";;;";

                int pax_id=PaxQry.FieldAsInteger("pax_id");

                CheckIn::TPaxDocItem doc;
                LoadPaxDoc(r1->part_key, pax_id, doc, PaxDocQry);

                CheckIn::TPaxDocoItem doco;
                LoadPaxDoco(r1->part_key, pax_id, doco, PaxDocoQry);

                if (doc.surname.empty())
                  f << PaxQry.FieldAsString("full_name") << ";";
                else
                  f << doc.surname
                    << (doc.first_name.empty()?"":" ") << doc.first_name
                    << (doc.second_name.empty()?"":" ") << doc.second_name << ";";

                f << doc.gender << ";"
                  << doco.birth_place << ";";

                if (doc.birth_date!=NoExists)
                  f << DateTimeToStr(doc.birth_date, "dd.mm.yyyy") << ";";
                else
                  f << ";";

                f << doc.nationality << ";"
                  << (doc.type=="P"?"Паспорт":"") << ";"
                  << doc.no << ";";

                if (doc.expiry_date!=NoExists)
                  f << DateTimeToStr(doc.expiry_date, "dd.mm.yyyy") << ";";
                else
                  f << ";";

                f << doc.issue_country << ";";

                if (doco.type=="V")
                {
                  f << doco.no << ";";
                  if (doco.issue_date!=NoExists)
                    f << DateTimeToStr(doco.issue_date, "dd.mm.yyyy") << ";";
                  else
                    f << ";";
                  f << doco.issue_place << ";"
                    << doco.applic_country << ";";
                }
                else
                {
                  f << ";;;;";
                };

                f << ";;Зарегистрирован;";

                EventsQry.SetVariable("reg_no", PaxQry.FieldAsInteger("reg_no"));
                EventsQry.Execute();
                if (!EventsQry.Eof && !EventsQry.FieldIsNULL("time"))
                {
                  TDateTime time_local=UTCToLocal(EventsQry.FieldAsDateTime("time"),
                                                     AirpTZRegion(r1->airp));
                  f << DateTimeToStr(time_local, "dd.mm.yyyy hh:nn:ss") << ";";
                }
                else
                  f << ";";

                f << ";;";

                if (PaxQry.FieldAsInteger("pr_brd")!=0)
                  f << "Пассажир вылетел;";
                else
                  f << "Пассажир не вылетел;";

                if (!PaxQry.FieldIsNULL("act_out"))
                {
                  TDateTime act_out_local=UTCToLocal(PaxQry.FieldAsDateTime("act_out"),
                                                     AirpTZRegion(r1->airp));
                  f << DateTimeToStr(act_out_local, "dd.mm.yyyy hh:nn");
                };

                f << endl;
              };

            };
          };
        };
      };
    }; //pass
    f.close();
  }
  catch(...)
  {
    try { f.close(); } catch( ... ) { };
    try
    {
      //в случае ошибки запишем пустой файл
      f.open(filename);
      if (f.is_open()) f.close();
    }
    catch( ... ) { };
    throw;
  };
  
  return 0;
};

void get_basel_aero_flight_stat(TDateTime part_key, int point_id, ofstream &f);

int get_basel_aero_stat(int argc,char **argv)
{
  vector<string> airps;
  airps.push_back("СОЧ");
  airps.push_back("АНА");
  airps.push_back("ГДЖ");
  airps.push_back("КПА");
  
  TQuery Qry(&OraSession);

  Qry.Clear();
  Qry.SQLText="SELECT TO_DATE('01.02.2012','DD.MM.YYYY') AS min_date FROM dual";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_date")) return 0;
  TDateTime min_date=Qry.FieldAsDateTime("min_date");

  Qry.Clear();
  Qry.SQLText="SELECT TO_DATE('01.01.2013','DD.MM.YYYY') AS max_date FROM dual";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_date")) return 0;
  TDateTime max_date=Qry.FieldAsDateTime("max_date");
  
  for(TDateTime curr_date=min_date; curr_date<max_date; curr_date=IncMonth(curr_date, 1))
  {
    for(vector<string>::const_iterator a=airps.begin(); a!=airps.end(); ++a)
    {
      ostringstream filename;
      filename << "ASTRA-" << ElemIdToElem(etAirp, *a, efmtCodeNative, AstraLocale::LANG_EN)
               << "-" << DateTimeToStr(curr_date, "yyyymm") << ".csv";
      ofstream f;
      f.open(filename.str().c_str());
      if (!f.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",filename.str().c_str());
      try
      {
        f << "viewDate;viewFlight;viewName;viewGroup;viewPCT;viewWeight;viewCarryon;viewPayWeight;"
          << "viewTag;viewUncheckin;viewStatus;viewCheckinNo;viewCheckinTime;viewChekinDuration;viewBoardingTime;"
          << "viewDeparturePlanTime;viewDepartureRealTime;viewBagNorms;viewPCTWeightPaidByType;viewClass"
          << endl;

        multimap< TDateTime, pair<TDateTime, int> > points;
        for(int pass=0; pass<=2; pass++)
        {
          ostringstream sql;
          if (pass==0)
            sql << "SELECT \n"
                   "    NULL part_key, \n";
          else
            sql << "SELECT \n"
                   "    arx_points.part_key, \n";

          sql << "    point_id, airp, scd_out \n";
          if (pass==0)
            sql << "FROM points \n"
                   "WHERE points.scd_out >= :FirstDate AND points.scd_out < :LastDate \n";
          if (pass==1)
            sql << "FROM arx_points \n"
                   "WHERE arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND \n"
                   "      arx_points.part_key >= :FirstDate and arx_points.part_key < :LastDate + :arx_trip_date_range \n";
          if (pass==2)
            sql << "FROM arx_points, \n"
                   "     (SELECT part_key, move_id FROM move_arx_ext \n"
                   "      WHERE part_key >= :LastDate + :arx_trip_date_range AND part_key <= :LastDate + date_range) arx_ext \n"
                   "WHERE arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND \n"
                   "      arx_points.part_key=arx_ext.part_key AND arx_points.move_id=arx_ext.move_id \n";

          sql << " AND pr_del = 0 AND pr_reg<>0 \n"
              << " AND airp=:airp \n";
          
          Qry.Clear();
          Qry.SQLText = sql.str().c_str();
          
          if (pass!=0)
            Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());

          //ProgTrace(TRACE5, "get_basel_aero_stat: pass=%d SQL=\n%s", pass, sql.str().c_str());
          TDateTime low_date=curr_date;
          TDateTime high_date=IncMonth(curr_date, 1);
          
          Qry.CreateVariable("FirstDate", otDate, low_date-1.0);
          Qry.CreateVariable("LastDate", otDate, high_date+1.0);
          Qry.CreateVariable("airp", otString, *a);
          Qry.Execute();
          string region=AirpTZRegion(*a);
          for(;!Qry.Eof;Qry.Next())
          {
          /*  string region;
            try
            {
              region=AirpTZRegion(Qry.FieldAsString("airp"));
            }
            catch(...)
            {
              continue;
            };*/

            TDateTime scd_local=UTCToLocal(Qry.FieldAsDateTime("scd_out"), region);
            if (scd_local<low_date || scd_local>=high_date) continue;

            points.insert( make_pair(scd_local,
                           make_pair(Qry.FieldIsNULL("part_key")?NoExists:Qry.FieldAsDateTime("part_key"),
                                     Qry.FieldAsInteger("point_id"))));
          };
        }; //pass
        printf("curr_date=%s\n", DateTimeToStr(curr_date,"dd.mm.yy").c_str());

        int processed=0;
        for(multimap< TDateTime, pair<TDateTime, int> >::const_iterator i=points.begin();
                                                                        i!=points.end();
                                                                        ++i, processed++)
        {
          alter_wait(processed, false, 10, 1);
          get_basel_aero_flight_stat(i->second.first, i->second.second, f);
        };


        f.close();
      }
      catch(...)
      {
        try { f.close(); } catch( ... ) { };
        try
        {
          //в случае ошибки запишем пустой файл
          f.open(filename.str().c_str());
          if (f.is_open()) f.close();
        }
        catch( ... ) { };
        throw;
      };
    };
  };

  return 0;
};

void get_basel_aero_flight_stat(TDateTime part_key, int point_id, ofstream &f)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql <<
    "SELECT airline, flt_no, suffix, airp, scd_out, act_out AS real_out ";
  if (part_key!=NoExists)
  {
    sql << "FROM arx_points "
           "WHERE part_key=:part_key AND point_id=:point_id AND "
           "      pr_del=0 AND pr_reg<>0 ";
    Qry.CreateVariable("part_key", otDate, part_key);
  }
  else
    sql << "FROM points "
           "WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0 ";
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) return;
  TTripInfo operFlt;
  operFlt.Init(Qry);

  string region=AirpTZRegion(operFlt.airp);


  map< pair<int, int>, pair<TDateTime, TDateTime> > events;
  Qry.Clear();
  sql.str("");
  sql <<
    "SELECT id3 AS grp_id, id2 AS reg_no, "
    "       MIN(DECODE(INSTR(msg,'зарегистрирован'),0,TO_DATE(NULL),time)) AS ckin_time, "
    "       MAX(DECODE(INSTR(msg,'прошел посадку'),0,TO_DATE(NULL),time)) AS brd_time ";
  if (part_key!=NoExists)
  {
    sql <<
      "FROM arx_events "
      "WHERE type=:evtPax AND part_key=:part_key AND id1=:point_id AND ";
    Qry.CreateVariable("part_key", otDate, part_key);
  }
  else
    sql <<
      "FROM events "
      "WHERE type=:evtPax AND id1=:point_id AND ";
  sql <<
    "      (msg like '%зарегистрирован%' OR msg like '%прошел посадку%') "
    "GROUP BY id3, id2";
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("evtPax", otString, EncodeEventType(ASTRA::evtPax));
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    int grp_id=Qry.FieldIsNULL("grp_id")?NoExists:Qry.FieldAsInteger("grp_id");
    int reg_no=Qry.FieldIsNULL("reg_no")?NoExists:Qry.FieldAsInteger("reg_no");
    TDateTime ckin_time=Qry.FieldIsNULL("ckin_time")?NoExists:Qry.FieldAsDateTime("ckin_time");
    TDateTime brd_time=Qry.FieldIsNULL("brd_time")?NoExists:Qry.FieldAsDateTime("brd_time");
    events[ make_pair(grp_id, reg_no) ] = make_pair(ckin_time, brd_time);
  };

  TQuery PaxNormQry(&OraSession);
  TQuery GrpNormQry(&OraSession);
  TQuery BagQry(&OraSession);
  TQuery PaidQry(&OraSession);
  if (part_key!=NoExists)
  {
    PaxNormQry.SQLText=
      "SELECT arx_pax_norms.bag_type, arx_pax_norms.norm_id, arx_pax_norms.norm_trfer, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM arx_pax_norms,bag_norms "
      "WHERE arx_pax_norms.norm_id=bag_norms.id(+) AND "
      "      arx_pax_norms.part_key=:part_key AND "
      "      arx_pax_norms.pax_id=:pax_id "
      "UNION "
      "SELECT arx_pax_norms.bag_type, arx_pax_norms.norm_id, arx_pax_norms.norm_trfer, "
      "       arx_bag_norms.norm_type, arx_bag_norms.amount, arx_bag_norms.weight, arx_bag_norms.per_unit "
      "FROM arx_pax_norms,arx_bag_norms "
      "WHERE arx_pax_norms.norm_id=arx_bag_norms.id(+) AND "
      "      arx_pax_norms.part_key=:part_key AND "
      "      arx_pax_norms.pax_id=:pax_id "
      "ORDER BY bag_type, norm_type NULLS LAST";
    PaxNormQry.CreateVariable("part_key", otDate, part_key);
      
    GrpNormQry.SQLText=
      "SELECT arx_grp_norms.bag_type, arx_grp_norms.norm_id, arx_grp_norms.norm_trfer, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM arx_grp_norms,bag_norms "
      "WHERE arx_grp_norms.norm_id=bag_norms.id(+) AND "
      "      arx_grp_norms.part_key=:part_key AND "
      "      arx_grp_norms.grp_id=:grp_id "
      "UNION "
      "SELECT arx_grp_norms.bag_type, arx_grp_norms.norm_id, arx_grp_norms.norm_trfer, "
      "       arx_bag_norms.norm_type, arx_bag_norms.amount, arx_bag_norms.weight, arx_bag_norms.per_unit "
      "FROM arx_grp_norms,arx_bag_norms "
      "WHERE arx_grp_norms.norm_id=arx_bag_norms.id(+) AND "
      "      arx_grp_norms.part_key=:part_key AND "
      "      arx_grp_norms.grp_id=:grp_id "
      "ORDER BY bag_type, norm_type NULLS LAST";
    GrpNormQry.CreateVariable("part_key", otDate, part_key);
    
    BagQry.SQLText=
      "SELECT bag_type, SUM(amount) AS amount, SUM(weight) AS weight "
      "FROM arx_pax_grp,arx_bag2 "
      "WHERE arx_pax_grp.part_key=arx_bag2.part_key AND "
      "      arx_pax_grp.grp_id=arx_bag2.grp_id AND "
      "      arx_pax_grp.part_key=:part_key AND "
      "      arx_pax_grp.point_dep=:point_id AND "
      "      arx_pax_grp.grp_id=:grp_id AND "
      "      arch.bag_pool_refused(arx_bag2.part_key,arx_bag2.grp_id,arx_bag2.bag_pool_num,arx_pax_grp.class,arx_pax_grp.bag_refuse)=0 "
      "GROUP BY bag_type";
    BagQry.CreateVariable("part_key", otDate, part_key);
    BagQry.CreateVariable("point_id", otInteger, point_id);
    
    PaidQry.SQLText=
      "SELECT bag_type, weight FROM arx_paid_bag WHERE part_key=:part_key AND grp_id=:grp_id";
    PaidQry.CreateVariable("part_key", otDate, part_key);
  }
  else
  {
    PaxNormQry.SQLText=
      "SELECT pax_norms.bag_type, pax_norms.norm_id, pax_norms.norm_trfer, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM pax_norms,bag_norms "
      "WHERE pax_norms.norm_id=bag_norms.id(+) AND pax_norms.pax_id=:pax_id ";

    GrpNormQry.SQLText=
      "SELECT grp_norms.bag_type, grp_norms.norm_id, grp_norms.norm_trfer, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM grp_norms,bag_norms "
      "WHERE grp_norms.norm_id=bag_norms.id(+) AND grp_norms.grp_id=:grp_id ";
      
    BagQry.SQLText=
      "SELECT bag_type, SUM(amount) AS amount, SUM(weight) AS weight "
      "FROM pax_grp,bag2 "
      "WHERE pax_grp.grp_id=bag2.grp_id AND "
      "      pax_grp.grp_id=:grp_id AND "
      "      ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 "
      "GROUP BY bag_type";
      
    PaidQry.SQLText=
      "SELECT bag_type, weight FROM paid_bag WHERE grp_id=:grp_id";
  };
  PaxNormQry.DeclareVariable("pax_id", otInteger);
  GrpNormQry.DeclareVariable("grp_id", otInteger);
  BagQry.DeclareVariable("grp_id", otInteger);
  PaidQry.DeclareVariable("grp_id", otInteger);
    

  Qry.Clear();
  sql.str("");
  sql <<
    "SELECT pax_grp.grp_id, pax_grp.class, pax.pax_id, pax.surname, pax.name, "
    "       pax.refuse, pax.pr_brd, pax.reg_no, "
    "         ";
  if (part_key!=NoExists)
  {
    sql <<
      "arch.get_bagAmount2(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_amount, "
      "arch.get_bagWeight2(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_weight, "
      "arch.get_rkWeight2(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS rk_weight, "
      "arch.get_excess(pax_grp.part_key,pax_grp.grp_id,pax.pax_id) AS excess, "
      "arch.get_birks2(pax_grp.part_key,pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,'RU') AS tags, "
      "arch.get_main_pax_id2(pax_grp.part_key,pax_grp.grp_id) AS main_pax_id "
      "FROM arx_pax_grp pax_grp, arx_pax pax "
      "WHERE pax_grp.part_key=pax.part_key(+) AND "
      "      pax_grp.grp_id=pax.grp_id(+) AND "
      "      pax_grp.part_key=:part_key AND "
      "      pax_grp.point_dep=:point_id "
      "ORDER BY pax.reg_no NULLS LAST";
      Qry.CreateVariable("part_key", otDate, part_key);
  }
  else
  {
    sql <<
      "ckin.get_bagAmount2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_amount, "
      "ckin.get_bagWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_weight, "
      "ckin.get_rkWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS rk_weight, "
      "ckin.get_excess(pax_grp.grp_id,pax.pax_id) AS excess, "
      "ckin.get_birks2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,'RU') AS tags, "
      "ckin.get_main_pax_id2(pax_grp.grp_id) AS main_pax_id "
      "FROM pax_grp, pax "
      "WHERE pax_grp.grp_id=pax.grp_id(+) AND "
      "      pax_grp.point_dep=:point_id "
      "ORDER BY pax.reg_no NULLS LAST";
  };
  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    int grp_id=Qry.FieldAsInteger("grp_id");
    int pax_id=Qry.FieldIsNULL("pax_id")?NoExists:Qry.FieldAsInteger("pax_id");
    int main_pax_id=Qry.FieldIsNULL("main_pax_id")?NoExists:Qry.FieldAsInteger("main_pax_id");
    f << (operFlt.scd_out==NoExists?"":DateTimeToStr(UTCToLocal(operFlt.scd_out,region), "dd.mm.yyyy")) << ";"
      << operFlt.airline
      << setw(3) << setfill('0') << operFlt.flt_no
      << operFlt.suffix << ";";
    if (pax_id!=NoExists)
      f  << Qry.FieldAsString("surname") << "/" << Qry.FieldAsString("name") << ";";
    else
      f << "БАГАЖ БЕЗ СОПРОВОЖДЕНИЯ" << ";";
    f << grp_id << ";"
      << (Qry.FieldAsInteger("bag_amount")==0?"":IntToString(Qry.FieldAsInteger("bag_amount"))) << ";"
      << (Qry.FieldAsInteger("bag_weight")==0?"":IntToString(Qry.FieldAsInteger("bag_weight"))) << ";"
      << (Qry.FieldAsInteger("rk_weight")==0?"":IntToString(Qry.FieldAsInteger("rk_weight"))) << ";"
      << (Qry.FieldAsInteger("excess")==0?"":IntToString(Qry.FieldAsInteger("excess"))) << ";"
      << Qry.FieldAsString("tags") << ";";
    pair<TDateTime, TDateTime> times(NoExists, NoExists);
    TQuery &NormQry=pax_id==NoExists?GrpNormQry:PaxNormQry;
    if (pax_id!=NoExists)
    {
      f << ElemIdToNameLong(etRefusalType, Qry.FieldAsString("refuse")) << ";"
        << (Qry.FieldIsNULL("refuse")?(Qry.FieldAsInteger("pr_brd")==0?"зарегистрирован":"прошел посадку"):"разрегистрирован") << ";"
        << Qry.FieldAsInteger("reg_no") << ";";

      map< pair<int, int>, pair<TDateTime, TDateTime> >::const_iterator i=events.find(make_pair(grp_id,Qry.FieldAsInteger("reg_no")));
      if (i!=events.end()) times=i->second;
      if (!Qry.FieldIsNULL("refuse") || Qry.FieldAsInteger("pr_brd")==0) times.second=NoExists;
      
      NormQry.SetVariable("pax_id", pax_id);
    }
    else
    {
      f << ";;;";
      map< pair<int, int>, pair<TDateTime, TDateTime> >::const_iterator i=events.find(make_pair(grp_id,NoExists));
      if (i!=events.end()) times=i->second;
      
      NormQry.SetVariable("grp_id", grp_id);
    };
    f << (times.first==NoExists?"":DateTimeToStr(UTCToLocal(times.first,region), "dd.mm.yyyy hh:nn")) << ";;"
      << (times.second==NoExists?"":DateTimeToStr(UTCToLocal(times.second,region), "dd.mm.yyyy hh:nn")) << ";"
      << (operFlt.scd_out==NoExists?"":DateTimeToStr(UTCToLocal(operFlt.scd_out,region), "dd.mm.yyyy hh:nn")) << ";"
      << (operFlt.real_out==NoExists?"":DateTimeToStr(UTCToLocal(operFlt.real_out,region), "dd.mm.yyyy hh:nn")) << ";";

    std::map< int/*bag_type*/, CheckIn::TNormItem> norms;
    NormQry.Execute();
    int prior_bag_type=NoExists;
    for(;!NormQry.Eof;NormQry.Next())
    {
      CheckIn::TPaxNormItem paxNormItem;
      CheckIn::TNormItem normItem;
      paxNormItem.fromDB(NormQry);
      normItem.fromDB(NormQry);
      
      int bag_type=paxNormItem.bag_type==NoExists?-1:paxNormItem.bag_type;
      
      if (prior_bag_type==bag_type) continue;
      prior_bag_type=bag_type;
      if (normItem.empty())
      {
        if (paxNormItem.empty()) continue;
        if (pax_id!=NoExists)
          printf("norm not found norm_id=%s part_key=%s pax_id=%d\n",
                 paxNormItem.norm_id==NoExists?"NoExists":IntToString(paxNormItem.norm_id).c_str(),
                 part_key==NoExists?"NoExists":DateTimeToStr(part_key,"dd.mm.yy hh:nn:ss").c_str(),
                 pax_id);
        else
          printf("norm not found norm_id=%s part_key=%s grp_id=%d\n",
                 paxNormItem.norm_id==NoExists?"NoExists":IntToString(paxNormItem.norm_id).c_str(),
                 part_key==NoExists?"NoExists":DateTimeToStr(part_key,"dd.mm.yy hh:nn:ss").c_str(),
                 grp_id);
        continue;
      };

      norms[bag_type]=normItem;
    };

    std::map< int/*bag_type*/, CheckIn::TNormItem>::const_iterator n=norms.begin();
    for(;n!=norms.end();++n)
    {
      if (n!=norms.begin()) f << ", ";
      if (n->first!=-1) f << setw(2) << setfill('0') << n->first << ": ";
      f << n->second.str();
    };
    
    f << ";";
    
    if (pax_id==NoExists || pax_id==main_pax_id)
    {
      map< int/*bag_type*/, TPaidToLogInfo> paid;
    
      BagQry.SetVariable("grp_id",grp_id);
      BagQry.Execute();
      if (!BagQry.Eof)
      {
        //багаж есть
        for(;!BagQry.Eof;BagQry.Next())
        {
          int bag_type=BagQry.FieldIsNULL("bag_type")?-1:BagQry.FieldAsInteger("bag_type");

          std::map< int/*bag_type*/, TPaidToLogInfo>::iterator i=paid.find(bag_type);
          if (i!=paid.end())
          {
            i->second.bag_amount+=BagQry.FieldAsInteger("amount");
            i->second.bag_weight+=BagQry.FieldAsInteger("weight");
          }
          else
          {
            TPaidToLogInfo &paidInfo=paid[bag_type];
            paidInfo.bag_type=bag_type;
            paidInfo.bag_amount=BagQry.FieldAsInteger("amount");
            paidInfo.bag_weight=BagQry.FieldAsInteger("weight");
          };
        };

        PaidQry.SetVariable("grp_id",grp_id);
        PaidQry.Execute();
        for(;!PaidQry.Eof;PaidQry.Next())
        {
          int bag_type=PaidQry.FieldIsNULL("bag_type")?-1:PaidQry.FieldAsInteger("bag_type");
          TPaidToLogInfo &paidInfo=paid[bag_type];
          paidInfo.bag_type=bag_type;
          paidInfo.paid_weight=PaidQry.FieldAsInteger("weight");
        };
        
        map< int, TPaidToLogInfo>::const_iterator p=paid.begin();
        for(;p!=paid.end();++p)
        {
          if (p!=paid.begin()) f << ", ";
          if (p->second.bag_type!=-1) f << setw(2) << setfill('0') << p->second.bag_type << ":";
          f << p->second.bag_amount << "/" << p->second.bag_weight << "/" << p->second.paid_weight;
        };
      };
    };
    
    f << ";";
    f << ElemIdToNameLong(etClass, Qry.FieldAsString("class"));
    f << endl;
  };
};

int create_tlg(int argc,char **argv)
{
  TCreateTlgInfo tlgInfo;
  tlgInfo.type="PTMN";
  tlgInfo.point_id=3229569;
  tlgInfo.airp_trfer="ДМД";
  tlgInfo.pr_lat=true;
  tlgInfo.addrs="MOWKB1H";
  TelegramInterface::create_tlg(tlgInfo);
  return 1;
};

int season_to_schedules(int argc,char **argv)
{
  //!!!TDateTime first_date = NowUTC()-3000;
  TDateTime first_date = NowUTC()-500;
  TDateTime last_date = NowUTC() + 750;
  try {
//    ConvertSeason( first_date, last_date );
  }
  catch(EXCEPTIONS::Exception e){
    ProgError( STDLOG,"EXCEPTIONS::Exception, what=%s", e.what() );
  }
  catch(...){
    ProgError( STDLOG,"unknown error" );
  }
  return 0;
}


