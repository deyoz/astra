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
#endif
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "daemon.h"

#define NICKNAME "VLAD"
#include "test.h"

using namespace BASIC;
using namespace EXCEPTIONS;

//#define ACK_WAIT_TIME         15*60   //seconds
#define WAIT_INTERVAL           1       //seconds
#define TLG_SCAN_INTERVAL       1      //seconds
#define SCAN_COUNT              30       //кол-во посылаемых телеграмм за одно сканирование

static char* OWN_CANON_NAME=NULL;
#ifdef __WIN32__
static SOCKET sockfd=INVALID_SOCKET;
#else
static int sockfd=-1;
#endif

static void scan_tlg(int tlg_id=-1);
int h2h_out(H2H_MSG *h2h_msg);

#ifdef __WIN32__
int main(int argc, char* argv[])
#else
int main_snd_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
#endif
{
  try
  {
    try
    {
      OpenLogFile("logairimp");
      if ((OWN_CANON_NAME=(char *)Tcl_GetVar(interp,"OWN_CANON_NAME",TCL_GLOBAL_ONLY))==NULL||
          strlen(OWN_CANON_NAME)!=5)
        throw Exception("Unknown or wrong OWN_CANON_NAME");

      int SND_PORT;
#ifdef __WIN32__
      SND_PORT= 8997
#else
      const char *port_tcl=Tcl_GetVar(interp,"SND_PORT",TCL_GLOBAL_ONLY);
      if (port_tcl==NULL||StrToInt(port_tcl,SND_PORT)==EOF)
        throw Exception("Unknown or wrong SND_PORT");
#endif
      ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
        ->connect_db();

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
      adr.sin_addr.s_addr=/*htonl(*/INADDR_ANY/*)*/; //???
      adr.sin_port=htons(SND_PORT);
#ifdef __WIN32__
      if (bind(sockfd,(struct sockaddr*)&adr,sizeof(adr))==SOCKET_ERROR)
        throw Exception("'bind' error %d",WSAGetLastError());
#else
      if (bind(sockfd,(struct sockaddr*)&adr,sizeof(adr))==-1)
        throw Exception("'bind' error %d: %s",errno,strerror(errno));
#endif
/*      fd_set rfds;
      struct timeval tv;
      int res;*/
      time_t scan_time=0;
      for (;;)
      {
/*        FD_ZERO(&rfds);
        FD_SET(sockfd,&rfds);
        tv.tv_sec=5;
        tv.tv_usec=0;
#ifdef __WIN32__
        if ((res=select(sockfd+1,&rfds,NULL,NULL,&tv))==SOCKET_ERROR)
          throw Exception("'select' error %d",WSAGetLastError());
#else
        if ((res=select(sockfd+1,&rfds,NULL,NULL,&tv))==-1)
          throw Exception("'select' error %d: %s",errno,strerror(errno));
#endif*/
#ifdef __WIN32__
        Sleep(WAIT_INTERVAL*1000);
#else
        sleep(WAIT_INTERVAL);
#endif
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
  catch(...) 
  {
    ProgError(STDLOG, "Unknown exception");	
  };
  return 0;
};

void scan_tlg(int tlg_id)
{
  static TQuery TlgQry(&OraSession);
  /*
  static TQuery UpdAllQry(&OraSession);
  if (UpdAllQry.SQLText.IsEmpty())
  {
    UpdAllQry.Clear();
    UpdAllQry.SQLText=
      "UPDATE tlgs SET status='ERR',time=SYSDATE\
       WHERE type IN ('OUTA','OUTB') AND status='PUT' AND SYSDATE-time> :ack_wait_time /86400";
    UpdAllQry.DeclareVariable("ack_wait_time",otInteger);
    UpdAllQry.SetVariable("ack_wait_time",ACK_WAIT_TIME);
  };*/

  static TQuery UpdQry(&OraSession);
  if (UpdQry.SQLText.IsEmpty())
  {
    UpdQry.Clear();
    UpdQry.SQLText=
      "DELETE FROM tlg_queue\
       WHERE id= :id AND status='PUT'";
    UpdQry.DeclareVariable("id",otInteger);
  };

  TQuery Qry(&OraSession);

  //поставим все телеграммы, которые так и не дошли до шлюза в ошибочную очередь
//  UpdAllQry.Execute();
  //считаем все телеграммы, которые еще не отправлены
  struct sockaddr_in to_addr;
  memset(&to_addr,0,sizeof(to_addr));
  AIRSRV_MSG tlg_out;
  int len,count;
  uint16_t ttl;
  H2H_MSG h2hinf;

  TlgQry.Clear();
  if (tlg_id<0)
  {
    TlgQry.SQLText=
      "SELECT tlg_queue.id,tlg_queue.tlg_num,tlg_queue.receiver,\
              SYSDATE,tlg_queue.time,ttl,tlgs.tlg_text,ip_address,ip_port\
       FROM tlgs,tlg_queue,rot\
       WHERE tlg_queue.id=tlgs.id AND\
             tlg_queue.receiver=rot.canon_name(+) AND tlg_queue.sender=rot.own_canon_name(+) AND\
             tlg_queue.sender=:sender AND\
             tlg_queue.type IN ('OUTA','OUTB') AND tlg_queue.status='PUT'\
       ORDER BY tlg_queue.time,tlg_queue.id";
  }
  else
  {
    TlgQry.SQLText=
      "SELECT tlg_queue.id,tlg_queue.tlg_num,tlg_queue.receiver,\
            SYSDATE,tlg_queue.time,ttl,tlgs.tlg_text,ip_address,ip_port\
       FROM tlgs,tlg_queue,rot\
       WHERE tlg_queue.id=tlgs.id AND tlg_queue.id=:id AND\
             tlg_queue.receiver=rot.canon_name(+) AND tlg_queue.sender=rot.own_canon_name(+) AND\
             tlg_queue.sender=:sender AND\
             tlg_queue.type IN ('OUTA','OUTB') AND tlg_queue.status='PUT'";
    TlgQry.CreateVariable("id",otInteger,tlg_id);
  };
  TlgQry.CreateVariable("sender",otString,OWN_CANON_NAME);

  count=0;
  TlgQry.Execute();
  while (!TlgQry.Eof&&count<SCAN_COUNT)
  {
    UpdQry.SetVariable("id",TlgQry.FieldAsInteger("id"));
    try
    {
      if (TlgQry.FieldIsNULL("ip_address")||TlgQry.FieldIsNULL("ip_port"))
        throw Exception("Unknown receiver");
#ifdef __WIN32__
      if ((to_addr.sin_addr.s_addr=inet_addr(TlgQry.FieldAsString("ip_address")))==INADDR_NONE)
        throw Exception("'inet_addr' error");
#else
      if (inet_aton(TlgQry.FieldAsString("ip_address"),&to_addr.sin_addr)==0)
        throw Exception("'inet_aton' error %d: %s",errno,strerror(errno));
#endif
      to_addr.sin_family=AF_INET;
      to_addr.sin_port=htons(TlgQry.FieldAsInteger("ip_port"));

      tlg_out.num=htonl(TlgQry.FieldAsInteger("tlg_num"));
      tlg_out.type=htons(TLG_OUT);
      tlg_out.Sender[5]=0;
      tlg_out.Receiver[5]=0;
      strncpy(tlg_out.Sender,OWN_CANON_NAME,5);
      strncpy(tlg_out.Receiver,TlgQry.FieldAsString("receiver"),5);
      len=TlgQry.GetSizeLongField("tlg_text");
      if (len>(int)sizeof(tlg_out.body)) throw Exception("Telegram too long");
      TlgQry.FieldAsLong("tlg_text",tlg_out.body);
      //проверим TTL
      ttl=0;
      if (!TlgQry.FieldIsNULL("ttl")&&
           (ttl=TlgQry.FieldAsInteger("ttl")-
           (int)((TlgQry.FieldAsDateTime("sysdate")-TlgQry.FieldAsDateTime("time"))*24*60*60))<=0)
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
      {
        //проверим, надо ли лепить h2h
        Qry.Clear();
        Qry.SQLText=
          "SELECT type,qri5,qri6,sender,receiver,tpr,err FROM h2h_tlgs WHERE id=:id";
        Qry.CreateVariable("id",otInteger,TlgQry.FieldAsInteger("id"));
        Qry.Execute();
        if (!Qry.Eof)
        {
          if (len>(int)sizeof(h2hinf.data)-1) throw Exception("Telegram too long. Can't create H2H header");
          strncpy(h2hinf.data,tlg_out.body,len);
          h2hinf.data[len]=0;
          h2hinf.type=Qry.FieldAsString("type")[0];
          h2hinf.qri5=Qry.FieldAsString("qri5")[0];
          h2hinf.qri6=Qry.FieldAsString("qri6")[0];
          strcpy(h2hinf.sndr,Qry.FieldAsString("sender"));
          strcpy(h2hinf.rcvr,Qry.FieldAsString("receiver"));
          strcpy(h2hinf.tpr,Qry.FieldAsString("tpr"));
          strcpy(h2hinf.err,Qry.FieldAsString("err"));
          if (h2h_out(&h2hinf)==0) throw Exception("Can't create H2H header");
          len=strlen(h2hinf.data);
          if (len>(int)sizeof(tlg_out.body)) throw Exception("H2H telegram too long");
          strncpy(tlg_out.body,h2hinf.data,len);
        };
        tlg_out.TTL=htons(ttl);
#ifdef __WIN32__
        if (sendto(sockfd,(char*)&tlg_out,sizeof(tlg_out)-sizeof(tlg_out.body)+len,0,
                   (struct sockaddr*)&to_addr,sizeof(to_addr))==SOCKET_ERROR)
          throw Exception("'sendto' error %d",WSAGetLastError());
#else
        if (sendto(sockfd,(char*)&tlg_out,sizeof(tlg_out)-sizeof(tlg_out.body)+len,0,
                   (struct sockaddr*)&to_addr,sizeof(to_addr))==-1)
          throw Exception("'sendto' error %d: %s",errno,strerror(errno));
#endif
        ProgTrace(TRACE0,"Attempt send telegram (tlg_num=%lu)", ntohl(tlg_out.num));
      };
    }
    catch(Exception E)
    {
      OraSession.Rollback();
      UpdQry.Execute();
      if (UpdQry.RowsProcessed()>0)
      {
        Qry.Clear();
        Qry.SQLText="UPDATE tlgs SET error= :error WHERE id= :id";
        Qry.CreateVariable("error",otString,"SEND");
        Qry.CreateVariable("id",otInteger,TlgQry.FieldAsInteger("id"));
        Qry.Execute();
      };
#ifndef __WIN32__
      ProgError(STDLOG,"Exception: %s (tlgs.id=%d)",
                          E.what(),TlgQry.FieldAsInteger("id"));
#endif
    };
    count++;
    TlgQry.Next();
  };
  OraSession.Commit();
  return;
};

/* Вставляем заголовок IATA Host-to-Host перед телом телеграммы */
/*int h2h_tlg(char *body, char type, char *sndr, char *rcvr, char *tpr, char *err)*/
/* ------------------------------------------------------------------------ */
int h2h_out(H2H_MSG *h2h_msg)
{
  int i=0, head_len, data_len;
  char h2h_head[MAX_H2H_HEAD_SIZE];

	if ( !h2h_msg->sndr	||	strlen(h2h_msg->sndr) > 10	||
		 !h2h_msg->rcvr	||	strlen(h2h_msg->rcvr) > 10	||
		 !h2h_msg->tpr	||	strlen(h2h_msg->tpr) > 19	||
							strlen(h2h_msg->err) > 2 )
	{
		ProgError(STDLOG, "h2h_out(): Can't forming H2H header");
	  return FALSE;
	}

  strcpy(h2h_head, H2H_BEG_STR);	/*	V.\rV	*/
  i += strlen(H2H_BEG_STR);
/*
  h2h_head[i++] = 0x56;				'V'
  h2h_head[i++] = 0x2E;				'.'
  h2h_head[i++] = 0x0D;				'\r'
  h2h_head[i++] = 0x56;				'V'
*/
  h2h_head[i++] = h2h_msg->type;		/*4 QRI1	query / reply /	alone	*/
#if 0
  h2h_head[i++] = 0x45;	/* 0x50; */		/*5	QRI2 'E'	hold protection		*/
#else
  h2h_head[i++] = 'L';	/* 0x50; */		/*5	QRI2 'E'	hold protection		*/
#endif
  h2h_head[i++] = 0x47;					/*6	QRI3 'G'	medium priority		*/
  h2h_head[i++] = 0x2E;					/*7	QRI4 '.'	no flow control		*/
  h2h_head[i++] = h2h_msg->qri5;		/*8 QRI5							*/
  h2h_head[i++] = h2h_msg->qri6;		/*9 QRI6							*/

/*
  if ( h2h_msg->part == 1 )				8opt	QRI5 'V' First seg
	h2h_head[i++] = 0x56;
  else
  if ( h2h_msg->part && !h2h_msg->end )	8opt	QRI5 'T' Intermediate seg
	h2h_head[i++] = 0x54;
  else
  if ( h2h_msg->part && h2h_msg->end )	8opt	QRI5 'U' End seg
	h2h_head[i++] = 0x55;
  else									8opt	QRI5 'W' Only segment
    h2h_head[i++] = 0x57;

  if ( h2h_msg->part )					9opt	QRI6 'A'-'Z' seg
	h2h_head[i++] = (char)(h2h_msg->part + 64);
*/

  h2h_head[i++] = 0x2F;					/*10'/'								*/

  h2h_head[i++] = 0x45;					/*	'E'								*/
  h2h_head[i++] = 0x35;					/*	'5'								*/
  strcpy(&h2h_head[i],h2h_msg->sndr);	/*	Sender							*/
  i += strlen(h2h_msg->sndr);			/*									*/

  h2h_head[i++] = 0x2F;					/*	'/'								*/

  h2h_head[i++] = 0x49;					/*	'I'								*/
  h2h_head[i++] = 0x35;					/*	'5'								*/
  strcpy(&h2h_head[i], h2h_msg->rcvr);	/*	Receiver						*/
  i += strlen(h2h_msg->rcvr);			/*									*/

  h2h_head[i++] = 0x2F;					/*	'/'								*/

  strcpy(&h2h_head[i++], "P");
  strcpy(&h2h_head[i], h2h_msg->tpr);	/*	TPR								*/
  i += strlen(h2h_msg->tpr);			/*									*/
  if ( *h2h_msg->err )					/*									*/
  {										/*									*/
	h2h_head[i++] = 0x2F;				/*	'/'								*/
	strcpy(&h2h_head[i],h2h_msg->err);	/*									*/
	i += strlen(h2h_msg->err);			/*									*/
  }										/*									*/
  h2h_head[i++] = 0x0D;					/*	'\r'*****************************/

  h2h_head[i++] = 0x56;			/*	'V'	*********************************/
  h2h_head[i++] = 0x47;			/*	'G'									*/
  h2h_head[i++] = 0x59;			/*	'Z'									*/
  h2h_head[i++] = 0x41;			/*	'A'									*/
  h2h_head[i++] = 0x0D;			/*	'\r'*********************************/
#ifdef H2H_STX_ETX
  h2h_head[i++] = 0x02;			/* STX	*********************************/
#endif
  h2h_head[i] =	0x00;

  head_len = i;

/*#ifdef H2H_STX_ETX*/
  strcat(h2h_msg->data, "\03");		/*	ETX	*************************/
/*#endif*/
  data_len = strlen(h2h_msg->data);

//ProgTrace(1, STDLOG, "strlen(h2h_head)=%d, head_len=%d", strlen(h2h_head), i);
  if ( head_len + data_len > MAX_TLG_LEN - 1 )
  {
		ProgError(STDLOG, "h2h_out(): Too much size of H2H tlg");
	return FALSE;
  }

  for ( i = data_len; i >= 0; i-- )
	h2h_msg->data[i+head_len] = h2h_msg->data[i];

  memcpy(h2h_msg->data, h2h_head, head_len);

  return TRUE;
}

