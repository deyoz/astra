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
#include <libtlg/hth.h>
#include <libtlg/telegrams.h>
#include "db_tquery.h"

#include "PgOraConfig.h"

#define NICKNAME "VLAD"
#include <serverlib/test.h>

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

static int sockfd=-1;

using AddrMap = std::map<pair<std::string, std::string>, sockaddr_in>;

static bool scan_tlg(bool sendOutAStepByStep, const AddrMap& addrMap);
//int h2h_out(H2H_MSG *h2h_msg);

AddrMap getAddrMap()
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT CANON_NAME, OWN_CANON_NAME, IP_ADDRESS, IP_PORT "
       "FROM ROT",
        PgOra::getROSession("ROT")
    );

    std::string receiver; // canon_name
    std::string sender; // own_canon_name
    std::string ip_address;
    int ip_port;

    cur.def(receiver)
       .def(sender)
       .def(ip_address)
       .def(ip_port)
       .exec();

    AddrMap addrMap;

    while (!cur.fen()) {
        sockaddr_in& to_addr = addrMap[std::make_pair(receiver, sender)];
        to_addr.sin_family = AF_INET;
        to_addr.sin_port = htons(ip_port);

        if (!inet_aton(ip_address.data(), &to_addr.sin_addr)) {
            throw Exception("'inet_aton' error %d: %s",errno,strerror(errno));
        }
    }

    return addrMap;
}

const sockaddr_in* getAddrByName(
    const AddrMap& map,
    const std::string& receiver,
    const std::string& sender)
{
    const auto& it = map.find(std::make_pair(receiver, sender));
    if (map.end() != it) {
        return &it->second;
    }
    return nullptr;
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
    sockaddr_in adr;
    memset(&adr,0,sizeof(adr));
    adr.sin_family=AF_INET;
    adr.sin_addr.s_addr=/*htonl(*/INADDR_ANY/*)*/; //???
    adr.sin_port=htons(SND_PORT);

    if (bind(sockfd,(struct sockaddr*)&adr,sizeof(adr))==-1)
      throw Exception("'bind' error %d: %s",errno,strerror(errno));

    char buf[10];
    bool receivedCmdTlgSndStepByStep=false;
    TDateTime lastSendOutAStepByStep=NoExists;
    std::time_t lastRotUpdate = std::time(nullptr);
    AddrMap addrMap = getAddrMap();
    for (;;)
    {
      InitLogTime(argc>0?argv[0]:NULL);
      bool sendOutAStepByStep=receivedCmdTlgSndStepByStep ||
                              lastSendOutAStepByStep==NoExists ||
                              (lastSendOutAStepByStep<NowUTC()-((double)TLG_STEP_BY_STEP_TIMEOUT())/MSecsPerDay);

      bool queue_not_empty=scan_tlg(sendOutAStepByStep, addrMap);
      if (sendOutAStepByStep) lastSendOutAStepByStep=NowUTC();

      *buf=0;
      waitCmd("CMD_TLG_SND",queue_not_empty?PROC_INTERVAL():WAIT_INTERVAL(),buf,sizeof(buf));
      receivedCmdTlgSndStepByStep=(strchr(buf,'S')!=NULL); //true только тогда когда пришел ответ на предыдущую OutAStepByStep
      // обновление кеша ROT
      if (ROT_UPDATE_INTERVAL() <= std::difftime(std::time(nullptr), lastRotUpdate)) {
        lastRotUpdate = std::time(nullptr);
        addrMap = getAddrMap();
      }
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

bool scan_tlg(bool sendOutAStepByStep, const AddrMap& addrMap)
{
  time_t time_start=time(NULL);

  static DB::TQuery TlgQry(PgOra::getROSession("TLG_QUEUE"));
  if (TlgQry.SQLText.empty())
  {
    TlgQry.Clear();

    TlgQry.SQLText = PgOra::supportsPg("TLG_QUEUE")

     ? "SELECT   id, tlg_num, sender, receiver, priority, "
                "time, last_send, ttl "
       "FROM     tlg_queue "
       "WHERE    type IN ('OUTA','OUTB','OAPP') AND status='PUT' "
       "ORDER BY priority, time_msec, tlg_num"

     : "SELECT /*+ INDEX (tlg_queue tlg_queue__idx2)*/ "
                "id, tlg_num, sender, receiver, priority, "
                "time, last_send, ttl "
       "FROM     tlg_queue "
       "WHERE    type IN ('OUTA','OUTB','OAPP') AND status='PUT' "
       "ORDER BY priority, time_msec, tlg_num";
  };

  static DB::TQuery TlgUpdQry(PgOra::getRWSession("TLG_QUEUE"));
  if (TlgUpdQry.SQLText.empty())
  {
    TlgUpdQry.Clear();
    TlgUpdQry.SQLText=
      "UPDATE tlg_queue SET last_send=:last_send "
      "WHERE id=:id AND "
      "      type IN ('OUTA','OUTB','OAPP') AND status='PUT' ";
    TlgUpdQry.DeclareVariable("last_send", otFloat);
    TlgUpdQry.DeclareVariable("id", otInteger);
  };

  struct addrComparator {
    bool operator() (const sockaddr_in& lhs, const sockaddr_in& rhs) const {
      return std::tie(lhs.sin_addr.s_addr, lhs.sin_port) < std::tie(rhs.sin_addr.s_addr, rhs.sin_port);
    }
  };
  std::map<sockaddr_in, int, addrComparator> addrCount;

  //считаем все телеграммы, которые еще не отправлены
  AIRSRV_MSG tlg_out = {};
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
      if (strcmp(TlgQry.FieldAsString("sender").data(), OWN_CANON_NAME())!=0) continue; //не добавлять критерий "tlg_queue.sender=:sender" в TlgQry - плохой план разбора
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

        // if (TlgQry.FieldIsNULL("ip_address")||TlgQry.FieldIsNULL("ip_port"))
        //   throw Exception("Unknown receiver %s", TlgQry.FieldAsString("receiver"));

        const sockaddr_in * addrPtr = nullptr;

        if (TlgQry.FieldIsNULL("receiver")
         || TlgQry.FieldIsNULL("sender")
         || nullptr == (addrPtr = getAddrByName(addrMap, TlgQry.FieldAsString("receiver"), TlgQry.FieldAsString("sender")))) {
          throw Exception("Unknown receiver %s", TlgQry.FieldAsString("receiver"));
        }

        const auto iCount = addrCount.find(*addrPtr);

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

            if (sendto(sockfd,(char*)&tlg_out,sizeof(tlg_out)-sizeof(tlg_out.body)+len,0,
                       (const sockaddr*)addrPtr,sizeof(sockaddr_in))==-1)
              throw Exception("'sendto' error %d: %s",errno,strerror(errno));
            monitor_idle_zapr_type(1, QUEPOT_NULL);
            TlgUpdQry.SetVariable("id", tlg_id);
            TlgUpdQry.SetVariable("last_send", nowUTC);
            TlgUpdQry.Execute();
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
          addrCount.insert(iCount, std::make_pair(*addrPtr, 1));
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

