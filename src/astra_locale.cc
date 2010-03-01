#include <string>
#include <stdarg.h>
#include "exceptions.h"
#include "stl_utils.h"
#include "oralib.h"
#include "exceptions.h"
#include "astra_locale.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"


using namespace std;

namespace AstraLocale {

TLocaleMessages *TLocaleMessages::Instance()
{
	tst();
  static TLocaleMessages *instance_ = 0;
  if ( !instance_ )
    instance_ = new TLocaleMessages();
  return instance_;
};

void TLocaleMessages::Clear()
{
	tid = -1;
	messages.clear();
}

void TLocaleMessages::Invalidate()
{
	TQuery Qry(&OraSession);
	if ( tid >= 0 )
	  Qry.SQLText =
	    "SELECT id,lang,text,tid,pr_del "
	    " FROM locale_messages ";
	else {
	  Qry.SQLText =
	    "SELECT id,lang,text,tid,pr_del "
	    " FROM locale_messages "
	    "WHERE tid>:tid";
	  Qry.CreateVariable( "tid", otInteger, tid );
	}
	Qry.Execute();
  while ( !Qry.Eof ) {
  	TLocaleMessage msg( Qry.FieldAsString( "lang" ), Qry.FieldAsString( "text" ), Qry.FieldAsInteger( "pr_del" ) );
  	messages[ string(Qry.FieldAsString( "id" )) ] = msg;
  	if ( tid < Qry.FieldAsInteger( "tid" ) )
  		tid = Qry.FieldAsInteger( "tid" );
  	Qry.Next();
  }
}

std::string TLocaleMessages::getText( const std::string &lexema_id, const std::string &lang, bool with_del )
{
	tst();
	string vlang = upperc(lang);
	string vid = upperc(lexema_id);
	if ( vlang.empty() ) {
		ProgError( STDLOG, "TLocaleMessages::etText, lang is empty, default lang RU" );
		vlang = "RU";
	}
	ProgTrace( TRACE5, "id=%s, lang=%s, with_del=%d", vid.c_str(), vlang.c_str(), with_del );
	if ( messages.find( vid ) == messages.end() )
		throw EXCEPTIONS::Exception( "TMessages::getText: message id=%s not found", vid.c_str() );
	TLocaleMessage msg = messages[ vid ];
	if ( msg.lang_messages.find( vlang ) == msg.lang_messages.end() )
		throw EXCEPTIONS::Exception( "TMessages::getText: message in lang='%s', id='%s' not found", vlang.c_str(), vid.c_str() );
	if ( !with_del && msg.pr_del )
		throw EXCEPTIONS::Exception( "TMessages::getText: invalid lang='%s', id='%s', pr_del=%d", vlang.c_str(), vid.c_str(), msg.pr_del );
	ProgTrace( TRACE5, "TLocaleMessages::getText return '%s'", msg.lang_messages[ vlang ].c_str() );
	return msg.lang_messages[ vlang ];
}

TLocaleMessages::TLocaleMessages()
{
	tst();
	Clear();
	tst();
  Invalidate();
  tst();
}

LParam::LParam( std::string aname, LexemaData avalue )
{
	name = aname;
	value = avalue;
}


LexemaData LParams::LexemaDataValue( const std::string &name, const boost::any &param ) {
  if ( !boost::any_cast<LexemaData>(&param) )
   	throw EXCEPTIONS::Exception( "TParams: param type is not int, param.type()=%s, name=%s",  param.type().name(), name.c_str() );
  return boost::any_cast<LexemaData>(param);
}


void buildMsg( const std::string &lang, LexemaData &lexemaData, std::string &text, std::string &master_lexema )
{
  tst();
  if ( master_lexema.empty() && lexemaData.lexema_id.find( FORMAT_MSG ) != string::npos )
  	master_lexema = lexemaData.lexema_id;
  char vval[500];
	LexemaData ld;
	string str_val;
	std::map<std::string, boost::any>::iterator lp;
  string lexema = TLocaleMessages::Instance()->getText( lexemaData.lexema_id, lang, false );
  LParser parser( lexema );
	text = lexema;
  for ( std::map<int, ElemData>::iterator i=parser.begin(); i!=parser.end(); i++ ) {
  	ProgTrace( TRACE5, "variable pos=%d, name='%s'", i->first, i->second.var_name.c_str() );
  	lp = lexemaData.lparams.find( i->second.var_name );
  	if ( lp == lexemaData.lparams.end() )
  		throw EXCEPTIONS::Exception( "variable '%s' not found in lexemaData.lparams", i->second.var_name.c_str() );

  	switch( i->second.lformat ) {
  		case lfString:
  			snprintf( vval, sizeof(vval), i->second.format.c_str(), lexemaData.lparams.StringValue( lp->first, lp->second ).c_str() );
  			str_val = vval;
  			break;
  		case lfInt:
  			snprintf( vval, sizeof(vval), i->second.format.c_str(), lexemaData.lparams.IntValue( lp->first, lp->second ) );
  			str_val = vval;
  			break;
  		case lfDouble:
  		case lfDateTime:
  			snprintf( vval, sizeof(vval), i->second.format.c_str(), lexemaData.lparams.DoubleValue( lp->first, lp->second ) );
  			str_val = vval;
  			break;
  		case lfLexema:
  			ld = lexemaData.lparams.LexemaDataValue( lp->first, lp->second );
  			buildMsg( lang, ld, str_val, master_lexema );
  			break;
  		case lfUnknown:
  			throw EXCEPTIONS::Exception( "Invalid format '%s', variable name=%s", i->second.format.c_str(), i->second.var_name.c_str() );
  	};
  	text.replace( i->second.first_elem, i->second.last_elem - i->second.first_elem + 1, str_val );
  	ProgTrace( TRACE5, "master_lexema=%s, text=%s", master_lexema.c_str(), text.c_str() );
  }

}

/*string UserException::getMsg( const std::string &lang )
{
	tst();
	LexemaData lexemaData;
	lexemaData.lexema_id = lexema_id;
	lexemaData.lparams = lparams;
	string res;
	buildMsg( lang, lexemaData, res );
	return res;
}*/

UserException::~UserException() throw()
{
	tst();
};

TLocaleFormat LParser::getFormat( std::string &format_val )
{
	string::size_type idx;
	idx = format_val.find( "s" );
	if ( idx != string::npos )
		return lfString;
	idx = format_val.find( "d" );
	if ( idx != string::npos )
		return lfInt;
	idx = format_val.find( "f" );
	if ( idx != string::npos )
		return lfDouble;
	idx = format_val.find( "L" );
	if ( idx != string::npos )
		return lfLexema;
	return lfUnknown;
}

LParser::LParser( const string &lexema )
{
	if ( lexema.empty() )
		throw EXCEPTIONS::Exception( "LParser: lexema is empty" );
  ProgTrace( TRACE5, "lexema=%s", lexema.c_str() );
	string::size_type first_elem_idx = lexema.find( VARIABLE_FIRST_ELEM );
  while ( first_elem_idx != string::npos ) {
  	string::size_type first_format_idx = lexema.find( FORMAT_FIRST_ELEM, first_elem_idx + VARIABLE_FIRST_ELEM.size() );
  	if ( first_format_idx == string::npos )
  		throw EXCEPTIONS::Exception( "LParser: FORMAT_FIRST_ELEM=%s not found, lexema %s", FORMAT_FIRST_ELEM.c_str(), lexema.c_str() );
  	string::size_type end_elem_idx = lexema.find( VARIABLE_END_ELEM, first_format_idx + FORMAT_FIRST_ELEM.size() );
  	if ( end_elem_idx == string::npos )
  		throw EXCEPTIONS::Exception( "LParser: VARIABLE_END_ELEM=%s not found, lexema %s", VARIABLE_END_ELEM.c_str(), lexema.c_str() );
  	ElemData ed;
  	ed.var_name = lexema.substr( first_elem_idx + VARIABLE_FIRST_ELEM.size(), first_format_idx - first_elem_idx - VARIABLE_FIRST_ELEM.size() );
    ed.format = lexema.substr( first_format_idx, end_elem_idx - first_format_idx );
    ed.lformat = getFormat( ed.format );
    ed.first_elem = first_elem_idx;
    ed.last_elem = end_elem_idx;
    insert( make_pair( ed.first_elem, ed ) );
    /* table. */
    ProgTrace( TRACE5, "var_name=%s, format=%s, first_elem=%d, last_elem=%d",
               ed.var_name.c_str(), ed.format.c_str(), ed.first_elem, ed.last_elem );
  	first_elem_idx = lexema.find( VARIABLE_FIRST_ELEM, first_elem_idx + 1 );
  }
}

} //end namespace AstraLocale
/* Когда телать обновления? при рестарте или каждый раз или по таймеру или по признаку */
/*create table lang_types (
code varchar2(2),
name varchar2(50)
)


CREATE TABLE locale_messages (
ID VARCHAR2(2000) NOT NULL,
LANG VARCHAR2(20) NOT NULL,
TEXT VARCHAR2(2000) NOT NULL,
PR_DEL NUMBER(1) NOT NULL,
TID NUMBER(9) NOT NULL)
*/
