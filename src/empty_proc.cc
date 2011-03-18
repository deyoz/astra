//---------------------------------------------------------------------------
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
    "      pax_grp.grp_id>=:min_grp_id AND pax_grp.grp_id<:min_grp_id+10000 ";
    //"    AND pax_grp.point_id_mark IS NULL ";
          
  Qry.DeclareVariable("min_grp_id",otInteger);
  
  TQuery UpdQry(&OraSession);
  UpdQry.SQLText=
          "BEGIN "
          "  BEGIN "
          "    SELECT point_id INTO :point_id_mark FROM mark_trips "
          "    WHERE scd=:scd_mark AND airline=:airline_mark AND flt_no=:flt_no_mark AND airp_dep=:airp_dep_mark AND "
          "          (suffix IS NULL AND :suffix_mark IS NULL OR suffix=:suffix_mark) FOR UPDATE; "
          "  EXCEPTION "
          "    WHEN NO_DATA_FOUND THEN "
          "      SELECT id__seq.nextval INTO :point_id_mark FROM dual; "
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
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
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
      };
      UpdQry.Execute();
    };
    OraSession.Commit();
    min_grp_id+=10000;
  };

  puts("Database altered successfully");
  return 0;
};




