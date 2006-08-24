#ifndef __WIN32__
 #include <unistd.h>
 #include <errno.h>
 #include <tcl.h>
 #include <math.h> 
#endif
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
#define TLG_SCAN_INTERVAL	30   	//seconds
#define SCAN_COUNT              10      //���-�� ࠧ��ࠥ��� ⥫��ࠬ� �� ���� ᪠��஢����

static const char* OWN_CANON_NAME=NULL;
static const char* ERR_CANON_NAME=NULL;

static void handle_tlg(void);

#ifdef __WIN32__
int main(int argc, char* argv[])
#else
int main_typeb_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
#endif
{
  try
  {
    try
    {
      OpenLogFile("logairimp");	
      if ((OWN_CANON_NAME=Tcl_GetVar(interp,"OWN_CANON_NAME",TCL_GLOBAL_ONLY))==NULL||
          strlen(OWN_CANON_NAME)!=5)
        throw Exception("Unknown or wrong OWN_CANON_NAME");

      ERR_CANON_NAME=Tcl_GetVar(interp,"ERR_CANON_NAME",TCL_GLOBAL_ONLY);
      
      ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
              ->connect_db();

      time_t scan_time=0;
      for(;;)
      {
      	sleep(WAIT_INTERVAL);
        if (time(NULL)-scan_time>=TLG_SCAN_INTERVAL)
        {
          handle_tlg();
          scan_time=time(NULL);
        };
      }; // end of loop
    }
    catch(EOracleError E)
    {
#ifndef __WIN32__
      ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
#endif
      throw;
    }
    catch(Exception E)
    {
#ifndef __WIN32__
      ProgError(STDLOG,"Exception: %s",E.what());
#endif
      throw;
    };
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
#ifndef __WIN32__
        ProgError(STDLOG,"Telegram (tlgs_in.id: %d, tlgs_in.num: %d): %s",tlg_id,tlg_num,E.what());
        SendTlg(ERR_CANON_NAME,OWN_CANON_NAME,"Telegram (tlgs_in.id: %d, tlgs_in.num: %d): %s",tlg_id,tlg_num,E.what());
#else
        char bufh[50];
        sprintf(bufh,"Telegram (id: %d)",tlg_id);
        Form1->PrintMsg(bufh);
        Form1->PrintMsg(E.what());
        Form1->PrintMsg("");
#endif
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
            //�஢�ઠ ⮫쪮 ��� DCS-⥫��ࠬ�!
            if (!EndingInfo.pr_final_part)
            {
              //�� �� �� ��� ᮡ࠭�
              if (local_date-TlgIdQry.FieldAsDateTime("time_receive")>30.0/1440) //30 �����
                throw ETlgError("Some parts not received");
              else
                continue;
            };
            //�ਢ離� � ३�� ��� DCS-⥫��ࠬ�
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
                  //᭠砫� �஢�ਬ �� �/� � ������ ३�
                  CodeShareQry.SetVariable("flt_no",(int)HeadingInfo.flt.flt_no);
                else
                  //��⮬ �஢�ਬ ⮫쪮 �� �/�
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
              //३� �� ������
              if (HeadingInfo.flt.scd<trunc_local_date) //���譨� ३� ⠪ � �� ����� � �ᯨᠭ��
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
          if (local_date-TlgIdQry.FieldAsDateTime("time_receive")>5.0/1440) //5 �����
            throw ETlgError("Out of memory");
          else
            continue;
        };
        if (!TlgInQry.Eof||tlg_num>0)
        {
          //�� �� �� ��� ᮡ࠭�
          if (local_date-TlgIdQry.FieldAsDateTime("time_receive")>30.0/1440) //30 �����
            throw ETlgError("Some parts not received");
          else
            continue;
          // �� �� ��� ᮡ࠭�
        };
        if (tlgLen==0) throw ETlgError("Empty");
        *(buf+tlgLen)=0;

        switch (GetTlgCategory(HeadingInfo.tlg_type))
        {
          case tcDCS:
            //ࠧ����� ⥫��ࠬ��
            part.p=buf;
            part.line=1;
            ParsePnlAdlBody(part,HeadingInfo,con);
            //�ਭ㤨⥫쭮 ࠧ����� ��᫥ 5 ����� ��᫥ ����祭��
            //(�� �㤥� ࠡ���� ⮫쪮 ��� ADL)
            forcibly=local_date-TlgIdQry.FieldAsDateTime("time_receive")>5.0/1440; //5 �����
            if (SavePnlAdlContent(TripsQry.FieldAsInteger("point_id"),HeadingInfo,con,forcibly,
                                  (char*)OWN_CANON_NAME,(char*)ERR_CANON_NAME))
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
                //�᫨ ⥫��ࠬ�� �� ���� �ਭ㤨⥫쭮 ࠧ�������
                //�� ���祭�� 10 ���� � ��� �믮������ ३� - ������� � ����祭��
                throw ETlgError("Time limit reached");
            };
            break;
          case tcAHM:
            part.p=buf;
            part.line=1;
            PasreAHMFltInfo(part,HeadingInfo);
            //�ਢ離� � ३��
            TripsQry.Clear();
            //����� �६� �뫥� ⮫쪮 �� ��襣� �㭪�, ���⮬� �⡨ࠥ� ३��,
            //� ������ ��砫�� �㭪� - ��� ��ய���
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
            TripsQry.SetVariable("scd",HeadingInfo.flt.scd); // ��� AHM HeadingInfo.flt.scd - UTC
            TripsQry.Execute();
            if (TripsQry.RowCount()==0)
            {
              //३� �� ������
              if (HeadingInfo.flt.scd<trunc_gmt_date) //���譨� ३� ⠪ � �� ����� � �ᯨᠭ��
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
#ifndef __WIN32__
        ProgError(STDLOG,"Telegram (tlgs_in.id: %d): %s",tlg_id,E.what());
        SendTlg(ERR_CANON_NAME,OWN_CANON_NAME,"Telegram (tlgs_in.id: %d): %s",tlg_id,E.what());
#else
        char bufh[50];
        sprintf(bufh,"Telegram (id: %d)",tlg_id);
        Form1->PrintMsg(bufh);
        Form1->PrintMsg(E.what());
        Form1->PrintMsg("");
#endif
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

