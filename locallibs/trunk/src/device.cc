#if HAVE_CONFIG_H
#endif

#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <tcl.h>
#include "object.h"
#include "msg_queue.h"
#include "tclmon.h"
#include "device.h"
#include "monitor.h"
#include <netdb.h>

#include "msg_framework.h"
#include "lwriter.h"
#ifndef h_errno
extern int h_errno;
#endif

#define NICKNAME "SYSTEM"
#include "slogger.h"

/*    ___DEVICE___   */
/**********************************************/
/*    ___IP___ */

/****************************************************************/
int get_ip_to_ip( char *buf, _IP_ *ip, char *e_str, int e_len )
{
 struct hostent *host;
 struct in_addr addr;
  if(inet_aton(buf,&addr)){
      memcpy( &(ip->l), &addr, sizeof(_IP_) );
  }else{
      host = gethostbyname( buf );
      if( host == (struct hostent *)NULL )
      {
         snprintf(e_str, e_len,
                 "Wrong IP-address (%s) (ERR = %d)",
                 buf,
                 h_errno );
         return RETCODE_ERR;
      }
      memcpy( &(ip->l), host->h_addr, sizeof(_IP_) );
  }
return RETCODE_OK;
}

/********************************************************************/
/* Данная функция производит разбор IP-адреса на октеты             */
/********************************************************************/
bool ip_str_to_dot(char *ip_str, _IpDotStruct_ *ip_dot, char *e_str, int e_len)
{
 char *ip_ptr;
 int  ip_len;
 char  ip_ch[22];
 char *dot_ptr;
 int i;
 _IP_ s_ip;
 char src_addr[20];
  if( get_ip_to_ip( ip_str, &(s_ip), e_str, e_len ) != RETCODE_OK )
  {
     return false;
  }
  snprintf( src_addr,
            sizeof(src_addr),
            "%.16s",
#if defined __SCO__ || defined __UNIXWARE__
            inet_ntoa( s_ip.a ) );
#else /* !defined __SCO__ && !defined __UNIXWARE__*/
            inet_ntoa( (struct in_addr)( s_ip.a ) ) );
#endif /* __SCO__ && __UNIXWARE__ */
  
  memset( (char *)ip_dot,
          0,
          sizeof(_IpDotStruct_) );
  ip_len = strlen(src_addr);
  if( ip_len > 20 )
  {
     return false;
  }
  memcpy(ip_ch, src_addr, ip_len);
  ip_ch[ip_len] = '.';
  ip_ch[ip_len+1] = 0;
  ip_ptr = ip_ch;
  for( i=0; i<4; i++ )
  {
     dot_ptr = strchr(ip_ptr,'.');
     if( dot_ptr == NULL )
     {
        return true;
     }
     *dot_ptr = 0;
     ip_dot->dot[i] = atoi(ip_ptr);
     ip_ptr = dot_ptr+1;
  }
 
return true;
}

/********************************************************************/
int ip_str_to_ip_diap(char *ip_str_src, _IpDiap_ *ip_diap, char *e_str, int e_len)
{
size_t len;
size_t len_beg;
size_t len_end;
char s_ip_beg[108];
char s_ip_end[108];
char *ptr;
char *ip_str;

char ip_str_all[]="0.0.0.0:255.255.255.255";

 memset( (char *)ip_diap,
         0,
         sizeof(_IpDiap_) );
         
 if(ip_str_src[0] == '*')
 {
    ip_str = ip_str_all;
 }
 else
 {
    ip_str = ip_str_src;
 }
 len = strlen(ip_str);
 ptr = strchr(ip_str, ':');
 if(ptr == NULL)
 {
    len_beg = len;
    len_end = 0;
 }
 else
 {
    len_beg = (int)(ptr-ip_str);
    len_end = len - len_beg - 1;
 }
 
 if(len_beg == 0 &&
    len_end == 0  )
 {
    return RETCODE_NONE;
 }
 if(len_beg > sizeof(s_ip_beg)-1 ||
    len_end > sizeof(s_ip_end)-1  )
 {
    snprintf(e_str,e_len,
             "IP-adress (%s) is too long", ip_str);
 }
 if(len_beg != 0)
 {
    memcpy(s_ip_beg, ip_str, len_beg)   ;
    s_ip_beg[len_beg] = 0;
    if( ! ip_str_to_dot(s_ip_beg, &(ip_diap->dot_beg), e_str, e_len) )
    {
       return RETCODE_ERR;
    }
 }
 if(len_end != 0)
 {
    memcpy(s_ip_end, ptr+1, len_end)   ;
    s_ip_end[len_end] = 0;
    if( ! ip_str_to_dot(s_ip_end, &(ip_diap->dot_end), e_str, e_len) )
    {
       return  RETCODE_ERR;
    }
 }
 if(len_end == 0) 
 {
    memcpy( (char *)&(ip_diap->dot_end),
            (char *)&(ip_diap->dot_beg),
            sizeof(_IpDotStruct_));
 }
 if(len_beg == 0) 
 {
    memcpy( (char *)&(ip_diap->dot_beg),
            (char *)&(ip_diap->dot_end),
            sizeof(_IpDotStruct_));
 }
 if( compare_ip_dot( &(ip_diap->dot_beg), &(ip_diap->dot_end) ) > 0)
 {
    snprintf(e_str,e_len,
             "(%s) BegIpAddr > EndIpAddr", ip_str);
   return RETCODE_ERR;
 }
return RETCODE_OK;
}

/********************************************************************/
/*RETURN: -1 - p1 < p2 */
/*         0 - p1 = p2 */
/*         1 - p1 > p2 */
int compare_ip_dot( _IpDotStruct_ *p1, _IpDotStruct_ *p2)
{
 int i;
  for( i=0; i<4; i++ )
  {
     if( p1->dot[i] < p2->dot[i] )
     {
        return -1;
     }
     if( p1->dot[i] > p2->dot[i] )
     {
        return 1;
     }
  }
return 0;
}

/********************************************************************/
/* Данная функция определяет, попадает ли IP-адрес p_src            */
/* в диапазон p_beg - p_end                                         */
/********************************************************************/
/* RETURN: RETCODE_ERR - NO                                                   */
/*         RETCODE_OK - OK                                                   */
/*------------------------------------------------------------------*/
int if_ip_in_diapason(_IpDotStruct_  *p_src, _IpDotStruct_ *p_beg, _IpDotStruct_ *p_end)
{
  if( compare_ip_dot( p_src, p_beg) < 0 )
  {
     return RETCODE_ERR;
  }
  if( compare_ip_dot( p_src, p_end) > 0 )
  {
     return RETCODE_ERR;
  }

return RETCODE_OK;
}


/*    ___TCP___ */
namespace _Tcp {
void _TcpStruct_::set0()
{
  connect = DEV_NOT_INIT;
  sockfd = -1;
  //np.constructor()???
  memset((char*)&np, 0, sizeof(_NetPoint_));
  in_format.set_format(MF_TAIL, TAIL_IN_MES);
  out_format.set_format( (MF_RET | MF_TAIL), TAIL_OUT_MES);
}

void _TcpStruct_::set(_DevFormat_ &_in_format, _DevFormat_ &_out_format)
{
 in_format._copy(_in_format);
 out_format._copy(_out_format);
}

char *_TcpStruct_::get_ip_port_str(struct sockaddr_in &ad)
{
 static char str[200];
 int len=sizeof(str);
 snprintf( str,len,
            "{%.16s:%d}: ",
#if defined __SCO__ || defined __UNIXWARE__
            inet_ntoa( ad.sin_addr ),
#else /* !defined __SCO__ && !defined __UNIXWARE__*/
            inet_ntoa( (struct in_addr)( ad.sin_addr ) ),
#endif /* __SCO__ && __UNIXWARE__ */
             ntohs( ad.sin_port ) );
           
 str[len-1]=0;
return str; 
 
}

char *_TcpStruct_::get_ip_str()
{
 static char str[20];
 int len=sizeof(str);
 snprintf( str,len-1,
          "%.16s",
  #if defined __SCO__ || defined __UNIXWARE__
          inet_ntoa( np.ip.a )
  #else /* !defined __SCO__ && !defined __UNIXWARE__*/
          inet_ntoa( (struct in_addr)( np.ip.a ) )
  #endif /* __SCO__ && __UNIXWARE__ */
          );
 str[len-1]=0;
return str; 
}

char *_TcpStruct_::get_port_str()
{
 static char str[20];
 int len=sizeof(str);
 
 snprintf( str,len-1,"%d",ntohs(np.port) );
 str[len-1]=0;
return str; 
}

char *_TcpStruct_::get_id()
{
 static char str[200];
 return get_id(str, sizeof(str) );
}

char *_TcpStruct_::get_id(char *s, int len)
{
 snprintf( s,len-1,
          "{%d:%.16s:%d}: ",
          sockfd,
  #if defined __SCO__ || defined __UNIXWARE__
          inet_ntoa( np.ip.a ),
  #else /* !defined __SCO__ && !defined __UNIXWARE__*/
          inet_ntoa( (struct in_addr)( np.ip.a ) ),
  #endif /* __SCO__ && __UNIXWARE__ */
          ntohs( np.port ) );
 s[len-1]=0;
return s; 
}

void _TcpStruct_::close_single_serv(short flag)
{
   if(sockfd != -1)
   {
      shutdown(sockfd, 2);
      close(sockfd);
      sockfd = -1;
   }
   connect = DEV_NOT_INIT;
}

/****************************************************************/
int _TcpStruct_::bind_socket()
{
 struct sockaddr_in ad;
 int  last_err;
 int one=1;
 
  if( setsockopt( sockfd, 
                  SOL_SOCKET, 
                  SO_REUSEADDR,
                  &one, sizeof(one) ) == -1 )
  {
     last_err = errno;
     ProgError(STDLOG,
             "Error set socket options (setsockopt) [Err=%d]\n"
             " %s",
             last_err,
             strerror( last_err )  );
     close_single_serv(0);
     return RETCODE_ERR;
  }

  memset( &ad, 0x00, sizeof(ad) );
  memcpy( &(ad.sin_addr), &(np.ip.l), sizeof(_IP_) );
  ad.sin_family = AF_INET;
  ad.sin_port   = np.port;
  if( bind( sockfd, (struct sockaddr *)&ad, sizeof(ad) ) == -1 )
  {
     last_err = errno;
     ProgError(STDLOG,
             "%sError bind socket (bind) [Err=%d]\n"
             " %s",
             get_id( ),
             last_err,
             strerror( last_err ) );
     close_single_serv(0);
     return RETCODE_ERR;
  }
return RETCODE_OK;
}


/****************************************************************/
bool _TcpStruct_::open_socket()
{
 int last_err;
  if( ( sockfd = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
  {
     last_err = errno;
     ProgError(STDLOG,
             "Error open socket (socket) [Err=%d]\n"
             " %s",
             last_err,
             strerror( last_err ) );
     return false;
  }
return true;
}

/****************************************************************/
int _TcpStruct_::set_sock_opt()
{
 int one=1;
 int last_err;
#ifdef __FreeBSD__
 int			    ttl;
 int			    optlen;
#endif /* __FreeBSD__*/

  if( fcntl( sockfd, F_SETFL, O_NONBLOCK ) == -1 )
  {
     last_err = errno;
     ProgError(STDLOG,
             "%sError set socket options (fcntl) [Err=%d]\n"
             " %s",
             get_id( ),
             last_err,
             strerror( last_err ) );
     return RETCODE_ERR;
  }
  if( setsockopt( sockfd, 
                  SOL_SOCKET, 
                  SO_KEEPALIVE,
                  &one, sizeof(one) ) == -1 )
  {
     last_err = errno;
     ProgError(STDLOG,
             "%sError set socket options (setsockopt) [Err=%d]\n"
             " %s",
             get_id( ),
             last_err,
             strerror( last_err )  );
     return RETCODE_ERR;
  }
#ifdef __FreeBSD__
     ttl = TTL;
     optlen = sizeof( ttl );
     if( setsockopt( sockfd, IPPROTO_IP, IP_TTL, &ttl, optlen ) == -1 )
     {
        last_err = errno;
        ProgError(STDLOG,
                "%sError setsockopt IP_TTL [Err=%d]\n"
                " %s",
                get_id( ),
                last_err,
                strerror( last_err )  );
        return RETCODE_ERR;
     }
     optlen = sizeof(ttl);

     if( getsockopt( sockfd, IPPROTO_IP, IP_TTL, &ttl, &optlen ) == -1 )
     {
        last_err = errno;
        ProgError(STDLOG,
                "%sError getsockopt IP_TTL [Err=%d]\n"
                " %s",
                get_id( ),
                last_err,
                strerror( last_err ) );
       return RETCODE_ERR;
    }

    if( optlen != 4  || 
        ttl    != TTL )
    {
       last_err = errno;
       ProgError(STDLOG,
               "%sError set ttl [Err=%d]\n"
               " %s",
               get_id( ),
               last_err,
               strerror( last_err ) );
       return RETCODE_ERR;
    }
#endif /* __FreeBSD__ */
return RETCODE_OK;
}

/****************************************************************/
bool _TcpStruct_::listen_socket()
{
 int last_err;
 
  if( listen( sockfd, 1 ) == -1 )
  {
     last_err = errno;
     ProgError(STDLOG,
             " Error during waiting connection (listen) [Err=%d]\n"
             " %s",
             last_err,
             strerror( last_err ) );
     close_single_serv(0);
     return false;
  }
  connect = DEV_LISTEN;
return true;
}


/****************************************************************/
bool _TcpStruct_::accept_socket(int &new_sock, struct sockaddr_in &ad)
{
 int                last_err;
 
  socklen_t adlen = sizeof(ad);
  if( ( new_sock = accept( sockfd,
                           (struct sockaddr *)(&ad),
                           &adlen                  ) ) == -1 )
  {
     last_err = errno;
     switch( last_err )
     {
        case EWOULDBLOCK:/* the socket is marked non-blocking and no         */
                         /* connections are present to be accepted           */
           return true;

        default:
           ProgError(STDLOG,
                   " Error accept connection (accept) [Err=%d]\n"
                   " %s",
                   last_err,
                   strerror( last_err ) );
           break;
     }
     close_single_serv(0);
     return false;
  }
  if( adlen != sizeof( struct sockaddr_in ) )
  {
     LogError(STDLOG)
       << get_ip_port_str(ad)
       << " Accept connection (accept):\n"
       << "adlen = ("<<adlen<<") "
       << "sizeof(struct sockaddr_in) = ("<<sizeof(struct sockaddr_in)<<")\n"
       << "Loosing connection";
     close( new_sock );
     return false;
  }
return true;
}

/****************************************************************/
int _TcpStruct_::recv_buf(char *buf, int *buf_len, char *e_str, int e_len )
{
 int last_err;
 int rec_len;
  rec_len = recv( sockfd,  
                  buf,  
                  *buf_len, 
                  0 );
  if( rec_len == -1 )
  {
     last_err = errno;
     switch( last_err )
     {
        case ENOENT:   /* Связь разорвана                                    */
           snprintf( e_str,
                     e_len,
                    "%sConnection lost after recv [Err=%d]",
                    get_id(),
                    last_err);
           return RETCODE_ERR;
             
        case EAGAIN:   /* The socket is marked non-blocking, and the receive */
                       /* operation would block, or a receive timeout had    */
                       /* been set, and the timeout expired before data were */
                       /* received                                           */
        case EINTR:    /* The receive was interrupted by delivery of a       */
                       /* signal before any  data were available             */
           return RETCODE_NONE;
           
        default:
           snprintf( e_str,
                     e_len,
                     "%sError receive message (recv) [Err=%d]\n"
                     " %s\n",
                     get_id(),
                    last_err,
                    strerror( last_err ) );
           return RETCODE_ERR;
     }
  }
  else if( rec_len == 0 )
  {
     snprintf( e_str,
               e_len,
               "%sConnection lost after recv (Ret=0)\n",
               get_id() );
     return RETCODE_ERR;
  }
*buf_len = rec_len;
return RETCODE_OK;
}                 

/****************************************************************/
int _TcpStruct_::send_buf(char *buf, int *buf_len, char *e_str, int e_len)
{
 int len;
 int last_err;
  len = send( sockfd, buf,*buf_len, 0 );
  if( len == -1 )
  {
     last_err = errno;
     switch( last_err )
     {
        case EAGAIN:   /* the socket is marked non-blocking, and the      */
                       /* requested operation would block                 */
           return RETCODE_NONE;

        default:
           snprintf( e_str,
                     e_len,
                    "Error send message (send) [Err=%d]\n"
                    " %s",
                    last_err,
                    strerror( last_err ) );
           return RETCODE_ERR;
     }
  }
*buf_len = len;
return RETCODE_OK;
}                  

void _TcpLine_::set(_DevFormat_ &in_format, _DevFormat_ &out_format)
{
  _TcpStruct_::set(in_format,out_format);
}

void _TcpLine_::close_line()
{
  _TcpStruct_::close_single_serv(0);
}

int _TcpLine_::set_connection(int fd, struct sockaddr_in &ad,_DevFormat_ &in_format,_DevFormat_ &out_format)
{
   sockfd = fd;
   connect = DEV_CONNECTED;
   np.port = ad.sin_port;
   memcpy( (char *)( &( np.ip.l ) ),
           (char *)( &( ad.sin_addr ) ),
           sizeof(_IP_)     );
   ProgTrace(TRACE1,
           "%sAccept connection (accept)",
           get_id() );
   int Ret = set_sock_opt();
   if(Ret != RETCODE_OK)
   {
      close_line();
      return RETCODE_ERR;
   }
return RETCODE_OK;
}



}// namespace _Tcp

/********************************************************************/
/* RETURN: RETCODE_ERR - NO                                         */
/*         RETCODE_NONE - таблица пуста                             */
/*         RETCODE_OK - OK                                          */
/*------------------------------------------------------------------*/
int if_ip_in_tcp_diapason(_IpDotStruct_  *p_src, _TcpDiap_ *p_tcpdiap)
{
 int i;
 _IpDiap_ *p_ipdiap;
 int Ret;
 
 if(p_tcpdiap->n_diap == 0)
 {
    return RETCODE_NONE;
 }
 for(i=0;i<p_tcpdiap->n_diap;i++)
 {
    p_ipdiap = &(p_tcpdiap->diap[i]);
    Ret = if_ip_in_diapason(p_src, &(p_ipdiap->dot_beg), &(p_ipdiap->dot_end));
    if(Ret == RETCODE_OK)
    {
       return RETCODE_OK;
    }
 }
return RETCODE_ERR;
}

/*    ___UDP___ */
namespace _Udp 
{
  
/****************************************************************/
char *_UdpStruct_::get_id()
{
 static char str[200];
 return get_id(str, sizeof(str) );
}

char *_UdpStruct_::get_id(char *s, int len)
{
 int addr_flag = udps_addr.addr_flag;
 if(addr_flag == UDP_ADDR_UN)
 {
    snprintf( s,len-1,"{UNIX:%d:%s}: ", sockfd, udps_addr.uaddr_un.sun_path);
 }
 else /* if(addr_flag == UDP_ADDR_IN) */
 {
    snprintf( s,len-1,"{INET:%d}: ", sockfd);
 }
 s[len-1]=0;
return s;
}

/****************************************************************/
void _UdpStruct_::close_serv()
{
   if(sockfd != -1)
   {
      shutdown(sockfd, 2);
      close(sockfd);
      sockfd = -1;
   }
   connect = DEV_NOT_INIT;
}
/**********************************************************/
bool _UdpStruct_::open_socket()
{
 int last_err;
 int addr_flag;

  addr_flag = udps_addr.addr_flag;
  
  sockfd = socket( (addr_flag==UDP_ADDR_UN)?AF_UNIX:AF_INET, 
                             SOCK_DGRAM, 0 );
  if( sockfd == -1 )
  {
     last_err = errno;
     ProgError(STDLOG,
             "UDP_%s: Error open socket (socket) [Err=%d]\n"
             " %s",
             (addr_flag==UDP_ADDR_UN)?"UNIX":"INET",
             last_err,
             strerror( last_err ) );
     return false;
  }
return true;
}
/*****************************************************/
/*                                                   */
/*****************************************************/
/* RETURN: RETCODE_OK   - OK                           */
/*         RETCODE_ERR  - ERR                          */
/*-----------------------------------------------------*/
int _UdpStruct_::bind_socket()
{
 int last_err;
 int addr_flag;
 int Ret;

  addr_flag = udps_addr.addr_flag;

  if(addr_flag == UDP_ADDR_UN)
  {
     unlink(udps_addr.uaddr_un.sun_path);
  }

  if(addr_flag == UDP_ADDR_UN)
  {
     Ret = bind( sockfd, 
            (struct sockaddr *)&(udps_addr.uaddr_un), 
            sizeof(udps_addr.uaddr_un) );
  }
  else /* (addr_flag == UDP_ADDR_IN)*/
  {
     Ret = bind( sockfd, 
            (struct sockaddr *)&(udps_addr.uaddr_in), 
            sizeof(udps_addr.uaddr_in) );
  }

  if( Ret < 0 )
  {
     last_err = errno; 
     ProgError(STDLOG,
             "%sError bind socket (bind) [Err=%d]\n"
             " %s",
             get_id(),
             last_err,
             strerror( last_err ) );
     return RETCODE_ERR;
  }
return RETCODE_OK;
}

/****************************************************************/
int _UdpStruct_::set_sock_opt()
{
 int last_err;
 int one = 1;

  if( fcntl( sockfd, F_SETFL, O_NONBLOCK ) == -1 )
  {
     last_err = errno;
     ProgError(STDLOG,
             "%sError set socket options (fcntl) [Err=%d]\n"
             " %s",
             get_id(),
             last_err,
             strerror( last_err ) );
     return RETCODE_ERR;
  }
  if(udps_addr.addr_flag == UDP_ADDR_IN)
  {
     if( setsockopt( sockfd, 
                     SOL_SOCKET, 
                     SO_REUSEADDR,
                     &one, sizeof(one) ) == -1 )
     {
        last_err = errno;
        ProgError(STDLOG,
                "%sError set socket options (setsockopt) [Err=%d]\n"
                " %s",
                get_id(),
                last_err,
                strerror( last_err )  );
        return RETCODE_ERR;
     }
  }
  connect = DEV_CONNECTED;
return RETCODE_OK;
}

/****************************************************************/
int _UdpStruct_::recv_buf(char *buf, int *buf_len, char *e_str, int e_len )
{
 int last_err;
 int rec_len;

  rec_len = recv( sockfd,  
                  buf,  
                  *buf_len, 
                  0);
  if( rec_len == -1 )
  {
     last_err = errno;
     switch( last_err )
     {
        case ENOENT:   /* Связь разорвана                                    */
           snprintf( e_str,
                     e_len,
                    "%sConnection lost after recv [Err=%d]",
                    get_id(  ),
                    last_err);
           return RETCODE_ERR;
             
        case EAGAIN:   /* The socket is marked non-blocking, and the receive */
                       /* operation would block, or a receive timeout had    */
                       /* been set, and the timeout expired before data were */
                       /* received                                           */
        case EINTR:    /* The receive was interrupted by delivery of a       */
                       /* signal before any  data were available             */
           return RETCODE_NONE;
           
        default:
           snprintf( e_str,
                     e_len,
                     "%sError receive message (recv) [Err=%d]\n"
                     " %s\n",
                     get_id(  ),
                    last_err,
                    strerror( last_err ) );
           return RETCODE_ERR;
     }
  }
  else if( rec_len == 0 )
  {
     snprintf( e_str,
               e_len,
               "%sConnection lost after recv (Ret=0)\n",
               get_id() );
     return RETCODE_ERR;
  }
*buf_len = rec_len;
return RETCODE_OK;
}              

/****************************************************************/
int _UdpStruct_::send_buf( char *buf, int len, 
                _UdpServAddr_  *p_udps_addr)
{
 int                 last_err;
 struct sockaddr *p_sockaddr;
 int              sockaddr_len;
 
 if(udps_addr.addr_flag == UDP_ADDR_UN)
 {
    p_sockaddr = (struct sockaddr *)&(p_udps_addr->uaddr_un);
    sockaddr_len = sizeof(struct sockaddr_un);
 }
 else /*if(udps_addr.addr_flag == UDP_ADDR_IN)*/
 {
    p_sockaddr = (struct sockaddr *)&(p_udps_addr->uaddr_in);
    sockaddr_len = sizeof(struct sockaddr_in);
 }

  if( sendto( sockfd, buf, len, 0, p_sockaddr, sockaddr_len) != len )
  {
     last_err = errno;
     if(errno == EAGAIN) 
     {
         ProgTrace(TRACE1, "%sError send message (sendto) [Err=%d]\n"
                 " %s", get_id(), last_err, strerror( last_err ));
     } 
     else 
     {
         ProgTrace(TRACE0, "%sError send message (sendto) [Err=%d]\n"
                 " %s", get_id(), last_err, strerror( last_err ));
     }
     return RETCODE_ERR;
  }
return RETCODE_OK;
}

void _UdpStruct_::config_serv(_UdpServAddr_ *p_udps_addr)
{
   memcpy( (char *)&(udps_addr),
           (char *)(p_udps_addr),
           sizeof(_UdpServAddr_) );
}



void UdpServer::preinit()
{
//  _UdpStruct_::set0();
  in_format.set_format( MF_LEN, NULL);
}

/****************************************************************/
int UdpServer::timeout_cli()
{
 int Ret;
  if(is_not_init())
  {
     if( ! open_socket() )
     {
        return RETCODE_ERR;
     }
     Ret = set_sock_opt();
     if(Ret != RETCODE_OK)
     {
        close_serv();
        return Ret;
     }
  }
return RETCODE_OK;  
}

}//namespace _Udp

/*    ___PIPE___ */
namespace _Pipe {
void _MesPipe_::set0()
{
     _in_fd=-1;
     _out_fd=-1;
     in_format.set_format(MF_LEN, NULL);
     out_format.set_format(MF_LEN, NULL);
}

int _MesPipe_::recv_buf( int fd, char *buf, int *buf_len, char *e_str, int e_len )
{
int len_rec;
int last_err;
  len_rec = read(fd, buf, *buf_len);
  if(len_rec < 0)
  {
    last_err = errno;
    switch(last_err)
    {
       case EAGAIN:
          return RETCODE_NONE;
       case EINTR:
          return RETCODE_NONE;
       default:
          snprintf(e_str,
                   e_len,
                   "<Pipe_%d>: Error read (err=%d)\n"
                   "%s", 
                   fd,
                   last_err,
                   strerror(last_err));
          return RETCODE_ERR;
    }
  }
  if(len_rec == 0)
  {
     return RETCODE_NONE;
  }
 *buf_len = len_rec;
return RETCODE_OK; 
}

/**********************************************/
int _MesPipe_::send_buf( int fd, char *buf, int *buf_len, char *e_str, int e_len )
{
 int len;

 e_str[0] = 0;
  len = write( fd, 
               buf, 
               *buf_len);
  if( len < 0 )
  {
     switch( errno )
     {
        case EAGAIN:
           return RETCODE_NONE;

        default:
           snprintf( e_str,
                     e_len,
                    "Error send message (write) [Err=%d]\n"
                    " %s",
                    errno,
                    strerror( errno ) );
           return RETCODE_ERR;
     }
  }
  *buf_len = len;
return RETCODE_OK;
}

}; // namespace _Pipe

/**********************************************/
int pipe_recv_buf( int fd, 
                   char *buf, int *buf_len,
                   char *e_str, int e_len )
{
int len_rec;
int last_err;
  len_rec = read(fd, buf, *buf_len);
  if(len_rec < 0)
  {
    last_err = errno;
    switch(last_err)
    {
       case EAGAIN:
          return RETCODE_NONE;
       case EINTR:
          return RETCODE_NONE;
       default:
          snprintf(e_str,
                   e_len,
                   "<Pipe_%d>: Error read (err=%d)\n"
                   "%s", 
                   fd,
                   last_err,
                   strerror(last_err));
          return RETCODE_ERR;
    }
  }
  if(len_rec == 0)
  {
     return RETCODE_NONE;
  }
 *buf_len = len_rec;
return RETCODE_OK; 
}

/**********************************************/
int pipe_send_buf( int fd, 
                   char *buf, int *buf_len,
                   char *e_str, int e_len )
{
 int len;

 e_str[0] = 0;
  len = write( fd, 
               buf, 
               *buf_len);
  if( len < 0 )
  {
     switch( errno )
     {
        case EAGAIN:
           return RETCODE_NONE;

        default:
           snprintf( e_str,
                     e_len,
                    "Error send message (write) [Err=%d]\n"
                    " %s",
                    errno,
                    strerror( errno ) );
           return RETCODE_ERR;
     }
  }
  *buf_len = len;
return RETCODE_OK;
}

