#ifndef _ASTRA_TICK_VIEW_H_
#define _ASTRA_TICK_VIEW_H_

#include "astra_ticket.h"
#include "etick/tick_view.h"
namespace Ticketing{

namespace TickView{

class OrigOfRequestViewer: public BaseOrigOfRequestViewer<OrigOfRequest>
{
public:
    virtual ~OrigOfRequestViewer(){}
};

class ResContrInfoViewer : public BaseResContrInfoViewer<ResContrInfo>
{
public:
    virtual ~ResContrInfoViewer(){}
};

class PassengerViewer : public BasePassengerViewer<Passenger>
{
public:
    virtual ~PassengerViewer(){}
};

class TaxDetailsViewer : public BaseTaxDetailsViewer<TaxDetails>
{
public:
    virtual ~TaxDetailsViewer () {}
};

class MonetaryInfoViewer : public BaseMonetaryInfoViewer<MonetaryInfo>
{
public:
    virtual ~MonetaryInfoViewer () {}
};

class FormOfPaymentViewer : public BaseFormOfPaymentViewer<FormOfPayment>
{
public:
    virtual ~FormOfPaymentViewer () {}
};

class FreeTextInfoViewer : public BaseFreeTextInfoViewer<FreeTextInfo>
{
public:
        virtual ~FreeTextInfoViewer () {}
};

class FrequentPassViewer : public BaseFrequentPassViewer<FrequentPass>
{
public:
    virtual ~FrequentPassViewer () {}
};

class CouponViewer : public BaseCouponViewer<Coupon, FrequentPass>
{
    public:
        virtual ~CouponViewer() {}
};

class TicketViewer : public BaseTicketViewer<Ticket, CouponViewer>
{
public:
    virtual ~TicketViewer () {}
};

class PnrViewer : public BasePnrViewer<OrigOfRequest,
                                         ResContrInfo,
                                         FormOfPayment,
                                         Ticket,
                                         Coupon,
                                         CouponViewer,
                                         Passenger,
                                         FrequentPass,
                                         FreeTextInfo,
                                         TaxDetails,
                                         MonetaryInfo>
{
public:
    virtual ~PnrViewer(){};
};

class PnrListViewer : public BasePnrListViewer<OrigOfRequest,
                                         ResContrInfo,
                                         FormOfPayment,
                                         Ticket,
                                         Coupon,
                                         CouponViewer,
                                         Passenger,
                                         FrequentPass>
{
    public:
        virtual ViewerDataList &viewData() const = 0;
        virtual ~PnrListViewer(){}
};

}
}
#endif /*_ASTRA_TICK_VIEW_H_*/
