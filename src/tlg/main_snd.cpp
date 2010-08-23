#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include "base_tables.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "serverlib/query_runner.h"
#include "serverlib/ourtime.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;

//#define ACK_WAIT_TIME         15*60   //seconds
#define WAIT_INTERVAL           10       //seconds
#define TLG_SCAN_INTERVAL       30       //seconds
#define SCAN_COUNT             100       //кол-во посылаемых телеграмм за одно сканирование

static int sockfd=-1;

static void scan_tlg(int tlg_id=-1);
int h2h_out(H2H_MSG *h2h_msg);

int main_snd_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(10);
    InitLogTime(NULL);
    OpenLogFile("logairimp");

    int SND_PORT;
    const char *port_tcl=Tcl_GetVar(interp,"SND_PORT",TCL_GLOBAL_ONLY);
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

    time_t scan_time=0;
    char buf[10];
    for (;;)
    {
      InitLogTime(NULL);
      if (time(NULL)-scan_time>=TLG_SCAN_INTERVAL)
      {
        InitLogTime(NULL);
        base_tables.Invalidate();
        scan_tlg();
        scan_time=time(NULL);
      };
      if (waitCmd("CMD_TLG_SND",WAIT_INTERVAL,buf,sizeof(buf)))
      {
        InitLogTime(NULL);
        base_tables.Invalidate();
        scan_tlg();
        scan_time=time(NULL);
      };
    }; // end of loop
  }
  catch(EOracleError &E)
  {
    ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
  }
  catch(Exception &E)
  {
    ProgError(STDLOG,"Exception: %s",E.what());
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

void scan_tlg(int tlg_id)
{
  time_t time_start=time(NULL);

  static TQuery TlgQry(&OraSession);

  TQuery Qry(&OraSession);

  //поставим все телеграммы, которые так и не дошли до шлюза в ошибочную очередь
//  UpdAllQry.Execute();
  //считаем все телеграммы, которые еще не отправлены
  struct sockaddr_in to_addr;
  memset(&to_addr,0,sizeof(to_addr));
  AIRSRV_MSG tlg_out;
  int len,count,ttl;
  uint16_t ttl16;
  H2H_MSG h2hinf;

  TlgQry.Clear();
  if (tlg_id<0)
  {
    TlgQry.SQLText=
      "SELECT tlg_queue.id,tlg_queue.tlg_num,tlg_queue.receiver,\
              system.UTCSYSDATE AS now,tlg_queue.time,ttl,tlgs.tlg_text,ip_address,ip_port\
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
            system.UTCSYSDATE AS now,tlg_queue.time,ttl,tlgs.tlg_text,ip_address,ip_port\
       FROM tlgs,tlg_queue,rot\
       WHERE tlg_queue.id=tlgs.id AND tlg_queue.id=:id AND\
             tlg_queue.receiver=rot.canon_name(+) AND tlg_queue.sender=rot.own_canon_name(+) AND\
             tlg_queue.sender=:sender AND\
             tlg_queue.type IN ('OUTA','OUTB') AND tlg_queue.status='PUT'";
    TlgQry.CreateVariable("id",otInteger,tlg_id);
  };
  TlgQry.CreateVariable("sender",otString,OWN_CANON_NAME());

  count=0;
  TlgQry.Execute();
  for(;!TlgQry.Eof&&count<SCAN_COUNT;count++,TlgQry.Next(),OraSession.Commit())
  {
    tlg_id=TlgQry.FieldAsInteger("id");
    try
    {
      if (TlgQry.FieldIsNULL("ip_address")||TlgQry.FieldIsNULL("ip_port"))
        throw Exception("Unknown receiver");

      if (inet_aton(TlgQry.FieldAsString("ip_address"),&to_addr.sin_addr)==0)
        throw Exception("'inet_aton' error %d: %s",errno,strerror(errno));
      to_addr.sin_family=AF_INET;
      to_addr.sin_port=htons(TlgQry.FieldAsInteger("ip_port"));

      tlg_out.num=htonl(TlgQry.FieldAsInteger("tlg_num"));
      tlg_out.type=htons(TLG_OUT);
      tlg_out.Sender[5]=0;
      tlg_out.Receiver[5]=0;
      strncpy(tlg_out.Sender,OWN_CANON_NAME(),5);
      strncpy(tlg_out.Receiver,TlgQry.FieldAsString("receiver"),5);
      len=TlgQry.GetSizeLongField("tlg_text");
      if (len>(int)sizeof(tlg_out.body)) throw Exception("Telegram too long");
      TlgQry.FieldAsLong("tlg_text",tlg_out.body);
      //проверим TTL
      ttl=0;
      if (!TlgQry.FieldIsNULL("ttl"))
        ttl=TlgQry.FieldAsInteger("ttl")-
            (int)((TlgQry.FieldAsDateTime("now")-TlgQry.FieldAsDateTime("time"))*24*60*60);
      if (!TlgQry.FieldIsNULL("ttl")&&ttl<=0)
      {
      	errorTlg(tlg_id,"TTL");
      }
      else
      {
      	ProgTrace(TRACE5,"ttl=%d",ttl);
      	ProgTrace(TRACE5,"ttl2=%d",(int)((TlgQry.FieldAsDateTime("now")-TlgQry.FieldAsDateTime("time"))*24*60*60));
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
        ttl16=ttl;
        tlg_out.TTL=htons(ttl16);

        if (sendto(sockfd,(char*)&tlg_out,sizeof(tlg_out)-sizeof(tlg_out.body)+len,0,
                   (struct sockaddr*)&to_addr,sizeof(to_addr))==-1)
          throw Exception("'sendto' error %d: %s",errno,strerror(errno));
        ProgTrace(TRACE0,"Attempt send telegram (tlg_num=%lu)", (unsigned long)ntohl(tlg_out.num));
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
        ProgError(STDLOG,"Exception: %s (tlgs.id=%d)",E.what(),tlg_id);
        errorTlg(tlg_id,"SEND",E.what());
        sendErrorTlg("Exception: %s (tlgs.id=%d)",E.what(),tlg_id);
      }
      catch(...) {};
    };
  };
  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! scan_tlg execute time: %ld secs, count=%d",
                     time_end-time_start,count);
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

