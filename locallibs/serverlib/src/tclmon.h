#ifndef __TCLMON_H__
 #define __TCLMON_H__

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
#include <sys/times.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <tcl.h>

#include <string>

void send_signal_tcp_inet(const char* address, short port, const void* data, const size_t len);
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if 0
typedef int (*handle_control_msg_t)(const char *msg, size_t len);
#endif /* 0 */


#ifndef __MAIN12345
#define __MAIN12345

#define SETFLAG_TO 2

#define N_ACTION  7
#define A_NONE    0
#define A_CREATE  1
#define A_RESTART 2
#define A_STOP    4
#define A_WORK    3
#define A_FINAL   5
#define A_FORCED_RESTART 6



typedef struct __FIELD_STR__
{
 int  f_num;
 char f_name[10];
}_Fields_;

#define SET_BIT(a, b) ( (a) |= (b) )
#define CLR_BIT(a, b) ( (a) &= ~(b) )
#define GET_BIT(a, b) ( (a) & (b) )
#define ISSET_BIT(a, b) ( ((a)&(b)) != 0 )
#define ISNSET_BIT(a, b) ( ((a)&(b)) != (b) )
#define ISNSET_BITS(a, b) ( ((a)&(b)) == 0 )

/* Тайм-аут проверки процессов на зависание */
#define TO_TEST_RESTART_PROC        1
#define TO_TEST_QUEFULL_PROC        1
#define TO_TEST_NOT_ASKED_PROC      5

#define PROC_FL_NONE         0x00
#define PROC_FL_MAIN         0x01 /* Если данный флаг выставлен, то данный процесс */
                                  /* не убивается при остановке "рабочих" процессов*/
#define PROC_FL_KILLNOTASKED 0x02 /* Если данный флаг выставлен, то данный процесс */
                                  /* будет убиваться в том случае, если он не отвечает */
                                  /* на запрос контрольного сокета */
#define PROC_FL_DEFMASK      0xFF /* Маска флагов из таблицы */
#define PROC_FL_SPEC         0x10 /* Специальный процесс (не перезапускается при */
                                  /* завершении работы данного процесса */
#define PROC_FL_DISPATCHER   0x20 /* Выставляется для процессов диспетчеров и означает,*/
                                  /* что они не будут перезапущены при перезапуске обработчиков*/

typedef int ProcessHandlerType(int supervisorSocket, int argc, char *argv[]);

struct NAME2F{
    std::string name;
    std::string log_group;
    ProcessHandlerType *pf;
    short proc_flag; /* see PROC_FL_... */
};
/**
 * getProcTable should be provided by application
 * 
 */
extern /*!*/ struct NAME2F const *getProcTable(int *len);
struct sockaddr_un;
int queue_main (int argc, char *argv[], int (*app_init)(Tcl_Interp *), 
        int (*app_start)(Tcl_Interp *), 
        void (*before_exit)(void));
void send_signal_udp(struct sockaddr_un *addr,const char *var,
        const char *var2,
        const void*data, size_t len);
void send_signal_tcp(const char *var,
        const char *var2,int suff,
        const void*data, size_t len);
void send_signal_udp_suff(struct sockaddr_un *addr,const char *var,
        const char *var2,int suff,
        const void*data,int len);
int write_set_flag(int TO_restart, int n_zapr);
int write_set_flag_type(int TO_restart, int n_zapr, int type_zapr, const char* const subgroup);
int getControlPipe();

void Abort(int i);
int main_monitor(int supervisorSocket, int argc, char *argv[]);
int write_set_queue_size(int nmes_que,int max_nmes_que, int write_to_log_flag);
int write_set_cur_req(const char *cur_req);
int write_clear_cur_req(void);
int set_logging(ClientData cl, Tcl_Interp *interp, int objc, Tcl_Obj* CONST objv[]);
void random_sleep();

/* Максимальная длина посылаемого монитору сообщения */
/* от графического терминала или интернет-клиента, стоящего на контроле */
#define _ML_CTRL_MES_FOR_XML_ 500

#if 0
void mes_for_process_from_monitor(_MesBl_ *p_mbl, handle_control_msg_t);
#endif /* 0 */
char const * tclmonCurrentProcessName();
const char* tclmonFullProcessName();

/* list of sys errors */
#define UNKNOWN_KEY               1 /* нет ключа */
#define EXPIRED_KEY               2 /* ключ просрочен */
#define WRONG_KEY                 3 /* ключ неверен */
#define WRONG_SYM_KEY             4 /* wrong symmetric key */
#define WRONG_OUR_KEY             9 /* client has wrong our public key */
#define UNKNOWN_SYM_KEY           5 /* unknown symmetric key */
#define UNKNOWN_ERR               6 /* прочие ошибки */
#define CRYPT_ERR_READ_RSA_KEY    7 /* ошибка чтения публ.ключа RSA */
#define CRYPT_ALLOC_ERR           8 /* memory allocation error */
#define UNKNOWN_CA_CERTIFICATE     10    /* CA сертификат не найден */
#define UNKNOWN_SERVER_CERTIFICATE 11    /* Server сертификат не найден */
#define UNKNOWN_CLIENT_CERTIFICATE 12 /* Client сертификат не найден */
#define WRONG_TERM_CRYPT_MODE      13 /* неправильный режим работы клиента */

#define QUEPOT_MAX           7 /* Количество типов запросов (без учета QUEPOT_NULL)*/
#define QUEPOT_ZAPR          0 /* Обычный запрос */
#define QUEPOT_LEVB          1 /* пришедшие запрос на уровне levB*/
#define QUEPOT_TLG_INP       2 /* Входящие телеграммы */
#define QUEPOT_TLG_EDI       3 /* Телеграмма Edifact */
#define QUEPOT_TLG_AIR       4 /* Телеграмма Airimp */
#define QUEPOT_TLG_RES       5 /* Телеграмма Resource */
#define QUEPOT_TLG_OTH       6 /* Телеграмма Other */
#define QUEPOT_NULL          7 /* Запрос который не учитывается в потоке */
#define QUEPOT_TLG_BEG       (QUEPOT_TLG_EDI) /* начальный номер для типов телеграмм */
#define QUEPOT_TLG_NUM       4 /* количество типов телеграмм */

/* Значения флагов для функции adjust_crypting_header */
#define SYMMETRIC_CRYPTING 1
#define RSA_CRYPTING       2
#define MESPRO_CRYPTING    3

#endif /* __MAIN12345 */

extern pid_t Pid_p;
extern const std::string SOFT_RESTART_CMD;

const char *getUnknownProcessName();

//returns process start time
void setProcStartTime(const time_t t);

// returns non zero if we need to keep silence
// normaly 5 seconds after process start for a list of errors
int startupKeepSilence();

bool isNosir();

#ifdef __cplusplus

int HandleControlMsg(const char* msg, size_t len);

}

int set_sig (void (*p) (int), int sig);

int semaphoreKey();

#endif /* __cplusplus */


#endif /*  __TCLMON_H__*/
