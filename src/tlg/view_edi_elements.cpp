#include "view_edi_elements.h"
#include "astra_consts.h"

#include <edilib/edi_func_cpp.h>
#include <edilib/edi_astra_msg_types.h>
#include <edilib/edi_sess.h>
#include <serverlib/str_utils.h>

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>


namespace edifact
{
using namespace edilib;


void viewUnbElement( _EDI_REAL_MES_STRUCT_* pMes, const UnbElem& elem )
{
    PushEdiPointW( pMes );
    SetEdiPointToSegmentW( pMes, SegmElement( "UNB" ) );

    PushEdiPointW( pMes );
    SetEdiPointToCompositeW( pMes, CompElement( "S002", 0 ) );
    SetEdiDataElem( pMes, DataElement( 7, 0 ), elem.m_senderCarrierCode );
    PopEdiPointW( pMes );

    PushEdiPointW( pMes );
    SetEdiPointToCompositeW( pMes, CompElement( "S003", 0 ) );
    SetEdiDataElem( pMes, DataElement( 7, 0 ), elem.m_recipientCarrierCode );
    PopEdiPointW( pMes );
    
    PopEdiPointW( pMes );
}

void viewUngElement( _EDI_REAL_MES_STRUCT_* pMes, const UngElem& elem )
{
    SetEdiSegment( pMes, SegmElement( "UNG" ) );

    PushEdiPointW( pMes );
    SetEdiPointToSegmentW( pMes, SegmElement( "UNG" ) );
    SetEdiDataElem( pMes, DataElement( 38, 0 ), elem.m_msgGroupName );

    SetEdiComposite( pMes, CompElement( "S006", 0 ) );
    PushEdiPointW( pMes );
    SetEdiPointToCompositeW( pMes, CompElement( "S006", 0 ) );
    SetEdiDataElem( pMes, DataElement( 40, 0 ), elem.m_senderName );
    SetEdiDataElem( pMes, DataElement( 7, 0 ), elem.m_senderCarrierCode );
    PopEdiPointW( pMes );

    SetEdiComposite( pMes, CompElement( "S007", 0 ) );
    PushEdiPointW( pMes );
    SetEdiPointToCompositeW( pMes, CompElement( "S007", 0 ) );
    SetEdiDataElem( pMes, DataElement( 44, 0 ), elem.m_recipientName );
    SetEdiDataElem( pMes, DataElement( 7, 0 ), elem.m_recipientCarrierCode );
    PopEdiPointW( pMes );

    SetEdiComposite( pMes, CompElement( "S004", 0 ) );
    PushEdiPointW( pMes );
    SetEdiPointToCompositeW( pMes, CompElement( "S004", 0 ) );
    SetEdiDataElem( pMes, DataElement( 17, 0 ), BASIC::DateTimeToStr( elem.m_prepareDateTime, "yymmdd" ) );
    SetEdiDataElem( pMes, DataElement( 19, 0 ), BASIC::DateTimeToStr( elem.m_prepareDateTime, "hhnn") );
    PopEdiPointW( pMes );

    SetEdiDataElem( pMes, DataElement( 48, 0 ), elem.m_groupRefNum );
    SetEdiDataElem( pMes, DataElement( 51, 0 ), elem.m_cntrlAgnCode );
    
    SetEdiComposite( pMes, CompElement( "S008", 0 ) );
    PushEdiPointW( pMes );
    SetEdiPointToCompositeW( pMes, CompElement( "S008", 0 ) );
    SetEdiDataElem( pMes, DataElement( 52, 0 ), elem.m_msgVerNum );
    SetEdiDataElem( pMes, DataElement( 54, 0 ), elem.m_msgRelNum );        
    PopEdiPointW( pMes );

    PopEdiPointW( pMes );
}

void viewUnhElement( _EDI_REAL_MES_STRUCT_* pMes, const UnhElem& elem )
{
    PushEdiPointW( pMes );
    SetEdiPointToSegmentW( pMes, SegmElement( "UNH" ) );

    SetEdiComposite( pMes, CompElement( "S009", 0 ) );    
    PushEdiPointW( pMes );
    SetEdiPointToCompositeW( pMes, CompElement( "S009", 0 ) );
    SetEdiDataElem( pMes, DataElement( 65, 0 ), elem.m_msgTypeId );
    SetEdiDataElem( pMes, DataElement( 52, 0 ), elem.m_msgVerNum );
    SetEdiDataElem( pMes, DataElement( 54, 0 ), elem.m_msgRelNum );    
    SetEdiDataElem( pMes, DataElement( 51, 0 ), elem.m_cntrlAgnCode );
    SetEdiDataElem( pMes, DataElement( 57, 0 ), elem.m_assAccCode );
    PopEdiPointW( pMes );
            
    SetEdiComposite( pMes, CompElement( "S010", 0 ) );
    PushEdiPointW( pMes );
    SetEdiPointToCompositeW( pMes, CompElement( "S010", 0 ) );
    std::string seqNum = boost::lexical_cast< std::string >( elem.m_seqNumber );
    SetEdiDataElem( pMes, DataElement( 70, 0 ), StrUtils::LPad( seqNum, 2, '0' ) );
    SetEdiDataElem( pMes, DataElement( 73, 0 ), UnhElem::seqFlagToStr( elem.m_seqFlag ) );
    PopEdiPointW( pMes );
        
    PopEdiPointW( pMes );
}

void viewUneElement( _EDI_REAL_MES_STRUCT_* pMes, const UneElem& elem )
{
    std::ostringstream une;
    une << elem.m_cntrlCnt << "+" << elem.m_refNum;
    SetEdiFullSegment( pMes, SegmElement( "UNE" ), une.str() );
}

void viewBgmElement( _EDI_REAL_MES_STRUCT_* pMes, const BgmElem& elem )
{
    std::ostringstream bgm;
    bgm << elem.m_docCode;
    if (!elem.m_docId.empty())
      bgm << "+" << elem.m_docId;
    SetEdiFullSegment( pMes, SegmElement( "BGM" ), bgm.str() );
}

void viewNadElement( _EDI_REAL_MES_STRUCT_* pMes, const NadElem& elem, int num )
{
    std::ostringstream nad;
    nad << elem.m_funcCode << "+++" << elem.m_partyName;
    if (elem.m_funcCode!="MS")
    {
      nad << ":" << elem.m_partyName2;
      if (!elem.m_partyName3.empty())
        nad << ":" << elem.m_partyName3;
      if (!elem.m_street.empty() ||
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
    SetEdiFullSegment( pMes, SegmElement( "NAD", num ), nad.str() );
}

void viewComElement( _EDI_REAL_MES_STRUCT_* pMes, const ComElem& elem, int num )
{
    std::ostringstream com;
    if( !elem.m_phone.empty() )
        com << elem.m_phone << ":" << "TE" << "+";
    if( !elem.m_fax.empty() )
        com << elem.m_fax << ":" << "FX" << "+";
    if( !elem.m_email.empty() )
        com << elem.m_email << ":" << "EM" << "+";
    SetEdiFullSegment( pMes, SegmElement( "COM", num ), com.str() );
}

void viewTdtElement( _EDI_REAL_MES_STRUCT_* pMes, const TdtElem& elem, int num )
{
    std::ostringstream tdt;
    tdt << elem.m_stageQuailifier << "+" << elem.m_journeyId
        << "+++" << elem.m_carrierId;
    SetEdiFullSegment( pMes, SegmElement( "TDT", num ), tdt.str() );
}

void viewLocElement( _EDI_REAL_MES_STRUCT_* pMes, const LocElem& elem, int num )
{
    std::ostringstream loc;
    loc << boost::lexical_cast< std::string >( elem.m_locQualifier ) << "+" << elem.m_locName;
    if (!elem.m_relatedLocName1.empty()||
        !elem.m_relatedLocName2.empty())
    {
      loc << "+";
      if (!elem.m_relatedLocName1.empty())
        loc << ":::" << elem.m_relatedLocName1;
      loc << "+";
      if (!elem.m_relatedLocName2.empty())
        loc << ":::" << elem.m_relatedLocName2;
    };

    SetEdiFullSegment( pMes, SegmElement( "LOC", num ), loc.str() );
}

void viewDtmElement( _EDI_REAL_MES_STRUCT_* pMes, const DtmElem& elem, int num )
{
    std::ostringstream dtm;
    dtm << boost::lexical_cast< std::string >( elem.m_dtmQualifier ) << ":";
    std::string format = ( elem.m_formatCode == "201" ? "yymmddhhnn" : "yymmdd" );
    dtm << ( elem.m_dateTime != ASTRA::NoExists ? BASIC::DateTimeToStr( elem.m_dateTime, format ) : "" );
    dtm << ":" << elem.m_formatCode;
    SetEdiFullSegment( pMes, SegmElement( "DTM", num ), dtm.str() );
}

void viewAttElement( _EDI_REAL_MES_STRUCT_* pMes, const AttElem& elem, int num )
{
    std::ostringstream att;
    att << elem.m_funcCode << "++" << elem.m_value;
    SetEdiFullSegment( pMes, SegmElement( "ATT", num ), att.str() );
}

void viewNatElement( _EDI_REAL_MES_STRUCT_* pMes, const NatElem& elem, int num )
{
    std::ostringstream nat;
    nat << elem.m_natQualifier << "+" << elem.m_nat;
    SetEdiFullSegment( pMes, SegmElement( "NAT", num ), nat.str() );
}

void viewRffElement( _EDI_REAL_MES_STRUCT_* pMes, const RffElem& elem, int num )
{
    std::ostringstream rff;
    rff << elem.m_rffQualifier << ":" << elem.m_ref;
    SetEdiFullSegment( pMes, SegmElement( "RFF", num ), rff.str() );
}

void viewDocElement( _EDI_REAL_MES_STRUCT_* pMes, const DocElem& elem, int num )
{
    std::ostringstream doc;
    doc << elem.m_docCode << ":" << elem.m_idCode << ":" << elem.m_respAgnCode << "+";
    doc << elem.m_docNum;
    SetEdiFullSegment( pMes, SegmElement( "DOC", num ), doc.str() );
}

void viewCntElement( _EDI_REAL_MES_STRUCT_* pMes, const CntElem& elem, int num )
{
    std::ostringstream cnt;
    cnt << boost::lexical_cast< std::string >( elem.m_cntType ) << ":";
    cnt << elem.m_cnt;
    SetEdiFullSegment( pMes, SegmElement( "CNT", num ), cnt.str() );
}

void viewTktElement( _EDI_REAL_MES_STRUCT_* pMes, const TktElem& elem )
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

    SetEdiFullSegment( pMes, SegmElement( "TKT" ), tkt.str() );
}

void viewCpnElement( _EDI_REAL_MES_STRUCT_* pMes, const CpnElem& elem )
{
    std::ostringstream cpn;
    cpn << elem.m_num << ":";
    if( elem.m_status )
        cpn << elem.m_status->code();
    cpn << ":";
    if( elem.m_amount.isValid() )
        cpn << elem.m_amount.amStr();
    cpn << ":";
    if( elem.m_media )
        cpn << elem.m_media->code();

    cpn << ":" << elem.m_sac << ":::";
    if( elem.m_connectedNum )
        cpn << elem.m_connectedNum;
    cpn << "::";
    if( !elem.m_action.empty() )
        cpn << elem.m_action;

    SetEdiFullSegment( pMes, SegmElement( "CPN" ), cpn.str() );
}

void viewEqnElement( _EDI_REAL_MES_STRUCT_* pMes, const EqnElem& elem )
{
    std::ostringstream eqn;
    eqn << elem.m_numberOfUnits << ":";
    eqn << elem.m_qualifier;
    SetEdiFullSegment( pMes, SegmElement( "EQN" ), eqn.str() );
}

void viewEqnElement( _EDI_REAL_MES_STRUCT_* pMes, const std::list<EqnElem>& lElem )
{
    if(lElem.empty())
        return;

    std::ostringstream eqn;
    BOOST_FOREACH(const EqnElem& elem, lElem)
    {
        eqn << elem.m_numberOfUnits << ":";
        eqn << elem.m_qualifier << "+";
    }
    SetEdiFullSegment(pMes, SegmElement( "EQN" ), eqn.str());
}

void viewOrgElement( _EDI_REAL_MES_STRUCT_* pMes, const Ticketing::OrigOfRequest& elem )
{
    std::ostringstream org;
    org << elem.airlineCode() << ":" + elem.locationCode() << "+";
    org << elem.pprNumber() << "+++";
    org << elem.type() << "+::";
    org << elem.langStr()<< "+" << elem.pult();
    SetEdiFullSegment( pMes, SegmElement( "ORG" ), org.str() );
}

}//namespace edifact
