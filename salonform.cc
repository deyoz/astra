#include <stdlib.h>
#include "salonform.h"
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
#include "stl_utils.h"
#include "images.h"
#include "salons.h"
#include "seats.h"

const char CurrName[] = " (ТЕК.)";

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;


void SalonsInterface::XMLReadSalons(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonsInterface::XMLReadSalons" );
  TReqInfo::Instance()->user.check_access( amRead );
  TSalons Salons;      
  Salons.trip_id = NodeAsInteger( "trip_id", reqNode );
  Salons.ClName = NodeAsString( "ClName", reqNode );
  bool PrepareShow = NodeAsInteger( "PrepareShow", reqNode );
  SetProp( resNode, "handle", "1" );
  xmlNodePtr ifaceNode = NewTextChild( resNode, "interface" );
  SetProp( ifaceNode, "id", "SalonsInterface" );
  SetProp( ifaceNode, "ver", "1" );    
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if ( PrepareShow )
    ImagesInterface::GetImages( reqNode, resNode );
  Salons.Read( rTripSalons );
  Salons.Build( NewTextChild( dataNode, "salons" ) );
};

void SalonsInterface::SalonFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonsInterface::SalonFormShow" );	
  TReqInfo::Instance()->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  ProgTrace( TRACE5, "trip_id=%d", trip_id );
  TQuery *Qry = OraSession.CreateQuery();
  
  try { 
   tst();
   Qry->SQLText = "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "\
                  "       comps.descr,0 as pr_comp "\
                  " FROM comps, trips "\
                  "WHERE trips.bc = comps.craft AND trips.trip_id = :trip_id "\
                  "UNION "\
                  "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "\
                  "       comps.descr,1 as pr_comp "\
                  " FROM comps, trips "\
                  "WHERE trips.bc = comps.craft AND trips.trip_id = :trip_id AND "\
                  "      trips.comp_id = comps.comp_id "\
                  "UNION "\
                  "SELECT -1,bc as craft,bort, "\
                  "        LTRIM(RTRIM( DECODE( a.f, 0, '', ' П'||a.f)||"\
                  "        DECODE( a.c, 0, '', ' Б'||a.c)|| "\
                  "        DECODE( a.y, 0, '', ' Э'||a.y) )) classes, '',1 "\
                  "FROM "\
                  "(SELECT -1, bc, bort, "\
                  "        NVL( SUM( DECODE( class, 'П', 1, 0 ) ), 0 ) as f, "\
                  "        NVL( SUM( DECODE( class, 'Б', 1, 0 ) ), 0 ) as c, "\
                  "        NVL( SUM( DECODE( class, 'Э', 1, 0 ) ), 0 ) as y "\
                  "  FROM trip_comp_elems, comp_elem_types, trips "\
                  " WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "
                  "       comp_elem_types.pr_seat <> 0 AND "\
                  "       trip_comp_elems.trip_id = trips.trip_id AND "\
                  "       trips.trip_id = :trip_id AND "\
                  "       trips.comp_id IS NULL "\
                  "GROUP BY bc, bort) a "\
                  "ORDER BY comp_id, craft, bort, classes, descr";
   Qry->DeclareVariable( "trip_id", otInteger );
   Qry->SetVariable( "trip_id", trip_id );
   tst();
   Qry->Execute();	
   tst();
   if ( Qry->RowCount() == 0 )
     throw UserException( "Нет компоновок по данному типу ВС" );
   xmlNodePtr compsNode = NewTextChild( dataNode, "comps"  );
   string StrVal;
   while ( !Qry->Eof ) {
     xmlNodePtr compNode = NewTextChild( compsNode, "comp" );     
     if ( !Qry->FieldIsNULL( "bort" ) && Qry->FieldAsInteger( "pr_comp" ) != 1 )
       StrVal = Qry->FieldAsString( "bort" );
     else
       StrVal = "  ";     
     StrVal += string( "  " ) + Qry->FieldAsString( "classes" );
     tst();
     if ( !Qry->FieldIsNULL( "descr" ) && Qry->FieldAsInteger( "pr_comp" ) != 1 )     
       StrVal += string( "  " ) + Qry->FieldAsString( "descr" );
     tst();
     if ( Qry->FieldAsInteger( "pr_comp" ) == 1 )
       StrVal += CurrName;     
     NewTextChild( compNode, "name", StrVal );
     NewTextChild( compNode, "comp_id", Qry->FieldAsInteger( "comp_id" ) );
     NewTextChild( compNode, "pr_comp", Qry->FieldAsInteger( "pr_comp" ) );
     NewTextChild( compNode, "craft", Qry->FieldAsString( "craft" ) );
     NewTextChild( compNode, "bort", Qry->FieldAsString( "bort" ) );
     NewTextChild( compNode, "classes", Qry->FieldAsString( "classes" ) );
     tst();
     NewTextChild( compNode, "descr", Qry->FieldAsString( "descr" ) );     
     tst();
     Qry->Next();
   }
   TSalons Salons;      
   Salons.trip_id = trip_id;
   Salons.ClName.clear();
   Salons.Read( rTripSalons );
   xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );     
   Salons.GetTripParams( trip_id, dataNode );     
   Salons.Build( salonsNode );
   tst();
   SelectPassengers( &Salons, Passengers );
   tst();
   if ( Passengers.existsNoSeats() )
     Passengers.Build( dataNode );
   tst();
  }
  catch( ... ) {
    OraSession.DeleteQuery( *Qry );
    throw;
  }
  OraSession.DeleteQuery( *Qry );  
  
}

void SalonsInterface::ExistsRegPassenger(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amRead );	
  bool SeatNoIsNull = NodeAsInteger( "SeatNoIsNull", reqNode );	
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  NewTextChild( resNode, "existsregpassengers", TSalons::InternalExistsRegPassenger( trip_id, SeatNoIsNull ) );
}

void SalonsInterface::SalonFormWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonsInterface::SalonFormWrite" );		
  TReqInfo::Instance()->user.check_access( amWrite );
  TQuery Qry( &OraSession );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  Qry.SQLText = "UPDATE trips set comp_id=:comp_id,trip_id=trip_id WHERE trip_id=:trip_id";
  Qry.DeclareVariable( "trip_id", otInteger );
  Qry.DeclareVariable( "comp_id", otInteger );
  Qry.SetVariable( "trip_id", trip_id );
  if ( comp_id == -2 )
    Qry.SetVariable( "comp_id", FNull );
  else
    Qry.SetVariable( "comp_id", comp_id );
  TSalons Salons;
  Salons.Parse( NodeAsNode( "salons", reqNode ) );  
  Salons.trip_id = trip_id;
  Salons.ClName = "";  
  Qry.Execute();
  Salons.Write( rTripSalons );
  bool pr_initcomp = NodeAsInteger( "initcomp", reqNode );
   
  if ( pr_initcomp ) { /* изменение компоновки */
    /* инициализация */                  
    Qry.Clear();
    Qry.SQLText = "BEGIN "\
                  " salons.initcomp( :trip_id ); "\
                  "END; ";
    Qry.DeclareVariable( "trip_id", otInteger );
    Qry.SetVariable( "trip_id", trip_id );
    Qry.Execute();
    /* запись в лог */
    xmlNodePtr refcompNode = NodeAsNode( "refcompon", reqNode );
    string msg = string( "Изменена компоновка рейса. Классы: " ) + 
                 NodeAsString( "ref", refcompNode );
    xmlNodePtr ctypeNode = NodeAsNode( "ctype", refcompNode );
    bool cBase = false;
    bool cChange = false;
    if ( ctypeNode ) {
      ctypeNode = ctypeNode->children; /* value */
      while ( ctypeNode ) {
      	string stype = NodeAsString( ctypeNode );
        cBase = ( stype == string( "cBase" ) );
        cChange = ( stype == string( "cChange" ) );
        ctypeNode = ctypeNode->next;
      }
    }
    if ( cBase ) {
      msg = string( "Назначена базовая компоновка (ид=" ) + 
            IntToString( comp_id ) + 
            "). Классы: " + NodeAsString( "classes", refcompNode );
      if ( cChange )
        msg = string( "Назначена компоновка рейса. Классы: " ) + 
              NodeAsString( "ref", refcompNode );
    }               
    msg += string( ", кодировка: " ) + NodeAsString( "lang", refcompNode );
    TReqInfo::Instance()->MsgToLog( msg, evtFlt, trip_id );
    /* перечитываение компоновки из БД */
    Salons.Read(  rTripSalons );              
  }
  if ( TSalons::InternalExistsRegPassenger( trip_id, false ) ) { /* есть зарегистрированные пассажиры */
    /* рассаживаем, записываем */
    ReSeats( &Salons, !pr_initcomp, false ); /* при старой компоновке удаляем занятые места */
    Salons.Write( rTripSalons ); /* сохранение салона с пересаженными пассажирами */     
  }    
  Qry.Clear();
  Qry.SQLText = 
     "BEGIN "\
     "DELETE trip_classes WHERE trip_id = :trip_id; "\
     "INSERT INTO trip_classes(trip_id,class,cfg,block,prot) "
     " SELECT :trip_id, class, NVL( SUM( DECODE( class, NULL, 0, 1 ) ), 0 ), "
     "        NVL( SUM( DECODE( class, NULL, 0, DECODE( enabled, NULL, 1, 0 ) ) ), 0 ), 0 "\
     "  FROM trip_comp_elems, comp_elem_types "\
     " WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "\
     "       comp_elem_types.pr_seat <> 0 AND "\
     "       trip_comp_elems.trip_id = :trip_id "\
     " GROUP BY class; "\
     "ckin.recount( :trip_id ); "\
     "END; ";
  Qry.DeclareVariable( "trip_id", otInteger );
  Qry.SetVariable( "trip_id", trip_id );
  Qry.Execute();    
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  TSalons::GetTripParams( trip_id, dataNode );        
  Salons.Build( salonsNode );
  tst();
  if ( Passengers.existsNoSeats() ) {
    tst();
    Passengers.Build( dataNode );   
  }
}

void SalonsInterface::Reseat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amWrite );	
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  int pax_id = NodeAsInteger( "pax_id", reqNode );	
  int num = NodeAsInteger( "num", reqNode );	
  int x = NodeAsInteger( "x", reqNode );	
  int y = NodeAsInteger( "y", reqNode );	
  ProgTrace(TRACE5, "SalonsInterface::Reseat, trip_id=%d, pax_id=%d", trip_id, pax_id );			
  TQuery Qry( &OraSession );
  /* лочим рейс */
  Qry.SQLText = "UPDATE trips set trip_id=trip_id WHERE trip_id=:trip_id";
  Qry.DeclareVariable( "trip_id", otInteger );
  Qry.SetVariable( "trip_id", trip_id );
  Qry.Execute();
  tst();
  /* считываем инфу по пассажиру */
  Qry.Clear();
  Qry.SQLText = "SELECT seat_no, prev_seat_no, seats, a.step step, surname, name,"\
                "       reg_no, grp_id "\
                " FROM pax,"\
                "( SELECT COUNT(*) step FROM pax_rem "\
                "   WHERE rem_code = 'STCR' AND pax_id=:pax_id ) a "\
                "WHERE pax.pax_id=:pax_id ";
  Qry.DeclareVariable( "pax_id", otInteger ); 
  Qry.SetVariable( "pax_id", pax_id );
  Qry.Execute();
  tst();  
  string placeName = Qry.FieldAsString( "seat_no" );
  string prevplaceName = Qry.FieldAsString( "prev_seat_no" );
  string fname = Qry.FieldAsString( "surname" );     
  string fullname = TrimString( fname ) + Qry.FieldAsString( "name" ); 
  TSeatStep Step;
  if ( Qry.FieldAsInteger( "step" ) )
    Step = sDown;
  else
    Step = sRight;
  int reg_no = Qry.FieldAsInteger( "reg_no" );
  int grp_id = Qry.FieldAsInteger( "grp_id" );
  /* считываем инфу по новому месту */
  Qry.Clear();
  Qry.SQLText = "SELECT yname||xname placename FROM trip_comp_elems "\
                " WHERE trip_id=:trip_id AND num=:num AND x=:x AND y=:y AND pr_free IS NOT NULL";
  Qry.DeclareVariable( "trip_id", otInteger );
  Qry.DeclareVariable( "num", otInteger );  
  Qry.DeclareVariable( "x", otInteger );
  Qry.DeclareVariable( "y", otInteger );  
  Qry.SetVariable( "trip_id", trip_id );
  Qry.SetVariable( "num", num );  
  Qry.SetVariable( "x", x );
  Qry.SetVariable( "y", y );
  Qry.Execute();
  tst();  
  try {
    if ( !Qry.RowCount() )
      throw 1;
    string nplaceName = Qry.FieldAsString( "placename" );      
    /* определяем было ли старое место */
    Qry.Clear();
    Qry.SQLText = "SELECT COUNT(*) c FROM trip_comp_elems "\
                  " WHERE trip_id=:trip_id AND yname||xname=:placename AND "
                  "       pr_free IS NULL AND rownum<=1";
    Qry.DeclareVariable( "trip_id", otInteger );                  
    Qry.DeclareVariable( "placename", otString );
    Qry.SetVariable( "trip_id", trip_id );
    Qry.SetVariable( "placename", placeName );
    Qry.Execute();
    int InUse = Qry.FieldAsInteger( "c" ) + 1; /* 1-посадка,2-пересадка */
    ProgTrace( TRACE5, "InUSe=%d, oldplace=%s, newplace=%s 1-seats, 2-reseats", 
               InUse, placeName.c_str(), nplaceName.c_str() );
    TReqInfo *reqinfo = TReqInfo::Instance();
    Qry.Clear();
    Qry.SQLText = "BEGIN "\
                  " salons.seatpass( :trip_id, :pax_id, :placename, :whatdo ); "\
                  " UPDATE pax SET seat_no=:placename,prev_seat_no=:placename,tid=tid__seq.nextval "\
                  "  WHERE pax_id=:pax_id; "\
                  " mvd.sync_pax(:pax_id,:term); "\
                  "END; ";
    Qry.DeclareVariable( "trip_id", otInteger );
    Qry.DeclareVariable( "pax_id", otInteger );
    Qry.DeclareVariable( "placename", otString );
    Qry.DeclareVariable( "whatdo", otInteger );
    Qry.DeclareVariable( "term", otString );    
    Qry.SetVariable( "trip_id", trip_id );
    Qry.SetVariable( "pax_id", pax_id );
    Qry.SetVariable( "placename", nplaceName );
    Qry.SetVariable( "whatdo", InUse );
    Qry.SetVariable( "term", reqinfo->desk.code );
    try {
      Qry.Execute(); 
    }
    catch( EOracleError e ) {
      tst();
      if ( e.Code == 1403 ) {        
        throw 1;
      }
      else throw;
    }
    reqinfo->MsgToLog( string( "Пассажир " ) + fullname + 
                       " пересажен. Новое место: " + nplaceName,
                       evtPax, trip_id, reg_no, grp_id );      
  }
  catch( int i ) {
    if ( i != 1 )
      throw;
    /* данные на клиенте устарели, надо обновить их */
     tst();
     TSalons Salons;
     Salons.trip_id = trip_id;
     Salons.Read( rTripSalons );
     xmlNodePtr dataNode = NewTextChild( resNode, "data" );
     xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
     TSalons::GetTripParams( trip_id, dataNode );
     Salons.Build( salonsNode );
     tst();
     SelectPassengers( &Salons, Passengers );
     tst();
     if ( Passengers.existsNoSeats() )
       Passengers.Build( dataNode );   
     showErrorMessageAndRollback( "Пересадка невозможна" );       
  }
};

void SalonsInterface::AutoReseatsPassengers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo::Instance()->user.check_access( amWrite );	
  int trip_id = NodeAsInteger( "trip_id", reqNode );	
  ProgTrace(TRACE5, "SalonsInterface::AutoReseatsPassengers, trip_id=%d", trip_id );				  
  TQuery Qry( &OraSession );
  /* лочим рейс */
  Qry.SQLText = "UPDATE trips set trip_id=trip_id WHERE trip_id=:trip_id";
  Qry.DeclareVariable( "trip_id", otInteger );
  Qry.SetVariable( "trip_id", trip_id );
  Qry.Execute();
  tst();  
  TSalons Salons;
  Salons.trip_id = trip_id;  
  Salons.Read( rTripSalons );    
  SelectPassengers( &Salons, Passengers );
  ReSeats( &Salons, true, true ); /* рассадка в новый салон, true - удаление занятых мест в салоне */
  Salons.Write( rTripSalons ); /* сохранение самого салона с пересаженными местами */
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  TSalons::GetTripParams( trip_id, dataNode );  
  Salons.Build( salonsNode );
  if ( Passengers.existsNoSeats() ) {
    tst();
    Passengers.Build( dataNode );  
  }	  
  tst();
}

void SalonsInterface::BaseComponFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int comp_id = NodeAsInteger( "comp_id", reqNode );	
  ProgTrace(TRACE5, "SalonsInterface::BaseComponFormShow, comp_id=%d", comp_id );				  
  TReqInfo::Instance()->user.check_access( amRead );
  TSalons Salons;
  Salons.comp_id = comp_id;
  Salons.Read( rComponSalons );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );  
  TSalons::GetCompParams( comp_id, dataNode );
  Salons.Build( salonsNode );
}

void SalonsInterface::BaseComponFormWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  ProgTrace( TRACE5, "SalonsInterface::BaseComponFormWrite, comp_id=%d", comp_id );
  TReqInfo::Instance()->user.check_access( amWrite );
  TSalons Salons;
  Salons.Parse( GetNode( "salons", reqNode ) );
  Salons.comp_id = NodeAsInteger( "comp_id", reqNode );
  string smodify = NodeAsString( "modify", reqNode );
  if ( smodify == "delete" )
    Salons.modify = mDelete;
  else
    if ( smodify == "add" )
      Salons.modify = mAdd;
    else
      if ( smodify == "change" )
        Salons.modify = mChange;
      else
        throw Exception( string( "Ошибка в значении тега modify " ) + smodify );
  Salons.craft = NodeAsString( "craft", reqNode );
  Salons.bort = NodeAsString( "bort", reqNode );
  Salons.descr = NodeAsString( "descr", reqNode );
  string classes = NodeAsString( "classes", reqNode );
  Salons.classes = RTrimString( classes );
  if ( Salons.craft.empty() )
    throw UserException( "Не задан тип ВС" );
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT bc FROM bc_code WHERE bc=:bc";
  Qry.DeclareVariable( "bc", otString );
  Qry.SetVariable( "bc", Salons.craft );
  Qry.Execute();
  if ( !Qry.RowCount() )
    throw UserException( "Неправильно задан тип ВС" );
  Salons.Write( rComponSalons );
  string msg;
  switch ( Salons.modify ) {
    case mDelete: 
      msg = string( "Удалена базовая компоновка (ид=" ) + IntToString( comp_id ) + ").";
      Salons.comp_id = -1;
      break;
    default:
      if ( Salons.modify == mAdd )
        msg = "Создана базовая компоновка (ид=";
      else
        msg = "Изменена базовая компоновка (ид=";
      msg += IntToString( Salons.comp_id );
      msg += "). Тип ВС: " + Salons.craft + ", борт: ";
      if ( Salons.bort.empty() )
        msg += "не указан";
      else
        msg += Salons.bort;
      msg += ", классы: " + Salons.classes + ", описание: ";
      if ( Salons.descr.empty() )
        msg += "не указано";
      else
        msg += Salons.descr;
      break;
  }
  TReqInfo::Instance()->MsgToLog( msg, evtComp, comp_id );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  NewTextChild( dataNode, "comp_id", Salons.comp_id );
  showMessage( "Изменения успешно сохранены" );
}

void SalonsInterface::BaseComponsRead(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "SalonsInterface::BaseComponsRead" );
  TReqInfo::Instance()->user.check_access( amRead );
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT comp_id,craft,bort,descr,classes FROM comps "\
                " ORDER BY craft,comp_id";
  Qry.Execute();
  xmlNodePtr node = NewTextChild( resNode, "data" );
  node = NewTextChild( node, "compons" );
  while ( !Qry.Eof ) {
    xmlNodePtr rnode = NewTextChild( node, "compon" );
    NewTextChild( rnode, "comp_id", Qry.FieldAsInteger( "comp_id" ) );
    NewTextChild( rnode, "craft", Qry.FieldAsString( "craft" ) );
    NewTextChild( rnode, "bort", Qry.FieldAsString( "bort" ) );
    NewTextChild( rnode, "descr", Qry.FieldAsString( "descr" ) );
    NewTextChild( rnode, "classes", Qry.FieldAsString( "classes" ) );
    Qry.Next();
  }
	}



void SalonsInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
