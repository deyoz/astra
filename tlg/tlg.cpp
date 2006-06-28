#include <tcl.h>
#include "logger.h"
#include "edilib/edi_func_cpp.h"

#define NICKNAME "VLAD"

static edi_loaded_char_sets edi_chrset[]=
{
    {"IATA", "\x3A\x2B,\x3F \x27" /* :+,? ' */},
    {"IATB", "\x1F\x1D,\x3F\x1D\x1C" /*Пурга какая-то!*/},
    {"SIRE", "\x3A\x2B,\x3F \"\n"}
};

int init_edifact(Tcl_Interp *interp, bool full_init)
{
    InitEdiLogger(ProgError,WriteLog,ProgTrace);

    if(CreateTemplateMessages(Tcl_GetVar(interp,"CONNECT_STRING",TCL_GLOBAL_ONLY),NULL)){
        return -1;
    }
    if (full_init)
    {
/*      if(InitEdiTypes(edi_msg_proc, edi_proc_sz)){
  	ProgError(STDLOG,"InitEdiTypes filed");
          return -2;
      }

*/
//	       SetEdiTempServiceFunc(FuncAfterEdiParseErr,
//   			    FuncBeforeEdiProc,
//                             FuncAfterEdiProc,
//   			    FuncAfterEdiProcErr,
//   			    FuncBeforeEdiSend,
//   			    FuncAfterEdiSend,
//   			    FuncAfterEdiSendErr);
    };

    if(InitEdiCharSet(edi_chrset, sizeof(edi_chrset)/sizeof(edi_chrset[0]))){
        ProgError(STDLOG,"InitEdiCharSet() failed");
        return -3;
    }

    return 0;
}