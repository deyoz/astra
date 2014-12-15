#include "EmdSysUpdateResponseHandler.h"

#include "astra_context.h"
//потом перенести в edi_context.h
#include "stl_utils.h"
#include "xml_unit.h"
#include "emdoc.h"
#include <jxtlib/xml_stuff.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

using namespace std;

namespace TlgHandling
{

EmdSysUpdateResponseHandler::EmdSysUpdateResponseHandler(_EDI_REAL_MES_STRUCT_ *pmes,
                                                         const edilib::EdiSessRdData *edisess)
    : AstraEdiResponseHandler(pmes, edisess)
{
}

void EmdSysUpdateResponseHandler::parse()
{

}

void EmdSysUpdateResponseHandler::handle()
{
  try
  {
    ProgTrace(TRACE5, "EmdSysUpdateResponseHandler::handle");
    string ctxt;
    AstraContext::GetContext("EDI_SESSION", ediSessId().get(), ctxt);
    ctxt=ConvertCodepage(ctxt,"CP866","UTF-8");

    XMLDoc ediSessCtxt(ctxt);
    if (ediSessCtxt.docPtr()!=NULL)
    {
      using namespace Ticketing;
      //для нормальной работы надо все дерево перевести в CP866:
      xml_decode_nodelist(ediSessCtxt.docPtr()->children);

      xmlNodePtr node=NodeAsNode("/context/emdoc",ediSessCtxt.docPtr());

      TEMDCtxtItem EMDCtxt;
      EMDCtxt.fromXML(node);

      TEMDocItem EMDocItem;
      EMDocItem.emd_no=EMDCtxt.asvc.emd_no;
      EMDocItem.emd_coupon=EMDCtxt.asvc.emd_coupon;
      EMDocItem.et_no=EMDCtxt.pax.tkn.no;
      EMDocItem.et_coupon=EMDCtxt.pax.tkn.coupon;
      EMDocItem.action=EMDCtxt.action;      

      using namespace edifact;
      pRemoteResults res = remoteResults();
      if (res==NULL)
        throw EXCEPTIONS::Exception("EmdSysUpdateResponseHandler::handle: res==NULL (ediSessId()=%d)", ediSessId().get());

      if (res->status() == RemoteStatus::CommonError)
      {
        if (res->remark().empty())
          EMDocItem.system_update_error="ОШИБКА " + res->ediErrCode();
        else
          EMDocItem.system_update_error=res->remark();
      };

      if (res->status() == RemoteStatus::Success) EMDocItem.toDB(TEMDocItem::SystemUpdate);

      if (res->status() == RemoteStatus::Success ||
          res->status() == RemoteStatus::CommonError)
      {
        TLogLocale event;
        event.ev_type=ASTRA::evtPay;
        event.lexema_id=res->status() == RemoteStatus::Success?
              (EMDocItem.action==CpnStatAction::associate?"EVT.ETICK_EMD_ASSOCIATION":
                                                          "EVT.ETICK_EMD_DISASSOCIATION"):
              (EMDocItem.action==CpnStatAction::associate?"EVT.ETICK_EMD_ASSOCIATION_MISTAKE":
                                                          "EVT.ETICK_EMD_DISASSOCIATION_MISTAKE");
        event.prms << PrmSmpl<std::string>("et_no", EMDocItem.et_no)
                   << PrmSmpl<int>("et_coupon", EMDocItem.et_coupon)
                   << PrmSmpl<std::string>("emd_no", EMDocItem.emd_no)
                   << PrmSmpl<int>("emd_coupon", EMDocItem.emd_coupon);

        if (res->status() == RemoteStatus::CommonError)
          event.prms << PrmSmpl<std::string>("err", EMDocItem.system_update_error);

        ProcEdiEvent(event, EMDCtxt, NULL, false);
      };
    };
    AstraContext::ClearContext("EDI_SESSION", ediSessId().get());
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "EmdSysUpdateResponseHandler::handle: %s", e.what());
  };
}

void EmdSysUpdateResponseHandler::onTimeOut()
{

}

void EmdSysUpdateResponseHandler::onCONTRL()
{

}

}//namespace TlgHandling
