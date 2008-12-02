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

const int CONST_MAXPLACE = 3;

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
enum TUseRem { sAllUse, sMaxUse, sOnlyUse, sNotUse, sNotUseDenial, sIgnoreUse, useremLength };
/* ����� ࠧ������ ���, ����� ᠦ��� �� ������ ����� ������ ࠧ�, �� ����� */
enum TUseAlone { uFalse3 /* ����� ��⠢���� ������ �� ��ᠤ�� ��㯯�*/,
	               uFalse1 /* ����� ��⠢���� ������ ⮫쪮 ���� ࠧ �� ��ᠤ�� ��㯯�*/,
	               uTrue /*����� ��⠢���� ������ �� ��ᠤ�� ��㯯� �� ���-�� ࠧ*/ };


string DecodeCanUseRems( TUseRem VCanUseRems )
{
	string res;
	switch( VCanUseRems ) {
		case sAllUse:
			  res = "sAllUse";
			  break;
		case sOnlyUse:
			  res = "sOnlyUse";
			  break;
		case sMaxUse:
			  res = "sMaxUse";
			  break;
		case sNotUseDenial:
			  res = "sNotUseDenial";
			  break;
		case sNotUse:
			  res = "sNotUse";
			  break;
		case sIgnoreUse:
			  res = "sIgnoreUse";
			  break;
		default:;
	}
	return res;
}



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

bool canUseOneRow=true; // �ᯮ�짮���� �� ��ᠤ�� ⮫쪮 ���� ��

int FSeatAlgo=0; // ������ ��ᠤ��

int MAXPLACE() {
	return (canUseOneRow)?1000:CONST_MAXPLACE; // �� ��ᠤ�� � ���� �� ���-�� ���� � ��� ����࠭�祭� ���� 3 ����
};

bool getCanUseOneRow() {
	return FSeatAlgo; //???
}

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

    /* �����稫� ��।����� ��室��� ������ ���� � ���ࠢ����� ��ᠤ */

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
//      ProgTrace( TRACE5, "placename=%s, pr_free=false", string(pl->yname+pl->xname).c_str() );
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
	bool res = false; // �ਧ��� ⮣�, �� � ���ᠦ�� ���� ६�ઠ, ����� ����饭� �� ��࠭��� ����
	bool pr_passmcls = find( Remarks.begin(), Remarks.end(), string("MCLS") ) != Remarks.end(); // �ਧ��� � ���ᠦ�� MCLS
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
//		ProgTrace( TRACE5, "prem->rem=%s", prem->rem.c_str() );
  	if ( !prem->pr_denial && prem->rem == "MCLS" && !pr_passmcls )
	  		no_mcls = true;
	}
//	ProgTrace( TRACE5, "res=%d, mcls=%d", res, no_mcls );
  return res || no_mcls; // �᫨ ���� ����饭��� ६�ઠ � ���ᠦ�� ��� ���� � ६�મ� MCLS, � � ���ᠦ�� �� ���
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
          Result + foundCount < MAXPLACE() &&
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
         ProgTrace( TRACE5, "result=%d, FP=(%d,%d)", Result, FP.x, FP.y );
         break;
      case sNotUseDenial:
      	if ( DoNotRemsSeats( place->rems ) )
      		return Result;
      	break;
      case sNotUse:
//      	 ProgTrace( TRACE5, "sNotUse: Result=%d, FP.x=%d, FP.y=%d", Result, FP.x, FP.y );
  	     for( vector<TRem>::const_iterator prem=place->rems.begin(); prem!=place->rems.end(); prem++ ) {
		       if ( !prem->pr_denial )
		       	 return Result;
  	     }
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
  if ( !Wanted && seatplaces.empty() && CanUseAlone != uFalse3 ) /*����� ��⠢���� ������*/
    Alone = true;// �� ���� ��室 �, ���� �ந��樠����஢��� ��. ��६����� Alone
  int foundAfter = FindPlaces_From( FP, 0, Step );
  if ( !foundAfter )
    return false;
//  ProgTrace( TRACE5, "FP=(%d,%d), foundafter=%d", FP.x, FP.y, foundAfter );
  foundCount += foundAfter;
  if ( foundAfter && Wanted ) { // �᫨ �� ��諨 ���� � ��� ���� Wanted
    if ( foundAfter > Wanted )
      foundAfter = Wanted;
    Wanted -= Put_Find_Places( FP, FP, foundAfter, Step );
    if ( Wanted <= 0 ) {
      return true; // �� �� ��諮��
    }
  }
  TPoint EP = FP;
  if ( foundAfter < MAXPLACE() ) {
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
//  ProgTrace( TRACE5, "FP=(%d,%d), foundbefore=%d", FP.x, FP.y, foundBefore );
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
    if ( Wanted <= 0 ) {
      return true; /* �� �� ��諮�� */
    }
  }
 /* ����� ����⠥��� ���᪠�� �१ ��室, �� �᫮��� �� ���� �� ��ਧ��⠫� */
  if ( CanUseTube && Step == sRight && foundCount < MAXPLACE() ) {
    EP.x = FP.x + foundAfter + 1; //???/* ��⠭���������� �� �।���������� ���� */
//    ProgTrace( TRACE5, "EP=(%d,%d)", EP.x, EP.y );
    if ( placeList->ValidPlace( EP ) ) {
      TPoint p( EP.x - 1, EP.y );
      TPlace *place = placeList->place( p ); /* ��६ �।. ���� */
      if ( !place->visible ) {
        foundTubeAfter = FindPlaces_From( EP, foundCount, Step ); /* ���� ��᫥ ��室� */
        foundCount += foundTubeAfter; /* 㢥��稢��� ��饥 ���-�� ���� */
//       ProgTrace( TRACE5, "FP=(%d,%d), foundTubeAfter=%d", EP.x, EP.y, foundTubeAfter );
        if ( foundTubeAfter && Wanted ) { /* �᫨ �� ��諨 ���� � ��� ���� Wanted */
          if ( foundTubeAfter > Wanted )
            foundTubeAfter = Wanted;
           Wanted -= Put_Find_Places( EP, EP, foundTubeAfter, Step ); /* ���� ��ࠬ��� EP */
         /* �.�. �窠 ����� ������ ��室���� �� ��ࢮ� ���� ��᫥ ��室�,
            ���� ࠡ���� �� �㤥� */
           if ( Wanted <= 0 ) {
             return true; /* �� �� ��諮�� */
           }
        }
      }
    }
    /* ����� ���� ������ �१ ��室 */
    if ( foundCount < MAXPLACE() ) {
      EP.x = FP.x - foundBefore - 2; // ��⠭���������� �� �।���������� ����
      TPoint VP = EP;
//      ProgTrace( TRACE5, "EP=(%d,%d)", EP.x, EP.y );
      if ( placeList->ValidPlace( EP ) ) {
        TPoint p( EP.x + 1, EP.y );
        TPlace *place = placeList->place( p ); /* ��६ ᫥�. ���� */
        if ( !place->visible ) { /* ᫥���饥 ���� �� ����� => ��室 */
          foundTubeBefore = FindPlaces_From( EP, foundCount, sLeft );
          foundCount += foundTubeBefore;
//          ProgTrace( TRACE5, "EP=(%d,%d), foundTubeBefore=%d", EP.x, EP.y, foundTubeBefore );
          if ( foundTubeBefore && Wanted ) { /* �᫨ �� ��諨 ���� � ��� ���� Wanted */
            if ( foundTubeBefore > Wanted )
              foundTubeBefore = Wanted;
            EP.x -= foundTubeBefore - 1;
            Wanted -= Put_Find_Places( EP, EP, foundTubeBefore, sLeft ); /* ���� ��ࠬ��� EP */
            if ( Wanted <= 0 ) {
              return true; /* �� �� ��諮�� */
            }
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
  if ( Step == sRight && foundCount == MAXPLACE() &&
       counters.p_Count_3() == Passengers.counters.p_Count_3() &&
       counters.p_Count_2() == Passengers.counters.p_Count_2() &&
       counters.p_Count() == Passengers.counters.p_Count() - MAXPLACE() - 1 ) {
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
    foundCount += Put_Find_Places( FP, EP, foundTubeBefore, Step );
  }
  if ( EndWanted > 0 ) { /* ���� 㤠���� ���� ᠬ�� ���宥 ����: */
    /* ᠬ�� 㤠������ �� ��᫥	����� ���������� � ⥪�饬 ��� */
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
  if ( foundCount == 1 && CanUseAlone != uTrue ) /*����� ��⠢���� ������*/
    if ( Alone )
      Alone = false;
    else  /* ����� 2 ࠧ� �⮡� ��﫮�� ���� ���� � ���. */
      return false; /*( p_Count_3( Step ) = Passengers.p_Count_3( Step ) )AND
               ( p_Count_2( Step ) = Passengers.p_Count_2( Step ) )AND
               ( p_Count( Step ) = Passengers.p_Count( Step ) ) ???};*/
  if ( counters.p_Count_3( Step ) == Passengers.counters.p_Count_3( Step ) &&
       counters.p_Count_2( Step ) == Passengers.counters.p_Count_2( Step ) &&
       counters.p_Count( Step ) == Passengers.counters.p_Count( Step ) ) {
    return true;
  }
  int lines = placeList->GetXsCount(), visible=0;
  for ( int line=0; line<lines; line++ ) {
  	TPoint f=FP;
  	f.x = line;
//  	ProgTrace( TRACE5, "line=%d, name=%s", line, placeList->GetXsName( line ).c_str() );
  	if ( placeList->GetXsName( line ).empty() ||
  		   !placeList->ValidPlace( f ) ||
  		   !placeList->place( f )->visible ||
  		   !placeList->place( f )->isplace )
  		continue;
    visible++;
  }
//  ProgTrace( TRACE5, "getCanUseOneRow()=%d, canUseOneRow=%d, foundCount=%d, visible=%d",
//             getCanUseOneRow(), canUseOneRow, foundCount, visible );
  if ( !getCanUseOneRow() || !canUseOneRow || canUseOneRow && foundCount == visible ) {
  /* ���室�� �� ᫥���騩 �� ???canUseOneRow??? */
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
         !SeatSubGrp_On( EP, Step, 0 ) ) { // ��祣� �� ᬮ��� ���� �����
      EP = FP;
      switch( (int)Step ) {
        case sRight:
           EP.y--;
           break;
        case sDown:
           EP.x--;
           break;
      }
      if ( EP.x < 0 || EP.y < 0 ||
           !SeatSubGrp_On( EP, Step, 0 ) ) // ��祣� �� ᬮ��� ���� �����
        return false;
    }
  }
  else return false;
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
  string ispPlaceName_lat, ispPlaceName_rus;
  for (ISeatPlace isp=seatplaces.begin(); isp!=seatplaces.end(); isp++) {
//    ProgTrace( TRACE5, "isp->oldPlaces.size()=%d, pass.countPlace=%d",
//               (int)isp->oldPlaces.size(), pass.countPlace );
    if ( isp->InUse || (int)isp->oldPlaces.size() != pass.countPlace ) /* ���-�� ���� ������ ᮢ������ */
      continue;
  	ispPlaceName_lat.clear();
  	ispPlaceName_rus.clear();
    TPlaceList *placeList = isp->placeList;
    ispPlaceName_lat = denorm_iata_row( placeList->GetYsName( isp->Pos.y ) );
    ispPlaceName_rus = ispPlaceName_lat;
    ispPlaceName_lat += denorm_iata_line( placeList->GetXsName( isp->Pos.x ), 1 );
    ispPlaceName_rus += denorm_iata_line( placeList->GetXsName( isp->Pos.x ), 0 );

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
    //ProgTrace( TRACE5, "EqualQ=%d", EqualQ );

    if ( pass.placeName == ispPlaceName_lat || pass.placeName == ispPlaceName_rus ||
    	   pass.preseat == ispPlaceName_lat || pass.preseat == ispPlaceName_rus )
      EqualQ += PR_EQUAL_N_PLACE;
    //ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( pass.placeRem.find( "SW" ) == 2  &&
        ( isp->Pos.x == 0 || isp->Pos.x == placeList->GetXsCount() - 1 ) ||
         pass.placeRem.find( "SA" ) == 2  &&
        ( isp->Pos.x - 1 >= 0 &&
          placeList->GetXsName( isp->Pos.x - 1 ).empty() ||
          isp->Pos.x + 1 < placeList->GetXsCount() &&
          placeList->GetXsName( isp->Pos.x + 1 ).empty() )
       )
      EqualQ += PR_EQUAL_REMPLACE;
//    ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( pass.prSmoke == isp->oldPlaces.begin()->pr_smoke )
      EqualQ += PR_EQUAL_SMOKE;
//    ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
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
//    ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( MaxEqualQ < EqualQ ) {
      MaxEqualQ = EqualQ;
      misp = isp;
      misp->isValid = pr_valid_place;
    }
  } /* end for */
 //ProgTrace( TRACE5, "MaxEqualQ=%d", MaxEqualQ );
 misp->InUse = true;
 return *misp;
}


bool CompSeats( TSeatPlace item1, TSeatPlace item2 )
{
	if ( item1.Pos.y < item2.Pos.y )
	  return true;
	else
		if ( item1.Pos.y > item2.Pos.y )
			return false;
		else
			if ( item1.Pos.x < item2.Pos.x )
				return true;
			else
				if ( item1.Pos.x > item2.Pos.x )
					return false;
				else
					return true;
};


void TSeatPlaces::PlacesToPassengers()
{
  if ( seatplaces.empty() )
   return;
  for (ISeatPlace isp=seatplaces.begin(); isp!=seatplaces.end(); isp++)
    isp->InUse = false;
  sort(seatplaces.begin(),seatplaces.end(),CompSeats);


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
//  ProgTrace( TRACE5, "FP(x=%d, y=%d)", FP.x, FP.y );
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
  try {
    if ( !placeName.empty() ) {
      /* ��������� ����஢ ���� ���ᠦ�஢ � ����ᨬ��� �� ���. ��� ���. ᠫ��� */
      for ( vector<TPlaceList*>::iterator iplaceList=CurrSalon->placelists.begin();
            iplaceList!=CurrSalon->placelists.end(); iplaceList++ ) {
        TPoint FP;
        if ( (*iplaceList)->GetisPlaceXY( placeName, FP ) ) {
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
//         		ProgTrace( TRACE5, "TUseRem=%s", DecodeCanUseRems( CanUseRems ).c_str() );
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
//  ProgTrace( TRACE5, "SeatsPassengers( ), canusemcls=%d", canUseMCLS );
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
//  ProgTrace( TRACE5, "pass.countPlace=%d", countPlace );
  for ( int j=0; j<countPlace; j++ ) {
    TSeat s;
//    ProgTrace( TRACE5, "pass.placeList->GetXsName( %d )=%s, pass.placeList->GetYsName( %d )=%s",
//    	           x, placeList->GetXsName( x ).c_str(), y, placeList->GetYsName( y ).c_str() );
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

void TPassengers::LoadRemarksPriority( )
{
	remarks.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT code, pr_comp FROM rem_types WHERE pr_comp IS NOT NULL";
  Qry.Execute();
  while ( !Qry.Eof ) {
    remarks[ Qry.FieldAsString( "code" ) ] = Qry.FieldAsInteger( "pr_comp" );
    Qry.Next();
  }
}

void TPassengers::addRemPriority( TPassenger &pass )
{
  int priority = 0;
  pass.maxRem.clear();
  if ( remarks.empty() )
  	LoadRemarksPriority();
  for ( vector<string>::iterator irem = pass.rems.begin(); irem != pass.rems.end(); irem++ ) {
    if ( remarks[ *irem ] > priority ) {
      priority = remarks[ *irem ];
      pass.maxRem = *irem;
    }
    pass.priority += remarks[ *irem ]*pass.countPlace; //???
  }
//???  pass.priority += priority*pass.countPlace;
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
//ProgTrace( TRACE5, "agent_seat=%s, preseat=%s, placeName=%s",
//	           pass.agent_seat.c_str(), pass.preseat.c_str(), pass.placeName.c_str() );
  if ( pass.countPlace > CONST_MAXPLACE || pass.countPlace <= 0 )
   throw Exception( "�� �����⨬�� ���-�� ���� ��� �ᠤ��" );

  size_t i = 0;
  for (; i < pass.agent_seat.size(); i++)
    if ( pass.agent_seat[ i ] != '0' )
      break;
  if ( i )
    pass.agent_seat.erase( 0, i );
	if ( !pass.agent_seat.empty() )
	 pass.placeName = pass.agent_seat;
  if ( !pass.preseat.empty() && !pass.placeName.empty() && pass.preseat != pass.placeName ) {
    pass.placeName = pass.preseat; //!!! �� ॣ����樨 ����� �������� �।���⥫쭮 �����祭��� ����
  }
//  ProgTrace( TRACE5, "pass.placeName=%s, pass.oldplacename=%s", pass.placeName.c_str(), pass.OldPlaceName.c_str() );

//  ProgTrace( TRACE5, "pass.placeStatus=%s, pass.preseat=%s", pass.placeStatus.c_str(), pass.preseat.c_str() );
  if ( pass.placeStatus == "BR" && !pass.preseat.empty() && pass.preseat == pass.placeName )
  	pass.placeStatus = "PS"; //???

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
  if ( remarks.empty() )
  	LoadRemarksPriority();
//  ProgTrace( TRACE5, "pass.rems.size()=%d", pass.rems.size() );
  for (vector<string>::iterator ir=pass.rems.begin(); ir!=pass.rems.end();  ) {
  	if ( remarks.find( *ir ) == remarks.end() )
  		ir = pass.rems.erase( ir );
  	else
  		ir++;
  }
//  ProgTrace( TRACE5, "pass.rems.size()=%d", pass.rems.size() );
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

bool ExistsBasePlace( TSalons &Salons, TPassenger &pass )
{
  TPlaceList *placeList;
  TPoint FP;
  vector<TPlace*> vpl;
  string placeName = pass.placeName;
  for ( vector<TPlaceList*>::iterator plList=Salons.placelists.begin();
        plList!=Salons.placelists.end(); plList++ ) {
    placeList = *plList;
    if ( placeList->GetisPlaceXY( placeName, FP ) ) {
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
            pass.placeList = placeList;
            pass.Pos.x = (*ipl)->x;
            pass.Pos.y = (*ipl)->y;
            pass.set_seat_no();
          }
          (*ipl)->pr_free = false;
        }
        return true;
      }
      vpl.clear();
    }
  }
  return false;
}

namespace SEATS {
/* ��ᠤ�� ���ᠦ�஢ */
void SeatsPassengers( TSalons *Salons, int SeatAlgo /* 0 - 㬮�砭�� */, bool FUse_PS )
{
  if ( !Passengers.getCount() )
    return;
  FSeatAlgo = SeatAlgo;
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

  FindMCLS = false;
  canUseMCLS = false;
  bool pr_MCLS = false;

  /* �� ᤥ����!!! �᫨ � ��� ���ᠦ�஢ ���� ����, � ⮣�� ��ᠤ�� �� ���⠬, ��� ��� ��㯯� */

  /* ��।������ ���� �� � ��㯯� ���ᠦ�� � �।���⥫쭮� ��ᠤ��� */
  bool Status_seat_no_BR=false;
  for ( int i=0; i<Passengers.getCount(); i++ ) {
  	TPassenger &pass = Passengers.Get( i );
  	if ( pass.placeStatus == "PS" ) {
  		Status_preseat = true;
  	}
  	if ( find( pass.rems.begin(), pass.rems.end(), string( "MCLS" ) ) != pass.rems.end() ) {
  		pr_MCLS = true;
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
   for ( int FCanUserMCLS=(int)pr_MCLS; FCanUserMCLS>=0; FCanUserMCLS-- ) {
    FindMCLS = FCanUserMCLS;
    canUseMCLS = FCanUserMCLS;

   for ( int FSeatAlg=0; FSeatAlg<seatAlgLength; FSeatAlg++ ) {
     SeatAlg = (TSeatAlg)FSeatAlg;
     /* �᫨ ���� � ��㯯� �।���⥫쭠� ��ᠤ��, � ⮣�� ᠦ��� ��� �⤥�쭮 */
     if ( ( Status_preseat || Status_seat_no_BR || SeatOnlyBasePlace /*!!!*/ ) && SeatAlg != sSeatPassengers )
     	 continue;
     for ( int FCanUseRems=0; FCanUseRems<useremLength; FCanUseRems++ ) {
        CanUseRems = (TUseRem)FCanUseRems;
        switch( (int)SeatAlg ) {
          case sSeatGrpOnBasePlace:
             switch( (int)CanUseRems ) {
               case sIgnoreUse:
               case sNotUseDenial:
               case sNotUse:
                 continue;
             }
             break;
          case sSeatGrp:
             switch( (int)CanUseRems ) {
               case sAllUse:
               case sMaxUse:
               case sOnlyUse:
               case sIgnoreUse:
               case sNotUseDenial: //???
                 continue; /*??? �� ������� ��㯯� ��� ���� � ६�ઠ��, ��� �� ���� ���뢠�� */
             }
             break;
        }
        /* ��⠢���� ������ ��᪮�쪮 ࠧ �� �鸞� �� ���ᠤ�� ��㯯� */
        for ( int FCanUseAlone=uFalse3; FCanUseAlone<=uTrue; FCanUseAlone++ ) {
          CanUseAlone = (TUseAlone)FCanUseAlone;
/*          if ( CanUseAlone == uFalse3 &&
               !( !Passengers.counters.p_Count_3( sRight ) &&
                  !Passengers.counters.p_Count_2( sRight ) &&
                  Passengers.counters.p_Count( sRight ) == 3 &&
                  !Passengers.counters.p_Count_3( sDown ) &&
                  !Passengers.counters.p_Count_2( sDown )
                 ) ) //???
            continue;???*/
          if ( CanUseAlone == uTrue && SeatAlg == sSeatPassengers )
            continue;
          /* �ᯮ�짮����� ����ᮢ ���� */
          for ( int KeyStatus=1; KeyStatus>=0; KeyStatus-- ) {
            if ( !KeyStatus && CanUseAlone == uFalse3 )
              continue;
            if ( !KeyStatus && ( SeatAlg == sSeatGrpOnBasePlace || SeatAlg == sSeatGrp ) )
            	continue;

            /* ������ ���ᨢ ����ᮢ ���� */
            SetStatuses( Statuses, Passengers.Get( 0 ).placeStatus, KeyStatus, Status_preseat );
            /* �஡�� �� ����ᮬ */
            for ( vector<string>::iterator st=Statuses.begin(); st!=Statuses.end(); st++ ) {
              PlaceStatus = *st;
              CanUseGood = ( PlaceStatus != "NG" ); /* ��㤮��� ���� */
              if ( !CanUseGood )
                PlaceStatus = "FP";
              /* ��� ०��� ��ᠤ�� � ����� ��� */
              for ( int FCanUseOneRow=getCanUseOneRow(); FCanUseOneRow>=0; FCanUseOneRow-- ) {
              	canUseOneRow = FCanUseOneRow;
                /* ��� ��室�� */
                for ( int FCanUseTube=0; FCanUseTube<=1; FCanUseTube++ ) {
                  /* ��� ��ᠤ�� �⤥���� ���ᠦ�஢ �� ���� ���뢠�� ��室� */
                  if ( FCanUseTube && SeatAlg == sSeatPassengers )
                    continue;
                  if ( FCanUseTube && CanUseAlone == uFalse3 )
                    continue;
              	  if ( canUseOneRow && !FCanUseTube )
              	  	continue;
                  CanUseTube = FCanUseTube;
                  for ( int FCanUseSmoke=Passengers.UseSmoke; FCanUseSmoke>=0; FCanUseSmoke-- ) {
/*                    if ( !FCanUseSmoke && CanUseAlone == uFalse3 )
                      continue; ???*/

                    CanUseSmoke = FCanUseSmoke;
/*                    ProgTrace( TRACE5, "seats with:SeatAlg=%d,FCanUseRems=%s,FCanUseAlone=%d,KeyStatus=%d,FCanUseTube=%d,FCanUseSmoke=%d,CanUseGood=%d,PlaceStatus=%s, MAXPLACE=%d,canUseOneRow=%d, CanUseMCLS=%d",
                               (int)SeatAlg,DecodeCanUseRems( CanUseRems ).c_str(),FCanUseAlone,KeyStatus,FCanUseTube,FCanUseSmoke,CanUseGood,PlaceStatus.c_str(), MAXPLACE(),canUseOneRow,canUseMCLS);*/
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
                  } /* end for smoke */
                } /* end for tube */
              } /* end for canuseonerow */
            } /* end for status */
          } /* end for use status */
        } /* end for alone */
      } /* end for CanUseRem */
    } /* end for FSeatAlg */
   }
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

bool GetPassengersForManualSeat( int point_id, TCompLayerType layer_type, TPassengers &p, bool pr_lat_seat )
{
	bool res = false;
	p.Clear();
  TQuery Qry( &OraSession );
  TQuery QrySeat( &OraSession );
  switch( layer_type ) {
  	case cltCheckin:
	    Qry.SQLText = //!!!
        "SELECT points.airline,"
        "       pax_grp.grp_id,"
        "       pax.pax_id,"
        "       pax.reg_no,"
        "       surname,"
        "       pax.name, "
        "       pax_grp.class,"
        "       cls_grp.code subclass,"
        "       seats,"
        "       pax.tid,"
        "       step, "
        "  salons.get_seat_no(pax.pax_id,:layer_type,pax.seats,pax_grp.point_dep,'one',rownum) AS seat_no "
        " FROM pax_grp,pax, cls_grp, points, "
        "( SELECT COUNT(*) step, pax_id FROM pax_rem "
        "   WHERE rem_code = 'STCR' "
        "  GROUP BY pax_id ) a "
        "WHERE pax_grp.grp_id=pax.grp_id AND "
        "      pax_grp.point_dep=:point_id AND "
        "      points.point_id = pax_grp.point_dep AND "
        "      pax_grp.class_grp = cls_grp.id AND "
        "      pax.pr_brd IS NOT NULL AND "
        "      seats > 0 AND "
        "      a.pax_id(+) = pax.pax_id "
        "ORDER BY pax.pax_id, pax.reg_no,pax_grp.grp_id ";
      QrySeat.SQLText =
        "SELECT first_xname, first_yname FROM trip_comp_layers "
        " WHERE point_id=:point_id AND pax_id=:pax_id ";
      break;
    default:
    	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct GetPassengersForManualSeat",  EncodeCompLayerType( layer_type ) );
    	throw UserException( "��⠭��������� ᫮� ����饭 ��� ��筮� ��ᠤ�� ���ᠦ�஢" );
  }
  QrySeat.CreateVariable( "point_id", otInteger, point_id );
  QrySeat.DeclareVariable( "pax_id", otInteger );
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType(layer_type) );
  Qry.Execute();
  int pax_id = -1;
  while ( !Qry.Eof ) {
    if ( pax_id != Qry.FieldAsInteger( "pax_id" ) ) {
      TPassenger pass;
      pass.pax_id = Qry.FieldAsInteger( "pax_id" );
      pass.placeName = Qry.FieldAsString( "seat_no" );
      pass.clname = Qry.FieldAsString( "class" );
      pass.countPlace = Qry.FieldAsInteger( "seats" );
      pass.tid = Qry.FieldAsInteger( "tid" );
      pass.grpId = Qry.FieldAsInteger( "grp_id" );
      pass.regNo = Qry.FieldAsInteger( "reg_no" );
      string fname = Qry.FieldAsString( "surname" );
      pass.fullName = TrimString( fname ) + " " + Qry.FieldAsString( "name" );
      pass.InUse = ( !pass.placeName.empty() );
      pass.isSeat = pass.InUse;

      if ( !pass.InUse ) { // ???����室��� ����� �।��饥 ����
      	res = true;
      	QrySeat.SetVariable( "pax_id", pass.pax_id );
      	QrySeat.Execute();
      	if ( !QrySeat.Eof )
      		//!!!pass.PrevPlaceName =
      		pass.placeName = denorm_iata_row( QrySeat.FieldAsString( "first_yname" ) ) +
      		                 denorm_iata_line( QrySeat.FieldAsString( "first_xname" ), pr_lat_seat );
      }

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
      ProgTrace( TRACE5, "seat_no=%s", pass.placeName.c_str() );
      p.Add( pass );
      pax_id = pass.pax_id;
    }
    Qry.Next();
  }
  return res;
}

void SaveTripSeatRanges( int point_id, TCompLayerType layer_type, vector<TSeatRange> &seats,
                         int pax_id, int point_dep, int point_arv )
{
  if (seats.empty()) return;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  SELECT comp_layers__seq.nextval INTO :range_id FROM dual; "
    "  INSERT INTO trip_comp_layers "
    "    (range_id,point_id,point_dep,point_arv,layer_type, "
    "     first_xname,last_xname,first_yname,last_yname,pax_id) "
    "  VALUES "
    "    (:range_id,:point_id,:point_dep,:point_arv,:layer_type, "
    "     :first_xname,:last_xname,:first_yname,:last_yname,:pax_id); "
    "END; ";
  Qry.CreateVariable( "range_id", otInteger, FNull );
  Qry.CreateVariable( "point_id", otInteger,point_id );
  Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
  if ( pax_id > 0 )
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
  else
  	Qry.CreateVariable( "pax_id", otInteger, FNull );
  if ( point_dep > 0 )
    Qry.CreateVariable( "point_dep", otInteger, point_dep );
  else
  	Qry.CreateVariable( "point_dep", otInteger, FNull );
  if ( point_arv > 0 )
    Qry.CreateVariable( "point_arv", otInteger, point_arv );
  else
  	Qry.CreateVariable( "point_arv", otInteger, FNull );
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
    case cltProtCkin:
      Qry.SQLText =
        "SELECT xname, yname FROM trip_comp_elems t, "
        "(SELECT num, x, y, class FROM trip_comp_elems "
        "  WHERE point_id=:point_id AND xname = :xname AND yname = :yname ) a, "
        "( SELECT code FROM comp_elem_types WHERE pr_seat <> 0 ) e "
        " WHERE t.point_id=:point_id AND "
        " t.num = a.num AND "
        " t.class = a.class AND "
        " t.x = DECODE(:pr_down,0,1,0) + a.x AND "
        " t.y = DECODE(:pr_down,0,0,1) + a.y AND "
        " t.elem_type = e.code ";
      Qry.CreateVariable( "point_id", otInteger, point_id );
      Qry.CreateVariable( "xname", otString, r.first.line );
      Qry.CreateVariable( "yname", otString, r.first.row );
      Qry.CreateVariable( "pr_down", otInteger, pr_down );
      Qry.Execute();
      if ( Qry.RowCount() ) {
      	strcpy( r.first.line, Qry.FieldAsString( "xname" ) );
      	strcpy( r.first.row, Qry.FieldAsString( "yname" ) );
      	r.second = r.first;
//      	ProgTrace( TRACE5, "getNextSeat: xname=%s, yname=%s", r.first.line, r.first.row );
      	return true;
      }
      break;
    default:
    	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct getNextSeat",  EncodeCompLayerType( layer_type ) );
    	throw UserException( "��⠭��������� ᫮� ����饭 ��� ࠧ��⪨" );
  }
  return false; //!!! ᠫ��� ����� � �� ����, � ࠧ����� ����
}

void ChangeLayer( TCompLayerType layer_type, int point_id, int pax_id, int &tid,
                  string first_xname, string first_yname, TSeatsType seat_type, bool pr_lat_seat )
{
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
  Qry.Clear();
  /* ���뢠�� ���� �� ���ᠦ��� */
  switch ( layer_type ) {
  	case cltCheckin:
      Qry.SQLText =
       "SELECT surname, name, reg_no, pax.grp_id, pax.seats, a.step step, pax.tid, '' target, point_dep, point_arv, "
       "       0 point_id, salons.get_seat_no(pax.pax_id,:layer_type,pax.seats,:point_dep,'list',rownum) AS seat_no "
       " FROM pax, pax_grp, "
       "( SELECT COUNT(*) step FROM pax_rem "
       "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "
       "WHERE pax.pax_id=:pax_id AND "
       "      pax_grp.grp_id=pax.grp_id ";
       Qry.CreateVariable( "point_dep", otInteger, point_id );
      break;
    case cltProtCkin:
      Qry.SQLText =
        "SELECT surname, name, 0 reg_no, crs_pax.pnr_id grp_id, seats, a.step step, crs_pax.tid, target, point_id, 0 point_arv, "
        "      salons.get_crs_seat_no(crs_pax.pax_id,:layer_type,crs_pax.seats,crs_pnr.point_id,'list',rownum) AS seat_no "
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
  Qry.Execute();
  // ���ᠦ�� �� ������ ��� ����������������� � ��㣮� �⮩�� ��� �� �।�. ��ᠤ�� ���ᠦ�� 㦥 ��ॣ����஢��
  if ( !Qry.RowCount() ) {
    ProgTrace( TRACE5, "!!! Passenger not found in funct ChangeLayer" );
    throw UserException( "���ᠦ�� �� ������. ������� �����"	);
  }
  string fullname = Qry.FieldAsString( "surname" );
  TrimString( fullname );
  fullname += string(" ") + Qry.FieldAsString( "name" );
  int idx1 = Qry.FieldAsInteger( "reg_no" );
  int idx2 = Qry.FieldAsInteger( "grp_id" );
  string target = Qry.FieldAsString( "target" );
  int point_id_tlg = Qry.FieldAsInteger( "point_id" );
  int point_arv = Qry.FieldAsInteger( "point_arv" );
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
  vector<TSeatRange> seats;
  if ( seat_type != stDropseat ) { // ���������� ����� ���� + �஢�ઠ
  // ���뢠�� ᫮� �� ������ ����� � ������ �஢��� �� �, �� ��� ᫮� 㦥 ����� ��㣨� ���ᠦ�஬
    Qry.Clear();
    Qry.SQLText =
      "SELECT r.layer_type, t.pax_id, t.crs_pax_id "
      " FROM trip_comp_layers t,trip_comp_ranges r, comp_layer_types "
      " WHERE t.point_id=:point_id AND "
      "       t.first_yname=:first_yname AND "
      "       t.first_xname=:first_xname AND "
      "       t.point_id=r.point_id AND "
      "       t.range_id=r.range_id AND "
      "       comp_layer_types.code = r.layer_type ";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.DeclareVariable( "first_xname", otString );
    Qry.DeclareVariable( "first_yname", otString );
    TQuery Qry1( &OraSession );
    Qry1.SQLText =
      "SELECT c1.code "
      " FROM trip_comp_layers t, trip_comp_ranges r, "
      "      comp_layer_types c1, comp_layer_types c2 "
      " WHERE t.point_id=:point_id AND "
      "       t.point_id=r.point_id AND "
      "       t.range_id=r.range_id AND "
      "       (t.crs_pax_id=:crs_pax_id OR t.pax_id=:crs_pax_id) AND "
      "       t.first_yname||t.first_xname!=:first_yname||:first_xname AND "
      "       c1.code=r.layer_type AND "
      "       c2.code=:layer_type AND "
      "       c1.priority<=c2.priority AND rownum<2";
    Qry1.CreateVariable( "point_id", otInteger, point_id );
    Qry1.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
    Qry1.DeclareVariable( "first_xname", otString );
    Qry1.DeclareVariable( "first_yname", otString );
    Qry1.DeclareVariable( "crs_pax_id", otInteger );
    TSeatRange r;
    strcpy( r.first.line, first_xname.c_str() );
    strcpy( r.first.row, first_yname.c_str() );
    r.second = r.first;
    for ( int i=0; i<seats_count; i++ ) { // �஡�� �� ���-�� ����
//    	ProgTrace( TRACE5, "seats.push_back: xname=%s, yname=%s, pr_down=%d, seats_count=%d",
//    	           r.first.line, r.first.row, pr_down, seats_count );
      Qry.SetVariable( "first_xname", r.first.line );
      Qry.SetVariable( "first_yname", r.first.row );
      Qry.Execute();
      int p_id;
      while ( !Qry.Eof ) {
  	    switch( layer_type ) {
          case cltCheckin:
          	if ( Qry.FieldIsNULL( "pax_id" ) )
          		if ( Qry.FieldIsNULL( "crs_pax_id" ) )
          			p_id = NoExists;
          		else
          		  p_id = Qry.FieldAsInteger( "crs_pax_id" );
          	else
      	      p_id = Qry.FieldAsInteger( "pax_id" );
      	    break;
  		    case cltProtCkin:
  		    	if ( Qry.FieldIsNULL( "crs_pax_id" ) )
  		    		p_id = NoExists;
  		    	else
  			      p_id = Qry.FieldAsInteger( "crs_pax_id" );
            break;
          default:
      	    ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
      	    throw UserException( "��⠭��������� ᫮� ����饭 ��� ࠧ��⪨" );
        }
       // ��⠥��� ������ ᫮� �� ����, ���஥ � ⠪ ����� ᫮� ��� ��㣮�� ���ᠦ��
        if ( p_id > NoExists && pax_id != p_id ) {
  	      switch( layer_type ) {
            case cltCheckin:
            	if ( !Qry.FieldIsNULL( "pax_id" ) )
            		throw UserException( "���� ����� ��㣨� ���ᠦ�஬" );
  		      case cltProtCkin:
              Qry1.SetVariable( "first_xname", r.first.line );
              Qry1.SetVariable( "first_yname", r.first.row );
              Qry1.SetVariable( "crs_pax_id", p_id );
              Qry1.Execute();
              ProgTrace( TRACE5, "Qry1.Eof=%d, crs_pax_id=%d, placename=%s", Qry1.Eof, p_id, string(string(r.first.row)+(r.first.line)).c_str() );
              if ( Qry1.Eof ) { // ��� ��㣮�� ���� � ����� ��᮪�� �ਮ��⮬
              	throw UserException( "���� ����� ��㣨� ���ᠦ�஬" );
              }
              break;
            default:
      	      ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
      	      throw UserException( "��⠭��������� ᫮� ����饭 ��� ࠧ��⪨" );
          }
        }
  	    Qry.Next();
      }
      seats.push_back( r );
      if ( i < seats_count - 1 && !getNextSeat( layer_type, point_id, r, pr_down ) )
      	throw UserException( "��������� ���� ������㯭� ��� ���ᠦ��" );
    }
  }

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
  		case cltProtCkin:
  			// 㤠����� �� ᠫ���, �᫨ ���� ࠧ��⪠
  	    Qry.SQLText =
          "DELETE FROM "
          "  (SELECT * FROM tlg_comp_layers,trip_comp_layers "
          "    WHERE tlg_comp_layers.range_id=trip_comp_layers.range_id AND "
          "       tlg_comp_layers.crs_pax_id=:pax_id AND "
          "       tlg_comp_layers.layer_type=:layer_type) ";
        Qry.CreateVariable( "pax_id", otInteger, pax_id );
        Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
        Qry.Execute();
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
    Qry.Execute();
/*!!!    if ( !Qry.RowCount() == seats ) { // ��⠥��� 㤠���� ᫮�, ���ண� ��� � ��
      throw UserException( "��室��� ���� �� �������" );
    }*/
  }
  // �����祭�� ������ ᫮�
  if ( seat_type != stDropseat ) { // ��ᠤ�� �� ����� ����
    Qry.Clear();
    Qry.SQLText = "SELECT tid__seq.nextval AS tid FROM dual";
    Qry.Execute();
    tid = Qry.FieldAsInteger( "tid" );
  	Qry.Clear();
  	switch ( layer_type ) {
  	  case cltCheckin:
  		  SaveTripSeatRanges( point_id, layer_type, seats, pax_id, point_id, point_arv );
  		  Qry.SQLText =
          "BEGIN "
          " UPDATE pax SET tid=:tid WHERE pax_id=:pax_id;"
          " mvd.sync_pax(:pax_id,:term); "
          "END;";
        Qry.CreateVariable( "term", otString, TReqInfo::Instance()->desk.code );
        break;
      case cltProtCkin:
        SaveTlgSeatRanges( point_id_tlg, target, layer_type, seats, pax_id, 0, false );
	      Qry.SQLText =
          "UPDATE crs_pax SET tid=:tid WHERE pax_id=:pax_id";
      	break;
      default:
      	ProgTrace( TRACE5, "!!! Unuseable layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
      	throw UserException( "��⠭��������� ᫮� ����饭 ��� ࠧ��⪨" );
    }
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "tid", otInteger, tid );
    Qry.Execute();
  }

  TReqInfo *reqinfo = TReqInfo::Instance();
  string new_seat_no;
  for (vector<TSeatRange>::iterator ns=seats.begin(); ns!=seats.end(); ns++ ) {
  	if ( !new_seat_no.empty() )
  		new_seat_no += " ";
    new_seat_no += denorm_iata_row( ns->first.row ) + denorm_iata_line( ns->first.line, pr_lat_seat );
  }
  switch( seat_type ) {
  	case stSeat:
  		switch( layer_type ) {
  			case cltCheckin:
          reqinfo->MsgToLog( string( "���ᠦ�� " ) + fullname +
                             " ��ᠦ�� �� ����: " +
                             new_seat_no,
                             evtPax, point_id, idx1, idx2 );
          break;
        case cltProtCkin:
          reqinfo->MsgToLog( string( "���ᠦ��� " ) + fullname +
                             " �।���⥫쭮 �����祭� ����. ����� ����: " +
                             new_seat_no,
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
                             new_seat_no,
                             evtPax, point_id, idx1, idx2 );
          break;
        case cltProtCkin:
          reqinfo->MsgToLog( string( "���ᠦ��� " ) + fullname +
                             " �।���⥫쭮 �����祭� ����. ����� ����: " +
                             new_seat_no,
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
        case cltProtCkin:
          reqinfo->MsgToLog( string( "���ᠦ��� " ) + fullname +
                             " �⬥���� �।���⥫쭮 �����祭��� ����: " + prior_seat,
                             evtPax, point_id, idx1, idx2 );
          break;
        default:;
  		}
  		break;
  }
}

void AutoReSeatsPassengers( TSalons &Salons, TPassengers &APass, int SeatAlgo )
{
	// ᠫ�� ᮤ�ন� �� ��ଠ��� ���� (��� ��䠫����� ����, ���ਬ�� � ࠧ�뢠��
  if ( Salons.placelists.empty() )
    throw Exception( "�� ����� ᠫ�� ��� ��⮬���᪮� ��ᠤ��" );
  FSeatAlgo = SeatAlgo;
  CurrSalon = &Salons;
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

  //!!! �� ������ pass.placeName, pass.PrevPlaceName, pass.OldPlaceName, pass.placeList,pass.InUse ,x,y???

  int s;
  try {
    for ( int vClass=0; vClass<=2; vClass++ ) {
      //!!!for ( int vSeats=0; vSeats<=1; vSeats++ ) {
        Passengers.Clear();
        s = APass.getCount();
        for ( int i=0; i<s; i++ ) {
          TPassenger &pass = APass.Get( i );
          ProgTrace( TRACE5, "isSeat=%d, pass.pax_id=%d, pass.placeName=%s", pass.isSeat, pass.pax_id, pass.placeName.c_str() );
          if ( pass.isSeat ) // ���ᠦ�� ��ᠦ��
            continue;
          pass.OldPlaceName = pass.placeName;
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
          if ( ExistsBasePlace( Salons, pass ) ) // ���ᠦ�� �� ��ᠦ��, �� ��諮�� ��� ���� ������� ���� - ����⨫� ��� ����� //??? ����஢�� !!!
            continue;
          Passengers.Add( pass ); /* ������������� � � ������ �� ��諮�� ������� ���� */
        } /* �஡������� �� �ᥬ ���ᠦ�ࠬ */
        GET_LINE_ARRAY( );
        /* ��ᠤ�� ���ᠦ�� � ���ண� �� ������� ������� ���� */
        SeatPlaces.SeatsPassengers( true );
        SeatPlaces.RollBack( );
        int s = Passengers.getCount();
        for ( int i=0; i<s; i++ ) {
          TPassenger &pass = Passengers.Get( i );
          int k = APass.getCount();
          for ( int j=0; j<k; j++	 ) {
            TPassenger &opass = APass.Get( j );
            if ( opass.pax_id == pass.pax_id ) {
            	opass = pass;
            	break;
            }
          }
        //!!!}
      }
    }
    TQuery QryPax( &OraSession );
    TQuery QryLayer( &OraSession );
    TQuery QryUpd( &OraSession );
    QryPax.SQLText =
      "SELECT point_dep, point_arv, "
      "       salons.get_seat_no(pax.pax_id,:layer_type,pax.seats,point_dep,'list',rownum) AS seat_no "
      " FROM pax, pax_grp "
      " WHERE pax.pax_id=:pax_id AND "
      "       pax.grp_id=pax_grp.grp_id ";
    QryPax.DeclareVariable( "pax_id", otInteger );
    QryPax.CreateVariable( "layer_type", otString, EncodeCompLayerType(cltCheckin) );
    QryLayer.SQLText =
      "DELETE FROM trip_comp_layers "
      " WHERE point_id=:point_id AND "
      "       layer_type=:layer_type AND "
      "       pax_id=:pax_id ";
    QryLayer.CreateVariable( "point_id", otInteger, CurrSalon->trip_id );
    QryLayer.DeclareVariable( "pax_id", otInteger );
    QryLayer.CreateVariable( "layer_type", otString, EncodeCompLayerType(cltCheckin) );
    QryUpd.SQLText =
      "BEGIN "
      " UPDATE pax SET tid=tid__seq.nextval WHERE pax_id=:pax_id;"
      " mvd.sync_pax(:pax_id,:term); "
      "END;";
    QryUpd.DeclareVariable( "pax_id", otInteger );
    QryUpd.DeclareVariable( "term", otString );

    ProgTrace( TRACE5, "passengers.count=%d", Passengers.getCount() );
    Passengers.Clear();

    s = APass.getCount();
    for ( int i=0; i<s; i++ ) {
    	TPassenger &pass = APass.Get( i );
    	Passengers.Add( pass );
    	if ( pass.isSeat )
    		continue;
     	QryPax.SetVariable( "pax_id", pass.pax_id );
      QryPax.Execute();
      if ( QryPax.Eof )
      	throw UserException( "�� ������ ���ࠢ����� � ���ᠦ��" );
      int point_dep = QryPax.FieldAsInteger( "point_dep" );
      int point_arv = QryPax.FieldAsInteger( "point_arv" );
      string prev_seat_no = QryPax.FieldAsString( "seat_no" );

    	if ( !pass.InUse ) { /* �� ᬮ��� ��ᠤ��� */
    		if ( !pass.placeName.empty() )
          TReqInfo::Instance()->MsgToLog( string("���ᠦ�� " ) + pass.fullName +
                                          " ��-�� ᬥ�� ���������� ��ᠦ�� � ���� " +
                                          prev_seat_no, evtPax, CurrSalon->trip_id, pass.regNo, pass.grpId );
      }
      else {
      	std::vector<TSeatRange> seats;
      	for ( vector<TSeat>::iterator i=pass.seat_no.begin(); i!=pass.seat_no.end(); i++ ) {
      		TSeatRange r(*i,*i);
      		seats.push_back( r );
      	}
        // ����室��� ���砫� 㤠���� �� ��� ��������� ���� �� ᫮� ॣ����樨
        QryLayer.CreateVariable( "pax_id", otInteger, pass.pax_id );
        QryLayer.Execute();
  		  SaveTripSeatRanges( Salons.trip_id, cltCheckin, seats, pass.pax_id, point_dep, point_arv );
  		  QryUpd.SetVariable( "pax_id", pass.pax_id );
        QryUpd.SetVariable( "term", TReqInfo::Instance()->desk.code );
        QryUpd.Execute();
        QryPax.Execute();
        string new_seat_no = QryPax.FieldAsString( "seat_no" );
        ProgTrace( TRACE5, "oldplace=%s, newplace=%s", prev_seat_no.c_str(), new_seat_no.c_str() );
      	if ( prev_seat_no != new_seat_no )/* ���ᠤ��� �� ��㣮� ���� */
          TReqInfo::Instance()->MsgToLog( string( "���ᠦ�� " ) + pass.fullName +
                                          " ��-�� ᬥ�� ���������� ���ᠦ�� �� ���� " +
                                          new_seat_no, evtPax, CurrSalon->trip_id, pass.regNo, pass.grpId );
      }
    }
  }
  catch( ... ) {
    SeatPlaces.RollBack( );
    throw;
  }
  SeatPlaces.RollBack( );
  ProgTrace( TRACE5, "passengers.count=%d", APass.getCount() );
}

int GetSeatAlgo(TQuery &Qry, string airline, int flt_no, string airp_dep)
{
  Qry.Clear();
  Qry.SQLText=
    "SELECT algo_type, "
    "       DECODE(airline,NULL,0,8)+ "
    "       DECODE(flt_no,NULL,0,2)+ "
    "       DECODE(airp_dep,NULL,0,4) AS priority "
    "FROM seat_algo_sets "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) AND "
    "      (airp_dep IS NULL OR airp_dep=:airp_dep) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("airline",otString,airline);
  Qry.CreateVariable("flt_no",otInteger,flt_no);
  Qry.CreateVariable("airp_dep",otString,airp_dep);
  Qry.Execute();
  int algo=0;
  if (!Qry.Eof)
  {
    algo=Qry.FieldAsInteger("algo_type");
    if (algo!=0) algo=1;
  };
  Qry.Close();
  return algo;
};

} /* end namespace SEATS */

void TPassengers::Build( TSalons &Salons, xmlNodePtr dataNode )
{
  if ( !Passengers.getCount() )
    return;
  xmlNodePtr passNode = NewTextChild( dataNode, "passengers" );
  for (VPassengers::iterator p=FPassengers.begin(); p!=FPassengers.end(); p++ ) {
    xmlNodePtr pNode = NewTextChild( passNode, "pass" );
    NewTextChild( pNode, "pax_id", p->pax_id );
    NewTextChild( pNode, "clname", p->clname );
    NewTextChild( pNode, "placename", p->placeName );
    NewTextChild( pNode, "prevplacename", p->PrevPlaceName ); //???
    NewTextChild( pNode, "oldplacename", p->OldPlaceName ); //???
    NewTextChild( pNode, "countplace", p->countPlace );
    NewTextChild( pNode, "tid", p->tid );
    NewTextChild( pNode, "isseat", p->isSeat );
    NewTextChild( pNode, "grp_id", p->grpId );
    NewTextChild( pNode, "reg_no", p->regNo );
    NewTextChild( pNode, "name", p->fullName );
    if ( !p->rems.empty() &&
    	   find( p->rems.begin(), p->rems.end(), string("STCR") ) != p->rems.end() )
      NewTextChild( pNode, "step", "�" );
    else
      NewTextChild( pNode, "step", "�" );
    NewTextChild( pNode, "inuse", p->InUse ); //???


    // !!!old version
    for ( vector<TPlaceList*>::iterator placeList=Salons.placelists.begin();
          placeList!=Salons.placelists.end(); placeList++ ) {
      if ( (*placeList)->GetisPlaceXY( p->placeName, p->Pos ) ) {
        p->placeList = *placeList;
        break;
      }
    }
    // !!!end old version



    NewTextChild( pNode, "x", p->Pos.x ); //???
    NewTextChild( pNode, "y", p->Pos.y ); //???
    if ( p->placeList )
      NewTextChild( pNode, "num", p->placeList->num ); //???
    else
      NewTextChild( pNode, "num", -1 ); //???
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
