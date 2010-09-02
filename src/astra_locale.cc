#include <string>
#include <stdarg.h>
#include <boost/crc.hpp>
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
	server_msgs.clear();
	client_msgs.clear();
}

void TLocaleMessages::Invalidate( std::string lang, bool pr_term )
{
	bool res = false;
	lang = upperc(lang);
	int tid;
	if ( pr_term )
		tid = client_msgs.get_tid( lang );
	else
		tid = server_msgs.get_tid( lang );
	ProgTrace( TRACE5, "before Invalidate: lang=%s, pr_term=%d, tid=%d, server_msgs.msgs.size()=%d, client_msgs.msgs.size()=%d", lang.c_str(), pr_term, tid, server_msgs.msgs.size(), client_msgs.msgs.size() );
	TQuery Qry(&OraSession);
	if ( tid < 0 )
	  Qry.SQLText =
	    "SELECT id,text,tid,pr_del "
	    " FROM locale_messages "
	    "WHERE lang=:lang AND (pr_term=:pr_term OR pr_term IS NULL) "
	    "ORDER BY id";
	else {
	  Qry.SQLText =
	    "SELECT id,text,tid,pr_del "
	    " FROM locale_messages "
	    "WHERE lang=:lang AND (pr_term=:pr_term OR pr_term IS NULL) AND tid>:tid "
	    "ORDER BY id";
	  Qry.CreateVariable( "tid", otInteger, tid );
	}
	Qry.CreateVariable( "lang", otString, lang );
	Qry.CreateVariable( "pr_term", otInteger, pr_term );
	Qry.Execute();
	tst();
  while ( !Qry.Eof ) {
  	if ( pr_term )
  		client_msgs.Add( Qry.FieldAsString( "id" ), lang, Qry.FieldAsString( "text" ), Qry.FieldAsInteger( "pr_del" ) );
  	else {
  	  server_msgs.Add( Qry.FieldAsString( "id" ), lang, Qry.FieldAsString( "text" ), Qry.FieldAsInteger( "pr_del" ) );
// 	  	ProgTrace( TRACE5, "id=%s, lang=%s, text=%s", Qry.FieldAsString( "id" ), lang.c_str(), server_msgs.msgs[Qry.FieldAsString( "id" )].lang_messages[ lang ].value.c_str() );
  	}

  	if ( tid < Qry.FieldAsInteger( "tid" ) )
  		tid = Qry.FieldAsInteger( "tid" );
  	Qry.Next();
  	res = true;
  }
  if ( pr_term )
		client_msgs.set_tid( lang, tid );
	else
		server_msgs.set_tid( lang, tid );
  ProgTrace( TRACE5, "after Invalidate: lang=%s, tid=%d, server_msgs.msgs.size()=%d, client_msgs.msgs.size()=%d, res=%d", lang.c_str(), tid, server_msgs.msgs.size(), client_msgs.msgs.size(), res );
  if ( res && pr_term ) { // создаем словарь формата клиент
	  string s;
	  for (map<std::string, TLocaleMessage>::iterator i=client_msgs.msgs.begin(); i!=client_msgs.msgs.end(); i++ ) {
	  	if ( i->second.lang_messages.find(lang) == i->second.lang_messages.end() ) // нет значения для заданного языка
	  		continue;
	  	if ( i->second.lang_messages[ lang ].pr_del != 0 ) // удален элемент
	  		continue;
	  	s += i->first + string(1,9) + i->second.lang_messages[ lang ].value + string(1,13) + string(1,10);
	  }
	  TDictionary d;
	  d.value = s;
    boost::crc_basic<32> crc32( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true );
    crc32.reset();
    crc32.process_bytes( d.value.c_str(), d.value.size() );
	  d.checksum = crc32.checksum();
	  dicts[ lang ] = d;
  }
}

std::string TLocaleMessages::getText( const std::string &lexema_id, const std::string &lang )
{
	tst();
	string vlang = upperc(lang);
	string vid = lexema_id;
	if ( vlang.empty() ) {
		vlang = "EN";
	}
	ProgTrace( TRACE5, "id=%s, lang=%s", vid.c_str(), vlang.c_str() );
	if ( server_msgs.msgs.find( vid ) == server_msgs.msgs.end() )
		throw EXCEPTIONS::Exception( "TMessages::getText: message id=%s not found", vid.c_str() );
	TLocaleMessage msg = server_msgs.msgs[ vid ];
	if ( msg.lang_messages.find( vlang ) == msg.lang_messages.end() )
		throw EXCEPTIONS::Exception( "TMessages::getText: message in lang='%s', id='%s' not found", vlang.c_str(), vid.c_str() );
	if ( msg.lang_messages[ vlang ].pr_del )
		throw EXCEPTIONS::Exception( "TMessages::getText: msg delete invalid lang='%s', id='%s', pr_del=%d", vlang.c_str(), vid.c_str(), msg.lang_messages[ vlang ].pr_del );
	ProgTrace( TRACE5, "TLocaleMessages::getText return '%s'", msg.lang_messages[ vlang ].value.c_str() );
	return msg.lang_messages[ vlang ].value;
}

TLocaleMessages::TLocaleMessages()
{
	Clear();
	Invalidate("RU",false);
	Invalidate("EN",false);
}

int TLocaleMessages::checksum(const std::string &lang)
{
	Invalidate(lang,true);
	if ( dicts.find(lang) != dicts.end() )
		return dicts[ lang ].checksum;
	else
		return -1;
}

std::string TLocaleMessages::getDictionary(const std::string &lang)
{
	if ( dicts.find(lang) != dicts.end() )
		return dicts[ lang ].value;
	else
		return string("");
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
    string lexema = TLocaleMessages::Instance()->getText( lexemaData.lexema_id, lang );
    LParser parser( lexema );
    text = lexema;
    for ( std::map<int, ElemData>::iterator i=parser.begin(); i!=parser.end(); i++ ) {
        ProgTrace( TRACE5, "variable pos=%d, name='%s'", i->first, i->second.var_name.c_str() );
        lp = lexemaData.lparams.find( i->second.var_name );
        if ( lp == lexemaData.lparams.end() and i->second.var_name != FORMAT_LINE_BREAK)
            throw EXCEPTIONS::Exception( "variable '%s' not found in lexemaData.lparams", i->second.var_name.c_str() );

        switch( i->second.lformat ) {
            case lfLineBreak:
                str_val = "\n"; //!!! UNIX & DOS handling problem?
                break;
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
tst();
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
    if(format_val == FORMAT_LINE_BREAK)
        return lfLineBreak;
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
        string::size_type end_elem_idx = lexema.find( VARIABLE_END_ELEM,  first_elem_idx + VARIABLE_FIRST_ELEM.size() );
        if(first_format_idx == string::npos or end_elem_idx != string::npos and first_format_idx > end_elem_idx) // special var encountered
            first_format_idx = end_elem_idx;

        if ( first_format_idx == string::npos )
            throw EXCEPTIONS::Exception( "LParser: FORMAT_FIRST_ELEM=%s not found, lexema %s", FORMAT_FIRST_ELEM.c_str(), lexema.c_str() );
        if ( end_elem_idx == string::npos )
            throw EXCEPTIONS::Exception( "LParser: VARIABLE_END_ELEM=%s not found, lexema %s", VARIABLE_END_ELEM.c_str(), lexema.c_str() );
        ElemData ed;
        ed.var_name = lexema.substr( first_elem_idx + VARIABLE_FIRST_ELEM.size(), first_format_idx - first_elem_idx - VARIABLE_FIRST_ELEM.size() );
        ed.format = lexema.substr( first_format_idx, end_elem_idx - first_format_idx );
        if(ed.format.empty())
            ed.format = ed.var_name;
        ed.lformat = getFormat( ed.format );
        ed.first_elem = first_elem_idx;
        ed.last_elem = end_elem_idx;
        insert( make_pair( ed.first_elem, ed ) );
        /* table. */
        ProgTrace( TRACE5, "var_name=%s, format=%s, first_elem=%d, last_elem=%d, lformat=%d",
                ed.var_name.c_str(), ed.format.c_str(), ed.first_elem, ed.last_elem, ed.lformat );
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
