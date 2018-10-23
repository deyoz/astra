//---------------------------------------------------------------------------
#include "obrnosir.h"
#include "date_time.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "arx_daily.h"
#include "checkin.h"
#include "passenger.h"
#include "telegram.h"
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
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace std;

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
    nosir_wait(processed, false, 10, 5);
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
    "WHERE time>=:low_date AND time<:high_date AND type<>'���' ";
  Qry.DeclareVariable("low_date", otDate);
  Qry.DeclareVariable("high_date", otDate);


  int processed=0;
  for(TDateTime curr_date=min_date; curr_date<max_date; curr_date+=1.0, processed++)
  {
    nosir_wait(processed, false, 10, 1);
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
    IF curRow.type='���' OR curRow.type='���' THEN
      SELECT COUNT(*) INTO i FROM points WHERE point_id=curRow.id1;
      IF i>0 THEN
        UPDATE drop_events SET pr_del=0 WHERE ev_order=curRow.ev_order;
        COMMIT;
      END IF;
    END IF;
    IF curRow.type='���' THEN
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
FROM drop_events WHERE pr_del IS NULL AND type IN ('���','���','���')
GROUP BY type,
         SUBSTR(msg,1,5),
         DECODE(id1,NULL,'NULL','NOT NULL'),
         DECODE(id2,NULL,'NULL','NOT NULL'),
         DECODE(id3,NULL,'NULL','NOT NULL')
ORDER BY max_time;
*/
int get_sirena_rozysk_stat(int argc,char **argv)
{
  string country="��";

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
        nosir_wait(processed, false, 10, 5);
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
        nosir_wait(processed, false, 10, 5);
        TTripRoute route;
        route.GetRouteAfter(i->first, i->second, trtWithCurrent, trtNotCancelled);
        //printf("routeAfter: %s\n", route.GetStr().c_str());
        for(TTripRoute::const_iterator r1=route.begin(); r1!=route.end(); r1++)
        {
          string city_dep, country_dep;
          try
          {
            const TAirpsRow &airpRow = (const TAirpsRow&)base_tables.get("airps").get_row("code",r1->airp);
            city_dep=airpRow.city;
            const TCitiesRow &cityRow = (const TCitiesRow&)base_tables.get("cities").get_row("code",airpRow.city);
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
              const TAirpsRow &airpRow = (const TAirpsRow&)base_tables.get("airps").get_row("code",r2->airp);
              city_arv=airpRow.city;
              const TCitiesRow &cityRow = (const TCitiesRow&)base_tables.get("cities").get_row("code",airpRow.city);
              country_arv=cityRow.country;
            }
            catch(EBaseTableError) {};
            if (country_dep==country || country_arv==country)
            {
              //nosir_wait(processed);
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
                    case C: f << "������-�����;";
                        break;
                    case Y: f << "������-�����;";
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
                  << (doc.type=="P"?"��ᯮ��":"") << ";"
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

                f << ";;��ॣ����஢��;";

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
                  f << "���ᠦ�� �뫥⥫;";
                else
                  f << "���ᠦ�� �� �뫥⥫;";

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
      //� ��砥 �訡�� ����襬 ���⮩ 䠩�
      f.open(filename);
      if (f.is_open()) f.close();
    }
    catch( ... ) { };
    throw;
  };

  return 0;
};

int test_trfer_exists(int argc,char **argv)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize("���");

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
  //TDateTime first_date = NowUTC()-500;
  //  TDateTime last_date = NowUTC() + 750;
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
                   "%2[A-Z�-��0-9]%5lu%3[A-Z�-��]%3[A-Z�-��]%8[0-9]%c",
                   airline, &flt_no, airp_dep, airp_arv, scd_out_local, &c);
        if (res!=6 || IsDigitIsLetter(c))
        {
          res=sscanf(dp->d_name,
                     "%2[A-Z�-��0-9]%5lu%1[A-Z�-��]%3[A-Z�-��]%3[A-Z�-��]%8[0-9]%c",
                     airline, &flt_no, suffix, airp_dep, airp_arv, scd_out_local, &c);
          if (res!=7 || IsDigitIsLetter(c)) continue;
        };
      }
      else
      {
        res=sscanf(dp->d_name,
                   "%3[A-Z�-��0-9]%5lu%3[A-Z�-��]%3[A-Z�-��]%8[0-9]%c",
                   airline, &flt_no, airp_dep, airp_arv, scd_out_local, &c);
        if (res!=6 || IsDigitIsLetter(c))
        {
          res=sscanf(dp->d_name,
                     "%3[A-Z�-��0-9]%5lu%1[A-Z�-��]%3[A-Z�-��]%3[A-Z�-��]%8[0-9]%c",
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
      if (StrToDateTime(scd_out_local, "yyyymmdd", flt.scd_out_local )==EOF) continue;

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
  //����� ��� ������ � ��ண� ��ࠡ��稪�, ����� �ࠢ����� १���⮢
  int step=0, stepcount=0;
  TReqInfo *reqInfo = TReqInfo::Instance();
  for ( TDateTime day=NowUTC()-3.0; day<=NowUTC()+3.0; day++ ) {
  for ( int iuser_type=0; iuser_type<3; iuser_type++ ) {
    for ( int itime_type=0; itime_type<3; itime_type++ ) {
      for ( int icond=0; icond<=11; icond++ ) {
        reqInfo->Initialize("���");
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
            airps.add_elem( "���" );
            airps.set_elems_permit(true);
            break;
          case 2:
            airps.add_elem( "���" );
            airps.set_elems_permit(false);
            break;
          case 3:
            airlines.add_elem( "��" );
            airlines.set_elems_permit(true);
            break;
          case 4:
            airlines.add_elem( "��" );
            airlines.set_elems_permit(false);
            break;
          case 5:
            airps.add_elem( "���" );
            airps.set_elems_permit(true);
            airlines.add_elem( "��" );
            airlines.set_elems_permit(true);
            break;
          case 6:
            airps.add_elem( "���" );
            airps.set_elems_permit(false);
            airlines.add_elem( "��" );
            airlines.set_elems_permit(true);
            break;
          case 7:
            airps.add_elem( "���" );
            airps.set_elems_permit(true);
            airlines.add_elem( "��" );
            airlines.set_elems_permit(false);
            break;
          case 8:
            airps.add_elem( "���" );
            airps.set_elems_permit(false);
            airlines.add_elem( "��" );
            airlines.set_elems_permit(false);
            break;
          case 9:
            airps.add_elem( "���" );
            airps.add_elem( "���" );
            airps.add_elem( "BBU" );
            airps.set_elems_permit(true);
            break;
          case 10:
            airlines.add_elem( "��" );
            airlines.add_elem( "��" );
            airlines.add_elem( "��" );
            airlines.set_elems_permit(true);
            break;
          case 11:
            airps.add_elem( "���" );
            airps.add_elem( "���" );
            airps.add_elem( "BBU" );
            airps.set_elems_permit(true);
            airlines.add_elem( "��" );
            airlines.add_elem( "��" );
            airlines.add_elem( "��" );
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
   type = "SOFI"; //�� ����� ���஢��
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
   type = "MINTRANS"; //�� ����� ���஢��
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
    "                        :now_local,NULL,0,'������� �.�.','������'); "
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
    airp_dep VARCHAR2(3),
    airp_arv VARCHAR2(3),
    craft       VARCHAR2(3),
    nationality VARCHAR2(3),
    num         NUMBER(9)
);


set serveroutput on;
set heading off;
set pagesize 5000;
set linesize 500;
set trimspool on;
spool '����⨪� �� �ࠦ������ �� 011215-310316.txt';
SELECT '��࠭�'||CHR(9)||'���'||CHR(9)||'���'||CHR(9)||'��� ��'||CHR(9)||'��� ��'||CHR(9)||'���ᠦ���' AS str
FROM dual;
SELECT NVL(pax_doc_countries.name,'�ࠦ����⢮ �� ��।�����')||CHR(9)||
       nationality||CHR(9)||
       DECODE(nationality, 'AZE', '���',
                           'ARM', '���',
                           'BLR', '���',
                           'KAZ', '���',
                           'KGZ', '���',
                           'MDA', '���',
                           'RUS', '���',
                           'UZB', '���',
                           'UKR', '���', NULL)||CHR(9)||
       NVL(crafts.name,'��� �� �� ��।����')||CHR(9)||
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

namespace NatStat {

    typedef map<string, int> TNat;
    typedef map<string, TNat> TCraft;
    typedef map<string, TCraft> TAirpArv;
    typedef map<string, TAirpArv> natStatMap;

    string get_nat_code(string country)
    {
        if(
                country == "AZE" or
                country == "ARM" or
                country == "BLR" or
                country == "KAZ" or
                country == "KGZ" or
                country == "MDA" or
                country == "RUS" or
                country == "UZB" or
                country == "UKR"
          )
            country = "���";
        else
            country.clear();
        return country;
    }

    int nat_stat(int argc,char **argv)
    {
        TDateTime FirstDate, LastDate;
        if (!getDateRangeFromArgs(argc, argv, FirstDate, LastDate))
          return 1;

        TQuery Qry(&OraSession);
        natStatMap stat;
        int processed=0;

        for(int step = 0; step < 2; step++) {
            tst();
            string SQLText =
                "SELECT point_id, craft ";
            if(step == 1)
                SQLText +=
                    "   ,part_key ";
            SQLText +=
                "FROM ";
            if(step == 0)
                SQLText +=
                    "   points ";
            else
                SQLText +=
                    "   arx_points ";
            SQLText +=
                "WHERE scd_out>=:FirstDate AND scd_out<:LastDate AND airline='��' AND "
                "      pr_reg<>0 AND pr_del>=0";
            Qry.Clear();
            Qry.SQLText= SQLText;
            Qry.CreateVariable("FirstDate", otDate, FirstDate);
            Qry.CreateVariable("LastDate", otDate, LastDate);
            Qry.Execute();
            list< pair<int, pair<TDateTime, string> > > point_ids;
            for(;!Qry.Eof;Qry.Next()) {
                TDateTime part_key = NoExists;
                if(step == 1)
                    part_key = Qry.FieldAsDateTime("part_key");
                point_ids.push_back(
                        make_pair(
                            Qry.FieldAsInteger("point_id"),
                            make_pair(
                                part_key,
                                Qry.FieldAsString("craft")
                                )
                            )
                        );
            }

            Qry.Clear();
            tst();
            SQLText =
                "SELECT "
                "   pax_grp.airp_dep, "
                "   pax_grp.airp_arv, "
                "   pax_doc.nationality, "
                "   COUNT(*) AS num "
                "FROM ";
            if(step == 0)
                SQLText +=
                    "   pax_grp, "
                    "   pax, "
                    "   pax_doc ";
            else
                SQLText +=
                    "   arx_pax_grp pax_grp, "
                    "   arx_pax pax, "
                    "   arx_pax_doc pax_doc ";
            SQLText +=
                "WHERE ";
            if(step == 1) {
                SQLText +=
                    "   pax_grp.part_key = :part_key and "
                    "   pax.part_key = :part_key and "
                    "   pax_doc.part_key = :part_key and ";
                Qry.DeclareVariable("part_key", otDate);
            }
            SQLText +=
                "   pax_grp.grp_id=pax.grp_id AND pax.pax_id=pax_doc.pax_id AND "
                "   pax_grp.status NOT IN ('E') AND pax_grp.point_dep=:point_id "
                "GROUP BY "
                "   pax_grp.airp_dep, "
                "   pax_grp.airp_arv, "
                "   pax_doc.nationality";

            tst();
            Qry.SQLText= SQLText;
            Qry.DeclareVariable("point_id", otInteger);
            tst();

            for(list< pair<int, pair<TDateTime, string> > >::const_iterator i=point_ids.begin(); i!=point_ids.end(); ++i)
            {
                tst();
                Qry.SetVariable("point_id", i->first);
                if(step == 1) {
                    Qry.SetVariable("part_key", i->second.first);
                }
                Qry.Execute();
                for(;!Qry.Eof;Qry.Next())
                {
                    int num=Qry.FieldAsInteger("num");
                    string airp_dep = Qry.FieldAsString("airp_dep");
                    string airp_arv = Qry.FieldAsString("airp_arv");
                    string nationality = Qry.FieldAsString("nationality");

                    stat[airp_dep][airp_arv][i->second.second][nationality] += num;
                }
                processed++;
                nosir_wait(processed, false, 10, 0);
            }
            tst();
        }

        Qry.Clear();
        Qry.SQLText="DELETE FROM drop_nat_stat";
        Qry.Execute();

        Qry.Clear();
        Qry.SQLText=
            "INSERT INTO drop_nat_stat( "
            "   airp_dep, "
            "   airp_arv, "
            "   craft, "
            "   nationality, "
            "   num "
            ") VALUES( "
            "   :airp_dep, "
            "   :airp_arv, "
            "   :craft, "
            "   :nationality, "
            "   :num)";
        Qry.DeclareVariable("airp_dep", otString);
        Qry.DeclareVariable("airp_arv", otString);
        Qry.DeclareVariable("craft", otString);
        Qry.DeclareVariable("nationality", otString);
        Qry.DeclareVariable("num", otInteger);

        const string delim = ";";
        TEncodedFileStream of("cp1251",
                (string)"nat_stat." +
                DateTimeToStr(FirstDate, "ddmmyy") + "-" +
                DateTimeToStr(LastDate, "ddmmyy") +
                ".csv");
        of
            << "�/� �뫥�" << delim
            << "�/� �ਫ��" << delim
            << "��� ��" << delim
            << "��࠭�" << delim
            << "���" << delim
            << "���-�� ���ᠦ�஢" << endl;

        for(natStatMap::const_iterator airp_dep =stat.begin(); airp_dep!=stat.end(); ++airp_dep)
            for(TAirpArv::const_iterator airp_arv = airp_dep->second.begin(); airp_arv != airp_dep->second.end(); airp_arv++)
                for(TCraft::const_iterator craft = airp_arv->second.begin(); craft != airp_arv->second.end(); craft++)
                    for(TNat::const_iterator nat = craft->second.begin(); nat != craft->second.end(); nat++) {
                        of
                            << ElemIdToNameLong(etAirp, airp_dep->first) << delim
                            << ElemIdToNameLong(etAirp, airp_arv->first) << delim
                            << (craft->first.empty() ? "��� �� �� ��।����" : craft->first) << delim
                            << (nat->first.empty() ? "�ࠦ����⢮ �� ��।�����" : ElemIdToNameLong(etPaxDocCountry, nat->first)) << delim
                            << get_nat_code(nat->first) << delim
                            << nat->second << endl;
                        Qry.SetVariable("airp_dep", airp_dep->first);
                        Qry.SetVariable("airp_arv", airp_arv->first);
                        Qry.SetVariable("craft", craft->first);
                        Qry.SetVariable("nationality", nat->first);
                        Qry.SetVariable("num", nat->second);
                        Qry.Execute();
                    }

        OraSession.Commit();

        return 0;
    }

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
spool '���ࠢ��쭮 ��ଫ���� �����.txt';
SELECT '�/�'||CHR(9)||
       '����'||CHR(9)||
       '�/�'||CHR(9)||
       '�뫥� (UTC)'||CHR(9)||
       '����� ��'||CHR(9)||
       '��ଠ � �����'||CHR(9)||
       '��ଠ �ਬ�����'||CHR(9)||
       '�६� �ਬ������ (UTC)'||CHR(9)||
       '��ୠ� ����権' AS str
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
    "      msg like '%ਬ����� ��ᮢ�� ��⥬� ����%' ";
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
          if (n->first.bag_type222!=WeightConcept::REGULAR_BAG_TYPE) continue;
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
    nosir_wait(processed, false, 10, 0);
  }

  return 0;
}

/*
void TZUpdate();

int tz_conversion(int, char **) {
    std::cout << "IANA timezone version: " << getTZDataVersion() << std::endl;

    TZUpdate();
    return 0;
}

#include "tz_test.h"

int test_conversion(int, char **) {
    std::cout << "IANA timezone version: " << getTZDataVersion() << std::endl;

    std::cout << "Creating points..." << std::endl;
    createPoints();
    std::cout << "Points created. Forming XML..." << std::endl;
    createXMLFromPoints();
    std::cout << "XML formed." << std::endl;

    return 0;
}
*/

#include "serverlib/xml_stuff.h" // ��� xml_decode_nodelist

int tst_vo(int, char**)
{
    string qry =
        "<?xml version='1.0' encoding='cp866'?> "
        "<term> "
        "  <query handle='0' id='print' ver='1' opr='DEN' screen='AIR.EXE' mode='STAND' lang='RU' term_id='158626489'> "
        "    <GetGRPPrintData> "
        "      <op_type>PRINT_BP</op_type> "
        "      <voucher/> "
        "      <grp_id>2591687</grp_id> "
        "      <pr_all>1</pr_all> "
        "      <dev_model>ML390</dev_model> "
        "      <fmt_type>EPSON</fmt_type> "
        "      <prnParams> "
        "        <pr_lat>0</pr_lat> "
        "        <encoding>CP866</encoding> "
        "        <offset>20</offset> "
        "        <top>0</top> "
        "      </prnParams> "
        "      <clientData> "
        "        <gate>31</gate> "
        "      </clientData> "
        "    </GetGRPPrintData> "
        "    <response/>" // ᠬ ����ᠫ, �㤠 �㤥� ��ᮢ뢠���� �⢥�
        "  </query> "
        "</term> ";
    xmlDocPtr doc = TextToXMLTree(qry); // �� ����� � UTF
    xml_decode_nodelist(doc->children);
    xmlNodePtr rootNode=xmlDocGetRootElement(doc);
    TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize("���");
    XMLRequestCtxt *ctxt = getXmlCtxt();
    JxtInterfaceMng::Instance()->
        GetInterface("print")->
        OnEvent("GetGRPPrintData",  ctxt,
                rootNode->children->children,
                rootNode->children->children->next);
    LogTrace(TRACE5) << GetXMLDocText(rootNode->doc);
    return 1; // 0 - ��������� ����������, 1 - rollback
}

string capt_code(const string &val, const string &suffix = "")
{
    string result = val;
    if(not suffix.empty())
        result += " " + suffix;
    result += ":";
    return result;
}

string tag_params(const string &date_format, const string &lang = "")
{
    ostringstream buf;
    if(not date_format.empty())
        buf << ",," << date_format;
    if(not lang.empty()) {
        if(buf.str().empty())
            buf << ",,," << lang;
        else
            buf << "," << lang;
    }
    ostringstream result;
    if(not buf.str().empty())
        result << "(" << buf.str() << ")";
    return result.str();
}

void move_pos(const string &code, int &xpos, int &ypos)
{
    if(code == "CITY_ARV_NAME")
    {
        ypos = 5;
        xpos = 80;
    }
    if(code == "NAME") {
        ypos = 5;
        xpos = 160;
    }
}

int prn_tags(int argc, char **argv)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select code from prn_tag_props where op_type = 'PRINT_BP' order by code";
    Qry.Execute();
    ofstream prn_tags("prn_tags");
    int ypos = 20;
    for(; not Qry.Eof; Qry.Next(), ypos += 5) {
        string code = Qry.FieldAsString("code");
        string date_format;
        if(
                code == "ACT" or
                code == "EST" or
                code == "SCD" or
                code == "TIME_PRINT"
          )
            date_format = "dd mmm yy hh:nn:ss";
        prn_tags
            << left << setw(20) << capt_code(code)
            << "'[<" << code << tag_params(date_format) << ">]'" << endl
            << left << setw(20) << capt_code(code, "RU")
            << "'[<" << code << tag_params(date_format, "R") << ">]'" << endl
            << left << setw(20) << capt_code(code, "EN")
            << "'[<" << code << tag_params(date_format, "E") << ">]'" << endl;
    }
    return 1;
}

/*
int prn_tags(int argc, char **argv)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select code from prn_tag_props where op_type = 'PRINT_BP' order by code";
    Qry.Execute();
    ostringstream prn_tags;
    int ypos = 20;
    int xpos = 0;
    int file_idx = 0;
    for(; not Qry.Eof; Qry.Next(), ypos += 15) {
        string code = Qry.FieldAsString("code");
        string date_format;
        if(
                code == "ACT" or
                code == "EST" or
                code == "SCD" or
                code == "TIME_PRINT"
          )
            date_format = "dd mmm yy hh:nn:ss";
        ostringstream current_tag;
        current_tag
            << xpos << "," << ypos << ",0, " << setw(20) << left << capt_code(code) << "'[<" << code << tag_params(date_format) << ">]'" << endl
            << xpos << "," <<  ypos + 5 << ",0, " << setw(20) << left << capt_code(code, "RU") << "'[<" << code << tag_params(date_format, "R") << ">]'" << endl
            << xpos << "," <<  ypos + 10 << ",0, " << setw(20) << left << capt_code(code, "EN") << "'[<" << code << tag_params(date_format, "E") << ">]'" << endl;
        if(prn_tags.str().size() + current_tag.str().size() > 4000) {
            ostringstream file_name;
            file_name << "prn_tags." << file_idx++;
            ofstream of(file_name.str().c_str());
            of << prn_tags.str();
            prn_tags.str("");
        }
        prn_tags << current_tag.str();
        //        move_pos(code, xpos, ypos);
    }
    if(not prn_tags.str().empty()) {
        ostringstream file_name;
        file_name << "prn_tags." << file_idx++;
        ofstream of(file_name.str().c_str());
        of << prn_tags.str();
    }
    return 1;
}
*/

/*
int IsNosir(void)
{
  return LocalIsNosir==1;
}
*/

struct TSTrip {
  int trip_id;
  int move_id;
  int num;
};

using namespace std;
using namespace BASIC::date_time;
using namespace SEASON;
using namespace ASTRA;

TDateTime getdiffhours( const std::string &region )
{
/*'Asia/Magadan' +2
'Asia/Anadyr') +0
'Asia/Kamchatka' +0
'Asia/Novokuznetsk' +0
'Europe/Samara' +0
'Europe/Kaliningrad' +1
'Europe/Moscow' +1
'Europe/Simferopol' +1
'Europe/Volgograd' +1
'Asia/Yekaterinburg' +1
'Asia/Omsk' +1
'Asia/Novosibirsk' +1
'Asia/Krasnoyarsk' +1
'Asia/Irkutsk' +1
'Asia/Yakutsk' +1
'Asia/Khandyga' +1
'Asia/Vladivostok' +1
'Asia/Sakhalin' +1
'Asia/Ust-Nera' +1
'Asia/Chita' +2
'Asia/Srednekolymsk' +1
*/
/*  if ( region == "Asia/Barnaul" ||
       region == "Europe/Astrakhan" ||
       region == "Europe/Ulyanovsk" ||
       region == "Asia/Chita" ||
       region == "Asia/Sakhalin" ) {
    return -1.0;
  }*/
  if ( region == "Asia/Novosibirsk" ) {
    return -1.0;
  }

  return 0.0;
}

using namespace boost::local_time;
using namespace boost::posix_time;


void gettime( const TDateTime &old_utc, TDateTime &new_utc,
              TDateTime &old_local, TDateTime &new_local, const std::string &region )
{
   new_utc = old_utc;
   old_local = NoExists;
   new_local = NoExists;
   if ( old_utc == NoExists ) {
     return;
   }

   /*tz_database &tz_db = get_tz_database();
   old_local = UTCToLocal( old_utc, region );
   ptime vtime( DateTimeToBoost( old_utc ) );
   time_zone_ptr tz = tz_db.time_zone_from_region( region );
   local_date_time ltime( vtime, tz ); // ��।��塞 ⥪�饥 �६� �����쭮�
   if ( ltime.is_dst() ) { // ���
     new_utc = new_utc + getdiffhours( region ) /24.0;
   }
   new_local = UTCToLocal( new_utc, region );*/
}

/*

update points set pr_del=-1 WHERE move_id in
( select move_id from points
where (scd_in >= to_date('24.10.14','DD.MM.YY') or scd_out >= to_date('24.10.14','DD.MM.YY')) AND pr_del <> -1 )
*/


const int SEASON_PERIOD_COUNT = 16;
const int SEASON_PRIOR_PERIOD = 13;

struct P {
    ptime b;
    ptime s;
    boolean summer;
    int dst;
};

void getPeriods( string filter_tz_region, vector<P> &periods ) {
  /*ptime utcd = second_clock::universal_time();
  int year = utcd.date().year();

  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region( filter_tz_region );
  if (tz==NULL) throw EXCEPTIONS::Exception("Region '%s' not found",filter_tz_region.c_str());
  local_date_time ld( utcd, tz ); // ��।��塞 ⥪�饥 �६� �����쭮�

  bool summer = true;
  // ��⠭�������� ���� ��� � �ਧ��� ��ਮ��
  for ( int i=0; i<SEASON_PRIOR_PERIOD; i++ ) {
    if ( tz->has_dst() ) {  // �᫨ ���� ���室 �� ������/��⭥� �ᯨᠭ��
        if ( i == 0 ) {
        //dst_offset = tz->dst_offset().hours();
        if ( ld.is_dst() ) {  // �᫨ ᥩ�� ���
          year--;
          summer = false;
        }
        else {  // �᫨ ᥩ�� ����
          ptime start_time = tz->dst_local_start_time( year );
          ProgTrace( TRACE5, "start_time=%s", DateTimeToStr( BoostToDateTime(start_time),"dd.mm.yy hh:nn:ss" ).c_str() );
          if ( ld.local_time() < start_time ) {
            tst();
            year--;
          }
          summer = true;
        }
      }
      else {
        summer = !summer;
        if ( !summer ) {  // �᫨ ᥩ�� ���
          year--;
        }
      }
    }
    else {
     year--;
     //dst_offset = 0;
    }

  }
  for ( int i=0; i<SEASON_PERIOD_COUNT; i++ ) {
    ptime s_time, e_time;
    string name;
    if ( tz->has_dst() ) {
      if ( summer ) {
        s_time = tz->dst_local_start_time( year ) - tz->base_utc_offset();
        e_time = tz->dst_local_end_time( year ) - tz->base_utc_offset() - seconds(1);
      }
      else {
        s_time = tz->dst_local_end_time( year ) - tz->base_utc_offset() - tz->dst_offset();
        year++;
        e_time = tz->dst_local_start_time( year ) - tz->base_utc_offset() - seconds(1);
      }
      summer = !summer;
    }
    else {
     // ��ਮ� - �� 楫� ���
     s_time = ptime( boost::gregorian::date(year,1,1) );
     year++;
     e_time = ptime( boost::gregorian::date(year,1,1) );
    }
    ProgTrace( TRACE5, "s_time=%s, e_time=%s, summer=%d, i=%d, dst=%i",
               DateTimeToStr( UTCToLocal( BoostToDateTime(s_time), filter_tz_region ),"dd.mm.yy hh:nn:ss" ).c_str(),
               DateTimeToStr( UTCToLocal( BoostToDateTime(e_time), filter_tz_region ), "dd.mm.yy hh:nn:ss" ).c_str(), !summer, i,
               tz->dst_offset().hours());
    P p;
    p.b = s_time;
    p.s = e_time;
    p.summer = !summer;
    p.dst =  tz->dst_offset().hours();
    periods.push_back( p );
  }*/
}


int points_dst_format(int argc,char **argv)
{
  bool prior = false;
  printf("argc=%i\n",argc);
  for(int i=0; i<argc; i++) {
    printf("argv[%i]='%s'\n",i,argv[i]);
    if ( string( argv[i] ) == "prior" ) {
      prior = true;
      tst();
      break;
    }
  }

  TQuery Qry(&OraSession);
  if ( prior ) {
    Qry.Clear();
    Qry.SQLText =
        "select distinct s.region region from seasons s, DATE_TIME_ZONESPEC d where s.region=d.id AND id IN ("
        "'Asia/Sakhalin',"
        "'Asia/Chita' )";
    tst();
    Qry.Execute();
    tst();
    vector<string> regions;
    for ( ; !Qry.Eof; Qry.Next() ) {
      regions.push_back( Qry.FieldAsString("region") );
    }
    Qry.SQLText =
        "DELETE FROM seasons where region in (  "
        "select distinct s.region from seasons s, DATE_TIME_ZONESPEC d "
        " where s.region=d.id AND d.id IN ( "
                  "'Asia/Sakhalin',"
                  "'Asia/Chita' )"
        ")";
    tst();
    Qry.Execute();
    tst();
    Qry.Clear();

    TQuery GQry( &OraSession );
    GQry.Clear();
    GQry.SQLText =
    "DECLARE i NUMBER;"
    "BEGIN "
    "SELECT COUNT(*) INTO i FROM seasons "
    " WHERE region=:region AND :first=first AND :last=last; "
    "IF i = 0 THEN "
    " INSERT INTO seasons(region,first,last,hours) VALUES(:region,:first,:last,:hours); "
    "END IF; "
    "END;";
    GQry.DeclareVariable( "region", otString );
    GQry.DeclareVariable( "first", otDate );
    GQry.DeclareVariable( "last", otDate );
    GQry.DeclareVariable( "hours", otInteger );

    for ( vector<string>::iterator ireg=regions.begin(); ireg!=regions.end(); ireg++) {
      vector<P> periods;
      getPeriods( *ireg, periods );
      GQry.SetVariable( "region", *ireg );
      for ( vector<P>::iterator p=periods.begin(); p!=periods.end(); p++ ) {
        int hours;
         if ( p->summer )
           hours = p->dst;
         else
           hours = 0;
         GQry.SetVariable( "first", BoostToDateTime( p->b ) );
         GQry.SetVariable( "last", BoostToDateTime( p->s) );
         GQry.SetVariable( "hours", hours );
         GQry.Execute();
       }
    }
  }
  Qry.Clear();
  Qry.SQLText =
    "select point_id,airp,scd_in,scd_out,est_in,est_out,act_in,act_out, c.tz_region region from airps a, cities c,"
    " ( select point_id,airp,scd_in,scd_out,est_in,est_out,act_in,act_out from points p "
    " where (scd_in >= to_date('25.03.16','DD.MM.YY') or scd_out >= to_date('25.03.16','DD.MM.YY')) AND pr_del <> -1 ) p "
    " WHERE "
    " c.tz_region IN ( "
    " 'Europe/Astrakhan',"
    " 'Asia/Chita',"
    " 'Asia/Barnaul',"
    " 'Europe/Ulyanovsk',"
    " 'Asia/Sakhalin') AND "
    " p.airp=a.code AND a.city=c.code AND c.country='��' ";
  Qry.Execute();
  TQuery UQry(&OraSession);
  UQry.SQLText =
    "UPDATE points SET scd_in=:scd_in,scd_out=:scd_out,est_in=:est_in,est_out=:est_out,act_in=:act_in,act_out=:act_out WHERE point_id=:point_id";
  UQry.DeclareVariable( "point_id", otInteger );
  UQry.DeclareVariable( "scd_in", otDate );
  UQry.DeclareVariable( "scd_out", otDate );
  UQry.DeclareVariable( "est_in", otDate );
  UQry.DeclareVariable( "est_out", otDate );
  UQry.DeclareVariable( "act_in", otDate );
  UQry.DeclareVariable( "act_out", otDate );
  TQuery CQry(&OraSession);
  if ( prior ) {
    CQry.SQLText =
    "INSERT INTO dpoints( point_id,scd_in,scd_out,est_in,est_out,act_in,act_out,pscd_in,pscd_out,pest_in,pest_out,pact_in,pact_out )"
    "VALUES(:point_id,:scd_in,:scd_out,:est_in,:est_out,:act_in,:act_out,:pscd_in,:pscd_out,:pest_in,:pest_out,:pact_in,:pact_out)";
  }
  else {
    CQry.SQLText =
    "UPDATE dpoints SET nscd_in=:nscd_in,nscd_out=:nscd_out,nest_in=:nest_in,nest_out=:nest_out,nact_in=:nact_in,nact_out=:nact_out WHERE point_id=:point_id";
  }
  CQry.DeclareVariable( "point_id", otInteger );
  if ( prior ) {
    CQry.DeclareVariable( "scd_in", otDate );
    CQry.DeclareVariable( "scd_out", otDate );
    CQry.DeclareVariable( "est_in", otDate );
    CQry.DeclareVariable( "est_out", otDate );
    CQry.DeclareVariable( "act_in", otDate );
    CQry.DeclareVariable( "act_out", otDate );
    CQry.DeclareVariable( "pscd_in", otDate );
    CQry.DeclareVariable( "pscd_out", otDate );
    CQry.DeclareVariable( "pest_in", otDate );
    CQry.DeclareVariable( "pest_out", otDate );
    CQry.DeclareVariable( "pact_in", otDate );
    CQry.DeclareVariable( "pact_out", otDate );
  }
  else {
    CQry.DeclareVariable( "nscd_in", otDate );
    CQry.DeclareVariable( "nscd_out", otDate );
    CQry.DeclareVariable( "nest_in", otDate );
    CQry.DeclareVariable( "nest_out", otDate );
    CQry.DeclareVariable( "nact_in", otDate );
    CQry.DeclareVariable( "nact_out", otDate );
  }

  for ( ;!Qry.Eof; Qry.Next() ) {
  TDateTime putc_scd_in = NoExists,
            putc_scd_out = NoExists,
            putc_est_in  = NoExists,
            putc_est_out  = NoExists,
            putc_act_in  = NoExists,
            putc_act_out = NoExists;
  TDateTime nutc_scd_in = NoExists,
            nutc_scd_out = NoExists,
            nutc_est_in  = NoExists,
            nutc_est_out  = NoExists,
            nutc_act_in  = NoExists,
            nutc_act_out = NoExists;
  TDateTime plocal_scd_in = NoExists,
            plocal_scd_out = NoExists,
            plocal_est_in  = NoExists,
            plocal_est_out  = NoExists,
            plocal_act_in  = NoExists,
            plocal_act_out = NoExists;
  TDateTime nlocal_scd_in = NoExists,
            nlocal_scd_out = NoExists,
            nlocal_est_in  = NoExists,
            nlocal_est_out  = NoExists,
            nlocal_act_in  = NoExists,
            nlocal_act_out = NoExists;
    if ( !Qry.FieldIsNULL( "scd_in" ) ) {
      putc_scd_in = Qry.FieldAsDateTime("scd_in");
      gettime( putc_scd_in, nutc_scd_in,
               plocal_scd_in, nlocal_scd_in, Qry.FieldAsString( "region" ) );
    }
    if ( !Qry.FieldIsNULL( "scd_out" ) ) {
      putc_scd_out = Qry.FieldAsDateTime("scd_out");
      gettime( putc_scd_out, nutc_scd_out,
               plocal_scd_out, nlocal_scd_out, Qry.FieldAsString( "region" ) );
    }
    if ( !Qry.FieldIsNULL( "est_in" ) ) {
      putc_est_in = Qry.FieldAsDateTime("est_in");
      gettime( putc_est_in, nutc_est_in,
               plocal_est_in, nlocal_est_in, Qry.FieldAsString( "region" ) );
    }
    if ( !Qry.FieldIsNULL( "est_out" ) ) {
      putc_est_out = Qry.FieldAsDateTime("est_out");
      gettime( putc_est_out, nutc_est_out,
               plocal_est_out, nlocal_est_out, Qry.FieldAsString( "region" ) );
    }
    if ( !Qry.FieldIsNULL( "act_in" ) ) {
      putc_act_in = Qry.FieldAsDateTime("act_in");
      gettime( putc_act_in, nutc_act_in,
               plocal_act_in, nlocal_act_in, Qry.FieldAsString( "region" ) );
    }
    if ( !Qry.FieldIsNULL( "act_out" ) ) {
      putc_act_out = Qry.FieldAsDateTime("act_out");
      gettime( putc_act_out, nutc_act_out,
               plocal_act_out, nlocal_act_out, Qry.FieldAsString( "region" ) );
    }
    UQry.SetVariable( "point_id", Qry.FieldAsInteger("point_id") );
    CQry.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
    if ( nutc_scd_in != NoExists ) {
      UQry.SetVariable( "scd_in", nutc_scd_in );
      if ( prior ) {
        CQry.SetVariable( "scd_in", putc_scd_in );
        CQry.SetVariable( "pscd_in", plocal_scd_in );
      }
      else {
        CQry.SetVariable( "nscd_in", nlocal_scd_in );
      }
    }
    else {
      UQry.SetVariable( "scd_in", FNull );
      if ( prior ) {
        CQry.SetVariable( "scd_in", FNull );
        CQry.SetVariable( "pscd_in", FNull );
      }
      else {
        CQry.SetVariable( "nscd_in", FNull );
      }
    }
    if ( nutc_scd_out != NoExists ) {
      UQry.SetVariable( "scd_out", nutc_scd_out );
      if ( prior ) {
        CQry.SetVariable( "scd_out", putc_scd_out );
        CQry.SetVariable( "pscd_out", plocal_scd_out );
      }
      else {
        CQry.SetVariable( "nscd_out", nlocal_scd_out );
      }
    }
    else {
      UQry.SetVariable( "scd_out", FNull );
      if ( prior ) {
        CQry.SetVariable( "scd_out", FNull );
        CQry.SetVariable( "pscd_out", FNull );
      }
      else {
        CQry.SetVariable( "nscd_out", FNull );
      }
    }
    if ( nutc_est_in != NoExists ) {
      UQry.SetVariable( "est_in", nutc_est_in );
      if ( prior ) {
        CQry.SetVariable( "est_in", putc_est_in );
        CQry.SetVariable( "pest_in", plocal_est_in );
      }
      else {
        CQry.SetVariable( "nest_in", nlocal_scd_in );
      }
    }
    else {
      UQry.SetVariable( "est_in", FNull );
      if ( prior ) {
        CQry.SetVariable( "est_in", FNull );
        CQry.SetVariable( "pest_in", FNull );
      }
      else {
        CQry.SetVariable( "nest_in", FNull );
      }
    }
    if ( nutc_est_out != NoExists ) {
      UQry.SetVariable( "est_out", nutc_est_out );
      if ( prior ) {
        CQry.SetVariable( "est_out", putc_est_out );
        CQry.SetVariable( "pest_out", plocal_est_out );
      }
      else {
        CQry.SetVariable( "nest_out", nlocal_est_out );
      }
    }
    else {
      UQry.SetVariable( "est_out", FNull );
      if ( prior ) {
        CQry.SetVariable( "est_out", FNull );
        CQry.SetVariable( "pest_out", FNull );
      }
      else {
        CQry.SetVariable( "nest_out", FNull );
      }
    }
    if ( nutc_act_in != NoExists ) {
      UQry.SetVariable( "act_in", nutc_act_in );
      if ( prior ) {
        CQry.SetVariable( "act_in", putc_act_in );
        CQry.SetVariable( "pact_in", plocal_act_in );
      }
      else {
        CQry.SetVariable( "nact_in", nlocal_act_in );
      }
    }
    else {
      UQry.SetVariable( "act_in", FNull );
      if ( prior ) {
        CQry.SetVariable( "act_in", FNull );
        CQry.SetVariable( "pact_in", FNull );
      }
      else {
        CQry.SetVariable( "nact_in", FNull );
      }
    }
    if ( nutc_act_out != NoExists ) {
      UQry.SetVariable( "act_out", nutc_act_out );
      if ( prior ) {
        CQry.SetVariable( "act_out", putc_act_out );
        CQry.SetVariable( "pact_out", plocal_act_out );
      }
      else {
        CQry.SetVariable( "nact_out", nlocal_act_out );
      }
    }
    else {
      UQry.SetVariable( "act_out", FNull );
      if ( prior ) {
        CQry.SetVariable( "act_out", FNull );
        CQry.SetVariable( "pact_out", FNull );
      }
      else {
        CQry.SetVariable( "nact_out", FNull );
      }
    }
    tst();
    CQry.Execute();
    if ( !prior ) {
      UQry.Execute();
    }
  }
  OraSession.Commit();
  if ( !prior ) {
    Qry.Clear();
    Qry.SQLText =
      "SELECT point_id from dpoints "
      " WHERE NVL(pscd_in,to_date('25.03.16','DD.MM.YY'))!=NVL(nscd_in,to_date('25.03.16','DD.MM.YY')) OR "
      "        NVL(pscd_out,to_date('25.03.16','DD.MM.YY'))!=NVL(nscd_out,to_date('25.03.16','DD.MM.YY')) OR "
      "        NVL(pest_in,to_date('25.03.16','DD.MM.YY'))!=NVL(nest_in,to_date('25.03.16','DD.MM.YY')) OR "
      "        NVL(pest_out,to_date('25.03.16','DD.MM.YY'))!=NVL(nest_out,to_date('25.03.16','DD.MM.YY')) OR "
      "        NVL(pact_in,to_date('25.03.16','DD.MM.YY'))!=NVL(nact_in,to_date('25.03.16','DD.MM.YY')) OR "
      "        NVL(pact_out,to_date('25.03.16','DD.MM.YY'))!=NVL(nact_out,to_date('25.03.16','DD.MM.YY')) ";
    Qry.Execute();
    for ( ; !Qry.Eof; Qry.Next() ) {
      ProgError( STDLOG, "point_id=%d", Qry.FieldAsInteger( "point_id" ) );
    }
  }
  return 0;
}

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

namespace TZ_2_DB {
    void from_file(vector<vector<string> > &data)
    {
        ifstream input("date_time_zonespec.csv");
        bool pr_first = true;
        for(string line; getline(input, line);) {
            if(pr_first) {
                pr_first = not pr_first; //skip first row
                continue;
            }
            vector<string> values;
            vector<string> row;
            boost::algorithm::split(values, line, boost::is_any_of(","));
            for(vector<string>::const_iterator i = values.begin(); i != values.end(); i++) {
                string val = *i;
                boost::algorithm::erase_all(val, "\"");
                row.push_back(val);
            }
            if(row.size() < 11) throw logic_error((string)"not enough fields " + IntToString(row.size()) + ": " + line);
            if(row.size() > 11) throw logic_error((string)"too many fields " + IntToString(row.size()) + ": " + line);
            data.push_back(row);
        }
    }

    string rowToString(const vector<string> &val)
    {
        string result;
        for(vector<string>::const_iterator i = val.begin(); i != val.end(); i++) {
            if(not result.empty()) result += ", ";
            result += *i;
        }
        return result;
    }

    void to_db(vector<vector<string> > &data)
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "begin "
            "   execute immediate 'drop table new_date_time_zonespec'; "
            "   execute immediate 'create table new_date_time_zonespec  "
            "(   id varchar2(50) not null,  "
            "    std_abbr varchar2(5),  "
            "    std_name varchar2(50),  "
            "    dst_abbr varchar2(5),  "
            "    dst_name varchar2(50),  "
            "    gmt_offset varchar2(9),  "
            "    dst_adjustment varchar2(9),  "
            "    dst_start_date_rule varchar2(10),  "
            "    start_time varchar2(9),  "
            "    dst_end_date_rule varchar2(10),  "
            "    end_time varchar2(9) "
            ")'; "
            "end; ";
        Qry.Execute();

        Qry.SQLText =
            "insert into new_date_time_zonespec( "
            " id, "
            " std_abbr, "
            " std_name, "
            " dst_abbr, "
            " dst_name, "
            " gmt_offset, "
            " dst_adjustment, "
            " dst_start_date_rule, "
            " start_time, "
            " dst_end_date_rule, "
            " end_time "
            ") values ( "
            " :id, "
            " :std_abbr, "
            " :std_name, "
            " :dst_abbr, "
            " :dst_name, "
            " :gmt_offset, "
            " :dst_adjustment, "
            " :dst_start_date_rule, "
            " :start_time, "
            " :dst_end_date_rule, "
            " :end_time "
            ")";

        Qry.DeclareVariable("id", otString);
        Qry.DeclareVariable("std_abbr", otString);
        Qry.DeclareVariable("std_name", otString);
        Qry.DeclareVariable("dst_abbr", otString);
        Qry.DeclareVariable("dst_name", otString);
        Qry.DeclareVariable("gmt_offset", otString);
        Qry.DeclareVariable("dst_adjustment", otString);
        Qry.DeclareVariable("dst_start_date_rule", otString);
        Qry.DeclareVariable("start_time", otString);
        Qry.DeclareVariable("dst_end_date_rule", otString);
        Qry.DeclareVariable("end_time", otString);

        for(vector<vector<string> >::const_iterator row = data.begin(); row != data.end(); row++) {
            int idx = 0;
            if(row->size() < 11) throw logic_error((string)"not enough fields in row " + IntToString(row->size()) + ": " + rowToString(*row));
            if(row->size() > 11) throw logic_error((string)"too many fields in row " + IntToString(row->size()) + ": " + rowToString(*row));
            for(vector<string>::const_iterator val = row->begin(); val != row->end(); val++, idx++) {
                switch(idx) {
                    case 0: Qry.SetVariable("id", *val); break;
                    case 1: Qry.SetVariable("std_abbr", *val); break;
                    case 2: Qry.SetVariable("std_name", *val); break;
                    case 3: Qry.SetVariable("dst_abbr", *val); break;
                    case 4: Qry.SetVariable("dst_name", *val); break;
                    case 5: Qry.SetVariable("gmt_offset", *val); break;
                    case 6: Qry.SetVariable("dst_adjustment", *val); break;
                    case 7: Qry.SetVariable("dst_start_date_rule", *val); break;
                    case 8: Qry.SetVariable("start_time", *val); break;
                    case 9: Qry.SetVariable("dst_end_date_rule", *val); break;
                    case 10: Qry.SetVariable("end_time", *val); break;
                }
            }
            Qry.Execute();
        }
        cout << data.size() << " rows inserted." << endl;
    }
}



int tz2db(int argc,char **argv)
{
    vector<vector<string> > data;
    TZ_2_DB::from_file(data);
    TZ_2_DB::to_db(data);
    return 0;
}

int seasons_dst_format(int argc,char **argv)
{
  tst();
  vector<TSTrip> trips;
  try{
  TQuery Qry(&OraSession);
  //1 - ����
  Qry.Clear();
  Qry.SQLText =
    "SELECT * FROM ( "
    " SELECT DISTINCT trip_id,move_id,num,first_day,last_day,days,d.region FROM sched_days d "
    " WHERE move_id IN ( "
    " SELECT move_id FROM routes, airps,cities WHERE airps.code=routes.airp AND airps.city=cities.code AND tz_region='Asia/Novosibirsk' ) "
    " AND first_day > to_date('23.07.16','DD.MM.YY') ) d "
    " WHERE d.move_id NOT IN (SELECT move_id FROM sched_days WHERE move_id=d.move_id AND first_day <to_date('23.07.16','DD.MM.YY') ) "
    " ORDER BY trip_id,move_id,num ";
  tst();
  Qry.Execute();
  tst();

  TQuery VQry(&OraSession);
  VQry.SQLText = "SELECT move_id FROM routes where move_id=:move_id AND airline='7�' AND rownum<2";
  VQry.DeclareVariable( "move_id", otInteger );


  TQuery VSQry(&OraSession);
  VSQry.SQLText = "SELECT move_id from drop_prior_sched_days WHERE trip_id=:trip_id AND move_id=:move_id";
  VSQry.DeclareVariable( "trip_id", otInteger);
  VSQry.DeclareVariable( "move_id", otInteger);

  TQuery UQry(&OraSession);
  UQry.SQLText =
    "BEGIN "
    " INSERT INTO drop_prior_sched_days(trip_id,move_id,num,prior_first_day,prior_last_day,prior_days,"
    "                                   first_day,last_day,days) "
    "  SELECT trip_id,move_id,num,first_day,last_day,days,:new_first_day,:new_last_day,:new_days "
    "   FROM sched_days WHERE trip_id=:trip_id AND move_id=:move_id AND num=:num;"
    "UPDATE sched_days SET first_day=:new_first_day, last_day=:new_last_day, days=:new_days "
    " WHERE trip_id=:trip_id AND move_id=:move_id AND num=:num; "
    "END;";
  UQry.DeclareVariable( "trip_id", otInteger );
  UQry.DeclareVariable( "move_id", otInteger );
  UQry.DeclareVariable( "num", otInteger );
  UQry.DeclareVariable( "new_first_day", otDate );
  UQry.DeclareVariable( "new_last_day", otDate );
  UQry.DeclareVariable( "new_days", otString );
  TQuery RQry(&OraSession);
  RQry.SQLText =
    "SELECT num,airp,airline,flt_no,suffix,craft,scd_in,delta_in,scd_out,delta_out,trip_type,litera,pr_del,f,c,y FROM routes "
    " WHERE move_id=:move_id "
    "ORDER BY num";
  RQry.DeclareVariable( "move_id", otInteger );
  TQuery RUQry(&OraSession);
  RUQry.SQLText =
    "BEGIN "
    "INSERT INTO drop_prior_routes(move_id,num,prior_scd_in,prior_delta_in,"
    "                              scd_in,delta_in,prior_scd_out,prior_delta_out,"
    "                              scd_out,delta_out) "
    "SELECT move_id,num,scd_in,delta_in,:new_scd_in,:new_delta_in,scd_out,delta_out,:new_scd_out,:new_delta_out "
    " FROM routes WHERE move_id=:move_id AND num=:num; "
    " UPDATE routes SET scd_in=:new_scd_in,delta_in=:new_delta_in,scd_out=:new_scd_out,delta_out=:new_delta_out "
    "  WHERE move_id=:move_id AND num=:num;"
    "END;";
  RUQry.DeclareVariable( "move_id", otInteger );
  RUQry.DeclareVariable( "num", otInteger );
  RUQry.DeclareVariable( "new_scd_in", otDate );
  RUQry.DeclareVariable( "new_delta_in", otInteger );
  RUQry.DeclareVariable( "new_scd_out", otDate );
  RUQry.DeclareVariable( "new_delta_out", otInteger );
  int move_id = NoExists;
  TBaseTable &baseairps = base_tables.get( "airps" );
  while ( !Qry.Eof ) {
    TSTrip t;
    t.trip_id = Qry.FieldAsInteger( "trip_id" );
    t.move_id = Qry.FieldAsInteger( "move_id" );
    t.num = Qry.FieldAsInteger( "num" );
    VQry.SetVariable( "move_id", t.move_id );
    VQry.Execute();
    if ( !VQry.Eof ) {
      Qry.Next();
      continue;
    }
    VSQry.SetVariable( "trip_id", t.trip_id );
    VSQry.SetVariable( "move_id", t.move_id );
    VSQry.Execute();
    if ( !VSQry.Eof ) {
      Qry.Next();
      continue;
    }
    trips.push_back( t );

    TDateTime prior_first_day = Qry.FieldAsDateTime( "first_day" );
    TDateTime prior_last_day = Qry.FieldAsDateTime( "last_day" );
    string region = Qry.FieldAsString( "region" );
    TDateTime trunc_pf, trunc_f;
    modf( prior_first_day, &trunc_pf );
    int trip_id = Qry.FieldAsInteger( "trip_id" );
    int sched_num = Qry.FieldAsInteger( "num" );
    string prior_days = Qry.FieldAsString( "days" );
    TDateTime first_day, last_day;
    string days;
    first_day = prior_first_day + getdiffhours( region )/24.0;
    last_day = prior_last_day + getdiffhours( region)/24.0;
    ProgTrace( TRACE5, "start, trip_id=%d, move_id=%d, num=%d", trip_id, Qry.FieldAsInteger( "move_id" ), sched_num );
    ProgTrace( TRACE5, "prior_first_day=%f, trunc_pf=%f", prior_first_day, trunc_pf );
    modf( first_day, &trunc_f );
    int diff = (int)trunc_f - (int)trunc_pf; // ���室 �� ����

    if ( move_id != Qry.FieldAsInteger( "move_id" ) ) {
      move_id = Qry.FieldAsInteger( "move_id" );
      RQry.SetVariable( "move_id", move_id );
      RQry.Execute();
      TDateTime scd_in = ASTRA::NoExists, prior_scd_in = ASTRA::NoExists,
                scd_out = ASTRA::NoExists, prior_scd_out = ASTRA::NoExists;
      int delta_in = 0, prior_delta_in = 0, delta_out = 0, prior_delta_out = 0;
      bool pr_dest_flight_time = false;
      int date_diff = 0;
      while ( !RQry.Eof ) {
        if ( RQry.FieldIsNULL( "scd_in" ) )
          prior_scd_in = NoExists;
        else
          prior_scd_in = RQry.FieldAsDateTime( "scd_in" );
        prior_delta_in = RQry.FieldAsInteger( "delta_in" );

        int route_num = RQry.FieldAsInteger( "num" );
        string city = ((const TAirpsRow&)baseairps.get_row( "code", RQry.FieldAsString( "airp" )  , true )).city;
        const TCitiesRow& row=(const TCitiesRow&)base_tables.get("cities").get_row("code",city,true);
        string city_region;
        try {
          city_region = CityTZRegion( city );
          if ( city_region.empty() )
            throw EXCEPTIONS::Exception("region empty");
        }
        catch(...) {
         ProgError(STDLOG,"city=%s, region empty move_id=%d", city.c_str(), move_id );
         break;
        }
        if (  prior_scd_in != NoExists ) {
          ProgTrace( TRACE5, "before convert move_id=%d, route.num=%d, scd_in=%s, delta_in=%d, airp=%s, region=%s",
                     move_id, route_num, DateTimeToStr( prior_scd_in, "dd.mm.yyyy hh:nn" ).c_str(),
                     prior_delta_in, RQry.FieldAsString( "airp" ), city_region.c_str() );
          if ( row.country == "��" ) {
            scd_in = trunc_f + prior_delta_in + prior_scd_in;
            ProgTrace( TRACE5, "scd_in=%f, scd_in=%s", scd_in, DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str() );
            scd_in += getdiffhours(city_region)/24.0;
            ProgTrace( TRACE5, "scd_in=%f, scd_in=%s", scd_in, DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str() );
            TDateTime f2, f3;
            f2 = modf( scd_in, &f3 );
            if ( f3 < trunc_f )
              scd_in = f3 - trunc_f - f2;
            else
              scd_in = f3 - trunc_f + f2;
            double df = 0.;
            scd_in = modf( scd_in, &df );
            delta_in = (int)df;
          }
          else {
            scd_in = prior_scd_in;
            delta_in = prior_delta_in;
          }
          if ( !pr_dest_flight_time && row.country == "��" ) {
            pr_dest_flight_time = ( prior_delta_in == 0 );
            if ( pr_dest_flight_time && delta_in != 0 ) {
              date_diff = delta_in;
              delta_in = 0;
              ProgError( STDLOG, "move_id=%d, route_num=%d, delta_in set 0", move_id, route_num );
            }
          }
          else {
            delta_in -= date_diff;
            ProgError( STDLOG, "move_id=%d, route_num=%d, delta_in set -date_diff=%d", move_id, route_num, date_diff );
          }
          ProgTrace( TRACE5, "after convert scd_in=%s, delta_in=%d",
                     DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str(), delta_in );
        }
        if ( RQry.FieldIsNULL( "scd_out" ) )
          prior_scd_out = NoExists;
        else
          prior_scd_out = RQry.FieldAsDateTime( "scd_out" );
        prior_delta_out = RQry.FieldAsInteger( "delta_out" );
        if ( prior_scd_out != NoExists ) {
          ProgTrace( TRACE5, "before convert move_id=%d,route.num=%d, scd_out=%s, delta_out=%d, airp=%s",
                     move_id, route_num, DateTimeToStr( prior_scd_out, "dd.mm.yyyy hh:nn" ).c_str(),
                     prior_delta_out, RQry.FieldAsString( "airp" ) );
          if ( row.country == "��" ) {
            scd_out = trunc_f + prior_delta_out + prior_scd_out;
            ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
            scd_out += getdiffhours(city_region)/24.0;
            ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
            TDateTime f2, f3;
            f2 = modf( scd_out, &f3 );
            if ( f3 < trunc_f )
              scd_out = f3 - trunc_f - f2;
            else
              scd_out = f3 - trunc_f + f2;
            ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
            double df;
            scd_out = modf( scd_out, &df );
            delta_out = (int)df;
          }
          else {
            scd_out = prior_scd_out;
            delta_out = prior_delta_out;
          }
          if ( !pr_dest_flight_time && row.country == "��" ) {
            pr_dest_flight_time = ( prior_delta_out == 0 );
            if ( pr_dest_flight_time && delta_out != 0 ) {
              date_diff = delta_out;
              delta_out = 0;
              ProgError( STDLOG, "move_id=%d, route_num=%d, delta_out set 0", move_id, route_num );
            }
          }
          else {
            delta_out -= date_diff;
            ProgError( STDLOG, "move_id=%d, route_num=%d, delta_out set -date_diff=%d", move_id, route_num, date_diff );
          }
          ProgTrace( TRACE5, "after convert scd_out=%s, delta_out=%d",
                     DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str(), delta_out );
        }
        RUQry.SetVariable( "move_id", move_id );
        RUQry.SetVariable( "num", route_num );
        if ( prior_scd_in != NoExists ) {
          RUQry.SetVariable( "new_scd_in", scd_in );
          RUQry.SetVariable( "new_delta_in", delta_in );
        }
        else {
          RUQry.SetVariable( "new_scd_in", FNull );
          RUQry.SetVariable( "new_delta_in", FNull );
        }
        if ( prior_scd_out != NoExists ) {
          RUQry.SetVariable( "new_scd_out", scd_out );
          RUQry.SetVariable( "new_delta_out", delta_out );
        }
        else {
          RUQry.SetVariable( "new_scd_out", FNull );
          RUQry.SetVariable( "new_delta_out", FNull );
        }
        RUQry.Execute();
        RQry.Next();
      }
      if ( !pr_dest_flight_time )
        ProgError( STDLOG, "!pr_dest_flight_time, move_id=%d", move_id );
    }
    days = prior_days;
    ProgTrace( TRACE5, "days=%s", days.c_str() );
    if ( diff ) { // ���������� ��� �믮������
      ProgTrace( TRACE5, "diff days=%d", diff );
      days = AddDays( days, diff );
      ProgTrace( TRACE5, "days=%s", days.c_str() );
    }
    ProgTrace( TRACE5, "trip_id=%d, move_id=%d, num=%d, prior_first_day=%s, prior_last_day=%s, prior_days=%s, "
               "first_day=%s, last_day=%s, days=%s",
               trip_id, move_id, sched_num,
               DateTimeToStr( prior_first_day, "dd.mm.yyyy hh:nn" ).c_str(),
               DateTimeToStr( prior_last_day, "dd.mm.yyyy hh:nn" ).c_str(),
               prior_days.c_str(),
               DateTimeToStr( first_day, "dd.mm.yyyy hh:nn" ).c_str(),
               DateTimeToStr( last_day, "dd.mm.yyyy hh:nn" ).c_str(),
               days.c_str() );
    UQry.SetVariable( "trip_id", trip_id );
    UQry.SetVariable( "move_id", move_id );
    UQry.SetVariable( "num", sched_num );
    UQry.SetVariable( "new_first_day", first_day );
    UQry.SetVariable( "new_last_day", last_day );
    UQry.SetVariable( "new_days", days );
    UQry.Execute();
    Qry.Next();
    OraSession.Commit();
  }
    /*Qry.Clear();
    Qry.SQLText =
      "SELECT SCHED_DAYS.TRIP_ID,SCHED_DAYS.MOVE_ID,SCHED_DAYS.NUM,SCHED_DAYS.FIRST_DAY, airp,scd_in,delta_in,scd_out,delta_out,routes.num route_num "
      " FROM ROUTES,SCHED_DAYS, SEASONS, "
      "( SELECT id region FROM DATE_TIME_ZONESPEC "
      " WHERE id IN ("
      "'Asia/Sakhalin',"
      "'Europe/Astrakhan',"
      "'Europe/Ulyanovsk',"
      "'Asia/Barnaul',"
      "'Asia/Chita'"
      " ) ) d "
      " WHERE "
      " ROUTES.MOVE_ID=SCHED_DAYS.MOVE_ID AND "
      " seasons.region=d.region "
      " AND seasons.first > to_date('27.03.16','DD.MM.YY') AND "
      //" AND seasons.hours=1 AND "
        " SEASONS.REGION=SCHED_DAYS.REGION AND SCD_IN IS NOT NULL AND "
      " TRUNC(FIRST_DAY)+DELTA_IN+(SCD_IN-TRUNC(SCD_IN)) NOT BETWEEN SEASONS.FIRST AND SEASONS.LAST AND "
      " FIRST_DAY BETWEEN SEASONS.FIRST AND SEASONS.LAST "
      " UNION "
      " SELECT SCHED_DAYS.TRIP_ID,SCHED_DAYS.MOVE_ID,SCHED_DAYS.NUM,SCHED_DAYS.FIRST_DAY, airp,scd_in,delta_in,scd_out,delta_out,routes.num route_num "
      " FROM ROUTES,SCHED_DAYS, SEASONS, "
      "( SELECT id region FROM DATE_TIME_ZONESPEC "
      " WHERE id IN ("
        "'Asia/Sakhalin',"
        "'Europe/Astrakhan',"
        "'Europe/Ulyanovsk',"
        "'Asia/Barnaul',"
        "'Asia/Chita'"
      " ) ) d "
      " WHERE "
      " ROUTES.MOVE_ID=SCHED_DAYS.MOVE_ID AND "
      " seasons.region=d.region "
      " AND seasons.last > to_date('27.03.16','DD.MM.YY') AND "
        //" AND seasons.hours=1 AND "
      " SEASONS.REGION=SCHED_DAYS.REGION AND SCD_OUT IS NOT NULL AND "
      " TRUNC(FIRST_DAY)+DELTA_OUT+(SCD_OUT-TRUNC(SCD_OUT)) NOT BETWEEN SEASONS.FIRST AND SEASONS.LAST AND "
      " FIRST_DAY BETWEEN SEASONS.FIRST AND SEASONS.LAST "
      " order by first_day ";
    tst();
    Qry.Execute();
    tst();
    while ( !Qry.Eof ) {
      vector<TSTrip>::iterator i=trips.end();
      for ( vector<TSTrip>::iterator i=trips.begin(); i!=trips.end(); i++ ) {
        if ( i->trip_id == Qry.FieldAsInteger( "trip_id" ) &&
             i->move_id == Qry.FieldAsInteger( "move_id" ) ) {
          ProgTrace( TRACE5, "trip already convert: trip_id=%d, move_id=%d, num=%d", i->trip_id, i->move_id, i->num );
          break;
        }
      }
      if ( i == trips.end() ) {
        ProgTrace( TRACE5, "trip convert: trip_id=%d, move_id=%d, num=%d", Qry.FieldAsInteger( "trip_id" ), Qry.FieldAsInteger( "move_id" ), Qry.FieldAsInteger( "num" ) );
        string city = ((TAirpsRow&)baseairps.get_row( "code", Qry.FieldAsString( "airp" )  , true )).city;
        TCitiesRow& row=(TCitiesRow&)base_tables.get("cities").get_row("code",city,true);
        string city_region;
        try {
           city_region = CityTZRegion( city );
           if ( city_region.empty() )
             throw EXCEPTIONS::Exception("region empty");
        }
        catch(...) {
          ProgError(STDLOG,"city=%s, region empty move_id=%d", city.c_str(), move_id );
          Qry.Next();
          continue;
       }
        if ( row.country == "��" ) {
            TDateTime trunc_f = ASTRA::NoExists, scd_in = ASTRA::NoExists, prior_scd_in = ASTRA::NoExists,
                      scd_out = ASTRA::NoExists, prior_scd_out = ASTRA::NoExists;
            modf( Qry.FieldAsDateTime( "first_day" ), &trunc_f );
            int delta_in = 0, prior_delta_in = 0, delta_out = 0, prior_delta_out = 0;
            if ( Qry.FieldIsNULL( "scd_in" ) )
              prior_scd_in = NoExists;
            else
              prior_scd_in = Qry.FieldAsDateTime( "scd_in" );
            prior_delta_in = Qry.FieldAsInteger( "delta_in" );

            if (  prior_scd_in != NoExists ) {
              scd_in = trunc_f + prior_delta_in + prior_scd_in;
              ProgTrace( TRACE5, "scd_in=%f, scd_in=%s", scd_in, DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str() );
              scd_in += getdiffhours( city_region ) /24.0;
              ProgTrace( TRACE5, "scd_in=%f, scd_in=%s", scd_in, DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str() );
              TDateTime f2, f3;
              f2 = modf( scd_in, &f3 );
              if ( f3 < trunc_f )
                scd_in = f3 - trunc_f - f2;
              else
                scd_in = f3 - trunc_f + f2;
              double df;
              scd_in = modf( scd_in, &df );
              delta_in = (int)df;
            }

            if ( Qry.FieldIsNULL( "scd_out" ) )
              prior_scd_out = NoExists;
            else
              prior_scd_out = Qry.FieldAsDateTime( "scd_out" );
            prior_delta_out = Qry.FieldAsInteger( "delta_out" );
            if ( prior_scd_out != NoExists ) {
              scd_out = trunc_f + prior_delta_out + prior_scd_out;
              ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
              scd_out += getdiffhours( city_region )/24.0;
              ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
              TDateTime f2, f3;
              f2 = modf( scd_out, &f3 );
              if ( f3 < trunc_f )
                scd_out = f3 - trunc_f - f2;
              else
                scd_out = f3 - trunc_f + f2;
              ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
              double df;
              scd_out = modf( scd_out, &df );
              delta_out = (int)df;
            }
            RUQry.SetVariable( "move_id", Qry.FieldAsInteger( "move_id" ) );
            RUQry.SetVariable( "num", Qry.FieldAsInteger( "route_num" ) );
            if ( prior_scd_in != NoExists ) {
              RUQry.SetVariable( "new_scd_in", scd_in );
              RUQry.SetVariable( "new_delta_in", delta_in );
            }
            else {
              RUQry.SetVariable( "new_scd_in", FNull );
              RUQry.SetVariable( "new_delta_in", FNull );
            }
            if ( prior_scd_out != NoExists ) {
              RUQry.SetVariable( "new_scd_out", scd_out );
              RUQry.SetVariable( "new_delta_out", delta_out );
            }
            else {
              RUQry.SetVariable( "new_scd_out", FNull );
              RUQry.SetVariable( "new_delta_out", FNull );
            }
            RUQry.Execute();
        }
      }
      Qry.Next();
      OraSession.Commit();
    }*/


  }
  catch(...){
    throw;
  };
  return 0;
}
