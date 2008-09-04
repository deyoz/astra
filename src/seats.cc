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
#include "tlg/tlg_parser.h"
#include "convert.h"

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

typedef vector<TLinesSalon> vecVarLines;

class TLines {
  private:
    vecVarLines line1;
    vecVarLines line2;
  public:
    vecVarLines &getVarLine( int ver );
    void clear();
};
enum TSeatAlg { sSeatGrpOnBasePlace, sSeatGrp, sSeatPassengers, seatAlgLength };
enum TUseRem { sAllUse, sOnlyUse, sMaxUse, sNotUseDenial, sNotUse, sIgnoreUse, useremLength };
/* ����� ࠧ������ ���, ����� ᠦ��� �� ������ ����� ������ ࠧ�, �� ����� */
enum TUseAlone { uFalse3, uFalse1, uTrue };

enum TClearPlaces { clNotFree, clStatus, clBlock };
typedef BitSet<TClearPlaces> FlagclearPlaces;

void ClearPlaces( FlagclearPlaces clPl );

inline bool LSD( int G3, int G2, int G, int V3, int V2, TWhere Where );


vector<TLinesSalon> &TLines::getVarLine( int var )
{
  if ( !var )
    return line1;
  else
    return line2;
}
void TLines::clear() {
  line1.clear();
  line2.clear();
}

/* �������� ��६���� ��� �⮣� ����� */
TSeatPlaces SeatPlaces;

TLines lines;

TSalons *CurrSalon;

bool CanUseStatus; /* ���� �� ������ ���� */
string PlaceStatus; /* ᠬ ����� */
bool CanUse_PS; /* ����� �� �ᯮ�짮���� ����� �।�� ��ᠤ�� ��� ���ᠦ�஢ � ��㣨�� ����ᠬ� */
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
bool FindMCLS=false;
bool canUseMCLS=false;

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
  if ( seatplaces.empty() )
    return;
  /* �஡�� �� ��������� �������� ���⠬ */
  TSeatPlace seatPlace;
  for ( int i=Begin; i<=End; i++ ) {
    seatPlace = seatplaces[ i ];
    /* �஡�� �� ���� ��࠭���� ���⠬ */
    for ( vector<TPlace>::iterator place=seatPlace.oldPlaces.begin();
          place!=seatPlace.oldPlaces.end(); place++ ) {
      /* ����祭�� ���� */
      int idx = seatPlace.placeList->GetPlaceIndex( seatPlace.Pos );
      TPlace *pl = seatPlace.placeList->place( idx );
      /* �⬥�� ��� ��������� ���� */
      pl->Assign( *place );
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
  } /* end for */
  vector<TSeatPlace>::iterator b = seatplaces.begin();
  seatplaces.erase( b + Begin, b + End + 1 );
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
  switch( (int)Step ) {
    case sLeft:
    case sRight:
           pp_Count = counters.p_Count( sRight );
           pp_Count2 = counters.p_Count_2( sRight );
           pp_Count3 = counters.p_Count_3( sRight );
           p_Prior = abs( FP.x - EP.x );
           break;
    case sDown:
    case sUp:
           pp_Count = counters.p_Count( sDown );
           pp_Count2 = counters.p_Count_2( sDown );
           pp_Count3 = counters.p_Count_3( sDown );
           p_Prior = abs( FP.y - EP.y ); // ࠧ��� �/� FP � EP
           break;
  }
  p_RCount = Passengers.counters.p_Count( Step ) - pp_Count;
  p_RCount2 = Passengers.counters.p_Count_2( Step ) - pp_Count2;
  p_RCount3 = Passengers.counters.p_Count_3( Step ) - pp_Count3;
  p_Next = foundCount - p_Prior;
  p_Step = 0;
  NTrunc_Count = foundCount;
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
    if ( Trunc_Count != NTrunc_Count ) {
      NTrunc_Count = Trunc_Count; /* �뤥���� ������� ����, � � ⠪�� ������ �� �㦭� */
      continue;
    }
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
//    ProgTrace( TRACE5, "EP=(%d,%d)", EP.x, EP.y );
//    ProgTrace( TRACE5, "seatplace.Pos=(%d,%d)", seatplace.Pos.x, seatplace.Pos.y );
    /* ��࠭塞 ���� ���� � ����砥� ���� � ᠫ��� ��� ������ */
    for ( int i=0; i<Trunc_Count; i++ ) {
      TPlace place;
      TPlace *pl = placeList->place( EP );
//      ProgTrace( TRACE5, "EP=(%d,%d)", EP.x, EP.y );
      place.Assign( *pl );
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
//    ProgTrace( TRACE5, "Result=%d", Result );
  } /* end while */
  return Result;
}

/* �-�� ��� ��।������ ���������� ��ᠤ�� ��� ���� � ����� ���� ����饭�� ६�ન */
bool DoNotRemsSeats( const vector<TRem> &rems )
{
	bool res = false;
	bool pr_passmcls = find( Remarks.begin(), Remarks.end(), string("MCLS") ) != Remarks.end();
	bool no_mcls = false;
	for( vector<string>::iterator nrem=Remarks.begin(); nrem!=Remarks.end(); nrem++ ) {
	  for( vector<TRem>::const_iterator prem=rems.begin(); prem!=rems.end(); prem++ ) {
		  if ( !prem->pr_denial )
			  continue;
			if ( *nrem == prem->rem && !res )
				res = true;
	  }
	}
  for( vector<TRem>::const_iterator prem=rems.begin(); prem!=rems.end(); prem++ ) {
		ProgTrace( TRACE5, "prem->rem=%s", prem->rem.c_str() );
  	if ( !prem->pr_denial && prem->rem == "MCLS" && !pr_passmcls )
	  		no_mcls = true;
	}
	ProgTrace( TRACE5, "res=%d, mcls=%d", res, no_mcls );
  return res || no_mcls;
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
    if ( canUseMCLS ) {
      for ( prem = place->rems.begin(); prem != place->rems.end(); prem++ ) {
        if ( !prem->pr_denial && prem->rem == "MCLS" )
        	break;
      }
      if ( FindMCLS && prem == place->rems.end() ||
      	   !FindMCLS && prem != place->rems.end() ) //  �� ᬮ��� ��ᠤ��� �� ����� MCLS
     	  break;
    }

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
         if ( irem == Remarks.end() || DoNotRemsSeats( place->rems ) )
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
           if ( prem == place->rems.end() || DoNotRemsSeats( place->rems ) )
             return Result;
         }
         break;
      case sNotUseDenial:
      	if ( DoNotRemsSeats( place->rems ) )
      		return Result;
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
//  ProgTrace( TRACE5, "SeatSubGrp_On FP=(%d,%d), Step=%d, Wanted=%d", FP.x, FP.y, Step, Wanted );
  int foundCount = 0; // ���-�� ��������� ���� �ᥣ�
  int foundBefore = 0; // ���-�� ��������� ���� �� � ��᫥ FP
  int foundTubeBefore = 0;
  int foundTubeAfter = 0;
  TPlaceList *placeList = CurrSalon->CurrPlaceList();
  if ( !Wanted && seatplaces.empty() && CanUseAlone != uFalse3 )
    Alone = true;// �� ���� ��室 �, ���� �ந��樠����஢��� ��. ��६����� Alone
  int foundAfter = FindPlaces_From( FP, 0, Step );
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
  if ( CanUseTube && Step == sRight && foundCount < MAXPLACE ) {
    EP.x += foundAfter + 1; /* ��⠭���������� �� �।���������� ���� */
    if ( placeList->ValidPlace( EP ) ) {
      TPoint p( EP.x - 1, EP.y );
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
  foundCount = Put_Find_Places( FP, EP, foundBefore + foundAfter, Step );
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
  if ( EndWanted > 0 ) { /* ���� 㤠���� ���� ᠬ�� ���宥 ����: */
    /* ᠬ�� 㤠������ �� ��᫥����� ���������� � ⥪�饬 ��� */
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
  if ( foundCount == 1 && CanUseAlone != uTrue )
    if ( Alone )
      Alone = false;
    else  /* ����� 2 ࠧ� �⮡� ��﫮�� ���� ���� � ���. */
      return false; /*( p_Count_3( Step ) = Passengers.p_Count_3( Step ) )AND
               ( p_Count_2( Step ) = Passengers.p_Count_2( Step ) )AND
               ( p_Count( Step ) = Passengers.p_Count( Step ) ) ???};*/
  if ( counters.p_Count_3( Step ) == Passengers.counters.p_Count_3( Step ) &&
       counters.p_Count_2( Step ) == Passengers.counters.p_Count_2( Step ) &&
       counters.p_Count( Step ) == Passengers.counters.p_Count( Step ) )
    return true;
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
  VSeatPlaces sp( seatplaces.begin(), seatplaces.end() );
  if ( Where != sUpDown )
  for (ISeatPlace isp = sp.begin(); isp!= sp.end(); isp++ ) {
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
//    ProgTrace( TRACE5, "isp->oldPlaces.size()=%d, pass.countPlace=%d",
//               (int)isp->oldPlaces.size(), pass.countPlace );
    if ( isp->InUse || (int)isp->oldPlaces.size() != pass.countPlace ) /* ���-�� ���� ������ ᮢ������ */
      continue;
    int EqualQ = 0;
    if ( (int)isp->oldPlaces.size() == pass.countPlace )
      EqualQ = pass.countPlace*10000; //??? always true!
    bool pr_valid_place = true;
    for (vector<string>::iterator irem=pass.rems.begin(); irem!= pass.rems.end(); irem++ ) {
      for( vector<TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
      	/* �஡�� �� ���⠬ ����� ����� �������� ���ᠦ�� */
      	vector<TRem>::iterator itr;
      	for ( itr=ipl->rems.begin(); itr!=ipl->rems.end(); itr++ ) {
      	  if ( *irem == itr->rem ) {
      	    if ( itr->pr_denial ) {
      	      EqualQ -= PR_REM_TO_REM;
      	      pr_valid_place = false;
      	    }
      	    else {
      	      EqualQ += PR_REM_TO_REM;
      	    }
      	    break;
      	  }
      	}
        if ( itr == ipl->rems.end() ) /* �� ��諨 ६��� � ���� */
          EqualQ += PR_REM_TO_NOREM;
      } /* ����� �஡��� �� ���⠬ */
    } /* ����� �஡��� �� ६�ઠ� ���ᠦ�� */
    ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    TPlaceList *placeList = isp->placeList;
    if ( pass.placeName == placeList->GetPlaceName( isp->Pos ) ||
    	   !pass.preseat.empty() && pass.preseat == placeList->GetPlaceName( isp->Pos ) )
      EqualQ += PR_EQUAL_N_PLACE;
    ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( pass.placeRem.find( "SW" ) == 2  &&
        ( isp->Pos.x == 0 || isp->Pos.x == placeList->GetXsCount() - 1 ) ||
         pass.placeRem.find( "SA" ) == 2  &&
        ( isp->Pos.x - 1 >= 0 &&
          placeList->GetXsName( isp->Pos.x - 1 ).empty() ||
          isp->Pos.x + 1 < placeList->GetXsCount() &&
          placeList->GetXsName( isp->Pos.x + 1 ).empty() )
       )
      EqualQ += PR_EQUAL_REMPLACE;
    ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( pass.prSmoke == isp->oldPlaces.begin()->pr_smoke )
      EqualQ += PR_EQUAL_SMOKE;
    ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( pass.pers_type != "��" ) {
      for (ISeatPlace nsp=seatplaces.begin(); nsp!=seatplaces.end(); nsp++) {
      	if ( nsp == isp || nsp->InUse || nsp->oldPlaces.size() != isp->oldPlaces.size() || nsp->placeList != isp->placeList )
          continue;
        if ( abs( nsp->Pos.x - isp->Pos.x ) == 1 && abs( nsp->Pos.y - isp->Pos.y ) == 0 ) {
          EqualQ += 1;
          break;
        }
      }
    }
    ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( MaxEqualQ < EqualQ ) {
      MaxEqualQ = EqualQ;
      misp = isp;
      misp->isValid = pr_valid_place;
    }
  } /* end for */
 ProgTrace( TRACE5, "MaxEqualQ=%d", MaxEqualQ );
 misp->InUse = true;
 return *misp;
}

void TSeatPlaces::PlacesToPassengers()
{
	tst();
  if ( seatplaces.empty() )
   return;
  tst();
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
    pass.isValidPlace = seatPlace.isValid;
    pass.set_seat_no();
  }
}

/* ��ᠤ�� �ᥩ ��㯯� ��稭�� � ����樨 FP */
bool TSeatPlaces::SeatsGrp_On( TPoint FP  )
{
  /* ������ ����祭�� ���� */
  RollBack( );
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
//  ProgTrace( TRACE5, "SeatsPassenger_OnBasePlace( )" );
  bool OldCanUseGood = CanUseGood;
  bool OldCanUseSmoke = CanUseSmoke;
  bool CanUseGood = false;
  bool CanUseSmoke = false;
  string nplaceName = placeName;
  try {
    if ( !placeName.empty() ) {
      /* ��������� ����஢ ���� ���ᠦ�஢ � ����ᨬ��� �� ���. ��� ���. ᠫ��� */
      if ( placeName == CharReplace( nplaceName, rus_seat, lat_seat ) )
        CharReplace( nplaceName, lat_seat, rus_seat );
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
    case sNotUseDenial:
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
//ProgTrace( TRACE5, "SeatGrpOnBasePlace( )" );
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
      if ( !pass.placeName.empty() &&
           SeatsPassenger_OnBasePlace( pass.placeName, pass.Step ) && /* ��諨 ������� ���� */
           LSD( G3, G2, G, V3, V2, sEveryWhere ) )
        return true;

      RollBack( );
      Passengers.SetCountersForPass( pass );
      if ( !Remarks.empty() ) { /* ���� ६�ઠ */
        // ����⠥��� ���� �� ६�થ
        for ( int Where=sLeftRight; Where<=sUpDown; Where++ ) {
          /* ��ਠ��� ���᪠ ����� ���������� ���� */
          for ( int linesVar=0; linesVar<=1; linesVar++ ) {
            for( vecVarLines::iterator ilines=lines.getVarLine( linesVar ).begin();
                 ilines!=lines.getVarLine( linesVar ).end(); ilines++ ) {
              CurrSalon->SetCurrPlaceList( ilines->placeList );
              int ylen = CurrSalon->CurrPlaceList()->GetYsCount();
              TPoint FP;
              for ( int y=0; y<ylen; y++ ) {
                for ( vector<int>::iterator z=ilines->lines.begin(); z!=ilines->lines.end(); z++ ) {
                  FP.x = *z;
                  FP.y = y; /* �஡�� �� ���⠬ */
                  /* ��ᠤ�� ᠬ��� ������� ���ᠦ�� */
                  if ( SeatSubGrp_On( FP, pass.Step, 0 ) && LSD( G3, G2, G, V3, V2, (TWhere )Where ) ) {
//                    ProgTrace( TRACE5, "SeatSubGrp_On return true" );
                    return true;
                  }
                  RollBack( ); /* �� ����稫��� �⪠� ������� ���� */
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
  RollBack( );
  return false;
}

/* ��ᠤ�� ��㯯� �� �ᥬ ᠫ���� */
bool TSeatPlaces::SeatsGrp( )
{
 RollBack( );
 for ( int linesVar=0; linesVar<=1; linesVar++ ) {
   for( vecVarLines::iterator ilines=lines.getVarLine( linesVar ).begin();
        ilines!=lines.getVarLine( linesVar ).end(); ilines++ ) {
     CurrSalon->SetCurrPlaceList( ilines->placeList );
     int ylen = CurrSalon->CurrPlaceList()->GetYsCount();
     TPoint FP;
     for ( int y=0; y<ylen; y++ ) {
       for ( vector<int>::iterator z=ilines->lines.begin(); z!=ilines->lines.end(); z++ ) {
         FP.x = *z;
         FP.y = y; /* �஡�� �� ���⠬ */
         if ( SeatsGrp_On( FP ) ) {
           return true;
         }
       }
     }
   }
 }
 RollBack( );
 return false;
}

bool isValidPlaceToPassenger( const vector<string> &passrems, const vector<TPlace> &places )
{
  for (vector<string>::const_iterator irem=passrems.begin(); irem!= passrems.end(); irem++ ) {
    for( vector<TPlace>::const_iterator ipl=places.begin(); ipl!=places.end(); ipl++ ) {
     /* �஡�� �� ���⠬ ����� ����� �������� ���ᠦ�� */
      vector<TRem>::const_iterator itr;
      for ( itr=ipl->rems.begin(); itr!=ipl->rems.end(); itr++ )
        if ( *irem == itr->rem && itr->pr_denial )
        	 return false;
    }
  }
  return true;
}

/* ��ᠤ�� ���ᠦ�஢ �� ���⠬ �� ���뢠� ��㯯� */
bool TSeatPlaces::SeatsPassengers( bool pr_autoreseats )
{
//ProgTrace( TRACE5, "SeatsPassengers( )" );
  bool OLDFindMCLS = FindMCLS;
  bool OLDcanUseMCLS = canUseMCLS;
  vector<TPassenger> npass;
  Passengers.copyTo( npass );
  try {
    for ( int i=(int)!pr_autoreseats; i<=1+(int)pr_autoreseats; i++ ) {

      for ( VPassengers::iterator ipass=npass.begin(); ipass!=npass.end(); ipass++ ) {
      	/* ����� ���ᠦ�� ��ᠦ�� ��� ��ᠤ�� �� �஭� � � ���ᠦ�� ����� �� �஭� ��� ��� �।���. ��ᠤ�� ��� � ���ᠦ�� 㪠���� �� � ���� */
        if ( ipass->InUse || PlaceStatus == "PS" && !CanUse_PS &&
        	                   ( ipass->placeStatus != PlaceStatus || ipass->preseat.empty() || ipass->preseat != ipass->placeName ) )
          continue;
        Passengers.Clear();
        ipass->placeList = NULL;
        ipass->seat_no.clear();
        int old_index = ipass->index;
//        ProgTrace( TRACE5, "pr_auto_reseats=%d, find=%d, i=%d",
//                   pr_autoreseats, find( ipass->rems.begin(), ipass->rems.end(), string("MCLS") ) != ipass->rems.end(), i );
        if ( pr_autoreseats ) {
          if ( i == 0 ) {
          	  if ( find( ipass->rems.begin(), ipass->rems.end(), string("MCLS") ) == ipass->rems.end() )
          	  	continue;
            	FindMCLS = true;
            	canUseMCLS = true;
            }
            else
            	if ( i == 1 ) {
            		if ( find( ipass->rems.begin(), ipass->rems.end(), string("MCLS") ) != ipass->rems.end() )
            			continue;
            	  FindMCLS = false;
            	  canUseMCLS = true;
              }
              else {
                canUseMCLS = false;
              }
        }
        Passengers.Add( *ipass );
        ipass->index = old_index;

        if ( SeatGrpOnBasePlace( ) ||
             ( CanUseRems == sNotUse || CanUseRems == sIgnoreUse || CanUseRems == sNotUseDenial /*!!!*/ ) &&
             ( !CanUseStatus || PlaceStatus == "PS" && CanUse_PS || PlaceStatus != "PS" ) && SeatsGrp( ) ) {
          if ( seatplaces.begin()->Step == sLeft || seatplaces.begin()->Step == sUp )
            throw Exception( "�������⨬�� ���祭�� ���ࠢ����� ��ᠤ��" );
          ipass->placeList = seatplaces.begin()->placeList;
          ipass->Pos = seatplaces.begin()->Pos;
          ipass->Step = seatplaces.begin()->Step;
          ipass->placeName = ipass->placeList->GetPlaceName( ipass->Pos );
          ipass->isValidPlace = isValidPlaceToPassenger( ipass->rems, seatplaces.begin()->oldPlaces );
          ipass->InUse = true;
          ipass->set_seat_no();
          Clear();
        }
      }
    }
  }
  catch( ... ) {
    FindMCLS = OLDFindMCLS;
    canUseMCLS = OLDcanUseMCLS;
    Passengers.Clear();
    Passengers.copyFrom( npass );
    throw;
  }
  FindMCLS = OLDFindMCLS;
  canUseMCLS = OLDcanUseMCLS;
  Passengers.Clear();
  Passengers.copyFrom( npass );
  for ( VPassengers::iterator ipass=npass.begin(); ipass!=npass.end(); ipass++ ) {
    if ( !ipass->InUse )
      return false;
  }
  return true;
}
///////////////////////////////////////////////
void TPassenger::set_seat_no()
{
  seat_no.clear();
  int x = Pos.x;
  int y = Pos.y;
  ProgTrace( TRACE5, "pass.countPlace=%d", countPlace );
  for ( int j=0; j<countPlace; j++ ) {
    TSeat s;
    ProgTrace( TRACE5, "pass.placeList->GetXsName( %d )=%s, pass.placeList->GetYsName( %d )=%s",
    	           x, placeList->GetXsName( x ).c_str(), y, placeList->GetYsName( y ).c_str() );
    strcpy( s.line, placeList->GetXsName( x ).c_str() );
    strcpy( s.row, placeList->GetYsName( y ).c_str() );
    seat_no.push_back( s );
    switch( (int)Step ) {
    	case sRight: 
    		x++;
    		break;  
    	case sDown:
    		y++;
    		break;
    }
  }
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
    Qry.SQLText = "SELECT code, pr_comp FROM rem_types WHERE pr_comp IS NOT NULL";
    Qry.Execute();
    while ( !Qry.Eof ) {
      remarks[ Qry.FieldAsString( "code" ) ] = Qry.FieldAsInteger( "pr_comp" );
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
  if ( !pass.placeName.empty() || !pass.preseat.empty() )
    pass.priority = PR_N_PLACE;
  if ( !pass.placeRem.empty() )
    pass.priority += PR_REMPLACE;
  if ( pass.prSmoke )
    pass.priority += PR_SMOKE;
  addRemPriority( pass );
}

bool greatIndex( const TPassenger &p1, const TPassenger &p2 )
{
  return p1.index < p2.index;
}

void TPassengers::sortByIndex()
{
  sort( FPassengers.begin(), FPassengers.end(), greatIndex );
}

void TPassengers::Add( TPassenger &pass )
{
	pass.placeName = pass.agent_seat;
  if ( !pass.preseat.empty() && !pass.placeName.empty() && pass.preseat != pass.placeName ) {
    pass.placeName = pass.preseat; //!!! �� ॣ����樨 ����� �������� �।���⥫쭮 �����祭��� ����
  }
  ProgTrace( TRACE5, "pass.placeName=%s", pass.placeName.c_str() );
	
  if ( pass.countPlace > MAXPLACE || pass.countPlace <= 0 )
   throw Exception( "�� �����⨬�� ���-�� ���� ��� �ᠤ��" );
//  ProgTrace( TRACE5, "pass.placeStatus=%s, pass.preseat=%s", pass.placeStatus.c_str(), pass.preseat.c_str() );
  if ( pass.placeStatus == "BR" && !pass.preseat.empty() && pass.preseat == pass.placeName )
  	pass.placeStatus = "PS";

//  ProgTrace(TRACE5, "pass.countPlace=%d", pass.countPlace );
//  for ( vector<string>::iterator i=pass.rems.begin(); i!=pass.rems.end(); i++ )
//    ProgTrace(TRACE5, "pass.rem=%s", i->c_str() );
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
  pass.index = (int)FPassengers.size();
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
  if ( SeatAlg == sSeatPassengers )
    return true;
//  ProgTrace( TRACE5, "LSD G3=%d, G2=%d, G=%d, V3=%d, V2=%d, Where=%d",
//             G3, G2, G, V3, V2, Where );
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
  lines.clear();
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
          if ( x - 1 >= 0 && (*iplaceList)->GetXsName( x - 1 ).empty() ||
               x + 1 <= xlen - 1 && (*iplaceList)->GetXsName( x + 1 ).empty() )
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
    lines.getVarLine( 0 ).push_back( linesSalonVar0 );
    lines.getVarLine( 1 ).push_back( linesSalonVar1 );
  }
}

void SetStatuses( vector<string> &Statuses, string status, bool First, bool use_PS )
{
  Statuses.clear();
  if ( status == "TR" )
    if ( First ) {
      Statuses.push_back( "TR" );
      Statuses.push_back( "FP" );
      Statuses.push_back( "NG" );
    }
    else {
      Statuses.push_back( "RZ" );
      Statuses.push_back( "BR" );
    	if ( use_PS )
        Statuses.push_back( "PS" );
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
        if ( use_PS )
          Statuses.push_back( "PS" );
      }
    else
      if ( status == "FP" )
        if ( First ) {
          Statuses.push_back( "FP" );
          Statuses.push_back( "NG" );
        }
        else {
          Statuses.push_back( "RZ" );
          Statuses.push_back( "BR" );
          if ( use_PS )
            Statuses.push_back( "PS" );
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
            if ( use_PS )
              Statuses.push_back( "PS" );
          }
        else
        	if ( status == "PS" ) {
        		if ( First ) {
        			if ( use_PS )
          	    Statuses.push_back( "PS" );
              Statuses.push_back( "FP" );
              Statuses.push_back( "NG" );
        		}
        		else {
        			Statuses.push_back( "RZ" );
        			Statuses.push_back( "BR" );
        		}
        	}
}

/*///////////////////////////END CLASS TPASSENGERS/////////////////////////*/

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
    if ( placeName == CharReplace( nplaceName, rus_seat, lat_seat ) )
      CharReplace( nplaceName, lat_seat, rus_seat );
    if ( placeName == nplaceName ) // � ���. � ���. ���������� ��������
      nplaceName.clear();
  }
  for ( vector<TPlaceList*>::iterator plList=CurrSalon->placelists.begin();
        plList!=CurrSalon->placelists.end(); plList++ ) {
    if ( !lang && pass.placeList ) { /* � ���ᠦ�� 㦥 ���� ��뫪� �� �㦭� ᠫ�� */
      placeList = pass.placeList;
      FP = pass.Pos;
    }
    else
      placeList = *plList;
    if ( !lang && pass.placeList ||
         !lang && !pass.placeList && placeList->GetisPlaceXY( placeName, FP ) ||
          lang && !nplaceName.empty() && placeList->GetisPlaceXY( nplaceName, FP ) ) {
      int j = 0;
      for ( ; j<pass.countPlace; j++ ) {
        if ( !placeList->ValidPlace( FP ) )
          break;
        TPlace *place = placeList->place( FP );
        bool findpass = ( find( pass.rems.begin(), pass.rems.end(), string("MCLS") ) != pass.rems.end() );
        bool findplace = false;
        for ( vector<TRem>::iterator r=place->rems.begin(); r!=place->rems.end(); r++ ) {
        	if ( !r->pr_denial && r->rem == "MCLS" ) {
        		findplace = true;
        		break;
        	}
        }
        if ( !place->visible || !place->isplace || place->block ||
             !place->pr_free || pass.clname != place->clname ||
             findpass != findplace )
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

namespace SEATS {
/* ��ᠤ�� ���ᠦ�஢ */
void SeatsPassengers( TSalons *Salons, bool FUse_PS )
{
  if ( !Passengers.getCount() )
    return;
  SeatPlaces.Clear();
  CurrSalon = Salons;
  vector<string> Statuses;
  CanUseStatus = true;
	CanUse_PS = FUse_PS;
  CanUseSmoke = false; /* ���� �� �㤥� ࠡ���� � ����騬� ���⠬� */
  CanUseElem_Type = false; /* ���� �� �㤥� ࠡ���� � ⨯��� ���� */
  bool Status_preseat = FUse_PS;//!!!false;
  GET_LINE_ARRAY( );
  SeatAlg = sSeatGrpOnBasePlace;

  /* �� ᤥ����!!! �᫨ � ��� ���ᠦ�஢ ���� ����, � ⮣�� ��ᠤ�� �� ���⠬, ��� ��� ��㯯� */

  /* ��।������ ���� �� � ��㯯� ���ᠦ�� � �।���⥫쭮� ��ᠤ��� */
  bool Status_seat_no_BR=false;
  for ( int i=0; i<Passengers.getCount(); i++ ) {
  	TPassenger &pass = Passengers.Get( i );
  	if ( pass.placeStatus == "PS" ) {
  		Status_preseat = true;
  	}
  }
  /*!!!*/
  bool SeatOnlyBasePlace=true;
  for ( int i=0; i<Passengers.getCount(); i++ ) {
  	TPassenger &pass = Passengers.Get( i );
  	if ( pass.placeName.empty() ) {
  		SeatOnlyBasePlace=false;
  		break;
  	}
  }  /*!!!*/


  try {
   for ( int FSeatAlg=0; FSeatAlg<seatAlgLength; FSeatAlg++ ) {
     SeatAlg = (TSeatAlg)FSeatAlg;
     /* �᫨ ���� � ��㯯� �।���⥫쭠� ��ᠤ��, � ⮣�� ᠦ��� ��� �⤥�쭮 */
     if ( ( Status_preseat || Status_seat_no_BR || SeatOnlyBasePlace /*!!!*/ ) && SeatAlg != sSeatPassengers )
     	 continue;
     for ( int FCanUseRems=0; FCanUseRems<useremLength; FCanUseRems++ ) {
        CanUseRems = (TUseRem)FCanUseRems;
        switch( (int)SeatAlg ) {
          case sSeatGrpOnBasePlace:
            if ( CanUseRems == sIgnoreUse /*|| CanUseRems == sNotUseDenial!!!*/ )
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
                SetStatuses( Statuses, Passengers.Get( 0 ).placeStatus, KeyStatus, Status_preseat );
                /* �஡�� �� ����ᮬ */
                for ( vector<string>::iterator st=Statuses.begin(); st!=Statuses.end(); st++ ) {
                  PlaceStatus = *st;
                  CanUseGood = ( PlaceStatus != "NG" ); /* ��㤮��� ���� */
                  if ( !CanUseGood )
                    PlaceStatus = "FP";
/*                  ProgTrace( TRACE5, "seats with:SeatAlg=%d,FCanUseRems=%d,FCanUseAlone=%d,FCanUseTube=%d,FCanUseSmoke=%d,CanUseGood=%d,PlaceStatus=%s",
                             (int)SeatAlg,FCanUseRems,FCanUseAlone,FCanUseTube,FCanUseSmoke,CanUseGood,PlaceStatus.c_str());*/
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
                      if ( SeatOnlyBasePlace ) {
                      	SeatOnlyBasePlace = false;
                      	FSeatAlg=0;
                      }
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
//    ProgTrace( TRACE5, "SeatAlg=%d, CanUseRems=%d", (int)SeatAlg, (int)CanUseRems );
    /* ��।������ ����祭��� ���� �� ���ᠦ�ࠬ, ⮫쪮 ��� SeatPlaces.SeatGrpOnBasePlace */
    if ( SeatAlg != sSeatPassengers ) {
      SeatPlaces.PlacesToPassengers( );
    }
    ProgTrace( TRACE5, "SeatAlg=%d, CanUseRems=%d", SeatAlg, CanUseRems );

    Passengers.sortByIndex();

    SeatPlaces.RollBack( );
    return;
  }
  SeatPlaces.RollBack( );
  throw UserException( "��⮬���᪠� ��ᠤ�� ����������" );
}

void SelectPassengers( TSalons *Salons, TPassengers &p )
{
  if ( !Salons )
    throw Exception( "�� ����� ᠫ��" );
  tst();
  p.Clear();
  tst();
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT points.airline,pax_grp.grp_id,pax.pax_id,pax.reg_no,surname,pax.name, "\
                "       seat_no,prev_seat_no,pax_grp.class,cls_grp.code subclass,seats,pax.tid,step "\
                " FROM pax_grp,pax, cls_grp, points, "\
                "( SELECT COUNT(*) step, pax_id FROM pax_rem "\
                "   WHERE rem_code = 'STCR' "\
                "  GROUP BY pax_id ) a "\
                "WHERE pax_grp.grp_id=pax.grp_id AND "\
                "      pax_grp.point_dep=:point_id AND "\
                "      points.point_id = pax_grp.point_dep AND "\
                "      pax_grp.class_grp = cls_grp.id AND "\
                "      pax.pr_brd IS NOT NULL AND "\
                "      seats > 0 AND "\
                "      a.pax_id(+) = pax.pax_id "\
                "ORDER BY pax.reg_no,pax_grp.grp_id ";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", Salons->trip_id );
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
    pass.tid = Qry.FieldAsInteger( "tid" );
    pass.grpId = Qry.FieldAsInteger( "grp_id" );
    pass.regNo = Qry.FieldAsInteger( "reg_no" );
    string fname = Qry.FieldAsString( "surname" );
    pass.fullName = TrimString( fname ) + " " + Qry.FieldAsString( "name" );
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
    if (
    	   Qry.FieldAsString( "airline" ) == string( "��" ) && string( "�" ) == Qry.FieldAsString( "subclass" ) ||
    	   Qry.FieldAsString( "airline" ) == string( "��" ) && string( "�" ) == Qry.FieldAsString( "subclass" )
    	 ) {
      ProgTrace( TRACE5, "subcls=%s", Qry.FieldAsString( "subclass" ) );
    	pass.rems.push_back( "MCLS" );
    }
    p.Add( pass );
    Qry.Next();
  }
}

void SaveTripSeatRanges( int point_id, TCompLayerType layer_type, vector<TSeatRange> &seats, int pax_id )
{
  if (seats.empty()) return;
  tst();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  SELECT comp_layers__seq.nextval INTO :range_id FROM dual; "
    "  INSERT INTO trip_comp_layers "
    "    (range_id,point_id,layer_type, "
    "     first_xname,last_xname,first_yname,last_yname,pax_id) "
    "  VALUES "
    "    (:range_id,:point_id,:layer_type, "
    "     :first_xname,:last_xname,:first_yname,:last_yname,:pax_id); "
    "END; ";
  Qry.CreateVariable( "range_id", otInteger, FNull );
  Qry.CreateVariable( "point_id", otInteger,point_id );
  Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
  if ( pax_id > 0 )
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
  else
  	Qry.CreateVariable( "pax_id", otInteger, FNull );
  Qry.DeclareVariable( "first_xname", otString );
  Qry.DeclareVariable( "last_xname", otString );
  Qry.DeclareVariable( "first_yname", otString );
  Qry.DeclareVariable( "last_yname", otString );

  for(vector<TSeatRange>::iterator i=seats.begin();i!=seats.end();i++)
  {
    Qry.SetVariable("first_xname",i->first.line);
    Qry.SetVariable("last_xname",i->second.line);
    Qry.SetVariable("first_yname",i->first.row);
    Qry.SetVariable("last_yname",i->second.row);
    Qry.Execute();
  };
}

bool getNextSeat( TCompLayerType layer_type, int point_id, TSeatRange &r, int pr_down )
{
	TQuery Qry( &OraSession );
  switch ( layer_type ) {
  	case cltCheckin:
    case cltPreseat:
      Qry.SQLText =
        "SELECT xname, yname FROM trip_comp_elems t, "
        "(SELECT num, x, y FROM trip_comp_elems "
        "  WHERE point_id=:point_id AND xname = :xname AND yname = :yname ) a "
        " WHERE t.point_id=:point_id AND "
        " t.num = a.num AND t.x = DECODE(:pr_down,0,1,0) + a.x AND t.y = DECODE(:pr_down,0,0,1) + a.y ";
      Qry.CreateVariable( "point_id", otInteger, point_id );
      Qry.CreateVariable( "xname", otString, r.first.line );
      Qry.CreateVariable( "yname", otString, r.first.row );
      Qry.CreateVariable( "pr_down", otInteger, pr_down );
      Qry.Execute();
      if ( Qry.RowCount() ) {
      	strcpy( r.first.line, Qry.FieldAsString( "xname" ) );
      	strcpy( r.first.row, Qry.FieldAsString( "yname" ) );
      	r.second = r.first;
      	ProgTrace( TRACE5, "getNextSeat: xname=%s, yname=%s", r.first.line, r.first.row );
      	return true;
      }
      break;
    default:
    	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
    	throw UserException( "��⠭��������� ᫮� ����饭 ��� ࠧ��⪨" );
  }
  return false;
}

void ChangeLayer( TCompLayerType layer_type, int point_id, int pax_id, int &tid,
                  string first_xname, string first_yname, TSeatsType seat_type, bool pr_lat_seat )
{
	tst();
	CanUse_PS = false; //!!!
	first_xname = norm_iata_line( first_xname );
	first_yname = norm_iata_line( first_yname );
	ProgTrace( TRACE5, "layer=%s, point_id=%d, pax_id=%d, first_xname=%s, first_yname=%s",
	           EncodeCompLayerType( layer_type ), point_id, pax_id, first_xname.c_str(), first_yname.c_str() );
  TQuery Qry( &OraSession );
  /* ��稬 ३� */
  Qry.SQLText = "UPDATE points SET point_id=point_id WHERE point_id=:point_id";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", point_id );
  Qry.Execute();
  tst();
  Qry.Clear();
  /* ���뢠�� ���� �� ���ᠦ��� */
  switch ( layer_type ) {
  	case cltCheckin:
      Qry.SQLText =
       "SELECT surname, name, reg_no, grp_id, seats, a.step step, tid, "" target, "
       "       salons.get_seat_no(pax.pax_id,:layer_type,pax.seats,:point_dep,rownum) AS seat_no, "          
       " FROM pax, "
       "( SELECT COUNT(*) step FROM pax_rem "
       "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "
       "WHERE pax_id=:pax_id";
       Qry.CreateVariable( "point_dep", otInteger, point_id );
      break;
    case cltPreseat:
      Qry.SQLText =
        "SELECT surname, name, 0 reg_no, crs_pax.pnr_id grp_id, seats, a.step step, crs_pax.tid, target, point_id, "
        "      salons.get_crs_seat_no(crs_pax.pax_id,:layer_type,crs_pax.seats,crs_pnr.point_id,rownum) AS seat_no "
        " FROM crs_pax, crs_pnr, "
        "( SELECT COUNT(*) step FROM crs_pax_rem "
        "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "
        " WHERE crs_pax.pax_id=:pax_id AND crs_pax.pnr_id=crs_pnr.pnr_id";
    	break;
    default:
    	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
    	throw UserException( "��⠭��������� ᫮� ����饭 ��� ࠧ��⪨" );
  }
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
  tst();
  Qry.Execute();
  tst();
  // ���ᠦ�� �� ������ ��� ����������������� � ��㣮� �⮩�� ��� �� �।�. ��ᠤ�� ���ᠦ�� 㦥 ��ॣ����஢��
  if ( !Qry.RowCount() ) {
    ProgTrace( TRACE5, "!!! Passenger not found in funct ChangeLayer" );
    throw UserException( "���ᠦ�� �� ������. ������� �����"	);
  }
  tst();
  string fullname = Qry.FieldAsString( "surname" );
  TrimString( fullname );
  fullname += string(" ") + Qry.FieldAsString( "name" );
  int idx1 = Qry.FieldAsInteger( "reg_no" );
  int idx2 = Qry.FieldAsInteger( "grp_id" );
  string target = Qry.FieldAsString( "target" );
  int point_id_tlg = Qry.FieldAsInteger( "point_id" );
  int seats_count = Qry.FieldAsInteger( "seats" );
  int pr_down;
  if ( Qry.FieldAsInteger( "step" ) )
    pr_down = 1;
  else
    pr_down = 0;
  string prior_seat = Qry.FieldAsString( "seat_no" );
  if ( !seats_count ) {
    ProgTrace( TRACE5, "!!! Passenger has count seats=0 in funct ChangeLayer" );
    throw UserException( "���ᠤ�� ����������. ������⢮ ���� ���������� ���ᠦ�஬ ࠢ�� ���" ); //!!!
  }

  if ( Qry.FieldAsInteger( "tid" ) != tid  ) {
    ProgTrace( TRACE5, "!!! Passenger has changed in other term in funct ChangeLayer" );
    throw UserException( string( "��������� �� ���ᠦ��� " ) + fullname + " �ந��������� � ��㣮� �⮩��. ������� �����" ); //!!!
  }
  if ( layer_type != cltCheckin && SALONS::Checkin( pax_id ) ) {
  	ProgTrace( TRACE5, "!!! Passenger set layer=%s, but his was chekin in funct ChangeLayer", EncodeCompLayerType( layer_type ) );
  	throw UserException( "���ᠦ�� ��ॣ����஢��. ������� �����" );
  }
  tst();
  // ���뢠�� ᫮� �� ������ ����� � ������ �஢��� �� �, �� ��� ᫮� 㦥 ����� ��㣨� ���ᠦ�஬
  Qry.Clear();
  Qry.SQLText =
    "SELECT layer_type, pax_id, crs_pax_id "
    " FROM trip_comp_layers "
    " WHERE point_id=:point_id AND "
    "       first_yname=:first_yname AND "
    "       first_xname=:first_xname ";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "first_xname", otString, first_xname );
  Qry.CreateVariable( "first_yname", otString, first_yname );
  Qry.Execute();
  tst();
  int p_id;
  while ( !Qry.Eof ) {
  	switch( layer_type ) {
      case cltCheckin:
      	p_id = Qry.FieldAsInteger( "pax_id" );
      	break;
  		case cltPreseat:
  			p_id = Qry.FieldAsInteger( "crs_pax_id" );
        break;
      default:
      	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
      	throw UserException( "��⠭��������� ᫮� ����饭 ��� ࠧ��⪨" );
    }
  	
  // ��⠥��� ������ ᫮� �� ����, ���஥ � ⠪ ����� ��� ᫮� ��� ��㣮�� ���ᠦ��
    if ( seat_type != stDropseat &&
    	   string( EncodeCompLayerType( layer_type ) ) == Qry.FieldAsString( "layer_type" ) &&
    	   pax_id != p_id ) {
  	  ProgTrace( TRACE5, "!!!ChangeLayer: seat_type!=stDropseat, EncodeCompLayerType( layer_type )=%s already found in trip_comp_layers for other passangers, point_id=%d in funct ChangeLayer",
  	             EncodeCompLayerType( layer_type ), point_id );
  	  throw UserException( "���� ����� ��㣨� ���ᠦ�஬" );
    }
  	Qry.Next();
  }
  tst();
  if ( seat_type != stSeat ) { // ���ᠤ��, ��ᠤ�� - 㤠����� ��ண� ᫮�
  	Qry.Clear();
  	switch( layer_type ) {
      case cltCheckin:
  	    Qry.SQLText =
          "DELETE FROM trip_comp_layers "
          " WHERE point_id=:point_id AND "
          "       layer_type=:layer_type AND "
          "       pax_id=:pax_id ";
        Qry.CreateVariable( "point_id", otInteger, point_id );
      	break;
  		case cltPreseat:
  			// 㤠����� �� ᠫ���, �᫨ ���� ࠧ��⪠
  	    Qry.SQLText =
          "DELETE FROM "
          "  (SELECT * FROM tlg_comp_layers,trip_comp_layers "
          "    WHERE tlg_comp_layers.range_id=trip_comp_layers.range_id AND "
          "       tlg_comp_layers.crs_pax_id=:pax_id AND "
          "       tlg_comp_layers.layer_type=:layer_type) ";
        Qry.CreateVariable( "pax_id", otInteger, pax_id );
        Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
        tst();
        Qry.Execute();
        tst();
        // 㤠����� �� ᫮� ⥫��ࠬ�
  			Qry.Clear();
      	Qry.SQLText =
	        "DELETE FROM tlg_comp_layers "
          " WHERE tlg_comp_layers.crs_pax_id=:pax_id AND "
          "       tlg_comp_layers.layer_type=:layer_type ";
        break;
      default:
      	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
      	throw UserException( "��⠭��������� ᫮� ����饭 ��� ࠧ��⪨" );
    }
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
    tst();
    Qry.Execute();
    tst();
/*!!!    if ( !Qry.RowCount() == seats ) { // ��⠥��� 㤠���� ᫮�, ���ண� ��� � ��
      throw UserException( "��室��� ���� �� �������" );
    }*/
  }
  tst();
  // �����祭�� ������ ᫮�
  if ( seat_type != stDropseat ) { // ��ᠤ�� �� ����� ����
    vector<TSeatRange> seats;
    TSeatRange r;
    strcpy( r.first.line, first_xname.c_str() );
    strcpy( r.first.row, first_yname.c_str() );
    r.second = r.first;
    for ( int i=0; i<seats_count; i++ ) { // �஡�� �� ���-�� ����
    	ProgTrace( TRACE5, "seats.push_back: xname=%s, yname=%s, pr_down=%d, seats_count=%d",
    	           r.first.line, r.first.row, pr_down, seats_count );
      seats.push_back( r );
      if ( !getNextSeat( layer_type, point_id, r, pr_down ) )
      	break;
    }
    Qry.Clear();
    Qry.SQLText = "SELECT tid__seq.nextval AS tid FROM dual";
    tst();
    Qry.Execute();
    tst();
    tid = Qry.FieldAsInteger( "tid" );
  	Qry.Clear();
  	switch ( layer_type ) {
  	  case cltCheckin:
  	  	tst();
  		  SaveTripSeatRanges( point_id, layer_type, seats, pax_id );
  		  tst();
  		  Qry.SQLText =
          "BEGIN "
          " UPDATE pax SET tid=:tid WHERE pax_id=:pax_id;"
          " mvd.sync_pax(:pax_id,:term); "
          "END;";
        Qry.CreateVariable( "term", otString, TReqInfo::Instance()->desk.code );
        break;
      case cltPreseat:
      	tst();
        SaveTlgSeatRanges( point_id_tlg, target, layer_type, seats, pax_id, 0, false );
        tst();
	      Qry.SQLText =
          "UPDATE crs_pax SET tid=:tid WHERE pax_id=:pax_id";
      	break;
      default:
      	ProgTrace( TRACE5, "!!! Unuseable layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
      	throw UserException( "��⠭��������� ᫮� ����饭 ��� ࠧ��⪨" );
    }
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "tid", otInteger, tid );
    tst();
    Qry.Execute();
    tst();
  }

  TReqInfo *reqinfo = TReqInfo::Instance();

  switch( seat_type ) {
  	case stSeat:
  		switch( layer_type ) {
  			case cltCheckin:
          reqinfo->MsgToLog( string( "���ᠦ�� " ) + fullname +
                             " ��ᠦ�� �� ����: " + 
                             denorm_iata_row( first_yname ) + denorm_iata_line( first_xname, pr_lat_seat ),
                             evtPax, point_id, idx1, idx2 );
          break;
        case cltPreseat:
          reqinfo->MsgToLog( string( "���ᠦ��� " ) + fullname +
                             " �।���⥫쭮 �����祭� ����. ����� ����: " + 
                             denorm_iata_row( first_yname ) + denorm_iata_line( first_xname, pr_lat_seat ),
                             evtPax, point_id, idx1, idx2 );
          break;
        default:;
  		}
  		break;
    case stReseat:
    	switch( layer_type ) {
  			case cltCheckin:
          reqinfo->MsgToLog( string( "���ᠦ�� " ) + fullname +
                             " ���ᠦ��. ����� ����: " + 
                             denorm_iata_row( first_yname ) + denorm_iata_line( first_xname, pr_lat_seat ),
                             evtPax, point_id, idx1, idx2 );
          break;
        case cltPreseat:
          reqinfo->MsgToLog( string( "���ᠦ��� " ) + fullname +
                             " �।���⥫쭮 �����祭� ����. ����� ����: " + 
                             denorm_iata_row( first_yname ) + denorm_iata_line( first_xname, pr_lat_seat ),
                             evtPax, point_id, idx1, idx2 );
          break;
        default:;
  		}
  		break;
  	case stDropseat:
    	switch( layer_type ) {
  			case cltCheckin:
          reqinfo->MsgToLog( string( "���ᠦ�� " ) + fullname +
                             " ��ᠦ��. ����: " + prior_seat,
                             evtPax, point_id, idx1, idx2 );
          break;
        case cltPreseat:
          reqinfo->MsgToLog( string( "���ᠦ��� " ) + fullname +
                             " �⬥���� �।���⥫쭮 �����祭��� ����: " + prior_seat,
                             evtPax, point_id, idx1, idx2 );
          break;
        default:;
  		}
  		break;
  }
  tst();
}

/* ���ᠤ�� ��� ��ᠤ�� ���ᠦ�� �����頥� ���� tid */
/*bool Reseat( TSeatsType seatstype, int trip_id, int pax_id, int &tid, int num, int x, int y, string &nplaceName, bool pr_cancel )
{
	CanUse_PS = false; //!!!
	if ( seatstype == sreseats )
		ProgTrace( TRACE5, "Reseats, trip_id=%d, pax_id=%d, num=%d, x=%d, y=%d", trip_id, pax_id, num, x, y );
	else
		ProgTrace( TRACE5, "Reserve, trip_id=%d, pax_id=%d, num=%d, x=%d, y=%d", trip_id, pax_id, num, x, y );
  TQuery Qry( &OraSession );*/
  /* ���뢠�� ���� �� ���ᠦ��� */
/*  if ( seatstype == sreseats )
    Qry.SQLText = "SELECT seat_no, prev_seat_no, seats, a.step step, surname, name,"\
                  "       reg_no, grp_id "\
                  " FROM pax,"\
                  "( SELECT COUNT(*) step FROM pax_rem "\
                  "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "\
                  "WHERE pax.pax_id=:pax_id ";
  else
    Qry.SQLText = "SELECT seat_no prev_seat_no, preseat_no seat_no, seats, a.step step, surname, name,"\
                  "       0 reg_no, pnr_id grp_id "\
                  " FROM crs_pax,"\
                  "( SELECT COUNT(*) step FROM crs_pax_rem "\
                  "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "\
                  "WHERE crs_pax.pax_id=:pax_id ";

  Qry.DeclareVariable( "pax_id", otInteger );
  Qry.SetVariable( "pax_id", pax_id );
  Qry.Execute();
  tst();
  string placeName = Qry.FieldAsString( "seat_no" );
  string fname = Qry.FieldAsString( "surname" );
  string fullname = TrimString( fname ) + " " + Qry.FieldAsString( "name" );
  TSeatStep Step;
  if ( Qry.FieldAsInteger( "step" ) )
    Step = sDown;
  else
    Step = sRight;
  int reg_no = Qry.FieldAsInteger( "reg_no" );
  int grp_id = Qry.FieldAsInteger( "grp_id" );
  if ( pr_cancel )
  	nplaceName.clear();
  else {*/
    /* ���뢠�� ���� �� ������ ����� */
/*    Qry.Clear();
    Qry.SQLText = "SELECT yname||xname placename FROM trip_comp_elems "\
                  " WHERE point_id=:point_id AND num=:num AND x=:x AND y=:y AND pr_free IS NOT NULL";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.DeclareVariable( "num", otInteger );
    Qry.DeclareVariable( "x", otInteger );
    Qry.DeclareVariable( "y", otInteger );
    Qry.SetVariable( "point_id", trip_id );
    Qry.SetVariable( "num", num );
    Qry.SetVariable( "x", x );
    Qry.SetVariable( "y", y );
    Qry.Execute();
    tst();
    if ( !Qry.RowCount() )
      return false;
    nplaceName = Qry.FieldAsString( "placename" );
  }*/
  /*??/ ��।��塞 �뫮 �� ��஥ ���� */
/*  int InUse;
  if ( pr_cancel )
  	InUse = 0;
  else {
    Qry.Clear();
    if ( seatstype == sreseats ) {
      Qry.SQLText = "SELECT COUNT(*) c FROM trip_comp_elems "\
                    " WHERE point_id=:point_id AND yname||xname=:placename AND "
                    "       pr_free IS NULL AND rownum<=1";
    }
    else {
      Qry.SQLText = "SELECT COUNT(*) c FROM trip_comp_elems "\
                    " WHERE point_id=:point_id AND yname||xname=:placename AND "
                    "       pr_free IS NOT NULL AND status='PS' AND rownum<=1";
    }
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.DeclareVariable( "placename", otString );
    Qry.SetVariable( "point_id", trip_id );
    Qry.SetVariable( "placename", placeName );
    Qry.Execute();
    InUse = Qry.FieldAsInteger( "c" ) + 1;*/ /* 1-��ᠤ��,2-���ᠤ�� */
/*  }

  ProgTrace( TRACE5, "InUSe=%d, oldplace=%s, newplace=%s 0 -delete, 1-seats, 2-reseats",
             InUse, placeName.c_str(), nplaceName.c_str() );
  TReqInfo *reqinfo = TReqInfo::Instance();
  Qry.Clear();
  Qry.SQLText = "SELECT tid__seq.nextval AS tid FROM dual";
  Qry.Execute();
  int new_tid = Qry.FieldAsInteger( "tid" );
  Qry.Clear();
  if ( seatstype == sreseats ) {
  	if ( InUse == 2 && !CanUse_PS ) { // ���ᠤ��, ���� ��ᬮ����, �⮡� ���ᠦ�� �� ��� ���� �� ����� ����, �।�����祭��� ��� ��㣮��
  		tst();
  		Qry.SQLText = "SELECT status FROM trip_comp_elems "
  		              " WHERE point_id=:point_id AND yname||xname=:nplacename";
  		Qry.CreateVariable( "point_id", otInteger, trip_id );
  		Qry.CreateVariable( "nplacename", otString, nplaceName );
  		Qry.Execute();
  		ProgTrace( TRACE5, "status=%s", Qry.FieldAsString( "status" ) );
  		if ( Qry.FieldAsString( "status" ) == string( "PS" ) ) {
  			return false;
  		}
  		Qry.Clear();
  		tst();
  	}
    Qry.SQLText = "BEGIN "\
                  " salons.seatpass( :point_id, :pax_id, :placename, :whatdo ); "\
                  " UPDATE pax SET seat_no=:placename,prev_seat_no=:placename,tid=tid__seq.currval "\
                  "  WHERE pax_id=:pax_id; "\
                  " UPDATE crs_pax SET preseat_no=NULL,tid=tid__seq.currval "\
                  "  WHERE pax_id=:pax_id; "\
                  " mvd.sync_pax(:pax_id,:term); "\
                  "END; ";
    Qry.DeclareVariable( "term", otString );
    Qry.SetVariable( "term", reqinfo->desk.code );
  }
  else {
    Qry.SQLText = "BEGIN "\
                  " salons.reserve( :point_id, :pax_id, :placename, 'PS', :whatdo ); "\
                  " UPDATE crs_pax SET preseat_no=:placename,tid=tid__seq.currval "\
                  "  WHERE pax_id=:pax_id; "\
                  "END; ";
  }
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.DeclareVariable( "pax_id", otInteger );
  Qry.DeclareVariable( "placename", otString );
  Qry.DeclareVariable( "whatdo", otInteger );
  Qry.SetVariable( "point_id", trip_id );
  Qry.SetVariable( "pax_id", pax_id );
  Qry.SetVariable( "placename", nplaceName );
  Qry.SetVariable( "whatdo", InUse );
  try {
    Qry.Execute();
  }
  catch( EOracleError e ) {
    tst();
    if ( e.Code == 1403 ) {
      return false;
    }
    else throw;
  }
  if ( seatstype == sreseats ) {
    reqinfo->MsgToLog( string( "���ᠦ�� " ) + fullname +
                       " ���ᠦ��. ����� ����: " + nplaceName,
                       evtPax, trip_id, reg_no, grp_id );
  }
  else {
  	if ( pr_cancel )
      reqinfo->MsgToLog( string( "���ᠦ��� " ) + fullname +
                         " �⬥���� �।���⥫쭮 �����祭��� ����: " + placeName,
                         evtPax, trip_id, reg_no, grp_id );
    else
      reqinfo->MsgToLog( string( "���ᠦ��� " ) + fullname +
                         " �।���⥫쭮 �����祭� ����. ����� ����: " + nplaceName,
                         evtPax, trip_id, reg_no, grp_id );
  }
  tid = new_tid;
  return true;
}*/

/* ��⮬���᪠� ��ᠤ�� ���ᠦ�஢
   ����� ��ᠤ�� ���ᠦ�஢ �� ��������� ���������� ��
   ᠫ�� ����㦥�, ����� ���� ���� - �ࠡ�⠫� setcraft ��� � �����묨 ���⠬� */
void ReSeatsPassengers( TSalons *Salons, bool DeleteNotFreePlaces, bool SeatOnNotBase )
{
  if ( !Salons )
    throw Exception( "�� ����� ᠫ�� ��� ��⮬���᪮� ��ᠤ��" );
  ProgTrace( TRACE5, "SalonsInterface::ReSeat with params: trip_id=%d, DeleteNotFreePlaces=%d, SeatOnNotBase=%d",
             Salons->trip_id, DeleteNotFreePlaces, SeatOnNotBase );
  CurrSalon = Salons;
  SeatAlg = sSeatPassengers;
  CanUseStatus = false; /* �� ���뢠�� ����� ���� */
  CanUse_PS = true;
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
          SeatPlaces.SeatsPassengers( true );
          tst();
          SeatPlaces.RollBack( );
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
    TQuery SQry( &OraSession );
    SQry.SQLText = "SELECT tid__seq.nextval AS tid FROM dual";
    TQuery Qry( &OraSession );
    Qry.SQLText = "BEGIN "\
                  "  UPDATE pax set seat_no=:seat_no,prev_seat_no=:prev_seat_no,tid=tid__seq.currval "\
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
    int new_tid;
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
          SQry.Execute();
          new_tid = SQry.FieldAsInteger( "tid" );
          Qry.Execute();
          pass.tid = new_tid;
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
          SQry.Execute();
          new_tid = SQry.FieldAsInteger( "tid" );
          Qry.Execute();
          pass.tid = new_tid;
        }
      }
      Passengers.Add( pass );
    }
  }
  catch( ... ) {
    SeatPlaces.RollBack( );
    throw;
  }
  SeatPlaces.RollBack( );
  ProgTrace( TRACE5, "passengers.count=%d", p.getCount() );
}

void SavePlaces( )
{
  TQuery Qry( &OraSession );
  TQuery RQry( &OraSession ); //㤠����� ���஭�஢����� ���� � ᠫ���
  RQry.SQLText =
    "DECLARE "
    " vpreseat crs_pax.preseat_no%TYPE; "
    "BEGIN "
    " :x := -1; :y := -1; :num := -1; "
    " BEGIN "
    "  SELECT preseat_no INTO vpreseat "
    "  FROM crs_pax "
    "  WHERE pax_id=:pax_id; "
    "  SELECT x, y, num INTO :x, :y, :num "
    "  FROM  trip_comp_elems "
    "  WHERE point_id = :point_id AND "
    "        yname||xname=DECODE( INSTR( vpreseat, '0' ), 1, SUBSTR( vpreseat, 2 ), vpreseat ) AND "
    "        status='PS'; "
    "  EXCEPTION WHEN NO_DATA_FOUND THEN NULL; "
    " END; "
    "END;";
  RQry.CreateVariable( "point_id", otInteger, CurrSalon->trip_id );
  RQry.DeclareVariable( "pax_id", otInteger );
  RQry.DeclareVariable( "x", otInteger );
  RQry.DeclareVariable( "y", otInteger );
  RQry.DeclareVariable( "num", otInteger );
// ����⨥ ���� � �᢮�������� ���஭�஢����� ����, �᫨ ⠪�� ����
  Qry.SQLText =
    "BEGIN "
    " UPDATE trip_comp_elems SET pr_free=NULL "
    "  WHERE point_id=:point_id AND num=:num AND x=:x AND y=:y AND pr_free IS NOT NULL; "
    " IF :bnum != -1 THEN "
    "  UPDATE trip_comp_elems "
    "    SET status='FP' "
    "  WHERE pr_free IS NOT NULL AND enabled IS NOT NULL AND "
    "        status = 'PS' AND "
    "        elem_type IN ( SELECT code FROM comp_elem_types WHERE pr_seat <> 0 ) AND "
    "        point_id = :point_id AND num = :bnum AND "
    "        x = :bx AND y = :by; "
    " END IF; "
    "END;";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.DeclareVariable( "num", otInteger );
  Qry.DeclareVariable( "x", otInteger );
  Qry.DeclareVariable( "y", otInteger );
  Qry.DeclareVariable( "bnum", otInteger );
  Qry.DeclareVariable( "bx", otInteger );
  Qry.DeclareVariable( "by", otInteger );
  Qry.SetVariable( "point_id", CurrSalon->trip_id );
  TPoint p, bp;
  for ( int i=0; i<Passengers.getCount(); i++ ) {
    TPassenger &pass = Passengers.Get( i );
    RQry.SetVariable( "pax_id", pass.pax_id );
    tst();
    RQry.Execute();
    Qry.SetVariable( "num", pass.placeList->num );
    Qry.SetVariable( "bnum", RQry.GetVariableAsInteger( "num" ) );
    bp.x = RQry.GetVariableAsInteger( "x" );
    bp.y = RQry.GetVariableAsInteger( "y" );
    ProgTrace( TRACE5, "bnum=%d, bx=%d, by=%d",
               RQry.GetVariableAsInteger( "num" ),
               RQry.GetVariableAsInteger( "x" ),
               RQry.GetVariableAsInteger( "y" ) );
    p = pass.Pos;
    for ( int j=0; j<pass.countPlace; j++ ) {
      Qry.SetVariable( "x", p.x );
      Qry.SetVariable( "y", p.y );
      Qry.SetVariable( "bx", bp.x );
      Qry.SetVariable( "by", bp.y );
      switch ( pass.Step ) {
        case sLeft:
          p.x--;
          bp.x--;
          break;
        case sRight:
          p.x++;
          bp.x++;
          break;
       case sUp:
         p.y--;
         bp.y--;
         break;
       case sDown:
         p.y++;
         bp.y++;
         break;
      }
      Qry.Execute();
    }
  }
}

} /* end namespace SEATS */

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
    NewTextChild( pNode, "tid", p->tid );
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

void NormalizeSeat(TSeat &seat)
{
  if (!is_iata_row(seat.row)) throw EConvertError("NormalizeSeat: non-IATA row '%s'",seat.row);
  if (!is_iata_line(seat.line)) throw EConvertError("NormalizeSeat: non-IATA line '%s'",seat.line);

  strncpy(seat.row,norm_iata_row(seat.row).c_str(),sizeof(seat.row));
  strncpy(seat.line,norm_iata_line(seat.line).c_str(),sizeof(seat.line));

  if (strnlen(seat.row,sizeof(seat.row)) >= sizeof(seat.row))
    throw EConvertError("NormalizeSeat: error in procedure norm_iata_row");
  if (strnlen(seat.line,sizeof(seat.line)) >= sizeof(seat.line))
    throw EConvertError("NormalizeSeat: error in procedure norm_iata_line");
};

void NormalizeSeatRange(TSeatRange &range)
{
  NormalizeSeat(range.first);
  NormalizeSeat(range.second);
};

bool NextNormSeatRow(TSeat &seat)
{
  int i;
  if (StrToInt(seat.row,i)==EOF)
    throw EConvertError("NextNormSeatRow: error in procedure norm_iata_row");
  if (i==199) return false;
  if (i==99) i=101; else i++;
  strncpy(seat.row,norm_iata_row(IntToString(i)).c_str(),sizeof(seat.row));
  if (strnlen(seat.row,sizeof(seat.row)) >= sizeof(seat.row))
    throw EConvertError("NextNormSeatRow: error in procedure norm_iata_row");
  return true;
};

bool PriorNormSeatRow(TSeat &seat)
{
  int i;
  if (StrToInt(seat.row,i)==EOF)
    throw EConvertError("PriorNormSeatRow: error in procedure norm_iata_row");
  if (i==1) return false;
  if (i==101) i=99; else i--;
  strncpy(seat.row,norm_iata_row(IntToString(i)).c_str(),sizeof(seat.row));
  if (strnlen(seat.row,sizeof(seat.row)) >= sizeof(seat.row))
    throw EConvertError("PriorNormSeatRow: error in procedure norm_iata_row");
  return true;
};

TSeat& FirstNormSeatRow(TSeat &seat)
{
  strncpy(seat.row,norm_iata_row(IntToString(1)).c_str(),sizeof(seat.row));
  if (strnlen(seat.row,sizeof(seat.row)) >= sizeof(seat.row))
    throw EConvertError("FirstNormSeatRow: error in procedure norm_iata_row");
  return seat;
};

TSeat& LastNormSeatRow(TSeat &seat)
{
  strncpy(seat.row,norm_iata_row(IntToString(199)).c_str(),sizeof(seat.row));
  if (strnlen(seat.row,sizeof(seat.row)) >= sizeof(seat.row))
    throw EConvertError("LastNormSeatRow: error in procedure norm_iata_row");
  return seat;
};


bool NextNormSeatLine(TSeat &seat)
{
  char *p;
  if ((p=strchr(lat_seat,seat.line[0]))==NULL)
    throw EConvertError("NextNormSeatLine: error in procedure norm_iata_line");
  p++;
  if (*p==0) return false;
  seat.line[0]=*p;
  seat.line[1]=0;
  return true;
};

bool PriorNormSeatLine(TSeat &seat)
{
  char *p;
  if ((p=strchr(lat_seat,seat.line[0]))==NULL)
    throw EConvertError("PriorNormSeatLine: error in procedure norm_iata_line");
  if (p==lat_seat) return false;
  p--;
  seat.line[0]=*p;
  seat.line[1]=0;
  return true;
};

TSeat& FirstNormSeatLine(TSeat &seat)
{
  if (strlen(lat_seat)<=0)
    throw EConvertError("FirstNormSeatLine: lat_seat is empty!");
  seat.line[0]=lat_seat[0];
  seat.line[1]=0;
  return seat;
};

TSeat& LastNormSeatLine(TSeat &seat)
{
  if (strlen(lat_seat)<=0)
    throw EConvertError("LastNormSeatLine: lat_seat is empty!");
  seat.line[0]=lat_seat[strlen(lat_seat)-1];
  seat.line[1]=0;
  return seat;
};

bool NextNormSeat(TSeat &seat)
{
  if (!NextNormSeatLine(seat))
  {
    if (!NextNormSeatRow(seat)) return false;
    FirstNormSeatLine(seat);
  };
  return true;
};

bool SeatInRange(TSeatRange &range, TSeat &seat)
{
  return strcmp(seat.row,range.first.row)>=0 &&
         strcmp(seat.row,range.second.row)<=0 &&
         strcmp(seat.line,range.first.line)>=0 &&
         strcmp(seat.line,range.second.line)<=0;
};

bool NextSeatInRange(TSeatRange &range, TSeat &seat)
{
  if (!SeatInRange(range,seat)) return false;
  if (strcmp(seat.line,range.second.line)>=0)
  {
    if (strcmp(seat.row,range.second.row)>=0) return false;
    strcpy(seat.line,range.first.line);
    NextNormSeatRow(seat);
  }
  else NextNormSeatLine(seat);
  return true;
};

bool SeatNoInSeats( vector<TSeat> &seats, string seat_no)
{
 /* for(vector<TSeat>::iterator iSeat=seats.begin();iSeat!=seats.end();iSeat++)
    if ((string)iSeat->row == seat_no)!!!*/
  return false;
};

