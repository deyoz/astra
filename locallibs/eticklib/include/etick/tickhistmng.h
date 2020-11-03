/*	2006 by Roman Kovalev 	*/
/*	rom@sirena2000.ru		*/
#ifndef _ETICK__TICKHISTMNG_H_
#define _ETICK__TICKHISTMNG_H_

#include <etick/tick_hist_reader.h>
#include <etick/tick_hist_view.h>

namespace Ticketing
{
    /**
     * Чтение/Просмотр истории ЭБ
    */
namespace TickHistMng{

    /**
     * Чтение информации о пассажире
    */
struct PassengerHistRdr
{
    /**
     * Чтение
     * @param reader Класс, определяющий как и от куда читать
     * @param rd Данные Reader'а
     * @return Пассажир
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
 * Отображение информации о пассажире
 */
struct PassengerHistDisp
{
    /**
     * Просмотр
     * @param viewer Класс, определяющий как и куда отображать
     * @param vd Данные viewer'а
     * @param pass Пассажир
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
 * Чтение истории билетов
*/
struct TicketHistRdr
{
    /**
     * Чтение
     * @param reader Класс, определяющий как и от куда читать
     * @param rd Данные Reader'а
     * @return История билетов
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
 * Просмотр истории билетов
 */
struct TicketHistDisp
{
    /**
     * Чтение
     * @param viewer Класс, определяющий как и куда отображать
     * @param vd Данные viewer'а
     * @param thist История билетов
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
 * Чтение истории
 */
struct PnrHistRdr
{
    /**
     * Читает историю билета
     * @param reader Класс, определяющий как и от куда читать
     * @return История
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

