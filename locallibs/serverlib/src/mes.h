#ifndef __MES_H__
#define __MES_H__

#include <new>
#include <deque>
#include <cstdio>

#define RETCODE_OK    1
#define RETCODE_NONE  0
#define RETCODE_ERR  -1
#define RETCODE_EXIT -2


#define TCLCMD_COMMAND      0
#define TCLCMD_SET_FLAG     1
#define TCLCMD_SHOW         2
#define TCLCMD_CONTROL      3
#define TCLCMD_PROCCMD      4
#define TCLCMD_NEEDSET_FLAG 5
#define TCLCMD_CUR_REQ      6


#define MAX_TAIL_LEN    20 
#define TAIL_IN_MES  "\r\n"
#define TAIL_OUT_MES "\r\n==>"

#define MF_Empty   0x00
#define MF_LEN     0x01
#define MF_TAIL    0x02
#define MF_RET     0x04
#define MF_MASK    0x07

class _DevFormat_
{
 private:
    char _tail_str[MAX_TAIL_LEN+1];
    size_t  _tail_len;
    unsigned short _format_flag;
 public:
    
    const char *tail_str() const {return _tail_str;}
    size_t tail_len() const {return _tail_len;}
    unsigned short format_flag() const { return (_format_flag&MF_MASK);}
    bool is_set_format_flag(unsigned short fl) const { return (_format_flag&fl);}
   void _reset()
   {
     _format_flag=MF_Empty;
     memset(_tail_str,0,sizeof(_tail_str));
     _tail_len=0;
     
   }
   _DevFormat_()
   {
     _reset();
   }
   _DevFormat_(unsigned short a_format_flag)
   {
     _reset();
     set_format( a_format_flag, NULL); 
   }

  void set_tail(const char *p_tail, size_t p_len)
  {
    _tail_len = p_len;
    if(p_tail==NULL || _tail_len<=0)
    {
      _tail_len=0;
      return;
    }
    if( _tail_len > MAX_TAIL_LEN )
    {
      _tail_len = MAX_TAIL_LEN;
    }
    memcpy(_tail_str, p_tail, _tail_len);
    _tail_str[_tail_len] = 0;
  }
   
  void set_tail(const char *p_tail)
  {
    set_tail(p_tail, (p_tail==NULL)?0:strlen(p_tail));
  }
   
   void set_format( unsigned short a_format_flag, const char *p_tail, size_t p_len) 
   {
     _format_flag = (a_format_flag&MF_MASK);
     set_tail(p_tail,p_len);
   }
   void set_format( unsigned short a_format_flag, const char *p_tail) 
   {
     _format_flag = (a_format_flag&MF_MASK);
     set_tail(p_tail);
   }
   
   void _copy(_DevFormat_ &src)
   {
     set_format( src.format_flag(), src.tail_str(), src.tail_len() );
   }
};


#define _MAX_CTRLS_FD_ 300 /*д.б.=_MAX_MES_CTRLS_+_MAX_ERR_CTRLS_*/
/*********************************************************/
class _CtrlsFd_
{
private:
 int _n_fd;
 int _fd[_MAX_CTRLS_FD_];
 
public:
 _CtrlsFd_() { _reset(); }
 void _reset()
 {
   _n_fd=0;
   for(int i=0;i<_MAX_CTRLS_FD_;i++) _fd[i]=0;/*или может быть лучше -1??? */
 }
 int n_fd() {return _n_fd;}
 int fd(int num) { return ( (num>=0 && num<n_fd() ) ? _fd[num] : -1 ); }
 bool add_ctrls_fd(int a_fd)
 {
#ifdef KSE_TODO   
// А может быть при (a_fd<0) надо вернуть true???
#endif/* KSE_TODO*/   
   if( a_fd<0 || _n_fd>=_MAX_CTRLS_FD_)
     return false;
   _fd[_n_fd]=a_fd;
   _n_fd++;
  return true;
 }
 
 void print_ctrls_fd()
 {
  printf("p_ctrls_fd->n_fd=%i\n",_n_fd);
  for(int i=0;i<_n_fd;i++)
  {
    printf("%3.3i - %i\n",i,_fd[i]);
  }
 }
 
 void _copy( _CtrlsFd_& src)
 {
   _reset();
   _n_fd=src.n_fd();
   for(int i=0;i<_n_fd;i++)
    _fd[i]= src.fd(i);
 }
 
 int get_full_ctrls_len()
 {
   if(_n_fd<=0)
     return 0;
   return sizeof(_n_fd)+_n_fd*sizeof(_fd[0]);
 }
 int get_ctrls_size() {return sizeof(*this); }
 
};

class _MesHead_
{
 public:  
  int len_text;
  int fd;
  int cmd;
  int ret;
  int head_str_len;
  int ctrls_len;

  void set_ctrls_len(int l) { ctrls_len=l;}
  int get_ctrls_len() { return ctrls_len;}

  //head_str
  int get_head_str_len() { return head_str_len;}
  void set_head_str_len(int l) {head_str_len=l;}   

  _MesHead_ &get_mes_head() { return *this; }
  int get_mes_len() {return len_text;}
  int get_mes_fd() {return fd;}
  int get_mes_cmd() {return cmd;}
  int get_mes_ret() {return ret;}

  
  void set_mes_len(int a_len) {len_text=a_len;}
  void set_mes_fd(int a_fd) {fd=a_fd;}
  void set_mes_cmd(int a_cmd) {cmd=a_cmd;}
  void set_mes_ret(int a_ret) {ret=a_ret;}
  void decrease_mes_len(int decr) {if(len_text>decr) {len_text-=decr;} else {len_text=0;}}
  void increase_mes_len(int incr) {len_text+=incr;}
  void _copy( const _MesHead_ &src)
  {
    _reset();//????
    len_text=src.len_text;
    fd=src.fd;
    cmd=src.cmd;
    ret=src.ret;
    head_str_len=src.head_str_len;
    ctrls_len=src.ctrls_len;
  }
  
  
  void _reset()
  {
    len_text=0;
    fd=-1;
    cmd=TCLCMD_COMMAND;
    ret=0;
    head_str_len=0;
    ctrls_len=0;
  }
  
  _MesHead_()
  {
    _reset();
  }
  
};

#define _DOP_MES_HEAD_LEN_       200
#define _MAX_MES_LEN_            100000
#define _RECV_SEND_ADD_LEN_      1000
#define _MAX_RECV_SEND_MES_LEN_  (_MAX_MES_LEN_+_RECV_SEND_ADD_LEN_)     
class _Message_
{
public:
  _MesHead_ mes_head;
  char      mes_text[_MAX_MES_LEN_];
  char      _head_str[_DOP_MES_HEAD_LEN_];
  _CtrlsFd_ _ctrls_fds;
  
  _MesHead_ &get_mes_head() { return mes_head; }
  int get_full_mes_len() 
  {
    return get_mes_head_size() +  
           get_mes_len() +  
           get_ctrls_len()+
           get_head_str_len();
  }
  bool check_mes_head(char *e_str, int e_len);
  //_CtrlsFd_
  _CtrlsFd_ &get_ctrls() {return _ctrls_fds; }
  void reset_ctrls()
  {
    _ctrls_fds._reset();
    get_mes_head().set_ctrls_len(0);
  }
  void set_ctrls(_CtrlsFd_ &src_ctrls_fds)
  {
    _ctrls_fds._copy(src_ctrls_fds);
    get_mes_head().set_ctrls_len(_ctrls_fds.get_full_ctrls_len());
  }
  void set_ctrls(char *p_str, int len)
  {
    if(len < 0) 
      len=0;
    if( len > get_ctrls_size() ) 
      len = get_ctrls_size();
    memcpy((char *)&_ctrls_fds, p_str, len);
  }
  int get_ctrls_len() {return get_mes_head().get_ctrls_len(); }
  int get_ctrls_size() {return _ctrls_fds.get_ctrls_size(); }

  // head_str
  char *get_head_str()         { return _head_str;}
  int get_head_str_size()      { return sizeof(_head_str);}
  void set_head_str_len(int l) { get_mes_head().set_head_str_len(l);}   
  int get_head_str_len()       { return get_mes_head().get_head_str_len();}   
  void reset_head_str()
  {
    memset(_head_str, 0, get_head_str_size() );
    set_head_str_len(0);
  }
  void set_head_str(char *p_str, int len)
  {
    if(len>=get_head_str_size())
      len=get_head_str_size();
    memcpy(_head_str, p_str, len );
    set_head_str_len(len);
  }
  
  int get_text_size() {return sizeof(mes_text);}
  int get_mes_len() {return get_mes_head().get_mes_len();}
  int get_mes_fd() {return get_mes_head().get_mes_fd();}
  int get_mes_cmd() {return get_mes_head().get_mes_cmd();}
  int get_mes_ret() {return get_mes_head().get_mes_ret();}
  static int get_mes_head_size() {return (int)sizeof(_MesHead_); }/*????*/
  void set_mes_ret(int ret) {return get_mes_head().set_mes_ret(ret);}
  void set_mes_len(int a_len) {get_mes_head().set_mes_len(a_len);}
  void set_mes_fd(int fd) {return get_mes_head().set_mes_fd(fd);}
  void set_mes_cmd(int cmd) {return get_mes_head().set_mes_cmd(cmd);}
  void decrease_mes_len(int decr) { get_mes_head().decrease_mes_len(decr);}
  void increase_mes_len(int incr) {get_mes_head().increase_mes_len(incr);}
  
  int concat_text(const char *str)
  {
    if(str==NULL)
      return -1;
    return concat_text(str,strlen(str));
  }
  int concat_text(const char *str, int len) 
  {
    if(str==NULL)
      return -1;
    if( get_mes_len()+len >= get_text_size() )
    {
      len=get_text_size()-get_mes_len();
      if(len<0)
        len=0;
    }
    memcpy(mes_text+get_mes_len(),str,len);
    mes_head.increase_mes_len(len);
    return len;
  }  

  int set_mes_text__rest_to_tail(const char *text)
  {
    return set_mes_text__rest_to_tail(text,strlen(text));
  }
  
  int set_mes_text__rest_to_tail(const char *text,int len)
  {
    return set_mes_text(MAX_TAIL_LEN,text,len);
  }
  
  int set_mes_text(const char *text)
  {
    return set_mes_text(text,strlen(text));
  }
  
  int set_mes_text(const char *text, int len)
  {
    return set_mes_text(0,text,len);
  }
  
  // rest_len - кол-во байт, которое должно остаться после записи text-а
  // (это место может понадобиться для добавления tail_str при отправке монитором сообщения клиенту)
  int set_mes_text(int rest_len, const char *text, int len)
  {
    if( len + rest_len > get_text_size() )
    {
      len = get_text_size() - rest_len;
      if(len<0)
        len=0;
    }
    memcpy( mes_text, text, len);
    get_mes_head().set_mes_len(len);
    return len;
  }
  
  char *get_mes_text() { return  mes_text; }
  _Message_ &get_message() { return *this; }
  
  
  void _copy( _Message_ &src)
  {
    mes_head._copy(src.mes_head);
    memcpy(mes_text,src.mes_text,sizeof(mes_text));
    memcpy(_head_str,src._head_str,sizeof(_head_str));
    _ctrls_fds._copy(src._ctrls_fds);
  }
  
  void _reset()
  {
    mes_head._reset();
    mes_text[0]=0;
    reset_head_str();
    _ctrls_fds._reset();
  }
  
  _Message_() : mes_head() , _ctrls_fds()
  {
    mes_text[0]=0;
  }
};

class _MesBl_ : public _Message_
{
public:  
  inline static const char *_class_const_name() { return "MesBl"; }
  
public:
  int cur_len;
  int full_len;
  char *beg_off;
  
  _MesBl_ &get_mbl() { return *this; }
  
  void prepare_to_send()
  {
    cur_len=0;
    full_len=get_full_mes_len();
    beg_off=(char *)&(get_message());
  }
  
  
  void _copy(_MesBl_ &src)
  {
    cur_len=src.cur_len;
    full_len=src.full_len;
    beg_off=(char *)&(get_message()) + cur_len;
    get_message()._copy( (src.get_message()) ); 
  }
  
  static _MesBl_ *_create()
  {
    try 
    {
      _MesBl_ *p_mbl = new _MesBl_();
      return p_mbl;
    }
    catch (std::bad_alloc&)
    {
      return NULL;
    }
  }
  
  static void _free(_MesBl_ *p_mbl)
  {
    if(p_mbl!=NULL)
    {
      delete p_mbl;
      p_mbl=NULL;
    }
  }
  
  static void _reset(_MesBl_ *p_mbl)
  {
    if(p_mbl==NULL)
      return;
    p_mbl->_reset();
  }
  
  void _reset()
  {
    _reset0();
    _Message_::_reset();
  }
  
  void _reset0()
  {
    cur_len = 0;
    full_len = 0;
    beg_off = (char *)&(get_message());
  }
  
  _MesBl_() : _Message_()
  {
    _reset0();
  }
};

/*********************************************************/
class QueMbl
{
private:
 size_t max_que_size;
 std::deque<_MesBl_ *> qMes;

public:
  size_t get_que_size() { return qMes.size(); }
  bool is_empty() { return ( qMes.empty() ); }
  inline size_t get_max_que_size() { return max_que_size; }
 
  void init(size_t a_max_que_size)
  {
    max_que_size=(a_max_que_size<=0) ? 0 : a_max_que_size ;
  }
  
  QueMbl(size_t a_max_que_size=0)
  {
    init(a_max_que_size);
  }
  
  _MesBl_ *get_first()
  {
   _MesBl_ *p_mbl=NULL;
   if( ! qMes.empty() )
   {
     p_mbl=qMes.front();
     qMes.pop_front();
   }
   return p_mbl;
  }
  
  // Return: true  - поставили в очередь   
  //         false - очередь переполнена 
  bool put_last( _MesBl_ *p_mbl )
  { 
    if(p_mbl==NULL)
      return true;
    if(get_max_que_size() > 0 && qMes.size() >= get_max_que_size() )
    {
       return false;
    }
    qMes.push_back(p_mbl);
   return true;
  }
  
  void put_first( _MesBl_ *p_mbl )
  {
    qMes.push_front(p_mbl);
  }
  
};

/*********************************************************/
#define CLIST_PROCID_MONITOR    0
#define CLIST_PROCID_SUPERVISOR 1
#define CLIST_PROCID_TCLMON     2
#define CLIST_PROCID_OTHER      3

#define MonitorMinMesBl    200
#define SupervisorMinMesBl 100
#define TclmonMinMesBl     100
#define maxMesBl 500

class CachedQMbl : public QueMbl 
{
private:
  int reject_alloc;  // Количество отказов в выдаче свободного буфера
  int cur_created;   // Текущее количество выделенных буферов, 
  int max_created;   // Максимальное количество выделенное буферов
  int procid;        // Идентификатор процесса, для которого эта очередь (из которого вызван clist_init()) 
  
public:
  CachedQMbl() : QueMbl()
  {
     reject_alloc=0;
     cur_created=0;
     max_created=0;
     procid=CLIST_PROCID_OTHER;
  }
  
  void init(int a_procid, size_t a_max_num)
  {
    procid=a_procid;
    QueMbl::init(a_max_num);
  }
  
  void uninit()
  {
  }
  
  void add_created(int cnt=1)
  {
    cur_created += cnt;
    if( max_created < cur_created )
    {
      max_created = cur_created;
    }
  }
  void add_rejected()
  {
    reject_alloc++;
  }
  bool set_freed()
  {
    if(cur_created<=0)
    {
      return false;
    }
    cur_created--;
    return true;
  }
  
  void free_mess( _MesBl_ *p_mbl )
  {
    if( p_mbl == NULL )  { return; }

    if( ! set_freed() )
    {
  #ifdef KSE_TODO
  #endif /* KSE_TODO */
  //???????? Такого быть не должно! Надо ругнуться!!!   
       //LogError(STDLOG)<<"Error in free_mess(): set_freed return 'false'";
    }
  
    _MesBl_::_free( p_mbl);
    p_mbl=NULL;
  }
  
  _MesBl_ *alloc_mess()
  {
  
    _MesBl_ * p_mbl = _MesBl_::_create(); 
    
    if( p_mbl != NULL )
      add_created();
    else //if(p_mbl==NULL)
      add_rejected();

    return p_mbl;
  }
  
#ifdef KSE_TODO
#endif /* KSE_TODO */
  bool show_buf_counts(char *str_mes, int len_mes);
};


class _MesRecv_
{
public:
  char recv_text[_MAX_RECV_SEND_MES_LEN_];
  int  recv_len;
  int cur_len;
  
  void _reset()
  {
    cur_len=0;
    recv_len=sizeof(recv_text);
  }
  _MesRecv_()
  {
    _reset();
  }
  int get_rest_read_len() {return (recv_len - cur_len);}
  char *get_ptr_to_read() 
  {
    if(cur_len<recv_len)
      return recv_text+cur_len;
    return NULL;
  }
  void set_readed(int readed_len) 
  {
    if(readed_len>0) 
    {
      cur_len += readed_len;
      if(cur_len>recv_len)
        cur_len=recv_len;
    }
  }
}/*_MesRecv_*/;

class _BufSend_
{
public:
  char send_text[_MAX_RECV_SEND_MES_LEN_];
  int  filled_len;
  int  sended_len;
  int  bin_data_len; 
  
  void _reset()
  {
    filled_len=0;
    sended_len=0;
    bin_data_len=0;
  }
  _BufSend_()
  {
    _reset();
  }
  int get_sended_len() {return sended_len;}
  int get_filled_len() {return filled_len;}
  int get_bin_data_len() {return bin_data_len;}
  int get_len_for_send() {return (filled_len - sended_len);}
  bool is_exist4send() {return (sended_len<filled_len); }
  char *get_text_ptr() { return send_text; }
  char *get_ptr_to_send() { return sended_len<filled_len ? send_text+sended_len : nullptr; }
  void add_sended_len(int len) 
  {
    if(len>0) 
    {
      sended_len += len;
      if(sended_len>filled_len)
        sended_len=filled_len;
    }
  }
  int get_len_for_add() 
  {
    if( filled_len > (int)sizeof(send_text) )
      return 0;
    return (sizeof(send_text) - filled_len);
  }
  bool add_buf_to_send(const char *buf, int len, bool bin_data_fl=false)
  {
    if(len<0 || len>get_len_for_add() )
      return false;
    memcpy(send_text+filled_len, buf, len);
    if( bin_data_fl && filled_len==bin_data_len )
      bin_data_len+= len;
    filled_len += len;
    return true;
  }
}/*_BufSend_*/;

/*********************************************************/
void clist_init( int procid );

/*********************************************************/
void clist_uninit( void );

/*********************************************************/
void show_buf_counts( char* str_mes, int len_mes );

/*********************************************************/
_MesBl_* new_mbl( void );

/*********************************************************/
void free_mbl( _MesBl_* p_mbl );

/**********************************************/
void clear_Qmbl( QueMbl& p_que );

/**********************************************/
void copy_mbl( _MesBl_* p_mbl_dest, _MesBl_* p_mbl_src );

/**********************************************/
void reset_mbl( _MesBl_* p_mbl );

/**********************************************/
int recv_mbl( int fd, _MesBl_* p_mbl );

/**********************************************/
int send_mbl( int fd, _MesBl_* p_mbl );

typedef int (*handle_control_msg_t)(const char *msg, size_t len);
void mes_for_process_from_monitor(_MesBl_ *p_mbl, handle_control_msg_t);

#endif /* __MES_H__ */
