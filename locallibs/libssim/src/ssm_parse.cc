#include <serverlib/expected.h>
#include <serverlib/str_utils.h>
#include <serverlib/dates.h>

#include <typeb/typeb_message.h>
#include <typeb/AnyStringElem.h>
#include <typeb/ReferenceElem.h>
#include <typeb/SsmElem.h>
#include <typeb/SsmTemplate.h>

#include "ssm_data_types.h"
#include "ssim_parse.h"
#include "ssim_utils.h"
#include "callbacks.h"
#include <serverlib/lngv_user.h>

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace {

static bool isXasm(const std::string& str)
{
    return (str.find("XASM") != std::string::npos);
}

struct EquipOnPeriodLeg
{
    ssim::PeriodInfo pi;
    boost::optional<ssim::EquipmentInfo> eqi;
    boost::optional<ssim::RoutingInfo> ri;
};

std::ostream& operator << (std::ostream& os, const EquipOnPeriodLeg& epr)
{
    os << epr.pi;
    if (epr.eqi)
        os << "\n" << *epr.eqi;
    if (epr.ri)
        os << "\n" << epr.ri->toString();
    return os;
}

struct ConEqtInfo
{
    std::list<ssim::PeriodInfo> periods;
    std::list< std::pair< ssim::EquipmentInfo, boost::optional< ssim::LegChangeInfo > > > equips;
};

static Expected<ssim::PeriodInfo> makeInfoFromPeriod(const typeb_parser::TbElement& elem)
{
    const typeb_parser::SsmLongPeriodElem& e = elem.cast<const typeb_parser::SsmLongPeriodElem>(STDLOG);
    LogTrace(TRACE5) << e;
    boost::gregorian::date dt1 = ssim::guessDate(e.date1());
    boost::gregorian::date dt2 = ssim::guessDate(e.date2());
    if (dt1.is_not_a_date() || dt2.is_not_a_date()) {
        return Message(STDLOG, _("#%1% Incorrect period %2%-%3%")).bind(e.line()).bind(e.date1()).bind(e.date2());
    }

    bool biweekly = false;
    if (e.freqRate() == "/W2") {
        biweekly = true;
    }
    else if (!e.freqRate().empty()) {
        return Message(STDLOG, _("#%1% Incorrect frequency rate")).bind(e.line());
    }

    boost::optional<Period> p = Period::create(dt1, dt2, e.freq(), biweekly);
    if (!p) {
        return Message(STDLOG, _("#%1% Incorrect period %2%-%3% %4%%5%"))
            .bind(e.line()).bind(dt1).bind(dt2).bind(e.freq()).bind(biweekly ? "/W2" : "");
    }

    if (p->empty()) {
        return Message(STDLOG, _("#%1% Empty period %2%")).bind(e.line()).bind(*p);
    }

    LogTrace(TRACE5) << "LongPeriodElem transformed";

    ssim::RawDeiMap map;
    if (!ssim::details::prepareRawDeiMap(map, e.dei())) {
        return Message(STDLOG, _("#%1% Incorrect dei on period %2%")).bind(e.line()).bind(*p);
    }
    ssim::PeriodInfo pi(*p, map);
    if (!pi.fillPartnerInfo()) {
        return Message(STDLOG, _("#%1% Incorrect code-sharing on period %2%")).bind(e.line()).bind(*p);
    }
    return pi;
}

static Expected<ssim::RevFlightInfo> makeInfoFromRevFlight(const typeb_parser::TbElement& elem)
{
    const typeb_parser::SsmRevFlightElem& e = elem.cast<const typeb_parser::SsmRevFlightElem>(STDLOG);
    LogTrace(TRACE5) << e;
    boost::optional<ct::Flight> flt = ct::Flight::fromStr(e.flight());
    if (!flt) {
        return Message(STDLOG, _("#%1% Incorrect flight %2%")).bind(e.line()).bind(e.flight());
    }
    boost::gregorian::date dt1 = ssim::guessDate(e.date1());
    boost::gregorian::date dt2 = ssim::guessDate(e.date2());
    if (dt1.is_not_a_date() || dt2.is_not_a_date()) {
        return Message(STDLOG, _("#%1% Incorrect period %2% - %3%")).bind(e.line()).bind(e.date1()).bind(e.date2());
    }
    bool biweekly = false;
    if (e.freqRate() == "/W2") {
        biweekly = true;
    } else if (!e.freqRate().empty()) {
        return Message(STDLOG, _("#%1% Incorrect frequency rate")).bind(e.line());
    }

    boost::optional<Period> p = Period::create(dt1, dt2, e.freq(), biweekly);
    if (!p) {
        return Message(STDLOG, _("#%1% Incorrect period %2%-%3% %4%%5%"))
            .bind(e.line()).bind(dt1).bind(dt2).bind(e.freq()).bind(biweekly ? "/W2" : "");
    }
    LogTrace(TRACE5) << "RevFlightElem transformed";
    return ssim::RevFlightInfo(*flt, ssim::PeriodInfo(*p));
}

static Expected<ssim::LegChangeInfo> makeInfoFromLegChange(const typeb_parser::TbElement& elem)
{
    const typeb_parser::SsmLegChangeElem& e = elem.cast<const typeb_parser::SsmLegChangeElem>(STDLOG);
    LogTrace(TRACE5) << e;

    ssim::RawDeiMap map;
    if (!ssim::details::prepareRawDeiMap(map, e.dei())) {
        return Message(STDLOG, _("#%1% Incorrect dei in leg change indentifier")).bind(e.line());
    }

    auto pts = ssim::details::parseLegChange(e.legsChange(), e.line());
    if (!pts) {
        return pts.err();
    }
    return ssim::LegChangeInfo(*pts, map);
}

static Message checkUnexpectedDeis(const ssim::PeriodInfoList& ps, const typeb_parser::TbElement& e)
{
    if (std::any_of(ps.begin(), ps.end(), [] (const ssim::PeriodInfo& x) { return !x.di.empty(); })) {
        return Message(STDLOG, _("#%1% Unexpected dei on period")).bind(e.line());
    }
    return Message();
}

static void reportParsedPeriods(const ct::Flight& flt, const ssim::PeriodInfoList& pss, ssim::ParseRequisitesCollector* pc)
{
    if (pc) {
        for (const ssim::PeriodInfo& pi : pss) {
            pc->appendPeriod(flt, pi.period);
        }
    }
}

//#############################################################################
static Expected<ssim::SsmSubmsgPtr> parseSkdRsd(const typeb_parser::TbGroupElement& grp, ssim::SsmActionType type, ssim::ParseRequisitesCollector* pc)
{
    ASSERT(type == ssim::SSM_SKD || type == ssim::SSM_RSD);
    //necessary elements, existence is guaranteed in typeb parser
    const typeb_parser::AnyStringElem & te = grp.byNum(0).cast<const typeb_parser::AnyStringElem>(STDLOG);
    const typeb_parser::SsmShortFlightElem & fle =
        grp.byNum(1).cast<const typeb_parser::SsmShortFlightElem>(STDLOG); //string without dei
    const typeb_parser::SsmSkdPeriodElem & pe = grp.byNum(2).cast<const typeb_parser::SsmSkdPeriodElem>(STDLOG);
    //supplimentary info is ignored as it was in old code

    boost::optional<ct::Flight> flt = ct::Flight::fromStr(fle.flight());
    if (!flt) {
        return Message(STDLOG, _("#%1% Incorrect flight %2%")).bind(fle.line()).bind(fle.flight());
    }

    ssim::details::reportParsedFlight(*flt, pc);

    boost::gregorian::date dt1 = ssim::guessDate(pe.effectDt());
    if (dt1.is_not_a_date()) {
        return Message(STDLOG, _("#%1% Incorrect period beginning %2%")).bind(pe.line()).bind(pe.effectDt());
    }

    boost::gregorian::date dt2;
    if (pe.discontDt().empty()) {
        LogTrace(TRACE0) << "No discontinue date in skd";
        dt2 = boost::gregorian::date(boost::gregorian::pos_infin);
    } else {
        dt2 = ssim::guessDate(pe.discontDt());
        if (dt2.is_not_a_date()) {
            return Message(STDLOG, _("#%1% Incorrect period ending %2%")).bind(pe.line()).bind(pe.discontDt());
       }
    }

    ssim::details::reportParsedPeriod(*flt, Period(dt1, dt2), pc);

    ssim::FlightInfo f(*flt);
    ssim::SkdPeriodInfo spi = { dt1, dt2 };
    if (type == ssim::SSM_SKD) {
        return ssim::SsmSubmsgPtr(new ssim::SsmSkdSubmsg(isXasm(te.toString()), f, spi));
    }
    return ssim::SsmSubmsgPtr(new ssim::SsmRsdSubmsg(f, spi));
}
//#############################################################################
static Expected<ssim::SsmSubmsgPtr> parseNewRpl(const typeb_parser::TbGroupElement& submsg, ssim::SsmActionType tp, ssim::ParseRequisitesCollector* pc)
{
    using namespace typeb_parser;

    ASSERT(tp == ssim::SSM_NEW || tp == ssim::SSM_RPL);
    const AnyStringElem& te = submsg.byNum(0).cast<const AnyStringElem>(STDLOG);
    const bool xasm = isXasm(te.toString());

    Expected<ssim::FlightInfo> fi = ssim::details::makeInfoFromFlight(submsg.byNum(1));
    if (!fi.valid()) {
        return fi.err();
    }

    ssim::details::reportParsedFlight(fi->flt, pc);

    const TbListElement& lst = submsg.byNum(2).elemAsList();
    std::list<EquipOnPeriodLeg> stuff;      //for constructor
    std::list<EquipOnPeriodLeg> currStuff;  //current half-baked structures
    std::string prevElem;
    for (TbListElement::const_iterator it = lst.begin(); it != lst.end(); ++it) {
        //for each group of three : period - equipment - leg
        const TbListElement & three = (*it)->elemAsList();
        for(TbListElement::const_iterator th = three.begin(); th != three.end(); ++th) {
            const std::string elem = (*th)->templ()->accessName();
            LogTrace(TRACE5) << "curr elem " << elem;
            if (elem == "LongPeriod") {
                //it could be period, leg or equipment before this period
                if (prevElem == "Routing") {
                    LogTrace(TRACE1) << "period after leg";
                    //all previous info was handled, currStuff is out of date
                    //its like nothing happened before
                    currStuff.clear();
                }
                //otherwise, previous one was period or equipment
                //either way we need new structure in current list
                Expected<ssim::PeriodInfo> pi = makeInfoFromPeriod(*(*th));
                if (!pi.valid()) {
                    return pi.err();
                }

                ssim::details::reportParsedPeriod(fi->flt, pi->period, pc);

                currStuff.push_back({ *pi });
                LogTrace(TRACE5) << "currStuff ++";
            } else if (elem == "Routing") {
                Expected<ssim::RoutingInfo> ri = ssim::details::makeInfoFromSsmRouting(*(*th));
                if (!ri.valid()) {
                    return ri.err();
                }
                if (currStuff.empty()) {
                    return Message(STDLOG, _("#%1% Incorrect string sequence")).bind((*th)->line());
                }
                for (const EquipOnPeriodLeg & ps : currStuff) {
                    if (!ps.eqi) {
                        return Message(STDLOG, _("#%1% Plane type missing")).bind((*th)->line());
                    }
                    stuff.push_back(ps);
                    stuff.back().ri = *ri;
                    LogTrace(TRACE5) << "routing updated; stuff ++";
                }
            } else /* elem == "Equipment" -- most complicated case */ {
                if (prevElem == elem) {
                    return Message(STDLOG, _("#%1% Incorrect string sequence")).bind((*th)->line());
                }
                Expected<ssim::EquipmentInfo> currEq = ssim::details::makeInfoFromEquip(*(*th));
                if (!currEq.valid()) {
                    return currEq.err();
                }
                //elem == Equipment and prevElem == Routing means that sequence is like
                //p1 - e1 - r1 - e2 -....
                //we'll wait for new legs after this. and for them equipment should be current one, not old
                bool needTotalRewrite = (prevElem == "Routing");
                //when we found routing we'd flushed accumulated info into stuff so change of currStuff'll break nothing
                //otherwise only empty equips should be filled
                for (EquipOnPeriodLeg & ps : currStuff) {
                    if (!ps.eqi || needTotalRewrite) {
                        ps.eqi = *currEq;
                    }
                }
            }
            prevElem = elem;
        }
    }
    if (prevElem != "Routing") { //last elem of that block must've been routing -- otherwise it's incomplete
        return Message(STDLOG, _("Incorrect format: no routing information in the end"));
    }
    LogTrace(TRACE5) << "EquipOnPeriodLeg dump";

    //and now merge this stuff into SsmProtoNewStuff
    std::map<ssim::PeriodInfo, ssim::LegStuffList> mp;
    for (const EquipOnPeriodLeg & epr : stuff) {
        LogTrace(TRACE5) << epr;
        //push legs for same periods in one place
        mp[epr.pi].push_back(ssim::LegStuff{*epr.ri, *epr.eqi});
    }
    std::list<ssim::SsmProtoNewStuff> ret;
    for (const auto& e : mp) {
        ret.push_back(ssim::SsmProtoNewStuff { e.first, e.second });
    }

    ssim::SegmentInfoList sil;
    CALL_MSG_RET(ssim::details::separateSegsAndSuppl(submsg, 3, sil));
    if (tp == ssim::SSM_NEW) {
        return ssim::SsmSubmsgPtr(new ssim::SsmNewSubmsg(xasm, *fi, ret, sil));
    }
    return ssim::SsmSubmsgPtr(new ssim::SsmRplSubmsg(xasm, *fi, ret, sil));
}
//#############################################################################
static Expected<ssim::SsmSubmsgPtr> parseEqtCon(const typeb_parser::TbGroupElement& grp, ssim::SsmActionType type, ssim::ParseRequisitesCollector* pc)
{
    using namespace typeb_parser;
    ASSERT(type == ssim::SSM_EQT || type == ssim::SSM_CON);
    //first - string with tlg type and xasm
    //second - flight string
    Expected<ssim::FlightInfo> fi = ssim::details::makeInfoFromFlight(grp.byNum(1));
    if (!fi.valid()) {
        return fi.err();
    }

    ssim::details::reportParsedFlight(fi->flt, pc);

    std::list<ConEqtInfo> stash;
    auto cpi = stash.begin();

    std::string prevElem;

    const typeb_parser::TbListElement & lst = grp.byNum(2).elemAsList();
    for (TbListElement::const_iterator it = lst.begin(); it != lst.end(); ++it) {
        //for each group of three : period - equipment - lci
        const TbListElement& three = (*it)->elemAsList();
        for (TbListElement::const_iterator th = three.begin(); th != three.end(); ++th) {
            const std::string elem = (*th)->templ()->accessName();
            LogTrace(TRACE5) << "curr elem " << elem;

            if (elem == "LongPeriod") {
                Expected<ssim::PeriodInfo> pi = makeInfoFromPeriod(*(*th));
                if (!pi) {
                    return pi.err();
                }

                ssim::details::reportParsedPeriod(fi->flt, pi->period, pc);

                if (prevElem.empty() || prevElem != elem) {
                    cpi = stash.insert(stash.end(), ConEqtInfo { {*pi} });
                } else {
                    cpi->periods.push_back(*pi);
                }
            } else if (elem == "Equipment") {
                if (prevElem.empty() || prevElem == elem) {
                    return Message(STDLOG, _("#%1% Incorrect string sequence")).bind((*th)->line());
                }

                Expected<ssim::EquipmentInfo> ei = ssim::details::makeInfoFromEquip(*(*th));
                if (!ei) {
                    return ei.err();
                }
                cpi->equips.emplace_back(*ei, boost::none);
            } else if (elem == "LegChange") {
                if (prevElem != "Equipment") {
                    return Message(STDLOG, _("#%1% Incorrect string sequence")).bind((*th)->line());
                }

                Expected<ssim::LegChangeInfo> li = makeInfoFromLegChange(*(*th));
                if (!li) {
                    return li.err();
                }
                cpi->equips.back().second = *li;
            } else {
                return Message(STDLOG, _("#%1% Incorrect string sequence")).bind((*th)->line());
            }
            prevElem = elem;
        }
    }

    std::list<ssim::ProtoEqtStuff> stuff;
    for (const ConEqtInfo& v : stash) {
        for (const ssim::PeriodInfo& pi : v.periods) {
            for (const auto& ei : v.equips) {
                stuff.emplace_back(ssim::ProtoEqtStuff { pi, ei.first, ei.second });
            }
        }
    }
    if (stuff.empty()) {
        return Message(STDLOG, _("Cannot find reasonable combination of period/equipment/leg-change"));
    }

    ssim::SegmentInfoList sil;
    CALL_MSG_RET(ssim::details::separateSegsAndSuppl(grp, 3, sil, false, ssim::details::isSegDeiAllowed_EqtCon));

    if (type == ssim::SSM_EQT) {
        return ssim::SsmSubmsgPtr(std::make_shared<ssim::SsmEqtSubmsg>(*fi, stuff, sil));
    }
    return ssim::SsmSubmsgPtr(std::make_shared<ssim::SsmConSubmsg>(*fi, stuff, sil));
}
//#############################################################################
static Expected<ssim::SsmSubmsgPtr> parseTim(const typeb_parser::TbGroupElement& submsg, ssim::ParseRequisitesCollector* pc)
{
    const typeb_parser::SsmShortFlightElem & fle = submsg.byNum(1).cast<const typeb_parser::SsmShortFlightElem>(STDLOG);
    boost::optional<ct::Flight> flt = ct::Flight::fromStr(fle.flight());
    if (!flt) {
        return Message(STDLOG, _("#%1% Incorrect flight")).bind(fle.line());
    }

    ssim::details::reportParsedFlight(*flt, pc);

    const typeb_parser::TbListElement & lst = submsg.byNum(2).elemAsList();
    std::list<ssim::SsmTimStuff> stuff;
    for (typeb_parser::TbListElement::const_iterator it = lst.begin(); it != lst.end(); ++it) {
        const typeb_parser::TbGroupElement & grp = (*it)->elemAsGroup();
        //every such group has one or more period and one or more legs
        Expected<ssim::PeriodInfoList> plist = ssim::details::transformList(grp.byNum(0).elemAsList(), makeInfoFromPeriod);
        if (!plist.valid()) {
            return plist.err();
        }

        reportParsedPeriods(*flt, *plist, pc);

        CALL_MSG_RET(checkUnexpectedDeis(*plist, grp.byNum(0)));

        Expected<ssim::RoutingInfoList> rlist = ssim::details::transformList(grp.byNum(1).elemAsList(), ssim::details::makeInfoFromSsmRouting);
        if (!rlist.valid()) {
            return rlist.err();
        }
        for (const ssim::RoutingInfo& ri : *rlist) {
            if (std::any_of(ri.di.begin(), ri.di.end(), [] (const auto& x) { return x.first != ct::DeiCode(7); })) {
                return Message(STDLOG, _("#%1% Unexpected dei on period")).bind(grp.byNum(1).line());
            }
        }
        for (const ssim::PeriodInfo& pi : *plist) {
            stuff.push_back(ssim::SsmTimStuff { pi, *rlist });
        }
    }
    ssim::SegmentInfoList sil;
    CALL_MSG_RET(ssim::details::separateSegsAndSuppl(submsg, 3, sil, false, ssim::details::isSegDeiAllowed_Tim));
    return ssim::SsmSubmsgPtr(new ssim::SsmTimSubmsg(ssim::FlightInfo(*flt), stuff, sil));
}
//#############################################################################
static Expected<ssim::SsmSubmsgPtr> parseFlt(const typeb_parser::TbGroupElement& grp, ssim::ParseRequisitesCollector* pc)
{
    const typeb_parser::SsmShortFlightElem & fle = grp.byNum(1).cast<const typeb_parser::SsmShortFlightElem>(STDLOG); //no dei in regexp
    boost::optional<ct::Flight> flt = ct::Flight::fromStr(fle.flight());
    if (!flt) {
        return Message(STDLOG, _("#%1% Incorrect flight")).bind(fle.line());
    }

    ssim::details::reportParsedFlight(*flt, pc);

    Expected<ssim::PeriodInfoList> pil = ssim::details::transformList(grp.byNum(2).elemAsList(), makeInfoFromPeriod);
    if (!pil.valid()) {
        return pil.err();
    }

    reportParsedPeriods(*flt, *pil, pc);

    CALL_MSG_RET(checkUnexpectedDeis(*pil, grp.byNum(2)));

    const typeb_parser::SsmShortFlightElem & newFle = grp.byNum(3).cast<const typeb_parser::SsmShortFlightElem>(STDLOG); //no dei in regexp
    boost::optional<ct::Flight> newFlt = ct::Flight::fromStr(newFle.flight());
    if (!newFlt) {
        return Message(STDLOG, _("#%1% New flight is incorrect")).bind(newFle.line());
    }

    ssim::FlightInfo f(*flt), newf(*newFlt);
    ssim::SegmentInfoList sil;
    CALL_MSG_RET (ssim::details::separateSegsAndSuppl(grp, 4, sil, false, ssim::details::isSegDeiAllowed_Flt));
    return ssim::SsmSubmsgPtr(new ssim::SsmFltSubmsg(f, newf, *pil, sil));
}
//#############################################################################
static Expected<ssim::SsmSubmsgPtr> parseRev(const typeb_parser::TbGroupElement& grp, ssim::ParseRequisitesCollector* pc)
{
    Expected<ssim::RevFlightInfo> fi = makeInfoFromRevFlight(grp.byNum(1));
    if (!fi.valid()) {
        return fi.err();
    }

    ssim::details::reportParsedPeriod(fi->flt, fi->pi.period, pc);

    //Typeb parser guarantees that there'll be at least one period element
    Expected<ssim::PeriodInfoList> pil = ssim::details::transformList(grp.byNum(2).elemAsList(), makeInfoFromPeriod);
    if (!pil.valid()) {
        return pil.err();
    }

    reportParsedPeriods(fi->flt, *pil, pc);

    CALL_MSG_RET(checkUnexpectedDeis(*pil, grp.byNum(2)));

    return ssim::SsmSubmsgPtr(new ssim::SsmRevSubmsg(*fi, *pil));
}
//#############################################################################
static Expected<ssim::SsmSubmsgPtr> parseAdm(const typeb_parser::TbGroupElement &submsg, ssim::ParseRequisitesCollector* pc)
{
    Expected<ssim::FlightInfo> flt = ssim::details::makeInfoFromFlight(submsg.byNum(1));
    if (!flt.valid()) {
        return flt.err();
    }

    ssim::details::reportParsedFlight(flt->flt, pc);

    Expected<ssim::PeriodInfoList> plist = ssim::details::transformList(submsg.byNum(2).elemAsList(), makeInfoFromPeriod);
    if (!plist.valid()) {
        return plist.err();
    }

    reportParsedPeriods(flt->flt, *plist, pc);

    ssim::SegmentInfoList sil;
    CALL_MSG_RET(ssim::details::separateSegsAndSuppl(submsg, 3, sil, true));
    return ssim::SsmSubmsgPtr(new ssim::SsmAdmSubmsg(ssim::FlightInfo(*flt), *plist, sil));
}
//#############################################################################
static Expected<ssim::SsmSubmsgPtr> parseCnl(const typeb_parser::TbGroupElement& grp, ssim::ParseRequisitesCollector* pc)
{
    const bool xasm = isXasm(grp.byNum(0).cast<typeb_parser::AnyStringElem>(STDLOG).toString());
    const typeb_parser::SsmShortFlightElem& fle = grp.byNum(1).cast<const typeb_parser::SsmShortFlightElem>(STDLOG); //string only

    boost::optional<ct::Flight> flt = ct::Flight::fromStr(fle.flight());
    if (!flt) {
        return Message(STDLOG, _("#%1% Incorrect flight %2%")).bind(fle.line()).bind(fle.flight());
    }

    ssim::details::reportParsedFlight(*flt, pc);

    //Typeb parser guarantees that there'll be at least one period element
    Expected<ssim::PeriodInfoList> periods = ssim::details::transformList(grp.byNum(2).elemAsList(), makeInfoFromPeriod);
    if (!periods.valid()) {
        return periods.err();
    }

    reportParsedPeriods(*flt, *periods, pc);

    CALL_MSG_RET(checkUnexpectedDeis(*periods, grp.byNum(2)));

    ssim::FlightInfo f(*flt); //no DEI
    return ssim::SsmSubmsgPtr(new ssim::SsmCnlSubmsg(xasm, f, *periods));
}
//#############################################################################
static Expected<ssim::SsmSubmsgPtr> parseNac(const typeb_parser::TbGroupElement& grp)
{
    using namespace typeb_parser;

    static const std::string flightElemName = typeb_parser::SsmFlightTemplate().name();
    static const std::string periodElemName = typeb_parser::SsmLongPeriodTemplate().name();

    const auto rejInfoList = std::find_if(grp.begin(), grp.end(), [] (const auto el) {
            return el->name() == "List of RejectInfo element";
            });
    const auto rejMsgList = std::find_if(grp.begin(), grp.end(), [] (const auto el) {
            return el->name() == "List of Group of elements";
            });

    std::string errors;
    for (const auto& rejElem: (*rejInfoList)->elemAsList()) { // RejectInfo list
        const SsmRejectInfoElem & re = rejElem->cast<const SsmRejectInfoElem>(STDLOG);
        errors += re.txt();
    }

    boost::optional<ssim::FlightInfo> fltInfo;
    ssim::PeriodInfoList periodsInfo;

    for (const auto& rejMsg: (*rejMsgList)->elemAsList()) { // Rejected message
        const auto& curElement = rejMsg->elemAsGroup().byNum(0);
        if (curElement.name() == flightElemName && !fltInfo) {
            Expected<ssim::FlightInfo> fltInfoTmp = ssim::details::makeInfoFromFlight(curElement);
            if (!fltInfoTmp.valid()) {
                return fltInfoTmp.err();
            }
            fltInfo = *fltInfoTmp;
        } else if (curElement.name() == periodElemName) {
            Expected<ssim::PeriodInfo> periodInfoTmp = makeInfoFromPeriod(curElement);
            if (!periodInfoTmp.valid()) {
                return periodInfoTmp.err();
            }
            periodsInfo.push_back(*periodInfoTmp);
        }
    }

    if (!fltInfo) {
        return Message(STDLOG, _("No flight found in SSM_NAC"));
    }

    return ssim::SsmSubmsgPtr(new ssim::SsmNacSubmsg(*fltInfo, errors, periodsInfo));
}
//#############################################################################
static Expected<ssim::SsmSubmsgPtr> createSubmessage(ssim::SsmActionType tp, const typeb_parser::TbGroupElement& grp, ssim::ParseRequisitesCollector* pc)
{
    if (tp == ssim::SSM_SKD || tp == ssim::SSM_RSD) {
        return parseSkdRsd(grp, tp, pc);
    }
    if (tp == ssim::SSM_NEW || tp == ssim::SSM_RPL) {
        return parseNewRpl(grp, tp, pc);
    }
    if (tp == ssim::SSM_EQT || tp == ssim::SSM_CON) {
        return parseEqtCon(grp, tp, pc);
    }
    if (tp == ssim::SSM_TIM) {
        return parseTim(grp, pc);
    }
    if (tp == ssim::SSM_FLT) {
        return parseFlt(grp, pc);
    }
    if (tp == ssim::SSM_REV) {
        return parseRev(grp, pc);
    }
    if (tp == ssim::SSM_ADM) {
        return parseAdm(grp, pc);
    }
    if (tp == ssim::SSM_CNL) {
        return parseCnl(grp, pc);
    }
    if (tp == ssim::SSM_NAC) {
        return parseNac(grp);
    }
    return Message(STDLOG, _("Message type is not supported"));
}

} //anonymous

namespace ssim {

Expected<SsmStruct> parseSsm(const std::string& s_ssm, ParseRequisitesCollector* pc)
{
    boost::optional<typeb_parser::TypeBMessage> msg;
    try {
        msg = typeb_parser::TypeBMessage::parse(s_ssm);
    } catch (typeb_parser::typeb_parse_except & e) {
        LogTrace(TRACE5) << e.what();
        return Message(STDLOG, _("Invalid tlg: line %1%")).bind(e.errLine());
    }

    LogTrace(TRACE5) << *msg;

    SsmStruct out;
    if (auto m = ssim::details::fillHead(*msg, out.head)) {
        return m;
    }

    //switch of submsgs
    const typeb_parser::TbListElement& submsgs = msg->find("Submsgs").elemAsList();

    bool withSKD = false;

    for (typeb_parser::TbListElement::const_iterator it = submsgs.begin(); it != submsgs.end(); ++it) {
        const typeb_parser::TbGroupElement & grp = (*it)->elemAsGroup().byNum(0).elemAsGroup();
        //'it' is submsg which is group of 1 element (switch). switch is group of our elements
        //first element is a string with type
        const typeb_parser::AnyStringElem & type = grp.byNum(0).cast<const typeb_parser::AnyStringElem>(STDLOG);
        ssim::SsmActionType action;
        if (!ssim::enumFromStr(action, type.toString().substr(0, 3))) {
            return Message(STDLOG, _("Invalid type of submsg: %1%")).bind(type.toString().substr(0, 3));
        }
        if (action == ssim::SSM_ACK) {
            LogTrace(TRACE5) << "ACK received";
            out.specials.push_back(SsmSubmsgPtr(new SsmAckSubmsg));
            continue;
            //NOTE на самом деле это не является необходимым -- если парсер найдет NAC или ACK,
            //то все последующие строки автоматически попадут в это сообщение (см. SsmTemplate, где
            //выставлено бесконечное число элементов, подходящих под .*). Поэтому никаких других групп,
            //сообщений и т.п. после NAC/ACK не найдется в любом случае.
        }

        if (action == ssim::SSM_SKD) {
            withSKD = true;
        }

        Expected<SsmSubmsgPtr> p = createSubmessage(action, grp, pc);
        if (!p.valid()) {
            LogTrace(TRACE0) << p.err().toString(UserLanguage::en_US());
            return p.err();
        }

        if (action == ssim::SSM_NAC) {
            out.specials.push_back(*p);
            continue;
        }

        out.submsgs[(*p)->fi.flt].push_back(*p);
    }

    if (!out.specials.empty() && !out.submsgs.empty()) {
        return Message(STDLOG, _("NAC/ACK cannot be used with other sub-messages"));
    }
    if (out.specials.empty() && out.submsgs.empty()) {
        return Message(STDLOG, _("No sub-messages in tlg"));
    }
    if (out.submsgs.size() > 1 && withSKD) {
        return Message(STDLOG, _("SSM with SKD sub-message cannot contain several flights"));
    }
    LogTrace(TRACE5) << out.toString();
    return out;
}

} //ssim
