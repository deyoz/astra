#ifndef _ETICK_TICKET_H_
#define _ETICK_TICKET_H_

#include <string>
#include <list>
#include <vector>
#include <numeric>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#define MY_STDLOG "ROMAN", __FILE__, __LINE__

#include "etick/exceptions.h"
#include "etick/tick_data.h"
#include "etick/tick_doctype.h"

#include <serverlib/lngv.h>

#include "etick/etick_msg.h"

namespace Ticketing{

void nologtrace(int lv, const char* n, const char* f, int l, const char* s);

template <class E> struct TraceElement : public std::unary_function<E, void>
{
    int Level;
    const char *Nick;
    const char *File;
    int Line;
    TraceElement(int level,const char *nick, const char *file, int line)
    :Level(level),Nick(nick),File(file),Line(line) { }
    void operator () (const E &e) const{
        e.Trace(Level, Nick, File, Line);
    }
};

/**
 * @brief Compares two lists
 * @param list1
 * @param list2
 * @return true if equal
 */
template<class T> bool cmplists(const std::list<T> &list1,
                                const std::list<T> &list2)
{
    if(list1.size() != list2.size())
    {
        return false;
    }
    // копируем список
    std::list<T> list2copy = list2;
    for(typename std::list<T>::const_iterator iter = list1.begin(); iter !=
        list1.end(); iter ++)
    {
        typename std::list<T>::iterator found =
                std::find(list2copy.begin(), list2copy.end(), *iter);
        if(found == list2copy.end())
        {
            return false;
        }
        // удаляем то, что уже нашли
        list2copy.erase(found);
    }

    return true;
}

namespace ItinDateStream{
struct ItinDate
{
    const boost::gregorian::date& d;
};
struct ItinTime
{
    const boost::posix_time::time_duration& t;
};
std::ostream & operator << (std::ostream& os, const ItinDate&);
std::ostream & operator << (std::ostream& os, const ItinTime&);
}

class BaseOrigOfRequest;
class BaseCoupon_info;
class BaseLuggage;
class BaseFormOfId;
class BaseMonetaryInfo;
class BaseTaxDetails;
class BaseResContrInfo;
class BaseFrequentPass;
class CommonTicketData;
class TourCode_t;
template<typename LuggageT> class BaseItin;
template<
        class Coupon_info,
        class ItinT,
        class FrequentPass,
        class FreeTextInfo
        > class BaseCoupon;
/**
 * @brief Печать содержимого BaseOrigOfRequest
 * @param os поток
 * @param org
 * @return
 */
std::ostream & operator << (std::ostream& os, const BaseOrigOfRequest &org);

/**
 * @brief Печать содержимого BaseResContrInfo
 * @param os
 * @param rci
 * @return
 */
std::ostream & operator << (std::ostream& os, const BaseResContrInfo &rci);

/**
 * @brief Печать содержимого BaseCoupon_info
 * @param os
 * @param time
 * @return
 */
std::ostream & operator << (std::ostream& os, const BaseCoupon_info &cpn);

/**
 * @brief Печать содержимого BaseLuggage
 * @param os
 * @param cpn
 * @return
 */
std::ostream & operator << (std::ostream& os, const BaseLuggage &lugg);

/**
 * @brief Печать содержимого BaseFormOfId
 * @param os
 * @param lugg
 * @return
 */
std::ostream & operator << (std::ostream& os, const BaseFormOfId &foid);

/**
 * @brief Печать содержимого BaseMonetaryInfo
 * @param os
 * @param monin
 * @return
 */
std::ostream & operator << (std::ostream& os, const BaseMonetaryInfo &monin);

/**
 * @brief Печать содержимого BaseTaxDetails
 * @param os
 * @param taxd
 * @return
 */
std::ostream & operator << (std::ostream &os, const BaseTaxDetails &taxd);

/**
 * @brief Печать содержимого CommonTicketData
 * @param os
 * @param ctd
 * @return
 */
std::ostream & operator << (std::ostream &os, const CommonTicketData &ctd);

/**
 * @brief Печать содержимого TourCode_t
 * @param os
 * @param ctd
 * @return
 */
std::ostream & operator << (std::ostream &os, const TourCode_t &ctd);

/**
 * @brief Печать содержимого BaseItin
 * @param os
 * @param Itin
 * @return std::ostream
 */
template <typename lugg>
        std::ostream & operator << (std::ostream& os, const BaseItin<lugg> &itin)
{
    using namespace ItinDateStream;
    //ПЛ-2427  Э 25ЯНВ06 ШРМ ПЛК НК1   1900 2100
    os << itin.airCode()
            << (itin.airCodeOper().empty()?"":(":"+itin.airCodeOper())) << "-"
            << itin.flightnum() << ":" << itin.flightnumOper() << " "
            << itin.classCodeStr(RUSSIAN) << " "
            << ItinDate{itin.date1()} << ":" << ItinTime{itin.time1()} << "-"
            << ItinDate{itin.date2()} << ":" << ItinTime{itin.time2()} << " "
            << itin.depPointCode() << " "
            << itin.arrPointCode() << " "
            << itin.rpistat() << " "
            << itin.fareBasis();

    os <<  "; Not Valid before " <<
            itin.validDates().first << ", After " << itin.validDates().second;

    os << " Luggage: " << itin.luggage();
    return os;
}

/**
 * @brief Печать содержимого BaseTaxDetails
 * @param os
 * @param taxd
 * @return
 */
std::ostream & operator << (std::ostream &os, const BaseFrequentPass &fpass);

/**
 * @brief Печать содержимого BaseCoupon
 * @param os
 * @param Itin
 * @return
 */
template<
        class Coupon_info,
        class ItinT,
        class FrequentPass,
        class FreeTextInfo
        >
std::ostream & operator << (std::ostream& os,
                            const BaseCoupon<Coupon_info,ItinT,FrequentPass,FreeTextInfo> &coupon)
{
    os << "CouponInfo: " << coupon.couponInfo();
    if(coupon.haveItin())
        os << ";\n Itin: " << coupon.itin();
    if(!coupon.lfreqPass().empty())
    {
        os << ";\n Freq: " << coupon.lfreqPass().front();
    }
    return os;
}

/**
 * @class BaseCoupon_info
 * @brief Информация по купону
*/
class BaseCoupon_info
{
    unsigned  Num;       // Номер купона
    CouponStatus Status; // Статус купона
    TicketMedia Media;   // Носитель (бумага или электронный)

    // Mutable, тк может инициализир. при переходе купона в конечный статус
    mutable std::string Sac; // Код авторизации

    boost::optional<CpnStatAction::CpnStatAction_t> ActionCode;
    boost::optional<unsigned> ConnectedCpnNum;
public:
    /**
     * @brief Traces current element
     * @param level
     * @param nick
     * @param file
     * @param line
     */
    virtual void Trace(int level,const char *nick, const char *file, int line) const;

    /**
     * @brief Base constructor
     * @param n
     * @param stat
     * @param med
     * @param sac
     */
    BaseCoupon_info(
                unsigned n,
                const CouponStatus &stat,
                const TicketMedia &med = TicketMedia::Electronic,
                const std::string &sac="");

    /**
     * @brief Settlement authorization code
     * @return
     */
    virtual const std::string & sac ()  const { return Sac; }
    /**
     * @brief set a new Settlement authorization code
     * @param sac_
     */
    virtual void setSac(const std::string &sac_) const {Sac = sac_;}
    /**
     * @brief Coupon media
     * @return
     */
    virtual const TicketMedia&  media() const { return Media; }
    /**
     * @brief Coupon number in a ticket
     * @return
     */
    virtual unsigned            num ()  const { return Num; }
    /**
     * @brief Coupon status
     * @return
     */
    virtual const CouponStatus& status() const { return Status; }

    /**
     * @brief Set new coupon media
     * @param m
     */
    virtual void setMedia(const TicketMedia& m) { Media = m; }
    /**
     * @brief Set new coupon status
     * @param st
     */
    virtual void setStatus(const CouponStatus &st) { Status = st; }

    virtual void setActionCode(const CpnStatAction::CpnStatAction_t actCode)
    {
        ActionCode = actCode;
    }

    virtual boost::optional<CpnStatAction::CpnStatAction_t> actionCode() const
    {
        return ActionCode;
    }

    virtual void setConnectedCpnNum(unsigned num) { ConnectedCpnNum = num; }

    virtual boost::optional<unsigned> connectedCpnNum() const { return ConnectedCpnNum; }

    /**
     * @brief Compares two coupons
     * @param ci
     * @return
     */
    virtual bool operator == (const BaseCoupon_info &ci) const;
    virtual ~BaseCoupon_info(){}
};

///@class BaseLuggage
///@brief Норма бесплатного багажа
class BaseLuggage
{
    boost::shared_ptr<Baggage> Bagg;
public:
    BaseLuggage()
    {
    }

    BaseLuggage(
            int quant,
            const std::string &chrg,
            const std::string &meas)
    :
            Bagg(new Baggage(quant, chrg, meas))
    {
    }

    virtual void Trace(int level,const char *nick, const char *file, int line) const;

    bool haveLuggage() const
    {
        return Bagg.get()!=NULL;
    }
    /**
     * @brief Проверка на равенство
     * @param lugg
     * @return true if equals
     */
    virtual bool operator == (const BaseLuggage &lugg) const;
    /**
     * @brief Доступ к полям Baggage
     * @return const pointer on Baggage
     */
    virtual const Baggage * operator -> () const;
    virtual ~BaseLuggage(){}
};

class BaseFormOfId
{
    FoidType Type;
    std::string Number;
    std::string Owner;
public:
    static const unsigned int MaxNumberLen = 25;
    static const unsigned int MaxOwnerLen  = 4;
    /**
     * @brief BaseFormOfId
     * @param t
     * @param num
     * @param own
     * @param rct reservation control type (if foids from RCI)
     */
    BaseFormOfId(const FoidType &t,
                 const std::string &num,
                 const std::string &own="")
         :Type(t),
          Number(num),
          Owner(own)
    {
    }

    const FoidType &type() const { return Type; }
    const std::string &number() const { return Number; }
    const std::string &owner()  const { return Owner;  }

    virtual bool operator == (const BaseFormOfId &foid) const;
    virtual void Trace(int level,const char *nick, const char *file, int line) const;

    static void Trace(int level,const char *nick, const char *file, int line,
                      const std::list<BaseFormOfId> &lfoid);
    virtual ~BaseFormOfId(){}
};

class BaseFrequentPass
{
    // Частолетающий пассажир
    std::string Ticknum;     // Номер билета к которому привязан
    unsigned Cpnnum;    // Номер купона к которому привязан (мб 0)
    std::string CompCode;    // Компания выдавшая
    std::string Docnum;      // Номер документа
public:
    BaseFrequentPass(
                const std::string & tnum,
                unsigned cpn,
                const std::string & comp,
                const std::string & doc)
    :
        Ticknum(tnum),
        Cpnnum(cpn),
        CompCode(comp),
        Docnum(doc)
    {
    }

	BaseFrequentPass(
                const std::string &comp,
                const  std::string &doc)
    :
            Cpnnum(0),
            CompCode(comp),
            Docnum(doc)
    {
    }

    virtual const std::string &compCode()const  { return CompCode;}
    virtual const std::string &docnum () const  { return Docnum;  }
    virtual const std::string &ticknum() const  { return Ticknum; }
    virtual unsigned      cpnnum () const  { return Cpnnum;  }

    virtual void Trace(int level,const char *nick, const char *file, int line) const;
    template <class FreqT>
    static void Trace(int level,const char *nick, const char *file, int line,
                      const std::list<FreqT> lFreq)
    {
        for_each(lFreq.begin(),lFreq.end(),
                 TraceElement<FreqT>(level,nick,file,line));
    }

    virtual bool operator == (const BaseFrequentPass &freq) const;
    virtual ~BaseFrequentPass(){}
};

///@class BaseItin
///@brief Базовый класс с информацией о маршруте
template <class LuggageT>
class BaseItin
{
    // Маршрут
    std::string Ticknum; // Номер билета к которому привязан

    std::string AirCode;     // Компания
    std::string OperAirCode; // Компания, выполняющая перевозку (если отлична от Airline)

    int Flightnum;   // Номер рейса
    int OperFlightnum; // Номер рейса компании, выполняющей перевозку
    SubClass Class;       // reservation booking designator

    boost::gregorian::date Date1;      // Дата вылета
    boost::posix_time::time_duration Time1;      // Время вылета
    boost::gregorian::date Date2;      // Дата прибытия
    boost::posix_time::time_duration Time2;      // Время прибытия

    std::string DepPointCode;    // Точка отрыва
    std::string ArrPointCode;    // Точка приземления (если все удачно)

    std::pair<boost::gregorian::date, boost::gregorian::date> ValidDates;

    ItinStatus RpiStat; // Статус маршрута (OK, NS, ...)
    std::string FareBasis;

    int Version;     // Версия

    LuggageT Lugg;    // Норма бесплатного багажа
public:
    static const unsigned FareBasisLen = 15;

    BaseItin(const std::string &tnum,
         const std::string &air,
         const std::string &oper_air,
         int flight,
         int flightOper,
         const SubClass &cls,
         const boost::gregorian::date  &date1,
         const boost::posix_time::time_duration &time1,
         const boost::gregorian::date  &date2,
         const boost::posix_time::time_duration &time2,
         const std::string &depPoint,
         const std::string &arrPoint,
         const std::pair<boost::gregorian::date, boost::gregorian::date> &VldDates,
         const ItinStatus &rpiStat,
         const std::string &Fare,
         int ver,
         const LuggageT &lugg)
    :
        Ticknum(tnum),
        AirCode(air),
        OperAirCode(oper_air),
        Flightnum(flight),
        OperFlightnum(flightOper),
        Class(cls),
        Date1(date1),
        Time1(time1),
        Date2(date2),
        Time2(time2),
        DepPointCode(depPoint),
        ArrPointCode(arrPoint),
        ValidDates(VldDates),
        RpiStat(rpiStat),
        FareBasis(Fare),
        Version(ver),
        Lugg(lugg)
    {
    }


    typedef boost::shared_ptr< BaseItin > SharedPtr;

    virtual const std::string & airCode()     const { return AirCode; }
    virtual const std::string & airCodeOper() const { return OperAirCode; }

    virtual void setAirCode(const std::string &AC)      { AirCode = AC; }
    virtual void setAirCodeOper(const std::string &OAC) { OperAirCode = OAC; }

    virtual int flightnum () const         { return Flightnum; }
    virtual int flightnumOper () const         { return OperFlightnum; }

    virtual const boost::gregorian::date  &date1() const     { return Date1; }
    virtual const boost::posix_time::time_duration &time1() const     { return Time1; }
    virtual const boost::gregorian::date  &date2() const     { return Date2; }
    virtual const boost::posix_time::time_duration &time2() const     { return Time2; }

    virtual const boost::gregorian::date  &depDate() const     { return date1(); }
    virtual const boost::posix_time::time_duration &depTime() const { return time1(); }
    /**
     * @brief departure date/time
     * @return
     */
    virtual boost::posix_time::ptime   depDateTime() const
    {
        return boost::posix_time::ptime(depDate(), depTime());
    }
    /**
     * @brief arrival date/time
     * @return
     */
    virtual boost::posix_time::ptime   arrDateTime() const
    {
        return boost::posix_time::ptime(arrDate(), arrTime());
    }
    virtual const boost::gregorian::date  &arrDate() const     { return date2(); }
    virtual const boost::posix_time::time_duration &arrTime() const { return time2(); }

    virtual const std::pair<boost::gregorian::date, boost::gregorian::date> &validDates() const { return ValidDates; }
    virtual std::string depPointCode()     const { return DepPointCode; }
    virtual std::string arrPointCode()     const { return ArrPointCode; }

    virtual const SubClass & rbd() const                  { return Class; }
    virtual bool haveRbd() const             { return not not Class; }

    virtual const char *classCodeStr(Language lang=ENGLISH, const char *nvl=" ") const
    {
        return (haveRbd()?Class->code(lang):nvl);
    }

    virtual const ItinStatus & rpistat() const { return RpiStat; }
    virtual const std::string &fareBasis() const         { return FareBasis; }
    virtual int version() const                     { return Version; }
    virtual const LuggageT &luggage() const          { return Lugg; }
    virtual const std::string  &ticknum() const          { return Ticknum; }

    /**
     * @brief return true if open date segment
     * @return
     */
    virtual bool isOpenDate() const {return depDate().is_special(); }

    virtual void setTicknum(const std::string &tnum) { Ticknum = tnum; }
    virtual void setRpiStat(const ItinStatus &rpi) { RpiStat = rpi; }
    virtual void setNValidBefore(const boost::gregorian::date &nvb) { ValidDates.first = nvb; }
    virtual void setNValidAfter(const boost::gregorian::date &nva) { ValidDates.second = nva; }
    virtual void setLuggage(const LuggageT &l) { Lugg = l; }
    virtual void setFareBasis(const std::string &fb) { FareBasis = fb; }
    virtual void setVersion(unsigned v) { Version = v; }
    /**
     * @brief задать город/порт вылета
     * @param depP
     */
    virtual void setDepPoint(const std::string &depP)
    {
        DepPointCode = depP;
    }
    /**
     * @brief задать город/порт прилета
     * @param arrP
     */
    virtual void setArrPoint(const std::string &arrP)
    {
        ArrPointCode = arrP;
    }
    /**
     * @brief Задать дату вылета
     * @param ddate
     */
    virtual void setDepDate(const boost::gregorian::date  &ddate)
    {
        Date1 = ddate;
    }
    /**
     * @brief Задать время вылета
     * @param dtime
     */
    virtual void setDepTime(const boost::posix_time::time_duration &dtime)
    {
        Time1 = dtime;
    }

    /**
     * @brief Задать дату прилета
     * @param adate
     */
    virtual void setArrDate(const boost::gregorian::date &adate)
    {
        Date2 = adate;
    }

    /**
     * @brief Задать время прилета
     * @param atime
    */
    virtual void setArrTime(const boost::posix_time::time_duration &atime)
    {
        Time2 = atime;
    }
    /**
     * @brief Задать номер рейса маркетинг перевозчика
     * @param flnum
     */
    virtual void setFlightnum(int flnum)
    {
        Flightnum = flnum;
    }

    /**
     * @brief Задать номер рейса опер. перевозчика
     * @param flnum
     */
    virtual void setOperFlightnum(int flnum)
    {
        OperFlightnum = flnum;
    }
    /**
     * @brief Задать подкласс обслуживания
     * @param sc
     */
    virtual void setRbd(const SubClass &sc)
    {
        Class = sc;
    }
#define __tst__ //LogTrace(5, "ROMAN", __FILE__, __LINE__) << " <<tst>> "
    virtual bool operator == (const BaseItin &it) const
    {
        if((__tst__ it.airCode() == this->airCode()) &&
           (__tst__ it.airCodeOper() == this->airCodeOper()) &&
           (__tst__ it.flightnum() == this->flightnum()) &&
           (__tst__ it.date1() == this->date1()) &&
           (__tst__ it.time1() == this->time1()) &&
           (__tst__ it.date2() == this->date2()) &&
           (__tst__ it.time2() == this->time2()) &&
           (__tst__ it.depPointCode() == this->depPointCode()) &&
           (__tst__ it.arrPointCode() == this->arrPointCode()) &&
           (__tst__ it.rbd() == this->rbd()) &&
           (__tst__ it.validDates() == this->validDates()) &&
           (__tst__ it.rpistat() == this->rpistat()) &&
           (__tst__ it.fareBasis() == this->fareBasis()) &&
           (__tst__ it.luggage() == this->luggage()) &&
           (__tst__ it.ticknum() == this->ticknum()))
        {
            nologtrace(6, MY_STDLOG, "Equal Itin's");
            return true;
        } else {
            nologtrace(6, MY_STDLOG, "Different Itin's");
//            LogTrace(6,"ROMAN",__FILE__,__LINE__) << "left itin: " << it <<
//                    ";\nright itin: " << *this;
            return false;
        }
    }
#undef __tst__

    virtual void Trace(int level,const char *nick, const char *file, int line) const
    {
        std::ostringstream os;
        os << *this;
        nologtrace(level,nick,file,line, os.str().c_str());
    }

    virtual ~BaseItin(){}
};

class BaseFreeTextInfo
{
    // Iteractive free text
    unsigned Num;       // Номер по порядку для данного уровня
    unsigned Level;     // Уровень (0-применим ко всему сообщ, 1 - к пассажиру, 2-к билету, 3-к купону)
    FreeTextType FTType;

    std::vector<std::string> Text;    // TEXT
    mutable std::string FullText;

    std::string TickNum;         // Номер билета к которому привязан
    unsigned Coupon;        // Номер купона к которому привязан
    std::string Status;     // status, coded. IFT+4:15:1+TEXT' where 1 is status.

    void check() const;
public:
    BaseFreeTextInfo(const FreeTextType &tp,
                     const std::string &txt="",
                     const std::string &tick="",
                     unsigned cpn=0)
    :
        Num(0),
        Level(0),
        FTType(tp),
        TickNum(tick),
        Coupon(cpn)
    {
        addText(txt);
    }

    BaseFreeTextInfo(
                unsigned nm,
                unsigned lvl,
                const FreeTextType &tp,
                const std::string &txt,
                const std::string &tick="",
                unsigned cpn=0)
    :
        Num(nm),
        Level(lvl),
        FTType(tp),
        TickNum(tick),
        Coupon(cpn)
    {
        addText(txt);
    }

    BaseFreeTextInfo(
                unsigned nm,
                unsigned lvl,
                const FreeTextType &tp,
                const std::vector<std::string> &txt,
                const std::string &tick = std::string(),
                unsigned cpn=0);

    virtual const std::string &qualifier()         const { return FTType->qual(); }
    virtual std::string             type()         const { return FTType->code(); }
    virtual const FreeTextType &fTType()           const { return FTType; }
    virtual const std::vector<std::string> &text() const { return Text; }
    virtual const std::string &text(unsigned i)    const { return Text[i]; }
    virtual unsigned numParts()             const { return Text.size(); }
    virtual unsigned num()                  const { return Num; }
    virtual unsigned level()                const { return Level; }
    virtual const std::string &tickNum()    const { return TickNum; }
    virtual unsigned coupon()               const { return Coupon; }
    virtual const std::string status()      const { return Status; }
    virtual const std::string &fullText()   const;
    virtual void addText(const std::string &txt);
    virtual void setText(const std::string &txt);
    virtual void setStatus(const std::string &status);

    virtual void Trace(int level,const char *nick, const char *file, int line) const;

    template<class FreeTxtT>
    static void Trace(int level,const char *nick, const char *file, int line,
                      const std::list<FreeTxtT> &lIft)
    {
        for_each(lIft.begin(),lIft.end(),
                 TraceElement<FreeTxtT>(level,nick,file,line));
    }

    virtual bool operator == (const BaseFreeTextInfo &fti) const;

    virtual ~BaseFreeTextInfo(){}
};

template<
        class Coupon_info,
        class ItinT,
        class FrequentPass,
        class FreeTextInfo
        >
class BaseCoupon
{
    Coupon_info Ci;                 // Инфа о Купоне
    typename ItinT::SharedPtr Itin_; // Маршрут  (в списке всегда один элемент)
    std::list<FrequentPass> lFreqPass;   // Номер частолетающего пассажира (в списке всегда один элемент)
    std::list<FreeTextInfo> lIft;        // Свободный текст

    std::string Ticknum;            // Номер билета к которому привязан
public:
    BaseCoupon(const Coupon_info &ci,
               const ItinT &i,
               const std::list<FrequentPass> &fPass,
               const std::string &tnum)
    :
        Ci(ci),
        Itin_(new ItinT(i)),
        lFreqPass(fPass),
        Ticknum(tnum)
    {
    }

    BaseCoupon(const Coupon_info &ci,
               const ItinT &i,
               const std::string &tnum)
    :
        Ci(ci),
        Itin_(new ItinT(i)),
        Ticknum(tnum)
    {
    }

    BaseCoupon(const Coupon_info &ci,
               const std::string &tnum)
    :
        Ci(ci),
        Ticknum(tnum)
    {
    }

    virtual bool haveItin()     const    { return Itin_.get()!=NULL;    }
    virtual const ItinT &itin() const
    {
        if(this->haveItin()){
            return *Itin_.get();
        } else {
            throw TickExceptions::tick_fatal_except(MY_STDLOG, "Itinerary not found");
        }
    }
    virtual void resetItin(typename ItinT::SharedPtr ptr = typename ItinT::SharedPtr())
    {
        Itin_ = ptr;
    }
    virtual typename ItinT::SharedPtr itinPtr()
    {
        return Itin_;
    }
    virtual const typename ItinT::SharedPtr itinPtr() const
    {
        return Itin_;
    }
    virtual ItinT &itin()
    {
        if(this->haveItin()){
            return *Itin_.get();
        } else {
            throw TickExceptions::tick_fatal_except(MY_STDLOG, "Itinerary not found");
        }
    }
    virtual const std::list<FrequentPass> &lfreqPass()  const { return lFreqPass; }
    virtual std::list<FrequentPass> &lfreqPass()  { return lFreqPass; }
    virtual const std::list<FreeTextInfo> &lift() const { return lIft; }
    virtual std::list<FreeTextInfo> &lift() { return lIft; }
    virtual const Coupon_info &couponInfo() const { return Ci; }
    virtual Coupon_info &couponInfo() { return Ci; }
    virtual const std::string &ticknum () const { return Ticknum; }

    virtual void Trace(int /*level*/,const char * /*nick*/, const char* /*file*/, int /*line*/) const
    {
//        LogTrace(level,nick,file,line) << (*this);
    }

    template<class CouponT>
    static void Trace(int level,const char *nick, const char *file, int line,
                              const std::list<CouponT> &lCoupon)
    {
        for_each(lCoupon.begin(),lCoupon.end(),
                 TraceElement<CouponT>(level,nick,file,line));
    }

    virtual bool operator == (const BaseCoupon &cpn) const
    {
        if(cpn.haveItin() == this->haveItin() &&
           (!cpn.haveItin() || (cpn.itin() == this->itin())) &&
           cpn.lfreqPass() == this->lfreqPass() &&
           cpn.couponInfo() == this->couponInfo() &&
           cpn.ticknum() == this->ticknum() ){

            nologtrace(6, MY_STDLOG, "Equal Coupons");
            return true;
        } else {
            nologtrace(6, MY_STDLOG, "Different Coupons");
            return false;
        }
    }

    void setFreqPass(std::list<FrequentPass> lFreq)
    {
        lFreqPass = lFreq;
    }

   /**
    * @brief clone current class instance
    * @return new BaseCoupon
    */
    virtual BaseCoupon *clone() const
    {
        if(haveItin())
            return new BaseCoupon<Coupon_info,ItinT,FrequentPass,FreeTextInfo>
                    (couponInfo(), itin(),lfreqPass(), ticknum());
        else
        {
            return new BaseCoupon<Coupon_info,ItinT,FrequentPass,FreeTextInfo>
                    (couponInfo(), ticknum());
        }
    }

    virtual ~BaseCoupon(){}
};


///@class BaseTicket
///@brief Информация о билете и его купонах
template<
        class CouponT
        >
class BaseTicket
{
public:
    typedef CouponT CouponType;
    typedef std::list<CouponType> CouponList;
private:
    TickStatAction::TickStatAction_t Tick_act_code; // Новый или на удаление (обмен)
    CouponList lCoupon;  // Купоны
    unsigned short Num;         // Номер по порядку
    void check() const;
protected:
    // Генерируется в ЦЭБ
    std::string Ticknum;        // Номер билета
    unsigned ConjunctionNum;
public:
    BaseTicket(const std::string &ticknum,
               TickStatAction::TickStatAction_t tick_act,
               unsigned short num,
               const std::list<CouponT> &lcoup)
    :
        Tick_act_code(tick_act),
        lCoupon(lcoup),
        Num(num),
        Ticknum(ticknum),
        ConjunctionNum(1)
    {
        check();
    }

    /**
     * @brief Добавить купон в билет
     * @param cpn купон
     */
    virtual void addCoupon(const CouponT &cpn)
    {
        lCoupon.push_back(cpn);
        check();
    }

    /**
     * @brief Вывести содержимое класса в лог
     * @param level
     * @param nick
     * @param file
     * @param line
     */
    virtual void Trace(int level,const char *nick, const char *file, int line) const
    {
        nologtrace(level, nick, file, line, ("Ticket #" + Ticknum + " [" + std::to_string(conjunctionNum()) + "] " + TickStatAction::TickActionTraceStr(Tick_act_code)).c_str());
        CouponT::Trace(level, nick, file, line, lCoupon);
    }

    /**
     * @brief Вывести содержимое списка классов в лог
     * @param level
     * @param nick
     * @param file
     * @param line
     * @param lTick - список билетов
     */
    template <class TicketT>
    static void Trace(int level,const char *nick, const char *file, int line,
                      const std::list<TicketT> &lTick)
    {
        for_each(lTick.begin(),lTick.end(),
                 TraceElement<TicketT>(level,nick,file,line));
    }

    virtual const std::string & ticknum()        const { return Ticknum; }

    /**
     * @brief Список купонов данного билета
     * @return returns std::list
     */
    virtual const CouponList &getCoupon() const { return lCoupon; }

    /**
     * @brief Список купонов данного билета
     * @return returns std::list
     */
    virtual CouponList &getCoupon() { return lCoupon; }

    /**
     * @brief Номер билета в связке
     * @return
     */
    virtual unsigned short num ()                const { return Num; }
    /**
     * @brief Новый или старый билет
     * @return TickStatAction::TickStatAction_t
     */
    virtual TickStatAction::TickStatAction_t actCode()   const { return Tick_act_code; }

    virtual bool operator == (const BaseTicket &tick) const
    {
        if(tick.num() == this->num() &&
           tick.actCode() == this->actCode() &&
           tick.getCoupon() == this->getCoupon()){

            nologtrace(6, MY_STDLOG, "Equal Tickets");
            return true;
        } else {
            nologtrace(6, MY_STDLOG, "Different Tickets");
            return false;
        }
    }

    /**
     * @brief sets the new Conjunction ticket number
     * @param theVal
     */
    virtual void setConjunctionNum(unsigned theVal)
    {
        ConjunctionNum = theVal;
        if(ConjunctionNum>4)
        {
            // такого не дб
            throw TickExceptions::tick_fatal_except(MY_STDLOG, EtErr::ProgErr,
                    "Invalid conjunction ticket number %d", theVal);
        }
    }

    /**
     * @brief Conjunction ticket number
     * @return
     */
    virtual unsigned conjunctionNum() const { return ConjunctionNum; }

    virtual ~BaseTicket(){}
};

/// @brief Сборы
/// @class BaseTaxDetails
class BaseTaxDetails
{
protected:
    TaxAmount::Amount AmValue;  // Кол-во
    TaxCategory Category;       // Категория
    std::string Type;           // Код сбора
    std::string FreeText;
public:
    BaseTaxDetails(const TaxAmount::Amount am,
                   const TaxCategory &cat,
                   const std::string type)
    :
        AmValue(am),
        Category(cat),
        Type(type)
    {
    }

    BaseTaxDetails(const std::string &taxText,
                   const TaxCategory &cat,
                   const std::string type)
    :
        AmValue(),
        Category(cat),
        Type(type),
        FreeText(taxText.substr(0,11))
    {
    }

    virtual void Trace(int level,const char *nick, const char *file, int line) const;
    template<class TaxDetailsT>
    static void Trace(int level,const char *nick, const char *file, int line,
                              const std::list<TaxDetailsT> &lTax)
    {
        for_each(lTax.begin(),lTax.end(),
                 TraceElement<TaxDetailsT>(level,nick,file,line));
    }


    /**
     * @brief Код сборов
     * @return string
     */
    virtual const std::string &type()                 const { return Type; }
    /**
     * @brief Значение
     * @return TaxAmount::Amount
     */
    virtual const TaxAmount::Amount & amValue () const { return AmValue; }

    virtual void setAmValue(const TaxAmount::Amount &av) { AmValue = av; }

    /**
     * @brief Категория сборов (Дополнительные/уплаченные/текущие)
     * @return
     */
    virtual const TaxCategory &category() const { return Category; }

    /**
     * @brief Сравнение
     * @param txd
     * @return
     */
    virtual bool operator == (const BaseTaxDetails &txd) const;

    virtual void setFreeText(const std::string &ft) { FreeText = ft; }

    /**
     * @brief Free text instead of amount value
    */
    virtual const std::string &freeText() const { return FreeText; }

    virtual ~BaseTaxDetails(){}
};

class BasePassenger
{
    std::string Surname; // Ф
    std::string Name;    // ИО
    int  Age;            // Only for unaccompanied minor
    std::string TypeCode;// Тип пассажира
public:
    typedef boost::shared_ptr< BasePassenger > SharedPtr;

    BasePassenger(const std::string &surn,
                  const std::string &name,
                  int age,
                  const std::string &TCode)
    :
            Surname(surn),
            Name(name),
            Age(age),
            TypeCode(TCode)
    {
    }

    virtual const std::string &surname() const   { return Surname; }
    virtual const std::string &name() const      { return Name; }
    virtual int age() const                      { return Age; }
    virtual const std::string &typeCode() const  { return TypeCode; }

    virtual void Trace(int level,const char *nick, const char *file, int line) const;

    virtual bool operator == (const BasePassenger &pass) const;
	// defines
    static const ushort MaxPassName=48;
    static const ushort MaxPassSurname=48;
    static const ushort MinPassSurname=1;

    virtual ~BasePassenger(){}
};

///@class BaseMonetaryInfo
///@brief Единица информации об оплате (тип, кол-во ..)
class BaseMonetaryInfo
{
    AmountCode Code;   // Тип оплаты
    TaxAmount::Amount AmValue; // Кол-во денешек
    MonetaryType MonType; // Тип (заменяет собой AmValue)
    std::string CurrCode; // Currency
    bool AddCollect;
public:
    /**
     * @brief Конструктор с числовым значеним величины. fi: B:100.00:RUR
     * @param c - Amount Code - B
     * @param am 100.00
     * @param CCode RUR
     */
    BaseMonetaryInfo(const AmountCode &c,
                     const TaxAmount::Amount am,
                     const std::string &CCode)
    :
        Code(c),
        AmValue(am),
        MonType(),
        CurrCode(CCode),
        AddCollect(false)
    {
    }

    /**
     * @brief Конструктор с типизированным значеним величины. fi: T:FREE:
     * @param c - Amount Code - T
     * @param am - FREE
     */
    BaseMonetaryInfo(const AmountCode &c,
                     const MonetaryType &mt,
                     const std::string &CCode = "")
    :
        Code(c),
        AmValue(),
        MonType(mt),
        CurrCode(CCode),
        AddCollect(false)
    {
    }

    virtual void Trace(int level,const char *nick, const char *file, int line) const;
    template <class MonetaryInfoT>
    static void Trace(int level,const char *nick, const char *file, int line,
                              const std::list<MonetaryInfoT> &lMon)
    {
        for_each(lMon.begin(),lMon.end(),
                 TraceElement<MonetaryInfoT>(level,nick,file,line));
    }

    /**
     * @brief Тип оплаты (B-base, T-total)
     * @return AmountCode
     */
    virtual const AmountCode &code() const { return Code; }

    /**
     * @brief Числовое значение оплаты
     * @return TaxAmount::Amount
     */
    virtual const TaxAmount::Amount &amValue() const { return AmValue; }
    /**
     * @brief Код валюты
     * @return std::string - 3-х симв код валюты
     */
    virtual const std::string currCode() const { return CurrCode; }

    /**
     * @brief Типизированное значение оплаты
     * @return MonetaryType, fi "IT"
     */
    virtual const MonetaryType &monType() const { return MonType; }

    /**
     * @brief Установить код валюты
     * @param c - std::string - 3-х симв код валюты
     */
    virtual void setCurrCode(const std::string &c) { CurrCode = c; }

    /**
     * @brief Оператор сравнения
     * @param mon
     * @return true if equals
     */
    virtual bool operator == (const BaseMonetaryInfo &mon) const;

    void setAddCollect(bool c);

    bool addCollect() const;

    /**
     * @brief virtual destructor
     */
    virtual ~BaseMonetaryInfo(){}
};

///@class BaseFormOfPayment
///@brief Форма оплаты
class BaseFormOfPayment
{
    std::string FopCode;
    FopIndicator FopInd;
    TaxAmount::Amount AmValue;  // Amount value
    std::string Vendor;         // Vendor code
    std::string AccountNum;     // Account Number
    boost::gregorian::date   ExpDate;             // Expiration Date
    std::string AppCode;        // Approval Code
    std::string FreeText;
public:
    static const ushort VCMax = 3;
    static const ushort VCMin = 2;
    static const ushort CrdNoMax = 19;
    static const ushort CrdNoMin = 12;
    static const ushort AppCodeMax = 6;
    BaseFormOfPayment(const std::string & fopCode,
                      const FopIndicator &fopI,
                      const TaxAmount::Amount &amVal,
                      const std::string &vendor,
                      const std::string &accNum,
                      const boost::gregorian::date &expDate,
                      const std::string &appCode)
    :
            FopCode(fopCode),
            FopInd(fopI),
            AmValue(amVal),
            Vendor(vendor),
            AccountNum(accNum),
            ExpDate(expDate),
            AppCode(appCode)
    {
    }

    virtual void Trace(int level,const char *nick, const char *file, int line) const;
    template <class FormOfPaymentT>
    static void Trace(int level,const char *nick, const char *file, int line,
                      const std::list<FormOfPaymentT> &lFop)
    {
        for_each(lFop.begin(),lFop.end(),
                 TraceElement<FormOfPaymentT>(level,nick,file,line));
    }

    /**
     * @brief Код формы оплаты
     * CA - cash...
     * @return
     */
    virtual const std::string fopCode() const { return FopCode; }
    /**
     * @brief Индикатор
     * 3- new fop ...
     * @return
     */
    virtual const FopIndicator & fopInd() const { return FopInd; }
    /**
     * @brief Значение
     * 200 руб нал
     * @return
     */
    virtual const TaxAmount::Amount &amValue() const { return AmValue; }
    /**
     * @brief Владелец
     * Владелец ПК - VISA
     * @return
     */
    virtual const std::string &vendor () const { return Vendor; }
    /**
     * @brief Номер док-та
     * номер ПК
     * @return
     */
    virtual const std::string &accountNum() const { return AccountNum; }
    /**
     * @brief Маскирует номер ПК в соотв с рекоммендациями IATA
     * @return
     */
    virtual std::string accountNumMask() const;
    /**
     * @brief Дата истечения срока действия карты
     * @return
     */
    virtual const boost::gregorian::date & expDate() const { return ExpDate; }

    /**
     * @brief Approval code
     * @return
     */
    virtual const std::string & appCode() const { return AppCode; }
    /**
     * @brief Если кредитная карта
     * @return true if CC
     */
    virtual bool isCC() const
    {
        return (vendor().size()>0 && accountNum().size()>0);
    }

    /**
     * @brief sets FOP free text
    */
    virtual void setFreeText(const std::string &ft) { FreeText = ft.substr(0, 70); }

    /**
     * @brief FOP free text
    */
    virtual std::string freeText() const { return FreeText; }

    /**
     * @brief Сравнение
     * @param fop
     * @return true if equals
     */
    virtual bool operator == (const BaseFormOfPayment &fop) const;

    /**
     *
     */
    virtual ~BaseFormOfPayment(){}
};

///@class BaseResContrInfo
///@brief Reservation Control Information
class BaseResContrInfo
{
public:
    ///@struct ReclocInfo
    ///@brief Airline And Recloc
    struct ReclocInfo
    {
        std::string Airline;
        std::string Recloc;
        ReclocInfo(const std::string &airl, const std::string &rl)
            :Airline(airl),Recloc(rl)
        {
        }
    };
    ///@typedef OtherAirlineReclocs
    ///@brief reclocs of interline partners
    typedef std::list<ReclocInfo> OtherAirlineReclocs;
private:
    std::string OurAirlineCode; //Наша компания
    std::string CrsAirlineCode; //Компания системы бронирования

    // Генерируется
    mutable std::string OurRecloc; //Собственный реклок
    mutable std::string RlIssuedFrom; // Если обмен, то RL, от которого получили этот
    std::string AirRecloc; //Реклок инвенторной системы
    std::string CrsRecloc; //Реклок системы бронирования

    boost::gregorian::date DateOfIssue; //Дата выписки ЭБ

    OtherAirlineReclocs OtherReclocs;
public:
    static const unsigned MaxRemoteRlLen = 20;

    BaseResContrInfo(const std::string &recloc,
                     const std::string &ourAwk,
                     const std::string &crsAwk,
                     const std::string &airRecloc,
                     const std::string &crsRecloc,
                     const boost::gregorian::date &dateI,
                     const OtherAirlineReclocs & = OtherAirlineReclocs())
    :
        OurAirlineCode(ourAwk),
        CrsAirlineCode(crsAwk),
        OurRecloc(recloc),
        AirRecloc(airRecloc),
        CrsRecloc(crsRecloc),
        DateOfIssue(dateI)
    {
    }

    virtual void Trace(int level,const char *nick, const char *file, int line) const;

    virtual std::string ourAirlineCode() const { return OurAirlineCode; }
    virtual std::string crsAirlineCode() const { return CrsAirlineCode; }

    virtual std::string ourRecloc() const    { return OurRecloc; }
    virtual void setOurRecloc(const std::string &rl) const
    {
        OurRecloc = rl;
    }

    virtual std::string crsRecloc() const    { return CrsRecloc; }
    virtual std::string airRecloc() const    { return AirRecloc; }
    virtual std::string rlIssuedFrom()const  { return RlIssuedFrom; }
    virtual const boost::gregorian::date &dateOfIssue() const { return DateOfIssue;  }

    virtual void setRlIssuedFrom(const std::string &rl) const { RlIssuedFrom = rl; }

    virtual void setOtherReclocs(const BaseResContrInfo::OtherAirlineReclocs &rl);

    /**
     * @brief Other airline reclocs (interline partners)
     * @return
     */
    virtual const OtherAirlineReclocs &otherAirlReclocs() const { return OtherReclocs; }

    /**
     * @brief Sets new Crs airline/recloc
     * @param airl
     * @param rl
     */
    virtual void setCrsRecloc(const std::string &airl, const std::string &rl);

    virtual bool operator == (const BaseResContrInfo &rci) const;

    virtual void addReclocInfo(const std::string &airl, const std::string &recloc);

    virtual ~BaseResContrInfo(){}
};

/**
 * @class BaseOrigOfRequest
 * @brief Данные оператора/системы
*/
class BaseOrigOfRequest
{ //ORIGINATOR OF REQUEST DETAILS
    std::string AirlineCode;        // Компания от имени * пришёл запрос
    std::string LocationCode;       // Город/порт базирования
    std::string PprNumber;          // Номер пункта продажы
    std::string OriginAgnCode;      // Агенство
    std::string OriginLocationCode; // Город/Порт базирования оператора
    std::string CountryCode;        // Страна оператора

    char Type[2];                   // Тип оператора, в нашем случае всегда "N"
    std::string Pult;               // Пульт
    std::string AuthCode;           // Номер оператора
    std::string CurrCode;           // Валюта оператора
    Language Lang;            // Язык оператора
public:

    static const unsigned AuthCodeMaxLen = 9;

    BaseOrigOfRequest(const std::string & airline,
                       const std::string & location,
                       const std::string &ppr,
                       const std::string & agn,
                       const std::string & originLocation,
                       char type,
                       const std::string &pult,
                       const std::string &authCode,
                       Language lang=ENGLISH);

    virtual const std::string & airlineCode  () const        { return AirlineCode; }
    virtual const std::string & locationCode () const        { return LocationCode; }

    virtual std::string pprNumber  () const { return PprNumber; }
    virtual std::string agencyCode () const { return OriginAgnCode; }

    virtual const std::string & originLocationCode() const { return OriginLocationCode; }
    virtual const char * type () const       { return Type; }
    virtual const std::string & pult() const            { return Pult; }
    virtual const std::string & authCode() const        { return AuthCode; }
    virtual Language lang () const     { return Lang; }
    virtual std::string langStr() const;
    /**
     * @brief Код валюты оператора
     * @return std::string - 3-х симв код валюты
     */
    virtual const std::string currCode() const { return CurrCode; }
    /**
     * @brief Установить код валюты оператора
     * @param c - std::string - 3-х симв код валюты
     */
    virtual void setCurrCode(const std::string &c) { CurrCode = c; }

    /**
     * @brief Установить страну оператора
     * @param c
     */
    virtual void setCountryCode(const std::string &c) { CountryCode = c; }

    /**
     * @brief agent's country code
     */
    virtual const std::string &countryCode() { return CountryCode; }

    /**
     * @brief Sets new ppr number
     * @param p
     */
    virtual void setPpr(const std::string &p) { PprNumber = p; }

    virtual void setAirline(const std::string &airl) { AirlineCode = airl; }

    /**
     * @brief sets new Agency code
     * @param agn
     */
    virtual void setAgency(const std::string &agn) { OriginAgnCode = agn; }

    virtual void setType(const char t) { Type[0] = t; }

    virtual void Trace(int level,const char *nick, const char *file, int line) const;

    virtual bool operator == (const BaseOrigOfRequest &org) const;

    virtual ~BaseOrigOfRequest(){}
};

/**
 * @class TourCode_t
 * @brief Tour code
*/
class TourCode_t
{
    std::string TourCode;
public:
    explicit TourCode_t(const std::string &tc) : TourCode(tc.substr(0,15))
    {
    }
    TourCode_t() : TourCode("")
    {
    }
    const std::string &code() const { return TourCode; }

    bool empty() const { return code().empty(); }
//     void setTourCode(const std::string &tc) { TourCode = tc; }
};

/**
 * @class class TicketingAgentInfo
 * @brief Ticketing Agent Information
*/
class TicketingAgentInfo_t
{
    std::string CompanyId;
    std::string AgentId;
    std::string AgentType;
public:
    explicit TicketingAgentInfo_t(const std::string &compID,
                                const std::string &AgnId,
                                const std::string &AgnType)
         :CompanyId(compID.substr(0,4)),
          AgentId(AgnId.substr(0,9)),
          AgentType(AgnType.substr(0,3))
    {
    }
    TicketingAgentInfo_t()
         :CompanyId(""), AgentId(""), AgentType("")
    {
    }

    const std::string &companyId() const { return CompanyId; }
    const std::string &agentId() const { return AgentId; }
    const std::string &agentType() const { return AgentType; }
};

class CommonTicketData
{
    TourCode_t TourCode;
    TicketingAgentInfo_t TicketingAgentInfo;
    //DocType::Type_t TicketType;

public:
    CommonTicketData(const TourCode_t &tc, const TicketingAgentInfo_t &ai,
                     DocType::Type_t /*tt*/ = DocType::Ticket)
        : TourCode(tc), TicketingAgentInfo(ai)//, TicketType(tt)
    {
    }
    CommonTicketData(){}

    const TourCode_t &tourCode() const { return TourCode; }
    const TicketingAgentInfo_t &ticketingAgentInfo() const { return TicketingAgentInfo; }
};

template<
        class OrigOfRequestT,
        class ResContrInfoT,
        class PassengerT,
        class TicketT,
        class TaxDetailsT,
        class MonetaryInfoT,
        class FormOfPaymentT,
        class FreeTextInfoT,
        class FormOfIdT
        >
class BasePnr
{
    OrigOfRequestT Org; // От чьего имени выписан/изменен
    ResContrInfoT Rci; // Pnr, company
    typename PassengerT::SharedPtr Pass; // Пассажир
    std::list <FormOfIdT>  lFoid; // FOID
    std::list <TicketT> lTicket; // Билеты
    std::list <TaxDetailsT> lTax; // Сборы
    std::list <MonetaryInfoT> lMon; // Тариф
    std::list <FormOfPaymentT> lFop; // Форма оплаты
    std::list <FreeTextInfoT> lIft; // Свободный текст
    CommonTicketData CTicketData;

    struct accumulateCCFop : public
            std::unary_function<std::list<FormOfPaymentT>, FormOfPaymentT>
    {
        std::list<FormOfPaymentT> & operator () (std::list<FormOfPaymentT> &lfop, const FormOfPaymentT &fop)
        {
            if(fop.isCC()){
                nologtrace(9, MY_STDLOG, " ");
                lfop.push_back(fop);
            }
            return lfop;
        }
    };

    struct collect_tick
    {
        TickStatAction::TickStatAction_t tact;
        std::list<TicketT> &ltck;

        collect_tick(TickStatAction::TickStatAction_t act, std::list<TicketT> &ltick)
            : tact(act), ltck(ltick) {}
        void operator () (const TicketT& t)
        {
            if(t.actCode() == tact)
            {
                ltck.push_back(t);
            }
        }
    };


public:
    typedef OrigOfRequestT OrigOfRequestType;
    typedef ResContrInfoT ResContrInfoType;
    typedef PassengerT PassengerType;
    typedef TicketT TicketType;
    typedef TaxDetailsT TaxDetailsType;
    typedef MonetaryInfoT MonetaryInfoType;
    typedef FormOfPaymentT FormOfPaymentType;
    typedef FreeTextInfoT FreeTextInfoType;
    typedef FormOfIdT  FormOfIdType;

    typedef std::list<TicketT> TicketList;
    typedef std::list<TaxDetailsT> TaxDetailsList;
    typedef std::list<MonetaryInfoT> MonetaryInfoList;
    typedef std::list<FormOfPaymentT> FormOfPaymentList;
    typedef std::list<FreeTextInfoT> FreeTextInfoList;
    typedef std::list<FormOfIdT>  FormOfIdList;

    typedef typename TicketList::iterator TicketIterator;
    typedef typename TaxDetailsList::iterator TaxDetailsIterator;
    typedef typename MonetaryInfoList::iterator MonetaryInfoIterator;
    typedef typename FormOfPaymentList::iterator FormOfPaymentIterator;
    typedef typename FreeTextInfoList::iterator FreeTextInfoIterator;
    typedef typename FormOfIdList::iterator FormOfIdIterator;
    typedef typename TicketT::CouponList::iterator CouponIterator;

    typedef typename TicketList::const_iterator TicketConstIterator;
    typedef typename TaxDetailsList::const_iterator TaxDetailsConstIterator;
    typedef typename MonetaryInfoList::const_iterator MonetaryInfoConstIterator;
    typedef typename FormOfPaymentList::const_iterator FormOfPaymentConstIterator;
    typedef typename FreeTextInfoList::const_iterator FreeTextInfoConstIterator;
    typedef typename FormOfIdList::const_iterator FormOfIdConstIterator;
    typedef typename TicketT::CouponList::const_iterator CouponConstIterator;

    // Ой как много копирований!!!!
    BasePnr(const OrigOfRequestT &og,
            const ResContrInfoT &rc,
            const std::list<FreeTextInfoT>  &lIt,
            const PassengerT &Pss,
            const std::list <FormOfIdT> &lfoid_,
            const std::list <TicketT> &lTk,
            const std::list <TaxDetailsT> &lTx,
            const std::list <MonetaryInfoT> &lMn,
            const std::list <FormOfPaymentT> &lFp,
            const CommonTicketData &ctd = CommonTicketData())
    :
        Org(og),
        Rci(rc),
        Pass(typename PassengerT::SharedPtr(new PassengerT(Pss))),
        lFoid(lfoid_),
        lTicket(lTk),
        lTax(lTx),
        lMon(lMn),
        lFop(lFp),
        lIft(lIt),
        CTicketData(ctd)
    {
    }

    BasePnr(const OrigOfRequestT &og,
            const ResContrInfoT &rc,
            const std::list<FreeTextInfoT>  &lIt,
            const typename PassengerT::SharedPtr &Pss,
            const std::list <FormOfIdT> &lfoid_,
            const std::list <TicketT> &lTk,
            const std::list <TaxDetailsT> &lTx,
            const std::list <MonetaryInfoT> &lMn,
            const std::list <FormOfPaymentT> &lFp,
            const CommonTicketData &ctd = CommonTicketData())
    :
        Org(og),
        Rci(rc),
        Pass(Pss),
        lFoid(lfoid_),
        lTicket(lTk),
        lTax(lTx),
        lMon(lMn),
        lFop(lFp),
        lIft(lIt),
        CTicketData(ctd)
    {
    }

    template <class PnrT>
    static void Trace(int level,const char *nick, const char *file, int line,
                      const PnrT &pnr)
    {
        nologtrace(level,nick,file,line,HelpCpp::string_cast(pnr.org()).c_str());
        nologtrace(level,nick,file,line,HelpCpp::string_cast(pnr.rci()).c_str());
        nologtrace(level,nick,file,line,HelpCpp::string_cast(pnr.cTicketData()).c_str());
        //LogTrace(level,nick,file,line) << pnr.org();
        //LogTrace(level,nick,file,line) << pnr.rci();
        //LogTrace(level,nick,file,line) << pnr.cTicketData();

        if(pnr.havePass())
            pnr.pass().Trace(level, nick, file, line);
        FormOfIdT::Trace(level, nick, file, line, pnr.lfoid());
        FormOfPaymentT::Trace(level, nick, file, line, pnr.lfop());
        MonetaryInfoT::Trace(level, nick, file, line, pnr.lmon());
        TaxDetailsT::Trace(level, nick, file, line, pnr.ltax());
        FreeTextInfoT::Trace(level, nick, file, line, pnr.lift());
        TicketT::Trace(level, nick, file, line, pnr.ltick());
    }

    virtual const OrigOfRequestT & org() const {return Org;}
    virtual OrigOfRequestT & org() {return Org;}
    virtual const ResContrInfoT & rci() const {return Rci;}
    virtual ResContrInfoT & rci() {return Rci;}
    virtual const std::list <FreeTextInfoT> & lift() const { return lIft; }
    virtual std::list <FreeTextInfoT> & lift() { return lIft; }
    virtual const PassengerT & pass() const
    {
        if(!havePass()){
            throw TickExceptions::tick_fatal_except(MY_STDLOG, EtErr::ProgErr, "Passenger not found");
        }
        return *Pass.get();
    }
    virtual PassengerT & pass()
    {
        if(!havePass()){
            throw TickExceptions::tick_fatal_except(MY_STDLOG, EtErr::ProgErr, "Passenger not found");
        }
        return *Pass.get();
    }
    virtual bool  havePass() const { return Pass.get()!=NULL; }

    virtual const std::list <FormOfIdT> &lfoid() const { return lFoid; }
    virtual std::list <FormOfIdT> &lfoid() { return lFoid; }

    virtual const std::list <TicketT> & ltick() const { return lTicket; }
    virtual std::list <TicketT> & ltick() { return lTicket; }

    virtual const std::list <TaxDetailsT> & ltax() const { return lTax; }
    virtual std::list <TaxDetailsT> & ltax() { return lTax; }

    virtual const std::list <MonetaryInfoT> &lmon() const { return lMon; }
    virtual std::list <MonetaryInfoT> &lmon() { return lMon; }

    virtual const std::list <FormOfPaymentT> &lfop() const { return lFop; }
    virtual std::list <FormOfPaymentT> &lfop() { return lFop; }

    virtual const CommonTicketData &cTicketData() const { return CTicketData; }
    virtual CommonTicketData &cTicketData() { return CTicketData; }

    virtual bool operator == (const BasePnr &p) const
    {
        if(p.org() == this->org() &&
           p.rci() == this->rci() &&
           p.havePass() == this->havePass() &&
           (!p.havePass() || (p.pass() == this->pass())) &&
           p.lfoid() == this->lfoid() &&
           cmplists(p.lift(), this->lift()) &&
           p.ltick() == this->ltick() &&
           p.ltax() == this->ltax() &&
           p.lmon() == this->lmon() &&
           p.lfop() == this->lfop())
        {

            nologtrace(6, MY_STDLOG, "Equal PNR!");
            return true;
        } else {
            nologtrace(6, MY_STDLOG, "Different PNR!");
            return false;
        }
    }

    // Возвр ФОП с кредитной картой
    virtual const FormOfPaymentT &ccFOP() const
    {
        std::list<FormOfPaymentT> lFop =
            std::accumulate(
                lfop().begin(),
                lfop().end(),
                std::list<FormOfPaymentT>(),
                accumulateCCFop());

        if(lFop.size() > 1){
            throw TickExceptions::tick_fatal_except(MY_STDLOG, EtErr::WRONG_CC, "too many FOPs with CC");
        } else if(lFop.size() == 0) {
            throw TickExceptions::tick_fatal_except(MY_STDLOG, EtErr::WRONG_CC, "There are not any FOPs with CC");
        }
        return lFop.front();
    }

    /**
     * @brief Sets a new Org for the Pnr image
     * @param theVal
     */
    virtual void setOrg(const OrigOfRequestT &theVal)
    {
        Org = theVal;
    }

    std::list<TicketT> makeOldTicketList() const
    {
        std::list<TicketT> ltkt;
        fillOldTicketList(ltkt);
        return ltkt;
    }
    void fillOldTicketList(std::list<TicketT> &ltkt) const
    {
        for_each(ltick().begin(), ltick().end(),
                 collect_tick(TickStatAction::oldtick, ltkt));
    }

    std::list<TicketT> makeNewTicketList() const
    {
        std::list<TicketT> ltkt;
        fillNewTicketList(ltkt);
        return ltkt;
    }
    void fillNewTicketList(std::list<TicketT> &ltkt) const
    {
        for_each(ltick().begin(), ltick().end(),
                 collect_tick(TickStatAction::newtick, ltkt));
    }

    virtual ~BasePnr(){}
};

template<
        class OrigOfRequestT,
        class ResContrInfoT,
        class PassengerT,
        class TicketT,
        class FormOfPaymentT,
        class MonetaryInfoT
                >
class BasePnrListItem
{
    PassengerT Pass;
    ResContrInfoT Rci;
    std::list<FormOfPaymentT> lFop;
    OrigOfRequestT Org;
    std::list<TicketT> lTicket;
    std::list<MonetaryInfoT> lMon;
public:
    BasePnrListItem(const PassengerT &pass,
                const ResContrInfoT &rci,
                const std::list<FormOfPaymentT> &lfop,
                const OrigOfRequestT &org,
                const std::list<TicketT> &ltick,
                const std::list<MonetaryInfoT> &lmon)
    :
        Pass(pass),
        Rci(rci),
        lFop(lfop),
        Org(org),
        lTicket(ltick),
        lMon(lmon)
    {
    }

	typedef OrigOfRequestT OrigOfRequestType;
	typedef ResContrInfoT ResContrInfoType;
	typedef PassengerT PassengerType;
	typedef TicketT TicketType;
	typedef FormOfPaymentT FormOfPaymentType;
    typedef MonetaryInfoT MonetaryInfoType;
    virtual const PassengerT &pass() const
    {
        return Pass;
    }
    virtual const ResContrInfoT &rci() const
    {
        return Rci;
    }
    virtual const std::list<FormOfPaymentT> &lfop() const
    {
        return lFop;
    }
    virtual const OrigOfRequestT &org() const
    {
        return Org;
    }
    virtual const std::list<TicketT> &lticket() const
    {
        return lTicket;
    }
    virtual const std::list<MonetaryInfoT> &lmon() const
    {
        return lMon;
    }

    virtual ~BasePnrListItem(){}
};

template <class PnrListItemT>
class BasePnrList
{
    std::list<PnrListItemT> lPnrItem;
public:
    typedef PnrListItemT PnrListItemType;
    BasePnrList(const std::list<PnrListItemT> &lItem):lPnrItem(lItem){}

    virtual const std::list<PnrListItemT> &lpnr() const
    {
        return lPnrItem;
    }

    virtual ~BasePnrList(){}
};

} // namespace Ticketing

template<typename CouponT>
void Ticketing::BaseTicket<CouponT>::check() const
{
    if(lCoupon.size() > 4)
    {
        throw TickExceptions::tick_soft_except(MY_STDLOG, EtErr::INV_COUPON_NUM,
                                               "Wrong coupon number")
                << ticknum()
                << std::to_string(lCoupon.size());
    }
}

#undef MY_STDLOG
#endif /*_ETICK_TICKET_H_*/
