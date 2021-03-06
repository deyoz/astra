#ifndef _ASTRA_LOCALE_H_
#define _ASTRA_LOCALE_H_

#include <map>
#include <string>
#include "exceptions.h"
#include "date_time.h"
#include <stl_utils.h>
#include <boost/any.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant.hpp>

enum TLangApplying { applyLang, applyLangForTranslit, applyLangForAll };

namespace AstraLocale {

using BASIC::date_time::TDateTime;

const std::string LANG_RU = "RU";
const std::string LANG_EN = "EN";
const std::string LANG_DEFAULT = LANG_EN;

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
    LParam( std::string aname, size_t avalue ) {
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
    const boost::any & getParam( const std::string &name ) const {
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
    std::string StringValue( const std::string &name, const boost::any &param ) const {
      if ( !boost::any_cast<std::string>(&param) )
        throw EXCEPTIONS::Exception( "TParams: param type is not string, name=%s", name.c_str() );
      return boost::any_cast<std::string>(param);
    }
    std::string StringValue( const std::string &name ) const {
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
    TDateTime DateTimeValue( const std::string &name ) {
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
    LexemaData(const std::string &alexema_id, const LParams &alparams): lexema_id(alexema_id), lparams(alparams) {};
    LexemaData(const std::string &alexema_id): lexema_id(alexema_id) {};
    LexemaData() {};
};

void buildMsg( const std::string &lang, LexemaData &lexemaData, std::string &text, std::string &master_lexema_id );

const std::string VARIABLE_FIRST_ELEM = "[";
const std::string VARIABLE_END_ELEM = "]";
const std::string FORMAT_FIRST_ELEM = "%";
const std::string FORMAT_LEXEMA = "L";
const std::string FORMAT_LINE_BREAK = "EOL";
const std::string FORMAT_MSG = "MSG.";
const std::string FORMAT_COVER = "WRAP.";
enum TLocaleFormat { lfInt, lfDouble, lfDateTime, lfString, lfLexema, lfLineBreak, lfUnknown };

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
        pr_del = 0;
    }
    TMessageText( const std::string &avalue, int apr_del ) {
        value = avalue;
        pr_del = apr_del;
    }
};

struct TLocaleMessage {
    std::map<std::string,TMessageText> lang_messages;
    void Add( std::string vlang, std::string vmessage, int pr_del ) {
        if ( lang_messages.find( vlang ) == lang_messages.end() )
          lang_messages.insert(make_pair(upperc(vlang),TMessageText(vmessage,pr_del)));
        else
            lang_messages[ vlang ] = TMessageText(vmessage,pr_del);
    }
    void Clear( std::string vlang ) {
    if ( lang_messages.find( vlang ) != lang_messages.end() ) {
      lang_messages[ vlang ] = TMessageText( "", 0 );
    }
  }
  bool isEmpty( ) {
    for ( std::map<std::string,TMessageText>::iterator i=lang_messages.begin(); i!=lang_messages.end(); i++ ) {
      if ( !i->second.value.empty() )
        return false;
    }
    return true;
  }
    TLocaleMessage( ){};
};

struct TDictionary {
    std::string value;
    int checksum;
};

struct TMsgs {
    std::map<std::string, int> tids; // ??? ?????? ?????? ????
    std::map<std::string, TLocaleMessage> msgs;
    void clear() {
        tids.clear();
        msgs.clear();
    }
    void clear( const std::string &lang ) {
    tids.erase( lang );
    for ( std::map<std::string, TLocaleMessage>::iterator i=msgs.begin(); i!=msgs.end(); i++ ) {
      i->second.Clear( lang );
      if ( i->second.isEmpty() )
        msgs.erase( i->first );
    }
  }
    int get_tid( const std::string &lang ) {
        if ( tids.find( lang ) != tids.end() )
            return tids[ lang ];
        else return -1;
    }
    void set_tid( const std::string &lang, int tid ) {
        tids[ lang ] = tid;
    }
    void Add( const std::string &id, const std::string &lang, const std::string &message, int pr_del ) {
    msgs[ id ].Add( lang, message, pr_del );
    }
};

class TLocaleMessages
{
    private:
        TMsgs server_msgs;
        TMsgs client_msgs;
        std::map<std::string,TDictionary> dicts;
    public:
        TLocaleMessages();
        void Clear();
        void Invalidate( std::string lang, bool pr_term );
      int checksum(const std::string &lang);
      std::string getDictionary(const std::string &lang);
        std::string getText( const std::string &lexema_id, const std::string &lang );
        static TLocaleMessages *Instance();
};

} //end namespace AstraLocale
#endif /*_ASTRA_LOCALE_H_*/
