#ifndef _TLG_H_
#define _TLG_H_

#include "oralib.h"
#include "serverlib/query_runner.h"
#include "astra_consts.h"
#include "EdifactRequest.h"
#include "tlg_source_edifact.h"
#include "tlg_source_typeb.h"
#include "db_tquery.h"

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
    int32_t num;   		      /* ����� ⥫��ࠬ�� */
    uint16_t type;
    char Sender[6];       /* ���ᨬ����� ����, �����襭�� �㫥� */
    char Receiver[6];
    uint16_t TTL;   /* �६� ���㠫쭮� ⥫��ࠬ�� � ᥪ㭤��*/
    char body[MAX_TLG_SIZE];
}AIRSRV_MSG;

namespace TlgHandling{
    class TlgSourceEdifact;
}//namespace TlgHandling

namespace edifact {
int init_edifact();
}//namespace edifact

int main_http_snd_tcl(int supervisorSocket, int argc, char *argv[]);
int main_snd_tcl(int supervisorSocket, int argc, char *argv[]);
int main_srv_tcl(int supervisorSocket, int argc, char *argv[]);
int main_typeb_handler_tcl(int supervisorSocket, int argc, char *argv[]);
int main_typeb_parser_tcl(int supervisorSocket, int argc, char *argv[]);
int main_edi_handler_tcl(int supervisorSocket, int argc, char *argv[]);
int main_iapi_edi_handler_tcl(int supervisorSocket, int argc, char *argv[]);
int main_itci_req_handler_tcl(int supervisorSocket, int argc, char *argv[]);
int main_itci_res_handler_tcl(int supervisorSocket, int argc, char *argv[]);
int main_apps_handler_tcl(int supervisorSocket, int argc, char *argv[]);
int main_apps_answer_emul_tcl(int supervisorSocket, int argc, char *argv[]);

const char* ETS_CANON_NAME();
const char* OWN_CANON_NAME();
const char* ERR_CANON_NAME();
const char* DEF_CANON_NAME();
int HANDLER_PROC_ATTEMPTS();

enum TTlgQueuePriority { qpOutA=1, qpOutAStepByStep=4, qpOutB=3, qpOutApp=2 };

enum TEdiTlgSubtype { stCommon, stItciReq, stItciRes, stIapi };

TEdiTlgSubtype specifyEdiTlgSubtype(const std::string& ediText);
std::string getEdiTlgSubtypeName(TEdiTlgSubtype st);

void putTypeBBody(int tlg_id, int tlg_num, const std::string &tlg_body);
std::string getTypeBBody(int tlg_id, int tlg_num);

void putTlgText(int tlg_id, const std::string &tlg_text);
std::string getTlgText(int tlg_id);

std::string getTlgText2(const tlgnum_t& tnum);

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

void sendEdiTlg(TlgHandling::TlgSourceEdifact& tlg,
                TTlgQueuePriority queuePriority,
                int ttl=20);
void sendTpbTlg(TlgHandling::TlgSourceTypeB& tlg);

int loadTlg(const std::string &text, int prev_typeb_tlg_id, bool &hist_uniq_error);
int loadTlg(const std::string &text);

void procTypeB(int tlg_id, int inc);
bool procTlg(int tlg_id);

#define MAX_CMD_LEN 50000
void sendCmd(const char* receiver, const char* cmd);
void sendCmd(const char* receiver, const char* cmd, int cmd_len);
int bindLocalSocket(const std::string &sun_path);
int waitCmd(const char* receiver, int msecs, char* buf, int buflen);

void sendCmdTlgHttpSnd();
void sendCmdTlgSnd();
void sendCmdTlgSndStepByStep();
void sendCmdTypeBHandler();
void sendCmdAppsHandler();
void sendCmdEdiCommonHandler();
void sendCmdEdiItciReqHandler();
void sendCmdEdiItciResHandler();

void sendCmdEdiHandler(TEdiTlgSubtype st);
void sendCmdEdiHandlerAtHook(TEdiTlgSubtype st);

struct tlg_info
{
  private:
    boost::optional<double> tlg_num;
  public:
    int id;
    std::string text;
    std::string sender;
    int proc_attempt;
    boost::optional<int> ttl;
    BASIC::date_time::TDateTime time;

    void fromDB(DB::TQuery &Qry);
    std::string tlgNumStr() const;
    bool ttlExpired() const;
};

void putTlg2OutQueue_wrap(const std::string& receiver,
                          const std::string& sender,
                          const std::string& type,
                          const std::string& text,
                          int priority,
                          int tlgNum,
                          int ttl);

#endif
