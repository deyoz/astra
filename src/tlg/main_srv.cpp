#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <string>
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "stl_utils.h"
#include "tlg_parser.h"
#include "daemon.h"
#include "edilib/edi_user_func.h"

#define NICKNAME "VLAD"
#include "test.h"

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

#define WAIT_INTERVAL           10      //seconds
#define TLG_SCAN_INTERVAL       30      //seconds
#define SCAN_COUNT              50      //кол-во разбираемых телеграмм за одно сканирование

static int sockfd=-1;

void process_tlg(void);
static void scan_tlg(void);
int h2h_in(char *h2h_tlg, H2H_MSG *h2h);

int main_srv_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
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
    time_t scan_time=0;
    for (;;)
    {
      FD_ZERO(&rfds);
      FD_SET(sockfd,&rfds);
      tv.tv_sec=WAIT_INTERVAL;
      tv.tv_usec=0;

      if ((res=select(sockfd+1,&rfds,NULL,NULL,&tv))==-1)
        throw Exception("'select' error %d: %s",errno,strerror(errno));
      if (res!=0&&FD_ISSET(sockfd,&rfds)) process_tlg();

      if (time(NULL)-scan_time>=TLG_SCAN_INTERVAL)
      {
        scan_tlg();
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
              "INSERT INTO tlgs(id,sender,tlg_num,receiver,type,error,time,tlg_text)\
               VALUES(tlgs_id.nextval,:sender,:tlg_num,:receiver,:type,NULL,system.UTCSYSDATE,:tlg_text)";
            TlgInsQry.DeclareVariable("tlg_text",otLong);
            TlgInsQry.SetLongVariable("tlg_text",tlg_body,tlg_len);
            TlgInsQry.Execute();
            // tlg_queue
            TlgInsQry.SQLText=
              "INSERT INTO tlg_queue(id,sender,tlg_num,receiver,type,status,time,ttl)\
               VALUES(tlgs_id.currval,:sender,:tlg_num,:receiver,:type,'PUT',system.UTCSYSDATE,:ttl)";
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
              "UPDATE tlg_queue SET status= :new_status\
               WHERE sender= :sender AND tlg_num= :tlg_num AND\
                     type IN ('OUTA','OUTB') AND status=:curr_status";
            TlgUpdQry.DeclareVariable("new_status",otString);
          }
          else
          {
            TlgUpdQry.SQLText=
              "DELETE FROM tlg_queue\
               WHERE sender= :sender AND tlg_num= :tlg_num AND\
                     type IN ('OUTA','OUTB') AND status=:curr_status";
          };
          TlgUpdQry.CreateVariable("sender",otString,tlg_in.Receiver); //OWN_CANON_NAME
          TlgUpdQry.CreateVariable("tlg_num",otInteger,(int)tlg_in.num);
          TlgUpdQry.DeclareVariable("curr_status",otString);
          switch(tlg_in.type)
          {
            case TLG_ACK:
              TlgUpdQry.SetVariable("curr_status","PUT");
              TlgUpdQry.SetVariable("new_status","SEND");
              ProgTrace(TRACE5,"PUT->SEND (tlg_num=%ld)",tlg_in.num);
              break;
            case TLG_CFG_ERR:
              TlgUpdQry.SetVariable("curr_status","PUT");
              //TlgUpdQry.SetVariable("new_status","ERR");
              ProgTrace(TRACE5,"PUT->ERR (tlg_num=%ld)",tlg_in.num);
              break;
            case TLG_F_ACK:
              TlgUpdQry.SetVariable("curr_status","SEND");
              //TlgUpdQry.SetVariable("new_status","DONE");
              ProgTrace(TRACE5,"SEND->DONE (tlg_num=%ld)",tlg_in.num);
              break;
            case TLG_F_NEG:
              TlgUpdQry.SetVariable("curr_status","SEND");
              TlgUpdQry.SetVariable("new_status","PUT");
              break;
          };
          TlgUpdQry.Execute();
          //если TlgUpdQry.RowsProcessed()==0,значит нарушена последовательность телеграмм
          if ((tlg_in.type==TLG_ACK||
               tlg_in.type==TLG_F_NEG)&&
              TlgUpdQry.RowsProcessed()==0)
          {
            OraSession.Rollback();
            ProgError(STDLOG,"Can't find tlg in tlg_queue "
                    "(sender: %s, tlg_num: %ld, curr_status: %s)",
                    tlg_in.Receiver, tlg_in.num,
                    TlgUpdQry.GetVariableAsString("curr_status"));
            return;
          };
          if (tlg_in.type==TLG_CFG_ERR)
          {
            TlgUpdQry.SQLText=
              "UPDATE tlgs SET error= :error\
               WHERE sender= :sender AND tlg_num= :tlg_num AND\
                     type IN ('OUTA','OUTB')";
            TlgUpdQry.CreateVariable("error",otString,"GATE");
            TlgUpdQry.DeleteVariable("curr_status");
            TlgUpdQry.Execute();
          };
        };
        break;
      case TLG_ACK_ACK:
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
      default:
        OraSession.Commit();
        return;
    };
    if ((tlg_in.type==TLG_IN||tlg_in.type==TLG_OUT)&&tlg_in.TTL>0)
    {
      if (time(NULL)-start_time>=tlg_in.TTL)
      {
        OraSession.Rollback();
        //по-хорошему надо бы if (is_edi) SendEdiTlgCONTRL
        return;
      };
    }
    else
    {
      tlg_out.num=htonl(tlg_in.num);
      tlg_out.TTL=htons(0);
      strncpy(tlg_out.Sender,tlg_in.Receiver,6);
      strncpy(tlg_out.Receiver,tlg_in.Sender,6);
      if (sendto(sockfd,(char*)&tlg_out,sizeof(tlg_out)-sizeof(tlg_out.body)+tlg_len,0,
                 (struct sockaddr*)&to_addr,sizeof(to_addr))==-1)
        throw Exception("'sendto' error %d: %s",errno,strerror(errno));
    };
    OraSession.Commit();
    if (is_edi) sendCmd("CMD_EDI_HANDLER","HELLO WORLD!");
  }
  catch(Exception E)
  {
    ProgError(STDLOG,"Exception: %s",E.what());
    sendErrorTlg(ERR_CANON_NAME(),OWN_CANON_NAME(),"Exception: %s",E.what());
    OraSession.Commit();
  };
  return;
};

void scan_tlg(void)
{
  static TQuery TlgQry(&OraSession);
  if (TlgQry.SQLText.IsEmpty())
  {
    //внимание порядок объединения таблиц важен!
    TlgQry.Clear();
    TlgQry.SQLText=
      "SELECT tlg_queue.id,tlgs.tlg_text,system.UTCSYSDATE AS now,tlg_queue.time,ttl\
       FROM tlgs,tlg_queue\
       WHERE tlg_queue.id=tlgs.id AND tlg_queue.receiver=:receiver AND\
             tlg_queue.type='INB' AND tlg_queue.status='PUT'\
       ORDER BY tlg_queue.time,tlg_queue.id";
    TlgQry.CreateVariable("receiver",otString,OWN_CANON_NAME());

  };

  static TQuery TlgIdQry(&OraSession);
  if (TlgIdQry.SQLText.IsEmpty())
  {
    TlgIdQry.Clear();
    TlgIdQry.SQLText=
     "BEGIN\
        SELECT id INTO :id FROM tlgs_in\
        WHERE type= :tlg_type AND\
              time_create BETWEEN :min_time_create AND :max_time_create AND\
              merge_key= :merge_key AND rownum=1 FOR UPDATE;\
      EXCEPTION\
        WHEN NO_DATA_FOUND THEN\
          SELECT tlg_in_out__seq.nextval INTO :id FROM dual;\
      END;";
    TlgIdQry.DeclareVariable("id",otInteger);
    TlgIdQry.DeclareVariable("tlg_type",otString);
    TlgIdQry.DeclareVariable("min_time_create",otDate);
    TlgIdQry.DeclareVariable("max_time_create",otDate);
    TlgIdQry.DeclareVariable("merge_key",otString);
  };

  static TQuery InsQry(&OraSession);
  if (InsQry.SQLText.IsEmpty())
  {
    InsQry.Clear();
    InsQry.SQLText=
       "INSERT INTO tlgs_in(id,num,type,addr,heading,body,ending,\
                           merge_key,time_create,time_receive,time_parse)\
        VALUES(NVL(:id,tlg_in_out__seq.nextval),\
               :part_no,:tlg_type,:addr,:heading,:body,:ending,\
               :merge_key,:time_create,system.UTCSYSDATE,NULL)";
    InsQry.DeclareVariable("id",otInteger);
    InsQry.DeclareVariable("part_no",otInteger);
    InsQry.DeclareVariable("tlg_type",otString);
    InsQry.DeclareVariable("addr",otString);
    InsQry.DeclareVariable("heading",otString);
    InsQry.DeclareVariable("body",otLong);
    InsQry.DeclareVariable("ending",otString);
    InsQry.DeclareVariable("merge_key",otString);
    InsQry.DeclareVariable("time_create",otDate);
  };

  TQuery Qry(&OraSession);

  int len,count,bufLen=0,buf2Len=0,tlg_id;
  char *buf=NULL,*buf2=NULL,*ph,c;
  bool pr_typeb_cmd=false;
  TTlgParts parts;
  THeadingInfo *HeadingInfo=NULL;
  TEndingInfo *EndingInfo=NULL;
  count=0;
  TlgQry.Execute();
  try
  {
    while (!TlgQry.Eof&&count<SCAN_COUNT)
    {
      //проверим TTL
      tlg_id=TlgQry.FieldAsInteger("id");
      if (!TlgQry.FieldIsNULL("ttl")&&
           (TlgQry.FieldAsDateTime("now")-TlgQry.FieldAsDateTime("time"))*24*60*60>=TlgQry.FieldAsInteger("ttl"))
      {
      	errorTlg(tlg_id,"TTL");
      }
      else
        try
        {
          len=TlgQry.GetSizeLongField("tlg_text")+1;
          if (len>bufLen)
          {
            if (buf==NULL)
              ph=(char*)malloc(len);
            else
              ph=(char*)realloc(buf,len);
            if (ph==NULL) throw EMemoryError("Out of memory");
            buf=ph;
            bufLen=len;
          };
          TlgQry.FieldAsLong("tlg_text",buf);
          buf[len-1]=0;
          parts=GetParts(buf);
          ParseHeading(parts.heading,HeadingInfo);
          ParseEnding(parts.ending,HeadingInfo,EndingInfo);
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
              InsQry.SetVariable("id",FNull);
            else
              InsQry.SetVariable("id",TlgIdQry.GetVariableAsInteger("id"));
            InsQry.SetVariable("part_no",(int)part_no);
            InsQry.SetVariable("merge_key",merge_key.str());
          }
          else
          {
            InsQry.SetVariable("id",FNull);
            InsQry.SetVariable("part_no",1);
            InsQry.SetVariable("merge_key",FNull);
          };

          InsQry.SetVariable("tlg_type",HeadingInfo->tlg_type);
          if (HeadingInfo->time_create!=0)
            InsQry.SetVariable("time_create",HeadingInfo->time_create);
          else
            InsQry.SetVariable("time_create",FNull);

          c=*parts.heading.p;
          *parts.heading.p=0;
          InsQry.SetVariable("addr",parts.addr.p);
          *parts.heading.p=c;

          c=*parts.body.p;
          *parts.body.p=0;
          InsQry.SetVariable("heading",parts.heading.p);
          *parts.body.p=c;

          if (parts.ending.p!=NULL)
          {
            InsQry.SetLongVariable("body",parts.body.p,parts.ending.p-parts.body.p);
            InsQry.SetVariable("ending",parts.ending.p);
          }
          else
          {
            InsQry.SetLongVariable("body",parts.body.p,strlen(parts.body.p));
            InsQry.SetVariable("ending",FNull);
          };
          try
          {
            if (deleteTlg(tlg_id))
            {
              InsQry.Execute();
              pr_typeb_cmd=true;
            };
          }
          catch(EOracleError E)
          {
            if (E.Code==1)
            {
              Qry.Clear();
              Qry.SQLText=
                "SELECT addr,heading,body,ending FROM tlgs_in WHERE id=:id AND num=:num";
              Qry.CreateVariable("id",otInteger,InsQry.GetVariableAsInteger("id"));
              Qry.CreateVariable("num",otInteger,InsQry.GetVariableAsInteger("part_no"));
              Qry.Execute();
              if (Qry.RowCount()!=0)
              {
                len=strlen(Qry.FieldAsString("addr"))+
                    strlen(Qry.FieldAsString("heading"))+
                    Qry.GetSizeLongField("body")+
                    strlen(Qry.FieldAsString("ending"))+1;
                if (len>buf2Len)
                {
                  if (buf2==NULL)
                    ph=(char*)malloc(len);
                  else
                    ph=(char*)realloc(buf2,len);
                  if (ph==NULL) throw EMemoryError("Out of memory");
                  buf2=ph;
                  buf2Len=len;
                };
                strcpy(buf2,Qry.FieldAsString("addr"));
                strcat(buf2,Qry.FieldAsString("heading"));
                len=strlen(buf2);
                Qry.FieldAsLong("body",buf2+len);
                len+=Qry.GetSizeLongField("body");
                buf2[len]=0;
                strcat(buf2,Qry.FieldAsString("ending"));
                if (strcmp(buf,buf2)!=0)
                {
                  long part_no;
                  if (HeadingInfo->tlg_cat==tcDCS)
                    part_no=dynamic_cast<TDCSHeadingInfo*>(HeadingInfo)->part_no;
                  else
                    part_no=dynamic_cast<TBSMHeadingInfo*>(HeadingInfo)->part_no;
                  if (part_no==1&&EndingInfo->pr_final_part)  //телеграмма состоит из одной части
                  {
                    InsQry.SetVariable("id",FNull);
                    InsQry.SetVariable("merge_key",FNull);
                    if (deleteTlg(tlg_id))
                    {
                      InsQry.Execute();
                      pr_typeb_cmd=true;
                    };
                  }
                  else throw ETlgError("Duplicate part number");
                }
                else
                {
                  errorTlg(tlg_id,"DUP");
                };
              }
              else throw ETlgError("Duplicate part number");
            }
            else throw;
          };
        }
        catch(EXCEPTIONS::Exception E)
        {
          ProgError(STDLOG,"Exception: %s (tlgs.id=%d)",
                       E.what(),TlgQry.FieldAsInteger("id"));
          sendErrorTlg(ERR_CANON_NAME(),OWN_CANON_NAME(),"Exception: %s (tlgs.id=%d)",
                  E.what(),TlgQry.FieldAsInteger("id"));
          errorTlg(tlg_id,"PARS");
        };
      count++;
      TlgQry.Next();
    };
    OraSession.Commit();
    if (pr_typeb_cmd) sendCmd("CMD_TYPEB_HANDLER","HELLO WORLD");
    if (HeadingInfo!=NULL) delete HeadingInfo;
    if (EndingInfo!=NULL) delete EndingInfo;
    if (buf!=NULL) free(buf);
    if (buf2!=NULL) free(buf2);
  }
  catch(...)
  {
    if (HeadingInfo!=NULL) delete HeadingInfo;
    if (EndingInfo!=NULL) delete EndingInfo;
    if (buf!=NULL) free(buf);
    if (buf2!=NULL) free(buf2);
    throw;
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
