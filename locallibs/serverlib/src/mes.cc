#if HAVE_CONFIG_H
#endif

#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <tcl.h>

#include "tclmon.h"
#include "device.h"
#include "monitor.h"

#define NICKNAME "KSE"
#include "test.h"

CachedQMbl  managQMbl;

/*********************************************************/
void clist_init( int procid )
{ /* инициализация системы CLIST */
  // TODO table?
  if(procid==CLIST_PROCID_MONITOR)
    managQMbl.init(procid, MonitorMinMesBl);
  else if(procid==CLIST_PROCID_SUPERVISOR)
    managQMbl.init(procid, SupervisorMinMesBl);
  else if(procid==CLIST_PROCID_TCLMON)
    managQMbl.init(procid, TclmonMinMesBl);
  else
    managQMbl.init(procid, 0);
}

/*********************************************************/
void clist_uninit( void )
{ /* реинициализация системы CLIST */

  managQMbl.uninit();
}

/*********************************************************/
void show_buf_counts( char* str_mes, int len_mes )
{
  managQMbl.show_buf_counts(str_mes,len_mes);
}

/*********************************************************/
_MesBl_* new_mbl( void )
{
  return managQMbl.alloc_mess();
}

/*********************************************************/
void free_mbl( _MesBl_* p_mbl )
{ 
  managQMbl.free_mess(p_mbl);
}

/**********************************************/
void clear_Qmbl( QueMbl& p_que )
{
  _MesBl_* p_mbl;
  while( (p_mbl = p_que.get_first() ) != NULL)
  {
    free_mbl( p_mbl );
  }
}

/**********************************************/
void copy_mbl( _MesBl_* p_mbl_dest, _MesBl_* p_mbl_src )
{
  p_mbl_dest->_copy(*p_mbl_src);
}

/**********************************************/
void reset_mbl( _MesBl_* p_mbl )
{
  if(p_mbl == NULL)
     return;
   
  p_mbl->_reset();
  p_mbl->set_mes_cmd(TCLCMD_COMMAND);
}


/**********************************************/
int recv_mbl( int fd, _MesBl_* p_mbl )
{
 int n_read;
 int n_need;
 int full_read=0;
 char err_str[_ML_ERRSTR_];
 int Ret;
 
#ifdef KSE_TO_DO 
#endif /* KSE_TO_DO */
  if(p_mbl==NULL)
  {
     ProgError(STDLOG, "recv_mbl: p_mbl=NULL!!!\n");
     return -1;
  }

 int hlen = p_mbl->get_mes_head_size();
 if(p_mbl->cur_len < hlen )
 {
   n_need = n_read = hlen - p_mbl->cur_len;
   Ret =pipe_recv_buf( fd, 
                       ((char *)&(p_mbl->get_message())) + p_mbl->cur_len,
                       &n_read,
                       err_str, sizeof(err_str) );
    
   if(Ret == RETCODE_ERR)
   {
     ProgError(STDLOG, "%s", err_str );
     return -1;
   }
   else if(Ret == RETCODE_NONE)
   {
     return -1;
   }
   full_read += n_read;
   p_mbl->cur_len += n_read;
   if( n_read < n_need)
   {
     return 0;
   }
 }
 if(p_mbl->full_len == 0 )
 {
   p_mbl->full_len = p_mbl->get_full_mes_len();
 }
 
 if(p_mbl->cur_len < p_mbl->full_len )
 {
   n_read = n_need = p_mbl->full_len - p_mbl->cur_len;
   Ret = pipe_recv_buf( fd, 
                        ((char *)&(p_mbl->get_message()))+p_mbl->cur_len,
                        &n_read,
                        err_str, sizeof(err_str) );
    
   if(Ret == RETCODE_ERR)
   {
     ProgError(STDLOG, "%s\n", err_str );
     return -1;
   }
   else if(Ret == RETCODE_NONE)
   {
     if(full_read == 0)
     {
        return -1;
     }
     return 0;
  }

  p_mbl->cur_len += n_read;
  if( n_read < n_need)
  {
     return 0;
  }
 }
return 1;
}

/**********************************************/
int send_mbl( int fd, _MesBl_* p_mbl )
{
 int n_write;
 int n_need;
 char err_str[_ML_ERRSTR_];
 int Ret;
 
#ifdef KSE_TO_DO 
#endif /* KSE_TO_DO */ 

 if(p_mbl==NULL)
   return 1;
 if(p_mbl->cur_len >= p_mbl->full_len )
 {
    return 1;
 }
 n_need = n_write = p_mbl->full_len - p_mbl->cur_len;
 Ret =pipe_send_buf( fd, 
                     ((char *)&(p_mbl->get_message())) + p_mbl->cur_len,
                     &n_write,
                     err_str, sizeof(err_str) );
    
  if(Ret == RETCODE_ERR)
  {
     ProgError(STDLOG, "%s", err_str );
     return -1;
  }
  else if(Ret == RETCODE_NONE)
  {
     return -1;
  }
  p_mbl->cur_len += n_write;
  if( n_write < n_need)
  {
     return 0;
  }
return 1;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

bool _Message_::check_mes_head(char *e_str, int e_len)
  {
    if(get_mes_len() < 0 )
    {
      snprintf(e_str,e_len,"Wrong length of message in head: %d",get_mes_len());
      return false;
    }
    if(get_head_str_len() < 0 )
    {
      snprintf(e_str,e_len,"Wrong length of head_str in head: %d",get_head_str_len());
      return false;
    }
    if(get_head_str_len() > get_head_str_size() )
    {
      snprintf(e_str,e_len,"Wrong length of head_str in head: %d (max=%d)",get_head_str_len(),get_head_str_size());
      return false;
    }
    if(get_ctrls_len() < 0 )
    {
      snprintf(e_str,e_len,"Wrong length of ctrls in head: %d",get_ctrls_len());
      return false;
    }
    if(get_ctrls_len() > get_ctrls_size() )
    {
      snprintf(e_str,e_len,"Wrong length of ctrls in head: %d (max=%d)",get_ctrls_len(),get_ctrls_size());
      return false;
    }
    if(get_mes_len()+get_head_str_len()+get_ctrls_len()>get_text_size())
    {
      snprintf(e_str,e_len,
        "Full message is too long: mes_len(%d)+head_str_len(%d)+ctrls_len(%d)>text_size(%d)",
        get_mes_len(), get_head_str_len(), get_ctrls_len(), get_text_size());
      return false;
    }
    return true;
  }


bool CachedQMbl::show_buf_counts(char *str_mes, int len_mes)
  {
    if(reject_alloc==0 && cur_created==0 && max_created==0)
      return false;
    if(str_mes!=NULL && len_mes>0)
    {
      int l=strlen(str_mes);
      if(l<len_mes)
      {
        snprintf( str_mes+l,len_mes-l,
                 "Buffers %s for process %s:\n",
                 _MesBl_::_class_const_name(),
                 ( procid==CLIST_PROCID_MONITOR)    ? "Monitor":
                 ( procid==CLIST_PROCID_SUPERVISOR) ? "Supervisor":
                 ( procid==CLIST_PROCID_TCLMON )    ? "Tclmon":
                 ( procid==CLIST_PROCID_OTHER )     ? "Other":
                                                      "???");
        str_mes[len_mes-1] = 0;
      }
      l=strlen(str_mes);
      if(l<len_mes)
      {
        snprintf( str_mes+l,len_mes-l,
                  "  current bufs nbr in queue:  %zu\n"
                  "  max limit of queue size:    %zu\n"
                  "  current created  bufs:      %d\n"
                  "  maximum created bufs:       %d\n"
                  "  reject for alloc:           %d\n",
                  QueMbl::get_que_size(),
                  QueMbl::get_max_que_size(),
                  cur_created,
                  max_created,
                  reject_alloc);
        str_mes[len_mes-1] = 0;
      }    
    }
   return true;
  }
