#include "docs_pax_list.h"
#include "salons.h"
#include <boost/algorithm/string.hpp>

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

// просто комментарий

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

void TPaxList::fromDB()
{
    QParams QryParams;
    QryParams << QParam("point_id", otInteger, point_id);
    string SQLText =
        "select "
        "   pax.* ";
    if(options.flags.isFlag(oeBagAmount))
        SQLText += "   ,ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bag_amount ";
    if(options.flags.isFlag(oeBagWeight))
        SQLText += "   ,ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) bag_weight ";
    if(options.flags.isFlag(oeRkAmount))
        SQLText += "   ,ckin.get_rkAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rk_amount ";
    if(options.flags.isFlag(oeRkWeight))
        SQLText += "   ,ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) rk_weight ";
    if(options.flags.isFlag(oeExcess))
        SQLText +=
            "   ,NVL(ckin.get_excess_wt(pax.grp_id, pax.pax_id, pax_grp.excess_wt, pax_grp.bag_refuse),0) AS excess_wt "
            "   ,NVL(ckin.get_excess_pc(pax.grp_id, pax.pax_id),0) AS excess_pc ";
    SQLText +=
        "from pax_grp, pax where "
        "   pax_grp.point_dep = :point_id and "
        "   pax_grp.grp_id = pax.grp_id and "
        "   pax_grp.status not in ('E') ";
    if(options.pr_brd) {
        switch(options.pr_brd.get().val) {
            case TBrdVal::bvNULL:
                SQLText += "and pax.pr_brd is null ";
                break;
            case TBrdVal::bvNOT_NULL:
                SQLText += "and pax.pr_brd is not null ";
                break;
            case TBrdVal::bvTRUE:
            case TBrdVal::bvFALSE:
                SQLText += "and pax.pr_brd = :pr_brd ";
                QryParams << QParam("pr_brd", otInteger, options.pr_brd.get().val != TBrdVal::bvFALSE);
                break;
        }
    }
    TCachedQuery Qry(SQLText, QryParams);
    Qry.get().Execute();
    fromDB(Qry.get());
}

void TPaxList::fromDB(TQuery &Qry)
{
    for(; !Qry.Eof; Qry.Next()) {
        TPaxPtr pax = getPaxPtr();
        pax->fromDB(Qry);

        if(options.mkt_flt and not options.mkt_flt.get().empty()) {
            TMktFlight db_mkt_flt;
            db_mkt_flt.getByPaxId(pax->simple.id);
            if(not(db_mkt_flt == options.mkt_flt.get()))
                return;
        }

        if(pax->simple.isCBBG()) {
            unbound_cbbg_list.add_cbbg(pax);
        } else {
            push_back(pax);
            unbound_cbbg_list.bind_cbbg(back());
        }
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

// Может, вынести в утилиты?
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

    if(pax_list.options.flags.isFlag(oeSeatNo) and not pax_list.complayers) {
        pax_list.complayers = boost::in_place();
        TAdvTripInfo flt_info;
        flt_info.getByPointId(pax_list.point_id);
        if(not SALONS2::isFreeSeating(flt_info.point_id) and not SALONS2::isEmptySalons(flt_info.point_id))
            getSalonLayers( flt_info.point_id, flt_info.point_num, flt_info.first_point, flt_info.pr_tranzit, pax_list.complayers.get(), false );
    }

    if(pax_list.complayers) {
        seat_list.add_seats(simple.paxId(), pax_list.complayers.get());
        simple.seat_no = (simple.is_jmp ? "JMP" : seat_list.get_seat_one(
                    pax_list.complayers.get().pr_craft_lat or
                    pax_list.options.lang != AstraLocale::LANG_RU));
        ostringstream s;
        s << setw(4) << right << simple.seat_no;
        _seat_no = s.str();
    }

    if(not pax_list.rem_grp and pax_list.options.rem_event_type) {
        pax_list.rem_grp = boost::in_place();
        pax_list.rem_grp.get().Load(pax_list.options.rem_event_type.get(), pax_list.point_id);
    }

    user_descr = FieldAsString(Qry, "user_descr");
    baggage.fromDB(Qry);
    if(pax_list.options.flags.isFlag(oeTags) and simple.bag_pool_num != NoExists)
        GetTagsByPool(simple.grp_id, simple.bag_pool_num, _tags, true);
    if(pax_list.rem_grp)
        GetRemarks(simple.id, pax_list.options.lang, _rems);
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
    options.clear();
    point_id = ASTRA::NoExists;
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
    if(pax_list.options.flags.isFlag(oeSeatNo)) {
        result << simple.seat_no;
        if(seats() > 1) result << "+" << seats() - 1;
    }
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
    rk_weight = FieldAsInteger(Qry, "rk_weight");
    amount = FieldAsInteger(Qry, "bag_amount");
    weight = FieldAsInteger(Qry, "bag_weight");
    excess_wt = FieldAsInteger(Qry, "excess_wt");
    excess_pc = FieldAsInteger(Qry, "excess_pc");
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

const string &TPax::cl() const
{
    return simple.cabin.cl.empty() ? grp().cl : simple.cabin.cl;
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
    // сконвертим в неповторяющиеся ремарки, с сохранением сортировки
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

string TPax::full_name_view() const
{
    return transliter(simple.full_name(), 1, pax_list.options.lang != AstraLocale::LANG_RU);
}

bool REPORTS::pax_cmp(TPaxPtr pax1, TPaxPtr pax2)
{
    switch(pax1->pax_list.options.sort) {
        case stServiceCode:
        case stRegNo:
            return pax1->simple.reg_no < pax2->simple.reg_no;
        case stSurname:
            return pax1->full_name_view() < pax2->full_name_view();
        case stSeatNo:
            return pax1->_seat_no < pax2->_seat_no;
    }
    return pax1->simple.reg_no < pax2->simple.reg_no;
}

const CheckIn::TSimplePaxGrpItem &TPax::grp() const
{
    auto &result = pax_list.grps[simple.grp_id];
    if(not result) {
        result = boost::in_place();
        result->getByGrpId(simple.grp_id);
    }
    return result.get();
}
