#ifndef _ASTRA_TICK_READ_EDI_H_
#define _ASTRA_TICK_READ_EDI_H_

#include "astra_tick_reader.h"
#include "tlg/edi_tlg.h"

#include <edilib/edi_func_cpp.h>

#include <string>

namespace Ticketing{
namespace TickReader{

    class REdiData : public ReaderData
    {
        EDI_REAL_MES_STRUCT *pMes;
        pair<string, TickStatAction::TickStatAction_t> CurrTicket;
        edifact::EdiMessageType MesType;
        unsigned CurrCoupon;
        unsigned PassSegGr;
        unsigned TvlSegGr;
    public:
        REdiData(EDI_REAL_MES_STRUCT *mes, edifact::EdiMessageType type);

        EDI_REAL_MES_STRUCT* EdiMes() const { return pMes; }

        void setTickNum(const pair<string, TickStatAction::TickStatAction_t> &tick){
            CurrTicket = tick;
        }
        void setCurrCoupon(unsigned num) {
            CurrCoupon=num;
        }
        const std::pair<string, TickStatAction::TickStatAction_t> &currTicket() const { return CurrTicket; }

        void resetTickCpn();

        unsigned currCoupon() const { return CurrCoupon; }
        unsigned passSegGrNum() const { return PassSegGr; }
        unsigned tvlSegGrNum() const { return TvlSegGr; }

        virtual unsigned short viewItem() const { return 0; }
        virtual bool isSingleTickView() const { return true; }
    };

    class REdiDataPack : public ReaderDataPack, public REdiData
    {
    public:
        REdiDataPack(EDI_REAL_MES_STRUCT *mes, edifact::EdiMessageType type);

        virtual unsigned countExpectedTickets() const;
        virtual bool isPackTickRead() const { return true; }

        virtual void incCurrentItem();
        virtual unsigned currentItem() const { return ReaderDataPack::currentItem(); }
    };

    class ResContrInfoEdiR : public ResContrInfoReader
    {
        public:
            virtual ResContrInfo operator() (ReaderData &) const;
            virtual ~ResContrInfoEdiR(){}
    };

    class OrigOfRequestEdiR : public OrigOfRequestReader
    {
        public :
            virtual OrigOfRequest operator ( )(ReaderData &) const;
            virtual ~OrigOfRequestEdiR(){}
    };

    class PassengerEdiR : public PassengerReader
    {
        public:
            virtual Passenger::SharedPtr operator () (ReaderData &) const;
            virtual ~PassengerEdiR () {}
    };

    class FormOfIdEdiR : public FormOfIdReader
    {
    public:
        virtual void operator () (ReaderData &, std::list<FormOfId> &lfoid) const;
        virtual ~FormOfIdEdiR(){}
    };

    class TaxDetailsEdiR : public TaxDetailsReader {
        public:
            virtual void operator () (ReaderData &, list<TaxDetails> &) const;
            virtual ~TaxDetailsEdiR () {}
    };

    class MonetaryInfoEdiR : public MonetaryInfoReader {
        public:
            virtual void operator () (ReaderData &, list<MonetaryInfo> &) const;
            virtual ~MonetaryInfoEdiR () {}
    };

    class FormOfPaymentEdiR : public FormOfPaymentReader {
        public:
            virtual void operator () (ReaderData &, list<FormOfPayment> &) const;
            virtual ~FormOfPaymentEdiR () {}
    };

    class FrequentPassEdiR : public FrequentPassReader
    {
        public:
            virtual void operator () (ReaderData &, list<FrequentPass> &) const;
            virtual ~FrequentPassEdiR(){}
    };

    class FreeTextInfoEdiR : public FreeTextInfoReader
    {
        public:
            virtual void operator () (ReaderData &, list<FreeTextInfo> &) const;
            virtual ~FreeTextInfoEdiR(){}
    };

    class CouponEdiR : public CouponReader {
        FrequentPassEdiR FreqPassEdiR;
        public:
            virtual const FrequentPassEdiR &frequentPassRead () const { return FreqPassEdiR; }
            virtual void operator () (ReaderData &, list<Coupon> &) const;
            virtual ~CouponEdiR() {}
    };

    class TicketEdiR : public TicketReader
    {
        CouponEdiR CpnEdiR;
        public:
            virtual const CouponReader &couponReader() const
            {
                return CpnEdiR;
            }
            virtual void operator () (ReaderData &actData,
            list<Ticket> &,
            const CouponReader &) const;
            virtual ~TicketEdiR () {}
    };

    class PnrEdiRead : public PnrReader
    {
        ResContrInfoEdiR  ResContrEdiR;
        TaxDetailsEdiR TaxEdiR;
        MonetaryInfoEdiR MonEdiR;
        FreeTextInfoEdiR IftEdiR;
        OrigOfRequestEdiR OrigOfReqEdiR;
        TicketEdiR TickEdiR;
        CouponEdiR CoupEdiR;
        FormOfPaymentEdiR FopEdiR;
        PassengerEdiR PassEdiR;
        FormOfIdEdiR  FoidEdiR;

        mutable REdiData ReadData;
        public:
            PnrEdiRead(EDI_REAL_MES_STRUCT *mes, edifact::EdiMessageType type)
                : ReadData(mes, type)
            {}
            virtual const OrigOfRequestEdiR & origOfReqRead () const { return OrigOfReqEdiR; }
            virtual const TicketEdiR &ticketRead () const { return TickEdiR; }
            virtual const CouponEdiR &couponRead () const { return CoupEdiR; }
            virtual const PassengerEdiR & passengerRead () const { return PassEdiR; }
            virtual const FormOfIdEdiR  & formOfIdRead() const { return FoidEdiR; }
            virtual const FormOfPaymentEdiR &formOfPaymentRead () const { return FopEdiR; }

            virtual const ResContrInfoEdiR & resContrInfoRead () const { return ResContrEdiR; }
            virtual const TaxDetailsEdiR & taxDetailsRead () const { return TaxEdiR; }
            virtual const MonetaryInfoEdiR &monetaryInfoRead () const { return MonEdiR; }
            virtual const FreeTextInfoEdiR &freeTextInfoRead () const { return IftEdiR; }

            virtual REdiData &readData () const { return ReadData; }
            virtual ~PnrEdiRead() {}
    };

    // read IFT from EDIFACT
    void readEdiIFT(EDI_REAL_MES_STRUCT *pMes, list<FreeTextInfo> &lIft);
    Coupon_info MakeCouponInfo(EDI_REAL_MES_STRUCT *pMes, TickStatAction::TickStatAction_t tickStatAction);

    // read ORG from EDIFACT
    OrigOfRequest readOrigOfRequest(EDI_REAL_MES_STRUCT* pMes);

    // read RCI from EDIFACT
    ResContrInfo readResContrInfo(EDI_REAL_MES_STRUCT* pMes);

    // read TIF from EDIFACT
    Passenger readPassenger(EDI_REAL_MES_STRUCT* pMes);

} // namespace Ticketing
} // namespace TickReader
#endif /*_ASTRA_TICK_READ_EDI_H_*/
