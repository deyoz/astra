#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <math.h> 
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
#include "daemon.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"

using namespace BASIC;
using namespace EXCEPTIONS;

#define WAIT_INTERVAL           60      //seconds
#define TLG_SCAN_INTERVAL      600   	//seconds
#define SCAN_COUNT             100      //кол-во разбираемых телеграмм за одно сканирование

static void handle_tlg(void);

int main_typeb_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{  
  try
  {
    OpenLogFile("logairimp");	
          
    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();

    time_t scan_time=0;
    char buf[2];
    for(;;)
    {      	
      if (time(NULL)-scan_time>=TLG_SCAN_INTERVAL)
      {
        handle_tlg();
        scan_time=time(NULL);
      };
      if (waitCmd("CMD_TYPEB_HANDLER",WAIT_INTERVAL,buf,sizeof(buf)))
      {
        handle_tlg();
        scan_time=time(NULL);
      };  
    }; // end of loop
  }
  catch(EOracleError E)
  {
    ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());    
  }
  catch(Exception E)
  {
    ProgError(STDLOG,"Exception: %s",E.what());    
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };
  try
  {
    OraSession.Rollback();
    OraSession.LogOff();
  }
  catch(...) 
  {
    ProgError(STDLOG, "Unknown exception");	
  };
  return 0;
};

void handle_tlg(void)
{
  static TQuery TlgIdQry(&OraSession);
  if (TlgIdQry.SQLText.IsEmpty())
  {
    TlgIdQry.Clear();
    TlgIdQry.SQLText=
      "SELECT id,\
              MAX(time_receive) AS time_receive\
       FROM tlgs_in WHERE time_parse IS NULL\
       GROUP BY id ORDER BY MAX(time_create),MAX(merge_key)";
  };

  static TQuery TlgInQry(&OraSession);
  if (TlgInQry.SQLText.IsEmpty())
  {
    TlgInQry.Clear();
    TlgInQry.SQLText=
      "SELECT id,num,heading,ending,body FROM tlgs_in\
       WHERE id=:id AND time_parse IS NULL\
       ORDER BY num DESC FOR UPDATE";
    TlgInQry.DeclareVariable("id",otInteger);
  };

  TQuery TlgInUpdQry(&OraSession);
  if (TlgInUpdQry.SQLText.IsEmpty())
  {
    TlgInUpdQry.Clear();
    TlgInUpdQry.SQLText=
      "UPDATE tlgs_in SET time_parse=SYSDATE, point_id=:point_id\
       WHERE id=:id AND time_parse IS NULL";
    TlgInUpdQry.DeclareVariable("id",otInteger);
    TlgInUpdQry.DeclareVariable("point_id",otInteger);
  };

  TQuery TripsQry(&OraSession);

  TQuery CodeShareQry(&OraSession);
  CodeShareQry.SQLText=
    "SELECT airline,flt_no FROM crs_code_share\
     WHERE airline_crs=:airline AND\
           (flt_no_crs=:flt_no OR flt_no_crs IS NULL AND :flt_no IS NULL)\
     ORDER BY flt_no_crs,airline,flt_no";
  CodeShareQry.DeclareVariable("airline",otString);
  CodeShareQry.DeclareVariable("flt_no",otInteger);

  int tlg_id,tlg_num,count;
  char *buf=NULL,*ph/*,trip[20]*/;
  int bufLen=0,tlgLen;
  bool forcibly;
  TTlgPartInfo part;
  THeadingInfo HeadingInfo;
  TEndingInfo EndingInfo;
  TPnlAdlContent con;
  BASIC::TDateTime local_date=BASIC::Now(false);
  BASIC::TDateTime gmt_date=BASIC::Now(true);
  BASIC::TDateTime trunc_local_date,trunc_gmt_date;
  modf(local_date,&trunc_local_date);
  modf(gmt_date,&trunc_gmt_date);

  count=0;
  TlgIdQry.Execute();
  try
  {
    for(;!TlgIdQry.Eof&&count<SCAN_COUNT;TlgIdQry.Next(),OraSession.Rollback())
    {
      tlg_id=TlgIdQry.FieldAsInteger("id");
      TlgInUpdQry.SetVariable("id",tlg_id);
      TlgInUpdQry.SetVariable("point_id",FNull);
      TlgInQry.SetVariable("id",tlg_id);
      TlgInQry.Execute();
      if (TlgInQry.RowCount()==0) continue;
      tlg_num=TlgInQry.FieldAsInteger("num");
      try
      {
        part.p=TlgInQry.FieldAsString("heading");
        part.line=1;
        ParseHeading(part,HeadingInfo);
        part.p=TlgInQry.FieldAsString("ending");
        part.line=1;
        strcpy(EndingInfo.tlg_type,HeadingInfo.tlg_type);
        EndingInfo.part_no=HeadingInfo.part_no;
        ParseEnding(part,EndingInfo);
      }
      catch(EXCEPTIONS::Exception E)
      {
        ProgError(STDLOG,"Telegram (tlgs_in.id: %d, tlgs_in.num: %d): %s",tlg_id,tlg_num,E.what());
        sendErrorTlg(ERR_CANON_NAME(),OWN_CANON_NAME(),"Telegram (tlgs_in.id: %d, tlgs_in.num: %d): %s",tlg_id,tlg_num,E.what());
        TlgInUpdQry.Execute();
        OraSession.Commit();
        count++;
        continue;
      };

      try
      {
        switch (GetTlgCategory(HeadingInfo.tlg_type))
        {
          case tcDCS:
            //проверка только для DCS-телеграмм!
            if (!EndingInfo.pr_final_part)
            {
              //не все еще части собраны
              if (local_date-TlgIdQry.FieldAsDateTime("time_receive")>30.0/1440) //30 минут
                throw ETlgError("Some parts not received");
              else
                continue;
            };
            //привязка к рейсу для DCS-телеграмм
            TripsQry.Clear();
            TripsQry.SQLText=
              "SELECT trip_id AS point_id FROM trips,options\
               WHERE company=:airline AND flt_no=:flt_no AND\
                     TRUNC(scd)= :scd AND status=0 AND options.cod=:airp\
               ORDER BY NVL(suffix,' ')";
            TripsQry.CreateVariable("airline",otString,HeadingInfo.flt.airline);
            TripsQry.CreateVariable("flt_no",otInteger,(int)HeadingInfo.flt.flt_no);
            TripsQry.CreateVariable("scd",otDate,HeadingInfo.flt.scd);
            TripsQry.CreateVariable("airp",otString,HeadingInfo.flt.brd_point);
            TripsQry.Execute();
            if (TripsQry.RowCount()==0)
            {
              CodeShareQry.SetVariable("airline",HeadingInfo.flt.airline);
              for(int i=0;i<2;i++)
              {
                if (i==0)
                  //сначала проверим по а/к и номеру рейса
                  CodeShareQry.SetVariable("flt_no",(int)HeadingInfo.flt.flt_no);
                else
                  //потом проверим только по а/к
                  CodeShareQry.SetVariable("flt_no",FNull);
                CodeShareQry.Execute();
                if (CodeShareQry.Eof) continue;
                for(;!CodeShareQry.Eof;CodeShareQry.Next())
                {
                  TripsQry.SetVariable("airline",CodeShareQry.FieldAsString("airline"));
                  if (!CodeShareQry.FieldIsNULL("flt_no"))
                    TripsQry.SetVariable("flt_no",CodeShareQry.FieldAsInteger("flt_no"));
                  else
                    TripsQry.SetVariable("flt_no",(int)HeadingInfo.flt.flt_no);
                  TripsQry.Execute();
                  if (TripsQry.RowCount()!=0)
                  {
                    strcpy(HeadingInfo.flt.airline,CodeShareQry.FieldAsString("airline"));
                    if (!CodeShareQry.FieldIsNULL("flt_no"))
                      HeadingInfo.flt.flt_no=CodeShareQry.FieldAsInteger("flt_no");
                    break;
                  };
                };
                break;
              };
            };
            if (TripsQry.RowCount()==0)
            {
              //рейс не найден
              if (HeadingInfo.flt.scd<trunc_local_date) //вчерашний рейс так и не появился в расписании
              {
                /*DateTimeToStr(HeadingInfo.flt.scd,"ddmmm",trip);
                throw ETlgError("Unknown flight %s%ld%s/%s",HeadingInfo.flt.airline,
                                                  HeadingInfo.flt.flt_no,
                                                  HeadingInfo.flt.suffix,
                                                  trip);*/
                TlgInUpdQry.Execute();
                OraSession.Commit();
                count++;
                continue;
              }
              else
                continue;
            };
            break;
          case tcUnknown:
            TlgInUpdQry.Execute();
            OraSession.Commit();
            count++;
            continue;
          default:;
        };

        //собираем тело телеграммы из нескольких частей
        tlgLen=0;
        bool pr_out_mem=false;
        for(;!TlgInQry.Eof;TlgInQry.Next(),tlg_num--)
        {
          if (tlg_num!=TlgInQry.FieldAsInteger("num")) break; //не все части собраны

          tlgLen+=TlgInQry.GetSizeLongField("body");
          if (tlgLen+1>bufLen)
          {
            if (bufLen==0)
              ph=(char*)malloc(tlgLen+1);
            else
              ph=(char*)realloc(buf,tlgLen+1);
            if (ph==NULL)
            {
              pr_out_mem=true;
              break;
            };
            buf=(char*)ph;
            bufLen=tlgLen+1;
          };
          memmove(buf+TlgInQry.GetSizeLongField("body"),buf,tlgLen-TlgInQry.GetSizeLongField("body"));
          TlgInQry.FieldAsLong("body",buf);

        };
        if (tlg_num<0) throw ETlgError("Strange part found");
        if (pr_out_mem)
        {
          // нехватка памяти
          if (local_date-TlgIdQry.FieldAsDateTime("time_receive")>5.0/1440) //5 минут
            throw ETlgError("Out of memory");
          else
            continue;
        };
        if (!TlgInQry.Eof||tlg_num>0)
        {
          //не все еще части собраны
          if (local_date-TlgIdQry.FieldAsDateTime("time_receive")>30.0/1440) //30 минут
            throw ETlgError("Some parts not received");
          else
            continue;
          // не все части собраны
        };
        if (tlgLen==0) throw ETlgError("Empty");
        *(buf+tlgLen)=0;

        switch (GetTlgCategory(HeadingInfo.tlg_type))
        {
          case tcDCS:
            //разобрать телеграмму
            part.p=buf;
            part.line=1;
            ParsePnlAdlBody(part,HeadingInfo,con);
            //принудительно разобрать после 5 минут после получения
            //(это будет работать только для ADL)
            forcibly=local_date-TlgIdQry.FieldAsDateTime("time_receive")>5.0/1440; //5 минут
            if (SavePnlAdlContent(TripsQry.FieldAsInteger("point_id"),HeadingInfo,con,forcibly,
                                  (char*)OWN_CANON_NAME(),(char*)ERR_CANON_NAME()))
            {
              TlgInUpdQry.SetVariable("point_id",TripsQry.FieldAsInteger("point_id"));
              TlgInUpdQry.Execute();
              OraSession.Commit();
              count++;
            }
            else
            {
              OraSession.Rollback();
              if (forcibly&&HeadingInfo.flt.scd<=local_date-10)
                //если телеграммы не хотят принудительно разбираться
                //по истечении 10 дней со дня выполнения рейса - записать в просроченные
                throw ETlgError("Time limit reached");
            };
            break;
          case tcAHM:
            part.p=buf;
            part.line=1;
            PasreAHMFltInfo(part,HeadingInfo);
            //привязка к рейсу
            TripsQry.Clear();
            //знаем время вылета только из нашего пункта, поэтому отбираем рейсы,
            //у которых начальный пункт - наш аэропорт
            TripsQry.SQLText=
              "SELECT trips.trip_id AS point_id\
               FROM trips,trips_in\
               WHERE trips.trip_id=trips_in.trip_id(+) AND\
                     (trips_in.trip IS NULL OR trips.trip<>trips_in.trip OR trips_in.status<0) AND\
                     trips.company=:airline AND trips.flt_no=:flt_no AND\
                     (trips.suffix=:suffix OR trips.suffix IS NULL AND :suffix IS NULL) AND\
                     TRUNC(system.ToUTC(trips.scd))=:scd AND trips.status=0";
            TripsQry.DeclareVariable("airline",otString);
            TripsQry.DeclareVariable("flt_no",otInteger);
            TripsQry.DeclareVariable("suffix",otString);
            TripsQry.DeclareVariable("scd",otDate);
            TripsQry.SetVariable("airline",HeadingInfo.flt.airline);
            TripsQry.SetVariable("flt_no",(int)HeadingInfo.flt.flt_no);
            TripsQry.SetVariable("suffix",HeadingInfo.flt.suffix);
            TripsQry.SetVariable("scd",HeadingInfo.flt.scd); // для AHM HeadingInfo.flt.scd - UTC
            TripsQry.Execute();
            if (TripsQry.RowCount()==0)
            {
              //рейс не найден
              if (HeadingInfo.flt.scd<trunc_gmt_date) //вчерашний рейс так и не появился в расписании
              {
                TlgInUpdQry.Execute();
                OraSession.Commit();
                count++;
              };
              continue;
            };
            TlgInUpdQry.SetVariable("point_id",TripsQry.FieldAsInteger("point_id"));
            TlgInUpdQry.Execute();
            OraSession.Commit();
            count++;
            break;
          default:;
        };
      }
      catch(EXCEPTIONS::Exception E)
      {
      	OraSession.Rollback();
        ProgError(STDLOG,"Telegram (tlgs_in.id: %d): %s",tlg_id,E.what());
        sendErrorTlg(ERR_CANON_NAME(),OWN_CANON_NAME(),"Telegram (tlgs_in.id: %d): %s",tlg_id,E.what());
        TlgInUpdQry.Execute();
        OraSession.Commit();
        count++;
      };
    };
    if (buf!=NULL) free(buf);
  }
  catch(...)
  {
    if (buf!=NULL) free(buf);
    throw;
  };
};

