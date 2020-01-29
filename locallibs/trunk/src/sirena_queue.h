#ifndef __SIRENA_QUEUE_H__
#define __SIRENA_QUEUE_H__

#include "monitor_ctl.h"

#ifdef __cplusplus
#include <string>
#include <vector>
#include <stdint.h>
extern "C" {
#endif

int get_hdr();
int set_hdr(int);

int get_our_public_key(char *pubkey_str, int *pubkey_len);
int get_our_private_key(char *privkey_str, int *privkey_len);
int * get_internal_msgid(void);
#ifdef __cplusplus
}

#ifdef XP_TESTING
void set_internal_msg_id(const int *msgid);
#endif

int save_sym_key(const char *pult, const char *key, int key_len);
int save_sym_key(int client_id, const char *key, int key_len, char *head);

int read_pub_key(const char *pult, char *key, int *key_len, int *sym_key_id);
int read_pub_key(int client_id, char *key, int *key_len);

int main_logger_tcl(Tcl_Interp *interp,int in, int out,Tcl_Obj *list);

size_t getGrp2ParamsByte();
size_t getGrp3ParamsByte();
size_t getParamsByteByGrp(int grp);

bool willSuspend(int flags); // returns true if saved_flag!=0 or saved_timeout!=0 (не соответствует реализации)

// готовит текст (без заголовка) сообщения о переполнении очереди
std::vector<uint8_t> getMsgForQueueFullWSAns(int err_code);

// готовит полный текст (с заголовком) сообщения о переполнении очереди
void createQueueFullWSAns(std::vector<uint8_t>& head, std::vector<uint8_t>& data, int err_code = -1);

#endif

namespace ServerFramework {

bool isPerespros();
bool willPerespros();
void setPerespros(bool val);

#ifdef XP_TESTING

std::string signal_matches_config_for_perespros(const std::string& s);
void setSavedSignal(const std::string& sigtext_);

#endif // XP_TESTING

void make_failed_head(std::vector<uint8_t>& h, const std::string& b);

}

#endif /* __SIRENA_QUEUE_H__ */
