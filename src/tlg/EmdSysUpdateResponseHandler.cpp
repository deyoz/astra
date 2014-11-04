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

      TEMDocItem EMDocItem;
      xmlNodePtr node=NodeAsNode("/context/emdoc",ediSessCtxt.docPtr())->children;
      EMDocItem.emd_no=NodeAsStringFast("doc_no", node);
      EMDocItem.emd_coupon=NodeAsIntegerFast("coupon_no", node);
      EMDocItem.action=NodeAsIntegerFast("associated", node)!=0?CpnStatAction::associate:
                                                                CpnStatAction::disassociate;
      EMDocItem.et_no=NodeAsStringFast("associated_no", node);
      EMDocItem.et_coupon=NodeAsIntegerFast("associated_coupon", node);

      TLogLocale locale;
      locale.ev_type=ASTRA::evtPay;
      locale.id1=NodeAsIntegerFast("point_id", node);
      locale.id2=NodeAsIntegerFast("reg_no", node);
      locale.id3=NodeAsIntegerFast("grp_id", node);

      using namespace edifact;
      pRemoteResults res = remoteResults();
      if (res==NULL)
        throw EXCEPTIONS::Exception("res==NULL (ediSessId()=%d)", ediSessId().get());      

      if (res->status() == RemoteStatus::CommonError)
      {
        if (res->remark().empty())
          EMDocItem.error="ОШИБКА " + res->ediErrCode();
        else
          EMDocItem.error=res->remark();
      };

      if (res->status() == RemoteStatus::Success) EMDocItem.toDB();

      if (res->status() == RemoteStatus::Success ||
          res->status() == RemoteStatus::CommonError)
      {
        locale.lexema_id = "EVT.PASSENGER_DATA";
        locale.prms << PrmSmpl<string>("pax_name", NodeAsStringFast("pax_full_name",node))
                    << PrmElem<string>("pers_type", etPersType, NodeAsStringFast("pers_type",node));

        PrmLexema lexema("param", res->status() == RemoteStatus::Success?
                           (EMDocItem.action==CpnStatAction::associate?"EVT.EMD_ASSOCIATION":
                                                                       "EVT.EMD_DISASSOCIATION"):
                           (EMDocItem.action==CpnStatAction::associate?"EVT.EMD_ASSOCIATION_MISTAKE":
                                                                       "EVT.EMD_DISASSOCIATION_MISTAKE"));


        lexema.prms << PrmSmpl<std::string>("et_no", EMDocItem.et_no)
                    << PrmSmpl<int>("et_coupon", EMDocItem.et_coupon)
                    << PrmSmpl<std::string>("emd_no", EMDocItem.emd_no)
                    << PrmSmpl<int>("emd_coupon", EMDocItem.emd_coupon);

        if (res->status() == RemoteStatus::CommonError)
          lexema.prms << PrmSmpl<std::string>("err", EMDocItem.error);

        locale.prms << lexema;

        TReqInfo *reqInfo=TReqInfo::Instance();
        reqInfo->LocaleToLog(locale);
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
