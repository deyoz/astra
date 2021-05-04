#include "date_time.h"
#include "astra_service.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
#include "lci_parser.h"
#include "ucm_parser.h"
#include "mvt_parser.h"
#include "uws_parser.h"
#include "ldm_parser.h"
#include "ifm_parser.h"
#include "typeb_utils.h"
#include "telegram.h"
#include "memory_manager.h"
#include "astra_main.h"
#include "qrys.h"
#include "TypeBHelpMng.h"
#include "edi_handler.h"
#include "typeb_handler.h"
#include "tlg_source_typeb.h"
#include "postpone_edifact.h"
#include "remote_system_context.h"
#include "astra_context.h"
#include "astra_ssim.h"
#include "db_tquery.h"

#include <serverlib/posthooks.h>
#include <serverlib/ourtime.h>
#include <serverlib/TlgLogger.h>
#include <serverlib/cursctl.h>
#include <serverlib/TlgLogger.h>
#include <serverlib/testmode.h>

#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <math.h>
#include <iostream>
#include <regex>

#include "PgOraConfig.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/test.h>


using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;
using namespace TypeB;

namespace TypeB {
    const std::string all_other_handler_id="all_other";
}

static void handle_tpb_tlg(const tlg_info &tlg);
static void prehandle_tpb_tlg(tlg_info &tlg);

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

static void initRsc(int tlg_id, const std::string& tlg_body)
{
    using Ticketing::RemoteSystemContext::SystemContext;
    SystemContext::initDummyContext();
    SystemContext::Instance(STDLOG).inbTlgInfo().setTlgSrc(tlg_body);
    tlgnum_t tlgNum = make_tlgnum(tlg_id);
    SystemContext::Instance(STDLOG).inbTlgInfo().setTlgNum(tlgNum);
    if(TlgHandling::isTlgPostponed(tlgNum)) {
        SystemContext::Instance(STDLOG).inbTlgInfo().setRepeatedlyProcessed();
    }
}

static bool handle_tlg(void);
static bool parse_tlg(const string &handler_id);

int main_typeb_handler_tcl(int supervisorSocket, int argc, char *argv[])
{
  try
  {
    sleep(10);
    InitLogTime(argc>0?argv[0]:NULL);

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();
    TlgLogger::setLogging();

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
  catch(const EOracleError &E)
  {
    ProgError(STDLOG,"EOracleError %d: %s\nSQL: %s)",E.Code,E.what(),E.SQLText());
  }
  catch(const std::exception &E)
  {
    ProgError(STDLOG,"std::exception: %s",E.what());
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };
  try
  {
    ASTRA::rollback();
    OraSession.LogOff();
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };
  return 0;
}

string getSocketName(const string &proc_name)
{
    return "CMD_TYPEB_PARSER_" + upperc(proc_name);
}

int main_typeb_parser_tcl(int supervisorSocket, int argc, char *argv[])
{
  try
  {
    sleep(15);
    InitLogTime(argc>0?argv[0]:NULL);

    std::string handler_id;
    if(argc!=2) {
      LogError(STDLOG) << __FUNCTION__ << ": wrong number of parameters: " << argc;
    } else {
        handler_id = argv[1];
    }

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();
    init_locale();

    char buf[10];
    for(;;)
    {
      InitLogTime(argc>0?argv[0]:NULL);
      base_tables.Invalidate();
      bool queue_not_empty=parse_tlg(handler_id);

      // This block added specially for TypeBHelpMng::notify(typeb_in_id)
      // notify func registers hook(setHAfter) which need to be handled here.
      // callPostHooksAfter() - calls after all possible commits
      callPostHooksAfter();
      emptyHookTables();
      //

      waitCmd(getSocketName(handler_id).c_str(),queue_not_empty?PARSER_PROC_INTERVAL():PARSER_WAIT_INTERVAL(),buf,sizeof(buf));
    }; // end of loop
  }
  catch(const EOracleError &E)
  {
    ProgError(STDLOG,"EOracleError %d: %s\nSQL: %s)",E.Code,E.what(),E.SQLText());
  }
  catch(const std::exception &E)
  {
    ProgError(STDLOG,"std::exception: %s",E.what());
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };
  try
  {
    ASTRA::rollback();
    OraSession.LogOff();
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };
  return 0;
}

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
    string flight = f->flt_info.toString();
    if (f==flts.begin())
      only_trace?ProgTrace(TRACE0, "Flights: %s", flight.c_str()):
                 ProgError(STDLOG, "Flights: %s", flight.c_str());
    else
      only_trace?ProgTrace(TRACE0, "         %s", flight.c_str()):
                 ProgError(STDLOG, "         %s", flight.c_str());
  };

  std::string err_msg = E.what();
  if (err_msg.empty())
  {
    LogError(STDLOG) << "E.what() empty before errorTypeB";
    err_msg = "UNKNOWN ERROR";
  }

  if (tlge!=NULL)
    errorTypeB(tlg_id, part_no, error_no, tlge->error_pos(), tlge->error_len(), err_msg);
  else
    errorTypeB(tlg_id, part_no, error_no, NoExists, NoExists, err_msg);
};

void bindTypeB(int typeb_tlg_id, const TFlightsForBind &flts, ETlgErrorType error_type)
{
  for(TFlightsForBind::const_iterator f=flts.begin(); f!=flts.end(); ++f)
  {
    try
    {
      TFltInfo normFlt(f->flt_info);
      NormalizeFltInfo(normFlt);
      SaveFlt(typeb_tlg_id, normFlt, f->bind_type, f->dateFlags, error_type);
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

  static DB::TQuery TlgQry(PgOra::getROSession("TLG_QUEUE"));
  if (TlgQry.SQLText.empty())
  {
    //�������� ���冷� ��ꥤ������ ⠡��� �����!
    TlgQry.SQLText=
      "SELECT tlg_queue.id,tlg_queue.time,ttl, "
      "       tlg_queue.tlg_num,tlg_queue.sender, "
      "       COALESCE(tlg_queue.proc_attempt,0) AS proc_attempt "
      "FROM tlg_queue "
      "WHERE tlg_queue.receiver=:receiver AND "
      "      tlg_queue.type='INB' AND tlg_queue.status='PUT' "
      "ORDER BY tlg_queue.time,tlg_queue.id";
    TlgQry.CreateVariable("receiver",otString,OWN_CANON_NAME());

  };

    int count = 0;
  TlgQry.Execute();
  try
  {
    for (;!TlgQry.Eof&&count<HANDLER_PROC_COUNT();count++,TlgQry.Next(),ASTRA::commit())
    {
        tlg_info tlgi = {};
        tlgi.fromDB(TlgQry);
        if (tlgi.ttlExpired()) errorTlg(tlgi.id,"TTL");

        prehandle_tpb_tlg(tlgi);
        handle_tpb_tlg(tlgi);
    }
  }
  catch(...)
  {
      ProgError(STDLOG, "Unknown error");
      throw;
  };

  queue_not_empty=!TlgQry.Eof;

  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! scan_tlg execute time: %ld secs, count=%d",
                     time_end-time_start,count);
  return queue_not_empty;
}

void parse_and_handle_tpb_tlg(const tlg_info &tlg)
{
    tlg_info tlg2(tlg);
    prehandle_tpb_tlg(tlg2);
    handle_tpb_tlg(tlg2);
    parse_tlg(all_other_handler_id);
}

void prehandle_tpb_tlg(tlg_info &tlg)
{
  std::regex re("^(([^\\n\\r]+(\\n|\\r\\n)){1,8}\\.\\w{7}[^\\n\\r]*(\\n|\\r\\n))PDM(\\n|\\r\\n)");
  tlg.text=std::regex_replace(tlg.text, re, "$1");
}

void set_tlg_part_no_tlgs_id(const int id, const int part_no, const int tlg_id)
{
  make_db_curs(
     "UPDATE tlgs SET typeb_tlg_id=:id, typeb_tlg_num=:part_no WHERE id=:tlgs_id",
      PgOra::getRWSession("TLGS"))
     .bind(":id", id)
     .bind(":part_no", part_no)
     .bind(":tlgs_id", tlg_id)
     .exec();
}

static void ins_typeb_in(int id, const std::string& proc_name)
{
    make_db_curs(
"insert into TYPEB_IN(ID, PROC_ATTEMPT, PROC_NAME) VALUES(:id, 0, :proc_name)",
                PgOra::getRWSession("TYPEB_IN"))
            .bind(":id",        id)
            .bind(":proc_name", proc_name)
            .exec();
}

static int ins_new_typeb_in(const std::string& proc_name)
{
    int id = PgOra::getSeqNextVal("tlg_in_out__seq");
    ins_typeb_in(id, proc_name);
    return id;
}

static std::string get_typeb_proc_name(const std::string& tlg_type)
{
    auto cur = make_db_curs(
"select PROC_NAME from TYPEB_PARSE_PROCESSES where TLG_TYPE = :tlg_type",
                PgOra::getROSession("TYPEB_PARSE_PROCESSES"));

    std::string proc_name;
    cur
            .def(proc_name)
            .bind(":tlg_type", tlg_type)
            .EXfet();

    if(cur.err() == DbCpp::ResultCode::NoDataFound) {
        return all_other_handler_id;
    }

    return proc_name;
}

static bool tlg_has_time_parse_not_null(int id)
{
    auto cur = make_db_curs(
"select 1 from TLGS_IN where ID = :id and TIME_PARSE is not null",
                PgOra::getROSession("TLGS_IN"));

    cur
            .bind(":id", id)
            .exfet();

    return cur.err() != DbCpp::ResultCode::NoDataFound;
}

static int get_tlgs_in_id(const std::string& tlg_type,
                          const std::string& merge_key,
                          const TDateTime& min_time_create,
                          const TDateTime& max_time_create)
{
    DB::TQuery Qry(PgOra::getROSession("TLGS_IN"));
    Qry.SQLText =
"select ID from TLGS_IN "
"where TYPE = :tlg_type "
"and MERGE_KEY = :merge_key "
"and TIME_CREATE between :min_time_create and :max_time_create "
"fetch first 1 rows only";

    Qry.CreateVariable("tlg_type",        otString, tlg_type);
    Qry.CreateVariable("merge_key",       otString, merge_key);
    Qry.CreateVariable("min_time_create", otDate,   min_time_create);
    Qry.CreateVariable("max_time_create", otDate,   max_time_create);
    Qry.Execute();

    return Qry.RowCount() ? Qry.FieldAsInteger("id") : ASTRA::NoExists;
}

void ins_tlg_in(int id, int part_no, const std::string& tlg_type,
                const std::string& addr, const std::string& heading, const std::string& ending,
                int is_final_part, const std::string& merge_key,
                const TDateTime& time_create,
                const TDateTime& time_receive,
                const TDateTime& time_parse,
                const TDateTime& time_receive_not_parse)
{
    DB::TQuery Qry(PgOra::getRWSession("TLGS_IN"));
    Qry.SQLText =
"insert into TLGS_IN(ID, NUM, TYPE, ADDR, HEADING, ENDING, IS_FINAL_PART, "
"                    MERGE_KEY, TIME_CREATE, TIME_RECEIVE, TIME_PARSE, TIME_RECEIVE_NOT_PARSE) "
"values(:id, :part_no, :tlg_type, :addr, :heading, :ending, :is_final_part, "
"       :merge_key, :time_create, :time_receive, :time_parse, :time_receive_not_parse)";

    Qry.CreateVariable("id",            otInteger, id);
    Qry.CreateVariable("part_no",       otInteger, part_no);
    Qry.CreateVariable("tlg_type",       otString, tlg_type);
    Qry.CreateVariable("addr",           otString, addr);
    Qry.CreateVariable("heading",        otString, heading);
    Qry.CreateVariable("ending",         otString, ending);
    Qry.CreateVariable("is_final_part", otInteger, is_final_part);
    Qry.CreateVariable("merge_key",      otString, merge_key);

    if(time_create != 0 && time_create != ASTRA::NoExists) {
        Qry.CreateVariable("time_create", otDate, time_create);
    } else {
        Qry.CreateVariable("time_create", otDate, FNull);
    }

    if(time_receive != 0 && time_receive != ASTRA::NoExists) {
        Qry.CreateVariable("time_receive", otDate, time_receive);
    } else {
        Qry.CreateVariable("time_receive", otDate, FNull);
    }

    if(time_parse != 0 && time_parse != ASTRA::NoExists) {
        Qry.CreateVariable("time_parse", otDate, time_parse);
    } else {
        Qry.CreateVariable("time_parse", otDate, FNull);
    }

    if(time_receive_not_parse != 0 && time_receive_not_parse != ASTRA::NoExists) {
        Qry.CreateVariable("time_receive_not_parse", otDate, time_receive_not_parse);
    } else {
        Qry.CreateVariable("time_receive_not_parse", otDate, FNull);
    }
    Qry.Execute();
}

static void upd_typeb_history(int id, int tlgs_id)
{
    make_db_curs(
"update TYPEB_IN_HISTORY set TLG_ID = :id where ID = :tlgs_id",
                PgOra::getRWSession("TYPEB_IN_HISTORY"))
            .bind(":id",      id)
            .bind(":tlgs_id", tlgs_id)
            .exec();
}

void handle_tpb_tlg(const tlg_info &tlg)
{
    LogTlg() << "| TNUM: " << tlg.id
             << " | GATEWAYNUM: " << tlg.tlgNumStr()
             << " | DIR: " << "INB"
             << " | ROUTER: " << tlg.sender
             << " | TSTAMP: " << boost::posix_time::second_clock::local_time() << "\n"
             << tlg.text;

    TMemoryManager mem(STDLOG);

    string socket_name;
    TTlgPartsText parts;
    THeadingInfo *HeadingInfo=NULL;
    TEndingInfo *EndingInfo=NULL;

    ProgTrace(TRACE1,"========= %d TLG: START HANDLE =============",tlg.id);
    ProgTrace(TRACE1,"========= (sender=%s tlg_num=%s) =============",
              tlg.sender.c_str(), tlg.tlgNumStr().c_str());

  //�஢�ਬ TTL

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

  if (tlg.proc_attempt>=HANDLER_PROC_ATTEMPTS())
  {
    ProgTrace(TRACE5, "handle_tlg: tlg_id=%d proc_attempt=%d", tlg.id, tlg.proc_attempt);
    errorTlg(tlg.id,"PROC");
  }
  else
  try
  {
    procTlg(tlg.id);
    ASTRA::commit();

    int typeb_tlg_id=NoExists;
    int typeb_tlg_num=1;    
    std::string typeb_tlg_type = "";
    ostringstream merge_key;

    list<ETlgError> errors;

    try
    {
      GetParts(tlg.text.c_str(), parts, HeadingInfo, bind_flts, mem);
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

      if(HeadingInfo && HeadingInfo->tlg_type) {
        typeb_tlg_type=std::string(HeadingInfo->tlg_type);
      }

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
          putUTG(tlg.id, typeb_tlg_num, info.tlg_type, fltInfo, tlg.text, extra);
        }
        else
        {
          TBSMHeadingInfo &info = *dynamic_cast<TBSMHeadingInfo*>(HeadingInfo);
          typeb_tlg_num=info.part_no;
          merge_key << " " << info.airp;
          if (!info.reference_number.empty())
            merge_key << " " << info.reference_number;
        };

        //TlgIdQry.SetVariable("id",FNull);
        //TlgIdQry.SetVariable("tlg_type",HeadingInfo->tlg_type);
        //TlgIdQry.SetVariable("merge_key",merge_key.str());
        TDateTime min_time_create = ASTRA::NoExists;
        TDateTime max_time_create = ASTRA::NoExists;

        if (strcmp(HeadingInfo->tlg_type,"PNL")==0)
        {
            min_time_create = HeadingInfo->time_create-2.0/1440;
            max_time_create = HeadingInfo->time_create+2.0/1440;
//          TlgIdQry.SetVariable("min_time_create",HeadingInfo->time_create-2.0/1440);
//          TlgIdQry.SetVariable("max_time_create",HeadingInfo->time_create+2.0/1440);
        }
        else
        {
          if (strcmp(HeadingInfo->tlg_type,"ADL")==0 && association_number_exists)
          {
              min_time_create = HeadingInfo->time_create-5.0/1440;
              max_time_create = HeadingInfo->time_create+5.0/1440;
//            TlgIdQry.SetVariable("min_time_create",HeadingInfo->time_create-5.0/1440);
//            TlgIdQry.SetVariable("max_time_create",HeadingInfo->time_create+5.0/1440);
          }
          else
          {
              min_time_create = HeadingInfo->time_create;
              max_time_create = HeadingInfo->time_create;
//            TlgIdQry.SetVariable("min_time_create",HeadingInfo->time_create);
//            TlgIdQry.SetVariable("max_time_create",HeadingInfo->time_create);
          };
        };
//        TlgIdQry.Execute();
//        if (!TlgIdQry.VariableIsNULL("id"))
//          typeb_tlg_id=TlgIdQry.GetVariableAsInteger("id");
        typeb_tlg_id = get_tlgs_in_id(typeb_tlg_type, merge_key.str(),
                                      min_time_create, max_time_create);
      };
    }
    catch(const ETlgError &E)
    {
      errors.push_back(E);
      parts.clear();
      parts.body=tlg.text;
    };

//    InsQry.get().SetVariable("part_no",typeb_tlg_num);
//    InsQry.get().SetVariable("merge_key",merge_key.str());

    TDateTime now_utc = NowUTC();
    TDateTime time_receive = now_utc;
    TDateTime time_parse = ASTRA::NoExists;
    TDateTime time_receive_not_parse = now_utc;

//    if (HeadingInfo!=NULL)
//    {
//      InsQry.get().SetVariable("tlg_type",HeadingInfo->tlg_type);
//      if (HeadingInfo->time_create!=0)
//        InsQry.get().SetVariable("time_create",HeadingInfo->time_create);
//      else
//        InsQry.get().SetVariable("time_create",FNull);
//    }
//    else
//    {
//      InsQry.get().SetVariable("tlg_type",FNull);
//      InsQry.get().SetVariable("time_create",FNull);
//    }

//    InsQry.get().SetVariable("addr", parts.addr);
//    InsQry.get().SetVariable("heading", parts.heading);
//    InsQry.get().SetVariable("ending", parts.ending);

//    if (EndingInfo!=NULL)
//      InsQry.get().SetVariable("is_final_part", (int)EndingInfo->pr_final_part);
//    else
//      InsQry.get().SetVariable("is_final_part", (int)false);

//    InsQry.get().SetVariable("tlgs_id", tlg.id);

    if (deleteTlg(tlg.id))
    {
      std::string proc_name = get_typeb_proc_name(typeb_tlg_type);
      bool insert_typeb=false;
      try
      {
        if (typeb_tlg_id != NoExists) {
            procTypeB(typeb_tlg_id, 0); //��窠
            if(tlg_has_time_parse_not_null(typeb_tlg_id)) {
              time_parse = now_utc;
              time_receive_not_parse = ASTRA::NoExists;
            }
        } else {
            typeb_tlg_id = ins_new_typeb_in(proc_name);
        }

        ins_tlg_in(typeb_tlg_id,
                   typeb_tlg_num,
                   typeb_tlg_type,
                   parts.addr, parts.heading, parts.ending,
                   EndingInfo ? EndingInfo->pr_final_part : 0,
                   merge_key.str(),
                   HeadingInfo ? HeadingInfo->time_create : 0,
                   time_receive,
                   time_parse,
                   time_receive_not_parse);

//        InsQry.get().SetVariable("id",typeb_tlg_id);

//        InsQry.get().Execute();

        upd_typeb_history(typeb_tlg_id, tlg.id);
        set_tlg_part_no_tlgs_id(typeb_tlg_id, typeb_tlg_num, tlg.id);

        socket_name = getSocketName(proc_name);
        insert_typeb=true;
      }
      catch(const EOracleError &E)
      {
        if (E.Code==1)
        {
          if (!errors.empty()) throw Exception("handle_tlg: strange situation");
          if (typeb_tlg_id==NoExists) throw Exception("handle_tlg: strange situation");

          DB::TQuery Qry(PgOra::getRWSession("TLGS_IN"));
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

          typeb_tlg_id = ins_new_typeb_in(proc_name);
//          InsQry.get().SetVariable("id", typeb_tlg_id);
//          InsQry.get().SetVariable("merge_key",FNull);
//          InsQry.get().Execute();

          time_receive = now_utc;
          time_parse = ASTRA::NoExists;
          time_receive_not_parse = now_utc;

          if(tlg_has_time_parse_not_null(typeb_tlg_id)) {
            time_parse = now_utc;
            time_receive_not_parse = ASTRA::NoExists;
          }

          ins_tlg_in(typeb_tlg_id,
                     typeb_tlg_num,
                     typeb_tlg_type,
                     parts.addr, parts.heading, parts.ending,
                     EndingInfo ? EndingInfo->pr_final_part : 0,
                     "",
                     HeadingInfo ? HeadingInfo->time_create : 0,
                     time_receive,
                     time_parse,
                     time_receive_not_parse);

          set_tlg_part_no_tlgs_id(typeb_tlg_id, typeb_tlg_num, tlg.id);

          socket_name = getSocketName(proc_name);
          insert_typeb=true;

          if (tlg.text!=typeb_in_text.str())
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
        //typeb_tlg_id=InsQry.get().VariableIsNULL("id")?NoExists:InsQry.get().GetVariableAsInteger("id");
        if (typeb_tlg_id==NoExists) throw Exception("handle_tlg: strange situation");
        //string typeb_tlg_type=InsQry.get().GetVariableAsString("tlg_type");
        putTypeBBody(typeb_tlg_id, typeb_tlg_num, parts.body);

        if (!errors.empty())
        {
          ETlgErrorType etype=tlgeNotError;
          ostringstream err_msg;
          for(list<ETlgError>::const_iterator e=errors.begin(); e!=errors.end(); ++e)
          {
            if(not err_msg.str().empty()) err_msg << std::endl;
            err_msg << e->what();
            progError(typeb_tlg_id, typeb_tlg_num, error_no, *e, "", bind_flts);  //��� �� ��������, �⮡� ��।������ tlg_type
            if (etype < e->error_type()) etype=e->error_type();
          };
          errorTlg(tlg.id,"PARS");
          parseTypeB(typeb_tlg_id);
          bindTypeB(typeb_tlg_id, bind_flts, etype);
          TypeBHelpMng::notify_msg(typeb_tlg_id, err_msg.str()); // �⢥訢��� �����, �᫨ ����.
        }
        else
        {
          bindTypeB(typeb_tlg_id, bind_flts, tlgeNotError); //�ਢ�뢠�� �⮡� ��⮬ �������� ᤥ���� forwarding
        }

        forwardTypeB(typeb_tlg_id, typeb_tlg_num, typeb_tlg_type);
      }
    }
  }
  catch(const std::exception &E)
  {
    ASTRA::rollback();
    try
    {
      if (isIgnoredEOracleError(E)) return;
      progError(tlg.id, NoExists, error_no, E, "", bind_flts);  //��� �� ��������, �⮡� ��।������ tlg_type
      errorTlg(tlg.id,"PARS",E.what());
    }
    catch(...) {};
#ifdef XP_TESTING
    if (inTestMode())
      throw;
#endif
  }
  catch(...)
  {
      mem.destroy(HeadingInfo, STDLOG);
      if (HeadingInfo!=NULL) delete HeadingInfo;
      mem.destroy(EndingInfo, STDLOG);
      if (EndingInfo!=NULL) delete EndingInfo;
      throw;
  }

  mem.destroy(HeadingInfo, STDLOG);
  if (HeadingInfo!=NULL) delete HeadingInfo;
  mem.destroy(EndingInfo, STDLOG);
  if (EndingInfo!=NULL) delete EndingInfo;

  ASTRA::commit();

  ProgTrace(TRACE1,"========= %d TLG: DONE HANDLE =============",tlg.id);
  ProgTrace(TRACE5, "IN: PUT->DONE (sender=%s, tlg_num=%s, time=%.10f)",
                    tlg.sender.c_str(),
                    tlg.tlgNumStr().c_str(),
                    NowUTC());

  if (not socket_name.empty()) sendCmd(socket_name.c_str(),"H");

  monitor_idle_zapr_type(1, QUEPOT_TLG_AIR);
}


#define PARTS_NOT_RECEIVE_TIMEOUT  5.0/1440  //5 ���
#define PARSING_FORCE_TIMEOUT      1.0/1440  //1 ���
#define PARSING_MAX_TIMEOUT        0         //����� �� �ମ��� ࠧ���騪
#define SCAN_TIMEOUT               60.0/1440 //1 ��

bool parse_tlg(const string &handler_id)
{
  bool queue_not_empty=false;

  time_t time_start=time(NULL);

  TMemoryManager mem(STDLOG);

  TDateTime utc_date=NowUTC();

  DB::TQuery TlgIdQry(PgOra::getROSession({"TLGS_IN", "TYPEB_IN"}));
  if (handler_id==all_other_handler_id)
        TlgIdQry.SQLText=
            "SELECT tlgs_in.id, "
            "       MAX(time_receive) AS time_receive, "
            "       MAX(time_create) AS max_time_create, "
            "       MIN(time_receive) AS min_time_receive, "
            "       MIN(typeb_in.proc_attempt) AS proc_attempt "
            "FROM tlgs_in, typeb_in "
            "WHERE tlgs_in.id = typeb_in.id AND "
            "      time_receive_not_parse>=:time_receive and "
            "      (proc_name = :handler_id or proc_name is null) "
            "GROUP BY tlgs_in.id "
            "ORDER BY max_time_create,min_time_receive,tlgs_in.id";
  else
        TlgIdQry.SQLText=
            "SELECT tlgs_in.id, "
            "       MAX(time_receive) AS time_receive, "
            "       MAX(time_create) AS max_time_create, "
            "       MIN(time_receive) AS min_time_receive, "
            "       MIN(typeb_in.proc_attempt) AS proc_attempt "
            "FROM tlgs_in, typeb_in "
            "WHERE tlgs_in.id = typeb_in.id AND "
            "      time_receive_not_parse>=:time_receive and "
            "      proc_name = :handler_id "
            "GROUP BY tlgs_in.id "
            "ORDER BY max_time_create,min_time_receive,tlgs_in.id";
  TlgIdQry.CreateVariable("time_receive",otDate,utc_date-SCAN_TIMEOUT);
  TlgIdQry.CreateVariable("handler_id",otString,handler_id);

  DB::TQuery TlgInQry(PgOra::getROSession("TLGS_IN"));
  TlgInQry.SQLText=
    "SELECT id,num,type,addr,heading,ending "
    "FROM tlgs_in "
    "WHERE id=:id "
    "ORDER BY num DESC";
  TlgInQry.DeclareVariable("id",otInteger);

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
    for(;!TlgIdQry.Eof&&count<PARSER_PROC_COUNT();TlgIdQry.Next(),ASTRA::rollback())
    {
      int tlg_id=TlgIdQry.FieldAsInteger("id");
      int error_no=NoExists;
      if (TlgIdQry.FieldAsInteger("proc_attempt")>=HANDLER_PROC_ATTEMPTS())
      {
        ProgTrace(TRACE5, "parse_tlg: tlg_id=%d proc_attempt=%d", tlg_id, TlgIdQry.FieldAsInteger("proc_attempt"));
        Exception E("%d attempts to parse previously failed", TlgIdQry.FieldAsInteger("proc_attempt"));
        progError(tlg_id, NoExists, error_no, E, "", TFlightsForBind());
        parseTypeB(tlg_id);
        ASTRA::commit();
        count++;
        continue;
      };
      procTypeB(tlg_id, 1);
      ASTRA::commit();

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
      catch(const std::exception &E)
      {
        count++;
        if (isIgnoredEOracleError(E)) continue;
        progError(tlg_id, tlg_num, error_no, E, tlg_type, bind_flts);
        parseTypeB(tlg_id);
        bindTypeB(tlg_id, bind_flts, E);
        ASTRA::commit();
#ifdef XP_TESTING
        if(inTestMode())
          throw;
#endif
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
          ASTRA::commit();
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
          ASTRA::commit();
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
                callPostHooksBefore();
                ASTRA::commit();
                count++;
                callPostHooksAfter();
                emptyHookTables();
              }
              else
              {
                ASTRA::rollback();
                if (forcibly&& /*info.flt.scd<=utc_date-10*/
                      (utc_date-time_receive) > PARSING_MAX_TIMEOUT)
                  //�᫨ ⥫��ࠬ�� �� ���� �ਭ㤨⥫쭮 ࠧ�������
                  //�� ���祭�� �����ண� �६��� - ������� � ����祭��
                  throw ETlgError(tlgeNotMonitorYesAlarm, "Time limit reached");
                procTypeB(tlg_id, -1);
                ASTRA::commit();
              };
            };
            if (strcmp(info.tlg_type,"PRL")==0)
            {
              TPNLADLPRLContent con;
              ParsePNLADLPRLContent(part,info,con);
              SavePNLADLPRLContent(tlg_id,info,con,true);
              parseTypeB(tlg_id);
              callPostHooksBefore();
              ASTRA::commit();
              count++;
              callPostHooksAfter();
              emptyHookTables();
            };
            if (strcmp(info.tlg_type,"PTM")==0)
            {
              TPtmContent con;
              ParsePTMContent(part,info,con);
              SavePTMContent(tlg_id,info,con);
              parseTypeB(tlg_id);
              callPostHooksBefore();
              ASTRA::commit();
              count++;
              callPostHooksAfter();
              emptyHookTables();
            };
            if (strcmp(info.tlg_type,"SOM")==0)
            {
              TSOMContent con;
              ParseSOMContent(part,info,con);
              SaveSOMContent(tlg_id,info,con);
              parseTypeB(tlg_id);
              callPostHooksBefore();
              ASTRA::commit();
              count++;
              callPostHooksAfter();
              emptyHookTables();
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
              callPostHooksBefore();
              ASTRA::commit();
              count++;
              callPostHooksAfter();
              emptyHookTables();
            };
            break;
          }
          case tcAHM:
          {
            TAHMHeadingInfo &info = *(dynamic_cast<TAHMHeadingInfo*>(HeadingInfo));

            if((string)info.tlg_type == "UWS") {
                UWSParser::TUWSContent con;
                UWSParser::ParseUWSContent(part, info, con, mem);
                UWSParser::SaveUWSContent(tlg_id, info, con);
            } else if((string)info.tlg_type == "MVT") {
                MVTParser::TMVTContent con;
                MVTParser::ParseMVTContent(part, info, con, mem);
                MVTParser::SaveMVTContent(tlg_id, info, con);
            } else {
                ParseAHMFltInfo(part,info,info.flt,info.bind_type);
                SaveFlt(tlg_id, info.flt, info.bind_type);
            }
            parseTypeB(tlg_id);
            callPostHooksBefore();
            ASTRA::commit();
            count++;
            callPostHooksAfter();
            emptyHookTables();
            break;
          }
          case tcUCM:
          case tcCPM:
          case tcSLS:
          case tcLDM:
          case tcNTM:
          case tcLOADSHEET:
          {
              TUCMHeadingInfo &info = *(dynamic_cast<TUCMHeadingInfo*>(HeadingInfo));
              if(HeadingInfo->tlg_cat == tcLDM) {
                  LDMParser::TLDMContent con;
                  LDMParser::ParseLDMContent(part, info, con, mem);
                  LDMParser::SaveLDMContent(tlg_id, info, con);
              } else
                  SaveFlt(tlg_id, info.flt_info.toFltInfo(), btFirstSeg);
              parseTypeB(tlg_id);
              callPostHooksBefore();
              ASTRA::commit();
              count++;
              callPostHooksAfter();
              emptyHookTables();
              break;
          }
          case tcLCI:
          {
            TLCIHeadingInfo &info = *(dynamic_cast<TLCIHeadingInfo*>(HeadingInfo));
            TLCIContent con;
            ParseLCIContent(part,info,con,mem);
            SaveLCIContent(tlg_id,time_receive,info,con);
            parseTypeB(tlg_id);
            callPostHooksBefore();
            ASTRA::commit();
            count++;
            callPostHooksAfter();
            emptyHookTables();
            break;
          }
          case tcIFM:
          {
            LogTrace(TRACE3) << "Enter to IFM handler";
            std::string tlgBody = parts.addr + parts.heading + parts.body;
            LogTrace(TRACE3) << "IFM body:\n" << tlgBody;

            initRsc(tlg_id, tlgBody); // remote system context

            HandleTypebIfm::handle(typeb_parser::TypeBMessage::parse(tlgBody));
            callPostHooksBefore();
            ASTRA::commit();
            count++;
            callPostHooksAfter();
            emptyHookTables();
            break;
          }
          case tcSSM:
          {
            std::string tlgBody = parts.addr + parts.heading + parts.body;
            HandleSSMTlg(tlgBody, tlg_id, bind_flts);
            parseTypeB(tlg_id);
            callPostHooksBefore();
            ASTRA::commit();
            count++;
            callPostHooksAfter();
            emptyHookTables();
            break;
          }
          default:
          {
            //⥫��ࠬ�� �������⭮�� ⨯� �ࠧ� ��襬 � ࠧ��࠭��
            parseTypeB(tlg_id);
            callPostHooksBefore();
            ASTRA::commit();
            count++;
            callPostHooksAfter();
            emptyHookTables();
          };
        };
      }
      catch(const TlgHandling::TlgToBePostponed& e)
      {
          LogTrace(TRACE1) << "Tlg " << e.tlgNum() << " to be postponed";
          callPostHooksBefore();
          ASTRA::commit();
          callPostHooksAfter();
          emptyHookTables();
      }
      catch(const std::exception &E)
      {
        count++;
        ASTRA::rollback();
        try
        {
          if (isIgnoredEOracleError(E)) continue;
          progError(tlg_id, NoExists, error_no, E, tlg_type, bind_flts);
          parseTypeB(tlg_id);
          bindTypeB(tlg_id, bind_flts, E);
          TypeBHelpMng::notify_msg(tlg_id, E.what()); // �⢥訢��� �����, �᫨ ����.
          ASTRA::commit();
        }
        catch(...) {};
#ifdef XP_TESTING
        if (inTestMode())
          throw;
#endif
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
}

int LCI_ACT_TIMEOUT()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("LCI_ACT_TIMEOUT",NoExists,NoExists,2);
  return VAR;
};

int LCI_EST_TIMEOUT()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("LCI_EST_TIMEOUT",NoExists,NoExists,48);
  return VAR;
};

void check_timeouts(const set<int> &spp_point_ids)
{
    if(spp_point_ids.empty())
        throw TypeB::ETlgError(TypeB::tlgeNotMonitorYesAlarm, "Flight not found");
    map<int, TTripInfo> trips;
    TDateTime time_receive = NowUTC();
    TNearestDate nd(time_receive);
    for(const auto &i: spp_point_ids) {
        trips[i].getByPointId(i);
        nd.sorted_points[trips[i].est_scd_out()] = i;
    }
    int point_id_spp = nd.get();
    TDateTime dep_time;
    int timeout;
    if(trips[point_id_spp].act_out_exists()) {
        dep_time = trips[point_id_spp].act_est_scd_out();
        timeout = LCI_ACT_TIMEOUT();
    } else {
        dep_time = trips[point_id_spp].est_scd_out();
        timeout = LCI_EST_TIMEOUT();
    }
    if((time_receive - dep_time) > timeout / 24.)
        throw TypeB::ETlgError(TypeB::tlgeNotMonitorYesAlarm, "Flight has departed");
}

void get_tlg_info(
        const string &tlg_text,
        string &tlg_type,
        string &airline,
        string &airp
        )
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
        LogTrace(TRACE5) << "tlg_type: " << tlg_type;

        part.p=parts.body.c_str();
        part.EOL_count=CalcEOLCount(parts.addr.c_str())+
            CalcEOLCount(parts.heading.c_str());
        part.offset=parts.addr.size()+
            parts.heading.size();

        switch (HeadingInfo->tlg_cat)
        {
            case tcDCS:
                {
                    TDCSHeadingInfo &info = *(dynamic_cast<TDCSHeadingInfo*>(HeadingInfo));
                    airline = info.flt.airline;
                    airp = info.flt.airp_dep;
                    break;
                }
            case tcLCI:
                {
                    TLCIHeadingInfo &info = *(dynamic_cast<TLCIHeadingInfo*>(HeadingInfo));

                    vector<int> spp_point_ids;
                    TTlgBinding(false).bind_flt(info.flt_info.toFltInfo(),btFirstSeg,spp_point_ids);
                    set<int> _spp_point_ids(spp_point_ids.begin(), spp_point_ids.end()); // remove duplicates
                    check_timeouts(_spp_point_ids);

                    airline = info.flt_info.flt.airline.c_str();
                    airp = info.flt_info.airp;
                    break;
                }
            case tcUCM:
            case tcCPM:
            case tcSLS:
            case tcLDM:
            case tcNTM:
            case tcLOADSHEET:
                {
                    TUCMHeadingInfo &info = *(dynamic_cast<TUCMHeadingInfo*>(HeadingInfo));
                    airline = info.flt_info.airline;
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
