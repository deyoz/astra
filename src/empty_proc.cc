//---------------------------------------------------------------------------
#include "basic.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "arx_daily.h"
#include "tlg/tlg_parser.h"
#include "tclmon/tcl_utils.h"
#include "serverlib/ourtime.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

const int sleepsec = 25;


int main_empty_proc_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(10);
    InitLogTime(NULL);
    OpenLogFile("log1");


    for( ;; )
    {
      sleep( sleepsec );
      InitLogTime(NULL);
      ProgTrace( TRACE0, "empty_proc: Next iteration");
    };
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
  };
  return 0;
}

#include "oralib.h"
#include "basic.h"
#include "astra_utils.h"

void prepare_upd_vars(TQuery &Qry, TQuery &UpdQry)
{
      UpdQry.SetVariable("grp_id",Qry.FieldAsInteger("grp_id"));
      if (!Qry.FieldIsNULL("grp_id_mark"))
      {
        //есть запись в market_flt
        UpdQry.SetVariable("airline_mark",Qry.FieldAsString("airline_mark"));
        UpdQry.SetVariable("flt_no_mark",Qry.FieldAsInteger("flt_no_mark"));
        UpdQry.SetVariable("suffix_mark",Qry.FieldAsString("suffix_mark"));
        UpdQry.SetVariable("scd_mark",Qry.FieldAsDateTime("scd_mark"));
        UpdQry.SetVariable("airp_dep_mark",Qry.FieldAsString("airp_dep_mark"));
        UpdQry.SetVariable("pr_mark_norms",(int)(Qry.FieldAsInteger("pr_mark_norms")!=0));
        UpdQry.SetVariable("point_id_mark",FNull);
      }
      else
      {
        //нет записи в market_flt
        UpdQry.SetVariable("airline_mark",Qry.FieldAsString("airline_oper"));

        if (!Qry.FieldIsNULL("flt_no_oper"))
          UpdQry.SetVariable("flt_no_mark",Qry.FieldAsInteger("flt_no_oper"));
        else
          UpdQry.SetVariable("flt_no_mark",FNull);

        UpdQry.SetVariable("suffix_mark",Qry.FieldAsString("suffix_oper"));
        if (!Qry.FieldIsNULL("scd_oper"))
        {
          //конвертим в локальную дату округленную до дня
          BASIC::TDateTime scd_oper=UTCToLocal(Qry.FieldAsDateTime("scd_oper"),AirpTZRegion(Qry.FieldAsString("airp_dep_oper")));
          modf(scd_oper,&scd_oper);
          UpdQry.SetVariable("scd_mark",scd_oper);
        }
        else
          UpdQry.SetVariable("scd_mark",FNull);
        UpdQry.SetVariable("airp_dep_mark",Qry.FieldAsString("airp_dep_oper"));
        UpdQry.SetVariable("pr_mark_norms",(int)false);
        UpdQry.SetVariable("point_id_mark",Qry.FieldAsInteger("point_dep"));
      };
};

int alter_db(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  
  Qry.Clear();
  Qry.SQLText="SELECT MIN(grp_id) AS min_grp_id, MAX(grp_id) AS max_grp_id FROM pax_grp";
  Qry.Execute();
  if (Qry.FieldIsNULL("min_grp_id") || Qry.FieldIsNULL("max_grp_id")) return 0;
  int min_grp_id=Qry.FieldAsInteger("min_grp_id");
  int max_grp_id=Qry.FieldAsInteger("max_grp_id");
  
  Qry.Clear();
  Qry.SQLText=
    "SELECT pax_grp.grp_id, "
    "       pax_grp.point_dep, "
    "       market_flt.grp_id AS grp_id_mark, "
    "       market_flt.airline AS airline_mark, "
    "       market_flt.flt_no AS flt_no_mark, "
    "       market_flt.suffix AS suffix_mark, "
    "       market_flt.scd AS scd_mark, "
    "       market_flt.airp_dep AS airp_dep_mark, "
    "       market_flt.pr_mark_norms, "
    "       points.airline AS airline_oper, "
    "       points.flt_no AS flt_no_oper, "
    "       points.suffix AS suffix_oper, "
    "       points.scd_out AS scd_oper, "
    "       points.airp AS airp_dep_oper "
    "FROM pax_grp,points,market_flt "
    "WHERE pax_grp.point_dep=points.point_id AND "
    "      pax_grp.grp_id=market_flt.grp_id(+) AND "
    "      pax_grp.grp_id>=:min_grp_id AND pax_grp.grp_id<:max_grp_id ";
    //"    AND pax_grp.point_id_mark IS NULL ";
          
  Qry.DeclareVariable("min_grp_id",otInteger);
  Qry.DeclareVariable("max_grp_id",otInteger);
  
  TQuery UpdQry(&OraSession);
  UpdQry.SQLText=
          "BEGIN "
          "  BEGIN "
          "    SELECT point_id INTO :point_id_mark FROM mark_trips "
          "    WHERE scd=:scd_mark AND airline=:airline_mark AND flt_no=:flt_no_mark AND airp_dep=:airp_dep_mark AND "
          "          (suffix IS NULL AND :suffix_mark IS NULL OR suffix=:suffix_mark) FOR UPDATE; "
          "  EXCEPTION "
          "    WHEN NO_DATA_FOUND THEN "
          "      IF :point_id_mark IS NULL THEN "
          "        SELECT id__seq.nextval INTO :point_id_mark FROM dual; "
          "      END IF; "
          "      INSERT INTO mark_trips(point_id,airline,flt_no,suffix,scd,airp_dep) "
          "      VALUES (:point_id_mark,:airline_mark,:flt_no_mark,:suffix_mark,:scd_mark,:airp_dep_mark); "
          "  END; "
          "  UPDATE pax_grp SET point_id_mark=:point_id_mark, pr_mark_norms=:pr_mark_norms WHERE grp_id=:grp_id; "
          "END;";
  UpdQry.DeclareVariable("grp_id",otInteger);
  UpdQry.DeclareVariable("point_id_mark",otInteger);
  UpdQry.DeclareVariable("airline_mark",otString);
  UpdQry.DeclareVariable("flt_no_mark",otInteger);
  UpdQry.DeclareVariable("suffix_mark",otString);
  UpdQry.DeclareVariable("scd_mark",otDate);
  UpdQry.DeclareVariable("airp_dep_mark",otString);
  UpdQry.DeclareVariable("pr_mark_norms",otInteger);
  
  while (min_grp_id<=max_grp_id)
  {
    Qry.SetVariable("min_grp_id",min_grp_id);
    Qry.SetVariable("max_grp_id",min_grp_id+ARX_MAX_ROWS());
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      prepare_upd_vars(Qry, UpdQry);
      UpdQry.Execute();
    };
    OraSession.Commit();
    min_grp_id+=ARX_MAX_ROWS();
  };

  puts("Database altered successfully");
  return 0;
};

using namespace ASTRA;
using namespace BASIC;

bool alter_arx(void)
{
  static time_t prior_exec=0;
  static int step=1;
  static TDateTime curr_part_key=NoExists;
  static TDateTime min_part_key=NoExists;
  static TDateTime max_part_key=NoExists;
  
  if (time(NULL)-prior_exec<ARX_SLEEP()) return false;
  
  time_t time_finish=time(NULL)+ARX_DURATION();
  
  TQuery Qry(&OraSession);

  if (curr_part_key==NoExists)
  {
    ProgTrace(TRACE5,"alter_arx START");
    for(int pass=1;pass<=2;pass++)
    {
      Qry.Clear();
      if (pass==1)
        Qry.SQLText="SELECT MIN(part_key) AS min_part_key, MAX(part_key) AS max_part_key FROM arx_pax_grp";
      else
        Qry.SQLText="SELECT MIN(part_key) AS min_part_key, MAX(part_key) AS max_part_key FROM arx_bag_receipts";
      Qry.Execute();
      if (!Qry.Eof)
      {
        if (!Qry.FieldIsNULL("min_part_key"))
        {
          if (min_part_key==NoExists || min_part_key>Qry.FieldAsDateTime("min_part_key"))
            min_part_key=Qry.FieldAsDateTime("min_part_key");
        };
        if (!Qry.FieldIsNULL("max_part_key"))
        {
          if (max_part_key==NoExists || max_part_key<Qry.FieldAsDateTime("max_part_key"))
            max_part_key=Qry.FieldAsDateTime("max_part_key");
        };
      };
    };

    if (min_part_key==NoExists || max_part_key==NoExists)
    {
      ProgTrace(TRACE5,"alter_arx FINISH: min_part_key==NoExists || max_part_key==NoExists");
      prior_exec=time(NULL);
      Qry.Clear();
      Qry.SQLText="UPDATE tasks SET pr_denial=1 WHERE name='alter_arx'";
      Qry.Execute();
      return true;
    };
    curr_part_key=min_part_key;
    modf(curr_part_key,&curr_part_key);
    ProgTrace(TRACE5, "alter_arx: min_part_key=%s, max_part_key=%s",
                      DateTimeToStr(min_part_key,"dd.mm.yy").c_str(),
                      DateTimeToStr(max_part_key,"dd.mm.yy").c_str());
  }
  else
  {
    ProgTrace(TRACE5, "alter_arx resumed  : curr_part_key=%s, step=%d",
                      DateTimeToStr(curr_part_key,"dd.mm.yy").c_str(), step);
  };
  
  Qry.Clear();
  Qry.SQLText=
    "SELECT arx_pax_grp.part_key, "
    "       arx_pax_grp.point_dep, "
    "       arx_pax_grp.grp_id, "
    "       arx_pax_grp.hall, "
    "       arx_pax_grp.user_id, "
    "       arx_market_flt.grp_id AS grp_id_mark, "
    "       arx_market_flt.airline AS airline_mark, "
    "       arx_market_flt.flt_no AS flt_no_mark, "
    "       arx_market_flt.suffix AS suffix_mark, "
    "       arx_market_flt.scd AS scd_mark, "
    "       arx_market_flt.airp_dep AS airp_dep_mark, "
    "       arx_market_flt.pr_mark_norms, "
    "       arx_points.airline AS airline_oper, "
    "       arx_points.flt_no AS flt_no_oper, "
    "       arx_points.suffix AS suffix_oper, "
    "       arx_points.scd_out AS scd_oper, "
    "       arx_points.airp AS airp_dep_oper "
    "FROM arx_pax_grp,arx_points,arx_market_flt "
    "WHERE arx_pax_grp.part_key=arx_points.part_key AND "
    "      arx_pax_grp.point_dep=arx_points.point_id AND "
    "      arx_pax_grp.part_key=arx_market_flt.part_key(+) AND "
    "      arx_pax_grp.grp_id=arx_market_flt.grp_id(+) AND "
    "      arx_pax_grp.part_key>=:first_part_key AND arx_pax_grp.part_key<:last_part_key AND "
    "      arx_pax_grp.point_id_mark IS NULL ";
  Qry.DeclareVariable("first_part_key", otDate);
  Qry.DeclareVariable("last_part_key", otDate);
    
  TQuery UpdQry(&OraSession);
  UpdQry.Clear();
  UpdQry.SQLText=
    "BEGIN "
    "  BEGIN "
    "    SELECT point_id INTO :point_id_mark FROM arx_mark_trips "
    "    WHERE part_key=:part_key AND scd=:scd_mark AND airline=:airline_mark AND flt_no=:flt_no_mark AND "
    "          airp_dep=:airp_dep_mark AND "
    "          (suffix IS NULL AND :suffix_mark IS NULL OR suffix=:suffix_mark) FOR UPDATE; "
    "  EXCEPTION "
    "    WHEN NO_DATA_FOUND THEN "
    "      IF :point_id_mark IS NULL THEN  "
    "        SELECT id__seq.nextval INTO :point_id_mark FROM dual; "
    "      END IF; "
    "      INSERT INTO arx_mark_trips(point_id,airline,flt_no,suffix,scd,airp_dep,part_key) "
    "      VALUES (:point_id_mark,:airline_mark,:flt_no_mark,:suffix_mark,:scd_mark,:airp_dep_mark,:part_key); "
    "      :rows_processed:=:rows_processed+SQL%ROWCOUNT; "
    "  END; "
    "  UPDATE arx_pax_grp SET point_id_mark=:point_id_mark, pr_mark_norms=:pr_mark_norms "
    "  WHERE part_key=:part_key AND point_dep=:point_dep AND grp_id=:grp_id AND point_id_mark IS NULL; "
    "  :rows_processed:=:rows_processed+SQL%ROWCOUNT; "
    "  UPDATE arx_bag2 "
    "  SET id=num, hall=:hall, user_id=:user_id "
    "  WHERE part_key=:part_key AND grp_id=:grp_id AND id IS NULL; "
    "  :rows_processed:=:rows_processed+SQL%ROWCOUNT; "
    "END;";
  UpdQry.DeclareVariable("part_key",otDate);
  UpdQry.DeclareVariable("point_dep",otInteger);
  UpdQry.DeclareVariable("grp_id",otInteger);
  UpdQry.DeclareVariable("point_id_mark",otInteger);
  UpdQry.DeclareVariable("airline_mark",otString);
  UpdQry.DeclareVariable("flt_no_mark",otInteger);
  UpdQry.DeclareVariable("suffix_mark",otString);
  UpdQry.DeclareVariable("scd_mark",otDate);
  UpdQry.DeclareVariable("airp_dep_mark",otString);
  UpdQry.DeclareVariable("pr_mark_norms",otInteger);
  UpdQry.DeclareVariable("rows_processed",otInteger);
  UpdQry.DeclareVariable("hall",otInteger);
  UpdQry.DeclareVariable("user_id",otInteger);
    
  TQuery Upd2Qry(&OraSession);
  Upd2Qry.Clear();
  Upd2Qry.SQLText=
    "UPDATE arx_bag_receipts SET desk_lang = 'RU' "
    "WHERE part_key>=:first_part_key AND part_key<:last_part_key AND "
    "      rownum<=:max_rows AND desk_lang IS NULL";
  Upd2Qry.DeclareVariable("first_part_key", otDate);
  Upd2Qry.DeclareVariable("last_part_key", otDate);
  Upd2Qry.CreateVariable("max_rows", otInteger, ARX_MAX_ROWS());

    
  for(;curr_part_key<=max_part_key; curr_part_key+=1.0) //цикл по дням
  {
    if (time_finish-time(NULL)<=0)
    {
      ProgTrace(TRACE5, "alter_arx suspended: curr_part_key=%s, step=%d",
                        DateTimeToStr(curr_part_key,"dd.mm.yy").c_str(), step);
      prior_exec=time(NULL);
      return false;
    };
    for(;step<=2;step++)
    {
      if (time_finish-time(NULL)<=0)
      {
        ProgTrace(TRACE5, "alter_arx suspended: curr_part_key=%s, step=%d",
                          DateTimeToStr(curr_part_key,"dd.mm.yy").c_str(), step);
        prior_exec=time(NULL);
        return false;
      };
      if (step==1)
      {
        Qry.SetVariable("first_part_key", curr_part_key);
        Qry.SetVariable("last_part_key", curr_part_key+1.0);
        Qry.Execute();

        UpdQry.SetVariable("rows_processed",(int)0);
        if (!Qry.Eof)
        {
          for(;!Qry.Eof;Qry.Next())
          {
            UpdQry.SetVariable("part_key",Qry.FieldAsDateTime("part_key"));
            UpdQry.SetVariable("point_dep",Qry.FieldAsInteger("point_dep"));
            if (!Qry.FieldIsNULL("hall"))
              UpdQry.SetVariable("hall",Qry.FieldAsInteger("hall"));
            else
              UpdQry.SetVariable("hall",FNull);
            UpdQry.SetVariable("user_id",Qry.FieldAsInteger("user_id"));
            prepare_upd_vars(Qry, UpdQry);
            UpdQry.Execute();
            if (UpdQry.GetVariableAsInteger("rows_processed")>=ARX_MAX_ROWS())
            {
              OraSession.Commit();
            /*  ProgTrace(TRACE5, "alter_arx: curr_part_key=%s, step=%d: %d rows commited",
                                DateTimeToStr(curr_part_key,"dd.mm.yy").c_str(),
                                step, UpdQry.GetVariableAsInteger("rows_processed"));*/
              UpdQry.SetVariable("rows_processed",(int)0);
            };
          };
          OraSession.Commit();
          /*if (UpdQry.GetVariableAsInteger("rows_processed")>0)
            ProgTrace(TRACE5, "alter_arx: curr_part_key=%s, step=%d: %d rows commited",
                              DateTimeToStr(curr_part_key,"dd.mm.yy").c_str(),
                              step, UpdQry.GetVariableAsInteger("rows_processed"));*/
        };
      };
      if (step==2)
      {
        Upd2Qry.SetVariable("first_part_key", curr_part_key);
        Upd2Qry.SetVariable("last_part_key", curr_part_key+1.0);
        do
        {
          Upd2Qry.Execute();
          OraSession.Commit();
          /*if (Upd2Qry.RowsProcessed()>0)
            ProgTrace(TRACE5, "alter_arx: curr_part_key=%s, step=%d: %d rows commited",
                              DateTimeToStr(curr_part_key,"dd.mm.yy").c_str(),
                              step, Upd2Qry.RowsProcessed());*/
        }
        while(Upd2Qry.RowsProcessed()>=ARX_MAX_ROWS());
      };
    };
    if (step>2) step=1;
  };
  
  ProgTrace(TRACE5,"alter_arx FINISH");
  prior_exec=time(NULL);
  Qry.Clear();
  Qry.SQLText="UPDATE tasks SET pr_denial=1 WHERE name='alter_arx'";
  Qry.Execute();
  return true;
};

using namespace std;

void voland_wait(int processed)
{
  static time_t start_time=time(NULL);
  if (time(NULL)-start_time>=5)
  {
    printf("%d iterations processed. sleep...", processed);
    fflush(stdout);
    sleep(5);
    printf("go!\n");
    start_time=time(NULL);
  };
};

int voland_points(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT part_key, move_id, point_id, airline, flt_no, suffix, airp, scd_out, "
    "       NVL(act_out,NVL(est_out,scd_out)) AS act_out, craft, bort, "
    "       NVL(act_in,NVL(est_in,scd_in)) AS act_in "
    "FROM arx_points "
    "WHERE part_key >= :curr_date AND part_key < :curr_date+1 AND "
    "      pr_del=0 "
    "ORDER BY move_id, point_num";
  Qry.DeclareVariable("curr_date", otDate);
  
  TQuery CompQry(&OraSession);
  CompQry.Clear();
  CompQry.SQLText=
    "SELECT part_key, point_id, class, cfg "
    "FROM arx_trip_classes,classes "
    "WHERE arx_trip_classes.class=classes.code AND "
    "      part_key=:part_key AND point_id=:point_id "
    "ORDER BY classes.priority";
  CompQry.DeclareVariable("part_key", otDate);
  CompQry.DeclareVariable("point_id", otInteger);
  
  TQuery InsQry(&OraSession);
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO voland_points "
    " (part_key,move_id,point_id,scd_out_utc,scd_out_local,day_of_week,airline,flt_no,suffix, "
    "  airp_out,act_out_utc,act_out_local,airp_in,act_in_utc,act_in_local,craft,bort,comp) "
    "VALUES "
    " (:part_key,:move_id,:point_id,:scd_out_utc,:scd_out_local,:day_of_week,:airline,:flt_no,:suffix, "
    "  :airp_out,:act_out_utc,:act_out_local,:airp_in,:act_in_utc,:act_in_local,:craft,:bort,:comp) ";
  InsQry.DeclareVariable("part_key", otDate);
  InsQry.DeclareVariable("move_id", otInteger);
  InsQry.DeclareVariable("point_id", otInteger);
  InsQry.DeclareVariable("scd_out_utc", otDate);
  InsQry.DeclareVariable("scd_out_local", otDate);
  InsQry.DeclareVariable("day_of_week", otInteger);
  InsQry.DeclareVariable("airline", otString);
  InsQry.DeclareVariable("flt_no", otInteger);
  InsQry.DeclareVariable("suffix", otString);
  InsQry.DeclareVariable("airp_out", otString);
  InsQry.DeclareVariable("act_out_utc", otDate);
  InsQry.DeclareVariable("act_out_local", otDate);
  InsQry.DeclareVariable("airp_in", otString);
  InsQry.DeclareVariable("act_in_utc", otDate);
  InsQry.DeclareVariable("act_in_local", otDate);
  InsQry.DeclareVariable("craft", otString);
  InsQry.DeclareVariable("bort", otString);
  InsQry.DeclareVariable("comp", otString);

  TDateTime first_range_date,last_range_date;
  StrToDateTime("01.01.2011", "dd.mm.yyyy", first_range_date);
  StrToDateTime("31.03.2011", "dd.mm.yyyy", last_range_date);
  
  int rows_processed=0;
  
  for(TDateTime curr_date=first_range_date;
      curr_date<=last_range_date+arx_trip_date_range;
      curr_date+=1.0)
  {
    voland_wait(rows_processed);
    Qry.SetVariable("curr_date", curr_date);
    Qry.Execute();
    for(;!Qry.Eof;)
    {
      if (Qry.FieldIsNULL("scd_out"))
      {
        Qry.Next();
        continue;
      };
      int move_id=Qry.FieldAsInteger("move_id");
      TDateTime part_key=Qry.FieldAsDateTime("part_key");
      int point_id=Qry.FieldAsInteger("point_id");
      try
      {
        InsQry.SetVariable("part_key", part_key);
        InsQry.SetVariable("move_id", move_id);
        InsQry.SetVariable("point_id", point_id);
        InsQry.SetVariable("scd_out_utc", Qry.FieldAsDateTime("scd_out") );
        TDateTime scd_out_local=UTCToLocal(Qry.FieldAsDateTime("scd_out"), AirpTZRegion(Qry.FieldAsString("airp")));
        InsQry.SetVariable("scd_out_local", scd_out_local);
        InsQry.SetVariable("day_of_week", DayOfWeek(scd_out_local) );
        InsQry.SetVariable("airline", Qry.FieldAsString("airline"));
        InsQry.SetVariable("flt_no", Qry.FieldAsInteger("flt_no"));
        InsQry.SetVariable("suffix", Qry.FieldAsString("suffix"));
        InsQry.SetVariable("airp_out", Qry.FieldAsString("airp"));
        InsQry.SetVariable("act_out_utc", Qry.FieldAsDateTime("act_out"));
        InsQry.SetVariable("act_out_local", UTCToLocal(Qry.FieldAsDateTime("act_out"), AirpTZRegion(Qry.FieldAsString("airp"))));
        InsQry.SetVariable("craft", Qry.FieldAsString("craft"));
        InsQry.SetVariable("bort", Qry.FieldAsString("bort"));
        //компоновка
        std::ostringstream comp;
        CompQry.SetVariable("part_key", part_key);
        CompQry.SetVariable("point_id", point_id);
        CompQry.Execute();
        for(;!CompQry.Eof; CompQry.Next())
          comp << CompQry.FieldAsString("class") << setw(2) << setfill('0') << CompQry.FieldAsInteger("cfg") << " ";

        InsQry.SetVariable("comp", comp.str());
      }
      catch(EXCEPTIONS::Exception &e)
      {
        ProgError(STDLOG, "Exception: %s", e.what());
        Qry.Next();
        continue;
      };
      Qry.Next();
      try
      {
        if (!Qry.Eof && move_id==Qry.FieldAsInteger("move_id"))
        {
          InsQry.SetVariable("airp_in", Qry.FieldAsString("airp"));
          if (!Qry.FieldIsNULL("act_in"))
          {
            InsQry.SetVariable("act_in_utc", Qry.FieldAsDateTime("act_in"));
            InsQry.SetVariable("act_in_local", UTCToLocal(Qry.FieldAsDateTime("act_in"), AirpTZRegion(Qry.FieldAsString("airp"))));
          }
          else
          {
            InsQry.SetVariable("act_in_utc", FNull);
            InsQry.SetVariable("act_in_local", FNull);
          };
          InsQry.Execute();
          rows_processed++;
        };
      }
      catch(EXCEPTIONS::Exception &e)
      {
        ProgError(STDLOG, "Exception: %s", e.what());
        continue;
      };
    };
    OraSession.Commit();
  };
  
  return 0;
};

int voland_pax(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT part_key, point_id FROM voland_points";
    
  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  PaxQry.SQLText=
      "INSERT INTO voland_pax(part_key, point_id, class, subclass, pax_count) "
      "SELECT :part_key, :point_id, class, subclass, COUNT(*) "
      "FROM arx_pax_grp, arx_pax "
      "WHERE arx_pax.part_key=arx_pax_grp.part_key AND "
      "      arx_pax.grp_id=arx_pax_grp.grp_id AND  "
      "      arx_pax_grp.part_key=:part_key AND arx_pax_grp.point_dep=:point_id AND "
      "      arx_pax.refuse IS NULL "
      "GROUP BY arx_pax_grp.class, arx_pax.subclass";
  PaxQry.DeclareVariable("part_key", otDate);
  PaxQry.DeclareVariable("point_id", otInteger);
  
  int rows_processed=0;
  
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    voland_wait(rows_processed);
    PaxQry.SetVariable("part_key", Qry.FieldAsDateTime("part_key"));
    PaxQry.SetVariable("point_id", Qry.FieldAsInteger("point_id"));
    PaxQry.Execute();
    if (PaxQry.RowsProcessed()>0)
    {
      rows_processed+=PaxQry.RowsProcessed();
      OraSession.Commit();
    };
  };
  OraSession.Commit();
  
  return 0;
};

int voland_events(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT part_key, move_id, point_id FROM voland_points";
    
  TQuery EventsQry(&OraSession);
  EventsQry.Clear();
  EventsQry.SQLText=
      "INSERT INTO voland_events(part_key, point_id, time, ev_order, msg) "
      "SELECT :part_key, :point_id, time, ev_order, msg "
      "FROM arx_events "
      "WHERE part_key=:part_key AND "
      "      type=:type AND "
      "      id1=:move_id AND "
      "      id2=:point_id AND "
      "      (msg like 'Изменение типа ВС%' OR "
      "       msg like 'Назначение ВС%' OR "
      "       msg like 'Изменение борта%' OR "
      "       msg like 'Назначение борта%') ";
             
  EventsQry.DeclareVariable("part_key", otDate);
  EventsQry.DeclareVariable("move_id", otInteger);
  EventsQry.DeclareVariable("point_id", otInteger);
  EventsQry.CreateVariable("type", otString, EncodeEventType(ASTRA::evtDisp));

  TQuery Events2Qry(&OraSession);
  Events2Qry.Clear();
  Events2Qry.SQLText=
      "INSERT INTO voland_events(part_key, point_id, time, ev_order, msg) "
      "SELECT :part_key, :point_id, time, ev_order, msg "
      "FROM arx_events "
      "WHERE part_key=:part_key AND "
      "      type=:type AND "
      "      id1=:point_id AND "
      "      (msg like 'Назначена базовая компоновка%') ";

  Events2Qry.DeclareVariable("part_key", otDate);
  Events2Qry.DeclareVariable("point_id", otInteger);
  Events2Qry.CreateVariable("type", otString, EncodeEventType(ASTRA::evtFlt));

  int rows_processed=0;

  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    voland_wait(rows_processed);
    EventsQry.SetVariable("part_key", Qry.FieldAsDateTime("part_key"));
    EventsQry.SetVariable("move_id", Qry.FieldAsInteger("move_id"));
    EventsQry.SetVariable("point_id", Qry.FieldAsInteger("point_id"));
    EventsQry.Execute();
    if (EventsQry.RowsProcessed()>0)
    {
      rows_processed+=EventsQry.RowsProcessed();
      OraSession.Commit();
    };
    voland_wait(rows_processed);
    Events2Qry.SetVariable("part_key", Qry.FieldAsDateTime("part_key"));
    Events2Qry.SetVariable("point_id", Qry.FieldAsInteger("point_id"));
    Events2Qry.Execute();
    if (Events2Qry.RowsProcessed()>0)
    {
      rows_processed+=Events2Qry.RowsProcessed();
      OraSession.Commit();
    };
  };
  OraSession.Commit();
    
  return 0;
};

int voland_events2(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT part_key, point_id, msg FROM voland_events";
    
  TQuery EventsQry(&OraSession);
  EventsQry.Clear();
  EventsQry.SQLText=
    "UPDATE voland_events SET comp_id=:comp_id "
    "WHERE part_key=:part_key AND point_id=:point_id AND msg=:msg";
  EventsQry.DeclareVariable("part_key", otDate);
  EventsQry.DeclareVariable("point_id", otInteger);
  EventsQry.DeclareVariable("msg", otString);
  EventsQry.DeclareVariable("comp_id", otInteger);

  int rows_processed=0;
  int comp_id;

  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    voland_wait(rows_processed);
    if (sscanf(Qry.FieldAsString("msg"), "Назначена базовая компоновка (ид=%d",&comp_id)!=0)
    {
      EventsQry.SetVariable("part_key", Qry.FieldAsDateTime("part_key"));
      EventsQry.SetVariable("point_id", Qry.FieldAsInteger("point_id"));
      EventsQry.SetVariable("msg", Qry.FieldAsString("msg"));
      EventsQry.SetVariable("comp_id", comp_id);
      EventsQry.Execute();
      if (EventsQry.RowsProcessed()>0)
      {
        rows_processed+=EventsQry.RowsProcessed();
        OraSession.Commit();
      };
    };
  };
  OraSession.Commit();

  return 0;
};

int voland_stat(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT scd_out_utc, airline, flt_no, suffix, airp_out, act_out_utc, "
    "       airp_in, act_in_utc, craft, bort, comp, part_key, point_id "
    "FROM voland_points "
    "WHERE part_key >= :curr_date AND part_key < :curr_date+1 AND airline='ЮТ' "
    "ORDER BY scd_out_utc";

  Qry.DeclareVariable("curr_date", otDate);
  
  TQuery PaxQry(&OraSession);
  PaxQry.Clear();
  PaxQry.SQLText=
    "SELECT voland_pax.class, voland_pax.subclass, pax_count "
    "FROM voland_pax, classes "
    "WHERE voland_pax.class=classes.code AND "
    "      voland_pax.part_key=:part_key AND voland_pax.point_id=:point_id "
    "ORDER BY classes.priority";
  PaxQry.DeclareVariable("part_key", otDate);
  PaxQry.DeclareVariable("point_id", otInteger);
  
  TQuery EventsQry(&OraSession);
  EventsQry.Clear();
  EventsQry.SQLText=
    "SELECT msg FROM voland_events "
    "WHERE voland_events.part_key=:part_key AND voland_events.point_id=:point_id "
    "ORDER BY time,ev_order";
  EventsQry.DeclareVariable("part_key", otDate);
  EventsQry.DeclareVariable("point_id", otInteger);
  
  TQuery CompQry(&OraSession);
  CompQry.Clear();
  CompQry.SQLText=
    "SELECT class, cfg "
    "FROM comp_classes,classes "
    "WHERE comp_classes.class=classes.code AND "
    "      comp_id=:comp_id "
    "ORDER BY classes.priority";
  CompQry.DeclareVariable("comp_id", otInteger);
  
  TDateTime first_range_date,last_range_date;
  StrToDateTime("01.01.2011", "dd.mm.yyyy", first_range_date);
  StrToDateTime("01.04.2011"/*"03.01.2011"*/, "dd.mm.yyyy", last_range_date);

  ofstream f;
  f.open("astra_stat.txt");
  if (!f.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'","astra_stat.txt");
  
  ofstream f2;
  f2.open("astra_events.txt");
  if (!f2.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'","astra_events.txt");
  
  int rows_processed=0;

  for(TDateTime curr_date=first_range_date;
      curr_date<=last_range_date+arx_trip_date_range;
      curr_date+=1.0)
  {
    voland_wait(rows_processed);
    Qry.SetVariable("curr_date", curr_date);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      if (Qry.FieldIsNULL("scd_out_utc")) continue;
      if (Qry.FieldIsNULL("airline")) continue;
      TDateTime scd_out_utc=Qry.FieldAsDateTime("scd_out_utc");
      if (scd_out_utc<first_range_date || scd_out_utc>=last_range_date) continue;
      
      ostringstream r;
      
      r << DateTimeToStr(scd_out_utc,"dd.mm.yyyy") << ";"
        << DayOfWeek(scd_out_utc) << ";"
        << Qry.FieldAsString("airline") << setw(3) << setfill('0') << Qry.FieldAsInteger("flt_no") << Qry.FieldAsString("suffix") << ";"
        << Qry.FieldAsString("airp_out") << ";";
      if (!Qry.FieldIsNULL("act_out_utc"))
        r << DateTimeToStr(Qry.FieldAsDateTime("act_out_utc"),"hh:nn") << ";"
          << DateTimeToStr(Qry.FieldAsDateTime("act_out_utc"),"dd.mm.yyyy") << ";";
      else
        r << ";;";
      r << Qry.FieldAsString("airp_in") << ";";
      if (!Qry.FieldIsNULL("act_in_utc"))
        r << DateTimeToStr(Qry.FieldAsDateTime("act_in_utc"),"hh:nn") << ";"
          << DateTimeToStr(Qry.FieldAsDateTime("act_in_utc"),"dd.mm.yyyy") << ";";
      else
        r << ";;";
      f << r.str();
        
      f << Qry.FieldAsString("craft") << ";"
        << Qry.FieldAsString("bort") << ";"
        << Qry.FieldAsString("comp") << ";";
        
      ostringstream str1,str2;

      PaxQry.SetVariable("part_key", Qry.FieldAsDateTime("part_key"));
      PaxQry.SetVariable("point_id", Qry.FieldAsInteger("point_id"));
      PaxQry.Execute();
      int cls_pax_count=0;
      for(;!PaxQry.Eof;)
      {
        string cls=PaxQry.FieldAsString("class");
        cls_pax_count+=PaxQry.FieldAsInteger("pax_count");
        if (!PaxQry.FieldIsNULL("subclass"))
          str2 << PaxQry.FieldAsString("subclass") << PaxQry.FieldAsInteger("pax_count") << " ";
        PaxQry.Next();
        if (PaxQry.Eof || cls!=PaxQry.FieldAsString("class"))
        {
          str1 << cls << cls_pax_count << " ";
          cls_pax_count=0;
        };
      };
      f << str1.str() << ";"
        << str2.str() << "\n";
        
      EventsQry.SetVariable("part_key", Qry.FieldAsDateTime("part_key"));
      EventsQry.SetVariable("point_id", Qry.FieldAsInteger("point_id"));
      EventsQry.Execute();
      int comp_id;
      string craft,bort;
      ostringstream comp;
      char buf[100];
      for(;!EventsQry.Eof;EventsQry.Next())
      {
        string msg=EventsQry.FieldAsString("msg");
      
        if (sscanf(msg.c_str(), "Назначена базовая компоновка (ид=%d",&comp_id)!=0)
        {
          comp.str("");
          CompQry.SetVariable("comp_id", comp_id);
          CompQry.Execute();
          for(;!CompQry.Eof; CompQry.Next())
            comp << CompQry.FieldAsString("class") << setw(2) << setfill('0') << CompQry.FieldAsInteger("cfg") << " ";
        }
        else
        if (sscanf(msg.c_str(), "Назначение ВС%s",buf)!=0)
        {
          craft=buf;
        }
        else
        if (sscanf(msg.c_str(), "Изменение типа ВС на%s",buf)!=0)
        {
          craft=buf;
        }
        else
        if (sscanf(msg.c_str(), "Назначение борта%s",buf)!=0)
        {
          bort=buf;
        }
        else
        if (sscanf(msg.c_str(), "Изменение борта на%s",buf)!=0)
        {
          bort=buf;
        };
        f2 << r.str()
           << craft << ";"
           << bort << ";"
           << comp.str() << ";"
           << msg << "\n";
        
      };
        
        
      rows_processed++;
    };
  };
  f.close();
  f2.close();
  
  ofstream f3;
  f3.open("astra_comps.txt");
  if (!f3.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'","astra_comps.txt");

  Qry.Clear();
  Qry.SQLText=
    "SELECT comp_id, craft, bort, descr "
    "FROM comps "
    "WHERE airline='ЮТ' "
    "ORDER BY craft, bort, time_create";
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    ostringstream comp;
    CompQry.SetVariable("comp_id", Qry.FieldAsInteger("comp_id"));
    CompQry.Execute();
    for(;!CompQry.Eof; CompQry.Next())
      comp << CompQry.FieldAsString("class") << setw(2) << setfill('0') << CompQry.FieldAsInteger("cfg") << " ";
  
    f3 << Qry.FieldAsString("craft") << ";"
       << Qry.FieldAsString("bort") << ";"
       << comp.str() << ";"
       << Qry.FieldAsString("descr") << "\n";
  };
  
  f3.close();

  return 0;
};

int evt_season(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(part_key) AS min_part_key FROM arx_events";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_part_key")) return 0;
  TDateTime min_part_key=Qry.FieldAsDateTime("min_part_key");
  
  Qry.SQLText="SELECT MAX(part_key) AS max_part_key FROM arx_events";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_part_key")) return 0;
  TDateTime max_part_key=Qry.FieldAsDateTime("max_part_key");
  
  Qry.Clear();
  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE evt_season_from_arx(low_part_key IN DATE, high_part_key IN DATE) "
    "IS "
    "  TYPE TRowidsTable IS TABLE OF ROWID; "
    "  rowids     TRowidsTable; "
    "BEGIN "
    "  SELECT rowid BULK COLLECT INTO rowids "
    "  FROM arx_events "
    "  WHERE part_key>=low_part_key AND part_key<high_part_key AND "
    "        type=system.evtSeason FOR UPDATE; "
    "  FORALL i IN 1..rowids.COUNT "
    "    INSERT INTO events "
    "      (type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3) "
    "    SELECT "
    "       type,time,ev_order,msg,screen,ev_user,station,id1,id2,id3 "
    "    FROM arx_events "
    "    WHERE rowid=rowids(i); "
    "  FORALL i IN 1..rowids.COUNT "
    "    DELETE FROM arx_events WHERE rowid=rowids(i); "
    "  IF rowids.COUNT>0 THEN COMMIT; END IF; "
    "END;";
  Qry.Execute();
  
  Qry.SQLText=
    "BEGIN "
    "  evt_season_from_arx(:low_part_key, :high_part_key); "
    "END;";
  Qry.DeclareVariable("low_part_key", otDate);
  Qry.DeclareVariable("high_part_key", otDate);

  int processed=0;
  
  for(TDateTime curr_part_key=min_part_key; curr_part_key<=max_part_key; curr_part_key+=1.0/24, processed++)
  {
    voland_wait(processed);
    Qry.SetVariable("low_part_key",curr_part_key);
    Qry.SetVariable("high_part_key",curr_part_key+1.0/24);
    Qry.Execute();
  };
  
  Qry.Clear();
  Qry.SQLText="DROP PROCEDURE evt_season_from_arx";
  Qry.Execute();
  
  return 0;
};
/*
int evt_season2(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(time) AS min_time FROM events";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_time")) return 0;
  TDateTime min_time=Qry.FieldAsDateTime("min_time");

  Qry.SQLText="SELECT MAX(time) AS max_time FROM events";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_time")) return 0;
  TDateTime max_time=Qry.FieldAsDateTime("max_time");

  Qry.SQLText=
    "SELECT * FROM events WHERE time>=:low_time AND time<:high_time AND UPPER(type)=:evt_season";

  return 0;
}; */

int alter_pax_doc(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(pax_id) AS min_pax_id FROM pax";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_pax_id")) return 0;
  int min_pax_id=Qry.FieldAsInteger("min_pax_id");

  Qry.SQLText="SELECT MAX(pax_id) AS max_pax_id FROM pax";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_pax_id")) return 0;
  int max_pax_id=Qry.FieldAsInteger("max_pax_id");
  
  Qry.SQLText="SELECT tid__seq.nextval AS tid FROM dual";
  Qry.Execute();
  int tid=Qry.FieldAsInteger("tid");
  
  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE alter_pax_doc(low_pax_id IN pax.pax_id%TYPE, high_pax_id IN pax.pax_id%TYPE, "
    "                                          vtid pax.tid%TYPE, processed IN OUT BINARY_INTEGER) "
    "IS "
    "  CURSOR cur(vlow_pax_id IN pax.pax_id%TYPE, vhigh_pax_id IN pax.pax_id%TYPE) IS "
    "    SELECT pax.pax_id,drop_document AS document "
    "    FROM pax, pax_doc "
    "    WHERE pax.pax_id=pax_doc.pax_id(+) AND "
    "          pax.pax_id>=vlow_pax_id AND pax.pax_id<vhigh_pax_id AND "
    "          drop_document IS NOT NULL AND (pax_doc.no IS NULL OR pax_doc.no<>drop_document) FOR UPDATE; "
    "  vtype              pax_doc.type%TYPE; "
    "  vissue_country     pax_doc.issue_country%TYPE; "
    "  vno                pax_doc.no%TYPE; "
    "  i                  BINARY_INTEGER; "
    "  CURSOR cur1(vpax_id IN pax.pax_id%TYPE, vdocument IN VARCHAR2) IS "
    "    SELECT type,issue_country,no,nationality, "
    "           birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi "
    "    FROM crs_pax_doc "
    "    WHERE pax_id=vpax_id AND no=vdocument "
    "    ORDER BY DECODE(type,'P',0,NULL,2,1), DECODE(rem_code,'DOCS',0,1), no; "
    "  row1 cur1%ROWTYPE; "
    "BEGIN "
    "  i:=0; "
    "  FOR curRow IN cur(low_pax_id, high_pax_id) LOOP "
    "    IF normalize_pax_doc(curRow.document, vtype, vissue_country, vno) THEN "
    "      DELETE FROM pax_doc WHERE pax_id=curRow.pax_id; "
    "      OPEN cur1(curRow.pax_id, vno); "
    "      FETCH cur1 INTO row1; "
    "      IF cur1%FOUND THEN "
    "        INSERT INTO pax_doc "
    "          (pax_id,type,issue_country,no,nationality, "
    "           birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi) "
    "        VALUES "
    "          (curRow.pax_id,row1.type,row1.issue_country,row1.no,row1.nationality, "
    "           row1.birth_date,row1.gender,row1.expiry_date,row1.surname,row1.first_name,row1.second_name,row1.pr_multi); "
    "      ELSE "
    "        INSERT INTO pax_doc(pax_id, type, issue_country, no, pr_multi) "
    "        VALUES(curRow.pax_id, vtype, vissue_country, vno, 0); "
    "      END IF; "
    "      CLOSE cur1; "
    "      UPDATE pax SET tid=vtid WHERE pax_id=curRow.pax_id; "
    "      i:=i+2; "
    "    ELSE "
    "      INSERT INTO drop_alter_doc_errors(part_key, pax_id, document) "
    "      VALUES(NULL, curRow.pax_id, curRow.document ); "
    "      i:=i+1; "
    "    END IF; "
    "    IF i>=10000 THEN "
    "      COMMIT; "
    "      i:=0; "
    "    END IF; "
    "    processed:=processed+1; "
    "  END LOOP; "
    "  IF i>0 THEN "
    "    COMMIT; "
    "  END IF; "
    "END;";
  Qry.Execute();
  
  Qry.SQLText=
    "BEGIN "
    "  alter_pax_doc(:low_pax_id, :high_pax_id, :tid, :processed); "
    "END; ";
  Qry.DeclareVariable("low_pax_id", otInteger);
  Qry.DeclareVariable("high_pax_id", otInteger);
  Qry.CreateVariable("tid", otInteger, tid);
  Qry.CreateVariable("processed", otInteger, (int)0);
  

  for(int curr_pax_id=min_pax_id; curr_pax_id<=max_pax_id; curr_pax_id+=10000)
  {
    voland_wait(Qry.GetVariableAsInteger("processed"));
    Qry.SetVariable("low_pax_id",curr_pax_id);
    Qry.SetVariable("high_pax_id",curr_pax_id+10000);
    Qry.Execute();
  };
  
  Qry.Clear();
  Qry.SQLText="DROP PROCEDURE alter_pax_doc";
  Qry.Execute();

  return 0;
};
  
int alter_pax_doc2(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT tid__seq.nextval AS tid FROM dual";
  Qry.Execute();
  int tid=Qry.FieldAsInteger("tid");
  
  TypeB::TTlgParser tlg;
  TypeB::TDocItem doc;
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  DELETE FROM pax_doc WHERE pax_id=:pax_id; "
    "  INSERT INTO pax_doc "
    "    (pax_id,type,issue_country,no,nationality, "
    "     birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi) "
    "  VALUES "
    "    (:pax_id,:type,:issue_country,:no,:nationality, "
    "     :birth_date,:gender,:expiry_date,:surname,:first_name,:second_name,:pr_multi); "
    "  UPDATE pax SET tid=:tid WHERE pax_id=:pax_id; "
    "  UPDATE drop_alter_doc_errors SET processed=1 WHERE pax_id=:pax_id AND part_key IS NULL; "
    "END;";
  Qry.DeclareVariable("pax_id",otInteger);
  Qry.DeclareVariable("type",otString);
  Qry.DeclareVariable("issue_country",otString);
  Qry.DeclareVariable("no",otString);
  Qry.DeclareVariable("nationality",otString);
  Qry.DeclareVariable("birth_date",otDate);
  Qry.DeclareVariable("gender",otString);
  Qry.DeclareVariable("expiry_date",otDate);
  Qry.DeclareVariable("surname",otString);
  Qry.DeclareVariable("first_name",otString);
  Qry.DeclareVariable("second_name",otString);
  Qry.DeclareVariable("pr_multi",otInteger);
  Qry.CreateVariable("tid", otInteger, tid);
  
  TQuery DocQry(&OraSession);
  DocQry.Clear();
  DocQry.SQLText=
    "SELECT pax.pax_id, pax.drop_document AS document FROM drop_alter_doc_errors, pax "
    "WHERE pax.pax_id=drop_alter_doc_errors.pax_id AND "
    "      (drop_alter_doc_errors.document like '_/__%' OR drop_alter_doc_errors.document like 'DOCS/_/__%') AND "
    "      drop_alter_doc_errors.part_key IS NULL "
    "FOR UPDATE";
  DocQry.Execute();

  for(;!DocQry.Eof;DocQry.Next())
  {
    string rem_text=DocQry.FieldAsString("document");
    if (rem_text.substr(0,5)=="DOCS/") rem_text.erase(0,5);
    rem_text="DOCS HK1/"+rem_text;
    
    if (!ParseDOCSRem(tlg,rem_text,doc)) continue;
    if (doc.Empty()) continue;
    if (*doc.no==0) continue;
      
    Qry.SetVariable("pax_id",DocQry.FieldAsInteger("pax_id"));
    Qry.SetVariable("type",doc.type);
    Qry.SetVariable("issue_country",doc.issue_country);
    Qry.SetVariable("no",doc.no);
    Qry.SetVariable("nationality",doc.nationality);
    if (doc.birth_date!=NoExists)
      Qry.SetVariable("birth_date",doc.birth_date);
    else
      Qry.SetVariable("birth_date",FNull);
    Qry.SetVariable("gender",doc.gender);
    if (doc.expiry_date!=NoExists)
      Qry.SetVariable("expiry_date",doc.expiry_date);
    else
      Qry.SetVariable("expiry_date",FNull);
    if (doc.surname.size()>64) doc.surname.erase(64);
    Qry.SetVariable("surname",doc.surname);
    if (doc.first_name.size()>64) doc.first_name.erase(64);
    Qry.SetVariable("first_name",doc.first_name);
    if (doc.second_name.size()>64) doc.second_name.erase(64);
    Qry.SetVariable("second_name",doc.second_name);
    Qry.SetVariable("pr_multi",(int)doc.pr_multi);
    Qry.Execute();
  };
  OraSession.Commit();
  
  return 0;
};

int alter_arx_pax_doc(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT MIN(part_key) AS min_part_key FROM arx_pax";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_part_key")) return 0;
  TDateTime min_part_key=Qry.FieldAsDateTime("min_part_key");

  Qry.SQLText="SELECT MAX(part_key) AS max_part_key FROM arx_pax";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_part_key")) return 0;
  TDateTime max_part_key=Qry.FieldAsDateTime("max_part_key");
  
  
  //Qry.SQLText="CREATE TABLE arx_doc_stat(no VARCHAR2(50) NOT NULL, num NUMBER NOT NULL)";
  //Qry.Execute();
  
  //Qry.SQLText="CREATE UNIQUE INDEX arx_doc_stat__IDX ON arx_doc_stat(no)";
  //Qry.Execute();
  /*
  Qry.Clear();
  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE get_arx_doc_stat(low_part_key IN DATE, high_part_key IN DATE) "
    "IS "
    "  CURSOR cur(vlow_part_key IN DATE, vhigh_part_key IN DATE) IS "
    "    SELECT TRANSLATE(TRIM(document),'0123456789', '**********') AS no "
    "    FROM arx_pax WHERE part_key>=vlow_part_key AND part_key<vhigh_part_key AND document IS NOT NULL; "
    "BEGIN "
    "  FOR curRow IN cur(low_part_key, high_part_key) LOOP "
    "    IF curRow.no IS NOT NULL THEN "
    "      UPDATE arx_doc_stat SET num=num+1 WHERE no=curRow.no; "
    "      IF SQL%ROWCOUNT=0 THEN "
    "        INSERT INTO arx_doc_stat(no, num) VALUES(curRow.no, 1); "
    "      END IF; "
    "    END IF; "
    "  END LOOP; "
    "  COMMIT; "
    "END; ";
  Qry.Execute();*/
  
  /*
  CREATE TABLE drop_alter_doc_errors
  (
    part_key DATE,
    pax_id   NUMBER(9) NOT NULL,
    document VARCHAR2(50) NOT NULL,
    processed NUMBER(1)
  );
  CREATE INDEX drop_alter_doc_errors__IDX on drop_alter_doc_errors(pax_id);
  */

  Qry.SQLText=
    "CREATE OR REPLACE PROCEDURE alter_arx_pax_doc(low_part_key IN DATE, high_part_key IN DATE) "
    "IS "
    "  CURSOR cur(vlow_part_key IN DATE, vhigh_part_key IN DATE) IS "
    "    SELECT part_key,pax_id,drop_document AS document "
    "    FROM arx_pax WHERE part_key>=vlow_part_key AND part_key<vhigh_part_key AND drop_document IS NOT NULL; "
    "  vtype              pax_doc.type%TYPE; "
    "  vissue_country     pax_doc.issue_country%TYPE; "
    "  vno                pax_doc.no%TYPE; "
    "  i                  BINARY_INTEGER; "
    "BEGIN "
    "  i:=0; "
    "  FOR curRow IN cur(low_part_key, high_part_key) LOOP "
    "    IF normalize_pax_doc(curRow.document, vtype, vissue_country, vno) THEN  "
    "      INSERT INTO arx_pax_doc(pax_id, type, issue_country, no, pr_multi, part_key) "
    "      VALUES(curRow.pax_id, vtype, vissue_country, vno, 0, curRow.part_key); "
    "    ELSE "
    "      INSERT INTO drop_alter_doc_errors(part_key, pax_id, document) "
    "      VALUES(curRow.part_key, curRow.pax_id, curRow.document ); "
    "    END IF; "
    "    i:=i+1; "
    "    IF i>=10000 THEN "
    "      COMMIT; "
    "      i:=0; "
    "    END IF; "
    "  END LOOP; "
    "  COMMIT; "
    "END;";
  Qry.Execute();
    
  Qry.SQLText=
    "BEGIN "
    "  alter_arx_pax_doc(:low_part_key, :high_part_key); "
    "END; ";
  Qry.DeclareVariable("low_part_key", otDate);
  Qry.DeclareVariable("high_part_key", otDate);
  
  int processed=0;

  for(TDateTime curr_part_key=min_part_key; curr_part_key<=max_part_key; curr_part_key+=1.0, processed++)
  {
    voland_wait(processed);
    Qry.SetVariable("low_part_key",curr_part_key);
    Qry.SetVariable("high_part_key",curr_part_key+1.0);
    Qry.Execute();
  };

  Qry.Clear();
  Qry.SQLText="DROP PROCEDURE alter_arx_pax_doc";
  Qry.Execute();
  
  //Qry.SQLText="DROP TABLE arx_doc_stat";
  //Qry.Execute();

  return 0;
};

int alter_arx_pax_doc2(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  TypeB::TTlgParser tlg;
  TypeB::TDocItem doc;
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  DELETE FROM arx_pax_doc WHERE part_key=:part_key AND pax_id=:pax_id; "
    "  INSERT INTO arx_pax_doc "
    "    (pax_id,type,issue_country,no,nationality, "
    "     birth_date,gender,expiry_date,surname,first_name,second_name,pr_multi,part_key) "
    "  VALUES "
    "    (:pax_id,:type,:issue_country,:no,:nationality, "
    "     :birth_date,:gender,:expiry_date,:surname,:first_name,:second_name,:pr_multi,:part_key); "
    "  UPDATE drop_alter_doc_errors SET processed=1 WHERE pax_id=:pax_id AND part_key=:part_key; "
    "END;";
  Qry.DeclareVariable("pax_id",otInteger);
  Qry.DeclareVariable("type",otString);
  Qry.DeclareVariable("issue_country",otString);
  Qry.DeclareVariable("no",otString);
  Qry.DeclareVariable("nationality",otString);
  Qry.DeclareVariable("birth_date",otDate);
  Qry.DeclareVariable("gender",otString);
  Qry.DeclareVariable("expiry_date",otDate);
  Qry.DeclareVariable("surname",otString);
  Qry.DeclareVariable("first_name",otString);
  Qry.DeclareVariable("second_name",otString);
  Qry.DeclareVariable("pr_multi",otInteger);
  Qry.DeclareVariable("part_key",otDate);

  TQuery DocQry(&OraSession);
  DocQry.Clear();
  DocQry.SQLText=
    "SELECT arx_pax.part_key, arx_pax.pax_id, arx_pax.drop_document AS document "
    "FROM drop_alter_doc_errors, arx_pax "
    "WHERE arx_pax.part_key=drop_alter_doc_errors.part_key AND "
    "      arx_pax.pax_id=drop_alter_doc_errors.pax_id AND "
    "      (drop_alter_doc_errors.document like '_/__%' OR drop_alter_doc_errors.document like 'DOCS/_/__%') AND "
    "      drop_alter_doc_errors.part_key IS NOT NULL ";
  DocQry.Execute();

  int processed=0;

  for(;!DocQry.Eof;DocQry.Next())
  {
    voland_wait(processed);
  
    string rem_text=DocQry.FieldAsString("document");
    if (rem_text.substr(0,5)=="DOCS/") rem_text.erase(0,5);
    rem_text="DOCS HK1/"+rem_text;

    if (!TypeB::ParseDOCSRem(tlg,rem_text,doc)) continue;
    if (doc.Empty()) continue;
    if (*doc.no==0) continue;

    Qry.SetVariable("pax_id",DocQry.FieldAsInteger("pax_id"));
    Qry.SetVariable("part_key",DocQry.FieldAsDateTime("part_key"));
    Qry.SetVariable("type",doc.type);
    Qry.SetVariable("issue_country",doc.issue_country);
    Qry.SetVariable("no",doc.no);
    Qry.SetVariable("nationality",doc.nationality);
    if (doc.birth_date!=NoExists)
      Qry.SetVariable("birth_date",doc.birth_date);
    else
      Qry.SetVariable("birth_date",FNull);
    Qry.SetVariable("gender",doc.gender);
    if (doc.expiry_date!=NoExists)
      Qry.SetVariable("expiry_date",doc.expiry_date);
    else
      Qry.SetVariable("expiry_date",FNull);
    if (doc.surname.size()>64) doc.surname.erase(64);
    Qry.SetVariable("surname",doc.surname);
    if (doc.first_name.size()>64) doc.first_name.erase(64);
    Qry.SetVariable("first_name",doc.first_name);
    if (doc.second_name.size()>64) doc.second_name.erase(64);
    Qry.SetVariable("second_name",doc.second_name);
    Qry.SetVariable("pr_multi",(int)doc.pr_multi);
    Qry.Execute();
    processed++;
  };
  OraSession.Commit();

  return 0;
};

