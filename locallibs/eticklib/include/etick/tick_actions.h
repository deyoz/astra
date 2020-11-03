//
// C++ Interface: tick_actions
//
// Description:
//
//
// Author: Roman Kovalev <rom@sirena2000.ru>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _ETICK_TICK_ACTIONS_H_
#define _ETICK_TICK_ACTIONS_H_
#include <list>
#include <etick/ticket.h>
namespace Ticketing{

namespace TickMng{
    class OrigOfRequestAct;
    class ResContrInfoAct;
    class PassengerAct;
    class CommonTicketDataAct;
    class FormOfIdAct;
    class TaxDetailsAct;
    class MonetaryInfoAct;
    class FreeTextInfoAct;
    class FormOfPaymentAct;
    class FrequentPassAct;
    class CouponAct;
    class TicketAct;
    struct PnrAct;
}


namespace TickAction{

class ActionData
{
public:
    virtual const std::string &recloc() const = 0;
    virtual void setRecloc(const std::string &rl) = 0;
    virtual ~ActionData() {}
};


template <class OrigOfRequestT>
class BaseOrigOfRequestAction
{
    friend class TickMng::OrigOfRequestAct;
    virtual void operator () (ActionData &actData, const OrigOfRequestT &) const = 0;
public:
    virtual ~BaseOrigOfRequestAction(){}
};

template <class OrigOfRequestT>
class BaseOrigOfRequestActBlank : public BaseOrigOfRequestAction<OrigOfRequestT>
{
    virtual void operator () (ActionData &, const OrigOfRequestT &) const {}
public:
    virtual ~BaseOrigOfRequestActBlank(){}
};

class BaseCommonTicketDataAction
{
    friend class TickMng::CommonTicketDataAct;
    virtual void operator () (ActionData &actData, const CommonTicketData &) const = 0;
public:
    virtual ~BaseCommonTicketDataAction(){}
};

class BaseCommonTicketDataActionBlank : public BaseCommonTicketDataAction
{
    virtual void operator () (ActionData &, const CommonTicketData &) const {}
public:
    virtual ~BaseCommonTicketDataActionBlank(){}
};


template <class ResContrInfoT>
class BaseResContrInfoAction
{
    friend class TickMng::ResContrInfoAct;
    virtual void operator () (ActionData &actData, const ResContrInfoT &) const = 0;
public:
    virtual ~BaseResContrInfoAction(){}
};

template<class ResContrInfoT>
class BaseResContrInfoActBlank : public BaseResContrInfoAction <ResContrInfoT>
{
    virtual void operator () (ActionData &, const ResContrInfoT &) const {}
public:
    virtual ~BaseResContrInfoActBlank(){}
};


template <class PassengerT>
class BasePassengerAction
{
    friend class TickMng::PassengerAct;
    virtual void operator () (ActionData &actData, const PassengerT &) const = 0;
public:
    virtual ~BasePassengerAction(){}
};

template<class PassengerT>
class BasePassengerActBlank : public BasePassengerAction<PassengerT>
{
    virtual void operator () (ActionData &, const PassengerT &) const {}
public:
    virtual ~BasePassengerActBlank(){}
};

template <class FormOfIdT>
class BaseFormOfIdAction
{
    friend class TickMng::FormOfIdAct;
    virtual void operator () (ActionData &actData, const std::list<FormOfIdT> &) const = 0;
public:
    virtual ~BaseFormOfIdAction(){}
};

template<class FormOfIdT>
class BaseFormOfIdActBlank : public BaseFormOfIdAction<FormOfIdT>
{
    virtual void operator () (ActionData &, const std::list<FormOfIdT> &) const {}
public:
    virtual ~BaseFormOfIdActBlank(){}
};

template<class TaxDetailsT>
class BaseTaxDetailsAction
{
    friend class TickMng::TaxDetailsAct;

    virtual void operator () (ActionData &, const std::list<TaxDetailsT> &) const = 0;
public:
    virtual ~BaseTaxDetailsAction () {}
};

template<class TaxDetailsT>
class BaseTaxDetailsActBlank : public BaseTaxDetailsAction<TaxDetailsT>
{
    virtual void operator () (ActionData &, const std::list<TaxDetailsT> &) const {}
public:
    virtual ~BaseTaxDetailsActBlank () {}
};

template<class MonetaryInfoT>
class BaseMonetaryInfoAction
{
    friend class TickMng::MonetaryInfoAct;
    virtual void operator () (ActionData &, const std::list<MonetaryInfoT> &) const = 0;
public:
    virtual ~BaseMonetaryInfoAction () {}
};

template<class MonetaryInfoT>
class BaseMonetaryInfoActBlank : public BaseMonetaryInfoAction<MonetaryInfoT>
{
    virtual void operator () (ActionData &, const std::list<MonetaryInfoT> &) const {}
public:
    virtual ~BaseMonetaryInfoActBlank () {}
};


template<class FormOfPaymentT>
class BaseFormOfPaymentAction
{
    friend class TickMng::FormOfPaymentAct;
    virtual void operator () (ActionData &actData, const std::list<FormOfPaymentT> &) const = 0;
public:
    virtual ~BaseFormOfPaymentAction () {}
};

template<class FormOfPaymentT>
class BaseFormOfPaymentActBlank : public BaseFormOfPaymentAction<FormOfPaymentT>
{
    virtual void operator () (ActionData &, const std::list<FormOfPaymentT> &) const {}
public:
    virtual ~BaseFormOfPaymentActBlank () {}
};


template<class FreeTextInfoT>
class BaseFreeTextInfoAction
{
    friend class TickMng::FreeTextInfoAct;
    virtual void operator () (ActionData &actData, const std::list<FreeTextInfoT> &) const = 0;
public:
    virtual ~BaseFreeTextInfoAction () {}
};

template<class FreeTextInfoT>
class BaseFreeTextInfoActBlank : public BaseFreeTextInfoAction<FreeTextInfoT>
{
    virtual void operator () (ActionData &, const std::list<FreeTextInfoT> &) const {}
public:
    virtual ~BaseFreeTextInfoActBlank () {}
};


template<class FrequentPassT>
class BaseFrequentPassAction
{
    friend class TickMng::FrequentPassAct;
    virtual void operator () (ActionData &actData, const std::list<FrequentPassT> &) const = 0;
public:
    virtual ~BaseFrequentPassAction(){}
};

template<class FrequentPassT>
class BaseFrequentPassActBlank : public BaseFrequentPassAction<FrequentPassT>
{
    virtual void operator () (ActionData &, const std::list<FrequentPassT> &) const {}
public:
    virtual ~BaseFrequentPassActBlank(){}
};

template<
        class CouponT,
        class Coupon_infoT,
        class ItinT,
        class FrequentPassActT>
class BaseCouponAction
{
    friend class TickMng::CouponAct;
    virtual void operator () (ActionData &actData,
                                const Coupon_infoT &ci,
                                const ItinT * const itin) const = 0;

    virtual const FrequentPassActT &frequentPassAct () const = 0;
public:
    virtual ~BaseCouponAction() {}
};


template<
        class CouponT,
        class Coupon_infoT,
        class ItinT,
        class FrequentPassActT
        >
class BaseCouponActBlank : public BaseCouponAction<
                                            CouponT,
                                            Coupon_infoT,
                                            ItinT,
                                            FrequentPassActT
                                            >
{
    FrequentPassActT freqAct;
    virtual void operator () (ActionData &,
                                const Coupon_infoT &,
                                const ItinT * const) const
    {
    }

    virtual const FrequentPassActT &frequentPassAct () const
    {
        return freqAct;
    }
public:
    virtual ~BaseCouponActBlank() {}
};

template <
        class TicketT,
        class CouponActT>
class BaseTicketAction
{
    friend class TickMng::TicketAct;
    virtual const CouponActT &couponAct() const = 0;
    virtual void operator () (ActionData &actData, const TicketT &) const = 0;
public:
    virtual ~BaseTicketAction () {}
};

template<class TicketT,
         class CouponActBlankT>
class BaseTicketActBlank : public BaseTicketAction
                                <TicketT,
                                 CouponActBlankT>
{
    CouponActBlankT CpnAct;
    virtual const CouponActBlankT
            &couponAct() const { return CpnAct; }
    virtual void operator () (ActionData &, const TicketT &) const {}
public:
    virtual ~BaseTicketActBlank () {}
};

template <
        class PnrT,
        class ResContrInfoActT,
        class OrigOfRequestActT,
        class PassengerActT,
        class FormOfIdActT,
        class TaxDetailsActT,
        class MonetaryInfoActT,
        class FormOfPaymentActT,
        class FreeTextInfoActT,
        class TicketActT,
        class CouponActT,
        class FrequentPassActT,
        class CommonTicketDataActT>
class BasePnrAction
{
    friend struct Ticketing::TickMng::PnrAct;

    virtual const ResContrInfoActT
            &resContrInfoAct    () const = 0;

    virtual const CommonTicketDataActT
            &commonTicketDataAct() const = 0;

    virtual const OrigOfRequestActT
            &origOfReqAct       () const = 0;

    virtual const PassengerActT
            &passengerAct       () const = 0;

    virtual const FormOfIdActT
            &formOfIdAct        () const = 0;

    virtual const TaxDetailsActT
            &taxDetailsAct      () const = 0;

    virtual const MonetaryInfoActT
            &monetaryInfoAct    () const = 0;

    virtual const FormOfPaymentActT
            &formOfPaymentAct   () const = 0;

    virtual const FreeTextInfoActT
            &freeTextInfoAct    () const = 0;

    virtual const TicketActT
            &ticketAct          () const = 0;

    virtual ActionData &actData() const = 0;

    virtual void operator () (PnrT &pnr) const = 0;

    virtual void after (PnrT &) const {}
public:
    virtual ~BasePnrAction(){};
};

}
}
#endif //_ETICK_TICK_ACTIONS_H_
