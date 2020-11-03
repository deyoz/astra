/*	2006 by Roman Kovalev 	*/
/*	rom@sirena2000.ru	*/
#ifndef _ETICK__TICK_HIST_VIEW_H_
#define _ETICK__TICK_HIST_VIEW_H_
namespace Ticketing
{
namespace TickHistView
{
class HistViewerData{
public:
    virtual ~HistViewerData() {}
};

template<class PassengerT>
class BasePassengerHistViewer {
public:
    virtual void operator () (HistViewerData &viewData,
                              const PassengerT &pass) const = 0;
    virtual ~BasePassengerHistViewer(){}
};

template<class TicketHistT>
class BaseTicketHistViewer {
public:
    virtual void operator () (HistViewerData &rdrData,
                              const TicketHistT &thist) const = 0;
    virtual ~BaseTicketHistViewer () {}
};

template<class PassengerT,
         class TicketHistT>
class BasePnrHistViewer
{
public:
    virtual HistViewerData &viewData() const = 0;

    virtual const BaseTicketHistViewer <TicketHistT>
            &ticketHistViewer() const = 0;
    virtual const BasePassengerHistViewer <PassengerT>
            &passengerHistViewer() const = 0;

    virtual ~BasePnrHistViewer(){};
};

}
}
#endif /*_TICK_HIST_VIEW_H_*/
