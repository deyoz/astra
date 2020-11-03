#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <stdlib.h>
#include <numeric>

#include <serverlib/lngv_utils.h>
#include <serverlib/dates_io.h>

#include "etick/ticket.h"
#include "etick/tick_actions.h"
#include "etick/tick_reader.h"
#include "etick/tick_view.h"

#include <serverlib/str_utils.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/test.h>
#include <serverlib/slogger.h>

namespace Ticketing{

void nologtrace(int lv, const char* n, const char* f, int l, const char* s)
{
    ProgTrace(lv,n,f,l,"%s",s);
}

namespace ItinDateStream{
std::ostream & operator << (std::ostream& os, const ItinDate& date)
{
    if (date.d.is_special()) {
        os << "??????";
    } else {
        os << date.d;
    }
    return os;
}

std::ostream & operator << (std::ostream& os, const ItinTime& time)
{
    if (time.t.is_special()) {
        os << "????";
    } else {
        os << time.t;
    }
    return os;
}
}

std::ostream & operator <<(std::ostream & os, const BaseOrigOfRequest & org)
{
    os << "'" << org.airlineCode() << "'/'" << org.locationCode() << "', '"
       << org.pprNumber() << "'/'" << org.agencyCode() << "', '"
       << org.originLocationCode() << "', '" << org.type() << "', "
       << "authCode: '" << org.authCode() << "', "
       << "pult: '" << org.pult() << "', "
       << "lang: '" << (org.lang() == RUSSIAN ? "RU" : "EN") << "', "
       << "currency: '" << org.currCode() << "'";

    return os;
}

std::ostream & operator <<(std::ostream & os, const BaseMonetaryInfo & monin)
{
    os << monin.code()->code() << ":";
    if(monin.monType())
        os << monin.monType() << "/";
    os << monin.amValue().amStr() << ":" << monin.currCode();

    return os;
}

std::ostream & operator <<(std::ostream & os, const TourCode_t & tc)
{
    os << tc.code();
    return os;
}

std::ostream & operator <<(std::ostream & os, const CommonTicketData & ctd)
{
    os << "TourCode: " << ctd.tourCode();
    os << "; "
          "Ticketing Agent Info: " << ctd.ticketingAgentInfo().companyId() <<
          "+" << ctd.ticketingAgentInfo().agentId() <<
          ":" << ctd.ticketingAgentInfo().agentType();
    return os;
}

void BaseFrequentPass::Trace(int level,const char *nick, const char *file, int line) const
{
    LogTrace(level, nick, file, line) << "FQTV: " << *this;
}

bool BaseFrequentPass::operator == (const BaseFrequentPass &freq) const
{
    if(freq.docnum() == this->docnum() &&
       freq.compCode() == this->compCode() &&
       freq.docnum() == this->docnum() &&
       freq.ticknum() == this->ticknum() &&
       freq.cpnnum() == this->cpnnum())
    {

        LogTrace(TRACE5) << "Equal FrequentPass's";
        return true;
    } else {

        LogTrace(TRACE5) << "Different FrequentPass's";
        return false;
    }
}

std::ostream & operator << (std::ostream &os, const BaseFrequentPass &fpass)
{
    os << fpass.compCode() << ":" << fpass.docnum();
    return os;
}

void BaseOrigOfRequest::Trace(int level, const char * nick, const char * file, int line) const
{
    LogTrace(level, nick, file, line) <<
            "ORG: " << *this;
}

BaseOrigOfRequest::BaseOrigOfRequest(const std::string &airline,
                                     const std::string &location,
                                     const std::string &ppr,
                                     const std::string &agn,
                                     const std::string &originLocation,
                                     char type,
                                     const std::string &pult,
                                     const std::string &authCode,
                                     Language lang)

    :
        AirlineCode(airline),
        LocationCode(location),
        PprNumber(ppr),
        OriginAgnCode(agn),
        OriginLocationCode(originLocation),
        Pult(pult),
        AuthCode(authCode),
        Lang(lang)
{
    Type[0]=type;
    Type[1]=0;
}

bool Ticketing::BaseOrigOfRequest::operator ==(const BaseOrigOfRequest & org) const
{
    if(org.airlineCode() == this->airlineCode() &&
       org.locationCode() == this->locationCode() &&
       org.pprNumber() == this->pprNumber() &&
       org.agencyCode() == this->agencyCode() &&
       org.originLocationCode() == this->originLocationCode() &&
       *org.type() == *this->type() &&
       org.pult() == this->pult() &&
       org.authCode() == this->authCode())
    {

        LogTrace(TRACE5) << "Equal ORG's";
        return true;
    }
    else
    {
        LogTrace(TRACE5) <<
                org.airlineCode() << "==" << this->airlineCode() << ", " <<
                org.locationCode() << "==" << this->locationCode() << ", " <<
                org.pprNumber() << "==" << this->pprNumber() << ", " <<
                org.agencyCode() << "==" << this->agencyCode() << ", " <<
                org.originLocationCode() << "==" <<
                this->originLocationCode() << ", " <<
                org.type() << "==" << this->type() << ", " <<
                org.pult() << "==" << this->pult() << ", " <<
                org.authCode() << "==" << this->authCode();
        LogTrace(TRACE5) << "Different ORG's";
        return false;
    }
}

std::string BaseOrigOfRequest::langStr() const
{
    return LangStr(lang());
}

BaseCoupon_info::BaseCoupon_info(unsigned n, const CouponStatus & stat,
                                 const TicketMedia & md, const std::string & sac)
    :
        Num(n),
        Status(stat),
        Media(md),
        Sac(sac)
{
}

bool BaseCoupon_info::operator ==(const BaseCoupon_info & ci) const
{
    if(ci.sac() == this->sac() &&
       ci.media() == this->media() &&
       ci.num() == this->num() &&
       ci.status()->codeInt() == this->status()->codeInt()){

        LogTrace(TRACE5) << "Equal Coupon_info";
        return true;
       } else {

           LogTrace(TRACE5) << "Different Coupon_info";
           return false;
       }
}

void BaseCoupon_info::Trace(int level, const char * nick, const char * file, int line) const
{
    LogTrace(level, nick, file, line) <<
                  "Coupon: " << *this;
}

std::ostream & operator <<(std::ostream & os, const BaseCoupon_info & cpn)
{
    os
            << "num: " << cpn.num()
            << ") Stat:"     << cpn.status()
            << " Media:"     << cpn.media()
            << " SAC:" << cpn.sac()
            << " action: ";
    if(cpn.actionCode())
        os << CpnStatAction::CpnActionTraceStr(cpn.actionCode().get());
    else
        os << "None";
    if(cpn.connectedCpnNum())
        os << " connectedNum: " << cpn.connectedCpnNum().get();

    return os;
}



bool BaseLuggage::operator ==(const BaseLuggage & lugg) const
{
    if((Bagg && lugg.Bagg && *lugg.Bagg.get() == *Bagg.get()) || (!Bagg && !lugg.Bagg))
    {
        LogTrace(6,"ROMAN",__FILE__,__LINE__) << "Equal Luggage's";
        return true;
    }
    else
    {
        LogTrace(6,"ROMAN",__FILE__,__LINE__) << "Different Luggage's";
        return false;
    }
}

std::ostream & operator <<(std::ostream & os, const BaseLuggage & lugg)
{
    if(lugg.haveLuggage())
        os << lugg->quantity() << ":" << lugg->charge() << ":" << lugg->measure();
    else
        os << "No luggage.";
    return os;
}

void BaseLuggage::Trace(int level, const char * nick, const char * file, int line) const
{
    LogTrace(level, nick, file, line) << "Luggage: " << *this;
}



bool BaseFormOfId::operator ==(const BaseFormOfId & foid) const
{
    if(type() == foid.type() &&
       number() == foid.number() &&
       owner() ==  foid.owner())
    {
        LogTrace(TRACE5) << "Equal Foids";
        return true;
    }
    else
    {
        LogTrace(TRACE5) << "Different Foids";
        return false;
    }
}

std::ostream & operator <<(std::ostream & os, const BaseFormOfId & foid)
{
    os << foid.type()->code() << ":" << foid.number() << ":" << foid.owner();
    return os;
}

void BaseFormOfId::Trace(int level, const char * nick, const char * file, int line) const
{
    LogTrace(level, nick, file, line) <<
                  "FOID " << *this;
}

void BaseFormOfId::Trace(int level, const char * nick, const char * file, int line, const std::list< BaseFormOfId > & lfoid)
{
    for_each(lfoid.begin(),lfoid.end(),
             TraceElement<BaseFormOfId>(level,nick,file,line));

}

const Baggage * BaseLuggage::operator ->() const
{
    if(haveLuggage())
        return Bagg.get();
    else
        throw TickExceptions::tick_fatal_except(STDLOG, EtErr::ProgErr, "no baggage");
}

void BaseMonetaryInfo::Trace(int level,const char *nick, const char *file, int line) const
{
    LogTrace(level, nick, file, line) << "MON: " << *this;
}

bool BaseMonetaryInfo::operator ==(const BaseMonetaryInfo & mon) const
{
    if(mon.currCode() == this->currCode() &&
       mon.amValue().amValue() == this->amValue().amValue() &&
       mon.code()->codeInt() == this->code()->codeInt() &&
       ((!mon.monType() && !this->monType()) || (mon.monType() == this->monType())))
    {

        LogTrace(TRACE5) << "Equal MonetaryInfo's";
        return true;
    } else {
        LogTrace(TRACE5) << "Different MonetaryInfo's";
        LogTrace(TRACE5) << "Mon left: " << *this <<
                 "\nMon right: " << mon;
        return false;
    }
}

void BaseMonetaryInfo::setAddCollect(bool c)
{
    AddCollect = c;
}

bool BaseMonetaryInfo::addCollect() const
{
    return AddCollect;
}

bool BaseFormOfPayment::operator ==(const BaseFormOfPayment & fop) const
{
    if(fop.fopCode() == this->fopCode() &&
       fop.fopInd() == this->fopInd() &&
       fop.amValue().amValue() == this->amValue().amValue() &&
       fop.vendor() == this->vendor() &&
       fop.accountNum() == this->accountNum() &&
       fop.expDate() == this->expDate() &&
       fop.appCode() == this->appCode())
    {

        LogTrace(6,"ROMAN",__FILE__,__LINE__) << "Equal FormOfPayment's";
        return true;
    } else {

        LogTrace(6,"ROMAN",__FILE__,__LINE__) << "Different FormOfPayment's";
        return false;
    }
}

std::ostream & operator <<(std::ostream & os, const BaseTaxDetails & taxd)
{
    os << taxd.type() << ":" << taxd.category() << ":";
    if(taxd.amValue().amValue())
        os << taxd.amValue().amStr();
    else
        os << taxd.freeText();
    return os;
}

void BaseTaxDetails::Trace(int level, const char * nick, const char * file, int line) const
{
    LogTrace(level, nick, file, line) << "Tax: " << *this;
}

bool BaseTaxDetails::operator ==(const BaseTaxDetails & txd) const
{
    if(txd.type() == this->type() &&
       txd.amValue().amValue() == this->amValue().amValue() &&
       txd.category() == this->category())
    {

        LogTrace(6,"ROMAN",__FILE__,__LINE__) << "Equal TaxDetails";
        return true;
    } else {

        LogTrace(6,"ROMAN",__FILE__,__LINE__) << "Different TaxDetails";
        return false;
    }
}

void BaseFreeTextInfo::addText(const std::string &txt)
{
    FullText.clear();
    for(size_t l = 0; l < txt.length(); l += 70)
    {
        Text.push_back(txt.substr(l,70));
    }
}

void BaseFreeTextInfo::setStatus(const std::string &stat)
{
    Status = stat;
}

void BaseFreeTextInfo::check() const
{
    if( ((level()==0 || level()==1) && (tickNum().size() || coupon())) ||
          ((tickNum().size()>0 && !coupon()) && level()!=2 ) ||
          ((tickNum().size()>0 &&  coupon()) && level()!=3 ) ||
          level()>3)
    {
            // текст на уровне 0 или 1 не содержит привязки к билету(купону)
            // на уровне 2 дб привязан только к билету
            // на уровне 3 дб привязан к билету и к купону
            // всего 4 уровня
        LogWarning("ROMAN", __FILE__, __LINE__) <<
                "level=" << level() <<
                ", tick=" << tickNum() <<
                ", coupon=" << coupon();
        throw TickExceptions::tick_fatal_except("ROMAN", __FILE__, __LINE__,
                EtErr::ProgErr, "Invalid level value");
    }
}

void BaseFreeTextInfo::Trace(int level,const char *nick, const char *file, int line) const
{
    LogTrace(level, nick, file, line) <<
            "IFT: Level:" << this->level() << " " <<
            qualifier() << ":" << type() << ":" << status() << ":" <<
            fTType()->description();
    for(unsigned i = 0; i < numParts(); i++){
        LogTrace(level, nick, file, line) << this->num() << ":" << this->text(i);
    }
}

void BaseFreeTextInfo::setText(const std::string & txt)
{
    Text.clear();
    addText(txt);
}

bool BaseFreeTextInfo::operator == (const BaseFreeTextInfo &fti) const
{
    if(fti.qualifier() == this->qualifier() &&
       fti.type() == this->type() &&
       fti.fTType()->codeInt() == this->fTType()->codeInt() &&
       fti.text() == this->text() &&
       fti.num() == this->num() &&
       fti.level() == this->level() &&
       fti.tickNum() == this->tickNum() &&
       fti.status() == this->status() &&
       fti.coupon() == this->coupon())
    {

        LogTrace(6,"ROMAN",__FILE__,__LINE__) << "Equal FreeTextInfo's";
        return true;
    } else {

        LogTrace(6,"ROMAN",__FILE__,__LINE__) << "Different FreeTextInfo's";
        return false;
    }
}

const std::string &BaseFreeTextInfo::fullText() const
{
    if(!FullText.size())
        FullText = StrUtils::join("", Text.begin(), Text.end());
    return FullText;
}

BaseFreeTextInfo::BaseFreeTextInfo(unsigned nm, unsigned lvl, const FreeTextType &tp,
                                   const std::vector<std::string> &txt, const std::string &tick,
                                   unsigned cpn)
    : Num(nm), Level(lvl), FTType(tp), TickNum(tick), Coupon(cpn)
{
    addText(StrUtils::join("", txt.begin(), txt.end()));
}

std::ostream & operator <<(std::ostream & os, const BaseResContrInfo & rci)
{
    os << "Issued " << rci.dateOfIssue() << "\n";

    os << "RCI: \n";
    if( !rci.ourAirlineCode().empty() ){
        os <<
                "Our: Awk " << rci.ourAirlineCode() <<
                ", Recloc " << rci.ourRecloc() << "\n";

        os <<
                "Air: Awk " << rci.ourAirlineCode() <<
                ", Recloc " << rci.airRecloc() << "\n";
    }

    if( !rci.crsAirlineCode().empty() )
    {
        os <<
                "Grs: Awk " << rci.crsAirlineCode() <<
                ", Recloc " << rci.crsRecloc() << "\n";
    }

    if(!rci.otherAirlReclocs().empty())
    {
        os << "Other airlines:\n";
        for(BaseResContrInfo::OtherAirlineReclocs::const_iterator iter =
            rci.otherAirlReclocs().begin();
            iter != rci.otherAirlReclocs().end(); iter ++)
        {
            os << "Airl:" << iter->Airline << " Rl:" << iter->Recloc << "\n";
        }
    }
    return os;
}

void BaseResContrInfo::Trace(int level, const char * nick, const char * file, int line) const
{
    LogTrace(level, nick, file, line) << *this;
}

void BaseResContrInfo::setOtherReclocs(const BaseResContrInfo::OtherAirlineReclocs & rl)
{
    OtherReclocs = rl;
}

void BaseResContrInfo::setCrsRecloc(const std::string &airl, const std::string &rl)
{
    CrsAirlineCode = airl;
    CrsRecloc = rl;
}

bool BaseResContrInfo::operator ==(const BaseResContrInfo & rci) const
{
    if(rci.ourAirlineCode() == this->ourAirlineCode() &&
       rci.crsAirlineCode() == this->crsAirlineCode() &&
       rci.ourRecloc() == this->ourRecloc() &&
       rci.crsRecloc() == this->crsRecloc() &&
       rci.airRecloc() == this->airRecloc() &&
       rci.rlIssuedFrom() == this->rlIssuedFrom() &&
       rci.dateOfIssue() == this->dateOfIssue()){

        LogTrace(TRACE5) << "Equal RCI's";
        return true;
       } else {
           LogTrace(TRACE5) << "Different RCI's";
           return false;
       }
}

void BaseResContrInfo::addReclocInfo(const std::string & airl, const std::string & recloc)
{
    OtherReclocs.push_back(ReclocInfo(airl, recloc));
}

void Ticketing::BasePassenger::Trace(int level, const char * nick, const char * file, int line) const
{
    std::ostringstream TraceStr;

    TraceStr << surname() << "/" << name() << "; ";
    if(age())
    {
        TraceStr << "Age: " << age() << "; ";
    }

    if(typeCode().size())
    {
        TraceStr << "Type: " << typeCode() << ";";
    }

    LogTrace(level, nick, file, line) << TraceStr.str();
}

bool Ticketing::BasePassenger::operator ==(const BasePassenger & pass) const
{
    if(pass.surname() == this->surname() &&
       pass.name() == this->name() &&
       pass.age() == this->age() &&
       pass.typeCode() == this->typeCode())
    {

        LogTrace(TRACE4) << "Equal Passenger";
        return true;
    } else {

        LogTrace(TRACE4) << "Different Passenger";
        return false;
    }
}

void BaseFormOfPayment::Trace(int level, const char *nick, const char *file, int line) const
{
    std::ostringstream TraceStr;
    TraceStr << "FOP: " <<
            (fopCode().size()?fopCode():"???") << ":" <<
            AmValue.amStr();

    if(Vendor.size()){
        TraceStr << " " << vendor() << ":" << accountNum() << ":" <<
                HelpCpp::string_cast(expDate(), "%d%b%Y") << ":" << appCode();
    }
    LogTrace(level, nick, file, line) << TraceStr.str();
}

std::string BaseFormOfPayment::accountNumMask() const
{
    std::string AccMask = AccountNum;
    if(AccMask.size() && AccMask.size()>4)
    {
        AccMask.replace(0, AccMask.size()-4, AccMask.size()-4, 'X');
    }
    return AccMask;
}

}
