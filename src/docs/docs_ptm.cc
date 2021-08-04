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

    DB::TQuery Qry(PgOra::getROSession({"PAX", "PAX_GRP"}), STDLOG);
    Qry.SQLText =
        "SELECT * FROM pax, pax_grp "
        "WHERE "
        "   pax_grp.point_dep = :point_id AND "
        "   pax_grp.grp_id = pax.grp_id ";
    Qry.CreateVariable("point_id", otInteger, pax_list.point_id);
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next())
        pax_list.fromDB(Qry, false /*calcExcessPC*/);
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

void TPMPax::fromDB(DB::TQuery &Qry)
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
                    return l.cls < r.cls;
                    /*
                    if(l.status == r.status)
                        return l.cls < r.cls;
                    else
                        return l.status < r.status;
                        */
                else
                    return l.lvl < r.lvl;
            else
                return l.point_id < r.point_id;
        else
            if(l.point_id == r.point_id)
                if(l.target == r.target)
                    if(l.pr_trfer == r.pr_trfer)
                        if(l.lvl == r.lvl)
                            return l.cls < r.cls;
                            /*
                            if(l.status == r.status)
                                return l.cls < r.cls;
                            else
                                return l.status < r.status;
                                */
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

        key.dump();

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
          NewTextChild(rowNode, "tags", pax.baggage.tags);
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
    DB::TQuery Qry(PgOra::getROSession({"PAX_GRP","POINTS","PAX","CLS_GRP","HALLS2","TRANSFER","TRFER_TRIPS","TCKIN_PAX_GRP"}), STDLOG); // ckin.get_rkWeight2, ckin.get_bagAmount2, ckin.get_bagWeight2, ckin.get_excess_wt
    string SQLText =
        "SELECT "
        "   pax.*, "
        "   pax_grp.point_dep AS trip_id, "
        "   pax_grp.airp_arv AS target, "
        "   points.point_num, ";
    if(rpt_params.pr_trfer)
        SQLText +=
            "    CASE WHEN transfer.grp_id IS NOT NULL THEN 1 ELSE 0 END AS pr_trfer, "
            "    trfer_trips.airline trfer_airline, "
            "    trfer_trips.flt_no trfer_flt_no, "
            "    trfer_trips.suffix trfer_suffix, "
            "    transfer.airp_arv trfer_airp_arv, "
            "    trfer_trips.scd trfer_scd, ";
    SQLText +=
        "   COALESCE(pax.cabin_class_grp, pax_grp.class_grp) class_grp, "
        "   CASE WHEN pax_grp.status = 'T' THEN pax_grp.status ELSE 'N' END AS status, "
        "   0 AS excess_pc, "
        "   pax_grp.grp_id "
        "FROM pax_grp "
        "JOIN points ON pax_grp.point_arv = points.point_id "
        "JOIN pax ON pax_grp.grp_id = pax.grp_id "
        "JOIN cls_grp ON pax_grp.class_grp = cls_grp.id "
        "LEFT OUTER JOIN halls2 ON pax_grp.hall = halls2.id ";
    if(rpt_params.pr_trfer) {
        SQLText += "LEFT OUTER JOIN ( "
                   "  transfer LEFT OUTER JOIN trfer_trips ON transfer.point_id_trfer = trfer_trips.point_id) "
                   "ON pax_grp.grp_id=transfer.grp_id AND transfer.pr_final <> 0 ";
    }
    if(rpt_params.trzt_autoreg != TRptParams::TrztAutoreg::All) {
        SQLText += "LEFT OUTER JOIN tckin_pax_grp "
                   "ON pax_grp.grp_id = tckin_pax_grp.grp_id "
                   "AND tckin_pax_grp.transit_num <> 0 ";
    }

    SQLText +=
        "WHERE "
        "   points.pr_del>=0 AND "
        "   pax_grp.point_dep = :point_id AND "
        "   pax_grp.class_grp is not null AND "
        "   pax_grp.status NOT IN ('E') AND "
        "   pr_brd IS NOT NULL AND "
        "   WHERE "
        "   CASE WHEN :pr_brd_pax = 0 "
        "   THEN (CASE WHEN pax.pr_brd IS NOT NULL THEN 0 ELSE -1 END) "
        "   ELSE pax.pr_brd "
        "   END = :pr_brd_pax AND ";
    Qry.CreateVariable("pr_brd_pax", otInteger, rpt_params.pr_brd);

    if(rpt_params.trzt_autoreg != TRptParams::TrztAutoreg::All) {
        if(rpt_params.trzt_autoreg == TRptParams::TrztAutoreg::Auto)
            SQLText +=
                "   pax_grp.status IN ('T') AND "
                "   tckin_pax_grp.grp_id is NOT NULL AND ";
        else
            SQLText +=
                "   NOT (pax_grp.status IN ('T') AND "
                "   tckin_pax_grp.grp_id IS NOT NULL) AND ";
    }

    if(not rpt_params.subcls.empty()) {
        SQLText +=
            "   pax.subclass = :subcls AND ";
        Qry.CreateVariable("subcls", otString, rpt_params.subcls);
    }

    if(not rpt_params.cls.empty()) {
        SQLText +=
            "   COALESCE(pax.cabin_class, pax_grp.class) = :cls AND ";
        Qry.CreateVariable("cls", otString, rpt_params.cls);
    }


    if(rpt_params.pr_et) //ЭБ
        SQLText +=
            "   pax.ticket_rem='TKNE' AND ";
    if(not rpt_params.airp_arv.empty()) { // сегмент
        SQLText +=
            "    pax_grp.airp_arv = :target AND ";
        Qry.CreateVariable("target", otString, rpt_params.airp_arv);
    }
    if(rpt_params.ckin_zone != ALL_CKIN_ZONES) {
        SQLText +=
            "   COALESCE(halls2.rpt_grp, ' ') = COALESCE(:zone, ' ') AND pax_grp.hall IS NOT NULL AND ";
        Qry.CreateVariable("zone", otString, rpt_params.ckin_zone);
    }
    SQLText +=
        "       CASE WHEN pax_grp.status = 'T' THEN pax_grp.status ELSE 'N' END IN ('T', 'N') ";
    ProgTrace(TRACE5, "SQLText: %s", SQLText.c_str());
    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, rpt_params.point_id);
    Qry.Execute();

    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    REPORTS::TPMPaxList pax_list(rpt_params);
    pax_list.fromDB(Qry, true /*calcExcessPC*/);
    pax_list.sort(REPORTS::pax_compare);
    PaxListToXML(pax_list, dataSetsNode, rpt_params);

    // Теперь переменные отчета
    DB::TQuery QryPoint(PgOra::getROSession("POINTS"), STDLOG);
    QryPoint.SQLText =
        "SELECT "
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
        "FROM "
        "   points "
        "WHERE "
        "   point_id = :point_id AND pr_del>=0";
    QryPoint.CreateVariable("point_id", otInteger, rpt_params.point_id);
    QryPoint.Execute();
    if(QryPoint.Eof) throw Exception("RunPM: variables fetch failed for point_id " + IntToString(rpt_params.point_id));

    TElemFmt airline_fmt = (TElemFmt)QryPoint.FieldAsInteger("airline_fmt");
    TElemFmt suffix_fmt = (TElemFmt)QryPoint.FieldAsInteger("suffix_fmt");
    TElemFmt craft_fmt = (TElemFmt)QryPoint.FieldAsInteger("craft_fmt");

    string airp = QryPoint.FieldAsString("airp");
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
            airline = QryPoint.FieldAsString("airline");
            flt_no = QryPoint.FieldAsInteger("flt_no");
            suffix = QryPoint.FieldAsString("suffix");
        }
    } else {
        airline = rpt_params.mkt_flt.airline;
        flt_no = rpt_params.mkt_flt.flt_no;
        suffix = rpt_params.mkt_flt.suffix;
        airline_fmt = efmtCodeNative;
        suffix_fmt = efmtCodeNative;
        craft_fmt = efmtCodeNative;
    }
    string craft = QryPoint.FieldAsString("craft");
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
    NewTextChild(variablesNode, "bort", QryPoint.FieldAsString("bort"));
    NewTextChild(variablesNode, "craft", rpt_params.ElemIdToReportElem(etCraft, craft, craft_fmt));
    NewTextChild(variablesNode, "park", QryPoint.FieldAsString("park"));
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
    if(not QryPoint.FieldIsNULL("act_out"))
        takeoff = UTCToLocal(QryPoint.FieldAsDateTime("act_out"), tz_region);
    NewTextChild(variablesNode, "takeoff", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm.yy hh:nn")));
    NewTextChild(variablesNode, "takeoff_date", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "dd.mm")));
    NewTextChild(variablesNode, "takeoff_time", (takeoff == NoExists ? "" : DateTimeToStr(takeoff, "hh:nn")));
    LogTrace(TRACE5) << GetXMLDocText(resNode->doc);
}

