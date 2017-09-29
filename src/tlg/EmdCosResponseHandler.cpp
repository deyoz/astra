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
      EMDocItem.emd_no=EMDCtxt.emd_no;
      EMDocItem.emd_coupon=EMDCtxt.emd_coupon;
      EMDocItem.et_no=EMDCtxt.pax.tkn.no;
      EMDocItem.et_coupon=EMDCtxt.pax.tkn.coupon;
      EMDocItem.action=EMDCtxt.action;
      EMDocItem.point_id=EMDCtxt.point_id;

      using namespace edifact;
      pRemoteResults res = remoteResults();
      if (res==NULL)
        throw EXCEPTIONS::Exception("EmdCosResponseHandler::handle: res==NULL (ediSessId()=%d)", ediSessId().get());


      if (res->status() == RemoteStatus::Success)
        EMDocItem.status=EMDCtxt.status;

      if (res->status() == RemoteStatus::CommonError)
      {
        if (res->remark().empty())
          EMDocItem.change_status_error="������ " + res->ediErrCode();
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
                     EMDocItem.emd_no.c_str(), EMDocItem.emd_coupon, EMDocItem.status->dispCode());

           event.lexema_id="EVT.EMD_CHANGE_STATUS";
           event.prms << PrmSmpl<std::string>("doc_no", EMDocItem.emd_no)
                      << PrmSmpl<int>("coupon_no", EMDocItem.emd_coupon)
                      << PrmSmpl<std::string>("coupon_status", EMDocItem.status->dispCode());
/*         ����� �ࠢ������ � �।��騬 ����ᮬ ⠪ ��� �� ����஫��㥬 ᬥ�� ����� EMD ����� � ᬥ��� ����� ��
           TEMDocItem prior;
           prior.fromDB(EMDocItem.emd_no, EMDocItem.emd_coupon, true);
           bool repeated=!prior.empty() && prior.status==EMDocItem.status;
*/
           bool repeated=false;
           ProcEdiEvent(event, EMDCtxt, node, repeated);
         };

         if (res->status() == RemoteStatus::CommonError)
         {
           ProgTrace(TRACE5, "emd_no=%s emd_coupon=%d error=%s",
                     EMDocItem.emd_no.c_str(), EMDocItem.emd_coupon, EMDocItem.change_status_error.c_str());

           //�����뢠�� � ���⥪�� ��� ��᫥���饣� �뢮�� �� �࠭ �ନ����
           using namespace AstraLocale;
           bool isGlobal=false; //!!!��⮬ ���稬�� ࠧ�����
           LexemaData error;
           error.lexema_id="MSG.EMD_CHANGE_STATUS_ERROR";
           error.lparams << LParam("emd", EMDocItem.emd_no_str())
                         << LParam("error", EMDocItem.change_status_error);

           AstraEdifact::ProcEdiError(error, node, isGlobal);

           //�����뢠�� � ���⥪�� ��� �뢮�� � ��ୠ� ����権
           event.lexema_id="EVT.EMD_CHANGE_STATUS_MISTAKE";
           event.prms << PrmSmpl<std::string>("emd", EMDocItem.emd_no_str())
                      << PrmSmpl<std::string>("error", EMDocItem.change_status_error);

           ProcEdiEvent(event, EMDCtxt, node, false);
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
