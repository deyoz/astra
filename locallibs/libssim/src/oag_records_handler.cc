#include "oag_records_handler.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/regex.hpp>

#include <serverlib/dates.h>
#include <serverlib/exception.h>
#include <serverlib/str_utils.h>

#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cctype>

#define NICKNAME "D.ZERNOV"
#include <serverlib/slogger.h>

static const size_t MaxLineLength = 58;

namespace {

static std::string convertDateVar(char dateVar)
{
    if (dateVar == 'A') {
        return "M1";
    }
    return std::string(1, dateVar);
}

static std::string MakeFlightText(const Oag::CarrierRecord& record)
{
    const Oag::FlightLegRecord& flg = record.flightLegs.front();
    return StrUtils::trim(flg.airline)
        + StrUtils::LPad(HelpCpp::string_cast(flg.flightNum), 3, '0')
        + flg.opSuffix.get_value_or(std::string());
}

static std::vector<std::string> convertMeals(const std::string& prbd, const std::string& meals)
{
    std::vector<std::string> res;

    const std::string rbds = StrUtils::trim(prbd);
    if (rbds.empty() || StrUtils::trim(meals).empty()) {
        return res;
    }

    //for ssim chapter 7: 2 bytes (meal service codes) per rbd in the same order as in prbd
    const std::string mls = StrUtils::RPad(meals, rbds.size() * 2, ' ');

    std::vector< std::pair< char, std::string > > sparsedMeals;
    for (size_t i = 0; i < rbds.size(); ++i) {
        sparsedMeals.emplace_back(rbds.at(i), StrUtils::trim(mls.substr(2 * i, 2)));
    }

    if (sparsedMeals.size() > 1) {
        const std::string& m = sparsedMeals.front().second;
        const bool sameMeals = std::all_of(sparsedMeals.begin(), sparsedMeals.end(), [&m] (const auto& v) {
            return v.second == m;
        });

        if (sameMeals && !m.empty()) {
            //using short chapter 4 format
            res.push_back("//" + m);
            return res;
        }
    }

    //using long format
    const size_t headerSize = 10; //DEPARR nnn

    std::string ssmMeals;
    for (const auto& v : sparsedMeals) {
        if (v.second.empty()) {
            continue;
        }

        const std::string md = '/' + std::string(1, v.first) + v.second;
        if (ssmMeals.size() + md.size() + headerSize > MaxLineLength) {
            res.push_back(ssmMeals);
            ssmMeals.clear();
        }
        ssmMeals.append(md);
    }
    if (!ssmMeals.empty()) {
        res.push_back(ssmMeals);
    }
    return res;
}

static void appendDupLegCrossRefs(std::ostream& os, size_t deiCode, const Oag::SegmentData& sd)
{
    std::vector<std::string> dupRefs;
    std::string currentDupRefs;
    for (const std::string& ref : StrUtils::split_string< std::vector<std::string> >(sd.data, '/', StrUtils::KeepEmptyTokens::True)) {
        //fixed length flight format: 'LH    1 ' -> 'LH001'
        const std::string f = StrUtils::trim(ref);
        if (f.size() < 7) {
            LogTrace(TRACE5) << "bad dublicate leg cross reference: '" << ref << "'";
            continue;
        }
        const std::string flt(
            StrUtils::trim(f.substr(0, 3))
            + StrUtils::LPad(StrUtils::trim(f.substr(3, 4)), 3, '0')
            + (f.size() == 8 ? f.substr(7) : std::string())
        );

        if (currentDupRefs.size() + flt.size() + 10 + 1 > MaxLineLength) {
            dupRefs.push_back(currentDupRefs);
            currentDupRefs.clear();
        }
        currentDupRefs += std::string(currentDupRefs.empty() ? 0 : 1, '/') + flt;
    }
    if (!currentDupRefs.empty()) {
        dupRefs.push_back(currentDupRefs);
    }
    for (const std::string& ref : dupRefs) {
        os << sd.dap.dep << sd.dap.arr << " " << deiCode << "/" << ref << std::endl;
    }
}

static const Oag::SegmentData* findDeiData(const Oag::FlightLegRecord& leg, size_t deiCode, const Oag::DepArrPoints& dap)
{
    auto si = leg.segData.find(deiCode);
    if (si != leg.segData.end()) {
        for (const Oag::SegmentData& sd : si->second) {
            if (sd.dap.dep == dap.dep && sd.dap.arr == dap.arr) {
                return &sd;
            }
        }
    }
    return nullptr;
}

static std::string getLegPRBD(const Oag::FlightLegRecord& leg)
{
    if (leg.passReservBookDes) {
        if (leg.passReservBookDes.get() != "XX") {
            return leg.passReservBookDes.get();
        }
        //try to get PRBD from DEI 106
        const Oag::SegmentData* si = findDeiData(leg, 106, { leg.departureStation, leg.arrivalStation });
        if (si) {
            return StrUtils::trim(si->data);
        }
    }
    return std::string();
}

static std::string getSegPRBD(const Oag::FlightLegRecord& leg, const Oag::DepArrPoints& dap)
{
    //assume correct file sequence
    //so all segment DEIs should be stated inside (with reference of) first leg of segment
    const Oag::SegmentData* si = findDeiData(leg, 101, dap);
    if (si) {
        return StrUtils::trim(si->data);
    }
    //no overrides - use prbd from the first leg
    return getLegPRBD(leg);
}

static void appendAuxTrafficData(
        std::ostream& os,
        const Oag::FlightLegRecord& leg,
        const Oag::DepArrPoints& dap,
        bool withQualifiers
    )
{
    for (const auto& v : leg.segData) {
        if (!((v.first >= 170 && v.first <= 173 && withQualifiers) || (v.first >= 713 && v.first <= 799))) {
            continue;
        }
        for (const Oag::SegmentData& sd : v.second) {
            if (sd.dap.dep == dap.dep && sd.dap.arr == dap.arr) {
                os << dap.dep << dap.arr << " 8/Z/" << v.first
                   << '/' << StrUtils::trim(sd.data) << std::endl;
            }
        }
    }
}

static void appendTrafficRestrictions(
        std::ostream& os, const Oag::FlightLegRecord& leg,
        const std::vector<std::string>& arrivalStations
    )
{
    for (size_t i = 0; i < leg.traffRestrCode.size(); ++i) {
        const char code = leg.traffRestrCode[i];
        if (code == ' ') {
            //no restriction
            continue;
        }

        const Oag::DepArrPoints dap(leg.departureStation, arrivalStations[i]);

        //get qualifiers if any stated
        size_t qualifier = 0;
        for (size_t qi : { 710, 711, 712 }) {
            if (findDeiData(leg, qi, dap)) {
                qualifier = qi;
                break;
            }
        }

        if (code == 'Z') {
            //restrictions should be stated in DEI 170-173
            //there can be additional comment in DEI 713-799
            if (qualifier != 0) {
                os << dap.dep << dap.arr << " 8/Z/" << qualifier << std::endl;
            }
            appendAuxTrafficData(os, leg, dap, true);
        } else {
            //one restriction for all traffic types
            os << dap.dep << dap.arr << " 8/" << code;
            if (qualifier != 0) {
                os << '/' << qualifier;
            }
            os << std::endl;

            appendAuxTrafficData(os, leg, dap, false);
        }
    }
    //-------------------------------------------------------------------------
    if (leg.traffRestrOverflow) {
        //more than 11 leg with restrictions
        for (size_t i = 11; i < arrivalStations.size(); ++i) {
            const Oag::DepArrPoints dap(leg.departureStation, arrivalStations[i]);

            for (size_t qi : { 710, 711, 712 }) {
                if (findDeiData(leg, qi, dap)) {
                    os << dap.dep << dap.arr << " 8/Z/" << qi << std::endl;
                    break;
                }
            }
            appendAuxTrafficData(os, leg, dap, true);
        }
    }
}

static void appendSecureFlight(std::ostream& os, const Oag::FlightLegRecord& leg, const Oag::CarrierRecord& rec)
{
    if ((rec.secureFlight == 'S' && leg.secureFlight != 'X') || leg.secureFlight == 'S') {
        os << leg.departureStation << leg.arrivalStation << " 504/S" << std::endl;
    }
}

} //anonymous

namespace Oag {

bool operator< (const DepArrPoints& x, const DepArrPoints& y)
{
    return (x.dep < y.dep) ? true : (x.arr < y.arr);
}

void OagRecordsHandler::handleRecords(const CarrierRecord& record)
{
    std::ostringstream os;

    os << MakeHeading(record);
    os << MakeActionInfo(record);
    os << MakeFlightInfo(record);
    os << MakePeriodInfo(record);
    os << MakeRoutingInfo(record);
    os << MakeSegmentInfo(record);

    const std::string result = os.str();

    if (outFile.empty()) {
        std::cout << result;
    } else {
        std::ofstream out( outFile, std::ios_base::out | std::ios_base::app );
        if ( !out.good() ) {
            std::cerr << "cannot open file '" << outFile << "' for writing" << std::endl;
            return;
        }
        out << result;
    }
}

void OagRecordsHandler::handleCarrierProcBegin(const std::string& carrier)
{
    if (outFile.empty()) {
        carrierProcBegin(carrier, std::cout);
    } else {
        std::ofstream out( outFile, std::ios_base::out | std::ios_base::app );
        if (out.good()) {
            carrierProcBegin(carrier, out);
        } else {
            std::cerr << "cannot open file '" << outFile << "' for writing" << std::endl;
        }
    }
}

void OagRecordsHandler::handleCarrierProcEnd(const std::string& carrier, const CarrierRecord& rec)
{
    if (outFile.empty()) {
        carrierProcEnd(carrier, rec, std::cout);
    } else {
        std::ofstream out( outFile, std::ios_base::out | std::ios_base::app );
        if (out.good()) {
            carrierProcEnd(carrier, rec, out);
        } else {
            std::cerr << "cannot open file '" << outFile << "' for writing" << std::endl;
        }
    }
}

HeaderComposer::HeaderComposer(const std::string& center_)
    : reciever(), sender("OAGFILE"), center(center_)
{}

HeaderComposer::HeaderComposer(const std::string& rcv, const std::string& snd)
    : reciever(rcv),
      sender(snd.size() <= 5
          ? snd.substr(0, 3) + "RM" + (snd.size() <= 3 ? std::string() : snd.substr(3))
          : snd
      )
{}

static std::string make_tlg_id(const boost::posix_time::ptime& tm)
{
    return Dates::ddmmrr(tm.date()).substr(0, 2) + Dates::hhmi(tm.time_of_day());
}

std::string HeaderComposer::compose(const std::string& airline) const
{
    const std::string rcv = (reciever.empty()
        ? center + "RM" + StrUtils::trim(airline)
        : reciever
    );
    return rcv + "\n." + sender + ' ' + make_tlg_id(Dates::currentDateTime()) + '\n';
}

OagSsmCreator::OagSsmCreator(const HeaderComposer& hc, const SsmGenerationOptions& opts, const std::string& outFile)
    : OagRecordsHandler(outFile), headerComposer(hc), genOpts(opts)
{
    typedef boost::sregex_token_iterator regex_iterator;
    std::ifstream saleconf("saleconf.map");
    if (!saleconf) {
        return;
    }
    std::string conf;
    boost::regex e("([0-9A-Z]{3})\\s{1,}(\".+\")\\s{1,}(.+)$");
    regex_iterator end;
    while (!saleconf.eof()) {
        std::getline(saleconf, conf);
        regex_iterator airIt(conf.begin(), conf.end(), e, 1);
        regex_iterator subclsIt(conf.begin(), conf.end(), e, 2);
        regex_iterator airConfIt(conf.begin(), conf.end(), e, 3);
        if (airIt != end && subclsIt != end && airConfIt != end) {
            std::string subcls = *subclsIt;
            std::string airConf = *airConfIt;
            StrUtils::replaceSubstr(subcls, "\"", "");
            StrUtils::replaceSubstr(subcls, " ", "");
            SaleConfRecord rec = {subcls, airConf};
            saleConfs[*airIt] = rec;
        }
    }
}

void OagSsmCreator::carrierProcBegin(const std::string& carrier, std::ostream&)
{
    LogTrace(TRACE1) << __FUNCTION__ << ": " << carrier;

    airline = StrUtils::rtrim(carrier);
    flights.clear();
}

void OagSsmCreator::carrierProcEnd(const std::string& carrier, const CarrierRecord& record, std::ostream& out)
{
    LogTrace(TRACE1) << __FUNCTION__ << ": " << carrier;

    if (flights.empty()) {
        return;
    }

    if (airline != StrUtils::rtrim(carrier)) {
        LogTrace(TRACE1) << "Unexpected carrier change: " << airline << " -> " << carrier;
        return;
    }

    if (genOpts.cancelMissingFlights && genOpts.findMissingFlights) {
        for (const std::string& flt : genOpts.findMissingFlights(flights)) {
            out << std::endl;
            out << headerComposer.compose(airline);
            out << "SSM" << std::endl;
            out << ((record.timeMode == CarrierRecord::UTC_TIME_MODE) ? "UTC" : "LT") << std::endl;
            out << "CNL XASM" << std::endl;
            out << flt << std::endl;
            out << record.schedulePeriodFrom << " ";
            if (genOpts.forceInfiniteValidity) {
                out << "00XXX00";
            } else {
                out << record.schedulePeriodTo;
            }
            out << " 1234567" << std::endl;
        }
    }

    airline.clear();
    flights.clear();
}

std::string OagSsmCreator::MakeHeading(const CarrierRecord& record)
{
    const std::string currentFlight = MakeFlightText(record);
    if (currentFlight == flight) {
        return "";
    }
    std::ostringstream os;
    if (!flight.empty()) {
        os << std::endl;
    }

    os << headerComposer.compose(record.airline);
    os << "SSM" << std::endl;

    os << ((record.timeMode == CarrierRecord::UTC_TIME_MODE) ? "UTC" : "LT") << std::endl;

    os << record.creationDate.substr(0, 5) << "00001E001/000000-";
    os << currentFlight << "/";
    os << record.creationDate.substr(0, 5) << std::endl;

    os << "SKD XASM" << std::endl;
    os << currentFlight << std::endl;
    os << record.schedulePeriodFrom << " ";
    if (genOpts.forceInfiniteValidity) {
        os << "00XXX00";
    } else {
        os << record.schedulePeriodTo;
    }
    os << std::endl;

    flight = currentFlight;

    if (airline != StrUtils::rtrim(record.airline)) {
        LogTrace(TRACE1) << "Unexpected carrier change: " << airline << " -> " << record.airline;
        handleCarrierProcBegin(record.airline);
    } else {
        flights.insert(currentFlight);
    }

    return os.str();
}

std::string OagSsmCreator::MakeActionInfo(const CarrierRecord&)
{
    return "//\nNEW\n";
}

static std::string resolveOprDisclosureInfo(const FlightLegRecord& leg)
{
    if (leg.airDiscloseCsh) {
        switch (leg.airDiscloseCsh.get()) {
        case 'L': return " 2/" + leg.aircraftOwner;
        case 'S': return " 9/" + leg.aircraftOwner;
        case 'Z': return " 2/X";
        case 'X': return " 9/X";
        default:;
        }
    }
    return std::string();
}

static bool isEntireOprDisclosure(const CarrierRecord& cr)
{
    if (cr.flightLegs.size() < 2) {
        return true;
    }
    for (auto li = std::next(cr.flightLegs.begin()), le = cr.flightLegs.end(); li != le; ++li) {
        if (li->airDiscloseCsh != cr.flightLegs.front().airDiscloseCsh) {
            return false;
        }
    }
    return true;
}

std::string OagSsmCreator::MakeFlightInfo(const CarrierRecord& record)
{
    std::ostringstream os;

    os << MakeFlightText(record);

    if (isEntireOprDisclosure(record)) {
        os << resolveOprDisclosureInfo(record.flightLegs.front());
    }

    os << std::endl;
    return os.str();
}

std::string OagSsmCreator::MakePeriodInfo(const CarrierRecord& record)
{
    std::ostringstream os;
    const FlightLegRecord& flg = record.flightLegs.front();
    os << flg.opPeriodFrom << " ";
    os << flg.opPeriodTo << " ";
    os << flg.opDays;
    if (flg.biweekly) {
        os << "/W2";
    }
    os << std::endl;
    return os.str();
}

std::string OagSsmCreator::makeEquipmentInfo(const FlightLegRecord& leg) const
{
    std::ostringstream os;
    os << leg.serviceType << " ";

    if (!saleConfs.empty()) {
        SaleMap::const_iterator conf = saleConfs.find(leg.airType);
        if (conf == saleConfs.end()) {
            conf = saleConfs.begin();
            os << conf->first;
        } else {
            os << leg.airType;
        }

        os << " " << conf->second.subcls << "." << conf->second.airConf;
    } else {
        os << leg.airType << ' ' << getLegPRBD(leg);
        if (leg.airConfig) {
            os << "." << leg.airConfig.get();
        }
    }
    return os.str();
}

std::string OagSsmCreator::MakeRoutingInfo(const CarrierRecord& record)
{
    std::ostringstream os;

    std::string prvEquip;
    for (const FlightLegRecord& leg : record.flightLegs) {
        //append equipment information (it can vary by leg)
        const std::string equip = makeEquipmentInfo(leg);
        if (prvEquip != equip) {
            os << equip << std::endl;
            prvEquip = equip;
        }

        os << leg.departureStation << leg.schAirDepTime;

        if (leg.depDateVar) {
            os << "/" << convertDateVar(*leg.depDateVar);
        }

        os  << " ";

        os << leg.arrivalStation << leg.schAirArrTime;

        if (leg.arrDateVar) {
            os << "/" << convertDateVar(*leg.arrDateVar);
        }

        if (!leg.mealServNote.empty()) {
            if (leg.mealServNote == "XX") {
                os << " 7/XX";
            } else {
                const auto mealsList = convertMeals(getLegPRBD(leg), leg.mealServNote);
                if (!mealsList.empty()) {
                    //here should be only 1 line (5 service pairs can be stated in Record type 3)
                    os << " 7" << mealsList.front();
                }
            }
        }

        if (!isEntireOprDisclosure(record)) {
            os << resolveOprDisclosureInfo(leg);
        }

        os  << std::endl;
    }

    return os.str();
}

std::string OagSsmCreator::MakeSegmentInfo(const CarrierRecord& record)
{
    std::ostringstream os;
    std::ostringstream otherOs;

    std::vector<std::string> arrivalStations;
    for (const FlightLegRecord& leg : record.flightLegs) {
        arrivalStations.push_back(leg.arrivalStation);
    }
    if (arrivalStations.size() < 11) {
        arrivalStations.insert(arrivalStations.end(), 11 - arrivalStations.size(), std::string());
    }

    for (const FlightLegRecord& leg : record.flightLegs) {
        std::set<DepArrPoints> explicitEt;

        for (const auto& v : leg.segData) {
            const size_t deiCode = v.first;

            if (deiCode == 10 || deiCode == 50) {
                for (const SegmentData& sd : v.second) {
                    appendDupLegCrossRefs(os, deiCode, sd);
                }
                continue;
            }

            if (deiCode == 109 || deiCode == 111) {
                for (const SegmentData& sd : v.second) {
                    const std::string rbds = (deiCode == 109 ? getLegPRBD(leg) : getSegPRBD(leg, sd.dap));
                    for (const std::string& line : convertMeals(rbds, sd.data)) {
                        os << sd.dap.dep << sd.dap.arr << ' ' << deiCode << line << std::endl;
                    }
                }
                continue;
            }

            if (deiCode == 505) {
                for (const SegmentData& sd : v.second) {
                    explicitEt.emplace(sd.dap);
                    otherOs << sd.dap.dep << sd.dap.arr << " 505/" << StrUtils::trim(sd.data) << std::endl;
                }
                continue;
            }

            if (deiCode == 106) {
                //DEI 106 should be already handled during equipment info creating so ignore it here
                continue;
            }

            if ((deiCode >= 170 && deiCode <= 173) || (deiCode >= 710 && deiCode <= 799)) {
                //traffic restrictions codes - handled later
                continue;
            }

            if (deiCode == 504) {
                //Secure Flight Indicator cannot be used in Record Type 4
                continue;
            }

            //forward another DEIs as-is
            for (const SegmentData& sd : v.second) {
                os << sd.dap.dep << sd.dap.arr << " " << deiCode << "/"
                   << StrUtils::trim(sd.data) << std::endl;
            }
        }
        //---------------------------------------------------------------------
        if (explicitEt.end() == explicitEt.find(DepArrPoints(leg.departureStation, leg.arrivalStation))) {
            if (record.eTicketInfo == std::string("ET")) {
                otherOs << leg.departureStation << leg.arrivalStation << " 505/ET" << std::endl;
            }
        }
        //---------------------------------------------------------------------
        if (leg.depPassTerminal) {
            otherOs << leg.departureStation << leg.arrivalStation << " ";
            otherOs << "99/" << leg.depPassTerminal.get() << std::endl;
        }
        if (leg.arrPassTerminal) {
            otherOs << leg.departureStation << leg.arrivalStation << " ";
            otherOs << "98/" << leg.arrPassTerminal.get() << std::endl;
        }
        //---------------------------------------------------------------------
        appendTrafficRestrictions(otherOs, leg, arrivalStations);
        appendSecureFlight(otherOs, leg, record);
    }
    os << otherOs.str();
    return os.str();
}
}
