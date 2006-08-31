#include <stdlib.h>
#include "seats.h"
#include "basic.h"
#define NICKNAME "DJEK" 
#include "setup.h" 
#include "test.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "str_utils.h"
#include "salons.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

const int PR_N_PLACE = 9;
const int PR_REMPLACE = 8;
const int PR_SMOKE = 7;

const int PR_REM_TO_REM = 100;
const int PR_REM_TO_NOREM = 90;
const int PR_EQUAL_N_PLACE = 50;
const int PR_EQUAL_REMPLACE = 20;
const int PR_EQUAL_SMOKE = 10;

const int MAXPLACE = 3;
  
typedef vector<int> TLine;

struct TLinesSalon {
  TLine lines;
  TPlaceList *placeList;
};

typedef vector<TLinesSalon> TLines;
enum TSeatAlg { sSeatGrpOnBasePlace, sSeatGrp, sSeatPassengers, seatLength };
enum TUseRem { sAllUse, sOnlyUse, sMaxUse, sNotUse, sIgnoreUse, useremLength };
/* ����� ࠧ������ ���, ����� ᠦ��� �� ������ ����� ������ ࠧ�, �� ����� */
enum TUseAlone { uFalse3, uFalse1, uTrue }; 

enum TClearPlaces { clNotFree, clStatus, clBlock };
typedef BitSet<TClearPlaces> FlagclearPlaces;

void ClearPlaces( FlagclearPlaces clPl );

inline bool LSD( int G3, int G2, int G, int V3, int V2, TWhere Where );


  
/* �������� ��६���� ��� �⮣� ����� */  
TSeatPlaces SeatPlaces;

TLines lines[2];

TSalons *CurrSalon;

bool CanUseStatus; /* ���� �� ������ ���� */
string PlaceStatus; /* ᠬ ����� */
bool CanUseSmoke; /* ���� ������ ���� */
bool CanUseElem_Type; /* ���� ���� �� ⨯� (⠡��⪠) */
string PlaceElem_Type; /* ᠬ ⨯ ���� */
bool CanUseGood; /* ���� ⮫쪮 㤮���� ���� */
TUseRem CanUseRems; /* ���� �� ६�થ */
vector<string> Remarks; /* ᠬ� ६�ઠ */
bool CanUseTube; /* ���� �१ ��室� */
TUseAlone CanUseAlone; /* ����� �� �ᯮ�짮���� ��ᠤ�� ������ � ��� - ����� ��ᠤ��� 
                          ��㯯� ��� �� ��㣮� */
TSeatAlg SeatAlg;


TCounters::TCounters()
{
  Clear();
}

void TCounters::Clear()
{
  p_Count_3G = 0;
  p_Count_2G = 0;
  p_CountG = 0;
  p_Count_3V = 0;
  p_Count_2V = 0;  	
}

int TCounters::p_Count_3( TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    return p_Count_3G;
  else 
    return p_Count_3V;	
}

int TCounters::p_Count_2( TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    return p_Count_2G;
  else
    return p_Count_2V;	
}

int TCounters::p_Count( TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    return p_CountG;
  else 
    return 0;	
}

void TCounters::Set_p_Count_3( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_Count_3G = Count;
  else 
    p_Count_3V = Count;	
}

void TCounters::Set_p_Count_2( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_Count_2G = Count;
  else 
    p_Count_2V = Count;	
}

void TCounters::Set_p_Count( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_CountG = Count;	
}

void TCounters::Add_p_Count_3( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_Count_3G += Count;
  else 
    p_Count_3V += Count;	  	
}

void TCounters::Add_p_Count_2( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_Count_2G += Count;
  else 
    p_Count_2V += Count;		
}

void TCounters::Add_p_Count( int Count, TSeatStep Step )
{
  if ( Step == sRight || Step == sLeft )
    p_CountG += Count;		
}

TSeatPlaces::TSeatPlaces()
{
  Clear();
}

TSeatPlaces::~TSeatPlaces()
{
  Clear();
}

void TSeatPlaces::Clear()
{
  seatplaces.clear();
  counters.Clear();
}

/* �⪠� ��� ��������� ���� ��� ��ᠤ�� */
void TSeatPlaces::RollBack( int Begin, int End )
{
ProgTrace( TRACE5, "RollBack Begin=%d, End=%d", Begin, End );
  if ( seatplaces.empty() )
    return;
  tst();
  /* �஡�� �� ��������� �������� ���⠬ */
  TSeatPlace seatPlace;
  for ( int i=Begin; i<=End; i++ ) {
    seatPlace = seatplaces[ i ];
    tst();
    /* �஡�� �� ���� ��࠭���� ���⠬ */
    for ( vector<TPlace>::iterator place=seatPlace.oldPlaces.begin(); 
          place!=seatPlace.oldPlaces.end(); place++ ) {
      /* ����祭�� ���� */
      tst();
      int idx = seatPlace.placeList->GetPlaceIndex( seatPlace.Pos );
      tst();
      TPlace *pl = seatPlace.placeList->place( idx );
      tst();
      /* �⬥�� ��� ��������� ���� */
      pl->Assign( *place );
      tst();
      switch( seatPlace.Step ) {
      	case sRight: seatPlace.Pos.x++;
      	             break;
      	case sLeft:  seatPlace.Pos.x--;
      	             break;
      	case sDown:  seatPlace.Pos.y++;
      	             break;
      	case sUp:    seatPlace.Pos.y--;
      	             break;
      }
    } /* end for */
    tst();
    switch ( (int)seatPlace.oldPlaces.size() ) {
      case 3: counters.Add_p_Count_3( -1, seatPlace.Step );
              break;
      case 2: counters.Add_p_Count_2( -1, seatPlace.Step );
              break;
      case 1: counters.Add_p_Count( -1, seatPlace.Step );
              break;
      default: throw Exception( "�訡�� ��ᠤ��" );
    }
    seatPlace.oldPlaces.clear();
    tst();    
  } /* end for */
  tst();
  vector<TSeatPlace>::iterator b = seatplaces.begin();
  seatplaces.erase( b + Begin, b + End );
  tst();
}
/* �⪠� ��� ��������� ���� ��� ��ᠤ�� */
void TSeatPlaces::RollBack( )
{
  if ( seatplaces.empty() )
    return;
  RollBack( 0, seatplaces.size() - 1 );
}

void TSeatPlaces::Add( TSeatPlace &seatplace )
{
  seatplaces.push_back( seatplace );
}

/* ����ᨬ �������� ����.
  FP - 㪠��⥫� �� ��室��� ���� �� ���ண� ��稭����� ����
  EP - 㪠��⥫� �� ��ࢮ� ��������� ����
  FoundCount - ���-�� ��������� ����
  Step - ���ࠢ����� ����� ��������� ����
  �����頥� ���-�� �ᯮ�짮������ ���� */
int TSeatPlaces::Put_Find_Places( TPoint FP, TPoint EP, int foundCount, TSeatStep Step )
{
  int p_RCount, p_RCount2, p_RCount3; /* ����室���� ���-�� 3-�, 2-�, 1-� ���� */
  int pp_Count, pp_Count2, pp_Count3; /* ����饥�� ���-�� 3-�, 2-�, 1-� ���� */
  int NTrunc_Count, Trunc_Count; /* ���-�� �뤥������ �� ��饣� �᫠ ������ ���� */
  int p_Prior, p_Next; /* ���-�� ���� �� FP � ��᫥ ���� */
  int p_Step; /* ���ࠢ����� ��ᠤ��. ��।������ � ����ᨬ��� �� ���-�� p_Prior � p_Next */
  int Need;
  TPlaceList *placeList;
  int Result = 0; /* ��饥 ���-�� ������⢮������ ���� */
  if ( foundCount == 0 )
   return Result; // �� ������ ����
  placeList = CurrSalon->CurrPlaceList();
  ProgTrace( TRACE5, "FP(%d,%d), placeList=%p", FP.x, FP.y, placeList );
  switch( (int)Step ) {
    case sLeft:
    case sRight:
           pp_Count = counters.p_Count( sRight );
           pp_Count2 = counters.p_Count_2( sRight );
           pp_Count3 = counters.p_Count_3( sRight );
           p_Prior = abs( FP.x - EP.x );
           break;
    default:
           pp_Count = counters.p_Count( sDown );
           pp_Count2 = counters.p_Count_2( sDown );
           pp_Count3 = counters.p_Count_3( sDown );
           p_Prior = abs( FP.y - EP.y ); // ࠧ��� �/� FP � EP        
  }
  tst();
  p_RCount = Passengers.counters.p_Count( Step ) - pp_Count;
  p_RCount2 = Passengers.counters.p_Count_2( Step ) - pp_Count2;
  p_RCount3 = Passengers.counters.p_Count_3( Step ) - pp_Count3;
  p_Next = foundCount - p_Prior;
  p_Step = 0;
  NTrunc_Count = foundCount;
  tst();
  while ( foundCount > 0 && p_RCount + p_RCount2 + p_RCount3 > 0 ) {
    switch ( NTrunc_Count ) {
      case 1: if ( p_RCount > 0 ) /* �㦭� 1-� ����� ���� ?*/
                Trunc_Count = 1;
              else
                Trunc_Count = 0; 
              break;
      case 2: if ( p_RCount2 > 0 ) /* �㦭� 2-� ����� ���� ?*/
                Trunc_Count = 2;
              else
                Trunc_Count = 1;
              break;
      case 3: if ( p_RCount3 > 0 ) /* �㦭� 3-� ����� ���� ?*/
                Trunc_Count = 3; /* �㦭� 3-� ����� ���� */
              else
                Trunc_Count = 2; /* �㦭� 2-� ����� ���� ? */
              break;
      default: Trunc_Count = 3;
    }
    if ( !Trunc_Count ) /* ����� ��祣� �� �㦭� �� ⮣�, �� �।�������� */
      break;
    ProgTrace( TRACE5, "Trunc_Count=%d", Trunc_Count );
    if ( Trunc_Count != NTrunc_Count ) {
      NTrunc_Count = Trunc_Count; /* �뤥���� ������� ����, � � ⠪�� ������ �� �㦭� */
      continue;
    }
    ProgTrace( TRACE5, "Trunc_Count=%d", Trunc_Count );
   /* �᫨ �� �����, � ⮣�� �� ����� ����, ����� ��� ���ࠨ���� �� ���-��
      ⥯��� ��। ���� �⮨� �����, � ������ ���� ����� ��ᠤ��, �᫨ ���� �����,
      祬 �� ᥩ�� �뤥���?
      �⢥�: ��� ⮣�, �⮡� �������� ���� ������� � ��室��� �窥 FP
      ��筥� � ⠪��� ���, � ���ண� ����� ���� �� FP. */
    if ( !p_Step ) { /* �ਧ��� ⮣�, �� �� ���� �� ��।����� � ������ ��� ���� ��稭��� */
      Need = p_RCount; /* �� �㦤����� � ⠪�� ���-�� 1-� ���� */
      switch( Trunc_Count ) {
        case 3: Need += p_RCount3*3 + p_RCount2*2; /* �᫨ ���� 3, � ���뢠�� �� */
                break;
        case 2: Need += p_RCount2*2; /* �᫨ ���� 2 => � �� ���� 3-� ��� ��饥 ���-�� ���� �� ��������, */
                break;               /* ⮣�� ���� ���뢠�� ⮫쪮 1-� � 2-� ���� */
      }
      /* Need - �� ��饥 �㦭�� ���-�� ����, ���஥ �������� 㤠����� �뤥����
         ⥯��� ��।��塞 ���ࠢ����� */
      tst();
      if ( p_Prior >= p_Next && Need > p_Next ) { /* ᫥�� ����� ����, ��筥� �ࠢ� */
        p_Step = -1;
        if ( p_Next < Need ) /* ���ࠢ�/���� ����� ����, 祬 �������� ����������� */
          Need = p_Next; /* ⮣�� ����� �� �� ����� ���ॡ�����! */
        switch( (int)Step ) {
          case sRight:
          case sLeft:
             EP.x = FP.x + Need - 1; /* ��६�頥��� �� ��室��� ������ */
             Step = sLeft; /* ��稭��� ��������� ����� */
             break;             
          case sDown:
          case sUp:
             EP.y = FP.y + Need - 1; /* ��६�頥��� �� ��室��� ������ */
             Step = sUp; /* ��稭��� ��������� ����� */
             break;
        }
      }
      else {
       p_Step = 1;  /* ��筥� ᫥�� */
       if ( Need <= p_Next )
         Need = 0; /* ��祣� �� �ண��� � ��稭��� � ⥪�饩 ����樨 */
       else
        if ( p_Prior < Need )
          Need = p_Prior;
        switch( (int)Step ) {
       	  case sRight:
       	  case sLeft:
       	     EP.x = FP.x - Need; /* ��६�頥��� �� ��室��� ������ */
             Step = sRight; /* ��稭��� ��������� ��ࠢ� */
             break;
          case sDown:
          case sUp:
             EP.y = FP.y - Need; /* ��६�頥��� �� ��室��� ������ */
             Step = sDown; /* ��稭��� ��������� ���� */
             break;
        }       	
      } /* ����� ��।������ ���ࠢ����� ( p_Prior >= p_Next ... ) */
    } /* ����� p_Step = 0 */
    tst();
    /* �뤥�塞 �� ���� ���� ��� ����� ���� */
    TSeatPlace seatplace;
    seatplace.placeList = placeList;    
    seatplace.Step = Step;
    seatplace.Pos = EP;
    switch( (int)Step ) {
      case sLeft:
         seatplace.Step = sRight;
         seatplace.Pos.x = seatplace.Pos.x - Trunc_Count + 1;
         break;
      case sUp:
         seatplace.Step = sDown;
         seatplace.Pos.y = seatplace.Pos.y - Trunc_Count + 1;
         break;
    }
    tst();
    /* ��࠭塞 ���� ���� � ����砥� ���� � ᠫ��� ��� ������ */
    for ( int i=0; i<Trunc_Count; i++ ) {
      TPlace place;
      TPlace *pl = placeList->place( EP );
      ProgTrace( TRACE5, "*pl=%p, EP(%d,%d)", pl, EP.x, EP.y );
      place.Assign( *pl );
      tst();
      if ( !place.pr_free || !place.isplace )
        throw Exception( "���ᠤ�� �믮����� �������⨬�� ������: �ᯮ�짮����� 㦥 ����⮣� ����" );
      seatplace.oldPlaces.push_back( place );
      pl->pr_free = false;
      switch( Step ) {
      	case sRight: 
      	   EP.x++;
      	   break;
      	case sLeft:
      	   EP.x--;
      	   break;
      	case sDown:
      	   EP.y++;
      	   break;
      	case sUp:
      	   EP.y--;
      	   break;      	 
      }
    } /* �����稫� ��࠭��� */
    Add( seatplace );
    switch( Trunc_Count ) {
      case 3:
         p_RCount3--;
         pp_Count3++;
         counters.Set_p_Count_3( pp_Count3, Step );
         break;
      case 2:
         p_RCount2--;
         pp_Count2++;
         counters.Set_p_Count_2( pp_Count2, Step );      
         break;
      case 1:
         p_RCount--;
         pp_Count++;
         counters.Set_p_Count( pp_Count, Step );
         break;      
    }
    Result += Trunc_Count; /* ���-�� 㦥 �ᯮ�짮������ ���� */
    foundCount -= Trunc_Count; /* ��饥 ���-�� �� �ᯮ�짮������ ���� */
    NTrunc_Count = foundCount; /* ���-�� ����, ����� ᥩ�� ����� ࠧ������� */
  } /* end while */
  ProgTrace( TRACE5, "Put_Find_Places, FP(%d,%d), EP(%d,%d), foundCount=%d, Step=%d, result=%d",
             FP.x, FP.y, EP.x, EP.y, foundCount, Step, Result );
  return Result;
}

/* ���� ���� �ᯮ�������� �冷� ��᫥����⥫쭮
   �����頥� ���-�� ��������� ����.
   FP - ���� ����, � ���ண� ���� �᪠��
   FoundCount - ���-�� 㦥 ��������� ����
   Step - ���ࠢ����� ���᪠
  �������� ��६����:
   CanUseStatus, PlaceStatus - ���� ��ண� �� ������ ����,
   CanUseSmoke - ���� ������ ����,
   CanUseElem_Type, PlaceElem_Type - ���� ��ண� �� ⨯� ���� (⠡��⪠),
   CanUseGood - ���� ⮫쪮 ���� ����,
   CanUseRem, PlaceRem - ���� ��ண� �� ६�થ ���� */
int TSeatPlaces::FindPlaces_From( TPoint FP, int foundCount, TSeatStep Step )
{
ProgTrace( TRACE5, "FindPlaces_From FP FP=(%d,%d), foundCount=%d, Step=%d",
                    FP.x, FP.y, foundCount, Step );
  int Result = 0;
  TPlaceList *placeList = CurrSalon->CurrPlaceList();
  if ( !placeList->ValidPlace( FP ) )
    return Result;
  TPoint EP = FP;
  TPlace *place = placeList->place( EP );
  vector<TRem>::iterator prem;
  vector<string>::iterator irem;
  while ( place->visible && place->pr_free && place->isplace && 
          place->clname == Passengers.clname &&
          !place->block &&
          Result + foundCount < MAXPLACE &&
          ( !CanUseStatus || place->status == PlaceStatus ) &&
          ( !CanUseSmoke || place->pr_smoke ) &&
          ( !CanUseElem_Type || place->elem_type == PlaceElem_Type ) &&
          ( !CanUseGood || !place->not_good ) ) {
    switch( (int)CanUseRems ) {
      case sOnlyUse:
         if ( place->rems.empty() || Remarks.empty() ) // ������ ��� �� ���� ᮢ��������
           return Result;
         for ( irem = Remarks.begin(); irem != Remarks.end(); irem++ ) {
           for ( prem = place->rems.begin(); prem != place->rems.end(); prem++ ) {
             if ( prem->rem == *irem && !prem->pr_denial )
               break;
           }
           if ( prem != place->rems.end() )
             break;
         }
         if ( irem == Remarks.end() )
           return Result;
         break;
      case sMaxUse:
      case sAllUse:
         if ( Remarks.size() != place->rems.size() )
           return Result;
         for ( irem = Remarks.begin(); irem != Remarks.end(); irem++ ) {
           for ( prem = place->rems.begin(); prem != place->rems.end(); prem++ ) {
             if ( prem->rem == *irem && !prem->pr_denial )
               break;
           }
           if ( prem == place->rems.end() )
             return Result;
         }
         break;
      case sNotUse:
         if ( !place->rems.empty() )
           return Result;
         break;
    } /* end switch */
    Result++; /* ��諨 �� ���� ���� */
    switch( Step ) {
      case sRight: 
         EP.x++;
         break;
      case sLeft:
         EP.x--;
         break;
      case sDown:
         EP.y++;
         break;
      case sUp:
         EP.y--;
         break;                  
    }
    if ( !placeList->ValidPlace( EP ) )
      break;
    place = placeList->place( EP );        
  } /* end while */
  return Result;
}


/* ���ᠤ�� ��㯯� � PlaceList, ��稭�� � ����樨 FP,
   � ���ࠢ����� Step ( ��ᬠ�ਢ����� ⮫쪮 sRight - ��ਧ��⠫쭠� ,
   � sDown - ���⨪��쭠� ��ᠤ�� => ��ᬠ�ਢ��� ⮫쪮 ���ᠦ�஢ ���. ��㯯�.
   �ਮ���� ���᪠ sRight: sRight, sLeft, sTubeRight (�१ ��室), sTubeLeft ).
   Wanted - �ᮡ� ��砩, ����� ���� ��ᠤ��� 2x2, � �� 3 � 1.
  �������� ��६����:
   CanUseTube - ���� �� Step = sRight �१ ��室�
   Alone - ��ᠤ��� ���� ���ᠦ�� � ��� ����� ⮫쪮 ���� ࠧ */
bool TSeatPlaces::SeatSubGrp_On( TPoint FP, TSeatStep Step, int Wanted )
{
  if ( Step == sLeft )
    Step = sRight;
  if ( Step == sUp )
    Step = sDown;
  ProgTrace( TRACE5, "SeatSubGrp_On FP=(%d,%d), Step=%d, Wanted=%d", FP.x, FP.y, Step, Wanted );
  int foundCount = 0; // ���-�� ��������� ���� �ᥣ�
  int foundBefore = 0; // ���-�� ��������� ���� �� � ��᫥ FP
  int foundTubeBefore = 0;
  int foundTubeAfter = 0;
  TPlaceList *placeList = CurrSalon->CurrPlaceList();
  if ( !Wanted && seatplaces.empty() && CanUseAlone != uFalse3 )
    Alone = true;// �� ���� ��室 �, ���� �ந��樠����஢��� ��. ��६����� Alone
  int foundAfter = FindPlaces_From( FP, 0, Step );
  ProgTrace( TRACE5, "foundAfter=%d", foundAfter );
  if ( !foundAfter )
    return false;
  foundCount += foundAfter;
  if ( foundAfter && Wanted ) { // �᫨ �� ��諨 ���� � ��� ���� Wanted
    if ( foundAfter > Wanted )
      foundAfter = Wanted;
    Wanted -= Put_Find_Places( FP, FP, foundAfter, Step );
    if ( Wanted <= 0 )
      return true; // �� �� ��諮��  	
  }
  TPoint EP = FP;
  if ( foundAfter < MAXPLACE ) {
    switch( (int)Step ) {
      case sRight:
         EP.x--;
         foundBefore = FindPlaces_From( EP, foundCount, sLeft );
         break;
      case sDown:
         EP.y--;
         foundBefore = FindPlaces_From( EP, foundCount, sUp );
         break;      
    }
  }
  ProgTrace( TRACE5, "foundBefore=%d", foundBefore );
  foundCount += foundBefore;
  if ( foundBefore && Wanted ) { // �᫨ �� ��諨 ���� � ��� ���� Wanted
    if ( foundBefore > Wanted )
      foundBefore = Wanted;
    switch( (int)Step ) {
      case sRight:
         EP.x -= foundBefore - 1;
         break;
      case sDown:
         EP.y -= foundBefore - 1; 
         break;
    }
    Wanted -= Put_Find_Places( FP, EP, foundBefore, Step );
    if ( Wanted <= 0 )
     return true; /* �� �� ��諮�� */
  }
 /* ����� ����⠥��� ���᪠�� �१ ��室, �� �᫮��� �� ���� �� ��ਧ��⠫� */
 tst();
  if ( CanUseTube && Step == sRight && foundCount < MAXPLACE ) {
    EP.x += foundAfter + 1; /* ��⠭���������� �� �।���������� ���� */
    if ( placeList->ValidPlace( EP ) ) {
      TPoint p( EP.x -1, EP.y );
      TPlace *place = placeList->place( p ); /* ��६ �।. ���� */
      if ( !place->visible ) {
        foundTubeAfter = FindPlaces_From( EP, foundCount, Step ); /* ���� ��᫥ ��室� */
        foundCount += foundTubeAfter; /* 㢥��稢��� ��饥 ���-�� ���� */
        if ( foundTubeAfter && Wanted ) { /* �᫨ �� ��諨 ���� � ��� ���� Wanted */
          if ( foundTubeAfter > Wanted )
            foundTubeAfter = Wanted;
           Wanted -= Put_Find_Places( EP, EP, foundTubeAfter, Step ); /* ���� ��ࠬ��� EP */
         /* �.�. �窠 ����� ������ ��室���� �� ��ࢮ� ���� ��᫥ ��室�, 
            ���� ࠡ���� �� �㤥� */
           if ( Wanted <= 0 )
             return true; /* �� �� ��諮�� */
        }        
      }
    }
    /* ����� ���� ������ �१ ��室 */
    if ( foundCount < MAXPLACE ) {
      EP.x -= foundBefore + 2; // ��⠭���������� �� �।���������� ����    	
      if ( placeList->ValidPlace( EP ) ) {
        TPoint p( EP.x + 1, EP.y );
        TPlace *place = placeList->place( p ); /* ��६ ᫥�. ���� */
        if ( !place->visible ) { /* ᫥���饥 ���� �� ����� => ��室 */
          foundTubeBefore = FindPlaces_From( EP, foundCount, sLeft );          
          foundCount += foundTubeBefore;          
          if ( foundTubeBefore && Wanted ) { /* �᫨ �� ��諨 ���� � ��� ���� Wanted */
            if ( foundTubeBefore > Wanted )
              foundTubeBefore = Wanted;
            EP.x -= foundTubeBefore - 1;
            Wanted -= Put_Find_Places( EP, EP, foundTubeBefore, sLeft ); /* ���� ��ࠬ��� EP */
            if ( Wanted <= 0 )
              return true; /* �� �� ��諮�� */          
          }
        }
      }
    }
  } // end of found tube
  if ( Wanted )
    return false;
  /* �᫨ �� �����, � Wanted = 0 ( ����砫쭮 ) � 
     �� ����� ���-�� ���� ��������� ᫥��, �ࠢ� ... */
  int EndWanted = 0; /* �ਧ��� ⮣�, �� ���� �㤥� 㤠���� ���� ��譥� ���� �� ��������� */
  if ( Step == sRight && foundCount == MAXPLACE &&
       counters.p_Count_3() == Passengers.counters.p_Count_3() &&
       counters.p_Count_2() == Passengers.counters.p_Count_2() &&
       counters.p_Count() == Passengers.counters.p_Count() - MAXPLACE - 1 ) {
    /* ���� ���஡����� ��ᠤ��� �� ᫥���騩 �� �� ������ 祫-�� � 2-� */
    EP = FP;
    EP.y++;
    if ( EP.y >= placeList->GetYsCount() || !SeatSubGrp_On( EP, Step, 2 ) )
      return false; /* �� ᬮ��� ��ᠤ��� 2-� �� ᫥���騩 �� */
    else {
      EndWanted = seatplaces.size(); /* ��諨 2 ���� �� ᫥���饬 ��� � �������� �� � FPlaces */
      /* ������� ��� ���稪� �� ���� ���� */
      counters.Add_p_Count( -1 );
    }
  }
  tst();
  /* ��稭��� ���������� ����祭�� ���� */
  EP = FP;
  switch( (int)Step ) {
    case sRight:
       EP.x -= foundBefore;
       break;
    case sDown:
       EP.y -= foundBefore; 
       break;
  }
  tst();
  foundCount = Put_Find_Places( FP, EP, foundBefore + foundAfter, Step );
  tst();
  if ( !foundCount )
    return false;
  if ( foundTubeAfter ) {
    EP = FP;
    switch( (int)Step ) {
      case sRight:
         EP.x += foundAfter + 1;
         break;
      case sDown:
         EP.y += foundAfter + 1;
         break;
    }
    foundCount +=  Put_Find_Places( EP, EP, foundTubeAfter, Step );
  }
  if ( foundTubeBefore ) {
    EP = FP;
    switch( (int)Step ) {
      case sRight:
         EP.x -= foundTubeBefore + foundBefore + 1;
         break;
      case sDown:
         EP.y += foundTubeBefore + foundBefore + 1;
         break;      
    }
    foundCount += Put_Find_Places( EP, EP, foundTubeBefore, Step );
  }
  tst();
  if ( EndWanted > 0 ) { /* ���� 㤠���� ���� ᠬ�� ���宥 ����: */
    /* ᠬ�� 㤠������ �� ��᫥����� ���������� � ⥪�饬 ��� */
    tst();
    EP.x = placeList->GetPlaceIndex( FP );
    EP.y = 0;
    int k = seatplaces.size();
    for ( int i=EndWanted; i<k; i++ ) {
      int m = abs( EP.x - placeList->GetPlaceIndex( seatplaces[ i ].Pos ) );
      if ( EP.y < m ) {
        EP.y = m;
        EndWanted = i;
      }
    }
    /* ��諨 ᠬ�� 㤠������ ���� � ᥩ�� 㤠��� ��� */
    RollBack( EndWanted, EndWanted );
    counters.Add_p_Count( 1 );
    return true; /* �� ��諨 ��室�� */
  }
  /* ⥯��� ���㤨� ᫥���騩 ��ਠ��: � ⥪�饬 ��� ��諨 �ᥣ� ���� ����.
     ����� ⠪�� �����⨬� ⮫쪮 ������� */
  tst();
  if ( foundCount == 1 && CanUseAlone != uTrue ) 
    if ( Alone )
      Alone = false;
    else  /* ����� 2 ࠧ� �⮡� ��﫮�� ���� ���� � ���. */
      return false; /*( p_Count_3( Step ) = Passengers.p_Count_3( Step ) )AND
               ( p_Count_2( Step ) = Passengers.p_Count_2( Step ) )AND
               ( p_Count( Step ) = Passengers.p_Count( Step ) ) ???};*/
  tst();
  if ( counters.p_Count_3( Step ) == Passengers.counters.p_Count_3( Step ) &&
       counters.p_Count_2( Step ) == Passengers.counters.p_Count_2( Step ) &&
       counters.p_Count( Step ) == Passengers.counters.p_Count( Step ) )
    return true;
  tst();
  /* ���室�� �� ᫥���騩 �� */
  EP = FP;
  switch( (int)Step ) {
    case sRight:
       EP.y++;
       break;
    case sDown:
       EP.x++;
       break;    
  }
  if ( EP.x >= placeList->GetXsCount() || EP.y >= placeList->GetYsCount() ||
       !SeatSubGrp_On( EP, Step, 0 ) ) // ��祣� �� ᬮ��� ���� �����
    return false;
  return true;
}

bool TSeatPlaces::SeatsStayedSubGrp( TWhere Where )
{
  /* �஢�ઠ � ��⠫��� �� ���� ��㯯�, ��� ��㯯� ��⮨� �� ������ 祫����� ? */
  if ( counters.p_Count_3( sRight ) == Passengers.counters.p_Count_3( sRight ) &&
       counters.p_Count_2( sRight ) == Passengers.counters.p_Count_2( sRight ) &&
       counters.p_Count( sRight ) == Passengers.counters.p_Count( sRight ) &&
       counters.p_Count_3( sDown ) == Passengers.counters.p_Count_3( sDown ) &&
       counters.p_Count_2( sDown ) == Passengers.counters.p_Count_2( sDown ) &&
       counters.p_Count( sDown ) == Passengers.counters.p_Count( sDown ) )
    return true;
  TPoint EP;
  /* ���� ��ᠤ��� ��⠢����� ���� ���ᠦ�஢ */
  /* ��� �⮣� ���� ����� ��࠭�� ���� ����� � ���஡����� ��ᠤ���
    ��⠢����� ��㯯� �冷� � 㦥 ��ᠦ����� */
  if ( Where != sUpDown )
  for (ISeatPlace isp = seatplaces.begin(); isp!= seatplaces.end(); isp++ ) {
    CurrSalon->SetCurrPlaceList( isp->placeList );
    for( vector<TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
      /* ���饬 �ࠢ� �� ���������� ���� */
      switch( (int)isp->Step ) {
        case sRight: 
           EP.x = isp->Pos.x + isp->oldPlaces.size();
           break;
        case sLeft: 
           EP.x = isp->Pos.x + 1;
           break;
        case sDown: 
           EP.y = isp->Pos.y + distance( isp->oldPlaces.begin(), ipl );
           break;
        case sUp: 
           EP.y = isp->Pos.y - distance( isp->oldPlaces.begin(), ipl );
           break;
      }
      switch( (int)isp->Step ) {
        case sRight:
        case sLeft: 
           EP.y = isp->Pos.y;
           break;
        case sDown:
        case sUp: 
           EP.x = isp->Pos.x + 1;
           break;
      }
      Alone = true; /* ����� ���� ���� ���� */
      if ( SeatSubGrp_On( EP, sRight, 0 ) )
        return true;
      TPlaceList *placeList = CurrSalon->CurrPlaceList();
      TPoint p( EP.x + 1, EP.y );
      if ( CanUseTube && placeList->ValidPlace( p ) &&
           !placeList->place( EP )->visible ) {
       /* ����� ���஡����� �᪠�� �१ ��室 */
       Alone = true; /* ����� ���� ���� ���� */
       if ( SeatSubGrp_On( p, sRight, 0 ) )
         return true;
      }
      /* ���饬 ᫥�� �� ���������� ���� */
      switch( (int)isp->Step ) {
        case sRight: 
           EP.x = isp->Pos.x - 1;
           break;
        case sLeft: 
           EP.x = isp->Pos.x - isp->oldPlaces.size();
           break;
        case sDown:
        case sUp: 
           EP.x = isp->Pos.x - 1;
           break;
      }
      Alone = true; /* ����� ���� ���� ���� */
      if ( SeatSubGrp_On( EP, sRight, 0 ) )
        return true;
      placeList = CurrSalon->CurrPlaceList();          
      p.x = EP.x - 1;
      p.y = EP.y;
      if ( CanUseTube &&
           placeList->ValidPlace( p ) &&
           !placeList->place( EP )->visible ) {
        /* ����� ���஡����� �᪠�� �१ ��室 */
        Alone = true; /* ����� ���� ���� ���� */
        p.x = EP.x - 1;
        p.y = EP.y;
        if ( SeatSubGrp_On( p, sRight, 0 ) )
          return true;
      }
      if ( isp->Step == sLeft || isp->Step == sRight )
        break;
    } /* end for */
    if ( Where != sLeftRight )
    /* ���饬 ᢥ��� � ᭨�� �� ���������� ���� */
    for( vector<TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
      switch( isp->Step ) {
        case sRight: 
           EP.x = isp->Pos.x + distance( isp->oldPlaces.begin(), ipl );
           break;
        case sLeft: 
           EP.x = isp->Pos.x - distance( isp->oldPlaces.begin(), ipl );
           break;
        case sDown: 
           EP.y = isp->Pos.y + isp->oldPlaces.size();
           break;
        case sUp: 
           EP.y = isp->Pos.y - isp->oldPlaces.size();
           break;
      }
      /* ��ᠤ�� ᢥ��� �� ������� ���� */
      switch( isp->Step ) {
        case sRight:
        case sLeft: 
           EP.y = isp->Pos.y - 1;
           break;
        case sDown:
        case sUp: 
           EP.x = isp->Pos.x;
           break;
      }
      if ( SeatSubGrp_On( EP, sRight, 0 ) )
        return true;
      /* ⥯��� ��ᬮ�ਬ ᭨�� */
      switch( isp->Step ) {
        case sRight:
        case sLeft: 
           EP.y = isp->Pos.y + 1;
           break;
        case sDown: 
           EP.y = isp->Pos.y + isp->oldPlaces.size();
           break;
        case sUp: 
           EP.y = isp->Pos.y - 1;
           break;
      }
      if ( SeatSubGrp_On( EP, sRight, 0 ) )
        return true;
      if ( isp->Step == sUp || isp->Step == sDown )
        break;
    } /* end for */
  } /* end for */
  return false;  
}

TSeatPlace &TSeatPlaces::GetEqualSeatPlace( TPassenger &pass )
{
  int MaxEqualQ = -10000;
  ISeatPlace misp; 
  for (ISeatPlace isp=seatplaces.begin(); isp!=seatplaces.end(); isp++) {
    if ( isp->InUse || (int)isp->oldPlaces.size() != pass.countPlace ) /* ���-�� ���� ������ ᮢ������ */
      continue;    
    int EqualQ = 0;
    if ( (int)isp->oldPlaces.size() == pass.countPlace )
      EqualQ = pass.countPlace*10000; //??? always true!
    for (vector<string>::iterator irem=pass.rems.begin(); irem!= pass.rems.end(); irem++ ) {
      for( vector<TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
      	/* �஡�� �� ���⠬ ����� ����� �������� ���ᠦ�� */
      	vector<TRem>::iterator itr;
      	for ( itr=ipl->rems.begin(); itr!=ipl->rems.end(); itr++ ) {
      	  if ( *irem == itr->rem ) {
      	    if ( !itr->pr_denial )
      	      EqualQ += PR_REM_TO_REM;
      	    else
      	      EqualQ -= PR_REM_TO_REM;
      	    break;
      	  }
      	}
        if ( itr == ipl->rems.end() ) /* �� ��諨 ६��� � ���� */
          EqualQ += PR_REM_TO_NOREM;      	
      } /* ����� �஡��� �� ���⠬ */    	
    } /* ����� �஡��� �� ६�ઠ� ���ᠦ�� */
    TPlaceList *placeList = isp->placeList;
    if ( pass.placeName == placeList->GetPlaceName( isp->Pos ) )
      EqualQ += PR_EQUAL_N_PLACE;
    if ( pass.placeRem.find( "SW" ) == 2  &&
        ( isp->Pos.x == 0 || isp->Pos.x == placeList->GetXsCount() - 1 ) ||
         pass.placeRem.find( "SA" ) == 2  &&
        ( isp->Pos.x - 1 >= 0 && 
          placeList->GetXsName( isp->Pos.x - 1 ).empty() ||
          isp->Pos.x + 1 < placeList->GetXsCount() &&
          placeList->GetXsName( isp->Pos.x + 1 ).empty() ) 
       )
      EqualQ += PR_EQUAL_REMPLACE;
    if ( pass.prSmoke == isp->oldPlaces.begin()->pr_smoke )
      EqualQ += PR_EQUAL_SMOKE;
    if ( MaxEqualQ < EqualQ ) {
      MaxEqualQ = EqualQ;
      misp = isp;
    }
  } /* end for */
 misp->InUse = true; 
 return *misp;
}

void TSeatPlaces::PlacesToPassengers()
{
  if ( seatplaces.empty() )
   return;
  for (ISeatPlace isp=seatplaces.begin(); isp!=seatplaces.end(); isp++)
    isp->InUse = false;
  int lp = Passengers.getCount();
  for ( int i=0; i<lp; i++ ) {
    TPassenger &pass = Passengers.Get( i );
    TSeatPlace &seatPlace = GetEqualSeatPlace( pass );
    if ( seatPlace.Step == sLeft || seatPlace.Step == sUp )
      throw Exception( "�������⨬�� ���祭�� ���ࠢ����� ��ᠤ��" );
    pass.placeList = seatPlace.placeList;
    pass.Pos = seatPlace.Pos;
    pass.Step = seatPlace.Step;
    pass.placeName = seatPlace.placeList->GetPlaceName( seatPlace.Pos );
  }
}

/* ��ᠤ�� �ᥩ ��㯯� ��稭�� � ����樨 FP */
bool TSeatPlaces::SeatsGrp_On( TPoint FP  )
{
  /* ������ ����祭�� ���� */
  RollBack();
  /* �᫨ ���� ���ᠦ��� � ��㯯� � ���⨪��쭮� ��ᠤ���, � �஡㥬 �� ��ᠤ��� */
  if ( Passengers.counters.p_Count_3( sDown ) + Passengers.counters.p_Count_2( sDown ) > 0 ) {
    if ( !SeatSubGrp_On( FP, sDown, 0 ) ) { /* �� ����砥��� */
      RollBack( );
      return false;
    }
    /* �᫨ ��� ��㣨� ���ᠦ�஢, � ⮣�� ��ᠤ�� �믮����� �ᯥ譮 */
    if ( Passengers.counters.p_Count_3() + 
         Passengers.counters.p_Count_2() + 
         Passengers.counters.p_Count() == 0 )
      return true;
  }
  else {
    if ( SeatSubGrp_On( FP, sRight, 0 ) )
      return true;
    if ( CanUseAlone != uTrue ) {
      RollBack( );
      return false;
    }
  }
  if ( seatplaces.empty() )
    return false;
  /* �᫨ �� ����� � ᬮ��� ��ᠤ��� ���� ��㯯�
     ��ᠦ����� ��⠢����� ���� ��㯯� */
  /* ����� ��������� ��ᠤ��� � ᢥ��� � ᭨�� �� ������� ���� */
  if ( SeatsStayedSubGrp( sEveryWhere ) ) 
    return true;
  RollBack( );
  return false;
}

bool TSeatPlaces::SeatsPassenger_OnBasePlace( string &placeName, TSeatStep Step )
{
ProgTrace( TRACE5, "SeatsPassenger_OnBasePlace( )" );
	
  bool OldCanUseGood = CanUseGood;
  bool OldCanUseSmoke = CanUseSmoke;
  bool CanUseGood = false;
  bool CanUseSmoke = false;
  string nplaceName = placeName;
  try {
    if ( !placeName.empty() ) {
      /* ��������� ����஢ ���� ���ᠦ�஢ � ����ᨬ��� �� ���. ��� ���. ᠫ��� */
      if ( placeName == CharReplace( nplaceName, LAT_NAME_LINES, RUS_NAME_LINES ) )
        CharReplace( nplaceName, RUS_NAME_LINES, LAT_NAME_LINES );
      if ( placeName == nplaceName )
        nplaceName.clear();
      for ( vector<TPlaceList*>::iterator iplaceList=CurrSalon->placelists.begin();
            iplaceList!=CurrSalon->placelists.end(); iplaceList++ ) {
        TPoint FP;
        if ( (*iplaceList)->GetisPlaceXY( placeName, FP ) ||
             (*iplaceList)->GetisPlaceXY( nplaceName, FP ) ) {
          CurrSalon->SetCurrPlaceList( *iplaceList );
          if ( SeatSubGrp_On( FP, Step, 0 ) ) {
            CanUseGood = OldCanUseGood;
            CanUseSmoke = OldCanUseSmoke;  	          	
            return true;
          }
          break;
        }
      }
    }
  }
  catch( ... ) {
    CanUseGood = OldCanUseGood;
    CanUseSmoke = OldCanUseSmoke;  	
    throw;
  }
  CanUseGood = OldCanUseGood;
  CanUseSmoke = OldCanUseSmoke;  	  
  return false;  
}

inline void getRemarks( TPassenger &pass )
{
  Remarks.clear();
  switch ( (int)CanUseRems ) {
    case sAllUse:
    case sOnlyUse: 
       Remarks.assign( pass.rems.begin(), pass.rems.end() );
       break;
    case sMaxUse: 
       if ( !pass.maxRem.empty() )
         Remarks.push_back( pass.maxRem );
       break;
  }
}

/* ���� � ����� ᠫ��� � �������� ��������� ��६�����.
   ��������� ���� CanUseRems � PlaceRemark */
bool TSeatPlaces::SeatGrpOnBasePlace( )
{
ProgTrace( TRACE5, "SeatGrpOnBasePlace( )" );
  int G3 = Passengers.counters.p_Count_3( sRight );
  int G2 = Passengers.counters.p_Count_2( sRight );
  int G = Passengers.counters.p_Count( sRight );
  int V3 = Passengers.counters.p_Count_3( sDown );
  int V2 = Passengers.counters.p_Count_2( sDown );
  TUseRem OldCanUseRems = CanUseRems;
  try {
    /* ���� ��室���� ���� ��� ���ᠦ�� */
    int lp = Passengers.getCount();
    for ( int i=0; i<lp; i++ ) {
      /* �뤥�塞 �� ��㯯� �������� ���ᠦ�� � ��室�� ��� ���� ���� */
      TPassenger &pass = Passengers.Get( i );
      Passengers.SetCountersForPass( pass );
      getRemarks( pass ); 
      tst(); 
      if ( SeatsPassenger_OnBasePlace( pass.placeName, pass.Step ) && /* ��諨 ������� ���� */
           LSD( G3, G2, G, V3, V2, sEveryWhere ) ) 
        return true;
      tst();
      RollBack();
      Passengers.SetCountersForPass( pass );
      if ( !Remarks.empty() ) { /* ���� ६�ઠ */
        // ����⠥��� ���� �� ६�થ
        for ( int Where=sLeftRight; Where<=sUpDown; Where++ ) { 
          /* ��ਠ��� ���᪠ ����� ���������� ���� */
          for ( int linesVar=0; linesVar<=1; linesVar++ ) {
            for( TLines::iterator ilines=lines[ linesVar ].begin();
                 ilines!=lines[ linesVar ].end(); ilines++ ) {
              CurrSalon->SetCurrPlaceList( ilines->placeList );
              int ylen = CurrSalon->CurrPlaceList()->GetYsCount();
              TPoint FP;
              for ( int y=0; y<ylen; y++ ) {
                for ( vector<int>::iterator z=ilines->lines.begin(); z!=ilines->lines.end(); z++ ) {
                  FP.x = *z;
                  FP.y = y; /* �஡�� �� ���⠬ */
                  /* ��ᠤ�� ᠬ��� ������� ���ᠦ�� */
                  if ( SeatSubGrp_On( FP, pass.Step, 0 ) && LSD( G3, G2, G, V3, V2, (TWhere )Where ) ) { 
                    ProgTrace( TRACE5, "SeatSubGrp_On return true" );
                    return true;
                  }
                  RollBack(); /* �� ����稫��� �⪠� ������� ���� */
                  Passengers.SetCountersForPass( pass ); /* �뤥�塞 ����� �⮣� ���ᠦ�� */
                }
              }
            }
          }
        }
      } /* ����� ���᪠ �� ६�થ */
    } /* end for */
  }
  catch( ... ) {
   Passengers.counters.Clear( );
   Passengers.counters.Add_p_Count_3( G3, sRight );
   Passengers.counters.Add_p_Count_2( G2, sRight );   
   Passengers.counters.Add_p_Count( G, sRight );      
   Passengers.counters.Add_p_Count_3( V3, sDown );
   Passengers.counters.Add_p_Count_2( V2, sDown );      
   CanUseRems = OldCanUseRems;  	
   throw;
  }
  Passengers.counters.Clear( );
  Passengers.counters.Add_p_Count_3( G3, sRight );
  Passengers.counters.Add_p_Count_2( G2, sRight );   
  Passengers.counters.Add_p_Count( G, sRight );      
  Passengers.counters.Add_p_Count_3( V3, sDown );
  Passengers.counters.Add_p_Count_2( V2, sDown );      
  CanUseRems = OldCanUseRems;  		
  RollBack();
  return false;
}

/* ��ᠤ�� ��㯯� �� �ᥬ ᠫ���� */
bool TSeatPlaces::SeatsGrp( )
{
ProgTrace( TRACE5, "SeatsGrp( )" );
 RollBack();
 for ( int linesVar=0; linesVar<=1; linesVar++ ) {
   for( TLines::iterator ilines=lines[ linesVar ].begin();
        ilines!=lines[ linesVar ].end(); ilines++ ) {
     CurrSalon->SetCurrPlaceList( ilines->placeList );
     int ylen = CurrSalon->CurrPlaceList()->GetYsCount();
     TPoint FP;
     for ( int y=0; y<ylen; y++ ) {
       for ( vector<int>::iterator z=ilines->lines.begin(); z!=ilines->lines.end(); z++ ) {
         FP.x = *z;
         FP.y = y; /* �஡�� �� ���⠬ */
         if ( SeatsGrp_On( FP ) ) {
           tst();
           return true;
         }
       }
     }
   }
 }
 RollBack();
 return false;
}

/* ��ᠤ�� ���ᠦ�஢ �� ���⠬ �� ���뢠� ��㯯� */
bool TSeatPlaces::SeatsPassengers( )
{	
ProgTrace( TRACE5, "SeatsPassengers( )" );	
  vector<TPassenger> npass;
  Passengers.copyTo( npass );
  try {
    for ( VPassengers::iterator ipass=npass.begin(); ipass!=npass.end(); ipass++ ) {
      if ( ipass->InUse )
        continue;
      ProgTrace( TRACE5, "ipass->placeName=%s", ipass->placeName.c_str() );
      Passengers.Clear();
      ipass->placeList = NULL;
      Passengers.Add( *ipass );
      if ( SeatGrpOnBasePlace( ) ||
           ( CanUseRems == sNotUse || CanUseRems == sIgnoreUse  ) && SeatsGrp( ) ) {
        ProgTrace( TRACE5, "seatplaces.size()=%d", seatplaces.size() );
        if ( seatplaces.begin()->Step == sLeft || seatplaces.begin()->Step == sUp )
          throw Exception( "�������⨬�� ���祭�� ���ࠢ����� ��ᠤ��" );
        tst();
        ipass->placeList = seatplaces.begin()->placeList;
        ipass->Pos = seatplaces.begin()->Pos;
        ipass->Step = seatplaces.begin()->Step;
        ipass->placeName = ipass->placeList->GetPlaceName( ipass->Pos );
        ipass->InUse = true;
        ProgTrace( TRACE5, "ipass->placeName=%s", ipass->placeName.c_str() );
        Clear();
        tst();
      }
    }
  }
  catch( ... ) {
    Passengers.Clear();
    Passengers.copyFrom( npass );
    throw;  	
  }
  tst();
  Passengers.Clear();
  tst();
  Passengers.copyFrom( npass );
  for ( VPassengers::iterator ipass=npass.begin(); ipass!=npass.end(); ipass++ ) {
    if ( !ipass->InUse )
      return false;
  }
  return true;
}

/*//////////////////////////////// CLASS TPASSENGERS ///////////////////////////////////*/
TPassengers::TPassengers()
{
 Clear();  
}

TPassengers::~TPassengers()
{
 Clear();	
}

void TPassengers::Clear()
{
  FPassengers.clear();
  KTube = false;  
  KWindow = false;
  clname.clear();
  this->UseSmoke = false;	
  counters.Clear();
}

void TPassengers::copyTo( VPassengers &npass )
{
  npass.clear();
  npass.assign( FPassengers.begin(), FPassengers.end() );
}

void TPassengers::copyFrom( VPassengers &npass )
{
  FPassengers.clear();
  FPassengers.assign( npass.begin(), npass.end() );
}

void TPassengers::addRemPriority( TPassenger &pass )
{
  int priority = 0;
  pass.maxRem.clear();  
  if ( remarks.empty() ) {
    TQuery Qry( &OraSession );
    Qry.SQLText = "SELECT cod, pr_comp FROM remark WHERE pr_comp IS NOT NULL";
    Qry.Execute();
    while ( !Qry.Eof ) {
      remarks[ Qry.FieldAsString( "cod" ) ] = Qry.FieldAsInteger( "pr_comp" );
      Qry.Next();
    }
  }	
  for ( vector<string>::iterator irem = pass.rems.begin(); irem != pass.rems.end(); irem++ ) {
    if ( remarks[ *irem ] > priority ) {
      priority = remarks[ *irem ];
      pass.maxRem = *irem;
    }
  }
  pass.priority += priority*pass.countPlace;
}

void TPassengers::Calc_Priority( TPassenger &pass)
{
  pass.priority = 0;
  if ( !pass.placeName.empty() )
    pass.priority = PR_N_PLACE;
  if ( !pass.placeRem.empty() )
    pass.priority += PR_REMPLACE;
  if ( pass.prSmoke )
    pass.priority += PR_SMOKE;
  addRemPriority( pass );
}


void TPassengers::Add( TPassenger &pass )
{
  if ( pass.countPlace > MAXPLACE || pass.countPlace <= 0 )
   throw Exception( "�� �����⨬�� ���-�� ���� ��� �ᠤ��" );
  bool Pr_PLC = false;	
  if ( pass.countPlace > 1 &&
       find( pass.rems.begin(), pass.rems.end(), string( "STCR" ) ) != pass.rems.end() ) {
    pass.Step = sDown;
    switch( pass.countPlace ) {
      case 2: counters.Add_p_Count_2( 1, sDown );
              break;
      case 3: counters.Add_p_Count_3( 1, sDown );
    }
    Pr_PLC = true;
  }
  if ( !Pr_PLC ) {
   pass.Step = sRight;
   switch( pass.countPlace ) {
     case 1: counters.Add_p_Count( 1, sRight );
             break;
     case 2: counters.Add_p_Count_2( 1, sRight );
             break;
     case 3: counters.Add_p_Count_3( 1, sRight );             
   }
  }
 // �����뢠�� �����  
  if ( clname.empty() && !pass.clname.empty() )
    clname = pass.clname;
 // �����뢠�� �ਮ���
  Calc_Priority( pass );  
  if ( pass.placeRem.find( "SW" ) == 2 )
    KWindow = true;
  if ( pass.placeRem.find( "SA" ) == 2 )
    KTube = true;
  if ( pass.placeRem.find( "SM" ) == 0 )
    this->UseSmoke = true;  
  vector<TPassenger>::iterator ipass;
  for ( ipass=FPassengers.begin(); ipass!=FPassengers.end(); ipass++ ) {
    if ( pass.priority > ipass->priority )
      break;
  }
  if ( ipass == FPassengers.end() )
    FPassengers.push_back( pass );
  else
    FPassengers.insert( ipass, pass );
}

int TPassengers::getCount()
{
  return FPassengers.size();
}

TPassenger &TPassengers::Get( int Idx )
{
  if ( Idx < 0 || Idx >= (int)FPassengers.size() )
    throw Exception( "Passeneger index out of range" );
  return FPassengers[ Idx ];
}

void TPassengers::SetCountersForPass( TPassenger  &pass )
{
  counters.Clear();
  switch( pass.countPlace ) {
    case 1: 
       counters.Add_p_Count( 1 );
       break;
    case 2: 
       counters.Add_p_Count_2( 1, pass.Step );
       break;
    case 3: 
       counters.Add_p_Count_3( 1, pass.Step );
       break;
  }
}

bool TSeatPlaces::LSD( int G3, int G2, int G, int V3, int V2, TWhere Where )
{
  tst();
  if ( SeatAlg == sSeatPassengers )
    return true;
  ProgTrace( TRACE5, "LSD G3=%d, G2=%d, G=%d, V3=%d, V2=%d, Where=%d",
             G3, G2, G, V3, V2, Where );
  /* �᫨ �� ����� � ⮣�� �� ᬮ��� ��ᠤ��� �������� 祫-�� �� ��㯯�
     ���஡㥬 ��ᠤ��� ��� ��⠫��� */
  Passengers.counters.Clear();
  Passengers.counters.Add_p_Count_3( G3, sRight );
  Passengers.counters.Add_p_Count_2( G2, sRight );
  Passengers.counters.Add_p_Count( G );
  Passengers.counters.Add_p_Count_3( V3, sDown );
  Passengers.counters.Add_p_Count_2( V2, sDown );
  TUseRem OldCanUseRems = CanUseRems;
  CanUseRems = sIgnoreUse;
  try {
    if ( SeatsStayedSubGrp( Where ) ) {
      CanUseRems = OldCanUseRems;
      return true;
    }
  }
  catch( ... ) {
    CanUseRems = OldCanUseRems;
    throw;
  }
  CanUseRems = OldCanUseRems;
  return false;
}


void GET_LINE_ARRAY( )
{
  lines[ 0 ].clear();
  lines[ 1 ].clear();
  Passengers.KWindow = ( Passengers.KWindow && !Passengers.KTube );
  Passengers.KTube = ( !Passengers.KWindow && Passengers.KTube );
  for ( vector<TPlaceList*>::iterator iplaceList=CurrSalon->placelists.begin();
        iplaceList!=CurrSalon->placelists.end(); iplaceList++ ) {
    int xlen = (*iplaceList)->GetXsCount();
    TLinesSalon linesSalonVar0, linesSalonVar1;
    int linesVar;
    for ( int x=0; x<xlen; x++ ) { 
      if ( (*iplaceList)->GetXsName( x ).empty() )
        continue;
      if ( Passengers.KWindow )
        if ( x == 0 || x == xlen - 1 )
          linesVar = 0;
        else
          linesVar = 1;
      else
        if ( Passengers.KTube ) {
          if ( x - 1 >= 0 && (*iplaceList)->GetXsName( x + 1 ).empty() )
            linesVar = 0;
          else
            linesVar = 1;
        }  
        else linesVar = 1;
      if ( linesVar == 0 )
        linesSalonVar0.lines.push_back( x );
      else
        linesSalonVar1.lines.push_back( x );
    } 
    linesSalonVar0.placeList = *iplaceList;
    linesSalonVar1.placeList = *iplaceList;    
    lines[ 0 ].push_back( linesSalonVar0 );
    lines[ 1 ].push_back( linesSalonVar1 );    
  }
}

void SetStatuses( vector<string> &Statuses,  string status, bool First )
{
  Statuses.clear();
  if ( status == "TR" )
    if ( First ) {    
      Statuses.push_back( "TR" );
      Statuses.push_back( "FP" );
      Statuses.push_back( "NG" );
    }
    else {
      Statuses.push_back( "BR" );
      Statuses.push_back( "RZ" );
    }
  else
    if ( status == "RZ" )
      if ( First ) {
        Statuses.push_back( "RZ" );
        Statuses.push_back( "FP" );
      }
      else {
        Statuses.push_back( "NG" );
        Statuses.push_back( "BR" );
      }
    else
      if ( status == "FP" )
        if ( First ) {
          Statuses.push_back( "FP" );
          Statuses.push_back( "NG" );
          Statuses.push_back( "BR" );
        }
        else {
          Statuses.push_back( "RZ" );
        }
      else
        if ( status == "BR" )
          if ( First ) {
            Statuses.push_back( "BR" );
            Statuses.push_back( "FP" );
            Statuses.push_back( "NG" );
          }
        else {
          Statuses.push_back( "RZ" );
        }
}

/*///////////////////////////END CLASS TPASSENGERS/////////////////////////*/


/* ��ᠤ�� ���ᠦ�஢ */
void SeatsPassengers( )
{
  if ( !Passengers.getCount() )
    return;
  vector<string> Statuses;
  CanUseStatus = true;
  CanUseSmoke = false; /* ���� �� �㤥� ࠡ���� � ����騬� ���⠬� */
  CanUseElem_Type = false; /* ���� �� �㤥� ࠡ���� � ⨯��� ���� */
  GET_LINE_ARRAY( );
  SeatAlg = sSeatGrpOnBasePlace;
  try {
    for ( int FSeatAlg=0; FSeatAlg<seatLength; FSeatAlg++ ) {
      SeatAlg = (TSeatAlg)FSeatAlg;
      for ( int FCanUseRems=0; FCanUseRems<useremLength; FCanUseRems++ ) {
        CanUseRems = (TUseRem)FCanUseRems;
        switch( (int)SeatAlg ) {
          case sSeatGrpOnBasePlace:
            if ( CanUseRems == sIgnoreUse )
              continue;
            break;
          case sSeatGrp:
             switch( (int)CanUseRems ) {
               case sAllUse:
               case sMaxUse:
               case sOnlyUse:
               case sIgnoreUse: 
                 continue; /*??? �� ������� ��㯯� ��� ���� � ६�ઠ��, ��� �� ���� ���뢠�� */
             }
             break;
        }
        /* ��⠢���� ������ ��᪮�쪮 ࠧ �� �鸞� �� ���ᠤ�� ��㯯� */
        for ( int FCanUseAlone=uFalse3; FCanUseAlone<=uTrue; FCanUseAlone++ ) { 
          CanUseAlone = (TUseAlone)FCanUseAlone;
          if ( CanUseAlone == uFalse3 &&
               !( !Passengers.counters.p_Count_3( sRight ) && 
                  !Passengers.counters.p_Count_2( sRight ) &&
                  Passengers.counters.p_Count( sRight ) == 3 &&
                  !Passengers.counters.p_Count_3( sDown ) &&
                  !Passengers.counters.p_Count_2( sDown )
                 ) )
            continue;
          if ( CanUseAlone == uTrue && SeatAlg == sSeatPassengers )
            continue;
          /* �ᯮ�짮����� ����ᮢ ���� */
          for ( int KeyStatus=1; KeyStatus>=0; KeyStatus-- ) {
            if ( !KeyStatus && CanUseAlone == uFalse3 )
              continue;
            /* ��� ��室�� */
            for ( int FCanUseTube=0; FCanUseTube<=1; FCanUseTube++ ) {
              /* ��� ��ᠤ�� �⤥���� ���ᠦ�஢ �� ���� ���뢠�� ��室� */
              if ( FCanUseTube && SeatAlg == sSeatPassengers ) 
                continue;
              if ( FCanUseTube && CanUseAlone == uFalse3 )
                continue; 
              CanUseTube = FCanUseTube;
              for ( int FCanUseSmoke=Passengers.UseSmoke; FCanUseSmoke>=0; FCanUseSmoke-- ) {
                if ( !FCanUseSmoke && CanUseAlone == uFalse3 )
                  continue;
                CanUseSmoke = FCanUseSmoke;
                SetStatuses( Statuses, Passengers.Get( 0 ).placeStatus, KeyStatus );
                /* �஡�� �� ����ᮬ */
                for ( vector<string>::iterator st=Statuses.begin(); st!=Statuses.end(); st++ ) {
                  PlaceStatus = *st;
                  CanUseGood = ( PlaceStatus != "NG" ); /* ��㤮��� ���� */
                  if ( !CanUseGood )
                    PlaceStatus = "FP";
                  switch( (int)SeatAlg ) {
                    case sSeatGrpOnBasePlace:
                      if ( SeatPlaces.SeatGrpOnBasePlace( ) )
                        throw 1;
                      break;
                    case sSeatGrp:
                      if ( SeatPlaces.SeatsGrp( ) )
                        throw 1;
                      break;
                    case sSeatPassengers:
                      if ( SeatPlaces.SeatsPassengers( ) )
                        throw 1;
                      break;
                  }
                } /* end for status */
              } /* end for smoke */
            } /* end for tube */
          } /* end for use status */
        } /* end for alone */
      } /* end for CanUseRem */
    } /* end for FSeatAlg */
    SeatAlg = (TSeatAlg)0;
  }
  catch( int ierror ) {
    if ( ierror != 1 )
      throw;
    /* ��।������ ����祭��� ���� �� ���ᠦ�ࠬ, ⮫쪮 ��� SeatPlaces.SeatGrpOnBasePlace */
    if ( SeatAlg != sSeatPassengers )
      SeatPlaces.PlacesToPassengers( );
    SeatPlaces.RollBack();
    return;      
  }
  SeatPlaces.RollBack();
  throw UserException( "��⮬���᪠� ��ᠤ�� ����������" );
}

void ClearPlaces( FlagclearPlaces clPl )
{
  for ( vector<TPlaceList*>::iterator placeList=CurrSalon->placelists.begin();
        placeList!=CurrSalon->placelists.end(); placeList++ ) {
    int s=(*placeList)->GetXsCount()*(*placeList)->GetYsCount();
    for ( int i=0; i<s; i++ ) {
      TPlace *place = (*placeList)->place( i );
      if ( clPl.isFlag( clNotFree ) )
        place->pr_free = true;
      if ( clPl.isFlag( clStatus ) )
        place->status = "FP";
      if ( clPl.isFlag( clBlock ) )
        place->block = false;
      place->passSel = false;
    }
  }
}

bool ExistsBasePlace( TPassenger &pass, int lang )
{
  TPlaceList *placeList;
  TPoint FP;
  vector<TPlace*> vpl;
  string placeName = pass.placeName;
  string nplaceName = placeName;
  if ( lang == 1 ) {
    if ( placeName == CharReplace( nplaceName, LAT_NAME_LINES, RUS_NAME_LINES ) )
      CharReplace( nplaceName, RUS_NAME_LINES, LAT_NAME_LINES );
    if ( placeName == nplaceName ) // � ���. � ���. ���������� ��������
      nplaceName.clear();
  }
  tst();
  for ( vector<TPlaceList*>::iterator plList=CurrSalon->placelists.begin();
        plList!=CurrSalon->placelists.end(); plList++ ) {
    tst();
    if ( !lang && pass.placeList ) { /* � ���ᠦ�� 㦥 ���� ��뫪� �� �㦭� ᠫ�� */
      placeList = pass.placeList;
      FP = pass.Pos;
    }
    else 
      placeList = *plList;
    if ( !lang && pass.placeList ||
         !lang && !pass.placeList && placeList->GetisPlaceXY( placeName, FP ) ||
          lang && !nplaceName.empty() && placeList->GetisPlaceXY( nplaceName, FP ) ) {      
      tst();
      int j = 0;
      for ( ; j<pass.countPlace; j++ ) {
        if ( !placeList->ValidPlace( FP ) )
          break;
        TPlace *place = placeList->place( FP );
        if ( !place->visible || !place->isplace || place->block ||
             !place->pr_free || pass.clname != place->clname )
          break;
        vpl.push_back( place );         
        switch( (int)pass.Step ) {
          case sRight: 
             FP.x++;
             break;
          case sDown: 
             FP.y++;
             break;
        }
      }
      if ( j == pass.countPlace ) {
        for ( vector<TPlace*>::iterator ipl=vpl.begin(); ipl!=vpl.end(); ipl++ ) {
          if ( ipl == vpl.begin() ) {
            pass.InUse = true;
            pass.placeName = (*ipl)->yname + (*ipl)->xname;
            pass.placeList = placeList;
            pass.Pos.x = (*ipl)->x;
            pass.Pos.y = (*ipl)->y;
          }
          (*ipl)->pr_free = false;
        }
        return true;
      }
      vpl.clear();
      if ( !lang && pass.placeList )
        return false;
    }
  }
  return false;
}

void SelectPassengers( TSalons *Salons, TPassengers &p )
{  
  if ( !Salons )
    throw Exception( "�� ����� ᠫ��" );
  tst();
  p.Clear();  
  tst();
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT pax_grp.grp_id,pax.pax_id,pax.reg_no,surname,name, "\
                "       seat_no,prev_seat_no,class,seats,step "\
                " FROM pax_grp,pax, "\
                "( SELECT COUNT(*) step, pax_id FROM pax_rem "\
                "   WHERE rem_code = 'STCR' "\
                "  GROUP BY pax_id ) a "\
                "WHERE pax_grp.grp_id=pax.grp_id AND "\
                "      point_id=:trip_id AND "\
                "      pr_wl=0 AND pax.pr_brd IS NOT NULL AND "\
                "      seats > 0 AND "\
                "      a.pax_id(+) = pax.pax_id "\
                "ORDER BY pax.reg_no,pax_grp.grp_id ";
  Qry.DeclareVariable( "trip_id", otInteger );
  Qry.SetVariable( "trip_id", Salons->trip_id );
  Qry.Execute( );
  tst();
  while ( !Qry.Eof ) {
    TPassenger pass;
    pass.pax_id = Qry.FieldAsInteger( "pax_id" );
    pass.placeName = Qry.FieldAsString( "seat_no" );
    pass.PrevPlaceName = Qry.FieldAsString( "prev_seat_no" );
    pass.OldPlaceName = Qry.FieldAsString( "seat_no" );
    pass.clname = Qry.FieldAsString( "class" );
    pass.countPlace = Qry.FieldAsInteger( "seats" );
    pass.grpId = Qry.FieldAsInteger( "grp_id" );
    pass.regNo = Qry.FieldAsInteger( "reg_no" );
    string fname = Qry.FieldAsString( "surname" );    
    pass.fullName = TrimString( fname ) + Qry.FieldAsString( "name" ); 
    tst();
    for ( vector<TPlaceList*>::iterator placeList=Salons->placelists.begin();
          placeList!=Salons->placelists.end(); placeList++ ) {
      if ( (*placeList)->GetisPlaceXY( pass.placeName, pass.Pos ) ) {
        pass.placeList = *placeList;
        break;
      }
    }
    pass.InUse = ( pass.placeList );
    if ( Qry.FieldAsInteger( "step" ) ) {
      pass.rems.push_back( "STCR" );
    }
    tst();
    p.Add( pass );
    tst();
    Qry.Next();
  }	
}

/* ��⮬���᪠� ��ᠤ�� ���ᠦ�஢ 
   ����� ��ᠤ�� ���ᠦ�஢ �� ��������� ���������� ��
   ᠫ�� ����㦥�, ����� ���� ���� - �ࠡ�⠫� setcraft ��� � �����묨 ���⠬� */
void ReSeats( TSalons *Salons, bool DeleteNotFreePlaces, bool SeatOnNotBase )
{
  if ( !Salons )
    throw Exception( "�� ����� ᠫ�� ��� ��⮬���᪮� ��ᠤ��" );
  ProgTrace( TRACE5, "SalonsInterface::ReSeat with params: trip_id=%d, DeleteNotFreePlaces=%d, SeatOnNotBase=%d",
             Salons->trip_id, DeleteNotFreePlaces, SeatOnNotBase );		    
  CurrSalon = Salons;	
  SeatAlg = sSeatPassengers;
  CanUseStatus = false; /* �� ���뢠�� ����� ���� */
  CanUseSmoke = false;
  CanUseElem_Type = false;
  CanUseGood = false;
  CanUseRems = sIgnoreUse;
  Remarks.clear();
  CanUseTube = true;
  CanUseAlone = uTrue;
  SeatPlaces.Clear();

  FlagclearPlaces clPl;
  if ( DeleteNotFreePlaces ) {
    clPl.setFlag( clNotFree );
    ClearPlaces( clPl );
  }
  TPassengers p;
  SelectPassengers( CurrSalon, p ); /* �롮� ���ᠦ�஢ */
  int s=p.getCount();
  for ( int i=0; i<s; i++ ) {
    TPassenger &pass=p.Get( i );
    ProgTrace( TRACE5, "pass(%d) placeName=%s, PrevPlaceName=%s, OldPlaceName=%s", 
               i, pass.placeName.c_str(), pass.PrevPlaceName.c_str(), pass.OldPlaceName.c_str() ); 
    pass.InUse = false; /* �� �� ��ᠦ��� */
    pass.isSeat = !pass.placeName.empty();
    if ( !pass.isSeat ) { /* �� ������ ������� ���� */
      pass.placeName = pass.PrevPlaceName; /* ������ �।��饥 */
      pass.OldPlaceName = pass.PrevPlaceName; /* ������ �।��饥 */
    }
    ProgTrace( TRACE5, "placename=%s", pass.placeName.c_str() );
  }  
  try {
    for ( int vClass=0; vClass<=2; vClass++ ) {
      for ( int vSeats=0; vSeats<=1; vSeats++ ) {
        Passengers.Clear();
        s = p.getCount();
        for ( int i=0; i<s; i++ ) {
          TPassenger &pass = p.Get( i );
          if ( pass.InUse )
            continue;
          switch( vClass ) { 
            case 0: 
               if ( pass.clname != "�" )
                 continue;
               break;
            case 1: 
               if ( pass.clname != "�" )
                 continue;
               break;
            case 2: 
               if ( pass.clname != "�" )
                 continue;
               break;
          }
          if ( ExistsBasePlace( pass, vSeats ) )	
            continue;
          tst();
          if ( SeatOnNotBase ) {
            tst();
            pass.InUse = false;
            Passengers.Add( pass ); /* ������������� � � ������ �� ��諮�� ������� ���� */
          }
        } /* �஡������� �� �ᥬ ���ᠦ�ࠬ */
        if ( SeatOnNotBase ) {
          tst();
          GET_LINE_ARRAY( );
          /* ��ᠤ�� ���ᠦ�� � ���ண� �� ������� ������� ���� */
          SeatPlaces.SeatsPassengers( ); 
          tst();
          SeatPlaces.RollBack();
          int s = Passengers.getCount();
          for ( int i=0; i<s; i++ ) {
            TPassenger &pass = Passengers.Get( i );
            int k = p.getCount();
            for ( int j=0; j<k; j++	 ) {
              TPassenger &opass = p.Get( j );
              if ( opass.pax_id == pass.pax_id ) {
              	opass = pass;
              	break;
              }
            }            
          }
        }
      }
    }
    ProgTrace( TRACE5, "passengers.count=%d", p.getCount() );
    /*  � १���� ����� ����� ᯨ᮪ ���ᠦ�஢ � Pass. �᫨ Not InUse - �� ᥫ */
    TQuery Qry( &OraSession );
    /*!!! - ���� �� ������� ���� tid ??? */
    Qry.SQLText = "BEGIN "\
                  "  UPDATE pax set seat_no=:seat_no,prev_seat_no=:prev_seat_no,tid=tid__seq.nextval "\
                  "  WHERE pax_id=:pax_id; "\
                  "  mvd.sync_pax(:pax_id,:term); "\
                  " END; ";
    Qry.DeclareVariable( "seat_no", otString );
    Qry.DeclareVariable( "prev_seat_no", otString );
    Qry.DeclareVariable( "pax_id", otInteger );
    Qry.DeclareVariable( "term", otString );
    Qry.SetVariable( "term", TReqInfo::Instance()->desk.code );
    /* ���⠢�塞 ��������� � ���� ���ᠦ�஢ */    
    Passengers.Clear();
    int s = p.getCount();
    for ( int i=0; i<s; i++ ) {
      tst();
      TPassenger &pass = p.Get( i );
      Qry.SetVariable( "pax_id", pass.pax_id ); /*!!! tid */
      if ( !pass.InUse ) { /* �� ᬮ��� ��ᠤ��� */      
        if ( pass.isSeat ) { /* ���ᠦ�� ᨤ�� */
          pass.PrevPlaceName = pass.placeName;
          pass.isSeat = false;
          Qry.SetVariable( "seat_no", FNull );
          if ( pass.PrevPlaceName.empty() )
            Qry.SetVariable( "prev_seat_no", FNull );
           else 
            Qry.SetVariable( "prev_seat_no", pass.PrevPlaceName );
          TReqInfo::Instance()->MsgToLog( string("���ᠦ�� " ) + pass.fullName + 
                        " ��-�� ᬥ�� ���������� ��ᠦ�� � ���� " + 
                        pass.OldPlaceName, evtPax, CurrSalon->trip_id, pass.regNo, pass.grpId );
          Qry.Execute();
        }
        pass.placeName.clear();
        pass.OldPlaceName.clear();
      }
      else { /* ��ᠤ��� ���ᠦ�� ��� ���ᠤ��� �� ��㣮� ���� */
        if ( !pass.isSeat || pass.placeName != pass.OldPlaceName ) {
          pass.PrevPlaceName = pass.placeName;/*??? */
          Qry.SetVariable( "seat_no", pass.placeName );
          Qry.SetVariable( "prev_seat_no", pass.placeName );
          if ( !pass.isSeat )
            TReqInfo::Instance()->MsgToLog( string( "���ᠦ�� " ) + pass.fullName + 
                          " ��-�� ᬥ�� ���������� ��ᠦ�� �� ���� " + 
                          pass.placeName, evtPax, CurrSalon->trip_id, pass.regNo, pass.grpId );
          else
            TReqInfo::Instance()->MsgToLog( string( "���ᠦ�� " ) + pass.fullName + 
                          " ��-�� ᬥ�� ���������� ���ᠦ�� �� ���� " + 
                          pass.placeName, evtPax, CurrSalon->trip_id, pass.regNo, pass.grpId );
          pass.OldPlaceName = pass.placeName;
          pass.isSeat = true;
          Qry.Execute();
        }
      }
      Passengers.Add( pass );
    }
  }
  catch( ... ) {
    SeatPlaces.RollBack();
    throw;
  }
  SeatPlaces.RollBack();
  ProgTrace( TRACE5, "passengers.count=%d", p.getCount() );
}

void TPassengers::Build( xmlNodePtr dataNode )
{
  if ( !Passengers.getCount() )
    return;
  xmlNodePtr passNode = NewTextChild( dataNode, "passengers" );  
  tst();
  for (VPassengers::iterator p=FPassengers.begin(); p!=FPassengers.end(); p++ ) {
    xmlNodePtr pNode = NewTextChild( passNode, "pass" );
    NewTextChild( pNode, "pax_id", p->pax_id );	
    NewTextChild( pNode, "clname", p->clname );
    NewTextChild( pNode, "placename", p->placeName );	
    NewTextChild( pNode, "prevplacename", p->PrevPlaceName );    
    NewTextChild( pNode, "oldplacename", p->OldPlaceName );    
    NewTextChild( pNode, "countplace", p->countPlace );
    NewTextChild( pNode, "isseat", p->isSeat );
    NewTextChild( pNode, "grp_id", p->grpId );
    NewTextChild( pNode, "reg_no", p->regNo );
    NewTextChild( pNode, "name", p->fullName );
    if ( !p->rems.empty() && p->rems[ 0 ] == "STCR" )
      NewTextChild( pNode, "step", "�" );	    
    else
      NewTextChild( pNode, "step", "�" );	    
    NewTextChild( pNode, "inuse", p->InUse );      
    NewTextChild( pNode, "x", p->Pos.x );      
    NewTextChild( pNode, "y", p->Pos.y );      
    if ( p->placeList )
      NewTextChild( pNode, "num", p->placeList->num );
    else
      NewTextChild( pNode, "num", -1 );
  }  
}

bool TPassengers::existsNoSeats()
{
  int s = getCount();
  for ( int i=0; i<s; i++ ) {
    if ( !Get( i ).InUse )
      return true;
  } 
  return false;
}



TPassengers Passengers;
