#include "docs_ptm.h"
#include <boost/algorithm/string.hpp>
#include "astra_utils.h"
#include "salons.h"
#include "stat/stat_utils.h"
#include "docs_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace REPORTS;
using namespace std;
using namespace ASTRA;
using namespace AstraLocale;
using namespace EXCEPTIONS;

bool REPORTS::pax_compare(TPaxPtr pax1, TPaxPtr pax2)
{
    const TPMPax &pm_pax1 = dynamic_cast<const TPMPax &>(*pax1);
    const TPMPax &pm_pax2 = dynamic_cast<const TPMPax &>(*pax2);

    if(pm_pax1.point_num != pm_pax2.point_num)
        return pm_pax1.point_num < pm_pax2.point_num;

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
            return pax1->full_name_view() < pax2->full_name_view();
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

TPMPaxList &TPMPax::get_pax_list() const
{
    return dynamic_cast<TPMPaxList&>(pax_list);
}

void TPMPax::fromDB(TQuery &Qry)
{
    TPax::fromDB(Qry);
    target = Qry.FieldAsString("target");
    last_target = get_last_target(Qry, get_pax_list().rpt_params);
    point_num = Qry.FieldAsInteger("point_num");
    status = Qry.FieldAsString("status");
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
}

void TPMPax::trace(TRACE_SIGNATURE)
{
    TPax::trace(TRACE_PARAMS);
    LogTrace(TRACE_PARAMS) << "_seat_no: '" << _seat_no << "'";
}

string TPMPax::rems() const
{
    if(get_pax_list().rpt_params.pr_et) {
        return tkn_str();
    } else {
        return TPax::rems();
    }
}

struct TPMTotalsKey {
    int point_id;
    int pr_trfer;
    string target;
    string status;
    string cls;
    string cls_name;
    int lvl;
    void dump() const;
    TPMTotalsKey():
        point_id(NoExists),
        pr_trfer(NoExists),
        lvl(NoExists)
    {
    };
};

void TPMTotalsKey::dump() const
{
    ProgTrace(TRACE5, "---TPMTotalsKey::dump()---");
    ProgTrace(TRACE5, "point_id: %d", point_id);
    ProgTrace(TRACE5, "pr_trfer: %d", pr_trfer);
    ProgTrace(TRACE5, "target: %s", target.c_str());
    ProgTrace(TRACE5, "status: %s", status.c_str());
    ProgTrace(TRACE5, "cls: %s", cls.c_str());
    ProgTrace(TRACE5, "cls_name: %s", cls_name.c_str());
    ProgTrace(TRACE5, "lvl: %d", lvl);
    ProgTrace(TRACE5, "--------------------------");
}

struct TPMTotalsCmp {
    bool operator() (const TPMTotalsKey &l, const TPMTotalsKey &r) const
    {
        if(l.pr_trfer == NoExists)
            if(l.point_id == r.point_id)
                if(l.lvl == r.lvl)
                    if(l.status == r.status)
                        return l.cls < r.cls;
                    else
                        return l.status < r.status;
                else
                    return l.lvl < r.lvl;
            else
                return l.point_id < r.point_id;
        else
            if(l.point_id == r.point_id)
                if(l.target == r.target)
                    if(l.pr_trfer == r.pr_trfer)
                        if(l.lvl == r.lvl)
                            if(l.status == r.status)
                                return l.cls < r.cls;
                            else
                                return l.status < r.status;
                        else
                            return l.lvl < r.lvl;
                    else
                        return l.pr_trfer < r.pr_trfer;
                else
                    return l.target < r.target;
            else
                return l.point_id < r.point_id;
    }
};

struct TPMTotalsRow {
    int seats, adl_m, adl_f, chd, inf, rk_weight, bag_amount, bag_weight;
    TBagKilos excess_wt;
    TBagPieces excess_pc;
    int xcr, dhc, mos, jmp;
    TPMTotalsRow():
        seats(0),
        adl_m(0),
        adl_f(0),
        chd(0),
        inf(0),
        rk_weight(0),
        bag_amount(0),
        bag_weight(0),
        excess_wt(0),
        excess_pc(0),
        xcr(0),
        dhc(0),
        mos(0),
        jmp(0)
    {};
};

typedef map<TPMTotalsKey, TPMTotalsRow, TPMTotalsCmp> TPMTotals;

void PMTotalsToXML(const TPMTotals &PMTotals, map<string, int> &fr_target_ref, xmlNodePtr dataSetsNode, TRptParams &rpt_params)
{
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, rpt_params.pr_trfer ? "v_pm_trfer_total" : "v_pm_total");

    for(TPMTotals::const_iterator im = PMTotals.begin(); im != PMTotals.end(); im++) {
        const TPMTotalsKey &key = im->first;
        const TPMTotalsRow &row = im->second;

        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        NewTextChild(rowNode, "point_id", key.point_id);
        if(rpt_params.pr_trfer) {
            NewTextChild(rowNode, "target", key.target);
            NewTextChild(rowNode, "fr_target_ref", fr_target_ref[key.target]);
            NewTextChild(rowNode, "pr_trfer", key.pr_trfer);
        }
        NewTextChild(rowNode, "status", key.status);
        NewTextChild(rowNode, "class_name", key.cls_name);
        NewTextChild(rowNode, "lvl", key.lvl);
        NewTextChild(rowNode, "seats", row.seats);
        NewTextChild(rowNode, "adl", row.adl_m+row.adl_f);
        NewTextChild(rowNode, "adl_f", row.adl_f);
        NewTextChild(rowNode, "chd", row.chd);
        NewTextChild(rowNode, "inf", row.inf);
        NewTextChild(rowNode, "rk_weight", row.rk_weight);
        NewTextChild(rowNode, "bag_amount", row.bag_amount);
        NewTextChild(rowNode, "bag_weight", row.bag_weight);
        NewTextChild(rowNode, "excess", TComplexBagExcess(row.excess_wt, row.excess_pc).
                view(OutputLang(rpt_params.GetLang()), true));
        NewTextChild(rowNode, "xcr", row.xcr);
        NewTextChild(rowNode, "dhc", row.dhc);
        NewTextChild(rowNode, "mos", row.mos);
        NewTextChild(rowNode, "jmp", row.jmp);
    }
}

void PaxListToXML(const REPORTS::TPMPaxList &pax_list, xmlNodePtr dataSetsNode, TRptParams &rpt_params)
{
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_pm_trfer");
    map<string, int> fr_target_ref;
    int fr_target_ref_idx = 0;
    TPMTotals PMTotals;
    for(const auto &pax_item: pax_list) {
        const REPORTS::TPMPax &pax = dynamic_cast<const REPORTS::TPMPax&>(*pax_item);
        TPMTotalsKey key;

        key.point_id = rpt_params.point_id;
        key.target = pax.target;
        key.status = pax.status;
        key.cls = rpt_params.ElemIdToReportElem(etClsGrp, pax.class_grp, efmtCodeNative);
        key.cls_name = rpt_params.ElemIdToReportElem(etClsGrp, pax.class_grp, efmtNameLong);
        key.lvl = pax.priority;
        key.pr_trfer = pax.pr_trfer;

        TPMTotalsRow &row = PMTotals[key];
        row.seats += pax_item->seats();
        switch(pax.simple.getTrickyGender())
        {
          case TTrickyGender::Male:
            row.adl_m++;
            break;
          case TTrickyGender::Female:
            row.adl_f++;
            break;
          case TTrickyGender::Child:
            row.chd++;
            break;
          case TTrickyGender::Infant:
            row.inf++;
            break;
          default:
            throw Exception("DecodePerson failed");
        }

        row.rk_weight += pax.rk_weight();
        row.bag_amount += pax.bag_amount();
        row.bag_weight += pax.bag_weight();
        row.excess_wt += pax.excess_wt();
        row.excess_pc += pax.excess_pc();

        switch(pax.simple.crew_type) {
            case TCrewType::ExtraCrew:
                row.xcr++;
                break;
            case TCrewType::DeadHeadCrew:
                row.dhc++;
                break;
            case TCrewType::MiscOperStaff:
                row.mos++;
                break;
            default:
                break;
        }
        if(pax.simple.is_jmp) row.jmp += pax.simple.seats;

        if(fr_target_ref.find(key.target) == fr_target_ref.end())
            fr_target_ref[key.target] = fr_target_ref_idx++;

        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");
        NewTextChild(rowNode, "reg_no", pax.simple.reg_no);
        NewTextChild(rowNode, "full_name", transliter(pax.simple.full_name(), 1, rpt_params.GetLang() != AstraLocale::LANG_RU));
        NewTextChild(rowNode, "last_target", pax.last_target);
        NewTextChild(rowNode, "pr_trfer", pax.pr_trfer);
        NewTextChild(rowNode, "airp_arv", pax.target);
        NewTextChild(rowNode, "fr_target_ref", fr_target_ref[key.target]);
        NewTextChild(rowNode, "airp_arv_name", rpt_params.ElemIdToReportElem(etAirp, key.target, efmtNameLong));
        NewTextChild(rowNode, "grp_id", pax.simple.grp_id);
        NewTextChild(rowNode, "class_name", key.cls_name);
        NewTextChild(rowNode, "class", key.cls);
        NewTextChild(rowNode, "seats", pax_item->seats());
        NewTextChild(rowNode, "crew_type", CrewTypes().encode(pax.simple.crew_type));
        NewTextChild(rowNode, "rk_weight", pax.rk_weight());
        NewTextChild(rowNode, "bag_amount", pax.bag_amount());
        NewTextChild(rowNode, "bag_weight", pax.bag_weight());
        NewTextChild(rowNode, "excess", TComplexBagExcess(pax.excess_wt(), pax.excess_pc()).
                                          view(OutputLang(rpt_params.GetLang()), true));
        // для суммы по группе Всего в классе
        NewTextChild(rowNode, "excess_pc", pax.excess_pc().getQuantity());
        NewTextChild(rowNode, "excess_kg", pax.excess_wt().getQuantity());
        {
          string gender;
          switch(pax.simple.getTrickyGender())
          {
            case TTrickyGender::Male:
              gender = "M";
              break;
            case TTrickyGender::Female:
              gender = "F";
              break;
            default:
              break;
          };
          NewTextChild(rowNode, "pers_type", DocTrickyGenders().encode(pax.simple.getTrickyGender()));
          NewTextChild(rowNode, "gender", gender);
          NewTextChild(rowNode, "tags", pax.get_tags());
          NewTextChild(rowNode, "seat_no", pax_item->seat_no());
          NewTextChild(rowNode, "remarks", pax.rems());
        }
    }

    PMTotalsToXML(PMTotals, fr_target_ref, dataSetsNode, rpt_params);
}

void REPORTS::PTM(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    string rpt_name;
    if(rpt_params.airp_arv.empty() ||
            rpt_params.rpt_type==rtPTMTXT) {
        if(rpt_params.pr_trfer)
            rpt_name="PMTrferTotalEL";
        else
            rpt_name="PMTotalEL";
    } else {
        if(rpt_params.pr_trfer)
            rpt_name="PMTrfer";
        else
            rpt_name="PM";
    };
    if (rpt_params.rpt_type==rtPTMTXT) rpt_name=rpt_name+"Txt";
    get_compatible_report_form(rpt_name, reqNode, resNode);

    {
        string et, et_lat;
        if(rpt_params.pr_et) {
            et = getLocaleText("(ЭБ)", rpt_params.dup_lang());
            et_lat = getLocaleText("(ЭБ)", AstraLocale::LANG_EN);
        }
        NewTextChild(variablesNode, "ptm", getLocaleText("CAP.DOC.PTM", LParams() << LParam("et", et), rpt_params.dup_lang()));
        NewTextChild(variablesNode, "ptm_lat", getLocaleText("CAP.DOC.PTM", LParams() << LParam("et", et_lat), AstraLocale::LANG_EN));
    }
    TQuery Qry(&OraSession);
    string SQLText =
        "SELECT \n"
        "   pax.*, \n"
        "   pax_grp.point_dep AS trip_id, \n"
        "   pax_grp.airp_arv AS target, \n"
        "   points.point_num, \n";
    if(rpt_params.pr_trfer)
        SQLText +=
            "    nvl2(transfer.grp_id, 1, 0) pr_trfer, \n"
            "    trfer_trips.airline trfer_airline, \n"
            "    trfer_trips.flt_no trfer_flt_no, \n"
            "    trfer_trips.suffix trfer_suffix, \n"
            "    transfer.airp_arv trfer_airp_arv, \n"
            "    trfer_trips.scd trfer_scd, \n";
    SQLText +=
        "   nvl(pax.cabin_class_grp, pax_grp.class_grp) class_grp, \n"
        "   DECODE(pax_grp.status, 'T', pax_grp.status, 'N') status, \n"
        "   NVL(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS rk_weight, \n"
        "   NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_amount, \n"
        "   NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_weight, \n"
        "   NVL(ckin.get_excess_wt(pax.grp_id, pax.pax_id, pax_grp.excess_wt, pax_grp.bag_refuse),0) AS excess_wt, \n"
        "   NVL(ckin.get_excess_pc(pax.grp_id, pax.pax_id),0) AS excess_pc, \n"
        "   pax_grp.grp_id \n"
        "FROM  \n"
        "   pax_grp, \n"
        "   points, \n"
        "   pax, \n"
        "   cls_grp, \n"
        "   halls2 \n";
    if(rpt_params.pr_trfer)
        SQLText += ", transfer, trfer_trips \n";
    SQLText +=
        "WHERE \n"
        "   points.pr_del>=0 AND \n"
        "   pax_grp.point_dep = :point_id and \n"
        "   pax_grp.point_arv = points.point_id and \n"
        "   pax_grp.grp_id=pax.grp_id AND \n"
        "   pax_grp.class_grp is not null AND \n"
        "   pax_grp.class_grp = cls_grp.id and \n"
        "   pax_grp.hall = halls2.id(+) and \n"
        "   pax_grp.status NOT IN ('E') and \n"
        "   pr_brd IS NOT NULL and \n"
        "   decode(:pr_brd_pax, 0, nvl2(pax.pr_brd, 0, -1), pax.pr_brd)  = :pr_brd_pax and \n";
    Qry.CreateVariable("pr_brd_pax", otInteger, rpt_params.pr_brd);

    if(not rpt_params.subcls.empty()) {
        SQLText +=
            "   pax.subclass = :subcls and \n";
        Qry.CreateVariable("subcls", otString, rpt_params.subcls);
    }

    if(not rpt_params.cls.empty()) {
        SQLText +=
            "   nvl(pax.cabin_class, pax_grp.class) = :cls and \n";
        Qry.CreateVariable("cls", otString, rpt_params.cls);
    }


    if(rpt_params.pr_et) //ЭБ
        SQLText +=
            "   pax.ticket_rem='TKNE' and \n";
    if(not rpt_params.airp_arv.empty()) { // сегмент
        SQLText +=
            "    pax_grp.airp_arv = :target AND \n";
        Qry.CreateVariable("target", otString, rpt_params.airp_arv);
    }
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        SQLText +=
            "   nvl(halls2.rpt_grp, ' ') = nvl(:zone, ' ') and pax_grp.hall IS NOT NULL and ";
        Qry.CreateVariable("zone", otString, rpt_params.ckin_zone);
    }
    SQLText +=
        "       DECODE(pax_grp.status, 'T', pax_grp.status, 'N') in ('T', 'N') \n";
    if(rpt_params.pr_trfer)
        SQLText +=
            " and pax_grp.grp_id=transfer.grp_id(+) and \n"
            " transfer.pr_final(+) <> 0 and \n"
            " transfer.point_id_trfer = trfer_trips.point_id(+) \n";
    ProgTrace(TRACE5, "SQLText: %s", SQLText.c_str());
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();

    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    REPORTS::TPMPaxList pax_list(rpt_params);
    pax_list.fromDB(Qry);
    pax_list.sort(REPORTS::pax_compare);
    PaxListToXML(pax_list, dataSetsNode, rpt_params);

    // Теперь переменные отчета
    Qry.Clear();
    Qry.SQLText =
        "select "
        "   airp, "
        "   airline, "
        "   flt_no, "
        "   suffix, "
        "   craft, "
        "   bort, "
        "   park_out park, "
        "   scd_out, "
        "   act_out, "
        "   airp_fmt, "
        "   airline_fmt, "
        "   suffix_fmt, "
        "   craft_fmt "
        "from "
        "   points "
        "where "
        "   point_id = :point_id AND pr_del>=0";
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();
    if(Qry.Eof) throw Exception("RunPM: variables fetch failed for point_id " + IntToString(rpt_params.point_id));

    TElemFmt airline_fmt = (TElemFmt)Qry.FieldAsInteger("airline_fmt");
    TElemFmt suffix_fmt = (TElemFmt)Qry.FieldAsInteger("suffix_fmt");
    TElemFmt craft_fmt = (TElemFmt)Qry.FieldAsInteger("craft_fmt");

    string airp = Qry.FieldAsString("airp");
    string airline, suffix;
    int flt_no = NoExists;
    if(rpt_params.mkt_flt.empty()) {
        Franchise::TProp franchise_prop;
        if(
                franchise_prop.get(rpt_params.point_id, Franchise::TPropType::paxManifest) and
                franchise_prop.val == Franchise::pvNo
          ) {
            airline = franchise_prop.franchisee.airline;
            flt_no = franchise_prop.franchisee.flt_no;
            suffix = franchise_prop.franchisee.suffix;
        } else {
            airline = Qry.FieldAsString("airline");
            flt_no = Qry.FieldAsInteger("flt_no");
            suffix = Qry.FieldAsString("suffix");
        }
    } else {
        airline = rpt_params.mkt_flt.airline;
        flt_no = rpt_params.mkt_flt.flt_no;
        suffix = rpt_params.mkt_flt.suffix;
        airline_fmt = efmtCodeNative;
        suffix_fmt = efmtCodeNative;
        craft_fmt = efmtCodeNative;
    }
    string craft = Qry.FieldAsString("craft");
    string tz_region = AirpTZRegion(airp);

    //    TCrafts crafts;

    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        NewTextChild(variablesNode, "zone", get_hall_list(airp, rpt_params));
    } else
        NewTextChild(variablesNode, "zone"); // пустой тег - нет детализации по залу
    NewTextChild(variablesNode, "own_airp_name", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, rpt_params.dup_lang())), rpt_params.dup_lang()));
    NewTextChild(variablesNode, "own_airp_name_lat", getLocaleText("CAP.DOC.AIRP_NAME",  LParams() << LParam("airp", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong, AstraLocale::LANG_EN)), AstraLocale::LANG_EN));
    NewTextChild(variablesNode, "airp_dep_name", rpt_params.ElemIdToReportElem(etAirp, airp, efmtNameLong));
    NewTextChild(variablesNode, "airline_name", rpt_params.ElemIdToReportElem(etAirline, airline, efmtNameLong));

    ostringstream flt;
    flt
        << rpt_params.ElemIdToReportElem(etAirline, airline, airline_fmt)
        << setw(3) << setfill('0') << flt_no
        << rpt_params.ElemIdToReportElem(etSuffix, suffix, suffix_fmt);
    NewTextChild(variablesNode, "flt", flt.str());
    NewTextChild(variablesNode, "bort", Qry.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", rpt_params.ElemIdToReportElem(etCraft, craft, craft_fmt));
    NewTextChild(variablesNode, "park", Qry.FieldAsString("park"));
    TDateTime scd_out = UTCToLocal(getReportSCDOut(rpt_params.point_id), tz_region);
    NewTextChild(variablesNode, "scd_date", DateTimeToStr(scd_out, "dd.mm", rpt_params.IsInter()));
    NewTextChild(variablesNode, "scd_time", DateTimeToStr(scd_out, "hh:nn", rpt_params.IsInter()));
    NewTextChild(variablesNode, "airp_arv_name", rpt_params.ElemIdToReportElem(etAirp, rpt_params.airp_arv, efmtNameLong));

    TDateTime issued = UTCToLocal(NowUTC(),TReqInfo::Instance()->desk.tz_region);
    NewTextChild(variablesNode, "date_issue", DateTimeToStr(issued, "dd.mm.yy hh:nn", rpt_params.IsInter()));

    NewTextChild(variablesNode, "pr_vip", 2);
    string pr_brd_pax_str = (string)"ПАССАЖИРЫ " + (rpt_params.pr_brd ? "(посаж)" : "(зарег)");
    NewTextChild(variablesNode, "pr_brd_pax", getLocaleText(pr_brd_pax_str, rpt_params.dup_lang()));
    NewTextChild(variablesNode, "pr_brd_pax_lat", getLocaleText(pr_brd_pax_str, AstraLocale::LANG_EN));
    NewTextChild(variablesNode, "pr_group", rpt_params.sort == stRegNo); // Если сортировка по рег. но., то выделяем группы пассажиров в fr-отчете
    NewTextChild(variablesNode, "kg", getLocaleText("кг", rpt_params.GetLang()));
    NewTextChild(variablesNode, "pc", getLocaleText("м", rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());
    STAT::set_variables(resNode, rpt_params.GetLang());
    trip_rpt_person(resNode, rpt_params);

    TDateTime takeoff = NoExists;
    if(not Qry.FieldIsNULL("act_out"))
        takeoff = UTCToLocal(Qry.FieldAsDateTime("act_out"), tz_region);
    NewTextChild(variablesNode, "takeoff", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm.yy hh:nn")));
    NewTextChild(variablesNode, "takeoff_date", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm")));
    NewTextChild(variablesNode, "takeoff_time", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "hh:nn")));
}

