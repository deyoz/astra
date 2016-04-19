#include "edi_elements.h"

#include <etick/tick_data.h>
#include <serverlib/dates.h>

#include <boost/foreach.hpp>

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
    os << "resp ref: " << ppd.m_passRespRef << "; ";
    os << "qry ref: " << ppd.m_passQryRef;
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
    os << "num of pieces: " << pbd.m_numOfPieces << "; ";
    os << "weight: " << pbd.m_weight;
    return os;
}

std::ostream& operator<<(std::ostream &os, const PsiElem &psi)
{
    os << "PSI: ";
    os << "osi: " << psi.m_osi << ";";
    os << "\nssrs:";
    BOOST_FOREACH(const PsiElem::SsrDetails& ssr, psi.m_lSsr) {
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
    os << "security id: " << pfd.m_securityId;
    return os;
}

std::ostream& operator<<(std::ostream &os, const ChdElem &chd)
{
    os << "CHD: ";
    os << "airline: " << chd.m_origAirline << "; ";
    os << "point: " << chd.m_origPoint << "; ";
    os << "host airlines: ";
    BOOST_FOREACH(const std::string& hostAirline, chd.m_hostAirlines) {
        os << hostAirline << ", ";
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const FsdElem &fsd)
{
    os << "FSD: ";
    os << "boarding time: " << Dates::hh24mi(fsd.m_boardingTime);
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
    os << "qry ref: " << upd.m_passQryRef;
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
    os << "action code: " << ubd.m_actionCode << "; ";
    os << "num of pieces: " << ubd.m_numOfPieces << "; ";
    os << "weight: " << ubd.m_weight;
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
    BOOST_FOREACH(const CbdElem::SeatColumn& seatColumn, cbd.m_lSeatColumns) {
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
    BOOST_FOREACH(const RodElem::SeatOccupation& seatOccup, rod.m_lSeatOccupations) {
        os << "\n" << seatOccup.m_col << ":[" << seatOccup.m_occup << "] -> ";
        BOOST_FOREACH(const std::string& c, seatOccup.m_lCharacteristics) {
            os << c << ",";
        }
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const PapElem &pap)
{
    os << "PAP: ";
    os << "birth date: " << pap.m_birthDate << "; ";
    os << "nationality: " << pap.m_nationality << "; ";
    os << "doc qualifier: " << pap.m_docQualifier << "; ";
    os << "doc number: " << pap.m_docNumber << "; ";
    os << "place of issue: " << pap.m_placeOfIssue << "; ";
    os << "expiry date: " << pap.m_expiryDate << "; ";
    os << "gender: " << pap.m_gender << "; ";
    os << "surname: " << pap.m_surname << "; ";
    os << "name: " << pap.m_name << "; ";
    return os;
}

}//namespace edifact
