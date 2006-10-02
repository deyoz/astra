#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <math.h>
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
#include "daemon.h"
#include "edi_tlg.h"
#include "edi_msg.h"
#include "posthooks.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"

using namespace BASIC;
using namespace EXCEPTIONS;
//using namespace tlg_process;

#define WAIT_INTERVAL           60      //seconds
#define TLG_SCAN_INTERVAL      600   	//seconds
#define SCAN_COUNT             100      //кол-во разбираемых телеграмм за одно сканирование

static void handle_tlg(void);

int main_edi_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{  
  try
  {
    OpenLogFile("logairimp");  

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();
    if (init_edifact()<0) throw Exception("'init_edifact' error");

    time_t scan_time=0;
    char buf[2];
    for(;;)
    {      	
      if (time(NULL)-scan_time>=TLG_SCAN_INTERVAL)
      {
        handle_tlg();
        scan_time=time(NULL);
      };
      if (waitCmd("CMD_EDI_HANDLER",WAIT_INTERVAL,buf,sizeof(buf)))
      {
        handle_tlg();
        scan_time=time(NULL);
      };  
    }; // end of loop
  }
  catch(EOracleError E)
  {
    ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
  }
  catch(std::exception E)
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

void handle_tlg(void)
{
  static TQuery TlgQry(&OraSession);
  if (TlgQry.SQLText.IsEmpty())
  {
    //внимание порядок объединения таблиц важен!
    TlgQry.Clear();
    TlgQry.SQLText=
      "SELECT tlg_queue.id,tlgs.tlg_text,tlg_queue.time,ttl\
       FROM tlgs,tlg_queue\
       WHERE tlg_queue.id=tlgs.id AND tlg_queue.receiver=:receiver AND\
             tlg_queue.type='INA' AND tlg_queue.status='PUT'\
       ORDER BY tlg_queue.time,tlg_queue.id";
    TlgQry.CreateVariable("receiver",otString,OWN_CANON_NAME());   
  };

  int count,tlg_id;

  count=0;
  TlgQry.Execute();
//  obr_tlg_queue tlg_obr(1); // Класс - обработчик телеграмм
  try
  {
      for(;!TlgQry.Eof && (count++)<SCAN_COUNT; TlgQry.Next(), OraSession.Rollback())
      {
      	  tlg_id=TlgQry.FieldAsInteger("id");
          ProgTrace(TRACE1,"========= %d TLG: START HANDLE =============",tlg_id);                    
          try{
              int len = TlgQry.GetSizeLongField("tlg_text");
              boost::shared_ptr< char > tlg (new (char [len+1]));
              TlgQry.FieldAsLong("tlg_text", tlg.get());
              tlg.get()[len]=0;
              ProgTrace(TRACE5,"TLG_IN: <%s>", tlg.get());
              proc_edifact(tlg.get());
              deleteTlg(tlg_id);
              callPostHooksBefore();
              OraSession.Commit();
              callPostHooksAfter();
              emptyHookTables();
          }
          catch(edi_exception &e)
          {
              OraSession.Rollback();	              	
              ProgTrace(TRACE0,"EdiExcept: %s:%s", e.errCode().c_str(), e.what());	              
              errorTlg(tlg_id,"PARS");
              OraSession.Commit();
          }
          catch(std::exception &e)
          {
              OraSession.Rollback();		
              ProgError(STDLOG, "std::exception: %s", e.what());
              errorTlg(tlg_id,"PARS");
              OraSession.Commit();              
          }
          catch(...)
          {
              OraSession.Rollback();		
              ProgError(STDLOG, "Unknown error");
              errorTlg(tlg_id,"UNKN");
              OraSession.Commit();              
          }
          ProgTrace(TRACE1,"========= %d TLG: DONE HANDLE =============",tlg_id);
      };
  }
  catch(...)
  {      
    ProgError(STDLOG, "Unknown error");    
    throw;
  };
}


#include "ocilocal.h"
using namespace OciCpp;
class EdiHelpSignal:public Posthooks::BaseHook {
    virtual int less2( const BaseHook *p) const;
    int msg1[4];
    char sigtext[1000];
    char ADDR[60];
    public:
        virtual void run();
        EdiHelpSignal(const int *msg,const char *adr,const char *txt)
        {
            msg1[0]=htonl(1);
            memcpy(msg1+1,msg,sizeof(msg[0])*3);
            strcpy(sigtext,txt);
            strcpy(ADDR,adr);
        }
        virtual EdiHelpSignal* clone() const;
};

EdiHelpSignal * EdiHelpSignal::clone() const
{
    return new EdiHelpSignal(*this);
}
int EdiHelpSignal::less2( const BaseHook *p) const
{
    const EdiHelpSignal &e=dynamic_cast<const EdiHelpSignal &> (*p);
    int compare=strcmp(ADDR,e.ADDR);
    if(compare!=0)
        return compare;
    compare=strcmp(sigtext,e.sigtext);
    if(compare!=0)
        return compare;
    return memcmp(msg1,e.msg1,sizeof (msg1));
}

void EdiHelpSignal::run()
{

    struct sockaddr_un addr;
    char buf[1000];
    memset(&addr,0,sizeof(addr));
    memcpy(buf,msg1,sizeof(msg1));
    strcpy(buf+sizeof(msg1),sigtext);
    send_signal_udp(&addr,0,ADDR,buf,sizeof(msg1)+strlen(sigtext)+1);
    ProgTrace(TRACE1,"send: %s",sigtext);
}

//*******************************************
// ФУНКЦИЯ КАК ВРЕМЕННОЕ РЕШЕНИЕ: ДОЛЖНА ЖИТЬ В SERVERLIB! Ёу
//*******************************************
/* Запускает механизм подготовленный save_edi_help_for_levb *
 * С    - курсор которым пользуется функция,                *
 *	и который вызывающая функция не боится испортить    *
 * pult - пульт для которого пришёл ответ                   *
 *----------------------------------------------------------*
 * Возвращает 0 при успешном завершении, <0 - при ошибке    */
int confirm_notify_levb(const char *pult)
{
    int id[3];
    int dummy;
    char address[60];
    char txt[1000];
    unsigned short binlen;
    ProgTrace(TRACE2,"confirm_notify_levb called %s",pult);
    try {
        CursCtl c=make_curs(
                "select intmsgid,address,text from edi_help where pult=:p"
                "  and date1>sysdate-1/1440 order by date1 desc");
        tst();
        c.autoNullStr().
                bind(":p",pult).defFull(&id,sizeof(id),0,&binlen,SQLT_BIN).
                def(address).def(txt).exfet();
        tst();
        if(c.err()==NO_DATA_FOUND){
            ProgTrace(TRACE1,"nothing in edi_help for %s",pult);
            return -1;
        }
        tst();
        if(binlen!=12){
            throw Exception("wrong len");
        }
        tst();
    } catch (Exception &e){
        ProgError(STDLOG,"%s",e.what());
        return -2;
    }
    ProgTrace(TRACE1,"confirm_notify_levb %d %d %d %s",
              id[0],id[1],id[2],
              address);
    binlen=12;
    make_curs("delete from edi_help where pult=:p and intmsgid=:id "
            "and rownum <2").bind(":p",pult).
            bindFull(":id",&id,sizeof(id),0,&binlen,SQLT_BIN).exec();

    CursCtl c2=make_curs(
            "select 1 from edi_help where pult=:p and intmsgid=:id"
            "  and date1>sysdate-1/1440");
    c2.bind(":p",pult).bindFull(
            ":id",id,sizeof id,0,&binlen,SQLT_BIN).
            def(dummy).exfet();

    if (c2.err()==NO_DATA_FOUND){
        ProgTrace(TRACE1,"prepare signal: %.20s",txt);
        sethAfter(EdiHelpSignal(id,address,txt));
    }else{
        ProgTrace(TRACE1,"more records for  %s",pult);
    }
    make_curs("delete from edi_help where date1<sysdate-5/1440").exec();
    return 0;
}
