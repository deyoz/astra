/*	2006 by Roman Kovalev 	*/
/*	rom@sirena2000.ru		*/
#ifndef _ETICK__TICKET_HIST_H_
#define _ETICK__TICKET_HIST_H_

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <list>
#include <vector>
#include <string>

// Local includes
#include <etick/ticket.h>
#include <etick/tick_data.h>
#include <etick/exceptions.h>

#include <serverlib/slogger_nonick.h>

namespace Ticketing
{
    /**
     * История Эл. Билета
    */
namespace TicketHist
{
    /**
     * Заголовок исторической записи
     * Информация о том кто, когда и что сделал
    */
    template<class OrigOfRequestT>
    class BaseHistHeadRecord
    {
        OrigOfRequestT Org;
        boost::posix_time::ptime DateOperation;
        HistCode HCode;
    public:
        /**
         *
         * @param org Данные оператора/системы
         * @param DateOp Дата операции
         * @param HCode_ Код операции
         * @return no value
         */
            BaseHistHeadRecord(const OrigOfRequestT & org,
                            const boost::posix_time::ptime &DateOp,
                            const HistCode &HCode_)
                :Org(org), DateOperation(DateOp), HCode(HCode_)
        {
            if(DateOp.is_special())
            {
                throw ClassIntegralityErr("Invalid date of operation");
            }
        }

        /**
        * Данные оператора
        * @return Кто проделал операцию
        */
        virtual const OrigOfRequestT & org() const { return Org; }

        /**
         * Время операции
         * @return Время операции
         */
        virtual const boost::posix_time::ptime &dateOperation() const
        {
            return DateOperation;
        }

        /**
         * Код операции
         * @return HistCode::HistCode
         */
        const HistCode &histCode() const
        {
            return HCode;
        }

        /**
         * Trace lement
         * @param level
         * @param nick
         * @param file
         * @param line
         */
        virtual void Trace(int level, const char *nick, const char *file,
                           int line) const
        {
            LogTrace(level, nick, file, line)
                    << "date/time: " << dateOperation()
                    << ", Operation: [" << histCode()->description() << "]/["
                    << (histCode().subCode() ? histCode().subCode()->description() : "")
                    << "]";

            org().Trace(level, nick, file, line);
        }

        /**
         * Trace list of elements
         * @param level
         * @param nick
         * @param file
         * @param line
         * @param lHHead Список HistHeadRecord
         */
        template<class HistHeadRecordT>
        static void Trace(int level, const char *nick, const char *file,
                          int line,
                          std::list< HistHeadRecordT > &lHHead)
        {
            for_each(lHHead.begin(),lHHead.end(),
                     TraceElement<HistHeadRecordT>(level,nick,file,line));
        }
        virtual ~BaseHistHeadRecord(){}
    };

    /**
     * Элемент истории купона.
     * Описывает состояние купона в определенный момент времени.
     * Содержит информацию о том кто привел его в это состояние.
    */
    template<
            class HistHeadRecordT,
            class CouponT
            >
    class BaseCouponHistItem
    {
        HistHeadRecordT HistHead;
        CouponT Cpn;
    public:
        /**
         * Создать элемент истории купона
         * @param hhead Кто/Когда проделал операцию
         * @param cpn Над каким купоном
         * @return No value
         */
        BaseCouponHistItem(const HistHeadRecordT &hhead,
                           const CouponT &cpn)
            : HistHead(hhead), Cpn(cpn)
        {
        }

        /**
         * Купон данного элемента истории
         * @return Купон
         */
        virtual const CouponT & coupon() const { return Cpn; }
        /**
         * @return Заголовок исторической записи
         */
        virtual const HistHeadRecordT & histHead() const { return HistHead; }

        /**
         * Trace
         * @param level
         * @param nick
         * @param file
         * @param line
         */
        virtual void Trace(int level, const char *nick, const char *file,
                           int line) const
        {
            histHead().Trace(level,nick,file,line);
            coupon().Trace(level,nick,file,line);
        }

        virtual ~BaseCouponHistItem(){}
    };


    /**
     * Элемент истории билета.
     * Содержит информацию об истории конкретного билета,
     * и всех его купонов.
    */
    template < class TicketT, class CouponHistItemT >
    class BaseTicketHistItem
    {
    public:
        /**
         * Тип - хронология одного купона.
         * До MaxCouponHistSize (99) записей
         */
        typedef std::list<CouponHistItemT> CouponHist;

        /**
         * Тип - История всех купонов билета
         * До 4-х записей
        */
        typedef std::vector<CouponHist> CouponsHist;
    private:
        TicketT TicketElem;
        CouponsHist CpnsHist;
    public:
        /**
         * Создать элемент истории билета
         * @param tick Информация по билету
         * @param vcpn История купонов (от 1 до 4 записей)
         * @return No value
         */
        BaseTicketHistItem(const TicketT &tick, const CouponsHist &vcpn)
            :TicketElem(tick), CpnsHist(vcpn)
        {
            unsigned ncoupons = couponsHist().size();
            if(ncoupons == 0)
            {
                throw ClassIntegralityErr("There are no coupons in the ticket history");
            } else if(ncoupons > 4)
            {
                throw ClassIntegralityErr("Too many coupons for one ticket");
            }
            for (unsigned i = 0; i<ncoupons; i++)
            {
                unsigned chistsize = couponsHist()[i].size();
                if(chistsize == 0)
                {
                    throw ClassIntegralityErr("Coupon history is blank");
                }/* else if (chistsize > MaxCouponHistSize)
                {
                    throw ClassIntegralityErr("Too long history for coupon, 99 is max");
                }*/
            }
        }

        /**
         *
         * @return Информацию по билету
         */
        const TicketT & ticket() const { return TicketElem; }
        /**
         *
         * @return История одного купона
         */
        const CouponsHist &couponsHist() const
        {
            return CpnsHist;
        }

       /**
         * Распечатка
         * @param level
         * @param nick
         * @param file
         * @param line
        */
        virtual void Trace(int level, const char *nick, const char *file,
                           int line) const
        {
            ticket().Trace(level, nick, file, line);
            for(unsigned i = 0; i<couponsHist().size(); i++)
            {
                if(couponsHist()[i].empty())
                {
                    LogWarning(nick, file, line) << "(" << i << ") coupon history empty!";
                }
                LogTrace(level, nick, file, line) <<
                        "=== CPN " <<
                        couponsHist()[i].front().coupon().couponInfo().num() <<
                        " (" << i << ")===";
                for_each(couponsHist()[i].begin(),
                         couponsHist()[i].end(),
                         TraceElement<CouponHistItemT>(level,nick,file,line));
            }
        }


        virtual ~BaseTicketHistItem(){}
    };


    /**
     * История одной записи (PNR).
     * Содержит до 4-х связанных билетов.
     *
    */
    template<class PassengerT, class TicketHistItemT>
    class BasePnrHist
    {
    public:
        /**
         * История билетов. До 4-х записей.
         */
        typedef std::vector <TicketHistItemT> TicketsHist;
        typedef PassengerT PassengerType;
    private:
//         std::string Recloc;
        PassengerT Passenger;
        TicketsHist TicksHist;
    public:
        /**
         * @param pass Пасссажир
         * @param thist История билетов пассажира
         * @return No valus
         */
        BasePnrHist(const PassengerT &pass,
                    const TicketsHist &thist)
            : Passenger(pass), TicksHist(thist)
        {
            if(ticksHist().empty())
            {
                throw ClassIntegralityErr("There are no tickets in the history");
            }
        }

        /**
         *
         * @return Пассажир
         */
        virtual const PassengerT &passenger() const { return Passenger; }
        /**
         *
         * @return История Билета
         */
        virtual const TicketsHist &ticksHist() const { return TicksHist; }

        /**
         * recloc, чья история
         * @return std::string - recloc
         */
//         virtual const std::string &recloc() const { return Recloc; }

        /**
         * Распечатка
         * @param level
         * @param nick
         * @param file
         * @param line
         */
        virtual void Trace(int level, const char *nick, const char *file,
                           int line) const
        {
            passenger().Trace(level, nick, file, line);
            for_each(ticksHist().begin(),ticksHist().end(),
                     TraceElement<TicketHistItemT>(level,nick,file,line));
        }
        virtual ~BasePnrHist(){}
    };
}
}
#endif /*_ETICK__TICKET_HIST_H_*/
