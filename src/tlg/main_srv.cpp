#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <string>
#include "astra_consts.h"
#include "astra_utils.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "edilib/edi_user_func.h"
#include "serverlib/ourtime.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

static const int WAIT_INTERVAL()       //seconds
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TLG_SRV_WAIT_INTERVAL",1,NoExists,60);
  return VAR;
};

static int sockfd=-1;

void process_tlg(void);
int h2h_in(char *h2h_tlg, H2H_MSG *h2h);

int main_srv_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(1);
    InitLogTime(NULL);
    OpenLogFile("logairimp");

    int SRV_PORT;
    const char *port_tcl=Tcl_GetVar(interp,"SRV_PORT",TCL_GLOBAL_ONLY);
    if (port_tcl==NULL||StrToInt(port_tcl,SRV_PORT)==EOF)
      throw Exception("Unknown or wrong SRV_PORT");

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
      ->connect_db();
    if (init_edifact()<0) throw Exception("'init_edifact' error");

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
      InitLogTime(NULL);

      FD_ZERO(&rfds);
      FD_SET(sockfd,&rfds);
      tv.tv_sec=WAIT_INTERVAL();
      tv.tv_usec=0;

      if ((res=select(sockfd+1,&rfds,NULL,NULL,&tv))==-1)
        throw Exception("'select' error %d: %s",errno,strerror(errno));
      if (res!=0&&FD_ISSET(sockfd,&rfds))
      {
        InitLogTime(NULL);
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
};

void process_tlg(void)
{
  struct sockaddr_in to_addr,from_addr;
  memset(&to_addr,0,sizeof(to_addr));
  memset(&from_addr,0,sizeof(from_addr));
  AIRSRV_MSG tlg_in,tlg_out;
  int len,tlg_len,tlg_len2,from_addr_len,tlg_header_len=sizeof(tlg_in)-sizeof(tlg_in.body);
  from_addr_len=sizeof(from_addr);
  char buf[sizeof(tlg_in.body)];
  char *tlg_body;

  H2H_MSG h2hinf;
  bool is_h2h=false;
  bool is_edi=false;
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
      "SELECT 1,router_num,ip_port FROM rot\
       WHERE ip_address=:addr AND ip_port=:port AND\
             own_canon_name=:own_canon_name AND canon_name=:canon_name\
       union\
       SELECT 2,router_num,ip_port FROM rot\
       WHERE ip_address=:addr AND ip_port=:port AND\
             own_canon_name=:own_canon_name\
       ORDER BY 1";
    RotQry.CreateVariable("addr",otString,inet_ntoa(from_addr.sin_addr));
    RotQry.CreateVariable("port",otInteger,ntohs(from_addr.sin_port));
    RotQry.CreateVariable("own_canon_name",otString,OWN_CANON_NAME());
    RotQry.CreateVariable("canon_name",otString,tlg_in.Sender);
    RotQry.Execute();
    if (RotQry.RowCount()==0)
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
        monitor_idle_zapr_type(1, QUEPOT_TLG_INP);
        //проверяем на host-to-host
        if (len-tlg_header_len>(int)sizeof(h2hinf.data)-1)
          throw Exception("Telegram too long. Can't check H2H header (sender=%s, num=%d, type=%d)",
                          tlg_in.Sender,tlg_in.num,tlg_in.type);
        if (h2h_in(tlg_in.body, &h2hinf) >= 0) is_h2h = true;

        if (is_h2h)
        {
          tlg_body=h2hinf.data;
          tlg_len=strlen(h2hinf.data);
        }
        else
        {
          tlg_body=tlg_in.body;
          tlg_len=len-tlg_header_len;
        };
        is_edi=IsEdifactText(tlg_body,tlg_len);
        if (tlg_in.TTL==1) return; //по-хорошему надо бы if (is_edi) SendEdiTlgCONTRL

        //сначала ищем id телеграммы, если таковая уже была
        {
          TQuery TlgQry(&OraSession);
          TlgQry.SQLText=
            "SELECT id,tlg_text FROM tlgs WHERE sender= :sender AND tlg_num= :tlg_num";
          TlgQry.CreateVariable("sender",otString,tlg_in.Sender);
          TlgQry.CreateVariable("tlg_num",otInteger,(int)tlg_in.num);
          TlgQry.Execute();
          for(;!TlgQry.Eof;TlgQry.Next())
          {
            tlg_len2=TlgQry.GetSizeLongField("tlg_text");
            if (tlg_len2==tlg_len)
            {
              TlgQry.FieldAsLong("tlg_text",buf);
              if (memcmp(tlg_body,buf,tlg_len2)==0) break;
            };
          };
          if (TlgQry.Eof) //не нашли - значит вставляем новую
          {
            ProgTrace(TRACE5,"IN: PUT (sender=%s, tlg_num=%ld, time=%f)", tlg_in.Sender, tlg_in.num, NowUTC());
            TQuery TlgInsQry(&OraSession);
            TlgInsQry.Clear();
            TlgInsQry.CreateVariable("sender",otString,tlg_in.Sender);
            TlgInsQry.CreateVariable("tlg_num",otInteger,(int)tlg_in.num);
            TlgInsQry.CreateVariable("receiver",otString,tlg_in.Receiver);
            TlgInsQry.DeclareVariable("type",otInteger);
            if (is_edi)
              TlgInsQry.CreateVariable("type",otString,"INA");
            else
              TlgInsQry.CreateVariable("type",otString,"INB");
            // tlgs
            TlgInsQry.SQLText=
              "INSERT INTO tlgs(id,sender,tlg_num,receiver,type,error,time,tlg_text) "
              "VALUES(tlgs_id.nextval,:sender,:tlg_num,:receiver,:type,NULL,system.UTCSYSDATE,:tlg_text)";
            TlgInsQry.DeclareVariable("tlg_text",otLong);
            TlgInsQry.SetLongVariable("tlg_text",tlg_body,tlg_len);
            TlgInsQry.Execute();
            // tlg_queue
            TlgInsQry.SQLText=
              "INSERT INTO tlg_queue(id,sender,tlg_num,receiver,type,status,time,ttl,next_send) "
              "VALUES(tlgs_id.currval,:sender,:tlg_num,:receiver,:type,'PUT',system.UTCSYSDATE,:ttl,NULL)";
            if (tlg_in.TTL>0)
              TlgInsQry.CreateVariable("ttl",otInteger,(int)(tlg_in.TTL-(time(NULL)-start_time)));
            else
              TlgInsQry.CreateVariable("ttl",otInteger,FNull);
            TlgInsQry.DeleteVariable("tlg_text");
            TlgInsQry.Execute();
            if (is_h2h)
            {
              //h2h_tlgs
              char buf[2];
              TlgInsQry.Clear();
              TlgInsQry.SQLText=
                "INSERT INTO h2h_tlgs(id,type,qri5,qri6,sender,receiver,tpr,err)\
                 VALUES(tlgs_id.currval,:type,:qri5,:qri6,:sender,:receiver,:tpr,:err)";
              buf[1]=0;
              buf[0]=h2hinf.type;
              TlgInsQry.CreateVariable("type",otString,buf);
              buf[0]=h2hinf.qri5;
              TlgInsQry.CreateVariable("qri5",otString,buf);
              buf[0]=h2hinf.qri6;
              TlgInsQry.CreateVariable("qri6",otString,buf);
              TlgInsQry.CreateVariable("sender",otString,h2hinf.sndr);
              TlgInsQry.CreateVariable("receiver",otString,h2hinf.rcvr);
              TlgInsQry.CreateVariable("tpr",otString,h2hinf.tpr);
              TlgInsQry.CreateVariable("err",otString,h2hinf.err);
              TlgInsQry.Execute();
            };
          };
        };
        break;
      case TLG_ACK:
      case TLG_F_ACK:
      case TLG_F_NEG:
      case TLG_CFG_ERR:
        //эта часть будет работать при условии генерации уникальных tlg_num для типа OUT!
        {
          TQuery TlgUpdQry(&OraSession);
          if (tlg_in.type==TLG_ACK||
              tlg_in.type==TLG_F_NEG)
          {
            TlgUpdQry.SQLText=
              "UPDATE tlg_queue SET status= :new_status, next_send= :next_send "
              "WHERE sender= :sender AND tlg_num= :tlg_num AND "
              "      type IN ('OUTA','OUTB') AND status=:curr_status";
            TlgUpdQry.DeclareVariable("new_status",otString);
            TlgUpdQry.DeclareVariable("next_send",otDate);
          }
          else
          {
            TlgUpdQry.SQLText=
              "DELETE FROM tlg_queue "
              "WHERE sender= :sender AND tlg_num= :tlg_num AND "
              "      type IN ('OUTA','OUTB') AND status=:curr_status";
          };
          TlgUpdQry.CreateVariable("sender",otString,tlg_in.Receiver); //OWN_CANON_NAME
          TlgUpdQry.CreateVariable("tlg_num",otInteger,(int)tlg_in.num);
          TlgUpdQry.DeclareVariable("curr_status",otString);
          switch(tlg_in.type)
          {
            case TLG_ACK:
              TlgUpdQry.SetVariable("curr_status","PUT");
              TlgUpdQry.SetVariable("new_status","SEND");
              TlgUpdQry.SetVariable("next_send",FNull);
              break;
            case TLG_CFG_ERR:
              TlgUpdQry.SetVariable("curr_status","PUT");
              break;
            case TLG_F_ACK:
              TlgUpdQry.SetVariable("curr_status","SEND");
              break;
            case TLG_F_NEG:
              TlgUpdQry.SetVariable("curr_status","SEND");
              TlgUpdQry.SetVariable("new_status","PUT");
              TlgUpdQry.SetVariable("next_send",NowUTC());
              break;
          };
          TlgUpdQry.Execute();
          //если TlgUpdQry.RowsProcessed()==0,значит нарушена последовательность телеграмм
          if ((tlg_in.type==TLG_ACK||
               tlg_in.type==TLG_F_NEG)&&
              TlgUpdQry.RowsProcessed()==0)
          {
            OraSession.Rollback();
            ProgTrace(TRACE5,"Attention! Can't find tlg in tlg_queue "
                             "(sender: %s, tlg_num: %ld, curr_status: %s)",
                             tlg_in.Receiver, tlg_in.num,
                             TlgUpdQry.GetVariableAsString("curr_status"));
            return;
          };
          if (tlg_in.type==TLG_CFG_ERR)
          {
            TlgUpdQry.SQLText=
              "UPDATE tlgs SET error= :error "
              "WHERE sender= :sender AND tlg_num= :tlg_num AND "
              "      type IN ('OUTA','OUTB')";
            TlgUpdQry.CreateVariable("error",otString,"GATE");
            TlgUpdQry.DeleteVariable("curr_status");
            TlgUpdQry.Execute();
          };
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
        tlg_out.type=htons(TLG_ACK_ACK);
        tlg_len=0;
        break;
      case TLG_CONN_REQ:
        tlg_out.type=htons(TLG_CONN_RES);
        tlg_len=0;
        break;
      default:
        OraSession.Commit();
        if (tlg_in.type==TLG_ACK)     ProgTrace(TRACE5,"OUT: PUT->SEND (sender=%s, tlg_num=%ld, time=%f)", tlg_in.Receiver, tlg_in.num, NowUTC());
        if (tlg_in.type==TLG_CFG_ERR) ProgTrace(TRACE5,"OUT: PUT->ERR (sender=%s, tlg_num=%ld, time=%f)", tlg_in.Receiver, tlg_in.num, NowUTC());
        return;
    };
    if ((tlg_in.type==TLG_IN||tlg_in.type==TLG_OUT)&&is_edi&&tlg_in.TTL>0)
    {
      if (time(NULL)-start_time>=tlg_in.TTL)
      {
        OraSession.Rollback();
        //по-хорошему надо бы if (is_edi) SendEdiTlgCONTRL
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
      case TLG_F_ACK:
        ProgTrace(TRACE5,"OUT: SEND->DONE (sender=%s, tlg_num=%ld, time=%f)", tlg_in.Receiver, tlg_in.num, NowUTC());
        break;
      case TLG_F_NEG:
        ProgTrace(TRACE5,"OUT: SEND->PUT (sender=%s, tlg_num=%ld, time=%f)", tlg_in.Receiver, tlg_in.num, NowUTC());
        break;
    };
    if (is_edi)
      sendCmd("CMD_EDI_HANDLER","H");
    else
      sendCmd("CMD_TYPEB_HANDLER","H");
  }
  catch(Exception E)
  {
    OraSession.Rollback();
    try
    {
      ProgError(STDLOG,"Exception: %s",E.what());
      //sendErrorTlg("Exception: %s",E.what());
      OraSession.Commit();
    }
    catch(...) {};
  };
  return;
};


/* -----------------------------------------------------------------------
   Вызывается в контексте airimp
   для телеграмм в обрамлении H2H.
   Выделяеет EDIFACTовскую телеграмму.
 */
int h2h_in(char *h2h_tlg, H2H_MSG *h2h)
{
  int len;
  char *p, *p1, *why_p, *end_p, part = 0;
/*
        memcpy(&tlg_len, h2h_tlg, sizeof(tlg_len));

        if ( tlg_len != strlen(h2h_tlg+2)+2 )
        {
          ProgError(STDLOG, "H2H Error of TLG length");
          return -1;
        }
*/
        memset(h2h, 0, sizeof(H2H_MSG));

        if ( memcmp(h2h_tlg, H2H_BEG_STR, strlen(H2H_BEG_STR)) )
        {
//        ProgTrace(1, STDLOG, "TLG not in H2H format");
          strcpy(h2h->data, h2h_tlg);
          return -1;
        }

        h2h->type = h2h_tlg[4];
/*
        if ( h2h_tlg[8] != '/')
        {
                if ( h2h_tlg[8] == 'V' )
                  part = 1;
                else
                if ( h2h_tlg[8] == 'U' )
                  h2h->end = TRUE;
                else
                if ( h2h_tlg[8] == 'T' )
                {
                        if ( h2h_tlg[9] != '/' )
                          part = h2h_tlg[9] - 64;               segment A - Z (1 - 26)
                        else
                          ProgError(STDLOG, "H2H Segment unknown");
                }
        }
        h2h->part = part;
*/
        if ( h2h_tlg[8] != '/')
        {
                h2h->qri5 = h2h_tlg[8];
                h2h->qri6 = h2h_tlg[9];
        }

        if ( (p=strstr(h2h_tlg,"/E")) && (p1=strchr(p+1,'/')) && ((len=p1-p-3) <= 10) )
        {
                memcpy(h2h->sndr, p+3, len);
                h2h->sndr[len] = 0;
        }
        else
        {
                ProgError(STDLOG, "H2H Sender too long");
          return -1;
        }

        if ( (p=strstr(h2h_tlg,"/I")) && (p1=strchr(p+1,'/')) && ((len=p1-p-3) <= 10) )
        {
                memcpy(h2h->rcvr, p+3, len);
                h2h->rcvr[len] = 0;
        }
        else
        {
                ProgError(STDLOG, "H2H Receiver too long");
          return -1;
        }

        p = strstr(p+1, "/P");

        end_p = strchr(p, '\r');

        if ( (why_p=strchr(p+1, '/')) && (why_p < end_p) )
          p1 = why_p;
        else
          p1 = end_p;

        if ( (len=p1-p-2) <= 20 )       /* -2 = "/P" */
        {
                memcpy(h2h->tpr, p+2, len);
                h2h->tpr[len] = 0;
        }
        else
        {
                ProgError(STDLOG, "H2H TPR too long");
          return -1;
        }

        if ( (p1 != end_p) && (end_p-p1-1 == 2) )
        {
                memcpy(h2h->err, p1+1, 2);
                h2h->err[2] = 0;
        }

        end_p = strchr(end_p+1, '\r');

        if ( *(end_p+1) == 0x02 )                                               /*      if STX  */
          end_p += 2;
        else
          end_p += 1;

        strcpy(h2h->data, end_p);

        if ( h2h->data[strlen(h2h->data)-1] == 0x03 )   /*      if ETX  */
          h2h->data[strlen(h2h->data)-1] = 0;

  return (int)part;
}
