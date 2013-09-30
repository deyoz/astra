//
// C++ Implementation: apis_edi_file
//
// Description:
//
//
// Author: anton <anton@whale>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "apis_edi_file.h"
#include "exceptions.h"
#include "view_edi_elements.h"
#include "config.h"
#include "tlg/tlg.h"

#include <edilib/edi_func_cpp.h>
#include <edilib/edi_astra_msg_types.h>
#include <edilib/edi_sess.h>

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include <time.h>
#include <sstream>

#define NICKNAME "ANTON"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>


namespace Paxlst
{
using namespace edilib;
using namespace ASTRA::edifact;


static const char* UnhNumber = "1";
static const char* VerNum = "D";
static const char* RelNum = "02B";
static const char* CntrlAgn = "UN";
static const char* Chset = "UNOA";
static const char* Apis = "APIS";
static const int   SyntaxVer = 4;


std::string createIataCode( const std::string& flight,
                            const BASIC::TDateTime& destDateTime )
{
    std::ostringstream iata;
    iata << flight;
    iata << BASIC::DateTimeToStr( destDateTime, "/yymmdd/hhnn" );
    return iata.str();
}

std::string createEdiPaxlstFileName( const std::string& carrierCode,
                                     const int& flightNumber,
                                     const std::string& flightSuffix,
                                     const std::string& origin,
                                     const std::string& destination,
                                     const BASIC::TDateTime& departureDate,
                                     const std::string& ext,
                                     unsigned partNum )
{
    ostringstream f;
    f << carrierCode << flightNumber << flightSuffix;

    std::ostringstream fname;
    fname << carrierCode
          << (f.str().size()<6?string(6-f.str().size(),'0'):"") << flightNumber
          << flightSuffix
          << origin << destination;
    fname << BASIC::DateTimeToStr( departureDate, "yyyymmdd" );
    fname << "." << ext;
    if( partNum )
        fname << ".PART" << partNum;
    return fname.str();
}

static std::string createEdiInterchangeReference()
{
    std::ostringstream ref;
    ref << time( NULL );
    return ref.str();
}

static UnhElem::SeqFlag getSeqFlag( unsigned partNum, unsigned partsCnt )
{
    UnhElem::SeqFlag seqFlag = UnhElem::Middle;
    if( partNum == 1 )
        seqFlag = UnhElem::First;
    if( partNum == partsCnt )
        seqFlag = UnhElem::Last;
    return seqFlag;
}

static void collectPaxlstMessage( _EDI_REAL_MES_STRUCT_* pMes, 
                                  const PaxlstInfo& paxlst,
                                  const BASIC::TDateTime& nowUtc,
                                  unsigned partNum,
                                  unsigned partsCnt,
                                  unsigned totalCnt )
{
    ResetEdiPointW( pMes );

    // UNB
    viewUnbElement( pMes, UnbElem( paxlst.senderCarrierCode(), 
                                   paxlst.recipientCarrierCode() ) );
    
    // UNG
    viewUngElement( pMes, UngElem( "PAXLST",
                                   paxlst.senderName(),
                                   paxlst.senderCarrierCode(),
                                   paxlst.recipientName(),
                                   paxlst.recipientCarrierCode(),
                                   nowUtc,
                                   UnhNumber,
                                   CntrlAgn,
                                   VerNum,
                                   RelNum ) );
    
    // UNH
    viewUnhElement( pMes, UnhElem( "PAXLST",
                                   VerNum,
                                   RelNum,
                                   CntrlAgn,
                                   "IATA",
                                   partNum,
                                   getSeqFlag( partNum, partsCnt ) ) ) ;
    
    // BGM
    viewBgmElement( pMes, BgmElem( "745" ) );

    if( !paxlst.partyName().empty() )
    {
        SetEdiSegGr( pMes, SegGrElement( 1, 0 ) );

        PushEdiPointW( pMes );
        SetEdiPointToSegGrW( pMes, SegGrElement( 1, 0 ) );

        // NAD
        viewNadElement( pMes, NadElem( "MS", paxlst.partyName() ) );

        if( !paxlst.phone().empty() || !paxlst.fax().empty() || !paxlst.email().empty() )
        {
            // COM
            viewComElement( pMes, ComElem( paxlst.phone(), 
                                           paxlst.fax(), 
                                           paxlst.email() ) );
        }

        PopEdiPointW( pMes );
    }

    SetEdiSegGr( pMes, SegGrElement( 2, 0 ) );
    PushEdiPointW( pMes );
    SetEdiPointToSegGrW( pMes, SegGrElement( 2, 0 ) );
    
    // TDT
    viewTdtElement( pMes, TdtElem( "20", paxlst.flight() ) );
    
    SetEdiSegGr( pMes, SegGrElement( 3, 0 ) );
    PushEdiPointW( pMes );
    SetEdiPointToSegGrW( pMes, SegGrElement( 3, 0 ) );    
    // LOC departure
    viewLocElement( pMes, LocElem( LocElem::Departure, paxlst.depPort() ) );
    // DTM departure
    viewDtmElement( pMes, DtmElem( DtmElem::Departure, paxlst.depDateTime(), "201" ) );
    PopEdiPointW( pMes );


    SetEdiSegGr( pMes, SegGrElement( 3, 1 ) );
    PushEdiPointW( pMes );
    SetEdiPointToSegGrW( pMes, SegGrElement( 3, 1 ) );
    // LOC arrival
    viewLocElement( pMes, LocElem( LocElem::Arrival, paxlst.arrPort() ) );    
    // DTM arrival
    viewDtmElement( pMes, DtmElem( DtmElem::Arrival, paxlst.arrDateTime(), "201" ) );
    PopEdiPointW( pMes );
    
    PopEdiPointW( pMes );


    PassengersList_t passList = paxlst.passengersList();
    int segmGroupNum = 0;
    for( std::list< Paxlst::PassengerInfo >::const_iterator it = passList.begin();
           it != passList.end(); ++it, segmGroupNum++ )
    {
        SetEdiSegGr( pMes, SegGrElement( 4, segmGroupNum ) );

        PushEdiPointW( pMes );
        SetEdiPointToSegGrW( pMes, SegGrElement( 4, segmGroupNum ) );
        
        // NAD
        viewNadElement( pMes, NadElem( "FL", it->surname(), it->name(), it->street(), it->city() ) );
        // ATT
        viewAttElement( pMes, AttElem( "2", it->sex() ) );
        // DTM
        viewDtmElement( pMes, DtmElem( DtmElem::DateOfBirth, it->birthDate() ) );
        
        int locNum = 0;
        if( !it->depPort().empty() )
        {
            // LOC
            viewLocElement( pMes, LocElem( LocElem::StartJourney, it->depPort() ), locNum++ );
        }
        if( !it->arrPort().empty() )
        {
            // LOC
            viewLocElement( pMes, LocElem( LocElem::FinishJourney, it->arrPort() ), locNum++ );
        }
        
        if( !it->nationality().empty() )
        {
            // NAT
            viewNatElement( pMes, NatElem( "2", it->nationality() ) );
        }

        if( !it->reservNum().empty() )
        {
            // RFF
            viewRffElement( pMes, RffElem( "AVF", it->reservNum() ) );
        }

        if( !it->docType().empty() || !it->docNumber().empty() )
        {
            SetEdiSegGr( pMes, SegGrElement( 5, 0 ) );
            PushEdiPointW( pMes );
            SetEdiPointToSegGrW( pMes, SegGrElement( 5, 0 ) );
            
            // DOC
            viewDocElement( pMes, DocElem( it->docType(), it->docNumber(), paxlst.settings().respAgnCode() ) );
            
            if( it->docExpirateDate() != ASTRA::NoExists )
            {
                // DTM
                viewDtmElement( pMes, DtmElem( DtmElem::DocExpireDate, it->docExpirateDate() ) );
            }


            if( !it->docCountry().empty() )
            {
                // LOC
                viewLocElement( pMes, LocElem( LocElem::DocCountry, it->docCountry() ) );
            }

            PopEdiPointW( pMes );
        }
        
        PopEdiPointW( pMes );
    }

    // CNT
    viewCntElement( pMes, CntElem( CntElem::PassengersTotal, totalCnt ) );
    
    // UNE
    viewUneElement( pMes, UneElem( UnhNumber ) );
}

static std::string ediMessageToStr( _EDI_REAL_MES_STRUCT_ *pMes )
{
    try
    {
        return WriteEdiMessage( pMes );
    }
    catch( edilib::Exception& e )
    {
        throw EXCEPTIONS::Exception( e.what() );
    }
}

static std::string createEdiPaxlstString( const PaxlstInfo& paxlst,
                                          const std::string& ediRef,
                                          unsigned partNum,
                                          unsigned partsCnt,
                                          unsigned totalCnt )
{
    BASIC::TDateTime nowUtc = BASIC::NowUTC();

    edi_mes_head edih;
    memset( &edih, 0, sizeof(edih) );
    edih.syntax_ver = SyntaxVer;
    edih.mes_num = 1;
    strcpy( edih.chset, Chset );
    strcpy( edih.to, paxlst.recipientName().c_str() );
    strcpy( edih.date, BASIC::DateTimeToStr( nowUtc, "yymmdd" ).c_str() );
    strcpy( edih.time, BASIC::DateTimeToStr( nowUtc, "hhnn" ).c_str() );
    strcpy( edih.from, paxlst.senderName().c_str() );
    strcpy( edih.acc_ref, paxlst.iataCode().c_str() );
    strcpy( edih.other_ref, "" );
    strcpy( edih.assoc_code, "" );
    strcpy( edih.our_ref, ediRef.c_str() );
    strcpy( edih.FseId, Apis );
    strcpy( edih.unh_number, UnhNumber );
    strcpy( edih.ver_num, VerNum );
    strcpy( edih.rel_num, RelNum );
    strcpy( edih.cntrl_agn, CntrlAgn );

    _EDI_REAL_MES_STRUCT_* pMes = GetEdiMesStructW();
    if( GetEdiMsgTypeByType( PAXLST, &edih ) )
        throw EXCEPTIONS::Exception( "GetEdiMsgTypeByType failed!" );

    CreateMesByHead( &edih );
    if( !pMes )
        throw EXCEPTIONS::Exception( "pMes is null" );

    collectPaxlstMessage( pMes, paxlst, nowUtc, partNum, partsCnt, totalCnt );
    return "UNA:+.? '\n" + ediMessageToStr( pMes );
}

static void splitPaxlst( std::list< PaxlstInfo >& splitted,
                         const PaxlstInfo& paxlst,    
                         unsigned partSize )
{
    PassengersList_t passList = paxlst.passengersList(), iterList;
    for( PassengersList_t::const_iterator it = passList.begin();
           it != passList.end(); ++it )
    {
        iterList.push_back( *it );
        if( ( iterList.size() == partSize ) || ( it == --passList.end() ) )
        {
            PaxlstInfo newPaxlst( paxlst );
            newPaxlst.setPassengersList( iterList );
            splitted.push_back( newPaxlst );
            iterList.clear();
        }
    }
}

//-----------------------------------------------------------------------------

void PaxlstInfo::addPassenger( const PassengerInfo& pass )
{ 
    if( pass.surname().empty() )
        throw EXCEPTIONS::Exception( "Empty passenger's surname!" );
    m_passList.push_back( pass );
}

std::string PaxlstInfo::toEdiString() const
{
    checkInvariant();
    return createEdiPaxlstString( *this, createEdiInterchangeReference(), 1, 1, passengersList().size() );
}

std::vector< std::string > PaxlstInfo::toEdiStrings( unsigned maxPaxPerString ) const
{
    checkInvariant();
    std::list< PaxlstInfo > splitted;
    splitPaxlst( splitted, *this, maxPaxPerString );
    LogTrace(TRACE3) << "paxlst splitted into " << splitted.size() << " parts";
    if( splitted.empty() )
        return std::vector< std::string >();
    std::vector< std::string > res;
    std::string ediRef = createEdiInterchangeReference();
    unsigned partNum = 0, 
            partsCnt = splitted.size();
    BOOST_FOREACH( const PaxlstInfo& paxlst, splitted )
    {
        res.push_back( createEdiPaxlstString( paxlst, 
                                              ediRef, 
                                              ++partNum, 
                                              partsCnt, 
                                              passengersList().size() ) );
    }

    return res;
}

void PaxlstInfo::checkInvariant() const
{
    if( passengersList().size() < 1 || passengersList().size() > 99999 )
        throw EXCEPTIONS::Exception( "Bad paxlst size!" );

    if( senderName().empty() )
        throw EXCEPTIONS::Exception( "Empty sender name!" );
}

}//namespace Paxlst


//-----------------------------------------------------------------------------

#ifdef XP_TESTING

#include "xp_testing.h"

using namespace xp_testing;

namespace 
{
    void init()
    {
        ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()->connect_db();
        init_edifact();
    }

    void tear_down()
    {
    }
    
    Paxlst::PaxlstInfo makePaxlst1()
    {
        Paxlst::PaxlstInfo paxlstInfo;
    
        paxlstInfo.setPartyName( "CDGkoAF" );
    
        paxlstInfo.setSenderName( "1h" );
        paxlstInfo.setRecipientName( "CzApIs" );
    
        Paxlst::PassengerInfo pass1;
        pass1.setSurname( "StRaNsKy" );
    
        paxlstInfo.addPassenger( pass1 );
    
        return paxlstInfo;
    }
    
    Paxlst::PaxlstInfo makePaxlst3()
    {
        Paxlst::PaxlstInfo paxlstInfo;
    
        paxlstInfo.setPartyName( "cdgKoaf" );
        paxlstInfo.setPhone( "0148642106" );
        paxlstInfo.setFax( "0148643999" );
    
        paxlstInfo.setSenderName( "1h" );
        paxlstInfo.setSenderCarrierCode( "zZ" );
        paxlstInfo.setRecipientName( "CzApIs" );
        paxlstInfo.setRecipientCarrierCode( "fR" );
        paxlstInfo.setIataCode( "OK688/071008/1310" );
    
        paxlstInfo.setFlight( "OK688" );
        paxlstInfo.setDepPort( "PrG" );
        BASIC::TDateTime depDate, arrDate;
        BASIC::StrToDateTime( "08.10.07 10:45:00", depDate ); //"0710081045"
        paxlstInfo.setDepDateTime( depDate );
        paxlstInfo.setArrPort( "BCN" );
        BASIC::StrToDateTime( "08.10.07 13:10:00", arrDate ); //"0710081310"
        paxlstInfo.setArrDateTime( arrDate );
    
        Paxlst::PassengerInfo pass1;
        pass1.setSurname( "STRANSKY" );
        pass1.setName( "JAROSLAV VICtOROVICH" );
        pass1.setSex( "M" );
        BASIC::TDateTime bd1;
        BASIC::StrToDateTime( "10.06.67 00:00:00", bd1 ); //"670610"
        pass1.setBirthDate( bd1 );
        pass1.setDepPort( "ZdN" );
        pass1.setArrPort( "bcN" );
        pass1.setNationality( "CZe" );
        pass1.setReservNum( "Z9WkH" );
        pass1.setDocType( "i" );
        pass1.setDocNumber( "102865098" );    
    
        Paxlst::PassengerInfo pass2;
        pass2.setSurname( "kovacs" );
        pass2.setName( "PETR" );
        pass2.setSex( "M" );
        BASIC::TDateTime bd2;
        BASIC::StrToDateTime( "09.12.69 00:00:00", bd2 ); //"691209"
        pass2.setBirthDate( bd2 );
        pass2.setDepPort( "ZDN" );
        pass2.setArrPort( "BCN" );
        pass2.setNationality( "CZE" );
        pass2.setReservNum( "Z9WJK" );
        pass2.setDocType( "p" );
        pass2.setDocNumber( "35485167" );
        BASIC::TDateTime expd1;
        BASIC::StrToDateTime( "11.09.08 00:00:00", expd1 );
        pass2.setDocExpirateDate( expd1 );    
    
        Paxlst::PassengerInfo pass3;
        pass3.setSurname( "LESKA" );
        pass3.setName( "PAVEL" );
        pass3.setSex( "M" );
        BASIC::TDateTime bd3;
        BASIC::StrToDateTime( "02.05.76 00:00:00", bd3 ); //"760502"
        pass3.setBirthDate( bd3 );
        pass3.setDepPort( "VIE" );
        pass3.setArrPort( "BCN" );
        pass3.setNationality( "CZE" );
        pass3.setReservNum( "z57l3" );
        pass3.setDocType( "P" );
        pass3.setDocNumber( "34356146" );
        pass3.setDocCountry( "RUS" );
    
        paxlstInfo.addPassenger( pass1 );
        paxlstInfo.addPassenger( pass2 );
        paxlstInfo.addPassenger( pass3 );
        
        return paxlstInfo;
    }
    
    Paxlst::PaxlstInfo makePaxlst3_long()
    {
        Paxlst::PaxlstInfo paxlstInfo;
    
        paxlstInfo.setPartyName( "CDGKOAFXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setPhone( "0148642106XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setFax( "0148643999XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
    
        paxlstInfo.setSenderName( "1HXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setSenderCarrierCode( "ZZXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setRecipientName( "CzApIsXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setRecipientCarrierCode( "FRXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        paxlstInfo.setIataCode( "OK688/071008/1310XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
    
        paxlstInfo.setFlight( "OK688XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
    
        paxlstInfo.setDepPort( "PRGXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        BASIC::TDateTime depDate;
        BASIC::StrToDateTime( "08.10.07 10:45:00", depDate ); //"0710081045"
        paxlstInfo.setDepDateTime( depDate );
    
        paxlstInfo.setArrPort( "BCNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        BASIC::TDateTime arrDate;
        BASIC::StrToDateTime( "08.10.07 13:10:00", arrDate ); //"0710081310"
        paxlstInfo.setArrDateTime( arrDate );
    
    
        Paxlst::PassengerInfo pass1;
        pass1.setSurname( "STRANSKYXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setName( "JAROSLAV VICTOROVICHXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setSex( "M" );
        BASIC::TDateTime bd1;
        BASIC::StrToDateTime( "10.06.67 00:00:00", bd1 ); //"670610"
        pass1.setBirthDate( bd1 );
        pass1.setDepPort( "ZDNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setArrPort( "BCNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setNationality( "CZEXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setDocNumber( "Z9WKHXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setDocType( "IXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass1.setDocNumber( "102865098XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
    
    
        Paxlst::PassengerInfo pass2;
        pass2.setSurname( "KOVACSXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setName( "PETRXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setSex( "MXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        BASIC::TDateTime bd2;
        BASIC::StrToDateTime( "09.12.69 00:00:00", bd2 ); //"691209"
        pass2.setBirthDate( bd2 );
        pass2.setDepPort( "ZDNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setArrPort( "BCNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setNationality( "CZEXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setReservNum( "Z9WJKXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setDocType( "PXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass2.setDocNumber( "35485167XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        BASIC::TDateTime expd2;
        BASIC::StrToDateTime( "11.09.08 00:00:00", expd2 );
        pass2.setDocExpirateDate( expd2 );
    
    
        Paxlst::PassengerInfo pass3;
        pass3.setSurname( "LESKAXXXXXXXXXXXXXXXXXXXXXdXXXXXXXXXXXXXXXXX" );
        pass3.setName( "PAVELXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setSex( "MXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        BASIC::TDateTime bd3;
        BASIC::StrToDateTime( "02.05.76 00:00:00", bd2 ); //"760502"
        pass3.setBirthDate( bd3 );
        pass3.setDepPort( "VIEXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setArrPort( "BCNXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setNationality( "CZEXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setReservNum( "Z57L3XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setDocType( "PXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setDocNumber( "34356146XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setNationality( "RUSXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setCity( "MOSCOWXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
        pass3.setStreet( "ARBATXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
    
        paxlstInfo.addPassenger( pass1 );
        paxlstInfo.addPassenger( pass2 );
        paxlstInfo.addPassenger( pass3 );
    
        return paxlstInfo;        
    }
    
    Paxlst::PaxlstInfo makePaxlst5()
    {
        Paxlst::PaxlstInfo paxlstInfo( makePaxlst3() );
        
        Paxlst::PassengerInfo pass4;
        pass4.setSurname( "PUTIN" );
        pass4.setName( "VOVA" );
        pass4.setSex( "M" );
        BASIC::TDateTime bd4;
        BASIC::StrToDateTime( "02.05.52 00:00:00", bd4 );
        pass4.setBirthDate( bd4 );
        pass4.setDepPort( "VIE" );
        pass4.setArrPort( "BCN" );
        pass4.setNationality( "RUS" );
        pass4.setReservNum( "GGFGD1" );
        pass4.setDocType( "P" );
        pass4.setDocNumber( "000001" );
        pass4.setDocCountry( "RUS" );
        
        Paxlst::PassengerInfo pass5;
        pass5.setSurname( "PUTINA" );
        pass5.setName( "LUDA" );
        pass5.setSex( "F" );
        BASIC::TDateTime bd5;
        BASIC::StrToDateTime( "10.05.55 00:00:00", bd5 );
        pass5.setBirthDate( bd5 );
        pass5.setDepPort( "VIE" );
        pass5.setArrPort( "BCN" );
        pass5.setNationality( "RUS" );
        pass5.setReservNum( "GGFGD2" );
        pass5.setDocType( "P" );
        pass5.setDocNumber( "000002" );
        pass5.setDocCountry( "RUS" );
        
        paxlstInfo.addPassenger( pass4 );
        paxlstInfo.addPassenger( pass5 );
        
        return paxlstInfo;
    }
}

///////////////////////////////////////////////////////////////////////////////

START_TEST( test1 )
{

    Paxlst::PaxlstInfo paxlstInfo( makePaxlst3() );

    std::string text = paxlstInfo.toEdiString();
    fail_if( text.empty() );

    // Ожидаемый текст
    TestStrings ts; 
    ts <<
      "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+01:F'\n"
      "BGM+745'\n"
      "NAD+MS+++CDGKOAF'\n"
      "COM+0148642106:TE+0148643999:FX'\n"
      "TDT+20+OK688'\n"
      "LOC+125+PRG'\n"
      "DTM+189:0710081045:201'\n"
      "LOC+87+BCN'\n"
      "DTM+232:0710081310:201'\n"
      "NAD+FL+++STRANSKY:JAROSLAV VICTOROVICH'\n"
      "ATT+2++M'\n"
      "DTM+329:670610'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WKH'\n"
      "DOC+I:110:111+102865098'\n"
      "NAD+FL+++KOVACS:PETR'\n"
      "ATT+2++M'\n"
      "DTM+329:691209'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WJK'\n"
      "DOC+P:110:111+35485167'\n"
      "DTM+36:080911'\n"
      "NAD+FL+++LESKA:PAVEL'\n"
      "ATT+2++M'\n"
      "DTM+329:760502'\n"
      "LOC+178+VIE'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z57L3'\n"
      "DOC+P:110:111+34356146'\n"
      "LOC+91+RUS'\n"
      "CNT+42:3'\n"
      "UNT+39+1'\n"
      "UNE+1+1'\n";

    // Сгенерированный текст
    LogTrace(TRACE5) << "\nText:\n" << text;

    std::string chk( ts.check( text ) );
    fail_unless( chk.empty(), "PAXLST mismatched %s", chk.c_str() );
}
END_TEST;

START_TEST( test2 )
{
    Paxlst::PaxlstInfo paxlstInfo( makePaxlst1() );

    std::string text = paxlstInfo.toEdiString();
    fail_if( text.empty() );

    // Ожидаемый текст
    TestStrings ts;
    ts <<
      "UNH+1+PAXLST:D:02B:UN:IATA++01:F'\n"
      "BGM+745'\n"
      "NAD+MS+++CDGKOAF'\n"
      "TDT+20'\n"
      "LOC+125'\n"
      "DTM+189::201'\n"
      "LOC+87'\n"
      "DTM+232::201'\n"
      "NAD+FL+++STRANSKY'\n"
      "ATT+2'\n"
      "DTM+329'\n"
      "CNT+42:1'\n"
      "UNT+15+1'\n"
      "UNE+1+1'\n";

    // Сгенерированный текст
    LogTrace(TRACE5) << "\nText:\n" << text;

    std::string chk( ts.check( text ) );
    fail_unless( chk.empty(), "PAXLST mismatched %s", chk.c_str() );
}
END_TEST;

START_TEST( test3 )
{
    Paxlst::PaxlstInfo paxlstInfo( makePaxlst3_long() );

    std::string text = paxlstInfo.toEdiString();
    fail_if( text.empty() );
    
    // Ожидаемый текст
    TestStrings ts;
    ts <<    
      "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310XXXXXXXXXXXXXXXXXX+01:F'\n"
      "BGM+745'\n"
      "NAD+MS+++CDGKOAFXXXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "COM+0148642106XXXXXXXXXXXXXXX:TE+0148643999XXXXXXXXXXXXXXX:FX'\n"
      "TDT+20+OK688XXXXXXXXXXXX'\n"
      "LOC+125+PRGXXXXXXXXXXXXXXXXXXXXXX'\n"
      "DTM+189:0710081045:201'\n"
      "LOC+87+BCNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "DTM+232:0710081310:201'\n"
      "NAD+FL+++STRANSKYXXXXXXXXXXXXXXXXXXXXXXXXXXX:JAROSLAV VICTOROVICHXXXXXXXXXXXXXXX'\n"
      "ATT+2++M'\n"
      "DTM+329:670610'\n"
      "LOC+178+ZDNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "LOC+179+BCNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "NAT+2+CZE'\n"

      "DOC+IXX:110:111+102865098XXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "NAD+FL+++KOVACSXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:PETRXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "ATT+2++MXXXXXXXXXXXXXXXX'\n"
      "DTM+329:691209'\n"
      "LOC+178+ZDNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "LOC+179+BCNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WJKXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "DOC+PXX:110:111+35485167XXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "DTM+36:080911'\n"
      "NAD+FL+++LESKAXXXXXXXXXXXXXXXXXXXXXDXXXXXXXX:PAVELXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX+ARBATXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX+MOSCOWXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "ATT+2++MXXXXXXXXXXXXXXXX'\n"
      "DTM+329:991230'\n"
      "LOC+178+VIEXXXXXXXXXXXXXXXXXXXXXX'\n"
      "LOC+179+BCNXXXXXXXXXXXXXXXXXXXXXX'\n"
      "NAT+2+RUS'\n"
      "RFF+AVF:Z57L3XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "DOC+PXX:110:111+34356146XXXXXXXXXXXXXXXXXXXXXXXXXXX'\n"
      "CNT+42:3'\n"
      "UNT+37+1'\n"
      "UNE+1+1'\n";

    // Сгенерированный текст
    LogTrace(TRACE5) << "\nText:\n" << text;

    std::string chk( ts.check( text ) );
    fail_unless( chk.empty(), "PAXLST mismatched %s", chk.c_str() );
}
END_TEST;

START_TEST( test4 )
{
    BASIC::TDateTime depDate;
    BASIC::StrToDateTime( "2007.09.07", "yyyy.mm.dd", depDate );

    std::string fname = Paxlst::createEdiPaxlstFileName( "OK", 421, "", "CAI", "PRG", depDate, "TXT" );
    fail_if( fname != "OK0421CAIPRG20070907.TXT" );
        
    fname = Paxlst::createEdiPaxlstFileName( "OK", 421, "", "CAI", "PRG", depDate, "TXT", 2 );
    fail_if( fname != "OK0421CAIPRG20070907.TXT.PART2" );
}
END_TEST;

START_TEST( test5 )
{
    BASIC::TDateTime destDate;
    BASIC::StrToDateTime( "2007.09.15 12:10", "yyyy.mm.dd hh:nn", destDate );

    std::string iataCode = Paxlst::createIataCode( "OK0012", destDate );
    fail_if( iataCode != "OK0012/070915/1210" );
}
END_TEST;

START_TEST( test6 )
{
    Paxlst::PaxlstInfo paxlstInfo = makePaxlst3();
    
    std::vector< std::string > tlgs = paxlstInfo.toEdiStrings( 3 );
    fail_unless( tlgs.size() == 1 );
    
    // Сгенерированный текст
    LogTrace(TRACE5) << "\nText:\n" << tlgs.front();

    // Ожидаемый текст
    TestStrings ts; 
    ts <<
      "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+01:F'\n"
      "BGM+745'\n"
      "NAD+MS+++CDGKOAF'\n"
      "COM+0148642106:TE+0148643999:FX'\n"
      "TDT+20+OK688'\n"
      "LOC+125+PRG'\n"
      "DTM+189:0710081045:201'\n"
      "LOC+87+BCN'\n"
      "DTM+232:0710081310:201'\n"
      "NAD+FL+++STRANSKY:JAROSLAV VICTOROVICH'\n"
      "ATT+2++M'\n"
      "DTM+329:670610'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WKH'\n"
      "DOC+I:110:111+102865098'\n"
      "NAD+FL+++KOVACS:PETR'\n"
      "ATT+2++M'\n"
      "DTM+329:691209'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WJK'\n"
      "DOC+P:110:111+35485167'\n"
      "DTM+36:080911'\n"
      "NAD+FL+++LESKA:PAVEL'\n"
      "ATT+2++M'\n"
      "DTM+329:760502'\n"
      "LOC+178+VIE'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z57L3'\n"
      "DOC+P:110:111+34356146'\n"
      "LOC+91+RUS'\n"
      "CNT+42:3'\n"
      "UNT+39+1'\n"
      "UNE+1+1'\n";
    
    std::string chk( ts.check( tlgs.front() ) );
    fail_unless( chk.empty(), "PAXLST mismatched %s", chk.c_str() );
}
END_TEST;

START_TEST( test7 )
{
    Paxlst::PaxlstInfo paxlstInfo = makePaxlst5();
    
    std::vector< std::string > tlgs = paxlstInfo.toEdiStrings( 2 );
    fail_unless( tlgs.size() == 3 );
    
    {
        TestStrings ts;
        ts <<
          "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+01:C'\n"
          "BGM+745'\n"
          "NAD+MS+++CDGKOAF'\n"
          "COM+0148642106:TE+0148643999:FX'\n"
          "TDT+20+OK688'\n"
          "LOC+125+PRG'\n"
          "DTM+189:0710081045:201'\n"
          "LOC+87+BCN'\n"
          "DTM+232:0710081310:201'\n"
          "NAD+FL+++STRANSKY:JAROSLAV VICTOROVICH'\n"
          "ATT+2++M'\n"
          "DTM+329:670610'\n"
          "LOC+178+ZDN'\n"
          "LOC+179+BCN'\n"
          "NAT+2+CZE'\n"
          "RFF+AVF:Z9WKH'\n"
          "DOC+I:110:111+102865098'\n"
          "NAD+FL+++KOVACS:PETR'\n"
          "ATT+2++M'\n"
          "DTM+329:691209'\n"
          "LOC+178+ZDN'\n"
          "LOC+179+BCN'\n"
          "NAT+2+CZE'\n"
          "RFF+AVF:Z9WJK'\n"
          "DOC+P:110:111+35485167'\n"
          "DTM+36:080911'\n"
          "CNT+42:5'\n"
          "UNT+30+1'\n"
          "UNE+1+1'\n";
        
        LogTrace(TRACE5) << "tlgs.part1:\n" << tlgs[ 0 ];
        std::string chk( ts.check( tlgs[ 0 ] ) );
        fail_unless( chk.empty(), "PAXLST part1 mismatched %s", chk.c_str() );
    }
    
    {
        TestStrings ts;
        ts <<
          "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+02'\n"
          "BGM+745'\n"
          "NAD+MS+++CDGKOAF'\n"
          "COM+0148642106:TE+0148643999:FX'\n"
          "TDT+20+OK688'\n"
          "LOC+125+PRG'\n"
          "DTM+189:0710081045:201'\n"
          "LOC+87+BCN'\n"
          "DTM+232:0710081310:201'\n"
          "NAD+FL+++LESKA:PAVEL'\n"
          "ATT+2++M'\n"
          "DTM+329:760502'\n"
          "LOC+178+VIE'\n"
          "LOC+179+BCN'\n"
          "NAT+2+CZE'\n"
          "RFF+AVF:Z57L3'\n"
          "DOC+P:110:111+34356146'\n"
          "LOC+91+RUS'\n"
          "NAD+FL+++PUTIN:VOVA'\n"
          "ATT+2++M'\n"
          "DTM+329:520502'\n"
          "LOC+178+VIE'\n"
          "LOC+179+BCN'\n"
          "NAT+2+RUS'\n"
          "RFF+AVF:GGFGD1'\n"
          "DOC+P:110:111+000001'\n"
          "LOC+91+RUS'\n"
          "CNT+42:5'\n"
          "UNT+31+1'\n"
          "UNE+1+1'\n";
              
        LogTrace(TRACE5) << "tlg.part2:\n" << tlgs[ 1 ];
        std::string chk( ts.check( tlgs[ 1 ] ) );
        fail_unless( chk.empty(), "PAXLST part2 mismatched %s", chk.c_str() );
    }
    
    {
        TestStrings ts;
        ts <<
          "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+03:F'\n"
          "BGM+745'\n"
          "NAD+MS+++CDGKOAF'\n"
          "COM+0148642106:TE+0148643999:FX'\n"
          "TDT+20+OK688'\n"
          "LOC+125+PRG'\n"
          "DTM+189:0710081045:201'\n"
          "LOC+87+BCN'\n"
          "DTM+232:0710081310:201'\n"
          "NAD+FL+++PUTINA:LUDA'\n"
          "ATT+2++F'\n"
          "DTM+329:550510'\n"
          "LOC+178+VIE'\n"
          "LOC+179+BCN'\n"
          "NAT+2+RUS'\n"
          "RFF+AVF:GGFGD2'\n"
          "DOC+P:110:111+000002'\n"
          "LOC+91+RUS'\n"
          "CNT+42:5'\n"
          "UNT+22+1'\n"
          "UNE+1+1'\n";
              
        LogTrace(TRACE5) << "tlg.part3:\n" << tlgs[ 2 ];
        std::string chk( ts.check( tlgs[ 2 ] ) );
        fail_unless( chk.empty(), "PAXLST part3 mismatched %s", chk.c_str() );
    }
}
END_TEST;

START_TEST( test8 )
{
    Paxlst::PaxlstInfo paxlstInfo = makePaxlst3();
    
    std::vector< std::string > tlgs = paxlstInfo.toEdiStrings( 50 );
    fail_unless( tlgs.size() == 1 );
 
    // Сгенерированный текст
    LogTrace(TRACE5) << "\nText:\n" << tlgs.front();

    // Ожидаемый текст
    TestStrings ts; 
    ts <<
      "UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+01:F'\n"
      "BGM+745'\n"
      "NAD+MS+++CDGKOAF'\n"
      "COM+0148642106:TE+0148643999:FX'\n"
      "TDT+20+OK688'\n"
      "LOC+125+PRG'\n"
      "DTM+189:0710081045:201'\n"
      "LOC+87+BCN'\n"
      "DTM+232:0710081310:201'\n"
      "NAD+FL+++STRANSKY:JAROSLAV VICTOROVICH'\n"
      "ATT+2++M'\n"
      "DTM+329:670610'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WKH'\n"
      "DOC+I:110:111+102865098'\n"
      "NAD+FL+++KOVACS:PETR'\n"
      "ATT+2++M'\n"
      "DTM+329:691209'\n"
      "LOC+178+ZDN'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z9WJK'\n"
      "DOC+P:110:111+35485167'\n"
      "DTM+36:080911'\n"
      "NAD+FL+++LESKA:PAVEL'\n"
      "ATT+2++M'\n"
      "DTM+329:760502'\n"
      "LOC+178+VIE'\n"
      "LOC+179+BCN'\n"
      "NAT+2+CZE'\n"
      "RFF+AVF:Z57L3'\n"
      "DOC+P:110:111+34356146'\n"
      "LOC+91+RUS'\n"
      "CNT+42:3'\n"
      "UNT+39+1'\n"
      "UNE+1+1'\n";
    
    std::string chk( ts.check( tlgs.front() ) );
    fail_unless( chk.empty(), "PAXLST mismatched %s", chk.c_str() );
}
END_TEST;


#define SUITENAME "apis_file"
TCASEREGISTER( init, tear_down)
{
    ADD_TEST( test1 );
    ADD_TEST( test2 );
    ADD_TEST( test3 );
    ADD_TEST( test4 );
    ADD_TEST( test5 );
    ADD_TEST( test6 );
    ADD_TEST( test7 );
    ADD_TEST( test8 );
}
TCASEFINISH;

#endif /*XP_TESTING*/
