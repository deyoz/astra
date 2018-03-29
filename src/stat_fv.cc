#include "stat_fv.h"
#include "xml_unit.h"
#include "boost/filesystem/operations.hpp"
#include <fstream>
#include "stl_utils.h"
#include "date_time.h"
#include "astra_misc.h"
#include "astra_locale.h"
#include "qrys.h"
#include "passenger.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
namespace fs = boost::filesystem;
using namespace BASIC::date_time;
using namespace ASTRA;

struct TGrpInfo {
    struct TGrpInfoItem {
        string cls;
        string airp_arv;

        bool pr_trfer_dep;
        bool pr_trfer_arv;
        void clear() {
            cls.clear();
            airp_arv.clear();
            pr_trfer_dep = false;
            pr_trfer_arv = false;
        }
        TGrpInfoItem() { clear(); }
    };

    typedef map<int, TGrpInfoItem> TGrpInfoMap;

    TGrpInfoMap items;
    TGrpInfoMap::iterator get(const TTripRoute &route, int grp_id);
};

TGrpInfo::TGrpInfoMap::iterator TGrpInfo::get(const TTripRoute &route, int grp_id)
{
    TGrpInfoMap::iterator result = items.find(grp_id);
    if(result == items.end()) {
        CheckIn::TPaxGrpItem grp;
        grp.fromDB(grp_id);
        TGrpInfoItem _grp_info;
        _grp_info.cls = grp.cl;



        /*
        TCkinRoute ckin_route;
        ckin_route.GetRouteBefore(grp_id, crtNotCurrent, crtIgnoreDependent);
        _grp_info.pr_trfer_dep=!ckin_route.empty();
        if(not _grp_info.pr_trfer) {
        }
        */


        TCkinRoute ckin_route;
        ckin_route.GetRouteBefore(grp_id, crtNotCurrent, crtIgnoreDependent);
        _grp_info.pr_trfer_dep=!ckin_route.empty();
        ckin_route.GetRouteAfter(grp_id, crtNotCurrent, crtIgnoreDependent);
        _grp_info.pr_trfer_arv = not ckin_route.empty();

        if(_grp_info.pr_trfer_arv) {
            _grp_info.airp_arv = ckin_route.back().airp_arv;
        } else {
            _grp_info.airp_arv = route.begin()->airp;
        }
        pair<map<int, TGrpInfoItem>::iterator, bool> ret = items.insert(make_pair(grp_id, _grp_info));
        result = ret.first;

    }
    return result;
}

void stat_fv_toXML(xmlNodePtr rootNode, int point_id)
{
    TReqInfo::Instance()->desk.lang = AstraLocale::LANG_EN;
//    xmlNodePtr airlineNode = NewTextChild(rootNode, "airline", "ФВ");
    xmlNodePtr flightInfoNode = NewTextChild(rootNode, "FlightInfo");

    TTripInfo info;
    info.getByPointId(point_id);
    ostringstream flight_number;
    flight_number
        << ElemIdToCodeNative(etAirline, info.airline)
        << setw(3) << setfill('0') << info.flt_no
        << ElemIdToCodeNative(etSuffix, info.suffix);

    NewTextChild(flightInfoNode, "FlightNumber", flight_number.str());
    NewTextChild(flightInfoNode, "FlightDate", DateTimeToStr(info.act_est_scd_out(), "yyyy-mm-dd"));
    NewTextChild(flightInfoNode, "FlightTime", DateTimeToStr(info.act_est_scd_out(), "hh:nn:ss"));
    NewTextChild(flightInfoNode, "DepartureAirportIATACode", ElemIdToCodeNative(etAirp, info.airp));
    TTripRoute route;
    route.GetRouteAfter(NoExists, info.point_id, trtNotCurrent, trtNotCancelled);
    NewTextChild(flightInfoNode, "ArrivalAirportIATACode", ElemIdToCodeNative(etAirp, route.begin()->airp));
    NewTextChild(flightInfoNode, "AirlineIATACode", ElemIdToCodeNative(etAirline, info.airline));

    xmlNodePtr GeneralDeclarationNode = NewTextChild(rootNode, "GeneralDeclaration");
    xmlNodePtr PassengerInfoNode = NewTextChild(GeneralDeclarationNode, "PassengerInfo");
    // Кол-во пассажиров, принятых в пункте отправления
    xmlNodePtr DeparturePassNode = NewTextChild(PassengerInfoNode, "DeparturePass");
    // Кол-во трансфертных пассажиров в аэропорту убытия
    xmlNodePtr TransferDepartPassNode = NewTextChild(PassengerInfoNode, "TransferDepartPass");
    // Кол-во пассажиров, выходящих  в аэропорту назначения
    xmlNodePtr DestinationPassNode = NewTextChild(PassengerInfoNode, "DestinationPass");
    // Кол-во трансфертных пассажиров в аэропорту назначения
    xmlNodePtr TransferDestinationPassNode = NewTextChild(PassengerInfoNode, "TransferDestinationPass");

    TCachedQuery Qry(
            "select "
            "   pax.*, "
            "   salons.get_seat_no(pax.pax_id,pax.seats,pax.is_jmp,pax_grp.status,pax_grp.point_dep,null,rownum,:pr_lat) AS seat_no, "
            "   NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_weight "
            "from "
            "   pax_grp, "
            "   pax "
            "where "
            "   pax_grp.point_dep = :point_id and "
            "   pax_grp.grp_id = pax.grp_id ",
            QParams()
            << QParam("point_id", otInteger, point_id)
            << QParam("pr_lat", otInteger, 1)
            );

    Qry.get().Execute();
    if(not Qry.get().Eof) {
        xmlNodePtr PassengerManifestNode = NewTextChild(rootNode, "PassengerManifest");
        xmlNodePtr PassengerInfoNode = NewTextChild(PassengerManifestNode, "PassengerInfo");

        TGrpInfo grp_info_map;

        int DeparturePass = 0;
        int TransferDepartPass = 0;
        int DestinationPass = 0;
        int TransferDestinationPass = 0;

        for(; not Qry.get().Eof; Qry.get().Next()) {
            CheckIn::TPaxItem pax;
            pax.fromDB(Qry.get());
            if(
                    not pax.doc.type.empty() or
                    not pax.doc.no.empty() or
                    not pax.doc.issue_country.empty()) {

                xmlNodePtr IdentityCardNode = NewTextChild(PassengerInfoNode, "IdentityCard");
                // Код вида документа, удостоверяющего личность. Для РФ
                NewTextChild(IdentityCardNode, "IdentityCardCode", pax.doc.type, "");
                // Номер документа, удостоверяющего личность
                NewTextChild(IdentityCardNode, "IdentityCardNumber", pax.doc.no, "");
                // Наименование страны, выдавшей документ
                NewTextChild(IdentityCardNode, "CountryName", pax.doc.issue_country, "");
            }
            xmlNodePtr PassengerNode = NewTextChild(PassengerInfoNode, "Passenger");
            NewTextChild(PassengerNode, "PersonSurname", transliter(pax.surname, 1, true), "");
            NewTextChild(PassengerNode, "PersonName", transliter(pax.name, 1, true), "");
            NewTextChild(PassengerNode, "Sex", pax.is_female() == NoExists or not pax.is_female() ? "MR" : "MS");
            NewTextChild(PassengerNode, "SeatNumber", pax.seat_no);
            TGrpInfo::TGrpInfoMap::iterator grp_info = grp_info_map.get(route, pax.grp_id);
            if(grp_info->second.pr_trfer_dep)
                TransferDepartPass++;
            if(grp_info->second.pr_trfer_arv)
                TransferDestinationPass++;
            if(
                    not grp_info->second.pr_trfer_dep and 
                    not grp_info->second.pr_trfer_dep)
            {
                DeparturePass++;
                DestinationPass++;
            }
            NewTextChild(PassengerNode, "PassClass", ElemIdToCodeNative(etClass, grp_info->second.cls));
            // Вес багажа брутто
            NewTextChild(PassengerNode, "GrossWeightQuantity", Qry.get().FieldAsInteger("bag_weight"));
            // Код измерения веса багажа (килограммы или фунты, K/F)
            NewTextChild(PassengerNode, "WeightUnitQualifierCode", "K");
            NewTextChild(PassengerNode, "DestinationAirportIATACode", ElemIdToCodeNative(etAirp, grp_info->second.airp_arv));
//            NewTextChild(PassengerNode, "WeaponSign");
            NewTextChild(PassengerNode, "PsychotropicAgentSign", 0);
        }
        NodeSetContent(DeparturePassNode, DeparturePass);
        NodeSetContent(TransferDepartPassNode, TransferDepartPass);
        NodeSetContent(DestinationPassNode, DestinationPass);
        NodeSetContent(TransferDestinationPassNode, TransferDestinationPass);
    }
}

void stat_fv(const TTripTaskKey &task)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": " << task;
    XMLDoc doc("DATAPACKET");
    xmlNodePtr rootNode = doc.docPtr()->children;
    stat_fv_toXML(rootNode, task.point_id);
    fs::path full_path = fs::system_complete(fs::path("STAT_FV"));
    fs::create_directories(full_path);
    /*
       if ( !fs::exists( full_path ) ) {
       LogTrace(TRACE5) << __FUNCTION__ << ": path not found: " << full_path.string();
       return;
       }
       */

    TDateTime now_utc = NowUTC();
    double days;
    int msecs = (int)(modf(now_utc, &days) * MSecsPerDay) % 1000;
    ostringstream fname;
    fname << "ASTRA_" << DateTimeToStr(now_utc, "yyyymmdd_hhnnss") << setw(3) << setfill('0') << msecs << ".xml";

    fs::path apath = full_path / fname.str();
    ofstream out(apath.string().c_str());
    if(!out.good()) {
        LogTrace(TRACE5) << __FUNCTION__ << ": Cannot open file " << apath.string();
        return;
    }
    out << ConvertCodepage(GetXMLDocText(doc.docPtr()), "CP866", "UTF-8");
}
