#include "EmdSysUpdateResponseHandler.h"

//��⮬ ��७��� � edi_context.h
#include "stl_utils.h"
#include "xml_unit.h"
#include "emdoc.h"
#include "edi_utils.h"
#include <jxtlib/xml_stuff.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

using namespace std;

namespace TlgHandling
{

EmdSysUpdateResponseHandler::EmdSysUpdateResponseHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                                         const edilib::EdiSessRdData* edisess)
    : AstraEdiResponseHandler(pMes, edisess)
{
}

void EmdSysUpdateResponseHandler::handle()
{
  try
  {
    ProgTrace(TRACE5, "EmdSysUpdateResponseHandler::handle");

    XMLDoc ediSessCtxt;
    AstraEdifact::getEdiSessionCtxt(ediSessId().get(), true, "EmdSysUpdateResponseHandler::handle", ediSessCtxt, false);
    if (ediSessCtxt.docPtr()!=NULL)
    {
      using namespace Ticketing;

      xmlNodePtr node=NodeAsNode("/context/emdoc",ediSessCtxt.docPtr());

      TEMDCtxtItem EMDCtxt;
      EMDCtxt.fromXML(node);

      TEMDocItem EMDocItem;
      EMDocItem.emd_no=EMDCtxt.emd_no;
      EMDocItem.emd_coupon=EMDCtxt.emd_coupon;
      EMDocItem.et_no=EMDCtxt.pax.tkn.no;
      EMDocItem.et_coupon=EMDCtxt.pax.tkn.coupon;
      EMDocItem.action=EMDCtxt.action;
      EMDocItem.point_id=EMDCtxt.point_id;

      using namespace edifact;
      pRemoteResults res = remoteResults();
      if (res==NULL)
        throw EXCEPTIONS::Exception("EmdSysUpdateResponseHandler::handle: res==NULL (ediSessId()=%d)", ediSessId().get());

      if (res->status() == RemoteStatus::CommonError)
      {
        if (res->remark().empty())
          EMDocItem.system_update_error="������ " + res->ediErrCode();
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

        AstraEdifact::ProcEvent(event, EMDCtxt, NULL, false);
      }
    }
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "EmdSysUpdateResponseHandler::handle: %s", e.what());
  }
}

}//namespace TlgHandling
