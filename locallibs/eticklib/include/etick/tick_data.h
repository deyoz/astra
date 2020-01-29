//
// C++ Interface: tick_data
//
// Description:
//
//
// Author: Roman Kovalev <rom@sirena2000.ru>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef _ETICK_TICK_DATA_H_
#define _ETICK_TICK_DATA_H_

#include <string>
#include <string.h>
#include <map>
#include <numeric>
#include <list>
#include <stdint.h>
#include "etick/lang.h"
#include "serverlib/base_code_set.h"
#include <serverlib/strong_types.h>
namespace Ticketing
{

/**
 * @class CouponStatList
 * @brief Coupon status description
*/
class CouponStatList : public BaseTypeElem<int>
{
    const char *DispCode;
    unsigned Flags;
    typedef BaseTypeElem<int> CodeListData_t;
    public:
        static const char *ElemName;
        CouponStatList(int codeI, const char *actCd,
                    const char *dcode,
                    unsigned fl,
                    const char *ldesc,
                    const char *rdesc) throw()
        :CodeListData_t(codeI, actCd, actCd, rdesc, ldesc),
        DispCode(dcode),
        Flags(fl)
        {
        }

        const char * dispCode() const { return DispCode; }
        unsigned flags() const { return Flags; }

        /**
         * @brief Final or not
         * @return
         */
        bool isFinal() const;
        /**
         * @brief ready to check in.
         * @return
         */
        bool isReadyToCheckin() const;
        /**
         * @brief can be changed not to pay attention to others
         * @return
         */
        bool isSingleChangeable() const;
        /**
         * @brief active coupon (we have control on it and it is not final)
         * @return
         */
        bool isActive() const;

        virtual ~CouponStatList(){}
};

/**
 * @class CouponStat
 * @brief Coupon status class holder
*/
class CouponStatus : public BaseTypeElemHolder<CouponStatList>
{
    public:
        typedef BaseTypeElemHolder<CouponStatList> TypeElemHolder;
        typedef TypeElemHolder::TypesMap CouponStatMap;
        typedef std::list< CouponStatus > listOfStatuses_t;

        explicit CouponStatus():TypeElemHolder() {}
        explicit CouponStatus(const TypeElemHolder &base) :
                TypeElemHolder(base)
        {
        }
        explicit CouponStatus(int t):TypeElemHolder(t) {}
        explicit CouponStatus(const std::string &status);
        explicit CouponStatus(const char *status);

        static CouponStatus fromDispCode(const std::string &status);
        static listOfStatuses_t getListOfStatuses(unsigned f1=0, unsigned f2=0);

    enum
    {
        Final_stat=0x01,
        Single_changeable=0x02,
        Ready_to_checkin=0x04
    };

    enum {
        OriginalIssue   =1,
        Exchanged       =2,
        Refunded        =3,
        Void            =4,
        Printed         =5,
        PrintExch       =6,
        Airport         =7,
        Checked         =8,
        Boarded         =9,
        Flown           =10,
        ExchangedFIM    =11,
        Irregular       =12,
        Notification    =13,
        Suspended       =14,
        Closed          =15,
        Paper           =16,
        Unavailable     =17
    };

    bool operator<(const CouponStatus &status) const { return (*this)->type() < status->type(); }
private:
    struct find_by_dispcode : public BaseTypeElemHolder<CouponStatList>::userFind
    {
        const char *code;
        find_by_dispcode(const char *c): code(c){}
        bool operator () (const TypeElemHolder::TypesMap::value_type &e) const
        {
            return !strcmp(e.second.dispCode(), code);
        }

        virtual std::string dataName() const { return "display code"; }
        virtual std::string dataStr()  const { return code;   }
        virtual ~find_by_dispcode(){}
    };
};

std::ostream  &operator << (std::ostream &s,
                            const CouponStatus &status);


class FoidTypeList : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
    public:
        static const char *ElemName;
        FoidTypeList(int codeI, const char *actCd,
                       const char *ldesc,
                       const char *rdesc) throw()
        :CodeListData_t(codeI, actCd, actCd, rdesc, ldesc)
        {
        }
        virtual ~FoidTypeList(){}
};

class FoidType : public BaseTypeElemHolder<FoidTypeList>
{
public:
    typedef BaseTypeElemHolder<FoidTypeList> TypeElemHolder;
    typedef TypeElemHolder::TypesMap FoidTypesMap;

    FoidType():TypeElemHolder()
    {
    }
    FoidType(const TypeElemHolder &base) :
        TypeElemHolder(base)
    {
    }
    FoidType(int t):TypeElemHolder(t)
    {
    }
    FoidType(const std::string &status);
    FoidType(const char *status);

    enum
    {
        CreditCard=1,
        DriversLicense=2,
        FrequentFlyer=3,
        CustomerId=6,
        InvoiceReference=8,
        Passport,
        NationalIdentity,
        ConfirmationNumber,
        TicketNumber,
        LocallyDefinedID
    };
};

class TicketMediaElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
    public:
        static const char *ElemName;
        TicketMediaElem(int codeI, const char *lcode, const char *rcode,
                        const char *ldesc,
                        const char *rdesc) throw()
        :CodeListData_t(codeI, rcode, lcode, rdesc, ldesc)
        {
        }
        virtual ~TicketMediaElem(){}
};

class TicketMedia : public BaseTypeElemHolder<TicketMediaElem>
{
public:
    typedef BaseTypeElemHolder<TicketMediaElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap TicketMediaMap;

    enum Media_t
    {
        Electronic=1,
        Paper=2
    };

    TicketMedia():TypeElemHolder()
    {
    }
    TicketMedia(const TypeElemHolder &base) :
        TypeElemHolder(base)
    {
    }
    TicketMedia(Media_t t):TypeElemHolder(t)
    {
    }
    TicketMedia(const std::string &status);
    TicketMedia(const char *status);

};

    class ItinStatusElem : public BaseTypeElem<int>
    {
        typedef BaseTypeElem<int> CodeListData_t;
    public:
        static const char *ElemName;
        ItinStatusElem(int codeI, const char *code,
                       const char *desc) throw()
            :CodeListData_t(codeI, code, code, desc, desc)
        {
        }
    };

    class ItinStatus : public BaseTypeElemHolder<ItinStatusElem>
    {
    public:
        typedef BaseTypeElemHolder<ItinStatusElem> TypeElemHolder;
        typedef TypeElemHolder::TypesMap ItinStatusMap;
        typedef std::list<ItinStatusElem> listElems_t;
        typedef enum
        {
            Confirmed=1,
            InfantNoSeat,
            OpenDate,
            NonAir,
            Standby,
            SpaceAvailable,
            Requested
        } Type_t;
        static const std::string Open;
        static const size_t Minlen=2;
        static const size_t Maxlen=3;

        ItinStatus();
        ItinStatus(const std::string &status);
        ItinStatus(Type_t stat);

        void setSeats(const std::string &seat)
        {
            Seats = seat;
        }
        const std::string &seats() const { return Seats; }

        /**
        * Базовый класс данного подкласса
        * @return
        */
        static const listElems_t &initStatList();
    private:
        std::string Seats;
        static listElems_t StaticInitStatList;
    };

namespace TickStatAction{
    typedef enum{
        newtick=1,
        oldtick,
        inConnectionWith
    }TickStatAction_t;

    TickStatAction_t GetTickAction(const char *act);
    TickStatAction_t GetTickActionAirimp(const char *act);
    std::string TickActionStr(TickStatAction_t act);
    std::string TickActionTraceStr(TickStatAction_t act);
}

namespace CpnStatAction{
    typedef enum{
        consumedAtIssuance=6,
        associate=702,
        disassociate=703
    }CpnStatAction_t;

    CpnStatAction_t GetCpnAction(const char *act);
    std::string CpnActionStr(CpnStatAction_t act);
    std::string CpnActionTraceStr(CpnStatAction_t act);
}



/// @class FopIndicatorElem
/// @brief Индикатор формы оплыты (Новая/Старая/Изначальная)
class FopIndicatorElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    FopIndicatorElem(int codeI, const char *code,
                   const char *desc) throw()
    :CodeListData_t(codeI, code, code, desc, desc)
    {
    }
};

/// @class FopIndicator
/// @brief Индикатор формы оплыты (Новая/Старая/Изначальная)
class FopIndicator : public BaseTypeElemHolder<FopIndicatorElem>
{
public:
    typedef BaseTypeElemHolder<FopIndicatorElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap FopIndicatorMap;
    typedef std::list<FopIndicatorElem> listElems_t;
    ///@enum FopIndicator_t
    ///@brief типы форм оплаты
    enum FopIndicator_t
    {
        /// Новая
        New=1,
        /// Изначальная (указывается при обмене)
        Orig=2,
        /// старая
        Old=3
    };
    /**
     * @brief От строки
     * @param act
     */
    FopIndicator(const std::string &act);
    /**
     * @brief От строки
     * @param act
     */
    FopIndicator(const char *act);
    /**
     * @brief От типа
     * @param act
     */
    FopIndicator(FopIndicator_t act);

    FopIndicator(){}
};

/// @class TaxCategoryElem
/// @brief Категория сборов (Текущие/оплаченные/дополнительные)
class TaxCategoryElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    TaxCategoryElem(int codeI, const char *code,
                    const char *desc) throw()
    :CodeListData_t(codeI, code, code, desc, desc)
    {
    }
};

/// @class TaxCategory
/// @brief Категория сборов (Текущие/оплаченные/дополнительные)
class TaxCategory : public BaseTypeElemHolder<TaxCategoryElem>
{
public:
    typedef BaseTypeElemHolder<TaxCategoryElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap FopIndicatorMap;
    typedef std::list<TaxCategoryElem> listElems_t;
    ///@enum TaxCategory_t
    ///@brief типы категорий сборов
    enum TaxCategory_t
    {
        /// Текущие
        Current=1,
        /// Оплаченные
        Paid=2,
        /// Дополнительные
        Additional=3,
        /// Сборы за валидацию
        CarrierFee=4,
        /// Сборы за валидацию, оплаченые
        TaxOnCarrierFee=5
    };
    /**
     * @brief Конструктор по-умолчанию
     */
    TaxCategory() : TypeElemHolder()
    {
    }
    /**
     * @brief От строки
     * @param act
     */
    TaxCategory(const std::string &act);
    /**
     * @brief От строки
     * @param act
     */
    TaxCategory(const char *act);
    /**
     * @brief От типа
     * @param act
     */
    TaxCategory(TaxCategory_t act);
};

DEFINE_BOOL(CutZeroFraction);

#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wtype-limits" 
DEFINE_NUMBER_WITH_RANGE_VALIDATION(MinFractionLength, unsigned short, 0, 10,
                                    "MIN FRACTION LENGTH", "МИН. ДЛИНА ДРОБНОЙ ЧАСТИ");
#pragma GCC diagnostic pop

///@namespace TaxAmount
namespace TaxAmount{
    typedef long long Amount_t;
    ///@class Amount
    ///@brief Величина (денежная) в числе или процентах
    class Amount
    {
        Amount_t Am;
        std::string Am_str;
        unsigned short dot_pos;
        std::string IntPart;
        std::string Fraction;
        bool Percent;
        bool Valid;
    public:
        static const std::string TaxExempt;
        enum AmountType_e
        {
            Ordinary,
            Percents
        };

        explicit Amount(const std::string &am, AmountType_e type = Ordinary);
        explicit Amount(Amount_t am, unsigned pos, AmountType_e type = Ordinary);
        explicit Amount():
            Am(0),Am_str("0.00"),dot_pos(2),IntPart("0"),Fraction("0"),
            Percent(false),Valid(false)
        {
        }

        Amount &setDot(unsigned short pos);
        Amount getCopyWithDot(unsigned short pos) const;

        Amount_t amValue() const;
        Amount_t amValue(unsigned short pos) const;

        const std::string &amStr() const;
        std::string amStr(CutZeroFraction cutZeroFraction,
                          MinFractionLength minFractionLength = MinFractionLength(0)) const;

        unsigned short dotPos() const
        {
            return dot_pos;
        }

        const std::string &intPart() const
        {
            return IntPart;
        }

        const std::string &fraction() const
        {
            return Fraction;
        }

        /**
         * @brief Возвращает true, если хранимое значение - проценты
         * @return bool
         */
        bool isPercents() const
        {
            return Percent;
        }
        bool isValid() const
        {
            return Valid;
        }

        static bool validate(const std::string &am);
        Amount& operator +=(const Amount &);
    };

    Amount operator + (const Amount &, const Amount &);
    Amount operator * (const Amount &, const Amount &);
    bool operator == (const Amount &, const Amount &);
    bool operator != (const Amount &, const Amount &);
    int amountToInt(const TaxAmount::Amount &amount);
}

class AmountCodeElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    AmountCodeElem(int codeI, const char *code, const char *desc) throw()
            :CodeListData_t(codeI, code, code, desc, desc){}
};

class AmountCode: public BaseTypeElemHolder<AmountCodeElem>
{
public:
    typedef BaseTypeElemHolder<AmountCodeElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap AmountCodeMap;
    typedef std::list<AmountCodeElem> listElems_t;

    enum AmountCode_t
    {
        Total = 0,       // (T) Total ticket amount
        Base_fare,       // (B) Base fare
        Equivalent,      // (E) Equivalent fare
        ExchRate,        // (D) Bank Echange Rate
        TotalAdd,        // (C) Additional collection total (TCH - Итоговая величина доплаты)
        Commission,      // (F) Commission amount
        CommissionRate,  // (G) Commission rate
        TotalSellAmount, // (I) Total ticket/document sell amount
        TotalTicket,     // (M) Ticket total amount
        Security,        // (Y) Security
        USDSecurity,     // (Y2) Конфиденциальный тариф в USD
        NetFare          // (H) Net fare amount (Base fare net amount)
    };

    /**
     * @brief От строки
     * @param act
     */
    AmountCode(const std::string &act);
    /**
     * @brief От строки
     * @param act
     */
    AmountCode(const char *act);
    /**
     * @brief Конструктор по типу
     * @brief И от типа
     * @param act
     */
    AmountCode(AmountCode_t t);

    /**
     * @brief Конструктор по-умолчанию
    */
    AmountCode(){}

};

class PassengerTypeElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    PassengerTypeElem(int codeI, const char *code,
                    const char *desc) throw()
    :CodeListData_t(codeI, code, code, desc, desc)
    {
    }

};

///@class PassengerType type of passenger
class PassengerType : public BaseTypeElemHolder<PassengerTypeElem>
{
public:
    typedef BaseTypeElemHolder<PassengerTypeElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap PassengerTypeMap;
    typedef std::list<PassengerTypeElem> listElems_t;

    ///@enum PassType_t
    ///@brief типы пассажиров
    enum PassType_t
    {
        Adult=1,
        Male,
        Female,
        Child,
        Infant_m, //infant male
        Infant_f, //infant female
        Infant,   //Infant
        Infant_num,// Infant, numeric code
        Infant_num_os, // Infant, numeric code, occupying seats
        Unaccomp_minor //Unaccompanied Minor
    };

    /**
     * @brief От строки
     * @param act
     */
    PassengerType(const std::string &act);
    /**
     * @brief От строки
     * @param act
     */
    PassengerType(const char *act);
    /**
     * @brief Конструктор по типу
     * @brief И от типа
     * @param act
     */
    PassengerType(PassType_t t);

    /**
     * @brief Конструктор по-умолчанию
    */
    PassengerType(){}
};

class AccompaniedIndicatorElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    AccompaniedIndicatorElem(int codeI, const char *code,
                    const char *desc) throw()
    :CodeListData_t(codeI, code, code, desc, desc)
    {
    }

};

///@class AccompaniedIndicator accompanied infant indicator
class AccompaniedIndicator : public BaseTypeElemHolder<AccompaniedIndicatorElem>
{
public:
    typedef BaseTypeElemHolder<AccompaniedIndicatorElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap AccompaniedIndicatorMap;
    typedef std::list<AccompaniedIndicatorElem> listElems_t;

    ///@enum Indicator_t
    ///@brief индикатор сопровождающего (ребёнка)
    enum Indicator_t
    {
        InfantWithoutSeat = 1,
        InfantWithSeat    = 2
    };

    /**
     * @brief От строки
     * @param act
     */
    AccompaniedIndicator(const std::string &act);
    /**
     * @brief От строки
     * @param act
     */
    AccompaniedIndicator(const char *act);
    /**
     * @brief Конструктор по типу
     * @brief И от типа
     * @param act
     */
    AccompaniedIndicator(Indicator_t t);

    /**
     * @brief Конструктор по-умолчанию
    */
    AccompaniedIndicator(){}
};

namespace BookingDesignator{
    /* по букве-коду подкласса получить его номер */
    int GetSubclsNum(unsigned char c);
    char GetSubclsCode(int i, Language lang=RUSSIAN);
} //BookingDesignator

/// @class BaseClassListElem - Базовый класс (отдельный элемент)
class BaseClassListElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    BaseClassListElem(int codeI, const char *lcode, const char *rcode,
                      const char *ldesc,
                      const char *rdesc) throw()
    :CodeListData_t(codeI, rcode, lcode, rdesc, ldesc)
    {
    }

    virtual ~BaseClassListElem(){}
};

/// @class BaseClass - Базовый класс
class BaseClass : public BaseTypeElemHolder<BaseClassListElem>
{
public:
    enum Type// Базовые классы
    {
        First=0, Business, Economy
    };
    typedef BaseTypeElemHolder<BaseClassListElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap BaseClassMap;
    typedef std::list< BaseClassListElem > listElems_t;

    BaseClass():TypeElemHolder()
    {
    }
    explicit BaseClass(const TypeElemHolder &base) :
            TypeElemHolder(base)
    {
    }
    explicit BaseClass(int t)
    :TypeElemHolder(t)
    {
    }

    explicit BaseClass(const char * rbd, Language l = ENGLISH);

//     static listElems_t subclassList();
};

std::ostream  &operator << (std::ostream &s,
                            const BaseClass &bclass);


class SubClassListElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
    BaseClass::Type BClass;
public:
    static const char *ElemName;
    SubClassListElem(int codeI, const char *rcode, const char *lcode, BaseClass::Type bcls) throw()
    :CodeListData_t(codeI, rcode, lcode, "", ""),BClass(bcls)
    {
    }

    /**
     * Базовый класс данного подкласса
     * @return
     */
    BaseClass baseClass() const { return BaseClass(BClass); }

    virtual ~SubClassListElem(){}
};

/// @class SubClass - подкласс
class SubClass : public BaseTypeElemHolder<SubClassListElem>
{
public:
    typedef BaseTypeElemHolder<SubClassListElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap SubClassMap;
    typedef std::list< SubClassListElem > listElems_t;

    enum Type
    {
        R=0,P,F,A,J,C,D,Z,  W,S,Y,B,H,K,L,M,N,Q,T,V,X,G,U,E,O,I
      /*|  First ||Business|| Economy                        |Business*/
    };
    /**
     * Default constructor
     */
    SubClass():TypeElemHolder(){}
    /**
     * Конструктор от int
     * @param scls
     */
    explicit SubClass(int scls):TypeElemHolder(scls){}
    /**
     * Конструктор от символа
     * @param scls
     */
    explicit SubClass(const char *c, Language l):
            TypeElemHolder(c,l)
    {
    }
    explicit SubClass(const std::string &c, Language l):
            TypeElemHolder(c,l)
    {
    }

    explicit SubClass(const char *c):
            TypeElemHolder(c)
    {
    }
    explicit SubClass(const std::string &c):
            TypeElemHolder(c)
    {
    }

    static const listElems_t &subclassList();
private:
    static listElems_t StaticSubClsList;
};

std::ostream  &operator << (std::ostream &s,
                            const SubClass &sclass);



class FreeTextTypeElem : public BaseTypeElem<int>
{
    std::string Qual;
    std::string Code;
protected:
    friend class FreeTextType;
    void setQual(const std::string &val) { Qual = val; }
    void setCode(const std::string &val) { Code = val; }
public:
    typedef BaseTypeElem<int> CodeListData_t;
    static const char *ElemName;
    FreeTextTypeElem(int codeI,
                     const char *qual,
                     const char *code,
                     const char *desc) throw()
    :CodeListData_t(codeI, code, code, desc, desc), Qual(qual), Code(code)
    {
    }
    /**
     * @brief Text Subject Qualifier
     * @return
     */
    const std::string &qual() const { return Qual; }
    /**
     * @brief Text info type
     * @return
     */
    virtual const char * code(Language = ENGLISH) const { return Code.c_str(); }

    virtual ~FreeTextTypeElem(){}
};

class FreeTextType : public BaseTypeElemHolder<FreeTextTypeElem>
{
public:
    static const unsigned FareCalcMaxLen = 1000;
    typedef BaseTypeElemHolder<FreeTextTypeElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap FreeTextTypeMap;

    enum FreeTextType_t
    {
        Unknown=0,
        LiteralText,
        TicketingTimeLimit,
        HeaderInfo,
        Telephone,
        TelephoneHome,
        TelephoneBusiness,
        FareCalc,
        AgnAirName,
        ServProvName,
        TchUniq,
        OriginalIssueInfo,
        Endorsement,
        FormOfPayment,
        PartyInfo,
        SponsorInfo,
        TrAgentTelephone,
        PaxSpecInfo,
        ServProviderLoc,
        FareAmount,
        NonSegRelatedItin,
        WholesalerAddress,
        TicketingRems,
        SupplierAgnInfo,
        TourNumber,
        InvoiceInfo,
        SpecRem,
        TchQrLinkPart,
        AvlRelatedText,
        ReferenceSellToken,
        MoreAvlToken,
        FareCalcReport,
        Remark,
        Osi,
        RfiscDescription,
        BookingPartyDetails,
        TicketNumber,
        EmailAddress,
        Commission
    };

    FreeTextType():TypeElemHolder()
    {
    }
    FreeTextType(const TypeElemHolder &base) :
        TypeElemHolder(base)
    {
    }
    FreeTextType(FreeTextType_t t):TypeElemHolder(t)
    {
    }
    FreeTextType(const std::string &code, const std::string &qual, bool strict=false);

private:
    struct find_by_code_qual : public BaseTypeElemHolder<FreeTextTypeElem>::userFind
    {
        std::string code;
        std::string qual;

        find_by_code_qual(std::string c, std::string q): code(c), qual(q){}
        bool operator () (const TypeElemHolder::TypesMap::value_type &e) const
        {
            return (e.second.qual() == qual &&
                    e.second.CodeListData_t::code(ENGLISH) == code);
        }

        virtual std::string dataName() const { return "Text qualifier and info type"; }
        virtual std::string dataStr()  const { return code + ":" + qual; }
        virtual ~find_by_code_qual(){}
    };
};

DECLARE_CODE_SET_ELEM(HistCodeElem)

    HistCodeElem(int codeI, const char *code,
                  const char *edifact_code,
                  const char *desc) throw()
        :CodeListData_t(codeI, code,/*rcode*/ code /*lcode*/, desc, desc),
          _edi_code(edifact_code)
    {
    }

    const std::string &ediCode() const { return _edi_code; }

private:
    std::string _edi_code;
};

///@brief Коды в истории билета
DECLARE_CODE_SET(HistCodeHolder, HistCodeElem)

    HistCodeHolder(const TypeElemHolder &code, const TypeElemHolder &/*subcode*/) :
            TypeElemHolder(code)
    {
    }

    static HistCodeHolder initByEdiCode(const std::string &ediCode)
    {
        return initByUserSearch(find_by_edicode(ediCode));
    }

private:
struct find_by_edicode : public BaseTypeElemHolder<HistCodeElem>::userFind
{
    std::string code;
    find_by_edicode(const std::string &c): code(c){}
    bool operator () (const TypeElemHolder::TypesMap::value_type &e) const
    {
        return e.second.ediCode() == code;
    }

    virtual std::string dataName() const { return "edifact code"; }
    virtual std::string dataStr()  const { return code;   }
    virtual ~find_by_edicode(){}
};

}; // HistCode

class HistCode
{
    HistCodeHolder _HistCode;
    HistCodeHolder _HistSubCode;
    struct boolean {int i;};
public:
    /**
     * @enum HistCodes_t
     * @brief Коды в истории билета
    */
    enum HistCodes_t
    {
        Issue        = 1,
        Exchange     = 2,
        Refund       = 3,
        Void         = 4,
        Print        = 5,
        PrintExch    = 6,
        RefundCancel = 7,
        SystemCancel = 8,
        ChangeStatus = 9,
        ReservChange = 10,
        ExchangeIssue= 11,
        VoidExchange = 12,
        PcAuth       = 13,
        /** Receiving  Airport Control (TKCUAC)*/
        ReceiveUAC   = 14,
        /** Receiving  Airport Control by our request*/
        ReceiveAC    = 15,
        /** Update SAC */
        SetSac       = 16,
        /** Revalidation */
        Revalidation = 17,

        // EMD
        IssueEmd        = 18,
        SystemUpdateEmd = 19,
        ExchangeEmd     = 20,
        RefundEmd       = 21,
        SystemCancelEmd = 22,
        RefundCancelEmd = 23,
        VoidExchangeEmd = 24,
        VoidEmd         = 25,
        ChangeStatusEmd = 26,
        ExchangeIssueEmd= 27,
        ReceiveUACEmd   = 28,
        ReceiveACEmd    = 29,
        SetSacEmd       = 30
    };

    ///@brief Доступ к полям элемента
    const HistCodeElem *operator -> () const
    {
        return _HistCode.operator -> ();
    }

    /**
     * @brief Проверка на инициализированность
     */
    operator int boolean::*() const
    {
        if(_HistCode)
            return &boolean::i;
        else
            return 0;
    }

    HistCode() {}
    HistCode(const int code, const int subcode = 0)
        : _HistCode(code)
    {
        if(subcode)
            _HistSubCode = HistCodeHolder(subcode);
    }
    HistCode(const std::string &code, const std::string &scode = "")
        : _HistCode(code)
    {
        if(!scode.empty())
            _HistSubCode = HistCodeHolder(scode);
    }

    const HistCodeHolder &subCode() const { return _HistSubCode; }
};

class HistSubCode
{
    HistCodeHolder _HistSubCode;
    struct boolean {int i;};
public:
    HistSubCode(const int code) : _HistSubCode(code) {}
    HistSubCode() {}
    HistSubCode(const HistCodeHolder &hsc);
    HistSubCode(const std::string &code) : _HistSubCode(code) {}

    static HistSubCode getByEdiCode(const std::string &edi_code);


    ///@brief Доступ к полям элемента
    const HistCodeElem *operator -> () const
    {
        return _HistSubCode.operator ->();
    }

    /**
     * @brief Проверка на инициализированность
     */
    operator int boolean::*() const
    {
        if(_HistSubCode)
            return &boolean::i;
        else
            return 0;
    }
};

class Baggage
{
public:
    enum Baggage_t
    {
        WeightKilo=1,
        WeightPounds=2,
        NumPieces=3,   // Number of pieces
        Nil=4          // NIL - no luggage
    };
    unsigned Allowance;
    std::string Charge;
    std::string Measure;
    Baggage_t ChargeQualifier;

    typedef std::map<std::string, Baggage_t> chrgQMap;
    static chrgQMap *charge_measure;
    static chrgQMap &get_charge_measure()
    {
        if(!charge_measure)
        {
            charge_measure = new chrgQMap;
            Baggage::filldata();
        }
        return *charge_measure;
    }
    static void filldata();
    static Baggage_t getQualifier(const std::string &charge,
                                          const std::string &measure);
    public:
        Baggage(unsigned allowance,
                const std::string &charge,
                const std::string &measure);

        inline unsigned allowance() const
        {
            return Allowance;
        }
        inline unsigned quantity() const
        {
            return allowance();
        }
        inline const std::string &charge() const
        {
            return Charge;
        }
        inline const std::string &measure() const
        {
            return Measure;
        }
        inline Baggage_t chargeQualifier() const
        {
            return ChargeQualifier;
        }

        const char * code(Language l=RUSSIAN) const;

        virtual bool operator == (const Baggage &lugg) const
        {
            if(lugg.allowance() == this->allowance() &&
                lugg.chargeQualifier()== this->chargeQualifier())
            {
                return true;
            } else {
                return false;
            }
        }

        virtual ~Baggage(){}
};


class MonetaryTypeElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
    std::string FreeText;
public:
    static const char *ElemName;
    static const size_t MaxFreeTextLength = 18; // by PADIS 04.1

    MonetaryTypeElem(int codeI, const char *rcode, const char *lcode,
                     const char *rdesc, const char * ldesc) throw()
            :CodeListData_t(codeI, rcode, lcode, rdesc, ldesc)
    {
    }


    void setFreeText(const std::string &txt);

    virtual const char * code(Language l = ENGLISH) const
    {
        return (FreeText.empty()) ? BaseTypeElem<int>::code(l) : FreeText.c_str();
    }
};


/// @class MonetaryType
/// @brief Monetary Types.
class MonetaryType : public BaseTypeElemHolder<MonetaryTypeElem>
{
    MonetaryType(const std::string & typestr, int);
public:
    ///@typedef BaseTypeElemHolder<MonetaryTypeElem> TypeElemHolder;
    typedef BaseTypeElemHolder<MonetaryTypeElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap MonetaryTypeMap;
    //typedef std::list<MonetaryTypeElem> listElems_t;

    ///@enum Type_t
    ///@brief Monetary Types.
    enum Type_t
    {
        /// free - no money
        Free=1,
        /// secret tariff
        It,
        /// Bulk tariff
        Bt,
        Bulk,
        /// Charter
        Charter,
        /// No ADC
        NoADC,
        NoADC_ns,

        NoFARE,

        // whatever you want!
        FreeText
    };

    /**
     * @brief Пустой конструктор
     * if(*this) returns false
     */
    MonetaryType();
    /**
     * @brief Конструктор от строки
     * @param typestr
     */
    MonetaryType(const std::string &typestr);
    /**
     * @brief Конструктор от типа
     * @param typ
     */
    MonetaryType(Type_t typ);
private:
//  static listElems_t StaticInitStatList;
};


/// @class PricingTicketingIndicator
/// @brief Индикаторы прайсинга/тикетинга (Non-refundable,Non-interlineable...)
class PricingTicketingIndicatorElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    PricingTicketingIndicatorElem(int codeI, const char *code,
                                  const char *desc) throw()
    :CodeListData_t(codeI, code, code, desc, desc)
    {
    }
};

class PricingTicketingIndicator : public BaseTypeElemHolder<PricingTicketingIndicatorElem>
{
public:
    typedef BaseTypeElemHolder<PricingTicketingIndicatorElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap FopIndicatorMap;
    typedef std::list<PricingTicketingIndicatorElem> listElems_t;
    ///@enum PricingTicketingIndicator_t
    ///@brief вариации прайсинга/тикетинга
    enum PricingTicketingIndicator_t
    {
        /// Невозвратный
        NonRefundable,
        /// Неинтерлайнбельный (лучше б не переводил:)
        NonInterlineable,
        ///
        NonEndorsable
    };
    /**
     * @brief От строки
     * @param act
     */
    PricingTicketingIndicator(const std::string &act);
    /**
     * @brief От строки
     * @param act
     */
    PricingTicketingIndicator(const char *act);
    /**
     * @brief От типа
     * @param act
     */
    PricingTicketingIndicator(PricingTicketingIndicator_t act);

    PricingTicketingIndicator() {}
};


/// @class PricingTicketingIndicator
/// @brief Признак продажи (upsell, wci)
class PricingTicketingSellTypeElem : public BaseTypeElem<int>
{
    typedef BaseTypeElem<int> CodeListData_t;
public:
    static const char *ElemName;
    PricingTicketingSellTypeElem(int codeI, const char *code,
                                  const char *desc) throw()
    :CodeListData_t(codeI, code, code, desc, desc)
    {
    }
};

class PricingTicketingSellType : public BaseTypeElemHolder<PricingTicketingSellTypeElem>
{
public:
    typedef BaseTypeElemHolder<PricingTicketingSellTypeElem> TypeElemHolder;
    typedef TypeElemHolder::TypesMap FopIndicatorMap;
    typedef std::list<PricingTicketingSellTypeElem> listElems_t;
    ///@enum PricingTicketingSellType_t
    ///@brief пути продажи
    enum PricingTicketingSellType_t
    {
        /// докупка
        Upsell,
        /// услуга на web-регистрации
        WebCheckinSale
    };
    /**
     * @brief От строки
     * @param act
     */
    PricingTicketingSellType(const std::string &act);
    /**
     * @brief От строки
     * @param act
     */
    PricingTicketingSellType(const char *act);
    /**
     * @brief От типа
     * @param act
     */
    PricingTicketingSellType(PricingTicketingSellType_t act);

    PricingTicketingSellType() {}
};


} //namespace Ticketing

#endif /*_ETICK_TICK_DATA_H_*/
