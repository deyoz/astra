#include "TypeBHelpMng.h"
#include "serverlib/msg_const.h"
#include "serverlib/monitor_ctl.h"
#include "serverlib/sirena_queue.h"
#include "serverlib/ehelpsig.h"
#include "serverlib/internal_msgid.h"
#include "serverlib/EdiHelpManager.h"
#include "serverlib/query_runner.h"
#include "serverlib/http_parser.h"
#include "serverlib/internal_msgid.h" 
#include "serverlib/posthooks.h"

#include "PgOraConfig.h"
#include "db_tquery.h"

#include "qrys.h"
#include "astra_utils.h"
#include "edi_utils.h"
#include "exceptions.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

namespace TypeBHelpMng {

using namespace std;
using namespace EXCEPTIONS;

struct TypeBHelp {
    std::string addr, intmsgid, text;
    int tlgs_id;
    int timeout; // seconds
    void Clear() {
        addr.clear();
        intmsgid.clear();
        text.clear();
        tlgs_id = ASTRA::NoExists;
        timeout = ASTRA::NoExists;
    }
    TypeBHelp(
            const std::string &aaddr,
            const std::string &aintmsgid,
            const std::string &atext,
            int atlgs_id,
            int atimeout
            ):
        addr(aaddr),
        intmsgid(aintmsgid),
        text(atext),
        tlgs_id(atlgs_id),
        timeout(atimeout)
    {};
    TypeBHelp() { Clear(); }
    TypeBHelp(int typeb_in_id)
    {
        fromDB(typeb_in_id);
    }
    void toDB();
    void fromDB(int typeb_in_id);
    int getTlgsId(int typeb_in_id);
};


void set_http_header(ServerFramework::HTTP::request &rq, const string &name, const string &value)
{
    const ServerFramework::HTTP::request::Headers::iterator cl = std::find(rq.headers.begin(),
            rq.headers.end(),
            name);
    if (rq.headers.end() != cl) {
        cl->value = value;
    }
}

int TypeBHelp::getTlgsId(int typeb_in_id)
{
    int result = ASTRA::NoExists;
    QParams QryParams;
    QryParams << QParam("typeb_in_id", otInteger, typeb_in_id);

    DB::TCachedQuery Qry(
        PgOra::getROSession("TLGS"),
       "select id from tlgs where typeb_tlg_id = :typeb_in_id",
        QryParams, STDLOG
    );

    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        if(result == ASTRA::NoExists)
            result = Qry.get().FieldAsInteger("id");
        else {
            // Смысл такой. Если запрос вернул более 1 строки,
            // то тлг состоит из нескольких частей,
            // чего по HTTP быть не может.
            // В этом случае результат не определен.
            result = ASTRA::NoExists;
            break;
        }
    }
    return result;
}

void TypeBHelp::fromDB(int typeb_in_id)
{
    Clear();
    if(typeb_in_id == ASTRA::NoExists) return;
    tlgs_id = getTlgsId(typeb_in_id);
    if(tlgs_id == ASTRA::NoExists) return;

    QParams params;
    params << QParam("tlgs_id", otInteger, tlgs_id);

    DB::TCachedQuery QryLock(
        PgOra::getRWSession("TYPEB_HELP"),
        "SELECT tlgs_id, address, intmsgid, text, timeout "
        "FROM typeb_help "
        "WHERE tlgs_id = :tlgs_id "
        "FOR UPDATE",
        params,
        STDLOG
    );
    QryLock.get().Execute();

    DB::TCachedQuery QryDel(
        PgOra::getRWSession("TYPEB_HELP"),
        "DELETE FROM typeb_help "
        "WHERE tlgs_id = :tlgs_id",
        params,
        STDLOG
    );
    QryDel.get().Execute();

    if (QryLock.get().RowsProcessed() == 0) {
        tlgs_id = ASTRA::NoExists;
    } else {
        addr = QryLock.get().FieldAsString("address");
        intmsgid = QryLock.get().FieldAsString("intmsgid");
        text = QryLock.get().FieldAsString("text");
    }
    timeout = ASTRA::NoExists;
}

void TypeBHelp::toDB()
{
    QParams params;
    params << QParam("address", otString, addr)
           << QParam("intmsgid", otString, intmsgid)
           << QParam("text", otString, text)
           << QParam("tlgs_id", otInteger, tlgs_id)
           << QParam("timeout", otDate, BASIC::date_time::NowUTC() + static_cast<TDateTime>(timeout) / (24 * 60 * 60));

    DB::TCachedQuery QryIns(
        PgOra::getRWSession("TYPEB_HELP"),
        "INSERT INTO typeb_help (address, intmsgid, text, tlgs_id, timeout) "
        "VALUES (:address, :intmsgid, :text, :tlgs_id, :timeout)",
        params,
        STDLOG
    );

    QryIns.get().Execute();
}

string IntMsgIdAsString(const int msg_id[])
{
    ostringstream res;
    res
        << hex << setfill('0')
        << setw(8) << htonl(msg_id[0])
        << setw(8) << htonl(msg_id[1])
        << setw(8) << htonl(msg_id[2]);
    return upperc(res.str());
}

void configForPerespros(int tlgs_id)
{
    int timeout = 40;

    Tcl_Obj *obj;
    static string addr;

    if (addr.empty()) {
        obj = Tcl_ObjGetVar2(getTclInterpretator(),
                current_group(),
                Tcl_NewStringObj("SIGNAL", -1),
                TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
        if (!obj)
        {
            ProgError(STDLOG, "Tcl_ObjGetVar2:%s",
                    Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
            Tcl_ResetResult(getTclInterpretator());
        }
        addr = Tcl_GetString(obj);
    }
    TypeBHelp typeb_help(
            addr,
            IntMsgIdAsString(get_internal_msgid()),
            ServerFramework::HTTP::get_cur_http_request().to_string(),
            tlgs_id,
            timeout
            );
    typeb_help.toDB();

    set_msg_type_and_timeout(MSG_ANSW_STORE_WAIT_SIG, timeout);
}

void make_notify_msg(string &msg, const string &content)
{
    ServerFramework::HTTP::request_parser parser;
    ServerFramework::HTTP::request rq;
    parser.parse(rq, msg.begin(), msg.end());
    set_http_header(rq, "Content-Length", boost::lexical_cast<std::string>(content.size()));
    set_http_header(rq, "OPERATION", "kick");
    msg = rq.to_string() + content;
}

// deprecated! used in typeb_handler.cpp only!
bool notify(int typeb_in_id, int typeb_out_id)
{
    return notify_ok(typeb_in_id, typeb_out_id);
}

bool notify_ok(int typeb_in_id, int typeb_out_id)
{
    return notify_msg(typeb_in_id, IntToString(typeb_out_id));
}

bool notify_msg(int typeb_in_id, const string &str)
{
    TypeBHelp typeb_help(typeb_in_id);
    bool result = typeb_help.tlgs_id != ASTRA::NoExists;
    if(result) {
        string intmsgid;
        if (!HexToString(typeb_help.intmsgid,intmsgid) || intmsgid.size()!=sizeof(int)*3)
            throw EXCEPTIONS::Exception("TypeBHelpMng.notify: wrong intmsgid=%s", typeb_help.intmsgid.c_str());
        std::array<uint32_t,3> a;
        memcpy(&a, intmsgid.c_str(), 12);
        make_notify_msg(typeb_help.text, str);
        sethAfter(EdiHelpSignal(ServerFramework::InternalMsgId(a),
                    typeb_help.addr.c_str(),
                    typeb_help.text.c_str()));
    }
    return result;
}

void clean_typeb_help()
{
    DB::TCachedQuery QryDel(
        PgOra::getRWSession("TYPEB_HELP"),
        "DELETE FROM typeb_help "
        "WHERE timeout < :timeout",
        QParams() << QParam("timeout", otDate, BASIC::date_time::NowUTC()),
        STDLOG
    );
    QryDel.get().Execute();
}

}
