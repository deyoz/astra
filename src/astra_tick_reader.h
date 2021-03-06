#ifndef _ASTRA_TICK_READER_H_
#define _ASTRA_TICK_READER_H_

#include <list>
#include "astra_ticket.h"
#include "etick/tick_reader.h"
namespace Ticketing{

namespace TickReader{
    using namespace std;

    class OrigOfRequestReader : public BaseOrigOfRequestReader<OrigOfRequest>
    {
        public:
            virtual ~OrigOfRequestReader(){}
    };

    class ResContrInfoReader : public BaseResContrInfoReader<ResContrInfo>
    {
        public:
            virtual ~ResContrInfoReader(){}
    };

    class PassengerReader : public BasePassengerReader<Passenger>
    {
        public:
            virtual ~PassengerReader(){}
    };

    class FormOfIdReader : public BaseFormOfIdReader<FormOfId>
    {
        public:
            virtual ~FormOfIdReader(){}
    };

    class FrequentPassReader : public BaseFrequentPassReader<FrequentPass>
    {
        public:
            virtual ~FrequentPassReader(){}
    };

    class FreeTextInfoReader : public BaseFreeTextInfoReader<FreeTextInfo>
    {
        public:
            virtual ~FreeTextInfoReader(){}
    };

    class TaxDetailsReader : public BaseTaxDetailsReader<TaxDetails>
    {
        public:
            virtual ~TaxDetailsReader () {}
    };

    class MonetaryInfoReader: public BaseMonetaryInfoReader <MonetaryInfo>
    {
        public:
            virtual ~MonetaryInfoReader () {}
    };

    class FormOfPaymentReader : public BaseFormOfPaymentReader<FormOfPayment>
    {
        public:
            virtual ~FormOfPaymentReader () {}
    };

    class CouponReader : public BaseCouponReader<Coupon, FrequentPass>
    {
        public:
            virtual ~CouponReader() {}
    };

    class TicketReader : public BaseTicketReader<Ticket, CouponReader>
    {
        public:
            virtual ~TicketReader () {}
    };

    class PnrReader : public BasePnrReader<OrigOfRequest,
    ResContrInfo,
    FormOfPayment,
    Ticket,
    Coupon,
    CouponReader,
    Passenger,
    FormOfId,
    FrequentPass,
    FreeTextInfo,
    TaxDetails,
    MonetaryInfo,
    Pnr>
    {
        public:
            virtual ~PnrReader(){};
    };

    class PnrListReader : public BasePnrListReader<OrigOfRequest,
    ResContrInfo,
    FormOfPayment,
    Ticket,
    Coupon,
    CouponReader,
    Passenger,
    FrequentPass,
    MonetaryInfo>
    {
        public:
            virtual ~PnrListReader(){};
    };

}
}
#endif //_ASTRA_TICK_READER_H_
