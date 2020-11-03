#ifdef HAVE_ORACLE_AQ

//#include <boost/algorithm/string/case_conv.hpp>

//#include "log/log.hpp"
//#include "log_defs.hpp"

#include <sstream>
#include <string>
#include <map>
#include <cstring>
#include <ostream>
#include "errors.hpp"
#include "text_message.hpp"
#include "aq.hpp"
#include <boost/algorithm/string.hpp>

#include "jms_log.hpp"

namespace AQ
{

oracle_param split_connect_string(const std::string& connect_string_in)
{
    std::string connect_string(connect_string_in.substr((connect_string_in.find("oracle://") == 0) ? 9 : 0 ));

    std::string::size_type pos = connect_string.find("/");
    oracle_param result;
    if (pos == std::string::npos)
    {
        LERR << "Invalid connect string: " << connect_string_in;
        throw jms::mq_error(-1, "Invalid oracle connect string");
    }
    result.login = connect_string.substr(0, pos);

    size_t qmark = connect_string.find_first_of('?', pos);
    std::string options;
    std::string tmp;
    if (qmark != std::string::npos )
    {
        options.assign(connect_string, qmark + 1, std::string::npos);
        tmp.assign(connect_string.substr(pos + 1, qmark - pos - 1 ));
    }
    else
    {
        tmp.assign(connect_string.substr(pos + 1));
    }
    pos = tmp.find("@");
    if (pos == std::string::npos)
    {
        result.password = tmp;
    }
    else
    {
        result.password = tmp.substr(0, pos);
        result.server = tmp.substr(pos + 1);
    }

    if (!options.empty())
    {
        std::map<std::string, std::string> parameters;
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, options, boost::algorithm::is_any_of("&"));
        static const char * valid_params[] = { "payload", "dummy" };

        for (const auto& t : tokens)
        {
            std::vector<std::string> pv_pair;
            boost::algorithm::split(pv_pair, t, boost::algorithm::is_any_of("="));
            if (pv_pair.size() != 2)
            {
                jms::mq_error e(-1, "invalid AQ params: inconsistent pair-value token" );
                LERR << e.what();
                throw e;
            }
            auto ins = parameters.emplace(boost::to_lower_copy(pv_pair[0]), std::move(pv_pair[1]));
            if (! ins.second )
            {
                jms::mq_error e(-1, "invalid AQ params: duplicated parameter: " + pv_pair[0] );
                LERR << e.what();
                throw e;
            }
            auto found = std::find(std::begin(valid_params), std::end(valid_params), ins.first->first);
            if (found == std::end(valid_params))
            {
                jms::mq_error e(-1, "invalid AQ params: unknown parameter: " + pv_pair[0] );
                LERR << e.what();
                throw e;
            }
        }

        auto found = parameters.find("payload");

        if (found != parameters.end() )
        {
            const auto& param = found->second;
            if (boost::to_lower_copy(param) != "jms") {
                size_t dot =  param.find_first_of('.');
                if (dot == std::string::npos)
                {
                    result.payload_ns = boost::to_upper_copy(result.login);
                    result.payload_type_name = param;
                }
                else
                {
                    result.payload_ns = param.substr(0, dot);
                    result.payload_type_name = param.substr(dot + 1);
                }
            }
        }
    }

    return result;
}



struct error_info
{
    int number;
    std::string message;
    error_info() : number(0) {}
};

error_info get_error_info(sword res, OCIError *errhp)
{
    text errbuf[512];
    sb4 errcode;
    error_info result;
    switch (res)
    {
    case OCI_NO_DATA:
        result.message = "OCI_NO_DATA";
        break;
    case OCI_ERROR:
    case OCI_SUCCESS_WITH_INFO:
        OCIErrorGet(errhp, 1, 0, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
        result.number = static_cast<int>(errcode);
        result.message = std::string("OCI_ERROR: ") + reinterpret_cast<char*>(errbuf);
        break;
    case OCI_INVALID_HANDLE:
        result.message = "OCI_INVALID_HANDLE";
        break;
    default:
        result.number = static_cast<int>(res);
        result.message = "unknown return code";
        break;
    }
    return result;

}

bool is_timeout(const error_info& error)
{
    return error.number == 25228;
}

bool is_error(const error_info& error)
{
    return error.number != 0;
}

void throw_jms_error(const error_info& error)
{
    throw jms::mq_error(error.number, error.message);
}

void check_result(sword result, OCIError* errhp, const std::string& func_name)
{
    const error_info error = get_error_info(result, errhp);
    if (result == OCI_SUCCESS_WITH_INFO)
    {
        LWARN << "In " << func_name
              << " OCI_WARN: " << error.number
              << " DESCRIPTION: " << error.message;
    }
    else if (result != OCI_SUCCESS)
    {
        LERR << "In " << func_name
             << " OCI_ERROR: " << error.number
             << " DESCRIPTION: " << error.message;
        throw_jms_error(error);
    }
}

sword init_binary_message(OCIEnv *env, OCIError *err, const std::string& user, const std::string& type_name)
{
   sword status = OCITypeVTInit(env, err);
   if (status == OCI_SUCCESS)
      status = OCITypeVTInsert(env, err,
         (unsigned char *) user.c_str(), user.size(),
         (unsigned char *) type_name.c_str(), type_name.size(),
         (unsigned char *) "$8.0", 4);
   return status;
}

sword init_text_message(OCIEnv *env, OCIError *err)
{
    sword status = OCITypeVTInit(env, err);
    if (status == OCI_SUCCESS)
    {
        status = OCITypeVTInsert
        (
                 env, err,
                 (unsigned char *) "SYS", 3,
                 (unsigned char *) "AQ$_JMS_TEXT_MESSAGE", 20,
                 (unsigned char *) "$8.0", 4
        );
    }
    if (status == OCI_SUCCESS)
    {
        status = OCITypeVTInsert
        (
                 env, err,
                 (unsigned char *) "SYS", 3,
                 (unsigned char *) "AQ$_JMS_HEADER", 14,
                 (unsigned char *) "$8.0", 4
        );
    }
    if (status == OCI_SUCCESS)
    {
        status = OCITypeVTInsert
        (
                 env, err,
                 (unsigned char *) "SYS", 3,
                 (unsigned char *) "AQ$_AGENT", 9,
                 (unsigned char *) "$8.0", 4
        );
    }
    if (status == OCI_SUCCESS)
    {
        status = OCITypeVTInsert
        (
                 env, err,
                 (unsigned char *) "SYS", 3,
                 (unsigned char *) "AQ$_JMS_USERPROPARRAY", 21,
                 (unsigned char *) "$8.0", 4
        );
    }
    return status;
}

void connection::enable_trace()
{
    text* sql_query = (text *)"ALTER SESSION SET SQL_TRACE = TRUE";

    OCIStmt* stmt = (OCIStmt*) 0;
    sword res = OCIHandleAlloc(oci_sql_.envhp, (dvoid**) &stmt, OCI_HTYPE_STMT, 0, NULL);
    check_result(res, oci_sql_.errhp, "OCIHandleAlloc");

    res = OCIStmtPrepare
    (
       stmt, oci_sql_.errhp, sql_query, strlen((char*)sql_query), (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT
    );
    check_result(res, oci_sql_.errhp, "OCIStmtPrepare");

    res = OCIStmtExecute
    (
       oci_sql_.svchp, stmt, oci_sql_.errhp, 1, 0, (OCISnapshot*)NULL, (OCISnapshot*)NULL, OCI_DEFAULT
    );
    check_result(res, oci_sql_.errhp, "OCIStmtExecute");

    res = OCIHandleFree(stmt, OCI_HTYPE_STMT);
    check_result(res, oci_sql_.errhp, "OCIHandleFree");

}

connection::connection(const std::string& connect_string, bool trace) : payload_type_(JmsPayload)
{
    sword res;
    connect_params_ = split_connect_string(connect_string);
    if (!connect_params_.payload_type_name.empty())
    {
        payload_type_ = BinaryPayload;
    }
    // create the environment

    ub4 envFlags = OCI_OBJECT | OCI_EVENTS | OCI_THREADED;

    /*
    res = OCIEnvNlsCreate(&oci_sql_.envhp, envFlags, 0, 0, 0, 0, 0, 0, 0, 0);
    check_result(res, (OCIError*) oci_sql_.envhp, "OCIEnvNlsCreate");
    // using envhp, if we have no errhp, according OCI documentation

    ub2 nls_charset_id = OCINlsCharSetNameToId(oci_sql_.envhp, (oratext*) "UTF8");

    res = OCIHandleFree(oci_sql_.envhp, OCI_HTYPE_ENV);
    check_result(res, NULL, "OCIHandleFree");

    if (nls_charset_id == 0)
    {
       throw std::logic_error("Unknown oracle charset");
    }

    res = OCIEnvNlsCreate(&oci_sql_.envhp, envFlags, 0, 0, 0, 0, 0, 0, nls_charset_id, OCI_UTF16ID);
    check_result(res, (OCIError*) oci_sql_.envhp, "OCIEnvNlsCreate");
    */
    res = OCIEnvCreate(&oci_sql_.envhp, envFlags, 0, 0, 0, 0, 0, 0);
    check_result(res, (OCIError*) oci_sql_.envhp, "OCIEnvCreate");

    // create the error handle
    res = OCIHandleAlloc(oci_sql_.envhp, reinterpret_cast<dvoid**> (&oci_sql_.errhp), OCI_HTYPE_ERROR, 0, 0);
    check_result(res, (OCIError*) oci_sql_.envhp, "OCIHandleAlloc");
    // create the server handle
    res = OCIHandleAlloc(oci_sql_.envhp, reinterpret_cast<dvoid**> (&srvhp), OCI_HTYPE_SERVER, 0, 0);
    check_result(res, oci_sql_.errhp, "OCIHandleAlloc");

    /*
    ub4 cachesize_src = 0;
    res = OCIAttrGet(oci_sql_.envhp, OCI_HTYPE_ENV, (dvoid*) & cachesize_src, NULL, OCI_ATTR_CACHE_OPT_SIZE, oci_sql_.errhp);
    check_result(res, oci_sql_.errhp, "OCIAttrGet");

    ub4 cachesize = 1000000;
    res = OCIAttrSet(oci_sql_.envhp, OCI_HTYPE_ENV, (dvoid*) & cachesize, 0, OCI_ATTR_CACHE_OPT_SIZE, oci_sql_.errhp);
    check_result(res, oci_sql_.errhp, "OCIAttrSet");
    */

    // create the server context
    std::string serviceName = connect_params_.server;
    sb4 serviceNameLen = static_cast<sb4>(serviceName.size());
    res = OCIServerAttach
    (
         srvhp, oci_sql_.errhp, reinterpret_cast<text*> (const_cast<char*> (serviceName.c_str())),
         serviceNameLen, OCI_DEFAULT
    );

    check_result(res, oci_sql_.errhp, "OCIServerAttach");

    // create service context handle
    res = OCIHandleAlloc(oci_sql_.envhp, reinterpret_cast<dvoid**> (&oci_sql_.svchp), OCI_HTYPE_SVCCTX, 0, 0);
    check_result(res, oci_sql_.errhp, "OCIHandleAlloc");
    // set the server attribute in the context handle
    res = OCIAttrSet(oci_sql_.svchp, OCI_HTYPE_SVCCTX, srvhp, 0, OCI_ATTR_SERVER, oci_sql_.errhp);
    check_result(res, oci_sql_.errhp, "OCIAttrSet");
    // allocate user session handle

    res = OCIHandleAlloc(oci_sql_.envhp, reinterpret_cast<dvoid**> (&usrhp), OCI_HTYPE_SESSION, 0, 0);
    check_result(res, oci_sql_.errhp, "OCIHandleAlloc");

    std::string userName = connect_params_.login;
    // set username attribute in the user session handle
    sb4 userNameLen = static_cast<sb4>(userName.size());
    res = OCIAttrSet
    (
             usrhp, OCI_HTYPE_SESSION,
             reinterpret_cast<dvoid*>(const_cast<char*>(userName.c_str())),
             userNameLen, OCI_ATTR_USERNAME, oci_sql_.errhp
    );
    check_result(res, oci_sql_.errhp, "OCIAttrSet");
    // set password attribute
    std::string password = connect_params_.password;
    sb4 passwordLen = static_cast<sb4>(password.size());
    res = OCIAttrSet
    (
             usrhp, OCI_HTYPE_SESSION,
             reinterpret_cast<dvoid*> (const_cast<char*> (password.c_str())),
             passwordLen, OCI_ATTR_PASSWORD, oci_sql_.errhp
    );
    check_result(res, oci_sql_.errhp, "OCIAttrSet");

    // begin the session
    res = OCISessionBegin(oci_sql_.svchp, oci_sql_.errhp, usrhp, OCI_CRED_RDBMS, OCI_DEFAULT);
    check_result(res, oci_sql_.errhp, "OCISessionBegin");

    // set the session in the context handle
    res = OCIAttrSet(oci_sql_.svchp, OCI_HTYPE_SVCCTX, usrhp, 0, OCI_ATTR_SESSION, oci_sql_.errhp);
    check_result(res, oci_sql_.errhp, "OCIAttrSet");

    if (trace)
    {
       enable_trace();
    }

    
    if (JmsPayload == payload_type_) {
        init_text_message(oci_sql_.envhp, oci_sql_.errhp);
    } else {
        init_binary_message(oci_sql_.envhp, oci_sql_.errhp, connect_params_.payload_ns, connect_params_.payload_type_name);
    }
}

connection::~connection()
{
    OCITransRollback(oci_sql_.svchp, oci_sql_.errhp, OCI_DEFAULT);
    OCISessionEnd(oci_sql_.svchp, oci_sql_.errhp, usrhp, OCI_DEFAULT);
    OCIHandleFree(usrhp, OCI_HTYPE_SESSION);
    OCIHandleFree(oci_sql_.svchp, OCI_HTYPE_SVCCTX);
    OCIServerDetach(srvhp, oci_sql_.errhp, OCI_DEFAULT);
    OCIHandleFree(srvhp, OCI_HTYPE_SERVER);
    OCIHandleFree(oci_sql_.errhp, OCI_HTYPE_ERROR);
    OCIHandleFree(oci_sql_.envhp, OCI_HTYPE_ENV);
}

void connection::break_wait()
{
    sword res = 0;
    res = OCIBreak(oci_sql_.svchp, oci_sql_.errhp);
    check_result(res, oci_sql_.errhp, "OCIBreak");

    res = OCIReset(oci_sql_.svchp, oci_sql_.errhp);
    // don't check result - success are not guaranted and doesn't needed in all cases
    // needed only for asynchronous operation breaks
    //check_result(res, oci_sql_.errhp, "OCIReset");

}

std::string connection::listen(const std::vector<std::string>& queue_names)
{
#ifdef NO_QUEUE_LISTEN
    {
       jms::mq_error e(-1, "listen not supported");
       LERR << e.what();
       throw e;
    }
#else // NO_QUEUE_LISTEN
    sword res = 0;
    //OCIAQAgent* agent_list[queue_names.size()];
    std::vector<OCIAQAgent*> agent_list(queue_names.size());

    OCIAQAgent* agent = (OCIAQAgent *) 0;
    for (size_t i = 0; i < queue_names.size(); ++i)
    {
        agent_list[i] = (OCIAQAgent *) 0;
        OCIDescriptorAlloc(oci_sql_.envhp, (dvoid **) & agent_list[i], OCI_DTYPE_AQAGENT, 0, (dvoid **) 0);

        OCIAttrSet
        (
             agent_list[i], OCI_DTYPE_AQAGENT, (dvoid *) "", 0, OCI_ATTR_AGENT_NAME, oci_sql_.errhp
        );

        OCIAttrSet
        (
             agent_list[i], OCI_DTYPE_AQAGENT, const_cast<char*> (queue_names[i].c_str()),
             queue_names[i].size(), OCI_ATTR_AGENT_ADDRESS, oci_sql_.errhp
        );
    }

    // TODO set infinite time delay in place -1
    res = OCIAQListen(oci_sql_.svchp, oci_sql_.errhp, &agent_list[0], queue_names.size(), -1, &agent, 0);
    check_result(res, oci_sql_.errhp, "OCIAQListen");

    char* queue_name;
    ub4 queue_size = 0;
    res = OCIAttrGet(agent, OCI_DTYPE_AQAGENT, (dvoid *) &queue_name, &queue_size, OCI_ATTR_AGENT_ADDRESS, oci_sql_.errhp);
    check_result(res, oci_sql_.errhp, "OCIAttrGet");

    for (size_t i = 0; i < queue_names.size(); ++i)
    {
        OCIDescriptorFree(agent_list[i], OCI_DTYPE_AQAGENT);
    }
    return std::string(queue_name);
#endif // NO_QUEUE_LISTEN
}


void connection::check()
{
    text* sql_query = (text *)"SELECT SYSDATE FROM DUAL";

    OCIStmt* stmt = NULL;
    sword res = OCIHandleAlloc(oci_sql_.envhp, (dvoid**) &stmt, OCI_HTYPE_STMT, 0, NULL);
    check_result(res, oci_sql_.errhp, "OCIHandleAlloc");

    res = OCIStmtPrepare
    (
       stmt, oci_sql_.errhp, sql_query, strlen((char*)sql_query), (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT
    );
    check_result(res, oci_sql_.errhp, "OCIStmtPrepare");

    char data[7] = {0};
    OCIDefine* defnp = NULL;

    res = OCIDefineByPos(stmt, &defnp, oci_sql_.errhp, 1, data, sizeof(data), SQLT_DAT, 0, 0, 0, OCI_DEFAULT);
    check_result(res, oci_sql_.errhp, "OCIDefineByPos");

    res = OCIStmtExecute
    (
       oci_sql_.svchp, stmt, oci_sql_.errhp, 1, 0, (OCISnapshot*)NULL, (OCISnapshot*)NULL, OCI_DEFAULT
    );
    check_result(res, oci_sql_.errhp, "OCIStmtExecute");

    /*
    if (defnp)
    {
       res = OCIHandleFree(defnp, OCI_HTYPE_DEFINE);
       check_result(res, oci_sql_.errhp, "OCIHandleFree");
    }
    */
    res = OCIHandleFree(stmt, OCI_HTYPE_STMT);
    check_result(res, oci_sql_.errhp, "OCIHandleFree");
}

void connection::commit()
{
    sword status = OCITransCommit(oci_sql_.svchp, oci_sql_.errhp, OCI_DEFAULT);
    check_result(status, oci_sql_.errhp, "OCITransCommit");
}

void connection::rollback()
{
    sword status = OCITransRollback(oci_sql_.svchp, oci_sql_.errhp, OCI_DEFAULT);
    check_result(status, oci_sql_.errhp, "OCIRollback");
}

//---------------------------------text_queue----------------------
std::shared_ptr<detail::text_queue> connection::create_text_queue(const std::string& name, bool xact)
{
    return std::shared_ptr<detail::text_queue>(new text_queue(oci_sql_, name, connect_params_.payload_ns, connect_params_.payload_type_name));
}

std::string text_queue::read_lob(OCILobLocator* lob)
{
    size_t bufoffset = 0;
    oraub8 amt = 0, char_amt = 0; // will read till the end
    oraub8 offset=1;
    oraub8 buflen = 0;
    oraub8 LobLen = 0;
    ub4 ChunkSize = 0;
    bool done=false;

    sword res = 0;

    res = OCILobGetChunkSize(session_.svchp, session_.errhp, lob, &ChunkSize);
    check_result(res, session_.errhp, "OCILobGetChunkSize");

    res = OCILobGetLength2(session_.svchp, session_.errhp, lob, &LobLen);
    check_result(res, session_.errhp, "OCILobGetLength2");

    std::vector<char> buf(LobLen + ChunkSize);
    LDEBUG << "size of buf: " << buf.size() << " for reading: " << LobLen << " bytes/chars";

    ub1 piece = OCI_FIRST_PIECE;
    while(!done)
    {
        if (buf.size() < (bufoffset + ChunkSize))
        {
            buf.resize(buf.size() + ChunkSize);
        }
        buflen = buf.size() - bufoffset;
        // amt and offset values used by OCI only at first call
        sword res=OCILobRead2(session_.svchp, session_.errhp, lob, &amt, &char_amt,
                offset, reinterpret_cast<dvoid *>(buf.data() + bufoffset), buflen,
                piece,   nullptr/*ctx*/, nullptr/*callback*/,
                (ub2) 0 /*csid*/, (ub1) SQLCS_IMPLICIT);
        switch (res)
        {
            case OCI_SUCCESS:
                LDEBUG << "Lob Read completed ";
                done = true;
            case OCI_NEED_DATA:
                piece = OCI_NEXT_PIECE;
                LDEBUG << "amt = " << amt << " char_amt = " << char_amt;
                bufoffset += amt;
                break;
            default:
                check_result(res, session_.errhp, "OCILobRead2");
                done = true;
        }
    }

    LDEBUG << "read: " << bufoffset;
    return std::string(&buf[0], bufoffset);
}

#if 1
void text_queue::write_lob(OCILobLocator* lob, const std::string& msg)
{
    LDEBUG << "data: " << msg;
    oraub8 amount = msg.size();
    oraub8 byte_amt = amount;
 //   oraub8 char_amt = 0;
    oraub8 offset = 1;
    const oraub8 increment_part = 8192;
    const char * data = msg.data();
    ub1 piece = (increment_part >= amount) ? OCI_ONE_PIECE : OCI_FIRST_PIECE ;
    oraub8 in_size = std::min(increment_part, amount);

    while (amount > 0)
    {
        LDEBUG << "offset: " << offset << " amount: " << amount << " in_size: " << in_size << " piece " << int(piece) ;
        sword res = OCILobWrite2(
              session_.svchp, session_.errhp, lob, &byte_amt, nullptr/*char_amtp*/, offset, (dvoid*) data,
              in_size, piece, nullptr, nullptr,
              (ub2) 0, (ub1) SQLCS_IMPLICIT
              );
        switch (res)
        {
            case OCI_SUCCESS:
                LDEBUG << "Lob write completed ";
            case OCI_NEED_DATA:
                data += byte_amt;
                offset += byte_amt;
                amount -= byte_amt;
                if (amount > in_size) {
                    piece = OCI_NEXT_PIECE;
                } else {
                    piece = OCI_LAST_PIECE;
                    in_size = amount;
                }
                break;
            default:
                check_result(res, session_.errhp, "OCILobWrite2");
                amount = 0;
        }
    }
}
#else
void text_queue::write_lob(OCILobLocator* lob, const std::string& msg)
{
    LDEBUG << "data: " << msg;
    ub4 amount = msg.size();
    ub4 offset = 1;
    const int increment_part = 4000;
    const char * data = msg.data();

     while (amount > 0)
     {
         ///TODO: fix for multi-byte encodings
        ub4 in_size = (increment_part > amount) ? amount : increment_part;
        LDEBUG << "offset: " << offset << " amount: " << amount
               << " in_size: " << in_size;
        ub4 out_size = in_size;
        int res = OCILobWrite
        (
              session_.svchp, session_.errhp, lob, &out_size, offset, (dvoid*) data,
              in_size, OCI_ONE_PIECE, (dvoid *) 0, (sb4(*)(dvoid *, dvoid *, ub4 *, ub1 *)) 0,
              (ub2) 0, (ub1) SQLCS_IMPLICIT
        );
        check_result(res, session_.errhp, "OCILobWrite");
        data += in_size;
        offset += out_size;
        amount -= in_size;
    }
}


#endif
#if 0
static int getOneByteLength(const std::string& str)
{
    int a = 0;
    for (std::string::const_iterator i = str.begin(); i != str.end(); ++i)
    {
        if ((unsigned char) (*i) >= 0xC0 && (unsigned char) (*i) <= 0xFD)
            ++i;
        ++a;
    }
    return a;
}
#endif

static ub4 queue_notification_callback(dvoid* pCtx, OCISubscription* pSubscrHp, dvoid* pPayload,
                                       ub4 iPayloadLen, dvoid* pDescriptor, ub4 iMode)
{
    queue_event_callback* p_cb = reinterpret_cast<queue_event_callback*> (pCtx);
    p_cb->on_event();
    return OCI_CONTINUE;
}

void text_queue::set_callback(queue_event_callback* cb)
{
    if (!cb)
    {
        jms::mq_error e(-1, "queue_event_callback ptr is NULL");
        LERR << e.what();
        throw e;
    }
    cb_ = cb;
    int res = 0;

    if (subs)
    {
        OCIHandleFree(subs, OCI_HTYPE_SUBSCRIPTION);
    }
    subs = NULL;
    //////////////////////////////////////////////////////////MEMORY LEAK
    res = OCIHandleAlloc(session_.envhp, (dvoid**) &subs, OCI_HTYPE_SUBSCRIPTION, 0, 0);
    check_result(res, session_.errhp, "OCIHandleAlloc");

    res = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, (dvoid *) name_.c_str(), name_.length(),
                     OCI_ATTR_SUBSCR_NAME, session_.errhp);
    check_result(res, session_.errhp, "OCIAttrSet");

    ub4 subs_namespace = OCI_SUBSCR_NAMESPACE_AQ;
    res = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, &subs_namespace, 0,
                     OCI_ATTR_SUBSCR_NAMESPACE, session_.errhp);
    check_result(res, session_.errhp, "OCIAttrSet");

    res = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, (dvoid *) & queue_notification_callback, 0,
                     OCI_ATTR_SUBSCR_CALLBACK, session_.errhp);
    check_result(res, session_.errhp, "OCIAttrSet");

    res = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, (dvoid *) cb_, 0,
                     OCI_ATTR_SUBSCR_CTX, session_.errhp);
    check_result(res, session_.errhp, "OCIAttrSet");

    ub4 subs_proto = OCI_SUBSCR_PROTO_OCI;
    res = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, &subs_proto, 0,
                     OCI_ATTR_SUBSCR_RECPTPROTO, session_.errhp);
    check_result(res, session_.errhp, "OCIAttrSet");

    res = OCIAttrSet((dvoid *) subs, OCI_HTYPE_SUBSCRIPTION, (dvoid *) notify_buffer_, sizeof (notify_buffer_),
                     OCI_ATTR_SUBSCR_PAYLOAD, session_.errhp);
    check_result(res, session_.errhp, "OCIAttrSet");

    res = OCISubscriptionRegister(session_.svchp, &subs, 1, session_.errhp, OCI_DEFAULT);
    check_result(res, session_.errhp, "OCISubscriptionRegister");
}

void text_queue::convert(const jms::text_message& in, BINARYAQPAYLOAD& out, BINARYAQPAYLOAD_ind& ind)
{
    sword res;
    size_t msg_size = in.text.size();
    if (!msg_size )
    {
        ind.MESSAGE = OCI_IND_NULL;
        return;
    }
    res = OCIDescriptorAlloc(session_.envhp, (dvoid **) &out.MESSAGE, OCI_DTYPE_LOB, 0, NULL);
    check_result(res, session_.errhp, "OCIDescriptorAlloc");
    res = OCILobCreateTemporary(session_.svchp, session_.errhp, out.MESSAGE, OCI_DEFAULT, OCI_DEFAULT,
            OCI_TEMP_BLOB, FALSE, OCI_DURATION_SESSION);
    check_result(res, session_.errhp, "OCILobCreateTemporary");
    ind.MESSAGE = OCI_IND_NOTNULL;
    write_lob(out.MESSAGE, in.text);
}

void text_queue::convert(const BINARYAQPAYLOAD* in, const BINARYAQPAYLOAD_ind* ind, jms::text_message& out)
{
    if (ind->MESSAGE == OCI_IND_NOTNULL)
    {
        out.text = read_lob(in->MESSAGE);
    }
    else
    {
        out.text.clear();
    }
}
void text_queue::convert(const jms::text_message& in, AQ__JMS_TEXT_MESSAGE& out, AQ__JMS_TEXT_MESSAGE_ind& ind)
{
    ind.TEXT_LEN = OCI_IND_NOTNULL;
    sword res;
    //int msg_size = in.text.size();
    int msg_size = 0;
    if (in.text.empty())
    {
        res = OCINumberFromInt(session_.errhp, &msg_size, sizeof(msg_size), OCI_NUMBER_SIGNED, &out.TEXT_LEN);
        check_result(res, session_.errhp, "OCINumberFromInt");
        return;
    }
#if 0
    //it should be size in chars
    msg_size = getOneByteLength(in.text);
    if (msg_size <= 4000)
    {
        //LDEBUG << "put to TEXT VC" << std::endl;
        res = OCIStringAssignText
        (
              session_.envhp, session_.errhp, (const oratext*) in.text.data(), (ub4) msg_size, &out.TEXT_VC
        );
        check_result(res, session_.errhp, "OCIStringAssignText");
        ind.TEXT_VC = OCI_IND_NOTNULL;
        res = OCINumberFromInt(session_.errhp, &msg_size, sizeof(msg_size), OCI_NUMBER_SIGNED, &out.TEXT_LEN);
        check_result(res, session_.errhp, "OCINumberFromInt");
    }
    else
#endif
    {
        //LDEBUG << "put to TEXT LOB" << std::endl;

        res = OCIDescriptorAlloc(session_.envhp, (dvoid **) &out.TEXT_LOB, OCI_DTYPE_LOB, 0, NULL);
        check_result(res, session_.errhp, "OCIDescriptorAlloc");
        res = OCILobCreateTemporary(session_.svchp, session_.errhp, out.TEXT_LOB, OCI_DEFAULT, OCI_DEFAULT,
                                       OCI_TEMP_CLOB, FALSE, OCI_DURATION_SESSION);
        check_result(res, session_.errhp, "OCILobCreateTemporary");
        ind.TEXT_LOB = OCI_IND_NOTNULL;
        write_lob(out.TEXT_LOB, in.text);
        oraub8 LobLen = 0;
        res = OCILobGetLength2(session_.svchp, session_.errhp, out.TEXT_LOB, &LobLen);
        LDEBUG << "TEXT_LEN=" << LobLen;
        check_result(res, session_.errhp, "OCILobGetLength2");
        res = OCINumberFromInt(session_.errhp, &LobLen, sizeof(LobLen), OCI_NUMBER_UNSIGNED, &out.TEXT_LEN);
        check_result(res, session_.errhp, "OCINumberFromInt");
    }
}

void text_queue::convert(const AQ__JMS_TEXT_MESSAGE* in, const AQ__JMS_TEXT_MESSAGE_ind* ind, jms::text_message& out)
{
    int msg_size = 0;
    if (ind->TEXT_LEN == OCI_IND_NOTNULL)
    {
        sword res;
        res = OCINumberToInt(session_.errhp, &in->TEXT_LEN, sizeof(msg_size), OCI_NUMBER_SIGNED, &msg_size);
        check_result(res, session_.errhp, "OCINumberToInt");
        //LDEBUG << "Message size: " << msg_size << std::endl;
    }
    if (ind->TEXT_VC == OCI_IND_NOTNULL)
    {
        out.text = std::string
        (
            (char *) OCIStringPtr(session_.envhp, in->TEXT_VC),
            (size_t) OCIStringSize(session_.envhp, in->TEXT_VC)
        );
        //LDEBUG << "Message in VC" << std::endl;
    }
    else if (ind->TEXT_LOB == OCI_IND_NOTNULL)
    {
        out.text = read_lob(in->TEXT_LOB);
        //LDEBUG << "Message in LOB " << std::endl;
    }
    //LDEBUG << "Message: " << out.text << std::endl;

    /*
    std::string agent_name = (char *)OCIStringPtr(session_.envhp, deqmsg->HEADER.REPLYTO.NAME);
    std::string agent_address = (char *)OCIStringPtr(session_.envhp, deqmsg->HEADER.REPLYTO.ADDRESS);
    int protocol = 0;
    res = OCINumberToInt
    (
       session_.errhp, &deqmsg->HEADER.REPLYTO.PROTOCOL, sizeof(protocol), OCI_NUMBER_SIGNED, &protocol
    );
    check_result(res, session_.errhp, "OCINumberToInt");
    LDEBUG << "Agent name: " << agent_name
              << " address: " << agent_address
              << " protocol: " << protocol
              << std::endl;
     */
}

bool text_queue::dequeue__
(
    jms::text_message& msg, unsigned wait_delay, const jms::recepient& agent_in,
    bool use_timeout, const std::string& corr_id_mask
)
{
    jms::recepient agent = (agent_in.empty() && !agents_.empty()) ? agents_.front() : agent_in;

    sword res = 0;

    OCIAQDeqOptions* deqopt = (OCIAQDeqOptions *) 0;
    OCIAQMsgProperties* msgprop = (OCIAQMsgProperties *) 0;

    res = OCIDescriptorAlloc(session_.envhp, (dvoid **) &deqopt, OCI_DTYPE_AQDEQ_OPTIONS, 0, (dvoid **) 0);
    check_result(res, session_.errhp, "OCIDescriptorAlloc");

    res = OCIDescriptorAlloc(session_.envhp, (dvoid **) &msgprop, OCI_DTYPE_AQMSG_PROPERTIES, 0, (dvoid **) 0);
    check_result(res, session_.errhp, "OCIDescriptorAlloc");

    if (use_timeout)
    {
        res = OCIAttrSet(deqopt, OCI_DTYPE_AQDEQ_OPTIONS, (dvoid *)& wait_delay, 0, OCI_ATTR_WAIT, session_.errhp);
        check_result(res, session_.errhp, "OCIAttrSet");
    }

    ub4 navigation = OCI_DEQ_NEXT_MSG;
    if (!corr_id_mask.empty())
    {
        res = OCIAttrSet
        (
            deqopt, OCI_DTYPE_AQDEQ_OPTIONS,
            const_cast<char*>(corr_id_mask.c_str()), corr_id_mask.size(),
            OCI_ATTR_CORRELATION, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrSet");
        navigation = OCI_DEQ_FIRST_MSG;
    }

    res = OCIAttrSet
    (
        deqopt, OCI_DTYPE_AQDEQ_OPTIONS,
        &navigation, 0,
        OCI_ATTR_NAVIGATION, session_.errhp
    );
    check_result(res, session_.errhp, "OCIAttrSet");

    if (!agent.empty())
    {
        res = OCIAttrSet
        (
              deqopt, OCI_DTYPE_AQDEQ_OPTIONS,
              const_cast<char *>(agent.c_str()), agent.size(),
              OCI_ATTR_CONSUMER_NAME, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrSet");
    }

    dvoid * deqmsg = nullptr;
    dvoid * deqind = nullptr;

    res = OCIAQDeq
    (
        session_.svchp, session_.errhp,
        (OraText *) name_.c_str(), deqopt, msgprop, mesg_tdo_,
        (dvoid **) &deqmsg, (dvoid **) &deqind, 0, OCI_DEFAULT
    );

    bool result = false;
    const error_info error = get_error_info(res, session_.errhp);
    if (is_error(error) && !is_timeout(error))
    {
        throw_jms_error(error);
    }

    if (!is_timeout(error))
    {
        sb4 delay = 0;
        res = OCIAttrGet
        (
              msgprop, OCI_DTYPE_AQMSG_PROPERTIES,
              (dvoid *) &delay, 0,
              OCI_ATTR_DELAY, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrGet");

        sb4 expiration = 0;
        res = OCIAttrGet
        (
              msgprop, OCI_DTYPE_AQMSG_PROPERTIES,
              (dvoid *) &expiration, 0,
              OCI_ATTR_EXPIRATION, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrGet");

        OraText* corr_id;
        ub4 corr_id_size = 0;
        res = OCIAttrGet
        (
              msgprop, OCI_DTYPE_AQMSG_PROPERTIES,
              (dvoid *) &corr_id, &corr_id_size,
              OCI_ATTR_CORRELATION, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrGet");

        OCIAQAgent* sender_id;
        res = OCIAttrGet
        (
              msgprop, OCI_DTYPE_AQMSG_PROPERTIES,
              (dvoid *) &sender_id, 0,
              OCI_ATTR_SENDER_ID, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrGet");

        OraText* reply_to;
        ub4 reply_to_size = 0;
        res = OCIAttrGet
        (
              sender_id, OCI_DTYPE_AQAGENT,
              (dvoid *) &reply_to, &reply_to_size,
              OCI_ATTR_AGENT_NAME, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrGet");


        if (JmsPayload == payload_type_) {
            convert(reinterpret_cast<AQ__JMS_TEXT_MESSAGE *>(deqmsg), reinterpret_cast<AQ__JMS_TEXT_MESSAGE_ind *>(deqind), msg);
        } else {
            convert(reinterpret_cast<BINARYAQPAYLOAD *>(deqmsg), reinterpret_cast<BINARYAQPAYLOAD_ind *>(deqind), msg);
        }

        msg.properties.correlation_id.assign((char*) corr_id, corr_id_size);
        msg.properties.reply_to.assign((char *) reply_to, reply_to_size);
        msg.properties.delay = delay;
        msg.properties.expiration = std::max(expiration, 0 );

        result = true;
    }

    res = OCIDescriptorFree(deqopt, OCI_DTYPE_AQDEQ_OPTIONS);
    check_result(res, session_.errhp, "OCIDescriptorFree");

    res = OCIDescriptorFree(msgprop, OCI_DTYPE_AQMSG_PROPERTIES);
    check_result(res, session_.errhp, "OCIDescriptorFree");

    if (deqmsg)
    {
        res = OCIObjectFree(session_.envhp, session_.errhp, deqmsg, OCI_OBJECTFREE_NONULL);
        check_result(res, session_.errhp, "OCIObjectFree");
    }

    if (deqind)
    {
        res = OCIObjectFree(session_.envhp, session_.errhp, deqind, OCI_OBJECTFREE_NONULL);
        check_result(res, session_.errhp, "OCIObjectFree");
    }

    return result;
}

jms::text_message text_queue::dequeue
(
    const jms::recepient& agent, const std::string& corr_id_mask
)
{
    jms::text_message msg;
    if (!dequeue__(msg, 0, agent, false, corr_id_mask))
    {
      jms::mq_error e(-1, "dequeue_failed");
      LERR << e.what();
      throw e;
    }
    return msg;
}

bool text_queue::dequeue
(
    jms::text_message& msg, unsigned wait_delay, const jms::recepient& agent, const std::string& corr_id_mask
)
{
    return dequeue__(msg, wait_delay, agent, true, corr_id_mask);
}

void text_queue::enqueue(const jms::text_message& msg, const jms::recepients& agents_in)
{
    const jms::recepients& agents = agents_in.empty() ? agents_ : agents_in;
    AQ__JMS_TEXT_MESSAGE enqmsg;
    AQ__JMS_TEXT_MESSAGE_ind enqind;
    BINARYAQPAYLOAD enqmsg_b;
    BINARYAQPAYLOAD_ind enqind_b;
    dvoid * enqmsg_p = nullptr;
    dvoid * enqind_p = nullptr;
 
    LDEBUG << "enqueue message";
    if (JmsPayload == payload_type_) {
        enqmsg_p = reinterpret_cast<dvoid *>(&enqmsg);
        enqind_p = reinterpret_cast<dvoid *>(&enqind);
        convert(msg, enqmsg, enqind);
    } else {
        enqmsg_p = reinterpret_cast<dvoid *>(&enqmsg_b);
        enqind_p = reinterpret_cast<dvoid *>(&enqind_b);
        convert(msg, enqmsg_b, enqind_b);
    }
    sword res = 0;


    OCIAQMsgProperties* msgprop = (OCIAQMsgProperties *) 0;
    OCIAQEnqOptions* enqopts = (OCIAQEnqOptions*) 0;


    res = OCIDescriptorAlloc(session_.envhp, (dvoid **) &msgprop, OCI_DTYPE_AQMSG_PROPERTIES, 0, (dvoid **) 0);
    check_result(res, session_.errhp, "OCIDescriptorAlloc");
    res = OCIDescriptorAlloc(session_.envhp, (dvoid **) &enqopts, OCI_DTYPE_AQENQ_OPTIONS, 0, (dvoid **) 0);
    check_result(res, session_.errhp, "OCIDescriptorAlloc");

    std::vector<OCIAQAgent*> enqagents(agents.size());

    //std::string address;
    if (!agents.empty())
    {
        LDEBUG << "setting agents: " << agents.size();
        jms::recepients::const_iterator pos = agents.begin();
        std::vector<OCIAQAgent*>::const_iterator epos = enqagents.begin();
        for (; pos != agents.end() && epos != enqagents.end(); ++pos, ++epos)
        {
            LDEBUG << "agent: " << *pos;
            res = OCIDescriptorAlloc(session_.envhp, (dvoid **) &(*epos), OCI_DTYPE_AQAGENT, 0, (dvoid **) 0);
            check_result(res, session_.errhp, "OCIDescriptorAlloc");
            res = OCIAttrSet
            (
                  *epos, OCI_DTYPE_AQAGENT,
                  const_cast<char*>(pos->c_str()), pos->size(),
                  OCI_ATTR_AGENT_NAME, session_.errhp
            );
            check_result(res, session_.errhp, "OCIAttrSet");
        }
        /*
        OCIAttrSet
        (
           enqagent, OCI_DTYPE_AQAGENT, const_cast<char*>(address.c_str()), address.size(),
           OCI_ATTR_AGENT_ADDRESS, session_.errhp
        );
         */
        res = OCIAttrSet
        (
              msgprop, OCI_DTYPE_AQMSG_PROPERTIES, (dvoid *) &enqagents[0], enqagents.size(),
              OCI_ATTR_RECIPIENT_LIST, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrSet");
    }

    OCIAQAgent* sender_id;
    if (!msg.properties.reply_to.empty())
    {
       LDEBUG << "setting reply to: " << msg.properties.reply_to;
       res = OCIDescriptorAlloc(session_.envhp, (dvoid **) &sender_id, OCI_DTYPE_AQAGENT, 0, (dvoid **) 0);
       check_result(res, session_.errhp, "OCIDescriptorAlloc");
       res = OCIAttrSet
       (
             sender_id, OCI_DTYPE_AQAGENT,
             const_cast<char*>(msg.properties.reply_to.c_str()), msg.properties.reply_to.size(),
             OCI_ATTR_AGENT_NAME, session_.errhp
       );
       check_result(res, session_.errhp, "OCIAttrSet");
    }

    if (0 != msg.properties.delay)
    {
        LDEBUG << "setting delay: " << msg.properties.delay;
        res = OCIAttrSet
        (
              msgprop, OCI_DTYPE_AQMSG_PROPERTIES,
              (dvoid *)&(msg.properties.delay), 0,
              OCI_ATTR_DELAY, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrSet");
    }

    if (msg.properties.expiration)
    {
        LDEBUG << "setting expiration: " << msg.properties.expiration;
        res = OCIAttrSet
        (
              msgprop, OCI_DTYPE_AQMSG_PROPERTIES,
              (dvoid *)&(msg.properties.expiration), 0,
              OCI_ATTR_EXPIRATION, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrSet");
    }

    if (!msg.properties.correlation_id.empty())
    {
        OraText corr_id[128];
        size_t corr_id_size = std::min(sizeof(corr_id) - 1, msg.properties.correlation_id.size());
        msg.properties.correlation_id.copy((char*) corr_id, corr_id_size);
        corr_id[corr_id_size] = '\0';
        res = OCIAttrSet
        (
              msgprop, OCI_DTYPE_AQMSG_PROPERTIES,
              (dvoid *) corr_id, corr_id_size,
              OCI_ATTR_CORRELATION, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrSet");

    }

    /*
    if (!msg.properties.reply_to.empty())
    {
        OraText reply_to[128];
        size_t corr_id_size = std::min(sizeof(corr_id) - 1, msg.properties.correlation_id.size());
        msg.properties.correlation_id.copy((char*) corr_id, corr_id_size);
        corr_id[corr_id_size] = '\0';
        res = OCIAttrSet
        (
              msgprop, OCI_DTYPE_AQMSG_PROPERTIES,
              (dvoid *) corr_id, corr_id_size,
              OCI_ATTR_CORRELATION, session_.errhp
        );
        check_result(res, session_.errhp, "OCIAttrSet");

    }
    */
    LDEBUG << "setting enqueue options";
    ub2 mode = OCI_MSG_PERSISTENT;
    res = OCIAttrSet
    (
        enqopts, OCI_DTYPE_AQENQ_OPTIONS,
        (dvoid *) &mode, sizeof (ub2),
        OCI_ATTR_MSG_DELIVERY_MODE, session_.errhp
    );
    check_result(res, session_.errhp, "OCIAttrSet");

    LDEBUG << "do enqueue";
    OCIRaw* msgid = (OCIRaw*) 0;
    res = OCIAQEnq
    (
        session_.svchp, session_.errhp, (OraText*) name_.c_str(), enqopts, msgprop, mesg_tdo_,
        (dvoid **) &enqmsg_p, (dvoid **) &enqind_p, &msgid, OCI_DEFAULT
    );
    check_result(res, session_.errhp, "OCIAQEnq");


    LDEBUG << "enqueue successfull";
    if (msgid)
    {
        res = OCIObjectFree(session_.envhp, session_.errhp, msgid, OCI_OBJECTFREE_FORCE);
        check_result(res, session_.errhp, "OCIObjectFree");
    }

    if (false /*enqind.TEXT_VC == OCI_IND_NOTNULL*/)
    {
        res = OCIObjectFree(session_.envhp, session_.errhp, enqmsg.TEXT_VC, OCI_OBJECTFREE_FORCE);
        check_result(res, session_.errhp, "OCIObjectFree");
    }
    if (!msg.text.empty())
    {
       OCILobLocator * pLob = nullptr;
       if (JmsPayload == payload_type_) {
           pLob = enqmsg.TEXT_LOB;
       } else {
           pLob = enqmsg_b.MESSAGE;
       }

       res = OCILobFreeTemporary(session_.svchp, session_.errhp, pLob);
       check_result(res, session_.errhp, "OCILobFreeTemporary");

       res = OCIDescriptorFree(pLob, OCI_DTYPE_LOB);
       check_result(res, session_.errhp, "OCIDescriptorFree");
    }
    res = OCIDescriptorFree(msgprop, OCI_DTYPE_AQMSG_PROPERTIES);
    check_result(res, session_.errhp, "OCIDescriptorFree");
    res = OCIDescriptorFree(enqopts, OCI_DTYPE_AQENQ_OPTIONS);
    check_result(res, session_.errhp, "OCIDescriptorFree");

    for (std::vector<OCIAQAgent*>::const_iterator epos = enqagents.begin(); epos != enqagents.end(); ++epos)
    {
        res = OCIDescriptorFree(*epos, OCI_DTYPE_AQAGENT);
        check_result(res, session_.errhp, "OCIDescriptorFree");
    }

    if (!msg.properties.reply_to.empty())
    {
       res = OCIDescriptorFree(sender_id, OCI_DTYPE_AQAGENT);
       check_result(res, session_.errhp, "OCIDescriptorFree");
    }
}

text_queue::text_queue(OraVars& session, const std::string& name, const std::string& payload_ns, const std::string& payload_type_name)
    : payload_type_(payload_type_name.empty()? JmsPayload : BinaryPayload), session_(session), name_(name), cb_(NULL), subs(NULL), mesg_tdo_(NULL)
{
    std::string ns = (JmsPayload == payload_type_) ? "SYS" : payload_ns;
    std::string typen = (JmsPayload == payload_type_) ? "AQ$_JMS_TEXT_MESSAGE" : payload_type_name;
    sword res = OCITypeByName
        (
         session_.envhp, session_.errhp, session_.svchp, (CONST text*) ns.c_str(), ns.size(),
         (CONST text*) typen.c_str(), typen.size(),
         (text *) 0, 0, OCI_DURATION_SESSION, OCI_TYPEGET_ALL, &mesg_tdo_
        );
    check_result(res, session_.errhp, "OCITypeByName");

    size_t pos = name_.find_first_of('/');
    if (pos != std::string::npos)
    {
        name_.resize(pos);
        auto agents_list = name.substr(pos + 1);
        boost::algorithm::split(agents_, agents_list, boost::algorithm::is_any_of(";,"), boost::token_compress_on);
    }
}


text_queue::~text_queue()
{
    if (mesg_tdo_)
    {
        OCIObjectFree(session_.envhp, session_.errhp, mesg_tdo_, OCI_OBJECTFREE_FORCE);
    }
    if (subs)
    {
        OCIHandleFree(subs, OCI_HTYPE_SUBSCRIPTION);
    }
}


}// namespace


#endif //HAVE_ORACLE_AQ

