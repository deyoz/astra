#ifndef _TLG_H_
#define _TLG_H_

#include "serverlib/query_runner.h"

/* ����⠭�� �����騥 ���ᨬ���� ���祭�� ��� ⥫��ࠬ� */
#define MAX_TLG_LEN       65536

/* ���ᨬ���� ࠧ��� ��।������� �� UDP ⥫��ࠬ�� (������ �� 業��) */
#define MAX_TLG_SIZE 10240

#define MAX_H2H_HEAD_SIZE	100

typedef enum TLG_TYPE   /*���� type*/
{
    TLG_IN=0,
    TLG_OUT,
    TLG_ACK,		/* ���⠭�� � ����祭�� ���஬ */
    TLG_F_ACK,		/* ���⠭�� � ����祭�� ����⮬ */
    TLG_F_NEG,		/* ���⠭�� � �����⠢�� */
    TLG_CRASH,		/* �� ᮮ�饭��, �� ���⢥ত���� TLG_F_ACK, ������ */
    TLG_ACK_ACK,	/* ���⠭�� � ����祭�� ����⮬ ���⠭権 TLG_F_ACK, TLG_F_NEG � TLG_CRASH */
    TLG_CFG_ERR,	/* ���⠭�� � ������������ ��।�� ⫣ �१ ����� */
    TLG_CONN_REQ, /* ���� �� � � ��� */
    TLG_CONN_RES  /* �⢥� �� ���� �� ��� � �� */
}TLG_TYPE;

/*�� ������ � ��㡠��� 楫� ��।����� � network �ଠ�*/
typedef struct AIRSRV_MSG
{
    long int num;   		      /* ����� ⥫��ࠬ�� */
    unsigned short int type;
    char Sender[6];       /* ���ᨬ����� ����, �����襭�� �㫥� */
    char Receiver[6];
    unsigned short int TTL;   /* �६� ���㠫쭮� ⥫��ࠬ�� � ᥪ㭤��*/
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

int main_http_snd_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
int main_snd_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
int main_srv_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
int main_typeb_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
int main_typeb_parser_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
int main_edi_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);

const char* ETS_CANON_NAME();
const char* OWN_CANON_NAME();
const char* ERR_CANON_NAME();
const char* DEF_CANON_NAME();
const int HANDLER_PROC_ATTEMPTS();

enum TTlgQueuePriority { qpOutA=1, qpOutAStepByStep=3, qpOutB=2 };

bool deleteTlg(int tlg_id);
bool errorTlg(int tlg_id, std::string type, std::string msg="");
int sendTlg(const char* receiver,
            const char* sender,
            TTlgQueuePriority queuePriority,
            int ttl,
            const std::string &text);
void loadTlg(const std::string &text);
bool procTlg(int tlg_id);
//void sendErrorTlg(const char *format, ...);

void sendCmd(const char* receiver, const char* cmd);
bool waitCmd(const char* receiver, int msecs, const char* buf, int buflen);

void sendCmdTlgHttpSnd();
void sendCmdTlgSnd();
void sendCmdTlgSndStepByStep();
void sendCmdTypeBHandler();

#endif
