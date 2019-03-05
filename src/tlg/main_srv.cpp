#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <string>
#include "misc.h"
#include "date_time.h"
#include "astra_consts.h"
#include "astra_main.h"
#include "astra_utils.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "apps_interaction.h"
#include <edilib/edi_user_func.h>
#include <edilib/edi_tables.h>
#include <serverlib/ourtime.h>
#include <libtlg/telegrams.h>



#define NICKNAME "VLAD"
#include <serverlib/slogger.h>

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

enum { tEdi, tAPPS, tTYPEB, tNone };

static int WAIT_INTERVAL()       //�����ᥪ㭤�
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TLG_SRV_WAIT_INTERVAL",1,NoExists,60000);
  return VAR;
};

static int sockfd=-1;

void process_tlg(void);
//int h2h_in(char *h2h_tlg, H2H_MSG *h2h);

int main_srv_tcl(int supervisorSocket, int argc, char *argv[])
{
  try
  {
    init_tlg_callbacks();
    sleep(1);
    InitLogTime(argc>0?argv[0]:NULL);

    int SRV_PORT;
    const char *port_tcl=Tcl_GetVar(getTclInterpretator(),"SRV_PORT",TCL_GLOBAL_ONLY);
    if (port_tcl==NULL||StrToInt(port_tcl,SRV_PORT)==EOF)
      throw Exception("Unknown or wrong SRV_PORT");

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
      ->connect_db();
    init_locale();

    if ((sockfd=socket(AF_INET,SOCK_DGRAM,0))==-1)
      throw Exception("'socket' error %d: %s",errno,strerror(errno));
    sockaddr_in adr;
    memset(&adr,0,sizeof(adr));
    adr.sin_family=AF_INET;
    adr.sin_addr.s_addr=htonl(INADDR_ANY); //???
    adr.sin_port=htons(SRV_PORT);

    if (bind(sockfd,(struct sockaddr*)&adr,sizeof(adr))==-1)
      throw Exception("'bind' error %d: %s",errno,strerror(errno));

    fd_set rfds;
    struct timeval tv;
    int res;
    for (;;)
    {
      InitLogTime(argc>0?argv[0]:NULL);

      FD_ZERO(&rfds);
      FD_SET(sockfd,&rfds);
      tv.tv_sec=WAIT_INTERVAL()/1000;
      tv.tv_usec=WAIT_INTERVAL()%1000*1000;

      if ((res=select(sockfd+1,&rfds,NULL,NULL,&tv))==-1)
        throw Exception("'select' error %d: %s",errno,strerror(errno));
      if (res!=0&&FD_ISSET(sockfd,&rfds))
      {
        InitLogTime(argc>0?argv[0]:NULL);
        process_tlg();
      };
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
    OraSession.Rollback();
    OraSession.LogOff();
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };
  return 0;
}

bool update_tlg_stat_time_send(const AIRSRV_MSG& tlg_in)
{
    TQuery TlgUpdQry(&OraSession);
    TlgUpdQry.SQLText=
      "UPDATE tlg_stat SET time_send=SYSTEM.UTCSYSDATE "
      "WHERE queue_tlg_id=:tlg_num AND sender_canon_name=:sender";
    TlgUpdQry.CreateVariable("sender",otString,tlg_in.Receiver); //OWN_CANON_NAME
    TlgUpdQry.CreateVariable("tlg_num",otInteger,(int)tlg_in.num);
    TlgUpdQry.Execute();
    return TlgUpdQry.RowsProcessed() > 0;
}

bool update_tlg_stat_time_receive(const AIRSRV_MSG& tlg_in)
{
    TQuery TlgUpdQry(&OraSession);
    TlgUpdQry.SQLText=
      "UPDATE tlg_stat SET time_receive=SYSTEM.UTCSYSDATE "
      "WHERE queue_tlg_id=:tlg_num AND sender_canon_name=:sender";
    TlgUpdQry.CreateVariable("sender",otString,tlg_in.Receiver); //OWN_CANON_NAME
    TlgUpdQry.CreateVariable("tlg_num",otInteger,(int)tlg_in.num);
    TlgUpdQry.Execute();
    return TlgUpdQry.RowsProcessed() > 0;
}

bool del_from_tlg_queue_by_status(const AIRSRV_MSG& tlg_in, const std::string& status)
{
    TQuery TlgUpdQry(&OraSession);
    TlgUpdQry.SQLText=
      "DELETE FROM tlg_queue "
      "WHERE sender= :sender AND tlg_num= :tlg_num AND "
      "      type IN ('OUTA','OUTB','OAPP') AND status= :status";
    TlgUpdQry.CreateVariable("sender", otString, tlg_in.Receiver);
    TlgUpdQry.CreateVariable("tlg_num",otInteger,(int)tlg_in.num);
    TlgUpdQry.CreateVariable("status", otString, status);
    TlgUpdQry.Execute();
    return TlgUpdQry.RowsProcessed() > 0;
}

bool del_from_tlg_queue(const AIRSRV_MSG& tlg_in)
{
    TQuery TlgUpdQry(&OraSession);
    TlgUpdQry.SQLText=
      "DELETE FROM tlg_queue "
      "WHERE sender= :sender AND tlg_num= :tlg_num AND "
      "      type IN ('OUTA','OUTB','OAPP')";
    TlgUpdQry.CreateVariable("sender", otString, tlg_in.Receiver);
    TlgUpdQry.CreateVariable("tlg_num",otInteger,(int)tlg_in.num);
    TlgUpdQry.Execute();
    return TlgUpdQry.RowsProcessed() > 0;
}

bool upd_tlg_queue_status(const AIRSRV_MSG& tlg_in,
                          const std::string& curStatus, const std::string& newStatus)
{
    TQuery TlgUpdQry(&OraSession);
    TlgUpdQry.SQLText=
      "UPDATE tlg_queue SET status= :new_status "
      "WHERE sender= :sender AND tlg_num= :tlg_num AND "
      "      type IN ('OUTA','OUTB','OAPP') AND status= :cur_status";
    TlgUpdQry.CreateVariable("sender",     otString, tlg_in.Receiver);
    TlgUpdQry.CreateVariable("tlg_num",    otInteger,(int)tlg_in.num);
    TlgUpdQry.CreateVariable("cur_status", otString, curStatus);
    TlgUpdQry.CreateVariable("new_status", otString, newStatus);
    TlgUpdQry.Execute();
    return TlgUpdQry.RowsProcessed() > 0;
}

bool upd_tlgs_by_error(const AIRSRV_MSG& tlg_in, const std::string& error)
{
    TQuery TlgUpdQry(&OraSession);
    TlgUpdQry.SQLText=
      "UPDATE tlgs SET error= :error "
      "WHERE tlg_num= :tlg_num AND sender= :sender AND "
      "      type IN ('OUTA','OUTB','OAPP')";
    TlgUpdQry.CreateVariable("sender", otString, tlg_in.Receiver);
    TlgUpdQry.CreateVariable("tlg_num",otInteger,(int)tlg_in.num);
    TlgUpdQry.CreateVariable("error",  otString, error);
    TlgUpdQry.Execute();
    return TlgUpdQry.RowsProcessed() > 0;
}

bool handle_tlg_ack(const AIRSRV_MSG& tlg_in)
{
    if(!upd_tlg_queue_status(tlg_in, "PUT", "SEND"))
    {
      ProgTrace(TRACE0,"Attention! Can't find tlg in tlg_queue "
                       "(sender: %s, tlg_num: %d, curr_status: PUT)",
                       tlg_in.Receiver, tlg_in.num);
      return false;
    }
    update_tlg_stat_time_send(tlg_in);

    return true;
}

bool handle_tlg_f_ack(const AIRSRV_MSG& tlg_in, bool& wasAck)
{
    wasAck = del_from_tlg_queue_by_status(tlg_in, "SEND");
    if(!wasAck) {
        LogTrace(TRACE1) << "ACK was not found for tlg: " << tlg_in.num;
        del_from_tlg_queue_by_status(tlg_in, "PUT");
        update_tlg_stat_time_send(tlg_in);
    }
    update_tlg_stat_time_receive(tlg_in);

    return true;
}

void process_tlg(void)
{
  struct sockaddr_in to_addr,from_addr;
  memset(&to_addr,0,sizeof(to_addr));
  memset(&from_addr,0,sizeof(from_addr));
  AIRSRV_MSG tlg_in = {}, tlg_out = {};
  int len,tlg_len,from_addr_len,tlg_header_len=sizeof(tlg_in)-sizeof(tlg_in.body);
  from_addr_len=sizeof(from_addr);
  char *tlg_body;
  bool wasAck = false;
  int type = tNone;
  TEdiTlgSubtype subtype = stCommon;
  time_t start_time=time(NULL);

  try
  {
    if ((len = recvfrom(sockfd,(char*)&tlg_in,sizeof(tlg_in),0,
                        (struct sockaddr*)&from_addr,(socklen_t*)&from_addr_len))==-1)
    {
      if (errno==ECONNREFUSED) return;
      throw Exception("'recvfrom' error %d: %s",errno,strerror(errno));
    };
    tlg_in.num=ntohl(tlg_in.num);
    tlg_in.type=ntohs(tlg_in.type);
    tlg_in.TTL=ntohs(tlg_in.TTL);
    //protection
    tlg_in.Sender[5]=0;
    tlg_in.Receiver[5]=0;

    if (tlg_in.type==TLG_IN||tlg_in.type==TLG_OUT)
    {
      if (len>(int)sizeof(tlg_in)-1)
        throw Exception("Telegram too long (sender=%s, num=%d, type=%d)",
                        tlg_in.Sender,tlg_in.num,tlg_in.type);
      if (len<=tlg_header_len)
        throw Exception("Telegram too small (sender=%s, num=%d, type=%d)",
                        tlg_in.Sender,tlg_in.num,tlg_in.type);
    }
    else
    {
      if (len>(int)sizeof(tlg_in)-1)
      //if (len>tlg_header_len)
        throw Exception("Telegram too long (sender=%s, num=%d, type=%d)",
                        tlg_in.Sender,tlg_in.num,tlg_in.type);
      if (len<tlg_header_len)
        throw Exception("Telegram too small (sender=%s, num=%d, type=%d)",
                        tlg_in.Sender,tlg_in.num,tlg_in.type);
    };
    if (strcmp(tlg_in.Receiver,OWN_CANON_NAME())!=0)
      throw Exception("Unknown telegram receiver %s",tlg_in.Receiver);

    TQuery RotQry(&OraSession);
    RotQry.SQLText=
      "SELECT ip_port FROM rot "
      "WHERE ip_address=:addr AND ip_port=:port AND "
      "      own_canon_name=:own_canon_name AND rownum<2";
    RotQry.CreateVariable("addr",otString,inet_ntoa(from_addr.sin_addr));
    RotQry.CreateVariable("port",otInteger,ntohs(from_addr.sin_port));
    RotQry.CreateVariable("own_canon_name",otString,OWN_CANON_NAME());
    RotQry.Execute();
    if (RotQry.Eof)
      throw Exception("Telegram from unknown router %s:%d",
                      RotQry.GetVariableAsString("addr"),
                      RotQry.GetVariableAsInteger("port"));

    to_addr.sin_addr=from_addr.sin_addr;
    to_addr.sin_family=AF_INET;
    to_addr.sin_port=htons(RotQry.FieldAsInteger("ip_port"));

    switch(tlg_in.type)
    {
      case TLG_IN:
      case TLG_OUT:
      {
        monitor_idle_zapr_type(1, QUEPOT_TLG_INP);
        //�஢��塞 �� host-to-host
//        if (len-tlg_header_len>(int)sizeof(h2hinf.data)-1)
//          throw Exception("Telegram too long. Can't check H2H header (sender=%s, num=%d, type=%d)",
//                          tlg_in.Sender,tlg_in.num,tlg_in.type);

        bool is_hth = false;
        hth::HthInfo hthInfo = {};
        size_t hthHeadLen = hth::fromString(tlg_in.body, hthInfo);

        if (hthHeadLen > 0) {
            tlg_body = tlg_in.body + hthHeadLen;
            hth::removeSpecSymbols(tlg_body);
            is_hth = true;
            hth::trace(TRACE5, hthInfo);
        } else {
            tlg_body = tlg_in.body;
        }

        tlg_len = strlen(tlg_body);

        if(IsEdifactText(tlg_body,tlg_len)) {
          type = tEdi;
          subtype = specifyEdiTlgSubtype(tlg_body);
        } else if (IsAPPSAnswText(tlg_body)) {
          type = tAPPS;
        } else {
          type = tTYPEB;
        }

        if (tlg_in.TTL==1) return; //��-��襬� ���� �� if (type == tEdi) SendEdiTlgCONTRL

        //᭠砫� �饬 id ⥫��ࠬ��, �᫨ ⠪���� 㦥 �뫠
        {
          TQuery TlgQry(&OraSession);
          TlgQry.SQLText=
            "SELECT id FROM tlgs WHERE tlg_num= :tlg_num AND sender= :sender ";
          TlgQry.CreateVariable("sender",otString,tlg_in.Sender);
          TlgQry.CreateVariable("tlg_num",otInteger,(int)tlg_in.num);
          TlgQry.Execute();
          for(;!TlgQry.Eof;TlgQry.Next())
          {
            string text=getTlgText(TlgQry.FieldAsInteger("id"));
            if ((int)text.size()==tlg_len)
            {
              if (memcmp(tlg_body,text.c_str(),tlg_len)==0) break;
            };
          };

          TDateTime nowUTC=NowUTC();
          if (TlgQry.Eof) //�� ��諨 - ����� ��⠢�塞 �����
          {
            ProgTrace(TRACE5,"IN: PUT (sender=%s, tlg_num=%d, time=%.10f)", tlg_in.Sender, tlg_in.num, nowUTC);

            int tlg_id = getNextTlgNum();

            TQuery TlgInsQry(&OraSession);
            TlgInsQry.Clear();
            TlgInsQry.CreateVariable("id",otInteger,tlg_id);
            TlgInsQry.CreateVariable("sender",otString,tlg_in.Sender);
            TlgInsQry.CreateVariable("tlg_num",otInteger,(int)tlg_in.num);
            TlgInsQry.CreateVariable("receiver",otString,tlg_in.Receiver);
            if ( type == tEdi )
              TlgInsQry.CreateVariable("type",otString,"INA");
            else if ( type == tTYPEB )
              TlgInsQry.CreateVariable("type",otString,"INB");
            else if ( type == tAPPS )
              TlgInsQry.CreateVariable("type",otString,"IAPP");

            TlgInsQry.CreateVariable("time",otDate,nowUTC);

            // tlgs
            TlgInsQry.SQLText=
              "INSERT INTO tlgs(id,sender,tlg_num,receiver,type,error,time,typeb_tlg_id,typeb_tlg_num) "
              "VALUES(:id,:sender,:tlg_num,:receiver,:type,NULL,:time,NULL,NULL)";
            TlgInsQry.Execute();
            putTlgText(tlg_id, string(tlg_body,tlg_len));

            // tlg_queue
            TlgInsQry.SQLText=
              "INSERT INTO tlg_queue(id,sender,tlg_num,receiver,type,subtype,priority,status,time,ttl,time_msec,last_send) "
              "VALUES(:id,:sender,:tlg_num,:receiver,:type,:subtype,1,'PUT',:time,:ttl,:time_msec,NULL)";
            if (tlg_in.TTL>0)
              TlgInsQry.CreateVariable("ttl",otInteger,(int)(tlg_in.TTL-(time(NULL)-start_time)));
            else
              TlgInsQry.CreateVariable("ttl",otInteger,FNull);
            TlgInsQry.CreateVariable("time_msec",otFloat,nowUTC);
            if ( type == tEdi )
              TlgInsQry.CreateVariable("subtype", otString, getEdiTlgSubtypeName(subtype));
            else
              TlgInsQry.CreateVariable("subtype", otString, FNull);
            TlgInsQry.Execute();
            if(is_hth)
            {
                tlgnum_t tlgNum(boost::lexical_cast<std::string>(tlg_id));
                telegrams::callbacks()->writeHthInfo(tlgNum, hthInfo);
            }
          }
          else
          {
            ProgTrace(TRACE5,"IN: already in tlgs (sender=%s, tlg_num=%d, time=%.10f)", tlg_in.Sender, tlg_in.num, nowUTC);
          }
        };
      }
        break;
      case TLG_ACK:
        {
          if(!handle_tlg_ack(tlg_in)) {
              OraSession.Rollback();
              return;
          }
        };
        break;
      case TLG_F_ACK:
        {
          handle_tlg_f_ack(tlg_in, wasAck);
        };
        break;
      case TLG_F_NEG:
      case TLG_CFG_ERR:
      case TLG_CRASH:
        {
          del_from_tlg_queue(tlg_in);
          upd_tlgs_by_error(tlg_in, "GATE");
        };
        break;
      case TLG_ACK_ACK:
      case TLG_CONN_REQ:
      case TLG_CONN_RES:
        break;
      default:
        throw Exception("Unknown telegram type %d",tlg_in.type);
        return;
    };

    switch (tlg_in.type)
    {
      case TLG_IN:
      case TLG_OUT:
        tlg_out.type=htons(TLG_F_ACK);
        tlg_len=len-tlg_header_len;
        if (tlg_len>9) tlg_len=9;
        memcpy(tlg_out.body,tlg_in.body,tlg_len);
        break;
      case TLG_F_ACK:
      case TLG_F_NEG:
      case TLG_CFG_ERR:
      case TLG_CRASH:
        tlg_out.type=htons(TLG_ACK_ACK);
        tlg_len=0;
        break;
      case TLG_CONN_REQ:
        tlg_out.type=htons(TLG_CONN_RES);
        tlg_len=0;
        break;
      default:
        OraSession.Commit();
        if (tlg_in.type==TLG_ACK)
        {
          ProgTrace(TRACE5,"OUT: PUT->SEND (sender=%s, tlg_num=%d, time=%.10f)", tlg_in.Receiver, tlg_in.num, NowUTC());
          sendCmdTlgSnd(); //����� ��ࠢ騪� (����� ��ࠢ���� ᫥������ ⥫��ࠬ��)
        };
        return;
    };
    if ((tlg_in.type==TLG_IN||tlg_in.type==TLG_OUT)&&(type==tEdi)&&tlg_in.TTL>0)
    {
      if (time(NULL)-start_time>=tlg_in.TTL)
      {
        OraSession.Rollback();
        //��-��襬� ���� �� if (type==tEdi) SendEdiTlgCONTRL
        return;
      };
    };

    tlg_out.num=htonl(tlg_in.num);
    tlg_out.TTL=htons(0);
    strncpy(tlg_out.Sender,tlg_in.Receiver,6);
    strncpy(tlg_out.Receiver,tlg_in.Sender,6);
    if (sendto(sockfd,(char*)&tlg_out,sizeof(tlg_out)-sizeof(tlg_out.body)+tlg_len,0,
               (struct sockaddr*)&to_addr,sizeof(to_addr))==-1)
      throw Exception("'sendto' error %d: %s",errno,strerror(errno));
    /*ProgTrace(TRACE5,"'sendto': tlg_num=%d, type=%d, sender=%s, receiver=%s",
                     ntohl(tlg_out.num),ntohs(tlg_out.type),tlg_out.Sender,tlg_out.Receiver);*/

    OraSession.Commit();
    switch(tlg_in.type)
    {
      case TLG_IN:
      case TLG_OUT:
        if ( type == tEdi ) {
            sendCmdEdiHandler(subtype);
        } else if ( type == tAPPS ) {
            sendCmdAppsHandler();
        } else if ( type == tTYPEB ) {
            sendCmdTypeBHandler();
        }
        break;
      case TLG_F_ACK:
        ProgTrace(TRACE5,"OUT: SEND->DONE (sender=%s, tlg_num=%d, time=%.10f)", tlg_in.Receiver, tlg_in.num, NowUTC());
        if(!wasAck) {
            ProgTrace(TRACE5, "Kick sender by TLG_F_ACK");
            sendCmdTlgSnd(); //����� ��ࠢ騪� (����� ��ࠢ���� ᫥������ ⥫��ࠬ��)
        }
        break;
      case TLG_F_NEG:
      case TLG_CFG_ERR:
      case TLG_CRASH:
        ProgTrace(TRACE5,"OUT: PUT/SEND->ERR (sender=%s, tlg_num=%d, time=%.10f)", tlg_in.Receiver, tlg_in.num, NowUTC());
        break;
    };
  }
  catch(Exception &E)
  {
    OraSession.Rollback();
    try
    {
      ProgError(STDLOG,"Exception: %s",E.what());
      OraSession.Commit();
    }
    catch(...) {};
  };
  return;
};


/* -----------------------------------------------------------------------
   ��뢠���� � ���⥪�� airimp
   ��� ⥫��ࠬ� � ��ࠬ����� H2H.
   �뤥�便� EDIFACT����� ⥫��ࠬ��.
 */
//int h2h_in(char *h2h_tlg, H2H_MSG *h2h)
//{
//  int len;
//  char *p, *p1, *why_p, *end_p, part = 0;
///*
//        memcpy(&tlg_len, h2h_tlg, sizeof(tlg_len));

//        if ( tlg_len != strlen(h2h_tlg+2)+2 )
//        {
//          ProgError(STDLOG, "H2H Error of TLG length");
//          return -1;
//        }
//*/
//        memset(h2h, 0, sizeof(H2H_MSG));

//        if ( memcmp(h2h_tlg, H2H_BEG_STR, strlen(H2H_BEG_STR)) )
//        {
////        ProgTrace(1, STDLOG, "TLG not in H2H format");
//          strcpy(h2h->data, h2h_tlg);
//          return -1;
//        }

//        h2h->type = h2h_tlg[4];
///*
//        if ( h2h_tlg[8] != '/')
//        {
//                if ( h2h_tlg[8] == 'V' )
//                  part = 1;
//                else
//                if ( h2h_tlg[8] == 'U' )
//                  h2h->end = TRUE;
//                else
//                if ( h2h_tlg[8] == 'T' )
//                {
//                        if ( h2h_tlg[9] != '/' )
//                          part = h2h_tlg[9] - 64;               segment A - Z (1 - 26)
//                        else
//                          ProgError(STDLOG, "H2H Segment unknown");
//                }
//        }
//        h2h->part = part;
//*/
//        if ( h2h_tlg[8] != '/')
//        {
//                h2h->qri5 = h2h_tlg[8];
//                h2h->qri6 = h2h_tlg[9];
//        }

//        if ( (p=strstr(h2h_tlg,"/E")) && (p1=strchr(p+1,'/')) && ((len=p1-p-3) <= 10) )
//        {
//                memcpy(h2h->sndr, p+3, len);
//                h2h->sndr[len] = 0;
//        }
//        else
//        {
//                ProgError(STDLOG, "H2H Sender too long");
//          return -1;
//        }

//        if ( (p=strstr(h2h_tlg,"/I")) && (p1=strchr(p+1,'/')) && ((len=p1-p-3) <= 10) )
//        {
//                memcpy(h2h->rcvr, p+3, len);
//                h2h->rcvr[len] = 0;
//        }
//        else
//        {
//                ProgError(STDLOG, "H2H Receiver too long");
//          return -1;
//        }

//        p = strstr(p+1, "/P");

//        end_p = strchr(p, '\r');

//        if ( (why_p=strchr(p+1, '/')) && (why_p < end_p) )
//          p1 = why_p;
//        else
//          p1 = end_p;

//        if ( (len=p1-p-2) <= 20 )       /* -2 = "/P" */
//        {
//                memcpy(h2h->tpr, p+2, len);
//                h2h->tpr[len] = 0;
//        }
//        else
//        {
//                ProgError(STDLOG, "H2H TPR too long");
//          return -1;
//        }

//        if ( (p1 != end_p) && (end_p-p1-1 == 2) )
//        {
//                memcpy(h2h->err, p1+1, 2);
//                h2h->err[2] = 0;
//        }

//        end_p = strchr(end_p+1, '\r');

//        if ( *(end_p+1) == 0x02 )                                               /*      if STX  */
//          end_p += 2;
//        else
//          end_p += 1;

//        strcpy(h2h->data, end_p);

//        if ( h2h->data[strlen(h2h->data)-1] == 0x03 )   /*      if ETX  */
//          h2h->data[strlen(h2h->data)-1] = 0;

//  return (int)part;
//}
