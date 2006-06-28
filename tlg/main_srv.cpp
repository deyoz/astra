#ifdef __WIN32__
 #include <winsock2.h>
 #include <winbase.h>
#else
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <unistd.h>
 #include <errno.h>
 #include <tcl.h>
 #include "lwriter.h"
#endif
#include "logger.h"
#include "astra_utils.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
#include "edilib/edi_user_func.h"

using namespace BASIC;
using namespace EXCEPTIONS;

#define NICKNAME "VLAD"


#define WAIT_INTERVAL           10      //seconds
#define TLG_SCAN_INTERVAL       30      //seconds
#define SCAN_COUNT              50      //кол-во разбираемых телеграмм за одно сканирование

static const char* OWN_CANON_NAME=NULL;
static const char* ERR_CANON_NAME=NULL;
#ifdef __WIN32__
static SOCKET sockfd=INVALID_SOCKET;
#else
static int sockfd=-1;
#endif

void process_tlg(void);
static void scan_tlg(void);
int h2h_in(char *h2h_tlg, H2H_MSG *h2h);

#ifdef __WIN32__
int main(int argc, char* argv[])
#else
int main_srv_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
#endif
{
  try
  {
    try
    {
//      OpenLogFile(Tcl_GetString(Tcl_GetVar2Ex(interp,"DEFLOGGER",0,TCL_GLOBAL_ONLY)));
      setLoggingGroup("log1",LOGGER_SYSTEM_SHM,0);

      if ((OWN_CANON_NAME=Tcl_GetVar(interp,"OWN_CANON_NAME",TCL_GLOBAL_ONLY))==NULL||
          strlen(OWN_CANON_NAME)!=5)
        throw Exception("Unknown or wrong OWN_CANON_NAME");

      ERR_CANON_NAME=Tcl_GetVar(interp,"ERR_CANON_NAME",TCL_GLOBAL_ONLY);

      int SRV_PORT;
#ifdef __WIN32__
      SRV_PORT= 8998
#else
      const char *port_tcl=Tcl_GetVar(interp,"SRV_PORT",TCL_GLOBAL_ONLY);
      if (port_tcl==NULL||StrToInt(port_tcl,SRV_PORT)==EOF)
        throw Exception("Unknown or wrong SRV_PORT");
#endif
      OraSession.LogOn((char *)Tcl_GetVar(interp,"CONNECT_STRING",TCL_GLOBAL_ONLY));
      if (init_edifact(interp,false)<0) throw Exception("'init_edifact' error");

#ifdef __WIN32__
      if ((sockfd=socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET)
        throw Exception("'socket' error %d",WSAGetLastError());
#else
      if ((sockfd=socket(AF_INET,SOCK_DGRAM,0))==-1)
        throw Exception("'socket' error %d: %s",errno,strerror(errno));
#endif
      sockaddr_in adr;
      memset(&adr,0,sizeof(adr));
      adr.sin_family=AF_INET;
      adr.sin_addr.s_addr=htonl(INADDR_ANY); //???
      adr.sin_port=htons(SRV_PORT);
#ifdef __WIN32__
      if (bind(sockfd,(struct sockaddr*)&adr,sizeof(adr))==SOCKET_ERROR)
        throw Exception("'bind' error %d",WSAGetLastError());
#else
      if (bind(sockfd,(struct sockaddr*)&adr,sizeof(adr))==-1)
        throw Exception("'bind' error %d: %s",errno,strerror(errno));
#endif
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
#ifdef __WIN32__
        if ((res=select(sockfd+1,&rfds,NULL,NULL,&tv))==SOCKET_ERROR)
          throw Exception("'select' error %d",WSAGetLastError());
#else
        if ((res=select(sockfd+1,&rfds,NULL,NULL,&tv))==-1)
          throw Exception("'select' error %d: %s",errno,strerror(errno));
#endif
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
#ifndef __WIN32__
      ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.Message);
#endif
      throw;
    }
    catch(Exception E)
    {
#ifndef __WIN32__
      ProgError(STDLOG,"Exception: %s",E.Message);
#endif
      throw;
    };
  }
  catch(...) {};
#ifdef __WIN32__
  if (sockfd!=INVALID_SOCKET) closesocket(sockfd);
#else
  if (sockfd!=-1) close(sockfd);
#endif
  try
  {
    OraSession.Rollback();
    OraSession.LogOff();
  }
  catch(...) {};
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
#ifdef __WIN32__
#else
    if ((len = recvfrom(sockfd,(char*)&tlg_in,sizeof(tlg_in),0,
                        (struct sockaddr*)&from_addr,(socklen_t*)&from_addr_len))==-1)
    {
      if (errno==ECONNREFUSED) return;
      throw Exception("'recvfrom' error %d: %s",errno,strerror(errno));
    };
#endif
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
    if (strcmp(tlg_in.Receiver,OWN_CANON_NAME)!=0)
      throw Exception("Unknown telegram receiver %s",tlg_in.Receiver);

    TQuery RotQry(&OraSession);
    RotQry.SQLText=
      "SELECT 1,router_num,ip_port FROM rot\
       WHERE ip_address=:addr AND ip_port=:port\
       AND canon_name=:canon_name\
       union\
       SELECT 2,router_num,ip_port FROM rot\
       WHERE ip_address=:addr AND ip_port=:port\
       ORDER BY 1";
    RotQry.CreateVariable("addr",otString,inet_ntoa(from_addr.sin_addr));
    RotQry.CreateVariable("port",otInteger,ntohs(from_addr.sin_port));
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
               VALUES(tlgs_id.nextval,:sender,:tlg_num,:receiver,:type,NULL,SYSDATE,:tlg_text)";
            TlgInsQry.DeclareVariable("tlg_text",otLong);
            TlgInsQry.SetLongVariable("tlg_text",tlg_body,tlg_len);
            TlgInsQry.Execute();
            // tlg_queue
            TlgInsQry.SQLText=
              "INSERT INTO tlg_queue(id,sender,tlg_num,receiver,type,status,time,ttl)\
               VALUES(tlgs_id.currval,:sender,:tlg_num,:receiver,:type,'PUT',SYSDATE,:ttl)";
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
              break;
            case TLG_CFG_ERR:
              TlgUpdQry.SetVariable("curr_status","PUT");
              //TlgUpdQry.SetVariable("new_status","ERR");
              break;
            case TLG_F_ACK:
              TlgUpdQry.SetVariable("curr_status","SEND");
              //TlgUpdQry.SetVariable("new_status","DONE");
              break;
            case TLG_F_NEG:
              TlgUpdQry.SetVariable("curr_status","SEND");
              TlgUpdQry.SetVariable("new_status","PUT");
              break;
          };
          TlgUpdQry.Execute();
          //если TlgUpdQry.RowsProcessed()==0,значит нарушена последовательность телеграмм
          if (TlgUpdQry.RowsProcessed()==0)
          {
            OraSession.Rollback();
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
#ifdef __WIN32__
        if (sendto(sockfd,(char*)&tlg_out,sizeof(tlg_out)-sizeof(tlg_out.body)+tlg_len,0,
                   (struct sockaddr*)&to_addr,sizeof(to_addr))==SOCKET_ERROR)
          throw Exception("'sendto' error %d",WSAGetLastError());
#else
        if (sendto(sockfd,(char*)&tlg_out,sizeof(tlg_out)-sizeof(tlg_out.body)+tlg_len,0,
                   (struct sockaddr*)&to_addr,sizeof(to_addr))==-1)
          throw Exception("'sendto' error %d: %s",errno,strerror(errno));
    };
#endif
  }
  catch(Exception E)
  {
#ifndef __WIN32__
    ProgError(STDLOG,"Exception: %s",E.Message);
    SendTlg(ERR_CANON_NAME,OWN_CANON_NAME,"Exception: %s",E.Message);
#endif
  };
  OraSession.Commit();
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
      "SELECT tlgs.id,tlgs.tlg_text,tlg_queue.time,ttl\
       FROM tlgs,tlg_queue\
       WHERE tlg_queue.id=tlgs.id AND tlg_queue.type='INB' AND tlg_queue.status='PUT'\
       ORDER BY DECODE(ttl,NULL,1,0),tlg_queue.time+NVL(ttl,0)/86400";
  };

  static TQuery UpdQry(&OraSession);
  if (UpdQry.SQLText.IsEmpty())
  {
    UpdQry.Clear();
    UpdQry.SQLText=
      "DELETE FROM tlg_queue\
       WHERE id= :id AND status='PUT'";
    UpdQry.DeclareVariable("id",otInteger);
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
       "INSERT INTO tlgs_in(id,num,type,point_id,addr,heading,body,ending,\
                           merge_key,time_create,time_receive,time_parse)\
        VALUES(NVL(:id,tlg_in_out__seq.nextval),\
               :part_no,:tlg_type,NULL,:addr,:heading,:body,:ending,\
               :merge_key,:time_create,SYSDATE,NULL)";
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

  int len,count,bufLen=0,buf2Len=0;
  char *buf=NULL,*buf2=NULL,*ph,c;
  TTlgParts parts;
  THeadingInfo HeadingInfo;
  TEndingInfo EndingInfo;
  count=0;
  TlgQry.Execute();
  try
  {
    while (!TlgQry.Eof&&count<SCAN_COUNT)
    {
      UpdQry.SetVariable("id",TlgQry.FieldAsInteger("id"));
      //проверим TTL
      if (!TlgQry.FieldIsNULL("ttl")&&
          (Now()-TlgQry.FieldAsDateTime("time"))*86400>=TlgQry.FieldAsInteger("ttl"))
      {
        UpdQry.Execute();
        if (UpdQry.RowsProcessed()>0)
        {
          Qry.Clear();
          Qry.SQLText="UPDATE tlgs SET error= :error WHERE id= :id";
          Qry.CreateVariable("error",otString,"TTL");
          Qry.CreateVariable("id",otInteger,TlgQry.FieldAsInteger("id"));
          Qry.Execute();
        };
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
          strcpy(EndingInfo.tlg_type,HeadingInfo.tlg_type);
          EndingInfo.part_no=HeadingInfo.part_no;
          ParseEnding(parts.ending,EndingInfo);
          if (parts.heading.p-parts.addr.p>255) throw ETlgError("Address too long");
          if (parts.body.p-parts.heading.p>100) throw ETlgError("Header too long");
          if (parts.ending.p!=NULL&&strlen(parts.ending.p)>20) throw ETlgError("End of message too long");

          if (GetTlgCategory(HeadingInfo.tlg_type)==tcDCS&&
              HeadingInfo.time_create!=0)
          {
            TlgIdQry.SetVariable("id",FNull);
            TlgIdQry.SetVariable("tlg_type",HeadingInfo.tlg_type);
            TlgIdQry.SetVariable("merge_key",HeadingInfo.merge_key);
            if (strcmp(HeadingInfo.tlg_type,"PNL")==0)
            {
              TlgIdQry.SetVariable("min_time_create",HeadingInfo.time_create-2.0/1440);
              TlgIdQry.SetVariable("max_time_create",HeadingInfo.time_create+2.0/1440);
            }
            else
            {
              TlgIdQry.SetVariable("min_time_create",HeadingInfo.time_create);
              TlgIdQry.SetVariable("max_time_create",HeadingInfo.time_create);
            };
            TlgIdQry.Execute();
            if (TlgIdQry.VariableIsNULL("id"))
              InsQry.SetVariable("id",FNull);
            else
              InsQry.SetVariable("id",TlgIdQry.GetVariableAsInteger("id"));
            if (HeadingInfo.part_no!=0)
              InsQry.SetVariable("part_no",(int)HeadingInfo.part_no);
            else
              InsQry.SetVariable("part_no",1);
            InsQry.SetVariable("merge_key",HeadingInfo.merge_key);
          }
          else
          {
            InsQry.SetVariable("id",FNull);
            InsQry.SetVariable("part_no",1);
            InsQry.SetVariable("merge_key",FNull);
          };

          InsQry.SetVariable("tlg_type",HeadingInfo.tlg_type);
          if (HeadingInfo.time_create!=0)
            InsQry.SetVariable("time_create",HeadingInfo.time_create);
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
            UpdQry.Execute();
            if (UpdQry.RowsProcessed()>0)
              InsQry.Execute();
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
                  if (HeadingInfo.part_no==1&&EndingInfo.pr_final_part)  //телеграмма состоит из одной части
                  {
                    InsQry.SetVariable("id",FNull);
                    InsQry.SetVariable("merge_key",FNull);
                    UpdQry.Execute();
                    if (UpdQry.RowsProcessed()>0)
                      InsQry.Execute();
                  }
                  else throw ETlgError("Duplicate part number");
                }
                else
                {
                  UpdQry.Execute();
                  if (UpdQry.RowsProcessed()>0)
                  {
                    Qry.Clear();
                    Qry.SQLText="UPDATE tlgs SET error= :error WHERE id= :id";
                    Qry.CreateVariable("error",otString,"DUP");
                    Qry.CreateVariable("id",otInteger,TlgQry.FieldAsInteger("id"));
                    Qry.Execute();
                  };
                };
              }
              else throw ETlgError("Duplicate part number");
            }
            else throw;
          };
        }
        catch(EXCEPTIONS::Exception E)
        {
#ifndef __WIN32__
          ProgError(STDLOG,"Exception: %s (tlgs.id=%d)",
                       E.Message,TlgQry.FieldAsInteger("id"));
          SendTlg(ERR_CANON_NAME,OWN_CANON_NAME,"Exception: %s (tlgs.id=%d)",
                  E.Message,TlgQry.FieldAsInteger("id"));
#endif
          UpdQry.Execute();
          if (UpdQry.RowsProcessed()>0)
          {
            Qry.Clear();
            Qry.SQLText="UPDATE tlgs SET error= :error WHERE id= :id";
            Qry.CreateVariable("error",otString,"PARS");
            Qry.CreateVariable("id",otInteger,TlgQry.FieldAsInteger("id"));
            Qry.Execute();
          };
        };
      count++;
      TlgQry.Next();
    };
    OraSession.Commit();
    if (buf!=NULL) free(buf);
    if (buf2!=NULL) free(buf2);
  }
  catch(...)
  {
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
