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

void get_custom_alarms(const string &airline, int pax_id, set<int> &alarms)
{
    LogTrace(TRACE5) << __func__ << ": started; airline: '" << airline << "'; pax_id: " << pax_id;
    alarms.clear();
    // Достаем RFISC-и
    // как платные, так и бесплатные
    RFISCsSet paxRFISCs;
    TPaidRFISCListWithAuto paid;
    paid.fromDB(pax_id, false);
    paid.getUniqRFISCSs(pax_id, paxRFISCs);
    if(paxRFISCs.empty()) paxRFISCs.insert("");

    // Достаем ремарки
    set<CheckIn::TPaxFQTItem> fqts;
    CheckIn::LoadPaxFQT(pax_id, fqts);
    // Если не найдено ни одной ремарки, добавляем пустую
    if(fqts.empty()) fqts.insert(CheckIn::TPaxFQTItem());

    // Достаем бренды
    TBrands brands;
    brands.get(pax_id);
    // Если не найдено ни одного бренда, добавляем пустой
    if(brands.empty()) brands.emplace_back();

    TCachedQuery Qry(
            "select alarm from custom_alarm_sets where "
            "   airline = :airline and "
            "   (rfisc is null or rfisc = :rfisc) and "
            "   (brand_airline is null or brand_airline = :brand_airline) and "
            "   (brand_code is null or brand_code = :brand_code) and "
            "   (fqt_airline is null or fqt_airline = :fqt_airline) and "
            "   (fqt_tier_level is null or fqt_tier_level = :fqt_tier_level) ",
            QParams()
            << QParam("airline", otString, airline)
            << QParam("rfisc", otString)
            << QParam("brand_airline", otString)
            << QParam("brand_code", otString)
            << QParam("fqt_airline", otString)
            << QParam("fqt_tier_level", otString));
    for(const auto &rfisc: paxRFISCs)
        for(const auto &fqt: fqts)
            for(const auto &brand: brands) {
                Qry.get().SetVariable("rfisc", rfisc);
                Qry.get().SetVariable("brand_airline", brand.oper_airline);
                Qry.get().SetVariable("brand_code", brand.code());
                Qry.get().SetVariable("fqt_airline", fqt.airline);
                Qry.get().SetVariable("fqt_tier_level", fqt.tier_level);

                LogTrace(TRACE5) << "--- vars list ---";
                for(int i = 0; i < Qry.get().VariablesCount(); i++)
                    LogTrace(TRACE5) << Qry.get().VariableName(i) << " = " << Qry.get().GetVariableAsString(i);

                Qry.get().Execute();
                for(; not Qry.get().Eof; Qry.get().Next())
                    alarms.insert(Qry.get().FieldAsInteger("alarm"));
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
    trace(TRACE5);
    get_custom_alarms(airline, pax_id, items[pax_id]);
    trace(TRACE5);
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
            LogTrace(TRACE5) << "custom alarms before insert: pax_id: " << pax.first << "; alarm_type: " << alarm_type;
            Qry.get().SetVariable("alarm_type", alarm_type);
            Qry.get().Execute();
        }
    }
}
