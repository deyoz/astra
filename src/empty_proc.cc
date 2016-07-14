//---------------------------------------------------------------------------
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
#include "sopp.h"
#include "file_queue.h"
#include "events.h"
#include "transfer.h"
#include "term_version.h"
#include "typeb_utils.h"
#include "misc.h"
#include "apis.h"
#include "oralib.h"
#include "httpClient.h"
#include "web_main.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace ASTRA;
using namespace BASIC;
using namespace std;

bool alter_wait(int processed, bool commit_before_sleep=false, int work_secs=5, int sleep_secs=5)
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
    return true;
  };
  return false;
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
    "SELECT :low_date, type, COUNT(*) FROM events_bilingual "
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
    "SELECT type, time, ev_order, msg, screen, ev_user, station, id1, id2, id3 FROM events_bilingual "
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
          "      pax_grp.status NOT IN ('E') AND "
          "      pax.pr_brd IS NOT NULL ";
      }
      PaxQry.DeclareVariable("point_dep", otInteger);
      PaxQry.DeclareVariable("point_arv", otInteger);

      TQuery EventsQry(&OraSession);
      EventsQry.Clear();
      if (pass==1)
      {
        if(ARX_EVENTS_DISABLED())
          throw AstraLocale::UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
        EventsQry.SQLText=
          "SELECT MIN(time) AS time FROM arx_events "
          "WHERE part_key=:part_key AND lang=:lang AND type=:evtPax AND id1=:point_dep AND id2=:reg_no";
        EventsQry.DeclareVariable("part_key", otDate);
      }
      else
      {
        EventsQry.SQLText=
          "SELECT MIN(time) AS time FROM events_bilingual "
          "WHERE lang=:lang AND type=:evtPax AND id1=:point_dep AND id2=:reg_no";
      };
      EventsQry.CreateVariable("lang", otString, AstraLocale::LANG_RU);
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
      printf("point_ids.size()=%zu\n", point_ids.size());
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
                LoadPaxDoc(r1->part_key, pax_id, doc);

                CheckIn::TPaxDocoItem doco;
                LoadPaxDoco(r1->part_key, pax_id, doco);

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

int create_tlg(int argc,char **argv)
{
/* !!!vlad
  TCreateTlgInfo tlgInfo;
  tlgInfo.type="PTMN";
  tlgInfo.point_id=3229569;
  tlgInfo.airp_trfer="ДМД";
  tlgInfo.pr_lat=true;
  tlgInfo.addrs.addrs="MOWKB1H";
  TelegramInterface::create_tlg(tlgInfo);
*/
/*
  TypeB::TestBSMElemOrder("IFNNPOOEEW");
  TypeB::TestBSMElemOrder("IFNPPOOEEW");
  TypeB::TestBSMElemOrder("IFFNNPPOEW");
  TypeB::TestBSMElemOrder("IFFNNPPOEV");
*/
  return 1;
};

int test_trfer_exists(int argc,char **argv)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize("МОВ");

  TDateTime first_date, last_date;
  StrToDateTime("26.05.2013 00:00:00","dd.mm.yyyy hh:nn:ss",first_date);
  StrToDateTime("31.05.2013 00:00:00","dd.mm.yyyy hh:nn:ss",last_date);

  TQuery FltQry(&OraSession);
  FltQry.Clear();
  FltQry.SQLText=
    "SELECT point_id, airline, flt_no, suffix, scd_out, airp "
    "FROM points WHERE move_id=:move_id AND pr_del>=0 ORDER BY point_num ";
  FltQry.DeclareVariable("move_id", otInteger);

  TQuery ExistsQry(&OraSession);

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT DISTINCT move_id FROM points "
    "WHERE (scd_out>=:first_date AND scd_out<:last_date OR "
    "       scd_in>=:first_date AND scd_in<:last_date)/* AND move_id IN (1236392, 1237032)*/";
  Qry.CreateVariable("first_date", otDate, first_date);
  Qry.CreateVariable("last_date", otDate, last_date);
  Qry.Execute();
  int compared=0;
  int different=0;
  int errors=0;
  for(;!Qry.Eof;Qry.Next())
  {
    int move_id=Qry.FieldAsInteger("move_id");
    FltQry.SetVariable("move_id", move_id);
    FltQry.Execute();
    for(;!FltQry.Eof;)
    {
      int point_id_out=FltQry.FieldAsInteger("point_id");
      TTripInfo flt(FltQry);
      FltQry.Next();
      if (FltQry.Eof) break;
      int point_id_in=FltQry.FieldAsInteger("point_id");
      for(int pass=0; pass<4; pass++)
      {
        int point_id=NoExists;
        TrferList::TTrferType trferType = TrferList::tckinInbound;
        string title;
        bool exists=false;
        try
        {
          if (pass==0)
          {
            title="TrferList::tckinInbound";
            trferType=TrferList::tckinInbound;
            point_id=point_id_out;
            continue;
          };
          if (pass==1)
          {
            title="TrferList::trferCkin";
            trferType=TrferList::trferCkin;
            point_id=point_id_out;
            exists=TrferList::trferCkinExists(point_id_out, ExistsQry);
          };
          if (pass==2)
          {
            title="TrferList::trferIn";
            trferType=TrferList::trferIn;
            point_id=point_id_in;
            exists=TrferList::trferInExists(point_id_in, point_id_out, ExistsQry);
          };
          if (pass==3)
          {
            title="TrferList::trferOut";
            trferType=TrferList::trferOut;
            point_id=point_id_out;
            exists=TrferList::trferOutExists(point_id_out, ExistsQry);
          };

          TTripInfo flt_tmp;
          vector<TrferList::TGrpItem> grps_ckin;
          vector<TrferList::TGrpItem> grps_tlg;
          TrferList::TrferFromDB(trferType, point_id, true, flt_tmp, grps_ckin, grps_tlg);

          compared++;
          if (exists!=(!grps_ckin.empty() || !grps_tlg.empty()))
          {
            different++;
            printf("%s: exists=%d grps_ckin.size()=%zu grps_tlg.size()=%zu point_id=%d\n",
                   title.c_str(), (int)exists, grps_ckin.size(), grps_tlg.size(), point_id);
          };
        }
        catch(EXCEPTIONS::Exception &e)
        {
          errors++;
          ProgError(STDLOG, "test_trfer_exists: %s (title=%s point_id=%d)", e.what(), title.c_str(), point_id );
        };
      };
    };
  };
  printf("compared=%d different=%d errors=%d\n", compared, different, errors);

  return 1;
};

int bind_trfer_trips(int argc,char **argv)
{
  TQuery Qry(&OraSession);

  Qry.Clear();
  Qry.SQLText="CREATE INDEX trfer_trips_tmp__IDX ON trfer_trips (scd ASC)";
  Qry.Execute();

  Qry.Clear();
  Qry.SQLText="SELECT MIN(scd) AS min_date FROM trfer_trips";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_date")) return 0;
  TDateTime min_date=Qry.FieldAsDateTime("min_date");

  Qry.Clear();
  Qry.SQLText="SELECT MAX(scd) AS max_date FROM trfer_trips";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_date")) return 0;
  TDateTime max_date=Qry.FieldAsDateTime("max_date");

  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id FROM trfer_trips "
    "WHERE point_id_spp IS NULL AND scd>=:low_date AND scd<:high_date";
  Qry.DeclareVariable("low_date", otDate);
  Qry.DeclareVariable("high_date", otDate);

  TTrferBinding trferBinding(false);

  for(TDateTime curr_date=min_date; curr_date<=max_date; curr_date+=1.0)
  {
    Qry.SetVariable("low_date",curr_date);
    Qry.SetVariable("high_date",curr_date+1.0);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
      trferBinding.bind_flt(Qry.FieldAsInteger("point_id"));
    OraSession.Commit();
  };

  Qry.Clear();
  Qry.SQLText="DROP INDEX trfer_trips_tmp__IDX";
  Qry.Execute();

  return 0;
};

int unbind_trfer_trips(int argc,char **argv)
{
  TQuery Qry(&OraSession);

  Qry.Clear();
  Qry.SQLText="CREATE INDEX trfer_trips_tmp__IDX ON trfer_trips (scd ASC)";
  Qry.Execute();

  Qry.Clear();
  Qry.SQLText="SELECT MIN(scd) AS min_date FROM trfer_trips";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_date")) return 0;
  TDateTime min_date=Qry.FieldAsDateTime("min_date");

  Qry.Clear();
  Qry.SQLText="SELECT MAX(scd) AS max_date FROM trfer_trips";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_date")) return 0;
  TDateTime max_date=Qry.FieldAsDateTime("max_date");

  Qry.Clear();
  Qry.SQLText=
    "UPDATE trfer_trips SET point_id_spp=NULL "
    "WHERE point_id_spp IS NOT NULL AND scd>=:low_date AND scd<:high_date";
  Qry.DeclareVariable("low_date", otDate);
  Qry.DeclareVariable("high_date", otDate);

  for(TDateTime curr_date=min_date; curr_date<=max_date; curr_date+=1.0)
  {
    Qry.SetVariable("low_date",curr_date);
    Qry.SetVariable("high_date",curr_date+1.0);
    Qry.Execute();
    OraSession.Commit();
  };

  Qry.Clear();
  Qry.SQLText="DROP INDEX trfer_trips_tmp__IDX";
  Qry.Execute();

  return 0;
};

int season_to_schedules(int argc,char **argv)
{
  //!!!TDateTime first_date = NowUTC()-3000;
  //BASIC::TDateTime first_date = NowUTC()-500;
  //  BASIC::TDateTime last_date = NowUTC() + 750;
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
/*
CREATE TABLE drop_test_typeb_utils1
(
  point_id NUMBER(9) NOT NULL,
  tlg_id NUMBER(9) NOT NULL,
  type VARCHAR2(6) NOT NULL,
  addr VARCHAR2(1000) NULL,
  addr_normal VARCHAR2(1000) NULL,
  pr_lat NUMBER(1) NOT NULL,
  extra VARCHAR2(50) NULL
);

CREATE TABLE drop_test_typeb_utils2
(
  point_id NUMBER(9) NOT NULL,
  type VARCHAR2(6) NOT NULL,
  addr VARCHAR2(1000) NULL,
  addr_normal VARCHAR2(1000) NULL,
  pr_lat NUMBER(1) NOT NULL,
  extra VARCHAR2(250) NULL
);

CREATE UNIQUE INDEX drop_test_typeb_utils1__IDX ON drop_test_typeb_utils1(tlg_id);

select count(*) from
(
SELECT point_id, type, addr_normal, pr_lat FROM drop_test_typeb_utils1 WHERE type IN
('PTM','PTMN','BTM', 'TPM', 'PSM' , 'PFS', 'PFSN' , 'FTL' , 'PRL', 'PIM', 'SOM', 'ETLD', 'LDM', 'CPM', 'MVTA')
MINUS
SELECT point_id, type, addr_normal, pr_lat FROM drop_test_typeb_utils2
ORDER BY type
);

select count(*) from
(
SELECT point_id, type, addr_normal, pr_lat FROM drop_test_typeb_utils2
MINUS
SELECT point_id, type, addr_normal, pr_lat FROM drop_test_typeb_utils1 WHERE type IN
('PTM','PTMN','BTM', 'TPM', 'PSM' , 'PFS', 'PFSN' , 'FTL' , 'PRL', 'PIM', 'SOM', 'ETLD', 'LDM', 'CPM', 'MVTA')
ORDER BY type
);

*/
int test_typeb_utils2(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="DELETE FROM drop_test_typeb_utils1";
  Qry.Execute();

  Qry.Clear();
  Qry.SQLText="DELETE FROM drop_test_typeb_utils2";
  Qry.Execute();

  OraSession.Commit();

  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,flt_no,suffix,airp,scd_out,act_out, "
    "       point_id,point_num,first_point,pr_tranzit "
    "FROM points "
//    "WHERE point_id=3977047";
    "WHERE scd_out BETWEEN TRUNC(SYSDATE-1) AND TRUNC(SYSDATE) AND act_out IS NOT NULL AND pr_del=0";

  TQuery TlgQry(&OraSession);
  TlgQry.Clear();
  TlgQry.SQLText=
    "INSERT INTO drop_test_typeb_utils1(point_id, tlg_id, type, addr, pr_lat, extra) "
    "SELECT tlg_out.point_id, tlg_out.id AS tlg_id, tlg_out.type, tlg_out.addr, tlg_out.pr_lat, tlg_out.extra "
    "FROM tlg_out "
    "WHERE tlg_out.point_id=:point_id AND tlg_out.num=1 ";
    //"   AND tlg_out.time_create BETWEEN :act_out-1/1440 AND :act_out+1/1440";
  TlgQry.DeclareVariable("point_id", otInteger);
  //TlgQry.DeclareVariable("act_out", otDate);

  TQuery Tlg2Qry(&OraSession);
  Tlg2Qry.Clear();
  Tlg2Qry.SQLText=
    "INSERT INTO drop_test_typeb_utils2(point_id, type, addr, addr_normal, pr_lat, extra) "
    "VALUES(:point_id, :type, :addr, :addr, :pr_lat, :extra) ";
  Tlg2Qry.DeclareVariable("point_id", otInteger);
  Tlg2Qry.DeclareVariable("type", otString);
  Tlg2Qry.DeclareVariable("addr", otString);
  Tlg2Qry.DeclareVariable("pr_lat", otInteger);
  Tlg2Qry.DeclareVariable("extra", otString);


  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    try
    {
      TAdvTripInfo fltInfo(Qry);
      TlgQry.SetVariable("point_id", fltInfo.point_id);
      //TlgQry.SetVariable("act_out", Qry.FieldAsDateTime("act_out"));
      TlgQry.Execute();

      Tlg2Qry.SetVariable("point_id", fltInfo.point_id);
      TypeB::TTakeoffCreator creator(fltInfo.point_id);
      creator << "MVTA";
      vector<TypeB::TCreateInfo> createInfo;
      creator.getInfo(createInfo);
      for(vector<TypeB::TCreateInfo>::const_iterator i=createInfo.begin(); i!=createInfo.end(); ++i)
      {
  /*      ProgTrace(TRACE5, "point_id=%d tlg_type=%s addr=%s",
                          i->point_id, i->get_tlg_type().c_str(), i->get_addrs().c_str());*/

          //if (!TypeB::TSendInfo(i->get_tlg_type(), fltInfo).isSend()) continue;

          Tlg2Qry.SetVariable("type", i->get_tlg_type());
          Tlg2Qry.SetVariable("addr", i->get_addrs());
          Tlg2Qry.SetVariable("pr_lat", (int)i->get_options().is_lat);
          localizedstream extra(AstraLocale::LANG_RU);
          Tlg2Qry.SetVariable("extra", i->get_options().extraStr(extra).str());
          Tlg2Qry.Execute();
      };
    }
    catch(...)
    {
      OraSession.Rollback();
      ProgTrace(TRACE5, "ERROR! point_id=%d", Qry.FieldAsInteger("point_id"));
    };
    OraSession.Commit();
  };

  Qry.Clear();
  Qry.SQLText="SELECT tlg_id, addr FROM drop_test_typeb_utils1";
  Qry.Execute();

  TlgQry.Clear();
  TlgQry.SQLText="UPDATE drop_test_typeb_utils1 SET addr_normal=:addr_normal WHERE tlg_id=:tlg_id";
  TlgQry.DeclareVariable("addr_normal", otString);
  TlgQry.DeclareVariable("tlg_id", otInteger);
  TypeB::TCreateInfo ci;
  for(;!Qry.Eof;Qry.Next())
  {
    ci.set_addrs(Qry.FieldAsString("addr"));
    TlgQry.SetVariable("tlg_id", Qry.FieldAsInteger("tlg_id"));
    TlgQry.SetVariable("addr_normal", ci.get_addrs());
    TlgQry.Execute();
  };

  OraSession.Commit();

  return 0;
};

void filter(vector<TypeB::TCreateInfo> &createInfo, set<string> tlg_types)
{
  if ( tlg_types.empty() ) {
    return;
  }
    while(true) {
        vector<TypeB::TCreateInfo>::iterator iv = createInfo.begin();
        for(; iv != createInfo.end(); iv++)
            if( tlg_types.find(iv->get_tlg_type()) == tlg_types.end())
                break;
        if(iv != createInfo.end())
            createInfo.erase(iv);
        else
            break;
    }
};


/*
void filter(vector<TypeB::TCreateInfo> &createInfo, string tlg_type)
{
    while(true) {
        vector<TypeB::TCreateInfo>::iterator iv = createInfo.begin();
        for(; iv != createInfo.end(); iv++)
            if(iv->get_tlg_type() != tlg_type)
                break;
        if(iv != createInfo.end())
            createInfo.erase(iv);
        else
            break;
    }
}
*/
/*
CREATE OR REPLACE
FUNCTION pax_is_female(vpax_id pax.pax_id%TYPE) RETURN NUMBER
IS
result NUMBER(1);
BEGIN
  SELECT DECODE(gender,'F',1,'FI',1,'M',0,'MI',0,NULL)
  INTO result
  FROM pax_doc
  WHERE pax_id=vpax_id;
  RETURN result;
EXCEPTION
  WHEN NO_DATA_FOUND THEN RETURN NULL;
END pax_is_female;
/
show error

DROP FUNCTION pax_is_female;
*/
int test_typeb_utils(int argc,char **argv)
{
    string interval = "SYSTEM.UTCSYSDATE-24/24 AND SYSTEM.UTCSYSDATE";
    if(argc == 2) interval = argv[1];
    if(argc > 2) {
        cout << "Usage: " << argv[0] << " \"" << interval << "\"" << endl;
        return 1;
    }

    set<string> tlg_types;
    tlg_types.insert("PRL");
    tlg_types.insert("ETL");
    tlg_types.insert("FTL");
    tlg_types.insert("PSM");
    tlg_types.insert("PIL");
    tlg_types.insert("PFS");
    //  tlg_types.insert("LCI");
    //  tlg_types.insert("COM");
    //  tlg_types.insert("PRL");
    //  tlg_types.insert("PRLC");
    //  tlg_types.insert("PSM");
    //  tlg_types.insert("PIL");
    //  tlg_types.insert("SOM");
    TQuery Qry(&OraSession);
    /*  Qry.SQLText =
        "INSERT INTO tranzit_algo_seats(id,airline,flt_no,airp,pr_new) "
        "SELECT 1,NULL,NULL,NULL,1 FROM dual";
        Qry.Execute();*/
    ofstream f1, f2;
    try
    {
        Qry.Clear();
        string SQLText = (string)
            "SELECT airline,flt_no,suffix,airp,scd_out,act_out, "
            "       point_id,point_num,first_point,pr_tranzit "
            "FROM points "
            //      "WHERE point_id=2253498";
            "WHERE scd_out BETWEEN " + interval + " AND act_out IS NOT NULL AND pr_del=0";

        Qry.SQLText= SQLText;
        TQuery TlgQry(&OraSession);
        TlgQry.Clear();
        string sql =
            "SELECT * "
            "FROM tlg_out, typeb_out_extra "
            "WHERE point_id=:point_id AND manual_creation=0 AND "
            "      tlg_out.id=typeb_out_extra.tlg_id(+) AND typeb_out_extra.lang(+)='EN' ";

        if ( !tlg_types.empty() ) {
            sql += " and type in " + GetSQLEnum(tlg_types);
        }
        sql += " ORDER BY type, typeb_out_extra.text, addr, id, num";
        TlgQry.SQLText=sql;
        TlgQry.DeclareVariable("point_id", otInteger);

        TQuery OrigQry(&OraSession);
        OrigQry.Clear();
        OrigQry.SQLText="SELECT * FROM typeb_originators WHERE id=:id";
        OrigQry.DeclareVariable("id", otInteger);

        string file_name;
        file_name="telegram1.txt";
        f1.open(file_name.c_str());
        if (!f1.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());
        file_name="telegram2.txt";
        f2.open(file_name.c_str());
        if (!f2.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());

        Qry.Execute();
        for(;!Qry.Eof;Qry.Next())
        {
            TDateTime time_create=NowUTC();
            TAdvTripInfo fltInfo(Qry);


            set<int> tlg_ids;

            for(int pass=0; pass<=1; pass++)
            {
                TlgQry.SetVariable("point_id", fltInfo.point_id);
                TlgQry.Execute();
                if (!TlgQry.Eof)
                {
                    string body;
                    for(;!TlgQry.Eof;)
                    {
                        TTlgOutPartInfo tlg;
                        tlg.fromDB(TlgQry);
                        //originator
                        TypeB::TOriginatorInfo origInfo;
                        OrigQry.SetVariable("id", tlg.originator_id);
                        OrigQry.Execute();
                        if (!OrigQry.Eof)
                            origInfo.fromDB(OrigQry);
                        tlg.origin=origInfo.originSection(time_create, TypeB::endl);

                        body+=tlg.body;
                        TlgQry.Next();
                        if (TlgQry.Eof || tlg.id!=TlgQry.FieldAsInteger("id"))
                        {
                            if (tlg_ids.find(tlg.id)==tlg_ids.end())
                            {
                                if (tlg.tlg_type!="BSM")
                                {
                                    ofstream &f=(pass==0?f1:f2);
                                    f << ConvertCodepage(tlg.addr, "CP866", "CP1251")
                                        << ConvertCodepage(tlg.origin, "CP866", "CP1251")
                                        << ConvertCodepage(tlg.heading, "CP866", "CP1251")
                                        << ConvertCodepage(body, "CP866", "CP1251")
                                        << ConvertCodepage(tlg.ending, "CP866", "CP1251")
                                        << "====================================================" << TypeB::endl;
                                };
                                tlg_ids.insert(tlg.id);
                            };
                            body.clear();
                        };
                    };

                };


                if (pass==0)
                {
                    vector<TypeB::TCreateInfo> createInfo;
                    TypeB::TTakeoffCreator(fltInfo.point_id).getInfo(createInfo);
                    filter(createInfo, tlg_types);
                    TelegramInterface::SendTlg(createInfo);

                    TypeB::TMVTACreator(fltInfo.point_id).getInfo(createInfo);
                    filter(createInfo, tlg_types);
                    TelegramInterface::SendTlg(createInfo);

                    TypeB::TCloseCheckInCreator(fltInfo.point_id).getInfo(createInfo);
                    filter(createInfo, tlg_types);
                    TelegramInterface::SendTlg(createInfo);

                    TypeB::TCloseBoardingCreator(fltInfo.point_id).getInfo(createInfo);
                    filter(createInfo, tlg_types);
                    TelegramInterface::SendTlg(createInfo);
                };
            };


            OraSession.Rollback();
        };
        OraSession.Rollback();
        if (f1.is_open()) f1.close();
        if (f2.is_open()) f2.close();
    }
    catch(EXCEPTIONS::Exception &e)
    {
        try
        {
            OraSession.Rollback();
            if (f1.is_open()) f1.close();
            if (f2.is_open()) f2.close();
            ProgError(STDLOG, "test_typeb_utils2: %s", e.what() );
        }
        catch(...) {};
        throw;
    }
    catch(...)
    {
        try
        {
            OraSession.Rollback();
            if (f1.is_open()) f1.close();
            if (f2.is_open()) f2.close();
        }
        catch(...) {};
        throw;
    };

    return 0;
};

#include <sys/types.h>
#include <dirent.h>

class TAPISFlight
{
  public:
    string airline;
    int flt_no;
    string suffix;
    string airp_dep;
    TDateTime scd_out_local;

    TAPISFlight()
    {
      flt_no=ASTRA::NoExists;
      scd_out_local=ASTRA::NoExists;
    };
    bool operator < (const TAPISFlight &item) const
    {
      if (airline!=item.airline)
        return airline<item.airline;
      if (flt_no!=item.flt_no)
        return flt_no<item.flt_no;
      if (suffix!=item.suffix)
        return suffix<item.suffix;
      if (airp_dep!=item.airp_dep)
        return airp_dep<item.airp_dep;
      return scd_out_local<item.scd_out_local;
    };
};

int compare_apis(int argc,char **argv)
{
  if (argc<2 || strcmp(argv[1],"")==0)
  {
    printf("dir not specified\n");
    return 0;
  };
  DIR *dirp=opendir(argv[1]);
  if (dirp==NULL)
  {
    printf("dir not opened\n");
    return 0;
  };

  if (edifact::init_edifact()<0)
  {
    printf("'init_edifact' error");
    return 0;
  };

  set<TAPISFlight> flts;

  try
  {

    struct dirent *dp;
    while ((dp = readdir (dirp)) != NULL)
    {

      if (strlen(dp->d_name)<3) continue;

      int res;
      char c;
      char airline[4];
      long unsigned int flt_no;
      char suffix[2];
      suffix[0]=0;
      char airp_dep[4];
      char airp_arv[4];
      char scd_out_local[9];
      if (IsDigit(dp->d_name[2]))
      {
        res=sscanf(dp->d_name,
                   "%2[A-ZА-ЯЁ0-9]%5lu%3[A-ZА-ЯЁ]%3[A-ZА-ЯЁ]%8[0-9]%c",
                   airline, &flt_no, airp_dep, airp_arv, scd_out_local, &c);
        if (res!=6 || IsDigitIsLetter(c))
        {
          res=sscanf(dp->d_name,
                     "%2[A-ZА-ЯЁ0-9]%5lu%1[A-ZА-ЯЁ]%3[A-ZА-ЯЁ]%3[A-ZА-ЯЁ]%8[0-9]%c",
                     airline, &flt_no, suffix, airp_dep, airp_arv, scd_out_local, &c);
          if (res!=7 || IsDigitIsLetter(c)) continue;
        };
      }
      else
      {
        res=sscanf(dp->d_name,
                   "%3[A-ZА-ЯЁ0-9]%5lu%3[A-ZА-ЯЁ]%3[A-ZА-ЯЁ]%8[0-9]%c",
                   airline, &flt_no, airp_dep, airp_arv, scd_out_local, &c);
        if (res!=6 || IsDigitIsLetter(c))
        {
          res=sscanf(dp->d_name,
                     "%3[A-ZА-ЯЁ0-9]%5lu%1[A-ZА-ЯЁ]%3[A-ZА-ЯЁ]%3[A-ZА-ЯЁ]%8[0-9]%c",
                     airline, &flt_no, suffix, airp_dep, airp_arv, scd_out_local, &c);
          if (res!=7 || IsDigitIsLetter(c)) continue;
        };
      };

      TElemFmt fmt=efmtUnknown;
      TAPISFlight flt;
      flt.airline=ElemToElemId(etAirline, airline, fmt);
      if (fmt==efmtUnknown) continue;
      flt.flt_no=flt_no;
      if (suffix[0]!=0)
      {
        flt.suffix=ElemToElemId(etSuffix, suffix, fmt);
        if (fmt==efmtUnknown) continue;
      };
      flt.airp_dep=ElemToElemId(etAirp, airp_dep, fmt);
      if (fmt==efmtUnknown) continue;
      if (BASIC::StrToDateTime(scd_out_local, "yyyymmdd", flt.scd_out_local )==EOF) continue;

      if (flt.scd_out_local<NowUTC()-7) continue;

      flts.insert(flt);
    };
    closedir (dirp);
  }
  catch(...)
  {
    closedir (dirp);
  };

  set<int> point_ids;
  for(set<TAPISFlight>::const_iterator i=flts.begin(); i!=flts.end(); ++i)
  {
    ostringstream s;
    s << i->airline << "|"
      << i->flt_no << "|"
      << i->suffix << "|"
      << i->airp_dep << "|"
      << DateTimeToStr(i->scd_out_local, "dd.mm.yyyy") << "|";

    //printf("%s", s.str().c_str());
    TSearchFltInfo filter;
    filter.airline=i->airline;
    filter.flt_no=i->flt_no;
    filter.suffix=i->suffix;
    filter.airp_dep=i->airp_dep;
    filter.scd_out=i->scd_out_local;
    filter.scd_out_in_utc=false;
    filter.only_with_reg=true;

    list<TAdvTripInfo> flts;
    SearchFlt(filter, flts);

    for(list<TAdvTripInfo>::const_iterator f=flts.begin(); f!=flts.end(); ++f)
    {
      point_ids.insert(f->point_id);
    };
  };

  for(set<int>::const_iterator i=point_ids.begin(); i!=point_ids.end(); ++i)
  {
    OraSession.Rollback();
    try
    {
      create_apis_file(*i, "");
      //printf("%d\n", *i);
    }
    catch(EXCEPTIONS::Exception &E) {ProgError(STDLOG, "exception: %s", E.what());}
    catch(...) {ProgError(STDLOG, "unknown error");};
  };

  OraSession.Rollback();

  return 0;
};

int test_sopp_sql(int argc,char **argv)
{
  boost::posix_time::ptime mcsTime;
  long int delta_exec_old=0, delta_exec_new=0, delta_sql_old=0, delta_sql_new=0;
  long int exec_old=0, exec_new=0, exec_old_sql=0, exec_new_sql=0;
  InitLogTime(argc>0?argv[0]:NULL);
  ofstream f1, f2;
  string file_name;
  file_name="sopp_test_old.txt";
  f1.open(file_name.c_str());
  if (!f1.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());
  file_name="sopp_test_new.txt";
  f2.open(file_name.c_str());
  if (!f2.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());
  int file_count=0, file_size=0;
  //запрос для нового и старого обработчика, далее сравнения результатов
  int step=0, stepcount=0;
  TReqInfo *reqInfo = TReqInfo::Instance();
  for ( TDateTime day=NowUTC()-3.0; day<=NowUTC()+3.0; day++ ) {
  for ( int iuser_type=0; iuser_type<3; iuser_type++ ) {
    for ( int itime_type=0; itime_type<3; itime_type++ ) {
      for ( int icond=0; icond<=11; icond++ ) {
        reqInfo->Initialize("МОВ");
        switch( itime_type ) {
          case 0:
            reqInfo->user.sets.time = ustTimeLocalDesk;
            break;
          case 1:
            reqInfo->user.sets.time = ustTimeLocalAirp;
            break;
          case 2:
            reqInfo->user.sets.time = ustTimeUTC;
            break;
        }
        switch( iuser_type ) {
          case 0:
            reqInfo->user.user_type = utSupport;
            break;
          case 1:
            reqInfo->user.user_type = utAirline;
            break;
          case 2:
            reqInfo->user.user_type = utAirport;
            break;
        }
        TAccessElems<string> airps, airlines;
        switch( icond ) {
          case 0:
            airps.set_elems_permit(true);
            airlines.set_elems_permit(true);
            break;
          case 1:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(true);
            break;
          case 2:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(false);
            break;
          case 3:
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(true);
            break;
          case 4:
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(false);
            break;
          case 5:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(true);
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(true);
            break;
          case 6:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(false);
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(true);
            break;
          case 7:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(true);
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(false);
            break;
          case 8:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(false);
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(false);
            break;
          case 9:
            airps.add_elem( "ВНК" );
            airps.add_elem( "СОЧ" );
            airps.add_elem( "BBU" );
            airps.set_elems_permit(true);
            break;
          case 10:
            airlines.add_elem( "ЮТ" );
            airlines.add_elem( "ПО" );
            airlines.add_elem( "НН" );
            airlines.set_elems_permit(true);
            break;
          case 11:
            airps.add_elem( "ВНК" );
            airps.add_elem( "СОЧ" );
            airps.add_elem( "BBU" );
            airps.set_elems_permit(true);
            airlines.add_elem( "ЮТ" );
            airlines.add_elem( "ПО" );
            airlines.add_elem( "НН" );
            airlines.set_elems_permit(true);
            break;
        }
        reqInfo->user.access.merge_airlines(airlines);
        reqInfo->user.access.merge_airps(airps);

          ProgTrace( TRACE5, "user_type=%d, time_type=%d, icond=%d, day=%s, step=%d",
                     iuser_type, itime_type, icond, DateTimeToStr( day, ServerFormatDateTimeAsString ).c_str(), step );
          string strNew, strOld;
          xmlDocPtr ReqDocNew = NULL,
                    ReqDocOld = NULL,
                    ResDocNew = NULL,
                    ResDocOld = NULL;
          stepcount++;
          try {
            ReqDocNew = CreateXMLDoc( "requestNew" );
            ReqDocOld = CreateXMLDoc( "requestOld" );
            xmlNodePtr reqNodeNew = NewTextChild( ReqDocNew->children, "query" );
            xmlNodePtr reqNodeOld = NewTextChild( ReqDocOld->children, "query" );
            NewTextChild( reqNodeNew, "flight_date", DateTimeToStr( day, ServerFormatDateTimeAsString ) );
            NewTextChild( reqNodeNew, "pr_verify_new_select", 1 );
            NewTextChild( reqNodeOld, "flight_date", DateTimeToStr( day, ServerFormatDateTimeAsString ) );
            ResDocNew = CreateXMLDoc( "result" );
            ResDocOld = CreateXMLDoc( "result" );
            xmlNodePtr resNodeNew = NewTextChild( ResDocNew->children, "term" );
            resNodeNew = NewTextChild( resNodeNew, "answer" );
            xmlNodePtr resNodeOld = NewTextChild( ResDocOld->children, "term" );
            resNodeOld = NewTextChild( resNodeOld, "answer" );

            if ( step == 0 ) {
              step = 1;
              mcsTime = boost::posix_time::microsec_clock::universal_time();
              IntReadTrips( NULL, reqNodeOld, resNodeOld, delta_sql_old );
              delta_exec_old = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
              strOld = XMLTreeToText( ResDocOld );
              exec_old += delta_exec_old;
              exec_old_sql += delta_sql_old;
              mcsTime = boost::posix_time::microsec_clock::universal_time();
              IntReadTrips( NULL, reqNodeNew, resNodeNew, delta_sql_new );
              delta_exec_new = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
              strNew = XMLTreeToText( ResDocNew );
              exec_new += delta_exec_new;
              exec_new_sql += delta_sql_new;
            }
            else {
              step = 0;
              mcsTime = boost::posix_time::microsec_clock::universal_time();
              IntReadTrips( NULL, reqNodeNew, resNodeNew, delta_sql_new );
              delta_exec_new = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
              strNew = XMLTreeToText( ResDocNew );
              exec_new += delta_exec_new;
              exec_new_sql += delta_sql_new;
              mcsTime = boost::posix_time::microsec_clock::universal_time();
              IntReadTrips( NULL, reqNodeOld, resNodeOld, delta_sql_old );
              delta_exec_old = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
              strOld = XMLTreeToText( ResDocOld );
              exec_old += delta_exec_old;
              exec_old_sql += delta_sql_old;
            }
            if ( strNew != strOld ) {
              ProgTrace( TRACE5, "DIFF!!!" );
              f1 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
              f1 << strOld;
              f2 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
              f2 << strNew;
              file_size += strOld.size();
              file_size += strNew.size();
            }
          }
          catch(EXCEPTIONS::Exception &e) {
            f1 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
            f1 << "==========================EXCEPTIONS::Exception &e="<<e.what()<<endl;
            if ( !strOld.empty() ) {
              f1 << strOld;
            }
            f2 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
            f2 << "==========================EXCEPTIONS::Exception &e="<<e.what()<<endl;
            if ( !strNew.empty() ) {
              f2 << strNew;
            }
          }
          catch( ... ) {
            f1 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
            f1 << "==========================Unknown Error=============================="<<endl;
            f2 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
            f2 << "==========================Unknown Error=============================="<<endl;
            xmlFreeDoc( ReqDocNew );
            xmlFreeDoc( ReqDocOld );
            xmlFreeDoc( ResDocNew );
            xmlFreeDoc( ResDocOld );
          }
          xmlFreeDoc( ReqDocNew );
          xmlFreeDoc( ReqDocOld );
          xmlFreeDoc( ResDocNew );
          xmlFreeDoc( ResDocOld );
          if ( delta_sql_new > delta_sql_old ) {
            f1<<"user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<",day="<<DateTimeToStr( day, ServerFormatDateTimeAsString )<<",step="<<step<<
                ", diff_exec_sql="<<((delta_sql_new-delta_sql_old)/1000)<<" ms, diff_exec="<<((delta_exec_new-delta_exec_old)/1000)<<" ms, delta_exec_new="<<delta_sql_new<<endl;
            f2<<"user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<",day="<<DateTimeToStr( day, ServerFormatDateTimeAsString )<<",step="<<step<<
                ", diff_exec_sql="<<((delta_sql_new-delta_sql_old)/1000)<<" ms, diff_exec="<<((delta_exec_new-delta_exec_old)/1000)<<" ms, delta_exec_new="<<delta_sql_new<<endl;
            file_size += 100;
          }
        }
        if ( file_size > 10000 ) {
          file_size=0;
          if (f1.is_open()) f1.close();
          if (f2.is_open()) f2.close();
          file_count++;
          file_name=string("sopp_test_old.txt") + IntToString( file_count );
          f1.open(file_name.c_str());
          if (!f1.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());
          file_name=string("sopp_test_new.txt") + IntToString( file_count );;
          f2.open(file_name.c_str());
          if (!f2.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());
        }
      }
    }
  }
  f1<<"exec_new="<<(exec_new/1000)<<" ms    exec_new_sql="<<(exec_new_sql/1000)<<" ms"<<endl;
  f1<<"exec_old="<<(exec_old/1000)<<" ms    exec_old_sql="<<(exec_old_sql/1000)<<" ms"<<endl;
  f2<<"exec_new="<<(exec_new/1000)<<" ms    exec_new_sql="<<(exec_new_sql/1000)<<" ms"<<endl;
  f2<<"exec_old="<<(exec_old/1000)<<" ms    exec_old_sql="<<(exec_old_sql/1000)<<" ms"<<endl;
  if (f1.is_open()) f1.close();
  if (f2.is_open()) f2.close();
  return 0;
}


int test_file_queue(int argc,char **argv)
{
   int res = 0;
   TQuery Qry(&OraSession);
   Qry.SQLText =
     "DELETE file_queue";
   Qry.Execute();
   std::map<std::string,std::string> params;
   params[ "PARAM_FILE_TYPE" ] = "AODB";
   params[ "PARAM_CANON_NAME" ] = "RASTRV";
   params[ "PARAM_IN_ORDER" ] = "TRUE";
   string sender = string("BETADC");
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
   }
   res++;
   tst();
   sleep( 2 );
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
   }
   res++;
   tst();
   TFileQueue::sendFile( id2 );
   file_queue.get( TFilterQueue(receiver, 1) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id1||
        file_queue.getstatus(id1) != string( "SEND" ) ||
        !file_queue.in_order( id1 ) ||
        !TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        !file_queue.isLastFile() ) {
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
   }
   res++;
   tst();
   sleep(2);
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
   }
   res++;
   tst();
   sleep(2);
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
   }
   tst();
   file_queue.doneFile( id1 );
   tst();
   sleep(2);
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
   }
   tst();
   res++;
   file_queue.doneFile( id3 );
   file_queue.get( TFilterQueue(receiver, 1) );
     if ( !file_queue.empty() ||
          !file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, file_queue.isLastFile=%d", res, file_queue.empty(), file_queue.isLastFile() );
   }
   tst();
   type = "MINTRANS"; //не важна сортировка
   params[ "WORKDIR" ] = "c:\\work";
   BASIC::TDateTime UTCSysdate = NowUTC() + 5.0/1440.0;
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
   }
   res++;
   file_queue.sendFile( id2 );
   file_queue.get( TFilterQueue(filter2) );
   if ( file_queue.empty() ||
        file_queue.begin()->id != id3||
        file_queue.getstatus(id2) != string( "SEND" ) ||
        file_queue.in_order( id1 ) ||
        file_queue.in_order( id3 ) ||
        TFileQueue::in_order( type ) ||
        file_queue.size() != 1 ||
        !file_queue.getparam_value( id1, "WORKDIR", param_value ) ||
        param_value != "c:\\work" ||
        !file_queue.isLastFile() ) {
     ProgError( STDLOG, "error%d: %d, %d", res, file_queue.empty(), file_queue.isLastFile() );
     if ( !file_queue.empty() ) {
       ProgError( STDLOG, "error%d: file_queue.begin()->id=%d, id1=%d, status=%s, in_order(%d)=%d, in_order(%s)=%d, size()=%zu,param_value=%s, isLastFile=%d",
                  res,
                  file_queue.begin()->id,
                  id3,
                  file_queue.getstatus(id1).c_str(),
                  id3,
                  file_queue.in_order( id3 ),
                  type.c_str(),
                  TFileQueue::in_order( type ),
                  file_queue.size(),
                  param_value.c_str(),
                  file_queue.isLastFile() );
     }
   }
   return res;
}

int convert_codeshare(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT * FROM codeshare_sets "
    "WHERE (last_date IS NULL OR last_date>=TO_DATE('01.04.15','DD.MM.YY')) AND "
    "      first_date<TO_DATE('01.04.15','DD.MM.YY') AND pr_mark_norms=0 AND pr_del=0";
  Qry.Execute();

  TQuery UpdQry(&OraSession);
  UpdQry.Clear();
  UpdQry.SQLText=
    "BEGIN "
    "  adm.add_codeshare_set(:id,:airline_oper,:flt_no_oper,:airp_dep,:airline_mark,:flt_no_mark, "
    "                        1,:pr_mark_bp,:pr_mark_rpt,:days,TO_DATE('01.04.15','DD.MM.YY'),:last_date, "
    "                        :now_local,NULL,0,'ЧИЧЕРОВ В.В.','МОВВЛА'); "
    "END; ";
  UpdQry.DeclareVariable("id", otInteger);
  UpdQry.DeclareVariable("airline_oper", otString);
  UpdQry.DeclareVariable("flt_no_oper", otInteger);
  UpdQry.DeclareVariable("airp_dep", otString);
  UpdQry.DeclareVariable("airline_mark", otString);
  UpdQry.DeclareVariable("flt_no_mark", otInteger);
  UpdQry.DeclareVariable("pr_mark_bp", otInteger);
  UpdQry.DeclareVariable("pr_mark_rpt", otInteger);
  UpdQry.DeclareVariable("days", otString);
  UpdQry.DeclareVariable("last_date", otDate);
  UpdQry.DeclareVariable("now_local", otDate);

  for(;!Qry.Eof;Qry.Next())
  {
    UpdQry.SetVariable("id", FNull);
    UpdQry.SetVariable("airline_oper", Qry.FieldAsString("airline_oper"));
    UpdQry.SetVariable("flt_no_oper", Qry.FieldAsInteger("flt_no_oper"));
    UpdQry.SetVariable("airp_dep", Qry.FieldAsString("airp_dep"));
    UpdQry.SetVariable("airline_mark", Qry.FieldAsString("airline_mark"));
    UpdQry.SetVariable("flt_no_mark", Qry.FieldAsInteger("flt_no_mark"));
    UpdQry.SetVariable("pr_mark_bp", Qry.FieldAsInteger("pr_mark_bp"));
    UpdQry.SetVariable("pr_mark_rpt", Qry.FieldAsInteger("pr_mark_rpt"));
    UpdQry.SetVariable("days", Qry.FieldAsString("days"));
    if (!Qry.FieldIsNULL("last_date"))
      UpdQry.SetVariable("last_date", Qry.FieldAsDateTime("last_date"));
    else
      UpdQry.SetVariable("last_date", FNull);


    string airp=Qry.FieldAsString("airp_dep");
    TDateTime now_local= UTCToLocal( NowUTC(), AirpTZRegion(airp) );
    modf(now_local,&now_local);
    UpdQry.SetVariable("now_local", now_local);

    UpdQry.Execute();

  };
  OraSession.Commit();
  return 0;
};

/*
CREATE TABLE drop_nat_stat
(
    nationality VARCHAR2(3),
    craft       VARCHAR2(3),
    num         NUMBER(9)
);


set serveroutput on;
set heading off;
set pagesize 5000;
set linesize 500;
set trimspool on;
spool 'Статистика по гражданству ЮТ 011215-310316.txt';
SELECT 'Страна'||CHR(9)||'Код'||CHR(9)||'СНГ'||CHR(9)||'Тип ВС'||CHR(9)||'Код ВС'||CHR(9)||'Пассажиры' AS str
FROM dual;
SELECT NVL(pax_doc_countries.name,'Гражданство не определено')||CHR(9)||
       nationality||CHR(9)||
       DECODE(nationality, 'AZE', 'СНГ',
                           'ARM', 'СНГ',
                           'BLR', 'СНГ',
                           'KAZ', 'СНГ',
                           'KGZ', 'СНГ',
                           'MDA', 'СНГ',
                           'RUS', 'СНГ',
                           'UZB', 'СНГ',
                           'UKR', 'СНГ', NULL)||CHR(9)||
       NVL(crafts.name,'Тип ВС не определен')||CHR(9)||
       drop_nat_stat.craft||CHR(9)||
       num AS str
FROM drop_nat_stat, pax_doc_countries, crafts
WHERE drop_nat_stat.nationality=pax_doc_countries.code(+) AND
      drop_nat_stat.craft=crafts.code(+)
ORDER BY pax_doc_countries.name, crafts.name;
spool off;
set heading on;
set pagesize 50;
set linesize 150;
*/

typedef pair<string/*nationality*/, string/*craft*/> natStatKey;
typedef map< natStatKey, int> natStatMap;

int nat_stat(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id, craft FROM points "
    "WHERE scd_out>=TO_DATE('01.12.2015','DD.MM.YYYY') AND scd_out<TO_DATE('01.04.2016','DD.MM.YYYY') AND airline='ЮТ' AND "
    "      pr_reg<>0 AND pr_del>=0";
  Qry.Execute();
  list< pair<int, string> > point_ids;
  for(;!Qry.Eof;Qry.Next())
    point_ids.push_back(make_pair(Qry.FieldAsInteger("point_id"), Qry.FieldAsString("craft")));

  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_doc.nationality, COUNT(*) AS num "
    "FROM pax_grp, pax, pax_doc "
    "WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=pax_doc.pax_id AND "
    "      pax_grp.status NOT IN ('E') AND pax_grp.point_dep=:point_id "
    "GROUP BY pax_doc.nationality";
  Qry.DeclareVariable("point_id", otInteger);

  natStatMap stat;
  int processed=0;
  for(list< pair<int, string> >::const_iterator i=point_ids.begin(); i!=point_ids.end(); ++i)
  {
    Qry.SetVariable("point_id", i->first);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      natStatKey key(Qry.FieldAsString("nationality"), i->second);

      int num=Qry.FieldAsInteger("num");
      natStatMap::iterator j=stat.find(key);
      if (j==stat.end())
        stat.insert(make_pair(key, num));
      else
        j->second+=num;
    }
    processed++;
    alter_wait(processed, false, 10, 0);
  }

  Qry.Clear();
  Qry.SQLText="DELETE FROM drop_nat_stat";
  Qry.Execute();

  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO drop_nat_stat(nationality, craft, num) VALUES(:nationality, :craft, :num)";
  Qry.DeclareVariable("nationality", otString);
  Qry.DeclareVariable("craft", otString);
  Qry.DeclareVariable("num", otInteger);
  for(natStatMap::const_iterator i=stat.begin(); i!=stat.end(); ++i)
  {
    Qry.SetVariable("nationality", i->first.first);
    Qry.SetVariable("craft", i->first.second);
    Qry.SetVariable("num", i->second);
    Qry.Execute();
  }
  OraSession.Commit();


  return 0;
}

#include "astra_locale.h"
using namespace AstraLocale;

int test_apis(int argc,char **argv)
{
  TReqInfo::Instance()->client_type=ctTerm;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id FROM points "
    "WHERE scd_out>=TO_DATE('06.02.2016','DD.MM.YYYY') AND scd_out<TO_DATE('06.03.2016','DD.MM.YYYY') AND "
    "      pr_reg<>0 AND pr_del>=0";
  Qry.Execute();
  list<int> point_ids;
  for(;!Qry.Eof;Qry.Next())
    point_ids.push_back(Qry.FieldAsInteger("point_id"));

  Qry.Clear();
  Qry.SQLText=
    "SELECT * FROM pax_grp WHERE point_dep=:point_id";
  Qry.DeclareVariable("point_id", otInteger);

  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  PaxQry.SQLText=
    "SELECT pax.*, NULL AS seat_no FROM pax WHERE grp_id=:grp_id";
  PaxQry.DeclareVariable("grp_id", otInteger);

//  CREATE TABLE drop_test_apis
//  (
//    pax_id NUMBER(9),
//    type   VARCHAR2(4),
//    error  VARCHAR2(500)
//  );

  TQuery InsQry(&OraSession);
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO drop_test_apis(pax_id, type, error) VALUES(:pax_id, SUBSTR(:type,1,500), :error)";
  InsQry.DeclareVariable("pax_id", otInteger);
  InsQry.DeclareVariable("type", otString);
  InsQry.DeclareVariable("error", otString);

  printf("%zu flights\n", point_ids.size());

  int processed=0;
  int docs_count=0, doco_count=0, doca_count=0, flt_count=0;
  for(list<int>::const_iterator i=point_ids.begin(); i!=point_ids.end(); ++i)
  {
    TTripInfo fltInfo;
    if (!fltInfo.getByPointId(*i)) continue;
    if (fltInfo.real_out==NoExists) continue;
    TRouteAPICheckInfo routeCheckInfo(*i);

    flt_count++;

    Qry.SetVariable("point_id", *i);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      CheckIn::TPaxGrpItem grp;
      grp.fromDB(Qry);
      TDateTime checkDate=UTCToLocal(fltInfo.real_out,AirpTZRegion(grp.airp_dep));
      boost::optional<const TCompleteAPICheckInfo&> checkInfo=routeCheckInfo.get(grp.airp_arv);
      if (!checkInfo)
        ProgError(STDLOG, "%s: checkInfo=false! point_id=%d airp_arv=%s", __FUNCTION__, *i, grp.airp_arv.c_str());

      PaxQry.SetVariable("grp_id", grp.id);
      PaxQry.Execute();
      for(;!PaxQry.Eof;PaxQry.Next())
      {
        CheckIn::TSimplePaxItem pax;
        pax.fromDB(PaxQry);

        CheckIn::TPaxDocItem doc;
        if (CheckIn::LoadPaxDoc(pax.id, doc))
        {
          CheckIn::TPaxDocItem doc_tmp=doc;
          try
          {
            HandleDoc(grp, pax, checkInfo?checkInfo.get():TCompleteAPICheckInfo(), checkDate, doc_tmp);
            if (!doc.equalAttrs(doc_tmp)) throw EXCEPTIONS::Exception("not equal data. Equal mask: %ld", doc.getEqualAttrsFieldsMask(doc_tmp));
            processed++;
            docs_count++;
          }
          catch(std::exception &e)
          {
            ProgTrace(TRACE5, "DOCS: pax_id=%d error=%s", pax.id, e.what());
            InsQry.SetVariable("pax_id", pax.id);
            InsQry.SetVariable("type", "DOCS");
            InsQry.SetVariable("error", e.what());
            InsQry.Execute();
            OraSession.Commit();
          }
        };

        CheckIn::TPaxDocoItem doco;
        if (CheckIn::LoadPaxDoco(pax.id, doco))
        {
          CheckIn::TPaxDocoItem doco_tmp=doco;
          try
          {
            HandleDoco(grp, pax, checkInfo?checkInfo.get():TCompleteAPICheckInfo(), checkDate, doco_tmp);
            if (!doco.equalAttrs(doco_tmp)) throw EXCEPTIONS::Exception("not equal data. Equal mask: %ld", doco.getEqualAttrsFieldsMask(doco_tmp));
            processed++;
            doco_count++;
          }
          catch(std::exception &e)
          {
            ProgTrace(TRACE5, "DOCO: pax_id=%d error=%s", pax.id, e.what());
            InsQry.SetVariable("pax_id", pax.id);
            InsQry.SetVariable("type", "DOCO");
            InsQry.SetVariable("error", e.what());
            InsQry.Execute();
            OraSession.Commit();
          }
        }

        std::list<CheckIn::TPaxDocaItem> doca;
        if (CheckIn::LoadPaxDoca(pax.id, doca))
        {
          for(list<CheckIn::TPaxDocaItem>::const_iterator d=doca.begin(); d!=doca.end(); ++d)
          {
            std::list<CheckIn::TPaxDocaItem> doca_tmp;
            doca_tmp.push_back(*d);
            try
            {
              HandleDoca(grp, pax, checkInfo?checkInfo.get():TCompleteAPICheckInfo(), doca_tmp);
              if (!(doca_tmp.front()==*d)) throw EXCEPTIONS::Exception("not equal data. Equal mask: %ld", doca_tmp.front().getEqualAttrsFieldsMask(*d));
              processed++;
              doca_count++;
            }
            catch(std::exception &e)
            {
              ProgTrace(TRACE5, "DOCA: pax_id=%d error=%s", pax.id, e.what());
              InsQry.SetVariable("pax_id", pax.id);
              InsQry.SetVariable("type", "DOCA");
              InsQry.SetVariable("error", e.what());
              InsQry.Execute();
              OraSession.Commit();
            }
          }
        }
      }
    }
    if (alter_wait(processed, false, 10, 0))
      printf("flights=%d docs=%d doco=%d doca=%d\n", flt_count, docs_count, doco_count, doca_count);
  }
  printf("%d iterations processed!\n", processed);
  printf("total: flights=%d docs=%d doco=%d doca=%d\n", flt_count, docs_count, doco_count, doca_count);

  return 0;
}

int test_apis2(int argc,char **argv)
{
  TReqInfo::Instance()->client_type=ctTerm;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id FROM points "
    "WHERE scd_out>=TO_DATE('29.02.2016','DD.MM.YYYY') AND scd_out<TO_DATE('07.03.2016','DD.MM.YYYY') AND "
    "      pr_reg<>0 AND pr_del>=0";
  Qry.Execute();
  list<int> point_ids;
  for(;!Qry.Eof;Qry.Next())
    point_ids.push_back(Qry.FieldAsInteger("point_id"));

//  CREATE TABLE drop_test_apis2
//  (
//    point_id NUMBER(9),
//    airp_arv VARCHAR(3),
//    apis_formats  VARCHAR2(100),
//    pass_doc_req  NUMBER(5),
//    pass_doco_req  NUMBER(5),
//    pass_docab_req  NUMBER(5),
//    pass_docar_req  NUMBER(5),
//    pass_docad_req  NUMBER(5),
//    pass_docad_req  NUMBER(5),
//    crew_doc_req  NUMBER(5),
//    crew_doco_req  NUMBER(5),
//    crew_docab_req  NUMBER(5),
//    crew_docar_req  NUMBER(5),
//    crew_docad_req  NUMBER(5),
//    crew_tkn_req  NUMBER(5),
//    is_inter NUMBER(2),
//    not_apis NUMBER(2)
//  );

//  SELECT DISTINCT is_inter, not_apis FROM drop_test_apis2;
//  SELECT DISTINCT apis_formats, is_inter, not_apis FROM drop_test_apis2 ORDER BY apis_formats;
//  SELECT COUNT(*), apis_formats, pass_doc_req, pass_doco_req, pass_docab_req, pass_docar_req, pass_docad_req, pass_tkn_req,
//         crew_doc_req, crew_doco_req, crew_docab_req, crew_docar_req, crew_docad_req, crew_tkn_req
//  FROM drop_test_apis2
//  GROUP BY apis_formats, pass_doc_req, pass_doco_req, pass_docab_req, pass_docar_req, pass_docad_req, pass_docad_req,
//           crew_doc_req, crew_doco_req, crew_docab_req, crew_docar_req, crew_docad_req, crew_tkn_req
//  ORDER BY apis_formats;

//  SELECT COUNT(*), SUBSTR(apis_formats,1,30), pass_doc_req, pass_doco_req,
//         crew_doc_req, crew_doco_req
//  FROM drop_test_apis2
//  GROUP BY SUBSTR(apis_formats,1,30), pass_doc_req, pass_doco_req,
//           crew_doc_req, crew_doco_req
//  ORDER BY 2;


  TQuery InsQry(&OraSession);
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO drop_test_apis2(point_id, airp_arv, apis_formats, "
    "  pass_doc_req, pass_doco_req, pass_docab_req, pass_docar_req, pass_docad_req, pass_tkn_req, "
    "  crew_doc_req, crew_doco_req, crew_docab_req, crew_docar_req, crew_docad_req, crew_tkn_req, "
    "  is_inter, not_apis)"
    "VALUES(:point_id, :airp_arv, :apis_formats, "
    "  :pass_doc_req, :pass_doco_req, :pass_docab_req, :pass_docar_req, :pass_docad_req, :pass_tkn_req, "
    "  :crew_doc_req, :crew_doco_req, :crew_docab_req, :crew_docar_req, :crew_docad_req, :crew_tkn_req, "
    "  :is_inter, :not_apis)";
  InsQry.DeclareVariable("point_id", otInteger);
  InsQry.DeclareVariable("airp_arv", otString);
  InsQry.DeclareVariable("apis_formats", otString);
  InsQry.DeclareVariable("pass_doc_req", otInteger);
  InsQry.DeclareVariable("pass_doco_req", otInteger);
  InsQry.DeclareVariable("pass_docab_req", otInteger);
  InsQry.DeclareVariable("pass_docar_req", otInteger);
  InsQry.DeclareVariable("pass_docad_req", otInteger);
  InsQry.DeclareVariable("pass_tkn_req", otInteger);
  InsQry.DeclareVariable("crew_doc_req", otInteger);
  InsQry.DeclareVariable("crew_doco_req", otInteger);
  InsQry.DeclareVariable("crew_docab_req", otInteger);
  InsQry.DeclareVariable("crew_docar_req", otInteger);
  InsQry.DeclareVariable("crew_docad_req", otInteger);
  InsQry.DeclareVariable("crew_tkn_req", otInteger);
  InsQry.DeclareVariable("is_inter", otInteger);
  InsQry.DeclareVariable("not_apis", otInteger);


  printf("%zu flights\n", point_ids.size());

  int processed=0;
  for(list<int>::const_iterator i=point_ids.begin(); i!=point_ids.end(); ++i)
  {
    TRouteAPICheckInfo routeCheckInfo(*i);
//    for(TRouteAPICheckInfo::const_iterator r=routeCheckInfo.begin(); r!=routeCheckInfo.end(); ++r)
//    {
//      const TCompleteAPICheckInfo &checkInfo=r->second;
//      InsQry.SetVariable("point_id", *i);
//      InsQry.SetVariable("airp_arv", r->first);
//      InsQry.SetVariable("apis_formats", GetSQLEnum(checkInfo.apis_formats()));
//      InsQry.SetVariable("pass_doc_req", (int)checkInfo.pass().get(apiDoc).required_fields);
//      InsQry.SetVariable("pass_doco_req", (int)checkInfo.pass().get(apiDoco).required_fields);
//      InsQry.SetVariable("pass_docab_req", (int)checkInfo.pass().get(apiDocaB).required_fields);
//      InsQry.SetVariable("pass_docar_req", (int)checkInfo.pass().get(apiDocaR).required_fields);
//      InsQry.SetVariable("pass_docad_req", (int)checkInfo.pass().get(apiDocaD).required_fields);
//      InsQry.SetVariable("pass_tkn_req", (int)checkInfo.pass().get(apiTkn).required_fields);
//      InsQry.SetVariable("crew_doc_req", (int)checkInfo.crew().get(apiDoc).required_fields);
//      InsQry.SetVariable("crew_doco_req", (int)checkInfo.crew().get(apiDoco).required_fields);
//      InsQry.SetVariable("crew_docab_req", (int)checkInfo.crew().get(apiDocaB).required_fields);
//      InsQry.SetVariable("crew_docar_req", (int)checkInfo.crew().get(apiDocaR).required_fields);
//      InsQry.SetVariable("crew_docad_req", (int)checkInfo.crew().get(apiDocaD).required_fields);
//      InsQry.SetVariable("crew_tkn_req", (int)checkInfo.crew().get(apiTkn).required_fields);

//      int is_inter=
//          (int)checkInfo.pass().get(apiDoc).is_inter+
//          (int)checkInfo.pass().get(apiDoco).is_inter+
//          (int)checkInfo.pass().get(apiDocaB).is_inter+
//          (int)checkInfo.pass().get(apiDocaR).is_inter+
//          (int)checkInfo.pass().get(apiDocaD).is_inter+
//          (int)checkInfo.pass().get(apiTkn).is_inter+
//          (int)checkInfo.crew().get(apiDoc).is_inter+
//          (int)checkInfo.crew().get(apiDoco).is_inter+
//          (int)checkInfo.crew().get(apiDocaB).is_inter+
//          (int)checkInfo.crew().get(apiDocaR).is_inter+
//          (int)checkInfo.crew().get(apiDocaD).is_inter+
//          (int)checkInfo.crew().get(apiTkn).is_inter;

//      int not_apis=
//          (int)checkInfo.pass().get(apiDoc).not_apis+
//          (int)checkInfo.pass().get(apiDoco).not_apis+
//          (int)checkInfo.pass().get(apiDocaB).not_apis+
//          (int)checkInfo.pass().get(apiDocaR).not_apis+
//          (int)checkInfo.pass().get(apiDocaD).not_apis+
//          (int)checkInfo.pass().get(apiTkn).not_apis+
//          (int)checkInfo.crew().get(apiDoc).not_apis+
//          (int)checkInfo.crew().get(apiDoco).not_apis+
//          (int)checkInfo.crew().get(apiDocaB).not_apis+
//          (int)checkInfo.crew().get(apiDocaR).not_apis+
//          (int)checkInfo.crew().get(apiDocaD).not_apis+
//          (int)checkInfo.crew().get(apiTkn).not_apis;


//      InsQry.SetVariable("is_inter", is_inter);
//      InsQry.SetVariable("not_apis", not_apis);
//      InsQry.Execute();
//      OraSession.Commit();
//      processed++;
//    }
    alter_wait(processed, false, 10, 0);
  }
  printf("%d iterations processed!\n", processed);

  return 0;
}


int test_apis3(int argc,char **argv)
{
  TReqInfo::Instance()->client_type=ctTerm;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT MIN(pax_id) AS min_pax_id FROM drop_test_apis10";
//    "SELECT MIN(pax_id) AS min_pax_id FROM pax_doc";
  Qry.Execute();
  int min_pax_id=Qry.FieldAsInteger("min_pax_id");
  Qry.SQLText=
    "SELECT MAX(pax_id) AS max_pax_id FROM drop_test_apis10";
//    "SELECT MAX(pax_id) AS max_pax_id FROM pax_doc";
  Qry.Execute();
  int max_pax_id=Qry.FieldAsInteger("max_pax_id");

  Qry.Clear();
  Qry.SQLText=
//    "SELECT pax.pax_id, pax_doc.surname AS doc_surname, pax.surname "
//    "FROM pax, pax_doc "
//    "WHERE pax_doc.pax_id=pax.pax_id AND "
//    "      pax_doc.pax_id>=:min_pax_id AND pax_doc.pax_id<:min_pax_id+100000 AND "
//    "      pax_doc.surname<>pax.surname";
    "SELECT pax_id, surname, REPLACE(doc_surname, ' ') AS doc_surname "
    "FROM drop_test_apis10 "
    "WHERE pax_id>=:min_pax_id AND pax_id<:min_pax_id+100000";
  Qry.DeclareVariable("min_pax_id", otInteger);

//CREATE TABLE drop_test_apis11
//(
//   pax_id                  NUMBER(9) NOT NULL,
//   surname                 VARCHAR2(64) NOT NULL,
//   doc_surname             VARCHAR(64) NOT NULL,
//   edit_dist_vlad          NUMBER(3) NOT NULL,
//   edit_dist_vlad_fmt      NUMBER(1) NOT NULL,
//   edit_dist_vlad_sim      NUMBER(3) NOT NULL,
//   edit_dist_vlad_sim_fmt  NUMBER(1) NOT NULL,
//   edit_dist               NUMBER(3) NOT NULL,
//   edit_dist_fmt           NUMBER(1) NOT NULL,
//   edit_dist_sim           NUMBER(3) NOT NULL,
//   edit_dist_sim_fmt       NUMBER(1) NOT NULL,
//   jaro_winkler_sim        NUMBER(3) NOT NULL,
//   jaro_winkler_sim_fmt    NUMBER(1) NOT NULL
//);

//CREATE UNIQUE INDEX drop_test_apis3__AK ON drop_test_apis3(pax_id);

  TQuery Qry2(&OraSession);
  Qry2.Clear();
  Qry2.SQLText=
    "SELECT :edit_distance_vlad AS edit_dist_vlad, "
    "       :edit_distance_vlad_sim AS edit_dist_vlad_sim, "
    "       utl_match.edit_distance(system.transliter(:surname, :fmt), system.transliter(:doc_surname, :fmt)) AS edit_dist, "
    "       utl_match.edit_distance_similarity(system.transliter(:surname, :fmt), system.transliter(:doc_surname, :fmt)) AS edit_dist_sim, "
    "       utl_match.jaro_winkler_similarity(system.transliter(:surname, :fmt), system.transliter(:doc_surname, :fmt)) AS jaro_winkler_sim "
    "FROM dual";
  Qry2.DeclareVariable("edit_distance_vlad", otInteger);
  Qry2.DeclareVariable("edit_distance_vlad_sim", otInteger);
  Qry2.DeclareVariable("surname", otString);
  Qry2.DeclareVariable("doc_surname", otString);
  Qry2.DeclareVariable("fmt", otInteger);

  TQuery InsQry(&OraSession);
  InsQry.Clear();
  InsQry.SQLText=
//    "INSERT INTO drop_test_apis3(pax_id, surname, doc_surname, "
    "INSERT INTO drop_test_apis11(pax_id, surname, doc_surname, "
    "  edit_dist_vlad, edit_dist_vlad_fmt, edit_dist_vlad_sim, edit_dist_vlad_sim_fmt, "
    "  edit_dist, edit_dist_fmt, edit_dist_sim, edit_dist_sim_fmt, "
    "  jaro_winkler_sim, jaro_winkler_sim_fmt) "
    "VALUES(:pax_id, :surname, :doc_surname, "
    "  :edit_dist_vlad, :edit_dist_vlad_fmt, :edit_dist_vlad_sim, :edit_dist_vlad_sim_fmt, "
    "  :edit_dist, :edit_dist_fmt, :edit_dist_sim, :edit_dist_sim_fmt, "
    "  :jaro_winkler_sim, :jaro_winkler_sim_fmt)";

  InsQry.DeclareVariable("pax_id", otInteger);
  InsQry.DeclareVariable("surname", otString);
  InsQry.DeclareVariable("doc_surname", otString);
  InsQry.DeclareVariable("edit_dist_vlad", otInteger);
  InsQry.DeclareVariable("edit_dist_vlad_fmt", otInteger);
  InsQry.DeclareVariable("edit_dist_vlad_sim", otInteger);
  InsQry.DeclareVariable("edit_dist_vlad_sim_fmt", otInteger);
  InsQry.DeclareVariable("edit_dist", otInteger);
  InsQry.DeclareVariable("edit_dist_fmt", otInteger);
  InsQry.DeclareVariable("edit_dist_sim", otInteger);
  InsQry.DeclareVariable("edit_dist_sim_fmt", otInteger);
  InsQry.DeclareVariable("jaro_winkler_sim", otInteger);
  InsQry.DeclareVariable("jaro_winkler_sim_fmt", otInteger);

  int processed=0;
  for(;min_pax_id<=max_pax_id; min_pax_id+=100000)
  {
    Qry.SetVariable("min_pax_id", min_pax_id);
    Qry.Execute();
    for(;!Qry.Eof; Qry.Next())
    {
      int pax_id=Qry.FieldAsInteger("pax_id");
      string surname=Qry.FieldAsString("surname");
      string doc_surname=Qry.FieldAsString("doc_surname");
//      string surname3=Qry.FieldAsString("surname3");
//      string doc_surname3=Qry.FieldAsString("doc_surname3");
//      string surname3_2=transliter(surname, 3, true);
//      string doc_surname3_2=transliter(doc_surname, 3, true);

//      if (surname3!=surname3_2)
//        ProgError(STDLOG, "%s: surname=%s surname3=%s surname3_2=%s",
//                  __FUNCTION__, surname.c_str(), surname3.c_str(), surname3_2.c_str());

//      if (doc_surname3!=doc_surname3_2)
//        ProgError(STDLOG, "%s: doc_surname=%s doc_surname3=%s doc_surname3_2=%s",
//                  __FUNCTION__, doc_surname.c_str(), doc_surname3.c_str(), doc_surname3_2.c_str());

      int edit_dist_vlad=NoExists;
      int edit_dist_vlad_fmt=NoExists;
      int edit_dist_vlad_sim=NoExists;
      int edit_dist_vlad_sim_fmt=NoExists;
      int edit_dist=NoExists;
      int edit_dist_fmt=NoExists;
      int edit_dist_sim=NoExists;
      int edit_dist_sim_fmt=NoExists;
      int jaro_winkler_sim=NoExists;
      int jaro_winkler_sim_fmt=NoExists;

      for(int fmt=0; fmt<4; fmt++)
      {
        string s1, s2;
        if (fmt==0)
        {
          s1=surname;
          s2=doc_surname;
        }
        else
        {
          s1=transliter(surname, fmt, true);
          s2=transliter(doc_surname, fmt, true);
        };

        Qry2.SetVariable("edit_distance_vlad", (int)EditDistance(s1, s2));
        Qry2.SetVariable("edit_distance_vlad_sim", EditDistanceSimilarity(s1, s2));
//        Qry2.SetVariable("surname", s1);
//        Qry2.SetVariable("doc_surname", s2);
        Qry2.SetVariable("surname", surname);
        Qry2.SetVariable("doc_surname", doc_surname);
        Qry2.SetVariable("fmt", fmt);
        Qry2.Execute();
        if (fmt==0 || edit_dist_vlad>Qry2.FieldAsInteger("edit_dist_vlad"))  //разница
        {
          edit_dist_vlad=Qry2.FieldAsInteger("edit_dist_vlad");
          edit_dist_vlad_fmt=fmt;
        };

        if (fmt==0 || edit_dist_vlad_sim<Qry2.FieldAsInteger("edit_dist_vlad_sim"))   //процент схожести
        {
          edit_dist_vlad_sim=Qry2.FieldAsInteger("edit_dist_vlad_sim");
          edit_dist_vlad_sim_fmt=fmt;
        };

        if (fmt==0 || edit_dist>Qry2.FieldAsInteger("edit_dist"))  //разница
        {
          edit_dist=Qry2.FieldAsInteger("edit_dist");
          edit_dist_fmt=fmt;
        };
        if (fmt==0 || edit_dist_sim<Qry2.FieldAsInteger("edit_dist_sim"))  //процент схожести
        {
          edit_dist_sim=Qry2.FieldAsInteger("edit_dist_sim");
          edit_dist_sim_fmt=fmt;
        };
        if (fmt==0 || jaro_winkler_sim<Qry2.FieldAsInteger("jaro_winkler_sim"))  //процент схожести
        {
          jaro_winkler_sim=Qry2.FieldAsInteger("jaro_winkler_sim");
          jaro_winkler_sim_fmt=fmt;
        };
      }

//      int sim=best_transliter_similarity(surname, doc_surname);
//      if (sim!=edit_dist_vlad_sim)
//        ProgError(STDLOG, "%s: surname=%s doc_surname=%s sim=%d edit_dist_vlad_sim=%d",
//                  __FUNCTION__, surname.c_str(), doc_surname.c_str(), sim, edit_dist_vlad_sim);

      if (edit_dist==0 && edit_dist_vlad==0) continue;

      InsQry.SetVariable("pax_id", pax_id);
      InsQry.SetVariable("surname", surname);
      InsQry.SetVariable("doc_surname", doc_surname);
      InsQry.SetVariable("edit_dist_vlad", edit_dist_vlad);
      InsQry.SetVariable("edit_dist_vlad_fmt", edit_dist_vlad_fmt);
      InsQry.SetVariable("edit_dist_vlad_sim", edit_dist_vlad_sim);
      InsQry.SetVariable("edit_dist_vlad_sim_fmt", edit_dist_vlad_sim_fmt);
      InsQry.SetVariable("edit_dist", edit_dist);
      InsQry.SetVariable("edit_dist_fmt", edit_dist_fmt);
      InsQry.SetVariable("edit_dist_sim", edit_dist_sim);
      InsQry.SetVariable("edit_dist_sim_fmt", edit_dist_sim_fmt);
      InsQry.SetVariable("jaro_winkler_sim", jaro_winkler_sim);
      InsQry.SetVariable("jaro_winkler_sim_fmt", jaro_winkler_sim_fmt);
      InsQry.Execute();
      OraSession.Commit();
      processed++;
    }
    if (alter_wait(processed, false, 10, 0))
      printf("min_pax_id=%d\n", min_pax_id);
  }
  printf("%d iterations processed!\n", processed);

//  select pax_id, refuse from pax where grp_id in
//  (select grp_id from pax_grp where point_dep in
//   (select point_id from points where point_id in (7539533, 7549362, 7549364, 7555270, 7552579, 7575840, 7578570, 7566337, 7560135, 7604111)));

//  select edit_dist_sim||': '||TRIM(surname||' | '||doc_surname)  from drop_test_apis3 WHERE edit_dist_sim<90 ORDER BY edit_dist_sim;


  return 0;
}

void print_diff(bool first_iter,
                const BASIC::TDateTime &date1,
                const BASIC::TDateTime &date2)
{
  int julian_date1=NoExists, year_last_digit1=NoExists, julian_date2=NoExists, year_last_digit2=NoExists;
  string dateStr1, dateStr2, error1, error2;
  dateStr1=DateTimeToStr(date1, "dd.mm.yy hh:nn:ss");
  dateStr2=DateTimeToStr(date2, "dd.mm.yy hh:nn:ss");

  try
  {
    JulianDate d(date1);
    julian_date1=d.getJulianDate();
    year_last_digit1=d.getYearLastDigit();
  }
  catch(EXCEPTIONS::EConvertError &e)
  {
    error1=e.what();
  };

  try
  {
    JulianDate d(date2);
    julian_date2=d.getJulianDate();
    year_last_digit2=d.getYearLastDigit();
  }
  catch(EXCEPTIONS::EConvertError &e)
  {
    error2=e.what();
  };

  if (julian_date1!=julian_date2 || year_last_digit1!=year_last_digit2 || error1!=error2 || first_iter)
  {
    printf("date=%s julian_date=%d year_last_digit=%d error=%s\n",
           dateStr2.c_str(), julian_date2, year_last_digit2, error2.c_str());
  }
}

void print_diff(bool first_iter,
                const int &julian_date1,
                const boost::optional<int> &year_last_digit1,
                const BASIC::TDateTime &time_point1,
                const JulianDate::TDirection &direction1,
                const int &julian_date2,
                const boost::optional<int> &year_last_digit2,
                const BASIC::TDateTime &time_point2,
                const JulianDate::TDirection &direction2)
{
  string dateStr1, dateStr2, error1, error2;
  string timePointStr1=DateTimeToStr(time_point1, "dd.mm.yy hh:nn:ss");
  string timePointStr2=DateTimeToStr(time_point2, "dd.mm.yy hh:nn:ss");

  try
  {
    if (year_last_digit1)
    {
      JulianDate d(julian_date1, year_last_digit1.get(), time_point1, direction1);
      dateStr1=DateTimeToStr(d.getDateTime(), "dd.mm.yy hh:nn:ss");
    }
    else
    {
      JulianDate d(julian_date1, time_point1, direction1);
      dateStr1=DateTimeToStr(d.getDateTime(), "dd.mm.yy hh:nn:ss");
    };
  }
  catch(EXCEPTIONS::EConvertError &e)
  {
    error1="error";
  };

  try
  {
    if (year_last_digit2)
    {
      JulianDate d(julian_date2, year_last_digit2.get(), time_point2, direction2);
      dateStr2=DateTimeToStr(d.getDateTime(), "dd.mm.yy hh:nn:ss");
    }
    else
    {
      JulianDate d(julian_date2, time_point2, direction2);
      dateStr2=DateTimeToStr(d.getDateTime(), "dd.mm.yy hh:nn:ss");
    };
  }
  catch(EXCEPTIONS::EConvertError &e)
  {
    error2="error";
  };

  if (dateStr1!=dateStr2 || error1!=error2 || first_iter)
  {
    if (year_last_digit2)
      printf("julian_date=%d year_last_digit=%d time_point=%s date=%s error=%s\n",
             julian_date2, year_last_digit2.get(), timePointStr2.c_str(), dateStr2.c_str(), error2.c_str());
    else
      printf("julian_date=%d time_point=%s date=%s error=%s\n",
             julian_date2, timePointStr2.c_str(), dateStr2.c_str(), error2.c_str());
  }
}

int test_julian(int argc,char **argv)
{
  {
    TDateTime fd, ld;
    StrToDateTime("28.12.16", "dd.mm.yy", fd);
    StrToDateTime("05.01.17", "dd.mm.yy", ld);
    TDateTime d1=fd;
    for(TDateTime d2=fd; d2<=ld; d2+=1.0/1440)
    {
      print_diff(d2==fd, d1, d2);
      d1=d2;
    }
  }
  {
    TDateTime fd, ld;
    StrToDateTime("28.12.1898", "dd.mm.yyyy", fd);
    StrToDateTime("05.01.1917", "dd.mm.yyyy", ld);
    int prior_julian_date=-100;
    for(TDateTime d2=fd; d2<=ld; d2+=1.0)
    {
      JulianDate d(d2);
      if (prior_julian_date+1!=d.getJulianDate())
        printf("date=%s prior_julian_date=%d julian_date=%d\n",
               DateTimeToStr(d2, "dd.mm.yy hh:nn:ss").c_str(),
               prior_julian_date,
               d.getJulianDate());


      prior_julian_date=d.getJulianDate();
    }
  }
  for(int pass2=0; pass2<3; pass2++)
  {
    int jd=100;
    if (pass2==1) jd=366;
    if (pass2==2) jd=369;
    for(int pass=0; pass<3; pass++)
    {
      printf("\n");
      JulianDate::TDirection dir=JulianDate::TDirection::everywhere;
      if (pass==1) dir=JulianDate::TDirection::before;
      if (pass==2) dir=JulianDate::TDirection::after;

      TDateTime fd, ld;
      StrToDateTime("28.12.1895", "dd.mm.yyyy", fd);
      StrToDateTime("05.01.1906", "dd.mm.yyyy", ld);
      TDateTime prior_time_point=fd;
      for(TDateTime time_point=fd; time_point<=ld; time_point+=1.0)
      {
          print_diff(time_point==fd,
                     jd, boost::none, prior_time_point, dir,
                     jd, boost::none, time_point, dir);
        prior_time_point=time_point;
      }

    }
  };

  for(int yld=-1; yld<11; yld++)
  {
    for(int pass=0; pass<3; pass++)
    {
      printf("\n");
      JulianDate::TDirection dir=JulianDate::TDirection::everywhere;
      if (pass==1) dir=JulianDate::TDirection::before;
      if (pass==2) dir=JulianDate::TDirection::after;

      TDateTime fd, ld;
      StrToDateTime("28.12.1878", "dd.mm.yyyy", fd);
      StrToDateTime("05.01.1922", "dd.mm.yyyy", ld);
      TDateTime prior_time_point=fd;
      for(TDateTime time_point=fd; time_point<=ld; time_point+=1.0)
      {
          print_diff(time_point==fd,
                     366, yld, prior_time_point, dir,
                     366, yld, time_point, dir);
        prior_time_point=time_point;
      }

    }
  };

  for(int pass=0; pass<3; pass++)
  {
    TDateTime time_point;
    StrToDateTime("01.04.1974 12:00", "dd.mm.yyyy hh:nn", time_point);

    printf("\n");
    JulianDate::TDirection dir=JulianDate::TDirection::everywhere;
    if (pass==1) dir=JulianDate::TDirection::before;
    if (pass==2) dir=JulianDate::TDirection::after;
    int prior_jd=0;
    for(int jd=0; jd<=367; jd++)
    {
      print_diff(jd==0,
                 prior_jd, boost::none, time_point, dir,
                 jd, boost::none, time_point, dir);
      prior_jd=jd;

    }
  };

  JulianDate(NowUTC()).trace(__FUNCTION__);

  return 0;
}

/*
CREATE TABLE drop_pc_wt_stat
(
   point_id        NUMBER(9) NOT NULL,
   ticket_no       VARCHAR2(15) NOT NULL,
   coupon_no       NUMBER(1) NOT NULL,
   etick_norm      NUMBER(3) NULL,
   etick_norm_unit VARCHAR2(2) NOT NULL,
   pax_norm        VARCHAR2(20) NULL,
   events_time     DATE NULL,
   events_msg      VARCHAR2(250) NULL
);

set serveroutput on;
set heading off;
set pagesize 30000;
set linesize 1000;
set trimspool on;
spool 'Неправильно оформленный багаж.txt';
SELECT 'А/к'||CHR(9)||
       'Рейс'||CHR(9)||
       'А/п'||CHR(9)||
       'Вылет (UTC)'||CHR(9)||
       'Номер ЭБ'||CHR(9)||
       'Норма в билете'||CHR(9)||
       'Норма применена'||CHR(9)||
       'Время применения (UTC)'||CHR(9)||
       'Журнал операций' AS str
FROM dual;
SELECT points.airline||CHR(9)||
       LPAD(points.flt_no, 3, '0')||points.suffix||CHR(9)||
       points.airp||CHR(9)||
       TO_CHAR(points.scd_out, 'DD.MM.YY HH24:MI')||CHR(9)||
       stat.ticket_no||'/'||stat.coupon_no||CHR(9)||
       stat.etick_norm||stat.etick_norm_unit||CHR(9)||
       pax_norm||CHR(9)||
       TO_CHAR(events_time, 'DD.MM.YY HH24:MI:SS')||CHR(9)||
       events_msg
FROM points, trip_sets, drop_pc_wt_stat stat
WHERE points.point_id=stat.point_id AND
      points.point_id=trip_sets.point_id AND trip_sets.piece_concept<>0
ORDER BY points.airline, points.airp, points.scd_out, points.flt_no,
         stat.ticket_no, stat.coupon_no;
spool off;
set heading on;
set pagesize 50;
set linesize 150;
*/

int pc_wt_stat(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="DELETE FROM drop_pc_wt_stat";
  Qry.Execute();
  OraSession.Commit();

  Qry.Clear();
  Qry.SQLText=
    "SELECT point_id FROM points "
    "WHERE scd_out>=:first_date AND scd_out<:last_date AND "
    "      pr_reg<>0 AND pr_del>=0";
  TDateTime last_date=NowUTC()+2;
  TDateTime first_date=IncMonth(last_date, -1);
  Qry.CreateVariable("first_date", otDate, first_date);
  Qry.CreateVariable("last_date", otDate, last_date);
  Qry.Execute();
  list< int > point_ids;
  for(;!Qry.Eof;Qry.Next())
    point_ids.push_back(Qry.FieldAsInteger("point_id"));

  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_grp.piece_concept, pax_grp.grp_id, pax.pax_id, "
    "       pax.ticket_rem, pax.ticket_no, pax.coupon_no, pax.ticket_confirm, "
    "       eticks_display.bag_norm, eticks_display.bag_norm_unit "
    "FROM pax_grp, pax, eticks_display "
    "WHERE pax_grp.grp_id=pax.grp_id AND pax.refuse IS NULL AND "
    "      pax.ticket_no=eticks_display.ticket_no(+) AND "
    "      pax.coupon_no=eticks_display.coupon_no(+) AND "
    "      pax_grp.point_dep=:point_id AND "
    "      pax_grp.piece_concept IS NOT NULL AND "
    "      eticks_display.bag_norm_unit IS NOT NULL AND "
    "      (pax_grp.piece_concept=0 AND eticks_display.bag_norm_unit='PC' OR "
    "       pax_grp.piece_concept<>0 AND eticks_display.bag_norm_unit<>'PC')";
  Qry.DeclareVariable("point_id", otInteger);

  TQuery InsQry(&OraSession);
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO drop_pc_wt_stat(point_id, ticket_no, coupon_no, etick_norm, etick_norm_unit, pax_norm, events_time, events_msg) "
    "VALUES(:point_id, :ticket_no, :coupon_no, :etick_norm, :etick_norm_unit, :pax_norm, :events_time, :events_msg)";
  InsQry.DeclareVariable("point_id", otInteger);
  InsQry.DeclareVariable("ticket_no", otString);
  InsQry.DeclareVariable("coupon_no", otInteger);
  InsQry.DeclareVariable("etick_norm", otInteger);
  InsQry.DeclareVariable("etick_norm_unit", otString);
  InsQry.DeclareVariable("pax_norm", otString);
  InsQry.DeclareVariable("events_time", otDate);
  InsQry.DeclareVariable("events_msg", otString);

  TQuery LogQry(&OraSession);
  LogQry.Clear();
  LogQry.SQLText=
    "SELECT msg, time "
    "FROM events_bilingual "
    "WHERE lang=:lang AND type=:type AND id1=:point_id AND id3=:grp_id AND "
    "      msg like '%рименена весовая система расчета%' ";
  LogQry.CreateVariable("lang", otString, AstraLocale::LANG_RU);
  LogQry.CreateVariable("type", otString, EncodeEventType(ASTRA::evtPax));
  LogQry.DeclareVariable("point_id", otInteger);
  LogQry.DeclareVariable("grp_id", otInteger);

  TQuery LogQry2(&OraSession);
  LogQry2.Clear();
  LogQry2.SQLText=
    "SELECT MIN(time) AS time "
    "FROM events_bilingual, web_clients "
    "WHERE events_bilingual.station=web_clients.desk(+) AND "
    "      events_bilingual.lang=:lang AND "
    "      events_bilingual.type=:type AND "
    "      events_bilingual.id1=:point_id AND "
    "      events_bilingual.id3=:grp_id AND "
    "      web_clients.desk IS NULL";
  LogQry2.CreateVariable("lang", otString, AstraLocale::LANG_RU);
  LogQry2.CreateVariable("type", otString, EncodeEventType(ASTRA::evtPax));
  LogQry2.DeclareVariable("point_id", otInteger);
  LogQry2.DeclareVariable("grp_id", otInteger);

  int processed=0;
  for(list< int >::const_iterator i=point_ids.begin(); i!=point_ids.end(); ++i)
  {
    Qry.SetVariable("point_id", *i);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      bool piece_concept=Qry.FieldAsInteger("piece_concept")!=0;
      int grp_id=Qry.FieldAsInteger("grp_id");
      string pax_norm_view, events_msg;
      TDateTime events_time=NoExists;
      if (!piece_concept)
      {
        int pax_id=Qry.FieldAsInteger("pax_id");
        list< pair<WeightConcept::TPaxNormItem, WeightConcept::TNormItem> > norms;
        WeightConcept::PaxNormsFromDB(NoExists, pax_id, norms);
        for(list< pair<WeightConcept::TPaxNormItem, WeightConcept::TNormItem> >::const_iterator n=norms.begin(); n!=norms.end(); ++n)
        {
          if (n->first.bag_type!=NoExists) continue;
          pax_norm_view=n->second.str(AstraLocale::LANG_RU);
          break;
        };

        int events_point_id=*i, events_grp_id=grp_id;
        TCkinRoute route;
        route.GetRouteBefore(grp_id, crtWithCurrent, crtIgnoreDependent);
        if (!route.empty())
        {
          events_point_id=route.front().point_dep;
          events_grp_id=route.front().grp_id;
        };

        LogQry.SetVariable("point_id", events_point_id);
        LogQry.SetVariable("grp_id", events_grp_id);
        LogQry.Execute();
        if (!LogQry.Eof)
        {
          events_msg=LogQry.FieldAsString("msg");
          events_time=LogQry.FieldAsDateTime("time");
        };
      }
      else pax_norm_view="PC";

      if (events_time==NoExists)
      {
        LogQry2.SetVariable("point_id", *i);
        LogQry2.SetVariable("grp_id", grp_id);
        LogQry2.Execute();
        if (!LogQry2.Eof && !LogQry2.FieldIsNULL("time"))
          events_time=LogQry2.FieldAsDateTime("time");
      }

      InsQry.SetVariable("point_id", *i);
      InsQry.SetVariable("ticket_no", Qry.FieldAsString("ticket_no"));
      InsQry.SetVariable("coupon_no", Qry.FieldAsInteger("coupon_no"));
      if (!Qry.FieldIsNULL("bag_norm"))
        InsQry.SetVariable("etick_norm", Qry.FieldAsInteger("bag_norm"));
      else
        InsQry.SetVariable("etick_norm", FNull);
      InsQry.SetVariable("etick_norm_unit", Qry.FieldAsString("bag_norm_unit"));
      InsQry.SetVariable("pax_norm", pax_norm_view);
      if (events_time!=NoExists)
        InsQry.SetVariable("events_time", events_time);
      else
        InsQry.SetVariable("events_time", FNull);
      InsQry.SetVariable("events_msg", events_msg);
      InsQry.Execute();
    };
    OraSession.Commit();
    processed++;
    alter_wait(processed, false, 10, 0);
  }

  return 0;
}


