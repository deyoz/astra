#include <tcl.h>
#include "ocilocal.h"
#include "edilib/edi_func_cpp.h"
#include "edilib/edi_types.h"
#include "edilib/edi_astra_msg_types.h"
#include "edi_tlg.h"
#include "edi_msg.h"
#include "astra_utils.h"
#include "etick/lang.h"

//#include "etick/exceptions.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"

using namespace edilib;
using namespace edilib::EdiSess;
using namespace Ticketing;


static edi_loaded_char_sets edi_chrset[]=
{
    {"IATA", "\x3A\x2B,\x3F \x27" /* :+,? ' */},
    {"IATB", "\x1F\x1D,\x3F\x1D\x1C" /*Пурга какая-то!*/},
    {"SIRE", "\x3A\x2B,\x3F \"\n"}
};

struct lsTKCREQ {
};
void lsTKTREC_destruct(void *data)
{

}

static EDI_MSG_TYPE edi_msg_proc[]=
{
#include "edilib/astra_msg_types.etp"
};
static int edi_proc_sz = sizeof(edi_msg_proc)/sizeof(edi_msg_proc[0]);


int FuncAfterEdiParseErr(int parse_ret, void *udata, int *err)
{
    if(parse_ret==EDI_MES_STRUCT_ERR)
    {
        //SendEdiTlgErrCONTRL(get_tlg_context()->rot_num, 0, *err);
    } else {
        *err=1;
    }

    return parse_ret;
}

int FuncBeforeEdiProc(edi_mes_head *pHead, void *udata, int *err)
{
#if 0
    edi_udata * data = ((edi_udata *)udata);
    int ret=0;
    try{
        data->sessData()->setMesHead(*pHead);

        UpdateEdiSession(data->sessData());
        ProgTrace(TRACE1,"Check edifact session - Ok");
        /* ВСЕ ХОРОШО, ПРОДОЛЖАЕМ ... */
        Utils::BeforeSoftError();
    }
    catch(edilib::Exception &e)
    {
        WriteLog(STDLOG, e.what());
        SendEdiTlgErrCONTRL(get_tlg_context()->rot_num, 0, *err);
        ret-=100;
        ProgTrace(TRACE2,"Read EDIFACT message / update EDIFACT session - failed");
    }
    return ret;
#endif
    return 0;
}

int FuncAfterEdiProc(edi_mes_head *pHead, void *udata, int *err)
{
#if 0
    int ret=0;
    edi_udata * data = ((edi_udata *)udata);
    if(pHead->msg_type_req == RESPONSE){
        /*Если обрабатываем ответ*/
        try{
            CommitEdiSession(data->sessData()->ediSession());
        }
        catch (tick_exception &x){
            *err=ret=-110;
            Utils::AfterSoftError();
            SendEdiTlgErrCONTRL(get_tlg_context()->rot_num, 0, *err);
        }
        catch(...){
            ProgError(STDLOG, "Unknown exception!");
        }
    }
    return ret;
#endif
    return 0;
}

int FuncAfterEdiProcErr(edi_mes_head *pHead, int ret, void *udata, int *err)
{
#if 0
    tst();
    if(ret < 0){
        Utils::AfterSoftError();
        if(*err == 0) {
            *err = 1;
        }
        SendEdiTlgErrCONTRL(get_tlg_context()->rot_num, 0, *err);
        ret -= 120;
    }
    return ret;
#endif
    return 0;
}

int FuncBeforeEdiSend(edi_mes_head *pHead, void *udata, int *err)
{

    tst();
    return 0;
}

int FuncAfterEdiSendErr(edi_mes_head *pHead, int ret, void *udata, int *err)
{
    tst();
    if(*err == 0) {
        *err = 1;//EDI_PROC_ERR;
    }
    ret -= 140;
    return ret;
}

int FuncAfterEdiSend(edi_mes_head *pHead, void *udata, int *err)
{
    int ret=0;
    edi_udata *ed=(edi_udata *) udata;

    try {
        std::string tlg = edilib::WriteEdiMessage(GetEdiMesStructW());
//        strcpy(last_unique, pHead->our_ref);

        // Создает запись в БД
        CommitEdiSession(ed->sessData()->ediSession());
        DeleteMesOutgoing();

        ProgTrace(TRACE1,"tlg out: %s", tlg.c_str());
        SendTlgType("MOWRT", "MOWRA", true, 99, tlg);
    }
    catch (edilib::Exception &x){
        ProgError(STDLOG, "%s", x.what());
        *err=1;//PROG_ERR;
    }
    catch(...)
    {
        ProgError(STDLOG, "PROG ERR");
        *err=1;
    }

    if (*err){
        DeleteMesOutgoing();
        if(!ret) ret=-9;
    }
    return ret;
}

void ParseTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);
void ProcTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);
void CreateTKCREQdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);

message_funcs_type message_TKCREQ[] =
{
    {"131", ParseTKCRESdisplay,
            ProcTKCRESdisplay,
            CreateTKCREQdisplay,
            "Ticket display"},
};

message_funcs_str message_funcs[] =
{
    {TKTREQ, "Ticketing", message_TKCREQ, sizeof(message_TKCREQ)/sizeof(message_TKCREQ[0])},
    {TKTRES, "Ticketing", message_TKCREQ, sizeof(message_TKCREQ)/sizeof(message_TKCREQ[0])},
};

int init_edifact()
{
    InitEdiLogger(ProgError,WriteLog,ProgTrace);

    if(CreateTemplateMessagesCur(LD,NULL)){
        return -1;
    }
    if(InitEdiTypes(edi_msg_proc, edi_proc_sz)){
        ProgError(STDLOG,"InitEdiTypes filed");
        return -2;
    }

    SetEdiTempServiceFunc(FuncAfterEdiParseErr,
                          FuncBeforeEdiProc,
                          FuncAfterEdiProc,
                          FuncAfterEdiProcErr,
                          FuncBeforeEdiSend,
                          FuncAfterEdiSend,
                          FuncAfterEdiSendErr);

    if(InitEdiCharSet(edi_chrset, sizeof(edi_chrset)/sizeof(edi_chrset[0]))){
        ProgError(STDLOG,"InitEdiCharSet() failed");
        return -3;
    }
    edilib::EdiSess::EdiSessLib::Instance()->
            setCallBacks(new edilib::EdiSess::EdiSessCallBack());

    EdiMesFuncs::init_messages(message_funcs,
                               sizeof(message_funcs)/sizeof(message_funcs[0]));
    return 0;
}

// Обработка EDIFACT
void proc_edifact(const std::string &tlg)
{
    edi_udata_rd udata(new AstraEdiSessRD());
    int err=0, ret;

    edi_mes_head edih;
    memset(&edih,0, sizeof(edih));
    udata.sessDataRd()->setMesHead(edih);

    ProgTrace(TRACE2, "Edifact Handle");
    ret = FullObrEdiMessage(tlg.c_str(),&edih,&udata,&err);

    if(ret){
        throw edi_fatal_except(STDLOG, EdiErr::EDI_PROC_ERR, "Ошибка обработки");
    }
}

EdiMesFuncs::messages_map_t *EdiMesFuncs::messages_map;
const message_funcs_type &EdiMesFuncs::GetEdiFunc(
        edi_msg_types_t mes_type, const std::string &msg_code)
{
    messages_map_t::const_iterator iter = get_map()->find(mes_type);
    if(iter == get_map()->end())
    {
        throw edi_fatal_except(STDLOG,EdiErr::EDI_PROC_ERR,
                                "No such message type %d in message function array",
                                mes_type);
    }
    const types_map_t &tmap = iter->second;
    types_map_t::const_iterator iter2 = tmap.find(msg_code);
    if(iter2 == tmap.end()){
        //err
        throw edi_soft_except (STDLOG, EdiErr::EDI_INV_MESSAGE_F,
                                "Unknown message function for message %d, code=%s",
                                mes_type, msg_code.c_str());
    }
    if(!iter2->second.parse || !iter2->second.proc || !iter2->second.collect_req)
    {
        throw edi_soft_except (STDLOG, EdiErr::EDI_NS_MESSAGE_F,
                                "Message function %s not supported", msg_code.c_str());
    }
    return iter2->second;

}

void SendEdiTlgTKCREQ_Disp(TickDisp &TDisp)
{
    int err=0;
    edi_udata_wr ud(new AstraEdiSessWR(), "131");

    tst();
    int ret = SendEdiMessage(TKTREQ, ud.sessData()->edih(), &ud, &TDisp, &err);
    if(!ret){
    } else {
        //throw
    }
}

void CreateTKCREQdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{
    EDI_REAL_MES_STRUCT *pMes = GetEdiMesStructW();
    TickDisp &TickD = dynamic_cast<TickDisp &>(*data);

    switch(TickD.dispType())
    {
        case TickDispByTickNo:
        {
            TickDispByNum & TickDisp= dynamic_cast<TickDispByNum &>(TickD);

            SetEdiSegGr(pMes, 1);
            SetEdiPointToSegGrW(pMes, 1);
            SetEdiFullSegment(pMes, "TKT",0, TickDisp.tickNum());
        }
            break;
        default:
            throw EdiExcept("Unsupported dispType");
    }
}

void ParseTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{
    TST();
    // Запись телеграммы в спец таблицу, для связи с obrzap'ом
    // вызов переспроса
}
void ProcTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{
    TST();
}


int ProcEDIREQ (edi_mes_head *pHead, void *udata, void *data, int *err)
{
    ProgTrace(TRACE4, "ProcEDIREQ: tlg_in is %s", pHead->msg_type_str->code);

    edi_udata *ud = (edi_udata *)udata;
    const message_funcs_type &mes_funcs=
            EdiMesFuncs::GetEdiFunc(pHead->msg_type,
                                    edilib::GetDBFName(GetEdiMesStruct(),
                                    edilib::DataElement(1225),
                                    EdiErr::EDI_PROC_ERR,
                                    edilib::CompElement("C302"),
                                    edilib::SegmElement("MSG")));

    mes_funcs.parse(pHead, *ud, 0);
    mes_funcs.proc(pHead, *ud, 0);
    return 0;
}

int CreateEDIREQ (edi_mes_head *pHead, void *udata, void *data, int *err)
{

    try{
        EDI_REAL_MES_STRUCT *pMes = GetEdiMesStructW();
        edi_udata_wr *ed=(edi_udata_wr *) udata;
        // Заполняет стр-ры: edi_mes_head && EdiSession
        // Из первой создастся стр-ра edifact сообщения
        // Из второй запись в БД

        const message_funcs_type &mes_funcs=
                EdiMesFuncs::GetEdiFunc(pHead->msg_type, ed->msgId());

        SetEdiSessMesAttrOnly(ed->sessDataWr());
        // Создает стр-ру EDIFACT
        if(::CreateMesByHead(ed->sessData()->edih()))
        {
            throw EdiExcept("Error in CreateMesByHead");
        }
        tst();

        SetEdiFullSegment(pMes, "MSG",0, ":"+ed->msgId());
        SetEdiFullSegment(pMes, "ORG",0, "NW+52519950+++A++PJ");

        mes_funcs.collect_req(pHead, *ed, static_cast<TickDisp *>(data));
    }
    catch(std::exception &e)
    {
        ProgError(STDLOG, e.what());
        *err = 1;
    }
    catch(...)
    {
        ProgError(STDLOG,"Unknown exception");
        *err = 1;
    }
    return *err;
}

int ProcCONTRL(edi_mes_head *pHead, void *udata, void *data, int *err)
{
    return 0;
}

int CreateCONTRL (edi_mes_head *pHead, void *udata, void *data, int *err)
{
    return 0;
}
