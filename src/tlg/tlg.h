#ifndef _TLG_H_
#define _TLG_H_

//#include "query_runner.h"
#include "daemon.h"

/* константы задающие максимальные значения для телеграмм */
#define MAX_TLG_LEN       65536

/* максимальный размер передаваемой по UDP телеграммы (зависит от центра) */
#define MAX_TLG_SIZE 10240

#define MAX_H2H_HEAD_SIZE	100

typedef enum TLG_TYPE   /*поле type*/
{
    TLG_IN=0,
    TLG_OUT,
    TLG_ACK,		/* Квитанция о получении роутером */
    TLG_F_ACK,		/* Квитанция о получении адресатом */
    TLG_F_NEG,		/* Квитанция о недоставке */
    TLG_CRASH,		/* Все сообщения, не подтвержденные TLG_F_ACK, потеряны */
    TLG_ACK_ACK,	/* Квитанция о получении адресатом квитанций TLG_F_ACK, TLG_F_NEG и TLG_CRASH */
    TLG_CFG_ERR		/* Квитанция о невозможности передачи тлг через ГРСшлюз */
}TLG_TYPE;

/*все длинные и двубайтные целые передаются в network формате*/
typedef struct AIRSRV_MSG
{
    long int num;   		      /* номер телеграммы */
    unsigned short int type;
    char Sender[6];       /* пятисимвольный адрес, завершенный нулем */
    char Receiver[6];
    unsigned short int TTL;   /* время актуальноти телеграммы в секундах*/
    char body[MAX_TLG_SIZE];
}AIRSRV_MSG;

#define H2H_BEG_STR		"V.\rV"

typedef struct H2H_MSG
{
	char data[MAX_TLG_LEN];
	char type;
	char sndr[21];
	char rcvr[21];
	char tpr[21];
	char err[3];
	char part;
	char end;
	char qri5;
	char qri6;
} H2H_MSG;

int init_edifact();

int main_snd_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
int main_srv_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
int main_typeb_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
int main_edi_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);

const char* ETS_CANON_NAME();
const char* OWN_CANON_NAME();
const char* ERR_CANON_NAME();
const char* DEF_CANON_NAME();
const char* OWN_SITA_ADDR();

bool deleteTlg(int tlg_id);
bool errorTlg(int tlg_id, std::string err);
void sendTlg(const char* receiver, const char* sender, bool isEdi, int ttl, const std::string &text);
void loadTlg(const std::string &text);
void sendErrorTlg(const char* receiver, const char* sender, const char *format, ...);

void sendCmd(const char* receiver, const char* cmd);
bool waitCmd(const char* receiver, int secs, const char* buf, int buflen);

void sendCmdTlgSnd();

#endif
