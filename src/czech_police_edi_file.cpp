//
// C++ Implementation: czech_police_edi_file
//
// Description:
//
//
// Author: anton <anton@whale>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "czech_police_edi_file.h"
#include <edilib/edi_func_cpp.h>
#include "edilib/edi_astra_msg_types.h"
#include "edilib/edi_sess.h"
#include "query_runner.h"

#include "xp_testing.h"
#ifdef XP_TESTING
#include "tlg/tlg.h"

#include <time.h>
#include <boost/lexical_cast.hpp>

#include <sstream>

std::string InterchangeReferenceTst = "";
std::string PrepareDateTst = "";
std::string PrepareHourTst = "";


namespace Paxlst
{

using boost::lexical_cast;
using boost::bad_lexical_cast;


string CreateEdiPaxlstString( const PaxlstInfo& paxlstInfo )
{
    if ( paxlstInfo.passangersList.size() < 1 ||
         paxlstInfo.passangersList.size() > 99999 )
    {
        throw PaxlstException( "CreateEdiPaxlstString error: Passengers list has bad size" );
    }

    if ( !paxlstInfo.isSenderNameSet() )
    {
        throw PaxlstException( "CreateEdiPaxlstString error: Sendername is not set" );
    }


    for ( list< PassengerInfo >::const_iterator it = paxlstInfo.passangersList.begin();
          it != paxlstInfo.passangersList.end(); ++it )
    {
        if ( !it->isPassengerSurnameSet() )
            throw PaxlstException( "CreateEdiPaxlstString error: \
Passengers list contains empty passenger( with empty surname )" );
    }

    BASIC::TDateTime nowDateTime = BASIC::NowUTC();

    string prepareDateStr = "", prepareHourStr = "",
    departureDateStr = "", arrivalDateStr = "";

    if ( !CreateDateTimeStr( prepareDateStr, nowDateTime, "yymmdd" ) )
    {
        throw PaxlstException( "CreateEdiPaxlstString error: error while create \
PrepareDate string" );
    }

    if ( !CreateDateTimeStr( prepareHourStr, nowDateTime, "hhnn" ) )
    {
        throw PaxlstException( "CreateEdiPaxlstString error: error while create \
PrepareTime string" );
    }

    if ( !CreateDateTimeStr( departureDateStr, paxlstInfo.departureDate, "yymmddhhnn" ) )
    {
        throw PaxlstException( "CreateEdiPaxlstString error: error while create \
DepartureDate string" );
    }

    if ( !CreateDateTimeStr( arrivalDateStr, paxlstInfo.arrivalDate, "yymmddhhnn" ) )
    {
        throw PaxlstException( "CreateEdiPaxlstString error: error while create \
ArrivalDate string" );
    }

    string interChangeRef = "asaaa";
    if ( !CreateEdiInterchangeReference( interChangeRef ) )
    {
        throw PaxlstException( "CreateEdiPaxlstString error: error while create \
InterchangeReference string" );
    }

    //<TST>
    InterchangeReferenceTst = interChangeRef;
    PrepareDateTst = prepareDateStr;
    PrepareHourTst = prepareHourStr;
    //</TST>

    edi_mes_head edih;
    memset( &edih, 0, sizeof( edih ) );


    edih.syntax_ver = 4;
    edih.mes_num = 1;


    strcpy( edih.chset, "UNOA" );
    strcpy( edih.to, "CZAPIS" );
    strcpy(edih.date, prepareDateStr.c_str() );
    strcpy(edih.time, prepareHourStr.c_str() );
    strcpy( edih.from, paxlstInfo.senderName.c_str() );
    strcpy( edih.acc_ref, paxlstInfo.iataCode.c_str() );

    strcpy( edih.other_ref, "" );
    strcpy( edih.assoc_code, "" );


    strcpy( edih.our_ref, interChangeRef.c_str() );
    strcpy( edih.FseId, "APIS" );

    strcpy( edih.unh_number, "1" );
    strcpy( edih.ver_num, "D" );
    strcpy( edih.rel_num, "02B" );
    strcpy( edih.cntrl_agn, "UN" );


    EDI_REAL_MES_STRUCT *pMes = GetEdiMesStructW();
    if( GetEdiMsgTypeByType( PAXLST, &edih ) )
    {
        throw PaxlstException( "CreateEdiPaxlstString error: GetEdiMsgTypeByType failed" );
    }

    CreateMesByHead( &edih );
    if ( pMes == 0 )
    {
        throw PaxlstException( "CreateEdiPaxlstString error: GetEdiMesStruct is null" );
    }



    using namespace edilib;


    ResetEdiPointW( pMes );

    //<UNB>
    PushEdiPointW( pMes );
    SetEdiPointToSegmentW( pMes, SegmElement( "UNB" ) );

        //<S002>
        PushEdiPointW( pMes );
        SetEdiPointToCompositeW( pMes, CompElement( "S002", 0 ) );

            //<DE:0007>
            SetEdiDataElem( pMes, DataElement( 7, 0 ),
                            paxlstInfo.senderCarrierCode.c_str() );
            //</DE:0007>

        PopEdiPointW( pMes );
        //</S002>

        //<S003>
        PushEdiPointW( pMes );
        SetEdiPointToCompositeW( pMes, CompElement( "S003", 0 ) );

            //<DE:0007>
            SetEdiDataElem( pMes, DataElement( 7, 0 ),
                            paxlstInfo.recipientCarrierCode.c_str() );
            //</DE:0007>

        PopEdiPointW( pMes );
        //</S003>

    PopEdiPointW( pMes );
    //</UNB>


    //<UNG>
    SetEdiSegment( pMes, SegmElement( "UNG" ) );

    PushEdiPointW( pMes );
    SetEdiPointToSegmentW( pMes, SegmElement( "UNG" ) );

        //<DE:0038>
        SetEdiDataElem( pMes, DataElement( 38, 0 ), "PAXLST" );
        //</DE:0038>

        //<S006>
        SetEdiComposite( pMes, CompElement( "S006", 0 ) );

        PushEdiPointW( pMes );
        SetEdiPointToCompositeW( pMes, CompElement( "S006", 0 ) );

            //<DE:0040>
            SetEdiDataElem( pMes, DataElement( 40, 0 ),
                            paxlstInfo.senderName.c_str() );
            //</DE:0040>

            //<DE:0007>
            SetEdiDataElem( pMes, DataElement( 7, 0 ),
                            paxlstInfo.senderCarrierCode.c_str() );
            //</DE:0007>

        PopEdiPointW( pMes );
        //</S006>

        //<S007>
        SetEdiComposite( pMes, CompElement( "S007", 0 ) );

        PushEdiPointW( pMes );
        SetEdiPointToCompositeW( pMes, CompElement( "S007", 0 ) );

            //<DE:0044>
            SetEdiDataElem( pMes, DataElement( 44, 0 ), "CZAPIS" );
            //</DE:0044>

            //<DE:0007>
            SetEdiDataElem( pMes, DataElement( 7, 0 ),
                            paxlstInfo.recipientCarrierCode.c_str() );
            //</DE:0007>

        PopEdiPointW( pMes );
        //</S007>

        //<S004>
        SetEdiComposite( pMes, CompElement( "S004", 0 ) );

        PushEdiPointW( pMes );
        SetEdiPointToCompositeW( pMes, CompElement( "S004", 0 ) );

            //<DE:0017>
            SetEdiDataElem( pMes, DataElement( 17, 0 ), prepareDateStr.c_str() );
            //</DE:0017>

            //<DE:0019>
            SetEdiDataElem( pMes, DataElement( 19, 0 ), prepareHourStr.c_str() );
            //</DE:0019>

        PopEdiPointW( pMes );
        //</S004>

        //<DE:0048>
        SetEdiDataElem( pMes, DataElement( 48, 0 ), "1" );
        //</DE:0048>

        //<DE:0051>
        SetEdiDataElem( pMes, DataElement( 51, 0 ), "UN" );
        //</DE:0051>

        //<S008>
        SetEdiFullComposite( "S008", 0, "D:02B" );
        //</S008>

    PopEdiPointW( pMes );
    //</UNG>


    //<UNH>
    PushEdiPointW( pMes );
    SetEdiPointToSegmentW( pMes, SegmElement( "UNH" ) );

        //<S009>
        SetEdiFullComposite( "S009", 0, "PAXLST:D:02B:UN:IATA" );
        //</S009>

        //<S010>
        SetEdiFullComposite( "S010", 0, "01:C" );
        //</S010>

    PopEdiPointW( pMes );
    //</UNH>


    //<BGM>
    SetEdiFullSegment( pMes, SegmElement( "BGM" ), "745" );
    //</BGM>


    if ( paxlstInfo.isPartyNameSet() )
    {
        //<Segment Group 1>
        SetEdiSegGr( pMes, SegGrElement( 1, 0 ) );

        PushEdiPointW( pMes );
        SetEdiPointToSegGrW( pMes, SegGrElement( 1, 0 ) );

            //<NAD>
            SetEdiSegment( pMes, SegmElement( "NAD" ) );

            PushEdiPointW( pMes );
            SetEdiPointToSegmentW( pMes, SegmElement( "NAD" ) );

                //<DE:3035>
                SetEdiDataElem( pMes, DataElement( 3035, 0 ), "MS" );
                //</DE:3035>

                //<C080>
                SetEdiComposite( pMes, CompElement( "C080", 0 ) );

                PushEdiPointW( pMes );
                SetEdiPointToCompositeW( pMes, CompElement( "C080", 0 ) );

                    //<DE:3036>
                    SetEdiDataElem( pMes, DataElement( 3036, 0 ),
                                    paxlstInfo.partyName.c_str() );
                    //</DE:3036>

                PopEdiPointW( pMes );
                //</C080>

            PopEdiPointW( pMes );
            //</NAD>

            if ( paxlstInfo.isPhoneAndFaxSet() )
            {
                //<COM>
                SetEdiSegment( pMes, SegmElement( "COM" ) );

                PushEdiPointW( pMes );
                SetEdiPointToSegmentW( pMes, SegmElement( "COM" ) );

                    //<C076(1)>
                    SetEdiComposite( pMes, CompElement( "C076", 0 ) );

                    PushEdiPointW( pMes );
                    SetEdiPointToCompositeW( pMes, CompElement( "C076", 0 ) );

                        //<DE:3148>
                        SetEdiDataElem( pMes, DataElement( 3148, 0 ),
                                        paxlstInfo.phone.c_str() );
                        //</DE:3148>

                        //<DE:3155>
                        SetEdiDataElem( pMes, DataElement( 3155, 0 ), "TE" );
                        //</DE:3155>

                    PopEdiPointW( pMes );
                    //</C076(1)>


                    //<C076(2)>
                    SetEdiComposite( pMes, CompElement( "C076", 1 ) );

                    PushEdiPointW( pMes );
                    SetEdiPointToCompositeW( pMes, CompElement( "C076", 1 ) );

                        //<DE:3148>
                        SetEdiDataElem( pMes, DataElement( 3148, 0 ),
                                        paxlstInfo.fax.c_str() );
                        //</DE:3148>

                        //<DE:3155>
                        SetEdiDataElem( pMes, DataElement( 3155, 0 ), "FX" );
                        //</DE:3155>

                    PopEdiPointW( pMes );
                    //</C076(2)>

                PopEdiPointW( pMes );
                //</COM>
            }

        PopEdiPointW( pMes );
        //</Segment Group 1>
    }


    //<Segment Group 2>
    SetEdiSegGr( pMes, SegGrElement( 2, 0 ) );

    PushEdiPointW( pMes );
    SetEdiPointToSegGrW( pMes, SegGrElement( 2, 0 ) );

        //<TDT>
        SetEdiSegment( pMes, SegmElement( "TDT" ) );

        PushEdiPointW( pMes );
        SetEdiPointToSegmentW( pMes, SegmElement( "TDT" ) );

            //<DE:8051>
            SetEdiDataElem( pMes, DataElement( 8051, 0 ), "20" );
            //</DE:8051>
            if ( paxlstInfo.isFlightSet() )
            {
                //<DE:8028>
                SetEdiDataElem( pMes, DataElement( 8028, 0 ),
                                paxlstInfo.flight.c_str() );
                //</DE:8028>
            }


        PopEdiPointW( pMes );
        //</TDT>


        //<Segment Group 3(1)>
        SetEdiSegGr( pMes, SegGrElement( 3, 0 ) );

        PushEdiPointW( pMes );
        SetEdiPointToSegGrW( pMes, SegGrElement( 3, 0 ) );

            //<LOC>
            SetEdiSegment( pMes, SegmElement( "LOC" ) );

            PushEdiPointW( pMes );
            SetEdiPointToSegmentW( pMes, SegmElement( "LOC" ) );

                //<DE:3227>
                SetEdiDataElem( pMes, DataElement( 3227, 0 ), "125" );
                //</DE:3227>

                if ( paxlstInfo.isDepartureAirportSet() )
                {
                    //<C517>
                    SetEdiComposite( pMes, CompElement( "C517", 0 ) );

                    PushEdiPointW( pMes );
                    SetEdiPointToCompositeW( pMes, CompElement( "C517", 0 ) );

                        //<DE:3225>
                        SetEdiDataElem( pMes, DataElement( 3225, 0 ),
                                        paxlstInfo.departureAirport.c_str() );
                        //</DE:3225>

                    PopEdiPointW( pMes );
                    //</C517>
                }

            PopEdiPointW( pMes );
            //</LOC>


            //<DTM>
            SetEdiSegment( pMes, SegmElement( "DTM" ) );

            PushEdiPointW( pMes );
            SetEdiPointToSegmentW( pMes, SegmElement( "DTM" ) );

                //<C507>
                SetEdiComposite( pMes, CompElement( "C507", 0 ) );

                PushEdiPointW( pMes );
                SetEdiPointToCompositeW( pMes, CompElement( "C507", 0 ) );

                    //<DE:2005>
                    SetEdiDataElem( pMes, DataElement( 2005, 0 ), "189" );
                    //</DE:2005>

                    if ( paxlstInfo.isDepartureDateSet() )
                    {
                        //<DE:2380>
                        SetEdiDataElem( pMes, DataElement( 2380, 0 ),
                                        departureDateStr.c_str() );
                        //</DE:2380>
                    }

                    //<DE:2379>
                    SetEdiDataElem( pMes, DataElement( 2379, 0 ), "201" );
                    //</DE:2379>

                PopEdiPointW( pMes );
                //</C507>

            PopEdiPointW( pMes );
            //</DTM>

        PopEdiPointW( pMes );
        //</Segment Group 3(1)


        //<Segment Group 3(2)>
        SetEdiSegGr( pMes, SegGrElement( 3, 1 ) );

        PushEdiPointW( pMes );
        SetEdiPointToSegGrW( pMes, SegGrElement( 3, 1 ) );

            //<LOC>
            SetEdiSegment( pMes, SegmElement( "LOC" ) );

            PushEdiPointW( pMes );
            SetEdiPointToSegmentW( pMes, SegmElement( "LOC" ) );

                //<DE:3227>
                SetEdiDataElem( pMes, DataElement( 3227, 0 ), "87" );
                //</DE:3227>

                if ( paxlstInfo.isArrivalAirportSet() )
                {
                    //<C517>
                    SetEdiComposite( pMes, CompElement( "C517", 0 ) );

                    PushEdiPointW( pMes );
                    SetEdiPointToCompositeW( pMes, CompElement( "C517", 0 ) );

                        //<DE:3225>
                        SetEdiDataElem( pMes, DataElement( 3225, 0 ),
                                        paxlstInfo.arrivalAirport.c_str() );
                        //</DE:3225>

                    PopEdiPointW( pMes );
                    //</C517>
                }

            PopEdiPointW( pMes );
            //</LOC>


            //<DTM>
            SetEdiSegment( pMes, SegmElement( "DTM" ) );

            PushEdiPointW( pMes );
            SetEdiPointToSegmentW( pMes, SegmElement( "DTM" ) );

                //<C507>
                SetEdiComposite( pMes, CompElement( "C507", 0 ) );

                PushEdiPointW( pMes );
                SetEdiPointToCompositeW( pMes, CompElement( "C507", 0 ) );

                    //<DE:2005>
                    SetEdiDataElem( pMes, DataElement( 2005, 0 ), "232" );
                    //</DE:2005>

                    if ( paxlstInfo.isArrivalDateSet() )
                    {
                        //<DE:2380>
                        SetEdiDataElem( pMes, DataElement( 2380, 0 ),
                                        arrivalDateStr.c_str() );

                        //</DE:2380>
                    }

                    //<DE:2379>
                    SetEdiDataElem( pMes, DataElement( 2379, 0 ), "201" );
                    //</DE:2379>

                PopEdiPointW( pMes );
                //</C507>

            PopEdiPointW( pMes );
            //</DTM>

        PopEdiPointW( pMes );
        //</Segment Group 3(2)


    PopEdiPointW( pMes );
    //</Segment Group 2>


    int i = 0;
    for ( list< Paxlst::PassengerInfo >::const_iterator it = paxlstInfo.passangersList.begin();
          it != paxlstInfo.passangersList.end(); ++it )
    {
        int segmGroupNum = i++;
        //<Segment Group 4(i)>
        SetEdiSegGr( pMes, SegGrElement( 4, segmGroupNum ) );

        PushEdiPointW( pMes );
        SetEdiPointToSegGrW( pMes, SegGrElement( 4, segmGroupNum ) );

            //<NAD>
            SetEdiSegment( pMes, SegmElement( "NAD" ) );

            PushEdiPointW( pMes );
            SetEdiPointToSegmentW( pMes, SegmElement( "NAD" ) );

                //<DE:3035>
                SetEdiDataElem( pMes, DataElement( 3035, 0 ), "FL" );
                //</DE:3035>


                //<C080>
                SetEdiComposite( pMes, CompElement( "C080", 0 ) );

                PushEdiPointW( pMes );
                SetEdiPointToCompositeW( pMes, CompElement( "C080", 0 ) );

                    //<DE:3036(1)>
                    SetEdiDataElem( pMes, DataElement( 3036, 0 ),
                                    it->passengerSurname.c_str() );
                    //</DE:3036(1)>

                    if ( it->isPassengerNameSet() )
                    {
                        //<DE:3036(2)>
                        SetEdiDataElem( pMes, DataElement( 3036, 1 ),
                                        it->passengerName.c_str() );
                        //</DE:3036(2)>
                    }

                PopEdiPointW( pMes );
                //</C080>

            PopEdiPointW( pMes );
            //</NAD>


            //<ATT>
            SetEdiSegment( pMes, SegmElement( "ATT" ) );

            PushEdiPointW( pMes );
            SetEdiPointToSegmentW( pMes, SegmElement( "ATT" ) );

                //<DE:9017>
                SetEdiDataElem( pMes, DataElement( 9017, 0 ), "2" );
                //</DE:9017>

                if ( it->isPassengerSexSet() )
                {
                    //<C956>
                    SetEdiComposite( pMes, CompElement( "C956", 0 ) );

                    PushEdiPointW( pMes );
                    SetEdiPointToCompositeW( pMes, CompElement( "C956", 0 ) );

                        //<DE:9019>
                        SetEdiDataElem( pMes, DataElement( 9019, 0 ),
                                        it->passengerSex.c_str() );
                        //</DE:9019>

                    PopEdiPointW( pMes );
                    //</C956>
                }

            PopEdiPointW( pMes );
            //</ATT>


            //<DTM>
            SetEdiSegment( pMes, SegmElement( "DTM" ) );

            PushEdiPointW( pMes );
            SetEdiPointToSegmentW( pMes, SegmElement( "DTM" ) );

                //<C507>
                SetEdiComposite( pMes, CompElement( "C507", 0 ) );

                PushEdiPointW( pMes );
                SetEdiPointToCompositeW( pMes, CompElement( "C507", 0 ) );

                    //<DE:2005>
                    SetEdiDataElem( pMes, DataElement( 2005, 0 ), "329" );
                    //</DE:2005>

                    if ( it->isBirthDateSet() )
                    {
                        //<DE:2380>
                        string birthDateStr = "";
                        if ( !CreateDateTimeStr( birthDateStr, it->birthDate, "yymmdd" ) )
                        {
                            throw PaxlstException( "CreateEdiPaxlstString error: \
error while create birthDate string" );
                        }
                        SetEdiDataElem( pMes, DataElement( 2380, 0 ),
                                        birthDateStr.c_str() );
                        //</DE:2380>
                    }

                PopEdiPointW( pMes );
                //</C507>

            PopEdiPointW( pMes );
            //</DTM>

            int sg4_LOC_counter = 0;
            if ( it->isDeparturePassengerSet() )
            {
                //<LOC>
                SetEdiSegment( pMes, SegmElement( "LOC", sg4_LOC_counter ) );

                PushEdiPointW( pMes );
                SetEdiPointToSegmentW( pMes, SegmElement( "LOC", sg4_LOC_counter ) );


                //<DE:3227>
                SetEdiDataElem( pMes, DataElement( 3227, 0 ), "178" );
                //</DE:3227>

                    //<C517>
                    SetEdiComposite( pMes, CompElement( "C517", 0 ) );

                    PushEdiPointW( pMes );
                    SetEdiPointToCompositeW( pMes, CompElement( "C517", 0 ) );

                        //<DE:3225>
                        SetEdiDataElem( pMes, DataElement( 3225, 0 ),
                                        it->departurePassenger.c_str() );
                        //</DE:3225>

                    PopEdiPointW( pMes );
                    //</C517>

                PopEdiPointW( pMes );
                //</LOC>

                sg4_LOC_counter++;
            }


            if ( it->isArrivalPassengerSet() )
            {
                //<LOC>
                SetEdiSegment( pMes, SegmElement( "LOC", sg4_LOC_counter ) );

                PushEdiPointW( pMes );
                SetEdiPointToSegmentW( pMes, SegmElement( "LOC", sg4_LOC_counter ) );


                    //<DE:3227>
                    SetEdiDataElem( pMes, DataElement( 3227, 0 ), "179" );
                    //</DE:3227>

                    //<C517>
                    SetEdiComposite( pMes, CompElement( "C517", 0 ) );

                    PushEdiPointW( pMes );
                    SetEdiPointToCompositeW( pMes, CompElement( "C517", 0 ) );

                        //<DE:3225>
                        SetEdiDataElem( pMes, DataElement( 3225, 0 ),
                                        it->arrivalPassenger.c_str() );
                        //</DE:3225>

                    PopEdiPointW( pMes );
                    //</C517>

                PopEdiPointW( pMes );
                //</LOC>
            }


            if ( it->isPassengerCountrySet() )
            {
                //<NAT>
                SetEdiSegment( pMes, SegmElement( "NAT" ) );

                PushEdiPointW( pMes );
                SetEdiPointToSegmentW( pMes, SegmElement( "NAT" ) );

                    //<DE:3493>
                    SetEdiDataElem( pMes, DataElement( 3493, 0 ), "2" );
                    //</DE:3493>

                        //<C042>
                        SetEdiComposite( pMes, CompElement( "U042", 0 ) );

                        PushEdiPointW( pMes );
                        SetEdiPointToCompositeW( pMes, CompElement( "U042", 0 ) );

                            //<DE:3293>
                            SetEdiDataElem( pMes, DataElement( 3293, 0 ),
                                            it->passengerCountry.c_str() );
                            //</DE:3293>

                        PopEdiPointW( pMes );
                        //</C042>

                PopEdiPointW( pMes );
                //</NAT>
            }


            if ( it->isPassengerNumberSet() )
            {
                //<RFF>
                SetEdiSegment( pMes, SegmElement( "RFF" ) );

                PushEdiPointW( pMes );
                SetEdiPointToSegmentW( pMes, SegmElement( "RFF" ) );

                    //<C506>
                    SetEdiComposite( pMes, CompElement( "C506", 0 ) );

                    PushEdiPointW( pMes );
                    SetEdiPointToCompositeW( pMes, CompElement( "C506", 0 ) );

                        //<DE:1153>
                        SetEdiDataElem( pMes, DataElement( 1153, 0 ), "AVF" );
                        //</DE:1153>

                            //<DE:1154>
                            SetEdiDataElem( pMes, DataElement( 1154, 0 ),
                                            it->passengerNumber.c_str() );
                            //</DE:1154>

                    PopEdiPointW( pMes );
                    //</C506>

                PopEdiPointW( pMes );
                //</RFF>
            }


            if ( it->isPassengerTypeOrIdNumberSet() )
            {
                //<Segment Group 5>
                SetEdiSegGr( pMes, SegGrElement( 5, 0 ) );

                PushEdiPointW( pMes );
                SetEdiPointToSegGrW( pMes, SegGrElement( 5, 0 ) );

                    //<DOC>
                    SetEdiSegment( pMes, SegmElement( "DOC" ) );

                    PushEdiPointW( pMes );
                    SetEdiPointToSegmentW( pMes, SegmElement( "DOC" ) );

                        //<C002>
                        SetEdiComposite( pMes, CompElement( "C002", 0 ) );

                        PushEdiPointW( pMes );
                        SetEdiPointToCompositeW( pMes, CompElement( "C002", 0 ) );

                            if ( it->isPassengerTypeSet() )
                            {
                                //<DE:1001>
                                SetEdiDataElem( pMes, DataElement( 1001, 0 ),
                                                it->passengerType.c_str() );
                                //</DE:1001>
                            }

                            //<DE:1131>
                            SetEdiDataElem( pMes, DataElement( 1131, 0 ), "110" );
                            //</DE:1131>

                            //<DE:3055>
                            SetEdiDataElem( pMes, DataElement( 3055, 0 ), "111" );
                            //</DE:3055>

                        PopEdiPointW( pMes );
                        //</C002>


                        //<C503>
                        SetEdiComposite( pMes, CompElement( "C503", 0 ) );

                        PushEdiPointW( pMes );
                        SetEdiPointToCompositeW( pMes, CompElement( "C503", 0 ) );

                            if ( it->isIdNumberSet() )
                            {
                                //<DE:1004>
                                SetEdiDataElem( pMes, DataElement( 1004, 0 ),
                                                it->idNumber.c_str() );
                                //</DE:1004>
                            }

                        PopEdiPointW( pMes );
                        //</C503>

                    PopEdiPointW( pMes );
                    //</DOC>


                    if ( it->isExpirateDateSet() )
                    {
                        //<DTM>
                        SetEdiSegment( pMes, SegmElement( "DTM" ) );

                        PushEdiPointW( pMes );
                        SetEdiPointToSegmentW( pMes, SegmElement( "DTM" ) );

                            //<C507>
                            SetEdiComposite( pMes, CompElement( "C507", 0 ) );

                            PushEdiPointW( pMes );
                            SetEdiPointToCompositeW( pMes, CompElement( "C507", 0 ) );

                                //<DE:2005>
                                SetEdiDataElem( pMes, DataElement( 2005, 0 ), "36" );
                                //</DE:2005>

                                //<DE:2380>
                                string expirateDateStr = "";
                                if ( !CreateDateTimeStr( expirateDateStr, it->expirateDate, "yymmdd" ) )
                                {
                                    throw PaxlstException( "CreateEdiPaxlstString error: \
error while create expirateDate string" );
                                }
                                SetEdiDataElem( pMes, DataElement( 2380, 0 ),
                                                expirateDateStr.c_str() );
                                //</DE:2380>

                            PopEdiPointW( pMes );
                            //</C507>

                        PopEdiPointW( pMes );
                        //</DTM>
                    }


                    if ( it->isDocCountrySet() )
                    {
                        //<LOC>
                        SetEdiSegment( pMes, SegmElement( "LOC" ) );

                        PushEdiPointW( pMes );
                        SetEdiPointToSegmentW( pMes, SegmElement( "LOC" ) );


                            //<DE:3227>
                            SetEdiDataElem( pMes, DataElement( 3227, 0 ), "91" );
                            //</DE:3227>

                            //<C517>
                            SetEdiComposite( pMes, CompElement( "C517", 0 ) );

                            PushEdiPointW( pMes );
                            SetEdiPointToCompositeW( pMes, CompElement( "C517", 0 ) );

                                //<DE:3225>
                                SetEdiDataElem( pMes, DataElement( 3225, 0 ),
                                                it->docCountry.c_str() );
                                //</DE:3225>

                            PopEdiPointW( pMes );
                            //</C517>

                        PopEdiPointW( pMes );
                        //</LOC>
                    }

                PopEdiPointW( pMes );
                //</Segment Group 5>
            }

        PopEdiPointW( pMes );
        //</Segment Group 4(i)>


        //<CNT>
        SetEdiSegment( pMes, SegmElement( "CNT" ) );

        PushEdiPointW( pMes );
        SetEdiPointToSegmentW( pMes, SegmElement( "CNT" ) );

            //<C270>
            SetEdiComposite( pMes, CompElement( "C270", 0 ) );

            PushEdiPointW( pMes );
            SetEdiPointToCompositeW( pMes, CompElement( "C270", 0 ) );

                //<DE:6069>
                SetEdiDataElem( pMes, DataElement( 6069, 0 ), "42" );
                //</DE:6069>

                //<DE:6066>
                SetEdiDataElem( pMes, DataElement( 6066, 0 ),
                                lexical_cast< string >( paxlstInfo.passangersList.size() ).c_str() );
                //</DE:6066>

            PopEdiPointW( pMes );
            //</C270>

        PopEdiPointW( pMes );
        //</CNT>

        //<UNE>
        SetEdiFullSegment( pMes, SegmElement( "UNE" ), "1+1" );
        //</UNE>

    }

    string res = "";
    try
    {
        res = WriteEdiMessage( pMes );
    }
    catch( edilib::Exception& e )
    {
        throw PaxlstException( "CreateEdiPaxlstString error: error while exec \
edilib::WriteEdiMessage()" );
    }

    return "UNA:+.? '" + res;
}


bool CreateEdiInterchangeReference( string& result )
{
    bool ret = false;
    result = "";
    try
    {
        result = lexical_cast< string >( time( NULL ) );
    }
    catch ( bad_lexical_cast& e )
    {
        result = "bad_interch_ref";
        return false;
    }

    ret = true;

    return ret;
}


bool CreateDateTimeStr( string& res, const BASIC::TDateTime& dt,
                        const string& format )
{
    bool ret = false;
    try
    {
        res = BASIC::DateTimeToStr( dt, format );
    }
    catch( std::exception& e )
    {
        return false;
    }

    ret = true;

    return ret;
}


string CreateIATACode( const string& flight, const string& destDate,
                       const string& destTime )
{
    return flight + "/" + destDate + "/" + destTime;
}


bool CreateIATACode( string& result, const string& flight,
                     const BASIC::TDateTime& destDateTime )
{
    result = "";
    string destDateTimeStr = "";

    if ( !CreateDateTimeStr( destDateTimeStr, destDateTime, "yymmdd/hhnn" ) )
        return false;

    result = flight + "/" + destDateTimeStr;

    return true;
}


string CreateEdiPaxlstFileName( const string& flight, const string& origin,
                                const string& destination,  const string& departureDate,
                                const string& ext )
{
    return flight + origin + destination + departureDate + "." + ext;
}


bool CreateEdiPaxlstFileName( string& result,
                              const string& flight, const string& origin,
                              const string& destination,
                              const BASIC::TDateTime& departureDate,
                              const string& ext )
{
    result = "";
    string depDateStr = "";
    if ( !CreateDateTimeStr( depDateStr, departureDate, "yyyymmdd" ) )
        return false;

    result = flight + origin + destination + depDateStr + "." + ext;

    return true;
}


} // namespace Paxlst



namespace {
    void init()
    {
        ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()->connect_db();
        init_edifact();
    }

    void tear_down()
    {
    }
}


START_TEST( czech_file_test1 )
{
    Paxlst::PaxlstInfo paxlstInfo;

    paxlstInfo.partyName = "CDGKOAF";
    paxlstInfo.phone = "0148642106";
    paxlstInfo.fax = "0148643999";

    paxlstInfo.senderName = "1H";
    paxlstInfo.senderCarrierCode = "ZZ";
    paxlstInfo.recipientCarrierCode = "FR";
    paxlstInfo.iataCode = "OK688/071008/1310";

    paxlstInfo.flight = "OK688";
    paxlstInfo.departureAirport = "PRG";
    BASIC::StrToDateTime( "08.10.07 10:45:00", paxlstInfo.departureDate ); //"0710081045"
    paxlstInfo.arrivalAirport = "BCN";
    BASIC::StrToDateTime( "08.10.07 13:10:00", paxlstInfo.arrivalDate ); //"0710081310"


    Paxlst::PassengerInfo passInfo1;
    passInfo1.passengerSurname = "STRANSKY";
    passInfo1.passengerName = "JAROSLAV";
    passInfo1.passengerSex = "M";
    BASIC::StrToDateTime( "10.06.67 00:00:00", passInfo1.birthDate ); //"670610"
    passInfo1.departurePassenger = "ZDN";
    passInfo1.arrivalPassenger = "BCN";
    passInfo1.passengerCountry = "CZE";
    passInfo1.passengerNumber = "Z9WKH";
    passInfo1.passengerType = "I";
    passInfo1.idNumber = "102865098";


    Paxlst::PassengerInfo passInfo2;
    passInfo2.passengerSurname = "KOVACS";
    passInfo2.passengerName = "PETR";
    passInfo2.passengerSex = "M";
    BASIC::StrToDateTime( "09.12.69 00:00:00", passInfo2.birthDate ); //"691209"
    passInfo2.departurePassenger = "ZDN";
    passInfo2.arrivalPassenger = "BCN";
    passInfo2.passengerCountry = "CZE";
    passInfo2.passengerNumber = "Z9WJK";
    passInfo2.passengerType = "P";
    passInfo2.idNumber = "35485167";
    BASIC::StrToDateTime( "11.09.08 00:00:00", passInfo2.expirateDate );


    Paxlst::PassengerInfo passInfo3;
    passInfo3.passengerSurname = "LESKA";
    passInfo3.passengerName = "PAVEL";
    passInfo3.passengerSex = "M";
    BASIC::StrToDateTime( "02.05.76 00:00:00", passInfo3.birthDate ); //"760502"
    passInfo3.departurePassenger = "VIE";
    passInfo3.arrivalPassenger = "BCN";
    passInfo3.passengerCountry = "CZE";
    passInfo3.passengerNumber = "Z57L3";
    passInfo3.passengerType = "P";
    passInfo3.idNumber = "34356146";
    passInfo3.docCountry = "RUS";


    paxlstInfo.passangersList.push_back( passInfo1 );
    paxlstInfo.passangersList.push_back( passInfo2 );
    paxlstInfo.passangersList.push_back( passInfo3 );


    std::string text = "", errText = "";
    if ( !paxlstInfo.toEdiString( text, errText ) )
    {
        fail( errText.c_str() );
    }


    std::stringstream dueResult; // Ожидаемый текст
    dueResult << "UNA:+.? " << "'";
    dueResult << "UNB+UNOA:4+1H:ZZ+CZAPIS:FR+";
    dueResult << PrepareDateTst << ":" << PrepareHourTst << "+";
    dueResult << InterchangeReferenceTst;
    dueResult << "++APIS'\
UNG+PAXLST+1H:ZZ+CZAPIS:FR+";
    dueResult << PrepareDateTst << ":" << PrepareHourTst;
    dueResult << "+1+UN+D:02B'\
UNH+1+PAXLST:D:02B:UN:IATA+OK688/071008/1310+01:C'\
BGM+745'\
NAD+MS+++CDGKOAF'\
COM+0148642106:TE+0148643999:FX'\
TDT+20+OK688'\
LOC+125+PRG'\
DTM+189:0710081045:201'\
LOC+87+BCN'\
DTM+232:0710081310:201'\
NAD+FL+++STRANSKY:JAROSLAV'\
ATT+2++M'\
DTM+329:670610'\
LOC+178+ZDN'\
LOC+179+BCN'\
NAT+2+CZE'\
RFF+AVF:Z9WKH'\
DOC+I:110:111+102865098'\
NAD+FL+++KOVACS:PETR'\
ATT+2++M'\
DTM+329:691209'\
LOC+178+ZDN'\
LOC+179+BCN'\
NAT+2+CZE'\
RFF+AVF:Z9WJK'\
DOC+P:110:111+35485167'\
DTM+36:080911'\
NAD+FL+++LESKA:PAVEL'\
ATT+2++M'\
DTM+329:760502'\
LOC+178+VIE'\
LOC+179+BCN'\
NAT+2+CZE'\
RFF+AVF:Z57L3'\
DOC+P:110:111+34356146'\
LOC+91+RUS'\
CNT+42:3'\
UNT+39+1'\
UNE+1+1'\
UNZ+1+";
    dueResult << InterchangeReferenceTst << "'";

    // Сгенерированный текст
    std::cout << "Test1 Text: " << std::endl << text << std::endl;

    // Ожидаемый текст
    //std::cout << "DueText: " << std::endl << dueResult.str() << std::endl;

    // Сравниваем
    if ( dueResult.str() != text )
    {
        fail( "telegram bodies are not equivalent" );
    }

}
END_TEST;


START_TEST( czech_file_test2 )
{
    Paxlst::PaxlstInfo paxlstInfo;

    paxlstInfo.partyName = "CDGKOAF";

    paxlstInfo.senderName = "1H";

    Paxlst::PassengerInfo passInfo1;
    passInfo1.passengerSurname = "STRANSKY";


    paxlstInfo.passangersList.push_back( passInfo1 );

    std::string text = "", errText = "";
    if ( !paxlstInfo.toEdiString( text, errText ) )
    {
        fail( errText.c_str() );
    }


    std::stringstream dueResult; // Ожидаемый текст
    dueResult << "UNA:+.? " << "'";
    dueResult << "UNB+UNOA:4+1H+CZAPIS+";
    dueResult << PrepareDateTst << ":" << PrepareHourTst << "+";
    dueResult << InterchangeReferenceTst;
    dueResult << "++APIS'\
UNG+PAXLST+1H+CZAPIS+";
    dueResult << PrepareDateTst << ":" << PrepareHourTst;
    dueResult << "+1+UN+D:02B'\
UNH+1+PAXLST:D:02B:UN:IATA++01:C'\
BGM+745'\
NAD+MS+++CDGKOAF'\
TDT+20'\
LOC+125'\
DTM+189::201'\
LOC+87'\
DTM+232::201'\
NAD+FL+++STRANSKY'\
ATT+2'\
DTM+329'\
CNT+42:1'\
UNT+15+1'\
UNE+1+1'\
UNZ+1+";
    dueResult << InterchangeReferenceTst << "'";

    // Сгенерированный текст
    std::cout << "Test2 Text: " << std::endl << text << std::endl;

    // Ожидаемый текст
    //std::cout << "DueText: " << std::endl << dueResult.str() << std::endl;

    // Сравниваем
    if ( dueResult.str() != text )
    {
        fail( "telegram bodies are not equivalent" );
    }

}
END_TEST;


START_TEST( czech_file_test3 )
{
    std::string edi_paxlst_file_name =
        Paxlst::CreateEdiPaxlstFileName( "OK0421", "CAI", "PRG", "20070907", "TXT" );

    if ( edi_paxlst_file_name != "OK0421CAIPRG20070907.TXT" )
    {
        fail( "CreateEdiPaxlstFileName() failed" );
    }
}
END_TEST;


START_TEST( czech_file_test4 )
{
    std::string edi_paxlst_file_name =
            Paxlst::CreateIATACode( "OK0012", "070915", "1210" );

    if ( edi_paxlst_file_name != "OK0012/070915/1210" )
    {
        fail( "CreateIATACode() failed" );
    }
}
END_TEST;


START_TEST( czech_file_test5 )
{
    BASIC::TDateTime depDate;
    BASIC::StrToDateTime( "2007.09.07", "yyyy.mm.dd", depDate );

    std::string edi_paxlst_file_name = "";

    if ( !Paxlst::CreateEdiPaxlstFileName( edi_paxlst_file_name,
            "OK0421", "CAI", "PRG", depDate, "TXT" ) )
    {
        fail( "CreateEdiPaxlstFileName() return false" );
    }

    if ( edi_paxlst_file_name != "OK0421CAIPRG20070907.TXT" )
    {
        fail( "CreateEdiPaxlstFileName() failed" );
    }
}
END_TEST;


START_TEST( czech_file_test6 )
{
    BASIC::TDateTime destDate;
    BASIC::StrToDateTime( "2007.09.15 12:10", "yyyy.mm.dd hh:nn", destDate );

    std::string iataCode = "";

    if ( !Paxlst::CreateIATACode( iataCode, "OK0012", destDate ) )
    {
        fail( "CreateIATACode() return false" );
    }

    if ( iataCode != "OK0012/070915/1210" )
    {
        fail( "CreateIATACode() failed" );
    }
}
END_TEST;


#define SUITENAME "czech_file"
TCASEREGISTER( init, tear_down)
{
    ADD_TEST( czech_file_test1 );
    ADD_TEST( czech_file_test2 );
    ADD_TEST( czech_file_test3 );
    ADD_TEST( czech_file_test4 );
    ADD_TEST( czech_file_test5 );
    ADD_TEST( czech_file_test6 );
}
TCASEFINISH;




#endif /*XP_TESTING*/
