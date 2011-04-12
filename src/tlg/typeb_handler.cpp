#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <math.h>
#include "astra_utils.h"
#include "base_tables.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
#include "memory_manager.h"
#include "comp_layers.h"
#include "serverlib/ourtime.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;

#define WAIT_INTERVAL           10      //seconds
#define TLG_SCAN_INTERVAL       60   	  //seconds
#define TLG_BIND_INTERVAL       60   	  //seconds
#define SCAN_COUNT             100      //���-�� ࠧ��ࠥ��� ⥫��ࠬ� �� ���� ᪠��஢����

static void handle_tlg(void);
static void bind_tlg(void);

int main_typeb_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(10);
    InitLogTime(NULL);
    OpenLogFile("logairimp");

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();

    time_t scan_time=0;
    time_t bind_time=0;
    char buf[10];
    for(;;)
    {
      InitLogTime(NULL);
      if (time(NULL)-scan_time>=TLG_SCAN_INTERVAL)
      {
        InitLogTime(NULL);
        base_tables.Invalidate();
        handle_tlg();
        scan_time=time(NULL);
      };
      if (time(NULL)-bind_time>=TLG_BIND_INTERVAL)
      {
        InitLogTime(NULL);
        base_tables.Invalidate();
        bind_tlg();
        bind_time=time(NULL);
      };
      if (waitCmd("CMD_TYPEB_HANDLER",WAIT_INTERVAL,buf,sizeof(buf)))
      {
        InitLogTime(NULL);
        base_tables.Invalidate();
        handle_tlg();
        scan_time=time(NULL);
      };
    }; // end of loop
  }
  catch(EOracleError &E)
  {
    ProgError(STDLOG,"EOracleError %d: %s\nSQL: %s)",E.Code,E.what(),E.SQLText());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"std::exception: %s",E.what());
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

#define PARTS_NOT_RECEIVE_TIMEOUT  1.0      //1 ����
#define OUT_OF_MEMORY_TIMEOUT      5.0/1440 //5 ���
#define PARSING_FORCE_TIMEOUT      5.0/1440 //5 ���
#define PARSING_MAX_TIMEOUT        1.0      //1 ����
#define SCAN_TIMEOUT               2.0      //2 ���

void handle_tlg(void)
{
  time_t time_start=time(NULL);
  
  TMemoryManager mem(STDLOG);

  TDateTime utc_date=NowUTC();

  static TQuery TlgIdQry(&OraSession);
  if (TlgIdQry.SQLText.IsEmpty())
  {
    TlgIdQry.Clear();
    TlgIdQry.SQLText=
      "SELECT id, "
      "       MAX(time_receive) AS time_receive, "
      "       MAX(time_create) AS max_time_create, "
      "       MIN(time_receive) AS min_time_receive "
      "FROM tlgs_in "
      "WHERE time_parse IS NULL AND time_receive>=:time_receive "
      "GROUP BY id "
      "ORDER BY max_time_create,min_time_receive,id";
    TlgIdQry.CreateVariable("time_receive",otDate,utc_date-SCAN_TIMEOUT);
  };

  static TQuery TlgInQry(&OraSession);
  if (TlgInQry.SQLText.IsEmpty())
  {
    TlgInQry.Clear();
    TlgInQry.SQLText=
      "SELECT id,num,heading,ending,body FROM tlgs_in "
      "WHERE id=:id "
      "ORDER BY num DESC FOR UPDATE";
    TlgInQry.DeclareVariable("id",otInteger);
  };

  TQuery TlgInUpdQry(&OraSession);
  if (TlgInUpdQry.SQLText.IsEmpty())
  {
    TlgInUpdQry.Clear();
    TlgInUpdQry.SQLText=
      "UPDATE tlgs_in SET time_parse=system.UTCSYSDATE "
      "WHERE id=:id AND time_parse IS NULL";
    TlgInUpdQry.DeclareVariable("id",otInteger);
  };

  TQuery TripsQry(&OraSession);

  TDateTime time_receive;
  int tlg_id,tlg_num,count;
  char *buf=NULL,*ph/*,trip[20]*/;
  int bufLen=0,tlgLen;
  bool forcibly;
  TTlgPartInfo part;
  THeadingInfo *HeadingInfo=NULL;
  TEndingInfo *EndingInfo=NULL;

  count=0;
  TlgIdQry.Execute();
  try
  {
    for(;!TlgIdQry.Eof&&count<SCAN_COUNT;TlgIdQry.Next(),OraSession.Rollback())
    {
      tlg_id=TlgIdQry.FieldAsInteger("id");
      time_receive=TlgIdQry.FieldAsDateTime("time_receive");

      TlgInUpdQry.SetVariable("id",tlg_id);
      TlgInQry.SetVariable("id",tlg_id);
      //�⠥� �� ��� ⥫��ࠬ��
      TlgInQry.Execute();
      if (TlgInQry.RowCount()==0) continue;
      tlg_num=TlgInQry.FieldAsInteger("num");
      try
      {
        part.p=TlgInQry.FieldAsString("heading");
        part.line=1;
        ParseHeading(part,HeadingInfo,mem);
        part.p=TlgInQry.FieldAsString("ending");
        part.line=1;
        ParseEnding(part,HeadingInfo,EndingInfo,mem);
      }
      catch(EXCEPTIONS::Exception &E)
      {
        count++;
        EOracleError *orae=dynamic_cast<EOracleError*>(&E);
      	if (orae!=NULL&&
      	    (orae->Code==4061||orae->Code==4068)) continue;
      	if (orae!=NULL)
      	  ProgError(STDLOG,"Telegram (tlgs_in.id: %d, tlgs_in.num: %d): %s\nSQL: %s",tlg_id,tlg_num,E.what(),orae->SQLText());
      	else
          ProgError(STDLOG,"Telegram (tlgs_in.id: %d, tlgs_in.num: %d): %s",tlg_id,tlg_num,E.what());
        //sendErrorTlg("Telegram (tlgs_in.id: %d, tlgs_in.num: %d): %s",tlg_id,tlg_num,E.what());
        TlgInUpdQry.Execute();
        OraSession.Commit();
        continue;
      };

      try
      {
        if ((HeadingInfo->tlg_cat==tcDCS||
             HeadingInfo->tlg_cat==tcBSM)&&
             !EndingInfo->pr_final_part)
        {
          //�� �� �� ��� ᮡ࠭�
          if (utc_date-time_receive > PARTS_NOT_RECEIVE_TIMEOUT)
            throw ETlgError("Some parts not received");
          else
            continue;
        };

                //ᮡ�ࠥ� ⥫� ⥫��ࠬ�� �� ��᪮�쪨� ��⥩
        tlgLen=0;
        bool pr_out_mem=false;
        for(;!TlgInQry.Eof;TlgInQry.Next(),tlg_num--)
        {
          if (tlg_num!=TlgInQry.FieldAsInteger("num")) break; //�� �� ��� ᮡ࠭�

          tlgLen+=TlgInQry.GetSizeLongField("body");
          if (tlgLen+1>bufLen)
          {
            if (bufLen==0)
              ph=(char*)mem.malloc(tlgLen+1, STDLOG);
            else
              ph=(char*)mem.realloc(buf,tlgLen+1, STDLOG);
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
          // ��墠⪠ �����
          if (utc_date-time_receive > OUT_OF_MEMORY_TIMEOUT)
            throw ETlgError("Out of memory");
          else
            continue;
        };
        if (!TlgInQry.Eof||tlg_num>0)
        {
          //�� �� �� ��� ᮡ࠭�
          if (utc_date-time_receive > PARTS_NOT_RECEIVE_TIMEOUT)
            throw ETlgError("Some parts not received");
          else
            continue;
          // �� �� ��� ᮡ࠭�
        };
        if (tlgLen==0) throw ETlgError("Empty");
        *(buf+tlgLen)=0;

        switch (HeadingInfo->tlg_cat)
        {
          case tcDCS:
          {
            //ࠧ����� ⥫��ࠬ��
            part.p=buf;
            part.line=1;
            TDCSHeadingInfo &info = *dynamic_cast<TDCSHeadingInfo*>(HeadingInfo);
            if (strcmp(info.tlg_type,"PNL")==0||
                strcmp(info.tlg_type,"ADL")==0)
            {
              TPnlAdlContent con;
              ParsePNLADLContent(part,info,con);
              //�ਭ㤨⥫쭮 ࠧ����� �१ ��।������� �६� ��᫥ ����祭��
              //(�� �㤥� ࠡ���� ⮫쪮 ��� ADL)
              forcibly=utc_date-time_receive > PARSING_FORCE_TIMEOUT;
              if (SavePNLADLContent(tlg_id,info,con,forcibly))
              {
                TlgInUpdQry.Execute();
                OraSession.Commit();
                count++;
              }
              else
              {
                OraSession.Rollback();
                if (forcibly&& /*info.flt.scd<=utc_date-10*/
                	  (utc_date-time_receive) > PARSING_MAX_TIMEOUT)
                  //�᫨ ⥫��ࠬ�� �� ���� �ਭ㤨⥫쭮 ࠧ�������
                  //�� ���祭�� �����ண� �६��� - ������� � ����祭��
                  throw ETlgError("Time limit reached");
              };
            };
            if (strcmp(info.tlg_type,"PTM")==0)
            {
              TPtmContent con;
              ParsePTMContent(part,info,con);
              SavePTMContent(tlg_id,info,con);
              TlgInUpdQry.Execute();
              OraSession.Commit();
              count++;
            };
            if (strcmp(info.tlg_type,"SOM")==0)
            {
              TSOMContent con;
              ParseSOMContent(part,info,con);
              SaveSOMContent(tlg_id,info,con);
              TlgInUpdQry.Execute();
              OraSession.Commit();
              count++;
            };
            break;
          }
          case tcBSM:
          {
            part.p=buf;
            part.line=1;
            TBSMHeadingInfo &info = *dynamic_cast<TBSMHeadingInfo*>(HeadingInfo);
            if (strcmp(info.tlg_type,"BTM")==0)
            {
              TBtmContent con;
              ParseBTMContent(part,info,con,mem);
              SaveBTMContent(tlg_id,info,con);
              TlgInUpdQry.Execute();
              OraSession.Commit();
              count++;
            };
            break;
          }
          case tcAHM:
          {
            part.p=buf;
            part.line=1;
            TFltInfo flt;
            ParseAHMFltInfo(part,flt);
            SaveFlt(tlg_id,flt,btFirstSeg);
            TlgInUpdQry.Execute();
            OraSession.Commit();
            count++;
            break;
          }
          default:
          {
            //⥫��ࠬ�� �������⭮�� ⨯� �ࠧ� ��襬 � ࠧ��࠭��
            TlgInUpdQry.Execute();
            OraSession.Commit();
            count++;
          };
        };
      }
      catch(EXCEPTIONS::Exception &E)
      {
        count++;
      	OraSession.Rollback();
      	try
      	{
        	EOracleError *orae=dynamic_cast<EOracleError*>(&E);
        	if (orae!=NULL&&
        	    (orae->Code==4061||orae->Code==4068)) continue;
        	if (orae!=NULL)
        	  ProgError(STDLOG,"Telegram (tlgs_in.id: %d): %s\nSQL: %s",tlg_id,E.what(),orae->SQLText());
        	else
            ProgError(STDLOG,"Telegram (tlgs_in.id: %d): %s",tlg_id,E.what());
          //sendErrorTlg("Telegram (tlgs_in.id: %d): %s",tlg_id,E.what());
          TlgInUpdQry.Execute();
          OraSession.Commit();
        }
        catch(...) {};
      };
    };
    mem.destroy(HeadingInfo, STDLOG);
    if (HeadingInfo!=NULL) delete HeadingInfo;
    mem.destroy(EndingInfo, STDLOG);
    if (EndingInfo!=NULL) delete EndingInfo;
    if (buf!=NULL) mem.free(buf, STDLOG);
  }
  catch(...)
  {
    mem.destroy(HeadingInfo, STDLOG);
    if (HeadingInfo!=NULL) delete HeadingInfo;
    mem.destroy(EndingInfo, STDLOG);
    if (EndingInfo!=NULL) delete EndingInfo;
    if (buf!=NULL) mem.free(buf, STDLOG);
    throw;
  };

  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! handle_tlg execute time: %ld secs, count=%d",
                     time_end-time_start,count);
  return;
};

void bind_tlg(void)
{
  time_t time_start=time(NULL);
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT point_id,airline,flt_no,suffix,scd,pr_utc,airp_dep,airp_arv,bind_type "
    "FROM tlg_binding,tlg_trips "
    "WHERE tlg_trips.point_id=tlg_binding.point_id_tlg(+) AND "
    "      tlg_binding.point_id_spp IS NULL AND "
    "      scd>=TRUNC(system.UTCSYSDATE)-3 AND "
    "      scd<=TRUNC(system.UTCSYSDATE)+3 ";
  Qry.Execute();

  int count=0;
  for(;!Qry.Eof;Qry.Next(),count++)
  {
    if (bind_tlg(Qry))
    {
      int point_id_tlg=Qry.FieldAsInteger("point_id");
      crs_recount(point_id_tlg,true);
      SyncTripCompLayers(point_id_tlg, ASTRA::NoExists, ASTRA::cltSOMTrzt);
      SyncTripCompLayers(point_id_tlg, ASTRA::NoExists, ASTRA::cltPRLTrzt);
      SyncTripCompLayers(point_id_tlg, ASTRA::NoExists, ASTRA::cltPNLCkin);
      SyncTripCompLayers(point_id_tlg, ASTRA::NoExists, ASTRA::cltProtCkin);
      SyncTripCompLayers(point_id_tlg, ASTRA::NoExists, ASTRA::cltPNLBeforePay);
      SyncTripCompLayers(point_id_tlg, ASTRA::NoExists, ASTRA::cltPNLAfterPay);
      SyncTripCompLayers(point_id_tlg, ASTRA::NoExists, ASTRA::cltProtBeforePay);
      SyncTripCompLayers(point_id_tlg, ASTRA::NoExists, ASTRA::cltProtAfterPay);
    };
  };
  OraSession.Commit();

  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! bind_tlg execute time: %ld secs, count=%d",
                     time_end-time_start,count);
  return;
};



