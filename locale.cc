#define NICKNAME "KONST"
#define NICKTRACE KONST_TRACE
#include "test.h"
#include "jxtlib.h"
#include "astra_callbacks.h"
#include "ocilocal.h"
#include "oralib.h"

int init_locale(void)
{
#if 0	
  ProgTrace(TRACE1,"init_locale");
  char  c_in[4],stracs[2],strtz[2],our_name[10];
  if(get_param("CENTER_CITY", c_in,sizeof(c_in)-1)<0)
  {
    ProgError(STDLOG,"CENTER_CITY not defined");
    return -1;
  }
  if(get_param("CENTER_NAME", Environ.c_name,sizeof(Environ.c_name)-1)<0)
    strcpy(Environ.c_name,c_in);

  strcpy(stracs,"0");
  if(get_param("USE_ACCESS", stracs,sizeof(stracs)-1)>=0 &&
     (*stracs!='0' && *stracs!='1'))
  {
    ProgError(STDLOG,"USE_ACCESS is invalid");
    return -1;
  }
  Environ.access=atoi(stracs);

  strcpy(strtz,"0");
  if(get_option("USE_TZ", strtz,sizeof(strtz)-1)>=0 &&
     (*strtz!='0' && *strtz!='1'))
  {
    ProgError(STDLOG,"USE_TZ is invalid");
    return -1;
  }
  Environ.use_tz=atoi(strtz);

  strcpy(our_name,"MOW1H");
  get_option("OURNAME", our_name,sizeof(our_name)-1);
  ProgTrace(TRACE3,"OurName=<%s>",our_name);
  
#ifdef AIRIMP
  if(SetEnviron()<0)
    ProgError(STDLOG,"SetEnviron failed");
#endif /* AIRIMP */

  if(Oparse(CU,"begin "
    "locale.init(:ci,:cr,:cl,:off,:c_in,:c_name,:dis,:inv,:our,:p1,:p2,:p3);"
    " end;")
      || Obndrs(CU,":c_in",c_in) 
      || Obndrs(CU,":c_name",Environ.c_name) || 
      Obndri(CU,":ci",Environ.city_i) ||
      Obndrs(CU,":cr",Environ.city_r) || 
      Obndrs(CU,":cl",Environ.city_l) || 
      Obndri(CU,":off",Environ.mainoff_i) || 
      Obndri(CU,":p1",Environ.Ptc[0]) || 
      Obndri(CU,":p2",Environ.Ptc[1]) || 
      Obndri(CU,":p3",Environ.Ptc[2]) || 
      Obndri(CU,":dis",Environ.distr) ||
      Obndri(CU,":inv",Environ.invent) ||
      Obndrs(CU,":our",our_name) ||
      Oexec(CU))
  {
    oci_error(CU);
    return -1;
  }
  ProgTrace(TRACE4,"%d",Environ.city_i);
  if(init_code_list()<0)
  {
    ProgError(STDLOG,"init_code_list");
    return -1;
  }

  if(getenv("SIRENA_INSTANCE"))
    single_run(getenv("SIRENA_INSTANCE"));
#endif /* 0 */
  
  //jxtlib::JXTLib::Instance()->SetCallbacks(new AstraCallbacks()); 
  return 0;
}
