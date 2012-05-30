#include "tlg.h"
#include "serverlib/ourtime.h"
#include "exceptions.h"
#include "oralib.h"
#include "astra_service.h"
#include "astra_utils.h"
#include "http_io.h"
#include "misc.h"
#include "xml_unit.h"

#define NICKNAME "DEN"
#include "serverlib/test.h"

using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace std;

static int sockfd=-1;

const string BSM_PARAM_PREFIX = "ADDR";
const string BSM_PARAM_HTTP = "_HTTP";
const string BSM_PARAM_SITA = "_SITA";
enum TParamName {pnHTTP, pnSITA};

bool validate_param_name(const string &val, TParamName pn)
{
    string postfix = (pn == pnHTTP ? BSM_PARAM_HTTP : BSM_PARAM_SITA);
    bool result =
        (val.size() >= BSM_PARAM_PREFIX.size() + postfix.size() + 1) and
        (val.substr(0, BSM_PARAM_PREFIX.size()) == BSM_PARAM_PREFIX) and
        (val.substr(val.size() - postfix.size()) == postfix);
    string num = val.substr(BSM_PARAM_PREFIX.size(), val.size() - BSM_PARAM_PREFIX.size() - postfix.size());
    if(result)
        for(string::iterator si = num.begin(); si != num.end(); si++)
            if(not IsDigit(*si)) {
                result = false;
                break;
            }
    return result;
}

static void scan_tlg(void)
{
    static TQuery TlgQry(&OraSession);
    if (TlgQry.SQLText.IsEmpty()) {
        TlgQry.Clear();
        TlgQry.SQLText =
            "select file_queue.id, files.data from file_queue, files where "
            "   file_queue.type = :type and "
            "   file_queue.sender = :sender and "
            "   file_queue.receiver = :receiver and "
            "   file_queue.status = :status and "
            "   file_queue.id = files.id ";
        TlgQry.CreateVariable("type", otString, "HTTPGET");
        TlgQry.CreateVariable("sender", otString, OWN_POINT_ADDR());
        TlgQry.CreateVariable("receiver", otString, OWN_POINT_ADDR());
        TlgQry.CreateVariable("status", otString, "PUT");
    }
    TlgQry.Execute();
    for(; not TlgQry.Eof; TlgQry.Next(), OraSession.Commit()) {
        int id = TlgQry.FieldAsInteger("id");
        ProgTrace(TRACE5, "processing id %d", id);
        try {
            int len = TlgQry.GetSizeLongField( "data" );
            void *p = (char*)malloc( len );
            if ( !p )
                throw Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
            TlgQry.FieldAsLong( "data", p );
            string data( (char*)p, len );

            map<string, string> fileparams;
            getFileParams(OWN_POINT_ADDR(), "HTTPGET", id, fileparams, true);
            for(map<string, string>::iterator im = fileparams.begin(); im != fileparams.end(); im++) {
                if(validate_param_name(im->first, pnHTTP)) { // handle HTTP
                    //!!!            send_bsm("bsm.icfairports.com", data);
                    string str_result;
                    ostringstream msg;
                    bool err = false;
                    try {
                        str_result = send_bsm(im->second, data);
                        msg
                            << "HTTPGET: Телеграмма BSM "
                            << "(ид=" << id << ", " << im->first << "=" << im->second << ") "
                            << "отправлена.";
                    } catch(Exception &E) {
                        err = true;
                        msg
                            << "HTTPGET: Ошибка отправки телеграммы BSM. "
                            << "(ид=" << id << ", " << im->first << "=" << im->second << "): "
                                << E.what();
                    } catch(...) {
                        err = true;
                        msg
                            << "HTTPGET: Ошибка отправки телеграммы BSM. "
                            << "(ид=" << id << ", " << im->first << "=" << im->second << "): ";
                    }
                    if(err) {
                        TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,id);
                        continue;
                    }
                    bool result = false;
                    xmlDocPtr xml_res = NULL;
                    try {
                        xml_res = TextToXMLTree( str_result );
                        if(xml_res == NULL)
                            throw Exception("TextToXMLTree failed");
                        xmlNodePtr resNode = xmlDocGetRootElement(xml_res);
                        result = (string)NodeAsStringFast("boolean", resNode) == "true";
                        xmlFreeDoc(xml_res);
                    } catch(...) {
                        if(xml_res)
                            xmlFreeDoc(xml_res);
                        msg << " Ошибка разбора XML-ответа.";
                        TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,id);
                        continue;
                    }
                    if(not result) msg << " Получен ответ false.";
                    TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,id);
                    if(result) break;
                } else if(validate_param_name(im->first, pnSITA)) // handle SITA
                    ;
            }
        } catch(...) {
            ProgTrace(TRACE5, "Something goes wrong");
        }

        deleteFile(id);
    }

}

int main_http_snd_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
    try
    {
        sleep(2);
        InitLogTime(NULL);
        OpenLogFile("logairimp");

        ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();

        char buf[10];
        for (;;)
        {
            InitLogTime(NULL);

            scan_tlg();

            waitCmd("CMD_TLG_HTTP_SND",60000,buf,sizeof(buf));
        }; // end of loop
    }
    catch(EOracleError &E)
    {
        ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
    }
    catch(std::exception &E)
    {
        ProgError(STDLOG,"std::exception: %s",E.what());
    }
    catch(...)
    {
        ProgError(STDLOG, "Unknown exception");
    };

    if (sockfd!=-1) close(sockfd);
    try
    {
        OraSession.Rollback();
        OraSession.LogOff();
    }
    catch(...)
    {
        ProgError(STDLOG, "Unknown exception");
    };
    return 0;
}
