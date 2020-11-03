#ifndef _MSG_FRAMEWORK_
#define _MSG_FRAMEWORK_
#include "object.h"
#ifdef __cplusplus
#include <vector>
#include "msg_queue.h"

struct Tcl_Obj;
struct Tcl_Interp;

extern "C" {
#endif

struct QueMsg;
struct MessageQueue;
enum SOCK_TYPE
{
    LEV_B_ACCEPT_OUT,           /* ACCEPT_IN  accept incoming clients AF_INET */
    LEV_B_ACCEPT_IN,            /*accept servers - AF_UNIX */
    LEV_B_OUT_STREAM,           /*connected clients/multiplexsors */
    LEV_B_IN_STREAM,            /*connected servers */
    LEV_ANY_ACCEPT_SIGNAL,
    LEV_B_OUT_DGRAM,
    LEV_A_ACCEPT_OUT,
    LEV_A_OUT_STREAM,
    LEV_A_IN_STREAM,
    LEV_C_OUT_STREAM,
    LEV_ANY_SIGNAL,
    APPLICATION_SOCKET,
    CONTROL_SOCK,               /*control connection */
    FREE
};
struct FUNCS
{
    int (*hlen)(void);                   /*number of chars to read the header */
    int (*application) (int socket); /*application specific processing */
    int (*header) (const char *head, int hlen);	/*how much bytes followed based on the header */
    int (*compose) (const char *head, int hlen, const char *body, int blen,
                    char **res, int len, const char *id, int idlen,
                    struct sockaddr_in *addr,int *special
                    );	/*what to put on the queue for resend */
    int (*queue_full)(struct MessageQueue *pq,int len,char *res);
    int (*split) (char *body, int blen, char **newbody, int *newblen,
                  struct sockaddr * addr, int *addrlen);	/*what to send to the socket from the queue */
      struct MessageQueue * (*getq) (const char *head,
                             int hlen, const char *body, int blen ,int special
                             );	/*in what queue put the message */
      struct MessageQueue * (*createqueue) (int);
    void (*delqueue) (struct MessageQueue *);
    int (*getid) (int sock, struct sockaddr * paddr, int *paddrlen,
                  char *id, int idlen);
    int idlen;
    int  (*should_compress)(const char * , int blen); /*check if outgoing msg
                                                    should be compressed*/ 
    void (*adjust_header)(char * , int newlen);  /*set flags and blen after
                                                   compression*/
    int (*is_compressed)(const char * );    /*check if incoming message is
                                                compressd*/
    int  (*should_sym_crypt)(const char * , int blen); /*check if outgoing msg
                                                         should be crypted by symmetrical key*/ 
    int (*is_sym_crypted)(const char * );    /*check if incoming message is
                                                crypted by simmetrical key*/
    int  (*should_pub_crypt)(const char * , int blen); /*check if outgoing msg
                                                         should be crypted by public key*/ 
    int (*is_pub_crypted)(const char * );    /*check if incoming message is
                                                crypted by public key*/
    int  (*should_mespro_crypt)(const char * , int blen); /*check if outgoing msg
                                                         should be crypted by mespro*/ 
    int (*is_mespro_crypted)(const char * );    /*check if incoming message is
                                                crypted by mespro*/
    void (*adjust_crypt_header)(char * , int newlen, int flags, int sym_key_id);  /*set blen and flags after crypting */
    //int (*form_crypt_error)(char *res, char *head, int hlen, int error);

    int (*fHandleControlMsg)(const char *msg, size_t len);
};

struct MessageQueue * findQueueById (const char *id);
struct MessageQueue * findQueueById2 (const char *id,int len);
void proc_init ();
int initAcceptInet (int port, struct FUNCS *pf);
int initAcceptUnix (const char *file, enum SOCK_TYPE stype, struct FUNCS *pf);
int initControl (int control, struct FUNCS *pf);
int connectObr (const char *file, enum SOCK_TYPE, struct FUNCS *pf);
int initUdpSock (int port, struct FUNCS *pf,const char *id);
int initSignalUdp(const char *file, struct FUNCS *pf);
int queueMainLoop (int curtimeout, int (*recon) (void));

// init logger
void logger_main_loop();
int logger_init(int supervisorSocket, int argc, char *argv[], int (*HandleControlMsg)(const char *msg, size_t len));


void set_check_timers(int *pf(void));
void set_def_check_timers( 
        struct MessageQueue * cq,
        struct MessageQueue * cq_spec,
        struct MessageQueue * (*aq)(char*,int),
        struct QueMsg *(*)(struct QueMsg *,struct QueMsg *),
        int(*)(struct QueMsg *,struct QueMsg *),
        int(*)(struct QueMsg *,struct QueMsg *),
        int deftout );

pList getTimerList(void);
//pList getInqueryList(void);
pList getAnswerList(void);

int calc_digest(const unsigned char *src_str, size_t src_len, unsigned char *dest_str, size_t *dest_len);
int levC_form_crypt_error(char *res,char *head, int hlen, int error);


#ifdef __cplusplus
}

struct WatchedInqElem
{
  pQueMsg pm;
  time_t createTime;
  int checkCount;

  WatchedInqElem(pQueMsg _pm) : pm(_pm), createTime(time(0)), checkCount(0) {}
};

typedef std::vector<WatchedInqElem> SigList_;

class InqHolder
{
  // works with signal messages
  static InqHolder *_instance;
  int maxLifeTime;
  SigList_ mm;
  InqHolder() : maxLifeTime(2) { mm.reserve(10); }
  public:
    static InqHolder *instance();
    void dropExpired(); // deletes expired from vector and frees them
    pQueMsg get(const int *msgid); // deletes desired from vector and returns pointer to it or 0 if not found
    void add(pQueMsg sig);
};


#endif

#endif /* _MSG_FRAMEWORK_ */
