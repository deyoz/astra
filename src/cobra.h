//---------------------------------------------------------------------------
#ifndef _COBRA_H_
#define _COBRA_H_

#include <tcl.h>
#include <netinet/in.h>
#include <map>
#include "xml_unit.h"
#include "basic.h"

const std::string CobraMsgCodePage = "UTF-8";
const std::string WBGarantMsgCodePage = "UTF-8";
const std::string COBRA_CLIENT_TYPE = "COBRA";
const std::string WB_GARANT_CLIENT_TYPE = "WB_GARANT";
const std::string CANON_NAME_COBRA_INCOMMING = "CBIN";
const std::string CANON_NAME_COBRA_OUTCOMMING = "CBOUT";
const std::string CANON_NAME_WB_GARANT_INCOMMING = "WBIN";
const std::string CANON_NAME_WB_GARANT_OUTCOMMING = "WBOUT";

const std::string SESS_MSG_ID = "MSG_ID";
const std::string SESS_MSG_FLAGS = "MSG_FLAGS";
const std::string SESS_INVALID_MSG = "invalid_msg";
const std::string SESS_COMMIT_MSG = "commit";

int main_tcp_cobra_tcl(int supervisorSocket, int argc, char *argv[]);
int main_cobra_handler_tcl(int supervisorSocket, int argc, char *argv[]);

int main_tcp_wb_garant_tcl(int supervisorSocket, int argc, char *argv[]);
int main_wb_garant_handler_tcl(int supervisorSocket, int argc, char *argv[]);


struct sessBuffers {
  void *sendbuf;
  int sendbuf_len;
  sessBuffers() {
    sendbuf = NULL;
    sendbuf_len = 0;
  }
};

class TTCPSession
{
  private:
    int handle;
  public:
    std::string client_type;
    bool use_heartBeat;
    BASIC::TDateTime heartBeat;
    bool pr_wait_heartBeat;
    virtual ~TTCPSession() {};
    TTCPSession( int vhandle );
    bool DoAcceptConnect( sockaddr_in &addr_out );
    void DoRead( void *recvbuf, int recvbuf_len );
    int DoWriteSize( int &msg_id );
    void DoWrite( int msg_id, void *sendbuf, int sendbuf_len );
    void DoHeartBeat();
    void DoException();
    void DoExecute();
    void HeartBeatOK();
    virtual bool AcceptConnect( sockaddr_in &addr_out ) = 0;
    virtual void Read( void *recvbuf, int recvbuf_len ) = 0;
    virtual void Write( int msg_id, void *sendbuf, int sendbuf_len ) = 0;
    virtual int WriteSize( int &msg_id ) = 0;
    virtual bool canUseHeartBeat(){
      return true;
    }
    virtual void HeartBeat() = 0;
    virtual void Exception() = 0;
    virtual void ProcessIncommingMsgs() = 0;
};

template <class T>
class TTCPListener
{
  private:
    int handle;
    int listen_port;
    std::map<int,T*> hclients;
    std::map<int,sessBuffers> buffers;
    bool Terminate;
    void AddClient( int vhandle, sockaddr_in &addr_out );
    void RemoveClient( int vhandle );
    bool DoAcceptConnect( sockaddr_in &addr_out );
  public:
    virtual ~TTCPListener();
    TTCPListener( int vlisten_port ) {
      listen_port = vlisten_port;
    }
    void Execute();
    virtual bool AcceptConnect( sockaddr_in &addr_out ) {
      return true;
    }
};

enum TServMsgStatus { smDelete, smWaitAnswer, smCommit, smForSend };
enum TExecuteStatus { teClientRequest, teClientAnswer, teServerRequest };

/*
  smDelete - работа закончена с сообщением
  smWaitAnswer - ждем ответа от клиента
  smCommit - сообщение готово к обработке (есть входящее)
  smForSend - сообщение готово к отправке
*/

/*struct TServMsg {
  int msg_id;
  xmlDocPtr receiveDoc;
  xmlDocPtr sendDoc;
  bool pr_test;
  int flags;
  BASIC::TDateTime putTime;
  TServMsgStatus status;
  TServMsg() {
    msg_id = 0;
    flags = 0;
    receiveDoc = NULL;
    sendDoc = NULL;
    pr_test = false;
    status = smDelete;
    putTime = -1;
  }
  ~TServMsg() {
    if ( receiveDoc == sendDoc )
      sendDoc = NULL;
    if ( receiveDoc != NULL )
      xmlFreeDoc( receiveDoc );
    if ( sendDoc != NULL )
      xmlFreeDoc( sendDoc );
  }
  bool isClientRequest() {
    return isClientRequest( flags );
  }
  static bool isClientRequest( int msg_flags ) {
    return ((msg_flags & 0x80000000) == 0x80000000);
  }
  void setClientRequest() {
    flags = (flags | 0x80000000); // добавляем бит
  }
  void setClientAsnwer() {
    flags = (flags & 0x7FFFFFFF); //очищаем бит
  }
}; */

struct TAstraServMsg {
  int msg_id;
  int flags;
  BASIC::TDateTime processTime;
  std::string strbody;
  TAstraServMsg() {
    msg_id = 0;
    flags = 0;
    //pr_test = false;
    processTime = -1;
  }
  static bool isClientRequest( int msg_flags ) {
    return ((msg_flags & 0x80000000) == 0x80000000);
  }
  void setClientAnswer() {
    flags = (flags & 0x7FFFFFFF); //очищаем бит
  }
  void setClientRequest() {
    flags = (flags | 0x80000000); // добавляем бит
  }
};


class ServSession: public TTCPSession
{
  private:
    int FID;
    void *buf;
    unsigned int len;
    std::vector<TAstraServMsg> inmsgs;
    std::vector<TAstraServMsg> outmsgs;
    void PutMsg( int msg_id, int msg_flags, const std::string &strbody );
    void parseBuffer( void **parse_buf, unsigned int &parse_len );
    std::string createHeartBeatStr();
  public:
    std::string MsgCodePage;
    ServSession( int vhandle ):TTCPSession( vhandle ) {
      FID = 200;
      buf = NULL;
      len = 0;
      MsgCodePage = CobraMsgCodePage;
    }
    virtual ~ServSession() {
     free( buf );
    }
    virtual bool AcceptConnect( sockaddr_in &addr_out );
    virtual void Read( void *recvbuf, int recvbuf_len );
    virtual void Write( int SessId, void *sendbuf, int sendbuf_len );
    virtual int WriteSize( int &SessId );
    virtual void HeartBeat();
    virtual void Exception();
    virtual bool isInvalidAnswer( const xmlDocPtr &doc ) = 0;
    virtual void ProcessIncommingMsgs();
    virtual void getOutCommingMsgs() = 0;
    virtual int getInQueueCountMsg() {
      return -1;
    }
    virtual int getOutQueueCountMsg() {
      return -1;
    }
    virtual void ProcessMsg( const TAstraServMsg &msg ) = 0;
    void SendRequest( const std::string &strbody, bool pr_text, bool pr_zip );
    void SendAnswer( int msg_id, const std::string &strbody, bool pr_text, bool pr_zip );
    bool Empty_OutCommingQueue() {
      return outmsgs.empty();
    }
    virtual void DoHook( ) {};
    /*virtual void DoProcessClientRequest( const xmlDocPtr &reqDoc, xmlDocPtr &resDoc ) = 0;
    virtual void DoProcessClientAnswer( const xmlDocPtr &reqDoc, const xmlDocPtr &resDoc ) = 0;
    virtual void DoProcessServerRequest( xmlDocPtr &resDoc ) = 0;*/
};

std::string createInvalidMsg( const std::string &MsgCodePage );

///////////////////////////////  COBRA  ////////////////////////////////////////
class CobraSession: public ServSession
{
  private:
    bool isHook;
  public:
    std::string desk;
    std::string msg_in_type;
    std::string msg_out_type;
    std::string cmd_hook;
    CobraSession( int vhandle ):ServSession( vhandle ) {
      isHook=false;
      client_type = COBRA_CLIENT_TYPE;
      msg_in_type = CANON_NAME_COBRA_INCOMMING;
      msg_out_type = CANON_NAME_COBRA_OUTCOMMING;
      cmd_hook = "CMD_PARSE_COBRA";
    };
    virtual ~CobraSession() {};
    virtual bool AcceptConnect( sockaddr_in &addr_out );
    virtual void getOutCommingMsgs();
    virtual int getInQueueCountMsg();
    virtual int getOutQueueCountMsg();
    virtual void ProcessMsg( const TAstraServMsg &msg );
    virtual void DoHook( );
    virtual bool isInvalidAnswer( const xmlDocPtr &doc );
};
///////////////////////// END COBRA ////////////////////////////////////////////
///////////////////////////////// WB_GARANT ////////////////////////////////////

int main_wb_garant_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);
int main_wb_garant_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist);


///////////////////////////////  WB_GARANT  ////////////////////////////////////////
class WBGarantSession: public CobraSession
{
  private:
  public:
    WBGarantSession( int vhandle ):CobraSession( vhandle ) {
      client_type = WB_GARANT_CLIENT_TYPE;
      msg_in_type = CANON_NAME_WB_GARANT_INCOMMING;
      msg_out_type = CANON_NAME_WB_GARANT_OUTCOMMING;
      cmd_hook = "CMD_PARSE_WB_GARANT";
    };
    virtual ~WBGarantSession() {};
    virtual bool isInvalidAnswer( const xmlDocPtr &doc );
};
//////////////////////////////////////////////////////////////////////////////////

//std::string AnswerFlight( const xmlNodePtr reqNode, std::vector<std::string> &errors );

#endif
