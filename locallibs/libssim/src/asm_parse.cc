#include <boost/lexical_cast.hpp>

#include <serverlib/dates.h>

#include <libssim/asm_data_types.h>
#include <libssim/ssim_parse.h>

#include <typeb/typeb_message.h>
#include <typeb/AnyStringElem.h>
#include <typeb/AsmTypes.h>
#include <serverlib/lngv_user.h>

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace {

static int dateShift(int daysOffset, const boost::gregorian::date& base)
{
    if (daysOffset == 0) {
        return 0;
    }

    //если есть сдвиг, то он - число месяца, см. разборщик ASM
    if ((base - boost::gregorian::days(1)).day() == daysOffset) {
        // действительно, вчера
        return -1;
    }
    if (daysOffset < base.day()) {
        // рейд 30, вылет из промежуточного пункта 1
        return daysOffset + (base.end_of_month().day() - base.day());
    }
    return daysOffset - base.day(); // рейд 20, вылет 22
}

// для ASM - нужно пересчитать временнЫе отклонения в терминах удаленности от даты рейда
// проверять, что отклонения меньше чем ... , мы здесь не будем
static void rebaseRouting(ssim::RoutingInfoList& legs, const boost::gregorian::date& dt)
{
    for (ssim::RoutingInfo& ri : legs) {
        LogTrace(TRACE5) << "dep|arr " << ri.depTime << " " << ri.arrTime;
        int depDays = dateShift(Dates::daysOffset(ri.depTime).days(), dt); // считаем сдвиг по дате
        int arrDays = dateShift(Dates::daysOffset(ri.arrTime).days(), dt);
        ri.depTime = Dates::dayTime(ri.depTime) + boost::posix_time::hours(24 * depDays);
        ri.arrTime = Dates::dayTime(ri.arrTime) + boost::posix_time::hours(24 * arrDays);
        LogTrace(TRACE5) << "rebase dep|arr " << ri.depTime << " " << ri.arrTime;
    }
}
//#############################################################################
static boost::optional<ct::FlightDate> parseFlightId(const std::string& fid)
{
    if (fid.empty()) {
        return boost::none;
    }
    size_t np = fid.find('/');
    assert(np != std::string::npos);
    const std::string fl = fid.substr(0, np);
    const std::string dt = fid.substr(np + 1);

    const boost::optional<ct::Flight> flt = ct::Flight::fromStr(fl);
    if (!flt) {
        LogTrace(TRACE5) << "Bad flight " << fl;
        return boost::none;
    }
    boost::gregorian::date date1;
    const boost::gregorian::date now = Dates::currentDate();
    if (dt.length() == 2) {
        //day only 01
        //must be within 3 days from now
        int d = boost::lexical_cast<int>(dt);
        if (d >= now.day()) {
            // дата еще будет, этот месяц
            date1 = boost::gregorian::date(now.year(), now.month(), d);
        } else {
            // это уже следующий месяц
            date1 = now + boost::gregorian::months(1);
            date1 = boost::gregorian::date(date1.year(), date1.month(), d); //может и год смениться, если сейчас 31 декабря
        }
        if (date1.is_not_a_date() || std::abs((date1 - now).days()) > 3) {
            LogTrace(TRACE5) << "bad date" << dt;
            return boost::none;
        }
    } else if (dt.length() == 5) {
        //day and month 01JAN
        //must be within 11 months from now
        date1 = Dates::DateFromDDMON(dt, Dates::GuessYear_Itin(), now);
        if (date1.is_not_a_date() || std::abs((date1 - now).days()) > 330) { //11 months
            LogTrace(TRACE5) << "bad date " << dt;
            return boost::none;
        }
    } else {
        //day month year 01JAN99
        date1 = Dates::DateFromDDMONYY(dt, Dates::YY2YYYY_UseCurrentCentury, now);
        if (date1.is_not_a_date()) {
            LogTrace(TRACE5) << "bad date " << dt;
        }
    }
    return ct::FlightDate { *flt, date1 };
}

static Expected<ssim::FlightIdInfo> makeInfoFromFlightId(const typeb_parser::TbElement& elem)
{
    const typeb_parser::AsmFlightElem& e = elem.cast<const typeb_parser::AsmFlightElem>(STDLOG);
    LogTrace(TRACE5) << e;
    boost::optional<ct::FlightDate> flt = parseFlightId(e.flightDate());
    if (!flt) {
        return Message(STDLOG, _("#%1% Incorrect flight %2%")).bind(e.line()).bind(e.flightDate());
    }

    boost::optional<nsi::Points> lgi;
    if (!e.legChange().empty()) {
        auto pts = ssim::details::parseLegChange(e.legChange(), e.line());
        if (!pts) {
            return pts.err();
        }
        lgi = *pts;
    }

    boost::optional<ct::FlightDate> newfd = parseFlightId(e.newFlightDate());
    ssim::RawDeiMap map;
    if (!ssim::details::prepareRawDeiMap(map, e.dei())) {
        return Message(STDLOG, _("#%1% Incorrect dei in line")).bind(e.line());
    }

    ssim::FlightIdInfo ret(*flt, newfd, map);
    if (!ret.fillPartnerInfo()) {
        return Message(STDLOG, _("#%1% Incorrect partner info")).bind(e.line());
    }
    ret.points = lgi;
    return ret;
}
//#############################################################################
static Expected<ssim::AsmSubmsgPtr> parseNewRpl(
        ssim::AsmActionType t, const ssim::FlightIdInfo& fid,
        const typeb_parser::TbGroupElement& grp
    )
{
    ASSERT(t == ssim::ASM_NEW || t == ssim::ASM_RPL);
    if (fid.newFld) {
        return Message(STDLOG, _("Prohibited symbols at the end of flight string"));
    }
    const typeb_parser::TbListElement& lst = grp.byNum(2).elemAsList();
    ssim::LegStuffList legStuff;
    for (typeb_parser::TbListElement::const_iterator it = lst.begin(); it != lst.end(); ++it) {
        const typeb_parser::TbGroupElement& cur = (*it)->elemAsGroup();
        Expected<ssim::EquipmentInfo> eo = ssim::details::makeInfoFromEquip(cur.byNum(0));
        if (!eo.valid()) {
            return eo.err();
        }
        Expected<ssim::RoutingInfoList> rl = ssim::details::transformList(cur.byNum(1).elemAsList(), ssim::details::makeInfoFromAsmRouting);
        if (!rl.valid()) {
            return rl.err();
        }
        rebaseRouting(*rl, fid.fld.dt);
        for (const ssim::RoutingInfo& ri : *rl) {
            legStuff.push_back(ssim::LegStuff{ri, *eo});
        }
    }
    ssim::SegmentInfoList segs;
    CALL_MSG_RET(ssim::details::separateSegsAndSuppl(grp, 3, segs));
    if (t == ssim::ASM_NEW) {
        return ssim::AsmSubmsgPtr(new ssim::AsmNewSubmsg(fid, legStuff, segs));
    }
    return ssim::AsmSubmsgPtr(new ssim::AsmRplSubmsg(fid, legStuff, segs));
}
//#############################################################################
static Expected<ssim::AsmSubmsgPtr> parseCnlRin(ssim::AsmActionType t, const ssim::FlightIdInfo& fid, const typeb_parser::TbGroupElement& grp)
{
    ASSERT(t == ssim::ASM_CNL || t == ssim::ASM_RIN);

    //no additional flight in string
    //no dei in flight for cnl
    if (fid.newFld || !fid.di.empty()) {
        return Message(STDLOG, _("Prohibited symbols at the end of flight string"));
    }
    if (t == ssim::ASM_CNL) {
        return ssim::AsmSubmsgPtr(new ssim::AsmCnlSubmsg(fid));
    }
    return ssim::AsmSubmsgPtr(new ssim::AsmRinSubmsg(fid));
}
//#############################################################################
static Expected<ssim::AsmSubmsgPtr> parseEqtCon(ssim::AsmActionType t, const ssim::FlightIdInfo& fid, const typeb_parser::TbGroupElement& grp)
{
    ASSERT(t == ssim::ASM_EQT || t == ssim::ASM_CON);
    if (fid.newFld || (t == ssim::ASM_CON && !fid.di.empty())) { // no deis in con
        return Message(STDLOG, _("Prohibited symbols at the end of flight string"));
    }
    Expected<ssim::EquipmentInfo> equip = ssim::details::makeInfoFromEquip(grp.byNum(2));
    if (!equip.valid()) {
        return equip.err();
    }
    ssim::SegmentInfoList segs;
    CALL_MSG_RET(ssim::details::separateSegsAndSuppl(grp, 3, segs, false, ssim::details::isSegDeiAllowed_EqtCon));
    if (t == ssim::ASM_EQT) {
        return ssim::AsmSubmsgPtr(new ssim::AsmEqtSubmsg(fid, *equip, segs));
    }
    return ssim::AsmSubmsgPtr(new ssim::AsmConSubmsg(fid, *equip, segs));
}
//#############################################################################
static Expected<ssim::AsmSubmsgPtr> parseFlt(const ssim::FlightIdInfo& fid, const typeb_parser::TbGroupElement& grp)
{
    //additional flight in string must be peresent
    if (!fid.newFld) {
        return Message(STDLOG, _("No new flight found"));
    }
    //no deis allowed
    if (!fid.di.empty()) {
        return Message(STDLOG, _("Prohibited symbols at the end of flight string"));
    }
    ssim::SegmentInfoList segs;
    CALL_MSG_RET(ssim::details::separateSegsAndSuppl(grp, 2, segs, false, ssim::details::isSegDeiAllowed_Flt));
    return ssim::AsmSubmsgPtr(new ssim::AsmFltSubmsg(fid, segs));
}
//#############################################################################
static Expected<ssim::AsmSubmsgPtr> parseTim(const ssim::FlightIdInfo& fid, const typeb_parser::TbGroupElement& grp)
{
    //no additional flight in string
    //no dei in flight for tim
    if (fid.newFld || !fid.di.empty()) {
        return Message(STDLOG, _("Prohibited symbols at the end of flight string"));
    }
    Expected<ssim::RoutingInfoList> legs = ssim::details::transformList(grp.byNum(2).elemAsList(), ssim::details::makeInfoFromAsmRouting);
    if (!legs.valid()) {
        return legs.err();
    }
    rebaseRouting(*legs, fid.fld.dt);
    // only dei 7 is allowed in routings
    for (const ssim::RoutingInfo& ri : *legs) {
        for (const auto& dei : ri.di) {
            if (dei.first.get() != 7) {
                return Message(STDLOG, _("Dei %1% on segment isn't supported in this type of tlg"))
                    .bind(dei.first.get());
            }
        }
    }
    ssim::SegmentInfoList segs;
    CALL_MSG_RET(ssim::details::separateSegsAndSuppl(grp, 3, segs, false, ssim::details::isSegDeiAllowed_Tim));
    return ssim::AsmSubmsgPtr(new ssim::AsmTimSubmsg(fid, *legs, segs));
}
//#############################################################################
static Expected<ssim::AsmSubmsgPtr> parseAdm(const ssim::FlightIdInfo& fid, const typeb_parser::TbGroupElement& grp)
{
    //no additional flight in string
    if (fid.newFld) {
        return Message(STDLOG, _("Prohibited symbols at the end of flight string"));
    }
    ssim::SegmentInfoList segs;
    CALL_MSG_RET(ssim::details::separateSegsAndSuppl(grp, 2, segs, true));
    return ssim::AsmSubmsgPtr(new ssim::AsmAdmSubmsg(fid, segs));
}
//#############################################################################
static Expected<ssim::AsmSubmsgPtr> parseRrt(const ssim::FlightIdInfo& fid, const typeb_parser::TbGroupElement& grp)
{
    //no additional flight in string
    //no dei in flight for rrt
    if (fid.newFld || !fid.di.empty()) {
        return Message(STDLOG, _("Prohibited symbols at the end of flight string"));
    }
    boost::optional<ssim::EquipmentInfo> equip;
    const std::string elem = grp.byNum(2).templ()->accessName();
    int offset = 0;
    if (elem == "Equipment") {
        Expected<ssim::EquipmentInfo> eq = ssim::details::makeInfoFromEquip(grp.byNum(2));
        if (!eq.valid()) {
            return eq.err();
        }
        equip = *eq;
        offset = 1;
    }
    Expected<ssim::RoutingInfoList> legs = ssim::details::transformList(grp.byNum(2 + offset).elemAsList(), ssim::details::makeInfoFromAsmRouting);
    if (!legs.valid()) {
        return legs.err();
    }
    rebaseRouting(*legs, fid.fld.dt);

    ssim::SegmentInfoList segs;
    CALL_MSG_RET(ssim::details::separateSegsAndSuppl(grp, 3 + offset, segs));
    return ssim::AsmSubmsgPtr(new ssim::AsmRrtSubmsg(fid, equip, *legs, segs));
}
//#############################################################################
static Expected<ssim::AsmSubmsgPtr> parseNac(const typeb_parser::TbGroupElement& grp)
{
    using namespace typeb_parser;

    static const std::string flightElemName = typeb_parser::AsmFlightTemplate().name();

    const auto rejInfoList = std::find_if(grp.begin(), grp.end(), [] (const auto& el) {
        return el->name() == "List of RejectInfo element";
    });
    const auto rejMsgList = std::find_if(grp.begin(), grp.end(), [] (const auto& el) {
        return el->name() == "List of Group of elements";
    });

    std::string errors;
    for (const auto& rejElem : (*rejInfoList)->elemAsList()) { // RejectInfo list
        const typeb_parser::AsmRejectInfoElem& re = rejElem->cast<const typeb_parser::AsmRejectInfoElem>(STDLOG);
        errors += re.txt();
    }

    for (const auto& curElGroup: (*rejMsgList)->elemAsList()) { // Rejected message
        const auto& curElement = curElGroup->elemAsGroup().byNum(0);
        if (curElement.name() == flightElemName) {
            Expected<ssim::FlightIdInfo> fid = makeInfoFromFlightId(curElement);
            if (!fid) {
                LogTrace(TRACE1) << "Failed to get flight-id: " << fid.err();
                continue;
            }
            return ssim::AsmSubmsgPtr(new ssim::AsmNacSubmsg(*fid, errors));
        }
    }
    return Message(STDLOG, _("No flight found in ASM-NAC"));
}
//#############################################################################
Expected<ssim::AsmSubmsgPtr> createSubmessage(ssim::AsmActionType t, const typeb_parser::TbGroupElement& grp, ssim::ParseRequisitesCollector* pc)
{
    if (t == ssim::ASM_NAC) {
        return parseNac(grp);
    }

    //first element is always flight, so get it here
    Expected<ssim::FlightIdInfo> fid = makeInfoFromFlightId(grp.byNum(1));
    if (!fid.valid()) {
        return fid.err();
    }

    ssim::details::reportParsedPeriod(fid->fld.flt, Period(fid->fld.dt, fid->fld.dt), pc);

    if (t == ssim::ASM_NEW || t == ssim::ASM_RPL) {
        return parseNewRpl(t, *fid, grp);
    }
    if (t == ssim::ASM_CNL || t == ssim::ASM_RIN) {
        return parseCnlRin(t, *fid, grp);
    }
    if (t == ssim::ASM_EQT || t == ssim::ASM_CON) {
        return parseEqtCon(t, *fid, grp);
    }
    if (t == ssim::ASM_FLT) {
        return parseFlt(*fid, grp);
    }
    if (t == ssim::ASM_TIM) {
        return parseTim(*fid, grp);
    }
    if (t == ssim::ASM_ADM) {
        return parseAdm(*fid, grp);
    }
    if (t == ssim::ASM_RRT) {
        return parseRrt(*fid, grp);
    }
    return Message(STDLOG, _("Message type is not supported"));
}

} //anonymous

namespace ssim {

Expected<AsmStruct> parseAsm(const std::string& tlg, ssim::ParseRequisitesCollector* pc)
{
    boost::optional<typeb_parser::TypeBMessage> msg;
    try {
        msg = typeb_parser::TypeBMessage::parse(tlg);
    } catch (typeb_parser::typeb_parse_except & e) {
        return Message(STDLOG, _("Invalid tlg: line %1%")).bind(e.errLine());
    }
    LogTrace(TRACE5) << *msg;

    AsmStruct out;
    if (auto m = ssim::details::fillHead(*msg, out.head)) {
        return m;
    }

    const typeb_parser::TbListElement& submsgs = msg->find("Submsgs").elemAsList();

    for (typeb_parser::TbListElement::const_iterator it = submsgs.begin(); it != submsgs.end(); ++it) {
        const typeb_parser::TbGroupElement& grp = (*it)->elemAsGroup().byNum(0).elemAsGroup();
        const typeb_parser::AnyStringElem& type = grp.byNum(0).cast<const typeb_parser::AnyStringElem>(STDLOG);

        ssim::AsmActionType tp;
        if (!ssim::enumFromStr(tp, type.toString().substr(0,3))) {
            return Message(STDLOG, _("Invalid type of submsg: %1%")).bind(type.toString().substr(0,3));
        }

        if (tp == ssim::ASM_ACK) {
            LogTrace(TRACE5) << "ACK received";
            out.specials.push_back(AsmSubmsgPtr(new AsmAckSubmsg));
            continue;
        }

        Expected<AsmSubmsgPtr> p = createSubmessage(tp, grp, pc);
        if (!p) {
            LogTrace(TRACE0) << p.err().toString(UserLanguage::en_US());
            return p.err();
        }

        if (tp == ssim::ASM_NAC) {
            out.specials.push_back(*p);
            continue;
        }

        out.submsgs.push_back(*p);
    }

    if (!out.specials.empty() && !out.submsgs.empty()) {
        return Message(STDLOG, _("NAC/ACK cannot be used with other sub-messages"));
    }
    if (out.specials.empty() && out.submsgs.empty()) {
        return Message(STDLOG, _("No sub-messages in tlg"));
    }

    return out;
}

} //ssim
