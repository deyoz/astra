/*	2006 by Roman Kovalev 	*/
/*	rom@sirena2000.ru	*/
#ifndef _ETICK__TICK_HIST_READER_H_
#define _ETICK__TICK_HIST_READER_H_
namespace Ticketing{
namespace TickHistReader{

    class HistReaderData{
    public:
        virtual ~HistReaderData() {}
    };

    template<class PassengerT>
    class BasePassengerHistReader {
    public:
        virtual PassengerT operator () (HistReaderData &rdrData) const = 0;
        virtual ~BasePassengerHistReader(){}
    };

    template<class TicketHistT>
    class BaseTicketHistReader {
    public:
        virtual TicketHistT operator () (HistReaderData &rdrData) const = 0;
        virtual ~BaseTicketHistReader () {}
    };

    template<class PassengerT,
             class TicketHistT>
    class BasePnrHistReader
    {
    public:
        virtual HistReaderData &readData() const = 0;

        virtual const BaseTicketHistReader <TicketHistT>
                &ticketHistReader() const = 0;
        virtual const BasePassengerHistReader <PassengerT>
                &passengerHistReader() const = 0;

        virtual ~BasePnrHistReader(){};
    };

}
}
#endif /*_ETICK__TICK_HIST_READER_H_*/
