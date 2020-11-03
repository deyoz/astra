#include <cmath>
#include <iomanip>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <serverlib/helpcpp.h>
#include <serverlib/cursctl.h>
#include <serverlib/str_utils.h>
#include "etick/tick_data.h"

#include "etick/exceptions.h"
#include "etick/lang.h"
#include "etick/etick_msg.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/test.h>
#include <serverlib/slogger.h>

using namespace std;

const std::string Ticketing::TaxAmount::Amount::TaxExempt = "EXEMPT";
const char *Ticketing::CouponStatList::ElemName = "Functional Service Elements Set";

#define ADD_ELEMENT(El, Ce, Cd, fl, Dr, Dl) {\
    using namespace Ticketing;\
    addElem( VTypes,  Ticketing::CouponStatList(Ticketing::CouponStatus::El, Ce, Cd, fl, Dr, Dl)); \
    }

DESCRIBE_CODE_SET( Ticketing::CouponStatList ) {
    ADD_ELEMENT( OriginalIssue,   "I",  "O",
                 CouponStatus::Single_changeable | CouponStatus::Ready_to_checkin,
                 "Open for Use", "Открыт");
    ADD_ELEMENT( Exchanged,       "E",  "E",
                 CouponStatus::Final_stat|CouponStatus::Single_changeable, //!
                 "Exchanged", "Принят в обмен");
    ADD_ELEMENT( Refunded,        "RF", "R",
                 CouponStatus::Final_stat,
                 "Refunded", "Возвращен");
    ADD_ELEMENT( Void,            "V",  "V",
                 CouponStatus::Final_stat,
                 "Void","Аннулирован");
    ADD_ELEMENT( Checked,         "CK", "C",
                 CouponStatus::Single_changeable|CouponStatus::Ready_to_checkin,
                 "Checked in", "Зарегистрирован");
    ADD_ELEMENT( Boarded,         "BD", "L",
                 CouponStatus::Single_changeable|CouponStatus::Ready_to_checkin,
                 "Lifted/Boarded", "Произведена посадка");
    ADD_ELEMENT( Printed,         "PR", "P",
                 CouponStatus::Final_stat,
                 "Printed", "Распечатан");
    ADD_ELEMENT( Airport,         "AL", "A",
                 CouponStatus::Single_changeable,
                 "Airport Control", "Аэропортовый контроль");
    ADD_ELEMENT( Flown,           "B",  "F",
                 CouponStatus::Final_stat|CouponStatus::Single_changeable,
                 "Flown", "Использован для перевозки");
    ADD_ELEMENT( ExchangedFIM,    "708","G",
                 CouponStatus::Final_stat|CouponStatus::Single_changeable,
                 "Exchanged/FIM", "Exchanged/FIM");
    ADD_ELEMENT( Irregular,       "IO", "I",
                 CouponStatus::Single_changeable,
                 "Irregular operations", "Ненормативные операции");
    ADD_ELEMENT( Notification,    "701","N",
                 CouponStatus::Single_changeable,
                 "Coupon notification", "Информационный купон");
    ADD_ELEMENT( Suspended,       "S",  "S",
                 CouponStatus::Single_changeable,
                 "Suspended", "Приостановлен");
    ADD_ELEMENT( PrintExch,       "PE", "X",
                 CouponStatus::Final_stat,
                 "Print Exchange", "Принят в обмен на бумажный");
    ADD_ELEMENT( Closed,          "CLO","Z",
                 CouponStatus::Final_stat|CouponStatus::Single_changeable,
                 "Closed", "Закрыт");
    ADD_ELEMENT( Paper,           "T",  "T",
                 CouponStatus::Final_stat,
                 "Paper", "Бумажный");
    ADD_ELEMENT( Unavailable,     "NAV","U",
                 CouponStatus::Single_changeable,
                 "Not Available", "Недоступен");
}
#undef ADD_ELEMENT

namespace Ticketing {

std::ostream & operator <<(std::ostream & s, const CouponStatus & status)
{
    if(status)
    {
        s << status->dispCode() << " (" << status->description() << ")";
    }
    else
    {
        s << "? (unknown)";
    }
    return s;
}

bool CouponStatList::isFinal() const
{
    return flags() & CouponStatus::Final_stat;
}

bool CouponStatList::isReadyToCheckin() const
{
    return flags() & CouponStatus::Ready_to_checkin;
}

bool CouponStatList::isSingleChangeable() const
{
    return flags() & CouponStatus::Single_changeable;
}

bool CouponStatList::isActive() const
{
    return !(type() == CouponStatus::Notification || isFinal());
}

CouponStatus::listOfStatuses_t CouponStatus::getListOfStatuses(unsigned f1, unsigned f2)
{
    listOfStatuses_t slist;
    for(CouponStatMap::const_iterator i = typesList().begin();
        i!=typesList().end(); i++)
    {
        if(((f1 & i->second.flags()) == f1) &&
           ((f2 & i->second.flags()))== 0)
        {
            slist.push_back( CouponStatus(i->second.code()) );
        }
    }
    return slist;
}

CouponStatus CouponStatus::fromDispCode(const std::string &disp)
{
    return CouponStatus
            (TypeElemHolder::initByUserSearch(CouponStatus::find_by_dispcode(disp.c_str())));
}

CouponStatus::CouponStatus(const std::string & status)
    :BaseTypeElemHolder<CouponStatList>(status)
{
}

CouponStatus::CouponStatus(const char * status)
    :BaseTypeElemHolder<CouponStatList>(status)
{
}

} // namespace Ticketing

const char *Ticketing::TicketMediaElem::ElemName = "Ticket media";

#define ADD_ELEMENT(El, Cl, Cr, Dl, Dr) \
    addElem( VTypes,  Ticketing::TicketMediaElem(Ticketing::TicketMedia::El, Cl, Cr, Dl, Dr));

DESCRIBE_CODE_SET( Ticketing::TicketMediaElem ) {
    ADD_ELEMENT( Electronic,  "E", "Э", "Electronic", "Электронный");
    ADD_ELEMENT( Paper,       "P", "Б", "Paper", "Бумажный");
}

Ticketing::TicketMedia::TicketMedia(const std::string & status)
        :BaseTypeElemHolder<TicketMediaElem>(status)
{
}

Ticketing::TicketMedia::TicketMedia(const char * status)
        :BaseTypeElemHolder<TicketMediaElem>(status)
{
}
#undef ADD_ELEMENT

const char *Ticketing::FoidTypeList::ElemName = "Foid type";
#define ADD_ELEMENT(El, Cl, Dl, Dr) \
    addElem( VTypes,  Ticketing::FoidTypeList(Ticketing::FoidType::El, Cl, Dl, Dr));

DESCRIBE_CODE_SET( Ticketing::FoidTypeList ) {
    ADD_ELEMENT(Passport,           "PP", "Passport", "Паспорт");
    ADD_ELEMENT(CreditCard,         "CC", "Credit card", "Кредитная карта");
    ADD_ELEMENT(FrequentFlyer,      "FF", "Frequent flyer", "Номер часто летающего пассажира");
    ADD_ELEMENT(NationalIdentity,   "NI", "National identity", "National identity");
    ADD_ELEMENT(ConfirmationNumber, "CN", "Confirmation number or Record Locator", "Номер брони (PNR)");
    ADD_ELEMENT(TicketNumber,       "TN", "Ticket number", "Номер билета");
    ADD_ELEMENT(LocallyDefinedID,   "ID", "Locally defined ID number", "Локально заданный номер");
    ADD_ELEMENT(DriversLicense,     "DL", "Drivers license", "Водительское удостоверение");
    ADD_ELEMENT(CustomerId,         "CI", "Customer Id", "Customer Id");
    ADD_ELEMENT(InvoiceReference,   "IR", "Invoice Reference", "Invoice Reference");
}

Ticketing::FoidType::FoidType(const std::string & status)
        :BaseTypeElemHolder<FoidTypeList>(status)
{
}

Ticketing::FoidType::FoidType(const char * status)
        :BaseTypeElemHolder<FoidTypeList>(status)
{
}
#undef ADD_ELEMENT


const char *Ticketing::ItinStatusElem::ElemName = "Segment status";
const std::string Ticketing::ItinStatus::Open="OPEN";

Ticketing::ItinStatus::listElems_t Ticketing::ItinStatus::StaticInitStatList = ItinStatus::listElems_t();

#define ADD_ELEMENT(El, Cl, Dl) \
    addElem( VTypes,  Ticketing::ItinStatusElem(Ticketing::ItinStatus::El, Cl, Dl));

DESCRIBE_CODE_SET( Ticketing::ItinStatusElem ) {
    ADD_ELEMENT(Confirmed,     "OK",    "Confirmed");
    ADD_ELEMENT(InfantNoSeat,  "NS",    "Infant, no seat");
    ADD_ELEMENT(OpenDate,      "OPE",   "Open date");
    ADD_ELEMENT(NonAir,        "G",     "Non air segment");
    ADD_ELEMENT(Standby,       "SB",    "Standby");
    ADD_ELEMENT(SpaceAvailable,"SA",    "Space available");
    ADD_ELEMENT(Requested,     "RQ",    "Requested");
}

namespace Ticketing {
const ItinStatus::listElems_t & ItinStatus::initStatList()
{
    if(!StaticInitStatList.empty())
    {
        return StaticInitStatList;
    }


    for(ItinStatusMap::const_iterator i = typesList().begin();
        i!=typesList().end(); i++)
    {
        StaticInitStatList.push_back( i->second );
    }
    return StaticInitStatList;
}

ItinStatus::ItinStatus()
    :TypeElemHolder()
{
}

ItinStatus::ItinStatus(const std::string & status)
    :TypeElemHolder(status)
{
}

ItinStatus::ItinStatus(Type_t stat)
        :TypeElemHolder(stat)
{
}
} // namespace Ticketing
#undef ADD_ELEMENT

const char *Ticketing::BaseClassListElem::ElemName = "Base reservation class";

#define ADD_ELEMENT(El, Cl, Cr, Dl, Dr) \
    addElem( VTypes,  Ticketing::BaseClassListElem(Ticketing::BaseClass::El, Cl, Cr, Dl, Dr));

DESCRIBE_CODE_SET( Ticketing::BaseClassListElem ) {
    ADD_ELEMENT(Economy,   "Y",  "Э", "Economy", "Эконом");
    ADD_ELEMENT(First,     "F",  "П", "Business", "Первый");
    ADD_ELEMENT(Business,  "C",  "Б", "Business", "Бизнес");
}

namespace Ticketing {

BaseClass::BaseClass(const char * rbd, Language l)
    :TypeElemHolder(rbd,l)
{
}

std::ostream & operator <<(std::ostream & s, const BaseClass & cls)
{
    s << cls->code() << " (" << cls->description() << ")";
    return s;
}
} // namespace Ticketing
#undef ADD_ELEMENT


const char *Ticketing::SubClassListElem::ElemName = "Reservation booking designator";
Ticketing::SubClass::listElems_t Ticketing::SubClass::StaticSubClsList = SubClass::listElems_t();
#define ADD_ELEMENT(El, Cl, Cr, Bc) \
    addElem( VTypes,  Ticketing::SubClassListElem(Ticketing::SubClass::El, Cl, Cr, Ticketing::BaseClass::Bc));

DESCRIBE_CODE_SET( Ticketing::SubClassListElem ) {
    ADD_ELEMENT(R, "Р","R", First);
    ADD_ELEMENT(P, "Ф","P", First);
    ADD_ELEMENT(F, "П","F", First);
    ADD_ELEMENT(A, "А","A", First);
    ADD_ELEMENT(J, "И","J", Business);
    ADD_ELEMENT(C, "Б","C", Business);
    ADD_ELEMENT(D, "Д","D", Business);
    ADD_ELEMENT(Z, "Ш","Z", Business);
    ADD_ELEMENT(I, "Ы","I", Business);
    ADD_ELEMENT(W, "Ю","W", Economy);
    ADD_ELEMENT(S, "С","S", Economy);
    ADD_ELEMENT(Y, "Э","Y", Economy);
    ADD_ELEMENT(B, "Ж","B", Economy);
    ADD_ELEMENT(H, "Ц","H", Economy);
    ADD_ELEMENT(K, "К","K", Economy);
    ADD_ELEMENT(L, "Л","L", Economy);
    ADD_ELEMENT(M, "М","M", Economy);
    ADD_ELEMENT(N, "Н","N", Economy);
    ADD_ELEMENT(Q, "Я","Q", Economy);
    ADD_ELEMENT(T, "Т","T", Economy);
    ADD_ELEMENT(V, "В","V", Economy);
    ADD_ELEMENT(X, "Х","X", Economy);
    ADD_ELEMENT(G, "Г","G", Economy);
    ADD_ELEMENT(U, "У","U", Economy);
    ADD_ELEMENT(E, "Е","E", Economy);
    ADD_ELEMENT(O, "О","O", Economy);
}

namespace Ticketing {
const SubClass::listElems_t &SubClass::subclassList()
{
    if(!StaticSubClsList.empty())
    {
        return StaticSubClsList;
    }


    for(SubClassMap::const_iterator i = typesList().begin();
        i!=typesList().end(); i++)
    {
        StaticSubClsList.push_back( i->second );
    }
    return StaticSubClsList;
}

std::ostream & operator <<(std::ostream & s, const SubClass & cls)
{
    s << cls->code();
    return s;
}
} // namespace Ticketing
#undef ADD_ELEMENT

namespace Ticketing {

namespace TickStatAction
{
using namespace TickExceptions;
    TickStatAction_t GetTickAction(const char *act)
    {
        if(!strcmp(act, "3")){
            return newtick;
        } else if(!strcmp(act, "2")){
            return oldtick;
        } else if(!strcmp(act, "4")) {
            return inConnectionWith;
        } else {
            throw tick_soft_except(STDLOG, EtErr::INV_TICK_ACT,
                                   "Unknown ticket action code %s", act);
        }
    }
    std::string TickActionStr(TickStatAction_t act)
    {
        switch(act)
        {
        case newtick:
            return "3";
        case oldtick:
            return "2";
        case inConnectionWith:
            return "4";
        default:
            throw tick_fatal_except(STDLOG, EtErr::INV_TICK_ACT,
                                    "Unknown ticket action type %d", act);
        }
    }

    std::string TickActionTraceStr(TickStatAction_t act)
    {
        switch(act)
        {
        case newtick:
            return "NEW";
        case oldtick:
            return "OLD";
        case inConnectionWith:
            return "IN CONNECTION";
        default:
            return "UNK";
        }
    }

    TickStatAction_t GetTickActionAirimp(const char *act)
    {
        if(!strcmp(act, "N")){
            return newtick;
        } else if(!strcmp(act, "O")){
            return oldtick;
        } else {
            throw tick_soft_except(STDLOG, EtErr::INV_TICK_ACT,
                                   "Unknown ticket action code %s", act);
        }
    }
} // namespace TickStatAction

namespace CpnStatAction
{
using namespace TickExceptions;
    CpnStatAction_t GetCpnAction(const char *act)
    {
        if(!strcmp(act, "6")){
            return consumedAtIssuance;
        } else if(!strcmp(act, "702")){
            return associate;
        } else if(!strcmp(act, "703")) {
            return disassociate;
        } else {
            throw tick_soft_except(STDLOG, EtErr::INV_CPN_ACT,
                                   "Unknown coupon action code %s", act);
        }
    }

    std::string CpnActionStr(CpnStatAction_t act)
    {
        switch(act)
        {
        case consumedAtIssuance:
            return "6";
        case associate:
            return "702";
        case disassociate:
            return "703";
        default:
            throw tick_fatal_except(STDLOG, EtErr::INV_CPN_ACT,
                                    "Unknown coupon action type %d", act);
        }
    }

    std::string CpnActionTraceStr(CpnStatAction_t act)
    {
        switch(act)
        {
        case consumedAtIssuance:
            return "Consumed at Issuance";
        case associate:
            return "Associate";
        case disassociate:
            return "Disassociate";
        default:
            return "Unknown cpn action";
        }
    }
} // namespace CpnStatAction

} // namespace Ticketing

const char *Ticketing::FopIndicatorElem::ElemName = "Form of payment indicator";

#define ADD_ELEMENT(El, Cl, Dl) \
    addElem( VTypes,  Ticketing::FopIndicatorElem(Ticketing::FopIndicator::El, Cl, Dl));

DESCRIBE_CODE_SET( Ticketing::FopIndicatorElem ) {
    ADD_ELEMENT(New, "3","New");
    ADD_ELEMENT(Old, "2","Old");
    ADD_ELEMENT(Orig,"5","Original issue");
}

namespace Ticketing {
FopIndicator::FopIndicator(const std::string &act)
    :TypeElemHolder(act)
{
}
FopIndicator::FopIndicator(const char *act)
    :TypeElemHolder(act)
{
}
FopIndicator::FopIndicator(FopIndicator::FopIndicator_t act)
    :TypeElemHolder(act)
{
}
} // namespace Ticketing
#undef ADD_ELEMENT

const char *Ticketing::TaxCategoryElem::ElemName = "Tax category";

#define ADD_ELEMENT(El, Cl, Dl) \
    addElem( VTypes,  Ticketing::TaxCategoryElem(Ticketing::TaxCategory::El, Cl, Dl));

DESCRIBE_CODE_SET( Ticketing::TaxCategoryElem ) {
    ADD_ELEMENT(Additional,      "700", "Additional collection");
    ADD_ELEMENT(Paid,            "701", "Paid");
    ADD_ELEMENT(Current,         "702", "Current");
    ADD_ELEMENT(CarrierFee,      "710", "Applies to carrier fee");
    ADD_ELEMENT(TaxOnCarrierFee, "711", "Applies to tax on carrier fee");
    ADD_ELEMENT(Exempt,          "E",   "Tax exempt");
}

namespace Ticketing {
TaxCategory::TaxCategory(const std::string &act)
    :TypeElemHolder(act)
{
}
TaxCategory::TaxCategory(const char *act)
    :TypeElemHolder(act)
{
}
TaxCategory::TaxCategory(TaxCategory::TaxCategory_t act)
    :TypeElemHolder(act)
{
}
} // namespace Ticketing
#undef ADD_ELEMENT


namespace Ticketing {
using namespace TickExceptions;
namespace TaxAmount {

Amount::Amount(const std::string &am, AmountType_e type)
    : Am_str(am),Percent(type==Percents)
{
    Valid = true;
    if(!am.size())
    {
        *this = Amount();
        return;
    }

    IntPart.reserve(Am_str.size());
    size_t i;
    for(i = 0; i < Am_str.size(); i++) {
        if(Am_str[i] == '.')
            break;
        IntPart += Am_str[i];
    }

    if (i + 1 < Am_str.size())
    {
        Fraction.assign(Am_str, i + 1, Am_str.size() - (i + 1));
    }

    if (Fraction.empty() && Am_str[i] != '.')
        Am_str = IntPart;
    else
        Am_str = IntPart + "." + Fraction;

    ProgTrace(TRACE4,"String value: [%s] parsed on integer part: [%s], fraction: [%s]",
              Am_str.c_str(), IntPart.c_str(), Fraction.c_str());

    dot_pos = Fraction.size();

    std::string fullAmount(IntPart+Fraction);
    try
    {
        this->Am = boost::lexical_cast<Amount_t>(fullAmount);
    }
    catch(boost::bad_lexical_cast &e)
    {
        LogWarning(STDLOG) << e.what();
        throw tick_soft_except(STDLOG, EtErr::INV_TAX_AMOUNT,
                               "Invalid amount value %s",
                               fullAmount.c_str());
    }

    if(IntPart[0] == '0' &&
       (Am <= 10000 && Am >= 0) &&
       Fraction.empty() && IntPart.size() == 5)
    {
        // Это проценты! По-edifact-овски 10000 = это 100.00%; 00001 = 0.01%
        Percent = true;
        dot_pos = 2;
    }
    else if(type == Percents)
    {
        throw tick_soft_except(STDLOG, EtErr::INV_TAX_AMOUNT,
                               "Invalid percents value %s", fullAmount.c_str());
    }
}

Amount::Amount(Amount_t am, unsigned pos, AmountType_e type)
        :Am(am),dot_pos(pos),Percent(type==Percents)
{
    Valid = true;
    if(type == Ordinary)
    {
        string am_str = HelpCpp::string_cast(am);
        int k = am_str.size() - dot_pos;
        if(k <= 0)
        {
            IntPart = "0";
            Fraction.append(std::abs(k), '0');
            Fraction.append(am_str);
        }
        else
        {
            IntPart.append(am_str, 0, k);
            Fraction.append(am_str, k, dot_pos);
        }
        Am_str = IntPart;
        if(!Fraction.empty())
            Am_str = Am_str + "." + Fraction;
    }
    else
    {
        if(Am<0 || (Am/::pow(10,pos))>100)
        {
            throw tick_soft_except(STDLOG, EtErr::INV_TAX_AMOUNT,
                                "Invalid amount value %d. It's not percents", Am);
        }

        std::stringstream str;
        str << std::setw(5) << std::setfill('0') << Am;
        IntPart.assign(str.str());
        Fraction.assign("");
        Am_str.assign(IntPart);
    }
}

Amount &Amount::setDot(unsigned short pos)
{
    if(!isPercents() && pos != dotPos())
    {
        *this = Amount(static_cast<Amount_t>(amValue()*
                pow(static_cast<double>(10), static_cast<double>(pos-dotPos()))),
                    pos, Ordinary);
    }
    return *this;
}

Amount Amount::getCopyWithDot(unsigned short pos) const
{
    if(!isPercents() && pos != dotPos())
    {
        return Amount(static_cast<Amount_t>(amValue()*
                pow(static_cast<double>(10), static_cast<double>(pos-dotPos()))),
                    pos, Ordinary);
    }
    return *this;
}

Amount_t Amount::amValue() const
{
    return Am;
}

Amount_t Amount::amValue(unsigned short pos) const
{
    if (dot_pos == pos) {
        return Am;
    }
    return dot_pos > pos ? Am / pow(10, dot_pos - pos)
                         : Am * pow(10, pos - dot_pos);
}

const string &Amount::amStr() const
{
    return Am_str;
}

string Amount::amStr(CutZeroFraction cutZeroFraction, MinFractionLength minFractionLength) const
{
    if (!Fraction.empty()) {
        if (Fraction.size() < minFractionLength.get()) {
            return (Am_str + std::string(minFractionLength.get() - Fraction.size(), '0'));
        } else if (cutZeroFraction) {
            const size_t pos = Fraction.find_last_not_of('0');
            std::string cuttedFraction = pos != std::string::npos ? Fraction.substr(0, pos + 1)
                                                                  : std::string();
            if (cuttedFraction.size() < minFractionLength.get()) {
                cuttedFraction += std::string(minFractionLength.get() - cuttedFraction.size(), '0');
            }
            if (cuttedFraction.empty()) {
                return IntPart;
            }
            return (IntPart + "." + cuttedFraction);
        }
    }
    return Am_str;
}

bool Amount::validate(const std::string &am)
{
    boost::smatch mAmount;
    const boost::regex rxAmount("[0-9]+\\.{0,1}[0-9]*");
    if (boost::regex_match(am, mAmount, rxAmount)) {
        return true;
    }
    return false;
}

Amount& Amount::operator +=(const Amount &a)
{
    if(this->isPercents() || a.isPercents())
        throw tick_fatal_except(STDLOG, EtErr::ProgErr, "Unable to summ percents");

    if(this->dotPos() > a.dotPos())
    {
        auto am = a;
        am.setDot(this->dotPos());
        return *this = Amount(amValue() + am.amValue(), dotPos(), Ordinary);
    }
    else
    {
        this->setDot(a.dotPos());
        return *this = Amount(amValue() + a.amValue(), dotPos(), Ordinary);
    }
    return *this;
}

Amount operator + (const Amount &am1_, const Amount &am2_)
{
    Amount am1 = am1_;
    return am1 += am2_;
}

Amount operator * (const Amount &am1, const Amount &am2)
{
    if(am1.isPercents() || am2.isPercents())
        throw tick_fatal_except(STDLOG, EtErr::ProgErr, "Unable to mult percents");

    Amount result(am1.amValue() * am2.amValue(), am1.dotPos() + am2.dotPos(),
                      Amount::Ordinary);
    if(am1.dotPos() > am2.dotPos())
        result.setDot(am1.dotPos());
    else
        result.setDot(am2.dotPos());
    return result;
}

bool operator == (const Amount &am1_, const Amount &am2_)
{
    Amount am1 = am1_;
    Amount am2 = am2_;

    if(am1.isPercents() || am2.isPercents())
        throw tick_fatal_except(STDLOG, EtErr::ProgErr, "Unable to cmp percents");

    if(am1.dotPos() > am2.dotPos())
        am2.setDot(am1.dotPos());
    else
        am1.setDot(am2.dotPos());

    return (am1.amValue() == am2.amValue());
}

bool operator != (const Amount &am1, const Amount &am2)
{
    return !(am1 == am2);
}

int amountToInt(const TaxAmount::Amount &amount)
{
    int amValue = amount.amValue();
    if (amount.dotPos() != 2) {
        amValue = amount.dotPos() > 2 ? amount.amValue() * pow(10, amount.dotPos() - 2)
                                      : amount.amValue() / pow(10, 2 - amount.dotPos());
    }
    return amValue;
}

}
} // Ticketing
#undef ADD_ELEMENT


const char *Ticketing::AmountCodeElem::ElemName = "Passenger Type";
#define ADD_ONE(El, Cl, Dl) \
    addElem( VTypes,  Ticketing::AmountCodeElem(Ticketing::AmountCode::El, Cl, Dl));

DESCRIBE_CODE_SET( Ticketing::AmountCodeElem ) {
    ADD_ONE(Total,          "T", "Total ticket sell amount");
    ADD_ONE(Base_fare,      "B", "Base fare");
    ADD_ONE(Equivalent,     "E", "Equivalent fare");
    ADD_ONE(ExchRate,       "D", "Bank exchange rate");
    ADD_ONE(TotalAdd,       "C", "Total additional charge");
    ADD_ONE(Commission,     "F", "Commission amount");
    ADD_ONE(CommissionRate, "G", "Commission rate");
    ADD_ONE(TotalSellAmount,"I", "Total ticket/document sell amount");
    ADD_ONE(TotalTicket,    "M", "Ticket total amount");
    ADD_ONE(Security,       "Y", "Security");
    ADD_ONE(USDSecurity,    "Y2","Конфиденциальный тариф в USD");
    ADD_ONE(NetFare,        "H", "Net fare amount (Base fare net amount)");
#undef ADD_ONE
}

namespace Ticketing {
AmountCode::AmountCode(const std::string &act)
    :TypeElemHolder(act)
{
}
AmountCode::AmountCode(const char *act)
    :TypeElemHolder(act)
{
}
AmountCode::AmountCode(AmountCode::AmountCode_t act)
    :TypeElemHolder(act)
{
}
} // namespace Ticketing

const char *Ticketing::PassengerTypeElem::ElemName = "Passenger Type";


#define ADD_ONE(El, Cl, Dl) \
    addElem( VTypes,  Ticketing::PassengerTypeElem(Ticketing::PassengerType::El, Cl, Dl));

DESCRIBE_CODE_SET( Ticketing::PassengerTypeElem ) {
    ADD_ONE(Adult,          "A" , "Adult");
    ADD_ONE(Male,           "M" , "Male" );
    ADD_ONE(Female,         "F" , "Female");
    ADD_ONE(Child,          "C" , "Child");
    ADD_ONE(Infant_m,       "IM", "Infant Male");
    ADD_ONE(Infant_f,       "IF", "Infant Female");
    ADD_ONE(Infant,         "IN", "Infant");
    ADD_ONE(Infant_num,     "766","Infant");
    ADD_ONE(Infant_num_os,  "767","Infant with seats");
    ADD_ONE(Unaccomp_minor, "UM", "Unaccompanied Minor");
#undef ADD_ONE
}

namespace Ticketing {
PassengerType::PassengerType(const std::string &act)
    :TypeElemHolder(act)
{
}
PassengerType::PassengerType(const char *act)
    :TypeElemHolder(act)
{
}
PassengerType::PassengerType(PassengerType::PassType_t act)
    :TypeElemHolder(act)
{
}
} // namespace Ticketing

const char *Ticketing::AccompaniedIndicatorElem::ElemName = "Accompanied Indicator";


#define ADD_ONE(El, Cl, Dl) \
    addElem( VTypes,  Ticketing::AccompaniedIndicatorElem(Ticketing::AccompaniedIndicator::El, Cl, Dl));

DESCRIBE_CODE_SET( Ticketing::AccompaniedIndicatorElem ) {
    ADD_ONE(InfantWithoutSeat, "1" , "Infant without seat");
    ADD_ONE(InfantWithSeat,    "2" , "Infant with seat" );
#undef ADD_ONE
}

namespace Ticketing {
AccompaniedIndicator::AccompaniedIndicator(const std::string &act)
    :TypeElemHolder(act)
{
}
AccompaniedIndicator::AccompaniedIndicator(const char *act)
    :TypeElemHolder(act)
{
}
AccompaniedIndicator::AccompaniedIndicator(AccompaniedIndicator::Indicator_t act)
    :TypeElemHolder(act)
{
}
} // namespace Ticketing

const char *Ticketing::FreeTextTypeElem::ElemName = "Free Text";

#define ADD_ONE(El, C1, C2,  Dl) \
    addElem( VTypes,  Ticketing::FreeTextTypeElem(Ticketing::FreeTextType::El, C1, C2, Dl));

DESCRIBE_CODE_SET( Ticketing::FreeTextTypeElem ) {
    ADD_ONE(Unknown,            "" ,    "" ,     "Unknown free text type");
    ADD_ONE(LiteralText,        "3",    "",      "Literal text");
    ADD_ONE(TicketingTimeLimit, "3",    "27",    "Ticketing time limit");
    ADD_ONE(HeaderInfo,         "3",    "50",    "Header information");
    ADD_ONE(TelephoneBusiness,  "4",    "3" ,    "Business telephone number");
    ADD_ONE(TelephoneHome,      "4",    "4" ,    "Home telephone number");
    ADD_ONE(Telephone,          "4",    "5" ,    "Telephone nature not known");
    ADD_ONE(TrAgentTelephone,   "4",    "6" ,    "Travel agent telephone number");
    ADD_ONE(Remark,             "4",    "7" ,    "Remarks (free text information)");
    ADD_ONE(PartyInfo,          "4",    "8",     "Indication to reference complete party information");
    ADD_ONE(TicketNumber,       "4",    "9",     "Ticket number");
    ADD_ONE(Endorsement,        "4",    "10",    "Endorsement information");
    ADD_ONE(Commission,         "4",    "11",    "Commission information");
    ADD_ONE(TourNumber,         "4",    "12",    "Tour number");
    ADD_ONE(SpecRem,            "4",    "13",    "Special remarks "
                                                 "(may contain coded information followed by free text)");
    ADD_ONE(FareCalc,           "4",    "15",    "Fare calculation at time of ticketing");
    ADD_ONE(FormOfPayment,      "4",    "16",    "Form of payment information");
    ADD_ONE(InvoiceInfo,        "4",    "18",    "Invoice information");
    ADD_ONE(WholesalerAddress,  "4",    "21",    "Wholesaler address");
    ADD_ONE(TicketingRems,      "4",    "23",    "Ticketing remarks");
    ADD_ONE(BookingPartyDetails,"4",    "24",    "Booking party details");
    ADD_ONE(Osi,                "4",    "28",    "Other service information (OSI)");
    ADD_ONE(FareAmount,         "4",    "34",    "Total Fare amount too large, text information included");
    ADD_ONE(NonSegRelatedItin,  "4",    "38",    "Non-segment related itinerary remarks");
    ADD_ONE(AgnAirName,         "4",    "39",    "Issuing agency/airline-name and place of issue");
    ADD_ONE(ServProvName,       "4",    "40",    "Name of service provider to present to");
    ADD_ONE(PaxSpecInfo,        "4",    "41",    "Passenger specific information");
    ADD_ONE(ServProviderLoc,    "4",    "42",    "Location of service provider to present at");
    ADD_ONE(SponsorInfo,        "4",    "43",    "Sponsor information");
    ADD_ONE(OriginalIssueInfo,  "4",    "45",    "Original issue information");
    ADD_ONE(RfiscDescription,   "4",    "47",    "Reason for issuance sub-code description");
    ADD_ONE(SupplierAgnInfo,    "4",    "55",    "Supplier agent Information");
    ADD_ONE(EmailAddress,       "4",    "61",    "E-Mail address");
    ADD_ONE(TchQrLinkPart,      "4",    "71",    "Tch ticket QR-code link part");
    ADD_ONE(AvlRelatedText,     "4",    "709",   "Availability related text");
    ADD_ONE(ReferenceSellToken, "4",    "711",   "Reference Sell Token");
    ADD_ONE(MoreAvlToken,       "4",    "712",   "More Availability Token");
    ADD_ONE(FareCalcReport,     "4",    "733",   "Fare calculation reporting");
    ADD_ONE(TchUniq,            "ZZZ",  "TID",   "Unique ID");
#undef ADD_ONE
}

namespace Ticketing {
FreeTextType::FreeTextType(const std::string &code, const string &qual, bool strict)
{
    try
    {
        *this = TypeElemHolder::initByUserSearch(FreeTextType::find_by_code_qual(code, qual));
    }
    catch(const NoSuchCode &e){
        if(strict)
        {
            throw;
        }
        *this = FreeTextType(Unknown);
        this->entry()->setCode(code);
        this->entry()->setQual(qual);
    }
}
} // namespace Ticketing

const char *Ticketing::HistCodeElem::ElemName = "History Code";
DESCRIBE_CODE_SET(Ticketing::HistCodeElem) {
#define ADD_ONE(a, code, edicode, desc) \
    addElem( VTypes, Ticketing::HistCodeElem(Ticketing::HistCode::a, code, edicode, desc))

    ADD_ONE(Issue,           "ISSUE" ,     "130", "Issue");
    ADD_ONE(Exchange,        "EXCHANGE",   "134", "Exchange");
    ADD_ONE(Refund,          "REFUND",     "135", "Refund");
    ADD_ONE(Void,            "VOID",       "133", "Void");
    ADD_ONE(VoidExchange,    "VOID_EXCH",  "776", "Void Exchange/Reissue");
    ADD_ONE(Print,           "PRINT",      "132", "Print etkt");
    ADD_ONE(PrintExch,       "PRINT_EXCH", "134", "Print exchange");
    ADD_ONE(RefundCancel,    "REF_CANCEL", "775", "Cancel of refund");
    ADD_ONE(SystemCancel,    "SYS_CANCEL", "79" , "System cancel");
    ADD_ONE(ChangeStatus,    "CHANGESTAT", "142", "Change of status");
    ADD_ONE(ReservChange,    "RESERVCHNG", "760", "Reservation change");
    ADD_ONE(ExchangeIssue,   "EXCH_ISSUE", "134", "Issued by exhange");
    ADD_ONE(PcAuth,          "PC_AUTH",    "777", "Plastic Card Authorization");
    ADD_ONE(ReceiveUAC,      "RECEIVEUAC", "107", "Receive UAC");
    ADD_ONE(ReceiveAC,       "RECEIVEAC",  "751", "Receive AC");
    ADD_ONE(SetSac,          "SET_SAC",    "142", "Set SAC");
    ADD_ONE(Revalidation,    "REVALIDAT",  "139", "Revalidation");
    ADD_ONE(SystemUpdateEmd, "UPDATE_EMD", "794", "EMD System Update");
    ADD_ONE(IssueEmd,        "ISSUE",      "796", "EMD Issue");
    ADD_ONE(ExchangeEmd,     "EXCHANGE",   "798", "EMD Exchange");
    ADD_ONE(RefundEmd,       "REFUND",     "799", "EMD Refund");
    ADD_ONE(SystemCancelEmd, "SYS_CANCEL", "800", "EMD System Cancel");
    ADD_ONE(RefundCancelEmd, "REF_CANCEL", "801", "EMD Refund Cancel");
    ADD_ONE(VoidExchangeEmd, "VOID_EXCH",  "802", "EMD Void Exchange/Reissue");
    ADD_ONE(VoidEmd,         "VOID",       "803", "EMD Void");
    ADD_ONE(ChangeStatusEmd, "CHANGESTAT", "793", "EMD Change of status");
    ADD_ONE(ExchangeIssueEmd,"EXCH_ISSUE", "798", "EMD Issued by exhange");
    ADD_ONE(ReceiveUACEmd,   "RECEIVEUAC", "224", "EMD Receive UAC");
    ADD_ONE(ReceiveACEmd,    "RECEIVEAC",  "792", "EMD Receive AC");
    ADD_ONE(SetSacEmd,       "SET_SAC",    "793", "EMD Set SAC");
#undef ADD_ONE
}


namespace Ticketing {
// Нормы багажа
Baggage::chrgQMap *Baggage::charge_measure=0;

Baggage::Baggage(unsigned allowance,
                    const std::string &charge,
                    const std::string &measure)
:Allowance(allowance),Charge(charge),Measure(measure)
{
    ChargeQualifier = getQualifier(this->charge(),this->measure());
}

void Baggage::filldata()
{
    get_charge_measure()["W^K"]   = Baggage::WeightKilo;
    get_charge_measure()["W^L"]   = Baggage::WeightPounds;
    get_charge_measure()["N^x"]   = Baggage::NumPieces;
    get_charge_measure()["702^x"] = Baggage::Nil;
}
Baggage::Baggage_t Baggage::getQualifier(const std::string &charge,
                                        const std::string &measure)
{
    const std::string key = (charge.empty()?"x":charge) + "^" +
            (measure.empty()?"x":measure);
    Baggage::chrgQMap::const_iterator i = get_charge_measure().find(key);
    if (i == get_charge_measure().end())
    {
        throw tick_soft_except(STDLOG, EtErr::ETS_INV_LUGGAGE,
                                "Unknown baggage charge and measure combination %s/%s",
                                charge.c_str(), measure.c_str());
    }
    return (*i).second;
}

const char * Baggage::code(Language l) const
{
    if(ChargeQualifier == WeightKilo)
    {
        return (l==RUSSIAN)?"КГ":"KG";
    }
    else if(ChargeQualifier == WeightPounds)
    {
        return (l==RUSSIAN)?"Ф":"L";
    }
    else if(ChargeQualifier == NumPieces)
    {
        // Мест/Number Of Pieces
        return (l==RUSSIAN)?"М":"N";
    }
    else if(ChargeQualifier == Nil)
    {
        return (l==RUSSIAN)?"Нет":"Nil";
    }
    else
    {
        throw tick_fatal_except(STDLOG, EtErr::ProgErr, "Unknown baggage code");
    }
}
} // namespace Ticketing;

const char *Ticketing::MonetaryTypeElem::ElemName = "Monetary Type";

DESCRIBE_CODE_SET(Ticketing::MonetaryTypeElem) {
#define ADD_ONE(a, rcode, lcode, rdesc, ldesc) \
    addElem( VTypes, Ticketing::MonetaryTypeElem(Ticketing::MonetaryType::a, rcode, lcode, rdesc, ldesc))

    ADD_ONE(Free,     "БЕСПЛАТН", "FREE",    "Free",   "Бесплатный");
    ADD_ONE(Charter,  "ЧАРТЕР",   "CHARTER", "Чартер", "Сharter");
    ADD_ONE(NoADC,    "НЕТ ДОПЛ", "NO ADC",  "Без доплаты",   "No additional collection");
    // no space
    ADD_ONE(NoADC_ns, "НЕТДОПЛ", "NOADC",    "Без доплаты",   "No additional collection");
    ADD_ONE(Bt,       "ВТ",       "BT",      "Bulk",   "Bulk");
    ADD_ONE(Bulk,     "BULK",     "BULK",    "Bulk",   "Bulk");
    ADD_ONE(It,       "ИТ",       "IT",      "Включающий тур", "Inclusive tour fare");
    ADD_ONE(NoFARE,   "НЕТ ТАР",  "NOFARE",  "Нет тарифа",   "No FARE");
    ADD_ONE(FreeText, "",         "",        "Не контролируемый формат",   "Free text without format control");
#undef ADD_ONE
}

namespace Ticketing {

void MonetaryTypeElem::setFreeText(const std::string &txt)
{
    if(txt.length() > MaxFreeTextLength) {
        throw tick_soft_except(STDLOG, EtErr::MISS_MONETARY_INF, // missing or invalid
                               "Invalid length of MON free text '%s'", txt.c_str());
    }
    FreeText = txt;
}

MonetaryType::MonetaryType()
        :TypeElemHolder()
{
}

MonetaryType::MonetaryType(const std::string & typestr, int)
    :TypeElemHolder(typestr)
{
}

MonetaryType::MonetaryType(const std::string & typestr)
{
    if(MonetaryType::checkExistence(typestr)) {
        *this = MonetaryType(typestr, 0);
    } else {
        *this = MonetaryType(MonetaryType::FreeText);
        entry()->setFreeText(typestr);
    }
}

MonetaryType::MonetaryType(Type_t typ)
        :TypeElemHolder(typ)
{
}

HistSubCode HistSubCode::getByEdiCode(const string &edi_code)
{
    return HistSubCode(HistCodeHolder::initByEdiCode(edi_code));
}

HistSubCode::HistSubCode(const HistCodeHolder &hsc)
    :_HistSubCode(hsc)
{
}

} // namespace Ticketing


const char *Ticketing::PricingTicketingIndicatorElem::ElemName = "Pricing/Ticketing indicator";

#define ADD_ELEMENT(El, Cl, Dl) \
    addElem( VTypes,  Ticketing::PricingTicketingIndicatorElem(Ticketing::PricingTicketingIndicator::El, Cl, Dl));

DESCRIBE_CODE_SET( Ticketing::PricingTicketingIndicatorElem ) {
    ADD_ELEMENT(NonRefundable,    "NR", "Non-refundable");
    ADD_ELEMENT(NonEndorsable,    "NE", "Non-endorsable");
    ADD_ELEMENT(NonInterlineable, "731","Non-interlineable");
}

namespace Ticketing {
PricingTicketingIndicator::PricingTicketingIndicator(const std::string &act)
    :TypeElemHolder(act)
{
}
PricingTicketingIndicator::PricingTicketingIndicator(const char *act)
    :TypeElemHolder(act)
{
}
PricingTicketingIndicator::PricingTicketingIndicator(PricingTicketingIndicator::PricingTicketingIndicator_t act)
    :TypeElemHolder(act)
{
}
} // namespace Ticketing
#undef ADD_ELEMENT



const char *Ticketing::PricingTicketingSellTypeElem::ElemName = "Pricing/Ticketing sell type";

#define ADD_ELEMENT(El, Cl, Dl) \
    addElem( VTypes,  Ticketing::PricingTicketingSellTypeElem(Ticketing::PricingTicketingSellType::El, Cl, Dl));

DESCRIBE_CODE_SET( Ticketing::PricingTicketingSellTypeElem ) {
    ADD_ELEMENT(Upsell,         "UPS", "Upsell");
    ADD_ELEMENT(WebCheckinSale, "WCS", "Web check-in sale");
}

namespace Ticketing {
PricingTicketingSellType::PricingTicketingSellType(const std::string &act)
    :TypeElemHolder(act)
{
}
PricingTicketingSellType::PricingTicketingSellType(const char *act)
    :TypeElemHolder(act)
{
}
PricingTicketingSellType::PricingTicketingSellType(PricingTicketingSellType::PricingTicketingSellType_t act)
    :TypeElemHolder(act)
{
}
} // namespace Ticketing
#undef ADD_ELEMENT

