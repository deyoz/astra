/*	2006 by Roman Kovalev 	*/
/*	rom@sirena2000.ru		*/
#ifndef _ETICK__TICKHISTMNG_H_
#define _ETICK__TICKHISTMNG_H_

#include <etick/tick_hist_reader.h>
#include <etick/tick_hist_view.h>

namespace Ticketing
{
    /**
     * �⥭��/��ᬮ�� ���ਨ ��
    */
namespace TickHistMng{

    /**
     * �⥭�� ���ଠ樨 � ���ᠦ��
    */
struct PassengerHistRdr
{
    /**
     * �⥭��
     * @param reader �����, ��।����騩 ��� � �� �㤠 ����
     * @param rd ����� Reader'�
     * @return ���ᠦ��
    */
    template<class PassengerT,
             class PassengerHistReaderT>
    static PassengerT
            doRead(const PassengerHistReaderT &reader,
                   TickHistReader::HistReaderData &rd)
    {
        return reader(rd);
    }
};

/**
 * �⮡ࠦ���� ���ଠ樨 � ���ᠦ��
 */
struct PassengerHistDisp
{
    /**
     * ��ᬮ��
     * @param viewer �����, ��।����騩 ��� � �㤠 �⮡ࠦ���
     * @param vd ����� viewer'�
     * @param pass ���ᠦ��
     * @return no value
     */
    template<class PassengerT, class PassengerHistViewerT>
    static void
            doDisplay(const PassengerHistViewerT &viewer,
                      TickHistView::HistViewerData &vd,
                      const PassengerT &pass)
    {
        return viewer(vd,pass);
    }
};


/**
 * �⥭�� ���ਨ ����⮢
*/
struct TicketHistRdr
{
    /**
     * �⥭��
     * @param reader �����, ��।����騩 ��� � �� �㤠 ����
     * @param rd ����� Reader'�
     * @return ����� ����⮢
     */
    template<class TicketHistT,
             class TicketHistReaderT>
    static TicketHistT
        doRead(const TicketHistReaderT &reader,
               TickHistReader::HistReaderData &rd)
    {
        return reader(rd);
    }
};

/**
 * ��ᬮ�� ���ਨ ����⮢
 */
struct TicketHistDisp
{
    /**
     * �⥭��
     * @param viewer �����, ��।����騩 ��� � �㤠 �⮡ࠦ���
     * @param vd ����� viewer'�
     * @param thist ����� ����⮢
     * @return no value
     */
    template<class TicketHistT, class TicketHistViewerT>
            static void
                doDisplay(const TicketHistViewerT &viewer,
                      TickHistView::HistViewerData &vd,
                      const TicketHistT &thist)
    {
        return viewer(vd, thist);
    }
};


/**
 * �⥭�� ���ਨ
 */
struct PnrHistRdr
{
    /**
     * ��⠥� ����� �����
     * @param reader �����, ��।����騩 ��� � �� �㤠 ����
     * @return �����
     */
    template <class PnrHistT, class PnrHistReaderT>
    static PnrHistT doRead( const PnrHistReaderT &reader)
    {
        TickHistReader::HistReaderData &rd = reader.readData();

        return PnrHistT(
                PassengerHistRdr::doRead<typename PnrHistT::PassengerType>
                (reader.passengerHistReader(), rd),
                TicketHistRdr::doRead<typename PnrHistT::TicketsHist>
                (reader.ticketHistReader(), rd));
    }
};

struct PnrHistDisp
{
    template <class PnrHistViewerT, class PnrHistT>
    static void doDisplay( const PnrHistViewerT &viwer, const PnrHistT &pnrHist)
    {
        TickHistView::HistViewerData &vd = viwer.viewData();

        PassengerHistDisp::doDisplay(viwer.passengerHistViewer(), vd,
                                     pnrHist.passenger());
        TicketHistDisp::doDisplay (viwer.ticketHistViewer(), vd,
                                   pnrHist.ticksHist());
    }
};

}
}

#endif /*_ETICK__TICKHISTMNG_H_*/

