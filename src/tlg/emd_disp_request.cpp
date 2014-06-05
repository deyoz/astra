#include "config.h"
#include "tlg/emd_disp_request.h"
#include <edilib/edi_func_cpp.h>
#include "astra_context.h"
#include <serverlib/EdiHelpManager.h>
#include "serverlib/query_runner.h"
#include "astra_ticket.h"
#include "edilib/edi_astra_msg_types.h"

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

using namespace edilib;

void CreateTKCREQEmdDisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{
    EDI_REAL_MES_STRUCT *pMes = GetEdiMesStructW();
    EmdDispParams &TickD = dynamic_cast<EmdDispParams &>(*data);

    switch(TickD.dispType())
    {
        case emdDispByNum:
        {
            EmdDispByNum & TickDisp= dynamic_cast<EmdDispByNum &>(TickD);

            SetEdiSegGr(pMes, 1);
            SetEdiPointToSegGrW(pMes, 1);
            SetEdiFullSegment(pMes, "TKT",0, TickDisp.tickNum());
        }
        break;
        default:
            throw EdiExcept("Unsupported dispType");
    }

    //запишем контексты
    AstraContext::SetContext("EDI_SESSION",
                             udata.sessData()->ediSession()->ida().get(),
                             TickD.context());

    if (TickD.req_ctxt_id()!=ASTRA::NoExists)
    {
      AstraContext::SetContext("EDI_HELP_INTMSGID",
                                 udata.sessData()->ediSession()->ida().get(),
                                 get_internal_msgid_hex());

      ServerFramework::getQueryRunner().getEdiHelpManager().
              configForPerespros(STDLOG, prepareKickText("ETSearchForm", TickD.req_ctxt_id()).c_str(),-1,15);
      LogTrace(TRACE3) << "get_internal_msgid_hex() = " << get_internal_msgid_hex();
    } else {
      LogTrace(TRACE3) << "TickD.req_ctxt_id() = " << TickD.req_ctxt_id();
    }
}

void ProcTKCRESemdDisplay(edi_mes_head *pHead, edi_udata &udata,
                             edi_common_data *data)
{

}

void ParseTKCRESemdDisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{

}

void SendEdiTlgTKCREQ_Disp(EmdDispParams &TDisp)
{
    int err=0;
    edi_udata_wr ud(new AstraEdiSessWR(TDisp.org().pult()), EdiMess::EmdDisplay);

    int ret = SendEdiMessage(TKCREQ, ud.sessData()->edih(), &ud, &TDisp, &err);
    if(ret)
    {
        throw EXCEPTIONS::Exception("SendEdiMessage DISPLAY failed");
    }
}


#ifdef XP_TESTING
#include <serverlib/func_placeholders.h>
static std::string FP_send_emd_disp(const std::vector<std::string> &p)
{
    Ticketing::OrigOfRequest org("UT");

    EmdDispByNum params(org, "", 0, p.at(0));
    SendEdiTlgTKCREQ_Disp(params);
    return "";
}

FP_REGISTER("send_emd_disp", FP_send_emd_disp);

#endif /* XP_TESTING */
