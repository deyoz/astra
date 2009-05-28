//---------------------------------------------------------------------------
/*#include <stdio.h>
#include <signal.h>
#include <fstream>
#include "timer.h"
#include "oralib.h"
#include "exceptions.h"
#include "etick.h"
#include "season.h"
#include "stages.h"
#include "tlg/tlg.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "astra_context.h"
#include "base_tables.h"
#include "astra_service.h"
#include "czech_police_edi_file.h"
#include "telegram.h"*/

/*#include "serverlib/daemon.h"
#include "serverlib/cfgproc.h"
#include "serverlib/posthooks.h"
#include "serverlib/perfom.h"*/
#include "tclmon/tcl_utils.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

const int sleepsec = 25;


int main_empty_proc_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(10);
    OpenLogFile("log1");


    for( ;; )
    {
      sleep( sleepsec );
      ProgTrace( TRACE0, "empty_proc: Next iteration");
    };
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
  };
  return 0;
}




