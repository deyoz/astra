#ifndef _TLG_H_
#define _TLG_H_

#include "oralib.h"
#include "serverlib/query_runner.h"
#include "astra_consts.h"

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
    TLG_CFG_ERR,	/* Квитанция о невозможности передачи тлг через ГРСшлюз */
    TLG_CONN_REQ, /* пинг из шлюза в хост */
    TLG_CONN_RES  /* ответ на пинг из хоста в шлюз */
}TLG_TYPE;

/*все длинные и двубайтные целые передаются в network формате*/
typedef struct AIRSRV_MSG
{
    int32_t num;   		      /* номер телеграммы */
    uint16_t type;
    char Sender[6];       /* пятисимвольный адрес, завершенный нулем */
    char Receiver[6];
    uint16_t TTL;   /* время актуальноти телеграммы в секундах*/
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

int main_http_snd_tcl(int supervisorSocket, int argc, char *argv[]);
int main_snd_tcl(int supervisorSocket, int argc, char *argv[]);
int main_srv_tcl(int supervisorSocket, int argc, char *argv[]);
int main_typeb_handler_tcl(int supervisorSocket, int argc, char *argv[]);
int main_typeb_parser_tcl(int supervisorSocket, int argc, char *argv[]);
int main_edi_handler_tcl(int supervisorSocket, int argc, char *argv[]);

const char* ETS_CANON_NAME();
const char* OWN_CANON_NAME();
const char* ERR_CANON_NAME();
const char* DEF_CANON_NAME();
const int HANDLER_PROC_ATTEMPTS();

enum TTlgQueuePriority { qpOutA=1, qpOutAStepByStep=3, qpOutB=2 };

int getNextTlgNum();
void putTypeBBody(int tlg_id, int tlg_num, const std::string &tlg_body);
std::string getTypeBBody(int tlg_id, int tlg_num,
                         TQuery &Qry); //!!! потом Qry убрать

void putTlgText(int tlg_id, const std::string &tlg_text);
std::string getTlgText(int tlg_id,
                       TQuery &Qry); //!!! потом Qry убрать

bool deleteTlg(int tlg_id);
bool errorTlg(int tlg_id, const std::string &type, const std::string &msg="");
void parseTypeB(int tlg_id);
void errorTypeB(int tlg_id,
                int part_no,
                int &error_no,
                int error_pos,
                int error_len,
                const std::string &text);

int saveTlg(const char * receiver,
            const char * sender,
            const char * type,
            int ttl,
            const std::string &text,
            int typeb_tlg_id = ASTRA::NoExists,
            int typeb_tlg_num = ASTRA::NoExists);

int sendTlg(const char* receiver,
            const char* sender,
            TTlgQueuePriority queuePriority,
            int ttl,
            const std::string &text,
            int typeb_tlg_id,
            int typeb_tlg_num);
void loadTlg(const std::string &text, int prev_typeb_tlg_id);
void procTypeB(int tlg_id, bool inc);
bool procTlg(int tlg_id);

#define MAX_CMD_LEN 50000
void sendCmd(const char* receiver, const char* cmd);
void sendCmd(const char* receiver, const char* cmd, int cmd_len);
int waitCmd(const char* receiver, int msecs, const char* buf, int buflen);

void sendCmdTlgHttpSnd();
void sendCmdTlgSnd();
void sendCmdTlgSndStepByStep();
void sendCmdTypeBHandler();

#endif
