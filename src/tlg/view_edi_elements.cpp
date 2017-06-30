#include "view_edi_elements.h"
#include "astra_consts.h"
#include "basetables.h"
#include "iatci_help.h"
#include "convert.h"

#include <edilib/edi_func_cpp.h>
#include <edilib/edi_astra_msg_types.h>
#include <edilib/edi_sess.h>
#include <etick/tick_data.h>
#include <serverlib/str_utils.h>
#include <serverlib/dates_io.h>
#include <serverlib/dates.h>

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>


namespace edifact
{
using namespace edilib;
using namespace BASIC::date_time;


void viewUnbElement(_EDI_REAL_MES_STRUCT_* pMes, const UnbElem& elem)
{
    PushEdiPointW(pMes);
    SetEdiPointToSegmentW(pMes, SegmElement("UNB"));

    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("S002", 0));
    SetEdiDataElem(pMes, DataElement(7, 0), elem.m_senderCarrierCode);
    PopEdiPointW(pMes);

    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("S003", 0));
    SetEdiDataElem(pMes, DataElement(7, 0), elem.m_recipientCarrierCode);
    PopEdiPointW(pMes);
    
    PopEdiPointW(pMes);
}

void viewUngElement(_EDI_REAL_MES_STRUCT_* pMes, const UngElem& elem)
{
    SetEdiSegment(pMes, SegmElement("UNG"));

    PushEdiPointW(pMes);
    SetEdiPointToSegmentW(pMes, SegmElement("UNG"));
    SetEdiDataElem(pMes, DataElement(38, 0), elem.m_msgGroupName);

    SetEdiComposite(pMes, CompElement("S006", 0));
    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("S006", 0));
    SetEdiDataElem(pMes, DataElement(40, 0), elem.m_senderName);
    SetEdiDataElem(pMes, DataElement(7, 0), elem.m_senderCarrierCode);
    PopEdiPointW(pMes);

    SetEdiComposite(pMes, CompElement("S007", 0));
    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("S007", 0));
    SetEdiDataElem(pMes, DataElement(44, 0), elem.m_recipientName);
    SetEdiDataElem(pMes, DataElement(7, 0), elem.m_recipientCarrierCode);
    PopEdiPointW(pMes);

    SetEdiComposite(pMes, CompElement("S004", 0));
    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("S004", 0));
    SetEdiDataElem(pMes, DataElement(17, 0), DateTimeToStr(elem.m_prepareDateTime, "yymmdd"));
    SetEdiDataElem(pMes, DataElement(19, 0), DateTimeToStr(elem.m_prepareDateTime, "hhnn"));
    PopEdiPointW(pMes);

    SetEdiDataElem(pMes, DataElement(48, 0), elem.m_groupRefNum);
    SetEdiDataElem(pMes, DataElement(51, 0), elem.m_cntrlAgnCode);
    
    SetEdiComposite(pMes, CompElement("S008", 0));
    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("S008", 0));
    SetEdiDataElem(pMes, DataElement(52, 0), elem.m_msgVerNum);
    SetEdiDataElem(pMes, DataElement(54, 0), elem.m_msgRelNum);
    PopEdiPointW(pMes);

    PopEdiPointW(pMes);
}

void viewUnhElement(_EDI_REAL_MES_STRUCT_* pMes, const UnhElem& elem)
{
    PushEdiPointW(pMes);
    SetEdiPointToSegmentW(pMes, SegmElement("UNH"));

    SetEdiComposite(pMes, CompElement("S009", 0));
    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("S009", 0));
    SetEdiDataElem(pMes, DataElement(65, 0), elem.m_msgTypeId);
    SetEdiDataElem(pMes, DataElement(52, 0), elem.m_msgVerNum);
    SetEdiDataElem(pMes, DataElement(54, 0), elem.m_msgRelNum);
    SetEdiDataElem(pMes, DataElement(51, 0), elem.m_cntrlAgnCode);
    SetEdiDataElem(pMes, DataElement(57, 0), elem.m_assAccCode);
    PopEdiPointW(pMes);
            
    SetEdiComposite(pMes, CompElement("S010", 0));
    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("S010", 0));
    std::string seqNum = boost::lexical_cast< std::string >(elem.m_seqNumber);
    SetEdiDataElem(pMes, DataElement(70, 0), StrUtils::LPad(seqNum, 2, '0'));
    SetEdiDataElem(pMes, DataElement(73, 0), UnhElem::seqFlagToStr(elem.m_seqFlag));
    PopEdiPointW(pMes);
        
    PopEdiPointW(pMes);
}

void viewUneElement(_EDI_REAL_MES_STRUCT_* pMes, const UneElem& elem)
{
    std::ostringstream une;
    une << elem.m_cntrlCnt << "+" << elem.m_refNum;
    SetEdiFullSegment(pMes, SegmElement("UNE"), une.str());
}

void viewBgmElement(_EDI_REAL_MES_STRUCT_* pMes, const BgmElem& elem)
{
    std::ostringstream bgm;
    bgm << elem.m_docCode;
    if(!elem.m_docId.empty())
      bgm << "+" << elem.m_docId;
    SetEdiFullSegment(pMes, SegmElement("BGM"), bgm.str());
}

void viewNadElement(_EDI_REAL_MES_STRUCT_* pMes, const NadElem& elem, int num)
{
    std::ostringstream nad;
    nad << elem.m_funcCode << "+++" << elem.m_partyName;
    if(elem.m_funcCode!="MS")
    {
      nad << ":" << elem.m_partyName2;
      if(!elem.m_partyName3.empty())
        nad << ":" << elem.m_partyName3;
      if(!elem.m_street.empty() ||
          !elem.m_city.empty() ||
          !elem.m_countrySubEntityCode.empty() ||
          !elem.m_postalCode.empty() ||
          !elem.m_country.empty())
      {
        nad << "+" << elem.m_street
            << "+" << elem.m_city
            << "+" << elem.m_countrySubEntityCode
            << "+" << elem.m_postalCode
            << "+" << elem.m_country;
      };
    };
    SetEdiFullSegment(pMes, SegmElement("NAD", num), nad.str());
}

void viewComElement(_EDI_REAL_MES_STRUCT_* pMes, const ComElem& elem, int num)
{
    std::ostringstream com;
    if(!elem.m_phone.empty())
        com << elem.m_phone << ":" << "TE" << "+";
    if(!elem.m_fax.empty())
        com << elem.m_fax << ":" << "FX" << "+";
    if(!elem.m_email.empty())
        com << elem.m_email << ":" << "EM" << "+";
    SetEdiFullSegment(pMes, SegmElement("COM", num), com.str());
}

void viewTdtElement(_EDI_REAL_MES_STRUCT_* pMes, const TdtElem& elem, int num)
{
    std::ostringstream tdt;
    tdt << elem.m_stageQuailifier << "+" << elem.m_journeyId
        << "+++" << elem.m_carrierId;
    SetEdiFullSegment(pMes, SegmElement("TDT", num), tdt.str());
}

void viewLocElement(_EDI_REAL_MES_STRUCT_* pMes, const LocElem& elem, int num)
{
    std::ostringstream loc;
    loc << boost::lexical_cast< std::string >(elem.m_locQualifier) << "+" << elem.m_locName;
    if(!elem.m_relatedLocName1.empty()||
        !elem.m_relatedLocName2.empty())
    {
      loc << "+";
      if(!elem.m_relatedLocName1.empty())
        loc << ":::" << elem.m_relatedLocName1;
      loc << "+";
      if(!elem.m_relatedLocName2.empty())
        loc << ":::" << elem.m_relatedLocName2;
    };

    SetEdiFullSegment(pMes, SegmElement("LOC", num), loc.str());
}

void viewDtmElement(_EDI_REAL_MES_STRUCT_* pMes, const DtmElem& elem, int num)
{
    std::ostringstream dtm;
    dtm << boost::lexical_cast< std::string >(elem.m_dtmQualifier) << ":";
    std::string format = (elem.m_formatCode == "201" ? "yymmddhhnn" : "yymmdd");
    dtm << (elem.m_dateTime != ASTRA::NoExists ? DateTimeToStr(elem.m_dateTime, format) : "");
    dtm << ":" << elem.m_formatCode;
    SetEdiFullSegment(pMes, SegmElement("DTM", num), dtm.str());
}

void viewAttElement(_EDI_REAL_MES_STRUCT_* pMes, const AttElem& elem, int num)
{
    std::ostringstream att;
    att << elem.m_funcCode << "++" << elem.m_value;
    SetEdiFullSegment(pMes, SegmElement("ATT", num), att.str());
}

void viewMeaElement(_EDI_REAL_MES_STRUCT_* pMes, const MeaElem& elem, int num)
{
    std::ostringstream mea;
    mea << MeaElem::meaQualifierToStr(elem.m_meaQualifier) << "++";
    if (elem.m_meaQualifier == MeaElem::BagWeight)
      mea << "KGM";
    mea << ":" << elem.m_mea;
    SetEdiFullSegment(pMes, SegmElement("MEA", num), mea.str());
}

void viewNatElement(_EDI_REAL_MES_STRUCT_* pMes, const NatElem& elem, int num)
{
    std::ostringstream nat;
    nat << elem.m_natQualifier << "+" << elem.m_nat;
    SetEdiFullSegment(pMes, SegmElement("NAT", num), nat.str());
}

void viewRffElement(_EDI_REAL_MES_STRUCT_* pMes, const RffElem& elem, int num)
{
    std::ostringstream rff;
    rff << elem.m_rffQualifier << ":" << elem.m_ref;
    SetEdiFullSegment(pMes, SegmElement("RFF", num), rff.str());
}

void viewDocElement(_EDI_REAL_MES_STRUCT_* pMes, const DocElem& elem, int num)
{
    std::ostringstream doc;
    doc << elem.m_docCode << ":" << elem.m_idCode << ":" << elem.m_respAgnCode << "+";
    doc << elem.m_docNum;
    SetEdiFullSegment(pMes, SegmElement("DOC", num), doc.str());
}

void viewCntElement(_EDI_REAL_MES_STRUCT_* pMes, const CntElem& elem, int num)
{
    std::ostringstream cnt;
    cnt << boost::lexical_cast< std::string >(elem.m_cntType) << ":";
    cnt << elem.m_cnt;
    SetEdiFullSegment(pMes, SegmElement("CNT", num), cnt.str());
}

void viewTvlElement(_EDI_REAL_MES_STRUCT_* pMes, const TvlElem& elem)
{
    std::ostringstream tvl;
    if(!elem.m_depDate.is_special())
        tvl << HelpCpp::string_cast(elem.m_depDate, "%d%m%y");
    tvl << ":";
    if(!elem.m_depTime.is_special())
        tvl << HelpCpp::string_cast(elem.m_depTime, "%H%M");
    tvl << ":";
    if(!elem.m_arrDate.is_special())
        tvl << HelpCpp::string_cast(elem.m_arrDate, "%d%m%y");
    tvl << ":";
    if(!elem.m_arrTime.is_special())
        tvl << HelpCpp::string_cast(elem.m_arrTime, "%H%M");
    
    tvl << "+" << elem.m_depPoint << "+" << elem.m_arrPoint;
    tvl << "+" << elem.m_airline;
    if(elem.m_flNum.valid())
        tvl << "+" << elem.m_flNum;
    
    SetEdiFullSegment(pMes, SegmElement("TVL"), tvl.str());
}

void viewItin(EDI_REAL_MES_STRUCT *pMes, const Ticketing::Itin &itin, int num)
{
    std::ostringstream tvl;

    tvl << (itin.date1().is_special()?"":
            HelpCpp::string_cast(itin.date1(), "%d%m%y"))
            << ":" << (itin.time1().is_special()?"":
            HelpCpp::string_cast(itin.time1(), "%H%M")) << "+" <<
            itin.depPointCode() << "+" <<
            itin.arrPointCode() << "+" <<
            itin.airCode();
    if(!itin.airCodeOper().empty()) {
        tvl << ":" << itin.airCodeOper();
    }
    tvl << "+";
    if(itin.flightnum()) {
        tvl << itin.flightnum();
    } else {
        tvl << Ticketing::ItinStatus::Open;
    }
    tvl << ":" << itin.classCodeStr();
    if (num) {
        tvl << "++" << num;
    }

    SetEdiFullSegment(pMes, SegmElement("TVL"), tvl.str());
}

void viewTktElement(_EDI_REAL_MES_STRUCT_* pMes, const TktElem& elem)
{
    std::ostringstream tkt;
    tkt << elem.m_ticketNum.get() << ":";
    if(elem.m_docType)
        tkt << elem.m_docType->code();
    tkt << ":";
    if(elem.m_conjunctionNum)
        tkt << elem.m_conjunctionNum.get();
    tkt << ":";
    if(elem.m_tickStatAction)
        tkt << Ticketing::TickStatAction::TickActionStr(elem.m_tickStatAction.get());
    tkt << "::";
    if(elem.m_inConnectionTicketNum)
        tkt << elem.m_inConnectionTicketNum.get();

    SetEdiFullSegment(pMes, SegmElement("TKT"), tkt.str());
}

void viewCpnElement(_EDI_REAL_MES_STRUCT_* pMes, const CpnElem& elem)
{
    std::ostringstream cpn;
    cpn << elem.m_num << ":";
    if(elem.m_status)
        cpn << elem.m_status->code();
    cpn << ":";
    if(elem.m_amount.isValid())
        cpn << elem.m_amount.amStr();
    cpn << ":";
    if(elem.m_media)
        cpn << elem.m_media->code();

    cpn << ":" << elem.m_sac << ":::";
    if(elem.m_connectedNum)
        cpn << elem.m_connectedNum;
    cpn << "::";
    if(!elem.m_action.empty())
        cpn << elem.m_action;

    SetEdiFullSegment(pMes, SegmElement("CPN"), cpn.str());
}

void viewEqnElement(_EDI_REAL_MES_STRUCT_* pMes, const EqnElem& elem)
{
    std::ostringstream eqn;
    eqn << elem.m_numberOfUnits << ":";
    eqn << elem.m_qualifier;
    SetEdiFullSegment(pMes, SegmElement("EQN"), eqn.str());
}

void viewEqnElement(_EDI_REAL_MES_STRUCT_* pMes, const std::list<EqnElem>& lElem)
{
    if(lElem.empty())
        return;

    std::ostringstream eqn;
    BOOST_FOREACH(const EqnElem& elem, lElem)
    {
        eqn << elem.m_numberOfUnits << ":";
        eqn << elem.m_qualifier << "+";
    }
    SetEdiFullSegment(pMes, SegmElement("EQN"), eqn.str());
}

void viewOrgElement(_EDI_REAL_MES_STRUCT_* pMes, const Ticketing::OrigOfRequest& elem)
{
    std::ostringstream org;
    org << elem.airlineCode() << ":" + elem.locationCode() << "+";
    org << elem.pprNumber() << "+++";
    org << elem.type() << "+::";
    org << elem.langStr()<< "+" << elem.pult();
    SetEdiFullSegment(pMes, SegmElement("ORG"), org.str());
}


//-------------------------------------IACTI---------------------------------------------

void viewLorElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::OriginatorDetails& origin)
{
    std::ostringstream lor;
    if(!origin.airline().empty()) {
        lor << BaseTables::Company(origin.airline())->code(/*lang*/);
    }
    lor << ":";
    if(!origin.port().empty()) {
        lor << BaseTables::Port(origin.port())->code(/*lang*/);
    }
    SetEdiFullSegment(pMes, SegmElement("LOR"), lor.str());
}

static std::string fullDateTimeString(const boost::gregorian::date& d,
                                      const boost::posix_time::time_duration& t)
{
    std::ostringstream str;
    str << Dates::rrmmdd(d);
//    if(!t.is_not_a_date_time())
//        str << Dates::hh24mi(t, false /*delimeter*/);
    return str.str();
}

void viewFdqElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::FlightDetails& nextFlight,
                    boost::optional<iatci::FlightDetails> currFlight)
{
    SetEdiSegment(pMes, SegmElement("FDQ"));
    PushEdiPointW(pMes);
    SetEdiPointToSegmentW(pMes, SegmElement("FDQ"));

    SetEdiComposite(pMes, CompElement("C013", 0));
    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("C013", 0));
    SetEdiDataElem(pMes, DataElement(3127, 0), BaseTables::Company(nextFlight.airline())->code(/*lang*/));
    PopEdiPointW(pMes);

    SetEdiComposite(pMes, CompElement("C014", 0));
    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("C014", 0));
    SetEdiDataElem(pMes, DataElement(3802, 0), HelpCpp::string_cast(nextFlight.flightNum()));
    PopEdiPointW(pMes);

    SetEdiDataElem(pMes, DataElement(2281, 0), fullDateTimeString(nextFlight.depDate(), nextFlight.depTime()));
    SetEdiDataElem(pMes, DataElement(3215, 0), BaseTables::Port(nextFlight.depPort())->code(/*lang*/));
    SetEdiDataElem(pMes, DataElement(3259, 0), BaseTables::Port(nextFlight.arrPort())->code(/*lang*/));

    if(currFlight)
    {
        SetEdiComposite(pMes, CompElement("C015", 0));
        PushEdiPointW(pMes);
        SetEdiPointToCompositeW(pMes, CompElement("C015", 0));
        SetEdiDataElem(pMes, DataElement(3127, 0), BaseTables::Company(currFlight->airline())->code(/*lang*/));
        PopEdiPointW(pMes);

        SetEdiComposite(pMes, CompElement("C016", 0));
        PushEdiPointW(pMes);
        SetEdiPointToCompositeW(pMes, CompElement("C016", 0));
        SetEdiDataElem(pMes, DataElement(3802, 0), HelpCpp::string_cast(currFlight->flightNum()));
        PopEdiPointW(pMes);

        SetEdiDataElem(pMes, DataElement(2281, 0, 1), fullDateTimeString(currFlight->depDate(), currFlight->depTime()));
        SetEdiDataElem(pMes, DataElement(2107, 0),    fullDateTimeString(currFlight->arrDate(), currFlight->arrTime()));
        SetEdiDataElem(pMes, DataElement(3215, 0, 1), BaseTables::Port(currFlight->depPort())->code(/*lang*/));
        SetEdiDataElem(pMes, DataElement(3259, 0, 1), BaseTables::Port(currFlight->arrPort())->code(/*lang*/));
    }

    PopEdiPointW(pMes);
}

void viewFdrElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::FlightDetails& flight,
                    const std::string& fcIndicator)
{
    SetEdiSegment(pMes, SegmElement("FDR"));
    PushEdiPointW(pMes);
    SetEdiPointToSegmentW(pMes, SegmElement("FDR"));

    SetEdiComposite(pMes, CompElement("C013", 0));
    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("C013", 0));
    SetEdiDataElem(pMes, DataElement(3127, 0), BaseTables::Company(flight.airline())->code(/*lang*/));
    PopEdiPointW(pMes);

    SetEdiComposite(pMes, CompElement("C014", 0));
    PushEdiPointW(pMes);
    SetEdiPointToCompositeW(pMes, CompElement("C014", 0));
    SetEdiDataElem(pMes, DataElement(3802, 0), HelpCpp::string_cast(flight.flightNum()));
    PopEdiPointW(pMes);

    SetEdiDataElem(pMes, DataElement(2281, 0), fullDateTimeString(flight.depDate(), flight.depTime()));
    SetEdiDataElem(pMes, DataElement(3215, 0), BaseTables::Port(flight.depPort())->code(/*lang*/));
    SetEdiDataElem(pMes, DataElement(3259, 0), BaseTables::Port(flight.arrPort())->code(/*lang*/));
    SetEdiDataElem(pMes, DataElement(2107, 0), fullDateTimeString(flight.arrDate(), flight.arrTime()));
    SetEdiDataElem(pMes, DataElement(9856, 0), fcIndicator);

    PopEdiPointW(pMes);
}

void viewPpdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::PaxDetails& pax)
{
    std::ostringstream ppd;
    ppd << pax.surname()
        << "+" << pax.typeAsString() << ":" << pax.withInftIndicatorAsString()
        << "+" << pax.respRef() << "+" << pax.name() << "++" << pax.qryRef();
    SetEdiFullSegment(pMes, SegmElement("PPD"), ppd.str());
}

void viewPpdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::PaxDetails& pax,
                    const iatci::PaxDetails& infant)
{
    std::ostringstream ppd;
    ppd << pax.surname()
        << "+" << pax.typeAsString() << ":" << pax.withInftIndicatorAsString()
        << "+";
    if(!pax.respRef().empty())
        ppd <<  pax.respRef() << ":" << infant.respRef();
    ppd << "+" << pax.name()
        << "+" << infant.surname() << ":" << infant.name()
        << "+";
    if(!pax.qryRef().empty())
        ppd << pax.qryRef() << ":" << infant.qryRef();
    SetEdiFullSegment(pMes, SegmElement("PPD"), ppd.str());
}

void viewSpdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::SelectPersonalDetails& pax)
{
    std::ostringstream spd;
    spd << pax.surname() << ":" << pax.name() << ":" << pax.rbd() << ":1"
        << "+" << iatci::normSeatNum(pax.seat())
        << "+" << pax.respRef()
        << "+" << pax.qryRef()
        << "+" << pax.securityId()
        << "+" << pax.recloc()
        << "++++" << pax.tickNum();
    SetEdiFullSegment(pMes, SegmElement("SPD"), spd.str());
}

void viewPsiElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::ServiceDetails& service)
{
    std::ostringstream psi;
    psi << service.osi();
    BOOST_FOREACH(const iatci::ServiceDetails::SsrInfo& ssr, service.lSsr())
    {
        psi << "+" << ssr.ssrCode() << ":";
        if(!ssr.airline().empty()) {
            psi << BaseTables::Company(ssr.airline())->code(/*lang*/);
        }
        psi << ":";
        if(ssr.isInfantTicket())
            psi << "INF";
        psi << ssr.ssrText() << "::";
        if(ssr.quantity())
            psi << ssr.quantity();
        psi << "::" << ssr.freeText();

    }

    if(!psi.str().empty())
        SetEdiFullSegment(pMes, SegmElement("PSI"), psi.str());
}

void viewPrdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::ReservationDetails& reserv)
{
    std::ostringstream prd;
    prd << reserv.rbd();
    SetEdiFullSegment(pMes, SegmElement("PRD"), prd.str());
}

void viewPsdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::SeatDetails& seat)
{
    std::ostringstream psd;
    psd << seat.smokeIndAsString();
    BOOST_FOREACH(const std::string& seatCharactesistic, seat.characteristics()) {
        psd << ":" << seatCharactesistic;
    }
    psd << "+" << iatci::normSeatNum(seat.seat());

    SetEdiFullSegment(pMes, SegmElement("PSD"), psd.str());
}

void viewPbdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::BaggageDetails& baggage)
{
    std::ostringstream pbd;
    if(baggage.bag()) {
        pbd << baggage.bag()->numOfPieces();
        if(baggage.bag()->numOfPieces()) {
            pbd << ":" << baggage.bag()->weight();
        }
    } else {
        pbd << "0";
    }
    pbd << "+";
    if(baggage.handBag()) {
        pbd << baggage.handBag()->numOfPieces();
        if(baggage.handBag()->numOfPieces()) {
            pbd << ":" << baggage.handBag()->weight();
        }
    }
    SetEdiFullSegment(pMes, SegmElement("PBD"), pbd.str());
}

void viewChdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::CascadeHostDetails& cascadeDetails)
{
    std::ostringstream chd;
    if(!cascadeDetails.originAirline().empty()) {
        chd << BaseTables::Company(cascadeDetails.originAirline())->code(/*lang*/);
    }
    chd << ":";
    if(!cascadeDetails.originPort().empty()) {
        chd << BaseTables::Port(cascadeDetails.originPort())->code(/*lang*/);
    }

    chd << "++++++++";
    BOOST_FOREACH(const std::string& hostAirline, cascadeDetails.hostAirlines()) {
        chd << "H::" << BaseTables::Company(hostAirline)->code(/*lang*/) << "+";
    }
    SetEdiFullSegment(pMes, SegmElement("CHD"), chd.str());
}

void viewRadElement(_EDI_REAL_MES_STRUCT_* pMes, const std::string& respType,
                                                 const std::string& respStatus)
{
   std::ostringstream rad;
   rad << respType << "+" << respStatus;
   SetEdiFullSegment(pMes, SegmElement("RAD"), rad.str());
}

void viewFsdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::FlightDetails& flight)
{
    FsdElem fsd(flight.boardingTime());
    viewFsdElement(pMes, fsd);
}

void viewFsdElement(_EDI_REAL_MES_STRUCT_* pMes, const FsdElem& elem)
{
    if(!elem.m_boardingTime.is_not_a_date_time())
    {
        std::ostringstream fsd;
        fsd << Dates::hh24mi(elem.m_boardingTime, false);
        SetEdiFullSegment(pMes, SegmElement("FSD"), fsd.str());
    }
}

void viewPfdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::FlightSeatDetails& seat,
                    const boost::optional<iatci::FlightSeatDetails>& infantSeat)
{
    PfdElem pfd;
    pfd.m_seat         = seat.seat();
    pfd.m_noSmokingInd = seat.smokeIndAsString();
    pfd.m_cabinClass   = seat.cabinClass();
    pfd.m_regNo        = seat.regNo();
    if(infantSeat) {
        pfd.m_infantRegNo = infantSeat->regNo();
    }
    viewPfdElement(pMes, pfd);
}

void viewPfdElement(_EDI_REAL_MES_STRUCT_* pMes, const PfdElem& elem)
{
    std::ostringstream pfd;
    pfd << iatci::normSeatNum(elem.m_seat);
    pfd << "+" << elem.m_noSmokingInd << ":";
    if(!elem.m_cabinClass.empty()) {
        Ticketing::BaseClass cabin(elem.m_cabinClass);
        pfd << cabin->code(ENGLISH) << ":"
            << cabin->codeInt() + 1;
    } else {
        pfd << ":";
    }

    pfd << ":::::" << "Y" << ":" << elem.m_noSmokingInd;


    if(!elem.m_regNo.empty()) {
        pfd << "+" << std::setfill('0') << std::setw(3) << elem.m_regNo;
        if(!elem.m_infantRegNo.empty()) {
           pfd << ":" << std::setfill('0') << std::setw(3) << elem.m_infantRegNo;
        }
    }
    pfd << "+" << "Y";
    SetEdiFullSegment(pMes, SegmElement("PFD"), pfd.str());
}

void viewErdElement(_EDI_REAL_MES_STRUCT_* pMes, const std::string& errLevel,
                                                 const std::string& errCode,
                                                 const std::string& errText)
{
    std::ostringstream erd;
    erd << errLevel << ":" << errCode << ":" << errText;
    SetEdiFullSegment(pMes, SegmElement("ERD"), erd.str());
}

void viewUpdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::UpdatePaxDetails &updPax)
{
    std::ostringstream upd;
    upd << updPax.actionCodeAsString() << "+";
    upd << updPax.surname() << "++" << updPax.name();
    SetEdiFullSegment(pMes, SegmElement("UPD"), upd.str());
}

void viewUsdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::UpdateSeatDetails& updSeat)
{
    std::ostringstream usd;
    usd << updSeat.smokeIndAsString();
    BOOST_FOREACH(const std::string& seatCharactesistic, updSeat.characteristics()) {
        usd << ":" << seatCharactesistic;
    }
    usd << "+" << iatci::normSeatNum(updSeat.seat());
    //usd << "++++" << updSeat.actionCodeAsString(); TODO

    SetEdiFullSegment(pMes, SegmElement("USD"), usd.str());
}

void viewUbdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::UpdateBaggageDetails& updBaggage)
{
    std::ostringstream ubd;
    if(updBaggage.bag()) {
        ubd << updBaggage.actionCodeAsString() << ":";
        ubd << updBaggage.bag()->numOfPieces();
        if(updBaggage.bag()->numOfPieces()) {
            ubd << ":" << updBaggage.bag()->weight();
        }
    }
    ubd << "+";

    if(updBaggage.handBag()) {
        ubd << updBaggage.actionCodeAsString() << ":";
        ubd << updBaggage.handBag()->numOfPieces();
        if(updBaggage.handBag()->numOfPieces()) {
            ubd << ":" << updBaggage.handBag()->weight();
        }
    }
    ubd << "+";

    ubd<< updBaggage.actionCodeAsString() << ":" << "NP";

    SetEdiFullSegment(pMes, SegmElement("UBD"), ubd.str());
}

void viewUapElement(_EDI_REAL_MES_STRUCT_* pMes,
                    const iatci::UpdateDocDetails& updDoc, const iatci::PaxDetails& pax)
{
    std::ostringstream uap;
    uap << updDoc.actionCodeAsString() << "+";
    uap << (pax.type() == iatci::PaxDetails::Infant ? "IN" : "A") << ":"
        << pax.surname() << ":"
        << pax.name() << ":"
        << Dates::rrmmdd(updDoc.birthDate())
        << ":::" << updDoc.nationality() << "++";
    uap << updDoc.docType() << ":"
        << updDoc.no() << ":"
        << updDoc.issueCountry() << ":::"
        << Dates::rrmmdd(updDoc.expiryDate()) << ":"
        << updDoc.gender() << "::::::"
        << updDoc.surname() << ":"
        << updDoc.name() << ":"
        << updDoc.secondName();
    SetEdiFullSegment(pMes, SegmElement("UAP"), uap.str());
}

void viewUsiElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::UpdateServiceDetails& updService)
{
    std::ostringstream usi;
    BOOST_FOREACH(const iatci::UpdateServiceDetails::UpdSsrInfo& ssr, updService.lSsr())
    {
        usi << "+" << ssr.actionCodeAsString() << ":"
                   << ssr.ssrCode() << ":";
        if(!ssr.airline().empty()) {
            usi << BaseTables::Company(ssr.airline())->code(/*lang*/);
        }
        usi << ":" << ssr.ssrText() << "::";
        if(ssr.quantity())
            usi << ssr.quantity();
        usi << "::" << ssr.freeText();
        if(ssr.isInfantTicket())
            usi << ":INF";
    }
    SetEdiFullSegment(pMes, SegmElement("USI"), usi.str());
}

void viewSrpElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::SeatRequestDetails& seatReqDetails)
{
    std::ostringstream srp;
    srp << seatReqDetails.cabinClass() << ":";
    srp << seatReqDetails.smokeIndAsString();
    SetEdiFullSegment(pMes, SegmElement("SRP"), srp.str());
}

void viewCbdElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::CabinDetails& cabinDetails, int num)
{
    std::ostringstream cbd;
    cbd << Ticketing::BaseClass(cabinDetails.classDesignator())->code(ENGLISH) << "+";
    cbd << std::setfill('0') << std::setw(3) << cabinDetails.rowRange().firstRow() << ":"
        << std::setfill('0') << std::setw(3) << cabinDetails.rowRange().lastRow() << "+";
    cbd << cabinDetails.deck() << "+";
    if(cabinDetails.smokingArea()) {
        cbd << std::setfill('0') << std::setw(3) << cabinDetails.smokingArea()->firstRow() << ":"
            << std::setfill('0') << std::setw(3) << cabinDetails.smokingArea()->lastRow();
    }
    cbd << "+";
    cbd << cabinDetails.defaultSeatOccupation() << "+";
    if(cabinDetails.overwingArea()) {
        cbd << std::setfill('0') << std::setw(3) << cabinDetails.overwingArea()->firstRow() << ":"
            << std::setfill('0') << std::setw(3) << cabinDetails.overwingArea()->lastRow();
    }
    cbd << "+";
    BOOST_FOREACH(const iatci::SeatColumnDetails& seatColumn, cabinDetails.seatColumns()) {
        cbd << iatci::normSeatLetter(seatColumn.column()) << ":"
            << seatColumn.desc1() << ":"
            << seatColumn.desc2() << "+";
    }
    SetEdiFullSegment(pMes, SegmElement("CBD", num), cbd.str());
}

void viewRodElement(_EDI_REAL_MES_STRUCT_* pMes, const iatci::RowDetails& rowDetails, int num)
{
    std::ostringstream rod;
    rod << rowDetails.row() << "+";
    rod << rowDetails.characteristic() << "+";
    BOOST_FOREACH(const iatci::SeatOccupationDetails& seatOccup, rowDetails.lOccupationDetails()) {
        rod << iatci::normSeatLetter(seatOccup.column())
            << ":" << seatOccup.occupation();
        BOOST_FOREACH(const std::string& seatCharstc, seatOccup.lCharacteristics()) {
            rod << ":" << seatCharstc;
        }
        rod << "+";
    }
    SetEdiFullSegment(pMes, SegmElement("ROD", num), rod.str());
}

void viewPapElement(_EDI_REAL_MES_STRUCT_* pMes,
                    const iatci::DocDetails& doc, const iatci::PaxDetails& pax)
{
    std::ostringstream pap;
    pap << (pax.type() == iatci::PaxDetails::Infant ? "IN" : "A") << ":"
        << pax.surname() << ":"
        << pax.name() << ":"
        << Dates::rrmmdd(doc.birthDate())
        << ":::" << doc.nationality() << "++";
    pap << doc.docType() << ":"
        << doc.no() << ":"
        << doc.issueCountry() << ":::"
        << Dates::rrmmdd(doc.expiryDate()) << ":"
        << doc.gender() << "::::::"
        << doc.surname() << ":"
        << doc.name() << ":"
        << doc.secondName();

    SetEdiFullSegment(pMes, SegmElement("PAP"), pap.str());
}

void viewPapElement(_EDI_REAL_MES_STRUCT_* pMes, bool infant)
{
    SetEdiFullSegment(pMes, SegmElement("PAP"), infant ? "IN" : "");
}

void viewAddElement(_EDI_REAL_MES_STRUCT_* pMes,
                    const iatci::AddressDetails& addr)
{
    std::ostringstream add;
    add << "+";
    for(const auto& addrInfo: addr.lAddr()) {
        add << addrInfo.type() << ":"
            << addrInfo.address() << ":"
            << addrInfo.city() << ":"
            << ":"
            << addrInfo.region() << ":"
            << addrInfo.country() << ":"
            << addrInfo.postalCode()
            << "+";
    }

    SetEdiFullSegment(pMes, SegmElement("ADD"), add.str());
}

}//namespace edifact
