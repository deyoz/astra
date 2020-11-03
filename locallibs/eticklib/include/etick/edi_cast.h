/*	2006 by Roman Kovalev 	*/
/*	roman@pike.dev.sirena2000.ru		*/
#ifndef _ETICK_EDI_CAST_H_
#define _ETICK_EDI_CAST_H_

#include "etick/tick_data.h"
#include "etick/emd_data.h"
#include "etick_msg.h"
#include "etick/ticket.h"
#include "etick/tick_doctype.h"
#include "edilib/edi_func_cpp.h"

namespace Ticketing{
namespace EdiCast {
    using namespace edilib;
    using namespace std;

    class CoupStatCast : public BaseEdiCast <CouponStatus>
    {
        ErrMsg_t Err;
        public:
            CoupStatCast (const ErrMsg_t &err):Err(err){ }
            virtual CouponStatus operator () (const BaseFDE &de);
    };

    class ItinStatCast : public BaseEdiCast <ItinStatus>
    {
        ErrMsg_t Err;
        bool MaybyNumSeat;
        public:
            ItinStatCast(const ErrMsg_t &err, bool mayby_numseat=false)
            :Err(err), MaybyNumSeat(mayby_numseat)
            {
            }
            virtual ItinStatus operator () (const BaseFDE &de);
    };

class DateCast : public BaseEdiCast <boost::gregorian::date>
{
    const char *DateForm;
    ErrMsg_t Err;
    public:
        DateCast (const char *df, const ErrMsg_t &err=EtErr::INV_DATE)
    : DateForm(df),Err(err) { }
        virtual boost::gregorian::date operator () (const BaseFDE &de);
};

class TimeCast : public BaseEdiCast <boost::posix_time::time_duration>
{
    const char *TimeForm;
    ErrMsg_t Err;
    public:
        TimeCast (const char *df, const ErrMsg_t &err=EtErr::INV_TIME)
    : TimeForm(df),Err(err) { }
        virtual boost::posix_time::time_duration operator () (const BaseFDE &de);
};

class RBDCast : public BaseEdiCast <SubClass>
{
    ErrMsg_t Err;
    public:
        RBDCast(const ErrMsg_t &err=EtErr::INV_RBD): Err(err) { }
        virtual SubClass operator () (const BaseFDE &de);
};

class TickActCast : public BaseEdiCast <TickStatAction::TickStatAction_t>
{
    ErrMsg_t Err;
    public:
        TickActCast(const ErrMsg_t & err): Err(err) { }
        virtual TickStatAction::TickStatAction_t operator () (const BaseFDE &de);
};

class CpnActCast : public BaseEdiCast <CpnStatAction::CpnStatAction_t>
{
    ErrMsg_t Err;
    public:
        CpnActCast(const ErrMsg_t & err): Err(err) { }
        virtual CpnStatAction::CpnStatAction_t operator () (const BaseFDE &de);
};

TaxAmount::Amount amountFromString(const std::string &amount,
                                   TaxAmount::Amount::AmountType_e Type = TaxAmount::Amount::Ordinary);

class AmountCast : public edilib::BaseEdiCast <TaxAmount::Amount>
{
    ErrMsg_t Err;
    TaxAmount::Amount::AmountType_e Type;
    public:
        AmountCast(const ErrMsg_t & err,
                   TaxAmount::Amount::AmountType_e type = TaxAmount::Amount::Ordinary)
            : Err(err), Type(type)
        {
        }
        virtual TaxAmount::Amount operator () (const BaseFDE &de);
};

///@class FopIndicatorCast
///@brief Get type from edifact string
class FopIndicatorCast : public edilib::BaseEdiCast <FopIndicator>
{
    ErrMsg_t Err;
    FopIndicator::FopIndicator_t defVal;
public:
    FopIndicatorCast(const ErrMsg_t & err, FopIndicator::FopIndicator_t defInd)
    : Err(err), defVal(defInd)
    {
    }
    virtual FopIndicator operator () (const BaseFDE &de);
};

class PassTypeCast : public BaseEdiCast <PassengerType>
{
    ErrMsg_t Err;
    public:
        PassTypeCast(const ErrMsg_t & err): Err(err) { }
        virtual PassengerType operator () (const BaseFDE &de);
};

class AmountCodeCast : public BaseEdiCast <AmountCode>
{
    ErrMsg_t Err;
    public:
        AmountCodeCast(const ErrMsg_t &err): Err(err) { }
        virtual AmountCode operator () (const BaseFDE &de);
};

class FoidTypeCast : public BaseEdiCast <FoidType>
{
    ErrMsg_t Err;
    public:
        FoidTypeCast(const ErrMsg_t &err): Err(err) { }
        virtual FoidType operator () (const BaseFDE &de);
};

///@class TicketMediaCast
///@brief Тип носителя билета
class TicketMediaCast : public BaseEdiCast <TicketMedia>
{
    ErrMsg_t Err;
    public:
        TicketMediaCast(const ErrMsg_t &err): Err(err) { }
        virtual TicketMedia operator () (const BaseFDE &de);
};

class TaxCategoryCast : public BaseEdiCast <TaxCategory>
{
    ErrMsg_t Err;
    TaxCategory::TaxCategory_t defVal;
    public:
        TaxCategoryCast(const ErrMsg_t &err, TaxCategory::TaxCategory_t def):
            Err(err),defVal(def)
        {
        }
        virtual TaxCategory operator () (const BaseFDE &de);
};

class DocTypeCast : public BaseEdiCast <DocType>
{
    ErrMsg_t Err;
public:
    DocTypeCast(const ErrMsg_t &err) : Err(err) {}
    DocType operator () (const BaseFDE &de);
};

class RficTypeCast : public BaseEdiCast <RficType>
{
    ErrMsg_t Err;
public:
    RficTypeCast(const ErrMsg_t &err) : Err(err) {}
    RficType operator () (const BaseFDE &de);
};


///@class FopIndicatorCast
///@brief Get type from edifact string
class PricingTicketingIndicatorCast : public edilib::BaseEdiCast <PricingTicketingIndicator>
{
    ErrMsg_t Err;
public:
    PricingTicketingIndicatorCast(const ErrMsg_t & err)
    : Err(err)
    {
    }
    virtual PricingTicketingIndicator operator () (const BaseFDE &de);
};



///@class PricingTicketingSellTypeCast
///@brief Get type from edifact string
class PricingTicketingSellTypeCast : public edilib::BaseEdiCast <PricingTicketingSellType>
{
    ErrMsg_t Err;
public:
    PricingTicketingSellTypeCast(const ErrMsg_t & err)
    : Err(err)
    {
    }
    virtual PricingTicketingSellType operator () (const BaseFDE &de);
};

} // EdiCast
} //namespace Ticketing
#endif /*_ETICK_EDI_CAST_H_*/
