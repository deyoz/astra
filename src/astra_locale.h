#ifndef _ASTRA_LOCALE_H_
#define _ASTRA_LOCALE_H_

#include <map>
#include <string>
#include "exceptions.h"
#include "basic.h"
#include <stl_utils.h>
#include <boost/any.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant.hpp>

namespace AstraLocale {

struct LexemaData;
class LParam;

/*struct LexemaData {
	std::string lexema_id;
	LParams lparams;
};*/

struct LParam {
	std::string name;
	boost::any value;
/*	LParam( std::string aname, boost::any avalue ) {
		name = aname;
	  value = avalue;
	}*/
	LParam( std::string aname, std::string avalue ) {
		name = aname;
	  value = avalue;
	}
	LParam( std::string aname, int avalue ) {
		name = aname;
	  value = avalue;
	}
	LParam( std::string aname, double avalue ) {
		name = aname;
	  value = avalue;
	}
	LParam( std::string aname, LexemaData avalue );/* {
		name = aname;
	  value = avalue;
	}*/
};

class LParams: public std::map<std::string, boost::any>
{
  private:
  	const boost::any & getParam( const std::string &name ) {
  		if ( this->find( name ) != this->end() )
  			return this->find( name )->second;
  		throw EXCEPTIONS::Exception( "TParams: invalid param name=%s", name.c_str() );
  	}
  public:
  	LParams& add(std::string name, boost::any &value) {
      this->insert( make_pair( name, value ) );
  		return *this;
  	}
  	LParams& operator << ( LParam p ) {
  		return add( p.name, p.value );
  	}
  	std::string StringValue( const std::string &name, const boost::any &param ) {
      if ( !boost::any_cast<std::string>(&param) )
      	throw EXCEPTIONS::Exception( "TParams: param type is not string, name=%s", name.c_str() );
      return boost::any_cast<std::string>(param);
  	}
  	std::string StringValue( const std::string &name ) {
      return StringValue( name, getParam( name ) );
  	}
  	int IntValue( const std::string &name, const boost::any &param ) {
      if ( param.type() != typeid(int) )
      	throw EXCEPTIONS::Exception( "TParams: param type is not int, param.type()=%s, name=%s",  param.type().name(), name.c_str() );
      return boost::any_cast<int>(param);
  	}
  	int IntValue( const std::string &name ) {
  		return IntValue( name, getParam( name ) );
  	}
  	double DoubleValue( const std::string &name, const boost::any &param ) {
      if ( param.type() != typeid(double) )
      	throw EXCEPTIONS::Exception( "TParams: param type is not double, param.type()=%s, name=%s", param.type().name(), name.c_str() );
      return boost::any_cast<double>(param);
  	}
  	double DoubleValue( const std::string &name ) {
  		return DoubleValue( name, getParam( name ) );
  	}
  	BASIC::TDateTime DateTimeValue( const std::string &name ) {
      return DoubleValue( name );
  	}
  	LexemaData LexemaDataValue( const std::string &name, const boost::any &param );
  	~LParams() {
  		clear();
  	}
};

struct LexemaData {
	std::string lexema_id;
	LParams lparams;
};

class UserException:public EXCEPTIONS::UserException
{
	private:
		std::string lexema_id;
		LParams lparams;
	public:
    UserException( int code, std::string vlexema, LParams &aparams):EXCEPTIONS::UserException(code,vlexema)
    {
    	lparams = aparams;
    	lexema_id = vlexema;
    }
    UserException( std::string vlexema, LParams &aparams):EXCEPTIONS::UserException(0,vlexema)
    {
    	lparams = aparams;
    	lexema_id = vlexema;
    }
    UserException( int code, std::string vlexema):EXCEPTIONS::UserException(code,vlexema)
    {
    	lexema_id = vlexema;
    }
    UserException( std::string vlexema):EXCEPTIONS::UserException(0,vlexema)
    {
    	lexema_id = vlexema;
    }
    LexemaData getLexemaData( ) {
    	LexemaData data;
    	data.lexema_id = lexema_id;
    	data.lparams = lparams;
    	return data;
    }
    virtual ~UserException() throw();
};

const std::string VARIABLE_FIRST_ELEM = "[";
const std::string VARIABLE_END_ELEM = "]";
const std::string FORMAT_FIRST_ELEM = "%";
const std::string FORMAT_LEXEMA = "L";
const std::string FORMAT_MSG = "MSG.";
const std::string FORMAT_COVER = "WRAP.";
enum TLocaleFormat { lfInt, lfDouble, lfDateTime, lfString, lfLexema, lfUnknown };

struct ElemData {
	std::string var_name;
	std::string format;
	TLocaleFormat lformat;
	std::string::size_type first_elem;
	std::string::size_type last_elem;
	ElemData() {
		lformat = lfUnknown;
		first_elem = std::string::npos;
		last_elem = std::string::npos;
	}
};

class LParser: public std::map<int, ElemData, std::greater<int> >
{
	private:
		TLocaleFormat getFormat( std::string &format_val );
	public:
  	LParser( const std::string &lexema );
};

struct TMessageText {
	std::string value;
	int pr_del;
	TMessageText() {
		value = "";
		pr_del=0;
	}
	TMessageText( const std::string &avalue, int apr_del ) {
		value = avalue;
		pr_del = apr_del;
	}
};

struct TLocaleMessage {
	std::map<std::string,TMessageText> lang_messages;
	void Add( std::string vlang, std::string vmessage, int pr_del ) {
		lang_messages.insert(make_pair(upperc(vlang),TMessageText(vmessage,pr_del)));
	}
	TLocaleMessage( ){};
};

class TLocaleMessages
{
	private:
		int tid;
		std::map<std::string, TLocaleMessage> messages;
	public:
		TLocaleMessages();
		void Clear();
		void Invalidate();
		std::string getText( const std::string &lexema_id, const std::string &lang, bool with_del );
		static TLocaleMessages *Instance();
};

void buildMsg( const std::string &lang, LexemaData &lexemaData, std::string &text, std::string &master_lexema_id );

} //end namespace AstraLocale
#endif /*_ASTRA_LOCALE_H_*/
