#include "EmdCosResponseHandler.h"
#include "remote_system_context.h"
#include "emdoc.h"
#include "edi_utils.h"
#include <jxtlib/xml_stuff.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

using namespace std;

namespace TlgHandling
{

namespace
{
    inline std::string getTlgSrc()
    {
        return Ticketing::RemoteSystemContext::SystemContext::Instance(STDLOG).inbTlgInfo().tlgSrc();
    }
}//namespace


EmdCosResponseHandler::EmdCosResponseHandler(_EDI_REAL_MES_STRUCT_ *pmes,
                                             const edilib::EdiSessRdData *edisess)
    : AstraEdiResponseHandler(pmes, edisess)
{}

void EmdCosResponseHandler::handle()
{
  try
  {
    ProgTrace(TRACE5, "EmdCosResponseHandler::handle");

    XMLDoc ediSessCtxt;
    AstraEdifact::getEdiSessionCtxt(ediSessId().get(), true, "EmdCosResponseHandler::handle", ediSessCtxt, false);
    if (ediSessCtxt.docPtr()!=NULL)
    {
      using namespace Ticketing;

      xmlNodePtr rootNode=NodeAsNode("/context",ediSessCtxt.docPtr());
      int req_ctxt_id=ASTRA::NoExists;
      if (GetNode("@req_ctxt_id",rootNode)!=NULL)
        req_ctxt_id=NodeAsInteger("@req_ctxt_id",rootNode);


      xmlNodePtr node=NodeAsNode("emdocs/emdoc", rootNode);

      TEMDCtxtItem EMDCtxt;
      EMDCtxt.fromXML(node, rootNode);

      TEMDocItem EMDocItem;
      EMDocItem.emd.no=EMDCtxt.emd.no;
      EMDocItem.emd.coupon=EMDCtxt.emd.coupon;
      EMDocItem.et.no=EMDCtxt.pax.tkn.no;
      EMDocItem.et.coupon=EMDCtxt.pax.tkn.coupon;
      EMDocItem.emd.action=EMDCtxt.emd.action;
      EMDocItem.point_id=EMDCtxt.point_id;

      using namespace edifact;
      pRemoteResults res = remoteResults();
      if (res==NULL)
        throw EXCEPTIONS::Exception("EmdCosResponseHandler::handle: res==NULL (ediSessId()=%d)", ediSessId().get());


      if (res->status() == RemoteStatus::Success)
        EMDocItem.emd.status=EMDCtxt.emd.status;

      if (res->status() == RemoteStatus::CommonError)
      {
        if (res->remark().empty())
          EMDocItem.change_status_error="ОШИБКА " + res->ediErrCode();
        else
          EMDocItem.change_status_error=res->remark();
      };

      if (res->status() == RemoteStatus::Success ||
          res->status() == RemoteStatus::CommonError)
      {
         TLogLocale event;
         event.ev_type=ASTRA::evtPay;

         if (res->status() == RemoteStatus::Success)
         {
           ProgTrace(TRACE5, "emd_no=%s emd_coupon=%d status=%s",
                     EMDocItem.emd.no.c_str(), EMDocItem.emd.coupon, EMDocItem.emd.status->dispCode());

           event.lexema_id="EVT.EMD_CHANGE_STATUS";
           event.prms << PrmSmpl<std::string>("doc_no", EMDocItem.emd.no)
                      << PrmSmpl<int>("coupon_no", EMDocItem.emd.coupon)
                      << PrmSmpl<std::string>("coupon_status", EMDocItem.emd.status->dispCode());
/*         нельзя сравнивать с предыдущим статусом так как не контролируем смену статуса EMD вместе со сменой статуса ЭБ
           TEMDocItem prior;
           prior.fromDB(EMDocItem.emd_no, EMDocItem.emd_coupon, true);
           bool repeated=!prior.empty() && prior.status==EMDocItem.status;
*/
           bool repeated=false;
           AstraEdifact::ProcEvent(event, EMDCtxt, node, repeated);
         };

         if (res->status() == RemoteStatus::CommonError)
         {
           ProgTrace(TRACE5, "emd_no=%s emd_coupon=%d error=%s",
                     EMDocItem.emd.no.c_str(), EMDocItem.emd.coupon, EMDocItem.change_status_error.c_str());

           //записываем в контекст для последующего вывода на экран терминала
           using namespace AstraLocale;
           bool isGlobal=false; //!!!потом научимся различать
           LexemaData error;
           error.lexema_id="MSG.EMD_CHANGE_STATUS_ERROR";
           error.lparams << LParam("emd", EMDocItem.emd.no_str())
                         << LParam("error", EMDocItem.change_status_error);

           AstraEdifact::ProcEdiError(error, node, isGlobal);

           //записываем в контекст для вывода в журнал операций
           event.lexema_id="EVT.EMD_CHANGE_STATUS_MISTAKE";
           event.prms << PrmSmpl<std::string>("emd", EMDocItem.emd.no_str())
                      << PrmSmpl<std::string>("error", EMDocItem.change_status_error);

           AstraEdifact::ProcEvent(event, EMDCtxt, node, false);
         };

         EMDocItem.toDB(TEMDocItem::ChangeOfStatus);

         AstraEdifact::addToEdiResponseCtxt(req_ctxt_id, node, "emdocs");
      }
    }
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "EmdCosResponseHandler::handle: %s", e.what());
  }
}

}//namespace TlgHandling
