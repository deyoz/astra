#ifndef __WIN32__
 #include <unistd.h>
 #include <errno.h>
 #include <tcl.h>
 #include <math.h>
 #include "lwriter.h"
#endif
#include "logger.h"
#include "astra_utils.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
//#include "edifact/obr_tlg_queue.h"

using namespace BASIC;
using namespace EXCEPTIONS;
//using namespace tlg_process;

#define NICKNAME "VLAD"
#include "test.h"

#define WAIT_INTERVAL           10      //seconds
#define TLG_SCAN_INTERVAL	30   	//seconds
#define SCAN_COUNT              10      //кол-во разбираемых телеграмм за одно сканирование

static const char* OWN_CANON_NAME=NULL;
static const char* ERR_CANON_NAME=NULL;

//bool obr_tlg_queue::has_removed;

static void handle_tlg(void);

#ifdef __WIN32__
int main(int argc, char* argv[])
#else
int main_edi_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
#endif
{
  try
  {
    try
    {      
      if ((OWN_CANON_NAME=Tcl_GetVar(interp,"OWN_CANON_NAME",TCL_GLOBAL_ONLY))==NULL||
          strlen(OWN_CANON_NAME)!=5)
        throw Exception("Unknown or wrong OWN_CANON_NAME");

      ERR_CANON_NAME=Tcl_GetVar(interp,"ERR_CANON_NAME",TCL_GLOBAL_ONLY);
      
//      if (init_edifact(interp,true)<0) throw Exception("'init_edifact' error");

      time_t scan_time=0;

      for(;;)
      {
      	sleep(WAIT_INTERVAL);
        if (time(NULL)-scan_time>=TLG_SCAN_INTERVAL)
        {
          handle_tlg();
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
  try
  {
    OraSession.Rollback();
    OraSession.LogOff();
  }
  catch(...) {};
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
      "SELECT tlgs.id,tlgs.tlg_text,tlg_queue.time,ttl\
       FROM tlgs,tlg_queue\
       WHERE tlg_queue.id=tlgs.id AND tlg_queue.type='INA' AND tlg_queue.status='PUT'\
       ORDER BY tlg_queue.time,tlg_queue.id";
       //ORDER BY DECODE(ttl,NULL,1,0),tlg_queue.time+NVL(ttl,0)/86400";
  };

  int count;

  count=0;
  TlgQry.Execute();
//  obr_tlg_queue tlg_obr(1); // Класс - обработчик телеграмм
  try
  {
    for(;!TlgQry.Eof&&count<SCAN_COUNT;TlgQry.Next(),OraSession.Rollback())
    {
/*	try{
	    tlg_obr.init();                   //инициализация
	    tlg_obr.next_from_queue();        //следущий на обработку
	    tlg_obr.classify();               //что за телеграмма?
	    tlg_obr.trace_tlg();              //печать
	    tlg_obr.obr_tlg();                //вызов обработчика
	    tlg_obr.trace_tlg_out();          //чем ответим
	    tlg_obr.insert_in2_out_queue();   //ответ на выход
	    tlg_obr.commit();                 //сохраним изменения БД
	}

	catch(EXCEPTIONS::TlgObrNoTlg &){
	    ProgTrace(TRACE5,"Before SLEEP(60):%d", tlg_obr.get_obr_num());
	    tcl_mode_sleep(-1,-1,0,60,0);
	}
	catch(EXCEPTIONS::Exception &e){
	    ProgError(STDLOG,e.what());
	    try {
		tlg_obr.rall_back2sp();
		tlg_obr.move_into_bad_queue("CRITICAL ERROR!");
		tlg_obr.commit();
	    }
	    catch(...){
		ProgError(STDLOG, " ERROR !!! ");
	    }
	    ProgTrace(TRACE5,"Before SLEEP(1):%d", tlg_obr.get_obr_num());
	    tcl_mode_sleep(-1,-1,0,1,0);
	}*/
    };
  }
  catch(...)
  {
    throw;
  };
}

