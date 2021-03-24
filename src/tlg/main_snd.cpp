#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_main.h"
#include "date_time.h"
#include "misc.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include <serverlib/query_runner.h>
#include <serverlib/ourtime.h>
#include <serverlib/timer.h>
#include <libtlg/hth.h>
#include <libtlg/telegrams.h>
#include "db_tquery.h"

#include "PgOraConfig.h"

#define NICKNAME "VLAD"
#include <serverlib/slogger.h>

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

static int WAIT_INTERVAL()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TLG_SND_WAIT_INTERVAL",1,NoExists,60000);
  return VAR;
};

static int PROC_INTERVAL()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TLG_SND_PROC_INTERVAL",0,10000,50);
  return VAR;
};

static int TLG_ACK_TIMEOUT()     //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TLG_ACK_TIMEOUT",1,10000,50);
  return VAR;
};

static int PROC_COUNT()          //кол-во посылаемых телеграмм за одно сканирование по каждому шлюзу
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TLG_SND_PROC_COUNT",1,10,10);
  return VAR;
};

static int TLG_STEP_BY_STEP_TIMEOUT()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TLG_STEP_BY_STEP_TIMEOUT",0,NoExists,5000);
  return VAR;
};

static int PUT_TIMEOUT()          //кол-во посылаемых телеграмм за одно сканирование по каждому шлюзу
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TLG_SND_PUT_TIMEOUT",60,NoExists,120);   //сколько минут могут максимально существовать телеграммы в статусе PUT
  return VAR;
};

static int ROT_UPDATE_INTERVAL() //секунды
{
    static int VAR = getTCLParam("ROT_UPDATE_INTERVAL", 0, 600, 60);
    return VAR;
}

static int SCAN_TLG_FETCH_LIMIT()
{
    static int VAR = getTCLParam("SCAN_TLG_FETCH_LIMIT", 1, 10000, 10);
    return VAR;
}

static int sockfd=-1;

static bool scan_tlg(bool sendOutAStepByStep);
//int h2h_out(H2H_MSG *h2h_msg);

sockaddr_in getAddrFromRot(const std::string& canon_name) {
    BaseTables::Router rot(canon_name);

    sockaddr_in to_addr;
    to_addr.sin_family = AF_INET;
    to_addr.sin_port = htons(rot->ipPort());

    if (!inet_aton(rot->ipAddress().data(), &to_addr.sin_addr)) {
        throw Exception("'inet_aton' error %d: %s",errno,strerror(errno));
    }

    return to_addr;
}

static const sockaddr_in& getAddrByName(const std::string& can_name) {
    static std::map<std::string, std::pair<std::time_t, sockaddr_in>> addrMap;
    const auto lb = addrMap.lower_bound(can_name);
    if (end(addrMap) != lb && !(addrMap.key_comp()(can_name, lb->first))) {
        // can_name exists
        if (ROT_UPDATE_INTERVAL() <= std::difftime(std::time(nullptr), lb->second.first)) {
            // тут обновление записи
            lb->second.first = std::time(nullptr);
            lb->second.second = getAddrFromRot(can_name);
        }
        return lb->second.second;
    } else {
        const auto it = addrMap.insert(lb, {can_name, std::make_pair(std::time(nullptr), getAddrFromRot(can_name))});
        return it->second.second;
    }
}

int main_snd_tcl(int supervisorSocket, int argc, char *argv[])
{
  try
  {
    init_tlg_callbacks();
    sleep(2);
    InitLogTime(argc>0?argv[0]:NULL);

    int SND_PORT;
    const char *port_tcl=Tcl_GetVar(getTclInterpretator(),"SND_PORT",TCL_GLOBAL_ONLY);
    if (port_tcl==NULL||StrToInt(port_tcl,SND_PORT)==EOF)
      throw Exception("Unknown or wrong SND_PORT");

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
      ->connect_db();

    if ((sockfd=socket(AF_INET,SOCK_DGRAM,0))==-1)
      throw Exception("'socket' error %d: %s",errno,strerror(errno));
    sockaddr_in adr{};
    adr.sin_family=AF_INET;
    adr.sin_addr.s_addr=/*htonl(*/INADDR_ANY/*)*/; //???
    adr.sin_port=htons(SND_PORT);

    if (bind(sockfd,(struct sockaddr*)&adr,sizeof(adr))==-1)
      throw Exception("'bind' error %d: %s",errno,strerror(errno));

    char buf[10];
    bool receivedCmdTlgSndStepByStep=false;
    TDateTime lastSendOutAStepByStep=NoExists;
    for (;;)
    {
      InitLogTime(argc>0?argv[0]:NULL);
      bool sendOutAStepByStep=receivedCmdTlgSndStepByStep ||
                              lastSendOutAStepByStep==NoExists ||
                              (lastSendOutAStepByStep<NowUTC()-((double)TLG_STEP_BY_STEP_TIMEOUT())/MSecsPerDay);

      bool queue_not_empty=scan_tlg(sendOutAStepByStep);
      if (sendOutAStepByStep) lastSendOutAStepByStep=NowUTC();

      *buf=0;
      waitCmd("CMD_TLG_SND",queue_not_empty?PROC_INTERVAL():WAIT_INTERVAL(),buf,sizeof(buf));
      receivedCmdTlgSndStepByStep=(strchr(buf,'S')!=NULL); //true только тогда когда пришел ответ на предыдущую OutAStepByStep
    }; // end of loop
  }
  catch(EOracleError &E)
  {
    ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"std::exception: %s",E.what());
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };

  if (sockfd!=-1) close(sockfd);
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
};

void updateTlgQueRenewLastSend(TDateTime time, int id)
{
    static auto& session = PgOra::getRWSession("TLG_QUEUE");
    make_db_curs(
       "UPDATE tlg_queue "
          "SET last_send = :last_send "
       "WHERE  id = :id "
          "AND type IN ('OUTA','OUTB','OAPP') "
          "AND status = 'PUT'",
        session)
       .stb()
       .bind(":last_send", time)
       .bind(":id", id)
       .exec();
}

bool scan_tlg(bool sendOutAStepByStep)
{
  time_t time_start=time(NULL);

  static DB::TQuery TlgQry(PgOra::getRWSession("TLG_QUEUE"));
  if (TlgQry.SQLText.empty())
  {
    if (PgOra::supportsPg("TLG_QUEUE")) {
        TlgQry.SQLText =
           "SELECT /*+ IndexScan(tlg_queue tlg_queue__idx2) */ "
                    "id, tlg_num, sender, receiver, priority, "
                    "time, last_send, ttl "
           "FROM     tlg_queue "
           "WHERE    type IN ('OUTA','OUTB','OAPP') AND status='PUT' "
           "ORDER BY priority, time_msec, tlg_num "
           "LIMIT    :limit";
        TlgQry.CreateVariable("limit", otInteger, SCAN_TLG_FETCH_LIMIT());
    } else {
        TlgQry.SQLText =
           "SELECT /*+ INDEX(tlg_queue tlg_queue__idx2) */ "
                    "id, tlg_num, sender, receiver, priority, "
                    "time, last_send, ttl "
           "FROM     tlg_queue "
           "WHERE    type IN ('OUTA','OUTB','OAPP') AND status='PUT' "
           "ORDER BY priority, time_msec, tlg_num";
    }
  }

  struct addrComparator {
    bool operator() (const sockaddr_in& lhs, const sockaddr_in& rhs) const {
      return std::tie(lhs.sin_addr.s_addr, lhs.sin_port) < std::tie(rhs.sin_addr.s_addr, rhs.sin_port);
    }
  };
  std::map<sockaddr_in, int, addrComparator> addrCount;

  //считаем все телеграммы, которые еще не отправлены
  AIRSRV_MSG tlg_out{};
  size_t len = 0;
  int ttl = 0;
  uint16_t ttl16 = 0;

  int trace_count=0;
  int firstStepByStepTlgId=ASTRA::NoExists;
  TlgQry.Execute();
  bool result=true;
  if (!TlgQry.Eof)
  {
  try
  {
    for(;!TlgQry.Eof;trace_count++,TlgQry.Next(),ASTRA::commit())
    {
      if (TlgQry.FieldAsString("sender") != OWN_CANON_NAME()) {
        //не добавлять критерий "tlg_queue.sender=:sender" в TlgQry - плохой план разбора
        continue;
      }
      int tlg_id=TlgQry.FieldAsInteger("id");
      int priority=TlgQry.FieldAsInteger("priority");
      try
      {
        if ((NowUTC()-TlgQry.FieldAsDateTime("time"))*1440>PUT_TIMEOUT())
        {
          //удаляем устаревшие
          errorTlg(tlg_id,"TIME");
          continue;
        }

        if (priority==(int)qpOutAStepByStep)
        {
          if (!sendOutAStepByStep ||
              (firstStepByStepTlgId!=ASTRA::NoExists && tlg_id!=firstStepByStepTlgId)) throw 1;

          firstStepByStepTlgId=tlg_id;
        }

        if (TlgQry.FieldIsNULL("receiver")
         || TlgQry.FieldIsNULL("sender")) {
          throw Exception("Unknown receiver %s", TlgQry.FieldAsString("receiver"));
        }

        const sockaddr_in addr_to = getAddrByName(TlgQry.FieldAsString("receiver"));

        const auto iCount = addrCount.find(addr_to);

        if (iCount!=addrCount.end() && iCount->second>=PROC_COUNT()) continue;

        tlg_out.num=htonl(TlgQry.FieldAsInteger("tlg_num"));
        tlg_out.type=htons(TLG_OUT);
        tlg_out.Sender[5]=0;
        tlg_out.Receiver[5]=0;
        strncpy(tlg_out.Sender,OWN_CANON_NAME(),5);
        strncpy(tlg_out.Receiver,TlgQry.FieldAsString("receiver").data(),5);
        string text=getTlgText(tlg_id);
        strcpy(tlg_out.body, text.c_str());

        //проверим, надо ли лепить h2h
        boost::optional<hth::HthInfo> hthInfo;
        {
            tlgnum_t tlgNum(boost::lexical_cast<std::string>(tlg_id));
            hth::HthInfo hi = {};
            int ret = telegrams::callbacks()->readHthInfo(tlgNum, hi);
            if (ret == 0) {
                hthInfo = hi;
            }
        }

        if (hthInfo) {
            hth::createTlg(tlg_out.body, hthInfo.get());
        }

        len=strlen(tlg_out.body);
        if (len>sizeof(tlg_out.body)) throw Exception("Telegram too long");

        //проверим TTL
        ttl=0;
        if (!TlgQry.FieldIsNULL("ttl"))
        {
          if (priority!=(int)qpOutAStepByStep)
            ttl=TlgQry.FieldAsInteger("ttl")-
                (int)((NowUTC()-TlgQry.FieldAsDateTime("time"))*SecsPerDay);
          else
            ttl=TlgQry.FieldAsInteger("ttl");
        }
        if (!TlgQry.FieldIsNULL("ttl") && ttl<=0)
        {
            errorTlg(tlg_id,"TTL");
        }
        else
        {
          TDateTime nowUTC=NowUTC();
          TDateTime last_send=0;
          if (!TlgQry.FieldIsNULL("last_send")) last_send=TlgQry.FieldAsFloat("last_send");
          if (last_send<nowUTC-((double)TLG_ACK_TIMEOUT())/MSecsPerDay)
          {
            //таймаут TLG_ACK истек, надо перепослать
            ttl16=ttl;
            tlg_out.TTL=htons(ttl16);

            if (-1 == sendto(sockfd,(char*)&tlg_out,sizeof(tlg_out)-sizeof(tlg_out.body)+len,0,
                       (const sockaddr*)&addr_to,sizeof(sockaddr_in))) {
              throw Exception("'sendto' error %d: %s",errno,strerror(errno));
            }
            monitor_idle_zapr_type(1, QUEPOT_NULL);
            updateTlgQueRenewLastSend(nowUTC, tlg_id);
            ProgTrace(TRACE5,"Attempt %s telegram (sender=%s, tlg_num=%ld, time=%.10f, priority=%d)",
                             last_send==0?"send":"resend",
                             tlg_out.Sender,
                             (unsigned long)ntohl(tlg_out.num),
                             nowUTC,
                             priority);
          };
        };

        if (addrCount.end() != iCount) {
          iCount->second++;
        } else {
          addrCount.insert(iCount, std::make_pair(addr_to, 1));
        }
      }
      catch(EXCEPTIONS::Exception &E)
      {
        ASTRA::rollback();
        try
        {
          if (isIgnoredEOracleError(E)) continue;
          ProgError(STDLOG,"Exception: %s (tlgs.id=%d)",E.what(),tlg_id);
          errorTlg(tlg_id,"SEND",E.what());
        }
        catch(...) {};
      };
    }
  }
  catch(int) {}
  }
  else result=false;

  TlgQry.Close();
  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! scan_tlg execute time: %ld secs, count=%d, sendOutAStepByStep=%d",
                     time_end-time_start,trace_count,(int)sendOutAStepByStep);
  return result;
};

/* Вставляем заголовок IATA Host-to-Host перед телом телеграммы */
/*int h2h_tlg(char *body, char type, char *sndr, char *rcvr, char *tpr, char *err)*/
/* ------------------------------------------------------------------------ */
//int h2h_out(H2H_MSG *h2h_msg)
//{
//  int i=0, head_len, data_len;
//  char h2h_head[MAX_H2H_HEAD_SIZE];

//    if ( !h2h_msg->sndr	||	strlen(h2h_msg->sndr) > 10	||
//         !h2h_msg->rcvr	||	strlen(h2h_msg->rcvr) > 10	||
//         !h2h_msg->tpr	||	strlen(h2h_msg->tpr) > 19	||
//                            strlen(h2h_msg->err) > 2 )
//    {
//        ProgError(STDLOG, "h2h_out(): Can't forming H2H header");
//      return FALSE;
//    }

//  strcpy(h2h_head, H2H_BEG_STR);	/*	V.\rV	*/
//  i += strlen(H2H_BEG_STR);
///*
//  h2h_head[i++] = 0x56;				'V'
//  h2h_head[i++] = 0x2E;				'.'
//  h2h_head[i++] = 0x0D;				'\r'
//  h2h_head[i++] = 0x56;				'V'
//*/
//  h2h_head[i++] = h2h_msg->type;		/*4 QRI1	query / reply /	alone	*/
//#if 0
//  h2h_head[i++] = 0x45;	/* 0x50; */		/*5	QRI2 'E'	hold protection		*/
//#else
//  h2h_head[i++] = 'L';	/* 0x50; */		/*5	QRI2 'E'	hold protection		*/
//#endif
//  h2h_head[i++] = 0x47;					/*6	QRI3 'G'	medium priority		*/
//  h2h_head[i++] = 0x2E;					/*7	QRI4 '.'	no flow control		*/
//  h2h_head[i++] = h2h_msg->qri5;		/*8 QRI5							*/
//  h2h_head[i++] = h2h_msg->qri6;		/*9 QRI6							*/

///*
//  if ( h2h_msg->part == 1 )				8opt	QRI5 'V' First seg
//    h2h_head[i++] = 0x56;
//  else
//  if ( h2h_msg->part && !h2h_msg->end )	8opt	QRI5 'T' Intermediate seg
//    h2h_head[i++] = 0x54;
//  else
//  if ( h2h_msg->part && h2h_msg->end )	8opt	QRI5 'U' End seg
//    h2h_head[i++] = 0x55;
//  else									8opt	QRI5 'W' Only segment
//    h2h_head[i++] = 0x57;

//  if ( h2h_msg->part )					9opt	QRI6 'A'-'Z' seg
//    h2h_head[i++] = (char)(h2h_msg->part + 64);
//*/

//  h2h_head[i++] = 0x2F;					/*10'/'								*/

//  h2h_head[i++] = 0x45;					/*	'E'								*/
//  h2h_head[i++] = 0x35;					/*	'5'								*/
//  strcpy(&h2h_head[i],h2h_msg->sndr);	/*	Sender							*/
//  i += strlen(h2h_msg->sndr);			/*									*/

//  h2h_head[i++] = 0x2F;					/*	'/'								*/

//  h2h_head[i++] = 0x49;					/*	'I'								*/
//  h2h_head[i++] = 0x35;					/*	'5'								*/
//  strcpy(&h2h_head[i], h2h_msg->rcvr);	/*	Receiver						*/
//  i += strlen(h2h_msg->rcvr);			/*									*/

//  h2h_head[i++] = 0x2F;					/*	'/'								*/

//  strcpy(&h2h_head[i++], "P");
//  strcpy(&h2h_head[i], h2h_msg->tpr);	/*	TPR								*/
//  i += strlen(h2h_msg->tpr);			/*									*/
//  if ( *h2h_msg->err )					/*									*/
//  {										/*									*/
//    h2h_head[i++] = 0x2F;				/*	'/'								*/
//    strcpy(&h2h_head[i],h2h_msg->err);	/*									*/
//    i += strlen(h2h_msg->err);			/*									*/
//  }										/*									*/
//  h2h_head[i++] = 0x0D;					/*	'\r'*****************************/

//  h2h_head[i++] = 0x56;			/*	'V'	*********************************/
//  h2h_head[i++] = 0x47;			/*	'G'									*/
//  h2h_head[i++] = 0x59;			/*	'Z'									*/
//  h2h_head[i++] = 0x41;			/*	'A'									*/
//  h2h_head[i++] = 0x0D;			/*	'\r'*********************************/
//#ifdef H2H_STX_ETX
//  h2h_head[i++] = 0x02;			/* STX	*********************************/
//#endif
//  h2h_head[i] =	0x00;

//  head_len = i;

///*#ifdef H2H_STX_ETX*/
//  strcat(h2h_msg->data, "\03");		/*	ETX	*************************/
///*#endif*/
//  data_len = strlen(h2h_msg->data);

////ProgTrace(1, STDLOG, "strlen(h2h_head)=%d, head_len=%d", strlen(h2h_head), i);
//  if ( head_len + data_len > MAX_TLG_LEN - 1 )
//  {
//        ProgError(STDLOG, "h2h_out(): Too much size of H2H tlg");
//    return FALSE;
//  }

//  for ( i = data_len; i >= 0; i-- )
//    h2h_msg->data[i+head_len] = h2h_msg->data[i];

//  memcpy(h2h_msg->data, h2h_head, head_len);

//  return TRUE;
//}


#ifdef XP_TESTING

#include "xp_testing.h"
#include <serverlib/TlgLogger.h>
#include <serverlib/timer.h>
#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/pg_cursctl.h>
#include <serverlib/cursctl.h>
#include <serverlib/ocilocal.h>
#include <serverlib/dbcpp_session.h>
#include <random>

static long long scan_tlg_model_index()
{
    static DB::TQuery TlgQry(PgOra::getROSession("TLG_QUEUE"));

    if (TlgQry.SQLText.empty()) {

        TlgQry.SQLText = PgOra::supportsPg("TLG_QUEUE")

         ? "SELECT /*+ IndexScan(tlg_queue tlg_queue__idx2) */ "
                    "id, tlg_num, sender, receiver, priority, "
                    "time, last_send, ttl "
           "FROM     tlg_queue "
           "WHERE    type IN ('OUTA','OUTB','OAPP') AND status='PUT' "
           "ORDER BY priority, time_msec, tlg_num"

         : "SELECT /*+ INDEX(tlg_queue tlg_queue__idx2) */ "
                     "id, tlg_num, sender, receiver, priority, "
                     "time, last_send, ttl "
           "FROM     tlg_queue "
           "WHERE    type IN ('OUTA','OUTB','OAPP') AND status='PUT' "
           "ORDER BY priority, time_msec, tlg_num";
    }

    long long c = 0;

    for (TlgQry.Execute(); !TlgQry.Eof; TlgQry.Next()) {
        ++c;
    }

    return c;
}

static long long scan_tlg_model_default()
{
    static DB::TQuery TlgQry(PgOra::getROSession("TLG_QUEUE"));

    if (TlgQry.SQLText.empty()) {

        TlgQry.SQLText =

           "SELECT   id, tlg_num, sender, receiver, priority, "
                    "time, last_send, ttl "
           "FROM     tlg_queue "
           "WHERE    type IN ('OUTA','OUTB','OAPP') AND status='PUT' "
           "ORDER BY priority, time_msec, tlg_num";
    }

    long long c = 0;

    for (TlgQry.Execute(); !TlgQry.Eof; TlgQry.Next()) {
        ++c;
    }

    return c;
}

std::vector<std::string> createRots()
{
    std::vector<std::string> rots;

    std::string localhost = "127.0.0.1";
    const int ip_port = 8888;

    for (int i = 0; i < 5; ++i) {
        const int id = PgOra::getSeqNextVal("ID__SEQ");
        const std::string canon_name = "RCVR" + std::to_string(i);

        rots.push_back(canon_name);

        DbCpp::CursCtl cur = make_db_curs(
           "INSERT INTO ROT(  canon_name,  id,  ip_address,  ip_port, loopback,  own_canon_name ) "
           "VALUES         ( :canon_name, :id, :ip_address, :ip_port,        1, :own_canon_name )",
            PgOra::getRWSession("ROT")
        );

        cur.stb()
           .bind(":canon_name", canon_name)
           .bind(":id", id)
           .bind(":ip_address", localhost)
           .bind(":ip_port", ip_port)
           .bind(":own_canon_name", OWN_CANON_NAME())
           .exec();
    }

    return rots;
}


void fillTlgQueue(const long long maxRecords, DbCpp::Session& session)
{
    LogTrace(TRACE6) << __FUNCTION__  << "Filling tlg_queue with " << maxRecords << " records." << std::endl;

    std::vector<std::string> rots = createRots();

    std::srand(0);

    const auto nowUTC    = boost::posix_time::second_clock::universal_time();
    // телеграммы которые находяться более 24 часов в очереди, удаляются в scan_tlg
    // если выставить границу в 48 часов - половина тлг будет созданно уже просроченными.
    const auto timeBound = boost::posix_time::hours(24);

    for (long long i = 0; i < maxRecords; ++i) {
        const std::string& receiver  = rots[rand() % rots.size()];
        const char*        sender    = OWN_CANON_NAME();
        const char*        type      = "OUTA";
        const std::string  text      = "TLG " + std::to_string(i);
        const int          tlgNum    = saveTlg(receiver.data(), sender, type, text, ASTRA::NoExists, ASTRA::NoExists);
        const int          priority  = 4;
        const char*        status    = "PUT";
        const int          time_msec = rand() % 1000;
        const auto         delta     = (timeBound.total_seconds() * (i - maxRecords))/maxRecords;
        const auto         time      = nowUTC + boost::posix_time::seconds(delta);

        DbCpp::CursCtl cur = make_db_curs(
           "INSERT INTO TLG_QUEUE( id,  sender,  tlg_num,  receiver,  type,  priority,  status,  time, ttl,  time_msec) "
           "VALUES               (:id, :sender, :tlg_num, :receiver, :type, :priority, :status, :time,  10, :time_msec)",
            session
        );

        cur.stb()
           .bind(":id", tlgNum)
           .bind(":sender", sender)
           .bind(":tlg_num", tlgNum)
           .bind(":receiver", receiver)
           .bind(":type", type)
           .bind(":priority", priority)
           .bind(":status", status)
           .bind(":time", time)
           .bind(":time_msec", time_msec)
           .exec();

        if (i % 1000 == 999) {
            session.commitInTestMode();
        }
    }
}

void truncate() {
    static auto& queue_session = PgOra::getRWSession("SP_PG_GROUP_TLG_QUE");
    static auto& rot_session = PgOra::getRWSession("ROT");

    make_db_curs("DELETE FROM TLGS_TEXT", queue_session).exec();
    make_db_curs("DELETE FROM TLG_QUEUE", queue_session).exec();
    make_db_curs("DELETE FROM TLGS", queue_session).exec();
    make_db_curs("DELETE FROM ROT", rot_session).exec();
    queue_session.commitInTestMode();
    queue_session.commit();
    rot_session.commitInTestMode();
    rot_session.commit();
}

void test2Body(const char* caption, long long (* func)(), const long long iterations)
{
    long long count = 0;

    HelpCpp::Timer timer;

    for (long long i = 0; i < iterations; ++i) {
        count += func();
    }

    LogTrace(TRACE6) << __FUNCTION__ << " " << caption << ": " << timer << "; count: " << count;
}

START_TEST(benchmark_scan_tlg_query_tquery)
{
    constexpr long long maxRecords = 1000;
    constexpr long long iterations = 100;

    auto& session = PgOra::getRWSession("TLG_QUEUE");

    truncate();

    fillTlgQueue(maxRecords, session);

    test2Body("TQuery - Default request", scan_tlg_model_default, iterations);

    test2Body("TQuery - Force using tlg_queue__idx2", scan_tlg_model_index, iterations);

    truncate();
}
END_TEST;

void testBody(const char* sql, DbCpp::Session& session, const char* caption, const long long iterations)
{
    long long count = 0;

    HelpCpp::Timer timer;

    for (long long i = 0; i < iterations; ++i) {
        DbCpp::CursCtl cur = make_db_curs(sql, session);
        int id;
        int tlg_num;
        std::string sender;
        std::string receiver;
        int priority;
        Dates::DateTime_t time;
        std::string last_send;
        int ttl;

        cur.def(id)
           .def(tlg_num)
           .def(sender)
           .def(receiver)
           .def(priority)
           .def(time)
           .defNull(last_send, "(null)")
           .def(ttl)
           .exec();

        while (!cur.fen()) {
            ++count;
        }
    }
    LogTrace(TRACE6) << __FUNCTION__ << " " << caption << ": " << timer << "; count: " << count;
}

START_TEST(benchmark_scan_tlg_query)
{
    constexpr long long maxRecords = 1000;
    constexpr long long iterations = 100;

    auto& session = PgOra::getRWSession("TLG_QUEUE");

    truncate();

    fillTlgQueue(maxRecords, session);

    try
    {
        {
            const char* caption = "DbCpp::CursCtl - Default request";
            const char* sql =
               "SELECT   id, tlg_num, sender, receiver, priority, "
                        "time, last_send, ttl "
               "FROM     tlg_queue "
               "WHERE    type IN ('OUTA','OUTB','OAPP') AND status='PUT' "
               "ORDER BY priority, time_msec, tlg_num";
            testBody(sql, session, caption, iterations);
        }

        {
            const char* caption = "DbCpp::CursCtl - Force using tlg_queue__idx2";
            const char* sql =
                PgOra::supportsPg("TLG_QUEUE")

                 ? "SELECT /*+ IndexScan (tlg_queue tlg_queue__idx2) */ "
                            "id, tlg_num, sender, receiver, priority, "
                            "time, last_send, ttl "
                   "FROM     tlg_queue "
                   "WHERE    type IN ('OUTA','OUTB','OAPP') AND status='PUT' "
                   "ORDER BY priority, time_msec, tlg_num"

                 : "SELECT /*+ INDEX (tlg_queue tlg_queue__idx2) */ "
                            "id, tlg_num, sender, receiver, priority, "
                            "time, last_send, ttl "
                   "FROM     tlg_queue "
                   "WHERE    type IN ('OUTA','OUTB','OAPP') AND status='PUT' "
                   "ORDER BY priority, time_msec, tlg_num";

            testBody(sql, session, caption, iterations);
        }
    } catch (const std::exception& e) {
        LogTrace(TRACE6) << __FUNCTION__ << ": Exception: " << e.what();
    }

    truncate();
}
END_TEST;

int getNumberOfRecords(DbCpp::Session& session) {
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT COUNT(*) FROM TLG_QUEUE",
        session
    );

    int count = 0;
    cur.stb()
       .def(count)
       .exec();

    if (!cur.fen()) {
        return count;
    }
    return 0;
}

START_TEST(check_scan_tlg)
{
    init_tlg_callbacks();

    int SND_PORT;
    const char *port_tcl=Tcl_GetVar(getTclInterpretator(),"SND_PORT",TCL_GLOBAL_ONLY);
    if (port_tcl==NULL||StrToInt(port_tcl,SND_PORT)==EOF)
      throw Exception("Unknown or wrong SND_PORT");

    if ((sockfd=socket(AF_INET,SOCK_DGRAM,0))==-1)
      throw Exception("'socket' error %d: %s",errno,strerror(errno));
    sockaddr_in adr{};
    adr.sin_family=AF_INET;
    adr.sin_addr.s_addr=/*htonl(*/INADDR_ANY/*)*/; //???
    adr.sin_port=htons(SND_PORT);

    if (bind(sockfd,(struct sockaddr*)&adr,sizeof(adr))==-1)
      throw Exception("'bind' error %d: %s",errno,strerror(errno));

    auto& session = PgOra::getRWSession("SP_PG_GROUP_TLG_QUE");

    constexpr long long maxRecords = 10000;
    constexpr long long iterations = 1000;

    truncate();
    fillTlgQueue(maxRecords, session);

    HelpCpp::Timer timer;
    for (long long i = 0; i < iterations; ++i) {
        scan_tlg(true);
        session.commitInTestMode();
    }

    LogTrace(TRACE6) << __FUNCTION__ << ": " << timer;
    truncate();
}
END_TEST;

#define SUITENAME "tlg_queue"
TCASEREGISTER(testInitDB, testShutDBConnection)
{
    SET_TIMEOUT(3000);
    ADD_TEST(benchmark_scan_tlg_query_tquery);
    ADD_TEST(benchmark_scan_tlg_query);
    ADD_TEST(check_scan_tlg);
}
TCASEFINISH;
#undef SUITENAME

#endif //XP_TESTING
