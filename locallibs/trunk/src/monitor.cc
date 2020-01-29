#if HAVE_CONFIG_H
#endif

#include <stdlib.h>
#include <syslog.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <errno.h>
#include <signal.h>
#include <tcl.h>
#include <stdexcept>
#include <fstream>
#include "object.h"
#include "msg_queue.h"
#include "tclmon.h"
#include "device.h"
#include "monitor.h"
#include "moncmd.h"
#include "tcl_utils.h"
#include "exception.h"

#include "msg_framework.h"
#include "lwriter.h"
#include <stdio.h>
#include <sys/shm.h>
#include <errno.h>

#define NICKNAME "KSE"
#include "test.h"
#include "log_manager.h"
#include "slogger.h"

#define TO_SHOW 0
#define TO_TCP 0
#define TO_CONTROL 0

/* TEST_MONITOR_SUPERVISOR_PIPE - define для тестирования коммуникационного канала (pipe) между процессами monitor и supervisor */
#define TEST_MONITOR_SUPERVISOR_PIPE
#ifdef TEST_MONITOR_SUPERVISOR_PIPE
  #define SETFLAG_TO_MONITOR 50
#endif /* TEST_MONITOR_SUPERVISOR_PIPE */

#include <iostream>
static CsaControl* csaControl;
static int finish;

template <class T> void _input_message_to_out_que(_MesBl_ *p_mbl, T &v);
template <class T> int receive_message(T &v);
template <class T> char *get_head_to_monit(T &v);
int razbor_message_len( _MesBl_ *p_mbl, _MesRecv_ &p_mr, char *e_str, int e_len );
static _MesBl_ *create_mbl_with_cmd(const char *text, int fd, int cmd);
static int razbor_message_tail( _MesBl_ *p_mbl, _MesRecv_ &p_mr, _DevFormat_ &in_format, char *e_str, int e_len);
void recode_mes_to_visual(char *src_str, int src_len, char *dest_str, int dest_len, int len4hex=0);

class MonitorDevice
{
public:

   virtual void dev_close(enum DEV_CLOSE_REASON cl_fl) 
   {
     switch(cl_fl)
     {
       case CLOSE_AFTER_SEND:
       case CLOSE_AFTER_RECV:
       case CLOSE_AFTER_POLLHUP:
       case CLOSE_AFTER_POLLERR:
         break;
       default:/*??*/
         break;
     }
   };
   virtual int dev_pollin_event(_DeviceKey_ &p_mon) {return RETCODE_OK;};   
   virtual int dev_pollout_event(_DeviceKey_ &p_mon) {return RETCODE_OK;};   
   virtual int dev_pollhup_event(_DeviceKey_ &p_mon) {dev_close(CLOSE_AFTER_POLLHUP); return RETCODE_OK;};   
   virtual int dev_pollerr_event(_DeviceKey_ &p_mon) {dev_close(CLOSE_AFTER_POLLERR); return RETCODE_OK;};   
   virtual int message_fully_received(_MesBl_ *p_mbl) { free_mbl(p_mbl); return RETCODE_OK; };
   virtual ~MonitorDevice() {}
};

/****************************************************************/
CsaControl::CsaControl(uint16_t key)
{
    int getFlag = 00600;
    int atFlag = 0;

    id_ = shmget(key, sizeof(_CsaControl_), getFlag);
    if (id_ < 0) {
        throw std::runtime_error("Can't get id shared memory");
    }
    csa_ = static_cast<_CsaControl_*>(shmat(id_, 0, atFlag));
    if (reinterpret_cast<void*>(-1) == csa_) {
        throw std::runtime_error("Can't attach to shared memory");
    }
}
/****************************************************************/
CsaControl::CsaControl(_CsaControl_* rawCsa):
    csa_(rawCsa),
    id_(-1)
{
}
/****************************************************************/
CsaControl::~CsaControl()
{
    if (0 < id_) {
        shmdt(csa_);
    }
}
/****************************************************************/
void constructCsaControl(uint16_t key)
{
    static int buf[sizeof(CsaControl) / sizeof(int) + 1];

    csaControl = new (reinterpret_cast<void*>(buf)) CsaControl(key);
}
/****************************************************************/
void constructCsaControl(_CsaControl_* rawCsa)
{
    static int buf[sizeof(CsaControl) / sizeof(int) + 1];

    csaControl = new (reinterpret_cast<void*>(buf)) CsaControl(rawCsa);
}
/****************************************************************/

namespace monitorControl {

class UdpServerObrzap : public _Udp::UdpServer
{
public:
  bool is_init;
  UdpServerObrzap() : UdpServer()
  {
    is_init=false;
    in_format.set_format( MF_LEN, NULL);
  }
  virtual void config_serv(_UdpServAddr_ *p_udps_addr)
  {
    UdpServer::config_serv( p_udps_addr );
    is_init=true;
  }
  int prepare_cli(_UdpServAddr_ *p_udps_addr);

  int timeout() { return UdpServer::timeout_cli(); }
  virtual void close_serv() 
  {   
    if(is_init) 
      UdpServer::close_serv(); 
    is_init=false;
  };
 virtual void dev_close(enum DEV_CLOSE_REASON cl_fl)
 {
   switch(cl_fl)
   {
     case CLOSE_AFTER_SEND: // не должно такого быть
     case CLOSE_AFTER_RECV: //???
       break;
     case CLOSE_AFTER_POLLHUP:
     case CLOSE_AFTER_POLLERR:
       close_serv();
       break;
     default:/*??*/
       break;
   }
 } 
};

int UdpServerObrzap::prepare_cli(_UdpServAddr_ *p_udps_addr)
{
  if(!is_init)
  {
     preinit( );
     config_serv( &(csaControl->csa()->udps_addr) );
     is_init = 1;
  }
  if( !is_connected() )
  {
     timeout();
  }
  if( !is_connected() )
  {
     return RETCODE_ERR;
  }

return RETCODE_OK;  
}

int is_mes_control(int type, const char *head, int hlen, const char *buf, int blen);
int is_log_control(const char *head, int hlen);
int compare_mes_control(_ControlMesItem_ &p_mes_ctrl, const char *text, int tlen);
int send_mes_control( _CtrlsFd_ &s_ctrls_fd, int type, const char *head, int hlen, 
  const char *buf, int blen, int blen_max, char *dop_head);

}; // namespace monitorControl 

namespace monitorMain {
 
int TcpLogFlag=0;

 struct pollfd descr_tab[_MaxMonTable_];
 _DeviceKey_    mon_table[_MaxMonTable_];
 int n_table=0;
  
//----------------------------
//---- class UdpServerMonitor
//----------------------------
class UdpServerMonitor : public _Udp::UdpServer, public _DeviceKey_, public MonitorDevice
{
public:
  _MesBl_ *p_in_mbl;
  
  UdpServerMonitor() : UdpServer(), _DeviceKey_(DEV_UDPSERV,1,-1)
  {
    p_in_mbl=NULL;
  };
  
  _MesBl_ *get_mbl_in()
  {
    _MesBl_ *p_mbl=p_in_mbl;
    p_in_mbl=NULL;
    return p_mbl;
  }
  void put_mbl_in(_MesBl_ *p_mbl) { p_in_mbl=p_mbl; }

  virtual bool config_serv(Tcl_Interp *interp, Tcl_Obj *obj_arr );
  int timeout();
  virtual void close_serv()
  {
    set_delete_fd_from_table( in_fd() );
  
    UdpServer::close_serv();
  }
  
  // MonitorDevice
  virtual void dev_close(enum DEV_CLOSE_REASON cl_fl)
  {
    switch(cl_fl)
    {
      case CLOSE_AFTER_SEND: // не должно такого быть
      case CLOSE_AFTER_RECV: // ???
        break;
      case CLOSE_AFTER_POLLHUP:
      case CLOSE_AFTER_POLLERR:
        close_serv();
        break;
      default:/*??*/
        break;
    }
  }
  virtual int dev_pollin_event(_DeviceKey_ &p_mon)
  {
    int Ret=receive_message(*this);
    return Ret;
  };   
  virtual int dev_pollout_event(_DeviceKey_ &p_mon) {return RETCODE_OK;/*????*/};   
  virtual int dev_pollhup_event(_DeviceKey_ &p_mon) {return RETCODE_OK;/*????*/};   
  virtual int dev_pollerr_event(_DeviceKey_ &p_mon) {return RETCODE_OK;/*????*/};   
  virtual int message_fully_received(_MesBl_ *p_mbl);
};

bool UdpServerMonitor::config_serv(Tcl_Interp *interp, Tcl_Obj *obj_arr )
{
  Tcl_Obj *pobj_par;
  int port;
  _UdpServAddr_ p_udps_addr;
  
  pobj_par = Tcl_ObjGetVar2(interp,obj_arr,Tcl_NewStringObj("UDP_FILE",-1),
                           TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG);
  if(pobj_par == NULL)
  {
    ProgError(STDLOG,
           "%s",
           Tcl_GetString(Tcl_GetObjResult(interp)));
    Tcl_ResetResult(interp);
    return false;
  }
  port = atoi( Tcl_GetString(pobj_par) );
  if(port != 0)
  {
    p_udps_addr.addr_flag = UDP_ADDR_IN;
    p_udps_addr.uaddr_in.sin_family=AF_INET;
    p_udps_addr.uaddr_in.sin_port   = htons(port);
    p_udps_addr.uaddr_in.sin_addr.s_addr = INADDR_ANY;
  }
  else
  {
    p_udps_addr.addr_flag = UDP_ADDR_UN;
    strcpy(p_udps_addr.uaddr_un.sun_path,Tcl_GetString(pobj_par));
    p_udps_addr.uaddr_un.sun_family=AF_UNIX;
  }

  UdpServer::config_serv(&p_udps_addr);
  memcpy( (char *)&(csaControl->csa()->udps_addr),
          (char *)(UdpServer::get_udps_addr()),
          sizeof(_UdpServAddr_) );
return true;
}

int UdpServerMonitor::timeout()
{
 int ret=RETCODE_OK;
  if(is_not_init())
  {
    if( ! open_socket() )
    {
      return RETCODE_ERR;
    }
    int Ret = set_sock_opt();
    if(Ret != RETCODE_OK)
    {
      close_serv();
      return Ret;
    }
    Ret = bind_socket();
    if(Ret != RETCODE_OK)
    {
       close_serv();
       return Ret;
    }
    if(ret==RETCODE_OK)
    {
       add_fd_to_table( in_fd(), DEV_UDPSERV, POLLIN, dev_num,dev_line);
    }
  }
return ret;  
}
//**** END class UdpServerMonitor

//----------------------------
//---- class TcpLineMonitor
//----------------------------
class TcpLineMonitor : public _Tcp::_TcpLine_ , public _DeviceKey_, public MonitorDevice
{
public :
 typedef _Tcp::_TcpLine_ SubTypeL;
 
 _MesBl_ *p_in_mbl;
 QueMbl  que_out;
 _BufSend_ buf_send;

  TcpLineMonitor(int a_dev_num, int a_dev_line) :  
       _Tcp::_TcpLine_(a_dev_num, a_dev_line), _DeviceKey_(DEV_TCPLINE,a_dev_num, a_dev_line), buf_send()
  {
    p_in_mbl = NULL;
    que_out.init( TCP_MAX_QUE );  
  }
  
  void send_appl();
  virtual int set_connection(int fd, struct sockaddr_in &ad,_DevFormat_ &in_format,_DevFormat_ &out_format);
   
  virtual void close_line()
  {
    if(in_fd() != -1)
    {
      set_delete_fd_from_table(in_fd());
      close_control_for_fd(in_fd());
    }
    clear_Qmbl(que_out);
    buf_send._reset();
    if( p_in_mbl != NULL )
    {
      free_mbl(p_in_mbl);
      p_in_mbl=NULL;      
    }
    SubTypeL::close_line();
  }

  _MesBl_ *get_mbl_in()
  {
    _MesBl_ *p_mbl=p_in_mbl;
    p_in_mbl=NULL;
    return p_mbl;
  }
  
  void put_mbl_in(_MesBl_ *p_mbl) 
  { 
    p_in_mbl=p_mbl; 
  }
  
  QueMbl *get_que_out() { return &que_out;}
  bool put_to_que_out(_MesBl_ *p_mbl) { return que_out.put_last(p_mbl); }

  static void show_header( char *res_str, int res_len);

  void show_line( int show_fl,char *res_str, int res_len);
  int send_answer_by_text(_MesBl_ *p_mbl, const char *answer_str, int retc);
  int recv_help_mes(Tcl_Obj *p_list_obj, int n_obj, _MesBl_ *p_mbl);
  int recv_show_mes(Tcl_Obj *p_list_obj, int n_obj, _MesBl_ *p_mbl);
  int recv_control_mes(Tcl_Obj *p_list_obj, int n_obj, _MesBl_ *p_mbl);
  // MonitorDevice
  virtual void dev_close(enum DEV_CLOSE_REASON cl_fl) 
  {
    switch(cl_fl)
    {
      case CLOSE_AFTER_SEND:
      case CLOSE_AFTER_RECV:
      case CLOSE_AFTER_POLLHUP:
      case CLOSE_AFTER_POLLERR:
        close_line();
        break;
      default:/*??*/
        close_line();
        break;
    }
  };
  virtual int dev_pollin_event(_DeviceKey_ &p_mon)   
  {
    int Ret=receive_message(*this);
    return Ret;
  };   
  virtual int dev_pollout_event(_DeviceKey_ &p_mon);   
  virtual int message_fully_received(_MesBl_ *p_mbl);
  
};

void TcpLineMonitor::send_appl()
{
    char s[] = "";
    if(_MesBl_ *p_mbl=create_mbl_with_cmd(s, sockfd, TCLCMD_COMMAND))
        _input_message_to_out_que(p_mbl, *this);
}

int TcpLineMonitor::set_connection( int fd, struct sockaddr_in &ad,_DevFormat_ &in_format,_DevFormat_ &out_format )
{
 int ret=SubTypeL::set_connection( fd, ad, in_format, out_format );
  if(ret==RETCODE_OK)
  {
    add_fd_to_table( sockfd, DEV_TCPLINE, POLLIN, dev_num, dev_line );
  }
return ret;
}

void TcpLineMonitor::show_header( char *res_str, int res_len )
{
  strncat(res_str, "NO        FD               IP  PORT QUE(MAX) BufSend\n", res_len);
}

void TcpLineMonitor::show_line( int show_fl, char *res_str, int res_len )
{
 char str_tmp[100];
  if( is_connected() )
  {
    snprintf( str_tmp, sizeof(str_tmp),
              "%-5d %6d %16.16s %5s %3zu(%3zu) %d/%d\n",
               dev_line,
               sockfd,
               get_ip_str(),
               get_port_str(),
               que_out.get_que_size(),
               que_out.get_max_que_size(),
               buf_send.get_sended_len(),
               buf_send.get_filled_len() );
    str_tmp[sizeof(str_tmp)-1] =0;
    strncat(res_str, str_tmp, res_len);
  }
}

//**** END class TcpLineMonitor

static size_t writeListenPort( const std::string& str )
{
  std::ofstream f(readStringFromTcl("MONITOR_PORT_FILE").c_str(), std::ios_base::out);
  f << str << std::endl;

  return str.size();
}

//----------------------------
//---- class TcpServMonitor
//----------------------------
class TcpServMonitor : public _Tcp::_TcpServ_<TcpLineMonitor> , public _DeviceKey_, public MonitorDevice
{
 public:
 typedef _Tcp::_TcpServ_<TcpLineMonitor> SubType;

  TcpServMonitor(int a_dev_num=-1) : SubType(a_dev_num), _DeviceKey_(DEV_TCPSERV,a_dev_num,-1)
  {
  }

  virtual char *get_id() { return SubType::get_id(); }
  virtual char *get_id( char *s, int len )
  {
    snprintf( s, len-1, "{TcpServ_%d}: ", dev_num );
    s[len-1]=0;
    return s;
  }
  static void show_diap( _TcpDiap_ &p_diap, const char *head_str, char *res_str, int res_len );
  void show_serv( int show_fl, char *res_str, int res_len );
 
  bool config_serv( Tcl_Interp *interp, Tcl_Obj *obj_arr );
  virtual bool listen_socket();
  virtual bool accept_socket();
  virtual void close_single_serv( short flag );

  // MonitorDevice
  virtual void dev_close( enum DEV_CLOSE_REASON cl_fl )
  {
    switch(cl_fl)
    {
      case CLOSE_AFTER_SEND: // не должно такого быть
      case CLOSE_AFTER_RECV:
        break;
      case CLOSE_AFTER_POLLHUP:
      case CLOSE_AFTER_POLLERR:
        close_single_serv(TCP_FL_WLINES);
        break;
      default:/*??*/
        break;
    }
  } 
  virtual int dev_pollin_event(_DeviceKey_ &p_mon) 
  { 
    accept_socket(); 
    return RETCODE_OK; 
  };
  virtual int dev_pollout_event( _DeviceKey_ &p_mon ) { return RETCODE_OK; };   
};

void TcpServMonitor::show_diap( _TcpDiap_ &p_diap, const char *head_str,char *res_str, int res_len )
{
 char str_tmp[100];
  if(p_diap.n_diap > 0)
  {
     strncat(res_str, head_str, res_len);
     for(int i=0;i<p_diap.n_diap;i++)
     {
        if(memcmp((char *)&(p_diap.diap[i].dot_beg),
                  (char *)&(p_diap.diap[i].dot_end),
                  sizeof(_IpDotStruct_)) == 0 )
        {
           snprintf(str_tmp,sizeof(str_tmp),
                    "  %d.%d.%d.%d\n",
                    p_diap.diap[i].dot_beg.dot[0], p_diap.diap[i].dot_beg.dot[1],
                    p_diap.diap[i].dot_beg.dot[2], p_diap.diap[i].dot_beg.dot[3]);
        }
        else
        {
           snprintf(str_tmp,sizeof(str_tmp),
                    "  %d.%d.%d.%d:%d.%d.%d.%d\n",
                    p_diap.diap[i].dot_beg.dot[0], p_diap.diap[i].dot_beg.dot[1],
                    p_diap.diap[i].dot_beg.dot[2], p_diap.diap[i].dot_beg.dot[3],
                    p_diap.diap[i].dot_end.dot[0], p_diap.diap[i].dot_end.dot[1],
                    p_diap.diap[i].dot_end.dot[2], p_diap.diap[i].dot_end.dot[3]);
        }
        str_tmp[sizeof(str_tmp)-1] =0;
        strncat(res_str, str_tmp, res_len);
     }
  }
}

void TcpServMonitor::show_serv( int show_fl,char *res_str, int res_len )
{
 char str_tmp[100];
  snprintf( str_tmp, sizeof(str_tmp),
            "TcpServer(%d) (%.16s:%s)\n",
            dev_num,
            get_ip_str(),
            get_port_str() );
  str_tmp[sizeof(str_tmp)-1] =0;
  strncat(res_str, str_tmp, res_len);

  TcpLineMonitor::show_header( res_str, res_len);
  for(size_t i=0;i<tcp_lines.size();i++)
  {
    tcp_lines[i].show_line( show_fl,res_str, res_len);
  }
  
  if(show_fl == 1)
  {
    show_diap(tcp_enable_diap, "Enable IPs:\n",res_str, res_len);
    show_diap(tcp_disable_diap, "Disable IPs:\n",res_str, res_len);
  }
  strncat(res_str, "\n", res_len);
}

bool TcpServMonitor::config_serv(Tcl_Interp *interp, Tcl_Obj *obj_arr )
{
 char err_str[_MAX_MES_LEN_];

 Tcl_Obj *pobj_par=Tcl_ObjGetVar2(interp,obj_arr,Tcl_NewStringObj("TCP_HOST",-1),
         TCL_GLOBAL_ONLY) ;
 if(!pobj_par)
 {
   np.ip.a.s_addr = INADDR_ANY;
 }
 else
 {
   char *p_str = Tcl_GetString(pobj_par);
   if(p_str == NULL)
   {
     ProgError(STDLOG, "%s(TCP_HOST): %s",
          Tcl_GetString(obj_arr), Tcl_GetString(Tcl_GetObjResult(interp)) );
     return false;
   }
   if( get_ip_to_ip( p_str, &(np.ip), 
                     err_str, sizeof(err_str) ) != RETCODE_OK )
   {
     ProgError(STDLOG,"%s(TCP_HOST): %s\n",
          Tcl_GetString(obj_arr),err_str);
     return false;
   }
 }

 pobj_par=Tcl_ObjGetVar2(interp,obj_arr,Tcl_NewStringObj("TCP_PORT",-1), TCL_GLOBAL_ONLY) ;
 if(!pobj_par)
 {
   ProgError(STDLOG,"%s(TCP_PORT): not defined",Tcl_GetString(obj_arr));
   return false;
 }

 int port;
 if(TCL_OK!=Tcl_GetIntFromObj(interp,pobj_par,&port))
 {
   ProgError(STDLOG, "%s(TCP_PORT): %s",
        Tcl_GetString(obj_arr), Tcl_GetString(Tcl_GetObjResult(interp)) );
    return false;
 }

 static const size_t callOnce = writeListenPort(std::string(Tcl_GetString(pobj_par)));
 if(callOnce)
     ProgTrace(TRACE5, "%s :: callOnce is %zu already", __func__, callOnce);

 np.port = htons(port);

 pobj_par=Tcl_ObjGetVar2(interp,obj_arr,Tcl_NewStringObj("TCP_IN_TAIL",-1), TCL_GLOBAL_ONLY) ;
 if( pobj_par )
 {
   char *p_str = Tcl_GetString(pobj_par);
   in_format.set_tail(p_str);
 }
 pobj_par=Tcl_ObjGetVar2(interp,obj_arr,Tcl_NewStringObj("TCP_OUT_TAIL",-1), TCL_GLOBAL_ONLY) ;
 if( pobj_par )
 {
   char *p_str = Tcl_GetString(pobj_par);
   out_format.set_tail(p_str);
 }

 if(dev_num == 0)
 {
   pobj_par=Tcl_ObjGetVar2(interp,obj_arr,Tcl_NewStringObj("TCP_LOG",-1), TCL_GLOBAL_ONLY) ;
   if(!pobj_par)
   {
     TcpLogFlag = 0;
   }
   else
   {
     int i_log;
     char *p_str = Tcl_GetString(pobj_par);
     if(p_str == NULL)
     {
       ProgError(STDLOG,"%s(TCP_LOG): %s",
            Tcl_GetString(obj_arr), Tcl_GetString(Tcl_GetObjResult(interp)) );
     }
     else
     {
       if(TCL_OK!=Tcl_GetIntFromObj(interp,pobj_par,&i_log))
       {
         ProgError(STDLOG,"%s(TCP_LOG): %s",
              Tcl_GetString(obj_arr), Tcl_GetString(Tcl_GetObjResult(interp)) );
       }
       else
       {
         TcpLogFlag = (i_log>0) ? 1 : 0 ;
       }
     }
   }
 }

 pobj_par=Tcl_ObjGetVar2(interp,obj_arr,Tcl_NewStringObj("TCP_EN_IP",-1),TCL_GLOBAL_ONLY) ;
 if(pobj_par)
 {
   int objc_sub;
   if( TCL_OK == Tcl_ListObjLength(interp, pobj_par,&objc_sub))
   {
     _TcpDiap_ *p_e_diap;
     p_e_diap = &(tcp_enable_diap);
     for(int i=0;i<objc_sub;i++)
     {
       Tcl_Obj *pobj_sub;
       Tcl_ListObjIndex(interp,pobj_par, i,&pobj_sub);
       char *p_str = Tcl_GetString(pobj_sub);
       if(p_e_diap->n_diap >= MAX_TCP_ENABLE_DIAP)
       {
         ProgTrace(TRACE1,"%s(TCP_EN_IP): Too many params",
              Tcl_GetString(obj_arr) );
         break;
       }
       int Ret = ip_str_to_ip_diap(p_str,&(p_e_diap->diap[p_e_diap->n_diap]), 
                   err_str, sizeof(err_str));
       if(Ret == RETCODE_ERR)
       {
         ProgError(STDLOG,"%s(TCP_EN_IP): %s\n", Tcl_GetString(obj_arr), err_str );
       }
       else if(Ret == RETCODE_OK)
       {
         p_e_diap->n_diap++;
       }
     }
   }
 }

 pobj_par=Tcl_ObjGetVar2(interp,obj_arr,Tcl_NewStringObj("TCP_DIS_IP",-1), TCL_GLOBAL_ONLY) ;
 if(pobj_par)
 {
   int objc_sub;
   if( TCL_OK == Tcl_ListObjLength(interp, pobj_par,&objc_sub) )
   {
     _TcpDiap_ *p_d_diap;
     p_d_diap = &(tcp_disable_diap);
     for(int i=0;i<objc_sub;i++)
     {
       Tcl_Obj *pobj_sub;
       Tcl_ListObjIndex(interp,pobj_par, i,&pobj_sub);
       char *p_str = Tcl_GetString(pobj_sub);
       if(p_d_diap->n_diap >= MAX_TCP_ENABLE_DIAP)
       {
         ProgTrace(TRACE1,"%s(TCP_DIS_IP): Too many params",Tcl_GetString(obj_arr) );
         break;
       }
       int Ret = ip_str_to_ip_diap(p_str, &(p_d_diap->diap[p_d_diap->n_diap]), 
               err_str, sizeof(err_str));
       if(Ret == RETCODE_ERR)
       {
         ProgError(STDLOG,"%s(TCP_EN_IP): <%s>\n",
              Tcl_GetString(obj_arr),err_str );
       }
       else if(Ret == RETCODE_OK)
       {
         p_d_diap->n_diap++;
       }
     }
   }
 }
return true;
}

bool TcpServMonitor::accept_socket()
{
 struct sockaddr_in ad;
 int    new_sock=-1;
 char   rem_addr_str[20];
 char   err_str[_MAX_MES_LEN_];
 
  bool ret = _TcpStruct_::accept_socket(new_sock, ad);
  if( !ret  || new_sock==-1)
    return ret;

/* ACCEPT OK */
  snprintf( rem_addr_str, sizeof(rem_addr_str), "%.16s",
#if defined __SCO__ || defined __UNIXWARE__
            inet_ntoa( ad.sin_addr ) );
#else /* !defined __SCO__ && !defined __UNIXWARE__*/
            inet_ntoa( (struct in_addr)( ad.sin_addr ) ) );
#endif /* __SCO__ && __UNIXWARE__ */

  if( ! is_enable_ip(rem_addr_str, err_str, sizeof(err_str)) )  
  {
    ProgError(STDLOG,"{%.16s:%d}: Accept connection (accept): %s",
         rem_addr_str,
         ntohs( ad.sin_port ),
         err_str );
    close( new_sock );
    return false;
  }

  // Ищем свободную линию
  int free_dev_line=-1;
  for(size_t i=0;i<tcp_lines.size();i++)
  {
    if(tcp_lines[i].is_not_inet())
    {
      free_dev_line=(int)i;
      break;
    }
  }
  // Свободных линий нету, попытаемся создать новую
  if(free_dev_line==-1)
  {
    if(tcp_lines.size()<_MAX_TCP_LINES_)
    {
      free_dev_line=(int)tcp_lines.size();
      TcpLineMonitor tcp_lin(dev_num,free_dev_line);
      tcp_lines.push_back(tcp_lin);
    }
  }
  if(free_dev_line>=0 && free_dev_line<(int)tcp_lines.size())
  {
    TcpLineMonitor &cur_line=tcp_lines[free_dev_line];
    if( cur_line.set_connection(new_sock, ad, get_in_format(), get_out_format() )==RETCODE_OK )
    {
      cur_line.send_appl();
    }
  }
  else
  {
    ProgError(STDLOG,
         "%sAccept connection (accept) from %s, but NO free lines: close connection",
         get_id(), get_ip_port_str(ad) );
    close(new_sock);
  }
  
return true;
}
 
void TcpServMonitor::close_single_serv(short flag)
{
  set_delete_fd_from_table(sockfd);
   
  SubType::close_single_serv(flag);
}

bool TcpServMonitor::listen_socket()
{
  if( ! SubType::listen_socket() )
    return false;
  
  add_fd_to_table(sockfd, DEV_TCPSERV, POLLIN, dev_num, dev_line);
return true;
} 
//**** END class TcpServMonitor


std::vector <TcpServMonitor> tcp_servs;

TcpServMonitor *get_tcp_serv_by_p_mon(int dev_num, int dev_line)
{
  TcpServMonitor *p_serv=NULL;
   for(size_t i=0;i<tcp_servs.size()&&(p_serv==NULL);i++)
   {
     if(tcp_servs[i].dev_num==dev_num)
       p_serv=&tcp_servs[i];
   }
   return p_serv;
}
 
TcpServMonitor *get_tcp_serv_by_p_mon(_DeviceKey_ &p_mon)
{
  return get_tcp_serv_by_p_mon(p_mon.dev_num,p_mon.dev_line);
}
 
TcpLineMonitor *get_tcp_line_by_sockfd(int sockfd)
{
  TcpLineMonitor *p_line=NULL;
   for(size_t i=0;i<tcp_servs.size()&&(p_line==NULL);i++)
   {
     p_line=tcp_servs[i].get_line_by_sockfd(sockfd);
   }
   return p_line;
}

TcpLineMonitor *get_tcp_line_by_p_mon(int dev_num, int dev_line)
{
  TcpLineMonitor *p_line=NULL;
   for(size_t i=0;i<tcp_servs.size()&&(p_line==NULL);i++)
   {
     p_line=tcp_servs[i].get_line_by_num(dev_num,dev_line);
   }
   return p_line;
}

TcpLineMonitor *get_tcp_line_by_p_mon(_DeviceKey_ &p_mon)
{
  return get_tcp_line_by_p_mon(p_mon.dev_num,p_mon.dev_line);
}

void tcp_close_all_servers( short flag)
{
  for(size_t i=0;i<tcp_servs.size();i++)
    tcp_servs[i].close_single_serv(flag);
}

void timeout_tcp(void)
{
  for(size_t i=0;i<tcp_servs.size();i++)
    tcp_servs[i].timeout_serv();
}

void show_tcp(int show_fl,char *res_str, int res_len)
{
  res_str[0] = 0;
  for(size_t i=0;i<tcp_servs.size();i++)
    tcp_servs[i].show_serv(show_fl,res_str, res_len);
}


UdpServerMonitor monitor_udp_serv;
UdpServerMonitor *get_udp_serv_by_p_mon(_DeviceKey_ &p_mon)
{
  return &monitor_udp_serv;
}
 
//----------------------------
//---- class PipeMonitor
//----------------------------
class PipeMonitor : public _Pipe::_MesPipe_, public _DeviceKey_ , public MonitorDevice
{
 public:
  _MesBl_ *p_in_mbl;
  QueMbl que_out;
 _BufSend_ buf_send;
  
  PipeMonitor(int a_dev_num=-1, int a_dev_line=-1) : _Pipe::_MesPipe_(), _DeviceKey_(DEV_PIPE,a_dev_num,a_dev_line), buf_send()
  {
    p_in_mbl = NULL;
    que_out.init( 0 );  
  }

  bool put_to_que_out(_MesBl_ *p_mbl)
  {
    return que_out.put_last(p_mbl);
  }
  
  _MesBl_ *get_mbl_in()
  {
    _MesBl_ *p_mbl=p_in_mbl;
    p_in_mbl=NULL;
    return p_mbl;
  }
  void put_mbl_in(_MesBl_ *p_mbl) { p_in_mbl=p_mbl; }
  QueMbl *get_que_out() { return &que_out;}
  
  virtual void dev_close(enum DEV_CLOSE_REASON cl_fl)
  {
    switch(cl_fl)
    {
      case CLOSE_AFTER_SEND:// ничего не делаем
        break;
      case CLOSE_AFTER_POLLHUP:
      case CLOSE_AFTER_POLLERR:
      case CLOSE_AFTER_RECV: //???
        exit(1);
        break;
      default:/*??*/
        break;
    }
  } 
  virtual int dev_pollin_event(_DeviceKey_ &p_mon)   
  {
    int Ret=receive_message(*this);
    return Ret;
  }   
  virtual int dev_pollout_event(_DeviceKey_ &p_mon);   
  virtual int dev_pollhup_event(_DeviceKey_ &p_mon) 
  { 
    dev_close(CLOSE_AFTER_POLLHUP);
    return RETCODE_OK;
  }   
  virtual int dev_pollerr_event(_DeviceKey_ &p_mon) 
  {
    dev_close(CLOSE_AFTER_POLLERR);
    return RETCODE_OK;
  }
  virtual int message_fully_received(_MesBl_ *p_mbl);
}; 

//**** END class PipeMonitor


 PipeMonitor monitor_pipe;
 PipeMonitor *get_pipe_by_p_mon(_DeviceKey_ &p_mon)
 {
   return &monitor_pipe;
 }
 
int send_mes_to_pipe(_MesBl_ *p_mbl)
{
  _input_message_to_out_que(p_mbl, monitor_pipe);
 return RETCODE_OK;  
}
 
template <class T> int send_mes_to_tcp(T &v, _MesBl_ *p_mbl, int fd=-1)
{
 if(fd>=0)
 {
   p_mbl->set_mes_fd(fd);
 }
 
 TcpLineMonitor *p_line=get_tcp_line_by_sockfd( p_mbl->get_mes_fd() );
 if(p_line == NULL)
 {
   ProgTrace(TRACE1, "Unknown fd <%d> from %s",
       p_mbl->get_mes_fd(), v.get_id());
   free_mbl(p_mbl);
   return RETCODE_NONE;
 }
 else
 {
  _input_message_to_out_que(p_mbl, *p_line);
 }
return RETCODE_OK;
}
};// namespace monitorMain 

using namespace monitorMain;


/****************************************************************/
void set_monfinish_on_sig(int sig)
{
  ProgTrace(TRACE1,"signal %d received",sig);
  if(finish==0)
  {
    finish=1;
  }
}

/****************************************************************/
int main_monitor(int supervisorSocket, int argc, char *argv[])
{
    ASSERT(1 < argc);

    int Ret = 0;

    /*---- CSA ----*/
    close_control_for_fd(-1);

    /*---- BUFFERS ----*/
    clist_init(CLIST_PROCID_MONITOR);

    /*---- TCP ----*/
    tcp_servs.clear();

    int n_tcps = argc - 1;
    if (n_tcps > _MAX_TCP_SERVERS_) {
        n_tcps = _MAX_TCP_SERVERS_;
    }
    for (int i_tcp = 0; i_tcp < n_tcps; ++i_tcp) {
        Tcl_Obj* obj_arr = Tcl_NewStringObj(argv[i_tcp + 1], -1);
        TcpServMonitor tcp_srv((int)tcp_servs.size());
        if( ! tcp_srv.config_serv(getTclInterpretator(), obj_arr) )
          return 1;
        tcp_servs.push_back(tcp_srv);

        if (i_tcp == 0) {
            /*---- UDP ----*/
            monitor_udp_serv.preinit();
            if( ! monitor_udp_serv.config_serv(getTclInterpretator(), obj_arr) )
              return 1;
        }
    }

    /*---- PIPE ----*/
    monitor_pipe.init(supervisorSocket, supervisorSocket);
    Ret = monitor_loop(getTclInterpretator());

    close_control_for_fd(-1);
    sleep(1);
    sleep(1);
    tcp_close_all_servers(TCP_FL_WLINES);
    monitor_udp_serv.close_serv();
    clist_uninit();
    return Ret;
}


/****************************************************************/
int shows(void)
{
 int Ret;
 static time_t time1_show=0;
 static time_t time1_set=0;
 static time_t time1_tcp=0;
 static time_t time1_ctrl=0;
 time_t time2;
 char str_res[1000];

  str_res[0] = 0;

  if(time1_show == 0 )
    time1_show = time(NULL);
  
  if(time1_set == 0 )
    time1_set = time(NULL);
  
  if(time1_ctrl == 0 )
    time1_ctrl = time(NULL);
  
  if(time1_tcp == 0 )
    time1_tcp = time(NULL);
  
  time2 = time(NULL);
  
  if( TO_TCP > 0 )   
  {
    if(difftime(time2, time1_tcp) > TO_TCP)
    {
      time1_tcp = time2;
      show_tcp( 0, str_res, sizeof(str_res));
      printf("%s", str_res);
    }
  }
  if( TO_CONTROL > 0 )   
  {
    if(difftime(time2, time1_ctrl) > TO_CONTROL)
    {
      time1_ctrl = time2;
      show_control();
    }
  }
  if(TO_SHOW > 0)
  {
    if(difftime(time2, time1_show) > TO_SHOW)
    {
      time1_show = time2;
      Ret = send_show(monitor_pipe.out_fd() );
      if(Ret != RETCODE_OK)
      {
        fprintf( stderr,"ERR send_show (%d)\n",Ret);
      }
    }
  }
#ifdef TEST_MONITOR_SUPERVISOR_PIPE
  if(SETFLAG_TO_MONITOR > 0)
  {
    if(difftime(time2, time1_set) > SETFLAG_TO_MONITOR)
    {
      time1_set = time2;
      Ret = send_set_flag(monitor_pipe.out_fd(), 0, 1, QUEPOT_NULL);
      if(Ret <= 0)
      {
        fprintf( stderr,"ERR send_set_flag (%d)\n",Ret);
      }
    }
  }
#else /* ! TEST_MONITOR_SUPERVISOR_PIPE */
  if(SETFLAG_TO > 0)
  {
    if(difftime(time2, time1_set) > SETFLAG_TO)
    {
      time1_set = time2;
      Ret = send_set_flag(monitor_pipe.out_fd(), 0, 0, QUEPOT_ZAPR);
      if(Ret <= 0)
      {
        fprintf( stderr,"ERR send_set_flag (%d)\n",Ret);
      }
    }
  }
#endif /* TEST_MONITOR_SUPERVISOR_PIPE*/
return RETCODE_OK;
}

template <class T> void monitor_poll_event(short revents,_DeviceKey_ &p_mon, T &v)
{
  if (revents & POLLIN) 
  {
    int Ret=v.dev_pollin_event(p_mon);
    if(Ret == RETCODE_ERR)
      v.dev_close( CLOSE_AFTER_RECV );
  }
  if (revents & POLLOUT) 
  {
    v.dev_pollout_event(p_mon);
  }
  if(revents & POLLHUP)
  {
    ProgTrace(TRACE1, "%s pollevent=POLLHUP", get_head_to_monit(v) );
    v.dev_pollhup_event(p_mon);
  }
  if( revents & POLLERR) 
  {
     ProgTrace(TRACE1, "%s pollevent=POLLERR", get_head_to_monit(v) );
     v.dev_pollerr_event(p_mon);
  }
}

void monitor_poll_event(short revents,_DeviceKey_ &p_mon)
{
  switch(p_mon.dev_type)
  {
    case DEV_TCPSERV:
    {
      TcpServMonitor *p_serv=get_tcp_serv_by_p_mon(p_mon);
      if(p_serv!=NULL)
      {
        monitor_poll_event( revents, p_mon, *p_serv );
      }
    }
    break;
    case DEV_PIPE:
    {
      PipeMonitor *p_pipe=get_pipe_by_p_mon(p_mon);
      if(p_pipe!=NULL)
      {
        monitor_poll_event( revents, p_mon, *p_pipe );
      }
    }
    break;
    case DEV_TCPLINE:
    {
      TcpLineMonitor *p_line=get_tcp_line_by_p_mon(p_mon);
      if(p_line!=NULL)
      {
        monitor_poll_event( revents, p_mon, *p_line );
      }
    }
    break;
    case DEV_UDPSERV:
    {
      UdpServerMonitor *p_udp=get_udp_serv_by_p_mon(p_mon);
      if(p_udp!=NULL)
      {
        monitor_poll_event( revents, p_mon, *p_udp );
      }
    }
    break;
    default: /* Не должно такого быть */
      break;
  }
}


/****************************************************************/
int monitor_loop(Tcl_Interp *interp)
{

    if(set_sig(set_monfinish_on_sig,SIGINT)<0 or set_sig(set_monfinish_on_sig,SIGTERM)<0)
        Abort(1);
    set_sig(regLogReopen,SIGUSR1);
 
  add_fd_to_table( monitor_pipe.in_fd(), DEV_PIPE, POLLIN, monitor_pipe.dev_num, monitor_pipe.dev_line );
  add_fd_to_table( monitor_pipe.out_fd(), DEV_PIPE, 0, monitor_pipe.dev_num, monitor_pipe.dev_line );
  while(finish == 0)
  {
    shows();
    timeout_control();
    timeout_tcp();
    monitor_udp_serv.timeout();

    int pres = poll (&(descr_tab[0]), n_table, 1000);
    if (pres < 0) 
    {
      if(errno == EINTR)
        continue;
      return RETCODE_ERR;
    }
    reopenLog();
    for(int i_table=n_table-1; (i_table>=0)&&(pres>0); i_table--)
    {
      short revents = descr_tab[i_table].revents;
      if(revents)
      {
        pres--;
        monitor_poll_event( revents,mon_table[i_table]);
      }
    }
    delete_fd_from_table();
  }

return RETCODE_OK;
}

/****************************************************************/
static _MesBl_ *create_mbl_with_cmd(const char *text, int fd, int cmd)
{
  _MesBl_ *p_mbl = new_mbl();
  if(p_mbl == NULL)
  {
     return NULL;
  }
  p_mbl->concat_text(text);
  p_mbl->set_mes_fd(fd);
  p_mbl->set_mes_cmd(cmd);
  
  p_mbl->prepare_to_send();
return p_mbl;
}

/*****************************************************************/
int send_show(int fd)
{
  _MesBl_ *p_mbl = create_mbl_with_cmd(ARRCMD_PS_SHOW, fd, TCLCMD_SHOW );
  if(p_mbl == NULL)
  {
     return RETCODE_ERR;
  }
return send_mes_to_pipe(p_mbl);
}

/*****************************************************************/
int send_set_flag(int fd, int TO_restart, int n_zapr, int type_zapr)
{
  char  str_set[200];
  snprintf(str_set, sizeof(str_set), "%s %d %d %d %d %s", ARRCMD_SET_FLAG_ARR_NO ,Pid_p, TO_restart, n_zapr, type_zapr, "{}");
  _MesBl_ *p_mbl = create_mbl_with_cmd(str_set, fd, TCLCMD_SET_FLAG);
  if(p_mbl == NULL)
  {
     return RETCODE_ERR;
  }
return send_mes_to_pipe(p_mbl);
}

/****************************************************************/
void add_fd_to_table(int fd, enum DEV_TYPE dev_type, short event, int dev_num, int dev_line)
{
  if(n_table >= _MaxMonTable_ )
  {
    ProgError(STDLOG,"Too many fd's");
    return;
  }
  descr_tab[n_table].fd = fd;
  descr_tab[n_table].events = event;
  mon_table[n_table].set(dev_type,dev_num, dev_line);
  n_table++;
}

/****************************************************************/
void change_event_in_table(int fd, short flag, short event)
{
  for(int i_table=0; i_table<n_table; i_table++)
  {
    if( descr_tab[i_table].fd == fd)
    {
      if(flag == 0) /* SET */
      {
        descr_tab[i_table].events = event;
      }
      else if(flag == 1) /* ADD */
      {
        descr_tab[i_table].events |= event;
      }
      else if(flag == 2) /* DELETE */
      {
        descr_tab[i_table].events &= ~event;
      }
      return;
    }
  }
}

/****************************************************************/
void delete_fd_from_table(void)
{
 for(int i_table=n_table-1; i_table>=0; i_table--)
 {
   if( mon_table[i_table].dev_type == DEV_FREE)
   {
     if(i_table != n_table-1)
     {
       memmove( descr_tab+i_table,
                descr_tab+i_table+1,
                (sizeof(descr_tab[0]))*(n_table-i_table-1) );
       memmove( mon_table+i_table,
                mon_table+i_table+1,
                (sizeof(mon_table[0]))*(n_table-i_table-1) );
     }
     n_table--;
   }
 }
}

/****************************************************************/
void set_delete_fd_from_table(int fd)
{
 if(fd < 0)
 {
   return;
 }
 for(int i_table=0; i_table<n_table; i_table++)
 {
   if( descr_tab[i_table].fd == fd)
   {
      mon_table[i_table].set0();
   }
 }
}

/****************************************************************/
void OurMes_from_pipe( _MesBl_ *p_mbl)
{
 Tcl_Interp *interp=getTclInterpretator();
 
  if(p_mbl->get_mes_ret() != TCL_OK)
  {
    ProgTrace(TRACE1,"ERROR execute TCL command(%d): <%.*s>",
         p_mbl->get_mes_ret(), p_mbl->get_mes_len(), p_mbl->get_mes_text());
  }
  else
  {
    switch(p_mbl->get_mes_cmd() )
    {
      case TCLCMD_SHOW:
      {
        int objc;
        Tcl_Obj *pobj = Tcl_NewStringObj(p_mbl->get_mes_text(), p_mbl->get_mes_len());
        int Ret = Tcl_ListObjLength(interp, pobj,&objc);
        if(Ret == TCL_OK)
        {
          for(int i=0;i<objc;i++)
          {
            Tcl_Obj *to;
            Tcl_ListObjIndex(interp,pobj, i,&to);
            printf( "%s\n", Tcl_GetString(to) );
          }
          printf("\n");
        }
      }
      break;
      default:
        break;
    }
  }
}

int PipeMonitor::message_fully_received(_MesBl_ *p_mbl) 
{
#ifdef TEST_MONITOR_SUPERVISOR_PIPE
  if(TcpLogFlag)
  {
    char str_head[1000];
    recode_mes_to_visual( (char *)&(p_mbl->get_mes_head()), p_mbl->get_mes_head_size(),
          str_head, sizeof(str_head), p_mbl->get_mes_head_size()/*len4hex*/ );
          
    char str_tmp[1000];
    recode_mes_to_visual( p_mbl->get_mes_text(), p_mbl->get_mes_len(),
          str_tmp, sizeof(str_tmp) );
          
    ProgTrace(TRACE1, "RECV%s<%d>(%s)(%s)", get_head_to_monit(*this), 
          p_mbl->get_mes_len(), str_head, str_tmp);
 }
#endif /* TEST_MONITOR_SUPERVISOR_PIPE */

#ifdef KSE_TODO
  if( p_mbl->get_mes_fd() == -1 && p_mbl->get_mes_cmd() == TCLCMD_PROCCMD )
  {
// ------------------------------------------------------------------------
// получили команду (непосредственно от супервизора или от кого-то через супервизор ),
// которую надо выполнить силами Tcl 
// ------------------------------------------------------------------------
  }
#endif /* KSE_TODO */
  if(p_mbl->get_mes_fd() == out_fd() )
  {
    Tcl_Interp *interp=getTclInterpretator();
   
    if(p_mbl->get_mes_ret() != TCL_OK)
    {
      ProgTrace(TRACE1,"ERROR execute TCL command(%d): <%.*s>",
           p_mbl->get_mes_ret(), p_mbl->get_mes_len(), p_mbl->get_mes_text());
    }
    else
    {
      switch(p_mbl->get_mes_cmd() )
      {
        case TCLCMD_SHOW:
        { 
          Tcl_Obj *pobj = Tcl_NewStringObj(p_mbl->get_mes_text(), p_mbl->get_mes_len());
          int objc;
          int Ret = Tcl_ListObjLength(interp, pobj,&objc);
          if(Ret == TCL_OK)
          {
            for(int i=0;i<objc;i++)
            {
              Tcl_Obj *to;
              Tcl_ListObjIndex(interp,pobj, i,&to);
              printf( "%s\n", Tcl_GetString(to) );
            }
            printf("\n");
          }
        }
        break;
        default:
          break;
      }
    }
    free_mbl(p_mbl);
  }
  else
  {
    send_mes_to_tcp(*this, p_mbl);
  }
return RETCODE_OK;
};


int UdpServerMonitor::message_fully_received(_MesBl_ *p_mbl) 
{
  _CtrlsFd_ &s_ctrls_fds = p_mbl->get_ctrls();
  if(s_ctrls_fds.n_fd()>0)
  {
    for(int i_fd=1;i_fd<s_ctrls_fds.n_fd();i_fd++)
    {
      int cur_fd = s_ctrls_fds.fd(i_fd);
      if(cur_fd<0)
        continue;
      _MesBl_* p_mbl_dest=new_mbl();
      if(p_mbl_dest!=NULL)
      {
        copy_mbl(p_mbl_dest,p_mbl);
        p_mbl_dest->reset_ctrls();
        p_mbl_dest->set_mes_fd( cur_fd );
        send_mes_to_tcp(*this,p_mbl_dest);
      }
    }
  
    p_mbl->set_mes_fd(s_ctrls_fds.fd(0));
  }
  send_mes_to_tcp(*this,p_mbl);
return RETCODE_OK;
}


int TcpLineMonitor::message_fully_received(_MesBl_ *p_mbl)
{
 Tcl_Interp *interp=getTclInterpretator();

 if(TcpLogFlag)
 {
  char str_tmp[1000];
   recode_mes_to_visual( p_mbl->get_mes_text(),p_mbl->get_mes_len(),
         str_tmp, sizeof(str_tmp) );
   ProgTrace(TRACE1, "RECV%s<%d>(%s)", get_head_to_monit(*this), 
         p_mbl->get_mes_len(), str_tmp);
 }
 
 Tcl_Obj *p_obj=Tcl_NewStringObj( p_mbl->get_mes_text(), p_mbl->get_mes_len());
 if(p_obj!=NULL)
 {
   int objc;
   if( Tcl_ListObjLength(interp, p_obj,&objc) == TCL_OK )
   {
     if( objc > 0 )
     {
       Tcl_Obj *p_to;
       Tcl_ListObjIndex(interp,p_obj, 0, &p_to);
       char *p_str = Tcl_GetString(p_to);
       if( strcmp(p_str, MONCMD_CTRL ) == 0  )
       {
         return recv_control_mes( p_obj, objc, p_mbl);
       }
       else if( strcmp(p_str, MONCMD_EXIT ) == 0 ||
                strcmp(p_str, MONCMD_QUIT ) == 0  )
       {
         free_mbl(p_mbl);
         return RETCODE_EXIT;
       }
       else if( strcmp(p_str, MONCMD_HELP ) == 0  )
       {
         return recv_help_mes( p_obj, objc, p_mbl);
       }
       else if( strcmp(p_str, MONCMD_SHOW ) == 0  )
       {
         int ret=recv_show_mes( p_obj, objc, p_mbl);
         if(ret==RETCODE_OK)
           return ret;
       }
#ifdef TEST_MONITOR_SUPERVISOR_PIPE
       else if( strcmp(p_str, "superpuperuser_exit_monitor" ) == 0  )
       {
         ProgTrace(TRACE1, "Принудительный останов монитора по убедительной просьбе от %s!!!", get_head_to_monit(*this));
         exit(1);
       }
#endif /* TEST_MONITOR_SUPERVISOR_PIPE */
     }
   }
 }
return send_mes_to_pipe(p_mbl);
}

/****************************************************************/
int TcpLineMonitor::send_answer_by_text(_MesBl_ *p_mbl, const char *answer_str, int retc)
{
  p_mbl->set_mes_ret(retc);
  p_mbl->set_mes_text(answer_str, strlen(answer_str));
  _input_message_to_out_que(p_mbl, *this);
return RETCODE_OK;
}

template <class T> char *get_head_to_monit(T &v)
{
 static char res_str[200];
 res_str[0] = 0;
 return v.get_id(res_str, sizeof(res_str));
}

/****************************************************************/
/* now for DEV_PIPE and DEV_UDPSERV and DEV_TCPLINE*/
template <class T> int receive_message( T &v)
{
 char err_str[_MAX_MES_LEN_];
 static _MesRecv_ mes_recv;

  mes_recv._reset();
  _DevFormat_ &in_format=v.get_in_format();
  
/* Прием буфера */  
  int Ret = v.recv_buf(  mes_recv.recv_text, &(mes_recv.recv_len),
                  err_str, sizeof(err_str) );
#ifdef TEST_MONITOR_SUPERVISOR_PIPE
  // отладочная печать для PIPE
  if(TcpLogFlag && v.get_dev_type()==DEV_PIPE )
  {
    if(Ret!=RETCODE_OK || mes_recv.recv_len<=0)
      ProgError(STDLOG, "PIPE_RECV: ret=%d len=%d",Ret,mes_recv.recv_len);
    else
      ProgTrace(TRACE1, "PIPE_RECV: ret=%d len=%d",Ret,mes_recv.recv_len);
  }
#endif /* TEST_MONITOR_SUPERVISOR_PIPE */

  if(Ret != RETCODE_OK)
  {
    if(Ret == RETCODE_ERR)
    {
      ProgError(STDLOG, "%s", err_str);
    }
    else if( Ret == RETCODE_NONE )
    {
      ProgTrace(TRACE1, "%sConnection lost", get_head_to_monit(v) );
      Ret = RETCODE_ERR;
    }
    return Ret;
  }
  _MesBl_ *p_mbl=v.get_mbl_in();
  while(mes_recv.cur_len < mes_recv.recv_len)
  {
    if(p_mbl == NULL)
    {
      p_mbl = new_mbl();
      if(p_mbl == NULL)
      {
        ProgTrace(TRACE1, "Can't alloc buffer for recv");
        return RETCODE_NONE;
      }
      p_mbl->set_mes_fd(v.in_fd());
    }
    if( in_format.is_set_format_flag(MF_LEN) )
    {
      Ret = razbor_message_len(p_mbl, mes_recv, err_str, sizeof(err_str) );
    }
    else if( in_format.is_set_format_flag(MF_TAIL) )
    {
      Ret = razbor_message_tail(p_mbl, mes_recv, in_format, err_str, sizeof(err_str) );
    }
    else
    {
      snprintf(err_str,sizeof(err_str),"Unknown input format flag <0x%x>", in_format.format_flag() );
      Ret = RETCODE_ERR;
    }
    
    if(Ret == RETCODE_ERR)
    {
      free_mbl(p_mbl);
      ProgTrace(TRACE1,"%s%s", get_head_to_monit(v), err_str);
      return Ret;
    }
    else if(Ret == RETCODE_OK)
    {
      int ret_loc=v.message_fully_received(p_mbl);
      if(ret_loc == RETCODE_EXIT)
      {
        ProgTrace(TRACE1,"%sClose connection by command", get_head_to_monit(v));
        return RETCODE_ERR;
      }
      p_mbl = NULL;
    }
  }
  if(p_mbl != NULL)
  {
    v.put_mbl_in(p_mbl);
  }
  
return RETCODE_OK;
}

/****************************************************************/
static int razbor_message_tail( _MesBl_ *p_mbl, _MesRecv_ &p_mr, _DevFormat_ &in_format,
                         char *e_str, int e_len)
{
 char *ptr=NULL;
 int len=1;
 
  while( (ptr=p_mr.get_ptr_to_read()) != NULL)
  {
    if(p_mbl->concat_text(ptr, len) != len)
    {
      snprintf( e_str, e_len,"Message too long: lost connection");
      return RETCODE_ERR;
    }
    p_mr.set_readed(len);
    
    if( p_mbl->get_mes_len() >= in_format.tail_len() )
    {
      if( memcmp(p_mbl->get_mes_text() + p_mbl->get_mes_len() - in_format.tail_len(),
                 in_format.tail_str(),
                 in_format.tail_len())  == 0)
      {
        p_mbl->decrease_mes_len( in_format.tail_len() );
        return RETCODE_OK;
      }
    }
  }
return RETCODE_NONE;
}

/****************************************************************/
int razbor_message_len( _MesBl_ *p_mbl, _MesRecv_ &p_mr, char *e_str, int e_len )
{

 e_str[0] = 0;
 
 
  int dest_already_inserted=p_mbl->get_mbl().cur_len;
  
  {// Считываем заголовок
    int cur_block_need_size = p_mbl->get_mes_head_size();
    
    if( dest_already_inserted < cur_block_need_size )
    {
      char *block_ptr = (char *)&(p_mbl->get_mes_head());
      int dest_need = cur_block_need_size - dest_already_inserted;
      int src_len = p_mr.get_rest_read_len();
      bool cur_block_fully_readed = true;
      if( dest_need > src_len )
      {
        dest_need = src_len;
        cur_block_fully_readed = false;
      }
      memcpy( block_ptr + dest_already_inserted,
              p_mr.get_ptr_to_read(),
              dest_need );
      p_mbl->get_mbl().cur_len += dest_need;
      p_mr.set_readed(dest_need);
      dest_already_inserted += dest_need;
      if( ! cur_block_fully_readed )
        return RETCODE_NONE;
      if( ! p_mbl->get_mbl().check_mes_head(e_str, e_len) )
        return RETCODE_ERR;
    }
    dest_already_inserted -= cur_block_need_size;
  }
 
  // Считываем текст
  {
    int cur_block_need_size=p_mbl->get_mes_len();
    /* Эту проверку уже провели в вышестоящем вызове check_mes_head()
    int max_block_size = p_mbl->get_text_size();
    if(cur_block_size > max_block_size)
    {
      snprintf( e_str, e_len, "Text in message too long (len=%d, max=%d): lost connection",
         cur_block_size,max_block_size);
      return RETCODE_ERR;
    }*/
   
    if( dest_already_inserted < cur_block_need_size )
    {
      char *block_ptr = p_mbl->get_mes_text();
      int dest_need = cur_block_need_size - dest_already_inserted;
      int src_len = p_mr.get_rest_read_len();
      bool cur_block_fully_readed = true;
      
      if(dest_need > src_len)
      {
        dest_need = src_len;
        cur_block_fully_readed = false;
      }
      memcpy( block_ptr + dest_already_inserted,
              p_mr.get_ptr_to_read(),
              dest_need );
      p_mbl->get_mbl().cur_len += dest_need;
      p_mr.set_readed(dest_need);
      dest_already_inserted += dest_need;
      if( ! cur_block_fully_readed )
        return RETCODE_NONE;
    }
    dest_already_inserted -= cur_block_need_size;
  }

  // Считываем head_str
  {
    int cur_block_need_size = p_mbl->get_head_str_len();
    /* Эту проверку уже провели в вышестоящем вызове check_mes_head()
    max_block_size = p_mbl->get_head_str_size();
    if(cur_block_size > max_block_size)
    {
      snprintf( e_str, e_len,"head_str in message too long (len=%d, max=%d): lost connection",
        cur_block_size,max_block_size);
      return RETCODE_ERR;
    }*/
    if( dest_already_inserted < cur_block_need_size )
    {
      char *block_ptr = p_mbl->get_head_str();
      int dest_need = cur_block_need_size - dest_already_inserted;
      int src_len = p_mr.get_rest_read_len();
      bool cur_block_fully_readed = true;
      
      if( dest_need > src_len )
      {
        dest_need = src_len;
        cur_block_fully_readed = false;
      }
      memcpy( block_ptr + dest_already_inserted,
              p_mr.get_ptr_to_read(),
              dest_need );
      p_mbl->get_mbl().cur_len += dest_need;
      p_mr.set_readed(dest_need);
      dest_already_inserted += dest_need;
      if( ! cur_block_fully_readed )
        return RETCODE_NONE;
    }
    dest_already_inserted -= cur_block_need_size;
  }
 
  // Считываем ctrls_fd
  {
    int cur_block_need_size = p_mbl->get_ctrls_len();
    /* Эту проверку уже провели в вышестоящем вызове check_mes_head()
    int max_block_size=p_mbl->get_ctrls_size();
    if(cur_block_size > max_block_size)
    {
      snprintf( e_str, e_len,"CtrlsFd in message too long (len=%d, max=%d): lost connection",
        cur_block_size,max_block_size);
      return RETCODE_ERR;
    }*/
    if( dest_already_inserted < cur_block_need_size )
    {
      char *block_ptr = (char *)&(p_mbl->get_ctrls());
      int dest_need = cur_block_need_size - dest_already_inserted;
      int src_len = p_mr.get_rest_read_len();
      bool cur_block_fully_readed = true;
      
      if( dest_need > src_len )
      {
        dest_need = src_len;
        cur_block_fully_readed = false;
      }
      memcpy( block_ptr + dest_already_inserted,
              p_mr.get_ptr_to_read(),
              dest_need );
      p_mbl->get_mbl().cur_len += dest_need;
      p_mr.set_readed(dest_need);
      dest_already_inserted += dest_need;
      if( ! cur_block_fully_readed )
        return RETCODE_NONE;
    }
    dest_already_inserted -= cur_block_need_size;
  }

  p_mbl->get_mbl().full_len=p_mbl->get_full_mes_len();

return RETCODE_OK;
}

/****************************************************************/
/* for DEV_TCPLINE , DEV_PIPE*/
template <class T> void _input_message_to_out_que(_MesBl_ *p_mbl, T &v)
{
  if( ! v.put_to_que_out( p_mbl ) )
  {
    free_mbl(p_mbl);
    return;
  }
  change_event_in_table(v.out_fd(), 1, POLLOUT);
}

/****************************************************************/
bool convert_Message_to_BufSend(_Message_ *p_mes, _BufSend_ &buf4send, _DevFormat_ &out_format)
{
 static char str_blank[]=" ";
 static char str_CR[]="\n";
 unsigned short d_format = out_format.format_flag();
 
  buf4send._reset();
  
  if(p_mes==NULL)
  {
    return false;
  }

  if( (d_format&MF_LEN) )
  {
    // копируем head
    if( ! buf4send.add_buf_to_send( (char *)&(p_mes->get_mes_head/*_ptr*/()), p_mes->get_mes_head_size(), true /*bin_data_fl*/) )
      return false;
    
    // text
    if( ! buf4send.add_buf_to_send( p_mes->get_mes_text(), p_mes->get_mes_len()) )
      return false;
    
    // head_str
    if( p_mes->get_head_str_len()>0 )
    {
      if( ! buf4send.add_buf_to_send( p_mes->get_head_str(),p_mes->get_head_str_len() ) )
        return false;
    }
    
    // ctrls
    if( p_mes->get_ctrls_len()>0 )
    {
      if( ! buf4send.add_buf_to_send( (char *)&(p_mes->get_ctrls()), p_mes->get_ctrls_len() ) )
        return false;
    }
  }
  else if( (d_format&MF_TAIL) )
  {
    if( (d_format & MF_RET) )
    {
      char str_ret[_DOP_MES_HEAD_LEN_+100];
        
      int len_ret = sprintf(str_ret,"%d",p_mes->get_mes_ret() );
      if( ! buf4send.add_buf_to_send( str_ret,len_ret) )
        return false;

      
      if(p_mes->get_head_str_len() > 0)
      {
        if( ! buf4send.add_buf_to_send( str_blank,strlen(str_blank) ) )
          return false;
        if( ! buf4send.add_buf_to_send( p_mes->get_head_str(), p_mes->get_head_str_len()) )
          return false;
      }
         
      if( ! buf4send.add_buf_to_send( str_CR,strlen(str_CR)) )
        return false;
    }

    // text
    if( ! buf4send.add_buf_to_send( p_mes->get_mes_text(), p_mes->get_mes_len()) )
      return false;

    if( ! buf4send.add_buf_to_send( out_format.tail_str(), out_format.tail_len() ) )
      return false;
  }
  
return true;
}

/****************************************************************/
/* Use in DEV_PIPE,DEV_TCPLINE*/
template <class T> void send_message(_DeviceKey_ &p_mon, T &v)
{
 char err_str[_ML_ERRSTR_];
 
  int fd = v.out_fd();
  _BufSend_& buf4send=v.buf_send;
  if( ! buf4send.is_exist4send() )
  {
    _MesBl_ *p_mbl = v.que_out.get_first( );
    if( p_mbl == NULL )
    {
      change_event_in_table(fd, 2, POLLOUT);
      return;
    }
    if( ! convert_Message_to_BufSend( &(p_mbl->get_message()), buf4send, v.get_out_format()) )
    {
      free_mbl(p_mbl);
      buf4send._reset();
      return;
    }
    free_mbl(p_mbl);
  }
  
  int need_len = buf4send.get_len_for_send();
#ifdef TEST_MONITOR_SUPERVISOR_PIPE
  int need_len_tmp=need_len;
#endif /* TEST_MONITOR_SUPERVISOR_PIPE */

  int Ret = v.send_buf( buf4send.get_ptr_to_send(), &need_len, err_str, sizeof(err_str) );
  
#ifdef TEST_MONITOR_SUPERVISOR_PIPE
  // отладочная печать для PIPE
  if( TcpLogFlag && v.get_dev_type()==DEV_PIPE )
  {
    if(Ret!=RETCODE_OK || need_len==0)
      ProgError(STDLOG, "PIPE_SEND: err=%d len=%d/%d (cur_len=%d full_len=%d)", Ret, need_len_tmp, need_len, buf4send.get_sended_len(), buf4send.get_filled_len());
    else
      ProgTrace(TRACE1, "PIPE_SEND: err=%d len=%d/%d (cur_len=%d full_len=%d)", Ret, need_len_tmp, need_len, buf4send.get_sended_len(), buf4send.get_filled_len() );
  }
#endif /* TEST_MONITOR_SUPERVISOR_PIPE */

  if(Ret == RETCODE_OK)
  {
    buf4send.add_sended_len(need_len);
    if( buf4send.is_exist4send() )
    {
      Ret = RETCODE_NONE;
    }
  }
  
  if( Ret == RETCODE_OK )
  {
    if(TcpLogFlag && v.get_dev_type()==DEV_TCPLINE )
    {
      char str_tmp[1000];
      recode_mes_to_visual( buf4send.get_text_ptr(), buf4send.get_filled_len(),
            str_tmp, sizeof(str_tmp) );
      ProgTrace(TRACE1, "SEND%s<%d>(%s)", get_head_to_monit(v),
            buf4send.get_filled_len(), str_tmp);
    }
#ifdef TEST_MONITOR_SUPERVISOR_PIPE
    else if(TcpLogFlag && v.get_dev_type()==DEV_PIPE )
    {
      char str_tmp[2000];
      recode_mes_to_visual( buf4send.get_text_ptr(), buf4send.get_filled_len(),
            str_tmp, sizeof(str_tmp), buf4send.get_bin_data_len()/*len4hex*/ );
            
      ProgTrace(TRACE1, "SEND%s<%d>(%s)", get_head_to_monit(v), 
            buf4send.get_filled_len(), str_tmp);
    }
#endif /* TEST_MONITOR_SUPERVISOR_PIPE */
    buf4send._reset();
    if( v.que_out.is_empty() )
    {
      change_event_in_table(fd, 2, POLLOUT);
    }
  }
  else if(Ret == RETCODE_ERR )
  {
    ProgError(STDLOG, "%s%s", get_head_to_monit(v),err_str);
    buf4send._reset();
    v.dev_close(CLOSE_AFTER_SEND);
  }
}

int TcpLineMonitor::dev_pollout_event(_DeviceKey_ &p_mon) 
{
  send_message(p_mon, *this);
  return RETCODE_OK;
};   

int PipeMonitor::dev_pollout_event(_DeviceKey_ &p_mon) 
{
 send_message(p_mon, *this);
 return RETCODE_OK;
};   


/****************************************************************/
/* Return: RETCODE_OK   - надо посылать данное сообщение */
/*         RETCODE_NONE - надо освободить данное сообщение */
int TcpLineMonitor::recv_help_mes( Tcl_Obj *p_list_obj, int n_obj, _MesBl_ *p_mbl)
{
 char str_mes[_MAX_MES_LEN_];
 bool fl=false;
 if(n_obj == 2)
 {
   Tcl_Obj *p_to;
   Tcl_ListObjIndex( getTclInterpretator(), p_list_obj, 1, &p_to);
   char *p_str = Tcl_GetString(p_to);
   
   if( strcmp(p_str, MONCMD_CTRL ) == 0  )
   {
     fl=true;     
     snprintf( str_mes, sizeof(str_mes),
               "Show controls:\n"
               "%s\n"
               "\n"
               "Clear controls:\n"
               "%s clear\n"
               "\n"
               "Init control <errors>:\n"
               "%s set %d\n"
               "\n"
               "Init control <messages>:\n"
               "%s set <Who> <Begpos> <Len> <Str>\n"
               "  where:\n"
               "   <Who>:    %d - head; %d - text; %d - log; %d - pult\n"
               "   <Begpos>: begin position\n"
               "   <Len>:    proceed length\n"
               "   <Str>:    regular expression\n"
               "\n",
               MONCMD_CTRL,
               MONCMD_CTRL,
               MONCMD_CTRL,
               _CTRL_WHO_ERRS_,
               MONCMD_CTRL,
               _CTRL_WHO_HEAD_,
               _CTRL_WHO_TEXT_,
               _CTRL_WHO_LOG_,
               _CTRL_WHO_PULT_);
   }
   else if( strcmp(p_str, MONCMD_PROC_STOP ) == 0  )
   {
     fl=true;     
     snprintf( str_mes, sizeof(str_mes),
               "Stop all processes:\n"
               "%s %s\n"
               "\n"
               "Stop all processes (without tclmon and monitor):\n"
               "%s %s\n"
               "\n"
               "Stop need processes:\n"
               "%s <name> <pids> ...\n"
               "  where:\n"
               "   <name> - name of process\n"
               "   <pids> - numbers of pid to stop\n",
               MONCMD_PROC_STOP,
               MONCMD_PROC_STOP_PARALL,
               MONCMD_PROC_STOP,
               MONCMD_PROC_STOP_PARSEL,
               MONCMD_PROC_STOP);
   }
   else if( strcmp(p_str, MONCMD_PROC_RESTART) == 0  )
   {
     fl=true;     
     snprintf( str_mes, sizeof(str_mes),
               "Restart all processes:\n"
               "%s %s\n"
               "\n"
               "Restart need processes:\n"
               "%s <name> <pids> ...\n"
               "  where:\n"
               "   <name> - name of process\n"
               "   <pids> - numbers of pid to restart\n",
               MONCMD_PROC_RESTART,
               MONCMD_PROC_RESTART_PARALL,
               MONCMD_PROC_RESTART);
   }
   else if( strcmp(p_str, MONCMD_SHOW ) == 0  )
   {
     fl=true;     
     snprintf( str_mes, sizeof(str_mes),
               "Show monitor info:\n"
               "%s buf    - information about bufs in Monitor\n"
               "%s tcp    - information about tcp-connections\n"
               "%s tcpall - full information about tcp-connections\n",
               MONCMD_SHOW,
               MONCMD_SHOW,
               MONCMD_SHOW);
   }
   else if( strcmp(p_str, MONCMD_PROC_CMD ) == 0  )
   {
     fl=true;     
     snprintf( str_mes, sizeof(str_mes),
               "Send command to process:\n"
               "%s <name> <pid> <command>\n"
               "  where:\n"
               "   <name> - name of process\n"
               "   <pids> - numbers of process pid\n"
               "   <command> - command name\n",
               MONCMD_PROC_CMD);
   }
   else if( 0 == strcmp(p_str, MONCMD_PROC_GRP_CMD ) )
   {
     fl=true;
     snprintf( str_mes, sizeof(str_mes),
               "Send command to group of processes:\n"
               "%s <group> <command>\n"
               "  where:\n"
               "   <group> - group of processes\n"
               "   <command> - command name\n",
               MONCMD_PROC_GRP_CMD);
   }
 }
 if(!fl)
 {
   fl=true;     
   snprintf( str_mes, sizeof(str_mes),
             "%-15.15s - show brief information about running processes\n"
             "%-15.15s - show full information about running processes\n"
             "%-15.15s - control\n"
             "%-15.15s - show monitor info\n"
             "%-15.15s - show system information\n"
             "%-15.15s - stop processes\n"
             "%-15.15s - start processes\n"
             "%-15.15s - restart processes\n"
             "%-15.15s - send command to process\n"
             "%-15.15s - information about bufs in Tclmon\n"
             "%-15.15s - send command to group processes"
             "\n"
             "%s <cmd> - information about parameters of the command <cmd>",
             MONCMD_PROC_BRIEFINFO,
             MONCMD_PROC_FULLINFO,
             MONCMD_CTRL,
             MONCMD_SHOW,
             MONCMD_SYSTEMINFO,
             MONCMD_PROC_STOP,
             MONCMD_PROC_START,
             MONCMD_PROC_RESTART,
             MONCMD_PROC_CMD,
             MONCMD_SHOW_TCLBUFS,
             MONCMD_PROC_GRP_CMD,
             MONCMD_HELP);
 }
 str_mes[sizeof(str_mes)-1] = 0;
return send_answer_by_text( p_mbl, str_mes, TCL_OK ); 
}

/****************************************************************/
/* Return: RETCODE_OK   - сообщение обработано и поставлено в очередь на вывод */
/*         RETCODE_NONE - это не наше сообщение, надо его послать в pipe */
int TcpLineMonitor::recv_show_mes( Tcl_Obj *p_list_obj, int n_obj, _MesBl_ *p_mbl )
{
 char str_mes[_MAX_MES_LEN_];
 
  if(n_obj == 2)
  {
    Tcl_Obj *p_to;
    Tcl_ListObjIndex( getTclInterpretator(), p_list_obj, 1, &p_to);
    char *p_str = Tcl_GetString(p_to);
    if( strcmp(p_str, "buf" ) == 0  )
    {
      str_mes[0] = 0;
      show_buf_counts(str_mes, sizeof(str_mes));
      return send_answer_by_text( p_mbl, str_mes, TCL_OK ); 
    }
    else if( strcmp(p_str, "tcpall" ) == 0  )
    {
      show_tcp(1, str_mes, sizeof(str_mes));
      str_mes[sizeof(str_mes)-1] = 0;
      return send_answer_by_text( p_mbl, str_mes, TCL_OK ); 
    }
    else if( strcmp(p_str, "tcp" ) == 0  )
    {
      show_tcp( 0, str_mes, sizeof(str_mes));
      str_mes[sizeof(str_mes)-1] = 0;
      return send_answer_by_text( p_mbl, str_mes, TCL_OK ); 
   }
 }
return RETCODE_NONE;
}

/****************************************************************/
/* Return: RETCODE_OK   - надо посылать данное сообщение */
/*         RETCODE_NONE - надо освободить данное сообщение */
int TcpLineMonitor::recv_control_mes( Tcl_Obj *p_list_obj, int n_obj, _MesBl_ *p_mbl )
{
 char str_mes[_MAX_MES_LEN_];

  int fd = in_fd();
  Tcl_Interp *interp=getTclInterpretator();
  
  /* #1 */
  int n_arg=1;
  if( n_obj <= n_arg )
  {
    // No arguments. Show info about control status for fd.
    str_mes[0] = 0;
    for(int i=0; i<_MAX_MES_CTRLS_; i++)
    {
      _ControlMesItem_ &p_mes_ctrl = csaControl->csa()->ctrl_mes_item[i];
      if( (p_mes_ctrl.ctrl_flag == _CTRL_FLAG_BUSY_ ) &&
          (p_mes_ctrl.ctrl_fd   == fd) )
      {
        snprintf( str_mes,sizeof(str_mes),"%d %d(%d) %s\n",
                  p_mes_ctrl.ctrl_who,
                  p_mes_ctrl.ctrl_begpos,
                  p_mes_ctrl.ctrl_len,
                  p_mes_ctrl.ctrl_str );
        str_mes[sizeof(str_mes)-1] = 0;
        return send_answer_by_text( p_mbl, str_mes, TCL_OK );
      }
    }
    for(int i=0; i<_MAX_ERR_CTRLS_; i++)
    {
      _ControlErrItem_ &p_err_ctrl = csaControl->csa()->ctrl_err_item[i];
      if( (p_err_ctrl.ctrl_flag == _CTRL_FLAG_BUSY_ ) &&
          (p_err_ctrl.ctrl_fd   == fd) )
      {
        snprintf( str_mes, sizeof(str_mes),"%d\n",_CTRL_WHO_ERRS_);
        str_mes[sizeof(str_mes)-1] = 0;
        return send_answer_by_text( p_mbl, str_mes, TCL_OK );
      }
    }
    return send_answer_by_text( p_mbl, str_mes, TCL_OK );
  }
 
  Tcl_Obj *p_to;
  Tcl_ListObjIndex( interp, p_list_obj, n_arg, &p_to);
  char *p_str = Tcl_GetString(p_to);
  if( strcmp(p_str, "clear" ) == 0  )
  {
    close_control_for_fd(fd);
    return send_answer_by_text( p_mbl, "OK", TCL_OK );
  }
  else if( strcmp(p_str, "set" ) != 0  )
  {
    return send_answer_by_text( p_mbl, "Wrong arg <Cmd>", TCL_ERROR );
  }
 
  if( n_obj < 3 )
  {
    return send_answer_by_text( p_mbl, "Wrong number of args", TCL_ERROR );
  }
  
  /* #2  Who: _CTRL_WHO_HEAD_ - head;*/
  /*          _CTRL_WHO_TEXT_ - text;*/
  /*          _CTRL_WHO_LOG_  - logs */
  /*          _CTRL_WHO_PULT_ - pult   */
  /*          _CTRL_WHO_ERRS_ - errors */
  n_arg++;
  Tcl_ListObjIndex(interp,p_list_obj, n_arg,&p_to);
  int who;
  if( Tcl_GetIntFromObj(interp,p_to, &who) != TCL_OK)
  {
    return send_answer_by_text( p_mbl, "Wrong arg <Who>", TCL_ERROR );
  }
  if( who != _CTRL_WHO_HEAD_ && 
      who != _CTRL_WHO_TEXT_ && 
      who != _CTRL_WHO_LOG_  &&
      who != _CTRL_WHO_PULT_ &&
      who != _CTRL_WHO_ERRS_  )
  {
    return send_answer_by_text( p_mbl, "Wrong args <Who>: (must be in [0,1,2,3])", TCL_ERROR );
  }
  if(who == _CTRL_WHO_ERRS_)
  {
    close_control_for_fd(fd);
    for(int i=0; i<_MAX_ERR_CTRLS_; i++)
    {
      _ControlErrItem_ &p_err_ctrl = csaControl->csa()->ctrl_err_item[i];
      if( p_err_ctrl.ctrl_flag == _CTRL_FLAG_FREE_ ) 
      {
        p_err_ctrl.ctrl_fd = fd;
        p_err_ctrl.ctrl_time   = time(NULL);
        p_err_ctrl.ctrl_flag   = _CTRL_FLAG_BUSY_;
        return send_answer_by_text( p_mbl, "OK", TCL_OK );
      }
    }
    return send_answer_by_text( p_mbl, "Not enouth space for ctrl err", TCL_ERROR );
  }
  
  if( n_obj != 6 )
  {
    return send_answer_by_text( p_mbl, "Wrong number of args", TCL_ERROR );
  }
  
  /* #3 Begpos */
  n_arg++;
  Tcl_ListObjIndex(interp,p_list_obj, n_arg,&p_to);
  int begpos;
  if( Tcl_GetIntFromObj(interp,p_to, &begpos) != TCL_OK)
  {
    return send_answer_by_text( p_mbl, "Wrong arg <Begpos>", TCL_ERROR );
  }
  if(begpos < 0)
  {
    return send_answer_by_text( p_mbl, "Wrong arg <Begpos>: must be >= 0", TCL_ERROR );
  }
  
  /* #4 Len */
  n_arg++;
  Tcl_ListObjIndex(interp,p_list_obj, n_arg,&p_to);
  int lenpos;
  if( Tcl_GetIntFromObj(interp,p_to, &lenpos) != TCL_OK)
  {
    return send_answer_by_text( p_mbl, "Wrong arg <Len>", TCL_ERROR );
  }
  if(lenpos <= 0)
  {
    return send_answer_by_text( p_mbl, "Wrong arg <Len>: must be > 0", TCL_ERROR );
  }
  
  /* #5 Regular expression */
  n_arg++;
  Tcl_ListObjIndex(interp,p_list_obj, n_arg,&p_to);
  p_str = Tcl_GetString(p_to);
  if(strlen(p_str) > csaControl->csa()->ctrl_mes_item[0].get_ctrl_str_size()-1 )
  {
    return send_answer_by_text( p_mbl, "Wrong args <Str>: is too long", TCL_ERROR );
  }
  
  close_control_for_fd(fd);
  for(int i=0; i<_MAX_MES_CTRLS_; i++)
  {
    _ControlMesItem_ &p_mes_ctrl = csaControl->csa()->ctrl_mes_item[i];
    if( p_mes_ctrl.ctrl_flag == _CTRL_FLAG_FREE_ ) 
    {
      strcpy(p_mes_ctrl.ctrl_str, p_str );
      p_mes_ctrl.ctrl_fd = fd;
      p_mes_ctrl.ctrl_begpos = begpos;
      p_mes_ctrl.ctrl_len    = lenpos;
      p_mes_ctrl.ctrl_who    = who;
      p_mes_ctrl.ctrl_time   = time(NULL);
      p_mes_ctrl.ctrl_flag   = _CTRL_FLAG_BUSY_;
      return send_answer_by_text( p_mbl, "OK", TCL_OK );
    }
  }
return send_answer_by_text( p_mbl, "Not enouth space for ctrl mes", TCL_ERROR );
}

/****************************************************************/
void timeout_control(void)
{
  time_t tm1 = time(NULL);
  for(int i=0; i<_MAX_MES_CTRLS_; i++)
  {
    _ControlMesItem_ &p_mes_ctrl = csaControl->csa()->ctrl_mes_item[i];
    if( p_mes_ctrl.ctrl_flag == _CTRL_FLAG_DEL_ )
    {
       int dt = difftime( tm1, p_mes_ctrl.ctrl_time);
       if(dt > CTRL_TO_CLR)
       {
         p_mes_ctrl._reset();
       }
    }
  }
  for(int i=0; i<_MAX_ERR_CTRLS_; i++)
  {
    _ControlErrItem_ &p_err_ctrl = csaControl->csa()->ctrl_err_item[i];
    if( p_err_ctrl.ctrl_flag == _CTRL_FLAG_DEL_ )
    {
       int dt = difftime( tm1, p_err_ctrl.ctrl_time);
       if(dt > CTRL_TO_CLR)
       {
         p_err_ctrl._reset();
       }
    }
  }
}

/****************************************************************/
void close_control_for_fd(int fd)
{

  for(int i=0; i<_MAX_MES_CTRLS_; i++)
  {
    _ControlMesItem_ &p_mes_ctrl = csaControl->csa()->ctrl_mes_item[i];
    if( ( p_mes_ctrl.ctrl_flag == _CTRL_FLAG_BUSY_    ) &&
        ( (p_mes_ctrl.ctrl_fd   == fd) || (fd == -1 ) )  )
    {
      p_mes_ctrl.ctrl_flag = _CTRL_FLAG_DEL_;
      p_mes_ctrl.ctrl_time = time(NULL);
    }
  }
  for(int i=0; i<_MAX_ERR_CTRLS_; i++)
  {
    _ControlErrItem_ &p_err_ctrl = csaControl->csa()->ctrl_err_item[i];
    if( ( p_err_ctrl.ctrl_flag == _CTRL_FLAG_BUSY_   ) &&
        ( (p_err_ctrl.ctrl_fd   == fd) || (fd == -1) )  )
    {
      p_err_ctrl.ctrl_flag = _CTRL_FLAG_DEL_;
      p_err_ctrl.ctrl_time = time(NULL);
    }
  }
}

/****************************************************************/
void show_control(void)
{
  time_t tm1 = time(NULL);
  for(int i=0; i<_MAX_MES_CTRLS_; i++)
  {
    _ControlMesItem_ &p_mes_ctrl = csaControl->csa()->ctrl_mes_item[i];
    int dt = difftime( tm1, p_mes_ctrl.ctrl_time);
    if( p_mes_ctrl.ctrl_flag != _CTRL_FLAG_FREE_ )
    {
      printf( "%d %x %d %d(%d) %s %d\n",
              p_mes_ctrl.ctrl_fd,
              p_mes_ctrl.ctrl_flag,
              p_mes_ctrl.ctrl_who,
              p_mes_ctrl.ctrl_begpos,
              p_mes_ctrl.ctrl_len,
              p_mes_ctrl.ctrl_str,
              (p_mes_ctrl.ctrl_flag==_CTRL_FLAG_BUSY_)?0:dt );
    }
  }
  for(int i=0; i<_MAX_ERR_CTRLS_; i++)
  {
    _ControlErrItem_ &p_err_ctrl = csaControl->csa()->ctrl_err_item[i];
    int dt = difftime( tm1, p_err_ctrl.ctrl_time);
    if( p_err_ctrl.ctrl_flag != _CTRL_FLAG_FREE_ )
    {
      printf( "%d %x %d\n",
              p_err_ctrl.ctrl_fd,
              p_err_ctrl.ctrl_flag,
              (p_err_ctrl.ctrl_flag==_CTRL_FLAG_BUSY_)?0:dt );
    }
  }
printf("\n");
}


namespace monitorControl {
/**********************************************/
/* type = 0 - запрос */
/*        1 - ответ  */
int is_mes_control(int type, const char *head, int hlen, const char *buf, int blen)
{
 //_ControlMesItem_ *p_mes_ctrl;
// _ControlMesItem_ s_mes_ctrl;
 int blen_max = -1;
 char dop_head[200];
 int dop_head_fl=0;
 _CtrlsFd_ s_ctrls_fd;

  for(int i_ctrl=0; i_ctrl<_MAX_MES_CTRLS_; i_ctrl++)
  {
    _ControlMesItem_ &p_mes_ctrl = csaControl->csa()->ctrl_mes_item[i_ctrl];
    
    if( p_mes_ctrl.ctrl_flag == _CTRL_FLAG_BUSY_ )
    {
      blen_max = -1;
      if( p_mes_ctrl.ctrl_who == _CTRL_WHO_HEAD_ && hlen > 0 && head != NULL )
      {
        if(compare_mes_control(p_mes_ctrl, head, hlen) == RETCODE_OK)
        {
           s_ctrls_fd.add_ctrls_fd(p_mes_ctrl.ctrl_fd);
        }
      }
      else if( p_mes_ctrl.ctrl_who == _CTRL_WHO_TEXT_ && blen > 0 && buf != NULL)
      {
        if(compare_mes_control(p_mes_ctrl, buf, blen) == RETCODE_OK)
        {
          s_ctrls_fd.add_ctrls_fd(p_mes_ctrl.ctrl_fd);
        }
      }
      
      if( p_mes_ctrl.ctrl_who == _CTRL_WHO_PULT_ && hlen > 0 && head != NULL )
      {
        _ControlMesItem_ s_mes_ctrl;
        s_mes_ctrl._copy(p_mes_ctrl);
        if(head[0] == 1)
        {
          s_mes_ctrl.ctrl_begpos = TAP_OFF_FOR_SIRENA_HEAD;
          s_mes_ctrl.ctrl_len = 6;
          if(compare_mes_control(s_mes_ctrl, head, hlen) == RETCODE_OK)
          {
            char tap_str[10+1];
            memcpy(tap_str,head+TAP_OFF_FOR_SIRENA_HEAD, 6);
            tap_str[6] = 0;
            snprintf(dop_head, sizeof(dop_head),"%d %s",head[0],tap_str);
            dop_head_fl=1;
            s_ctrls_fd.add_ctrls_fd(s_mes_ctrl.ctrl_fd);
          }
        }
        else if(head[0] == 2)
        {
          char tap_str[10+1];
          short groupid;
          blen_max = _ML_CTRL_MES_FOR_XML_;
          memcpy(&groupid,head+TAP_OFF_FOR_INET_HEAD,2);
          groupid=ntohs(groupid);        
          snprintf(tap_str,sizeof(tap_str),"%hd",groupid);
          if(0==strncmp(s_mes_ctrl.ctrl_str,tap_str,8))
          {
            snprintf(dop_head,sizeof(dop_head),"%d %s",head[0],tap_str);
            dop_head_fl=1;
            s_ctrls_fd.add_ctrls_fd(s_mes_ctrl.ctrl_fd);
          }
        }
        else if(head[0] == 3)
        {
          blen_max = _ML_CTRL_MES_FOR_XML_;
          s_mes_ctrl.ctrl_begpos = TAP_OFF_FOR_XML_HEAD;
          s_mes_ctrl.ctrl_len = 8;
          if(compare_mes_control(s_mes_ctrl, head, hlen) == RETCODE_OK)
          {
            char tap_str[10+1];
            memcpy(tap_str,head+TAP_OFF_FOR_XML_HEAD, 6);
            tap_str[6] = 0;
            snprintf(dop_head,sizeof(dop_head),"%d %s",head[0],tap_str);
            dop_head_fl=1;
            s_ctrls_fd.add_ctrls_fd(s_mes_ctrl.ctrl_fd);
          }
        }
      }
      else
      {
         continue;
      }
    }
  }
  send_mes_control(s_ctrls_fd, type, head, hlen, buf, blen,blen_max,
     (dop_head_fl==1)?dop_head:NULL);
return RETCODE_OK;  
}

/**********************************************/
int is_log_control(const char *head, int hlen)
{
 _CtrlsFd_ s_ctrls_fd;

  for(int i_ctrl=0; i_ctrl<_MAX_MES_CTRLS_; i_ctrl++)
  {
    _ControlMesItem_ &p_mes_ctrl = csaControl->csa()->ctrl_mes_item[i_ctrl];
    
    if( p_mes_ctrl.ctrl_flag == _CTRL_FLAG_BUSY_ )
    {
      if( p_mes_ctrl.ctrl_who == _CTRL_WHO_LOG_ &&
          hlen > 0 && head != NULL)
      {
        if( compare_mes_control(p_mes_ctrl, head, hlen) == RETCODE_OK)
        {
          s_ctrls_fd.add_ctrls_fd(p_mes_ctrl.ctrl_fd);
        }
      }
      else
        continue;
    }
  }
  send_mes_control(s_ctrls_fd, 0, head, hlen, NULL, 0, -1, NULL);
return RETCODE_OK;  
}

/**********************************************/
int is_errors_control(const char *head, int hlen)
{
 _CtrlsFd_ s_ctrls_fd;

  for(int i_ctrl=0; i_ctrl<_MAX_ERR_CTRLS_; i_ctrl++)
  {
    _ControlErrItem_ &p_err_ctrl = csaControl->csa()->ctrl_err_item[i_ctrl];
    
    if( p_err_ctrl.ctrl_flag == _CTRL_FLAG_BUSY_ )
    {
      if( hlen > 0 && head != NULL)
      {
          s_ctrls_fd.add_ctrls_fd(p_err_ctrl.ctrl_fd);
      }
    }
  }
  
  send_mes_control(s_ctrls_fd, 0, head, hlen, NULL, 0, -1, NULL);
return RETCODE_OK;  
}

/**********************************************/
int compare_mes_control(_ControlMesItem_ &p_mes_ctrl, const char *text, int tlen)
{
  if(p_mes_ctrl.ctrl_begpos >= tlen)
  {
    return RETCODE_NONE;
  }
  int rest_len = tlen-p_mes_ctrl.ctrl_begpos;
  if(rest_len > p_mes_ctrl.ctrl_len)
  {
    rest_len = p_mes_ctrl.ctrl_len;
  }
  Tcl_Obj *pobj_mes = Tcl_NewStringObj(text+p_mes_ctrl.ctrl_begpos, rest_len);
  if(pobj_mes == NULL)
  {
    return RETCODE_NONE;
  }
  Tcl_Obj *pobj_ctrl = Tcl_NewStringObj(p_mes_ctrl.ctrl_str, -1);
  if(pobj_ctrl == NULL)
  {
    Tcl_DecrRefCount(pobj_mes);
    return RETCODE_NONE;
  }
  
  // !!!!!!!!!!!! НЕ ЗАБЫТЬ !!!!!!!!!!!!!!!!!!!!!!!!!
  // В Tcl8.5 обнаружилась такая неприятность:
  // если в качестве рег.выражения указать ".ОВКСЕ", то для сообщений от пульта МОВКСЕ Match срабатывает (return 1);
  // во всех остальных проверенных случаях (маска - "МОВКСЕ","МОВКС.","МОВ.СЕ") Match не срабатывает (return 0).
  //
  // Но если перед вызовом Match вызывается какая-нибудь функция для pobj_mes 
  // (проверено на Tcl_GetCharLength() и Tcl_GetString()), то после этого Match срабатывает правильно (return 1).
  // Причем после вызова одной из перечисленных дополнительных функций в структуре pobj_mes происходят изменения
  //
  // После выхода Tcl8.6 надо будет проверить: если этот баг починят, 
  //то можно будет убрать нижеследующий вызов ф-ции Tcl_GetCharLength().
  // !!!!!!!!!!!! НЕ ЗАБЫТЬ !!!!!!!!!!!!!!!!!!!!!!!!!
  Tcl_GetCharLength(pobj_mes);
  
  if( Tcl_RegExpMatchObj(NULL, pobj_mes, pobj_ctrl) == 1)
  {
     Tcl_DecrRefCount(pobj_mes);
     Tcl_DecrRefCount(pobj_ctrl);
     return RETCODE_OK;
  }
  Tcl_DecrRefCount(pobj_mes);
  Tcl_DecrRefCount(pobj_ctrl);
return RETCODE_NONE;
}

/**********************************************/
static UdpServerObrzap controlUdpServ;

void close_control_server(void)
{
  controlUdpServ.close_serv();
}

/**********************************************/
int send_mes_control( _CtrlsFd_ &s_ctrls_fd, int type, const char *head, int hlen, 
   const char *buf, int blen, int blen_max, char *dop_head)
{
  _UdpServAddr_ udps_addr;
  
   if (s_ctrls_fd.n_fd()<=0)
     return RETCODE_OK;
  
  if(controlUdpServ.prepare_cli( &(csaControl->csa()->udps_addr))!=RETCODE_OK)
  {
    return RETCODE_ERR;
  }
  memcpy( (char *)&udps_addr,
          (char *)&(csaControl->csa()->udps_addr),
          sizeof(_UdpServAddr_) );
  
  _Message_ s_mes;
  s_mes.set_mes_ret( TCL_OK );
  s_mes.set_mes_cmd( TCLCMD_CONTROL );

// Если у нас в структуре только один fd, то выставим его в заголовок, 
// а структуру ctrls_fds передавать не будем
  if( s_ctrls_fd.n_fd() == 1 )
  {
    s_mes.set_mes_fd( s_ctrls_fd.fd(0) );
    s_mes.reset_ctrls();
  }
  else
  {
    s_mes.set_mes_fd(0);
    s_mes.set_ctrls(s_ctrls_fd);
  }

  int hlen_cur = 0;
  if( hlen > 0 && head != NULL )
  {
    hlen_cur = s_mes.concat_text( head, hlen );
    if( hlen_cur != hlen )
    {
      LogTrace(TRACE1)<<"Control message is too long for concat (ret="
        <<hlen_cur<<") hlen="<<hlen<<" max="<<s_mes.get_text_size();
      return RETCODE_ERR;
    }
  }
  
  int blen_cur = blen;
  if( blen_max > 0 && blen_cur > blen_max )
  {
     blen_cur = blen_max;
  }
  if( blen_cur > 0 && buf != NULL )
  {
    int ret=s_mes.concat_text(buf ,blen_cur);
    if( ret != blen_cur )
    {
      LogTrace(TRACE1)<<"Control message is too long for concat (ret="<<ret<<")"
        <<" hlen_cur="<<hlen_cur<<" blen_cur="<<blen_cur
        <<" (max="<<s_mes.get_text_size()<<")"
        <<" [blen="<<blen<<" blen_max="<<blen_max<<"]";
      return RETCODE_ERR;
    }
  }
  
  char str_tmp[_DOP_MES_HEAD_LEN_+1];
  int len_tmp=snprintf( str_tmp, sizeof(str_tmp),
                 "%d %d %d %s", 
                 hlen_cur, blen, type, 
                 (dop_head==NULL) ? " " : dop_head );
  s_mes.set_head_str(str_tmp, len_tmp);
  
  _DevFormat_ out_format(MF_LEN);
  _BufSend_ buf4send;
  if( ! convert_Message_to_BufSend( &s_mes, buf4send, out_format ) )
  {
    LogTrace(TRACE1)<<"Control message convert to _BufSend_ error";
    return RETCODE_ERR;
  }

  if( controlUdpServ.send_buf( buf4send.get_ptr_to_send(), buf4send.get_len_for_send(), &udps_addr) != RETCODE_OK )
  {
    controlUdpServ.close_serv();
    return RETCODE_ERR;
  }

  return RETCODE_OK;
}
}; //namespace monitorControl


void mon_ftst(char *str)
{
 FILE *out;
  out = fopen("ttt","a");
  if(out == NULL)
  {
    return;
  }
  fprintf(out, "%s\n", str);
  fflush(out);
  fclose(out);
}

void mon_ftst_str_x(char *str, int x)
{
 FILE *out;
  out = fopen("ttt","a");
  if(out == NULL)
  {
     return;
  }

  fprintf(out, "%s=0x%X\n", str,x);
  fflush(out);
  fclose(out);
}

void mon_ftst_str_i(char *str, int i)
{
 FILE *out;
  out = fopen("ttt","a");
  if(out == NULL)
  {
    return;
  }

  fprintf(out, "%s=%d\n", str,i);
  fflush(out);
  fclose(out);
}
void mon_ftst_str_2i(char *str, int i1,int i2)
{
 FILE *out;
  out = fopen("ttt","a");
  if(out == NULL)
  {
    return;
  }

  fprintf(out, "%s=%d(%d)\n", str,i1,i2);
  fflush(out);
  fclose(out);
}

/****************************************************************/
int mon_fprint_hex(int i, char ch)
{
  char str_hex[]="0123456789ABCDEF";
  int c1;
  int c2;
  FILE *out;
  out = fopen("ttt","a");
  if(out == NULL)
  {
    return RETCODE_ERR;
  }
  if( ((unsigned char)ch) >=0x20 && 
      ((unsigned char)ch) <=0xF0  )
  {
    fprintf(out,"%i - %c\n",i,ch);
  }
  else
  {
    c1 = (ch&0xF0)>>4;
    c2 = ch&0x0F;
    fprintf(out, "%i - <%c%c>\n", i,str_hex[c1], str_hex[c2]);
  }
  fclose(out);
return RETCODE_OK;
}

/****************************************************************/
void recode_mes_to_visual(char *src_str, int src_len, char *dest_str, int dest_len, int len4hex )
{
 char str_tmp[10];
 char str_hex[]="0123456789ABCDEF";
 
  int i_dest=0;
  for(int i_src=0;i_src<src_len;i_src++)
  {
    char ch = src_str[i_src];
    if( (i_src>=len4hex) && (((unsigned char)ch) >= 0x20 && ((unsigned char)ch) <= 0xF0) )
    {
      str_tmp[0] = ch;
      str_tmp[1] = 0;
    }
    else
    {
      int c1 = (ch&0xF0)>>4;
      int c2 = ch&0x0F;
      sprintf(str_tmp,"<%c%c>", str_hex[c1], str_hex[c2]);
    }
    int l=strlen(str_tmp);
    if ((dest_len - 1) < (i_dest + l)) {
        break;
    }
    memcpy(dest_str+i_dest, str_tmp, l);
    i_dest+=l;
  }
dest_str[i_dest]=0;
}
