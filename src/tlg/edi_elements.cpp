#include "edi_elements.h"

#include <etick/tick_data.h>
#include <serverlib/dates.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact
{

RciElements RciElements::operator+( const RciElements &other ) const
{
    RciElements res = *this;
    res.m_lFoid.insert(res.m_lFoid.end(), other.m_lFoid.begin(), other.m_lFoid.end());
    res.m_lReclocs.insert(res.m_lReclocs.end(), other.m_lReclocs.begin(), other.m_lReclocs.end());

    return res;
}

//-----------------------------------------------------------------------------

MonElements MonElements::operator+( const MonElements &other ) const
{
    MonElements res = *this;
    res.m_lMon.insert(res.m_lMon.end(), other.m_lMon.begin(), other.m_lMon.end());

    return res;
}

//-----------------------------------------------------------------------------

TxdElements TxdElements::operator +(const TxdElements &other) const
{
    TxdElements res = *this;
    res.m_lTax.insert(res.m_lTax.end(), other.m_lTax.begin(), other.m_lTax.end());

    return res;
}

//-----------------------------------------------------------------------------

FopElements FopElements::operator +(const FopElements &other) const
{
    FopElements res = *this;
    res.m_lFop.insert(res.m_lFop.end(), other.m_lFop.begin(), other.m_lFop.end());

    return res;
}

//-----------------------------------------------------------------------------

IftElements IftElements::operator +(const IftElements &other) const
{
    IftElements res = *this;
    res.m_lIft.insert(res.m_lIft.end(), other.m_lIft.begin(), other.m_lIft.end());

    return res;
}

//-----------------------------------------------------------------------------

boost::optional<PapElem::PapDoc> PapElem::findVisa() const
{
    for(const auto& doc: m_docs) {
        if(doc.m_docQualifier == "V" || doc.m_docQualifier == "VI") {
            return doc;
        }
    }

    return boost::none;
}

boost::optional<PapElem::PapDoc> PapElem::findDoc() const
{
    for(const auto& doc: m_docs) {
        if(doc.m_docQualifier != "V" && doc.m_docQualifier != "VI") {
            return doc;
        }
    }

    return boost::none;
}

//-----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream &os, const TktElem &tkt)
{
    os << "TKT: ";
    os << "tnum: " << tkt.m_ticketNum
       << " docType: " << tkt.m_docType;

    os << " Nbooklets: ";
    if(tkt.m_nBooklets)
        os << *tkt.m_nBooklets;
    else
        os << "NO";

    os << " TickAct: ";
    if(tkt.m_tickStatAction)
        os << Ticketing::TickStatAction::TickActionStr(*tkt.m_tickStatAction);
    else
        os << "NO";

    os << " tnumConn: ";
    if(tkt.m_inConnectionTicketNum)
        os << *tkt.m_inConnectionTicketNum;
    else
        os << "NO";

    return os;
}

std::ostream& operator<<(std::ostream &os, const CpnElem& cpn)
{
    os << "CPN: ";
    os << "num: " << cpn.m_num << "; ";
    if(cpn.m_media)
        os << cpn.m_media->code() << "; ";
    else
        os << "NO; ";
    os << "amount: " << (cpn.m_amount.isValid() ? cpn.m_amount.amStr() : "NO") << "; ";
    os << "status: ";
    if(cpn.m_status)
        os << cpn.m_status;
    else
        os << "NO";
    os << "; "
       << "action request:" << (!cpn.m_action.empty() ? cpn.m_action : "NO") << "; "
       << "SAC:" << (!cpn.m_sac.empty() ? cpn.m_sac : "NO") << "; ";
    return os;
}

std::ostream& operator<<(std::ostream &os, const LorElem &lor)
{
    os << "LOR: ";
    os << "airline: " << lor.m_airline << "; ";
    os << "port: " << lor.m_port;
    return os;
}

std::ostream& operator<<(std::ostream &os, const FdqElem &fdq)
{
    os << "FDQ: ";
    os << "outb airline: " << fdq.m_outbAirl << "; ";
    os << "outb flight: " << fdq.m_outbFlNum << "; ";
    os << "outb depd: " << Dates::ddmmyyyy(fdq.m_outbDepDate) << "; ";
    os << "outb dept: " << Dates::hh24mi(fdq.m_outbDepTime) << "; ";
    os << "outb deppoint: " << fdq.m_outbDepPoint << "; ";
    os << "outb arrpoint: " << fdq.m_outbArrPoint << "\n";
    os << "inb airline: " << fdq.m_inbAirl << "; ";
    os << "inb flight: " << fdq.m_inbFlNum << "; ";
    os << "inb depd: " << Dates::ddmmyyyy(fdq.m_inbDepDate) << "; ";
    os << "inb dept: " << Dates::hh24mi(fdq.m_inbDepTime) << "; ";
    os << "inb arrd: " << Dates::ddmmyyyy(fdq.m_inbArrDate) << "; ";
    os << "inb arrt: " << Dates::hh24mi(fdq.m_inbArrTime) << "; ";
    os << "inb deppoint: " << fdq.m_inbDepPoint << "; ";
    os << "inb arrpoint: " << fdq.m_outbArrPoint;
    return os;
}

std::ostream& operator<<(std::ostream &os, const PpdElem &ppd)
{
    os << "PPD: ";
    os << "surname: " << ppd.m_passSurname << "; ";
    os << "name: " << ppd.m_passName << "; ";
    os << "type: " << ppd.m_passType << "; ";
    os << "withInft: " << ppd.m_withInftIndicator << "; ";
    os << "resp ref: " << ppd.m_passRespRef << "; ";
    os << "qry ref: " << ppd.m_passQryRef << "; ";
    os << "inft surname: " << ppd.m_inftSurname << "; ";
    os << "inft name: " << ppd.m_inftName << "; ";
    os << "inft resp ref: " << ppd.m_inftRespRef << "; ";
    os << "inft qry ref: " << ppd.m_inftQryRef;
    return os;
}

std::ostream& operator<<(std::ostream &os, const PrdElem &prd)
{
    os << "PRD: ";
    os << "rbd: " << prd.m_rbd;
    return os;
}

std::ostream& operator<<(std::ostream &os, const PsdElem &psd)
{
    os << "PSD: ";
    os << "seat: " << psd.m_seat << "; ";
    os << "nosmoking: " << psd.m_noSmokingInd << "; ";
    os << "characterstic: " << psd.m_characteristic;
    return os;
}

std::ostream& operator<<(std::ostream &os, const PbdElem &pbd)
{
    os << "PBD: ";
    if(pbd.m_bag) {
        os << "Bag: ";
        os << "num of pieces: " << pbd.m_bag->m_numOfPieces << "; ";
        os << "weight: " << pbd.m_bag->m_weight;
    }
    os << " ";
    if(pbd.m_handBag) {
        os << "Hand bag: ";
        os << "num of pieces: " << pbd.m_handBag->m_numOfPieces << "; ";
        os << "weight: " << pbd.m_handBag->m_weight;
    }
    os << "\nTags ";
    for(const auto& tag: pbd.m_tags) {
        os << "carrier code: " << tag.m_carrierCode << "; ";
        os << "tag num: " << tag.m_tagNum << "; ";
        os << "num consec: " << tag.m_qtty << "; ";
        os << "dest: " << tag.m_dest << "; ";
        os << "account code: " << tag.m_accode << "\n";
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const PsiElem &psi)
{
    os << "PSI: ";
    os << "osi: " << psi.m_osi << ";";
    os << "\nssrs:";
    for(const PsiElem::SsrDetails& ssr: psi.m_lSsr) {
        os << "\ncode: " << ssr.m_ssrCode << "; ";
        os << "airline: " << ssr.m_airline << "; ";
        os << "text: " << ssr.m_ssrText << "; ";
        if(ssr.m_age)
            os << "age: " << ssr.m_age << "; ";
        if(ssr.m_numOfPieces)
            os << "num of pieces: " << ssr.m_numOfPieces << "; ";
        if(ssr.m_weight)
            os << "weight: " << ssr.m_weight << "; ";
        os << "free text: " << ssr.m_freeText << "; ";
        os << "numeric of units qualifier: " << ssr.m_qualifier;
    }

    return os;
}

std::ostream& operator<<(std::ostream &os, const FdrElem &fdr)
{
    os << "FDR: ";
    os << "airline: " << fdr.m_airl << "; ";
    os << "flight: " << fdr.m_flNum << "; ";
    os << "depd: " << Dates::ddmmyyyy(fdr.m_depDate) << "; ";
    os << "dept: " << Dates::hh24mi(fdr.m_depTime) << "; ";
    os << "arrd: " << Dates::ddmmyyyy(fdr.m_arrDate) << "; ";
    os << "arrt: " << Dates::hh24mi(fdr.m_arrTime) << "; ";
    os << "deppoint: " << fdr.m_depPoint << "; ";
    os << "arrpoint: " << fdr.m_arrPoint;
    return os;
}

std::ostream& operator<<(std::ostream &os, const RadElem &rad)
{
    os << "RAD: ";
    os << "resp type: " << rad.m_respType << "; ";
    os << "status: " << rad.m_status;
    return os;
}

std::ostream& operator<<(std::ostream &os, const PfdElem &pfd)
{
    os << "PFD: ";
    os << "seat: " << pfd.m_seat << "; ";
    os << "nosmoking: " << pfd.m_noSmokingInd << "; ";
    os << "cabin class: " << pfd.m_cabinClass << "; ";
    os << "reg no: " << pfd.m_regNo << "; ";
    os << "infant reg no: " << pfd.m_infantRegNo;
    return os;
}

std::ostream& operator<<(std::ostream &os, const ChdElem &chd)
{
    os << "CHD: ";
    os << "airline: " << chd.m_origAirline << "; ";
    os << "point: " << chd.m_origPoint << "; ";
    os << "outb airline: " << chd.m_outbAirline << "; ";
    os << "outb flight: " << chd.m_outbFlNum << "; ";
    os << "dep date: " << Dates::ddmmyyyy(chd.m_depDate) << "; ";
    os << "dep point: " << chd.m_depPoint << "; ";
    os << "arr point: " << chd.m_arrPoint << "; ";
    os << "outb flight continuation indicator: " << chd.m_outbFlContinIndic;
    os << "host airlines: ";
    for(const std::string& hostAirline: chd.m_hostAirlines) {
        os << hostAirline << ", ";
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const FsdElem &fsd)
{
    os << "FSD: ";
    os << "boarding time: " << Dates::hh24mi(fsd.m_boardingTime) << "; ";
    os << "gate: " << fsd.m_gate;
    return os;
}

std::ostream& operator<<(std::ostream &os, const ErdElem &erd)
{
    os << "ERD: ";
    os << "level: " << erd.m_level << "; ";
    os << "message number: " << erd.m_messageNumber << "; ";
    os << "message text: " << erd.m_messageText;
    return os;
}

std::ostream& operator<<(std::ostream &os, const SpdElem &spd)
{
    os << "SPD: ";
    os << "surname: " << spd.m_passSurname << "; ";
    os << "name: " << spd.m_passName << "; ";
    os << "rbd: " << spd.m_rbd << "; ";
    os << "seat: " << spd.m_passSeat;
    os << "resp ref: " << spd.m_passRespRef << "; ";
    os << "qry ref: " << spd.m_passQryRef << "; ";
    os << "security id: " << spd.m_securityId << "; ";
    os << "recloc: " << spd.m_recloc << "; ";
    os << "ticknum: " << spd.m_tickNum;
    return os;
}

std::ostream& operator<<(std::ostream &os, const UpdElem &upd)
{
    os << "UPD: ";
    os << "action code: " << upd.m_actionCode << "; ";
    os << "surname: " << upd.m_surname << "; ";
    os << "name: " << upd.m_name << "; ";
    os << "withInft: " << upd.m_withInftIndicator << "; ";
    os << "qry ref: " << upd.m_passQryRef << "; ";
    os << "inft surname: " << upd.m_inftSurname << "; ";
    os << "inft name: " << upd.m_inftName << "; ";
    os << "inft qry ref: " << upd.m_inftQryRef;
    return os;
}

std::ostream& operator<<(std::ostream &os, const UsdElem &usd)
{
    os << "USD: ";
    os << "action code: " << usd.m_actionCode << "; ";
    os << "seat: " << usd.m_seat << "; ";
    os << "nosmoking: " << usd.m_noSmokingInd;
    return os;
}

std::ostream& operator<<(std::ostream &os, const UbdElem &ubd)
{
    os << "UBD: ";
    if(ubd.m_bag) {
        os << "Update bag ";
        os << "action code: " << ubd.m_bag->m_actionCode << "; ";
        os << "num of pieces: " << ubd.m_bag->m_numOfPieces << "; ";
        os << "weight: " << ubd.m_bag->m_weight;
    }
    os << " ";
    if(ubd.m_handBag) {
        os << "Update hand bag ";
        os << "action code: " << ubd.m_handBag->m_actionCode << "; ";
        os << "num of pieces: " << ubd.m_handBag->m_numOfPieces << "; ";
        os << "weight: " << ubd.m_handBag->m_weight;
    }
    os << "\nUpdate tag ";
    for(const auto& tag: ubd.m_tags) {
        os << "action code: " << tag.m_actionCode << "; ";
        os << "carrier code: " << tag.m_carrierCode << "; ";
        os << "tag num: " << tag.m_tagNum << "; ";
        os << "num consec: " << tag.m_qtty << "; ";
        os << "dest: " << tag.m_dest << "; ";
        os << "account code: " << tag.m_accode << "\n";
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const WadElem &wad)
{
    os << "WAD: ";
    os << "level: " << wad.m_level << "; ";
    os << "message number: " << wad.m_messageNumber << "; ";
    os << "message text: " << wad.m_messageText;
    return os;
}

std::ostream& operator<<(std::ostream &os, const SrpElem &srp)
{
    os << "SRP: ";
    os << "cabin class: " << srp.m_cabinClass << "; ";
    os << "nosmoking: " << srp.m_noSmokingInd;
    return os;
}

std::ostream& operator<<(std::ostream &os, const EqdElem &eqd)
{
    os << "EQD: ";
    os << "equipment: " << eqd.m_equipment;
    return os;
}

std::ostream& operator<<(std::ostream &os, const CbdElem &cbd)
{
    os << "CBD: ";
    os << "cabin class: " << cbd.m_cabinClass << "; ";
    os << "first class row: " << cbd.m_firstClassRow << "; last class row: " << cbd.m_lastClassRow << "; ";
    os << "deck: " << cbd.m_deck << "; ";
    os << "first smoking row: " << cbd.m_firstSmokingRow << "; last smoking row: " << cbd.m_lastSmokingRow << "; ";
    os << "seat occupation default indicator: " << cbd.m_seatOccupDefIndic << "; ";
    os << "first overwing row: " << cbd.m_firstOverwingRow << "; last overwing row: " << cbd.m_lastOverwingRow << "\n";
    os << "seat columns:\n";
    for(const CbdElem::SeatColumn& seatColumn: cbd.m_lSeatColumns) {
        os << seatColumn.m_col << ":" << seatColumn.m_desc1 << ":" << seatColumn.m_desc2 << ", ";
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const RodElem &rod)
{
    os << "ROD: ";
    os << "row: " << rod.m_row << "; ";
    os << "row áharacteristic: " << rod.m_characteristic << "; ";
    os << "\nseat occupations:";
    for(const RodElem::SeatOccupation& seatOccup: rod.m_lSeatOccupations) {
        os << "\n" << seatOccup.m_col << ":[" << seatOccup.m_occup << "] -> ";
        for(const std::string& c: seatOccup.m_lCharacteristics) {
            os << c << ",";
        }
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const PapElem &pap)
{
    os << "PAP: ";
    os << "type: " << pap.m_type << "; ";
    os << "birth date: " << pap.m_birthDate << "; ";
    os << "nationality: " << pap.m_nationality << ";\n";
    for(const auto& papDoc: pap.m_docs) {
        os << "Doc: ";
        os << "qualifier: " << papDoc.m_docQualifier << "; ";
        os << "doc number: " << papDoc.m_docNumber << "; ";
        os << "place of issue: " << papDoc.m_placeOfIssue << "; ";
        os << "free text: " << papDoc.m_freeText << "; ";
        os << "expiry date: " << papDoc.m_expiryDate << "; ";
        os << "gender: " << papDoc.m_gender << "; ";
        os << "city of issue: " << papDoc.m_cityOfIssue << "; ";
        os << "issue date: " << papDoc.m_issueDate << "; ";
        os << "surname: " << papDoc.m_surname << "; ";
        os << "name: " << papDoc.m_name << "; ";
        os << "other name: " << papDoc.m_otherName << "; ";
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const AddElem &add)
{
    os << "ADD: ";
    os << "action code: " << add.m_actionCode << "; ";
    for(const AddElem::Address& addr: add.m_lAddr) {
        os << "purpose code: " << addr.m_purposeCode << "; ";
        os << "address: " << addr.m_address << "; ";
        os << "city: " << addr.m_city << "; ";
        os << "region: " << addr.m_region << "; ";
        os << "country: " << addr.m_country << "; ";
        os << "postal code: " << addr.m_postalCode << "; ";
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const UapElem &uap)
{
    os << "UAP: ";
    os << "action code: " << uap.m_actionCode << "; ";
    os << "type: " << uap.m_type << "; ";
    os << "birth date: " << uap.m_birthDate << "; ";
    os << "nationality: " << uap.m_nationality << "; ";
    for(const auto& uapDoc: uap.m_docs) {
        os << "Doc: ";
        os << "qualifier: " << uapDoc.m_docQualifier << "; ";
        os << "doc number: " << uapDoc.m_docNumber << "; ";
        os << "place of issue: " << uapDoc.m_placeOfIssue << "; ";
        os << "free text: " << uapDoc.m_freeText << "; ";
        os << "expiry date: " << uapDoc.m_expiryDate << "; ";
        os << "gender: " << uapDoc.m_gender << "; ";
        os << "city of issue: " << uapDoc.m_cityOfIssue << "; ";
        os << "issue date: " << uapDoc.m_issueDate << "; ";
        os << "surname: " << uapDoc.m_surname << "; ";
        os << "name: " << uapDoc.m_name << "; ";
        os << "other name: " << uapDoc.m_otherName << "; ";
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const UsiElem &usi)
{
    os << "USI: ";
    os << "\nssrs:";
    for(const UsiElem::UpdSsrDetails& ssr: usi.m_lSsr) {
        os << "\naction code: " << ssr.m_actionCode << "; ";
        os << "\ncode: " << ssr.m_ssrCode << "; ";
        os << "airline: " << ssr.m_airline << "; ";
        os << "text: " << ssr.m_ssrText << "; ";
        if(ssr.m_age)
            os << "age: " << ssr.m_age << "; ";
        if(ssr.m_numOfPieces)
            os << "num of pieces: " << ssr.m_numOfPieces << "; ";
        if(ssr.m_weight)
            os << "weight: " << ssr.m_weight << "; ";
        os << "free text: " << ssr.m_freeText << "; ";
        os << "numeric of units qualifier: " << ssr.m_qualifier;
    }

    return os;
}

}//namespace edifact
