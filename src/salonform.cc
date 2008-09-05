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
#include "convert.h"

const char CurrName[] = " (���.)";

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;

bool filterComp( const string &airline, const string &airp );


void SalonsInterface::CheckInShow( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonsInterface::CheckInShow" );
  //TReqInfo::Instance()->user.check_access( amRead );
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
  SALONS::GetTripParams( Salons.trip_id, dataNode );
  Salons.Build( NewTextChild( dataNode, "salons" ) );
};

void SalonsInterface::SalonFormShow(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonsInterface::SalonFormShow" );
  //TReqInfo::Instance()->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  ProgTrace( TRACE5, "trip_id=%d", trip_id );
  TQuery Qry( &OraSession );
  SALONS::GetTripParams( trip_id, dataNode );
  Qry.SQLText = "SELECT airline FROM points WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  if ( !Qry.RowCount() )
  	throw UserException( "���� �� ������" );
  string trip_airline = Qry.FieldAsString( "airline" );
  Qry.Clear();
  Qry.SQLText = "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "\
                "       comps.descr,0 as pr_comp, comps.airline, comps.airp "\
                " FROM comps, points "\
                "WHERE points.craft = comps.craft AND points.point_id = :point_id "\
                "UNION "\
                "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "\
                "       comps.descr,1 as pr_comp, null, null "\
                " FROM comps, points, trip_sets "\
                "WHERE points.point_id=trip_sets.point_id AND "\
                "      points.craft = comps.craft AND points.point_id = :point_id AND "\
                "      trip_sets.comp_id = comps.comp_id "\
                "UNION "\
                "SELECT -1, craft, bort, "\
                "        LTRIM(RTRIM( DECODE( a.f, 0, '', ' �'||a.f)||"\
                "        DECODE( a.c, 0, '', ' �'||a.c)|| "\
                "        DECODE( a.y, 0, '', ' �'||a.y) )) classes, "\
                "        null,1, null, null "\
                "FROM "\
                "(SELECT -1, craft, bort, "\
                "        NVL( SUM( DECODE( class, '�', 1, 0 ) ), 0 ) as f, "\
                "        NVL( SUM( DECODE( class, '�', 1, 0 ) ), 0 ) as c, "\
                "        NVL( SUM( DECODE( class, '�', 1, 0 ) ), 0 ) as y "\
                "  FROM trip_comp_elems, comp_elem_types, points, trip_sets "\
                " WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "
                "       comp_elem_types.pr_seat <> 0 AND "\
                "       trip_comp_elems.point_id = points.point_id AND "\
                "       points.point_id = :point_id AND "\
                "       trip_sets.point_id(+) = points.point_id AND "\
                "       trip_sets.comp_id IS NULL "\
                "GROUP BY craft, bort) a "\
                "ORDER BY comp_id, craft, bort, classes, descr";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", trip_id );
  Qry.Execute();
  try {
    xmlNodePtr compsNode = NULL;
    string StrVal;
    while ( !Qry.Eof ) {
    	if ( Qry.FieldAsInteger( "pr_comp" ) || /* ���� ���������� ⮫쪮 �� ����������� �㦭�� �/� ��� ���⮢� ����������� */
    		   ( Qry.FieldIsNULL( "airline" ) || trip_airline == Qry.FieldAsString( "airline" ) ) &&
    		   filterComp( Qry.FieldAsString( "airline" ), Qry.FieldAsString( "airp" ) ) ) {
      	if ( !compsNode )
      		compsNode = NewTextChild( dataNode, "comps"  );
        xmlNodePtr compNode = NewTextChild( compsNode, "comp" );
        if ( !Qry.FieldIsNULL( "airline" ) )
        	StrVal = Qry.FieldAsString( "airline" );
        else
        	StrVal = Qry.FieldAsString( "airp" );
        if ( StrVal.length() == 2 )        	
          StrVal += "  ";
        else
        	StrVal += " ";
        if ( !Qry.FieldIsNULL( "bort" ) && Qry.FieldAsInteger( "pr_comp" ) != 1 )
          StrVal += Qry.FieldAsString( "bort" );
        else
          StrVal += "  ";
        StrVal += string( "  " ) + Qry.FieldAsString( "classes" );
        if ( !Qry.FieldIsNULL( "descr" ) && Qry.FieldAsInteger( "pr_comp" ) != 1 )
          StrVal += string( "  " ) + Qry.FieldAsString( "descr" );
        if ( Qry.FieldAsInteger( "pr_comp" ) == 1 )
          StrVal += CurrName;
        NewTextChild( compNode, "name", StrVal );
        NewTextChild( compNode, "comp_id", Qry.FieldAsInteger( "comp_id" ) );
        NewTextChild( compNode, "pr_comp", Qry.FieldAsInteger( "pr_comp" ) );
        NewTextChild( compNode, "craft", Qry.FieldAsString( "craft" ) );
        NewTextChild( compNode, "bort", Qry.FieldAsString( "bort" ) );
        NewTextChild( compNode, "classes", Qry.FieldAsString( "classes" ) );
        NewTextChild( compNode, "descr", Qry.FieldAsString( "descr" ) );
      }
      Qry.Next();
    }
    if ( !compsNode )
      throw UserException( "��� ���������� �� ������� ⨯� ��" );    
   tst();
   TSalons Salons;
   Salons.trip_id = trip_id;
   Salons.ClName.clear();
   tst();
   Salons.Read( rTripSalons );
   xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
   Salons.Build( salonsNode );
   SEATS::SelectPassengers( &Salons, Passengers );
   tst();
   if ( Passengers.existsNoSeats() )
     Passengers.Build( dataNode );
 }
 catch( UserException ue ) {
   showErrorMessage( ue.what() );
   tst();
 }
}

void SalonsInterface::ExistsRegPassenger(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //TReqInfo::Instance()->user.check_access( amRead );
  bool SeatNoIsNull = NodeAsInteger( "SeatNoIsNull", reqNode );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  NewTextChild( resNode, "existsregpassengers", SALONS::InternalExistsRegPassenger( trip_id, SeatNoIsNull ) );
}

void SalonsInterface::SalonFormWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE5, "SalonsInterface::SalonFormWrite" );
  //TReqInfo::Instance()->user.check_access( amWrite );
  TQuery Qry( &OraSession );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.SQLText = "UPDATE points SET point_id=point_id WHERE point_id=:point_id";
  Qry.Execute();
  Qry.SQLText = "UPDATE trip_sets SET comp_id=:comp_id WHERE point_id=:point_id";
  Qry.DeclareVariable( "comp_id", otInteger );
  if ( comp_id == -2 )
    Qry.SetVariable( "comp_id", FNull );
  else
    Qry.SetVariable( "comp_id", comp_id );
  TSalons Salons;
  Salons.Parse( NodeAsNode( "salons", reqNode ) );
  Salons.verifyValidRem( "MCLS", "�"); //???
  Salons.trip_id = trip_id;
  Salons.ClName = "";
  Qry.Execute();
  tst();
  Salons.Write( rTripSalons );
  tst();
  bool pr_initcomp = NodeAsInteger( "initcomp", reqNode );

  if ( pr_initcomp ) { /* ��������� ���������� */
    /* ���樠������ */
    Qry.Clear();
    Qry.SQLText = "BEGIN "\
                  " salons.initcomp( :point_id, 1 ); "\
                  "END; ";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.SetVariable( "point_id", trip_id );
    Qry.Execute();
    /* ������ � ��� */
    xmlNodePtr refcompNode = NodeAsNode( "refcompon", reqNode );
    string msg = string( "�������� ���������� ३�. ������: " ) +
                 NodeAsString( "classes", refcompNode );
    xmlNodePtr ctypeNode = NodeAsNode( "ctype", refcompNode );
    bool cBase = false;
    bool cChange = false;
    if ( ctypeNode ) {
    	tst();
      ctypeNode = ctypeNode->children; /* value */
      while ( ctypeNode ) {
      	string stype = NodeAsString( ctypeNode );
        cBase = ( stype == string( "cBase" ) || cBase );
        cChange = ( stype == string( "cChange" ) || cChange );
        ctypeNode = ctypeNode->next;
      }
    }
    ProgTrace( TRACE5, "cBase=%d, cChange=%d", cBase, cChange );
    if ( cBase ) {
      msg = string( "�����祭� ������� ���������� (��=" ) +
            IntToString( comp_id ) +
            "). ������: " + NodeAsString( "classes", refcompNode );
      if ( cChange )
        msg = string( "�����祭� ���������� ३�. ������: " ) +
              NodeAsString( "classes", refcompNode );
    }
    msg += string( ", ����஢��: " ) + NodeAsString( "lang", refcompNode );
    TReqInfo::Instance()->MsgToLog( msg, evtFlt, trip_id );
    /* �����뢠���� ���������� �� �� */
    Salons.Read(  rTripSalons );
  }
  else {
    /* ���樠������ */
    tst();
    Qry.Clear();
    Qry.SQLText = "BEGIN "\
                  " salons.initcomp( :point_id, 0 ); "\
                  "END; ";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.SetVariable( "point_id", trip_id );
    Qry.Execute(); 
    xmlNodePtr refcompNode = NodeAsNode( "refcompon", reqNode );
    string msg = string( "�������� ���������� ३�. ������: " ) +
                 NodeAsString( "classes", refcompNode );     	
    msg += string( ", ����஢��: " ) + NodeAsString( "lang", refcompNode );
    TReqInfo::Instance()->MsgToLog( msg, evtFlt, trip_id );   
    /* �����뢠���� ���������� �� �� */	
    Salons.Read(  rTripSalons );              
  }
  Passengers.Clear();
  if ( SALONS::InternalExistsRegPassenger( trip_id, false ) ) { /* ���� ��ॣ����஢���� ���ᠦ��� */
    /* ��ᠦ�����, �����뢠�� */
    SEATS::ReSeatsPassengers( &Salons, !pr_initcomp, false ); /* �� ��ன ���������� 㤠�塞 ������ ���� */
    Salons.Write( rTripSalons ); /* ��࠭���� ᠫ��� � ���ᠦ���묨 ���ᠦ�ࠬ� */
  }
  Qry.Clear();
  Qry.SQLText =
     "BEGIN "\
     "DELETE trip_classes WHERE point_id = :point_id; "\
     "INSERT INTO trip_classes(point_id,class,cfg,block,prot) "
     " SELECT :point_id, class, NVL( SUM( DECODE( class, NULL, 0, 1 ) ), 0 ), "
     "        NVL( SUM( DECODE( class, NULL, 0, DECODE( enabled, NULL, 1, 0 ) ) ), 0 ), 0 "\
     "  FROM trip_comp_elems, comp_elem_types "\
     " WHERE trip_comp_elems.elem_type = comp_elem_types.code AND "\
     "       comp_elem_types.pr_seat <> 0 AND "\
     "       trip_comp_elems.point_id = :point_id "\
     " GROUP BY class; "\
     "ckin.recount( :point_id ); "\
     "END; ";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", trip_id );
  Qry.Execute();
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  SALONS::GetTripParams( trip_id, dataNode );
  Salons.Build( salonsNode );
  tst();
  if ( Passengers.existsNoSeats() ) {
    tst();
    Passengers.Build( dataNode );
  }
}

void SalonsInterface::DeleteReserveSeat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "trip_id", reqNode );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  TQuery Qry( &OraSession );    
  Qry.SQLText = 
    "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("���� �� ������. ������� �����");
  bool pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );	
  
/*  string xname, yname;
  xmlNodePtr n = GetNode( "placename", reqNode );
  bool seats_exists = true;
  if ( n ) {
  	string placeName = NodeAsString( n );
    TQuery Qry( &OraSession );  	
    Qry.SQLText = 
      "SELECT xname, yname FROM trip_comp_elems "
       " WHERE point_id=:point_id AND yname||xname=DECODE( INSTR( :placename, '0' ), 1, SUBSTR( :placename, 2 ), :placename )";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "placename", otString, placeName );
    Qry.Execute();
    seats_exists = ( Qry.RowCount() );    	
    if ( seats_exists ) {
    	xname = Qry.FieldAsString( "xname" );
    	yname = Qry.FieldAsString( "yname" );
    }
  }
  else {
  	xname = NodeAsString( "xname", reqNode );
  	yname = NodeAsString( "yname", reqNode );
  }  */
  
	ProgTrace(TRACE5, "SalonsInterface::DeleteReserveSeat, point_id=%d, pax_id=%d, tid=%d", point_id, pax_id, tid );    
	
  TSalons Salons;
  Salons.trip_id = point_id;
  Salons.Read( rTripSalons );
  vector<SALONS::TSalonSeat> seats;
	
  try {
  	SEATS::ChangeLayer( cltPreseat, point_id, pax_id, tid, "", "", stDropseat, pr_lat_seat );
  	SALONS::getSalonChanges( Salons, seats );
  	Qry.Clear();
  	Qry.SQLText =
  	  "SELECT "
      "  salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS crs_seat_no, "     
      "  salons.get_crs_seat_no(crs_pax.pax_id,:preseat_layer,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS preseat_no, "          
      "  salons.get_seat_no(pax.pax_id,:checkin_layer,pax.seats,pax_grp.point_dep,'seats',rownum) AS seat_no "          
      "FROM crs_pnr,crs_pax,pax,pax_grp "
      "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      pax.grp_id=pax_grp.grp_id(+) AND "
      "      crs_pax.pax_id=:pax_id";
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "preseat_layer", otString, EncodeCompLayerType(ASTRA::cltPreseat) );
    Qry.CreateVariable( "checkin_layer", otString, EncodeCompLayerType(ASTRA::cltCheckin) );    
    Qry.Execute();
    if ( Qry.Eof )
    	throw UserException( "���ᠦ�� �� ������" );    
    /* ���� ��।��� ����� ���� tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !Qry.FieldIsNULL( "crs_seat_no" ) )
    	NewTextChild( dataNode, "crs_seat_no", Qry.FieldAsString( "crs_seat_no" ) );
    if ( !Qry.FieldIsNULL( "preseat_no" ) )
    	NewTextChild( dataNode, "preseat_no", Qry.FieldAsString( "preseat_no" ) );
    if ( !Qry.FieldIsNULL( "seat_no" ) )
    	NewTextChild( dataNode, "seat_no", Qry.FieldAsString( "seat_no" ) );
   	SALONS::BuildSalonChanges( dataNode, seats );
  }
  catch( UserException ue ) {
    TSalons Salons;
    Salons.trip_id = point_id;
    Salons.Read( rTripSalons );
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );    
    SALONS::GetTripParams( point_id, dataNode );
    Salons.Build( salonsNode );
  	showErrorMessageAndRollback( ue.what() );
  }  
}

void SalonsInterface::Reseat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //TReqInfo::Instance()->user.check_access( amWrite );
  TSeatsType seat_type = stReseat;
  int point_id = NodeAsInteger( "trip_id", reqNode );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  string xname, yname;
  xmlNodePtr n = GetNode( "num", reqNode );
  TQuery Qry( &OraSession );  
  
  Qry.SQLText = 
    "SELECT pr_lat_seat FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.Execute();
  if ( Qry.Eof ) throw UserException("���� �� ������. ������� �����");
  bool pr_lat_seat = Qry.FieldAsInteger( "pr_lat_seat" );	
  
  if ( n ) {
    int num = NodeAsInteger( "num", reqNode );
    int x = NodeAsInteger( "x", reqNode );
    int y = NodeAsInteger( "y", reqNode );
    Qry.SQLText = 
      "SELECT xname, yname FROM trip_comp_elems "
       " WHERE point_id=:point_id AND num=:num AND x=:x AND y=:y";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.CreateVariable( "num", otInteger, num );
    Qry.CreateVariable( "x", otInteger, x );
    Qry.CreateVariable( "y", otInteger, y );
    Qry.Execute();
    if ( Qry.RowCount() ) {
    	xname = Qry.FieldAsString( "xname" );
    	yname = Qry.FieldAsString( "yname" );
    }     
  }
  else {
  	xname = norm_iata_line( NodeAsString( "xname", reqNode ) );
  	yname = norm_iata_row( NodeAsString( "yname", reqNode ) );  	
  }
  TCompLayerType layer_type;
  if ( GetNode( "checkin", reqNode ) )
  	layer_type = cltCheckin;
  else
  	if ( GetNode( "reserve", reqNode ) )
  		layer_type = cltPreseat;
  	else layer_type = cltUnknown;
  ProgTrace(TRACE5, "SalonsInterface::Reseat, point_id=%d, pax_id=%d, tid=%d", point_id, pax_id, tid );
  
  TSalons Salons;
  Salons.trip_id = point_id;
  Salons.Read( rTripSalons );
  vector<SALONS::TSalonSeat> seats;
  
  try {  		
  	SEATS::ChangeLayer( cltPreseat, point_id, pax_id, tid, xname, yname, seat_type, pr_lat_seat );
  	SALONS::getSalonChanges( Salons, seats );
  	Qry.Clear();
  	Qry.SQLText =
  	  "SELECT "
      "  salons.get_crs_seat_no(crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS crs_seat_no, "     
      "  salons.get_crs_seat_no(crs_pax.pax_id,:preseat_layer,crs_pax.seats,crs_pnr.point_id,'seats',rownum) AS preseat_no, "          
      "  salons.get_seat_no(pax.pax_id,:checkin_layer,pax.seats,pax_grp.point_dep,'seats',rownum) AS seat_no "          
      "FROM crs_pnr,crs_pax,pax,pax_grp "
      "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
      "      crs_pax.pax_id=pax.pax_id(+) AND "
      "      pax.grp_id=pax_grp.grp_id(+) AND "
      "      crs_pax.pax_id=:pax_id";
    Qry.CreateVariable( "pax_id", otInteger, pax_id );
    Qry.CreateVariable( "preseat_layer", otString, EncodeCompLayerType(ASTRA::cltPreseat) );
    Qry.CreateVariable( "checkin_layer", otString, EncodeCompLayerType(ASTRA::cltCheckin) );    
    Qry.Execute();
    if ( Qry.Eof )
    	throw UserException( "���ᠦ�� �� ������" );    
    /* ���� ��।��� ����� ���� tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    if ( !Qry.FieldIsNULL( "crs_seat_no" ) )
    	NewTextChild( dataNode, "crs_seat_no", Qry.FieldAsString( "crs_seat_no" ) );
    if ( !Qry.FieldIsNULL( "preseat_no" ) )
    	NewTextChild( dataNode, "preseat_no", Qry.FieldAsString( "preseat_no" ) );
    if ( !Qry.FieldIsNULL( "seat_no" ) )
    	NewTextChild( dataNode, "seat_no", Qry.FieldAsString( "seat_no" ) );  		
    /* ���� ��।��� ����� ���� tid */
    NewTextChild( dataNode, "tid", tid );
    NewTextChild( dataNode, "placename", denorm_iata_row( yname ) + denorm_iata_line( xname, pr_lat_seat ) );        
    SALONS::BuildSalonChanges( dataNode, seats );
  }
  catch( UserException ue ) {
    tst();
    TSalons Salons;
    Salons.trip_id = point_id;
    Salons.Read( rTripSalons );
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );    
    SALONS::GetTripParams( point_id, dataNode );
    Salons.Build( salonsNode );
/*???    
    if ( !checkinNode ) {
      if ( errmsg.empty() )
      	if ( setseatNode )
      		errmsg = "���������� �।���⥫쭮� �����祭�� ������� ����. ������� �����";
      	else {
          errmsg = "���ᠤ�� ����������. ������� �����";
          SEATS::SelectPassengers( &Salons, Passengers );
          tst();
          if ( Passengers.existsNoSeats() )
            Passengers.Build( dataNode );
        }
    }
    else
    	if ( errmsg.empty() )
    		errmsg = "���ᠤ�� ����������";*/
  	showErrorMessageAndRollback( ue.what() );
  }
  
};

void SalonsInterface::AutoReseatsPassengers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //TReqInfo::Instance()->user.check_access( amWrite );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  ProgTrace(TRACE5, "SalonsInterface::AutoReseatsPassengers, trip_id=%d", trip_id );
  TQuery Qry( &OraSession );
  /* ��稬 ३� */
  Qry.SQLText = "UPDATE points SET point_id=point_id WHERE point_id=:point_id";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", trip_id );
  Qry.Execute();
  tst();
  TSalons Salons;
  Salons.trip_id = trip_id;
  Salons.Read( rTripSalons );
  SEATS::SelectPassengers( &Salons, Passengers );
  SEATS::ReSeatsPassengers( &Salons, true, true ); /* ��ᠤ�� � ���� ᠫ��, true - 㤠����� ������� ���� � ᠫ��� */
  Salons.Write( rTripSalons ); /* ��࠭���� ᠬ��� ᠫ��� � ���ᠦ���묨 ���⠬� */
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  SALONS::GetTripParams( trip_id, dataNode );
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
  //TReqInfo::Instance()->user.check_access( amRead );
  TSalons Salons;
  Salons.comp_id = comp_id;
  Salons.Read( rComponSalons );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
  SALONS::GetCompParams( comp_id, dataNode );
  Salons.Build( salonsNode );
}

void SalonsInterface::BaseComponFormWrite(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int comp_id = NodeAsInteger( "comp_id", reqNode );
  ProgTrace( TRACE5, "SalonsInterface::BaseComponFormWrite, comp_id=%d", comp_id );
  //TReqInfo::Instance()->user.check_access( amWrite );
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
        throw Exception( string( "�訡�� � ���祭�� ⥣� modify " ) + smodify );       
  TReqInfo *r = TReqInfo::Instance();          
  xmlNodePtr a = GetNode( "airline", reqNode );
  if ( a )
   Salons.airline = NodeAsString( a );
  else 
  	if ( r->user.access.airlines.size() == 1 )
  		Salons.airline = *r->user.access.airlines.begin();
 	a = GetNode( "airp", reqNode );
 	if ( a ) {
 		Salons.airp = NodeAsString( a );
 		Salons.airline.clear();
 	}
 	else
  	if ( r->user.user_type != utAirline && r->user.access.airps.size() == 1 && !GetNode( "airline", reqNode ) ) {
  		Salons.airp = *r->user.access.airps.begin();
  		Salons.airline.clear();
    }
  TQuery Qry( &OraSession );    
  if ( Salons.modify != mDelete ) {
    if ( !Salons.airline.empty() ) {
      Qry.SQLText = "SELECT code FROM airlines WHERE code=:airline";
      Qry.CreateVariable( "airline", otString, Salons.airline );
      Qry.Execute();
      if ( !Qry.RowCount() )
        throw UserException( "���ࠢ��쭮 ����� ��� ������������" );  
    }
    if ( !Salons.airp.empty() ) {
      Qry.Clear();
      Qry.SQLText = "SELECT code FROM airps WHERE code=:airp";
      Qry.CreateVariable( "airp", otString, Salons.airp );
      Qry.Execute();
      if ( !Qry.RowCount() )
        throw UserException( "���ࠢ��쭮 ����� ��� ��ய���" );    
    }
  
    if ( (int)Salons.airline.empty() + (int)Salons.airp.empty() != 1 ) { 
    	if ( Salons.airline.empty() )
    	  throw UserException( "������ ���� ����� ��� ������������ ��� ��� ��ய���" );
    	else
    		throw UserException( "�����६����� ������� ������������ � ��ய��� ����饭�" ); // ??? ��祬�?
    }
  
    if ( ( r->user.user_type == utAirline ||
           r->user.user_type == utSupport && Salons.airp.empty() && !r->user.access.airlines.empty() ) &&
    	   find( r->user.access.airlines.begin(), 
    	         r->user.access.airlines.end(), Salons.airline ) == r->user.access.airlines.end() ) {
 	  	if ( Salons.airline.empty() )
 		  	throw UserException( "�� ����� ��� ������������" );
  	  else
    		throw UserException( "� ������ ��� �ࠢ ����� ���������� ��� �������� ������������" );         
    }
    if ( ( r->user.user_type == utAirport || 
    	     r->user.user_type == utSupport && Salons.airline.empty() && !r->user.access.airps.empty() ) &&
    	   find( r->user.access.airps.begin(), 
    	         r->user.access.airps.end(), Salons.airp ) == r->user.access.airps.end() ) {
 	  	if ( Salons.airp.empty() )
 	  		throw UserException( "�� ����� ��� ��ய���" );
 	  	else
 	  	  throw UserException( "� ������ ��� �ࠢ ����� ���������� ��� ��������� ��ய���" ); 		   	  	         	
    }
  }	
  Salons.craft = NodeAsString( "craft", reqNode );
  Salons.bort = NodeAsString( "bort", reqNode );
  Salons.descr = NodeAsString( "descr", reqNode );
  string classes = NodeAsString( "classes", reqNode );
  Salons.classes = RTrimString( classes );
  if ( Salons.craft.empty() )
    throw UserException( "�� ����� ⨯ ��" );    
  Qry.Clear();
  Qry.SQLText = "SELECT code FROM crafts WHERE code=:craft";
  Qry.DeclareVariable( "craft", otString );
  Qry.SetVariable( "craft", Salons.craft );
  Qry.Execute();
  if ( !Qry.RowCount() )
    throw UserException( "���ࠢ��쭮 ����� ⨯ ��" );
  Salons.verifyValidRem( "MCLS", "�" );
  Salons.Write( rComponSalons );
  string msg;
  switch ( Salons.modify ) {
    case mDelete:
      msg = string( "������� ������� ���������� (��=" ) + IntToString( comp_id ) + ").";
      Salons.comp_id = -1;
      break;
    default:
      if ( Salons.modify == mAdd )
        msg = "������� ������� ���������� (��=";
      else
        msg = "�������� ������� ���������� (��=";
      msg += IntToString( Salons.comp_id );
      msg += "). ��� �/�: ";
      if ( Salons.airline.empty() )
      	msg += "�� 㪠���";
      else
      	msg += Salons.airline; 
      msg += ", ��� �/�: ";
      if ( Salons.airp.empty() )
      	msg += "�� 㪠���";
      else
      	msg += Salons.airp;       
      msg += ", ⨯ ��: " + Salons.craft + ", ����: ";
      if ( Salons.bort.empty() )
        msg += "�� 㪠���";
      else
        msg += Salons.bort;
      msg += ", ������: " + Salons.classes + ", ���ᠭ��: ";
      if ( Salons.descr.empty() )
        msg += "�� 㪠����";
      else
        msg += Salons.descr;
      break;
  }
  r->MsgToLog( msg, evtComp, comp_id );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  NewTextChild( dataNode, "comp_id", Salons.comp_id );
  if ( !Salons.airline.empty() )
    NewTextChild( dataNode, "airline", Salons.airline );
  if ( !Salons.airp.empty() )
    NewTextChild( dataNode, "airp", Salons.airp );
  showMessage( "��������� �ᯥ譮 ��࠭���" );
}

bool filterComp( const string &airline, const string &airp )
{
	TReqInfo *r = TReqInfo::Instance();	
  return
       ( (int)airline.empty() + (int)airp.empty() == 1 &&
 		   (( 
 		     r->user.user_type == utAirline &&
 		     find( r->user.access.airlines.begin(), 
 		           r->user.access.airlines.end(), airline ) != r->user.access.airlines.end() /*&&
  		     ( r->user.access.airps.empty() || 
  		       find( r->user.access.airps.begin(), 
  		             r->user.access.airps.end(), 
  		             Qry.FieldAsString( "airp" ) ) != r->user.access.airps.end() )*/
  		  ) 
  		  ||
  		  (
  		    r->user.user_type == utAirport &&
  		    ( airp.empty() && ( r->user.access.airlines.empty() ||
  		       find( r->user.access.airlines.begin(), 
  		             r->user.access.airlines.end(), airline ) != r->user.access.airlines.end() ) ||
  		       find( r->user.access.airps.begin(), 
  		             r->user.access.airps.end(), airp ) != r->user.access.airps.end() )
  		   )
  		   ||
  		   (
  		     r->user.user_type == utSupport &&
  		     ( airp.empty() ||
  		       r->user.access.airps.empty() ||
   		       find( r->user.access.airps.begin(), 
    	             r->user.access.airps.end(), airp ) != r->user.access.airps.end() ) &&
  		     ( airline.empty() ||
  		       r->user.access.airlines.empty() ||
  		       find( r->user.access.airlines.begin(), 
  		             r->user.access.airlines.end(), airline ) != r->user.access.airlines.end() )
  		   )) 
  		 );
}

void SalonsInterface::BaseComponsRead(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *r = TReqInfo::Instance();	
  ProgTrace( TRACE5, "SalonsInterface::BaseComponsRead" );
  if ( r->user.user_type == utAirline && r->user.access.airlines.empty() ||
  	   r->user.user_type == utAirport && r->user.access.airps.empty() )
  	throw UserException( "��� �ࠢ ����㯠 � ������ �����������" );
  //TReqInfo::Instance()->user.check_access( amRead );
  TQuery Qry( &OraSession );
  if ( r->user.user_type == utAirport )
    Qry.SQLText = "SELECT airline,airp,comp_id,craft,bort,descr,classes FROM comps "\
                  " ORDER BY airp,airline,craft,comp_id";  	
  else
    Qry.SQLText = "SELECT airline,airp,comp_id,craft,bort,descr,classes FROM comps "\
                  " ORDER BY airline,airp,craft,comp_id";
  Qry.Execute();
  xmlNodePtr node = NewTextChild( resNode, "data" );
  node = NewTextChild( node, "compons" );
  while ( !Qry.Eof ) {
  	if ( filterComp( Qry.FieldAsString( "airline" ), Qry.FieldAsString( "airp" ) ) ) {
      xmlNodePtr rnode = NewTextChild( node, "compon" );
      NewTextChild( rnode, "comp_id", Qry.FieldAsInteger( "comp_id" ) );
      if ( r->user.user_type == utAirline && 
           r->user.access.airlines.size() > 1 ||
           r->user.user_type != utAirline && 
           ( r->user.access.airlines.empty() || r->user.access.airlines.size() > 1 ) ||
           r->user.user_type == utSupport && r->user.access.airlines.size() >= 1 && r->user.access.airps.size() >= 1 )
        NewTextChild( rnode, "airline", Qry.FieldAsString( "airline" ) );   
      if ( r->user.user_type == utAirport && r->user.access.airps.size() > 1 ||
           r->user.user_type == utSupport &&
           ( r->user.access.airps.empty() || r->user.access.airps.size() > 1 ||
             r->user.access.airlines.size() >= 1 && r->user.access.airps.size() >= 1 ) )
    	  NewTextChild( rnode, "airp", Qry.FieldAsString( "airp" ) );    	         
      NewTextChild( rnode, "craft", Qry.FieldAsString( "craft" ) );
      NewTextChild( rnode, "bort", Qry.FieldAsString( "bort" ) );
      NewTextChild( rnode, "descr", Qry.FieldAsString( "descr" ) );
      NewTextChild( rnode, "classes", Qry.FieldAsString( "classes" ) );		     			                  		     			     
      if ( r->user.user_type == utAirport && !Qry.FieldIsNULL( "airline" ) )
        NewTextChild( rnode, "canedit", 0 );
		}
  	Qry.Next();		    	
  }
}

void SalonsInterface::ChangeBC(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "SalonsInterface::ChangeBC" );
  //TReqInfo::Instance()->user.check_access( amWrite );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "BEGIN "\
    " UPDATE points SET cfaft = :craft WHERE point_id = :point_id; "\
    " UPDATE trip_sets SET comp_id = NULL WHERE point_id = :point_id; "\
    " DELETE trip_comp_rem WHERE point_id = :point_id; "\
    " DELETE trip_comp_elems WHERE point_id = :point_id; "\
    " DELETE trip_classes WHERE point_id = :point_id; "\
    "END; ";
  string bc = NodeAsString( "craft", reqNode );
  int trip_id = NodeAsInteger( "point_id", reqNode );
  Qry.CreateVariable( "craft", otString,  bc );
  Qry.CreateVariable( "point_id", otInteger, trip_id );
  Qry.Execute();
  TReqInfo::Instance()->MsgToLog( string( "������� ⨯ �� �� " ) + bc +
                                  ". ������ ���������� ३� 㤠����." , evtFlt, trip_id );
  SalonFormShow( ctxt, reqNode, resNode );
}


void SalonsInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};





