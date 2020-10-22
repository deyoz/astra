#include "custom_alarms.h"
#include "dcs_services.h"
#include "rfisc.h"
#include "brands.h"
#include "checkin.h"
#include "alarms.h"
#include "base_callbacks.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;

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

size_t TCustomAlarms::TSets::TRow::cost() const
{
    return
        not rfisc.empty() +
        not rfisc_tlg.empty() +
        not brand_code.empty() +
        not fqt_tier_level.empty();
}

bool TCustomAlarms::TSets::get(const std::string &airline)
{
    auto &sets = items[airline];
    if(not sets) {
        sets = boost::in_place();
        TCachedQuery Qry("select * from custom_alarm_sets where airline = :airline",
                QParams() << QParam("airline", otString, airline));
        Qry.get().Execute();
        for(; not Qry.get().Eof; Qry.get().Next())
            sets->push_back(TRow(
                Qry.get().FieldAsString("rfisc"),
                Qry.get().FieldAsString("rfisc_tlg"),
                Qry.get().FieldAsString("brand_airline"),
                Qry.get().FieldAsString("brand_code"),
                Qry.get().FieldAsString("fqt_airline"),
                Qry.get().FieldAsString("fqt_tier_level"),
                Qry.get().FieldAsInteger("alarm")));
    }
    return not sets->empty();
}

string TCustomAlarms::TSets::TRow::str() const
{
    ostringstream result;
    result
        << setw(16) << rfisc
        << setw(16) << rfisc_tlg
        << setw(4) << brand_airline
        << setw(11) << brand_code
        << setw(4) << fqt_airline
        << setw(30) << fqt_tier_level
        << setw(10) << alarm
        << " COST: " << cost();
    return result.str();
}

void TCustomAlarms::TSets::fromDB(const std::string &airline, int pax_id, set<int> &alarms)
{
    LogTrace(TRACE5) << __func__ << ": started; airline: '" << airline << "'; pax_id: " << pax_id;

    alarms.clear();

    if(get(airline)) {
        auto &sets = items[airline];

        boost::optional<RFISCsSet> paxRFISCs;
        boost::optional<set<CheckIn::TPaxFQTItem>> fqts;
        boost::optional<vector<CheckIn::TPaxASVCItem>> asvcs;

        map<size_t, list<TRow>> selected;

        for(const auto &row: sets.get()) {
            bool result = true;
            // поиск rfsic
            if(not row.rfisc.empty()) {
                result = false;
                if(not paxRFISCs) {
                    paxRFISCs = boost::in_place();
                    TPaidRFISCListWithAuto paid;
                    paid.fromDB(pax_id, false);
                    paid.getUniqRFISCSs(pax_id, paxRFISCs.get());
                }
                for(const auto &rfisc: paxRFISCs.get())
                    if(rfisc == row.rfisc) {
                        result = true;
                        break;
                    }
            }
            // поиск rfsic_tlg
            if(result and not row.rfisc_tlg.empty()) {
                result = false;
                if(not asvcs) {
                    asvcs = boost::in_place();
                    LoadPaxASVC(pax_id, asvcs.get());
                }
                for(const auto &asvc: asvcs.get())
                    if(asvc.RFISC == row.rfisc_tlg) {
                        result = true;
                        break;
                    }
            }
            // поиск по брендам
            if(result and not row.brand_code.empty()) {
                result = false;
                brands.get(pax_id);
                for(const auto &brand: brands)
                    if(
                            brand.code() == row.brand_code and
                            brand.oper_airline == row.brand_airline
                      ) {
                        result = true;
                        break;
                    }

            }
            // поиск по уровню участия
            if(result and not row.fqt_airline.empty()) {
                result = false;
                if(not fqts) {
                    fqts = boost::in_place();
                    CheckIn::LoadPaxFQT(pax_id, fqts.get());
                }
                for(const auto &fqt: fqts.get())
                    if(
                            fqt.airline == row.fqt_airline and
                            fqt.tier_level == row.fqt_tier_level
                      ) {
                        result = true;
                        break;
                    }
            }
            if(result)
                selected[row.cost()].push_back(row);
        }
        if(not selected.empty())
            for(const auto &row: selected.rbegin()->second)
                alarms.insert(row.alarm);
    }
}

const TCustomAlarms &TCustomAlarms::getByGrpId(int grp_id)
{
    clear();
    TTripInfo info;
    if(info.getByGrpId(grp_id) and sets.get(info.airline)) {
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

void TCustomAlarms::fromDB(bool all, int id)
{
    clear();

    string SQLText;
    SQLText =
        "select "
        "   pa.pax_id, "
        "   pa.alarm_type "
        "from ";
    if(all) {
        SQLText +=
            "   pax_grp, "
            "   pax, ";
    }
    SQLText +=
        "   pax_custom_alarms pa "
        "where ";
    if(all)
        SQLText +=
            "   pax_grp.point_dep = :id and "
            "   pax_grp.grp_id = pax.grp_id and "
            "   pax.pax_id = pa.pax_id ";
    else
        SQLText += "   pa.pax_id = :id ";

    TCachedQuery Qry(SQLText, QParams() << QParam("id", otInteger, id));
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
