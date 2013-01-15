#include <stdlib.h>

#include "seats.h"
#include "basic.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "salons.h"
#include "comp_layers.h"
#include "convert.h"
#include "images.h"
#include "serverlib/str_utils.h"
#include "tripinfo.h"
#include "aodb.h"
#include "term_version.h"
#include "alarms.h"
#include "passenger.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace AstraLocale;
using namespace BASIC;
using namespace ASTRA;
using namespace SALONS2;


namespace SEATS2 //new terminal
{


const int PR_N_PLACE = 9;
const int PR_REMPLACE = 8;
const int PR_SMOKE = 7;

const int PR_REM_TO_REM = 100;
const int PR_REM_TO_NOREM = 90;
const int PR_EQUAL_N_PLACE = 50;
const int PR_EQUAL_REMPLACE = 20;
const int PR_EQUAL_SMOKE = 10;

const int CONST_MAXPLACE = 3;


typedef vector<SALONS2::TPoint> TSeatCoords;

struct TSeatCoordsSalon {
  map<int,TSeatCoords> coords;
  SALONS2::TPlaceList *placeList;
};

typedef vector<TSeatCoordsSalon> vecSeatCoordsVars;

class TSeatCoordsClass {
  private:
  	vecSeatCoordsVars vecSeatCoords;
    void clear();
  public:
    vecSeatCoordsVars &getSeatCoordsVars( );
    void refreshCoords( TSeatAlgoTypes ASeatAlgoType, bool pr_window, bool pr_tube );
};
enum TSeatAlg { sSeatGrpOnBasePlace, sSeatGrp, sSeatPassengers, seatAlgLength };
enum TUseRem { sAllUse, sMaxUse, sOnlyUse, sNotUse_NotUseDenial, sNotUse, sNotUseDenial, sIgnoreUse, useremLength };
/*
 aAllUse - ᮢ������� ��� ६�ப �� ᠬ��� �ਮ��⭮�� ���ᠦ���
 sMaxUse - ᮢ������� ᠬ�� �ਮ��⭮� ६�ન
 sOnlyUse - ��� �� ���� ᮢ������
 sNotUse - ����饭� �ᯮ�짮���� ���� � ࠧ�襭��� ६�મ�, �᫨ � ���ᠦ�� ⠪�� ६�ન ���
 sNotUseDenial - ����饭� �ᯮ�짮���� ���� � ����饭��� ६�મ�, ��� ���� � ���ᠦ��
 sIgnoreUse - �����஢��� ६�ન
*/
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
	  case sNotUse_NotUseDenial:
	  	  res = "sNotUse_NotUseDenial";
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

/* �������� ��६���� ��� �⮣� ����� */
TSeatPlaces SeatPlaces;
SALONS2::TSalons *CurrSalon;
TSeatCoordsClass seatCoords;

bool CanUseLayers; /* ���� �� ᫮� ���� */
bool CanUseMutiLayer; /* ���� �� ��㯯� ᫮�� */
TCompLayerType PlaceLayer; /* ᠬ ᫮� */
vector<TCompLayerType> SeatsLayers;
TUseLayers UseLayers; // ����� �� �ᯮ�짮���� ���� � ᫮�� ��� ��ᠤ�� ���ᠦ�஢ � ��㣨�� ᫮ﬨ

//bool CanUse_PS; /* ����� �� �ᯮ�짮���� ����� �।�. ��ᠤ�� ��� ���ᠦ�஢ � ��㣨�� ����ᠬ� */
bool CanUseSmoke; /* ���� ������ ���� */
vector<string> PlaceElem_Types; /* ࠧ�襭�� ⨯� ���� ���� ���� �� ⨯� (⠡��⪠)*/
TUseRem CanUseRems; /* ���� �� ६�થ */
vector<string> Remarks; /* ᠬ� ६�ઠ */
bool CanUseTube; /* ���� �१ ��室� */
TUseAlone CanUseAlone; /* ����� �� �ᯮ�짮���� ��ᠤ�� ������ � ��� - ����� ��ᠤ���
                          ��㯯� ��� �� ��㣮� */
TSeatAlg SeatAlg;
bool FindSUBCLS=false; // ��室�� �� ⮣�, �� � ��㯯� �� ����� ���� ���ᠦ�஢ � ࠧ�묨 �������ᠬ�!
bool canUseSUBCLS=false;
string SUBCLS_REM;

bool canUseOneRow=true; // �ᯮ�짮���� �� ��ᠤ�� ⮫쪮 ���� �� - ��।������ �� �᭮�� �-樨 getCanUseOneRow() � 横��

struct TCondRate {
  bool pr_web;
  map<int, TPlaceWebTariff> rates;
  map<int, TPlaceWebTariff>::iterator current_rate;
  TCondRate( ) {
    current_rate = rates.end();
  }
  double convertUniValue( TPlaceWebTariff &rate ) {
    return rate.value;
  }
  void Init( SALONS2::TSalons &Salons, bool apr_web ) {
    pr_web = apr_web;
    rates.clear();
    rates[ 0 ].value = 0.0; // �ᥣ�� ������ - ����砥�, �� ���� �ᯮ�짮���� ���� ��� ���
    if ( pr_web ) // �� ���뢠�� ����� ����
      return;
    SALONS2::TPlaceList *placeList;
    map<double,int> vars;
    for ( vector<SALONS2::TPlaceList*>::iterator plList=Salons.placelists.begin();
          plList!=Salons.placelists.end(); plList++ ) {
      placeList = *plList;
      for ( IPlace i=placeList->places.begin(); i!=placeList->places.end(); i++ ) {
        if ( i->WebTariff.value == 0.0 || !i->visible || !i->isplace )
          continue;
        double univalue = convertUniValue( i->WebTariff );
        if ( vars.find( univalue ) == vars.end() ) { // �� ��諨
          bool pr_ins = false;
          for ( map<double,int>::iterator p=vars.begin(); p!=vars.end(); p++ ) {
            if ( univalue < p->first ) {
              if ( !pr_ins ) {
                pr_ins = true;
                vars[ univalue ] = p->second;
                for ( int k=vars.size(); k >p->second; k-- )
                  rates[ k ] = rates[ k - 1 ];
                rates[ p->second ] = i->WebTariff;
              }
              vars[ p->first ] = vars[ p->first ] + 1;
            }
          }
          if ( !pr_ins )
           vars[ univalue ] = vars.size();
           rates[ vars[ univalue ] ] = i->WebTariff;
        }
      }
    }
    current_rate = rates.begin();
    for ( map<int, TPlaceWebTariff>::iterator i=rates.begin(); i!=rates.end(); i++ ) {
      ProgTrace( TRACE5, " rates.first=%d, rates.second.value=%f", i->first, i->second.value );
    }
  }
  bool CanUseRate( TPlace *place ) { /* �᫨ �� �������� ���� ���஡����� �� ��ᠤ�� � �� ᬮ��� ��ᠤ��� ��� ��� ��䮢 �� ३� ��� ���� ��� ���, � ����� �ᯮ�짮���� */
    bool res = ( pr_web || current_rate == rates.end() || place->WebTariff.value == 0.0 );
    if ( !res ) {
      for ( map<int, TPlaceWebTariff>::iterator i=rates.begin(); ; i++ ) { // ��ᬮ�� ��� ��䮢, ���. ���஢��� � ���浪� �����⠭�� �ਮ��� �ᯮ�짮�����
        if ( i->second.value == place->WebTariff.value ) {
          res = true;
          break;
        }
        if ( i == current_rate ) // ����� ���� ��� �� ����㯭�
          break;
      }
    }
    return res;
  }
  bool isIgnoreRates( ) {
    map<int, TPlaceWebTariff>::iterator i = rates.end();
    if ( !rates.empty() )
      i--;
    return ( current_rate == rates.end() || i == current_rate );
  }
};

TCondRate condRates;

TSeatAlgoParams FSeatAlgoParams; // ������ ��ᠤ��


void GetUseLayers( TUseLayers &uselayers )
{
  uselayers.clear();
}

bool CanUseLayer( const TCompLayerType &layer, TUseLayers FUseLayers )
{
  return ( FUseLayers.find( layer ) != FUseLayers.end() && FUseLayers[ layer ] );
}

int MAXPLACE() {
	return (canUseOneRow)?1000:CONST_MAXPLACE; // �� ��ᠤ�� � ���� �� ���-�� ���� � ��� ����࠭�祭� ���� 3 ����
};

void getValidChildElem_Types( vector<string> &elem_types ) {
	elem_types.clear();
  elem_types.push_back( "�" ); //!!! ����� ���� ࠧ�襭��� ����
  elem_types.push_back( "�" ); //!!!
}

bool is_valid_seat_elem_type( const string &pers_type, const std::vector<SALONS2::TPlace> &places )
{
  if ( pers_type != "��" ) { // �஢�ઠ �� ���� � ���਩���� ��室�
    vector<string> elem_types;
    getValidChildElem_Types( elem_types );
    for(std::vector<SALONS2::TPlace>::const_iterator ipl=places.begin(); ipl!=places.end(); ipl++ ) {
      if ( find( elem_types.begin(), elem_types.end(), ipl->elem_type ) == elem_types.end() )
      	return false;
    }
  }
  return true;
}

bool getCanUseOneRow() {
	return FSeatAlgoParams.pr_canUseOneRow;
}

vecSeatCoordsVars &TSeatCoordsClass::getSeatCoordsVars( )
{
	return vecSeatCoords;
}

void TSeatCoordsClass::clear() {
  vecSeatCoords.clear();
}

int getVariantCoord( TPlaceList *placeList, int x, int xlen, int y, int ylen, bool pr_window, bool pr_tube )
{
  if ( pr_window )
    if ( x == 0 || x == xlen - 1 )
      return 0;
    else
      return 1;
  else
    if ( pr_tube )
      if ( x - 1 >= 0 && placeList->GetXsName( x - 1 ).empty() ||
           x + 1 <= xlen - 1 && placeList->GetXsName( x + 1 ).empty() )
        return 0;
      else
        return 1;
    else return 1;
}

void TSeatCoordsClass::refreshCoords( TSeatAlgoTypes ASeatAlgoType, bool pr_window, bool pr_tube )
{
  clear();
  int xlen, ylen;
  // ᢥ��� ����
  bool pr_UpDown = ( ASeatAlgoType == sdUpDown_Row || ASeatAlgoType == sdUpDown_Line );
  if ( pr_UpDown ) {
    for ( vector<SALONS2::TPlaceList*>::iterator iplaceList=CurrSalon->placelists.begin();
          iplaceList!=CurrSalon->placelists.end(); iplaceList++ ) {
      TSeatCoordsSalon coordSalon;
      coordSalon.placeList = *iplaceList;
      xlen = (*iplaceList)->GetXsCount();
      ylen = (*iplaceList)->GetYsCount();
      if ( ASeatAlgoType == sdUpDown_Line ) {
        for ( int x=0; x<xlen; x++ ) {
          if ( (*iplaceList)->GetXsName( x ).empty() )
            continue;
          for ( int y=0; y<ylen; y++ ) {
            coordSalon.coords[ getVariantCoord( *iplaceList, x, xlen, y, ylen, pr_window, pr_tube ) ].push_back( TPoint( x, y ) );
          } // end for ys
        } // end for xs
      } // end if
      else {
    		for ( int y=0; y<ylen; y++ ) {
     			for ( int x=0; x<xlen; x++ ) {
            if ( (*iplaceList)->GetXsName( x ).empty() )
              continue;
    			 	coordSalon.coords[ getVariantCoord( *iplaceList, x, xlen, y, ylen, pr_window, pr_tube ) ].push_back( TPoint( x, y ) );
    		  } // end for xs
    	  } // end for ys
      }
      vecSeatCoords.push_back( coordSalon );
    } // end for placelists
  }
  else { // ᭨�� �����
    for ( vector<SALONS2::TPlaceList*>::reverse_iterator iplaceList=CurrSalon->placelists.rbegin();
          iplaceList!=CurrSalon->placelists.rend(); iplaceList++ ) {
      TSeatCoordsSalon coordSalon;
      coordSalon.placeList = *iplaceList;
      xlen = (*iplaceList)->GetXsCount();
      ylen = (*iplaceList)->GetYsCount();
      if ( ASeatAlgoType == sdDownUp_Line ) {
      	ProgTrace( TRACE5, "ASeatAlgoType=sdDownUp_Line: xlen=%d, ylen=%d", xlen, ylen );
        for ( int x=0; x<xlen; x++ ) {
          if ( (*iplaceList)->GetXsName( x ).empty() )
            continue;
          for ( int y=ylen-1; y>=0; y-- ) {
            coordSalon.coords[ getVariantCoord( *iplaceList, x, xlen, y, ylen, pr_window, pr_tube ) ].push_back( TPoint( x, y ) );
          } // end for ys
        } // end for xs
      } // end if
      else {
      	ProgTrace( TRACE5, "ASeatAlgoType=sdDownUp_Row" );
      	for ( int y=ylen-1; y>=0; y-- ) {
     			for ( int x=0; x<xlen; x++ ) {
            if ( (*iplaceList)->GetXsName( x ).empty() )
              continue;
    			 	coordSalon.coords[ getVariantCoord( *iplaceList, x, xlen, y, ylen, pr_window, pr_tube ) ].push_back( TPoint( x, y ) );
    		  } // end for xs
    	  } // end for ys
      }
      ProgTrace( TRACE5, "coordSalon.coords.size()=%d", coordSalon.coords.size() );
      vecSeatCoords.push_back( coordSalon );
    }
  }
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

TSeatPlaces::TSeatPlaces(  )
{
  Clear();
}

TSeatPlaces::~TSeatPlaces()
{
  Clear();
}

void TSeatPlaces::Clear()
{
	grp_status = cltUnknown;
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
    for ( vector<SALONS2::TPlace>::iterator place=seatPlace.oldPlaces.begin();
          place!=seatPlace.oldPlaces.end(); place++ ) {
      /* ����祭�� ���� */
      int idx = seatPlace.placeList->GetPlaceIndex( seatPlace.Pos );
      SALONS2::TPlace *pl = seatPlace.placeList->place( idx );
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
      default: throw EXCEPTIONS::Exception( "�訡�� ��ᠤ��" );
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
int TSeatPlaces::Put_Find_Places( SALONS2::TPoint FP, SALONS2::TPoint EP, int foundCount, TSeatStep Step )
{
  int p_RCount, p_RCount2, p_RCount3; /* ����室���� ���-�� 3-�, 2-�, 1-� ���� */
  int pp_Count, pp_Count2, pp_Count3; /* ����饥�� ���-�� 3-�, 2-�, 1-� ���� */
  int NTrunc_Count, Trunc_Count; /* ���-�� �뤥������ �� ��饣� �᫠ ������ ���� */
  int p_Prior, p_Next; /* ���-�� ���� �� FP � ��᫥ ���� */
  int p_Step; /* ���ࠢ����� ��ᠤ��. ��।������ � ����ᨬ��� �� ���-�� p_Prior � p_Next */
  int Need;
  SALONS2::TPlaceList *placeList;
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
    /* ��࠭塞 ���� ���� � ����砥� ���� � ᠫ��� ��� ������ */
    for ( int i=0; i<Trunc_Count; i++ ) {
      SALONS2::TPlace place;
      SALONS2::TPlace *pl = placeList->place( EP );
      place.Assign( *pl );
      if ( !CurrSalon->placeIsFree( &place ) || !place.isplace )
        throw EXCEPTIONS::Exception( "���ᠤ�� �믮����� �������⨬�� ������: �ᯮ�짮����� 㦥 ����⮣� ����" );
      seatplace.oldPlaces.push_back( place );
      pl->AddLayerToPlace( grp_status, 0, 0, NoExists, NoExists, CurrSalon->getPriority( grp_status ) );
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

bool isREM_SUBCLS( string rem )
{
	return ( rem.size() == 4 && rem.substr(1,3) == "CLS" &&
		       *(rem.c_str()) >= 'A' && *(rem.c_str()) <= 'Z' );
}

/* �-�� ��� ��।������ ���������� ��ᠤ�� ��� ���� � ������ ���� ����饭�� ६�ન */
bool DoNotRemsSeats( const vector<SALONS2::TRem> &rems )
{
	bool res = false; // �ਧ��� ⮣�, �� � ���ᠦ�� ���� ६�ઠ, ����� ����饭� �� ��࠭��� ����
	bool pr_passsubcls = false;
	// ��।��塞 ���� �� � ���ᠦ�� ६�ઠ ��������
	//unsigned int i=0;
	vector<string>::iterator yrem=Remarks.begin();
	for (; yrem!=Remarks.end(); yrem++ ) {
		if ( isREM_SUBCLS( *yrem ) ) {
			pr_passsubcls = true;
			break;
		}
	}

/* old version	for ( ; i<sizeof(TSubcls_Remarks)/sizeof(const char*); i++ ) {
		if ( find( Remarks.begin(), Remarks.end(), string(TSubcls_Remarks[i]) ) != Remarks.end() ) { // � ���ᠦ�� SUBCLS
			pr_passsubcls = true;
			break;
	  }
	}*/

	bool no_subcls = false;
	// ��।��塞 ���� �� ����饭��� ६�ઠ �� ����, ���. ������ � ���ᠦ��
	for( vector<string>::iterator nrem=Remarks.begin(); nrem!=Remarks.end(); nrem++ ) {
	  for( vector<SALONS2::TRem>::const_iterator prem=rems.begin(); prem!=rems.end(); prem++ ) {
		  if ( !prem->pr_denial )
			  continue;
			if ( *nrem == prem->rem && !res )
				res = true;
	  }
	}
	// �஡�� �� ६�ઠ� ����, �᫨ ���� ६�ઠ ��������
  for( vector<SALONS2::TRem>::const_iterator prem=rems.begin(); prem!=rems.end(); prem++ ) {
	  if ( !prem->pr_denial && isREM_SUBCLS( prem->rem ) && (!pr_passsubcls || *yrem != prem->rem ) ) {
	  	  no_subcls = true;
	  	  break;
	  }
	}
/*  for( vector<SALONS2::TRem>::const_iterator prem=rems.begin(); prem!=rems.end(); prem++ ) {
    for ( unsigned int j=0; j<sizeof(TSubcls_Remarks)/sizeof(const char*); j++ ) {
  	  if ( !prem->pr_denial && prem->rem == TSubcls_Remarks[j] && (!pr_passsubcls || TSubcls_Remarks[j] != TSubcls_Remarks[i]) ) {
	  	  no_subcls = true;
	  	  break;
	  	}
	  }
	}*/
  return res || no_subcls; // �᫨ ���� ����饭��� ६�ઠ � ���ᠦ�� ��� ���� � ६�મ� SUBCLS, � � ���ᠦ�� �� ���
}

bool VerifyUseLayer( TPlace *place )
{
	bool res = !CanUseLayers || PlaceLayer == cltUnknown && place->layers.empty();
	if ( !res ) {
		for (std::vector<TPlaceLayer>::iterator i=place->layers.begin(); i!=place->layers.end(); i++ ) {
			if ( find( SeatsLayers.begin(), SeatsLayers.end(), i->layer_type ) == SeatsLayers.end() ) { // ������ ����� �ਮ���� ᫮�, ���ண� ��� � ᯨ᪥ ����������� ᫮��
				break;
		  }
		  if ( CanUseMutiLayer || i->layer_type == PlaceLayer ) { // ������ �㦭� ᫮�
		  	res = true;
		  	break;
		  }
	  }
	}
	return res;
}

/* ���� ���� �ᯮ�������� �冷� ��᫥����⥫쭮
   �����頥� ���-�� ��������� ����.
   FP - ���� ����, � ���ண� ���� �᪠��
   FoundCount - ���-�� 㦥 ��������� ����
   Step - ���ࠢ����� ���᪠
  �������� ��६����:
   CanUselayer, Placelayer - ���� ��ண� �� ������ ����,
   CanUseSmoke - ���� ������ ����,
   PlaceElem_Types - ���� ��ண� �� ⨯� ���� (⠡��⪠),
   CanUseRem, PlaceRem - ���� ��ண� �� ६�થ ���� */
int TSeatPlaces::FindPlaces_From( SALONS2::TPoint FP, int foundCount, TSeatStep Step )
{
  int Result = 0;
  SALONS2::TPlaceList *placeList = CurrSalon->CurrPlaceList();
  if ( !placeList->ValidPlace( FP ) )
    return Result;
  SALONS2::TPoint EP = FP;
  SALONS2::TPlace *place = placeList->place( EP );
  vector<SALONS2::TRem>::iterator prem;
  vector<string>::iterator irem;
/*  if ( SeatAlg == 1 && !canUseSUBCLS && CanUseRems == sNotUse_NotUseDenial )
 	  ProgTrace( TRACE5, "sNotUse_NotUseDenial CurrSalon->placeIsFree( place )=%d,place->isplace=%d,place->visible=%d,Passengers.clname=%s",
 	             CurrSalon->placeIsFree( place ),place->isplace,place->visible,Passengers.clname.c_str() );*/
  while ( CurrSalon->placeIsFree( place ) && place->isplace && place->visible &&
          place->clname == Passengers.clname &&
          Result + foundCount < MAXPLACE() &&
          VerifyUseLayer( place ) &&
          condRates.CanUseRate( place )
          /* 11.11.09
          ( !CanUseLayers ||
            place->isLayer( PlaceLayer ) ||
            PlaceLayer == cltUnknown && place->layers.empty() )*/ &&
          ( !CanUseSmoke || place->isLayer( cltSmoke ) ) &&
          ( PlaceElem_Types.empty() || find( PlaceElem_Types.begin(), PlaceElem_Types.end(), place->elem_type ) != PlaceElem_Types.end() ) ) {
    if ( canUseSUBCLS ) {
      for ( prem = place->rems.begin(); prem != place->rems.end(); prem++ ) {
        if ( !prem->pr_denial && prem->rem == SUBCLS_REM )
        	break;
      }
      if ( FindSUBCLS && prem == place->rems.end() ||
      	   !FindSUBCLS && prem != place->rems.end() ) //  �� ᬮ��� ��ᠤ��� �� ����� SUBCLS
     	  break;
    }
    if ( SUBCLS_REM.empty() || !canUseSUBCLS ) {
    	bool pr_find=false;
    	for ( prem = place->rems.begin(); prem != place->rems.end(); prem++ ) {
    	  if ( !prem->pr_denial && isREM_SUBCLS( prem->rem ) ) {
    	  	pr_find=true;
    	  	break;
    	  }
    	}
     if ( pr_find )
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
      case sNotUse_NotUseDenial:
      case sNotUseDenial:
      	if ( DoNotRemsSeats( place->rems ) ) {
      		return Result;
      	}
      	if ( CanUseRems == sNotUseDenial ) break;
      case sNotUse:
  	     for( vector<SALONS2::TRem>::const_iterator prem=place->rems.begin(); prem!=place->rems.end(); prem++ ) {
//  	     	 ProgTrace( TRACE5, "sNotUse: Result=%d, FP.x=%d, FP.y=%d, rem=%s", Result, FP.x, FP.y, prem->rem.c_str() );
		       if ( !prem->pr_denial ) {
		       	 return Result;
		       }
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
bool TSeatPlaces::SeatSubGrp_On( SALONS2::TPoint FP, TSeatStep Step, int Wanted )
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
  SALONS2::TPlaceList *placeList = CurrSalon->CurrPlaceList();
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
  SALONS2::TPoint EP = FP;
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
    if ( placeList->ValidPlace( EP ) ) {
      SALONS2::TPoint p( EP.x - 1, EP.y );
      SALONS2::TPlace *place = placeList->place( p ); /* ��६ �।. ���� */
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
      SALONS2::TPoint VP = EP;
//      ProgTrace( TRACE5, "EP=(%d,%d)", EP.x, EP.y );
      if ( placeList->ValidPlace( EP ) ) {
        SALONS2::TPoint p( EP.x + 1, EP.y );
        SALONS2::TPlace *place = placeList->place( p ); /* ��६ ᫥�. ���� */
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
    if ( EP.y >= placeList->GetYsCount() || !SeatSubGrp_On( EP, Step, 2 ) ) {
      return false; /* �� ᬮ��� ��ᠤ��� 2-� �� ᫥���騩 �� */
    }
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
  if ( counters.p_Count_3( sDown ) == Passengers.counters.p_Count_3( sDown ) &&
       counters.p_Count_2( sDown ) == Passengers.counters.p_Count_2( sDown ) &&
       counters.p_Count_3( sRight ) == Passengers.counters.p_Count_3( sRight ) &&
       counters.p_Count_2( sRight ) == Passengers.counters.p_Count_2( sRight ) &&
       counters.p_Count( Step ) == Passengers.counters.p_Count( Step ) ) {
    return true;
  }
  int lines = placeList->GetXsCount(), visible=0;
  for ( int line=0; line<lines; line++ ) {
  	SALONS2::TPoint f=FP;
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
       counters.p_Count( sDown ) == Passengers.counters.p_Count( sDown ) ) {
    return true;
  }

  SALONS2::TPoint EP;
  /* ���� ��ᠤ��� ��⠢����� ���� ���ᠦ�஢ */
  /* ��� �⮣� ���� ����� ��࠭�� ���� ����� � ���஡����� ��ᠤ���
    ��⠢����� ��㯯� �冷� � 㦥 ��ᠦ����� */
  VSeatPlaces sp( seatplaces.begin(), seatplaces.end() );
  if ( Where != sUpDown )
  for (ISeatPlace isp = sp.begin(); isp!= sp.end(); isp++ ) {
    CurrSalon->SetCurrPlaceList( isp->placeList );
    for( vector<SALONS2::TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
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
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        return true;
       }
      SALONS2::TPlaceList *placeList = CurrSalon->CurrPlaceList();
      SALONS2::TPoint p( EP.x + 1, EP.y );
      if ( CanUseTube && placeList->ValidPlace( p ) &&
           !placeList->place( EP )->visible ) {
       /* ����� ���஡����� �᪠�� �१ ��室 */
       Alone = true; /* ����� ���� ���� ���� */
       if ( SeatSubGrp_On( p, sRight, 0 ) ) {
         return true;
       }
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
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        return true;
      }
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
        if ( SeatSubGrp_On( p, sRight, 0 ) ) {
          return true;
        }
      }
      if ( isp->Step == sLeft || isp->Step == sRight )
        break;
    } /* end for */
    if ( Where != sLeftRight )
    /* ���饬 ᢥ��� � ᭨�� �� ���������� ���� */
    for( vector<SALONS2::TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
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
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        return true;
      }
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
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        return true;
      }
      if ( isp->Step == sUp || isp->Step == sDown )
        break;
    } /* end for */
  } /* end for */
  return false;
}

TSeatPlace &TSeatPlaces::GetEqualSeatPlace( TPassenger &pass )
{
	int MaxEqualQ = -10000;
  ISeatPlace misp=seatplaces.end();
  string ispPlaceName_lat, ispPlaceName_rus;
  for (ISeatPlace isp=seatplaces.begin(); isp!=seatplaces.end(); isp++) {
//    ProgTrace( TRACE5, "isp->oldPlaces.size()=%d, pass.countPlace=%d",
//               (int)isp->oldPlaces.size(), pass.countPlace );
    if ( isp->InUse || (int)isp->oldPlaces.size() != pass.countPlace ) /* ���-�� ���� ������ ᮢ������ */
      continue;
  	ispPlaceName_lat.clear();
  	ispPlaceName_rus.clear();
    SALONS2::TPlaceList *placeList = isp->placeList;
    ispPlaceName_lat = denorm_iata_row( placeList->GetYsName( isp->Pos.y ), NULL );
    ispPlaceName_rus = ispPlaceName_lat;
    ispPlaceName_lat += denorm_iata_line( placeList->GetXsName( isp->Pos.x ), 1 );
    ispPlaceName_rus += denorm_iata_line( placeList->GetXsName( isp->Pos.x ), 0 );
    int EqualQ = 0;
    if ( (int)isp->oldPlaces.size() == pass.countPlace )
      EqualQ = pass.countPlace*10000; //??? always true!
    bool pr_valid_place = true;
    vector<string> vrems;
    pass.get_remarks( vrems );
    for (vector<string>::iterator irem=vrems.begin(); irem!= vrems.end(); irem++ ) { // �஡�� �� ६�ઠ� ���ᠦ��
      for( vector<SALONS2::TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
      	/* �஡�� �� ���⠬ ����� ����� �������� ���ᠦ�� */
      	vector<SALONS2::TRem>::iterator itr;
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

    // ⨯ ����
    ProgTrace( TRACE5, "pass.pers_type=%s, isp->oldPlaces->begin=%s, is_valid_seat_elem_type=%d", pass.pers_type.c_str(),
               string(isp->oldPlaces.begin()->xname + isp->oldPlaces.begin()->yname).c_str(), is_valid_seat_elem_type( pass.pers_type, isp->oldPlaces ) );
    if ( is_valid_seat_elem_type( pass.pers_type, isp->oldPlaces ) )
    	EqualQ += PR_REM_TO_REM;
    else
    	EqualQ -= PR_REM_TO_REM;

    /*01.04.11if ( pass.placeName == ispPlaceName_lat || pass.placeName == ispPlaceName_rus ||  //???agent_seat!!!
    	   pass.preseat == ispPlaceName_lat || pass.preseat == ispPlaceName_rus )*/
    if ( pass.preseat_no == ispPlaceName_lat || pass.preseat_no == ispPlaceName_rus )
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
    if ( pass.prSmoke == isp->oldPlaces.begin()->isLayer( cltSmoke ) )
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
    //ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( MaxEqualQ < EqualQ ) {
      MaxEqualQ = EqualQ;
      misp = isp;
      misp->isValid = pr_valid_place;
    }
  } /* end for */
 if ( misp==seatplaces.end() )
   ProgError( STDLOG, "GetEqualSeatPlace: misp=seatplaces.end()=%d", misp==seatplaces.end() );
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
      throw EXCEPTIONS::Exception( "�������⨬�� ���祭�� ���ࠢ����� ��ᠤ��" );
    pass.placeList = seatPlace.placeList;
    pass.Pos = seatPlace.Pos;
    pass.Step = seatPlace.Step;
    pass.foundSeats = seatPlace.placeList->GetPlaceName( seatPlace.Pos );
    pass.isValidPlace = seatPlace.isValid;
    pass.set_seat_no();
  }
}

/* ��ᠤ�� �ᥩ ��㯯� ��稭�� � ����樨 FP */
bool TSeatPlaces::SeatsGrp_On( SALONS2::TPoint FP  )
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
    if ( SeatSubGrp_On( FP, sRight, 0 ) ) {
      return true;
    }
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
  bool OldCanUseSmoke = CanUseSmoke;
  bool CanUseSmoke = false;
  try {
    if ( !placeName.empty() ) {
      /* ��������� ����஢ ���� ���ᠦ�஢ � ����ᨬ��� �� ���. ��� ���. ᠫ��� */
      for ( vector<SALONS2::TPlaceList*>::iterator iplaceList=CurrSalon->placelists.begin();
            iplaceList!=CurrSalon->placelists.end(); iplaceList++ ) {
        SALONS2::TPoint FP;
        if ( (*iplaceList)->GetisPlaceXY( placeName, FP ) ) {
          CurrSalon->SetCurrPlaceList( *iplaceList );
          if ( SeatSubGrp_On( FP, Step, 0 ) ) {
          	tst();
            CanUseSmoke = OldCanUseSmoke;
            return true;
          }
          break;
        }
      }
    }
  }
  catch( ... ) {
    CanUseSmoke = OldCanUseSmoke;
    throw;
  }
  CanUseSmoke = OldCanUseSmoke;
  return false;
}

inline void getRemarks( TPassenger &pass )
{
  Remarks.clear();
  switch ( (int)CanUseRems ) {
    case sAllUse:
    case sOnlyUse:
    case sNotUse_NotUseDenial:
    case sNotUseDenial:
    	 pass.get_remarks( Remarks );
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
  vecSeatCoordsVars CoordsVars = seatCoords.getSeatCoordsVars( );
  try {
    /* ���� ��室���� ���� ��� ���ᠦ�� */
    int lp = Passengers.getCount();
    for ( int i=0; i<lp; i++ ) {
      /* �뤥�塞 �� ��㯯� �������� ���ᠦ�� � ��室�� ��� ���� ���� */
      TPassenger &pass = Passengers.Get( i );
      //ProgTrace( TRACE5, "pass.countPlace=%d, pass.step=%d(sDown=%d)", pass.countPlace, pass.Step, sDown );
      Passengers.SetCountersForPass( pass );
      getRemarks( pass );
      if ( !pass.preseat_no.empty() &&
           SeatsPassenger_OnBasePlace( pass.preseat_no, pass.Step ) && /* ��諨 ������� ���� */
           LSD( G3, G2, G, V3, V2, sEveryWhere ) )
        return true;

      RollBack( );
      Passengers.SetCountersForPass( pass );
      if ( !Remarks.empty() ) { /* ���� ६�ઠ */
        // ����⠥��� ���� �� ६�થ
        for ( int Where=sLeftRight; Where<=sUpDown; Where++ ) {
          /* ��ਠ��� ���᪠ ����� ���������� ���� */
          for ( int varCoord=0; varCoord<=1; varCoord++ ) {
            for( vecSeatCoordsVars::iterator icoord=CoordsVars.begin(); icoord!=CoordsVars.end(); icoord++ ) {
              CurrSalon->SetCurrPlaceList( icoord->placeList );
              for ( vector<TPoint>::iterator ic=icoord->coords[ varCoord ].begin(); ic!=icoord->coords[ varCoord ].end(); ic++ ) {
                if ( SeatSubGrp_On( *ic, pass.Step, 0 ) && LSD( G3, G2, G, V3, V2, (TWhere )Where ) ) {
                	//ProgTrace( TRACE5, "G3=%d, G2=%d, G=%d, V3=%d, V2=%d, commit", G3, G2, G, V3, V2 );
                	tst();
                  return true;
                }
                //ProgTrace( TRACE5, "rollback" );
                RollBack( ); /* �� ����稫��� �⪠� ������� ���� */
                Passengers.SetCountersForPass( pass ); /* �뤥�塞 ����� �⮣� ���ᠦ�� */
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
 vecSeatCoordsVars CoordsVars = seatCoords.getSeatCoordsVars( );
 for ( int varCoord=0; varCoord<=1; varCoord++ ) {
   for( vecSeatCoordsVars::iterator icoord=CoordsVars.begin(); icoord!=CoordsVars.end(); icoord++ ) {
     CurrSalon->SetCurrPlaceList( icoord->placeList );
     for ( vector<TPoint>::iterator ic=icoord->coords[ varCoord ].begin(); ic!=icoord->coords[ varCoord ].end(); ic++ ) {
     	 if ( SeatsGrp_On( *ic ) ) {
      	 return true;
       }
     }
   }
 }
 RollBack( );
 return false;
}

bool isUserProtectLayer( TCompLayerType layer_type )
{
  return ( layer_type == cltProtCkin ||
           layer_type == cltProtBeforePay ||
           layer_type == cltPNLBeforePay ||
           layer_type == cltProtAfterPay ||
           layer_type == cltPNLAfterPay );
}

/* ��ᠤ�� ���ᠦ�஢ �� ���⠬ �� ���뢠� ��㯯� */
bool TSeatPlaces::SeatsPassengers( bool pr_autoreseats )
{
  bool OLDFindSUBCLS = FindSUBCLS;
  bool OLDcanUseSUBCLS = canUseSUBCLS;
  string OLDSUBCLS_REM = SUBCLS_REM;
  vector<TPassenger> npass;
  Passengers.copyTo( npass );
  string OLDclname = Passengers.clname;
  bool OLDKTube = Passengers.KTube;
  bool OLDKWindow = Passengers.KWindow;
  bool OLDUseSmoke = Passengers.UseSmoke;
  int OLDp_Count_3G = Passengers.counters.p_Count_3(sRight);
  int OLDp_Count_2G = Passengers.counters.p_Count_2(sRight);
  int OLDp_CountG = Passengers.counters.p_Count(sRight);
  int OLDp_Count_3V = Passengers.counters.p_Count_3(sDown);
  int OLDp_Count_2V = Passengers.counters.p_Count_2(sDown);

  bool pr_seat = false;

  try {
  	for ( int FCanUseElem_Type=1; FCanUseElem_Type>=0; FCanUseElem_Type-- ) { // ���砥 ��⮬��. ��ᠤ�� 2 ��室�: � ��⮬ ⨯� ���� � ��� ��� ⨯� ����
    for ( /*int i=(int)!pr_autoreseats; i<=1+(int)pr_autoreseats; i++*/ int i=0; i<=2; i++ ) {
      for ( VPassengers::iterator ipass=npass.begin(); ipass!=npass.end(); ipass++ ) {
      	/* ����� ���ᠦ�� ��ᠦ�� ��� ��ᠤ�� �� �஭� � � ���ᠦ�� ����� �� �஭� ��� ��� �।���. ��ᠤ�� ��� � ���ᠦ�� 㪠���� �� � ���� */
      	if ( ipass->InUse )
      		pr_seat = true;
        if ( ipass->InUse )
          continue;
        if ( isUserProtectLayer( PlaceLayer ) && !CanUseLayer( PlaceLayer, UseLayers ) &&
             ( ipass->preseat_layer != PlaceLayer ) )
          continue;

          
/*???31.03.11        if ( ipass->InUse || PlaceLayer == cltProtCkin && !CanUseLayer( cltProtCkin, UseLayers ) && //!!!
        	                   ( ipass->layer != PlaceLayer || ipass->preseat.empty() || ipass->preseat != ipass->placeName ) )
          continue;*/
        Passengers.Clear();
        ipass->placeList = NULL;
        ipass->seat_no.clear();
        int old_index = ipass->index;

        PlaceElem_Types.clear();
        if ( FCanUseElem_Type == 1 && ipass->countPlace > 0 && ipass->pers_type != "��" ) {
         	getValidChildElem_Types( PlaceElem_Types );
        }

        if ( pr_autoreseats ) {
          if ( i == 0 ) {
          	  if ( ipass->SUBCLS_REM.empty() )
          	  	continue;
            	FindSUBCLS = true;
            	canUseSUBCLS = true;
            	SUBCLS_REM = ipass->SUBCLS_REM;
          }
          else
          	if ( i == 1 ) {
          		if ( !ipass->SUBCLS_REM.empty() )
          			continue;
          	  FindSUBCLS = false;
          	  canUseSUBCLS = true;
            }
            else {
              canUseSUBCLS = false;
            }
        }
        else { // ���� ��室: �᫨ �ᯮ������ ���� �� ���������, � ᠦ��� ��� ���ᠦ�஢ � �㦭� �������ᮬ
       			if ( i == 2 || i == 0 && canUseSUBCLS && ipass->SUBCLS_REM != SUBCLS_REM )
       		   continue; // ᥩ�� ���� ���� �� �������ᠬ, � � ���ᠦ�� ��� ��� - �� ��⠥��� ��� ��ᠤ���
       		  if ( i == 1 ) { // ᥩ�� ���� ���� ���� ��� ���ᠦ�஢ � ������ ��� ��������
/*       		  	ProgTrace( TRACE5, "ipass->SUBCLS_REM=%s, SUBCLS_REM=%s, canUseSUBCLS=%d, pr_seat_SUBCLS=%d",
       		  	           ipass->SUBCLS_REM.c_str(), SUBCLS_REM.c_str(), canUseSUBCLS, pr_seat_SUBCLS );*/
          		if ( canUseSUBCLS && !pr_seat )
          			continue;
       		  	FindSUBCLS = false;
//       		  	canUseSUBCLS = false;
       		  }
        }
        Passengers.Add( *ipass );
        ipass->index = old_index;



        if ( SeatGrpOnBasePlace( ) ||
             ( CanUseRems == sNotUse_NotUseDenial ||
               CanUseRems == sNotUse ||
               CanUseRems == sIgnoreUse ||
               CanUseRems == sNotUseDenial ) &&
             ( !CanUseLayers ||
               PlaceLayer == cltProtCkin && CanUseLayer( cltProtCkin, UseLayers ) ||
               PlaceLayer == cltProtAfterPay && CanUseLayer( cltProtAfterPay, UseLayers ) ||
               PlaceLayer == cltPNLAfterPay && CanUseLayer( cltPNLAfterPay, UseLayers ) ||
               PlaceLayer == cltProtBeforePay && CanUseLayer( cltProtBeforePay, UseLayers ) ||
               PlaceLayer == cltPNLBeforePay && CanUseLayer( cltPNLBeforePay, UseLayers ) ||
               !isUserProtectLayer( PlaceLayer ) ) &&
               SeatsGrp( ) ) { // ⮣�� ����� ��室��� ���� �� �ᥬ� ᠫ���
          if ( seatplaces.begin()->Step == sLeft || seatplaces.begin()->Step == sUp )
            throw EXCEPTIONS::Exception( "�������⨬�� ���祭�� ���ࠢ����� ��ᠤ��" );
          ipass->placeList = seatplaces.begin()->placeList;
          ipass->Pos = seatplaces.begin()->Pos;
          ipass->Step = seatplaces.begin()->Step;
          ipass->foundSeats = ipass->placeList->GetPlaceName( ipass->Pos );
          ipass->isValidPlace = ipass->is_valid_seats( seatplaces.begin()->oldPlaces );
          ipass->InUse = true;
          ipass->set_seat_no();
          TCompLayerType l = grp_status;
          Clear();
          grp_status = l;//??? ���� ⠪ ������, �� �� ��ᨢ�
          if ( !pr_autoreseats && i == 0 && canUseSUBCLS ) {
          	pr_seat = true;
          }
        }
      } // for passengers
    } // end for i=0..2
    } // end for FCanUseElem_Type
  }
  catch( ... ) {
    FindSUBCLS = OLDFindSUBCLS;
    canUseSUBCLS = OLDcanUseSUBCLS;
    SUBCLS_REM = OLDSUBCLS_REM;
    Passengers.Clear();
    Passengers.copyFrom( npass );
    Passengers.clname = OLDclname;
    Passengers.KTube = OLDKTube;
    Passengers.KWindow = OLDKWindow;
    Passengers.UseSmoke = OLDUseSmoke;
    Passengers.counters.Set_p_Count_3( OLDp_Count_3G, sRight );
    Passengers.counters.Set_p_Count_2( OLDp_Count_2G, sRight );
    Passengers.counters.Set_p_Count( OLDp_CountG, sRight );
    Passengers.counters.Set_p_Count_3( OLDp_Count_3V, sDown );
    Passengers.counters.Set_p_Count_2( OLDp_Count_2V, sDown );
    throw;
  }
  FindSUBCLS = OLDFindSUBCLS;
  canUseSUBCLS = OLDcanUseSUBCLS;
  SUBCLS_REM = OLDSUBCLS_REM;
  Passengers.Clear();
  Passengers.copyFrom( npass );
  Passengers.clname = OLDclname;
  Passengers.KTube = OLDKTube;
  Passengers.KWindow = OLDKWindow;
  Passengers.UseSmoke = OLDUseSmoke;
  Passengers.counters.Set_p_Count_3( OLDp_Count_3G, sRight );
  Passengers.counters.Set_p_Count_2( OLDp_Count_2G, sRight );
  Passengers.counters.Set_p_Count( OLDp_CountG, sRight );
  Passengers.counters.Set_p_Count_3( OLDp_Count_3V, sDown );
  Passengers.counters.Set_p_Count_2( OLDp_Count_2V, sDown );

  for ( VPassengers::iterator ipass=npass.begin(); ipass!=npass.end(); ipass++ ) {
    if ( !ipass->InUse ) {
      return false;
    }
  }
  return true;
}
///////////////////////////////////////////////
void TPassenger::set_seat_no()
{
  seat_no.clear();
  int x = Pos.x;
  int y = Pos.y;
  for ( int j=0; j<countPlace; j++ ) {
    TSeat s;
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

TSublsRems::TSublsRems( const std::string &vairline )
{
	airline = vairline;
	TQuery Qry(&OraSession );
	Qry.SQLText =
	 "SELECT subclass,rem FROM comp_subcls_sets "
	 " WHERE airline=:airline ";
	Qry.CreateVariable( "airline", otString, vairline );
	Qry.Execute();
	while ( !Qry.Eof ) {
		TSublsRem r;
		r.subclass = Qry.FieldAsString( "subclass" );
		r.rem = Qry.FieldAsString( "rem" );
		rems.push_back( r );
		Qry.Next();
	}
}


bool TSublsRems::IsSubClsRem( const string &subclass, string &vrem )
{
	vrem.clear();
	for ( vector<TSublsRem>::iterator i=rems.begin(); i!=rems.end(); i++ ) {
		if ( i->subclass == subclass ) {
			vrem = i->rem;
			break;
		}
	}
  return !vrem.empty();
}

void TPassenger::add_rem( std::string code )
{
	if ( isREM_SUBCLS( code ) )
		SUBCLS_REM = code;
	rems.push_back( code );
}

void TPassenger::calc_priority( std::map<std::string, int> &remarks )
{
  for (vector<string>::iterator ir=rems.begin(); ir!=rems.end(); ) {
  	if ( remarks.find( *ir ) == remarks.end() )
  		ir = rems.erase( ir );
  	else
  		ir++;
  }
  priority = 0;
  if ( !preseat_no.empty() ) //???
    priority = PR_N_PLACE;
  if ( !placeRem.empty() )
    priority += PR_REMPLACE;
  if ( prSmoke )
    priority += PR_SMOKE;
  int vpriority = 0;
  maxRem.clear();
  for ( std::vector<string>::iterator irem = rems.begin(); irem != rems.end(); irem++ ) {
    if ( remarks[ *irem ] > vpriority ) {
      vpriority = remarks[ *irem ];
      maxRem = *irem;
    }
    priority += remarks[ *irem ]*countPlace; //???
  }
  //???  priority += priority*countPlace;
}

void TPassenger::get_remarks( std::vector<std::string> &vrems )
{
 	vrems.clear();
  for (std::vector<string>::iterator irem=rems.begin(); irem!=rems.end(); irem++ ) {
    vrems.push_back( *irem );
  }
}

bool TPassenger::isRemark( std::string code )
{
  for (std::vector<string>::iterator irem=rems.begin(); irem!=rems.end(); irem++ ) {
  	if ( *irem == code )
  		return true;
  }
	return false;
}

bool TPassenger::is_valid_seats( const std::vector<SALONS2::TPlace> &places )
{
  for ( std::vector<string>::iterator irem=rems.begin(); irem!= rems.end(); irem++ ) {
    for( std::vector<SALONS2::TPlace>::const_iterator ipl=places.begin(); ipl!=places.end(); ipl++ ) {
     /* �஡�� �� ���⠬ ����� ����� �������� ���ᠦ�� */
      std::vector<SALONS2::TRem>::const_iterator itr;
      for ( itr=ipl->rems.begin(); itr!=ipl->rems.end(); itr++ )
        if ( *irem == itr->rem && itr->pr_denial )
       	  return false;
    }
  }
  for( std::vector<SALONS2::TPlace>::const_iterator ipl=places.begin(); ipl!=places.end(); ipl++ ) {
    if ( !ipl->layers.empty() &&
         isUserProtectLayer( ipl->layers.begin()->layer_type ) &&
         ipl->layers.begin()->pax_id != paxId )
      return false;
  }
  return is_valid_seat_elem_type( pers_type, places );
}

void TPassenger::build( xmlNodePtr pNode, const TDefaults& def )
{
  NewTextChild( pNode, "grp_id", grpId );
  NewTextChild( pNode, "pax_id", paxId );
  if (TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
  {
    NewTextChild( pNode, "clname", clname, def.clname );
    NewTextChild( pNode, "grp_layer_type",
                         EncodeCompLayerType(grp_status),
                         EncodeCompLayerType(def.grp_status) );
    NewTextChild( pNode, "pers_type",
                         ElemIdToCodeNative(etPersType, pers_type),
                         ElemIdToCodeNative(etPersType, def.pers_type) );
  }
  else
  {
    NewTextChild( pNode, "clname", clname );
    NewTextChild( pNode, "grp_layer_type",
                         EncodeCompLayerType(grp_status) );
    NewTextChild( pNode, "pers_type", pers_type );
  };
  NewTextChild( pNode, "reg_no", regNo );
  NewTextChild( pNode, "name", fullName );
  NewTextChild( pNode, "seat_no", foundSeats, def.placeName );
  NewTextChild( pNode, "wl_type", wl_type, def.wl_type );
  NewTextChild( pNode, "seats", countPlace, def.countPlace );
  NewTextChild( pNode, "tid", tid );
  NewTextChild( pNode, "isseat", (int)isSeat, (int)def.isSeat );
  NewTextChild( pNode, "ticket_no", ticket_no, def.ticket_no );
  NewTextChild( pNode, "document", document, def.document );
  NewTextChild( pNode, "bag_weight", bag_weight, def.bag_weight );
  NewTextChild( pNode, "bag_amount", bag_amount, def.bag_amount );
  NewTextChild( pNode, "excess", excess, def.excess );
  NewTextChild( pNode, "trip_from", trip_from, def.trip_from );

  string comp_rem;
  bool pr_down = false;
  if ( !rems.empty() ) {
  	for ( vector<string>::iterator r=rems.begin(); r!=rems.end(); r++ ) {
  		comp_rem += *r + " ";
  		if ( *r == "STCR" )
  			pr_down = true;
  	};
  };
  NewTextChild( pNode, "comp_rem", comp_rem, def.comp_rem );
  NewTextChild( pNode, "pr_down", (int)pr_down, (int)def.pr_down );
  NewTextChild( pNode, "pass_rem", pass_rem, def.pass_rem );
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

void TPassengers::LoadRemarksPriority( std::map<std::string, int> &rems )
{
	rems.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT code, pr_comp FROM comp_rem_types WHERE pr_comp IS NOT NULL";
  Qry.Execute();
  while ( !Qry.Eof ) {
    rems[ Qry.FieldAsString( "code" ) ] = Qry.FieldAsInteger( "pr_comp" );
    Qry.Next();
  }
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
  bool Pr_PLC = false;
  if ( pass.countPlace > 1 && pass.isRemark( string( "STCR" ) )	 ) {
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
  	LoadRemarksPriority( remarks );
  pass.calc_priority( remarks );
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

void TPassengers::Add( SALONS2::TSalons &Salons, TPassenger &pass )
{
  if ( pass.countPlace > CONST_MAXPLACE || pass.countPlace <= 0 )
   throw EXCEPTIONS::Exception( "�� �����⨬�� ���-�� ���� ��� �ᠤ��" );

  size_t i = 0;
  for (; i < pass.preseat_no.size(); i++)
    if ( pass.preseat_no[ i ] != '0' )
      break;
  if ( i )
    pass.preseat_no.erase( 0, i );
  pass.preseat_layer = cltUnknown; // ����砥�, �� ����� �����稫 ����, ��� �� ���� �� ������-����� ᫮� - ���� �஢�����
  pass.preseat_pax_id = 0;
  // ��।������ ᠬ��� �ਮ��⭮�� ᫮� � ����� ���� ��� ���ᠦ��
  if ( pass.grp_status == cltTranzit ) {
     pass.preseat_layer = cltProtTrzt;
  }
  else { // ��।������ �ਮ��⭮�� ᫮� ��� ���ᠦ��
    SALONS2::TPlaceList *placeList;
    for ( vector<SALONS2::TPlaceList*>::iterator plList=Salons.placelists.begin();
          plList!=Salons.placelists.end(); plList++ ) {
      placeList = *plList;
      for ( IPlace i=placeList->places.begin(); i!=placeList->places.end(); i++ ) {
        if ( !i->visible || !i->isplace || !i->isPax || i->layers.begin()->pax_id != pass.paxId )
          continue;
        TPoint p( i->x, i->y );
        pass.preseat_no = denorm_iata_row( i->yname, NULL ) + denorm_iata_line( i->xname, Salons.getLatSeat() );
        pass.preseat_layer = i->layers.begin()->layer_type;
        pass.preseat_pax_id = i->layers.begin()->pax_id;
        break;
      }
    }
  }
  Add( pass );
}

int TPassengers::getCount()
{
  return FPassengers.size();
}

TPassenger &TPassengers::Get( int Idx )
{
  if ( Idx < 0 || Idx >= (int)FPassengers.size() )
    throw EXCEPTIONS::Exception( "Passeneger index out of range" );
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


void SetLayers( vector<TCompLayerType> &Layers,
                bool &CanUseMutiLayer,
                TCompLayerType layer,
                int Step,
                const TUseLayers &preseat_layers,
                TClientType client_type )
{
  Layers.clear();
  CanUseMutiLayer = ( Step <= -1 );
  switch ( layer ) {
    case cltTranzit:
    case cltProtTrzt:
      if ( Step != 0 ) {
        Layers.push_back( cltProtTrzt );
        Layers.push_back( cltUnknown );
        Layers.push_back( cltUncomfort );
      };
      if ( Step != 1 ) {
        if ( client_type == ASTRA::ctTerm )
          Layers.push_back( cltProtect );
        Layers.push_back( cltPNLCkin );
      	for( TUseLayers::const_iterator l=preseat_layers.begin(); l!=preseat_layers.end(); l++ ) {
          if ( l->second )
            Layers.push_back( l->first );
        }
      }
      break;
    case cltProtect:
      if ( Step != 0 ) {
        if ( client_type == ASTRA::ctTerm )
          Layers.push_back( cltProtect );
        Layers.push_back( cltUnknown );
      };
      if ( Step != 1 ) {
        Layers.push_back( cltUncomfort );
        Layers.push_back( cltPNLCkin );
      	for( TUseLayers::const_iterator l=preseat_layers.begin(); l!=preseat_layers.end(); l++ ) {
          if ( l->second )
            Layers.push_back( l->first );
        }
      }
      break;
    case cltUnknown:
      if ( Step != 0 ) {
        Layers.push_back( cltUnknown );
        Layers.push_back( cltUncomfort );
      };
      if ( Step != 1 ) {
        if ( client_type == ASTRA::ctTerm )
          Layers.push_back( cltProtect );
        Layers.push_back( cltPNLCkin );
      	for( TUseLayers::const_iterator l=preseat_layers.begin(); l!=preseat_layers.end(); l++ ) {
          if ( l->second )
            Layers.push_back( l->first );
        }
      }
      break;
    case cltPNLCkin:
      if ( Step != 0 ) {
       	Layers.push_back( cltPNLCkin );
        Layers.push_back( cltUnknown );
        Layers.push_back( cltUncomfort );
      };
      if ( Step != 1 ) {
        if ( client_type == ASTRA::ctTerm )
          Layers.push_back( cltProtect );
       	for( TUseLayers::const_iterator l=preseat_layers.begin(); l!=preseat_layers.end(); l++ ) {
          if ( l->second )
            Layers.push_back( l->first );
        }
      }
      break;
    case cltProtCkin:
    case cltPNLBeforePay: //???
    case cltProtBeforePay:
    case cltPNLAfterPay: //???
    case cltProtAfterPay:
   		if ( Step != 0 ) {
       	for( TUseLayers::const_iterator l=preseat_layers.begin(); l!=preseat_layers.end(); l++ ) {
          if ( l->second )
            Layers.push_back( l->first );
        }
        Layers.push_back( cltUnknown );
        Layers.push_back( cltUncomfort );
      }
      if ( Step != 1 ) {
        if ( client_type == ASTRA::ctTerm )
      	  Layers.push_back( cltProtect );
      	Layers.push_back( cltPNLCkin );
      }
      break;
    default:;
  }
}

/*///////////////////////////END CLASS TPASSENGERS/////////////////////////*/

bool ExistsBasePlace( SALONS2::TSalons &Salons, TPassenger &pass )
{
  SALONS2::TPlaceList *placeList;
  SALONS2::TPoint FP;
  vector<SALONS2::TPlace*> vpl;
  string placeName = pass.preseat_no;
  for ( vector<SALONS2::TPlaceList*>::iterator plList=Salons.placelists.begin();
        plList!=Salons.placelists.end(); plList++ ) {
    placeList = *plList;
    if ( placeList->GetisPlaceXY( placeName, FP ) ) {
      int j = 0;
      for ( ; j<pass.countPlace; j++ ) {
        if ( !placeList->ValidPlace( FP ) )
          break;
        SALONS2::TPlace *place = placeList->place( FP );
        bool findpass = !pass.SUBCLS_REM.empty();
        bool findplace = false;
        for ( vector<SALONS2::TRem>::iterator r=place->rems.begin(); r!=place->rems.end(); r++ ) {
        	if ( !r->pr_denial && r->rem == pass.SUBCLS_REM ) {
        		findplace = true;
        		break;
        	}
        }
        ProgTrace( TRACE5, "Salons.placeIsFree( place )=%d, seat_no=%s", Salons.placeIsFree( place ), string(place->yname+place->xname).c_str() );
      	for ( std::vector<TPlaceLayer>::iterator i=place->layers.begin(); i!=place->layers.end(); i++ ) {
      		ProgTrace( TRACE5, "layer_type=%s", EncodeCompLayerType(i->layer_type) );
        }

        if ( !place->visible || !place->isplace ||
             !Salons.placeIsFree( place ) || pass.clname != place->clname ||
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
        for ( vector<SALONS2::TPlace*>::iterator ipl=vpl.begin(); ipl!=vpl.end(); ipl++ ) {
          if ( ipl == vpl.begin() ) {
            pass.InUse = true;
            pass.placeList = placeList;
            pass.Pos.x = (*ipl)->x;
            pass.Pos.y = (*ipl)->y;
            pass.set_seat_no();
          }
          (*ipl)->AddLayerToPlace( pass.grp_status, 0, pass.paxId, NoExists, NoExists, Salons.getPriority( pass.grp_status ) );
        }
        return true;
      }
      vpl.clear();
    }
  }
  return false;
}

/* ��ᠤ�� ���ᠦ�஢ */
void SeatsPassengers( SALONS2::TSalons *Salons,
                      TSeatAlgoParams ASeatAlgoParams /* sdUpDown_Line - 㬮�砭�� */,
                      TClientType client_type,
                      TPassengers &passengers )
{
	ProgTrace( TRACE5, "NEWSEATS, ASeatAlgoParams=%d", (int)ASeatAlgoParams.SeatAlgoType );
  if ( !passengers.getCount() )
    return;

  GetUseLayers( UseLayers );
  TUseLayers preseat_layers, curr_preseat_layers;

  int param1,param2,param4,param5,param6,param7,param9,param10,param11;
  string param3,param8,param12;

  FSeatAlgoParams = ASeatAlgoParams;
  SeatPlaces.Clear();
  SeatPlaces.grp_status = passengers.Get( 0 ).grp_status;
  CurrSalon = Salons;
  CanUseLayers = true;
  CanUseSmoke = false; /* ���� �� �㤥� ࠡ���� � ����騬� ���⠬� */
  vector<string> PElemTypes;
  preseat_layers[ cltProtCkin ] = CanUseLayer( cltProtCkin, UseLayers );
  preseat_layers[ cltProtBeforePay ] = CanUseLayer( cltProtBeforePay, UseLayers );
  preseat_layers[ cltProtAfterPay ] = CanUseLayer( cltProtAfterPay, UseLayers );
  preseat_layers[ cltPNLBeforePay ] = CanUseLayer( cltPNLBeforePay, UseLayers );
  preseat_layers[ cltPNLAfterPay ] = CanUseLayer( cltPNLAfterPay, UseLayers );
  
  Passengers.KWindow = ( Passengers.KWindow && !Passengers.KTube );
  Passengers.KTube = ( !Passengers.KWindow && Passengers.KTube );
  seatCoords.refreshCoords( FSeatAlgoParams.SeatAlgoType, Passengers.KWindow, Passengers.KTube );
  SeatAlg = sSeatGrpOnBasePlace;

  FindSUBCLS = false;
  canUseSUBCLS = false;
  SUBCLS_REM = "";
  CanUseMutiLayer = false;

  /* �� ᤥ����!!! �᫨ � ��� ���ᠦ�஢ ���� ����, � ⮣�� ��ᠤ�� �� ���⠬, ��� ��� ��㯯� */

  /* ��।������ ���� �� � ��㯯� ���ᠦ�� � �।���⥫쭮� ��ᠤ��� */
  bool pr_pay = false;
  bool Status_seat_no_BR=false, pr_all_pass_SUBCLS=true, pr_SUBCLS=false;
  for ( int i=0; i<passengers.getCount(); i++ ) {
  	TPassenger &pass = passengers.Get( i );
  	if ( isUserProtectLayer( pass.preseat_layer ) ) {
      preseat_layers[ pass.preseat_layer ] = true;
      ProgTrace( TRACE5, "preseat_layers: pass.preseat_layer=%s", EncodeCompLayerType( pass.preseat_layer ) );
      if ( pass.preseat_layer == cltProtBeforePay ||
           pass.preseat_layer == cltPNLBeforePay ||
           pass.preseat_layer == cltProtAfterPay ||
           pass.preseat_layer == cltPNLAfterPay )
        pr_pay = true;
  	}
  	if ( PElemTypes.size() == 0 && pass.countPlace > 0 && pass.pers_type != "��"  ) {
  		getValidChildElem_Types( PElemTypes );
  	}
  	if ( !pass.SUBCLS_REM.empty() ) {
  		pr_SUBCLS = true;
  		if ( !SUBCLS_REM.empty() && SUBCLS_REM != pass.SUBCLS_REM )
  			pr_all_pass_SUBCLS = false;
  		SUBCLS_REM = pass.SUBCLS_REM;
    }
    else
    	pr_all_pass_SUBCLS = false;

  }
  condRates.Init( *Salons, pr_pay ); // ᮡ�ࠥ� �� ⨯� ������ ���� � ���ᨢ �� �ਮ��⠬, �᫨ �� web-������, � �� ���뢠��

  ProgTrace( TRACE5, "pr_SUBCLS=%d,pr_all_pass_SUBCLS=%d, SUBCLS_REM=%s, pr_pay=%d", pr_SUBCLS, pr_all_pass_SUBCLS, SUBCLS_REM.c_str(), pr_pay );

  bool SeatOnlyBasePlace=true;
  for ( int i=0; i<passengers.getCount(); i++ ) {
  	TPassenger &pass = passengers.Get( i );
  	if ( pass.preseat_no.empty() ) {
  		SeatOnlyBasePlace=false;
  		break;
  	}
  }

  try {
   for ( int FCanUserSUBCLS=(int)pr_SUBCLS; FCanUserSUBCLS>=0; FCanUserSUBCLS-- ) {
   	 if ( pr_SUBCLS && FCanUserSUBCLS == 0 )
   	   ProgTrace( TRACE5, ">>>SeatsPassengers: error FCanUserSUBCLS=false, pr_SUBCLS=%d,pr_all_pass_SUBCLS=%d, SUBCLS_REM=%s", pr_SUBCLS, pr_all_pass_SUBCLS, SUBCLS_REM.c_str() );
     FindSUBCLS = FCanUserSUBCLS;
     canUseSUBCLS = FCanUserSUBCLS;
     // ??? �� ������ ��� ���ᠦ�஢ � ���쬨: ��࠭��� ��㯯� ��� ���� ࠧ���쭮, �� �� ࠧ�襭�� ���� ???
     // ���� ࠧ������
     for ( int FCanUseElem_Type=!PElemTypes.empty(); FCanUseElem_Type>=0; FCanUseElem_Type-- ) { // ���� �� ⨯�� ���� + �����஢���� ⨯� ����
       PlaceElem_Types.clear();
   	   if ( FCanUseElem_Type != 0 ) {
         PlaceElem_Types.assign( PElemTypes.begin(), PElemTypes.end() );
   	   }

       for ( int FSeatAlg=0; FSeatAlg<seatAlgLength; FSeatAlg++ ) {
         SeatAlg = (TSeatAlg)FSeatAlg;
         if ( SeatAlg == sSeatPassengers && !PElemTypes.empty() && FCanUseElem_Type == 0 )
           continue;
         bool use_preseat_layer = ( CanUseLayer( cltProtCkin, preseat_layers ) ||
                                    CanUseLayer( cltProtBeforePay, preseat_layers ) ||
                                    CanUseLayer( cltPNLBeforePay, preseat_layers ) ||
                                    CanUseLayer( cltProtAfterPay, preseat_layers ) ||
                                    CanUseLayer( cltPNLAfterPay, preseat_layers ) );
         /* �᫨ ���� � ��㯯� �।���⥫쭠� ��ᠤ��, � ⮣�� ᠦ��� ��� �⤥�쭮 */
         /* �᫨ ���� � ��㯯� �������� � � �� �� � ��� ���ᠦ�஢, � ⮣�� ᠦ��� ��� �⤥�쭮 */
         if ( ( use_preseat_layer ||
                Status_seat_no_BR ||
                SeatOnlyBasePlace || // ��� ������� ���ᠦ�� ������ ᢮� ����� ����
                canUseSUBCLS && pr_SUBCLS && !pr_all_pass_SUBCLS ) // �᫨ ���� ��㯯� ���ᠦ�஢ �।� ������ ���� ���ᠦ��� � �������ᮬ, �� �� ��
      	      &&
      	      SeatAlg != sSeatPassengers ) {
      	    continue;
         }
         for ( int FCanUseRems=0; FCanUseRems<useremLength; FCanUseRems++ ) {
           CanUseRems = (TUseRem)FCanUseRems;
           switch( (int)SeatAlg ) {
             case sSeatGrpOnBasePlace:
               switch( (int)CanUseRems ) {
                 case sIgnoreUse:
                 case sNotUse_NotUseDenial:
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
             case sSeatPassengers:
               if ( use_preseat_layer && CanUseRems != sIgnoreUse ) // ������㥬 ६�ન � ��砥 �।���⥫쭮� ࠧ��⪨
                 continue;
               break;
           }

           /* �ᯮ�짮����� ������ ���� */
           for ( condRates.current_rate = condRates.rates.begin(); condRates.current_rate != condRates.rates.end(); condRates.current_rate++ ) {
             if ( condRates.current_rate != condRates.rates.begin() && SeatAlg != sSeatPassengers ) { //???
//               ProgTrace( TRACE5, "condRates.current_rate->first=%d", condRates.current_rate->first );
               continue;
             }
             //ProgTrace( TRACE5, "SeatAlg == %d, condRates.current_rate.first=%d, condRates.current_rate->second.value=%f",
//                        SeatAlg, condRates.current_rate->first, condRates.current_rate->second.value );
             /* �ᯮ�짮����� ����ᮢ ���� */
             //���� ���뢠�� ��砩, ����� ��⠫��� ���� ⮫쪮 � ᫮ﬨ cltProtBeforePay, cltPNLBeforePay, cltProtAfterPay, cltPNLAfterPay - ���� 㬥��
             // ���砫� ���� ���஡����� �ᯮ�짮���� ⮫쪮 ���� � ᫮ﬨ cltProtBeforePay, cltPNLBeforePay, � ��⮬ + cltProtAfterPay, cltPNLAfterPay
             for ( int KeyLayers=1; KeyLayers>=-3; KeyLayers-- ) {
               if ( !KeyLayers && ( SeatAlg == sSeatGrpOnBasePlace || SeatAlg == sSeatGrp ) ||
                     KeyLayers < -1 && ( SeatAlg != sSeatPassengers || !condRates.isIgnoreRates( ) ) ) { // �᫨ ����� �ᯮ�짮���� ����� ᫮�, � ⮫쪮 �� ��ᠤ�� ���ᠦ�஢ �� ������
               	 continue;
               }
               curr_preseat_layers.clear();
               curr_preseat_layers.insert( preseat_layers.begin(), preseat_layers.end() );
               if ( KeyLayers == -2 ) {
                 if ( CanUseLayer( cltProtBeforePay, curr_preseat_layers ) &&
                      CanUseLayer( cltPNLBeforePay, curr_preseat_layers ) ) {
                   continue;
                 }
                 curr_preseat_layers[ cltProtBeforePay ] = true;
                 curr_preseat_layers[ cltPNLBeforePay ] = true;
               }
               if ( KeyLayers == -3 ) {
                 if ( CanUseLayer( cltProtBeforePay, curr_preseat_layers ) &&
                      CanUseLayer( cltPNLBeforePay, curr_preseat_layers ) &&
                      CanUseLayer( cltProtBeforePay, curr_preseat_layers ) &&
                      CanUseLayer( cltPNLBeforePay, curr_preseat_layers ) ) {
                   continue;
                 }
                 curr_preseat_layers[ cltProtBeforePay ] = true;
                 curr_preseat_layers[ cltPNLBeforePay ] = true;
                 curr_preseat_layers[ cltProtAfterPay ] = true;
                 curr_preseat_layers[ cltPNLAfterPay ] = true;
               }
               /* ������ ���ᨢ ����ᮢ ���� */
               SetLayers( SeatsLayers,
                          CanUseMutiLayer,
                          passengers.Get( 0 ).preseat_layer,
                          KeyLayers,
                          curr_preseat_layers,
                          client_type );
               /* �஡�� �� ����ᮬ */
               for ( vector<TCompLayerType>::iterator l=SeatsLayers.begin(); l!=SeatsLayers.end(); l++ ) {
                 PlaceLayer = *l;
                 /* ��⠢���� ������ ��᪮�쪮 ࠧ �� �鸞� �� ���ᠤ�� ��㯯� */
                 for ( int FCanUseAlone=uFalse3; FCanUseAlone<=uTrue; FCanUseAlone++ ) {
                   CanUseAlone = (TUseAlone)FCanUseAlone;
                   if ( CanUseAlone == uTrue && CanUseRems == sNotUse_NotUseDenial	 ) {
            	     // �᫨ ���ᠦ�஢ ����� ��⠢���� ᪮�쪮 㣮��� ࠧ �� ������ � ��� � ����� ᠦ��� ⮫쪮 �� ���� � �㦭묨 ६�ઠ��
            	       continue;
                   }
                   if ( CanUseAlone == uTrue && SeatAlg == sSeatPassengers ) {
                     continue;
                   }
                   if ( !KeyLayers && CanUseAlone == uFalse3 ) {
                     continue;
                   }
                   /* ��� ०��� ��ᠤ�� � ����� ��� */
                   for ( int FCanUseOneRow=getCanUseOneRow(); FCanUseOneRow>=0; FCanUseOneRow-- ) {
                	   canUseOneRow = FCanUseOneRow;
                     /* ��� ��室�� */
                     for ( int FCanUseTube=0; FCanUseTube<=1; FCanUseTube++ ) {
                       /* ��� ��ᠤ�� �⤥���� ���ᠦ�஢ �� ���� ���뢠�� ��室� */
                       if ( FCanUseTube && SeatAlg == sSeatPassengers ) {
                         continue;
                       }
                       if ( FCanUseTube && CanUseAlone == uFalse3 && !canUseOneRow ) {
                         continue;
                       }
                	     if ( canUseOneRow && !FCanUseTube ) {
                	  	   continue;
                	     }
                       CanUseTube = FCanUseTube;
                       for ( int FCanUseSmoke=passengers.UseSmoke; FCanUseSmoke>=0; FCanUseSmoke-- ) {
                         CanUseSmoke = FCanUseSmoke;
                         param1 = (int)SeatAlg;
                         param2 = FCanUseElem_Type;
                         param3 = DecodeCanUseRems( CanUseRems );
                         param4 = FCanUseAlone;
                         param5 = KeyLayers;
                         param6 = FCanUseTube;
                         param7 = FCanUseSmoke;
                         param8 = EncodeCompLayerType(PlaceLayer);
                         param9 = MAXPLACE();
                         param10 = canUseOneRow;
                         param11 = canUseSUBCLS;
                         param12 = SUBCLS_REM;
                         switch( (int)SeatAlg ) {
                           case sSeatGrpOnBasePlace:
                             if ( SeatPlaces.SeatGrpOnBasePlace( ) ) {
                               throw 1;
                             }
                             break;
                           case sSeatGrp:
                        	   getRemarks( passengers.Get( 0 ) ); // ��� ᠬ��� �ਮ��⭮��
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
                         } /* end switch SeatAlg */
                       } /* end for FCanUseSmoke */
                     } /* end for FCanUseTube */
                   } /* end for FCanUseOneRow */
                 } /* end for FCanUseAlone */
               } /* end for use SeatsLayers */
             } /* end for KeyLayer */
           } /* end for condRates */
         } /* end for CanUseRem */
       } /* end for FSeatAlg */
     } /*  end for FCanUseElem_Type */
   } /*  end for FCanUserSUBCLS */
    SeatAlg = (TSeatAlg)0;
  }
  catch( int ierror ) {
    if ( ierror != 1 )
      throw;
    ProgTrace( TRACE5, "seats with:SeatAlg=%d,FCanUseElem_Type=%d,FCanUseRems=%s,FCanUseAlone=%d,KeyLayers=%d,FCanUseTube=%d,FCanUseSmoke=%d,PlaceStatus=%s, MAXPLACE=%d,canUseOneRow=%d, CanUseSUBCLS=%d, SUBCLS_REM=%s",
               param1,param2,param3.c_str(),param4,param5,param6,param7,param8.c_str(),param9,param10,param11,param12.c_str());

//    ProgTrace( TRACE5, "SeatAlg=%d, CanUseRems=%d", (int)SeatAlg, (int)CanUseRems );
    /* ��।������ ����祭��� ���� �� ���ᠦ�ࠬ, ⮫쪮 ��� SeatPlaces.SeatGrpOnBasePlace */
    if ( SeatAlg != sSeatPassengers ) {
      SeatPlaces.PlacesToPassengers( );
    }

    Passengers.sortByIndex();

    SeatPlaces.RollBack( );
    return;
  }
  SeatPlaces.RollBack( );
  throw UserException( "MSG.SEATS.NOT_AVAIL_AUTO_SEATS" );
}

bool GetPassengersForWaitList( int point_id, TPassengers &p )
{
	bool res = false;
	TQuery Qry( &OraSession );
  TQuery RemsQry( &OraSession );
  TQuery PaxDocQry( &OraSession );
  TPaxSeats priorSeats( point_id );

  p.Clear();
  RemsQry.SQLText =
    "SELECT rem, rem_code, pax.pax_id, comp_rem_types.pr_comp "
    " FROM pax_rem, pax_grp, pax, comp_rem_types "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_dep=:point_id AND "
    "      pax.pr_brd IS NOT NULL AND "
    "      pax.seats > 0 AND "
    "      pax_rem.pax_id=pax.pax_id AND "
    "      rem_code=comp_rem_types.code(+) "
    " ORDER BY pax.pax_id, pr_comp, code ";
  RemsQry.CreateVariable( "point_id", otInteger, point_id );

  Qry.SQLText =
    "SELECT airline "
    " FROM points "
    " WHERE points.point_id=:point_id AND points.pr_del!=-1 AND points.pr_reg<>0";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
  	throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  string airline = Qry.FieldAsString( "airline" );
  TGrpStatusTypes &grp_status_types = (TGrpStatusTypes &)base_tables.get("GRP_STATUS_TYPES");

  Qry.Clear();
  Qry.SQLText =
    "SELECT pax_grp.grp_id, "
    "       pax.pax_id, "
    "       pax.reg_no, "
    "       surname, "
    "       pax.name, "
    "       pax_grp.class, "
    "       cls_grp.code subclass, "
    "       pax.seats, "
    "       pax_grp.status, "
    "       pax.pers_type, "
    "       pax.ticket_no, "
    "       ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_weight, "
    "       ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum) AS bag_amount, "
    "       ckin.get_excess(pax.grp_id,pax.pax_id) AS excess, "
    "       pax.tid, "
    "       pax.wl_type, "
    "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'list',rownum) AS seat_no, "
    "       tckin_pax_grp.tckin_id, tckin_pax_grp.seg_no "
    "FROM pax_grp, pax, cls_grp, tckin_pax_grp "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_dep=:point_id AND "
    "      pax_grp.class_grp = cls_grp.id AND "
    "      pax_grp.grp_id=tckin_pax_grp.grp_id(+) AND "
    "      pax.pr_brd IS NOT NULL AND "
    "      pax.seats > 0 "
    " ORDER BY pax.pax_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();

  TSublsRems subcls_rems(airline);

  RemsQry.Execute();
  TCkinRoute tckin_route;
  while ( !Qry.Eof ) {
    TPassenger pass;
    pass.paxId = Qry.FieldAsInteger( "pax_id" );
    pass.foundSeats = Qry.FieldAsString( "seat_no" );
    pass.clname = Qry.FieldAsString( "class" );
    pass.countPlace = Qry.FieldAsInteger( "seats" );
    pass.tid = Qry.FieldAsInteger( "tid" );
    pass.grpId = Qry.FieldAsInteger( "grp_id" );
    pass.regNo = Qry.FieldAsInteger( "reg_no" );
    string fname = Qry.FieldAsString( "surname" );
    pass.fullName = TrimString( fname ) + " " + Qry.FieldAsString( "name" );
    pass.ticket_no = Qry.FieldAsString( "ticket_no" );
    pass.document = CheckIn::GetPaxDocStr(NoExists, Qry.FieldAsInteger( "pax_id" ), PaxDocQry, true);
    pass.bag_weight = Qry.FieldAsInteger( "bag_weight" );
    pass.bag_amount = Qry.FieldAsInteger( "bag_amount" );
    pass.excess = Qry.FieldAsInteger( "excess" );
    pass.grp_status = DecodeCompLayerType(((TGrpStatusTypesRow&)grp_status_types.get_row("code",Qry.FieldAsString( "status" ))).layer_type.c_str());
    pass.pers_type = Qry.FieldAsString( "pers_type" );
    pass.wl_type = Qry.FieldAsString( "wl_type" );
    pass.InUse = ( !pass.foundSeats.empty() );
    pass.isSeat = pass.InUse;
    if ( pass.foundSeats.empty() ) { // ???����室��� ����� �।��饥 ����
    	res = true;
  		string old_seat_no;
    		if ( pass.wl_type.empty() ) {
    		  old_seat_no = priorSeats.getSeats( pass.paxId, "seats" );
    		  if ( !old_seat_no.empty() )
    		  	old_seat_no = "(" + old_seat_no + ")";
    		}
    		else
    			old_seat_no = AstraLocale::getLocaleText("��");
    		if ( !old_seat_no.empty() )
    			pass.foundSeats = old_seat_no;
    }
    while ( !RemsQry.Eof && RemsQry.FieldAsInteger( "pax_id" ) <= pass.paxId ) {
    	if ( RemsQry.FieldAsInteger( "pax_id" ) == pass.paxId ) {
    		pass.add_rem( RemsQry.FieldAsString( "rem_code" ) );
    		pass.pass_rem += string( ".R/" ) + RemsQry.FieldAsString( "rem" ) + "   ";
    	}
      RemsQry.Next();
    }
    string pass_rem;
    if ( subcls_rems.IsSubClsRem( Qry.FieldAsString( "subclass" ), pass_rem ) )
      pass.add_rem( pass_rem );


    if (!Qry.FieldIsNULL("tckin_id"))
    {
      TCkinRouteItem priorSeg;
      tckin_route.GetPriorSeg(Qry.FieldAsInteger("tckin_id"),
                              Qry.FieldAsInteger("seg_no"),
                              crtIgnoreDependent,
                              priorSeg);
      if (priorSeg.grp_id!=NoExists)
      {
        TDateTime local_scd_out = UTCToClient(priorSeg.operFlt.scd_out,AirpTZRegion(priorSeg.operFlt.airp));

    	  ostringstream trip;
    	  trip << ElemIdToElemCtxt( ecDisp, etAirline, priorSeg.operFlt.airline, priorSeg.operFlt.airline_fmt )
    	       << setw(3) << setfill('0') << priorSeg.operFlt.flt_no
    	       << ElemIdToElemCtxt( ecDisp, etSuffix, priorSeg.operFlt.suffix, priorSeg.operFlt.suffix_fmt )
    	       << "/" << DateTimeToStr( local_scd_out, "dd" );

    	  pass.trip_from = trip.str();
      };
    };
      
    p.Add( pass );
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
    "     first_xname,last_xname,first_yname,last_yname,pax_id,time_create) "
    "  VALUES "
    "    (:range_id,:point_id,:point_dep,:point_arv,:layer_type, "
    "     :first_xname,:last_xname,:first_yname,:last_yname,:pax_id,system.UTCSYSDATE); "
    "  IF :pax_id IS NOT NULL THEN "
    "    UPDATE pax SET wl_type=NULL WHERE pax_id=:pax_id; "
    "  END IF; "
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

bool getNextSeat( int point_id, TSeatRange &r, int pr_down )
{
	TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT num, x, y, xname, yname FROM trip_comp_elems t, "
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
   	return true;
  }
  return false; //!!! ᠫ��� ����� � �� ����, � ࠧ����� ����
}

bool getCurrSeat( TSalons &ASalons, TSeatRange &r, TSalonPoint &p )
{
	TPoint pt;
	bool res=false;
	for( vector<TPlaceList*>::iterator placeList = ASalons.placelists.begin();placeList != ASalons.placelists.end(); placeList++ ) {
		if ( (*placeList)->GetisPlaceXY( string(r.first.row)+r.first.line, pt ) ) {
			p.num = (*placeList)->num;
			p.x = pt.x;
			p.y = pt.y;
			res=true;
			break;
		}
	}
	return res;
}

void ChangeLayer( TCompLayerType layer_type, int point_id, int pax_id, int &tid,
                  string first_xname, string first_yname, TSeatsType seat_type, bool pr_lat_seat )
{

  UseLayers[ cltProtCkin ] = false;
  UseLayers[ cltProtBeforePay ] = false;
  UseLayers[ cltPNLBeforePay ] = false;
  UseLayers[ cltProtAfterPay ] = false;
  UseLayers[ cltPNLAfterPay ] = false;
	//CanUse_PS = false; //!!!
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
  	case cltGoShow:
  	case cltTranzit:
  	case cltCheckin:
  	case cltTCheckin:
      Qry.SQLText =
       "SELECT surname, name, reg_no, pax.grp_id, pax.seats, a.step step, pax.tid, '' airp_arv, point_dep, point_arv, "
       "       0 point_id, salons.get_seat_no(pax.pax_id,pax.seats,NULL,:point_dep,'list',rownum) AS seat_no, "
       "       class, pers_type "
       " FROM pax, pax_grp, "
       "( SELECT COUNT(*) step FROM pax_rem "
       "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "
       "WHERE pax.pax_id=:pax_id AND "
       "      pax_grp.grp_id=pax.grp_id ";
       Qry.CreateVariable( "point_dep", otInteger, point_id );
      break;
    case cltProtCkin:
      Qry.SQLText =
        "SELECT surname, name, 0 reg_no, 0 grp_id, seats, a.step step, crs_pax.tid, airp_arv, point_id, 0 point_arv, "
        "       NULL AS seat_no, class, pers_type "
        " FROM crs_pax, crs_pnr, "
        "( SELECT COUNT(*) step FROM crs_pax_rem "
        "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "
        " WHERE crs_pax.pax_id=:pax_id AND crs_pax.pr_del=0 AND "
        "       crs_pax.pnr_id=crs_pnr.pnr_id";
      //Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
    	break;
    default:
    	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
    	throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
  }
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  // ���ᠦ�� �� ������ ��� ����������������� � ��㣮� �⮩�� ��� �� �।�. ��ᠤ�� ���ᠦ�� 㦥 ��ॣ����஢��
  if ( !Qry.RowCount() ) {
    ProgTrace( TRACE5, "!!! Passenger not found in funct ChangeLayer" );
    throw UserException( "MSG.PASSENGER.NOT_FOUND.REFRESH_DATA"	);
  }

  string strclass = Qry.FieldAsString( "class" );
  ProgTrace( TRACE5, "subclass=%s", strclass.c_str() );
  string fullname = Qry.FieldAsString( "surname" );
  TrimString( fullname );
  fullname += string(" ") + Qry.FieldAsString( "name" );
  int idx1 = Qry.FieldAsInteger( "reg_no" );
  int idx2 = Qry.FieldAsInteger( "grp_id" );
  string airp_arv = Qry.FieldAsString( "airp_arv" );
  int point_id_tlg = Qry.FieldAsInteger( "point_id" );
  int point_arv = Qry.FieldAsInteger( "point_arv" );
  int seats_count = Qry.FieldAsInteger( "seats" );
  int pr_down;
  if ( Qry.FieldAsInteger( "step" ) )
    pr_down = 1;
  else
    pr_down = 0;
  string prior_seat = Qry.FieldAsString( "seat_no" );
  string pers_type = Qry.FieldAsString( "pers_type" );
  if ( !seats_count ) {
    ProgTrace( TRACE5, "!!! Passenger has count seats=0 in funct ChangeLayer" );
    throw UserException( "MSG.SEATS.NOT_RESEATS_SEATS_ZERO" );
  }

  if ( Qry.FieldAsInteger( "tid" ) != tid  ) {
    ProgTrace( TRACE5, "!!! Passenger has changed in other term in funct ChangeLayer" );
    throw UserException( "MSG.PASSENGER.CHANGED_FROM_OTHER_DESK.REFRESH_DATA",
                         LParams()<<LParam("surname", fullname ) );
  }
  if ( ( layer_type != cltGoShow &&
  	     layer_type != cltCheckin &&
  	     layer_type != cltTCheckin &&
  	     layer_type != cltTranzit ) && SALONS2::Checkin( pax_id ) ) { //???
  	ProgTrace( TRACE5, "!!! Passenger set layer=%s, but his was chekin in funct ChangeLayer", EncodeCompLayerType( layer_type ) );
  	throw UserException( "MSG.PASSENGER.CHECKED.REFRESH_DATA" );
  }
  
  SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  Salons.Read();
  TSeatRange r;
  TSalonPoint p;
  TPlace* place;

  // �஢�ઠ �� �, �� ���ᠦ�� ����� 㦥 ����� �ਮ���� ᫮�, � �⨬ �������� ����� �ਮ����
  //�᫨ ���ᠦ�� ����� ����� ᫮�, � ����� ࠡ���� � �।���⥫쭮� ࠧ��⪮� ???
  Qry.Clear();
  Qry.SQLText =
    "SELECT trip_comp_layers.layer_type, first_xname, first_yname "
    " FROM trip_comp_layers, comp_layer_types "
    "WHERE point_id=:point_id AND (crs_pax_id=:pax_id OR pax_id=:pax_id) AND "
    "      trip_comp_layers.layer_type = comp_layer_types.code AND "
    "      comp_layer_types.priority<:priority "
    "ORDER BY priority";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.CreateVariable( "priority", otInteger, Salons.getPriority( layer_type ) );
  Qry.Execute();
  // �롨ࠥ� �� ����� �ਮ���� ᫮� �� ���ᠦ���
  while ( !Qry.Eof ) {
    strcpy( r.first.line, Qry.FieldAsString( "first_xname" ) );
    strcpy( r.first.row, Qry.FieldAsString( "first_yname" ) );
    r.second = r.first;
    // ��室�� ���� � ᫮ﬨ � ᠫ���
    if ( getCurrSeat( Salons, r, p ) ) {
      vector<TPlaceList*>::iterator placeList = Salons.placelists.end();
      for( placeList = Salons.placelists.begin();placeList != Salons.placelists.end(); placeList++ ) {
      	if ( (*placeList)->num == p.num )
      		break;
      }
      if ( placeList != Salons.placelists.end() ) {
      	SALONS2::TPoint coord( p.x, p.y );
      	place = (*placeList)->place( coord );
        ProgTrace( TRACE5, "pax_id=%d, seat_no=%s, layer_type=%s, priority=%d",
                   pax_id, string(place->yname+place->xname).c_str(), EncodeCompLayerType( layer_type ), Salons.getPriority( layer_type ) );
      	// �஢��塞 � �ࠢ�� �� ᫮� ������� ��� ��४��� ����� �ਮ���� ᫮��
      	if ( place->isLayer( DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ) ) )
          throw UserException( "MSG.SEATS.SEAT_NO.EXIT_MORE_PRIORITY" );
      }
    }
    Qry.Next();
  }

  vector<TSeatRange> seats;
  if ( seat_type != stDropseat ) { // ���������� ����� ���� + �஢�ઠ
  	TQuery QrySeatRules( &OraSession );
  	QrySeatRules.SQLText =
	    "SELECT pr_owner FROM comp_layer_rules "
	    "WHERE src_layer=:new_layer AND dest_layer=:old_layer";
  	QrySeatRules.CreateVariable( "new_layer", otString, EncodeCompLayerType( layer_type ) );
  	QrySeatRules.DeclareVariable( "old_layer", otString );
  // ���뢠�� ᫮� �� ������ ����� � ������ �஢��� �� �, �� ��� ᫮� 㦥 ����� ��㣨� ���ᠦ�஬
	  seats.clear();
    strcpy( r.first.line, first_xname.c_str() );
    strcpy( r.first.row, first_yname.c_str() );
    r.second = r.first;
    if ( !getCurrSeat( Salons, r, p ) )
    	throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    vector<TPlaceList*>::iterator placeList = Salons.placelists.end();
    for( placeList = Salons.placelists.begin();placeList != Salons.placelists.end(); placeList++ ) {
    	if ( (*placeList)->num == p.num )
    		break;
    }
    if ( placeList == Salons.placelists.end() )
    	throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    for ( int i=0; i<seats_count; i++ ) { // �஡�� �� ���-�� ���� � �� ���⠬
    	SALONS2::TPoint coord( p.x, p.y );
    	place = (*placeList)->place( coord );
    	if ( !place->visible || !place->isplace || place->clname != strclass )
    		throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
    	// �஢�ઠ �� �, �� ���ᠦ�� �� "��" � ���� � ���਩���� ��室�
      if ( pers_type != "��" ) {
      	vector<string> elem_types;
      	getValidChildElem_Types( elem_types );
      	if ( find( elem_types.begin(), elem_types.end(), place->elem_type ) == elem_types.end() ) {
      		throw UserException( "MSG.SEATS.SEAT_NO.NOT_AVAIL" );
      	}
      }
    	// �஢�ઠ �� �, �� �� ����� �ࠢ� �������� ᫮� �� �� ���� �� ���ᠦ���
    	if ( !place->layers.empty() ) {
    		if ( place->layers.begin()->pax_id == pax_id &&
    			   place->layers.begin()->layer_type == layer_type )
    			throw UserException( "MSG.SEATS.SEAT_NO.PASSENGER_OWNER" );
			  QrySeatRules.SetVariable( "old_layer", EncodeCompLayerType( place->layers.begin()->layer_type ) );
    	  ProgTrace( TRACE5, "old layer=%s", EncodeCompLayerType( place->layers.begin()->layer_type ) );
    	  QrySeatRules.Execute();
		    if ( QrySeatRules.Eof )
    			throw UserException( "MSG.SEATS.UNABLE_SET_CURRENT" );
    		if ( QrySeatRules.FieldAsInteger( "pr_owner" ) && pax_id != place->layers.begin()->pax_id )
    			throw UserException( "MSG.SEATS.SEAT_NO.OCCUPIED_OTHER_PASSENGER" );
    	}

    	strcpy( r.first.line, place->xname.c_str() );
     	strcpy( r.first.row, place->yname.c_str() );
   	  r.second = r.first;
      seats.push_back( r );
      if ( pr_down )
      	p.y++;
      else
      	p.x++;
    }

/*???	  if ( Qry.Eof ) {
	  	ProgError( STDLOG, "CanChangeLayer: error xname=%s, yname=%s", first_xname.c_str(), first_yname.c_str() );
	  	throw UserException( "MSG.SEATS.SEAT_NO.SEATS_NOT_AVAIL" );
	  } */
  }

  int curr_tid = NoExists;
  TPointIdsForCheck point_ids_spp;
  point_ids_spp.insert( make_pair(point_id, layer_type) );
  if ( seat_type != stSeat ) { // ���ᠤ��, ��ᠤ�� - 㤠����� ��ண� ᫮�
    	switch( layer_type ) {
    		case cltGoShow:
      	case cltTranzit:
      	case cltCheckin:
  	    case cltTCheckin:
    	    Qry.Clear();
    	    Qry.SQLText =
            "BEGIN "
            " DELETE FROM trip_comp_layers "
            "  WHERE point_id=:point_id AND "
            "        layer_type=:layer_type AND "
            "        pax_id=:pax_id; "
            " IF :tid IS NULL THEN "
            "   SELECT cycle_tid__seq.nextval INTO :tid FROM dual; "
            "   UPDATE pax SET tid=:tid WHERE pax_id=:pax_id;"
            "   mvd.sync_pax(:pax_id,:term); "
            " END IF;"
            "END;";
          Qry.CreateVariable( "point_id", otInteger, point_id );
          Qry.CreateVariable( "pax_id", otInteger, pax_id );
          Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
          if ( curr_tid == NoExists )
            Qry.CreateVariable( "tid", otInteger, FNull );
          else
            Qry.CreateVariable( "tid", otInteger, curr_tid );
          Qry.CreateVariable( "term", otString, TReqInfo::Instance()->desk.code );
          Qry.Execute();
          curr_tid = Qry.GetVariableAsInteger( "tid" );
        	break;
    		case cltProtCkin:
    			// 㤠����� �� ᠫ���, �᫨ ���� ࠧ��⪠
    			DeleteTlgSeatRanges( layer_type, pax_id, curr_tid, point_ids_spp );
          break;
        default:
        	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
        	throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
    }
  }
  // �����祭�� ������ ᫮�
  if ( seat_type != stDropseat ) { // ��ᠤ�� �� ����� ����
  	switch ( layer_type ) {
  		case cltGoShow:
    	case cltTranzit:
    	case cltCheckin:
    	case cltTCheckin:
  		  SaveTripSeatRanges( point_id, layer_type, seats, pax_id, point_id, point_arv );
  		  Qry.Clear();
  		  Qry.SQLText =
          "BEGIN "
          " IF :tid IS NULL THEN "
          "   SELECT cycle_tid__seq.nextval INTO :tid FROM dual; "
          "   UPDATE pax SET tid=:tid WHERE pax_id=:pax_id;"
          "   mvd.sync_pax(:pax_id,:term); "
          " END IF;"
          "END;";
        Qry.CreateVariable( "term", otString, TReqInfo::Instance()->desk.code );
        Qry.CreateVariable( "pax_id", otInteger, pax_id );
        if ( curr_tid == NoExists )
          Qry.CreateVariable( "tid", otInteger, FNull );
        else
          Qry.CreateVariable( "tid", otInteger, curr_tid );
        Qry.CreateVariable( "term", otString, TReqInfo::Instance()->desk.code );
        Qry.Execute();
        curr_tid = Qry.GetVariableAsInteger( "tid" );
        break;
      case cltProtCkin:
        InsertTlgSeatRanges( point_id_tlg, airp_arv, layer_type, seats, pax_id, NoExists, NoExists, false, curr_tid, point_ids_spp );
      	break;
      default:
      	ProgTrace( TRACE5, "!!! Unuseable layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
      	throw UserException( "MSG.SEATS.SET_LAYER_NOT_AVAIL" );
    }
  }
  
  tid = curr_tid;

  TReqInfo *reqinfo = TReqInfo::Instance();
  string new_seat_no;
  for (vector<TSeatRange>::iterator ns=seats.begin(); ns!=seats.end(); ns++ ) {
  	if ( !new_seat_no.empty() )
  		new_seat_no += " ";
    new_seat_no += denorm_iata_row( ns->first.row, NULL ) + denorm_iata_line( ns->first.line, pr_lat_seat );
  }
  switch( seat_type ) {
  	case stSeat:
  		switch( layer_type ) {
  			case cltGoShow:
  	    case cltTranzit:
  	    case cltCheckin:
  	    case cltTCheckin:
          reqinfo->MsgToLog( string( "���ᠦ�� " ) + fullname +
                             " ��ᠦ�� �� ����: " +
                             new_seat_no,
                             evtPax, point_id, idx1, idx2 );
          if ( is_sync_aodb( point_id ) )
            update_aodb_pax_change( point_id, pax_id, idx1, "�" );
          break;
        default:;
  		}
  		break;
    case stReseat:
    	switch( layer_type ) {
    		case cltGoShow:
       	case cltTranzit:
  	    case cltCheckin:
  	    case cltTCheckin:
          reqinfo->MsgToLog( string( "���ᠦ�� " ) + fullname +
                             " ���ᠦ��. ����� ����: " +
                             new_seat_no,
                             evtPax, point_id, idx1, idx2 );
          if ( is_sync_aodb( point_id ) )
            update_aodb_pax_change( point_id, pax_id, idx1, "�" );
          break;
        default:;
  		}
  		break;
  	case stDropseat:
    	switch( layer_type ) {
    		case cltGoShow:
      	case cltTranzit:
      	case cltCheckin:
  	    case cltTCheckin:
          reqinfo->MsgToLog( string( "���ᠦ�� " ) + fullname +
                             " ��ᠦ��. ����: " + prior_seat,
                             evtPax, point_id, idx1, idx2 );
          if ( is_sync_aodb( point_id ) )
            update_aodb_pax_change( point_id, pax_id, idx1, "�" );
          break;
        default:;
  		}
  		break;
  }
  check_layer_change( point_ids_spp );
}

void AutoReSeatsPassengers( SALONS2::TSalons &Salons, TPassengers &APass, TSeatAlgoParams ASeatAlgoParams )
{
	// ᠫ�� ᮤ�ন� �� ��ଠ��� ���� (��� ���������� ����, ���ਬ�� � ࠧ�뢠��
  if ( Salons.placelists.empty() )
    throw EXCEPTIONS::Exception( "�� ����� ᠫ�� ��� ��⮬���᪮� ��ᠤ��" );
  FSeatAlgoParams = ASeatAlgoParams;
  CurrSalon = &Salons;
  SeatAlg = sSeatPassengers;
  CanUseLayers = false; /* �� ���뢠�� ����� ���� */
  UseLayers[ cltProtCkin ] = true;
  UseLayers[ cltProtBeforePay ] = true;
  UseLayers[ cltProtAfterPay ] = true;
  UseLayers[ cltPNLBeforePay ] = true;
  UseLayers[ cltPNLAfterPay ] = true;
  //CanUse_PS = true;
  CanUseSmoke = false;
  PlaceElem_Types.clear();
  CanUseRems = sIgnoreUse;
  Remarks.clear();
  CanUseTube = true;
  CanUseAlone = uTrue;
  SeatPlaces.Clear();

  //!!! �� ������ pass.placeName, pass.PrevPlaceName, pass.OldPlaceName, pass.placeList,pass.InUse ,x,y???
	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT layer_type FROM grp_status_types ORDER BY priority ";
	Qry.Execute();

  int s;
  try {
    while ( !Qry.Eof ) { // �஡�� �� ᫮�
      for ( int vClass=0; vClass<=2; vClass++ ) { // �஡�� �� ����ᠬ
        Passengers.Clear();
          s = APass.getCount();
        for ( int i=0; i<s; i++ ) { // �஡�� �� ���ᠦ�ࠬ
          TPassenger &pass = APass.Get( i );
          if ( pass.isSeat ) // ���ᠦ�� ��ᠦ��
            continue;
          if ( pass.grp_status != DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ) ) // ࠧ������ ���ᠦ�஢ �� ⨯�� �஭�, �࠭���...
          	continue;
          ProgTrace( TRACE5, "isSeat=%d, pass.pax_id=%d, pass.foundSeats=%s, pass.clname=%s, layer_type=%s, pass.grp_status=%s, equal_y_class=%d",
                     pass.isSeat, pass.paxId, pass.foundSeats.c_str(), pass.clname.c_str(), Qry.FieldAsString( "layer_type" ),
                     EncodeCompLayerType( pass.grp_status ), pass.clname == "�" );

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
          tst();
          if ( ExistsBasePlace( Salons, pass ) ) { // ���ᠦ�� �� ��ᠦ��, �� ��諮�� ��� ���� ������� ���� - ����⨫� ��� ����� //??? ����஢�� !!!
          	tst();
            continue;
          }
          Passengers.Add( pass ); /* ������������� � � ������ �� ��諮�� ������� ���� */
        } /* �஡������� �� �ᥬ ���ᠦ�ࠬ */
        if ( Passengers.getCount() ) {
          Passengers.KWindow = ( Passengers.KWindow && !Passengers.KTube );
          Passengers.KTube = ( !Passengers.KWindow && Passengers.KTube );
          seatCoords.refreshCoords( FSeatAlgoParams.SeatAlgoType, Passengers.KWindow, Passengers.KTube );
          /* ��ᠤ�� ���ᠦ�� � ���ண� �� ������� ������� ���� */
          ProgTrace( TRACE5, "AutoReSeatsPassengers: Passengers.getCount()=%d, layer_type=%s", Passengers.getCount(),Qry.FieldAsString( "layer_type" ) );
          SeatPlaces.grp_status = Passengers.Get( 0 ).grp_status;
          SeatPlaces.SeatsPassengers( true );
          SeatPlaces.RollBack( );
          int s = Passengers.getCount();
          for ( int i=0; i<s; i++ ) {
            TPassenger &pass = Passengers.Get( i );
            int k = APass.getCount();
            for ( int j=0; j<k; j++	 ) {
              TPassenger &opass = APass.Get( j );
              if ( opass.paxId == pass.paxId ) {
              	opass = pass;
              	ProgTrace( TRACE5, "pass.pax_id=%d, pass foundSeats=%s, pass.isSeat=%d, pass.InUse=%d", pass.paxId,pass.foundSeats.c_str(), pass.isSeat, pass.InUse );
              	break;
              }
            }
          }
        }
      } // ����� �஡��� �� ����ᠬ
      Qry.Next();
    } // ����� �஡��� �� ᫮�
    TQuery QryPax( &OraSession );
    TQuery QryLayer( &OraSession );
    TQuery QryUpd( &OraSession );
    QryPax.SQLText =
      "SELECT point_dep, point_arv, "
      "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,point_dep,'list',rownum) AS seat_no "
      " FROM pax, pax_grp "
      " WHERE pax.pax_id=:pax_id AND "
      "       pax.grp_id=pax_grp.grp_id ";
    QryPax.DeclareVariable( "pax_id", otInteger );
    QryLayer.SQLText =
      "DELETE FROM trip_comp_layers "
      " WHERE pax_id=:pax_id ";
    QryLayer.DeclareVariable( "pax_id", otInteger );
    QryUpd.SQLText =
      "BEGIN "
      " UPDATE pax SET tid=cycle_tid__seq.nextval WHERE pax_id=:pax_id;"
      " mvd.sync_pax(:pax_id,:term); "
      "END;";
    QryUpd.DeclareVariable( "pax_id", otInteger );
    QryUpd.DeclareVariable( "term", otString );

    Passengers.Clear();

    s = APass.getCount();
    for ( int i=0; i<s; i++ ) {
    	TPassenger &pass = APass.Get( i );
    	ProgTrace( TRACE5, "pass.pax_id=%d, pass.isSeat=%d", pass.paxId, pass.isSeat );
    	Passengers.Add( pass );
    	if ( pass.isSeat )
    		continue;
     	QryPax.SetVariable( "pax_id", pass.paxId );
      QryPax.Execute();
      if ( QryPax.Eof )
      	throw UserException( "MSG.SEATS.SEATS_DIRECTION_NOT_SET" );
      int point_dep = QryPax.FieldAsInteger( "point_dep" );
      int point_arv = QryPax.FieldAsInteger( "point_arv" );
      string prev_seat_no = QryPax.FieldAsString( "seat_no" );
      ProgTrace( TRACE5, "pax_id=%d, prev_seat_no=%s,pass.InUse=%d", pass.paxId, prev_seat_no.c_str(), pass.InUse );

    	if ( !pass.InUse ) { /* �� ᬮ��� ��ᠤ��� */
    		if ( !pass.foundSeats.empty() ) {
          TReqInfo::Instance()->MsgToLog( string("���ᠦ�� " ) + pass.fullName +
                                          " ��-�� ᬥ�� ���������� ��ᠦ�� � ���� " +
                                          prev_seat_no, evtPax, Salons.trip_id, pass.regNo, pass.grpId );
          if ( is_sync_aodb( Salons.trip_id ) )
            update_aodb_pax_change( Salons.trip_id, pass.paxId, pass.regNo, "�" );
        }
      }
      else {
      	std::vector<TSeatRange> seats;
      	for ( vector<TSeat>::iterator i=pass.seat_no.begin(); i!=pass.seat_no.end(); i++ ) {
      		TSeatRange r(*i,*i);
      		seats.push_back( r );
      	}
        // ����室��� ���砫� 㤠���� �� ��� ��������� ���� �� ᫮� ॣ����樨
        QryLayer.SetVariable( "pax_id", pass.paxId );
        QryLayer.Execute();
  		  SaveTripSeatRanges( Salons.trip_id, pass.grp_status, seats, pass.paxId, point_dep, point_arv ); //???
  		  QryUpd.SetVariable( "pax_id", pass.paxId );
        QryUpd.SetVariable( "term", TReqInfo::Instance()->desk.code );
        QryUpd.Execute();
        QryPax.Execute();
        string new_seat_no = QryPax.FieldAsString( "seat_no" );
        ProgTrace( TRACE5, "oldplace=%s, newplace=%s", prev_seat_no.c_str(), new_seat_no.c_str() );
      	if ( prev_seat_no != new_seat_no ){ /* ���ᠤ��� �� ��㣮� ���� */
          TReqInfo::Instance()->MsgToLog( string( "���ᠦ�� " ) + pass.fullName +
                                          " ��-�� ᬥ�� ���������� ���ᠦ�� �� ���� " +
                                          new_seat_no, evtPax, Salons.trip_id, pass.regNo, pass.grpId );
          if ( is_sync_aodb( Salons.trip_id ) )
            update_aodb_pax_change( Salons.trip_id, pass.paxId, pass.regNo, "�" );
        }
      }
    }
  }
  catch( ... ) {
    SeatPlaces.RollBack( );
    throw;
  }
  SeatPlaces.RollBack( );
  check_waitlist_alarm( Salons.trip_id );
  ProgTrace( TRACE5, "passengers.count=%d", APass.getCount() );
}

TSeatAlgoParams GetSeatAlgo(TQuery &Qry, string airline, int flt_no, string airp_dep)
{
  Qry.Clear();
  Qry.SQLText=
    "SELECT algo_type, pr_seat_in_row,"
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
  TSeatAlgoParams res;
  if (!Qry.Eof)
  {
    res.SeatAlgoType=(TSeatAlgoTypes)Qry.FieldAsInteger("algo_type");
    res.pr_canUseOneRow = Qry.FieldAsInteger("pr_seat_in_row");
  };
  Qry.Close();
  return res;
};

bool CompGrp( TPassenger item1, TPassenger item2 )
{
  TBaseTable &classes=base_tables.get("classes");
  const TBaseTableRow &row1=classes.get_row("code",item1.clname);
  const TBaseTableRow &row2=classes.get_row("code",item2.clname);
  if ( row1.AsInteger( "priority" ) < row2.AsInteger( "priority" ) )
  	return true;
  else
    if ( row1.AsInteger( "priority" ) > row2.AsInteger( "priority" ) )
    	return false;
    else
	    if ( item1.grpId < item2.grpId )
	      return true;
	    else
		    if ( item1.grpId > item2.grpId )
			    return false;
		    else
			    if ( item1.paxId < item2.paxId )
				    return true;
			    else
				    if ( item1.paxId > item2.paxId )
					    return false;
				    else
					    return true;
};


void TPassengers::Build( xmlNodePtr dataNode )
{
  if ( !getCount() )
    return;
  for (VPassengers::iterator p=FPassengers.begin(); p!=FPassengers.end(); p++ ) {
  	p->InUse = false;
  }

  xmlNodePtr passNode = NewTextChild( dataNode, "passengers" );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT code,layer_type,name FROM grp_status_types ORDER BY priority";
  Qry.Execute();

  TDefaults def;
  bool createDefaults=false;
  vector<TPassenger> ps;
  while ( !Qry.Eof ) {
    for (VPassengers::iterator p=FPassengers.begin(); p!=FPassengers.end(); p++ ) {
    	if ( p->InUse || p->grp_status != DecodeCompLayerType(Qry.FieldAsString( "layer_type" )) )
    		continue;
    	ps.push_back( *p );
    }
    // ���஢�� �� grp_status + ����� + ��㯯�
    sort(ps.begin(),ps.end(),CompGrp);
    ProgTrace( TRACE5, "ps.size()=%d, layer_type=%s", ps.size(), Qry.FieldAsString( "layer_type" ) );
    if ( !ps.empty() ) {
      createDefaults=true;
    	xmlNodePtr pNode = NewTextChild( passNode, "layer_type", Qry.FieldAsString( "layer_type" ) );
    	SetProp( pNode, "name", Qry.FieldAsString( "name" ) );
    	for ( vector<TPassenger>::iterator ip=ps.begin(); ip!=ps.end(); ip++ ) {
    		ip->build( NewTextChild( pNode, "pass" ), def );
    		ip->InUse = true;
    	}
    	ps.clear();
    }
  	Qry.Next();
  }
  if (createDefaults)
  {
    xmlNodePtr defNode = NewTextChild( dataNode, "defaults" );
    NewTextChild( defNode, "clname", def.clname );
    NewTextChild( defNode, "grp_layer_type", EncodeCompLayerType(def.grp_status) );
    NewTextChild( defNode, "pers_type", ElemIdToCodeNative(etPersType, def.pers_type) );
    NewTextChild( defNode, "seat_no", def.placeName );
    NewTextChild( defNode, "wl_type", def.wl_type );
    NewTextChild( defNode, "seats", def.countPlace );
    NewTextChild( defNode, "isseat", (int)def.isSeat );
    NewTextChild( defNode, "ticket_no", def.ticket_no );
    NewTextChild( defNode, "document", def.document );
    NewTextChild( defNode, "bag_weight", def.bag_weight );
    NewTextChild( defNode, "bag_amount", def.bag_amount );
    NewTextChild( defNode, "excess", def.excess );
    NewTextChild( defNode, "trip_from", def.trip_from );
    NewTextChild( defNode, "comp_rem", def.comp_rem );
    NewTextChild( defNode, "pr_down", (int)def.pr_down );
    NewTextChild( defNode, "pass_rem", def.pass_rem );
  };
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

} // end namespace SEATS2


//seats with:SeatAlg=2,FCanUseRems=sIgnoreUse,FCanUseAlone=0,KeyStatus=1,FCanUseTube=0,FCanUseSmoke=0,PlaceStatus=, MAXPLACE=3,canUseOneRow=0, CanUseSUBCLS=1, SUBCLS_REM=SUBCLS
//seats with:SeatAlg=2,FCanUseRems=sNotUse_NotUseDenial,FCanUseAlone=1,KeyStatus=1,FCanUseTube=0,FCanUseS



