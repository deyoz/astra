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
     * ����� ��. �����
    */
namespace TicketHist
{
    /**
     * ��������� �����᪮� �����
     * ���ଠ�� � ⮬ ��, ����� � �� ᤥ���
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
         * @param org ����� ������/��⥬�
         * @param DateOp ��� ����樨
         * @param HCode_ ��� ����樨
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
        * ����� ������
        * @return �� �த���� ������
        */
        virtual const OrigOfRequestT & org() const { return Org; }

        /**
         * �६� ����樨
         * @return �६� ����樨
         */
        virtual const boost::posix_time::ptime &dateOperation() const
        {
            return DateOperation;
        }

        /**
         * ��� ����樨
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
         * @param lHHead ���᮪ HistHeadRecord
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
     * ������� ���ਨ �㯮��.
     * ����뢠�� ���ﭨ� �㯮�� � ��।������ ������ �६���.
     * ����ন� ���ଠ�� � ⮬ �� �ਢ�� ��� � �� ���ﭨ�.
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
         * ������� ����� ���ਨ �㯮��
         * @param hhead ��/����� �த���� ������
         * @param cpn ��� ����� �㯮���
         * @return No value
         */
        BaseCouponHistItem(const HistHeadRecordT &hhead,
                           const CouponT &cpn)
            : HistHead(hhead), Cpn(cpn)
        {
        }

        /**
         * �㯮� ������� ����� ���ਨ
         * @return �㯮�
         */
        virtual const CouponT & coupon() const { return Cpn; }
        /**
         * @return ��������� �����᪮� �����
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
     * ������� ���ਨ �����.
     * ����ন� ���ଠ�� �� ���ਨ �����⭮�� �����,
     * � ��� ��� �㯮���.
    */
    template < class TicketT, class CouponHistItemT >
    class BaseTicketHistItem
    {
    public:
        /**
         * ��� - �஭������ ������ �㯮��.
         * �� MaxCouponHistSize (99) ����ᥩ
         */
        typedef std::list<CouponHistItemT> CouponHist;

        /**
         * ��� - ����� ��� �㯮��� �����
         * �� 4-� ����ᥩ
        */
        typedef std::vector<CouponHist> CouponsHist;
    private:
        TicketT TicketElem;
        CouponsHist CpnsHist;
    public:
        /**
         * ������� ����� ���ਨ �����
         * @param tick ���ଠ�� �� ������
         * @param vcpn ����� �㯮��� (�� 1 �� 4 ����ᥩ)
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
         * @return ���ଠ�� �� ������
         */
        const TicketT & ticket() const { return TicketElem; }
        /**
         *
         * @return ����� ������ �㯮��
         */
        const CouponsHist &couponsHist() const
        {
            return CpnsHist;
        }

       /**
         * ��ᯥ�⪠
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
     * ����� ����� ����� (PNR).
     * ����ন� �� 4-� �易���� ����⮢.
     *
    */
    template<class PassengerT, class TicketHistItemT>
    class BasePnrHist
    {
    public:
        /**
         * ����� ����⮢. �� 4-� ����ᥩ.
         */
        typedef std::vector <TicketHistItemT> TicketsHist;
        typedef PassengerT PassengerType;
    private:
//         std::string Recloc;
        PassengerT Passenger;
        TicketsHist TicksHist;
    public:
        /**
         * @param pass ����ᠦ��
         * @param thist ����� ����⮢ ���ᠦ��
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
         * @return ���ᠦ��
         */
        virtual const PassengerT &passenger() const { return Passenger; }
        /**
         *
         * @return ����� �����
         */
        virtual const TicketsHist &ticksHist() const { return TicksHist; }

        /**
         * recloc, ��� �����
         * @return std::string - recloc
         */
//         virtual const std::string &recloc() const { return Recloc; }

        /**
         * ��ᯥ�⪠
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
