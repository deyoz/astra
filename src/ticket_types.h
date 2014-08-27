#ifndef _TICKET_TYPES_H_
#define _TICKET_TYPES_H_
#include <string>
#include <serverlib/int_parameters.h>

namespace Ticketing {

    class TicketNum_t {
        static const std::string TchFormCode;
        static const std::string AlOwnFormCode;

        std::string Ticket;
        static const unsigned int StrictLen = 13;
        static bool isValid(const std::string &ticket);
    public:
        TicketNum_t(){}
        explicit TicketNum_t(const std::string &tick);

        const std::string &get() const { return Ticket; }
        std::string &get() { return Ticket; }
        bool empty() const { return Ticket.empty(); }

        /**
         * @brief account number
         * @return
         */
        std::string accNum() const;

        /**
         * @brief Определяет, выписан ли данный билет через ЦЭБ ТКП
         * @return bool
         */
        bool isTchControlled() const;

        static void check(const std::string &tick);

        static std::string cut_check_digit(const std::string & tick);
    };

    inline bool operator<(const TicketNum_t &v1, const TicketNum_t &v2)
    {
        return v1.get() < v2.get();
    }

    inline bool operator==(const TicketNum_t &t1, const TicketNum_t &t2)
    {
        return t1.get() == t2.get();
    }

    inline bool operator!=(const TicketNum_t &t1, const TicketNum_t &t2)
    {
        return t1.get() != t2.get();
    }

    inline std::ostream  &operator << (std::ostream &s, const TicketNum_t &v )
    {
        s<<v.get();
        return s;
    }

    MakeIntParamType(BaseCouponNum_t, int);
    class CouponNum_t : public BaseCouponNum_t
    {
        static bool isValid(int n);
    public:
        explicit CouponNum_t(int n);
        CouponNum_t() : BaseCouponNum_t() {}
        virtual ~CouponNum_t(){}
    };


    class TicketCpn_t {
        TicketNum_t TicketNum;
        CouponNum_t Cpn;
    public:
        TicketCpn_t()
        {}
        explicit TicketCpn_t(const TicketNum_t &tick, CouponNum_t cpn)
            :TicketNum(tick), Cpn(cpn)
        {
        }

        explicit TicketCpn_t(const std::string &tick, int cpn)
            :TicketNum(tick), Cpn(cpn)
        {
        }

        const TicketNum_t &ticket() const { return TicketNum; }
        const CouponNum_t &cpn() const { return Cpn; }
    };

    inline bool operator<(const TicketCpn_t &v1, const TicketCpn_t &v2)
    {
        return (v1.ticket() == v2.ticket())?
                    (v1.cpn() < v2.cpn())
                :
                    (v1.ticket() < v2.ticket());
    }

    inline bool operator==(const TicketCpn_t &t1, const TicketCpn_t &t2)
    {
        return (t1.cpn() == t2.cpn() && t1.ticket() == t2.ticket());
    }

    inline std::ostream  &operator << (std::ostream &s, const TicketCpn_t &v )
    {
        s<<v.ticket() << "/" << v.cpn();
        return s;
    }

} // namespace Ticketing

#endif /* _TICKET_TYPES_H_ */
