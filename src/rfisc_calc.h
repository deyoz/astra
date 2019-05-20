#ifndef _RFISC_CALC_H_
#define _RFISC_CALC_H_

#include <string>

#include "rfisc.h"
#include "rfisc_sirena.h"
#include "payment_base.h"
#include "emdoc.h"

bool tryEnlargeServicePayment(TPaidRFISCList &paid_bag,
                              CheckIn::TServicePaymentList &payment,
                              const TGrpServiceAutoList &svcsAuto,
                              const TCkinGrpIds &tckin_grp_ids,
                              const CheckIn::TGrpEMDProps &grp_emd_props,
                              const boost::optional< std::list<TEMDCtxtItem> > &confirmed_emd);

bool tryCheckinServicesAuto(TGrpServiceAutoList &svcsAuto,
                            const CheckIn::TServicePaymentList &payment,
                            const TCkinGrpIds &tckin_grp_ids,
                            const CheckIn::TGrpEMDProps &emdProps,
                            const boost::optional< std::list<TEMDCtxtItem> > &confirmed_emd);

class SvcPaymentStatusNotApplicable : public EXCEPTIONS::Exception
{
  public:
    SvcPaymentStatusNotApplicable(const std::string& message)
      :EXCEPTIONS::Exception(message) {}
    SvcPaymentStatusNotApplicable(int grpId)
      :EXCEPTIONS::Exception("SvcPaymentStatus request not applicable for grp_id=%d", grpId) {}
};

typedef std::function<void()> RollbackBeforeRequestFunction;

bool getSvcPaymentStatus(int first_grp_id,
                         const boost::optional< std::list< std::pair<int, TRFISCKey> > >& additionalBaggage,
                         xmlNodePtr reqNode,
                         xmlNodePtr externalSysResNode,
                         const RollbackBeforeRequestFunction& rollbackFunction,
                         const SvcSirenaResponseHandler& resHandler,
                         SirenaExchange::TLastExchangeList &SirenaExchangeList,
                         TCkinGrpIds& tckin_grp_ids,
                         TPaidRFISCList& paid,
                         bool& httpWasSent);

#endif

