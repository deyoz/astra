#if HAVE_CONFIG_H
#endif

/* pragma cplusplus */
#include <string>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <arpa/inet.h>
#include <cstring>
#include "tcl_utils.h"
#include "query_runner.h"
#include "msg_framework.h"
#include "monitor_ctl.h"
#include "ourtime.h"
#include "logger.h"
#include "sirena_queue.h"
#include "profiler.h"
#ifdef USE_MESPRO
#include "mespro_crypt.h"
#endif // USE_MESPRO
#include "proc_ab.h"
#include "proc_c.h"
#include "tcl_args.h"
#include "levc_callbacks.h"
#include "msg_const.h"
#include "fastcgi.h"
#include "fcgi.h"
#include "http_logs_callbacks.h"
#include "http_parser.h"
#include "httpsrv.h"
#include "helpcpp.h"
#include "dates_io.h"
#include "xml_stuff.h"

#define NICKNAME "KONST"
#include "slogger.h"

using namespace ServerFramework;

int main_shmserv(int supervisorSocket, int argc, char *argv[]);
int main_httpsrv(int supervisorSocket, int argc, char* argv[]);

namespace balancer {
int main_balancer(int supervisorSocket, int argc, char* argv[]);
int main_balancer_clients_stats(int supervisorSocket, int argc, char* argv[]);
} // namespace balancer

size_t getGrp2ParamsByte() {  return GRP2_PARAMS_BYTE;  }

size_t getGrp3ParamsByte() {  return GRP3_PARAMS_BYTE;  }

size_t getParamsByteByGrp(int grp)
{
  switch(grp)
  {
    case 2:
      return GRP2_PARAMS_BYTE;
    case 3:
      return GRP3_PARAMS_BYTE;
    default:
      return 0;
  }
}

static int hdr;
static bool is_perespros = false;
void gprofPrepare();

extern "C" int set_hdr(int new_hdr)
{
    return std::exchange(hdr, new_hdr);
}

extern "C" int get_hdr()
{
    return hdr;
}

namespace ServerFramework {

void setPerespros(bool val)
{
    is_perespros = val;
}

bool isPerespros()
{
    return is_perespros;
}

}

static Tcl_Obj *curgrp = NULL;
void set_cur_grp(Tcl_Obj *grp)
{
    curgrp = grp;
    Tcl_IncrRefCount(curgrp);
}

void set_cur_grp_string(const char *s)
{
    if (curgrp) {
        Tcl_DecrRefCount(curgrp);
    }
    curgrp = Tcl_NewStringObj(s, -1);
    Tcl_IncrRefCount(curgrp);
}

Tcl_Obj * current_group(void)
{
    return curgrp;
}

std::string current_group2()
{
    Tcl_Obj* cg = current_group();

    if(cg){
        return std::string( Tcl_GetString(cg) );
    }else{
        return std::string();
    }
}

int HandleControlMsg(const char *msg, size_t len)
{
    if(isProfilingCmd(msg, len)) {
        handle_profiling_cmd(msg, len);
    }

    return 0;
}

std::string getInetAnsParams(const uint8_t *head, const size_t &hlen)
{
  int ext_msgid=0;
  if(hlen>13)
  {
    memcpy(&ext_msgid, head+9, 4);
    ext_msgid=ntohl(ext_msgid);
  }

  short client_id=0;
  if(hlen>47)
  {
    memcpy(&client_id, head+45, 2);
    client_id=ntohs(client_id);
  }

  std::string instance=readStringFromTcl("INSTANCE_NAME", "NIN");

  std::string res;
  res+=" client_id=\""+std::to_string(client_id)+"\"";
  res+=" msgid=\""+std::to_string(ext_msgid)+"\"";
  res+=" time=\""+HelpCpp::string_cast(boost::posix_time::second_clock::local_time(), "%H:%M:%S %d.%m.%Y")+"\"";
  res+=" instance=\""+CP866toUTF8(instance)+"\"";
  return res;
}

std::vector<uint8_t> getMsgForQueueFullWSAns(int err_code, const std::string &ansDetails)
{
    constexpr char h1[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<sirena><answer";
    constexpr char h2[] = "><error code='";
    constexpr char f[]  = "'>Please retry in 15 seconds</error></answer></sirena>";
    const std::string e = std::to_string(err_code);
    std::vector<uint8_t> r(sizeof(h1)-1 + ansDetails.size() + sizeof(h2)-1 + e.size() + sizeof(f)-1);
    std::copy(h1, h1+sizeof(h1)-1, r.data());
    std::copy(ansDetails.begin(), ansDetails.end(), r.data()+sizeof(h1)-1);
    std::copy(h2, h2+sizeof(h2)-1, r.data()+sizeof(h1)-1+ansDetails.size());
    std::copy(e.begin(), e.end(), r.data()+sizeof(h1)-1+ansDetails.size()+sizeof(h2)-1);
    std::copy(f, f+sizeof(f)-1, r.data()+sizeof(h1)-1+ansDetails.size()+sizeof(h2)-1+e.size());
    return r;
}

void createQueueFullWSAns(std::vector<uint8_t>& head, std::vector<uint8_t>& data, int err_code)
{
    ProgTrace(TRACE5,"%s:: err_code=%i, head.size=%zu", __FUNCTION__, err_code, head.size());

    data = getMsgForQueueFullWSAns(err_code,getInetAnsParams(&head[0],head.size()));
    const int newlen = htonl(data.size());

    memcpy(head.data() + 1, &newlen, sizeof(newlen));
    head[GRP2_PARAMS_BYTE] = 0;
    head[GRP2_PARAMS_BYTE + 1] = 0;
    head[GRP2_PARAMS_BYTE + 1] |= REQ_NOT_PROCESSED;
}

static int saved_flag;
static unsigned char saved_timeout;
static int msgid_[3];
static HTTP::request cur_http_request;
static std::string cur_http_raw_request;

extern "C" int * get_internal_msgid(void)
{
  return msgid_;
}

#ifdef XP_TESTING
void set_internal_msg_id(const int *msgid)
{
    memcpy(msgid_, msgid, sizeof(msgid_));
}
#endif

void set_timeout( unsigned  char timeout)
{
    ProgTrace( TRACE1 , "set_timeout to %c" , timeout );
    saved_timeout=timeout;
}
void set_flag_and_timeout(const int flag, unsigned char timeout)
{
    saved_flag |= flag;
    saved_timeout = timeout;
}
void set_msg_type_and_timeout( int flag, unsigned  char timeout)
{
    saved_flag=flag;
    saved_timeout=timeout;
}
static void set_msg_type_and_timeout2(std::vector<uint8_t>& head)
{
    if(isPerespros())
        saved_flag |= MSG_ANSW_ANSWER;
    if(head.size() < LEVBDATAOFF + 12 + 4 + 1)
    {
        ProgError(STDLOG, "head.size=%zu  while LEVBDATAOFF+12=%i", head.size(), LEVBDATAOFF+12);
        return;
    }
    memcpy(head.data() + LEVBDATAOFF + 12, &saved_flag, 4);
    head[LEVBDATAOFF + 12 + 4] = saved_timeout;
}

void set_msg_type_and_timeout_http(std::vector<uint8_t>& head)
{
    assert(HTTP_HEADER_PREFIX == head.size());

    if(isPerespros()) {
        saved_flag |= MSG_ANSW_ANSWER;
    }

    head[HTTP_HEADER_FLAGS] = saved_flag & std::numeric_limits<uint8_t>::max();
    *(reinterpret_cast<uint32_t*>(head.data() + HTTP_HEADER_TIMEOUT)) = saved_timeout;
}

bool willSuspend(int flag)
{
  // в хидере отмечено, что здесь должны проверяться статические переменные saved_flag и saved_timeout
  return (flag&MSG_ANSW_STORE_WAIT_SIG);
}

static void fcgi_main(const std::vector<uint8_t>& body, std::vector<uint8_t>& a_body)
{
    try {
        Fcgi::Request req(body);
        Fcgi::Response res(req.id(), a_body);
        applicationCallbacks()->fcgi_responder(res, req);
    } catch (const comtech::Exception& e) {
        Fcgi::Response res(Fcgi::requestId(body), a_body);
        res.StdErr() << e.what();
        res.set_status(FCGI_UNKNOWN_ROLE);
    }
}

HTTP::request HTTP::get_cur_http_request()
{
    return cur_http_request;
}

std::string makeHttpSignalText(const std::string& txt)
{
    const HTTP::request::Headers::iterator cl = std::find(cur_http_request.headers.begin(),
                                                          cur_http_request.headers.end(),
                                                          "Content-Length");
    if (cur_http_request.headers.end() != cl) {
        cl->value = std::to_string(txt.size());
    }

    return cur_http_request.to_string() + txt;
}

std::string makeHttpSignalText()
{
    return cur_http_raw_request;
}

class QueryRunnerHolder
{
public:
    QueryRunnerHolder(QueryRunner& qr) {
        setQueryRunner(qr);
    }
    ~QueryRunnerHolder() {
        clearQueryRunner();
    }
};

static void http_main(const std::vector<uint8_t>& body, std::vector<uint8_t>& a_body)
{
    HTTP::request_parser parser;
    HTTP::request req;
    boost::tribool result;
    HTTP::reply rep;
    ServerFramework::HttpLogs::HTTPLogData logdata;

    a_body.clear();
    boost::tie(result, boost::tuples::ignore) = parser.parse(req, body.begin(), body.end());
    if (result) {
        QueryRunner qr(InternetQueryRunner());
        qr.setPult(HelpCpp::convertToId(*reinterpret_cast<uint64_t*>(msgid_), httpsrv::Pult::maxLength(), std::string()));
        cur_http_raw_request.assign(body.begin(), body.end());
        cur_http_request = req;

        QueryRunnerHolder qrh(qr);
        logdata = ServerFramework::HttpLogs::log(body, {{":uri", req.uri},
                                                        {":instance", qr.getEdiHelpManager().instanceName()},
                                                        {":msgid", qr.getEdiHelpManager().msgId().asString()},
                                                        {":pult", qr.pult()}});
        applicationCallbacks()->http_handle(rep, req);

    } else {
        rep = HTTP::reply::stock_reply(HTTP::reply::bad_request);
    }

    rep.to_buffer(a_body);

    if (logdata.recid > 0) {
        logdata.code = static_cast<int32_t>(rep.status);
        ServerFramework::HttpLogs::log(a_body, logdata);
    }
}

#ifdef XP_TESTING
// Just for testing NON-perespros http requests
struct OblomHolder
{
    bool& n;
    int hdr;
    bool ok = false;
    OblomHolder(bool& needNewMsgid) : n(needNewMsgid), hdr(set_hdr(2)) {
        set_msg_type_and_timeout(0,0);
    }
    ~OblomHolder() {
        set_hdr(hdr);
        n = not ok or not willPerespros();
    }
};

void ServerFramework::http_main_for_test(std::string const &in, std::string &out)
{
  static bool needNewMsgid = true;
  static std::string query_name;
  if(needNewMsgid)
  {
      static int counter = 0;

      int* msgid = get_internal_msgid();
      msgid[0] = ++counter;
      msgid[1] = getpid();
      msgid[2] = time(0);
      ServerFramework::setPerespros(false);
  }
  else
  {
//      auto saved_signal = ServerFramework::signal_matches_config_for_perespros(q);
//      if(not saved_signal.empty())
//          throw xp_testing::SirenaLocalisedException(0, "signal_matches_config_for_perespros : <" + q + "> instead of 'sent' <" + saved_signal + "> in perespros");
      ServerFramework::setPerespros(true);
//      query_name.copy(reinterpret_cast<char*>(&h[13]), 32);
  }
  OblomHolder oh(needNewMsgid);

  std::vector<uint8_t> h_in(in.begin(), in.end()), h_out;
  http_main(h_in, h_out);
  out = std::string(h_out.begin(), h_out.end());
}
#endif /* XP_TESTING */

void levC_compose(const char *head, size_t hlen, const std::vector<uint8_t>& body, std::vector<uint8_t>& a_head, std::vector<uint8_t>& a_body)
{
  ProgTrace(TRACE7,"levC_compose(hlen=%zu, blen=%zu)",hlen,body.size());

  const int hdr = head[0];

  // Разберемся с ping
  constexpr char tst[] = "<_connection_test_";
  if(std::search(body.begin(), body.end(), tst, tst+sizeof(tst)-1) != body.end())
  {
      constexpr uint8_t answ[]="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                               "<sirena><answer><_connection_test_>ok</_connection_test_></answer></sirena>";
      constexpr uint32_t answ_len=sizeof(answ)-1;
      const uint32_t answ_hlen=htonl(answ_len);

      a_head.assign(head,head+hlen);
      a_body.assign(answ,answ+answ_len);

      memcpy(a_head.data()+1, &answ_hlen, 4);

      if(hdr==2)
      {
        short grp;
        memcpy(&grp,head+45,2);
        ProgTrace(TRACE1,"_connection_test_ from client_id %i from %.30s to %.30s",ntohs(grp),head+(4+4+4+32+8+16+16+4+4+8)+1, tclmonFullProcessName());
      }

      if(hdr==2 || hdr==3)
          a_head[ getParamsByteByGrp(hdr) ] = 0;

      return;
  }

  monitor_beg_work();
  set_msg_type_and_timeout(0,0);
  applicationCallbacks()->message_control(0, head, hlen, reinterpret_cast<const char*>(body.data()), body.size());

  set_hdr(hdr);

  memcpy(msgid_,head+LEVBDATAOFF,12);
  ProgTrace(TRACE7,"hdr=%i { %u %u %u } ",hdr,msgid_[0],msgid_[1],msgid_[2]);

  // TODO make *_proc have a signature as fcgi does
  switch(hdr)
  {
      case 1: {
          constexpr size_t head_offset = LEVBDATAOFF + LEVBDATA + sizeof(sockaddr_in);
          if(head_offset + SIRENATECHHEAD != hlen) {
              throw std::runtime_error("invalid hlen");
          }

      char shortb[4000];
      memcpy(shortb+SIRENATECHHEAD,body.data(),body.size());
      memcpy(shortb,head+head_offset,SIRENATECHHEAD);

      a_head.assign(head,head+hlen);

      const uint8_t confirm_connection[SIRENATECHHEAD] = { 0,0,0,0,0,0x05,0xff,0xff,0,0,0,0,0,0 };

      if(memcmp(shortb,"\0\0\0\020\0\0\0\0\0\0\0\0\0\0",14)!=0)
      {
          char res[8000];
          int err_code = 0;
          int res_buf_len = sizeof(res);
          if(int rc = applicationCallbacks()->text_proc(shortb, res, body.size()+SIRENATECHHEAD, &res_buf_len, &err_code))
          {
              ProgTrace(TRACE1,"main_obrzap_process %d",rc);
              monitor_idle();
              return;
          }
          if(res_buf_len < SIRENATECHHEAD)
          {
              res_buf_len = SIRENATECHHEAD;
              memcpy(res, confirm_connection, SIRENATECHHEAD);
          }
          std::copy(res, res+SIRENATECHHEAD, a_head.begin()+head_offset);
          a_body.assign(res+SIRENATECHHEAD, res+res_buf_len);
          const uint32_t blen = htonl(a_body.size());
          memcpy(a_head.data()+1, &blen, 4);
          set_msg_type_and_timeout2(a_head);
      } else {
          ProgTrace(TRACE7,"confirm_connection");
          std::copy(confirm_connection, confirm_connection+SIRENATECHHEAD, a_head.begin()+head_offset);
          a_body.clear();
          a_head[1]=a_head[2]=a_head[3]=a_head[4]=0; // body len
      }

      a_head[GRP3_PARAMS_BYTE] = MSG_TEXT;

              break;
      }
      case 2: {
              ApplicationCallbacks::Grp2Head h;
              if(h.size() != hlen)
              {
                  a_head.assign(head, head+hlen);
                  a_body = body;
                  break;
              }
              std::copy_n(head, hlen, h.begin());
              std::tie(h, a_body) = applicationCallbacks()->internet_proc(h, body);
              a_head.assign(h.begin(), h.end());
              set_msg_type_and_timeout2(a_head);
              const uint32_t blen = htonl(a_body.size());
              memcpy(a_head.data()+1, &blen, 4);

              break;
      }
      case 3: // XML_TERMINAL
          {
              char* res = 0;
              int newlen = applicationCallbacks()->jxt_proc(reinterpret_cast<const char*>(body.data()),body.size(),head,hlen,&res,0);
              if(newlen<=0)
                  ProgTrace(TRACE1,"xml_terminal_main failed");
              else
              {
                  a_head.assign(res,res+hlen);
                  a_body.assign(res+hlen,res+newlen);
                  set_msg_type_and_timeout2(a_head);
                  free(res);
              }
              const uint32_t blen = htonl(a_body.size());
              memcpy(a_head.data()+1, &blen, 4);

              break;
          }
      case 4:
        {
            fcgi_main(body, a_body);
            uint32_t len = htonl(a_body.size());
            a_head.assign(head, head + hlen);
            memcpy(a_head.data() + 1, &len, 4);
            set_msg_type_and_timeout2(a_head);
            break;
        }
      case HEADTYPE_HTTP:
      {
          memcpy(msgid_, head + 1, sizeof(msgid_));

          http_main(body, a_body);
          a_head.assign(head, head + hlen);
          set_msg_type_and_timeout_http(a_head);
          applicationCallbacks()->message_control(1, reinterpret_cast<const char*>(a_head.data()), a_head.size(),
                                                     reinterpret_cast<const char*>(a_body.data()), a_body.size());
          monitor_idle_zapr(1);
          return;
      }
      default:

        ProgError(STDLOG,"unsupported hdr %d",hdr);
        monitor_idle();
        return;
  }

  if(size_t b_ = getParamsByteByGrp(hdr))
      a_head[b_] &= ~MSG_COMPRESSED;

  a_head[COMM_PARAMS_BYTE] = 0;

  applicationCallbacks()->message_control(1, reinterpret_cast<const char*>(a_head.data()), a_head.size(),
                                             reinterpret_cast<const char*>(a_body.data()), a_body.size());
  monitor_idle_zapr(1);
}

namespace ServerFramework {

bool willPerespros()
{
    return willSuspend(::saved_flag);
}

void make_failed_head(std::vector<uint8_t>& h, const std::string& b)
{
    if(size_t params_byte = getParamsByteByGrp(h[0]))
        h[params_byte] |= MSG_SYS_ERROR;

    uint32_t newlen = htonl(b.size());
    memcpy(h.data()+1, &newlen, 4);
}

}

/*****************************************************************************/
/******************      Compression callbacks     ***************************/
/*****************************************************************************/

bool levC_should_compress(const uint8_t* h, size_t blen)
{
    if(size_t params_byte = getParamsByteByGrp(h[0]))
        return (h[params_byte] & MSG_CAN_COMPRESS) and not (h[params_byte] & MSG_BINARY);
    return false;
}

void levC_adjust_header(uint8_t* h, uint32_t newlen)
/* called if outgoing message is compressed and compressed size is less than */
/* original size */
{
  ProgTrace(TRACE5,"Outgoing message is compressed and now it's length is %u!", newlen);
  newlen = htonl(newlen);
  memcpy(h+1,&newlen,4);
  if(size_t params_byte = getParamsByteByGrp(h[0]))
      h[params_byte] |= MSG_COMPRESSED;
}

bool levC_is_perespros(const std::vector<uint8_t>& head)
{
    return head[(head[0] == HEADTYPE_HTTP ? HTTP_HEADER_FLAGS : COMM_PARAMS_BYTE)] & MSG_TYPE_PERESPROS;
}

bool levC_is_compressed(const char *h)
{
  if(isPerespros())
      return 0;
  if(size_t params_byte = getParamsByteByGrp(h[0]))
      if(h[params_byte] & MSG_COMPRESSED)
      {
          ProgTrace(TRACE5,"Incoming message is compressed!");
          return 1;
      }
  return 0;
}

/*****************************************************************************/
/******************       Crypting callbacks       ***************************/
/*****************************************************************************/

#ifdef USE_MESPRO

void getMPCryptParams(const char *head, int hlen, int *err, MPCryptParams &params)
{
  return applicationCallbacks()->getMesProParams(head,hlen,err,params);
}

#endif // USE_MESPRO

int get_sym_key(const char *head, int hlen, unsigned char *key_str, size_t key_len, int *sym_key_id)
/* returns 0 on success, error code (tclmon.h) on failure */
{
  if(head[0]==3) /* request from jxt */
  {
    char pult[7];
    sprintf(pult,"%.6s",head+45);
    return applicationCallbacks()->read_sym_key(pult,key_str,key_len,sym_key_id);
  }
  if(head[0]==2) /* internet */
  {
      ApplicationCallbacks::Grp2Head h;
      std::copy_n(head, hlen, h.begin());
    int id=applicationCallbacks()->getIClientID(h);
    if(sym_key_id)
        *sym_key_id = applicationCallbacks()->sym_key_id(h);
    return applicationCallbacks()->read_sym_key(id,key_str,key_len,sym_key_id);
  }
  ProgTrace(TRACE1,"get_sym_key for wrong msg: head[0]=%i",head[0]);
  return UNKNOWN_ERR;
}

bool levC_should_sym_crypt(const char *h, int len)
{
    if(size_t params_byte = getParamsByteByGrp(h[0]))
        return h[params_byte] & MSG_ENCRYPTED;
    return false;
}

bool levC_is_sym_crypted(const char * h)
{
    if(isPerespros())
        return false;
    if(size_t params_byte = getParamsByteByGrp(h[0]))
        return h[params_byte] & MSG_ENCRYPTED;
    return false;
}

bool levC_should_mespro_crypt(const char *h, int len)
{
    if(size_t params_byte = getParamsByteByGrp(h[0]))
        return h[params_byte+1] & MSG_MESPRO_CRYPT;
    return false;
}

bool levC_is_mespro_crypted(const char * h)
{
    if(isPerespros())
        return false;
    if(size_t params_byte = getParamsByteByGrp(h[0]))
        return h[params_byte+1] & MSG_MESPRO_CRYPT;
    return false;
}

bool levC_should_pub_crypt(const char *h, int len)
{
    if(size_t params_byte = getParamsByteByGrp(h[0]))
        return h[params_byte] & MSG_PUB_CRYPT;
    return false;
}

bool levC_is_pub_crypted(const char * h)
{
    if(isPerespros())
        return 0;
    if(size_t params_byte = getParamsByteByGrp(h[0]))
        return h[params_byte] & MSG_PUB_CRYPT;
    return false;
}

void levC_adjust_crypt_header(char *h, int newlen, int flags, int sym_key_id)
{
  ProgTrace(TRACE5,"adjusting header after crypting "
            "(flags=%i,newlen=%i,sym_key_id=%i)",flags,newlen,sym_key_id);
  newlen=htonl(newlen);
  const size_t byte = getParamsByteByGrp(h[0]);
  memcpy(h+1,&newlen,sizeof(int));
  if(h[0]==2)
  {
    if(flags==SYMMETRIC_CRYPTING)//  || flags==RSA_CRYPTING) // sym or RSA crypting
    {
      // Save sym_key_id
      int sk_id=htonl(sym_key_id);
      memcpy(h+byte+2,&sk_id,sizeof(int));
    }
  }
  switch(flags)
  {
    case SYMMETRIC_CRYPTING: // symmetric crypting
      (h[byte])|=MSG_ENCRYPTED;
      (h[byte])&=~MSG_PUB_CRYPT;
      (h[byte+1])&=~MSG_MESPRO_CRYPT;
      break;
    case RSA_CRYPTING: // RSA crypting
      (h[byte])|=MSG_PUB_CRYPT;
      (h[byte])&=~MSG_ENCRYPTED;
      (h[byte+1])&=~MSG_MESPRO_CRYPT;
      break;
    case MESPRO_CRYPTING:
      (h[byte+1])|=MSG_MESPRO_CRYPT;
      (h[byte])&=~MSG_ENCRYPTED;
      (h[byte])&=~MSG_PUB_CRYPT;
      break;
    default:
      (h[byte])&=~MSG_PUB_CRYPT;
      (h[byte])&=~MSG_ENCRYPTED;
      (h[byte+1])&=~MSG_MESPRO_CRYPT;
  }
}

int get_our_key(char *outkey_str, int *outkey_len, const std::string &key_name)
/* returns 0 on success, error code (tclmon.h) on failure */
{
  char ourkey[2001];
  int ourkey_len=0;

  if (!ourkey_len)
  {
    Tcl_Obj *obj;
    char *tmp;

    obj=Tcl_ObjGetVar2(getTclInterpretator(),
                       Tcl_NewStringObj(key_name.c_str(),-1),0,
                       TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG);
    if(!obj)
    {
      ProgError(STDLOG, "%s", Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
      return UNKNOWN_ERR;
    }
    if (!(tmp=Tcl_GetStringFromObj(obj, &ourkey_len)))
    {
      hdr=-1;
      ProgError(STDLOG, "%s", Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
      return UNKNOWN_ERR;
    }
    if (static_cast<size_t>(ourkey_len)>sizeof(ourkey))
    {
      ProgError(STDLOG, "ourkey_len=%i, sizeof(ourkey)=%zd", ourkey_len, sizeof(ourkey));
      return UNKNOWN_ERR;
    }
    snprintf(ourkey, sizeof(ourkey), "%.*s", ourkey_len, tmp);
    ourkey[ourkey_len]='\0';
  }
  *outkey_len=snprintf(outkey_str, *outkey_len, "%.*s", ourkey_len, ourkey);
  return 0;
}

int get_our_private_key(char *privkey_str, int *privkey_len)
/* returns 0 on success, error code (tclmon.h) on failure */
{
  static char sp_str[2001];
  static int sp_len=0;
  if(sp_len==0)
  {
    sp_len=sizeof(sp_str)-1;
    int ret=get_our_key(sp_str,&sp_len,"OUR_PRIVATE_KEY");
    if(ret!=0)
      return ret;
  }
  *privkey_len=snprintf(privkey_str,*privkey_len,"%.*s",sp_len,sp_str);
  return 0;
}

int get_our_public_key(char *pubkey_str, int *pubkey_len)
/* returns 0 on success, error code (tclmon.h) on failure */
{
  static char sp_str[2001];
  static int sp_len=0;
  if(sp_len==0)
  {
    sp_len=sizeof(sp_str)-1;
    int ret=get_our_key(sp_str,&sp_len,"OUR_PUBLIC_KEY");
    if(ret!=0)
      return ret;
  }
  *pubkey_len=snprintf(pubkey_str,*pubkey_len,"%.*s",sp_len,sp_str);
  return 0;
}

int get_public_key(const char *head, int hlen, char *pubkey_str, int *pubkey_len)
/* returns 0 on success, error code (tclmon.h) on failure */
{
   ApplicationCallbacks *ac=
             applicationCallbacks();

  if(head[0]==3) /* request from jxt */
  {
    char pult[7];
    sprintf(pult,"%.6s",head+45);
    return ac->read_pub_key(pult,pubkey_str,pubkey_len);
  }
  if(head[0]==2) /* internet */
  {
      ApplicationCallbacks::Grp2Head h;
      std::copy_n(head, hlen, h.begin());
    int id=ac->getIClientID(h);
    return ac->read_pub_key(id,pubkey_str,pubkey_len);
  }
  ProgTrace(TRACE1,"get_public_key for wrong msg: head[0]=%i",head[0]);
  return UNKNOWN_ERR;
}

size_t levC_form_crypt_error(char* res, size_t res_len, const char* head, size_t hlen, int error)
{
    return applicationCallbacks()->form_crypt_error(res,res_len,head,hlen,error);
}

size_t ApplicationCallbacks::form_crypt_error(char* res, size_t res_len, const char* head, size_t hlen, int error)
{
  const size_t byte = getParamsByteByGrp(head[0]);
  const char* err_text = 0;

  if(res_len <= hlen)
  {
      ProgError(STDLOG, "%zu <= %zu\n", res_len, hlen);
      return 0;
  }

  switch(error)
  {
      case 0: /* no error code is set */
          error=UNKNOWN_ERR;
          ProgTrace(TRACE1,"how could this happen????");
          //err_text = "Ошибка программы - обратитесь к разработчикам!";
          err_text = "Program error - contact the developers!";
          break;
      case UNKNOWN_KEY:
          //err_text = "Шифрованное соединение: нужен ключ";
          err_text = "Encrypted connection: need a key";
          break;
      case EXPIRED_KEY:
          //err_text = "Срок действия ключа истек - пришлите новый ключ";
          err_text = "The key has been expired - submit a new one";
          break;
      case WRONG_KEY:
          //err_text = "Ошибка проверки подписи сообщения";
          err_text = "Failed to verify message signature";
          break;
      case WRONG_SYM_KEY:
          //err_text = "Неверный симметричный ключ";
          err_text = "Invalid symmetric key";
          break;
      case UNKNOWN_SYM_KEY:
          //err_text = "Неизвестный симметричный ключ";
          err_text = "Unknown symmetric key";
          break;
      case CRYPT_ERR_READ_RSA_KEY:
          //err_text = "Открытый ключ клиента поврежден - пришлите новый ключ";
          err_text = "Client's public key is damaged - submit a new one";
          break;
      case WRONG_OUR_KEY:
      case UNKNOWN_ERR:
      case CRYPT_ALLOC_ERR:
      default:
          //err_text = "Ошибка шифрования - проверьте ключи и используемый алгоритм!";
          err_text = "Failed to encrypt - check the keys and the algorithm used!";
  }

  memcpy(res, head, hlen);
  res[byte] |= MSG_SYS_ERROR;
  res[byte] &= ~(MSG_BINARY | MSG_CAN_COMPRESS | MSG_COMPRESSED);

  size_t newlen;
  if(head[0]==2)
  {
      const uint16_t grp = static_cast<uint8_t>(head[byte-2]) * 256 + static_cast<uint8_t>(head[byte-1]);
      ProgTrace(TRACE1,"Request from clientID %i :: Crypting error occured: (%i) '%s'", grp, error, err_text);

      newlen = snprintf(res+hlen, res_len-hlen,
                        "<?xml version='1.0' encoding='UTF-8'?>\n"
                        "<sirena><answer%s><error code='-1' crypt_error='%i'>%i %s</error></answer></sirena>",
                        getInetAnsParams((const uint8_t *)head,hlen).c_str(), error, error, err_text);
      res[byte] &= ~MSG_TEXT;
  }
  else
  {
      ProgTrace(TRACE1,"Crypting error occured: (%i) '%s'", error, err_text);
      newlen = snprintf(res+hlen, res_len-hlen, "%i %s", error, err_text);
      res[byte] |= MSG_TEXT;
  }
  levC_adjust_crypt_header(res,newlen,0,0);

  ProgTrace(TRACE1,"MSG_TEXT=%i",(res[byte])&MSG_TEXT);
  ProgTrace(TRACE1,"MSG_BINARY=%i",(res[byte])&MSG_BINARY);
  ProgTrace(TRACE1,"MSG_COMPRESSED=%i",(res[byte])&MSG_COMPRESSED);
  ProgTrace(TRACE1,"MSG_ENCRYPTED=%i",(res[byte])&MSG_ENCRYPTED);
  ProgTrace(TRACE1,"MSG_CAN_COMPRESS=%i",(res[byte])&MSG_CAN_COMPRESS);
  ProgTrace(TRACE1,"MSG_SPECIAL_PROC=%i",(res[byte])&MSG_SPECIAL_PROC);
  ProgTrace(TRACE1,"MSG_PUB_CRYPT=%i",(res[byte])&MSG_PUB_CRYPT);
  ProgTrace(TRACE1,"MSG_SYS_ERROR=%i",(res[byte])&MSG_SYS_ERROR);

  ProgTrace(TRACE5,"res(utf8)='%.*s'",int(newlen),res+hlen);

  return newlen+hlen;
}

//-----------------------------------------------------------------------

Tcl_Obj* obj_tcl_arg(Tcl_Interp* interp, Tcl_Obj* grp, const char* name, bool throw_if_notf)
{
    if(Tcl_Obj* ptr = Tcl_ObjGetVar2(interp, grp, Tcl_NewStringObj(name,-1), TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG))
        return ptr;
    else if(throw_if_notf)
        throw std::runtime_error(Tcl_GetString(Tcl_GetObjResult(interp)));
    else
        return 0;
}

const char* str_tcl_arg(Tcl_Interp* interp, Tcl_Obj* grp, const char* name, const char* nvl)
{
    if(Tcl_Obj* obj = obj_tcl_arg(interp, grp, name, nvl==NULL))
        return Tcl_GetString( obj );
    else
        return nvl;
}

int int_tcl_arg(Tcl_Interp* interp, Tcl_Obj* grp, const char* name, const char* nvl)
{
    if(Tcl_Obj* obj = obj_tcl_arg(interp, grp, name, nvl==NULL))
    {
        int a = 0;
        if(TCL_OK != Tcl_GetIntFromObj(interp, obj, &a))
            throw std::runtime_error(Tcl_GetString(Tcl_GetObjResult(interp)));
        return a;
    }
    else
    {
        errno = 0;    // To distinguish success/failure after call
        char* endptr = 0;
        int a = strtol(nvl, &endptr, 10);
        if(errno or (endptr and (*endptr or endptr == nvl)))
            throw std::runtime_error("invalid int argument");
        return a;
    }
}

//-----------------------------------------------------------------------

Aparams::Queue q_tcl_params(Tcl_Interp* interp, Tcl_Obj* grp)
{
    Aparams::Queue p = {};

    p.drop = int_tcl_arg(interp,grp,"QUEUE_LEN_DROP","10");
    p.warn = int_tcl_arg(interp,grp,"QUEUE_LEN_WARN","5");
    p.warn_log = int_tcl_arg(interp,grp,"QUEUE_LEN_WARN_LOG","7");
    p.full_timestamp = int_tcl_arg(interp,grp,"QUEUE_FULL_TIMESTAMP","0");

    return p;
}

//-----------------------------------------------------------------------

Aparams a_tcl_params(Tcl_Interp* interp, Tcl_Obj* grp)
{
    Aparams p = {};

    p.obrzaps = int_tcl_arg(interp,grp,"OBRZAP_NUM");
    p.headtype = int_tcl_arg(interp,grp,"HEADTYPE");
    p.addr.first  = str_tcl_arg(interp,grp,"IP_ADDRESS","0.0.0.0");
    p.addr.second = int_tcl_arg(interp,grp, "BPORT");
    p.ipc_c = str_tcl_arg(interp,grp,"BIN");
    p.ipc_signal = str_tcl_arg(interp,grp,"SIGNAL");
    p.msg_expired_timeout = int_tcl_arg(interp,grp,"MSG_EXPIRED_TIMEOUT","15");
    p.queue = q_tcl_params(interp,grp);

    return p;
}

//-----------------------------------------------------------------------

int proc_ab(int control, const ATcpParams& p)
{
    set_signal(term3);

    int ret = ServerFramework::proc_ab_tcp_impl(control, p);

    gprofPrepare();
    return ret;
}

//-----------------------------------------------------------------------

int proc_f(int control, const ATcpParams& p)
{
    set_signal(term3);

    int ret = 0/*ServerFramework::proc_ab_fcg_impl(control, p)*/;

    gprofPrepare();
    return ret;
}

//-----------------------------------------------------------------------

int proc_http(int control, const ATcpParams& p)
{
    set_signal(term3);

    int ret = ServerFramework::proc_ab_http_impl(control, p);

    gprofPrepare();
    return ret;
}

//-----------------------------------------------------------------------

int proc_http_secure(int control, const ATcpParams& p)
{
    set_signal(term3);

    int ret = ServerFramework::proc_ab_http_secure_impl(control, p);

    gprofPrepare();
    return ret;
}

//-----------------------------------------------------------------------

static ATcpParams atcp_tcl_params(Tcl_Interp* interp, Tcl_Obj* grp)
{
    ATcpParams p;

    p.obrzaps             = int_tcl_arg(interp, grp, "OBRZAP_NUM");
    p.headtype            = int_tcl_arg(interp ,grp, "HEADTYPE");
    p.addr.first          = str_tcl_arg(interp, grp, "IP_ADDRESS", "0.0.0.0");
    p.addr.second         = int_tcl_arg(interp, grp, "APORT", "0");
    p.ipc_c               = str_tcl_arg(interp, grp, "BIN");
    p.ipc_signal          = str_tcl_arg(interp, grp, "SIGNAL");
    p.msg_expired_timeout = int_tcl_arg(interp, grp, "MSG_EXPIRED_TIMEOUT", "15");
    p.queue               = q_tcl_params(interp, grp);
    p.max_connections     = int_tcl_arg(interp, grp, "MAX_CONNECTIONS", "0");
    p.balancerAddr.first  = str_tcl_arg(interp, grp, "BALANCER_IP_ADDRESS", "0.0.0.0");
    p.balancerAddr.second = int_tcl_arg(interp, grp, "BALANCER_PORT", "0");

    return p;
}

//-----------------------------------------------------------------------

extern "C" int aproc(int supervisorSocket, int argc, char *argv[])
{
    ASSERT(1 < argc);

    Tcl_Obj* grp = Tcl_NewStringObj(argv[1], -1);
    ATcpParams p = atcp_tcl_params(getTclInterpretator(), grp);
    set_cur_grp( grp );
    return proc_ab(supervisorSocket, p);
}

//-----------------------------------------------------------------------

extern "C" int fproc(int supervisorSocket, int argc, char *argv[])
{
    ASSERT(1 < argc);

    Tcl_Obj* grp = Tcl_NewStringObj(argv[1], -1);
    ATcpParams p = atcp_tcl_params(getTclInterpretator(), grp);
    set_cur_grp( grp );
    return proc_f(supervisorSocket, p);
}

//-----------------------------------------------------------------------

extern "C" int http_proc(int supervisorSocket, int argc, char *argv[])
{
    ASSERT(1 < argc);

    Tcl_Obj* grp = Tcl_NewStringObj(argv[1], -1);
    ATcpParams p = atcp_tcl_params(getTclInterpretator(), grp);
    set_cur_grp(grp);

    return proc_http(supervisorSocket, p);
}

//-----------------------------------------------------------------------

extern "C" int http_secure_proc(int supervisorSocket, int argc, char *argv[])
{
    ASSERT(1 < argc);

    Tcl_Obj* grp = Tcl_NewStringObj(argv[1], -1);
    ATcpParams p = atcp_tcl_params(getTclInterpretator(), grp);
    set_cur_grp(grp);

    return proc_http_secure(supervisorSocket, p);
}

//-----------------------------------------------------------------------

extern "C" int proc_a_udp(int supervisorSocket, int argc, char *argv[])
{
    ASSERT(1 < argc);

    set_signal(term3);

    Tcl_Obj* grp = Tcl_NewStringObj(argv[1], -1);
    Aparams p = a_tcl_params(getTclInterpretator(), grp);
    set_cur_grp( grp );
    int ret = ServerFramework::proc_ab_udp_impl(supervisorSocket, p);

    gprofPrepare();
    return ret;
}

//-----------------------------------------------------------------------

int main_obrzap(int supervisorSocket, int argc, char *argv[]);
int main_express_dispatcher(int supervisorSocket, int argc, char *argv[]);

#ifdef XP_TESTING
int main_test_daemon(int supervisorSocket, int argc, char *argv[]);
int main_edi_help_checker_daemon(int supervisorSocket, int argc, char *argv[]);
#endif // XP_TESTING

int main_logger(int supervisorSocket, int argc, char *argv[])
{
    set_signal(term3);
    int ret = logger_init(supervisorSocket, argc, argv, HandleControlMsg);

    if (ret) {
        ProgError(STDLOG, "logger_init failed: %d", ret);
        sleep(10);
        return 1;
    }
    logger_main_loop();
    return 0;
}

namespace ServerFramework
{
    Obrzapnik::Obrzapnik()
        : finished_(false), ac_(new ApplicationCallbacks),
          query_runner(0), empty_query_runner(0)
    {
        add("leva_udp", "log_sys", proc_a_udp);
        add("leva", "log_sys", aproc);
        add("levf", "log_sys", fproc);
        add("levh", "log_sys", http_proc);
        add("levhssl", "log_sys", http_secure_proc);
        add("logger", "logger", main_logger);
        add("monitor", "monitor", main_monitor);
        add("obrzap", NULL, main_obrzap);
        add("shmsrv", "logdaemon", main_shmserv);
        add("xpr_disp", "logairimp", main_express_dispatcher);
        add("httpsrv", "logdaemon", main_httpsrv);
        add("balancer", "logbalancer", balancer::main_balancer);
        add("balancer_clients_stats", "logbalancer", balancer::main_balancer_clients_stats);
#ifdef XP_TESTING
        add("daemon", "logdaemon", main_test_daemon);
        add("edi_help_checker", "logdaemon", main_edi_help_checker_daemon);
#endif // XP_TESTING
    }

    Obrzapnik::~Obrzapnik() {  delete ac_;  }

    Obrzapnik * Obrzapnik::getInstance()
    {
        static Obrzapnik *instance=0;
        if(!instance)
        {
            instance=new Obrzapnik();
        }
        return instance;
    }
}
int main_obrzap_init( int xp_test,const char * grp)
{
  if (grp)
  {
    set_cur_grp_string(grp);
  }

  InitLogTime(NULL);

  applicationCallbacks()->connect_db();

  if(!xp_test)
      set_signal(term3);

  monitor_regular();

  applicationCallbacks()->levC_app_init();
  /** preconnect end**/
  return 0;
}

int main_obrzap_init() try
{
    return main_obrzap_init(0,0);
}
catch(const comtech::Exception &e)
{
    ProgError(STDLOG, "Exception from main_obrzap_init: '%s'", e.what());
    e.print(TRACE0);
    return 1;
}

//-----------------------------------------------------------------------

Cparams c_tcl_params(Tcl_Interp* interp, Tcl_Obj* grp)
{
    Cparams p = {};

    p.ipc_c = str_tcl_arg(interp,grp,"BIN");
    p.ipc_signal = str_tcl_arg(interp,grp,"SIGNAL");
    //p.c_id = ;

    return p;
}

//-----------------------------------------------------------------------

int proc_c(int control, const Cparams& p)
{
    if(int e = main_obrzap_init())
        return e;

    int ret = ServerFramework::proc_c_impl(control, p);

    gprofPrepare();
    return ret;
}

//-----------------------------------------------------------------------

int main_obrzap(int supervisorSocket, int argc, char *argv[])
{
    if(argc != 2)
    {
        std::cerr << ">>>>>> argc = " << argc << '\n';
        for(int i=0; i<argc; i++)
            std::cerr << ">>>>>> " << argv[i] << '\n';
        std::cerr << std::endl;
    }
    ASSERT(2 == argc);

    Tcl_Obj* grp = Tcl_NewStringObj(argv[1], -1);
    Cparams p = c_tcl_params(getTclInterpretator(), grp);

    set_cur_grp(grp);

    return proc_c(supervisorSocket, p);
}

//-----------------------------------------------------------------------

