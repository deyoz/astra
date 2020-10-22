#if HAVE_CONFIG_H
#endif

#include <iostream>
#include <langinfo.h>
#include <string>
#include <iostream>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "cursctl.h"
#include "pg_cursctl.h"
#include "dates_oci.h"
#include "query_runner.h"
#include "sirena_queue.h"
#include "EdiHelpManager.h"
#include "tcl_utils.h"
#include "msg_const.h"
#include "monitor.h" //for monitorControl
#include "fastcgi.h"
#include "fcgi.h"
#include "log_manager.h"
#include "lwriter.h"
#include "testmode.h"
#include "oci_selector_char.h"

#define NICKNAME "SYSTEM"
#include "slogger.h"

#ifdef CFP_DEBUG
char cfp_debug[1000]="";
#endif /* CFP_DEBUG */

using namespace std;

void constructCsaControl(bool isNosir);

static bool nosir;

bool isNosir() {
    return nosir;
}

namespace
{

static int tcl_enable_log(ClientData cl, Tcl_Interp *interp,
                          int objc,Tcl_Obj* CONST objv[])
{
    static const int ARGC_IN_ENABLE_LOG = 3;

    if(ARGC_IN_ENABLE_LOG == objc ){
        const std::string type( Tcl_GetString( objv[1] ) );
        const std::string id( Tcl_GetString( objv[2] ) );

        LogManager::Instance().load( type, id );

        return TCL_OK;
    }
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp,"Wrong arguments count, must be - enable_log <type of object> <id of object>", NULL);

    return TCL_ERROR;
}

static int tcl_disable_log(ClientData cl, Tcl_Interp *interp,
                          int objc, Tcl_Obj* CONST objv[])
{
    static const int ARG_IN_DISABLE_LOG = 3;

    if(ARG_IN_DISABLE_LOG == objc){
        const std::string type( Tcl_GetString( objv[1] ) );
        const std::string id( Tcl_GetString( objv[2] ) );

        LogManager::Instance().unload( type, id );

        return TCL_OK;
    }
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp,
            "Wrong arguments count, must be - disable_log <type of object> <id of object>", NULL);

    return TCL_ERROR;
}

static int tcl_set_log_level(ClientData cl, Tcl_Interp* interp,
                             int objc, Tcl_Obj* CONST objv[])
{
    static const int ARGC_IN_SET_LOG_LEVEL = 2;

    Tcl_ResetResult(interp);
    if (ARGC_IN_SET_LOG_LEVEL == objc) {
        int newLogLevel;

        // В случае ошибки вернет 1, описание в Tcl_GetStringResult()
        if (TCL_OK != Tcl_GetIntFromObj(interp, objv[1], &newLogLevel)) {
            return TCL_ERROR;
        }

        CutLogHolder::Instance().setLogLevel(newLogLevel);

        return TCL_OK;
    }
    Tcl_AppendResult(interp,
            "Wrong arguments count, must be - set_log_level <new log_level value>", NULL);

    return TCL_ERROR;
}

}

void set_flag_and_timeout(const int flag, const unsigned char timeout);

namespace ServerFramework
{

int ApplicationCallbacks::message_control(int type /* 0 - request, 1 - answer */,
                                const char *head, int hlen,
                                const char *body, int blen)
{
  return monitorControl::is_mes_control(type,head,hlen,body,blen);
}

void ApplicationCallbacks::init()
{
}

std::tuple<ApplicationCallbacks::Grp2Head, std::vector<uint8_t>> ApplicationCallbacks::internet_proc(const ApplicationCallbacks::Grp2Head& head, const std::vector<uint8_t>& body)
{
  const char ret[] = "Hello from internet_proc";
  return std::make_tuple(head, std::vector<uint8_t>(ret, ret + sizeof(ret) - 1));
}

int ApplicationCallbacks::jxt_proc(const char *body, int blen,
                    const char *head, int hlen, char **res, int len)
{
  char ret[100]="Hello from jxt_proc";
  int anslen=strlen(ret);
  int newlen=anslen+hlen;
  if(newlen>len && (*res=(char *)malloc(newlen*sizeof(char)))==NULL)
  {
    // malloc failed
    abort();
  }
  memcpy(*res,head,hlen);
  memcpy(*res+hlen,ret,anslen);
  return newlen;
}

int ApplicationCallbacks::text_proc(const char *mes1, char *mes2, size_t len1, int *len2, int *err_code)
{
    static const char requestWithPerespros[] = "МШЗ"; //МногоШаговый Запрос

    if (SIRENATECHHEAD < len1) {
        if (!strncmp(mes1 + SIRENATECHHEAD, requestWithPerespros, sizeof(requestWithPerespros) - 1)){
            static const char waitAnswer[] = "PROCESS REQUEST FAULT";
            static const char pult[] = "МОВКАМ";
            int* const msgId = get_internal_msgid();
            boost::posix_time::ptime local_time = boost::posix_time::second_clock::local_time();

            LogTrace(TRACE5) << "Config for perespros";

            OciCpp::CursCtl cur = make_curs("DECLARE\nPRAGMA AUTONOMOUS_TRANSACTION;\n"
                                       "BEGIN\n"
                                            "INSERT INTO EDI_HELP"
                                            "(PULT, INTMSGID, ADDRESS, TEXT, DATE1, SESSION_ID, TIMEOUT) "
                                            "VALUES"
                                            "(:pult, :id, :addr, :txt, :local_time, :sid, :timeout);"
                                            "COMMIT;\n"
                                       "END;");

            cur
                .bindFull(":id", msgId, 3 * sizeof(int), 0, 0, SQLT_BIN)
                .bind(":pult",       pult)
                .bind(":local_time", OciCpp::to_oracle_datetime(local_time))
                .bind(":timeout",    OciCpp::to_oracle_datetime(
                                                    local_time + boost::posix_time::seconds(15)
                                     )
                )
                .bind(":addr",       Tcl_GetVar2(getTclInterpretator(), "grp1_Txt", "SIGNAL", TCL_GLOBAL_ONLY))
                .bind(":txt",        "1###")
                .bind(":sid",        1)
                .exec();

            if (*len2 < static_cast<int>(SIRENATECHHEAD + sizeof(waitAnswer))) {
                return -1;
            }

            memcpy(mes2 + SIRENATECHHEAD, waitAnswer, sizeof(waitAnswer));
            *len2 = sizeof(waitAnswer) + SIRENATECHHEAD;
            memcpy(mes2, mes1, SIRENATECHHEAD);
            mes2[0] = sizeof(waitAnswer)/256;
            mes2[1] = sizeof(waitAnswer)%256;

            set_flag_and_timeout(MSG_ANSW_STORE_WAIT_SIG, 15);
        } else {
            static const char res[] = "HELLO FROM TEXT_PROC";
            const int anslen = sizeof res;

            if(*len2 < SIRENATECHHEAD + anslen)
                return -1;

            memcpy(mes2+SIRENATECHHEAD,res,anslen);
            *len2 = anslen+SIRENATECHHEAD;

            memcpy(mes2,mes1,SIRENATECHHEAD);
            mes2[0] = anslen/256;
            mes2[1] = anslen%256;
        }
        mes2[2] = 0x18;
        mes2[3] = 0x1;
        mes2[4] = uint8_t(0x80|0x20);
        mes2[5] = 0x80;
        mes2[6] = 0;
        mes2[7] = 0;
    }

    return 0;
}

//-----------------------------------------------------------------------

void ApplicationCallbacks::fcgi_responder(Fcgi::Response& res, const Fcgi::Request& req)
{
    using namespace ServerFramework::Fcgi;
    const std::map<std::string, std::string>& p = req.params();

    for (std::map<std::string, std::string>::const_iterator i = p.begin(); i != p.end(); ++i)
        LogTrace(TRACE1) << '[' << i->first << "] => (" << i->second << ')';

    std::string payload = req.stdin_text();

    LogTrace(TRACE1) << "payload(" << payload.size() << " bytes) : <" << payload << '>';
    res.StdOut() << "Content-Type: text/plain" << "\r\n\r\n";
    res.StdOut() << "Some quite default handler";
    res.StdErr() << "handle smth";
}

//-----------------------------------------------------------------------

void ApplicationCallbacks::http_handle(HTTP::reply& rep, const HTTP::request& req)
{
    LogTrace(TRACE1) << "Method: " << req.method;
    LogTrace(TRACE1) << "URI: " << req.uri;
    LogTrace(TRACE1) << "HTTP version: " << req.http_version_major << '.' << req.http_version_minor;
    LogTrace(TRACE1) << "HTTP headers:";
    for (HTTP::request::Headers::const_iterator i = req.headers.begin();
            i != req.headers.end();
            ++i) {
        LogTrace(TRACE0) << '\t' << i->name << ':' << i->value;
    }

    LogTrace(TRACE0) << "Content dump:";
    LogTrace(TRACE0) << req.content;

    rep = HTTP::reply::stock_reply(HTTP::reply::ok);
}

//-----------------------------------------------------------------------

void ApplicationCallbacks::connect_db()
{
    OciCpp::createMainSession(STDLOG,get_connect_string());
}

int ApplicationCallbacks::commit_db()
{
    commit();
    return 0;
}
int ApplicationCallbacks::rollback_db()
{
    rollback();
    return 0;
}

void registerTclCommand(char const *name,
            int (*fptr)(ClientData, Tcl_Interp *, int objc, Tcl_Obj *CONST objv[]),
            ClientData data )
{
    TclFuncRegister::getInstance().Register(name,fptr,data);
}

int ApplicationCallbacks::tcl_init(Tcl_Interp* interp)
{
    if (TCL_OK != Tcl_SetSystemEncoding(interp, "cp866")) {
        fprintf(stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)));

        return -1;
    }
    if (!Tcl_CreateObjCommand(interp, "enable_log",
                tcl_enable_log, static_cast<ClientData>(0), 0) ){
        fprintf( stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)) );

        return -1;
    }
    if (!Tcl_CreateObjCommand(interp, "disable_log",
                tcl_disable_log, static_cast<ClientData>(0), 0) ){
        fprintf( stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)) );

        return -1;
    }
    if (!Tcl_CreateObjCommand(interp, "set_log_level",
                tcl_set_log_level, static_cast<ClientData>(0), 0) ) {
        fprintf( stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)) );

        return -1;
    }
    if ( !Tcl_CreateObjCommand(interp, "set_logging",
                set_logging, static_cast<ClientData>(0), 0) ) {
        fprintf( stderr, "%s\n", Tcl_GetString(Tcl_GetObjResult(interp)) );

        return -1;
    }

    TclFuncRegister::getInstance().init(interp);
    return 0;
}

}

namespace {
int tcl_init_local(Tcl_Interp *interp)
{
    return ServerFramework::applicationCallbacks()->tcl_init(interp);
}
int tcl_start_local(Tcl_Interp *interp)
{
    return ServerFramework::applicationCallbacks()->tcl_start(interp);
}
void on_exit_local(void)
{
    return ServerFramework::applicationCallbacks()->on_exit();
}


}

namespace ServerFramework {
int ApplicationCallbacks::nosir_proc(int argc,char **argv)
{
    cerr << "this is nosir mode , but nosir_proc not implemented"
        << endl;
    return 0;
}
void ApplicationCallbacks::help_nosir()
{
    cerr << "this is help for nosir, but help_nosir not implemented"
        << endl;
}
int ApplicationCallbacks::run(int argc,char **argv)
{
    static const int MAX_ARGC = 1024 - 1;

    check_risc_order();
    if (MAX_ARGC < argc) {
        exit(1);
    }
    if (argc >= 3 && (strcmp(argv[2], "-help") == 0 ||
                      strcmp(argv[2], "--help") == 0 ||
                      strcmp(argv[2], "/?") == 0)) {
        help_nosir();
        return 0;
    }
    if (argc >= 2 && (strcmp(argv[1], "-help")==0 ||
                      strcmp(argv[1], "--help")==0 ||
                      strcmp(argv[1], "/?") == 0)) {
        help_nosir();
        return 0;
    }
    char* ARGS[MAX_ARGC];
    int arg_i = 1;
    int arg_o = 1;
    ARGS[0]=argv[0];
    if(argc>arg_i && strstr(argv[arg_i],".tcl")){
        ARGS[arg_o++] = argv[arg_i++];
    }else{
        fprintf(stderr, "Second argument must be config_file_name.tcl for supervisor\n");
        exit(1);
    }

    if ((argc > arg_i) && (0 == strcmp(argv[arg_i], "-nosir"))) {
        ARGS[arg_o++] = argv[arg_i++];
        nosir = true;
        setenv("OBRZAP_NOSIR", "YES", 1);
    }

    Logger::setTracer(new Logger::ServerlibTracer());
    for(;arg_i<argc;++arg_i,++arg_o){
        ARGS[arg_o] = argv[arg_i];
    }
    if(nosir && arg_o<4){
        help_nosir();
        return 1;
    }
    ARGS[arg_o]=0;

    return queue_main(arg_o, ARGS, tcl_init_local, tcl_start_local, on_exit_local);
}

QueryRunner::QueryRunner(EdiHelpManagerPtr e)
    : em(e)
{
    em->setQueryRunner(this);
    ServerFramework::setQueryRunner(*this);
}

void QueryRunner::setPult(std::string const &p) const
{
    pult_ = p;
}

std::string QueryRunner::pult() const
{
    return pult_;
}

void QueryRunner::setEdiHelpManager(EdiHelpManagerPtr e)
{
    em = e;
}

EdiHelpManager& QueryRunner::getEdiHelpManager() const
{
    return *em;
}

QueryRunner::~QueryRunner()
{
    clearQueryRunner();
}

Tgroup QueryRunner::environment() const
{
    switch(get_hdr())
    {
        case 1:  return tg_text;
        case 2:  return tg_inet;
        case 3:  return tg_jxt;
        case 4:  return tg_http;
        default: return tg_unspecified;
                 // throw comtech::Exception("unknown obrzap group");
    }
}

QueryRunner EmptyQueryRunner()
{
    return QueryRunner(QueryRunner::EdiHelpManagerPtr(new EdiHelpManager(0)));
}
QueryRunner TextQueryRunner()
{
    return QueryRunner(QueryRunner::EdiHelpManagerPtr(new EdiHelpManager(MSG_ANSW_STORE_WAIT_SIG)));
}
QueryRunner InternetQueryRunner()
{
    return QueryRunner(QueryRunner::EdiHelpManagerPtr(new EdiHelpManager(MSG_ANSW_STORE_WAIT_SIG)));
}
void setQueryRunner(QueryRunner const &q)
{
    Obrzapnik::getInstance()->query_runner=&q;
}
void clearQueryRunner(void)
{
    Obrzapnik::getInstance()->query_runner=0;
}
QueryRunner const & getQueryRunner(void)
{
    if(Obrzapnik::getInstance()->query_runner)
        return *Obrzapnik::getInstance()->query_runner;
    if(!Obrzapnik::getInstance()->empty_query_runner){
        Obrzapnik::getInstance()->empty_query_runner=new QueryRunner(EmptyQueryRunner());
    }
    return *Obrzapnik::getInstance()->empty_query_runner;
}

} // namespace ServerFramework

extern "C" NAME2F const *getProcTable(int *len)
{
    return ServerFramework::Obrzapnik::getInstance()->getProcTable(len);
}
