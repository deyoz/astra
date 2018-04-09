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
#include "docs.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
namespace fs = boost::filesystem;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace EXCEPTIONS;

const char* STAT_FV_PATH()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("STAT_FV_PATH","");
  return VAR.c_str();
};

struct TGrpInfo {
    private:
        string airline;
    public:
        struct TGrpInfoItem {
            string cls;
            vector<CheckIn::TTransferItem> trfer;

            struct TPrWeapon {
                map<int, bool> items; // <bag_pool_num, pr_weapon>
                void fromDB(const CheckIn::TPaxGrpItem &grp, const string &airline);
                bool get(int bag_pool_num);
            };

            TPrWeapon pr_weapon;

            void clear()
            {
                cls.clear();
                trfer.clear();
            }
            TGrpInfoItem() { clear(); }
        };

        typedef map<int, TGrpInfoItem> TGrpInfoMap;

        TGrpInfoMap items;
        TGrpInfoMap::iterator get(int grp_id);
        TGrpInfo(const string _airline): airline(_airline) {}
};

bool TGrpInfo::TGrpInfoItem::TPrWeapon::get(int bag_pool_num)
{
    return items.find(bag_pool_num) != items.end();
}

void TGrpInfo::TGrpInfoItem::TPrWeapon::fromDB(const CheckIn::TPaxGrpItem &grp, const string &airline)
{
    TCachedQuery Qry("select * from bag2 where grp_id = :grp_id",
            QParams() << QParam("grp_id", otInteger, grp.id));
    Qry.get().Execute();
    if(not Qry.get().Eof) {
        t_rpt_bm_bag_name bag_names;
        bag_names.init("", airline, true);
        TRptParams rpt_params(AstraLocale::LANG_EN);
        for(; not Qry.get().Eof; Qry.get().Next()) {
            TBagTagRow row;
            row.rfisc = Qry.get().FieldAsString("rfisc");
            if(not Qry.get().FieldIsNULL("bag_type"))
                row.bag_type = Qry.get().FieldAsInteger("bag_type");
            bag_names.get(grp.cl, row, rpt_params);
            if(row.bag_name == "WEAPON")
                items.insert(make_pair(Qry.get().FieldAsInteger("bag_pool_num"), true));
        }
    }
}

TGrpInfo::TGrpInfoMap::iterator TGrpInfo::get(int grp_id)
{
    TGrpInfoMap::iterator result = items.find(grp_id);
    if(result == items.end()) {
        CheckIn::TPaxGrpItem grp;
        grp.fromDB(grp_id);
        TGrpInfoItem item;
        item.cls = grp.cl;
        CheckIn::LoadTransfer(grp_id, item.trfer);
        item.pr_weapon.fromDB(grp, airline);
        pair<TGrpInfoMap::iterator, bool> ret = items.insert(make_pair(grp_id, item));
        result = ret.first;
    }
    return result;
}

struct TTrferInfo {
    bool pr_trfer_dep;
    bool pr_trfer_arv;
    string airp_arv;
    void clear()
    {
        pr_trfer_dep = false;
        pr_trfer_arv = false;
        airp_arv.clear();
    }
    TTrferInfo() { clear(); }
    void get(int pax_id, const TTripRoute &route, const vector<CheckIn::TTransferItem> &trfer);
};

void TTrferInfo::get(int pax_id, const TTripRoute &route, const vector<CheckIn::TTransferItem> &trfer)
{
    map<int, CheckIn::TCkinPaxTknItem> tkns;

    GetTCkinTicketsBefore(pax_id, tkns);
    pr_trfer_dep = not tkns.empty();

    GetTCkinTicketsAfter(pax_id, tkns);
    pr_trfer_arv = not tkns.empty();

    if(pr_trfer_arv) {
        LogTrace(TRACE5) << "pax_id: " << pax_id << "; trfer by GetTCkinTickets";
        CheckIn::TSimplePaxItem pax;
        pax.getByPaxId(tkns.rbegin()->second.pax_id);
        CheckIn::TPaxGrpItem grp;
        grp.fromDB(pax.grp_id);
        airp_arv = grp.airp_arv;
    } else {
        pr_trfer_arv = not trfer.empty();
        if(pr_trfer_arv) {
            LogTrace(TRACE5) << "pax_id: " << pax_id << "; trfer by TTransferItem";
            airp_arv = trfer.back().airp_arv;
        } else {
            airp_arv = route.begin()->airp;
        }
    }

    /* 
    pr_trfer_arv = not trfer.empty();
    if(pr_trfer_arv) {
        LogTrace(TRACE5) << "pax_id: " << pax_id << "; trfer by TTransferItem";
        airp_arv = trfer.back().airp_arv;
    } else {
        GetTCkinTicketsAfter(pax_id, tkns);
        pr_trfer_arv = not tkns.empty();
        if(pr_trfer_arv) {
            LogTrace(TRACE5) << "pax_id: " << pax_id << "; trfer by GetTCkinTickets";
            CheckIn::TSimplePaxItem pax;
            pax.getByPaxId(tkns.rbegin()->second.pax_id);
            CheckIn::TPaxGrpItem grp;
            grp.fromDB(pax.grp_id);
            airp_arv = grp.airp_arv;
        } else {
            airp_arv = route.begin()->airp;
        }
    }
    */
}

void get_trfer_info(
        int pax_id,
        bool &pr_trfer_dep,
        bool &pr_trfer_arv,
        string &airp_arv
        ) {
}

void stat_fv_toXML(xmlNodePtr rootNode, int point_id)
{
    TReqInfo::Instance()->desk.lang = AstraLocale::LANG_EN;
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
    if(not info.bort.empty())
        NewTextChild(flightInfoNode, "AirplaneRegNumber", info.bort);

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

    xmlNodePtr GeneralDeclarationNode = NewTextChild(rootNode, "GeneralDeclaration");
    xmlNodePtr PassengerManifestNode = NewTextChild(rootNode, "PassengerManifest");

    if(not Qry.get().Eof) {
        xmlNodePtr PassengerInfoNode = NewTextChild(GeneralDeclarationNode, "PassengerInfo");
        // Кол-во пассажиров, принятых в пункте отправления
        xmlNodePtr DeparturePassNode = NewTextChild(PassengerInfoNode, "DeparturePass");
        // Кол-во трансфертных пассажиров в аэропорту убытия
        xmlNodePtr TransferDepartPassNode = NewTextChild(PassengerInfoNode, "TransferDepartPass");
        // Кол-во пассажиров, выходящих  в аэропорту назначения
        xmlNodePtr DestinationPassNode = NewTextChild(PassengerInfoNode, "DestinationPass");
        // Кол-во трансфертных пассажиров в аэропорту назначения
        xmlNodePtr TransferDestinationPassNode = NewTextChild(PassengerInfoNode, "TransferDestinationPass");


        TGrpInfo grp_info_map(info.airline);

        int DeparturePass = 0;
        int TransferDepartPass = 0;
        int DestinationPass = 0;
        int TransferDestinationPass = 0;

        for(; not Qry.get().Eof; Qry.get().Next()) {
            xmlNodePtr PassengerInfoNode = NewTextChild(PassengerManifestNode, "PassengerInfo");
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
            auto grp_info = grp_info_map.get(pax.grp_id);

            TTrferInfo trfer_info;
            trfer_info.get(pax.id, route, grp_info->second.trfer);

            if(trfer_info.pr_trfer_dep)
                TransferDepartPass++;
            if(trfer_info.pr_trfer_arv)
                TransferDestinationPass++;
            if(not trfer_info.pr_trfer_dep)
                DeparturePass++;
            if(not trfer_info.pr_trfer_arv)
                DestinationPass++;

            NewTextChild(PassengerNode, "PassClass", ElemIdToCodeNative(etClass, grp_info->second.cls));
            // Вес багажа брутто
            NewTextChild(PassengerNode, "GrossWeightQuantity", Qry.get().FieldAsInteger("bag_weight"));
            // Код измерения веса багажа (килограммы или фунты, K/F)
            NewTextChild(PassengerNode, "WeightUnitQualifierCode", "K");
            NewTextChild(PassengerNode, "DestinationAirportIATACode", ElemIdToCodeNative(etAirp, trfer_info.airp_arv));
            NewTextChild(PassengerNode, "WeaponSign", grp_info->second.pr_weapon.get(pax.bag_pool_num));
            NewTextChild(PassengerNode, "PsychotropicAgentSign", 0);

            multiset<TBagTagNumber> tags;
            GetTagsByPool(pax.grp_id, pax.bag_pool_num, tags);
            if(not tags.empty()) {
                xmlNodePtr LuggageTagNode = NewTextChild(PassengerNode, "LuggageTag");
                for(const auto &tag : tags)
                    NewTextChild(LuggageTagNode, "LuggageNumber", tag.str());
            }
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
    if(strlen(STAT_FV_PATH()) == 0) {
        LogTrace(TRACE5) << __FUNCTION__ << "STAT_FV_PATH is empty";
        return;
    }
    XMLDoc doc("DATAPACKET");
    xmlNodePtr rootNode = doc.docPtr()->children;
    SetProp(rootNode, "xmlns", "http://tempuri.org/XMLSchema.xsd");
    stat_fv_toXML(rootNode, task.point_id);
    fs::path full_path = fs::system_complete(fs::path(STAT_FV_PATH()));
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
    out << ConvertCodepage(GetXMLDocTextOptions(doc.docPtr(), xmlSaveOption(XML_SAVE_FORMAT + XML_SAVE_NO_EMPTY)), "CP866", "UTF-8");
}
