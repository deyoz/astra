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

const char CurrName[] = " (���.)";

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;


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
  TSalons::GetTripParams( Salons.trip_id, dataNode );
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
  TSalons::GetTripParams( trip_id, dataNode );

 tst();
 Qry.SQLText = "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "\
               "       comps.descr,0 as pr_comp "\
               " FROM comps, points "\
               "WHERE points.craft = comps.craft AND points.point_id = :point_id "\
               "UNION "\
               "SELECT comps.comp_id,comps.craft,comps.bort,comps.classes, "\
               "       comps.descr,1 as pr_comp "\
               " FROM comps, points, trip_sets "\
               "WHERE points.point_id=trip_sets.point_id AND "\
               "      points.craft = comps.craft AND points.point_id = :point_id AND "\
               "      trip_sets.comp_id = comps.comp_id "\
               "UNION "\
               "SELECT -1, craft, bort, "\
               "        LTRIM(RTRIM( DECODE( a.f, 0, '', ' �'||a.f)||"\
               "        DECODE( a.c, 0, '', ' �'||a.c)|| "\
               "        DECODE( a.y, 0, '', ' �'||a.y) )) classes, '',1 "\
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
 tst();
 Qry.Execute();
 tst();
 try {
   if ( Qry.RowCount() == 0 )
     throw UserException( "��� ���������� �� ������� ⨯� ��" );
   xmlNodePtr compsNode = NewTextChild( dataNode, "comps"  );
   string StrVal;
   while ( !Qry.Eof ) {
     xmlNodePtr compNode = NewTextChild( compsNode, "comp" );
     if ( !Qry.FieldIsNULL( "bort" ) && Qry.FieldAsInteger( "pr_comp" ) != 1 )
       StrVal = Qry.FieldAsString( "bort" );
     else
       StrVal = "  ";
     StrVal += string( "  " ) + Qry.FieldAsString( "classes" );
     tst();
     if ( !Qry.FieldIsNULL( "descr" ) && Qry.FieldAsInteger( "pr_comp" ) != 1 )
       StrVal += string( "  " ) + Qry.FieldAsString( "descr" );
     tst();
     if ( Qry.FieldAsInteger( "pr_comp" ) == 1 )
       StrVal += CurrName;
     NewTextChild( compNode, "name", StrVal );
     NewTextChild( compNode, "comp_id", Qry.FieldAsInteger( "comp_id" ) );
     NewTextChild( compNode, "pr_comp", Qry.FieldAsInteger( "pr_comp" ) );
     NewTextChild( compNode, "craft", Qry.FieldAsString( "craft" ) );
     NewTextChild( compNode, "bort", Qry.FieldAsString( "bort" ) );
     NewTextChild( compNode, "classes", Qry.FieldAsString( "classes" ) );
     tst();
     NewTextChild( compNode, "descr", Qry.FieldAsString( "descr" ) );
     tst();
     Qry.Next();
   }
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
  NewTextChild( resNode, "existsregpassengers", TSalons::InternalExistsRegPassenger( trip_id, SeatNoIsNull ) );
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
  Salons.Write( rTripSalons );
  bool pr_initcomp = NodeAsInteger( "initcomp", reqNode );

  if ( pr_initcomp ) { /* ��������� ���������� */
    /* ���樠������ */
    Qry.Clear();
    Qry.SQLText = "BEGIN "\
                  " salons.initcomp( :point_id ); "\
                  "END; ";
    Qry.DeclareVariable( "point_id", otInteger );
    Qry.SetVariable( "point_id", trip_id );
    Qry.Execute();
    /* ������ � ��� */
    xmlNodePtr refcompNode = NodeAsNode( "refcompon", reqNode );
    string msg = string( "�������� ���������� ३�. ������: " ) +
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
      msg = string( "�����祭� ������� ���������� (��=" ) +
            IntToString( comp_id ) +
            "). ������: " + NodeAsString( "classes", refcompNode );
      if ( cChange )
        msg = string( "�����祭� ���������� ३�. ������: " ) +
              NodeAsString( "ref", refcompNode );
    }
    msg += string( ", ����஢��: " ) + NodeAsString( "lang", refcompNode );
    TReqInfo::Instance()->MsgToLog( msg, evtFlt, trip_id );
    /* �����뢠���� ���������� �� �� */
    Salons.Read(  rTripSalons );
  }
  Passengers.Clear();
  if ( TSalons::InternalExistsRegPassenger( trip_id, false ) ) { /* ���� ��ॣ����஢���� ���ᠦ��� */
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
  //TReqInfo::Instance()->user.check_access( amWrite );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  int pax_id = NodeAsInteger( "pax_id", reqNode );
  int tid = NodeAsInteger( "tid", reqNode );
  int num = NodeAsInteger( "num", reqNode );
  int x = NodeAsInteger( "x", reqNode );
  int y = NodeAsInteger( "y", reqNode );
  xmlNodePtr checkinNode = GetNode( "checkin", reqNode );
  xmlNodePtr setseatNode = GetNode( "reserve", reqNode );
  ProgTrace(TRACE5, "SalonsInterface::Reseat, trip_id=%d, pax_id=%d, tid=%d", trip_id, pax_id, tid );
  TQuery Qry( &OraSession );
  /* ��稬 ३� */
  Qry.SQLText = "UPDATE points SET point_id=point_id WHERE point_id=:point_id";
  Qry.DeclareVariable( "point_id", otInteger );
  Qry.SetVariable( "point_id", trip_id );
  Qry.Execute();
  Qry.Clear();
  TSeatsType seatstype;
  /* �஢�ઠ �� �, �� ����� �� ���ᠦ��� �� ���﫨�� */
  if ( setseatNode ) {
  	seatstype = sreserve;
  	Qry.SQLText = "SELECT tid,TRIM( surname||' '||name ) name FROM crs_pax WHERE pax_id=:pax_id";
  }
  else {
  	seatstype = sreseats;
  	Qry.SQLText = "SELECT tid,TRIM( surname||' '||name ) name FROM pax WHERE pax_id=:pax_id";
  }
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  string errmsg;
  if ( !Qry.RowCount() )
  	if ( setseatNode ) 
  		errmsg = "���������� �।���⥫쭮� �����祭�� ������� ����";
    else
    	errmsg = "���ᠤ�� ����������";
  if ( errmsg.empty() && Qry.FieldAsInteger( "tid" ) != tid ) {
    if ( checkinNode )
      errmsg = string( "��������� �� ���ᠦ��� " ) + Qry.FieldAsString( "name" ) +
               " �ந��������� � ��㣮� �⮩��. ������� �����";
    else
      errmsg = string( "��������� �� ���ᠦ��� " ) + Qry.FieldAsString( "name" ) +
               " �ந��������� � ��㣮� �⮩��. ����� ���������";
  }
  string nplaceName;
  if ( !errmsg.empty() || !SEATS::Reseat( seatstype, trip_id, pax_id, tid, num, x, y, nplaceName ) ) {
    /* ����� �� ������ ���५�, ���� �������� �� */
    tst();
    TSalons Salons;
    Salons.trip_id = trip_id;
    Salons.Read( rTripSalons );
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    xmlNodePtr salonsNode = NewTextChild( dataNode, "salons" );
    TSalons::GetTripParams( trip_id, dataNode );
    Salons.Build( salonsNode );
    tst();
    if ( !checkinNode ) {
      if ( errmsg.empty() )
      	if ( setseatNode )
      		errmsg = "���������� �।���⥫쭮� �����祭�� ������� ����";
      	else {
          errmsg = "���ᠤ�� ����������";
          SEATS::SelectPassengers( &Salons, Passengers );
          tst();
          if ( Passengers.existsNoSeats() )
            Passengers.Build( dataNode );
        }
    }
    showErrorMessageAndRollback( errmsg );
  }
  else {
    /* ���� ��।��� ����� ���� tid */
    xmlNodePtr dataNode = NewTextChild( resNode, "data" );
    NewTextChild( dataNode, "tid", tid );
    NewTextChild( dataNode, "placename", nplaceName );
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
  //TReqInfo::Instance()->user.check_access( amRead );
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
  Salons.craft = NodeAsString( "craft", reqNode );
  Salons.bort = NodeAsString( "bort", reqNode );
  Salons.descr = NodeAsString( "descr", reqNode );
  string classes = NodeAsString( "classes", reqNode );
  Salons.classes = RTrimString( classes );
  if ( Salons.craft.empty() )
    throw UserException( "�� ����� ⨯ ��" );
  TQuery Qry( &OraSession );
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
      msg += "). ��� ��: " + Salons.craft + ", ����: ";
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
  TReqInfo::Instance()->MsgToLog( msg, evtComp, comp_id );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  NewTextChild( dataNode, "comp_id", Salons.comp_id );
  showMessage( "��������� �ᯥ譮 ��࠭���" );
}

void SalonsInterface::BaseComponsRead(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "SalonsInterface::BaseComponsRead" );
  //TReqInfo::Instance()->user.check_access( amRead );
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
