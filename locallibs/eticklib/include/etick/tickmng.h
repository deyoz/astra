//
// C++ Interface: tickmsg
//
// Description: Просмотр/Чтение/Операции над ЭБ.
//
//
// Author: Roman Kovalev <rom@sirena2000.ru>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef _ETICK_TICKMNG_H_
#define _ETICK_TICKMNG_H_
#include "etick/exceptions.h"
#include "etick/tick_actions.h"
#include "etick/tick_reader.h"
#include "etick/tick_view.h"

namespace Ticketing{
/**
 * Операции/Просмотр/Чтение ЭБ
*/
namespace TickMng{

using namespace TickReader;

struct FrequentPassAct
{
    template <class FreqT>
    static void doAction(const TickAction::BaseFrequentPassAction<FreqT> &act,
                         TickAction::ActionData &ad,
                         const std::list<FreqT> &lFreq)
    {
        act(ad, lFreq);
    }
};

struct FrequentPassRdr
{
    template <class FreqT>
    static void doRead(const TickReader::BaseFrequentPassReader<FreqT> &reader,
                       TickReader::ReaderData &rd,
                       std::list<FreqT> &lFreq)
    {
        reader(rd, lFreq);
    }
};

struct FreeTextInfoAct
{
    template <class FreeTxtT>
    static void doAction(const TickAction::BaseFreeTextInfoAction<FreeTxtT> &act,
                         TickAction::ActionData &ad,
                         const std::list<FreeTxtT> &lIft)
    {
        act(ad, lIft);
    }
};

struct FreeTextInfoDisp
{
    template<class FreeTxtT>
    static void doDisplay(const TickView::BaseFreeTextInfoViewer<FreeTxtT> &viewer,
                          TickView::ViewerData &vd,
                          const std::list<FreeTxtT> &lIft)
    {
        viewer(vd, lIft);
    }
};

struct FreeTextInfoRdr
{
    template<class FreeTxtT>
    static void doRead(const TickReader::BaseFreeTextInfoReader<FreeTxtT> &reader,
                       TickReader::ReaderData &rd,
                       std::list<FreeTxtT> &lIft)
    {
        reader(rd, lIft);
    }
};


//========== COUPON ===========
struct CouponAct
{
    template<class CouponActT,
            class CouponT>
    static void doAction(const CouponActT &act,
                         TickAction::ActionData &ad,
                         const CouponT &cpn)
    {
        act(ad, cpn.couponInfo(), cpn.haveItin()?&cpn.itin():NULL);
        if(cpn.lfreqPass().size())
            FrequentPassAct::doAction(act.frequentPassAct(), ad, cpn.lfreqPass());
    }
};

//===========PASSENGER==========

struct PassengerAct
{
    template<class PassengerT>
    static void doAction(const TickAction::BasePassengerAction<PassengerT> &act,
                         TickAction::ActionData &ad,
                         const PassengerT &pass)
    {
        act(ad, pass);
    }
};

struct PassengerRdr
{
    template<class PassengerT,
             class PassengerReaderT>
    static typename PassengerT::SharedPtr
        doRead(const PassengerReaderT &reader,
                   TickReader::ReaderData &rd)
            {
                return reader(rd);
            }
};

//============FOID===================

struct FormOfIdAct
{
    template<class FoidT>
    static void doAction(const TickAction::BaseFormOfIdAction<FoidT> &act,
                         TickAction::ActionData &ad,
                         const std::list<FoidT> &foid)
    {
        act(ad, foid);
    }
};

struct FormOfIdRdr
{
    template<class FoidT,
             class FormOfIdReaderT>
    void doRead(const FormOfIdReaderT &reader,
                TickReader::ReaderData &rd,
                std::list<FoidT> &lfoid)
    {
        return reader(rd, lfoid);
    }
};

//============FormOfPayment==========
struct FormOfPaymentRdr
{
    template<class FormOfPaymentT>
    static void doRead(const BaseFormOfPaymentReader<FormOfPaymentT> &reader,
                       TickReader::ReaderData &rd,
                       std::list<FormOfPaymentT> &lFop)
    {
        reader(rd, lFop);
    }
};

struct FormOfPaymentAct
{
    template<class FormOfPaymentT>
            static void doAction(const TickAction::BaseFormOfPaymentAction<FormOfPaymentT> &act,
                         TickAction::ActionData &ad,
                         const std::list<FormOfPaymentT> &lFop)
    {
        act(ad, lFop);
    }
};

struct FormOfPaymentDisp
{
    template< class FormOfPaymentT>
            static void doDisplay(const TickView::BaseFormOfPaymentViewer<FormOfPaymentT> &viewer,
                          TickView::ViewerData &vd,
                          const std::list<FormOfPaymentT> &lFop)
    {
        viewer(vd, lFop);
    }
};

struct TicketAct
{
    template <
            class TickActionT,
            class TicketT>
    static void doAction(const TickActionT &act,
                         TickAction::ActionData &ad,
                         const std::list<TicketT> &lTick)
    {
        for(typename std::list<TicketT>::const_iterator i = lTick.begin(); i != lTick.end() ; i++)
        {
            act(ad, *i);
            for(typename std::list<typename TicketT::CouponType>::const_iterator j=(*i).getCoupon().begin();
                j!=(*i).getCoupon().end();
                j++)
            {
                CouponAct::doAction(act.couponAct(), ad, (*j));
            }
        }
    }
};

struct TicketRdr
{
    template <class TickReaderT, class TicketT>
    static void doRead (const TickReaderT &reader,
                        TickReader::ReaderData &rd,
                        std::list<TicketT> &lTick)
    {
        reader(rd, lTick, reader.couponReader());
    }

    template <class TicketT, class TickReaderT>
    static std::list<TicketT> doRead (const TickReaderT &reader,
                                 TickReader::ReaderData &rd)
    {
        std::list<TicketT> lTick;
        reader(rd, lTick, reader.couponReader());
        return lTick;
    }
};

struct TaxDetailsAct
{
    template<class TaxDetailsT>
    static void doAction(const TickAction::BaseTaxDetailsAction<TaxDetailsT> &act,
                         TickAction::ActionData &ad,
                         const std::list<TaxDetailsT> &lTax)
    {
        act(ad, lTax);
    }
};

struct MonetaryInfoAct
{
    template<class MonetaryInfoT>
    static void doAction(const TickAction::BaseMonetaryInfoAction<MonetaryInfoT> &act,
                         TickAction::ActionData &ad,
                         const std::list<MonetaryInfoT> &lMon)
    {
        act(ad, lMon);
    }
};

struct ResContrInfoAct
{
    template <class ResContrInfoT>
    static void doAction(const TickAction::BaseResContrInfoAction<ResContrInfoT> &act,
                         TickAction::ActionData &ad,
                         const ResContrInfoT &rci)
    {
        act(ad, rci);
    }
};

struct CommonTicketDataAct
{
    static void doAction(const TickAction::BaseCommonTicketDataAction &act,
                         TickAction::ActionData &ad,
                         const CommonTicketData &ctd)
    {
        act(ad, ctd);
    }
};

struct OrigOfRequestAct
{
    template <class OrigOfRequestT>
            static void doAction(const TickAction::BaseOrigOfRequestAction<OrigOfRequestT> &orgAct,
                         TickAction::ActionData &ad,
                         const OrigOfRequestT &org)
    {
        orgAct(ad, org);
    }
};
struct OrigOfRequestRdr
{
    template <class OrigOfRequestT>
    static OrigOfRequestT doRead(
            const TickReader::BaseOrigOfRequestReader<OrigOfRequestT> &reader,
            TickReader::ReaderData &rd)
    {
        return reader(rd);
    }
};

struct PnrAct
{
    template<class PnrActT, class PnrT>
    static void doAction(const PnrActT &act, PnrT &pnr)
    {
        act (pnr);
        TickAction::ActionData &ad = act.actData();

        ResContrInfoAct::doAction (act.resContrInfoAct(), ad, pnr.rci());
        OrigOfRequestAct::doAction(act.origOfReqAct(),    ad, pnr.org());
        FreeTextInfoAct::doAction (act.freeTextInfoAct(), ad, pnr.lift());
        CommonTicketDataAct::doAction (act.commonTicketDataAct(), ad, pnr.cTicketData());
        if(pnr.havePass())
            PassengerAct::doAction(act.passengerAct(),    ad, pnr.pass());

        FormOfIdAct::doAction     (act.formOfIdAct(),     ad, pnr.lfoid());
        TaxDetailsAct::doAction   (act.taxDetailsAct(),   ad, pnr.ltax());
        MonetaryInfoAct::doAction (act.monetaryInfoAct(), ad, pnr.lmon());
        FormOfPaymentAct::doAction(act.formOfPaymentAct(),ad, pnr.lfop());
        TicketAct::doAction       (act.ticketAct(),       ad, pnr.ltick());
        act.after(pnr);
    }

};

struct PnrDisp
{
    template<class PnrViewerT,
             class PnrT>
    static void doDisplay(const PnrViewerT &Viewer,
                   const PnrT &pnr)
    {
        TickView::ViewerData &vd = Viewer.viewData();

//         Viewer(pnr);
        if(pnr.havePass()){
            Viewer.passengerView()(vd, pnr.pass());
        } else {
            //ProgTrace(TRACE2, "There are no passengers");
        }
        Viewer.resContrInfoView()(vd, pnr.rci());
        Viewer.origOfReqView()(vd, pnr.org());
        Viewer.commonTicketDataView()(vd, pnr.cTicketData());
        Viewer.formOfIdView()(vd, pnr.lfoid());
        Viewer.monetaryInfoView()(vd, pnr.lmon());
        Viewer.formOfPaymentView()(vd, pnr.lfop());
        Viewer.taxDetailsView()(vd, pnr.ltax());
        Viewer.freeTextInfoView()(vd, pnr.lift());
        Viewer.ticketView()(vd, pnr.ltick(), Viewer.couponView());
    }
};

struct PnrRdr
{
    template <class PnrT, class PnrReaderT>
    static PnrT doRead( const PnrReaderT &reader)
    {
        TickReader::ReaderData &rd = reader.readData();

        std::list <typename PnrT::FormOfPaymentType> lFop;
        std::list <typename PnrT::TaxDetailsType> lTax;
        std::list <typename PnrT::MonetaryInfoType> lMon;
        std::list <typename PnrT::FreeTextInfoType> lIft;
        std::list <typename PnrT::TicketType> lTicket;
        std::list <typename PnrT::FormOfIdType> lFoid;

        typename PnrT::PassengerType::SharedPtr Pass =
                PassengerRdr::doRead<typename PnrT::PassengerType>
                                        (reader.passengerRead(), rd);
        typename PnrT::ResContrInfoType Rci = reader.resContrInfoRead()(rd);
        typename PnrT::OrigOfRequestType Org = reader.origOfReqRead()(rd);
        CommonTicketData td = reader.commonTicketDataRead()(rd);

        reader.formOfIdRead()(rd, lFoid);
        reader.taxDetailsRead()(rd, lTax);
        reader.monetaryInfoRead()(rd, lMon);
        reader.formOfPaymentRead()(rd, lFop);
        FreeTextInfoRdr::doRead(reader.freeTextInfoRead(), rd, lIft);
        reader.ticketRead()(rd, lTicket, reader.couponRead());

        PnrT pnr(Org, Rci, lIft, Pass, lFoid, lTicket, lTax, lMon, lFop, td);
        reader.checkData(pnr);

        return pnr;
    }

    template <class PnrT, class PnrReaderT>
    static void doRead( const PnrReaderT &reader, std::list<PnrT> &lpnr)
    {
        TickReader::ReaderDataPack &rd = reader.readDataPack();

        std::list <typename PnrT::FormOfPaymentType> lFop;
        std::list <typename PnrT::TaxDetailsType> lTax;
        std::list <typename PnrT::MonetaryInfoType> lMon;
        std::list <typename PnrT::FreeTextInfoType> lIft;

        // read level 0 data

        reader.taxDetailsRead()(rd, lTax);
        reader.monetaryInfoRead()(rd, lMon);
        reader.formOfPaymentRead()(rd, lFop);

        FreeTextInfoRdr::doRead(reader.freeTextInfoRead(), rd, lIft);

        unsigned num = rd.countExpectedTickets();
        rd.unsetCommonLevel();
        for( unsigned curr=0; curr<num; curr ++)
        {
            rd.incCurrentItem();
            std::list <typename PnrT::FormOfPaymentType> lFop2;
            std::list <typename PnrT::TaxDetailsType> lTax2;
            std::list <typename PnrT::MonetaryInfoType> lMon2;
            std::list <typename PnrT::FreeTextInfoType> lIft2;

            std::list <typename PnrT::TicketType> lTicket;
            std::list <typename PnrT::FormOfIdType> lFoid;

            typename PnrT::PassengerType::SharedPtr Pass =
                    PassengerRdr::doRead<typename PnrT::PassengerType>
                    (reader.passengerRead(), rd);

            ///NOTE Rci/Org can be found in both levels - 0 and 2
            typename PnrT::ResContrInfoType Rci = reader.resContrInfoRead()(rd);
            typename PnrT::OrigOfRequestType Org = reader.origOfReqRead()(rd);
            CommonTicketData td = reader.commonTicketDataRead()(rd);

            reader.formOfIdRead()(rd, lFoid);

            reader.taxDetailsRead()(rd, lTax2);
            lTax2.insert(lTax2.begin(), lTax.begin(), lTax.end());

            reader.monetaryInfoRead()(rd, lMon2);
            lMon2.insert(lMon2.begin(), lMon.begin(), lMon.end());

            reader.formOfPaymentRead()(rd, lFop2);
            lFop2.insert(lFop2.begin(), lFop.begin(), lFop.end());

            FreeTextInfoRdr::doRead(reader.freeTextInfoRead(), rd, lIft2);
            lIft2.insert(lIft2.end(), lIft.begin(), lIft.end());
            reader.ticketRead()(rd, lTicket, reader.couponRead());

            PnrT pnr(Org, Rci, lIft2, Pass, lFoid, lTicket, lTax2, lMon2, lFop2, td);
            reader.checkData(pnr);

            lpnr.push_back(pnr);
        }

        reader.checkData(lpnr);
        return;
    }
};


struct PnrListItemRdr
{
    template <class PnrListItemT, class PnrReaderT>
    static PnrListItemT doRead(const PnrReaderT &reader)
    {
        TickReader::ReaderData &rd = reader.readData();

        typename PnrListItemT::PassengerType::SharedPtr Pass =
        PassengerRdr::doRead<typename PnrListItemT::PassengerType>
                                    (reader.passengerRead(), rd);
        typename PnrListItemT::ResContrInfoType Rci = reader.resContrInfoRead()(rd);
        typename PnrListItemT::OrigOfRequestType Org = reader.origOfReqRead()(rd);

        std::list <typename PnrListItemT::FormOfPaymentType> lFop;
        reader.formOfPaymentRead()(rd, lFop);

        std::list <typename PnrListItemT::TicketType> lTicket;
        reader.ticketRead()(rd, lTicket, reader.couponRead());

        return PnrListItem(*Pass.get(), Rci, lFop, Org, lTicket);
    }
};

struct PnrListItemDisp
{
    template<class PnrViewerT, class PnrListItemT>
            static void doDisplay(const PnrViewerT &viewer,
                                  const PnrListItemT &pnrItem)
    {
        TickView::ViewerData &vd = viewer.viewData();

        viewer.passengerView()(vd, pnrItem.pass());
        viewer.resContrInfoView()(vd, pnrItem.rci());
        viewer.formOfPaymentView()(vd, pnrItem.lfop());
        viewer.origOfReqView()(vd, pnrItem.org());
        viewer.ticketView()(vd, pnrItem.lticket(), viewer.couponView());
        viewer.monetaryInfoView()(vd, pnrItem.lmon());
    }
};


struct PnrListDisp
{
    template<class PnrListViewerT, class PnrListItemT>
    struct displayPnrList : public std::unary_function <void, const PnrListItemT &>
    {
        int currNum;
        const PnrListViewerT &Viewer;
        displayPnrList(const PnrListViewerT &viewer):currNum(0),Viewer(viewer){}
        void operator () (const PnrListItemT &item)
        {
            Viewer.viewData().setViewItemNum( currNum++ );
            PnrListItemDisp::doDisplay(Viewer, item);
        }
    };

    template <class PnrListViewerT, class PnrListT>
    static void doDisplay(const PnrListViewerT &viewer, const PnrListT &PnrList)
    {
        for_each(PnrList.lpnr().begin(), PnrList.lpnr().end(),
            displayPnrList<PnrListViewerT, typename PnrListT::PnrListItemType>(viewer));
    }
};

struct PnrListRdr
{
    template<class PnrListT, class PnrListReaderT>
    static PnrListT doRead (const PnrListReaderT &reader)
    {
        if(reader.readData().numberOfItems()<1)
        {
            throw TickExceptions::tick_fatal_except("ROMAN",__FILE__,__LINE__,"",
                                    "Too few data to display list of Pnr [%d]",
                                    reader.readData().numberOfItems());
        }

        typedef typename PnrListT::PnrListItemType PnrListItemT;
        std::list<PnrListItemT> lPnrItem;
        for(unsigned i=0; i<reader.readData().numberOfItems(); i++){
            TickReader::ReaderData &rd = reader.readData(i);

            typename PnrListItemT::PassengerType::SharedPtr Pass =
                    PassengerRdr::doRead<typename PnrListItemT::PassengerType>
                    (reader.passengerRead(), rd);
            typename PnrListItemT::ResContrInfoType Rci = reader.resContrInfoRead()(rd);
            typename PnrListItemT::OrigOfRequestType Org = reader.origOfReqRead()(rd);

            std::list <typename PnrListItemT::FormOfPaymentType> lFop;
            reader.formOfPaymentRead()(rd, lFop);

            std::list <typename PnrListItemT::TicketType> lTicket;
            reader.ticketRead()(rd, lTicket, reader.couponRead());

            std::list <typename PnrListItemT::MonetaryInfoType> lMon;
            reader.monetaryInfoRead()(rd, lMon);

            lPnrItem.push_back(PnrListItemT(*Pass.get(), Rci, lFop, Org, lTicket, lMon));
        }

        return PnrListT(lPnrItem);

    }
};

}
}
#endif /*_ETICK_TICKMNG_H_*/
