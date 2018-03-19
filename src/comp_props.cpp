#include "comp_props.h"
#include "salonform.h"
#include "astra_locale.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_api.h"
#include "oralib.h"
#include "stl_utils.h"
#include "images.h"
#include "points.h"
#include "salons.h"
#include "seats.h"
#include "seats_utils.h"
#include "convert.h"
#include "iatci.h"
#include "iatci_help.h"
#include "astra_misc.h"
#include "tlg/tlg_parser.h" // only for convert_salons
#include "term_version.h"
#include "alarms.h"
#include "serverlib/str_utils.h"

#define NICKNAME "DJEK"
#include <serverlib/slogger.h>

using namespace std;
using namespace AstraLocale;
using namespace ASTRA;
using namespace BASIC_SALONS;

/*

  CREATE TABLE comp_section_types(
  code VARCHAR2(10),
  name VARCHAR2(50),
  name_lat VARCHAR2(50)
  );
ALTER TABLE comp_section_types
       ADD CONSTRAINT comp_section_types__PK PRIMARY KEY (code);

 INSERT INTO comp_section_types VALUES('tail','Хвост самолета','Aircraft tail');
 INSERT INTO comp_section_types VALUES('section','Багажная секция','Luggage section');
 INSERT INTO comp_section_types VALUES('nose','Нос самолета','Aircraft nose');
 INSERT INTO comp_section_types VALUES('wing','Крыло самолета','Aircraft wing');
 COMMIT;
 ALTER TABLE comp_sections ADD code VARCHAR2(10);
 UPDATE comp_sections SET code='section';
 COMMIT;
 ALTER TABLE comp_sections MODIFY code NOT NULL;
ALTER TABLE comp_sections
       ADD CONSTRAINT comp_sections__section_types
              FOREIGN KEY (code)
                             REFERENCES comp_section_types  (code);


 *
 *
 */

namespace SALONS2
{

simpleProps& simpleProps::read( int comp_id ) {
  clear();
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT name, first_rownum, last_rownum FROM comp_sections "
    " WHERE comp_id=:comp_id AND code=:code "
    "ORDER BY first_rownum";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.CreateVariable( "code", otString, code );
  Qry.Execute();
  for( ; !Qry.Eof; Qry.Next() ) {
    push_back( SimpleProp( Qry.FieldAsString( "name" ),
                           Qry.FieldAsInteger( "first_rownum" ),
                           Qry.FieldAsInteger( "last_rownum" ) )
              );
  }
  return *this;
}

simpleProps& simpleProps::write( int comp_id ) {
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "DELETE comp_sections WHERE comp_id=:comp_id AND code=:code";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.CreateVariable( "code", otString, code );
  Qry.Execute();
  ProgTrace( TRACE5, "RowCount=%d", Qry.RowCount() );
  bool pr_exists = Qry.RowCount();
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO comp_sections(comp_id,code,name,first_rownum,last_rownum) "
    " VALUES(:comp_id,:code,:name,:first_rownum,:last_rownum)";
  Qry.CreateVariable( "comp_id", otInteger, comp_id );
  Qry.CreateVariable( "code", otString, code );
  Qry.DeclareVariable( "name", otString );
  Qry.DeclareVariable( "first_rownum", otInteger );
  Qry.DeclareVariable( "last_rownum", otInteger );
  PrmLexema lexema(componPropCodes::Instance()->getWrapIsSectDelLexema( code ), "");
  if ( empty() && pr_exists ) {
    lexema.ChangeLexemaId( componPropCodes::Instance()->getDeleteLexema( code ) );
  }
  PrmEnum prmenum("sect", " ");
  bool pr_empty = true;
  for ( simpleProps::const_iterator i=begin(); i!=end(); i++ ) {
    PrmLexema new_lexema("", "EVT.SECTIONS");
    new_lexema.prms << PrmSmpl<std::string>("name", i->getName()) << PrmSmpl<int>("FirstRow", i->getFirstRow()+1)
                    << PrmSmpl<int>("LastRow", i->getLastRow()+1);//!!!ANNA ряды выводятся неправильно - это индекс ряда, т.к. названия номера ряда может меняться
    prmenum.prms << new_lexema;
    Qry.SetVariable( "name", i->getName() );
    Qry.SetVariable( "first_rownum", i->getFirstRow() );
    Qry.SetVariable( "last_rownum", i->getLastRow() );
    Qry.Execute();
    pr_empty = false;
  }
  if (empty() && pr_exists && pr_empty)
    TReqInfo::Instance()->LocaleToLog(componPropCodes::Instance()->getDeleteLexema( code ), ASTRA::evtComp, comp_id);
  else
    TReqInfo::Instance()->LocaleToLog(componPropCodes::Instance()->getAssignedLexema( code ), LEvntPrms() << lexema << prmenum, ASTRA::evtComp, comp_id);
  return *this;
}

simpleProps& simpleProps::parse( xmlNodePtr sectionsNode ) {
  clear();
  if ( !sectionsNode ) {
    return *this;
  }
  sectionsNode = sectionsNode->children;
  while ( sectionsNode && std::string((const char*)sectionsNode->name) == "section" ) {
    std::string name = NodeAsString( sectionsNode );
    if ( !IsAscii7( name ) ) {
      throw UserException( componPropCodes::Instance()->getInvalidSectionNameLexema( code ) );
    }
    push_back( SimpleProp( name,
                           NodeAsInteger( "@FirstRowIdx", sectionsNode ),
                           NodeAsInteger( "@LastRowIdx", sectionsNode ) ) );
    sectionsNode = sectionsNode->next;
  }
  return *this;
}

simpleProps& simpleProps::build( xmlNodePtr sectionsNode ) {
  if ( !sectionsNode ) {
    return *this;
  }
  SetProp( sectionsNode, "code", code );
  for ( vector<SimpleProp>::const_iterator i=this->begin(); i!=end(); i++ ) {
    xmlNodePtr cnode = NewTextChild( sectionsNode, "section", i->getName() );
    SetProp( cnode, "FirstRowIdx", i->getFirstRow() );
    SetProp( cnode, "LastRowIdx", i->getLastRow() );
  }
  return *this;
}

void componPropCodes::buildSections( int comp_id, const std::string &lang, xmlNodePtr dataNode ) {
  simpleProps sections( SECTION );
  sections.read( comp_id ).build( NewTextChild( dataNode, "CompSections" ) );
  xmlNodePtr n = NewTextChild( dataNode, "sections" );
  std::vector<std::string> codes = getCodes();
  xmlNodePtr nodeCodes = NewTextChild( n, "codes" );
  for ( std::vector<std::string>::const_iterator icode=codes.begin(); icode!=codes.end(); icode++ ) {
    xmlNodePtr nodeCode = NewTextChild( nodeCodes, "item" );
    SetProp( nodeCode, "code", *icode );
    SetProp( nodeCode, "name", getName( lang, *icode ) );
    SetProp( nodeCode, "color", getColor( *icode ) );
    if ( SECTION == *icode ) {
      continue;
    }
    simpleProps sections( *icode );
    sections.read( comp_id ).build( NewTextChild( n, "sections" ) );
  }
}
}
//end namespace
