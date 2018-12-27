#include "pax_list.h"
#include "salons.h"
#include <boost/algorithm/string.hpp>

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

// ���� �������਩

using namespace REPORTS;
using namespace std;
using namespace ASTRA;

TPaxPtr TPaxList::getPaxPtr()
{
    return TPaxPtr(new TPax(*this));
}

void TUnboundCBBGList::trace(TRACE_SIGNATURE)
{
    LogTrace(TRACE_PARAMS) << "---TUnboundCBBGList::trace---";
    for(const auto &item: *this)
        item->trace(TRACE_PARAMS);
    LogTrace(TRACE_PARAMS) << "-----------------------------";
}

void TUnboundCBBGList::add_cbbg(TPaxPtr _cbbg_pax)
{
    if(not _cbbg_pax->simple.isCBBG()) return;
    for(auto &pax: _cbbg_pax->pax_list) {
        for(auto &paxCBBG: pax->cbbg_list) {
            if(paxCBBG.seat_id == _cbbg_pax->simple.paxId()) {
                paxCBBG.pax_info = _cbbg_pax;
                return;
            }
        }
    }
    push_back(_cbbg_pax);
}

void TUnboundCBBGList::bind_cbbg(TPaxPtr _pax)
{
    auto cbbg = this->begin();
    for(; cbbg != this->end(); cbbg++) {
        auto paxCBBG = _pax->cbbg_list.begin();
        for(; paxCBBG != _pax->cbbg_list.end(); paxCBBG++) {
            if(paxCBBG->seat_id == (*cbbg)->simple.paxId()) {
                paxCBBG->pax_info = *cbbg;
                break;
            }
        }
        if(paxCBBG != _pax->cbbg_list.end()) break;
    }
    if(cbbg != this->end()) this->erase(cbbg);
}

void TPaxList::fromDB(TQuery &Qry)
{
    TPaxPtr pax = getPaxPtr();
    pax->fromDB(Qry);

    if(mkt_flt and not mkt_flt.get().empty()) {
        TMktFlight db_mkt_flt;
        db_mkt_flt.getByPaxId(pax->simple.id);
        if(not(db_mkt_flt == mkt_flt.get()))
            return;
    }

    if(pax->simple.isCBBG()) {
        unbound_cbbg_list.add_cbbg(pax);
    } else {
        push_back(pax);
        unbound_cbbg_list.bind_cbbg(back());
    }
}

void TPax::trace(TRACE_SIGNATURE)
{
    LogTrace(TRACE_PARAMS) << "reg_no: " << simple.reg_no;
    LogTrace(TRACE_PARAMS) << "name: " << simple.name;
    LogTrace(TRACE_PARAMS) << "surname: " << simple.surname;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info) {
            LogTrace(TRACE_PARAMS) << "  reg_no: " << cbbg.pax_info->simple.reg_no;
            LogTrace(TRACE_PARAMS) << "  name: " << cbbg.pax_info->simple.name;
            LogTrace(TRACE_PARAMS) << "  surname: " << cbbg.pax_info->simple.surname;
        }
    }
}

// �����, �뭥�� � �⨫���?
string FieldAsString(TQuery &Qry, const string &name)
{
    string result;
    int col_idx = Qry.GetFieldIndex(name);
    if(col_idx >= 0)
        result = Qry.FieldAsString(col_idx);
    return result;
}

int FieldAsInteger(TQuery &Qry, const string &name)
{
    int result = NoExists;
    int col_idx = Qry.GetFieldIndex(name);
    if(col_idx >= 0)
        result = Qry.FieldAsInteger(col_idx);
    return result;
}

void TPax::fromDB(TQuery &Qry)
{
    simple.fromDB(Qry);
    if(not simple.isCBBG())
        cbbg_list.fromDB(simple.id);

    if(not pax_list.complayers) {
        pax_list.complayers = boost::in_place();
        TAdvTripInfo flt_info;
        flt_info.getByPointId(pax_list.point_id);
        if(not SALONS2::isFreeSeating(flt_info.point_id) and not SALONS2::isEmptySalons(flt_info.point_id))
            getSalonLayers( flt_info.point_id, flt_info.point_num, flt_info.first_point, flt_info.pr_tranzit, pax_list.complayers.get(), false );
    }

    seat_list.add_seats(simple.paxId(), pax_list.complayers.get());
    simple.seat_no = (simple.is_jmp ? "JMP" : seat_list.get_seat_one(pax_list.lang != AstraLocale::LANG_RU));

    if(not pax_list.rem_grp and pax_list.rem_event_type) {
        pax_list.rem_grp = boost::in_place();
        pax_list.rem_grp.get().Load(pax_list.rem_event_type.get(), pax_list.point_id);
    }

    user_descr = FieldAsString(Qry, "user_descr");
    baggage.fromDB(Qry);
    if(simple.bag_pool_num != NoExists)
        GetTagsByPool(simple.grp_id, simple.bag_pool_num, _tags, true);
    GetRemarks(simple.id, pax_list.lang, _rems);
}

void TPaxList::trace(TRACE_SIGNATURE)
{
    LogTrace(TRACE_PARAMS) << "---TPaxList::trace---";
    for(const auto &pax: *this)
        pax->trace(TRACE_PARAMS);
    for(const auto &unbound: unbound_cbbg_list)
        LogTrace(TRACE_PARAMS) << unbound->simple.reg_no << " not applied";

}

void TCrsSeatsBlockingList::fromDB(int _pax_id)
{
    clear();
    pax_id = _pax_id;
    TCachedQuery Qry("select * from crs_seats_blocking where pax_id = :pax_id",
            QParams() << QParam("pax_id", otInteger, pax_id));
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        emplace_back();
        back().fromDB(Qry.get());
    }
}

void TCrsSeatsBlockingItem::fromDB(TQuery &Qry)
{
    clear();
    if(not Qry.Eof) {
        seat_id = Qry.FieldAsInteger("seat_id");
        surname = Qry.FieldAsString("surname");
        name = Qry.FieldAsString("name");
        pax_id = Qry.FieldAsInteger("pax_id");
    }
}

void TPaxList::clear()
{
    point_id = ASTRA::NoExists;
    lang = AstraLocale::LANG_RU;
    mkt_flt = boost::none;
    rem_event_type = boost::none;
    list<TPaxPtr>::clear();
    unbound_cbbg_list.clear();
    complayers = boost::none;
    rem_grp = boost::none;
}

TPaxList::TPaxList(int _point_id)
{
    clear();
    point_id = _point_id;
}

string TPax::seat_no() const
{
    ostringstream result;
    result << simple.seat_no;
    if(seats() > 1) result << "+" << seats() - 1;
    return result.str();
}

int TPax::seats() const
{
    int result = simple.seats;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result += cbbg.pax_info->simple.seats;
    }
    return result;
}

string TPax::tkn_str() const
{
    string result;
    if(not simple.tkn.empty())
        result += simple.tkn.no_str();
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info) {
            if(not cbbg.pax_info->simple.tkn.empty()) {
                if(not result.empty())
                    result += " ";
                result += cbbg.pax_info->simple.tkn.no_str();
            }
        }
    }
    return result;
}

void TBaggage::fromDB(TQuery &Qry)
{
    rk_amount = FieldAsInteger(Qry, "rk_amount");
    rk_weight = Qry.FieldAsInteger("rk_weight");
    amount = Qry.FieldAsInteger("bag_amount");
    weight = Qry.FieldAsInteger("bag_weight");
    excess_wt = Qry.FieldAsInteger("excess_wt");
    excess_pc = Qry.FieldAsInteger("excess_pc");
}

int TPax::bag_amount() const
{
    int result = baggage.amount;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result += cbbg.pax_info->baggage.amount;
    }
    return result;
}

int TPax::bag_weight() const
{
    int result = baggage.weight;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result += cbbg.pax_info->baggage.weight;
    }
    return result;
}

int TPax::rk_amount() const
{
    int result = baggage.rk_amount;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result += cbbg.pax_info->baggage.rk_amount;
    }
    return result;
}

int TPax::rk_weight() const
{
    int result = baggage.rk_weight;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result += cbbg.pax_info->baggage.rk_weight;
    }
    return result;
}

TBagKilos TPax::excess_wt() const
{
    TBagKilos result = baggage.excess_wt;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result += cbbg.pax_info->baggage.excess_wt;
    }
    return result;
}

TBagPieces TPax::excess_pc() const
{
    TBagPieces result = baggage.excess_pc;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result += cbbg.pax_info->baggage.excess_pc;
    }
    return result;
}

string TPax::get_tags() const
{
    multiset<TBagTagNumber> result = _tags;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result.insert(
                    cbbg.pax_info->_tags.begin(),
                    cbbg.pax_info->_tags.end());
    }
    return GetTagRangesStrShort(result);
}

string TPax::rems() const
{
    multiset<CheckIn::TPaxRemItem> result = _rems;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result.insert(
                    cbbg.pax_info->_rems.begin(),
                    cbbg.pax_info->_rems.end());
    }
    string result_str = GetRemarkStr(pax_list.rem_grp.get(), result, "\n");
    // ᪮����⨬ � ���������騥�� ६�ન, � ��࠭����� ���஢��
    vector<string> items;
    boost::split(items, result_str, boost::is_any_of("\n"));
    set<string> unique_items;
    string unique_items_str;
    for(const auto &i: items) {
        if(unique_items_str.find(i) == string::npos) {
            if(not unique_items_str.empty())
                unique_items_str += " ";
            unique_items_str += i;
        }
    }
    return unique_items_str;
}

