#include <stdlib.h>
#include "seats.h"
#include "basic.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "salons.h"
#include "tlg/tlg_parser.h"
#include "convert.h"
#include "images.h"
#include "serverlib/str_utils.h"
#include "tripinfo.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;
using namespace SALONS2;


namespace SEATS2
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

typedef vector<int> TLine;

struct TLinesSalon {
  TLine lines;
  SALONS2::TPlaceList *placeList;
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
enum TUseRem { sAllUse, sMaxUse, sOnlyUse, sNotUse_NotUseDenial, sNotUse, sNotUseDenial, sIgnoreUse, useremLength };
/*
 aAllUse - совпадение всех ремарок по самому приоритетному пассажиру
 sMaxUse - совпадение самой приоритетной ремарки
 sOnlyUse - хотя бы одна совпадает
 sNotUse - запрещено использовать место с разрешенной ремаркой, если у пассажира такой ремарки нет
 sNotUseDenial - запрещено использовать место с запрещенной ремаркой, кот есть у пассажира
 sIgnoreUse - игнорировать ремарки
*/
/* Нельзя разбивать трех, нельзя сажать по одному более одного раза, все можно */
enum TUseAlone { uFalse3 /* нельзя оставлять одного при рассадке группы*/,
	               uFalse1 /* можно оставлять одного только один раз при рассадке группы*/,
	               uTrue /*можно оставлять одного при рассадке группы любое кол-во раз*/ };


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

/* глобальные переменные для этого модуля */
TSeatPlaces SeatPlaces;
TLines lines;

SALONS2::TSalons *CurrSalon;

bool CanUseLayers; /* поиск по слою места */
TCompLayerType PlaceLayer; /* сам слой */
bool CanUse_PS; /* можно ли использовать статус предвю рассадки для пассажиров с другими статусами */
bool CanUseSmoke; /* поиск курящих мест */
bool CanUseElem_Type; /* поиск мест по типу (табуретка) */
string PlaceElem_Type; /* сам тип места */
TUseRem CanUseRems; /* поиск по ремарке */
vector<string> Remarks; /* сама ремарка */
bool CanUseTube; /* поиск через проходы */
TUseAlone CanUseAlone; /* можно ли использовать посадку одного в ряду - может посадить
                          группу друг за другом */
TSeatAlg SeatAlg;
bool FindMCLS=false;
bool canUseMCLS=false;

bool canUseOneRow=true; // использовать при рассадке только один ряд

int FSeatAlgo=0; // алгоритм рассадки

int MAXPLACE() {
	return (canUseOneRow)?1000:CONST_MAXPLACE; // при рассадке в один ряд кол-во мест в ряду неограничено иначе 3 места
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

/* откат части найденных мест для рассадки */
void TSeatPlaces::RollBack( int Begin, int End )
{
  if ( seatplaces.empty() )
    return;
  /* пробег по измененным найденным местам */
  TSeatPlace seatPlace;
  for ( int i=Begin; i<=End; i++ ) {
    seatPlace = seatplaces[ i ];
    /* пробег по старым сохраненным местам */
    for ( vector<SALONS2::TPlace>::iterator place=seatPlace.oldPlaces.begin();
          place!=seatPlace.oldPlaces.end(); place++ ) {
      /* получение места */
      int idx = seatPlace.placeList->GetPlaceIndex( seatPlace.Pos );
      SALONS2::TPlace *pl = seatPlace.placeList->place( idx );
      /* отмена всех изменений места */
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
      default: throw Exception( "Ошибка рассадки" );
    }
    seatPlace.oldPlaces.clear();
  } /* end for */
  vector<TSeatPlace>::iterator b = seatplaces.begin();
  seatplaces.erase( b + Begin, b + End + 1 );
}

/* откат всех найденных мест для рассадки */
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

/* Заносим найденные места.
  FP - указатель на исходное место от которого начинается поиск
  EP - указатель на первое найденное место
  FoundCount - кол-во найденных мест
  Step - направление отсчета найденных мест
  Возвращаем кол-во использованных мест */
int TSeatPlaces::Put_Find_Places( SALONS2::TPoint FP, SALONS2::TPoint EP, int foundCount, TSeatStep Step )
{
  int p_RCount, p_RCount2, p_RCount3; /* необходимое кол-во 3-х, 2-х, 1-х мест */
  int pp_Count, pp_Count2, pp_Count3; /* имеющееся кол-во 3-х, 2-х, 1-х мест */
  int NTrunc_Count, Trunc_Count; /* кол-во выделенных из общего числа данных мест */
  int p_Prior, p_Next; /* Кол-во мест до FP и после него */
  int p_Step; /* направление рассадки. Определяется в зависимости от кол-ва p_Prior и p_Next */
  int Need;
  SALONS2::TPlaceList *placeList;
  int Result = 0; /* общее кол-во задействованных мест */
  if ( foundCount == 0 )
   return Result; // не задано мест
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
           p_Prior = abs( FP.y - EP.y ); // разница м/у FP и EP
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
      case 1: if ( p_RCount > 0 ) /* нужны 1-х местные места ?*/
                Trunc_Count = 1;
              else
                Trunc_Count = 0;
              break;
      case 2: if ( p_RCount2 > 0 ) /* нужны 2-х местные места ?*/
                Trunc_Count = 2;
              else
                Trunc_Count = 1;
              break;
      case 3: if ( p_RCount3 > 0 ) /* нужны 3-х местные места ?*/
                Trunc_Count = 3; /* нужны 3-х местные места */
              else
                Trunc_Count = 2; /* нужны 2-х местные места ? */
              break;
      default: Trunc_Count = 3;
    }
    if ( !Trunc_Count ) /* больше ничего не нужно из того, что предлагается */
      break;
    if ( Trunc_Count != NTrunc_Count ) {
      NTrunc_Count = Trunc_Count; /* выделили меньшую часть, а то такая большая не нужна */
      continue;
    }
   /* если мы здесь, то тогда мы имеем места, которые нас устраивают по кол-ву
      теперь перед нами стоит вопрос, с какого места начать посадку, если мест больше,
      чем мы сейчас выделии?
      Ответ: для того, чтобы наиболее быть близким к исходной точке FP
      начнем с такого края, у которого меньше мест до FP. */
    if ( !p_Step ) { /* признак того, что мы пока не определили с какого края надо начинать */
      Need = p_RCount; /* мы нуждаемся в таком кол-ве 1-х мест */
      switch( Trunc_Count ) {
        case 3: Need += p_RCount3*3 + p_RCount2*2; /* если надо 3, то учитываем все */
                break;
        case 2: Need += p_RCount2*2; /* если надо 2 => то не надо 3-х или общее кол-во мест не позволяет, */
                break;               /* тогда надо учитывать только 1-х и 2-х места */
      }


      /* Need - это общее нужное кол-во мест, которое возможно удасться выделить
         теперь определяем направление */
      if ( p_Prior >= p_Next && Need > p_Next ) { /* слева больше мест, начнем справа */
        p_Step = -1;
        if ( p_Next < Need ) /* направо/вниз больше мест, чем возможно понадобится */
          Need = p_Next; /* тогда больше мы не можем потребовать! */
        switch( (int)Step ) {
          case sRight:
          case sLeft:
             EP.x = FP.x + Need - 1; /* перемещаемся на исходную позицию */
             Step = sLeft; /* начинаем двигаться влево */
             break;
          case sDown:
          case sUp:
             EP.y = FP.y + Need - 1; /* перемещаемся на исходную позицию */
             Step = sUp; /* начинаем двигаться вверх */
             break;
        }
      }
      else {
       p_Step = 1;  /* начнем слева */
       if ( Need <= p_Next )
         Need = 0; /* ничего не трогаем и начинаем с текущей позиции */
       else
        if ( p_Prior < Need )
          Need = p_Prior;
        switch( (int)Step ) {
       	  case sRight:
       	  case sLeft:
       	     EP.x = FP.x - Need; /* перемещаемся на исходную позицию */
             Step = sRight; /* начинаем двигаться вправо */
             break;
          case sDown:
          case sUp:
             EP.y = FP.y - Need; /* перемещаемся на исходную позицию */
             Step = sDown; /* начинаем двигаться вниз */
             break;
        }
      } /* конец определения направления ( p_Prior >= p_Next ... ) */
    } /* конец p_Step = 0 */

    /* закончили определять исходную позицию места и направления рассад */

    /* выделяем еще одно место под набор мест */
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
    /* сохраняем старые места и помечаем места в салоне как занятые */
    for ( int i=0; i<Trunc_Count; i++ ) {
      SALONS2::TPlace place;
      SALONS2::TPlace *pl = placeList->place( EP );
//      ProgTrace( TRACE5, "placename=%s", string(pl->yname+pl->xname).c_str() );
      place.Assign( *pl );
      if ( !CurrSalon->placeIsFree( &place ) || !place.isplace )
        throw Exception( "Рассадка выполнила недопустимую операцию: использование уже занятого места" );
      seatplace.oldPlaces.push_back( place );
      pl->AddLayerToPlace( grp_status, 0, 0, CurrSalon->getPriority( grp_status ) );
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
    } /* закончили сохранять */
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
    Result += Trunc_Count; /* кол-во уже использованных мест */
    foundCount -= Trunc_Count; /* общее кол-во не использованных мест */
    NTrunc_Count = foundCount; /* кол-во мест, которые сейчас начнут разбираться */
//    ProgTrace( TRACE5, "Result=%d", Result );
  } /* end while */
  return Result;
}

/* ф-ция для определения возможности рассадки для мест у которых есть запрещенные ремарки */
bool DoNotRemsSeats( const vector<SALONS2::TRem> &rems )
{
	bool res = false; // признак того, что у пассажира есть ремарка, которая запрещена на выбранном месте
	bool pr_passmcls = find( Remarks.begin(), Remarks.end(), string("MCLS") ) != Remarks.end(); // признак у пассажира MCLS
	bool no_mcls = false;
	for( vector<string>::iterator nrem=Remarks.begin(); nrem!=Remarks.end(); nrem++ ) {
	  for( vector<SALONS2::TRem>::const_iterator prem=rems.begin(); prem!=rems.end(); prem++ ) {
		  if ( !prem->pr_denial )
			  continue;
			if ( *nrem == prem->rem && !res )
				res = true;
	  }
	}
  for( vector<SALONS2::TRem>::const_iterator prem=rems.begin(); prem!=rems.end(); prem++ ) {
//		ProgTrace( TRACE5, "prem->rem=%s", prem->rem.c_str() );
  	if ( !prem->pr_denial && prem->rem == "MCLS" && !pr_passmcls )
	  		no_mcls = true;
	}
//	ProgTrace( TRACE5, "res=%d, mcls=%d", res, no_mcls );
  return res || no_mcls; // если есть запрещенная ремарка у пассажира или место с ремаркой MCLS, а у пассажира ее нет
}

/* поиск мест расположенных рядом последовательно
   возвращает кол-во найденных мест.
   FP - адрес места, с которого надо искать
   FoundCount - кол-во уже найденных мест
   Step - направление поиска
  Глобальные переменные:
   CanUselayer, Placelayer - поиск строго по статусу мест,
   CanUseSmoke - поиск курящих мест,
   CanUseElem_Type, PlaceElem_Type - поиск строго по типу места (табуретка),
   CanUseRem, PlaceRem - поиск строго по ремарке места */
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
  while ( CurrSalon->placeIsFree( place ) && place->isplace && place->visible &&
          place->clname == Passengers.clname &&
          Result + foundCount < MAXPLACE() &&
          ( !CanUseLayers || place->isLayer( PlaceLayer ) || PlaceLayer == cltUnknown && place->layers.empty() ) &&
          ( !CanUseSmoke || place->isLayer( cltSmoke ) ) &&
          ( !CanUseElem_Type || place->elem_type == PlaceElem_Type ) ) {
    if ( canUseMCLS ) {
      for ( prem = place->rems.begin(); prem != place->rems.end(); prem++ ) {
        if ( !prem->pr_denial && prem->rem == "MCLS" )
        	break;
      }
      if ( FindMCLS && prem == place->rems.end() ||
      	   !FindMCLS && prem != place->rems.end() ) //  не смогли посадить на класс MCLS
     	  break;
    }
    switch( (int)CanUseRems ) {
      case sOnlyUse:
         if ( place->rems.empty() || Remarks.empty() ) // найдем хотя бы одну совпадающую
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
      	if ( DoNotRemsSeats( place->rems ) )
      		return Result;
      	if ( sNotUseDenial ) break;
      case sNotUse:
//      	 ProgTrace( TRACE5, "sNotUse: Result=%d, FP.x=%d, FP.y=%d", Result, FP.x, FP.y );
  	     for( vector<SALONS2::TRem>::const_iterator prem=place->rems.begin(); prem!=place->rems.end(); prem++ ) {
  //	     	 ProgTrace( TRACE5, "sNotUse: Result=%d, FP.x=%d, FP.y=%d, rem=%s", Result, FP.x, FP.y, prem->rem.c_str() );
		       if ( !prem->pr_denial ) {
		//       	 tst();
		       	 return Result;
		       }
  	     }
         break;
    } /* end switch */
    Result++; /* нашли еще одно место */
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

/* Рассадка группы в PlaceList, начиная с позиции FP,
   в направлении Step ( рассматриваются только sRight - горизонтальная ,
   и sDown - вертикальная рассадка => рассматриваем только пассажиров опр. группы.
   Приоритеты поиска sRight: sRight, sLeft, sTubeRight (через проход), sTubeLeft ).
   Wanted - особый случай, когда надо посадить 2x2, а не 3 и 1.
  Глобальные переменные:
   CanUseTube - поиск при Step = sRight через проходы
   Alone - посадить одно пассажира в ряду можно только один раз */
bool TSeatPlaces::SeatSubGrp_On( SALONS2::TPoint FP, TSeatStep Step, int Wanted )
{
  if ( Step == sLeft )
    Step = sRight;
  if ( Step == sUp )
    Step = sDown;
//  ProgTrace( TRACE5, "SeatSubGrp_On FP=(%d,%d), Step=%d, Wanted=%d", FP.x, FP.y, Step, Wanted );
  int foundCount = 0; // кол-во найденных мест всего
  int foundBefore = 0; // кол-во найденных мест до и после FP
  int foundTubeBefore = 0;
  int foundTubeAfter = 0;
  SALONS2::TPlaceList *placeList = CurrSalon->CurrPlaceList();
  if ( !Wanted && seatplaces.empty() && CanUseAlone != uFalse3 ) /*нельзя оставлять одного*/
    Alone = true;// это первый заход сюда, надо проинициализировать гл. переменную Alone
  int foundAfter = FindPlaces_From( FP, 0, Step );
  if ( !foundAfter )
    return false;
//  ProgTrace( TRACE5, "FP=(%d,%d), foundafter=%d", FP.x, FP.y, foundAfter );
  foundCount += foundAfter;
  if ( foundAfter && Wanted ) { // если мы нашли места и нам надо Wanted
    if ( foundAfter > Wanted )
      foundAfter = Wanted;
    Wanted -= Put_Find_Places( FP, FP, foundAfter, Step );
    if ( Wanted <= 0 ) {
      return true; // Ура все нашлось
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
  if ( foundBefore && Wanted ) { // если мы нашли места и нам надо Wanted
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
      return true; /* Ура все нашлось */
    }
  }
 /* далее попытаемся поискать через проход, при условии что поиск по горизонтали */
  if ( CanUseTube && Step == sRight && foundCount < MAXPLACE() ) {
    EP.x = FP.x + foundAfter + 1; //???/* устанавливаемся на предполагаемое место */
    if ( placeList->ValidPlace( EP ) ) {
      SALONS2::TPoint p( EP.x - 1, EP.y );
      SALONS2::TPlace *place = placeList->place( p ); /* берем пред. место */
      if ( !place->visible ) {
        foundTubeAfter = FindPlaces_From( EP, foundCount, Step ); /* поиск после прохода */
        foundCount += foundTubeAfter; /* увеличиваем общее кол-во мест */
//       ProgTrace( TRACE5, "FP=(%d,%d), foundTubeAfter=%d", EP.x, EP.y, foundTubeAfter );
        if ( foundTubeAfter && Wanted ) { /* если мы нашли места и нам надо Wanted */
          if ( foundTubeAfter > Wanted )
            foundTubeAfter = Wanted;
           Wanted -= Put_Find_Places( EP, EP, foundTubeAfter, Step ); /* первый параметр EP */
         /* т.к. точка отсчета должна находится на первом месте после прохода,
            иначе работать не будет */
           if ( Wanted <= 0 ) {
             return true; /* Ура все нашлось */
           }
        }
      }
    }
    /* далее поиск налево через проход */
    if ( foundCount < MAXPLACE() ) {
      EP.x = FP.x - foundBefore - 2; // устанавливаемся на предполагаемое место
      SALONS2::TPoint VP = EP;
//      ProgTrace( TRACE5, "EP=(%d,%d)", EP.x, EP.y );
      if ( placeList->ValidPlace( EP ) ) {
        SALONS2::TPoint p( EP.x + 1, EP.y );
        SALONS2::TPlace *place = placeList->place( p ); /* берем след. место */
        if ( !place->visible ) { /* следующее место не видно => проход */
          foundTubeBefore = FindPlaces_From( EP, foundCount, sLeft );
          foundCount += foundTubeBefore;
//          ProgTrace( TRACE5, "EP=(%d,%d), foundTubeBefore=%d", EP.x, EP.y, foundTubeBefore );
          if ( foundTubeBefore && Wanted ) { /* если мы нашли места и нам надо Wanted */
            if ( foundTubeBefore > Wanted )
              foundTubeBefore = Wanted;
            EP.x -= foundTubeBefore - 1;
            Wanted -= Put_Find_Places( EP, EP, foundTubeBefore, sLeft ); /* первый параметр EP */
            if ( Wanted <= 0 ) {
              return true; /* Ура все нашлось */
            }
          }
        }
      }
    }
  } // end of found tube
  if ( Wanted )
    return false;
  /* если мы здесь, то Wanted = 0 ( изначально ) и
     мы имеем кол-во мест найденных слева, справа ... */
  int EndWanted = 0; /* признак того, что надо будет удалить одно лишнее место из найденных */
  if ( Step == sRight && foundCount == MAXPLACE() &&
       counters.p_Count_3() == Passengers.counters.p_Count_3() &&
       counters.p_Count_2() == Passengers.counters.p_Count_2() &&
       counters.p_Count() == Passengers.counters.p_Count() - MAXPLACE() - 1 ) {
    /* надо попробовать посадить на следующий ряд не одного чел-ка а 2-х */
    EP = FP;
    EP.y++;
    if ( EP.y >= placeList->GetYsCount() || !SeatSubGrp_On( EP, Step, 2 ) ) {
      return false; /* не смогли посадить 2-х на следующий ряд */
    }
    else {
      EndWanted = seatplaces.size(); /* нашли 2 места на следующем ряду и положили их в FPlaces */
      /* обманем наши счетчики на одно место */
      counters.Add_p_Count( -1 );
    }
  }
  /* начинаем запоминать полученные места */
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
  if ( EndWanted > 0 ) { /* надо удалить одно самое плохое место: */
    /* самое удаленное от после	днего найденного в текущем ряду */
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
    /* нашли самое удаленное место и сейчас удалим его */
    RollBack( EndWanted, EndWanted );
    counters.Add_p_Count( 1 );
    return true; /* все нашли выходим */
  }
  /* теперь обсудим следующий вариант: в текущем ряду нашли всего одно место.
     пусть такое допустимо только однажды */
  if ( foundCount == 1 && CanUseAlone != uTrue ) /*нельзя оставлять одного*/
    if ( Alone )
      Alone = false;
    else  /* нельзя 2 раза чтобы появлялось одно место в ряду. */
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
  tst();
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
  /* переходим на следующий ряд ???canUseOneRow??? */
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
         !SeatSubGrp_On( EP, Step, 0 ) ) { // ничего не смогли найти дальше
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
           !SeatSubGrp_On( EP, Step, 0 ) ) // ничего не смогли найти дальше
        return false;
    }
  }
  else return false;
  return true;
}

bool TSeatPlaces::SeatsStayedSubGrp( TWhere Where )
{
  /* проверка а осталась ли часть группы, или группа состоит из одного человека ? */
  if ( counters.p_Count_3( sRight ) == Passengers.counters.p_Count_3( sRight ) &&
       counters.p_Count_2( sRight ) == Passengers.counters.p_Count_2( sRight ) &&
       counters.p_Count( sRight ) == Passengers.counters.p_Count( sRight ) &&
       counters.p_Count_3( sDown ) == Passengers.counters.p_Count_3( sDown ) &&
       counters.p_Count_2( sDown ) == Passengers.counters.p_Count_2( sDown ) &&
       counters.p_Count( sDown ) == Passengers.counters.p_Count( sDown ) ) {
    return true;
  }

  SALONS2::TPoint EP;
  /* надо рассадить оставшуюся часть пассажиров */
  /* для этого надо обойти выбранные места вокруг и попробовать рассадить
    оставшуюся группу рядом с уже рассаженной */
  VSeatPlaces sp( seatplaces.begin(), seatplaces.end() );
  if ( Where != sUpDown )
  for (ISeatPlace isp = sp.begin(); isp!= sp.end(); isp++ ) {
    CurrSalon->SetCurrPlaceList( isp->placeList );
    for( vector<SALONS2::TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
      /* поищем справа от найденного места */
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
      Alone = true; /* можно найти одно место */
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        return true;
       }
      tst();
      SALONS2::TPlaceList *placeList = CurrSalon->CurrPlaceList();
      SALONS2::TPoint p( EP.x + 1, EP.y );
      if ( CanUseTube && placeList->ValidPlace( p ) &&
           !placeList->place( EP )->visible ) {
       /* можно попробовать искать через проход */
       Alone = true; /* можно найти одно место */
       if ( SeatSubGrp_On( p, sRight, 0 ) ) {
         return true;
       }
      }
      /* поищем слева от найденного места */
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
      Alone = true; /* можно найти одно место */
      if ( SeatSubGrp_On( EP, sRight, 0 ) ) {
        return true;
      }
      placeList = CurrSalon->CurrPlaceList();
      p.x = EP.x - 1;
      p.y = EP.y;
      if ( CanUseTube &&
           placeList->ValidPlace( p ) &&
           !placeList->place( EP )->visible ) {
        /* можно попробовать искать через проход */
        Alone = true; /* можно найти одно место */
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
    /* поищем сверху и снизу от найденного места */
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
      /* рассадка сверху от занятых мест */
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
      /* теперь посмотрим снизу */
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
  ISeatPlace misp;
  string ispPlaceName_lat, ispPlaceName_rus;
  for (ISeatPlace isp=seatplaces.begin(); isp!=seatplaces.end(); isp++) {
//    ProgTrace( TRACE5, "isp->oldPlaces.size()=%d, pass.countPlace=%d",
//               (int)isp->oldPlaces.size(), pass.countPlace );
    if ( isp->InUse || (int)isp->oldPlaces.size() != pass.countPlace ) /* кол-во мест должно совпадать */
      continue;
  	ispPlaceName_lat.clear();
  	ispPlaceName_rus.clear();
    SALONS2::TPlaceList *placeList = isp->placeList;
    ispPlaceName_lat = denorm_iata_row( placeList->GetYsName( isp->Pos.y ) );
    ispPlaceName_rus = ispPlaceName_lat;
    ispPlaceName_lat += denorm_iata_line( placeList->GetXsName( isp->Pos.x ), 1 );
    ispPlaceName_rus += denorm_iata_line( placeList->GetXsName( isp->Pos.x ), 0 );
    int EqualQ = 0;
    if ( (int)isp->oldPlaces.size() == pass.countPlace )
      EqualQ = pass.countPlace*10000; //??? always true!
    bool pr_valid_place = true;
    vector<string> vrems;
    pass.get_remarks( vrems );
    for (vector<string>::iterator irem=vrems.begin(); irem!= vrems.end(); irem++ ) { // пробег по ремаркам пассажира
      for( vector<SALONS2::TPlace>::iterator ipl=isp->oldPlaces.begin(); ipl!=isp->oldPlaces.end(); ipl++ ) {
      	/* пробег по местам которые может занимать пассажир */
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
        if ( itr == ipl->rems.end() ) /* не нашли ремарку у места */
          EqualQ += PR_REM_TO_NOREM;
      } /* конец пробега по местам */
    } /* конец пробега по ремаркам пассажира */

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
    if ( pass.prSmoke == isp->oldPlaces.begin()->isLayer( cltSmoke ) )
      EqualQ += PR_EQUAL_SMOKE;
//    ProgTrace( TRACE5, "EqualQ=%d", EqualQ );
    if ( pass.pers_type != "ВЗ" ) {
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
      throw Exception( "Недопустимое значение направления рассадки" );
    pass.placeList = seatPlace.placeList;
    pass.Pos = seatPlace.Pos;
    pass.Step = seatPlace.Step;
    pass.placeName = seatPlace.placeList->GetPlaceName( seatPlace.Pos );
    pass.isValidPlace = seatPlace.isValid;
    pass.set_seat_no();
  }
}

/* рассадка всей группы начиная с позиции FP */
bool TSeatPlaces::SeatsGrp_On( SALONS2::TPoint FP  )
{
//  ProgTrace( TRACE5, "FP(x=%d, y=%d)", FP.x, FP.y );
  /* очистить помеченные места */
  RollBack( );
  /* если есть пассажиры в группе с вертикальной рассадкой, то пробуем их рассадить */
  if ( Passengers.counters.p_Count_3( sDown ) + Passengers.counters.p_Count_2( sDown ) > 0 ) {
    if ( !SeatSubGrp_On( FP, sDown, 0 ) ) { /* не получается */
      RollBack( );
      return false;
    }
    /* если нет других пассажиров, то тогда рассадка выполнена успешно */
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
  /* если мы здесь то смогли рассадить часть группы
     рассаживаем оставшуюся часть группы */
  /* можно попытаться посадить и сверху и снизу от занятых мест */
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
      /* конвертация номеров мест пассажиров в зависимости от лат. или рус. салона */
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

/* поиск в одном салоне и заданных глобальных переменных.
   изменяется лишь CanUseRems и PlaceRemark */
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
    /* поиск исходного места для пассажира */
    int lp = Passengers.getCount();
    for ( int i=0; i<lp; i++ ) {
      /* выделяем из группы главного пассажира и находим для него место */
      TPassenger &pass = Passengers.Get( i );
      //ProgTrace( TRACE5, "pass.countPlace=%d, pass.step=%d(sDown=%d)", pass.countPlace, pass.Step, sDown );
      Passengers.SetCountersForPass( pass );
      getRemarks( pass );
      if ( !pass.placeName.empty() &&
           SeatsPassenger_OnBasePlace( pass.placeName, pass.Step ) && /* нашли базовое место */
           LSD( G3, G2, G, V3, V2, sEveryWhere ) )
        return true;

      RollBack( );
      Passengers.SetCountersForPass( pass );
      if ( !Remarks.empty() ) { /* есть ремарка */
        // попытаемся найти по ремарке
        for ( int Where=sLeftRight; Where<=sUpDown; Where++ ) {
          /* варианты поиска возле найденного места */
          for ( int linesVar=0; linesVar<=1; linesVar++ ) {
            for( vecVarLines::iterator ilines=lines.getVarLine( linesVar ).begin();
                 ilines!=lines.getVarLine( linesVar ).end(); ilines++ ) {
              CurrSalon->SetCurrPlaceList( ilines->placeList );
              int ylen = CurrSalon->CurrPlaceList()->GetYsCount();
              SALONS2::TPoint FP;
              for ( int y=0; y<ylen; y++ ) {
                for ( vector<int>::iterator z=ilines->lines.begin(); z!=ilines->lines.end(); z++ ) {
                  FP.x = *z;
                  FP.y = y; /* пробег по местам */
                  /* посадка самого важного пассажира */
                  if ( SeatSubGrp_On( FP, pass.Step, 0 ) && LSD( G3, G2, G, V3, V2, (TWhere )Where ) ) {
                  	//ProgTrace( TRACE5, "G3=%d, G2=%d, G=%d, V3=%d, V2=%d, commit", G3, G2, G, V3, V2 );
                    return true;
                  }
                  //ProgTrace( TRACE5, "rollback" );
                  RollBack( ); /* не получилось откат занятых мест */
                  Passengers.SetCountersForPass( pass ); /* выделяем опять этого пассажира */
                }
              }
            }
          }
        }
      } /* конец поиска по ремарке */
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

/* рассадка группы по всем салонам */
bool TSeatPlaces::SeatsGrp( )
{
 RollBack( );
 for ( int linesVar=0; linesVar<=1; linesVar++ ) {
   for( vecVarLines::iterator ilines=lines.getVarLine( linesVar ).begin();
        ilines!=lines.getVarLine( linesVar ).end(); ilines++ ) {
     CurrSalon->SetCurrPlaceList( ilines->placeList );
     int ylen = CurrSalon->CurrPlaceList()->GetYsCount();
     SALONS2::TPoint FP;
     for ( int y=0; y<ylen; y++ ) {
       for ( vector<int>::iterator z=ilines->lines.begin(); z!=ilines->lines.end(); z++ ) {
         FP.x = *z;
         FP.y = y; /* пробег по местам */
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

/* рассадка пассажиров по местам не учитывая группу */
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
      	/* когда пассажир посажен или рассадка на бронь и у пассажира статус не бронь или нет предвар. рассадки или у пассажира указано не то место */
        if ( ipass->InUse || PlaceLayer == cltProtCkin && !CanUse_PS &&
        	                   ( ipass->layer != PlaceLayer || ipass->preseat.empty() || ipass->preseat != ipass->placeName ) )
          continue;
        Passengers.Clear();
        ipass->placeList = NULL;
        ipass->seat_no.clear();
        int old_index = ipass->index;
//        ProgTrace( TRACE5, "pr_auto_reseats=%d, find=%d, i=%d",
//                   pr_autoreseats, find( ipass->rems.begin(), ipass->rems.end(), string("MCLS") ) != ipass->rems.end(), i );
        if ( pr_autoreseats ) {
          if ( i == 0 ) {
          	  if ( !ipass->pr_MCLS )
          	  	continue;
            	FindMCLS = true;
            	canUseMCLS = true;
          }
          else
          	if ( i == 1 ) {
          		if ( ipass->pr_MCLS )
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
             ( CanUseRems == sNotUse_NotUseDenial || CanUseRems == sNotUse || CanUseRems == sIgnoreUse || CanUseRems == sNotUseDenial /*!!!*/ ) &&
             ( !CanUseLayers || PlaceLayer == cltProtCkin && CanUse_PS || PlaceLayer != cltProtCkin ) && SeatsGrp( ) ) {
          if ( seatplaces.begin()->Step == sLeft || seatplaces.begin()->Step == sUp )
            throw Exception( "Недопустимое значение направления рассадки" );
          tst();
          ipass->placeList = seatplaces.begin()->placeList;
          ipass->Pos = seatplaces.begin()->Pos;
          ipass->Step = seatplaces.begin()->Step;
          ipass->placeName = ipass->placeList->GetPlaceName( ipass->Pos );
          ipass->isValidPlace = ipass->is_valid_seats( seatplaces.begin()->oldPlaces );
          ipass->InUse = true;
          tst();
          ipass->set_seat_no();
          TCompLayerType l = grp_status;
          Clear();
          grp_status = l;//??? надо так делать, но не красиво
          tst();
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

void TPassenger::add_rem( std::string code )
{
	ProgTrace( TRACE5, "code=%s", code.c_str() );
	if ( code == "MCLS" )
		pr_MCLS = true;
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
  if ( !placeName.empty() || !preseat.empty() )
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
  for (std::vector<string>::iterator irem=rems.begin(); irem!= rems.end(); irem++ ) {
    for(std::vector<SALONS2::TPlace>::const_iterator ipl=places.begin(); ipl!=places.end(); ipl++ ) {
     /* пробег по местам которые может занимать пассажир */
      std::vector<SALONS2::TRem>::const_iterator itr;
      for ( itr=ipl->rems.begin(); itr!=ipl->rems.end(); itr++ )
        if ( *irem == itr->rem && itr->pr_denial )
       	  return false;
    }
  }
  return true;
}

void TPassenger::build( xmlNodePtr pNode )
{
  NewTextChild( pNode, "grp_id", grpId );
  NewTextChild( pNode, "pax_id", pax_id );
  NewTextChild( pNode, "grp_layer_type", EncodeCompLayerType(grp_status) );
  NewTextChild( pNode, "pers_type", pers_type );
  NewTextChild( pNode, "reg_no", regNo );
  NewTextChild( pNode, "name", fullName );
  NewTextChild( pNode, "clname", clname );
  if ( !placeName.empty() )
    NewTextChild( pNode, "seat_no", placeName );
  if ( !wl_type.empty() )
  	NewTextChild( pNode, "wl_type", wl_type );
  if ( countPlace != 1 )
    NewTextChild( pNode, "seats", countPlace );
  NewTextChild( pNode, "tid", tid );
  if ( !isSeat )
    NewTextChild( pNode, "isseat", isSeat );
  if ( !ticket_no.empty() )
    NewTextChild( pNode, "ticket_no", ticket_no );
  if ( !document.empty() )
    NewTextChild( pNode, "document", document );
  if ( bag_weight )
    NewTextChild( pNode, "bag_weight", bag_weight );
  if ( bag_amount )
    NewTextChild( pNode, "bag_amount", bag_amount );
  if ( excess )
    NewTextChild( pNode, "excess", excess );
  if ( !trip_from.empty() )
  	NewTextChild( pNode, "trip_from", trip_from );

  if ( !rems.empty() ) {
  	string rem;
  	bool pr_down = false;
  	for ( vector<string>::iterator r=rems.begin(); r!=rems.end(); r++ ) {
  		rem += *r + " ";
  		if ( *r == "STCR" )
  			pr_down = true;
  	}
  	NewTextChild( pNode, "comp_rem", rem );
  	if ( pr_down )
  	  NewTextChild( pNode, "pr_down", 1 );
  }
  if ( !pass_rem.empty() )
  	NewTextChild( pNode, "pass_rem", pass_rem );
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
  Qry.SQLText = "SELECT code, pr_comp FROM rem_types WHERE pr_comp IS NOT NULL";
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
  if ( pass.countPlace > CONST_MAXPLACE || pass.countPlace <= 0 )
   throw Exception( "Не допустимое кол-во мест для расадки" );

  size_t i = 0;
  for (; i < pass.agent_seat.size(); i++)
    if ( pass.agent_seat[ i ] != '0' )
      break;
  if ( i )
    pass.agent_seat.erase( 0, i );
	if ( !pass.agent_seat.empty() )
	 pass.placeName = pass.agent_seat;
  if ( !pass.preseat.empty() && !pass.placeName.empty() && pass.preseat != pass.placeName ) {
    pass.placeName = pass.preseat; //!!! при регистрации нельзя изменить предварительно назначенное место
  }
  if ( pass.layer == cltPNLCkin && !pass.preseat.empty() && pass.preseat == pass.placeName )
  	pass.layer = cltProtCkin; //!!!

  bool Pr_PLC = false;
  if ( pass.countPlace > 1 && pass.isRemark( string( "STCR" ) )	 ) {
    pass.Step = sDown;
    switch( pass.countPlace ) {
      case 2: counters.Add_p_Count_2( 1, sDown );
              break;
      case 3: tst();
      	      counters.Add_p_Count_3( 1, sDown );
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
 // высчитываем класс
  if ( clname.empty() && !pass.clname.empty() )
    clname = pass.clname;
 // высчитываем приоритет
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
  /* если мы здесь то тогда мы смогли посадить главного чел-ка из группы
     попробуем посадить всех остальных */
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
  for ( vector<SALONS2::TPlaceList*>::iterator iplaceList=CurrSalon->placelists.begin();
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

// !!! вычисляем на основе данных из БД
void SetLayers( vector<TCompLayerType> &Layers, TCompLayerType layer, bool First, bool use_PS )
{
  Layers.clear();
  if ( layer == cltTranzit || layer == cltProtTrzt )
    if ( First ) {
      Layers.push_back( cltProtTrzt );
      Layers.push_back( cltUnknown );
      Layers.push_back( cltUncomfort );
    }
    else {
      Layers.push_back( cltProtect );
      Layers.push_back( cltPNLCkin );
    	if ( use_PS )
        Layers.push_back( cltProtCkin );
    }
  else
    if ( layer == cltProtect )
      if ( First ) {
        Layers.push_back( cltProtect );
        Layers.push_back( cltUnknown );
      }
      else {
        Layers.push_back( cltUncomfort );
        Layers.push_back( cltPNLCkin );
        if ( use_PS )
          Layers.push_back( cltProtCkin );
      }
    else
      if ( layer == cltUnknown )
        if ( First ) {
          Layers.push_back( cltUnknown );
          Layers.push_back( cltUncomfort );
        }
        else {
          Layers.push_back( cltProtect );
          Layers.push_back( cltPNLCkin );
          if ( use_PS )
            Layers.push_back( cltProtCkin );
        }
      else
        if ( layer == cltPNLCkin )
          if ( First ) {
          	Layers.push_back( cltPNLCkin );
            Layers.push_back( cltUnknown );
            Layers.push_back( cltUncomfort );
          }
          else {
            Layers.push_back( cltProtect );
            if ( use_PS )
              Layers.push_back( cltProtCkin );
          }
        else
        	if ( layer == cltProtCkin ) {
        		if ( First ) {
        			if ( use_PS )
          	    Layers.push_back( cltProtCkin );
              Layers.push_back( cltUnknown );
              Layers.push_back( cltUncomfort );
        		}
        		else {
        			Layers.push_back( cltProtect );
        			Layers.push_back( cltPNLCkin );
        		}
        	}
}

/*///////////////////////////END CLASS TPASSENGERS/////////////////////////*/

bool ExistsBasePlace( SALONS2::TSalons &Salons, TPassenger &pass )
{
  SALONS2::TPlaceList *placeList;
  SALONS2::TPoint FP;
  vector<SALONS2::TPlace*> vpl;
  string placeName = pass.placeName;
  for ( vector<SALONS2::TPlaceList*>::iterator plList=Salons.placelists.begin();
        plList!=Salons.placelists.end(); plList++ ) {
    placeList = *plList;
    if ( placeList->GetisPlaceXY( placeName, FP ) ) {
      int j = 0;
      for ( ; j<pass.countPlace; j++ ) {
        if ( !placeList->ValidPlace( FP ) )
          break;
        SALONS2::TPlace *place = placeList->place( FP );
        bool findpass = pass.pr_MCLS;
        bool findplace = false;
        for ( vector<SALONS2::TRem>::iterator r=place->rems.begin(); r!=place->rems.end(); r++ ) {
        	if ( !r->pr_denial && r->rem == "MCLS" ) {
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
          (*ipl)->AddLayerToPlace( pass.grp_status, 0, pass.pax_id, Salons.getPriority( pass.grp_status ) );
        }
        return true;
      }
      vpl.clear();
    }
  }
  return false;
}

/* рассадка пассажиров */
void SeatsPassengers( SALONS2::TSalons *Salons, int SeatAlgo /* 0 - умолчание */,  TPassengers &passengers, bool FUse_PS )
{
	ProgTrace( TRACE5, "NEWSEATS" );
  if ( !passengers.getCount() )
    return;
  FSeatAlgo = SeatAlgo;
  SeatPlaces.Clear();
  SeatPlaces.grp_status = passengers.Get( 0 ).grp_status;
  ProgTrace( TRACE5, "SeatPlaces.grp_status=%s,counters.p_Count_3( sDown )=%d", EncodeCompLayerType(SeatPlaces.grp_status), passengers.counters.p_Count_3( sDown ) );
  CurrSalon = Salons;
  vector<TCompLayerType> Layers;
  CanUseLayers = true;
	CanUse_PS = FUse_PS;
  CanUseSmoke = false; /* пока не будем работать с курящими местами */
  CanUseElem_Type = false; /* пока не будем работать с типами мест */
  bool Status_preseat = FUse_PS;//!!!false;
  GET_LINE_ARRAY( );
  SeatAlg = sSeatGrpOnBasePlace;

  FindMCLS = false;
  canUseMCLS = false;
  bool pr_MCLS = false;

  /* не сделано!!! если у всех пассажиров есть места, то тогда рассадка по местам, без учета группы */

  /* определение есть ли в группе пассажир с предварительной рассадкой */
  bool Status_seat_no_BR=false;
  for ( int i=0; i<passengers.getCount(); i++ ) {
  	TPassenger &pass = passengers.Get( i );
  	if ( pass.layer == cltProtCkin ) { // !!!
  		Status_preseat = true;
  	}
  	if ( pass.pr_MCLS ) {
  		pr_MCLS = true;
    }
  }
  /*!!!*/
  bool SeatOnlyBasePlace=true;
  for ( int i=0; i<passengers.getCount(); i++ ) {
  	TPassenger &pass = passengers.Get( i );
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
     /* если есть в группе предварительная рассадка, то тогда сажаем всех отдельно */
     if ( ( Status_preseat || Status_seat_no_BR || SeatOnlyBasePlace /*!!!*/ ) && SeatAlg != sSeatPassengers )
     	 continue;
     for ( int FCanUseRems=0; FCanUseRems<useremLength; FCanUseRems++ ) {
        CanUseRems = (TUseRem)FCanUseRems;
        switch( (int)SeatAlg ) {
          case sSeatGrpOnBasePlace:
             switch( (int)CanUseRems ) {
               case sIgnoreUse:
               case sNotUse_NotUseDenial:
               case sNotUseDenial:
               case sNotUse:
               	 ProgTrace( TRACE5, "continue: SeatAlg=%d, CanUseRems=%d", SeatAlg, CanUseRems );
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
               	 ProgTrace( TRACE5, "continue: SeatAlg=%d, CanUseRems=%d", SeatAlg, CanUseRems );
                 continue; /*??? что главнее группа или места с ремарками, кот не надо учитывать */
             }
             break;
        }
        /* оставлять одного несколько раз на рядах при Рассадке группы */
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
          if ( CanUseAlone == uTrue && CanUseRems == sNotUse_NotUseDenial	 ) {
          	// если пассажиров можно оставлять сколько угодно раз по одному в ряду и можно сажать только на места с нужными ремарками
          	continue;
          }
          if ( CanUseAlone == uTrue && SeatAlg == sSeatPassengers ) {
          	ProgTrace( TRACE5, "continue: SeatAlg=%d, CanUseRems=%d, CanUseAlone=%d", SeatAlg, CanUseRems, CanUseAlone );
            continue;
          }
          /* использование статусов мест */
          for ( int KeyLayers=1; KeyLayers>=0; KeyLayers-- ) {
            if ( !KeyLayers && CanUseAlone == uFalse3 ) {
            	ProgTrace( TRACE5, "continue: SeatAlg=%d, CanUseRems=%d, CanUseAlone=%d, KeyLayers=%d", SeatAlg, CanUseRems, CanUseAlone, KeyLayers );
              continue;
            }
            if ( !KeyLayers && ( SeatAlg == sSeatGrpOnBasePlace || SeatAlg == sSeatGrp ) ) {
            	ProgTrace( TRACE5, "continue: SeatAlg=%d, CanUseRems=%d, CanUseAlone=%d, KeyLayers=%d", SeatAlg, CanUseRems, CanUseAlone, KeyLayers );
            	continue;
            }

            /* задаем массив статусов мест */
            SetLayers( Layers, passengers.Get( 0 ).layer, KeyLayers, Status_preseat );
            /* пробег по статусом */
            for ( vector<TCompLayerType>::iterator l=Layers.begin(); l!=Layers.end(); l++ ) {
              PlaceLayer = *l;
              /* учет режима рассадки в одном ряду */
              for ( int FCanUseOneRow=getCanUseOneRow(); FCanUseOneRow>=0; FCanUseOneRow-- ) {
              	tst();
              	canUseOneRow = FCanUseOneRow;
                /* учет проходов */
                for ( int FCanUseTube=0; FCanUseTube<=1; FCanUseTube++ ) {
                  /* для рассадки отдельных пассажиров не надо учитывать проходы */
                  if ( FCanUseTube && SeatAlg == sSeatPassengers ) {
                  	ProgTrace( TRACE5, "continue: SeatAlg=%d, CanUseRems=%d, CanUseAlone=%d, KeyLayers=%d,FCanUseTube=%d,FCanUseOneRow=%d", SeatAlg, CanUseRems, CanUseAlone, KeyLayers,FCanUseTube,FCanUseOneRow );
                    continue;
                  }
                  if ( FCanUseTube && CanUseAlone == uFalse3 && !canUseOneRow ) {
                  	ProgTrace( TRACE5, "continue: SeatAlg=%d, CanUseRems=%d, CanUseAlone=%d, KeyLayers=%d,FCanUseTube=%d,FCanUseOneRow=%d", SeatAlg, CanUseRems, CanUseAlone, KeyLayers,FCanUseTube, FCanUseOneRow );
                    continue;
                  }
              	  if ( canUseOneRow && !FCanUseTube ) {
              	  	ProgTrace( TRACE5, "continue: SeatAlg=%d, CanUseRems=%d, CanUseAlone=%d, KeyLayers=%d,FCanUseTube=%d,FCanUseOneRow=%d", SeatAlg, CanUseRems, CanUseAlone, KeyLayers,FCanUseTube,FCanUseOneRow );
              	  	continue;
              	  }
                  CanUseTube = FCanUseTube;
                  for ( int FCanUseSmoke=passengers.UseSmoke; FCanUseSmoke>=0; FCanUseSmoke-- ) {
/*                    if ( !FCanUseSmoke && CanUseAlone == uFalse3 )
                      continue; ???*/

                    CanUseSmoke = FCanUseSmoke;
                    ProgTrace( TRACE5, "seats with:SeatAlg=%d,FCanUseRems=%s,FCanUseAlone=%d,KeyStatus=%d,FCanUseTube=%d,FCanUseSmoke=%d,PlaceStatus=%s, MAXPLACE=%d,canUseOneRow=%d, CanUseMCLS=%d",
                               (int)SeatAlg,DecodeCanUseRems( CanUseRems ).c_str(),FCanUseAlone,KeyLayers,FCanUseTube,FCanUseSmoke,EncodeCompLayerType(PlaceLayer),MAXPLACE(),canUseOneRow,canUseMCLS);
                    switch( (int)SeatAlg ) {
                      case sSeatGrpOnBasePlace:
                        if ( SeatPlaces.SeatGrpOnBasePlace( ) ) {
                        	tst();
                          throw 1;
                        }
                        break;
                      case sSeatGrp:
                      	getRemarks( passengers.Get( 0 ) ); // для самого приоритетного
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
    /* распределение полученных мест по пассажирам, только для SeatPlaces.SeatGrpOnBasePlace */
    if ( SeatAlg != sSeatPassengers ) {
      SeatPlaces.PlacesToPassengers( );
    }

    Passengers.sortByIndex();

    SeatPlaces.RollBack( );
    return;
  }
  SeatPlaces.RollBack( );
  throw UserException( "Автоматическая рассадка невозможна" );
}

bool GetPassengersForWaitList( int point_id, TPassengers &p, bool pr_exists )
{
	bool res = false;
	TQuery Qry( &OraSession );
  TQuery RemsQry( &OraSession );
	TQuery QryTCkinTrip( &OraSession );
  TPaxSeats priorSeats( point_id );
	if ( !pr_exists ) {
	  p.Clear();
    RemsQry.SQLText =
      "SELECT rem, rem_code, pax.pax_id, rem_types.pr_comp "
      " FROM pax_rem, pax_grp, pax, rem_types "
      "WHERE pax_grp.grp_id=pax.grp_id AND "
      "      pax_grp.point_dep=:point_id AND "
      "      pax.pr_brd IS NOT NULL AND "
      "      pax.seats > 0 AND "
      "      pax_rem.pax_id=pax.pax_id AND "
      "      rem_code=rem_types.code(+) "
      " ORDER BY pax.pax_id, pr_comp, code ";
    RemsQry.CreateVariable( "point_id", otInteger, point_id );
  }
  Qry.SQLText =
    "SELECT airline "
    " FROM points "
    " WHERE points.point_id=:point_id AND points.pr_del!=-1 AND points.pr_reg<>0";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof )
  	throw UserException( "Рейс не найден" );
  string airline = Qry.FieldAsString( "airline" );
  map<string,TCompLayerType> statuses;
  if ( !pr_exists ) {
    Qry.Clear();
    Qry.SQLText =
      "SELECT code,layer_type FROM grp_status_types";
    Qry.Execute();
    while ( !Qry.Eof ) {
    	statuses[ Qry.FieldAsString( "code" ) ] = DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) );
    	Qry.Next();
    }
  }
  QryTCkinTrip.SQLText =
    "SELECT airline,flt_no,suffix,scd_out,airline_fmt,suffix_fmt "
    " FROM points, pax_grp, "
    " (SELECT MAX(tckin2.seg_no), tckin2.grp_id FROM tckin_pax_grp tckin1, tckin_pax_grp tckin2 "
    "   WHERE tckin1.grp_id=:grp_id AND tckin2.tckin_id=tckin1.tckin_id AND tckin2.seg_no<tckin1.seg_no "
    "  GROUP BY tckin2.grp_id) tckin "
    " WHERE pax_grp.grp_id=tckin.grp_id AND points.point_id=pax_grp.point_dep";
  QryTCkinTrip.DeclareVariable( "grp_id", otInteger );

  Qry.Clear();
  string sql =
    "SELECT pax_grp.grp_id,"
    "       pax.pax_id,"
    "       pax.reg_no,"
    "       surname,"
    "       pax.name, "
    "       pax_grp.class,"
    "       cls_grp.code subclass,"
    "       pax.seats,"
    "       pax_grp.status, "
    "       pax.pers_type, "
    "       pax.ticket_no, "
    "       pax.document, "
    "       ckin.get_bagWeight(pax.grp_id,pax.pax_id,rownum) AS bag_weight,"
    "       ckin.get_bagAmount(pax.grp_id,pax.pax_id,rownum) AS bag_amount, "
    "       ckin.get_excess(pax.grp_id,pax.pax_id) AS excess,"
    "       pax.tid,"
    "       pax.wl_type, "
    "       salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'list',rownum) AS seat_no "
    " FROM pax_grp, pax, cls_grp "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_dep=:point_id AND "
    "      pax_grp.class_grp = cls_grp.id AND "
    "      pax.pr_brd IS NOT NULL AND "
    "      pax.seats > 0 ";
  if ( pr_exists )
  	sql += " AND salons.get_seat_no(pax.pax_id,pax.seats,pax_grp.status,pax_grp.point_dep,'list',rownum) IS NULL AND rownum<2 ";
  else
    sql += " ORDER BY pax.pax_id";
  Qry.SQLText = sql;
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( pr_exists ) {
  	return !Qry.Eof;
  }
  RemsQry.Execute();
  while ( !Qry.Eof ) {
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
    pass.ticket_no = Qry.FieldAsString( "ticket_no" );
    pass.document = Qry.FieldAsString( "document" );
    pass.bag_weight = Qry.FieldAsInteger( "bag_weight" );
    pass.bag_amount = Qry.FieldAsInteger( "bag_amount" );
    pass.excess = Qry.FieldAsInteger( "excess" );
    pass.grp_status = statuses[ Qry.FieldAsString( "status" ) ];
    pass.pers_type = Qry.FieldAsString( "pers_type" );
    pass.wl_type = Qry.FieldAsString( "wl_type" );
    pass.InUse = ( !pass.placeName.empty() );
    pass.isSeat = pass.InUse;
    if ( pass.placeName.empty() ) { // ???необходимо выбрать предыдущее место
    	res = true;
  		string old_seat_no;
    		if ( pass.wl_type.empty() ) {
    		  old_seat_no = priorSeats.getSeats( pass.pax_id, "seats" );
    		  if ( !old_seat_no.empty() )
    		  	old_seat_no = "(" + old_seat_no + ")";
    		}
    		else
    			old_seat_no = "ЛО";
    		if ( !old_seat_no.empty() )
    			pass.placeName = old_seat_no;
    }
    while ( !RemsQry.Eof && RemsQry.FieldAsInteger( "pax_id" ) <= pass.pax_id ) {
    	if ( RemsQry.FieldAsInteger( "pax_id" ) == pass.pax_id ) {
    		pass.add_rem( RemsQry.FieldAsString( "rem_code" ) );
    		pass.pass_rem += string( ".R/" ) + RemsQry.FieldAsString( "rem" ) + "   ";
    	}
      RemsQry.Next();
    }
    if (
  	     airline == string( "ЮТ" ) && string( "М" ) == Qry.FieldAsString( "subclass" ) ||
  	     airline == string( "ПО" ) && string( "Ю" ) == Qry.FieldAsString( "subclass" )
  	   ) {
  	  pass.add_rem( "MCLS" );
    }
    if ( pass.grp_status == cltTCheckin ) {
    	ProgTrace( TRACE5, "grp_id=%d", pass.grpId );
    	QryTCkinTrip.SetVariable( "grp_id", pass.grpId );
    	QryTCkinTrip.Execute();
    	if ( !QryTCkinTrip.Eof ) {
    		pass.trip_from =
    		ElemIdToElemCtxt( ecDisp, etAirline, QryTCkinTrip.FieldAsString( "airline" ), QryTCkinTrip.FieldAsInteger( "airline_fmt" ) ) +
    		QryTCkinTrip.FieldAsString( "flt_no" ) +
    		ElemIdToElemCtxt( ecDisp, etSuffix, QryTCkinTrip.FieldAsString( "suffix" ), QryTCkinTrip.FieldAsInteger( "suffix_fmt" ) ) + "/" +
    		DateTimeToStr( QryTCkinTrip.FieldAsDateTime( "scd_out" ), "dd" );

    	}
    }
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
  return false; //!!! салона может и не быть, а разметить надо
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
	CanUse_PS = false; //!!!
	first_xname = norm_iata_line( first_xname );
	first_yname = norm_iata_line( first_yname );
	ProgTrace( TRACE5, "layer=%s, point_id=%d, pax_id=%d, first_xname=%s, first_yname=%s",
	           EncodeCompLayerType( layer_type ), point_id, pax_id, first_xname.c_str(), first_yname.c_str() );
  TQuery Qry( &OraSession );
  /* лочим рейс */
  Qry.SQLText = "UPDATE points SET point_id=point_id WHERE point_id=:point_id";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", point_id );
  Qry.Execute();
	Qry.Clear();
  /* считываем инфу по пассажиру */
  switch ( layer_type ) {
  	case cltGoShow:
  	case cltTranzit:
  	case cltCheckin:
  	case cltTCheckin:
      Qry.SQLText =
       "SELECT surname, name, reg_no, pax.grp_id, pax.seats, a.step step, pax.tid, '' target, point_dep, point_arv, "
       "       0 point_id, salons.get_seat_no(pax.pax_id,pax.seats,NULL,:point_dep,'list',rownum) AS seat_no, class "
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
        "      salons.get_crs_seat_no(crs_pax.pax_id,:layer_type,crs_pax.seats,crs_pnr.point_id,'list',rownum) AS seat_no, class "
        " FROM crs_pax, crs_pnr, "
        "( SELECT COUNT(*) step FROM crs_pax_rem "
        "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "
        " WHERE crs_pax.pax_id=:pax_id AND crs_pax.pnr_id=crs_pnr.pnr_id";
      Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
    	break;
    default:
    	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
    	throw UserException( "Устанавливаемый слой запрещен для разметки" );
  }
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  // пассажир не найден или изменеоизводились с другой стойки или при предв. рассадке пассажир уже зарегистрирован
  if ( !Qry.RowCount() ) {
    ProgTrace( TRACE5, "!!! Passenger not found in funct ChangeLayer" );
    throw UserException( "Пассажир не найден. Обновите данные"	);
  }

  string strclass = Qry.FieldAsString( "class" );
  ProgTrace( TRACE5, "subclass=%s", strclass.c_str() );
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
    throw UserException( "Пересадка невозможна. Количество мест занимаемых пассажиром равно нулю" ); //!!!
  }

  if ( Qry.FieldAsInteger( "tid" ) != tid  ) {
    ProgTrace( TRACE5, "!!! Passenger has changed in other term in funct ChangeLayer" );
    throw UserException( string( "Изменения по пассажиру " ) + fullname + " производились с другой стойки. Обновите данные" ); //!!!
  }
  if ( ( layer_type != cltCheckin && layer_type != cltTCheckin && layer_type != cltTranzit ) && SALONS2::Checkin( pax_id ) ) { //???
  	ProgTrace( TRACE5, "!!! Passenger set layer=%s, but his was chekin in funct ChangeLayer", EncodeCompLayerType( layer_type ) );
  	throw UserException( "Пассажир зарегистрирован. Обновите данные" );
  }
  vector<TSeatRange> seats;
  if ( seat_type != stDropseat ) { // заполнение вектора мест + проверка
  	TQuery QrySeatRules( &OraSession );
  	QrySeatRules.SQLText =
	   "SELECT pr_owner FROM comp_layer_rules "
	   "WHERE src_layer=:new_layer AND dest_layer=:old_layer";
  	QrySeatRules.CreateVariable( "new_layer", otString, EncodeCompLayerType( layer_type ) );
  	QrySeatRules.DeclareVariable( "old_layer", otString );
  // считываем слои по новому месту и делаем проверку на то, что этот слой уже занят другим пассажиром
  	SALONS2::TSalons Salons( point_id, SALONS2::rTripSalons );
  	Salons.Read();
	  seats.clear();
    TSeatRange r;
    TSalonPoint p;
    TPlace* place;
    strcpy( r.first.line, first_xname.c_str() );
    strcpy( r.first.row, first_yname.c_str() );
    r.second = r.first;
    if ( !getCurrSeat( Salons, r, p ) )
    	throw UserException( "Указанное место недоступно для пассажира" );
    vector<TPlaceList*>::iterator placeList = Salons.placelists.end();
    for( placeList = Salons.placelists.begin();placeList != Salons.placelists.end(); placeList++ ) {
    	if ( (*placeList)->num == p.num )
    		break;
    }
    if ( placeList == Salons.placelists.end() )
    	throw UserException( "Указанное место недоступно для пассажира" );
    for ( int i=0; i<seats_count; i++ ) { // пробег по кол-ву мест и по местам
    	SALONS2::TPoint coord( p.x, p.y );
    	place = (*placeList)->place( coord );
    	if ( !place->visible || !place->isplace || place->clname != strclass )
    		throw UserException( "Указанное место недоступно для пассажира" );
    	// проверка на то, что мы имеем право назначить слой на эти места по пассажиру
    	if ( !place->layers.empty() ) {
    		if ( place->layers.begin()->pax_id == pax_id &&
    			   place->layers.begin()->layer_type == layer_type )
    			throw UserException( "Место принадлежит пассажиру" );
			  QrySeatRules.SetVariable( "old_layer", EncodeCompLayerType( place->layers.begin()->layer_type ) );
    	  ProgTrace( TRACE5, "old layer=%s", EncodeCompLayerType( place->layers.begin()->layer_type ) );
    	  QrySeatRules.Execute();
		    if ( QrySeatRules.Eof )
    			throw UserException( "Невозможно назначить заданное место" );
    		if ( QrySeatRules.FieldAsInteger( "pr_owner" ) && pax_id != place->layers.begin()->pax_id )
    			throw UserException( "Место занято другим пассажиром" );
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

	  if ( Qry.Eof ) {
	  	ProgError( STDLOG, "CanChangeLayer: error xname=%s, yname=%s", first_xname.c_str(), first_yname.c_str() );
	  	throw UserException( "Указанные места недоступны" );
	  }
  }

  if ( seat_type != stSeat ) { // пересадка, высадка - удаление старого слоя
  	Qry.Clear();

  	Qry.Clear();
    	switch( layer_type ) {
    		case cltGoShow:
      	case cltTranzit:
      	case cltCheckin:
  	    case cltTCheckin:
  	    Qry.SQLText =
          "DELETE FROM trip_comp_layers "
          " WHERE point_id=:point_id AND "
          "       layer_type=:layer_type AND "
          "       pax_id=:pax_id ";
        Qry.CreateVariable( "point_id", otInteger, point_id );
      	break;
  		case cltProtCkin:
  			// удаление из салона, если есть разметка
  	    Qry.SQLText =
          "DELETE FROM "
          "  (SELECT * FROM tlg_comp_layers,trip_comp_layers "
          "    WHERE tlg_comp_layers.range_id=trip_comp_layers.range_id AND "
          "       tlg_comp_layers.crs_pax_id=:pax_id AND "
          "       tlg_comp_layers.layer_type=:layer_type) ";
        Qry.CreateVariable( "pax_id", otInteger, pax_id );
        Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
        Qry.Execute();
        // удаление из слоя телеграмм
  			Qry.Clear();
      	Qry.SQLText =
	        "DELETE FROM tlg_comp_layers "
          " WHERE tlg_comp_layers.crs_pax_id=:pax_id AND "
          "       tlg_comp_layers.layer_type=:layer_type ";
        break;
      default:
      	ProgTrace( TRACE5, "!!! Unusible layer=%s in funct ChangeLayer",  EncodeCompLayerType( layer_type ) );
      	throw UserException( "Устанавливаемый слой запрещен для разметки" );
    }
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "layer_type", otString, EncodeCompLayerType( layer_type ) );
    Qry.Execute();
/*!!!    if ( !Qry.RowCount() == seats || prior_seat.empty() ) { // пытаемся удалить слой, которого нет в БД
      throw UserException( "Исходное место не найдено" );
    }*/
  }
  // назначение нового слоя
  if ( seat_type != stDropseat ) { // посадка на новое место
    Qry.Clear();
    Qry.SQLText = "SELECT tid__seq.nextval AS tid FROM dual";
    Qry.Execute();
    tid = Qry.FieldAsInteger( "tid" );
  	Qry.Clear();
  	switch ( layer_type ) {
  		case cltGoShow:
    	case cltTranzit:
    	case cltCheckin:
    	case cltTCheckin:
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
      	throw UserException( "Устанавливаемый слой запрещен для разметки" );
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
  			case cltGoShow:
  	    case cltTranzit:
  	    case cltCheckin:
  	    case cltTCheckin:
          reqinfo->MsgToLog( string( "Пассажир " ) + fullname +
                             " посажен на место: " +
                             new_seat_no,
                             evtPax, point_id, idx1, idx2 );
          break;
        case cltProtCkin:
          reqinfo->MsgToLog( string( "Пассажиру " ) + fullname +
                             " предварительно назначено место. Новое место: " +
                             new_seat_no,
                             evtPax, point_id, idx1, idx2 );
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
          reqinfo->MsgToLog( string( "Пассажир " ) + fullname +
                             " пересажен. Новое место: " +
                             new_seat_no,
                             evtPax, point_id, idx1, idx2 );
          break;
        case cltProtCkin:
          reqinfo->MsgToLog( string( "Пассажиру " ) + fullname +
                             " предварительно назначено место. Новое место: " +
                             new_seat_no,
                             evtPax, point_id, idx1, idx2 );
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
          reqinfo->MsgToLog( string( "Пассажир " ) + fullname +
                             " высажен. Место: " + prior_seat,
                             evtPax, point_id, idx1, idx2 );
          break;
        case cltProtCkin:
          reqinfo->MsgToLog( string( "Пассажиру " ) + fullname +
                             " отменено предварительно назначенное место: " + prior_seat,
                             evtPax, point_id, idx1, idx2 );
          break;
        default:;
  		}
  		break;
  }
  check_waitlist_alarm( point_id );
}

void AutoReSeatsPassengers( SALONS2::TSalons &Salons, TPassengers &APass, int SeatAlgo )
{
	// салон содержит все нормальные места (нет инвалидных мест, например с разрывами
  if ( Salons.placelists.empty() )
    throw Exception( "Не задан салон для автоматической рассадки" );
  FSeatAlgo = SeatAlgo;
  CurrSalon = &Salons;
  SeatAlg = sSeatPassengers;
  CanUseLayers = false; /* не учитываем статус мест */
  CanUse_PS = true;
  CanUseSmoke = false;
  CanUseElem_Type = false;
  CanUseRems = sIgnoreUse;
  Remarks.clear();
  CanUseTube = true;
  CanUseAlone = uTrue;
  SeatPlaces.Clear();

  //!!! не задано pass.placeName, pass.PrevPlaceName, pass.OldPlaceName, pass.placeList,pass.InUse ,x,y???
	TQuery Qry( &OraSession );
	Qry.SQLText = "SELECT layer_type FROM grp_status_types ORDER BY priority ";
	Qry.Execute();

  int s;
  try {
    while ( !Qry.Eof ) { // пробег по слоям
      for ( int vClass=0; vClass<=2; vClass++ ) { // пробег по классам
        Passengers.Clear();
          s = APass.getCount();
        for ( int i=0; i<s; i++ ) { // пробег по пассажирам
          TPassenger &pass = APass.Get( i );
          if ( pass.isSeat ) // пассажир посажен
            continue;
          if ( pass.grp_status != DecodeCompLayerType( Qry.FieldAsString( "layer_type" ) ) ) // разбиваем пассажиров по типам Бронь, Транзит...
          	continue;
          ProgTrace( TRACE5, "isSeat=%d, pass.pax_id=%d, pass.placeName=%s, pass.clname=%s, layer_type=%s, pass.grp_status=%s, equal_y_class=%d",
                     pass.isSeat, pass.pax_id, pass.placeName.c_str(), pass.clname.c_str(), Qry.FieldAsString( "layer_type" ),
                     EncodeCompLayerType( pass.grp_status ), pass.clname == "Э" );

          switch( vClass ) {
            case 0:
               if ( pass.clname != "П" )
                 continue;
               break;
            case 1:
               if ( pass.clname != "Б" )
                 continue;
               break;
            case 2:
               if ( pass.clname != "Э" )
                 continue;
               break;
          }
          tst();
          if ( ExistsBasePlace( Salons, pass ) ) { // пассажир не посажен, но нашлось для него базовое место - пометили как занято //??? кодировка !!!
          	tst();
            continue;
          }
          Passengers.Add( pass ); /* накапливаются те у которых не нашлось базовых мест */
        } /* пробежались по всем пассажирам */
        if ( Passengers.getCount() ) {
          GET_LINE_ARRAY( );
          /* рассадка пассажира у которого не найдено базовое место */
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
              if ( opass.pax_id == pass.pax_id ) {
              	opass = pass;
              	ProgTrace( TRACE5, "pass.pax_id=%d, pass placeName=%s, pass.isSeat=%d, pass.InUse=%d", pass.pax_id,pass.placeName.c_str(), pass.isSeat, pass.InUse );
              	break;
              }
            }
          }
        }
      } // конец пробега по классам
      Qry.Next();
    } // конец пробега по слоям
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
      " UPDATE pax SET tid=tid__seq.nextval WHERE pax_id=:pax_id;"
      " mvd.sync_pax(:pax_id,:term); "
      "END;";
    QryUpd.DeclareVariable( "pax_id", otInteger );
    QryUpd.DeclareVariable( "term", otString );

    Passengers.Clear();

    s = APass.getCount();
    for ( int i=0; i<s; i++ ) {
    	TPassenger &pass = APass.Get( i );
    	ProgTrace( TRACE5, "pass.pax_id=%d, pass.isSeat=%d", pass.pax_id, pass.isSeat );
    	Passengers.Add( pass );
    	if ( pass.isSeat )
    		continue;
     	QryPax.SetVariable( "pax_id", pass.pax_id );
      QryPax.Execute();
      if ( QryPax.Eof )
      	throw UserException( "Не задано направление у пассажира" );
      int point_dep = QryPax.FieldAsInteger( "point_dep" );
      int point_arv = QryPax.FieldAsInteger( "point_arv" );
      string prev_seat_no = QryPax.FieldAsString( "seat_no" );
      ProgTrace( TRACE5, "pax_id=%d, prev_seat_no=%s,pass.InUse=%d", pass.pax_id, prev_seat_no.c_str(), pass.InUse );

    	if ( !pass.InUse ) { /* не смогли посадить */
    		if ( !pass.placeName.empty() )
          TReqInfo::Instance()->MsgToLog( string("Пассажир " ) + pass.fullName +
                                          " из-за смены компоновки высажен с места " +
                                          prev_seat_no, evtPax, Salons.trip_id, pass.regNo, pass.grpId );
      }
      else {
      	std::vector<TSeatRange> seats;
      	for ( vector<TSeat>::iterator i=pass.seat_no.begin(); i!=pass.seat_no.end(); i++ ) {
      		TSeatRange r(*i,*i);
      		seats.push_back( r );
      	}
        // необходимо вначале удалить все его инвалидные места из слоя регистрации
        QryLayer.SetVariable( "pax_id", pass.pax_id );
        QryLayer.Execute();
  		  SaveTripSeatRanges( Salons.trip_id, pass.grp_status, seats, pass.pax_id, point_dep, point_arv ); //???
  		  QryUpd.SetVariable( "pax_id", pass.pax_id );
        QryUpd.SetVariable( "term", TReqInfo::Instance()->desk.code );
        QryUpd.Execute();
        QryPax.Execute();
        string new_seat_no = QryPax.FieldAsString( "seat_no" );
        ProgTrace( TRACE5, "oldplace=%s, newplace=%s", prev_seat_no.c_str(), new_seat_no.c_str() );
      	if ( prev_seat_no != new_seat_no )/* пересадили на другое место */
          TReqInfo::Instance()->MsgToLog( string( "Пассажир " ) + pass.fullName +
                                          " из-за смены компоновки пересажен на место " +
                                          new_seat_no, evtPax, Salons.trip_id, pass.regNo, pass.grpId );
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

bool CompGrp( TPassenger item1, TPassenger item2 )
{
  TBaseTable &classes=base_tables.get("classes");
  TBaseTableRow &row1=classes.get_row("code",item1.clname);
  TBaseTableRow &row2=classes.get_row("code",item2.clname);
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
			    if ( item1.pax_id < item2.pax_id )
				    return true;
			    else
				    if ( item1.pax_id > item2.pax_id )
					    return false;
				    else
					    return true;
};


void TPassengers::Build( xmlNodePtr dataNode )
{
	tst();
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
  vector<TPassenger> ps;
  while ( !Qry.Eof ) {
    for (VPassengers::iterator p=FPassengers.begin(); p!=FPassengers.end(); p++ ) {
    	if ( p->InUse || p->grp_status != DecodeCompLayerType(Qry.FieldAsString( "layer_type" )) )
    		continue;
    	ps.push_back( *p );
    }
    // сортировка по grp_status + класс + группа
    sort(ps.begin(),ps.end(),CompGrp);
    ProgTrace( TRACE5, "ps.size()=%d, layer_type=%s", ps.size(), Qry.FieldAsString( "layer_type" ) );
    if ( !ps.empty() ) {
    	xmlNodePtr pNode = NewTextChild( passNode, "layer_type", Qry.FieldAsString( "layer_type" ) );
    	SetProp( pNode, "name", Qry.FieldAsString( "name" ) );
    	for ( vector<TPassenger>::iterator ip=ps.begin(); ip!=ps.end(); ip++ ) {
    		ip->build( NewTextChild( pNode, "pass" ) );
    		ip->InUse = true;
    	}
    	ps.clear();
    }
  	Qry.Next();
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

} // end namespace SEATS2
