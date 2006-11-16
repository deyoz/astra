#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <math.h>
#include "astra_utils.h"
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

#define WAIT_INTERVAL           10      //seconds
#define TLG_SCAN_INTERVAL       60   	  //seconds
#define SCAN_COUNT             100      //���-�� ࠧ��ࠥ��� ⥫��ࠬ� �� ���� ᪠��஢����

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
      "UPDATE tlgs_in SET time_parse=system.UTCSYSDATE "
      "WHERE id=:id AND time_parse IS NULL";
    TlgInUpdQry.DeclareVariable("id",otInteger);
  };

  TQuery TripsQry(&OraSession);

  int tlg_id,tlg_num,count;
  char *buf=NULL,*ph/*,trip[20]*/;
  int bufLen=0,tlgLen;
  bool forcibly;
  TTlgPartInfo part;
  THeadingInfo *HeadingInfo=NULL;
  TEndingInfo *EndingInfo=NULL;

  TDateTime utc_date=NowUTC();
  TDateTime trunc_utc_date;
  modf(utc_date,&trunc_utc_date);

  count=0;
  TlgIdQry.Execute();
  try
  {
    for(;!TlgIdQry.Eof&&count<SCAN_COUNT;TlgIdQry.Next(),OraSession.Rollback())
    {
      tlg_id=TlgIdQry.FieldAsInteger("id");
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
        ParseHeading(part,HeadingInfo);
        part.p=TlgInQry.FieldAsString("ending");
        part.line=1;
        ParseEnding(part,HeadingInfo,EndingInfo);
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
        if ((HeadingInfo->tlg_cat==tcDCS||
             HeadingInfo->tlg_cat==tcBSM)&&
             !EndingInfo->pr_final_part)
        {
          //�� �� �� ��� ᮡ࠭�
          if (utc_date-TlgIdQry.FieldAsDateTime("time_receive")>30.0/1440) //30 �����
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
          // ��墠⪠ �����
          if (utc_date-TlgIdQry.FieldAsDateTime("time_receive")>5.0/1440) //5 �����
            throw ETlgError("Out of memory");
          else
            continue;
        };
        if (!TlgInQry.Eof||tlg_num>0)
        {
          //�� �� �� ��� ᮡ࠭�
          if (utc_date-TlgIdQry.FieldAsDateTime("time_receive")>30.0/1440) //30 �����
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
              //�ਭ㤨⥫쭮 ࠧ����� ��᫥ 5 ����� ��᫥ ����祭��
              //(�� �㤥� ࠡ���� ⮫쪮 ��� ADL)
              forcibly=utc_date-TlgIdQry.FieldAsDateTime("time_receive")>5.0/1440; //5 �����
              if (SavePNLADLContent(tlg_id,info,con,forcibly))
              {
                TlgInUpdQry.Execute();
                OraSession.Commit();
                count++;
              }
              else
              {
                OraSession.Rollback();
                if (forcibly&&info.flt.scd<=utc_date-10)
                  //�᫨ ⥫��ࠬ�� �� ���� �ਭ㤨⥫쭮 ࠧ�������
                  //�� ���祭�� 10 ���� � ��� �믮������ ३� - ������� � ����祭��
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
              ParseBTMContent(part,info,con);
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
    if (HeadingInfo!=NULL) delete HeadingInfo;
    if (EndingInfo!=NULL) delete EndingInfo;
    if (buf!=NULL) free(buf);
  }
  catch(...)
  {
    if (HeadingInfo!=NULL) delete HeadingInfo;
    if (EndingInfo!=NULL) delete EndingInfo;
    if (buf!=NULL) free(buf);
    throw;
  };
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT tlg_trips.point_id FROM tlg_binding,tlg_trips "
    "WHERE tlg_trips.point_id=tlg_binding.point_id_tlg(+) AND "
    "      tlg_binding.point_id_spp IS NULL ";
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    bind_tlg(Qry.FieldAsInteger("point_id"));
  };
  OraSession.Commit();
};



