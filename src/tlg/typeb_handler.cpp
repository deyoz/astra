#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <math.h>
#include <iostream>
#include "astra_consts.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
#include "lci_parser.h"
//#include "ssm_parser.h"
#include "memory_manager.h"
#include "qrys.h"
#include "serverlib/ourtime.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;
using namespace TypeB;

static const int HANDLER_WAIT_INTERVAL()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_HANDLER_WAIT_INTERVAL",1,NoExists,60000);
  return VAR;
};

static const int HANDLER_PROC_INTERVAL()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_HANDLER_PROC_INTERVAL",0,NoExists,1000);
  return VAR;
};

static const int HANDLER_PROC_COUNT()          //кол-во обрабатываемых частей телеграмм за одну итерацию
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_HANDLER_PROC_COUNT",1,NoExists,100);
  return VAR;
};

static const int PARSER_WAIT_INTERVAL()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_PARSER_WAIT_INTERVAL",1,NoExists,60000);
  return VAR;
};

static const int PARSER_PROC_INTERVAL()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_PARSER_PROC_INTERVAL",0,NoExists,10000);
  return VAR;
};

static const int PARSER_PROC_COUNT()          //кол-во разбираемых телеграмм за одну итерацию
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_PARSER_PROC_COUNT",1,NoExists,100);
  return VAR;
};

static bool handle_tlg(void);
static bool parse_tlg(void);

int main_typeb_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(10);
    InitLogTime(NULL);
    OpenLogFile("logairimp");

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();

    char buf[10];
    for(;;)
    {
      InitLogTime(NULL);
      base_tables.Invalidate();
      bool queue_not_empty=handle_tlg();
      
      waitCmd("CMD_TYPEB_HANDLER",queue_not_empty?HANDLER_PROC_INTERVAL():HANDLER_WAIT_INTERVAL(),buf,sizeof(buf));
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

int main_typeb_parser_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(15);
    InitLogTime(NULL);
    OpenLogFile("logairimp");

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();

    char buf[10];
    for(;;)
    {
      InitLogTime(NULL);
      base_tables.Invalidate();
      bool queue_not_empty=parse_tlg();

      waitCmd("CMD_TYPEB_PARSER",queue_not_empty?PARSER_PROC_INTERVAL():PARSER_WAIT_INTERVAL(),buf,sizeof(buf));
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

void SaveTypebInHistory(int prev_tlg_id, int tlg_id)
{
    const char* sql = "insert into typeb_in_history(prev_tlg_id, tlg_id) values(:prev_tlg_id, :tlg_id)";
    QParams QryParams;
    QryParams
        << QParam("prev_tlg_id", otInteger, prev_tlg_id)
        << QParam("tlg_id", otInteger, tlg_id);
    TCachedQuery Qry(sql, QryParams);
    Qry.get().Execute();
}

bool handle_tlg(void)
{
  bool queue_not_empty=false;

  time_t time_start=time(NULL);

  TMemoryManager mem(STDLOG);

  static TQuery TlgQry(&OraSession);
  if (TlgQry.SQLText.IsEmpty())
  {
    //внимание порядок объединения таблиц важен!
    TlgQry.Clear();
    TlgQry.SQLText=
      "SELECT tlg_queue.id,tlgs.tlg_text,tlgs.typeb_tlg_id,tlg_queue.time,ttl, "
      "       tlg_queue.tlg_num,tlg_queue.sender, "
      "       NVL(tlg_queue.proc_attempt,0) AS proc_attempt "
      "FROM tlgs,tlg_queue "
      "WHERE tlg_queue.id=tlgs.id AND tlg_queue.receiver=:receiver AND "
      "      tlg_queue.type='INB' AND tlg_queue.status='PUT' "
      "ORDER BY tlg_queue.time,tlg_queue.id";
    TlgQry.CreateVariable("receiver",otString,OWN_CANON_NAME());

  };

  static TQuery TlgIdQry(&OraSession);
  if (TlgIdQry.SQLText.IsEmpty())
  {
    TlgIdQry.Clear();
    TlgIdQry.SQLText=
     "BEGIN "
     "  SELECT id INTO :id FROM tlgs_in "
     "  WHERE type= :tlg_type AND "
     "        time_create BETWEEN :min_time_create AND :max_time_create AND "
     "        merge_key= :merge_key AND rownum=1 FOR UPDATE; "
     "EXCEPTION "
     "  WHEN NO_DATA_FOUND THEN "
     "    SELECT tlg_in_out__seq.nextval INTO :id FROM dual; "
     "END;";
    TlgIdQry.DeclareVariable("id",otInteger);
    TlgIdQry.DeclareVariable("tlg_type",otString);
    TlgIdQry.DeclareVariable("min_time_create",otDate);
    TlgIdQry.DeclareVariable("max_time_create",otDate);
    TlgIdQry.DeclareVariable("merge_key",otString);
  };

  const char* ins_sql=
    "BEGIN "
    "  :ret_id := nvl(:id, tlg_in_out__seq.nextval); "
    "  INSERT INTO tlgs_in(id,num,type,addr,heading,ending, "
    "                     merge_key,time_create,time_receive,time_parse,time_receive_not_parse) "
    "  VALUES(:ret_id, "
    "         :part_no,:tlg_type,:addr,:heading,:ending, "
    "         :merge_key,:time_create,system.UTCSYSDATE,NULL,system.UTCSYSDATE); "
    "  UPDATE tlgs SET typeb_tlg_id=:ret_id, typeb_tlg_num=:part_no WHERE id=:tlgs_id; "
    "END;";
  QParams QryParams;
  QryParams << QParam("id",otInteger)
            << QParam("ret_id",otInteger)
            << QParam("part_no",otInteger)
            << QParam("tlg_type",otString)
            << QParam("addr",otString)
            << QParam("heading",otString)
            << QParam("ending",otString)
            << QParam("merge_key",otString)
            << QParam("time_create",otDate)
            << QParam("tlgs_id",otInteger);

  TCachedQuery InsQry(ins_sql, QryParams);

  TQuery Qry(&OraSession);

  int count,tlg_id;
  size_t len, bufLen=0;
  char *buf=NULL,*ph,c;
  bool pr_typeb_cmd=false;
  TTlgParts parts;
  THeadingInfo *HeadingInfo=NULL;
  TEndingInfo *EndingInfo=NULL;

  count=0;
  TlgQry.Execute();
  try
  {
    for (;!TlgQry.Eof&&count<HANDLER_PROC_COUNT();count++,TlgQry.Next(),OraSession.Commit())
    {
      //проверим TTL
      tlg_id=TlgQry.FieldAsInteger("id");

      if (!TlgQry.FieldIsNULL("ttl")&&
           (NowUTC()-TlgQry.FieldAsDateTime("time"))*BASIC::SecsPerDay>=TlgQry.FieldAsInteger("ttl"))
      {
      	errorTlg(tlg_id,"TTL");
      }
      else
      if (TlgQry.FieldAsInteger("proc_attempt")>=HANDLER_PROC_ATTEMPTS())
      {
        ProgTrace(TRACE5, "handle_tlg: tlg_id=%d proc_attempt=%d", tlg_id, TlgQry.FieldAsInteger("proc_attempt"));
        errorTlg(tlg_id,"PROC");
      }
      else
      try
      {
        procTlg(tlg_id);
        OraSession.Commit();
      
        string tlgs_text=getTlgText(tlg_id, TlgQry);

        len=tlgs_text.size()+1;
        if (len>bufLen)
        {
          if (buf==NULL)
            ph=(char*)mem.malloc(len, STDLOG);
          else
            ph=(char*)mem.realloc(buf,len, STDLOG);
          if (ph==NULL) throw EMemoryError("Out of memory");
          buf=ph;
          bufLen=len;
        };
        memcpy(buf, tlgs_text.c_str(), tlgs_text.size());
        buf[len-1]=0;

        parts=GetParts(buf,mem);
        ParseHeading(parts.heading,HeadingInfo,mem);
        ParseEnding(parts.ending,HeadingInfo,EndingInfo,mem);
        if (parts.heading.p-parts.addr.p>255) throw ETlgError("Address too long");
        if (parts.body.p-parts.heading.p>100) throw ETlgError("Header too long");
        if (parts.ending.p!=NULL&&strlen(parts.ending.p)>20) throw ETlgError("End of message too long");

        if ((HeadingInfo->tlg_cat==tcDCS||
             HeadingInfo->tlg_cat==tcBSM)&&
             HeadingInfo->time_create!=0)
        {
          long part_no;
          ostringstream merge_key;
          merge_key << "." << HeadingInfo->sender;
          if (HeadingInfo->tlg_cat==tcDCS)
          {
            TDCSHeadingInfo &info = *dynamic_cast<TDCSHeadingInfo*>(HeadingInfo);
            part_no=info.part_no;
            merge_key << " " << info.flt.airline << setw(3) << setfill('0') << info.flt.flt_no
                             << info.flt.suffix << "/" << DateTimeToStr(info.flt.scd,"ddmmm") << " "
                             << info.flt.airp_dep << info.flt.airp_arv;
            if (info.association_number>0)
              merge_key << " " << setw(6) << setfill('0') << info.association_number;
          }
          else
          {
            TBSMHeadingInfo &info = *dynamic_cast<TBSMHeadingInfo*>(HeadingInfo);
            part_no=info.part_no;
            merge_key << " " << info.airp;
            if (!info.reference_number.empty())
              merge_key << " " << info.reference_number;
          };

          TlgIdQry.SetVariable("id",FNull);
          TlgIdQry.SetVariable("tlg_type",HeadingInfo->tlg_type);
          TlgIdQry.SetVariable("merge_key",merge_key.str());
          if (strcmp(HeadingInfo->tlg_type,"PNL")==0)
          {
            TlgIdQry.SetVariable("min_time_create",HeadingInfo->time_create-2.0/1440);
            TlgIdQry.SetVariable("max_time_create",HeadingInfo->time_create+2.0/1440);
          }
          else
          {
            TlgIdQry.SetVariable("min_time_create",HeadingInfo->time_create);
            TlgIdQry.SetVariable("max_time_create",HeadingInfo->time_create);
          };
          TlgIdQry.Execute();
          if (TlgIdQry.VariableIsNULL("id"))
            InsQry.get().SetVariable("id",FNull);
          else
            InsQry.get().SetVariable("id",TlgIdQry.GetVariableAsInteger("id"));
          InsQry.get().SetVariable("part_no",(int)part_no);
          InsQry.get().SetVariable("merge_key",merge_key.str());
        }
        else
        {
          InsQry.get().SetVariable("id",FNull);
          InsQry.get().SetVariable("part_no",1);
          InsQry.get().SetVariable("merge_key",FNull);
        };

        InsQry.get().SetVariable("tlg_type",HeadingInfo->tlg_type);
        if (HeadingInfo->time_create!=0)
          InsQry.get().SetVariable("time_create",HeadingInfo->time_create);
        else
          InsQry.get().SetVariable("time_create",FNull);

        c=*parts.heading.p;
        *parts.heading.p=0;
        InsQry.get().SetVariable("addr",parts.addr.p);
        *parts.heading.p=c;

        c=*parts.body.p;
        *parts.body.p=0;
        InsQry.get().SetVariable("heading",parts.heading.p);
        *parts.body.p=c;

        string body;

        if (parts.ending.p!=NULL)
        {
          body.assign(parts.body.p,parts.ending.p-parts.body.p);
          InsQry.get().SetVariable("ending",parts.ending.p);
        }
        else
        {
          body.assign(parts.body.p,strlen(parts.body.p));
          InsQry.get().SetVariable("ending",FNull);
        };
        InsQry.get().SetVariable("tlgs_id", tlg_id);

        if (deleteTlg(tlg_id))
        {
          bool insert_typeb=false;
          try
          {
            InsQry.get().Execute();
            pr_typeb_cmd=true;
            insert_typeb=true;
          }
          catch(EOracleError E)
          {
            if (E.Code==1)
            {
              int typeb_tlg_id=InsQry.get().VariableIsNULL("id")?NoExists:InsQry.get().GetVariableAsInteger("id");
              int typeb_tlg_num=InsQry.get().GetVariableAsInteger("part_no");

              if (typeb_tlg_id==NoExists) throw ETlgError("Duplicate part number");
              Qry.Clear();
              Qry.SQLText=
                "SELECT addr,heading,body,ending FROM tlgs_in WHERE id=:id AND num=:num";
              Qry.CreateVariable("id",otInteger,typeb_tlg_id);
              Qry.CreateVariable("num",otInteger,typeb_tlg_num);
              Qry.Execute();
              if (Qry.Eof) throw ETlgError("Duplicate part number");

              ostringstream typeb_in_text;
              typeb_in_text << Qry.FieldAsString("addr")
                            << Qry.FieldAsString("heading")
                            << getTypeBBody(typeb_tlg_id, typeb_tlg_num, Qry)
                            << Qry.FieldAsString("ending");

              if (tlgs_text!=typeb_in_text.str())
              {
                long part_no;
                if (HeadingInfo->tlg_cat==tcDCS)
                  part_no=dynamic_cast<TDCSHeadingInfo*>(HeadingInfo)->part_no;
                else
                  part_no=dynamic_cast<TBSMHeadingInfo*>(HeadingInfo)->part_no;
                if (part_no==1&&EndingInfo->pr_final_part)  //телеграмма состоит из одной части
                {
                  InsQry.get().SetVariable("id",FNull);
                  InsQry.get().SetVariable("merge_key",FNull);
                  InsQry.get().Execute();
                  if(not TlgQry.FieldIsNULL("typeb_tlg_id"))
                      SaveTypebInHistory(TlgQry.FieldAsInteger("typeb_tlg_id"), InsQry.get().GetVariableAsInteger("ret_id"));
                  pr_typeb_cmd=true;
                  insert_typeb=true;
                }
                else throw ETlgError("Duplicate part number");
              }
              else
              {
                errorTlg(tlg_id,"DUP");
              };
            }
            else throw;
          };
          if (insert_typeb)
          {
            int typeb_tlg_id=InsQry.get().VariableIsNULL("id")?NoExists:InsQry.get().GetVariableAsInteger("id");
            int typeb_tlg_num=InsQry.get().GetVariableAsInteger("part_no");
            putTypeBBody(typeb_tlg_id, typeb_tlg_num, body);
          };
        };
      }
      catch(EXCEPTIONS::Exception &E)
      {
        OraSession.Rollback();
        try
        {
          EOracleError *orae=dynamic_cast<EOracleError*>(&E);
      	  if (orae!=NULL&&
      	      (orae->Code==4061||orae->Code==4068)) continue;
          ProgError(STDLOG,"Exception: %s (tlgs.id=%d)",
                       E.what(),TlgQry.FieldAsInteger("id"));
          errorTlg(tlg_id,"PARS",E.what());
          //sendErrorTlg("Exception: %s (tlgs.id=%d)",
          //             E.what(),TlgQry.FieldAsInteger("id"));
        }
        catch(...) {};
      };
      ProgTrace(TRACE5, "IN: PUT->DONE (sender=%s, tlg_num=%ld, time=%.10f)",
                        TlgQry.FieldAsString("sender"),
                        (unsigned long)TlgQry.FieldAsInteger("tlg_num"),
                        NowUTC());
      monitor_idle_zapr_type(1, QUEPOT_TLG_AIR);
    };
    queue_not_empty=!TlgQry.Eof;
    if (pr_typeb_cmd) sendCmd("CMD_TYPEB_PARSER","H");
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
    ProgTrace(TRACE5,"Attention! scan_tlg execute time: %ld secs, count=%d",
                     time_end-time_start,count);
  return queue_not_empty;
};

#define PARTS_NOT_RECEIVE_TIMEOUT  30.0/1440 //30 мин
#define OUT_OF_MEMORY_TIMEOUT      5.0/1440  //5 мин
#define PARSING_FORCE_TIMEOUT      5.0/1440  //5 мин
#define PARSING_MAX_TIMEOUT        0         //вообще не тормозим разборщик
#define SCAN_TIMEOUT               60.0/1440 //1 час

bool parse_tlg(void)
{
  bool queue_not_empty=false;

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
      "WHERE time_receive_not_parse>=:time_receive "
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
      "UPDATE tlgs_in SET time_parse=system.UTCSYSDATE, time_receive_not_parse=NULL "
      "WHERE id=:id AND time_parse IS NULL";
    TlgInUpdQry.DeclareVariable("id",otInteger);
  };

  TQuery TripsQry(&OraSession);

  TDateTime time_receive;
  int tlg_id,tlg_num,count;
  size_t len, bufLen=0;
  char *buf=NULL,*ph;
  bool forcibly;
  TTlgPartInfo part;
  THeadingInfo *HeadingInfo=NULL;
  TEndingInfo *EndingInfo=NULL;

  count=0;
  TlgIdQry.Execute();
  try
  {
    for(;!TlgIdQry.Eof&&count<PARSER_PROC_COUNT();TlgIdQry.Next(),OraSession.Rollback())
    {
      tlg_id=TlgIdQry.FieldAsInteger("id");
      ProgTrace(TRACE5, "tlg_id: %d", tlg_id);
      time_receive=TlgIdQry.FieldAsDateTime("time_receive");

      TlgInUpdQry.SetVariable("id",tlg_id);
      TlgInQry.SetVariable("id",tlg_id);
      //читаем все части телеграммы
      TlgInQry.Execute();
      if (TlgInQry.Eof) continue;
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
          //не все еще части собраны
          if (utc_date-time_receive > PARTS_NOT_RECEIVE_TIMEOUT)
            throw ETlgError("Some parts not received");
          else
            continue;
        };

        //собираем тело телеграммы из нескольких частей
        string body;
        for(;!TlgInQry.Eof;TlgInQry.Next(),tlg_num--)
        {
          if (tlg_num!=TlgInQry.FieldAsInteger("num")) break; //не все части собраны

          body=getTypeBBody(tlg_id, tlg_num, TlgInQry) + body;
        };
        if (tlg_num<0) throw ETlgError("Strange part found");

        if (!TlgInQry.Eof||tlg_num>0)
        {
          //не все еще части собраны
          if (utc_date-time_receive > PARTS_NOT_RECEIVE_TIMEOUT)
            throw ETlgError("Some parts not received");
          else
            continue;
          // не все части собраны
        };

        len=body.size()+1;
        if (len>bufLen)
        {
          if (buf==NULL)
            ph=(char*)mem.malloc(len, STDLOG);
          else
            ph=(char*)mem.realloc(buf,len, STDLOG);
          if (ph==NULL) throw EMemoryError("Out of memory");
          buf=ph;
          bufLen=len;
        };
        memcpy(buf, body.c_str(), body.size());
        buf[len-1]=0;

        switch (HeadingInfo->tlg_cat)
        {
          case tcDCS:
          {
            //разобрать телеграмму
            part.p=buf;
            part.line=1;
            TDCSHeadingInfo &info = *(dynamic_cast<TDCSHeadingInfo*>(HeadingInfo));
            if (strcmp(info.tlg_type,"PNL")==0||
                strcmp(info.tlg_type,"ADL")==0)
            {
              TPNLADLPRLContent con;
              ParsePNLADLPRLContent(part,info,con);
              //принудительно разобрать через определенное время после получения
              //(это будет работать только для ADL)
              forcibly=utc_date-time_receive > PARSING_FORCE_TIMEOUT;
              if (SavePNLADLPRLContent(tlg_id,info,con,forcibly))
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
                  //если телеграммы не хотят принудительно разбираться
                  //по истечении некоторого времени - записать в просроченные
                  throw ETlgError("Time limit reached");
              };
            };
            if (strcmp(info.tlg_type,"PRL")==0)
            {
              TPNLADLPRLContent con;
              ParsePNLADLPRLContent(part,info,con);
              SavePNLADLPRLContent(tlg_id,info,con,true);
              TlgInUpdQry.Execute();
              OraSession.Commit();
              count++;
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
            TBSMHeadingInfo &info = *(dynamic_cast<TBSMHeadingInfo*>(HeadingInfo));
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
            TAHMHeadingInfo &info = *(dynamic_cast<TAHMHeadingInfo*>(HeadingInfo));
            TFltInfo flt;
            TBindType bind_type;
            ParseAHMFltInfo(part,info,flt,bind_type);
            SaveFlt2(tlg_id,flt,bind_type);
            TlgInUpdQry.Execute();
            OraSession.Commit();
            count++;
            break;
          }
          case tcLCI:
          {
            part.p=buf;
            part.line=1;
            TLCIHeadingInfo &info = *(dynamic_cast<TLCIHeadingInfo*>(HeadingInfo));
            TLCIContent con;
            {
                string buf = part.p;
                ostringstream out_buf;
                for(string::iterator si = buf.begin(); si != buf.end(); si++) {
                    out_buf << *si << " " << hex << (int)*si << endl;
                }
                ProgTrace(TRACE5, "out_buf: %s", out_buf.str().c_str());
            }
            ParseLCIContent(part,info,con,mem);
            SaveLCIContent(tlg_id,info,con);
            TlgInUpdQry.Execute();
            OraSession.Commit();
            count++;
            break;
          }
          /*
          case tcSSM:
          {
            part.p=buf;
            part.line=1;
            TSSMHeadingInfo &info = *(dynamic_cast<TSSMHeadingInfo*>(HeadingInfo));
            TSSMContent con;
            ParseSSMContent(part,info,con,mem);
            SaveSSMContent(tlg_id,info,con);
            TlgInUpdQry.Execute();
            OraSession.Commit();
            count++;
            break;
          }
          case tcASM:
          {
            part.p=buf;
            part.line=1;
            TSSMHeadingInfo &info = *(dynamic_cast<TSSMHeadingInfo*>(HeadingInfo));
            TASMContent con;
            ParseASMContent(part,info,con,mem);
            SaveASMContent(tlg_id,info,con);
            TlgInUpdQry.Execute();
            OraSession.Commit();
            count++;
            break;
          }
          */
          default:
          {
            //телеграмму неизвестного типа сразу пишем в разобранные
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
        	{
        	  if (strcmp(E.what(),"Time limit reached")!=0)
              ProgError(STDLOG,"Telegram (tlgs_in.id: %d): %s",tlg_id,E.what());
            else
              ProgTrace(TRACE0,"Telegram (tlgs_in.id: %d): %s",tlg_id,E.what());
          };
          //sendErrorTlg("Telegram (tlgs_in.id: %d): %s",tlg_id,E.what());
          TlgInUpdQry.Execute();
          OraSession.Commit();
        }
        catch(...) {};
      };
    };
    queue_not_empty=!TlgIdQry.Eof;
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
                     
  return queue_not_empty;
};




