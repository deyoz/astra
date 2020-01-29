//
// C++ Interface: tick_view
//
// Description:
//
//
// Author: Roman Kovalev <rom@sirena2000.ru>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _ETICK_TICK_VIEW_H_
#define _ETICK_TICK_VIEW_H_
#include <list>
namespace Ticketing{

class CommonTicketData;

namespace TickView{
using namespace std;
class ViewerData{
public:
    virtual unsigned short viewItem() const = 0;
    virtual bool isSingleTickView() const = 0;
    virtual ~ViewerData() {}
};

class ViewerDataList : public ViewerData
{
public:
    virtual void setViewItemNum(int) = 0;
    virtual ~ViewerDataList(){}
};

template<class OrigOfRequestT>
class BaseOrigOfRequestViewer {
public:
    virtual void operator () (ViewerData &actData, const OrigOfRequestT &) const = 0;
    virtual ~BaseOrigOfRequestViewer(){}
};

template<class ResContrInfoT>
class BaseResContrInfoViewer {
public:
    virtual void operator () (ViewerData &actData, const ResContrInfoT &) const = 0;
    virtual ~BaseResContrInfoViewer(){}
};

class BaseCommonTicketDataViewer {
public:
    virtual void operator () (ViewerData &, const CommonTicketData &) const = 0;
    virtual ~BaseCommonTicketDataViewer(){}
};

class DummyCommonTicketDataViewer : public BaseCommonTicketDataViewer
{
public:
    virtual void operator () (ViewerData &, const CommonTicketData &) const {};
    virtual ~DummyCommonTicketDataViewer(){}
};

template<class PassengerT>
class BasePassengerViewer
{
public:
    virtual void operator () (ViewerData &actData, const PassengerT &) const = 0;
    virtual ~BasePassengerViewer(){}
};

template<class FormOfIdT>
class BaseFormOfIdViewer
{
public:
    virtual void operator () (ViewerData &actData, const std::list<FormOfIdT> &) const = 0;
    virtual ~BaseFormOfIdViewer(){}
};

template<class TaxDetailsT>
class BaseTaxDetailsViewer
{
public:
    virtual void operator () (ViewerData &actData, const list<TaxDetailsT> &) const = 0;
    virtual ~BaseTaxDetailsViewer () {}
};

template<class MonetaryInfoT>
class BaseMonetaryInfoViewer {
public:
    virtual void operator () (ViewerData &actData, const list<MonetaryInfoT> &) const = 0;
    virtual ~BaseMonetaryInfoViewer () {}
};

template<class FormOfPaymentT>
class BaseFormOfPaymentViewer {
public:
    virtual void operator () (ViewerData &actData, const list<FormOfPaymentT> &) const = 0;
    virtual ~BaseFormOfPaymentViewer () {}
};

template<class FreeTextInfoT>
class BaseFreeTextInfoViewer{
public:
    virtual void operator () (ViewerData &actData, const list<FreeTextInfoT> &) const = 0;
    virtual ~BaseFreeTextInfoViewer () {}
};

template<class FrequentPassT>
class BaseFrequentPassViewer {
public:
    virtual void operator () (ViewerData &actData, const list<FrequentPassT> &) const = 0;
    virtual ~BaseFrequentPassViewer () {}
};

template<
        class CouponT,
        class FrequentPassT
                >
class BaseCouponViewer {
public:
    virtual const BaseFrequentPassViewer<FrequentPassT> &freqPassView () const = 0;
    virtual void operator () (ViewerData &actData, const list<CouponT> &) const = 0;
    virtual ~BaseCouponViewer() {}
};

template<class TicketT,
         class CouponViewerT>
class BaseTicketViewer
{
public:
    virtual void operator ()
            (ViewerData &actData,
             const list<TicketT> &,
             const CouponViewerT &) const = 0;
    virtual ~BaseTicketViewer () {}
};

template <
        class OrigOfRequestT,
        class ResContrInfoT,
        class FormOfPaymentT,
        class TicketT,
        class CouponT,
        class CouponViewerT,
        class PassengerT,
        class FrequentPassT,
        class MonetaryInfoT
                >
class CommonPnrViewer
{
public:
    virtual const BaseOrigOfRequestViewer<OrigOfRequestT> &origOfReqView () const = 0;
    virtual const BasePassengerViewer<PassengerT> &passengerView () const = 0;
    virtual const BaseResContrInfoViewer<ResContrInfoT> &resContrInfoView () const = 0;
    virtual const BaseFormOfPaymentViewer<FormOfPaymentT> &formOfPaymentView () const = 0;
    virtual const BaseTicketViewer<TicketT, CouponViewerT> &ticketView() const = 0;
    virtual const BaseCouponViewer<CouponT, FrequentPassT> &couponView() const = 0;
    virtual const BaseMonetaryInfoViewer<MonetaryInfoT> &monetaryInfoView() const = 0;

    virtual ~CommonPnrViewer(){}
};

template <
        class OrigOfRequestT,
        class ResContrInfoT,
        class FormOfPaymentT,
        class TicketT,
        class CouponT,
        class CouponViewerT,
        class PassengerT,
        class FrequentPassT,
        class FreeTextInfoT,
        class TaxDetailsT,
        class MonetaryInfoT
                >
class BasePnrViewer : public CommonPnrViewer<
                            OrigOfRequestT,
                            ResContrInfoT,
                            FormOfPaymentT,
                            TicketT,
                            CouponT,
                            CouponViewerT,
                            PassengerT,
                            FrequentPassT,
                            MonetaryInfoT>
{
/*    typedef BasePnr<
        OrigOfRequestT,
        ResContrInfoT,
        PassengerT,
        TicketT,
        TaxDetailsT,
        MonetaryInfoT,
        FormOfPaymentT,
        FreeTextInfoT> BasePnrType;*/
    DummyCommonTicketDataViewer CTicketDataViewer;
public:
    virtual const BaseTaxDetailsViewer<TaxDetailsT> &taxDetailsView () const = 0;
    virtual const BaseMonetaryInfoViewer<MonetaryInfoT> &monetaryInfoView () const = 0;
    virtual const BaseFreeTextInfoViewer<FreeTextInfoT> &freeTextInfoView () const = 0;
    virtual const BaseCommonTicketDataViewer & commonTicketDataView() const { return CTicketDataViewer; }
    virtual ViewerData &viewData() const = 0;
    virtual ~BasePnrViewer(){};
};


template <
        class OrigOfRequestT,
        class ResContrInfoT,
        class FormOfPaymentT,
        class TicketT,
        class CouponT,
        class CouponViewerT,
        class PassengerT,
        class FrequentPassT,
        class MonetaryInfoT
                >
class BasePnrListViewer : public CommonPnrViewer<
                            OrigOfRequestT,
                            ResContrInfoT,
                            FormOfPaymentT,
                            TicketT,
                            CouponT,
                            CouponViewerT,
                            PassengerT,
                            FrequentPassT,
                            MonetaryInfoT>
{
public:
    virtual ViewerDataList &viewData() const = 0;
    virtual ~BasePnrListViewer(){}
};

}
}

#endif /*_TICK_VIEW_H_*/
