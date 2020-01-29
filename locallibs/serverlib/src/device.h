#ifndef __TCP_H__
 #define __TCP_H__

#ifdef __cplusplus
 #include <vector>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include "mes.h"

 #define __SCO__

 #define _MAX_TCP_SERVERS_ 5
 #define _MAX_TCP_LINES_ 200
 #define _ALL_TCP_ITEMS_ ((1+(_MAX_TCP_LINES_))*(_MAX_TCP_SERVERS_))
 #define TCP_FL_WLINES   0x01
 #define MAX_TCP_ENABLE_DIAP 100

/*Максимальная длина очереди на вывод в TCP*/
 #define TCP_MAX_QUE  30

enum DEV_CLOSE_REASON
{
  CLOSE_AFTER_SEND,
  CLOSE_AFTER_RECV,
  CLOSE_AFTER_POLLHUP,
  CLOSE_AFTER_POLLERR
  
};

enum DEV_CONNECT
{
  DEV_NOT_INIT,
  DEV_LISTEN,
  DEV_CONNECTED
};

 typedef union __IP__
 {
    unsigned int   l;
    struct in_addr a;
 }_IP_;

typedef struct __IP_DOT_STRUCT__
{
   int  dot[4];
}_IpDotStruct_;

typedef struct __IP_DIAPAZON__
{
 _IpDotStruct_ dot_beg;
 _IpDotStruct_ dot_end;
}_IpDiap_;

typedef struct __TCP_DIAP__
{
 _IpDiap_ diap[MAX_TCP_ENABLE_DIAP];
 int n_diap;
}_TcpDiap_;

 typedef struct __NET_POINT__
 {
    _IP_ ip;
    int  port;
 }_NetPoint_;
/****************************************************************/
int get_ip_to_ip( char *buf, _IP_ *ip, char *e_str, int e_len );

/********************************************************************/
/* Данная функция производит разбор IP-адреса на октеты             */
/********************************************************************/
bool ip_str_to_dot(char *ip_str, _IpDotStruct_ *ip_dot, char *e_str, int e_len);

/********************************************************************/
int ip_str_to_ip_diap(char *ip_str_src, _IpDiap_ *ip_diap, char *e_str, int e_len);

/********************************************************************/
/*RETURN: -1 - p1 < p2 */
/*         0 - p1 = p2 */
/*         1 - p1 > p2 */
int compare_ip_dot( _IpDotStruct_ *p1, _IpDotStruct_ *p2);

/********************************************************************/
/* Данная функция определяет, попадает ли IP-адрес p_src            */
/* в диапазон p_beg - p_end                                         */
/********************************************************************/
/* RETURN: RETCODE_ERR - NO                                                   */
/*         RETCODE_OK - OK                                                   */
/*------------------------------------------------------------------*/
int if_ip_in_diapason(_IpDotStruct_  *p_src, _IpDotStruct_ *p_beg, _IpDotStruct_ *p_end);
/********************************************************************/
/* RETURN: RETCODE_ERR - NO                                         */
/*         RETCODE_NONE - таблица пуста                             */
/*         RETCODE_OK - OK                                          */
/*------------------------------------------------------------------*/
int if_ip_in_tcp_diapason(_IpDotStruct_  *p_src, _TcpDiap_ *p_tcpdiap);

namespace _Tcp {
  
 class _TcpStruct_
 {
  public:
    _NetPoint_ np;
    enum DEV_CONNECT connect;
    int sockfd;
    _DevFormat_ in_format;
    _DevFormat_ out_format;
    
  void set0();
  virtual void set(_DevFormat_ &in_format, _DevFormat_ &out_format);
  _TcpStruct_() {set0(); }
  virtual ~_TcpStruct_() {}
  
  _DevFormat_ &get_in_format() { return in_format; }
  _DevFormat_ &get_out_format() { return out_format; }
  bool is_connected() {return (connect==DEV_CONNECTED);}
  bool is_not_inet() {return (connect==DEV_NOT_INIT);}
  int in_fd() { return sockfd; }
  int out_fd() { return sockfd; }
  static char *get_ip_port_str(struct sockaddr_in &ad);
  char *get_ip_str();
  char *get_port_str();
  
  virtual char *get_id();
  virtual char *get_id(char *s, int len);
    
  bool open_socket();
  int bind_socket();
  int set_sock_opt();
  virtual bool listen_socket();
  
  virtual void close_single_serv(short flag);
  virtual bool accept_socket(int &new_sock, struct sockaddr_in &ad);
  int recv_buf(char *buf, int *buf_len, char *e_str, int e_len );
  int send_buf(char *buf, int *buf_len, char *e_str, int e_len);
  
 };
 
 class  _TcpLine_ : public _TcpStruct_
 {
   public:
    _TcpLine_(int a_dev_num, int a_dev_line) {}
    virtual ~_TcpLine_() {}
    virtual void set(_DevFormat_ &in_format, _DevFormat_ &out_format);
    virtual void close_line();
    virtual int set_connection(int fd, struct sockaddr_in &ad,_DevFormat_ &in_format,_DevFormat_ &out_format);
    void light_close_after_send() { close_line();}

    
 };
 
template <class TL> class _TcpServ_ : public _TcpStruct_
{
public:
  
  std::vector<TL>  tcp_lines;
  _TcpDiap_   tcp_enable_diap;
  _TcpDiap_   tcp_disable_diap;
 
 bool is_enable_ip(char *ip_str, char *e_str, int e_len)
 {
 _IpDotStruct_ s_ipdot;
 int Ret;

  if( ! ip_str_to_dot(ip_str, &s_ipdot, e_str, e_len) )  
  {
     return false;
  }
  snprintf(e_str, e_len, "Undescribed connection");
  Ret = if_ip_in_tcp_diapason( &s_ipdot, &(tcp_disable_diap));
  if(Ret == RETCODE_OK)
  {
     return false;
  }
  Ret = if_ip_in_tcp_diapason( &s_ipdot, &(tcp_enable_diap));
  if(Ret == RETCODE_ERR)
  {
     return false;
  }
return true;
 }

 
  _TcpServ_(int a_dev_num=-1) : _TcpStruct_()
  {
    //tcp_enable_diap.constructor()???
    memset( &tcp_enable_diap, 0, sizeof(tcp_enable_diap) );
    //tcp_disable_diap.constructor()???
    memset( &tcp_disable_diap, 0, sizeof(tcp_disable_diap) );
  };
  
  
  TL *get_line_by_sockfd(int sockfd)
  {
    size_t n=tcp_lines.size();
    for(size_t i=0;i<n;i++)
    {
      if(tcp_lines[i].in_fd()==sockfd)
        return &(tcp_lines[i]);
    }
    return NULL;
  }
  TL *get_line_by_num(int dev_num, int dev_line)
  {
    size_t n=tcp_lines.size();
    for(size_t i=0;i<n;i++)
    {
      if( tcp_lines[i].is_connected() && tcp_lines[i].dev_num==dev_num && tcp_lines[i].dev_line==dev_line)
        return &(tcp_lines[i]);
    }
    return NULL;
  }
  
virtual void close_single_serv(short flag)
{
   if(flag&TCP_FL_WLINES)
   {
      for(size_t i=0;i<tcp_lines.size();i++)
      {
         tcp_lines[i].close_line();
      }
   }
   _TcpStruct_::close_single_serv(flag);
}

/****************************************************************/
void timeout_serv()
{
 int Ret;
  if(connect == DEV_NOT_INIT)
  {
     if( ! open_socket() )
     {
        return;
     }
     Ret = bind_socket();
     if(Ret != RETCODE_OK)
     {
        return;
     }
     Ret = set_sock_opt();
     if(Ret != RETCODE_OK)
     {
        return;
     }
     if( ! listen_socket() )
     {
        return;
     }
  }
}

 void light_close_after_send() {};

 virtual char *get_id() {return _TcpStruct_::get_id();}
 virtual char *get_id(char *s, int len)
 {
   snprintf(s, len-1,"{TcpServ_}: ");
   s[len-1]=0;
   return s;
 }
 

}/*_TcpServ_*/;

} // namespace _Tcp 
 
#define UDP_ADDR_UN   0
#define UDP_ADDR_IN   1 
 typedef struct __UDP_SERV_ADDR__
 {
    int addr_flag;  /* UDP_ADDR_UN - addr_un */
                    /* UDP_ADDR_IN - addr_in */
    struct sockaddr_un uaddr_un;
    struct sockaddr_in uaddr_in;
 }_UdpServAddr_;
 

namespace _Udp {

class _UdpStruct_
{
 public:
    _UdpServAddr_ udps_addr;
    enum DEV_CONNECT connect;
    int sockfd;
    
    _UdpStruct_() 
    {
      //udps_addr.constructor()???
      memset( &udps_addr,0,sizeof(udps_addr));
      connect = DEV_NOT_INIT;
      sockfd  = -1;
    }
    virtual ~_UdpStruct_() {}
    int in_fd() {return sockfd;}
    int out_fd() {return sockfd;}
    _UdpServAddr_ *get_udps_addr() {return &(udps_addr);}
    
    char *get_id();
    char *get_id(char *s, int len);

    virtual void config_serv(_UdpServAddr_ *p_udps_addr);
    
   virtual void close_serv();
   bool open_socket();
   int bind_socket();
   int set_sock_opt();
   bool is_connected() {return (connect==DEV_CONNECTED);}
   bool is_not_init() {return (connect==DEV_NOT_INIT);}
   int recv_buf(char *buf, int *buf_len, char *e_str, int e_len );
   int send_buf( char *buf, int len,_UdpServAddr_  *p_udps_addr);
   void light_close_after_send() {};
    
};//_UdpStruct_;
  
class UdpServer : public _UdpStruct_
{
 public:
  _DevFormat_ in_format;
  
 UdpServer() : _UdpStruct_() 
 {
   // in_format.constructor() ???
 }
 virtual ~UdpServer() {}
 
 _DevFormat_ &get_in_format() { return in_format; }
 
 void preinit();
 int timeout_cli();
 virtual void close_serv() {_UdpStruct_::close_serv(); };

};

}//namespace _Udp

namespace _Pipe {
 class _MesPipe_ 
 {
  public:
   int _in_fd;
   int _out_fd;
   _DevFormat_ in_format;
   _DevFormat_ out_format;
   void set0();
   _MesPipe_() 
   { 
     set0();
   }
   
   _DevFormat_ &get_out_format() { return out_format; }
   _DevFormat_ &get_in_format() { return in_format; }
   int init(int in_fd, int out_fd)
   {
     _in_fd=in_fd;
     _out_fd=out_fd;
     return RETCODE_OK;
   }
   int in_fd() {return _in_fd;}
   int out_fd() {return _out_fd;}
   int recv_buf( char *buf, int *buf_len, char *e_str, int e_len )
   {
     return recv_buf( in_fd(), buf, buf_len, e_str, e_len );
   };
   static int recv_buf( int fd, char *buf, int *buf_len, char *e_str, int e_len );
   int send_buf( char *buf, int *buf_len, char *e_str, int e_len )
   {
     return send_buf( out_fd(), buf, buf_len, e_str, e_len );
   };
   static int send_buf( int fd, char *buf, int *buf_len, char *e_str, int e_len );
   char *get_id()
   {
      static char res_str[100];
      return get_id(res_str, sizeof(res_str));
   }
   char *get_id(char *s, int len)
   {
     snprintf(s, len-1, "{PIPE_%d}: ", in_fd());
     s[len-1]=0;
     return s;
   }   
   void light_close_after_send() {};
 };
};// namespace _Pipe





/*    ___TCP___ */


/**********************************************/
int pipe_recv_buf( int fd, 
                   char *buf, int *buf_len,
                   char *e_str, int e_len );

/**********************************************/
int pipe_send_buf( int fd, 
                   char *buf, int *buf_len,
                   char *e_str, int e_len );
#endif /* __cplusplus */
#endif /* __TCP_H__ */
