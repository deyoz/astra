#include "docs_pax_list.h"
#include <boost/algorithm/string.hpp>

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace REPORTS;
using namespace std;
using namespace ASTRA;

bool REPORTS::pax_compare(TPaxPtr pax1, TPaxPtr pax2)
{
    const TPMPax &pm_pax1 = dynamic_cast<const TPMPax &>(*pax1);
    const TPMPax &pm_pax2 = dynamic_cast<const TPMPax &>(*pax2);

    if(pm_pax1.get_pax_list().rpt_params.pr_trfer) {
        if(pm_pax1.pr_trfer != pm_pax2.pr_trfer)
            return pm_pax1.pr_trfer < pm_pax2.pr_trfer;
        if(pm_pax1.trfer_airp_arv != pm_pax2.trfer_airp_arv)
            return pm_pax1.trfer_airp_arv < pm_pax2.trfer_airp_arv;
    }
    if(pm_pax1.priority != pm_pax2.priority)
        return pm_pax1.priority < pm_pax2.priority;
    if(pm_pax1.cls != pm_pax2.cls)
        return pm_pax1.cls < pm_pax2.cls;

    switch(pm_pax1.get_pax_list().rpt_params.sort) {
        case stServiceCode:
        case stRegNo:
            return pm_pax1.simple.reg_no < pm_pax2.simple.reg_no;
        case stSurname:
            if(pm_pax1.simple.surname != pm_pax2.simple.surname)
                return pm_pax1.simple.surname < pm_pax2.simple.surname;
            if(pm_pax1.simple.name != pm_pax2.simple.name)
                return pm_pax1.simple.name < pm_pax2.simple.name;
            break;
        case stSeatNo:
            if(pm_pax1._seat_no != pm_pax2._seat_no)
                return pm_pax1._seat_no < pm_pax2._seat_no;
            break;
    }
    return pm_pax1.simple.reg_no < pm_pax2.simple.reg_no;
}

int REPORTS::nosir_cbbg(int argc, char** argv)
{
    TRptParams rpt_params(AstraLocale::LANG_RU);
    rpt_params.point_id = 4683700;
    TPMPaxList pax_list(rpt_params);

    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select * from pax, pax_grp where "
        "   pax_grp.point_dep = :point_id and "
        "   pax_grp.grp_id = pax.grp_id ";
    Qry.CreateVariable("point_id", otInteger, pax_list.point_id);
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next())
        pax_list.fromDB(Qry);
    pax_list.sort(pax_compare);
    pax_list.trace(TRACE5);

    return 1;
}

TPaxPtr TPMPaxList::getPaxPtr()
{
    return TPaxPtr(new TPMPax(*this));
}

TPMPax &TPMPax::pm_pax(TPaxPtr val) const
{
    return dynamic_cast<TPMPax&>(*val);
}

TPMPaxList &TPMPax::get_pax_list() const
{
    return dynamic_cast<TPMPaxList&>(pax_list);
}

void TPMPax::fromDB(TQuery &Qry)
{
    TPax::fromDB(Qry);
    simple.seat_no = (simple.is_jmp ? "JMP" : seat_list.get_seat_one(get_pax_list().rpt_params.GetLang() != AstraLocale::LANG_RU));
    _seat_no = " " + simple.seat_no;
    target = Qry.FieldAsString("target");
    last_target = get_last_target(Qry, get_pax_list().rpt_params);
    status = Qry.FieldAsString("status");
    if(get_pax_list().rpt_params.pr_et) {
        if(not simple.tkn.empty())
            _rems.emplace("", simple.tkn.no + IntToString(simple.tkn.coupon));
    } else
        GetRemarks(simple.id, get_pax_list().rpt_params.GetLang(), _rems);
    point_id = Qry.FieldAsInteger("trip_id");
    class_grp = Qry.FieldAsInteger("class_grp");
    priority = ((const TClsGrpRow&)base_tables.get("cls_grp").get_row( "id", class_grp, true)).priority;
    cls = ((const TClsGrpRow&)base_tables.get("cls_grp").get_row( "id", class_grp, true)).cl;
    if(get_pax_list().rpt_params.pr_trfer) {
        pr_trfer = Qry.FieldAsInteger("pr_trfer");
        trfer_airline = Qry.FieldAsString("trfer_airline");
        trfer_flt_no = Qry.FieldAsInteger("trfer_flt_no");
        trfer_suffix = Qry.FieldAsString("trfer_suffix");
        trfer_airp_arv = Qry.FieldAsString("trfer_airp_arv");
        trfer_scd = Qry.FieldAsDateTime("trfer_scd");
    }
    _rk_weight = Qry.FieldAsInteger("rk_weight");
    _bag_amount = Qry.FieldAsInteger("bag_amount");
    _bag_weight = Qry.FieldAsInteger("bag_weight");
    excess_wt = Qry.FieldAsInteger("excess_wt");
    excess_pc = Qry.FieldAsInteger("excess_pc");
    if(simple.bag_pool_num != NoExists)
        GetTagsByPool(simple.grp_id, simple.bag_pool_num, _tags, true);
}

void TPMPax::trace(TRACE_SIGNATURE)
{
    TPax::trace(TRACE_PARAMS);
    LogTrace(TRACE_PARAMS) << "_seat_no: '" << _seat_no << "'";
}

string REPORTS::get_last_target(TQuery &Qry, TRptParams &rpt_params)
{
    string result;
    if(rpt_params.pr_trfer) {
        string airline = Qry.FieldAsString("trfer_airline");
        if(!airline.empty()) {
            ostringstream buf;
            buf
                << rpt_params.ElemIdToReportElem(etAirp, Qry.FieldAsString("trfer_airp_arv"), efmtNameLong).substr(0, 50)
                << "("
                << rpt_params.ElemIdToReportElem(etAirline, airline, efmtCodeNative)
                << setw(3) << setfill('0') << Qry.FieldAsInteger("trfer_flt_no")
                << rpt_params.ElemIdToReportElem(etSuffix, Qry.FieldAsString("trfer_suffix"), efmtCodeNative)
                << ")/" << DateTimeToStr(Qry.FieldAsDateTime("trfer_scd"), "dd");
            result = buf.str();
        }
    }
    return result;
}

int TPMPax::seats() const
{
    int result = simple.seats;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result += pm_pax(cbbg.pax_info).simple.seats;
    }
    return result;
}

string TPMPax::seat_no() const
{
    ostringstream result;
    result << simple.seat_no;
    if(seats() > 1) result << "+" << seats() - 1;
    return result.str();
}

int TPMPax::bag_amount() const
{
    int result = _bag_amount;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result += pm_pax(cbbg.pax_info)._bag_amount;
    }
    return result;
}

int TPMPax::bag_weight() const
{
    int result = _bag_weight;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result += pm_pax(cbbg.pax_info)._bag_weight;
    }
    return result;
}

int TPMPax::rk_weight() const
{
    int result = _rk_weight;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result += pm_pax(cbbg.pax_info)._rk_weight;
    }
    return result;
}

string TPMPax::get_tags() const
{
    multiset<TBagTagNumber> result = _tags;
    for(const auto &cbbg: cbbg_list) {
        if(cbbg.pax_info)
            result.insert(
                    pm_pax(cbbg.pax_info)._tags.begin(),
                    pm_pax(cbbg.pax_info)._tags.end());
    }
    return GetTagRangesStrShort(result);
}

string TPMPax::rems() const
{
    if(get_pax_list().rpt_params.pr_et) {
        string result;
        if(not simple.tkn.empty())
            result += simple.tkn.no_str();
        for(const auto &cbbg: cbbg_list) {
            if(cbbg.pax_info) {
                if(not pm_pax(cbbg.pax_info).simple.tkn.empty()) {
                    if(not result.empty())
                        result += " ";
                    result += pm_pax(cbbg.pax_info).simple.tkn.no_str();
                }
            }
        }
        return result;
    } else {
        multiset<CheckIn::TPaxRemItem> result = _rems;
        for(const auto &cbbg: cbbg_list) {
            if(cbbg.pax_info)
                result.insert(
                        pm_pax(cbbg.pax_info)._rems.begin(),
                        pm_pax(cbbg.pax_info)._rems.end());
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
}
