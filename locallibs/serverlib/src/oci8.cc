#include <oci.h>
#include <string>
#include <boost/algorithm/string/case_conv.hpp>

#include "oci8.h"
#include "ocilocal.h"
#include "cursctl.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

static void check_result__(const char* file, int line, const char* func, int res, OCIError* errhp)
{
    if (res == OCI_SUCCESS)
        return;
    ProgError(STDLOG, "%s:%d %s failed: res=%d", file, line, func, res);

    char s[10000];
    int errcode = 0;
    OCIErrorGet(errhp, 1, 0, &errcode, (OraText*) s, sizeof(s), OCI_HTYPE_ERROR);
    ProgTrace(TRACE1, "err=%s", s);
}
#define check_result(res, func) check_result__(__FILE__, __LINE__, #func, res, errhp)


namespace OciCpp
{
 
void setClientInfo(OCIEnv *envhp, OCIError *errhp, OCISvcCtx *svchp,
                   const std::string &connectString,
                   const std::string &clientInfo); // ocilocal.cc

bool checkerr8(const char *nick, const char *file, int line, sword status, OCIError *errhp, const std::string& connectString)
{
    if (status == OCI_SUCCESS)
    {
        return false;
    }
    ProgTrace(TRACE1, "checkerr8() called from %s:%s:%i", nick, file, line);
    text errbuf[1024];
    sb4 errcode = 0;

    switch (status)
    {
    case OCI_SUCCESS_WITH_INFO:
        ProgError(STDLOG, "[%s] Error - OCI_SUCCESS_WITH_INFO", connectString.c_str());
        break;
    case OCI_NEED_DATA:
        ProgError(STDLOG, "[%s] Error - OCI_NEED_DATA", connectString.c_str());
        break;
    case OCI_NO_DATA:
        ProgError(STDLOG, "[%s] Error - OCI_NODATA", connectString.c_str());
        break;
    case OCI_ERROR:
        OCIErrorGet((dvoid *)errhp, (ub4) 1,  NULL, &errcode,
                    errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
        ProgError(STDLOG, "[%s] Error - %.*s", connectString.c_str(), static_cast<int>(sizeof(errbuf)), errbuf);
        break;
    case OCI_INVALID_HANDLE:
        ProgError(STDLOG, "[%s] Error - OCI_INVALID_HANDLE", connectString.c_str());
        break;
    case OCI_STILL_EXECUTING:
        ProgError(STDLOG, "[%s] Error - OCI_STILL_EXECUTE", connectString.c_str());
        break;
    case OCI_CONTINUE:
        ProgError(STDLOG, "[%s] Error - OCI_CONTINUE", connectString.c_str());
        break;
    default:
        ProgError(STDLOG, "[%s] Error - default (status=%i)", connectString.c_str(), status);
        break;
    }
    return true;
}



void Oci8Session::setClientInfo(const std::string &clientInfo)
{
  return OciCpp::setClientInfo(envhp, errhp, svchp, getConnectString(), clientInfo);
}

Oci8Session::Oci8Session(const char *nickname, const char *file, int line, const std::string& connStr): converted_session(NULL), converted_mode(0)
{
    split_connect_string(connStr, connection_param);
    const std::string& login = connection_param.login;
    const std::string& password = connection_param.password;
    const std::string& server = connection_param.server;
    Logger::getTracer().ProgTrace(getRealTraceLev(12), nickname, file, line, "Oci8Session: login=%s, password=%s, server=%s", login.c_str(), password.c_str(), server.c_str());

    status = OCIEnvCreate(&envhp, (ub4) OCI_EVENTS | OCI_OBJECT, 0, 0, 0, 0, 0, 0);
    check_result(status, OCIEnvCreate);

    status = OCIHandleAlloc((dvoid *) envhp, (dvoid **) &errhp, OCI_HTYPE_ERROR, 0, 0);
    check_result(status, OCIHandleAlloc);

    status = OCIHandleAlloc((dvoid *) envhp, (dvoid **) &srvhp, OCI_HTYPE_SERVER, 0, 0);
    check_result(status, OCIHandleAlloc);

    status = OCIHandleAlloc((dvoid *) envhp, (dvoid **) &svchp, OCI_HTYPE_SVCCTX, 0, 0);
    check_result(status, OCIHandleAlloc);

    status = OCIServerAttach(srvhp, errhp, reinterpret_cast<const OraText*>(server.data()), (ub4)server.size(), 0);
    check_result(status, OCIServerAttach);

    status = OCIAttrSet((dvoid *) svchp, OCI_HTYPE_SVCCTX, (dvoid *)srvhp, 0, OCI_ATTR_SERVER, (OCIError *) errhp);
    check_result(status, OCIAttrSet);

    status = OCIHandleAlloc((dvoid *) envhp, (dvoid **)&usrhp, (ub4) OCI_HTYPE_SESSION, 0, 0);
    check_result(status, OCIHandleAlloc);

    status = OCIAttrSet((dvoid *) usrhp, (ub4) OCI_HTYPE_SESSION,
            (dvoid *) login.c_str(), (ub4) login.size(),
            (ub4) OCI_ATTR_USERNAME, errhp);
    check_result(status, OCIAttrSet);

    status = OCIAttrSet((dvoid *) usrhp, (ub4) OCI_HTYPE_SESSION,
            (dvoid *) password.c_str(), (ub4) password.size(),
            (ub4) OCI_ATTR_PASSWORD, errhp);
    check_result(status, OCIAttrSet);

    status = OCISessionBegin(svchp, errhp, usrhp, OCI_CRED_RDBMS, OCI_STMT_CACHE);
    check_result(status, OCISessionBegin);

    status = OCIAttrSet((dvoid *) svchp, (ub4) OCI_HTYPE_SVCCTX, (dvoid *) usrhp, 0, (ub4) OCI_ATTR_SESSION, errhp);
    check_result(status, OCIAttrSet);

    if (!oci8_set_cache_size(svchp,errhp))
      check_result(-1, OCISessionBegin);

    char clientInfo[65]={};
    snprintf(clientInfo,sizeof(clientInfo),"Oci8Session %s:%i",file,line);
    setClientInfo(clientInfo);

    Logger::getTracer().ProgTrace(TRACE5, "Oci8Session has been created");
}

Oci8Session::Oci8Session(const char *nickname, const char *file, int line, OciSession& conv_sess)
    : converted_session(&conv_sess),
      converted_mode(conv_sess.mode())
{
    LogTrace(getTraceLev(TRACE5),nickname, file, line)<<"Oci8Session(OciSession)";
    if(converted_mode == 7)
    {
        conv_sess.set8();
    }
    envhp = conv_sess.native().envhp;
    svchp = conv_sess.native().svchp;
    errhp = conv_sess.native().errhp;
    srvhp = conv_sess.native().srvhp;
    usrhp = conv_sess.native().usrhp;
    split_connect_string(conv_sess.getConnectString(), connection_param);
    /*char clientInfo[65]={};
    snprintf(clientInfo,sizeof(clientInfo),"%s:%i from session",file,line);
    setClientInfo(clientInfo);*/
}

Oci8Session::Oci8Session(OciSession& conv_sess) : converted_session(&conv_sess),
                                                  converted_mode(conv_sess.mode())
{
    LogTrace(TRACE5)<<"Oci8Session(OciSession)";
    if(converted_mode == 7)
    {
        conv_sess.set8();
    }
    envhp = conv_sess.native().envhp;
    svchp = conv_sess.native().svchp;
    errhp = conv_sess.native().errhp;
    srvhp = conv_sess.native().srvhp;
    usrhp = conv_sess.native().usrhp;
    split_connect_string(conv_sess.getConnectString(), connection_param);
}

bool Oci8Session::checkerr(const char *nick, const char *file, int line, sword _status)
{
    this->status = _status;
    return checkerr(nick, file, line);
}

Oci8Session& Oci8Session::instance(const char *nick, const char *file, int line)
{
    static Oci8Session* instance = 0;
    if (!instance)
    {
        instance = new Oci8Session(nick, file, line, get_connect_string());
    }
    return *instance;
}

Oci8Session& Oci8Session::instance()
{
    static Oci8Session* instance = 0;
    if (!instance)
    {
        instance = new Oci8Session(STDLOG, get_connect_string());
    }
    return *instance;
}

int Oci8Session::err() const
{
    text errbuf[1024];
    sb4 errcode = 0;
    if (status != OCI_SUCCESS)
    {
        OCIErrorGet((dvoid *)errhp, (ub4) 1,  NULL, &errcode,
                    errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
    }
    return errcode;
}

static ub4 queue_notification_callback(dvoid* pCtx, OCISubscription* pSubscrHp, dvoid* pPayload,
                                       ub4 iPayloadLen, dvoid* pDescriptor, ub4 iMode)
{
    AqEventCallback* p_cb = reinterpret_cast<AqEventCallback*>(pCtx);
    p_cb->onEvent();
    return OCI_CONTINUE;
}

void Oci8Session::setAqCallback(const std::string& name, OciCpp::AqEventCallback* cb)
{
    ProgTrace(TRACE5, "OCISubscription: queue=%s", name.c_str());
    if (!cb)
        throw comtech::Exception("aq_event_callback ptr is NULL");

    OCISubscription* subs = NULL;
    status = OCIHandleAlloc(envhp, (dvoid**) & subs, OCI_HTYPE_SUBSCRIPTION, 0, 0);
    check_result(status, OCIHandleAlloc);

    status = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, (dvoid *) name.c_str(), name.length(),
                     OCI_ATTR_SUBSCR_NAME, errhp);
    check_result(status, OCIAttrSet);

    ub4 subs_namespace = OCI_SUBSCR_NAMESPACE_AQ;
    status = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, &subs_namespace, 0,
                     OCI_ATTR_SUBSCR_NAMESPACE, errhp);
    check_result(status, OCIAttrSet);

    status = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, (dvoid *) & queue_notification_callback, 0,
                     OCI_ATTR_SUBSCR_CALLBACK, errhp);
    check_result(status, OCIAttrSet);

    status = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, (dvoid *) cb, 0,
                     OCI_ATTR_SUBSCR_CTX, errhp);
    check_result(status, OCIAttrSet);

    ub4 subs_proto = OCI_SUBSCR_PROTO_OCI;
    status = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, &subs_proto, 0,
                     OCI_ATTR_SUBSCR_RECPTPROTO, errhp);
    check_result(status, OCIAttrSet);

    status = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, 0, 0,
                     OCI_ATTR_SUBSCR_PAYLOAD, errhp);
    check_result(status, OCIAttrSet);

    status = OCISubscriptionRegister(svchp, &subs, 1, errhp, OCI_DEFAULT);
    check_result(status, OCISubscriptionRegister);
}


bool Oci8Session::commit()
{
    OCITransCommit(svchp, errhp, OCI_DEFAULT);
    return true;
}

bool Oci8Session::rollback()
{
    OCITransRollback(svchp, errhp, OCI_DEFAULT);
    return true;
}

bool Oci8Session::checkerr(const char *nick, const char *file, int line)
{
    return checkerr8(nick,file,line,status,errhp,getConnectString());
}

OciStatementHandle::~OciStatementHandle()
{
    sword err = OCIHandleFree(stmthp, OCI_HTYPE_STMT);
    if (err == OCI_INVALID_HANDLE)
    {
        ProgError(STDLOG, "Unable to free invalid statement handle");
    }
}

OciStatementHandle Oci8Session::getStHandle()
{
    OCIStmt *stmthp = 0; // Statement handler
    if (status == OCIHandleAlloc((dvoid*)envhp, (dvoid**)&stmthp, OCI_HTYPE_STMT, 0, 0))
    {
        if (checkerr(STDLOG))
        {
            throw ociexception("Allocation of statement handle failed");
        }
    }
    return OciStatementHandle(stmthp);
}

std::string Oci8Session::getConnectString() const
{
    return make_connect_string(connection_param);
}

Oci8Session::~Oci8Session()
{
    if(converted_session)
    {
        if(converted_mode == 7)
        {
            converted_session->set7();
        }
    }
    else
    {
        // Warning: OCISessionEnd() automatically commits current session
        // So, we'll better rollback session before closing it
        OCITransRollback(svchp, errhp, OCI_DEFAULT);
        status = OCISessionEnd(svchp, errhp, usrhp, OCI_DEFAULT);

        OCIHandleFree(usrhp, OCI_HTYPE_SESSION);

        status = OCIServerDetach(srvhp, errhp, OCI_DEFAULT);

        OCIHandleFree(svchp, OCI_HTYPE_SVCCTX);
        OCIHandleFree(srvhp, OCI_HTYPE_SERVER);
        OCIHandleFree(errhp, OCI_HTYPE_ERROR);
    }
}



OCILobLocator* createLobLocator(Oci8Session& os);

namespace{


void FreeLobLocator(OCILobLocator* Lob_loc_data)
{
    if(Lob_loc_data)
        OCIDescriptorFree((dvoid*)Lob_loc_data, OCI_DTYPE_LOB);
}


OCILobLocator* createLob_local(Oci8Session& os, dvoid *data, size_t size, ub4 dtype)
{
  if(os.status)
      return 0;
  OCILobLocator* Lob_loc_data = createLobLocator(os);

  if (!Lob_loc_data)
      return 0;

  if (size > 0 && (os.status = OCILobCreateTemporary(os.svchp, os.errhp, Lob_loc_data, OCI_DEFAULT, OCI_DEFAULT,
                                       dtype, FALSE, OCI_DURATION_SESSION)) )
  {
      if(os.checkerr(STDLOG))
      {
          FreeLobLocator(Lob_loc_data);
          return 0;
      }
  }
  ub4 amt = size;
  if (size > 0 && (os.status = OCILobWrite(os.svchp, os.errhp, Lob_loc_data, &amt, 1,
                             data, size, OCI_ONE_PIECE, 0, 0, 0, SQLCS_IMPLICIT)))
  {
      if(os.checkerr(STDLOG))
      {
          FreeLobLocator(Lob_loc_data);
          return 0;
      }
  }
  return Lob_loc_data;
}
}

OCILobLocator* createLob(Oci8Session& os, const std::vector<char>& data, ub4 dtype)
{
    return createLob_local(os, (dvoid *)data.data(), data.size(), dtype);
}

OCILobLocator* createLob(Oci8Session& os, const std::vector<uint8_t>& data, ub4 dtype)
{
    return createLob_local(os, (dvoid *)data.data(), data.size(), dtype);
}

OCILobLocator* createLob(Oci8Session& os, const std::string& data, ub4 dtype)
{
    return createLob_local(os, (dvoid *)data.data(), data.size(),dtype);
}

OCILobLocator* createLobLocator(Oci8Session& os)
{
  OCILobLocator* Lob_loc_data = 0;

  if((os.status = OCIDescriptorAlloc(os.envhp,(dvoid **)&Lob_loc_data, OCI_DTYPE_LOB, 0, 0) ))
  {
      if(os.checkerr(STDLOG))
      {
          FreeLobLocator(Lob_loc_data);
          Lob_loc_data = 0;
      }
  }
  return Lob_loc_data;
}

} // namespace OciCpp
#ifndef ENABLE_PG_TESTS
#ifdef XP_TESTING
#include "test.h"
#include "ocilocal.h"
#include "xp_test_utils.h"
#include "monitor_ctl.h"

#include "checkunit.h"

using namespace std;
using namespace OciCpp;

#if 0
namespace {
void setup (void)
{
    testInitDB();
}

namespace{
bool enqueue__(const std::string& aqUser, const std::string& queueName, std::string& msgId, const std::string& msg, const std::string& corrId)
{
    LogTrace(TRACE5) << "msgId=" << msgId << " corrId=" << corrId << " msg=[" << msg << "]";
    char msgIdBuff[1024 + 1] = {0};
    int res = 0;

    std::string sql("declare "
    "begin "
    " :res := ");
    if (!aqUser.empty())
        sql += aqUser + ".";
    sql += "aq_pack.enqueue_with_correlation(:que, utl_raw.cast_to_raw(:msg), :corrId, :msgId); "
    "end;";
    OciCpp::CursCtl cr = make_curs(sql);
    cr.autoNull();
    cr.bind(":que", queueName)
        .bind(":msg", msg)
        .bind(":corrId", corrId)
        .bindOutNull(":msgId", msgIdBuff, "")
        .bindOut(":res", res);
    cr.exec();
    if (res != 0) {
        LogError(STDLOG) << "enqueue failed: res=" << res << " errText: " << msgIdBuff;
        return false;
    }
    msgId = msgIdBuff;
    LogTrace(TRACE5) << "enqueue: msgId=" << msgId << " corrId=" << corrId;

    return true;
}
bool dequeue__(const std::string& aqUser, const std::string& queueName, std::string& msgId, std::string& msg, int waitDelay, std::string& corrId)
{
    LogTrace(TRACE5) << "dequeue: queueName=" << queueName << " corrId=" << corrId << " delay=" << waitDelay;
    int res = 0;
    char msgIdBuff[1024 + 1] = {0};
    char corrIdBuff[1024 + 1] = {0};
    char msgBuff[32767 + 1] = {0};
    int msgBuffLen = 0;
    std::string sql("declare "
    "  rawMsg RAW(32767); "
    "  enqTime  DATE; "
    "begin "
    " :res := ");
    if (!aqUser.empty())
        sql += aqUser + ".";
    sql += "aq_pack.dequeue_raw(:que, :delay, :msgId, enqTime, :msgLen, rawMsg, :corrId); "
    " :msg := utl_raw.cast_to_varchar2(rawMsg); "
    "end; ";
    OciCpp::CursCtl cr = make_curs(sql);
    cr.autoNull();
    cr.bind(":que", queueName)
        .bind(":delay", waitDelay)
        .bindOutNull(":msgId", msgIdBuff, "")
        .bindOutNull(":msgLen", msgBuffLen, 0)
        .bindOutNull(":msg", msgBuff, "")
        .bindOutNull(":corrId", corrIdBuff, "")
        .bindOut(":res", res);
    cr.exec();
    if (res == -410) {
        LogError(STDLOG) << "dequeue failed: res=" << res << " errText: " << msgIdBuff;
        return false;
    } else if (res == 0) {
        ProgTrace(TRACE5, "no messages");
        return false;
    }
    msgId = msgIdBuff;
    msg = msgBuff;
    corrId = corrIdBuff;
    LogTrace(TRACE5) << "dequeue: msgId=" << msgId << " corrIdBuff=" << corrIdBuff;

    return true;
}

class TestAqCallback
    : public AqEventCallback
{
public:
    TestAqCallback() {}
    virtual ~TestAqCallback() {}
    virtual void onEvent()
    {
        ++counter;
    }
    static int counter;
};
int TestAqCallback::counter = 0;
}

/**
 * need queadm.upload_queue and grant dequeue from default user (e.g. sirena)
 *
 * execute as sysdba
BEGIN
   DBMS_AQADM.GRANT_QUEUE_PRIVILEGE(
   privilege     =>     'DEQUEUE',
   queue_name    =>     'aqadm.GDS_TICKET_STAT',
   grantee       =>     'username');
END;

 * execute as que owner
declare
  que VARCHAR2(1000);
  msg VARCHAR2(1000);
  corrId VARCHAR2(1000);
  msgId VARCHAR2(1000);
  res NUMBER;
begin
  que := 'quename';
  msg := 'lalala';
  corrId := 'corrId';
  msgId := 'msgId';
  res := aq_pack.enqueue_with_correlation(que, utl_raw.cast_to_raw(msg), corrId, msgId);
  dbms_output.put_line('res='||TO_CHAR(res));
  dbms_output.put_line('msgId='||msgId);
  dbms_output.put_line('corrId='||corrId);
end;
*
 * */
START_TEST(check_setAqCallback)
{
    std::string aqUser = "queadm";
    std::string aqDequeue = "queadm.upload_queue";
    std::string aqEnqueue = "queadm.upload_queue";

    TestAqCallback cb;
    OciCpp::createMainSession(STDLOG,get_connect_string());
    Oci8Session::instance(STDLOG).setAqCallback(aqDequeue, &cb);

    std::string msgId, corrId("corrId");
    std::string msg = "hello world";

    fail_unless(dequeue__(aqUser, aqDequeue, msgId, msg, 1, corrId) == 0, "failed dequeue");

    fail_unless(enqueue__(aqUser, aqEnqueue, msgId, msg, corrId), "failed enqueue");
    LogTrace(TRACE5) << "after enqueue: msgId=" << msgId << " msg=" << msg;
    msgId = "";
    std::string msg2;
    fail_unless(dequeue__(aqUser, aqDequeue, msgId, msg2, 1, corrId), "failed dequeue");
    LogTrace(TRACE5) << "after dequeue: msgId=" << msgId << " msg2=" << msg2;
    fail_unless(msg2 == msg, "bad msg from dequeue");
    make_curs("commit").exec();
    sleep(1);
    fail_unless(TestAqCallback::counter != 0, "failed callback, counter=%d", TestAqCallback::counter);
}
END_TEST

#define SUITENAME "AqCallback"
TCASEREGISTER(setup, 0)
    SET_TIMEOUT(20);
    ADD_TEST(check_setAqCallback)
TCASEFINISH
}
#endif // 0
#include "oci8cursor.h"

namespace {
const std::string clob_test_table = "TEST_SERVERLIB_CLOB_TABLE";
const std::string blob_test_table = "TEST_SERVERLIB_BLOB_TABLE";
const size_t lob_const_test_size = 256000;
const size_t lob_var_test_size = 64000;

const size_t lob_test_records_count = 20;
const size_t lob_test_fields_per_record = 2;
std::vector<std::vector<char>> test_data;


std::vector<char> generate_single_test_data(size_t test_data_size)
{
    LogTrace(TRACE4) << "Generating test data with Length = " << test_data_size;
    std::vector<char> ret(test_data_size);
    for ( auto& i : ret)
    {
        i = rand() % 256;
    }
    return ret;
}

void generate_test_data()
{
    const size_t test_data_count = lob_test_records_count * lob_test_fields_per_record;
    for(size_t i = 0; i < test_data_count; ++i)
    {
        test_data.push_back(generate_single_test_data(lob_const_test_size + rand() % lob_var_test_size));
    }
}

void LobFallback (void)
{
    OciCpp::Curs8Ctl cur1(STDLOG, "DROP TABLE " + clob_test_table);
    cur1.noThrowError(CERR_TABLE_NOT_EXISTS).exec();
    OciCpp::Curs8Ctl cur2(STDLOG, "DROP TABLE " + blob_test_table);
    cur2.noThrowError(CERR_TABLE_NOT_EXISTS).exec();
}

void LobSetup (void)
{
    testInitDB();
    srand(time(NULL));
    LobFallback();
    OciCpp::Curs8Ctl cur1(STDLOG, "CREATE TABLE " + clob_test_table  + "(IDA NUMBER NOT NULL, lob_field1 CLOB, lob_field2 CLOB)");
    cur1.exec();
    OciCpp::Curs8Ctl cur2(STDLOG, "CREATE TABLE " + blob_test_table  + "(IDA NUMBER NOT NULL, lob_field1 BLOB, lob_field2 BLOB)");
    cur2.exec();
    generate_test_data();
}

START_TEST(check_BLobStr)
{
    std::vector<std::string> str_test_data;

    for (size_t i = 1 ; i < test_data.size(); i += 2 )
    {
        size_t ida = i / 2;
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + blob_test_table + "(IDA, lob_field1 , lob_field2)"
                                 "VALUES (:ida, :blobdata1,  :blobdata2) ");

        cur.bind(":ida", ida);
        str_test_data.emplace_back(test_data[i - 1].begin(), test_data[i - 1].end());
        cur.bindBlob(":blobdata1", str_test_data.back());
        str_test_data.emplace_back(test_data[i].begin(), test_data[i].end());
        cur.bindBlob(":blobdata2", str_test_data.back());
        cur.exec();
    }

    {
        size_t ida = test_data.size();
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + blob_test_table + "(IDA, lob_field1)"
                                 "VALUES (:ida, :blobdata1) ");
        cur.bind(":ida", ida).bindBlob(":blobdata1", std::string()).exec();
    }

    size_t ida = 0;
    std::string data1;
    std::string data2;
    size_t i = 0;
    LogTrace(TRACE5) << "Check Selecting";

    OciCpp::Curs8Ctl cur(STDLOG, "SELECT IDA, lob_field1, lob_field2 from " + blob_test_table + " ORDER BY IDA ");

    LogTrace(TRACE5) << "Def and exec";

    cur.def(ida).defBlob(data1).defBlob(data2).exec();

    LogTrace(TRACE5) << "fetching ";

    int null_recs = 0;

    while (!cur.fen())
    {
        LogTrace(TRACE5) << "fetched " << ida ;
        if(ida == test_data.size())
        {
            ++null_recs;
            fail_unless(data1.empty());
            fail_unless(data2.empty());
        }
        else
        {
            fail_unless(ida == i / 2);
            fail_unless(data1 == str_test_data[i]);
            fail_unless(++i < str_test_data.size());
            fail_unless(data2 == str_test_data[i]);
            ++i;
        }
    }
    fail_unless( null_recs == 1);
}
END_TEST


START_TEST(check_BLobCVec)
{
    for (size_t i = 1 ; i < test_data.size(); i += 2 )
    {
        size_t ida = i / 2;
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + blob_test_table + "(IDA, lob_field1 , lob_field2)"
                                 "VALUES (:ida, :blobdata1,  :blobdata2) ");
        cur.bind(":ida", ida).bindBlob(":blobdata1", test_data[i - 1]).bindBlob(":blobdata2", test_data[i] ).exec();
    }

    {
        size_t ida = test_data.size();
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + blob_test_table + "(IDA, lob_field1)"
                                 "VALUES (:ida, :blobdata1) ");
        cur.bind(":ida", ida).bindBlob(":blobdata1", std::vector<char>()).exec();
    }

    size_t ida = 0;
    std::vector<char> data1;
    std::vector<char> data2;
    size_t  i = 0;
    LogTrace(TRACE5) << "Check Selecting";

    OciCpp::Curs8Ctl cur(STDLOG, "SELECT IDA, lob_field1, lob_field2 from " + blob_test_table + " ORDER BY IDA ");

    LogTrace(TRACE5) << "Def and exec";

    cur.def(ida).defBlob(data1).defBlob(data2).exec();

    LogTrace(TRACE5) << "fetching ";

    int null_recs = 0;

    while (!cur.fen())
    {
        LogTrace(TRACE5) << "fetched " << ida ;
        if(ida == test_data.size())
        {
            ++null_recs;
            fail_unless(data1.empty());
            fail_unless(data2.empty());
        }
        else
        {
            fail_unless(ida == i / 2);
            fail_unless(data1 == test_data[i]);
            fail_unless(++i < test_data.size());
            fail_unless(data2 == test_data[i]);
            ++i;
        }
    }
    fail_unless( null_recs == 1);
}
END_TEST

START_TEST(check_BLobUVec)
{
    std::vector<std::vector<unsigned char>> uc_test_data;

    for (size_t i = 1 ; i < test_data.size(); i += 2 )
    {
        size_t ida = i / 2;
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + blob_test_table + "(IDA, lob_field1 , lob_field2)"
                                 "VALUES (:ida, :blobdata1,  :blobdata2) ");
        cur.bind(":ida", ida);
        uc_test_data.emplace_back(test_data[i - 1].begin(), test_data[i - 1].end());
        cur.bindBlob(":blobdata1", uc_test_data.back() );
        uc_test_data.emplace_back(test_data[i].begin(), test_data[i].end());
        cur.bindBlob(":blobdata2", uc_test_data.back() );
        cur.exec();
    }

    {
        size_t ida = test_data.size();
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + blob_test_table + "(IDA, lob_field1)"
                                 "VALUES (:ida, :blobdata1) ");
        cur.bind(":ida", ida).bindBlob(":blobdata1", std::vector<char>()).exec();
    }

    size_t ida = 0;
    std::vector<unsigned char> data1;
    std::vector<unsigned char> data2;
    size_t i = 0;
    LogTrace(TRACE5) << "Check Selecting";

    OciCpp::Curs8Ctl cur(STDLOG, "SELECT IDA, lob_field1, lob_field2 from " + blob_test_table + " ORDER BY IDA ");

    LogTrace(TRACE5) << "Def and exec";

    cur.def(ida).defBlob(data1).defBlob(data2).exec();

    LogTrace(TRACE5) << "fetching ";

    int null_recs = 0;

    while (!cur.fen())
    {
        LogTrace(TRACE5) << "fetched " << ida ;
        if(ida == test_data.size())
        {
            ++null_recs;
            fail_unless(data1.empty());
            fail_unless(data2.empty());
        }
        else
        {
            fail_unless(ida == i / 2);
            fail_unless(data1 == uc_test_data[i]);
            fail_unless(++i < uc_test_data.size());
            fail_unless(data2 == uc_test_data[i]);
            ++i;
        }
    }
    fail_unless( null_recs == 1);
}
END_TEST

START_TEST(check_CLobStr)
{
    std::vector<std::string> str_test_data;

    for (size_t i = 1 ; i < test_data.size(); i += 2 )
    {
        size_t ida = i / 2;
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + clob_test_table + "(IDA, lob_field1 , lob_field2)"
                                 "VALUES (:ida, :clobdata1,  :clobdata2) ");

        cur.bind(":ida", ida);
        str_test_data.emplace_back(test_data[i - 1].begin(), test_data[i - 1].end());
        cur.bindClob(":clobdata1", str_test_data.back());
        str_test_data.emplace_back(test_data[i].begin(), test_data[i].end());
        cur.bindClob(":clobdata2", str_test_data.back());
        cur.exec();
    }

    {
        size_t ida = test_data.size();
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + clob_test_table + "(IDA, lob_field1)"
                                 "VALUES (:ida, :clobdata1) ");
        cur.bind(":ida", ida).bindClob(":clobdata1", std::string()).exec();
    }

    size_t ida = 0;
    std::string data1;
    std::string data2;
    size_t i = 0;
    LogTrace(TRACE5) << "Check Selecting";

    OciCpp::Curs8Ctl cur(STDLOG, "SELECT IDA, lob_field1, lob_field2 from " + clob_test_table + " ORDER BY IDA ");

    LogTrace(TRACE5) << "Def and exec";

    cur.def(ida).defClob(data1).defClob(data2).exec();

    LogTrace(TRACE5) << "fetching ";

    int null_recs = 0;

    while (!cur.fen())
    {
        LogTrace(TRACE5) << "fetched " << ida ;
        if(ida == test_data.size())
        {
            ++null_recs;
            fail_unless(data1.empty());
            fail_unless(data2.empty());
        }
        else
        {
            fail_unless(ida == i / 2);
            fail_unless(data1 == str_test_data[i]);
            fail_unless(++i < str_test_data.size());
            fail_unless(data2 == str_test_data[i]);
            ++i;
        }
    }
    fail_unless( null_recs == 1);
}
END_TEST

START_TEST(check_CLobCVec)
{
    for (size_t i = 1 ; i < test_data.size(); i += 2 )
    {
        size_t ida = i / 2;
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + clob_test_table + "(IDA, lob_field1 , lob_field2)"
                                 "VALUES (:ida, :clobdata1,  :clobdata2) ");
        cur.bind(":ida", ida).bindClob(":clobdata1", test_data[i - 1]).bindClob(":clobdata2", test_data[i] ).exec();
    }

    {
        size_t ida = test_data.size();
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + clob_test_table + "(IDA, lob_field1)"
                                 "VALUES (:ida, :clobdata1) ");
        cur.bind(":ida", ida).bindClob(":clobdata1", std::vector<char>()).exec();
    }

    size_t ida = 0;
    std::vector<char> data1;
    std::vector<char> data2;
    size_t i = 0;
    LogTrace(TRACE5) << "Check Selecting";

    OciCpp::Curs8Ctl cur(STDLOG, "SELECT IDA, lob_field1, lob_field2 from " + clob_test_table + " ORDER BY IDA ");

    LogTrace(TRACE5) << "Def and exec";

    cur.def(ida).defClob(data1).defClob(data2).exec();

    LogTrace(TRACE5) << "fetching ";

    int null_recs = 0;

    while (!cur.fen())
    {
        LogTrace(TRACE5) << "fetched " << ida ;
        if(ida == test_data.size())
        {
            ++null_recs;
            fail_unless(data1.empty());
            fail_unless(data2.empty());
        }
        else
        {
            fail_unless(ida == i / 2);
            fail_unless(data1 == test_data[i]);
            fail_unless(++i < test_data.size());
            fail_unless(data2 == test_data[i]);
            ++i;
        }
    }
    fail_unless( null_recs == 1);
}
END_TEST

START_TEST(check_CLobUVec)
{
    std::vector<std::vector<unsigned char>> uc_test_data;

    for (size_t i = 1 ; i < test_data.size(); i += 2 )
    {
        size_t ida = i / 2;
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + clob_test_table + "(IDA, lob_field1 , lob_field2)"
                                 "VALUES (:ida, :clobdata1,  :clobdata2) ");
        cur.bind(":ida", ida);
        uc_test_data.emplace_back(test_data[i - 1].begin(), test_data[i - 1].end());
        cur.bindClob(":clobdata1", uc_test_data.back() );
        uc_test_data.emplace_back(test_data[i].begin(), test_data[i].end());
        cur.bindClob(":clobdata2", uc_test_data.back() );
        cur.exec();
    }

    {
        size_t ida = test_data.size();
        LogTrace(TRACE5) << "Inserting rec " << ida;
        OciCpp::Curs8Ctl cur(STDLOG, "INSERT INTO " + clob_test_table + "(IDA, lob_field1)"
                                 "VALUES (:ida, :clobdata1) ");
        cur.bind(":ida", ida).bindClob(":clobdata1", std::vector<char>()).exec();
    }

    size_t ida = 0;
    std::vector<unsigned char> data1;
    std::vector<unsigned char> data2;
    size_t i = 0;
    LogTrace(TRACE5) << "Check Selecting";

    OciCpp::Curs8Ctl cur(STDLOG, "SELECT IDA, lob_field1, lob_field2 from " + clob_test_table + " ORDER BY IDA ");

    LogTrace(TRACE5) << "Def and exec";

    cur.def(ida).defClob(data1).defClob(data2).exec();

    LogTrace(TRACE5) << "fetching ";

    int null_recs = 0;

    while (!cur.fen())
    {
        LogTrace(TRACE5) << "fetched " << ida ;
        if(ida == test_data.size())
        {
            ++null_recs;
            fail_unless(data1.empty());
            fail_unless(data2.empty());
        }
        else
        {
            fail_unless(ida == i / 2);
            fail_unless(data1 == uc_test_data[i]);
            fail_unless(++i < uc_test_data.size());
            fail_unless(data2 == uc_test_data[i]);
            ++i;
        }
    }
    fail_unless( null_recs == 1);
}
END_TEST

START_TEST(check_string_oci8)
{
    std::string outbuf, inbuf = "This is just sample test string for checking correctness of std::string bindig and defining.";

    OciCpp::Curs8Ctl curs(STDLOG, "SELECT :inbuf FROM DUAL");
    curs
        .def(outbuf)
        .bind(":inbuf", inbuf)
        .EXfet();
    fail_unless( outbuf == inbuf);

    inbuf = "This is the second sample test string.";
    curs.EXfet();
    fail_unless( outbuf == inbuf);

    {
        OciCpp::Curs8Ctl curs(STDLOG,
                "SELECT 'sample string' FROM DUAL UNION ALL "
                "SELECT 'sample string' FROM DUAL UNION ALL "
                "SELECT 'sample string' FROM DUAL UNION ALL "
                "SELECT NULL FROM DUAL UNION ALL "
                "SELECT 'sample string' FROM DUAL UNION ALL "
                "SELECT 'sample string' FROM DUAL "
                );
        std::string outbuf;
        curs
            .def(outbuf)
            .exec();
        int i = 0;
        while (!curs.fen())
        {
            if (++i == 4){
                fail_unless(outbuf.size() == 0);
            } else {
                fail_unless(outbuf == "sample string");
            }
        }

    }

}
END_TEST

OciCpp::Curs8Ctl create_test_cursor(std::string& bind1, std::string& bind2, int& bind3, std::string& def1, std::string& def2, int& def3, std::string& def4, int& def5 )
{
       OciCpp::Curs8Ctl curs(STDLOG,
                "SELECT :bind1, :bind2, :bind3,'sample string', 1 FROM DUAL UNION ALL "
                "SELECT :bind1, :bind2, :bind3,'sample string', 2 FROM DUAL UNION ALL "
                "SELECT :bind1, :bind2, :bind3,'sample string', 3 FROM DUAL UNION ALL "
                "SELECT :bind1, :bind2, :bind3,           NULL, 4 FROM DUAL UNION ALL "
                "SELECT :bind1, :bind2, :bind3,'sample string', 5 FROM DUAL UNION ALL "
                "SELECT :bind1, :bind2, :bind3,'sample string', 6 FROM DUAL "
                );
       tst();

       auto curs2 = std::move(curs);
       tst();
       curs2.bind(":bind1", bind1);
       curs2.bind(":bind2", bind2);
       curs2.bind(":bind3", bind3);
       curs2.def(def1);
       curs2.def(def2);
       curs2.def(def3);
       curs2.def(def4);
       curs2.def(def5);
       auto curs3 = std::move(curs2);
       tst();
       curs3.exec();
       for (int i = 1; i < 4; ++i)
       {
           fail_unless(!curs3.fen());
           fail_unless(def1 == bind1);
           fail_unless(def2 == bind2);
           fail_unless(def3 == bind3);
           fail_unless(def4 == "sample string");
           fail_unless(def5 == i);
       }
       auto curs4 = std::move(curs3);
       tst();
       return curs4;
}

void test_cursor(OciCpp::Curs8Ctl&& curs, std::string& bind1, std::string& bind2, int& bind3, std::string& def1, std::string& def2, int& def3, std::string& def4, int& def5 )
{
    OciCpp::Curs8Ctl cur(std::forward<OciCpp::Curs8Ctl>(curs));
    bind1 = "This is the third string ";
    bind2 = "This is the third string ";
    bind3 =  0xfeedcafe;
    cur.exec();
    for (int i = 1; i <= 6; ++i)
    {
        fail_unless(!cur.fen());
        fail_unless(def1 == bind1);
        fail_unless(def2 == bind2);
        fail_unless(def3 == bind3);
        fail_unless(def4 == ((i == 4) ? "" : "sample string"));
        fail_unless(def5 == i);
    }
}

START_TEST(check_oci8_cursor_move)
{
    std::string bind1 = "This is the first string ";
    std::string bind2 = "This is the second string ";
    int bind3 = 0xdeadbeef;
    std::string def1;
    std::string def2;
    int def3 = 0;
    std::string def4;
    int def5 = 0;
    auto cur = create_test_cursor(bind1, bind2, bind3, def1, def2, def3, def4, def5);
    for (int i = 4; i <= 6; ++i)
    {
        fail_unless(!cur.fen());
        fail_unless(def1 == bind1);
        fail_unless(def2 == bind2);
        fail_unless(def3 == bind3);
        fail_unless(def4 == ((i == 4) ? "" : "sample string"));
        fail_unless(def5 == i);
    }
   tst();
   test_cursor(std::move(cur), bind1, bind2, bind3, def1, def2, def3, def4, def5);
}
END_TEST


#undef SUITENAME 
#define SUITENAME "Lobs"
TCASEREGISTER(LobSetup, LobFallback)
    SET_TIMEOUT(60);
    ADD_TEST(check_BLobStr)
    ADD_TEST(check_BLobCVec)
    ADD_TEST(check_BLobUVec)
    ADD_TEST(check_CLobStr)
    ADD_TEST(check_CLobCVec)
    ADD_TEST(check_CLobUVec)
TCASEFINISH

#undef SUITENAME 
#define SUITENAME "SqlUtil"
TCASEREGISTER(testInitDB, testShutDBConnection)
    SET_TIMEOUT(60);
    ADD_TEST(check_string_oci8)
    ADD_TEST(check_oci8_cursor_move)
TCASEFINISH

}
#endif // XP_TESTING
#endif /*ENABLE_PG_TESTS*/
