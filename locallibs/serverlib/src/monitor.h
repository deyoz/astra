#ifndef __MONITOR_H__
#define __MONITOR_H__

#include "device.h"
#include <serverlib/noncopyable.h>

/* NpipeFds(1)+UdpServ(1)+(TcpServ(1)+NtcpLines)*_MAX_TCP_SERVERS_ */
#define _MaxMonTable_ (2+_ALL_TCP_ITEMS_)


/* Тайм-аут для очистки удаленных записей контроля */
#define CTRL_TO_CLR 5

#define _ML_ERRSTR_ 1000

//#define minn(x,y) ( ((x)<(y)) ? (x) : (y))
#define _MAX_MES_CTRLS_ 100
#define _MAX_ERR_CTRLS_ 200

#define TAP_OFF_FOR_SIRENA_HEAD 199
#define TAP_OFF_FOR_INET_HEAD   45 
#define TAP_OFF_FOR_XML_HEAD    45

/* see _ControlMesItem_.ctrl_flag и _ControlErrItem_.ctrl_flag*/
#define _CTRL_FLAG_FREE_ 0
#define _CTRL_FLAG_BUSY_ 1
#define _CTRL_FLAG_DEL_  2

#define _CTRL_WHO_HEAD_ 0
#define _CTRL_WHO_TEXT_ 1
#define _CTRL_WHO_LOG_  2
#define _CTRL_WHO_PULT_ 3
#define _CTRL_WHO_ERRS_ 4


class _ControlMesItem_
{
public:
  int     ctrl_fd;
  int     ctrl_flag; /* _CTRL_FLAG_FREE_ - структура свободна */
                     /* _CTRL_FLAG_BUSY_ - структура занята   */
                     /* _CTRL_FLAG_DEL_ - структура освобождается */
  int     ctrl_who;  /* _CTRL_WHO_HEAD_ - контроль заголовка */
                     /* _CTRL_WHO_TEXT_ - контроль текста запроса */
                     /* _CTRL_WHO_LOG_ - контроль логов */
                     /* _CTRL_WHO_PULT_ - контроль пульта */
                     /* _CTRL_WHO_ERRS_ - контроль ошибок */
  int     ctrl_begpos;
  int     ctrl_len;
  char    ctrl_str[100];
  time_t  ctrl_time;
  
  void _reset()
  {
    ctrl_fd=0;
    ctrl_flag=_CTRL_FLAG_FREE_;
    ctrl_who=_CTRL_WHO_HEAD_;
    ctrl_begpos=0;
    ctrl_len=0;
    ctrl_str[0]=0;
    ctrl_time=0;
  }
  void _copy(_ControlMesItem_ &src)
  {
    ctrl_fd=src.ctrl_fd;
    ctrl_flag=src.ctrl_flag;
    ctrl_who=src.ctrl_who;
    ctrl_begpos=src.ctrl_begpos;
    ctrl_len=src.ctrl_len;
    memcpy(ctrl_str,src.ctrl_str,sizeof(ctrl_str));
    ctrl_time=src.ctrl_time;
  }
  
  _ControlMesItem_()
  {
    _reset();
  }
  int get_ctrl_str_size() {return sizeof(ctrl_str);}
};

class _ControlErrItem_
{
public:
  int     ctrl_fd;
  int     ctrl_flag; /* _CTRL_FLAG_FREE_ - структура свободна */
                     /* _CTRL_FLAG_BUSY_ - структура занята   */
                     /* _CTRL_FLAG_DEL_ - структура освобождается */
  time_t  ctrl_time;

  void _reset()
  {
    ctrl_fd=0;
    ctrl_flag=_CTRL_FLAG_FREE_;
    ctrl_time=0;
  }
  _ControlErrItem_()
  {
    _reset();
  }
};

class _CsaControl_
{
public:
 _ControlMesItem_ ctrl_mes_item[_MAX_MES_CTRLS_];
 _ControlErrItem_ ctrl_err_item[_MAX_ERR_CTRLS_];
 _UdpServAddr_ udps_addr;
};

enum DEV_TYPE
{
 DEV_FREE,
 DEV_PIPE,
 DEV_TCPSERV,
 DEV_TCPLINE,
 DEV_UDPSERV
};
 
class _DeviceKey_
{
public:
 enum DEV_TYPE dev_type;
 int  dev_num;
 int  dev_line;
 
 void set0()
 {
   dev_type=DEV_FREE;
   dev_num=-1;
   dev_line=-1;
 }
 _DeviceKey_() {
   set0();
 }
 _DeviceKey_(enum DEV_TYPE a_dev_type,int a_dev_num, int a_dev_line) {
   set(a_dev_type, a_dev_num, a_dev_line); 
 }
 
 void set(_DeviceKey_ &key) {set(key.dev_type, key.dev_num, key.dev_line); }
 
 void set(enum DEV_TYPE a_dev_type,int a_dev_num, int a_dev_line)
 {
   dev_type=a_dev_type;
   dev_num=a_dev_num;
   dev_line=a_dev_line;
 }
 _DeviceKey_ &get_dev_key() {return *this;};
 enum DEV_TYPE get_dev_type() {return dev_type; }
};

/****************************************************************/
class CsaControl :
    private comtech::noncopyable
{
public:
    explicit CsaControl(uint16_t key);

    explicit CsaControl(_CsaControl_* rawCsa);

    ~CsaControl();

    _CsaControl_* csa() {
        return csa_;
    }

private:
    _CsaControl_* csa_;
    int id_;
};
/****************************************************************/
int monitor_loop(Tcl_Interp *interp);

/*****************************************************************/
int send_show(int fd);

/*****************************************************************/
int send_set_flag(int fd, int TO_restart, int n_zapr, int type_zapr);

/****************************************************************/
void add_fd_to_table(int fd, enum DEV_TYPE dev_type, short event, int dev_num, int dev_line);

/****************************************************************/
void change_event_in_table(int fd, short flag, short event);

/****************************************************************/
void delete_fd_from_table(void);

/****************************************************************/
void set_delete_fd_from_table(int fd);

/****************************************************************/
void OurMes_from_pipe( _MesBl_ *p_mbl);

template <class T> void send_message(_DeviceKey_ &p_mon, T &v);

/****************************************************************/
void timeout_control(void);

/****************************************************************/
void close_control_for_fd(int fd);

/****************************************************************/
void show_control(void);


/**********************************************/
void close_control_server(void);


void mon_ftst(char *str);

void mon_ftst_str_x(char *str, int x);

void mon_ftst_str_i(char *str, int i);

void mon_ftst_str_2i(char *str, int i1,int i2);

/****************************************************************/
int mon_fprint_hex(int i, char ch);

namespace monitorControl {
int is_mes_control(int type, const char *head, int hlen, const char *buf, int blen);
int is_log_control(const char *head, int hlen);
int is_errors_control(const char *head, int hlen);
void close_control_server(void);
}; // namespace monitorControl 

#endif /* __MONITOR_H__*/
