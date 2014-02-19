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

} // namespace Ticketing

#endif /* _TICKET_TYPES_H_ */
