#include "tlg.h"
#include "serverlib/ourtime.h"
#include "exceptions.h"
#include "oralib.h"
//#include "astra_service.h"
#include "file_queue.h"
#include "astra_utils.h"
#include "http_io.h"
#include "misc.h"
#include "xml_unit.h"
#include "serverlib/posthooks.h"
#include "telegram.h"
#include "typeb_utils.h"

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

void send_error(
        ostringstream &msg,
        const string &tlg_short_name,
        int id,
        const string &param_name,
        const string &param_value,
        const string &err_msg = "")
{
    msg
        << FILE_HTTP_TYPEB_TYPE << ": �訡�� ��ࠢ�� ⥫��ࠬ�� " << tlg_short_name << ". "
        << "(��=" << id << ", " << param_name << "=" << param_value << "): "
            << err_msg;
}

void parse_format_ayt(const string &answer, ostringstream &msg)
{
    bool result = false;
    xmlDocPtr xml_res = NULL;
    try {
        xml_res = TextToXMLTree( answer );
        if(xml_res == NULL)
            throw Exception("TextToXMLTree failed");
        xmlNodePtr resNode = xmlDocGetRootElement(xml_res);
        result = (string)NodeAsStringFast("boolean", resNode) == "true";
        xmlFreeDoc(xml_res);
    } catch(Exception &E) {
        if(xml_res)
            xmlFreeDoc(xml_res);
        msg << " �訡�� ࠧ��� XML-�⢥�. '" << answer << "': " << E.what();
        throw;
    } catch(...) {
        if(xml_res)
            xmlFreeDoc(xml_res);
        msg << " �訡�� ࠧ��� XML-�⢥�. '" << answer << "'";
        throw;
    }
    if(not result) msg << " ����祭 �⢥� false.";
}

enum TTypeBFormat {
    tbfAYT,
    tbfUnknown
};

const char *TTypeBFormatS[] =
{
    "AYT",
    ""
};

TTypeBFormat DecodeTypeBFormat(const string &s)
{
    unsigned int i;
    for(i=0;i<sizeof(TTypeBFormatS)/sizeof(TTypeBFormatS[0]);i+=1) if (s == TTypeBFormatS[i]) break;
    if (i<sizeof(TTypeBFormatS)/sizeof(TTypeBFormatS[0]))
        return (TTypeBFormat)i;
    else
        return tbfUnknown;
};

const char* EncodeTypeBFormat(TTypeBFormat p)
{
    return TTypeBFormatS[p];
};

typedef string  (*TTypeBFormatSend)(const string &uri, const string &data);
typedef void  (*TTypeBFormatParse)(const string &answer, ostringstream &msg);

static void scan_tlg(void)
{
  time_t time_start=time(NULL);
  TFileQueue file_queue;
  file_queue.get( TFilterQueue( OWN_POINT_ADDR(), FILE_HTTP_TYPEB_TYPE ) );
  int trace_count=0;
  for ( TFileQueue::iterator item=file_queue.begin();
        item!=file_queue.end(); item++, trace_count++, ASTRA::commit() ) {
    try {
      TTlgOutPartInfo p;
      p.addFromFileParams(item->params); //���砫� �⠥� ��ࠬ����, ⠪ ��� � �⮩ ��楤�� TTlgOutPartInfo ������
      p.body=item->data;

      string tlg_short_name = ((TTypeBTypesRow&)(base_tables.get("typeb_types").get_row("code",p.tlg_type))).short_name;
      TTypeBFormat format = DecodeTypeBFormat(item->params[FILE_PARAM_FORMAT]);
      TTypeBFormatSend sender = NULL;
      TTypeBFormatParse parser = NULL;
      if(format == tbfUnknown)
        throw Exception("param %s not found for %s", FILE_PARAM_FORMAT.c_str(), item->params[FILE_PARAM_FORMAT].c_str());
      switch(format) {
        case tbfAYT:
              sender = send_format_ayt;
              parser = parse_format_ayt;
              break;
        default:
              throw Exception("format %s not supported yet", EncodeTypeBFormat(format));
      }
      for(map<string, string>::iterator im = item->params.begin(); im != item->params.end(); im++) {
        if(validate_param_name(im->first, pnHTTP)) { // handle HTTP
          string str_result, lexema_id;
          LEvntPrms params;
          ostringstream msg;
          bool err = false;
          try {
            if(not sender)
              throw Exception("sender not defined");

            str_result = (*sender)(im->second, p.heading+p.body+p.ending);
            msg
              << FILE_HTTP_TYPEB_TYPE << ": �����ࠬ�� " << tlg_short_name << " "
              << "(��=" << item->id << ", " << im->first << "=" << im->second << ") "
              << "��ࠢ����.";
            params << PrmElem<std::string>("tlg_name", etTypeBType, p.tlg_type, efmtNameShort)
                   << PrmSmpl<int>("tlg_id", item->id) << PrmSmpl<string>("first", im->first)
                   << PrmSmpl<string>("second", im->second);
            if(str_result.empty()) {
              err = true;
              msg << " ��� �⢥� �� �ࢥ�.";
              lexema_id = "EVT.HTTP_TYPEB_TLG_SENT_NO_RESPONSE";
            }
            else
              lexema_id = "EVT.HTTP_TYPEB_TLG_SENT";
          }
          catch(Exception &E) {
            err = true;
            send_error(msg, tlg_short_name, item->id, im->first, im->second, E.what());
          }
          catch(...) {
            err = true;
            if(not parser)
              throw Exception("parser not defined");
            send_error(msg, tlg_short_name, item->id, im->first, im->second);
          }
          if(err) {
            TReqInfo::Instance()->LocaleToLog(lexema_id, params, evtTlg, p.point_id, item->id);
            continue;
          }
          try {
           (*parser)(str_result, msg);
          }
          catch(...) {
            TReqInfo::Instance()->LocaleToLog(lexema_id, params, evtTlg, p.point_id, item->id);
            continue;
          }
          TReqInfo::Instance()->LocaleToLog(lexema_id, params, evtTlg, p.point_id, item->id);
          TFileQueue::deleteFile(item->id);
          break;
        } else
            if(validate_param_name(im->first, pnSITA)) { // handle SITA
              p.id=NoExists;
              p.num=1;
              p.addr=TypeB::format_addr_line(im->second);
              TelegramInterface::SaveTlgOutPart(p, true, false);
              TelegramInterface::SendTlg(p.id, false);
              TFileQueue::deleteFile(item->id);
              break;
            }
      } // end for map fileparams
    }
    catch(Exception &E) {
        ASTRA::rollback();
        try
        {
           if (isIgnoredEOracleError(E)) continue;
           ProgError(STDLOG,"Exception: %s (file id=%d)",E.what(),item->id);
        }
        catch(...) {};

    }
    catch(...) {
        ASTRA::rollback();
        ProgError(STDLOG, "Something goes wrong");
    }
  }
  time_t time_end=time(NULL);
  if (time_end-time_start>5)
      ProgTrace(TRACE5,"Attention! %s scan_tlg execute time: %ld secs, count=%d",
              FILE_HTTP_TYPEB_TYPE.c_str(), time_end-time_start,trace_count);
}

static int WAIT_HTTP_TYPEB_INTERVAL()       //�����ᥪ㭤�
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("TLG_SND_WAIT_HTTP_TYPEB_INTERVAL",1,NoExists,60000);
  return VAR;
};

int main_http_snd_tcl(int supervisorSocket, int argc, char *argv[])
{
    try
    {
        sleep(5);
        InitLogTime(argc>0?argv[0]:NULL);

        ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();

        char buf[10];
        for (;;)
        {
            InitLogTime(argc>0?argv[0]:NULL);

            scan_tlg();

            waitCmd("CMD_TLG_HTTP_SND",WAIT_HTTP_TYPEB_INTERVAL(),buf,sizeof(buf));
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
        ASTRA::rollback();
        OraSession.LogOff();
    }
    catch(...)
    {
        ProgError(STDLOG, "Unknown exception");
    };
    return 0;
}
