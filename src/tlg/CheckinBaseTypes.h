#ifndef _CHECKINBASETYPES_H_
#define _CHECKINBASETYPES_H_

#include <serverlib/int_parameters.h>
#include <serverlib/strong_types.h>
#include <serverlib/str_utils.h>
#include "exceptions.h"

#include <boost/lexical_cast.hpp>


namespace Ticketing
{
    MakeIntParamType(FlightNum_t, unsigned);
    MakeIntParamType(SystemAddrs_t, int);
    MakeIntParamType(EdifactProfile_t, int);
    MakeIntParamType(Point_t, int);
    //MakeIntParamType(RouterId_t, int);

    FlightNum_t getFlightNum(const std::string &s);
    FlightNum_t getFlightNum(int n);

    ///@class TicketBaseTypeExcept
    ///@brief Этот exception кидается при неправильном использовании TicketBaseTypes
    class TicketBaseTypeExcept : public EXCEPTIONS::Exception
    {
        public:
        TicketBaseTypeExcept(const std::string &e)
            :EXCEPTIONS::Exception(e)
        {
        }
    };


    enum IDS
    {
        POINT,
        CITY=0,
        PORT,
        AIRLINE,
        COUNTRY,
        ROUTER
    };


    template <IDS T> struct Traits
    {
    };


    ///@class Value template < IDS T,class IntType> class Value
    ///@brief Базовый класс для типизации int-like переменных
    template < IDS T,class IntType> class Value
    {
        IntType val_;
    public:
        explicit Value( IntType v):val_(v){}
        Value( ):val_(std::numeric_limits<IntType>::max()){}
        IntType get() const
        {
            if(val_==std::numeric_limits<IntType>::max()){
                throw TicketBaseTypeExcept("undefined int param used :"
                        +HelpCpp:: string_cast(T));
            }
            return val_;
        }
        IntType * never_use_except_in_OciCpp ()
        {
            return & val_;
        }
        IntType const * never_use_except_in_OciCpp () const
        {
            return & val_;
        }

    private:
        struct boolean {int i;};
    public:
        typedef IntType BaseType;
        /**
         * @brief проверка на инициированность
         */
        operator int boolean::*() const
        {
            if(val_ == std::numeric_limits<IntType>::max())
            {
                return NULL;
            }
            else
            {
                return &boolean::i;
            }
        }

        Value<T,IntType> inline  operator+=(IntType i)
        {
            (void) Traits<T>::addInt;
            return Value<T,IntType>(get()+i);
        }
        Value<T,IntType> inline  operator-=(IntType i)
        {
            (void) Traits<T>::addInt;
            return Value<T,IntType>(get()-i);
        }
        /**
         * @brief Преобразовать в строковое представление
         * @return
         */
        std::string toString() const
        {
            return HelpCpp::string_cast(get());
        }
        /**
         * @brief Построить элемент из строкового значения
         * @param const std::string &str
         * @return Value<T,IntType>
         */
        static Value<T,IntType> fromString(const std::string &str)
        {
            return Value<T,IntType>(boost::lexical_cast<IntType>(str));
        }
    };

    /**
     * @brief Правило вывода на экран/в лог
     * @param s stream
     * @param v value
     * @return stream
     */
    template <IDS T,class IntType> std::ostream  &
            operator << (std::ostream &s, Value<T,IntType> v )
    {
        if(v)
        {
            s<<v.get();
        }
        else
        {
            s<<"?";
        }
        return s;
    }

template <IDS T,class IntType>
    bool operator==(Value<T,IntType> v1,Value<T,IntType> v2)
    {
        return v1 && v2 && v1.get()==v2.get();
    }
template <IDS T,class IntType>
    inline bool operator!=(Value<T,IntType> v1,Value<T,IntType> v2)
    {
        return !v1 || !v2 || v1.get()!=v2.get();
    }
template <IDS T, class IntType>
    inline bool operator==(const IntType i, const Value<T,IntType> v)
    {
        (void) Traits<T>::can_compare_to_int;
        return v && i==v.get();
    }
template <IDS T, class IntType>
    inline bool operator==( Value<T,IntType> v,IntType i)
    {
        (void) Traits<T>::can_compare_to_int;
        return v && i==v.get();
    }
template <IDS T, class IntType>
    inline bool operator!=(IntType i, Value<T,IntType> v)
    {
        (void) Traits<T>::can_compare_to_int;
        return !v || i!=v.get();
    }
template <IDS T, class IntType>
    inline bool operator!=(Value<T,IntType> v,IntType i)
    {
        (void) Traits<T>::can_compare_to_int;
        return !v || i!=v.get();
    }
template <IDS T,class IntType>
    inline bool operator<(Value<T,IntType> v1,Value<T,IntType> v2)
    {
        return v1.get()<v2.get();
    }


    typedef Value<CITY, int>    City_t;
    typedef Value<PORT, int>    Port_t;
    typedef Value<AIRLINE, int> Airline_t;
    typedef Value<COUNTRY, int> Country_t;
    typedef Value<ROUTER, int>  RouterId_t;

} //namespace Ticketing

#endif /*_CHECKINBASETYPES_H_*/
