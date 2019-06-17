#include "custom_alarms.h"
#include "dcs_services.h"
#include "rfisc.h"
#include "brands.h"
#include "checkin.h"
#include "alarms.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;

class TicketCustomAlarmCallbacks: public TicketCallbacks
{
    public:
        virtual void onChangeTicket(TRACE_SIGNATURE, int grp_id)
        {
            LogTrace(TRACE_PARAMS) << __func__ << " started; grp_id: " << grp_id;
            TGrpAlarmHook::set(Alarm::SyncCustomAlarms, grp_id);
        }
};

void init_ticket_callbacks()
{
    CallbacksSingleton<TicketCallbacks>::Instance()->setCallbacks(new TicketCustomAlarmCallbacks);
}

class RFISCCustomAlarmCallbacks: public RFISCCallbacks
{
    public:
        virtual void afterRFISCChange(TRACE_SIGNATURE, int grp_id)
        {
            LogTrace(TRACE_PARAMS) << __func__ << " started; grp_id: " << grp_id;
            TGrpAlarmHook::set(Alarm::SyncCustomAlarms, grp_id);
        }
};

void init_rfisc_callbacks()
{
    CallbacksSingleton<RFISCCallbacks>::Instance()->setCallbacks(new RFISCCustomAlarmCallbacks);
}

class PaxFQTCallbacks: public PaxRemCallbacks
{
    public:
        virtual void afterPaxFQTChange(TRACE_SIGNATURE, int pax_id)
        {
            LogTrace(TRACE_PARAMS) << __func__ << " started, pax_id: " << pax_id;
            TPaxAlarmHook::set(Alarm::SyncCustomAlarms, pax_id);
        }
};

void init_fqt_callbacks()
{
    CallbacksSingleton<PaxRemCallbacks>::Instance()->setCallbacks(new PaxFQTCallbacks);
}


void TCustomAlarms::TSets::fromDB(const std::string &airline, int pax_id, set<int> &alarms)
{
    LogTrace(TRACE5) << __func__ << ": started; airline: '" << airline << "'; pax_id: " << pax_id;
    alarms.clear();

    auto &sets = items[airline];

    if(not sets) {
        sets = boost::in_place();
        TCachedQuery Qry("select * from custom_alarm_sets where airline = :airline",
                QParams() << QParam("airline", otString, airline));
        Qry.get().Execute();
        for(; not Qry.get().Eof; Qry.get().Next())
            sets.get()
                [Qry.get().FieldAsString("rfisc")]
                [Qry.get().FieldAsString("brand_airline")]
                [Qry.get().FieldAsString("brand_code")]
                [Qry.get().FieldAsString("fqt_airline")]
                [Qry.get().FieldAsString("fqt_tier_level")]
                =
                Qry.get().FieldAsInteger("alarm");
    }

    if(not sets.get().empty()) {

        boost::optional<RFISCsSet> paxRFISCs;
        boost::optional<set<CheckIn::TPaxFQTItem>> fqts;

        for(const auto &rfisc: sets.get())
            for(const auto &brand_airline: rfisc.second)
                for(const auto &brand_code: brand_airline.second)
                    for(const auto &fqt_airline: brand_code.second)
                        for(const auto &fqt_tier_level: fqt_airline.second) {
                            bool result = false;
                            // поиск rfsic
                            if(not rfisc.first.empty()) {
                                if(not paxRFISCs) {
                                    paxRFISCs = boost::in_place();
                                    TPaidRFISCListWithAuto paid;
                                    paid.fromDB(pax_id, false);
                                    paid.getUniqRFISCSs(pax_id, paxRFISCs.get());
                                }
                                for(const auto &_rfisc: paxRFISCs.get())
                                    if(_rfisc == rfisc.first) {
                                        result = true;
                                        break;
                                    }
                            }
                            // поиск по брендам
                            if(not result and not brand_code.first.empty()) {
                                brands.get(pax_id);
                                for(const auto &_brand: brands)
                                    if(
                                            _brand.code() == brand_code.first and
                                            _brand.oper_airline == brand_airline.first
                                      ) {
                                        result = true;
                                        break;
                                    }

                            }
                            // поиск по уровню участия
                            if(not result and not fqt_airline.first.empty()) {
                                if(not fqts) {
                                    fqts = boost::in_place();
                                    CheckIn::LoadPaxFQT(pax_id, fqts.get());
                                }
                                for(const auto &_fqt: fqts.get())
                                    if(
                                            _fqt.airline == fqt_airline.first and
                                            _fqt.tier_level == fqt_tier_level.first
                                      ) {
                                        result = true;
                                        break;
                                    }
                            }
                            if(result)
                                alarms.insert(fqt_tier_level.second);
                        }
    }
}

const TCustomAlarms &TCustomAlarms::getByGrpId(int grp_id)
{
    clear();
    TTripInfo info;
    if(info.getByGrpId(grp_id)) {
        TCachedQuery Qry("select pax_id from pax where grp_id = :grp_id",
                QParams() << QParam("grp_id", otInteger, grp_id));
        Qry.get().Execute();
        for(; not Qry.get().Eof; Qry.get().Next())
            getByPaxId(Qry.get().FieldAsInteger("pax_id"), false, info.airline);
    }
    return *this;
}

const TCustomAlarms &TCustomAlarms::getByPaxId(int pax_id, bool pr_clear, const string &vairline)
{
    if(pr_clear) clear();
    string airline;
    if(vairline.empty()) {
        TTripInfo info;
        info.getByPaxId(pax_id);
        airline = info.airline;
    } else
        airline = vairline;
    sets.fromDB(airline, pax_id, items[pax_id]);
    return *this;
}

void TCustomAlarms::toXML(xmlNodePtr paxNode, int pax_id)
{
    if(not paxNode) return;

    map<int, set<int>>::const_iterator pax = items.find(pax_id);
    if(pax != items.end()) {
        xmlNodePtr currNode = paxNode->children;
        if(not currNode) return;
        xmlNodePtr alarmsNode = GetNodeFast("alarms", currNode);
        if(not alarmsNode)
            alarmsNode=NewTextChild(paxNode, "alarms");
        for(const auto alarm: pax->second)
            NewTextChild(alarmsNode, "alarm", ElemIdToNameLong(etCustomAlarmType, alarm));
    }
}

void TCustomAlarms::fromDB(int point_id)
{
    clear();
    TCachedQuery Qry(
            "select "
            "   pa.pax_id, "
            "   pa.alarm_type "
            "from "
            "   pax_grp, "
            "   pax, "
            "   pax_custom_alarms pa "
            "where "
            "   pax_grp.point_dep = :point_id and "
            "   pax_grp.grp_id = pax.grp_id and "
            "   pax.pax_id = pa.pax_id ",
            QParams() << QParam("point_id", otInteger, point_id));
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next())
        items[Qry.get().FieldAsInteger("pax_id")].insert(Qry.get().FieldAsInteger("alarm_type"));
}

void TCustomAlarms::trace(TRACE_SIGNATURE) const
{
    LogTrace(TRACE_PARAMS) << "--- TCustomAlarms trace, size(): " << items.size() << "---";
    for(const auto &pax: items)
        for(const auto &alarm: pax.second)
            LogTrace(TRACE_PARAMS) << "pax_id: " << pax.first << "; alarm: " << alarm;
    LogTrace(TRACE_PARAMS) << "---------";
}

void TCustomAlarms::toDB() const
{
    trace(TRACE5);
    TCachedQuery delQry("delete from pax_custom_alarms where pax_id = :pax_id",
            QParams() << QParam("pax_id", otInteger));
    TCachedQuery Qry("insert into pax_custom_alarms(pax_id, alarm_type) values(:pax_id, :alarm_type)",
            QParams() << QParam("pax_id", otInteger) << QParam("alarm_type", otInteger));
    for(const auto &pax: items) {
        delQry.get().SetVariable("pax_id", pax.first);
        delQry.get().Execute();
        Qry.get().SetVariable("pax_id", pax.first);
        for(const auto &alarm_type: pax.second) {
            Qry.get().SetVariable("alarm_type", alarm_type);
            Qry.get().Execute();
        }
    }
}
