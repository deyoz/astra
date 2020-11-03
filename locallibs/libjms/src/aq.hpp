#ifdef HAVE_ORACLE_AQ

#ifndef __AQ_H__
#define __AQ_H__

#include <string>
#include <vector>
#include <oci.h>

#include "detail.hpp"

namespace AQ
{

struct OraVars
{
   OCIEnv *envhp;
   OCIServer *srvhp;
   OCIError *errhp;
   OCISvcCtx *svchp;
};

typedef OCIRef AQ__JMS_TEXT_MESSAGE_ref;
typedef OCIRef AQ__JMS_HEADER_ref;
typedef OCIRef AQ__AGENT_ref;
typedef OCIArray AQ__JMS_USERPROPARRAY;

struct AQ__AGENT
{
   OCIString * NAME;
   OCIString * ADDRESS;
   OCINumber PROTOCOL;
   AQ__AGENT() : NAME(NULL), ADDRESS(NULL) {}
};

struct AQ__AGENT_ind
{
   OCIInd _atomic;
   OCIInd NAME;
   OCIInd ADDRESS;
   OCIInd PROTOCOL;
   AQ__AGENT_ind() : _atomic(OCI_IND_NOTNULL), NAME(OCI_IND_NULL), ADDRESS(OCI_IND_NULL), PROTOCOL(OCI_IND_NULL) {};
};

struct AQ__JMS_HEADER
{
   AQ__AGENT REPLYTO;
   OCIString * TYPE;
   OCIString * USERID;
   OCIString * APPID;
   OCIString * GROUPID;
   OCINumber GROUPSEQ;
   AQ__JMS_USERPROPARRAY * PROPERTIES;
   AQ__JMS_HEADER() : TYPE(NULL), USERID(NULL), APPID(NULL), GROUPID(NULL), PROPERTIES(NULL) {};
};

struct AQ__JMS_HEADER_ind
{
   OCIInd _atomic;
   AQ__AGENT_ind REPLYTO;
   OCIInd TYPE;
   OCIInd USERID;
   OCIInd APPID;
   OCIInd GROUPID;
   OCIInd GROUPSEQ;
   OCIInd PROPERTIES;
   AQ__JMS_HEADER_ind() :
     _atomic(OCI_IND_NOTNULL), TYPE(OCI_IND_NULL), USERID(OCI_IND_NULL), APPID(OCI_IND_NULL),
     GROUPID(OCI_IND_NULL), GROUPSEQ(OCI_IND_NULL), PROPERTIES(OCI_IND_NULL) {};
};

struct AQ__JMS_TEXT_MESSAGE
{
   AQ__JMS_HEADER HEADER;
   OCINumber TEXT_LEN;
   OCIString * TEXT_VC;
   OCIClobLocator * TEXT_LOB;
   AQ__JMS_TEXT_MESSAGE() : TEXT_VC(NULL), TEXT_LOB(NULL) {}
};

struct AQ__JMS_TEXT_MESSAGE_ind
{
   OCIInd _atomic;
   AQ__JMS_HEADER_ind HEADER;
   OCIInd TEXT_LEN;
   OCIInd TEXT_VC;
   OCIInd TEXT_LOB;
   AQ__JMS_TEXT_MESSAGE_ind() :
     _atomic(OCI_IND_NOTNULL), TEXT_LEN(OCI_IND_NULL), TEXT_VC(OCI_IND_NULL), TEXT_LOB(OCI_IND_NULL) {}
};


struct BINARYAQPAYLOAD
{
   OCILobLocator * MESSAGE;
   BINARYAQPAYLOAD() : MESSAGE(NULL) {}
};

struct BINARYAQPAYLOAD_ind
{
   OCIInd _atomic;
   OCIInd MESSAGE;
   BINARYAQPAYLOAD_ind() : _atomic(OCI_IND_NOTNULL), MESSAGE(OCI_IND_NULL) {}
};


class queue_event_callback
{
public:
    virtual void on_event() = 0;
    virtual ~queue_event_callback() {}
};

enum payload_type_t {JmsPayload=0, BinaryPayload = 1};
struct oracle_param
{
    std::string login;
    std::string password;
    std::string server;
    std::string payload_ns;
    std::string payload_type_name;
};


class connection;

class text_queue : public detail::text_queue
{
  friend class connection;


  public:
    /**
     *  blocks until message is dequeued
     * */
    jms::text_message dequeue(const jms::recepient&, const std::string& = "") override;
    /**
     *  blocks for wait_delay seconds or until message is dequeued
     * */
    bool dequeue(jms::text_message& msg, unsigned wait_delay = 0,
                 const jms::recepient& = jms::recepient(), const std::string& = "") override;
    void enqueue(const jms::text_message&, const jms::recepients&) override;
    void set_callback(queue_event_callback* cb);
    ~text_queue();
  private:
    text_queue(OraVars&, const std::string&, const std::string&, const std::string&);
    std::string read_lob(OCILobLocator*);
    void write_lob(OCILobLocator*, const std::string&);
    void convert(const AQ__JMS_TEXT_MESSAGE*, const AQ__JMS_TEXT_MESSAGE_ind*, jms::text_message&);
    void convert(const jms::text_message&, AQ__JMS_TEXT_MESSAGE&, AQ__JMS_TEXT_MESSAGE_ind&);
    void convert(const BINARYAQPAYLOAD*, const BINARYAQPAYLOAD_ind*, jms::text_message&);
    void convert(const jms::text_message&, BINARYAQPAYLOAD&, BINARYAQPAYLOAD_ind&);

 
    bool dequeue__(jms::text_message&, unsigned, const std::string&, bool, const std::string& = std::string());

  private:
    payload_type_t payload_type_;
    OraVars& session_;
    std::string name_;
    queue_event_callback* cb_;
    OCISubscription* subs;
    char notify_buffer_[100];
    OCIType* mesg_tdo_;
    jms::recepients agents_;
};

class connection : public detail::connection
{
  public:
    connection(const std::string&, bool = false);
    ~connection();
    std::string listen(const std::vector<std::string>& queue_names) override;
    void check() override;
    void commit() override;
    void rollback() override;
    void break_wait() override;
    std::shared_ptr<detail::text_queue> create_text_queue(const std::string& , bool xact=true) override;
    virtual bool has_internal_recode() const override { return (payload_type_ == JmsPayload) ? true : false; }
  private:
    payload_type_t payload_type_;
    void enable_trace();
    connection(const connection&);
    connection& operator = (const connection&);
    OCIServer   *srvhp;
    OCISession  *usrhp;
    OraVars     oci_sql_;
    oracle_param connect_params_;
};

}// namespace AQ

#endif //__AQ_H__
#endif //HAVE_ORACLE_AQ

