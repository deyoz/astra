//
// C++ Interface: typeb_pp.h
//
// Description: preprocessor for typeb messages
//
//
// Author: Anton <anton.bokov@sirena2000.ru>, (C) 2010
//
//

#ifndef _TYPEB_PP_H_
#define _TYPEB_PP_H_

#include <list>
#include <string>
#include <boost/shared_ptr.hpp>
#include <serverlib/exception.h>
#include <serverlib/lngv.h>


namespace typeb_pp
{

typedef int TpbPpRuleId_t;
typedef std::list< TpbPpRuleId_t > TpbPpRuleIds_t;

//-----------------------------------------------------------------------------

struct MatchParams
{
    MatchParams()
    {
        reset();
    }

    void reset()
    {
        StartPos = std::string::npos;
        EndPos = std::string::npos;
    }

    size_t StartPos;
    size_t EndPos;
};

//-----------------------------------------------------------------------------

struct RuleParams
{
    RuleParams() :
        SearchPos( 0 )
    {}
    RuleParams( const std::string& src ) :
        Source( src ), SearchPos( 0 )
    {}

    std::string Source;
    std::string Result;
    MatchParams Match;
    size_t SearchPos;
};

//-----------------------------------------------------------------------------

class TpbPpException: public comtech::Exception
{
public:
    TpbPpException( const char* nick, const char* file, unsigned line,
                    const std::string& text ) throw ()
        : comtech::Exception( nick, file, line, "", text )
    {}

    virtual ~TpbPpException() throw () {}
};

//-----------------------------------------------------------------------------

class TpbPpTemplate
{
public:
    enum Kind
    {
        Simple = 0,     // Шаблон для поиска полного соответствия его телу
        Regex = 1,      // Шаблон для поиска соответствия регулярному выражению
    };

    TpbPpTemplate( const std::string& body, bool wholeWords = false ) :
        m_body( body ), m_wholeWords( wholeWords )
    {}

    virtual ~TpbPpTemplate() {}
    virtual Kind kind() const { return Simple; }
    virtual bool search( RuleParams& rp ) const;
    virtual std::string description( Language lang, bool html ) const;

    const std::string& body() const { return m_body; }

    bool wholeWords() const { return m_wholeWords; }
    void setWholeWords( bool wholeWords ) { m_wholeWords = wholeWords; }

protected:
    std::string m_body;
    bool m_wholeWords;
};

typedef boost::shared_ptr< TpbPpTemplate > TpbPpTemplatePtr_t;
typedef boost::shared_ptr< const TpbPpTemplate > CTpbPpTemplatePtr_t;

//-----------------------------------------------------------------------------

class TpbPpRegexTemplate: public TpbPpTemplate
{
public:
    TpbPpRegexTemplate( const std::string& re ) :
        TpbPpTemplate( re )
    {}
    virtual ~TpbPpRegexTemplate() {}
    virtual Kind kind() const { return Regex; }
    virtual bool search( RuleParams& rp ) const;
    virtual std::string description( Language lang, bool html ) const;
};

//-----------------------------------------------------------------------------

class TpbPpAction
{
    std::string m_param;

public:
    enum Kind
    {
        Replace = 0,        // Замена шаблона
        RemoveBefore = 1,   // Удаление текста ДО первой встречи шаблона
        RemoveAfter = 2,    // Удаление текста ПОСЛЕ первой встречи шаблона
        RemoveWholeLine = 3,// Удаление строки, в которой встречается шаблон
    };

    TpbPpAction( const std::string& param = std::string() ) :
        m_param( param )
    {}
    virtual ~TpbPpAction() {}
    virtual Kind kind() const = 0;
    virtual bool perform( RuleParams& rp ) const = 0;
    virtual std::string description( Language lang, bool html ) const = 0;

    const std::string& param() const { return m_param; }
    void setParam( const std::string& par ) { m_param = par; }
};

typedef boost::shared_ptr< TpbPpAction > TpbPpActionPtr_t;
typedef boost::shared_ptr< const TpbPpAction > CTpbPpActionPtr_t;

//-----------------------------------------------------------------------------

class TpbPpReplaceAction: public TpbPpAction
{
public:
    TpbPpReplaceAction( const std::string& replaceWith = std::string() ) :
        TpbPpAction( replaceWith )
    {}
    virtual ~TpbPpReplaceAction() {}
    virtual Kind kind() const { return Replace; }
    virtual bool perform( RuleParams& rp ) const;
    virtual std::string description( Language lang, bool html ) const;

    std::string replaceWith() const { return param(); }
    void setReplaceWith( const std::string& replaceWith ) { setParam( replaceWith ); }
};

//-----------------------------------------------------------------------------

class TpbPpRemoveBeforeAction: public TpbPpAction
{
public:
    TpbPpRemoveBeforeAction( const std::string& param = std::string() ) :
        TpbPpAction( param )
    {}
    virtual ~TpbPpRemoveBeforeAction() {}
    virtual Kind kind() const { return RemoveBefore; }
    virtual bool perform( RuleParams& rp ) const;
    virtual std::string description( Language lang, bool html ) const;

    bool fromLineStart() const { return param() == "fls"; }
    void setFromLineStart( bool fls = true ) { setParam( fls ? "fls" : "" ); }
};

//-----------------------------------------------------------------------------

class TpbPpRemoveAfterAction: public TpbPpAction
{
public:
    TpbPpRemoveAfterAction( const std::string& param = std::string() ) :
        TpbPpAction( param )
    {}
    virtual ~TpbPpRemoveAfterAction() {}
    virtual Kind kind() const { return RemoveAfter; }
    virtual bool perform( RuleParams& rp ) const;
    virtual std::string description( Language lang, bool html ) const;

    bool beforeLineFinish() const { return param() == "blf"; }
    void setBeforeLineFinish( bool blf = true ) { setParam( blf ? "blf" : "" ); }
};

//-----------------------------------------------------------------------------

class TpbPpRemoveWholeLineAction: public TpbPpAction
{
public:
    TpbPpRemoveWholeLineAction( const std::string& param = std::string() ) :
        TpbPpAction( param )
    {}
    virtual ~TpbPpRemoveWholeLineAction() {}
    virtual Kind kind() const { return RemoveWholeLine; }
    virtual bool perform( RuleParams& rp ) const;
    virtual std::string description( Language lang, bool html ) const;
};

//-----------------------------------------------------------------------------

class TpbPpRule
{
public:
    TpbPpRule() :
        m_id( 0 ), m_loopsCounter( 0 )
    {}

    TpbPpRule( const TpbPpRuleId_t& id, const std::string& name ) :
        m_id( id ), m_name( name ), m_loopsCounter( 0 )
    {}

    TpbPpRule( const std::string& name, TpbPpTemplate* templ, TpbPpAction* act ) :
        m_id( 0 ), m_name( name ), m_loopsCounter( 0 )
    {
        m_template.reset( templ );
        m_action.reset( act );
    }

    virtual ~TpbPpRule() {}

    TpbPpRuleId_t id() const { return m_id; }

    const std::string& name() const { return m_name; }
    void setName( const std::string& name ) { m_name = name; }

    virtual std::string description( Language lang = RUSSIAN, bool html = false ) const;

    void setTemplate( TpbPpTemplate* templ ) { m_template.reset( templ ); }
    void setAction( TpbPpAction* act ) { m_action.reset( act ); }

    TpbPpTemplatePtr_t templ() const { return m_template; }
    TpbPpActionPtr_t action() const { return m_action; }

    virtual std::string apply( const std::string& tpbText ) const;

    void setId( const TpbPpRuleId_t& id ) { m_id = id; }
protected:
    bool checkInfiniteLooping() const;

private:
    TpbPpRuleId_t m_id;
    TpbPpTemplatePtr_t m_template;
    TpbPpActionPtr_t m_action;
    std::string m_name;
    mutable size_t m_loopsCounter;
    static const size_t InfiniteLoopsCount = 1000;
};

typedef boost::shared_ptr< TpbPpRule > TpbPpRulePtr_t;
typedef boost::shared_ptr< const TpbPpRule > CTpbPpRulePtr_t;
typedef std::list< TpbPpRulePtr_t > TpbPpRules_t;
typedef std::list< CTpbPpRulePtr_t > CTpbPpRules_t;

//-----------------------------------------------------------------------------

class TpbPpRulesDbIface
{
public:
    virtual void readRules( TpbPpRules_t& rules ) const = 0;
    virtual void writeDb(TpbPpRule &rule) const = 0;
    virtual void updateDb(const TpbPpRule &) const = 0;
    virtual void deleteDb(const TpbPpRule &) const = 0;
    virtual TpbPpRuleId_t generateNexId() const = 0;
    virtual ~TpbPpRulesDbIface() {}
};

//-----------------------------------------------------------------------------

class TpbPpBaseRulesStorage
{
public:
    TpbPpBaseRulesStorage();

    const TpbPpRules_t& rules() const { return m_rules; }
    TpbPpRules_t rules( const TpbPpRuleIds_t& ids ) const;

    TpbPpRulePtr_t findRule( TpbPpRuleId_t ruleId, bool throwNf = false ) const;

    void addRule( TpbPpRulePtr_t rule );
    void updateRule( TpbPpRulePtr_t rule );
    void deleteRule( TpbPpRuleId_t ruleId );
    void syncDb();
    static void setDbIface(TpbPpRulesDbIface *iface) { DbIface.reset(iface); };
    static const TpbPpRulesDbIface* dbIface();
protected:
    TpbPpRules_t m_rules;
    static boost::shared_ptr<TpbPpRulesDbIface> DbIface;
};

//-----------------------------------------------------------------------------

class TpbPpBaseRulesLoader
{
public:
    TpbPpBaseRulesLoader( const TpbPpBaseRulesStorage* rulesStorage ) :
        m_rulesStorage( rulesStorage )
    {}

    virtual ~TpbPpBaseRulesLoader() {}

    TpbPpRules_t loadRules() const;

protected:
    virtual TpbPpRuleIds_t loadRuleIds() const = 0;

private:
    TpbPpBaseRulesStorage const* m_rulesStorage;
};

typedef boost::shared_ptr< TpbPpBaseRulesLoader > TpbPpBaseRulesLoaderPtr_t;
typedef boost::shared_ptr< const TpbPpBaseRulesLoader > CTpbPpBaseRulesLoaderPtr_t;

//-----------------------------------------------------------------------------

class TpbPp
{
public:
    TpbPp( const std::string& srcText, TpbPpBaseRulesLoader const* rulesLoader ) :
        m_srcText( srcText )
    {
        m_rulesLoader.reset( rulesLoader );
    }

    std::string preprocess();
    static std::string preprocess( const std::string& srcText,
                                   TpbPpBaseRulesLoader const* rulesLoader );

private:
    std::string m_srcText;
    CTpbPpBaseRulesLoaderPtr_t m_rulesLoader;
};

//-----------------------------------------------------------------------------

class TpbPpTemplateFactory
{
public:
    static TpbPpTemplate* createTemplate( TpbPpTemplate::Kind kind,
                                          const std::string& body );
    static TpbPpTemplate::Kind short2kind( short shortKind );
};

//-----------------------------------------------------------------------------

class TpbPpActionFactory
{
public:
    static TpbPpAction* createAction( TpbPpAction::Kind kind,
                                      const std::string& actParam = std::string() );
    static TpbPpAction::Kind short2kind( short shortKind );
};

//-----------------------------------------------------------------------------

// Маскирование/Демаскирование символа перевода строки (\n)
std::string outerEndls2Inner( const std::string& outer );
std::string innerEndls2Outer( const std::string& inner );
// Маскирование/Демаскирование символа возврата каретки (\r)
std::string outerRets2Inner( const std::string& outer );
std::string innerRets2Outer( const std::string& inner );
// Маскирование/Демаскирование шаблона
std::string outerTempl2Inner( const std::string& outer );
std::string innerTempl2Outer( const std::string& inner );


}//namespace typeb_pp

#endif /*_TYPEB_PP_H_*/
