#include "TypeBHelpMng.h"
#include "serverlib/msg_const.h"
#include "serverlib/monitor_ctl.h"
#include "serverlib/sirena_queue.h"
#include "serverlib/ehelpsig.h"
#include "serverlib/internal_msgid.h"
#include "serverlib/EdiHelpManager.h"
#include "serverlib/query_runner.h"
#include "serverlib/http_parser.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

#include "qrys.h"
#include "astra_utils.h"
#include "edi_utils.h"
#include "exceptions.h"

namespace TypeBHelpMng {

using namespace std;
using namespace EXCEPTIONS;

void set_http_header(ServerFramework::HTTP::request &rq, const string &name, const string &value)
{
    const ServerFramework::HTTP::request::Headers::iterator cl = std::find(rq.headers.begin(),
            rq.headers.end(),
            name);
    if (rq.headers.end() != cl) {
        cl->value = value;
    }
}

void TypeBHelp::fromDB(int typeb_in_id)
{
    if(typeb_in_id == ASTRA::NoExists) return;

    QParams QryParams;
    QryParams
        << QParam("typeb_in_id", otInteger, typeb_in_id)
        << QParam("tlgs_id", otInteger)
        << QParam("address", otString)
        << QParam("intmsgid", otString)
        << QParam("text", otString)
        << QParam("timeout", otDate)
        ;
    TCachedQuery Qry(
            "begin "
            "delete from typeb_help where tlgs_id = "
            "   (select id from tlgs where typeb_tlg_id = :typeb_in_id) "
            "returning "
            "   tlgs_id, "
            "   address, "
            "   intmsgid, "
            "   text, "
            "   timeout "
            "into "
            "   :tlgs_id, "
            "   :address, "
            "   :intmsgid, "
            "   :text, "
            "   :timeout; "
            "end; ",
            QryParams);
    Qry.get().Execute();
    addr = Qry.get().GetVariableAsString("address");
    intmsgid = Qry.get().GetVariableAsString("intmsgid");
    text = Qry.get().GetVariableAsString("text");
    tlgs_id = Qry.get().GetVariableAsInteger("tlgs_id");
    timeout = ASTRA::NoExists;
}

void TypeBHelp::toDB()
{
    QParams QryParams;
    QryParams
        << QParam("address", otString, addr)
        << QParam("intmsgid", otString, intmsgid)
        << QParam("text", otString, text)
        << QParam("tlgs_id", otInteger, tlgs_id)
        << QParam("timeout", otInteger, timeout);
    TCachedQuery Qry(
            "insert into typeb_help(address, intmsgid, text, tlgs_id, timeout) "
            "values(:address, :intmsgid, :text, :tlgs_id, system.utcsysdate + :timeout/(24*60*60))", QryParams);
    Qry.get().Execute();
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
    int timeout = 70;
    set_msg_type_and_timeout(MSG_ANSW_STORE_WAIT_SIG, timeout);

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
}

void make_notify_msg(string &msg, int typeb_out_id)
{
    ServerFramework::HTTP::request_parser parser;
    ServerFramework::HTTP::request rq;
    parser.parse(rq, msg.begin(), msg.end());
    string str_id = IntToString(typeb_out_id);
    set_http_header(rq, "Content-Length", boost::lexical_cast<std::string>(str_id.size()));
    set_http_header(rq, "OPERATION", "kick");
    msg = rq.to_string() + str_id;
}

bool notify(int typeb_in_id, int typeb_out_id)
{
    TypeBHelp typeb_help(typeb_in_id);
    bool result = typeb_help.tlgs_id != ASTRA::NoExists;
    if(result) {
        string intmsgid;
        if (!HexToString(typeb_help.intmsgid,intmsgid) || intmsgid.size()!=sizeof(int)*3)
            throw EXCEPTIONS::Exception("TypeBHelpMng.notify: wrong intmsgid=%s", typeb_help.intmsgid.c_str());
        make_notify_msg(typeb_help.text, typeb_out_id);
        sethAfter(EdiHelpSignal((const int*)intmsgid.c_str(),
                    typeb_help.addr.c_str(),
                    typeb_help.text.c_str()));
    }
    return result;
}

void clean_typeb_help()
{
    TCachedQuery Qry("delete from typeb_help where timeout < system.utcsysdate");
    Qry.get().Execute();
}

}
