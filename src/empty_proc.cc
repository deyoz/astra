//---------------------------------------------------------------------------
#include "basic.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "arx_daily.h"
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




