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
#include "mvt_parser.h"
#include "typeb_utils.h"
#include "telegram.h"
#include "memory_manager.h"
#include "qrys.h"
#include "TypeBHelpMng.h"
#include "serverlib/ourtime.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

#include "astra_service.h"
#include "serverlib/posthooks.h"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;
using namespace TypeB;

static int HANDLER_WAIT_INTERVAL()       //�����ᥪ㭤�
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_HANDLER_WAIT_INTERVAL",1,NoExists,60000);
  return VAR;
};

static int HANDLER_PROC_INTERVAL()       //�����ᥪ㭤�
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_HANDLER_PROC_INTERVAL",0,NoExists,1000);
  return VAR;
};

static int HANDLER_PROC_COUNT()          //���-�� ��ࠡ��뢠���� ��⥩ ⥫��ࠬ� �� ���� �����
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_HANDLER_PROC_COUNT",1,NoExists,100);
  return VAR;
};

static int PARSER_WAIT_INTERVAL()       //�����ᥪ㭤�
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_PARSER_WAIT_INTERVAL",1,NoExists,60000);
  return VAR;
};

static int PARSER_PROC_INTERVAL()       //�����ᥪ㭤�
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_PARSER_PROC_INTERVAL",0,NoExists,10000);
  return VAR;
};

static int PARSER_PROC_COUNT()          //���-�� ࠧ��ࠥ��� ⥫��ࠬ� �� ���� �����
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TYPEB_PARSER_PROC_COUNT",1,NoExists,100);
  return VAR;
};

static bool handle_tlg(void);
static bool parse_tlg(void);

int main_typeb_handler_tcl(int supervisorSocket, int argc, char *argv[])
{
  try
  {
    sleep(10);
    InitLogTime(argc>0?argv[0]:NULL);

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();

    char buf[10];
    for(;;)
    {
      InitLogTime(argc>0?argv[0]:NULL);
      base_tables.Invalidate();
      bool queue_not_empty=handle_tlg();

      callPostHooksAfter();
      emptyHookTables();

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

int main_typeb_parser_tcl(int supervisorSocket, int argc, char *argv[])
{
  try
  {
    sleep(15);
    InitLogTime(argc>0?argv[0]:NULL);

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();

    char buf[10];
    for(;;)
    {
      InitLogTime(argc>0?argv[0]:NULL);
      base_tables.Invalidate();
      bool queue_not_empty=parse_tlg();

      // This block added specially for TypeBHelpMng::notify(typeb_in_id)
      // notify func registers hook(setHAfter) which need to be handled here.
      // callPostHooksAfter() - calls after all possible commits
      callPostHooksAfter();
      emptyHookTables();
      //

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

void progError(int tlg_id,
               int part_no,
               int &error_no,
               const std::exception &E,
               const string &tlg_type,
               const TFlightsForBind &flts)
{
  const ETlgError *tlge=dynamic_cast<const ETlgError*>(&E);
  bool only_trace=(tlge!=NULL && (tlge->error_type()==tlgeNotError ||
                                  tlge->error_type()==tlgeNotMonitorNotAlarm ||
                                  tlge->error_type()==tlgeNotMonitorYesAlarm));

  if (part_no!=NoExists)
    only_trace?ProgTrace(TRACE0, "Telegram (id=%d, part_no=%d, type=%s): %s", tlg_id, part_no, tlg_type.c_str(), E.what()):
               ProgError(STDLOG, "Telegram (id=%d, part_no=%d, type=%s): %s", tlg_id, part_no, tlg_type.c_str(), E.what());
  else
    only_trace?ProgTrace(TRACE0, "Telegram (id=%d, type=%s): %s", tlg_id, tlg_type.c_str(), E.what()):
               ProgError(STDLOG, "Telegram (id=%d, type=%s): %s", tlg_id, tlg_type.c_str(), E.what());

  const EOracleError *orae=dynamic_cast<const EOracleError*>(&E);
  if (orae!=NULL)
  {
    only_trace?ProgTrace(TRACE0, "SQL: %s", orae->SQLText()):
               ProgError(STDLOG, "SQL: %s", orae->SQLText());
  };

  if (tlge!=NULL)
  {
    if (tlge->error_line()!=NoExists)
      only_trace?ProgTrace(TRACE0, "Line %d: %s", tlge->error_line(), tlge->error_text().c_str()):
                 ProgError(STDLOG, "Line %d: %s", tlge->error_line(), tlge->error_text().c_str());
    else
    {
      if (!tlge->error_text().empty())
        only_trace?ProgTrace(TRACE0, "Line: %s", tlge->error_text().c_str()):
                   ProgError(STDLOG, "Line: %s", tlge->error_text().c_str());
    };
  };

  for(TFlightsForBind::const_iterator f=flts.begin(); f!=flts.end(); ++f)
  {
    ostringstream flight;
    flight << f->first.airline
           << setw(3) << setfill('0') << f->first.flt_no << f->first.suffix
           << "/" << DateTimeToStr(f->first.scd, "ddmmm")
           << " " << f->first.airp_dep << f->first.airp_arv;
    if (f==flts.begin())
      only_trace?ProgTrace(TRACE0, "Flights: %s", flight.str().c_str()):
                 ProgError(STDLOG, "Flights: %s", flight.str().c_str());
    else
      only_trace?ProgTrace(TRACE0, "         %s", flight.str().c_str()):
                 ProgError(STDLOG, "         %s", flight.str().c_str());
  };

  if (tlge!=NULL)
    errorTypeB(tlg_id, part_no, error_no, tlge->error_pos(), tlge->error_len(), E.what());
  else
    errorTypeB(tlg_id, part_no, error_no, NoExists, NoExists, E.what());
};

void bindTypeB(int typeb_tlg_id, const TFlightsForBind &flts, ETlgErrorType error_type)
{
  for(TFlightsForBind::const_iterator f=flts.begin(); f!=flts.end(); ++f)
  {
    try
    {
      TFltInfo normFlt(f->first);
      NormalizeFltInfo(normFlt);
      SaveFlt(typeb_tlg_id, normFlt, f->second, error_type);
    }
    catch(...) {};
  };
};

void bindTypeB(int typeb_tlg_id, const TFlightsForBind &flts, const std::exception &E)
{
  const ETlgError *tlge=dynamic_cast<const ETlgError*>(&E);
  if (tlge!=NULL)
    bindTypeB(typeb_tlg_id, flts, tlge->error_type());
  else
    bindTypeB(typeb_tlg_id, flts, tlgeYesMonitorYesAlarm);
};

void forwardTypeB(const int typeb_tlg_id,
                  const int typeb_tlg_num,
                  const string &typeb_tlg_type)
{
  if (typeb_tlg_type!="PNL" && typeb_tlg_type!="ADL") return;
  TCachedQuery Qry("SELECT tlg_binding.point_id_spp "
                   "FROM tlg_binding, tlg_source "
                   "WHERE tlg_binding.point_id_tlg=tlg_source.point_id_tlg AND tlg_source.tlg_id=:tlg_id",
                   QParams() << QParam("tlg_id", otInteger, typeb_tlg_id));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    TForwarder forwarder(Qry.get().FieldAsInteger("point_id_spp"), typeb_tlg_id, typeb_tlg_num);
    forwarder << typeb_tlg_type << "PNLADL";
    vector<TypeB::TCreateInfo> createInfo;
    forwarder.getInfo(createInfo);
    TelegramInterface::SendTlg(createInfo, NoExists, true);
  };
};

bool handle_tlg(void)
{
  bool queue_not_empty=false;

  time_t time_start=time(NULL);

  TMemoryManager mem(STDLOG);

  static TQuery TlgQry(&OraSession);
  if (TlgQry.SQLText.IsEmpty())
  {
    //�������� ���冷� ��ꥤ������ ⠡��� �����!
    TlgQry.Clear();
    TlgQry.SQLText=
      "SELECT tlg_queue.id,tlg_queue.time,ttl, "
      "       tlg_queue.tlg_num,tlg_queue.sender, "
      "       NVL(tlg_queue.proc_attempt,0) AS proc_attempt "
      "FROM tlg_queue "
      "WHERE tlg_queue.receiver=:receiver AND "
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
     "        merge_key= :merge_key AND rownum<2; "
     "EXCEPTION "
     "  WHEN NO_DATA_FOUND THEN NULL; "
     "END;";
    TlgIdQry.DeclareVariable("id",otInteger);
    TlgIdQry.DeclareVariable("tlg_type",otString);
    TlgIdQry.DeclareVariable("min_time_create",otDate);
    TlgIdQry.DeclareVariable("max_time_create",otDate);
    TlgIdQry.DeclareVariable("merge_key",otString);
  };

  const char* ins_sql=
    "DECLARE"
    "  CURSOR cur(vid tlgs_in.id%TYPE) IS "
    "    SELECT id, time_parse FROM tlgs_in "
    "    WHERE id=vid ORDER BY num DESC; "
    "  vtime_parse tlgs_in.time_parse%TYPE; "
    "  vtime_receive_not_parse tlgs_in.time_receive_not_parse%TYPE; "
    "  vnow DATE; "
    "BEGIN "
    "  vnow:=system.UTCSYSDATE; "
    "  vtime_parse:=NULL; "
    "  vtime_receive_not_parse:=vnow; "
    "  IF :id IS NULL THEN "
    "    SELECT tlg_in_out__seq.nextval INTO :id FROM dual; "
    "    INSERT INTO typeb_in(id,proc_attempt) VALUES(:id,0); "
    "  ELSE "
    "    FOR curRow IN cur(:id) LOOP "
    "      IF curRow.time_parse IS NOT NULL THEN "
    "        vtime_parse:=vnow; "
    "        vtime_receive_not_parse:=NULL; "
    "      END IF;"
    "    END LOOP; "
    "  END IF; "
    "  INSERT INTO tlgs_in(id,num,type,addr,heading,ending,is_final_part, "
    "    merge_key,time_create,time_receive,time_parse,time_receive_not_parse) "
    "  VALUES(:id,:part_no,:tlg_type,:addr,:heading,:ending,:is_final_part, "
    "    :merge_key,:time_create,vnow,vtime_parse,vtime_receive_not_parse); "
    "  UPDATE typeb_in_history SET tlg_id=:id WHERE id=:tlgs_id; "
    "  UPDATE tlgs SET typeb_tlg_id=:id, typeb_tlg_num=:part_no WHERE id=:tlgs_id; "
    "END;";
  QParams QryParams;
  QryParams << QParam("id",otInteger)
            << QParam("part_no",otInteger)
            << QParam("tlg_type",otString)
            << QParam("addr",otString)
            << QParam("heading",otString)
            << QParam("ending",otString)
            << QParam("is_final_part",otInteger)
            << QParam("merge_key",otString)
            << QParam("time_create",otDate)
            << QParam("tlgs_id",otInteger);

  TCachedQuery InsQry(ins_sql, QryParams);

  TQuery Qry(&OraSession);

  int count;
  bool pr_typeb_cmd=false;
  TTlgPartsText parts;
  THeadingInfo *HeadingInfo=NULL;
  TEndingInfo *EndingInfo=NULL;

  count=0;
  TlgQry.Execute();
  try
  {
    for (;!TlgQry.Eof&&count<HANDLER_PROC_COUNT();count++,TlgQry.Next(),OraSession.Commit())
    {
      //�஢�ਬ TTL
      int tlg_id=TlgQry.FieldAsInteger("id");
      int error_no=NoExists;

      TFlightsForBind bind_flts;
      if (HeadingInfo!=NULL)
      {
        mem.destroy(HeadingInfo, STDLOG);
        delete HeadingInfo;
        HeadingInfo = NULL;
      };
      if (EndingInfo!=NULL)
      {
        mem.destroy(EndingInfo, STDLOG);
        delete EndingInfo;
        EndingInfo = NULL;
      };

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

        string tlgs_text=getTlgText(tlg_id);
        int typeb_tlg_id=NoExists;
        int typeb_tlg_num=1;
        ostringstream merge_key;

        list<ETlgError> errors;

        try
        {
          GetParts(tlgs_text.c_str(), parts, HeadingInfo, bind_flts, mem);
          if (parts.addr.size()>255) throw ETlgError("Address too long");
          if (parts.heading.size()>100) throw ETlgError("Header too long");
          if (parts.ending.size()>20) throw ETlgError("End of message too long");

          TTlgPartInfo part;
          part.p=parts.heading.c_str();
          part.EOL_count=CalcEOLCount(parts.addr.c_str());
          part.offset=parts.addr.size();
          ParseHeading(part,HeadingInfo,bind_flts,mem);

          part.p=parts.ending.c_str();
          part.EOL_count+=CalcEOLCount(parts.heading.c_str());
          part.EOL_count+=CalcEOLCount(parts.body.c_str());
          part.offset+=parts.heading.size();
          part.offset+=parts.body.size();
          ParseEnding(part,HeadingInfo,EndingInfo,mem);

          if ((HeadingInfo->tlg_cat==tcDCS ||
               HeadingInfo->tlg_cat==tcBSM) &&
              HeadingInfo->time_create!=0)
          {
            merge_key << "." << HeadingInfo->sender;
            bool association_number_exists=false;
            if (HeadingInfo->tlg_cat==tcDCS)
            {
              TDCSHeadingInfo &info = *dynamic_cast<TDCSHeadingInfo*>(HeadingInfo);
              typeb_tlg_num=info.part_no;
              merge_key << " " << info.flt.airline << setw(3) << setfill('0') << info.flt.flt_no
                               << info.flt.suffix << "/" << DateTimeToStr(info.flt.scd,"ddmmm") << " "
                               << info.flt.airp_dep << info.flt.airp_arv;
              if (info.association_number!=NoExists)
              {
                association_number_exists=true;
                merge_key << " " << setw(6) << setfill('0') << info.association_number;                
              };

              TTripInfo fltInfo;
              TElemFmt fmt;
              fltInfo.airline = ElemToElemId(etAirline, info.flt.airline, fmt);
              fltInfo.airp = ElemToElemId(etAirp, info.flt.airp_dep, fmt);
              fltInfo.flt_no = info.flt.flt_no;
              fltInfo.suffix = ElemToElemId(etSuffix, info.flt.suffix, fmt);
              fltInfo.scd_out = info.flt.scd;
              map<string, string> extra;
              putUTG(tlg_id, typeb_tlg_num, info.tlg_type, fltInfo, tlgs_text, extra);

            }
            else
            {
              TBSMHeadingInfo &info = *dynamic_cast<TBSMHeadingInfo*>(HeadingInfo);
              typeb_tlg_num=info.part_no;
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
              if (strcmp(HeadingInfo->tlg_type,"ADL")==0 && association_number_exists)
              {
                TlgIdQry.SetVariable("min_time_create",HeadingInfo->time_create-5.0/1440);
                TlgIdQry.SetVariable("max_time_create",HeadingInfo->time_create+5.0/1440);
              }
              else
              {
                TlgIdQry.SetVariable("min_time_create",HeadingInfo->time_create);
                TlgIdQry.SetVariable("max_time_create",HeadingInfo->time_create);
              };
            };
            TlgIdQry.Execute();
            if (!TlgIdQry.VariableIsNULL("id"))
              typeb_tlg_id=TlgIdQry.GetVariableAsInteger("id");
          };
        }
        catch(ETlgError &E)
        {
          errors.push_back(E);
          parts.clear();
          parts.body=tlgs_text;
        };

        if (typeb_tlg_id!=NoExists)
          InsQry.get().SetVariable("id",typeb_tlg_id);
        else
          InsQry.get().SetVariable("id",FNull);
        InsQry.get().SetVariable("part_no",typeb_tlg_num);
        InsQry.get().SetVariable("merge_key",merge_key.str());

        if (HeadingInfo!=NULL)
        {
          InsQry.get().SetVariable("tlg_type",HeadingInfo->tlg_type);
          if (HeadingInfo->time_create!=0)
            InsQry.get().SetVariable("time_create",HeadingInfo->time_create);
          else
            InsQry.get().SetVariable("time_create",FNull);
        }
        else
        {
          InsQry.get().SetVariable("tlg_type",FNull);
          InsQry.get().SetVariable("time_create",FNull);
        };

        InsQry.get().SetVariable("addr", parts.addr);
        InsQry.get().SetVariable("heading", parts.heading);
        InsQry.get().SetVariable("ending", parts.ending);

        if (EndingInfo!=NULL)
          InsQry.get().SetVariable("is_final_part", (int)EndingInfo->pr_final_part);
        else
          InsQry.get().SetVariable("is_final_part", (int)false);

        InsQry.get().SetVariable("tlgs_id", tlg_id);

        if (deleteTlg(tlg_id))
        {
          bool insert_typeb=false;
          try
          {
            if (typeb_tlg_id!=NoExists)
              procTypeB(typeb_tlg_id, 0); //��窠
            InsQry.get().Execute();
            pr_typeb_cmd=true;
            insert_typeb=true;
          }
          catch(EOracleError E)
          {
            if (E.Code==1)
            {
              if (!errors.empty()) throw Exception("handle_tlg: strange situation");
              if (typeb_tlg_id==NoExists) throw Exception("handle_tlg: strange situation");

              Qry.Clear();
              Qry.SQLText=
                "SELECT addr,heading,ending FROM tlgs_in WHERE id=:id AND num=:num";
              Qry.CreateVariable("id",otInteger,typeb_tlg_id);
              Qry.CreateVariable("num",otInteger,typeb_tlg_num);
              Qry.Execute();

              ostringstream typeb_in_text;
              if (!Qry.Eof)
                typeb_in_text << Qry.FieldAsString("addr")
                              << Qry.FieldAsString("heading")
                              << getTypeBBody(typeb_tlg_id, typeb_tlg_num)
                              << Qry.FieldAsString("ending");

              InsQry.get().SetVariable("id",FNull);
              InsQry.get().SetVariable("merge_key",FNull);
              InsQry.get().Execute();
              pr_typeb_cmd=true;
              insert_typeb=true;

              if (tlgs_text!=typeb_in_text.str())
              {
                long part_no;
                if (HeadingInfo->tlg_cat==tcDCS)
                  part_no=dynamic_cast<TDCSHeadingInfo*>(HeadingInfo)->part_no;
                else
                  part_no=dynamic_cast<TBSMHeadingInfo*>(HeadingInfo)->part_no;
                if (!(part_no==1&&EndingInfo->pr_final_part))  //⥫��ࠬ�� �� ��⮨� �� ����� ���
                  errors.push_back(ETlgError("Duplicate part number with different text"));
              }
              else errors.push_back(ETlgError(tlgeNotMonitorNotAlarm, "Telegram duplicated"));
            }
            else throw;
          };
          if (insert_typeb)
          {
            typeb_tlg_id=InsQry.get().VariableIsNULL("id")?NoExists:InsQry.get().GetVariableAsInteger("id");
            if (typeb_tlg_id==NoExists) throw Exception("handle_tlg: strange situation");
            typeb_tlg_num=InsQry.get().GetVariableAsInteger("part_no");
            string typeb_tlg_type=InsQry.get().GetVariableAsString("tlg_type");
            putTypeBBody(typeb_tlg_id, typeb_tlg_num, parts.body);

            if (!errors.empty())
            {
              ETlgErrorType etype=tlgeNotError;
              for(list<ETlgError>::const_iterator e=errors.begin(); e!=errors.end(); ++e)
              {
                progError(typeb_tlg_id, typeb_tlg_num, error_no, *e, "", bind_flts);  //��� �� ��������, �⮡� ��।������ tlg_type
                if (etype < e->error_type()) etype=e->error_type();
              };
              errorTlg(tlg_id,"PARS");
              parseTypeB(typeb_tlg_id);
              bindTypeB(typeb_tlg_id, bind_flts, etype);
              TypeBHelpMng::notify(typeb_tlg_id, ASTRA::NoExists); // �⢥訢��� �����, �᫨ ����.
            }
            else
            {
              bindTypeB(typeb_tlg_id, bind_flts, tlgeNotError); //�ਢ�뢠�� �⮡� ��⮬ �������� ᤥ���� forwarding
            };
            forwardTypeB(typeb_tlg_id, typeb_tlg_num, typeb_tlg_type);
          };
        };
      }
      catch(std::exception &E)
      {
        OraSession.Rollback();
        try
        {
          EOracleError *orae=dynamic_cast<EOracleError*>(&E);
          if (orae!=NULL&&
              (orae->Code==4061||orae->Code==4068)) continue;
          progError(tlg_id, NoExists, error_no, E, "", bind_flts);  //��� �� ��������, �⮡� ��।������ tlg_type
          errorTlg(tlg_id,"PARS",E.what());
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
  }
  catch(...)
  {
    mem.destroy(HeadingInfo, STDLOG);
    if (HeadingInfo!=NULL) delete HeadingInfo;
    mem.destroy(EndingInfo, STDLOG);
    if (EndingInfo!=NULL) delete EndingInfo;
    throw;
  };

  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! scan_tlg execute time: %ld secs, count=%d",
                     time_end-time_start,count);
  return queue_not_empty;
};

#define PARTS_NOT_RECEIVE_TIMEOUT  30.0/1440 //30 ���
#define PARSING_FORCE_TIMEOUT      5.0/1440  //5 ���
#define PARSING_MAX_TIMEOUT        0         //����� �� �ମ��� ࠧ���騪
#define SCAN_TIMEOUT               60.0/1440 //1 ��

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
      "SELECT tlgs_in.id, "
      "       MAX(time_receive) AS time_receive, "
      "       MAX(time_create) AS max_time_create, "
      "       MIN(time_receive) AS min_time_receive, "
      "       MIN(NVL(typeb_in.proc_attempt,0)) AS proc_attempt "
      "FROM tlgs_in, typeb_in "
      "WHERE tlgs_in.id = typeb_in.id(+) AND "   //��⮬ ����(+)
      "      time_receive_not_parse>=:time_receive "
      "GROUP BY tlgs_in.id "
      "ORDER BY max_time_create,min_time_receive,tlgs_in.id";
    TlgIdQry.CreateVariable("time_receive",otDate,utc_date-SCAN_TIMEOUT);
  };

  static TQuery TlgInQry(&OraSession);
  if (TlgInQry.SQLText.IsEmpty())
  {
    TlgInQry.Clear();
    TlgInQry.SQLText=
      "SELECT id,num,type,addr,heading,ending "
      "FROM tlgs_in "
      "WHERE id=:id "
      "ORDER BY num DESC";
    TlgInQry.DeclareVariable("id",otInteger);
  };

  TQuery TripsQry(&OraSession);

  TDateTime time_receive;
  int count;
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
      int tlg_id=TlgIdQry.FieldAsInteger("id");
      int error_no=NoExists;
      if (TlgIdQry.FieldAsInteger("proc_attempt")>=HANDLER_PROC_ATTEMPTS())
      {
        ProgTrace(TRACE5, "parse_tlg: tlg_id=%d proc_attempt=%d", tlg_id, TlgIdQry.FieldAsInteger("proc_attempt"));
        Exception E("%d attempts to parse previously failed", TlgIdQry.FieldAsInteger("proc_attempt"));
        progError(tlg_id, NoExists, error_no, E, "", TFlightsForBind());
        parseTypeB(tlg_id);
        OraSession.Commit();
        count++;
        continue;
      };
      procTypeB(tlg_id, 1);
      OraSession.Commit();

      ProgTrace(TRACE5, "tlg_id: %d", tlg_id);
      time_receive=TlgIdQry.FieldAsDateTime("time_receive");

      procTypeB(tlg_id, 0); //��窠
      TlgInQry.SetVariable("id",tlg_id);
      //�⠥� �� ��� ⥫��ࠬ��
      TlgInQry.Execute();
      if (TlgInQry.Eof) continue;
      int tlg_num=TlgInQry.FieldAsInteger("num");
      string tlg_type=TlgInQry.FieldAsString("type");
      TTlgPartsText parts;
      parts.addr=TlgInQry.FieldAsString("addr");
      parts.heading=TlgInQry.FieldAsString("heading");
      parts.body=getTypeBBody(tlg_id, tlg_num);
      parts.ending=TlgInQry.FieldAsString("ending");

      TFlightsForBind bind_flts;
      try
      {
        part.p=parts.heading.c_str();
        part.EOL_count=CalcEOLCount(parts.addr.c_str());
        part.offset=parts.addr.size();
        ParseHeading(part,HeadingInfo,bind_flts,mem);

        part.p=parts.ending.c_str();
        part.EOL_count+=CalcEOLCount(parts.heading.c_str());
        part.EOL_count+=CalcEOLCount(parts.body.c_str());
        part.offset+=parts.heading.size();
        part.offset+=parts.body.size();
        ParseEnding(part,HeadingInfo,EndingInfo,mem);
      }
      catch(std::exception &E)
      {
        count++;
        EOracleError *orae=dynamic_cast<EOracleError*>(&E);
        if (orae!=NULL&&
            (orae->Code==4061||orae->Code==4068)) continue;
        progError(tlg_id, tlg_num, error_no, E, tlg_type, bind_flts);
        parseTypeB(tlg_id);
        bindTypeB(tlg_id, bind_flts, E);
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
          procTypeB(tlg_id, -1);
          OraSession.Commit();
          continue;
        };

        parts.body.clear();
        //ᮡ�ࠥ� ⥫� ⥫��ࠬ�� �� ��᪮�쪨� ��⥩
        for(;!TlgInQry.Eof;TlgInQry.Next(),tlg_num--)
        {
          if (tlg_num!=TlgInQry.FieldAsInteger("num")) break; //�� �� ��� ᮡ࠭�

          parts.heading=TlgInQry.FieldAsString("heading");
          parts.body=getTypeBBody(tlg_id, tlg_num) + parts.body;
        };
        if (tlg_num<0) throw ETlgError("Strange part found");

        if (!TlgInQry.Eof||tlg_num>0)
        {
          //�� �� �� ��� ᮡ࠭�
          if (utc_date-time_receive > PARTS_NOT_RECEIVE_TIMEOUT)
            throw ETlgError("Some parts not received");
          procTypeB(tlg_id, -1);
          OraSession.Commit();
          continue;
        };

        part.p=parts.body.c_str();
        part.EOL_count=CalcEOLCount(parts.addr.c_str())+
                       CalcEOLCount(parts.heading.c_str());
        part.offset=parts.addr.size()+
                    parts.heading.size();

        switch (HeadingInfo->tlg_cat)
        {
          case tcDCS:
          {
            //ࠧ����� ⥫��ࠬ��
            TDCSHeadingInfo &info = *(dynamic_cast<TDCSHeadingInfo*>(HeadingInfo));
            if (strcmp(info.tlg_type,"PNL")==0||
                strcmp(info.tlg_type,"ADL")==0)
            {
              TPNLADLPRLContent con;
              ParsePNLADLPRLContent(part,info,con);
              //�ਭ㤨⥫쭮 ࠧ����� �१ ��।������� �६� ��᫥ ����祭��
              //(�� �㤥� ࠡ���� ⮫쪮 ��� ADL)
              forcibly=utc_date-time_receive > PARSING_FORCE_TIMEOUT;
              if (SavePNLADLPRLContent(tlg_id,info,con,forcibly))
              {
                parseTypeB(tlg_id);
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
                  throw ETlgError(tlgeNotMonitorYesAlarm, "Time limit reached");
                procTypeB(tlg_id, -1);
                OraSession.Commit();
              };
            };
            if (strcmp(info.tlg_type,"PRL")==0)
            {
              TPNLADLPRLContent con;
              ParsePNLADLPRLContent(part,info,con);
              SavePNLADLPRLContent(tlg_id,info,con,true);
              parseTypeB(tlg_id);
              OraSession.Commit();
              count++;
            };
            if (strcmp(info.tlg_type,"PTM")==0)
            {
              TPtmContent con;
              ParsePTMContent(part,info,con);
              SavePTMContent(tlg_id,info,con);
              parseTypeB(tlg_id);
              OraSession.Commit();
              count++;
            };
            if (strcmp(info.tlg_type,"SOM")==0)
            {
              TSOMContent con;
              ParseSOMContent(part,info,con);
              SaveSOMContent(tlg_id,info,con);
              parseTypeB(tlg_id);
              OraSession.Commit();
              count++;
            };
            break;
          }
          case tcBSM:
          {
            TBSMHeadingInfo &info = *(dynamic_cast<TBSMHeadingInfo*>(HeadingInfo));
            if (strcmp(info.tlg_type,"BTM")==0)
            {
              TBtmContent con;
              ParseBTMContent(part,info,con,mem);
              SaveBTMContent(tlg_id,info,con);
              parseTypeB(tlg_id);
              OraSession.Commit();
              count++;
            };
            break;
          }
          case tcAHM:
          {
            TAHMHeadingInfo &info = *(dynamic_cast<TAHMHeadingInfo*>(HeadingInfo));
            ParseAHMFltInfo(part,info,info.flt,info.bind_type);

            if((string)info.tlg_type == "MVT") {
                MVTParser::TMVTContent con;
                MVTParser::ParseMVTContent(part, info, con, mem);
                MVTParser::SaveMVTContent(tlg_id, info, con);
            } else {
                SaveFlt(tlg_id,info.flt,info.bind_type);
            }
            parseTypeB(tlg_id);
            OraSession.Commit();
            count++;
            break;
          }
          case tcLCI:
          {
            TLCIHeadingInfo &info = *(dynamic_cast<TLCIHeadingInfo*>(HeadingInfo));
            TLCIContent con;
            {
                string buf = part.p;
                ostringstream out_buf;
                for(string::iterator si = buf.begin(); si != buf.end(); si++) {
                    out_buf << *si << " " << hex << (int)*si << std::endl;
                }
                ProgTrace(TRACE5, "out_buf: %s", out_buf.str().c_str());
            }
            ParseLCIContent(part,info,con,mem);
            SaveLCIContent(tlg_id,info,con);
            parseTypeB(tlg_id);
            OraSession.Commit();
            count++;
            break;
          }
          /*
          case tcSSM:
          {
            TSSMHeadingInfo &info = *(dynamic_cast<TSSMHeadingInfo*>(HeadingInfo));
            TSSMContent con;
            ParseSSMContent(part,info,con,mem);
            SaveSSMContent(tlg_id,info,con);
            parseTypeB(tlg_id);
            OraSession.Commit();
            count++;
            break;
          }
          case tcASM:
          {
            TSSMHeadingInfo &info = *(dynamic_cast<TSSMHeadingInfo*>(HeadingInfo));
            TASMContent con;
            ParseASMContent(part,info,con,mem);
            SaveASMContent(tlg_id,info,con);
            parseTypeB(tlg_id);
            OraSession.Commit();
            count++;
            break;
          }
          */
          default:
          {
            //⥫��ࠬ�� �������⭮�� ⨯� �ࠧ� ��襬 � ࠧ��࠭��
            parseTypeB(tlg_id);
            OraSession.Commit();
            count++;
          };
        };
      }
      catch(std::exception &E)
      {
        count++;
        OraSession.Rollback();
        try
        {
          EOracleError *orae=dynamic_cast<EOracleError*>(&E);
          if (orae!=NULL&&
             (orae->Code==4061||orae->Code==4068)) continue;
          progError(tlg_id, NoExists, error_no, E, tlg_type, bind_flts);
          parseTypeB(tlg_id);
          bindTypeB(tlg_id, bind_flts, E);
          TypeBHelpMng::notify(tlg_id, ASTRA::NoExists); // �⢥訢��� �����, �᫨ ����.
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
  }
  catch(...)
  {
    mem.destroy(HeadingInfo, STDLOG);
    if (HeadingInfo!=NULL) delete HeadingInfo;
    mem.destroy(EndingInfo, STDLOG);
    if (EndingInfo!=NULL) delete EndingInfo;
    throw;
  };

  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! handle_tlg execute time: %ld secs, count=%d",
                     time_end-time_start,count);

  return queue_not_empty;
};

void get_tlg_info(
        const string &tlg_text,
        string &tlg_type,
        string &airline,
        string &airp)
{
    tlg_type.clear();
    airline.clear();
    airp.clear();
    TypeB::TTlgPartsText parts;
    TypeB::THeadingInfo *HeadingInfo = NULL;
    TypeB::TFlightsForBind bind_flts;
    TMemoryManager mem(STDLOG);

    try
    {
        GetParts(tlg_text.c_str(), parts, HeadingInfo, bind_flts, mem);
        TypeB::TTlgPartInfo part;
        part.p = parts.heading.c_str();
        part.EOL_count = TypeB::CalcEOLCount(parts.addr.c_str());
        part.offset = parts.addr.size();
        ParseHeading(part, HeadingInfo, bind_flts, mem);

        tlg_type = HeadingInfo->tlg_type;

        part.p=parts.body.c_str();
        part.EOL_count=CalcEOLCount(parts.addr.c_str())+
            CalcEOLCount(parts.heading.c_str());
        part.offset=parts.addr.size()+
            parts.heading.size();

        switch (HeadingInfo->tlg_cat)
        {
            case tcLCI:
                {
                    TLCIHeadingInfo &info = *(dynamic_cast<TLCIHeadingInfo*>(HeadingInfo));
                    airline = info.flt_info.flt.airline.c_str();
                    airp = info.flt_info.airp;
                    break;
                }
            case tcAHM:
                {
                    TAHMHeadingInfo &info = *(dynamic_cast<TAHMHeadingInfo*>(HeadingInfo));
                    TFltInfo flt;
                    TBindType bind_type;
                    ParseAHMFltInfo(part,info,flt,bind_type);
                    vector<int> spp_point_ids;
                    TTlgBinding(false).bind_flt(flt,bind_type,spp_point_ids);
                    set<int> s(spp_point_ids.begin(), spp_point_ids.end()); // remove duplicates
                    if(s.size() == 1) {
                        TCachedQuery Qry("select airp from points where point_id = :id",
                                QParams() << QParam("id", otInteger, *s.begin()));
                        Qry.get().Execute();
                        if(not Qry.get().Eof) {
                            airline = flt.airline;
                            airp = Qry.get().FieldAsString("airp");
                        }
                    }
                    break;
                }
            default:
                ;
        }

        if (HeadingInfo != NULL)
        {
            mem.destroy(HeadingInfo, STDLOG);
            delete HeadingInfo;
            HeadingInfo = NULL;
        };
    }
    catch(...)
    {
        mem.destroy(HeadingInfo, STDLOG);
        if (HeadingInfo!=NULL) delete HeadingInfo;
        throw;
    };
}
