/*
*  C++ Implementation: typeb_pp.cc
*
* Description: preprocessor for typeb messages
*
*
* Author: Anton <anton.bokov@sirena2000.ru>, (C) 2010
*
*/

#include "typeb_pp.h"

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>
#include <serverlib/str_utils.h>

#include <ctype.h>
#include <boost/regex.hpp>


//#define TPB_PP_DEBUG

namespace typeb_pp
{

using std::string;
boost::shared_ptr<TpbPpRulesDbIface> TpbPpBaseRulesStorage::DbIface;

bool TpbPpTemplate::search( RuleParams& rp ) const
{
    rp.Result = rp.Source;
    rp.Match.StartPos = rp.Source.find( m_body, rp.SearchPos );
#ifdef TPB_PP_DEBUG
    LogTrace( TRACE3 ) << "SearchPos: " << rp.SearchPos;
#endif /*TPB_PP_DEBUG*/
    bool matched = ( rp.Match.StartPos != string::npos );
    if( matched )
    {
        rp.Match.EndPos = rp.Match.StartPos + m_body.length() - 1;
#ifdef TPB_PP_DEBUG
        LogTrace( TRACE3 ) << "Source[" << rp.Match.StartPos << "]: " << rp.Source[ rp.Match.StartPos ];
        LogTrace( TRACE3 ) << "Source[" << rp.Match.EndPos << "]: " << rp.Source[ rp.Match.EndPos ];
#endif /*TPB_PP_DEBUG*/

        if ( wholeWords() )
        {
            if( ( rp.Match.StartPos > 0 ) &&
                ( !isspace( ( int )rp.Source[ rp.Match.StartPos - 1 ] ) ) )
            {
#ifdef TPB_PP_DEBUG
                tst();
#endif /*TPB_PP_DEBUG*/
                matched = false;
            }
            if( ( rp.Match.EndPos < rp.Source.length() - 1 ) &&
                ( !isspace( ( int )rp.Source[ rp.Match.EndPos + 1 ] ) ) )
            {
#ifdef TPB_PP_DEBUG
                tst();
#endif /*TPB_PP_DEBUG*/
                matched = false;
            }

            if( !matched )
            {
#ifdef TPB_PP_DEBUG
                tst();
#endif /*TPB_PP_DEBUG*/
                rp.SearchPos = rp.Match.EndPos;
                return search( rp );
            }
        }
    }
    return matched;
}

string TpbPpTemplate::description( Language lang, bool html ) const
{
    string res = ( lang == RUSSIAN ? "шаблона " : "of template " );
    if( html )
    {
        res += "<font color='blue'>";
    }
    res += innerTempl2Outer( body() );
    if( html )
    {
        res += "</font>";
    }
    return res;
}

//-----------------------------------------------------------------------------

bool TpbPpRegexTemplate::search( RuleParams& rp ) const
{
    string reBody = m_body;
    if( wholeWords() )
        reBody = "\\<" + m_body + "\\>";

    rp.Result = rp.Source;
    string::const_iterator begin = rp.Source.begin(),
                             end = rp.Source.end();

#ifdef TPB_PP_DEBUG
    LogTrace( TRACE3 ) << "SearchPos: " << rp.SearchPos;
#endif /*TPB_PP_DEBUG*/

    boost::smatch what;
    bool matched = boost::regex_search( begin + rp.SearchPos, end, what, boost::regex( reBody ) );
    if( matched )
    {
        rp.Match.StartPos = what[ 0 ].first - begin;
        rp.Match.EndPos = what[ 0 ].second - begin - 1;
#ifdef TPB_PP_DEBUG
        LogTrace( TRACE3 ) << "Source[" << rp.Match.StartPos << "]: " << rp.Source[ rp.Match.StartPos ];
        LogTrace( TRACE3 ) << "Source[" << rp.Match.EndPos << "]: " << rp.Source[ rp.Match.EndPos ];
#endif /*TPB_PP_DEBUG*/

        if( wholeWords() )
        {
            if( ( rp.Match.StartPos > 0 ) &&
                ( !isspace( (int )rp.Source[ rp.Match.StartPos - 1 ] ) ) )
            {
#ifdef TPB_PP_DEBUG
                tst();
#endif /*TPB_PP_DEBUG*/
                matched = false;
            }
            if( ( rp.Match.EndPos < rp.Source.length() - 1 ) &&
                ( !isspace( ( int )rp.Source[ rp.Match.EndPos + 1 ] ) ) )
            {
#ifdef TPB_PP_DEBUG
                tst();
#endif /*TPB_PP_DEBUG*/
                matched = false;
            }

            if( !matched )
            {
#ifdef TPB_PP_DEBUG
                tst();
#endif /*TPB_PP_DEBUG*/
                rp.SearchPos = rp.Match.EndPos;
                return search( rp );
            }
        }
    }
    return matched;
}

string TpbPpRegexTemplate::description( Language lang, bool html ) const
{
    string res = ( lang == RUSSIAN ? "рег.выражения " : "of regexp " );
    if( html )
    {
        res += "<font color='green'>";
    }
    res += innerTempl2Outer( body() );
    if( html )
    {
        res += "</font>";
    }
    return res;
}

//-----------------------------------------------------------------------------

bool TpbPpReplaceAction::perform( RuleParams& rp ) const
{
    if( rp.Match.StartPos != string::npos )
    {
        rp.Result = rp.Source.substr( 0, rp.Match.StartPos );
        rp.Result += replaceWith();
        rp.SearchPos = rp.Result.length() - 1;

#ifdef TPB_PP_DEBUG
        LogTrace( TRACE3 ) << "SearchPos new: " << rp.SearchPos;
#endif /*TPB_PP_DEBUG*/

        if( rp.Match.EndPos != string::npos )
        {
            rp.Result += rp.Source.substr( rp.Match.EndPos + 1, rp.Source.length() );
        }
    }
#ifdef TPB_PP_DEBUG
    LogTrace( TRACE3 ) << "Source:\n" << rp.Source;
    LogTrace( TRACE3 ) << "Result:\n" << rp.Result;
#endif /*TPB_PP_DEBUG*/
    return ( rp.Result != rp.Source );
}

string TpbPpReplaceAction::description( Language lang, bool html ) const
{
    return ( lang == RUSSIAN ? "Замена " : "Replace " );
}

//-----------------------------------------------------------------------------

bool TpbPpRemoveBeforeAction::perform( RuleParams& rp ) const
{
#ifdef TPB_PP_DEBUG
    tst();
#endif /*TPB_PP_DEBUG*/
    bool actionNotNeeded = false;
    if( rp.Match.StartPos != string::npos )
    {
#ifdef TPB_PP_DEBUG
        tst();
#endif /*TPB_PP_DEBUG*/
        rp.Result = rp.Source.substr( rp.Match.StartPos );
        if( rp.Match.EndPos != string::npos )
        {
            rp.SearchPos = rp.Match.EndPos;
        }
        else
        {
            rp.SearchPos = 0;
        }

#ifdef TPB_PP_DEBUG
        LogTrace( TRACE3 ) << "SearchPos new: " << rp.SearchPos;
#endif /*TPB_PP_DEBUG*/

        if( fromLineStart() )
        {
            size_t i;
            for( i = rp.Match.StartPos; rp.Source[ i ] != '\n' && i > 0; i-- );
            actionNotNeeded = ( ( i == rp.Match.StartPos - 1 ) || ( rp.Match.StartPos == 0 ) );
#ifdef TPB_PP_DEBUG
            if( actionNotNeeded )
            {
                LogTrace( TRACE3 ) << "action not needed";
            }
            LogTrace( TRACE3 ) << "i = " << i;
            LogTrace( TRACE3 ) << "Source[i] = " << rp.Source[ i ];
#endif /*TPB_PP_DEBUG*/
            if( i > 0 )
            {
                rp.Result = rp.Source.substr( 0, i + 1 ) + rp.Result;
            }
        }
    }
#ifdef TPB_PP_DEBUG
    LogTrace( TRACE3 ) << "Source:\n" << rp.Source;
    LogTrace( TRACE3 ) << "Result:\n" << rp.Result;
#endif /*TPB_PP_DEBUG*/
    return ( ( rp.Result != rp.Source ) || actionNotNeeded );
}

string TpbPpRemoveBeforeAction::description( Language lang, bool html ) const
{
    return ( lang == RUSSIAN ? "Удаление до " : "Remove before " );
}

//-----------------------------------------------------------------------------

bool TpbPpRemoveAfterAction::perform( RuleParams& rp ) const
{
#ifdef TPB_PP_DEBUG
    tst();
#endif /*TPB_PP_DEBUG*/
    bool actionNotNeeded = false;
    if( rp.Match.EndPos != string::npos )
    {
#ifdef TPB_PP_DEBUG
        tst();
#endif /*TPB_PP_DEBUG*/
        rp.Result = rp.Source.substr( 0, rp.Match.EndPos + 1 );
        rp.SearchPos = rp.Match.EndPos;

#ifdef TPB_PP_DEBUG
        LogTrace( TRACE3 ) << "SearchPos new: " << rp.SearchPos;
#endif /*TPB_PP_DEBUG*/

        if( beforeLineFinish() )
        {
#ifdef TPB_PP_DEBUG
            tst();
#endif /*TPB_PP_DEBUG*/
            size_t len = rp.Source.length();
            size_t i;
            for( i = rp.Match.EndPos; rp.Source[ i ] != '\n' && i < len - 1; i++ );
            actionNotNeeded = ( ( i == rp.Match.EndPos + 1 ) || ( rp.Match.EndPos == rp.Source.length() - 1 ) );
#ifdef TPB_PP_DEBUG
            if( actionNotNeeded )
            {
                LogTrace( TRACE3 ) << "action not needed";
            }
            LogTrace( TRACE3 ) << "i = " << i;
            LogTrace( TRACE3 ) << "source[i] = " << rp.Source[ i ];
#endif /*TPB_PP_DEBUG*/
            if( i > 0 && i < len - 1 )
            {
                rp.Result += rp.Source.substr( i );
            }
        }
    }
#ifdef TPB_PP_DEBUG
    LogTrace( TRACE3 ) << "Source:\n" << rp.Source;
    LogTrace( TRACE3 ) << "Result:\n" << rp.Result;
#endif /*TPB_PP_DEBUG*/
    return ( ( rp.Result != rp.Source ) || actionNotNeeded );
}

string TpbPpRemoveAfterAction::description( Language lang, bool html ) const
{
    return ( lang == RUSSIAN ? "Удаление после " : "Remove after " );
}

//-----------------------------------------------------------------------------

bool TpbPpRemoveWholeLineAction::perform( RuleParams& rp ) const
{
#ifdef TPB_PP_DEBUG
    tst();
#endif /*TPB_PP_DEBUG*/
    if( rp.Match.StartPos != string::npos && rp.Match.EndPos != string::npos )
    {
#ifdef TPB_PP_DEBUG
        tst();
#endif /*TPB_PP_DEBUG*/
        size_t len = rp.Source.length();

        size_t i1;
        for( i1 = rp.Match.StartPos; rp.Source[ i1 ] != '\n' && i1 > 0; i1-- );

        size_t i2;
        for( i2 = rp.Match.EndPos; rp.Source[ i2 ] != '\n' && i2 < len - 1; i2++ );

        if( i1 == 0 )
        {
            i2++;
        }

#ifdef TPB_PP_DEBUG
        LogTrace( TRACE3 ) << "i1 = " << i1;
        LogTrace( TRACE3 ) << "Source[i1] = " << rp.Source[ i1 ];
        LogTrace( TRACE3 ) << "i2 = " << i2;
        LogTrace( TRACE3 ) << "Source[i2] = " << rp.Source[ i2 ];
#endif /*TPB_PP_DEBUG*/

        rp.Result = "";
        if( i1 > 0 )
        {
            rp.Result += rp.Source.substr( 0, i1 );
        }

        if( i2 > 0 && i2 < len - 1 )
        {
            rp.Result += rp.Source.substr( i2 );
        }
    }
#ifdef TPB_PP_DEBUG
    LogTrace( TRACE3 ) << "Source:\n" << rp.Source;
    LogTrace( TRACE3 ) << "Result:\n" << rp.Result;
#endif /*TPB_PP_DEBUG*/
    return ( rp.Result != rp.Source );
}

string TpbPpRemoveWholeLineAction::description( Language lang, bool html ) const
{
    return ( lang == RUSSIAN ? "Удаление строки после встречи " : "Remove line where matched " );
}

//-----------------------------------------------------------------------------

string TpbPpRule::description( Language lang, bool html ) const
{
    string res = "";
    if( html )
    {
        res += "<html>";
    }
    res += m_action->description( lang, html ) + m_template->description( lang, html );
    if( html )
    {
        res += "</html>";
    }
    return res;
}

string TpbPpRule::apply( const string& tpbText ) const
{
    m_loopsCounter = 0;

    if( m_template && m_action )
    {
        RuleParams rp( tpbText );

        string saveSource = rp.Source;
        while( m_template->search( rp ) )
        {
#ifdef TPB_PP_DEBUG
            LogTrace( TRACE3 ) << "search successfull for template: " << m_template->description( ENGLISH, false );
#endif /*TPB_PP_DEBUG*/
            if( !m_action->perform( rp ) )
            {
                LogTrace( TRACE0 ) << "can't to perform typeb preprocessor action";
                break;
            }
            rp.Source = rp.Result;

            if( !checkInfiniteLooping() )
            {
                rp.Source = rp.Result = saveSource;
                break;
            }
        }
        return rp.Result;
    }

    throw TpbPpException( STDLOG, "invalid typeb preprocessor rule" );
}

bool TpbPpRule::checkInfiniteLooping() const
{
    if( ++m_loopsCounter > InfiniteLoopsCount )
    {
        LogError( STDLOG ) << "Infinite looping while apply rule[" << id() << "]. Rollback...";
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------

struct FindTpbPpRule
{
    TpbPpRuleId_t const& rr ;
    bool operator()( const TpbPpRulePtr_t& lr) const
    {
        return ( lr->id() && ( lr->id() == rr ) );
    }
};

//-----------------------------------------------------------------------------

TpbPpBaseRulesStorage::TpbPpBaseRulesStorage()
{
    syncDb();
}

TpbPpRules_t TpbPpBaseRulesStorage::rules( const TpbPpRuleIds_t& ids ) const
{
    TpbPpRules_t rules;
    for( TpbPpRulePtr_t rule:  m_rules )
    {
        if( find( ids.begin(), ids.end(), rule->id() ) != ids.end() )
        {
            rules.push_back( rule );
        }
    }
    return rules;
}

TpbPpRulePtr_t TpbPpBaseRulesStorage::findRule( TpbPpRuleId_t ruleId, bool throwNf ) const
{
    TpbPpRules_t::const_iterator it = find_if( m_rules.begin(),
                                               m_rules.end(),
                                               FindTpbPpRule { ruleId } );
    if( it != m_rules.end() )
    {
        return *it;
    }

    if( throwNf )
    {
        throw TpbPpException( STDLOG, "rule not found" );
    }
    return TpbPpRulePtr_t();
}

void TpbPpBaseRulesStorage::addRule( TpbPpRulePtr_t rule )
{
    TpbPpBaseRulesStorage::dbIface()->writeDb(*rule);
    m_rules.push_back( rule );
}

void TpbPpBaseRulesStorage::updateRule( TpbPpRulePtr_t rule )
{
    if( !findRule( rule->id() ) )
    {
        throw TpbPpException( STDLOG, "attempt to update rule which not found in memory" );
    }
    TpbPpBaseRulesStorage::dbIface()->updateDb(*rule);
}

void TpbPpBaseRulesStorage::deleteRule( TpbPpRuleId_t ruleId )
{
    TpbPpRules_t::iterator it = find_if( m_rules.begin(),
                                         m_rules.end(),
                                         FindTpbPpRule { ruleId } );
    if( it == m_rules.end() )
    {
        throw TpbPpException( STDLOG, "attempt to delete rule which not found in memory" );
    }
    TpbPpBaseRulesStorage::dbIface()->deleteDb(**it);
    m_rules.erase( it );
}

void TpbPpBaseRulesStorage::syncDb()
{
    TpbPpBaseRulesStorage::dbIface()->readRules( m_rules );
}

const TpbPpRulesDbIface* TpbPpBaseRulesStorage::dbIface()
{
    ASSERT(!!DbIface);
    return DbIface.get();
}

//-----------------------------------------------------------------------------

TpbPpRules_t TpbPpBaseRulesLoader::loadRules() const
{
    return m_rulesStorage->rules( loadRuleIds() );
}

//-----------------------------------------------------------------------------

string TpbPp::preprocess()
{
    TpbPpRules_t rules = m_rulesLoader->loadRules();
    string tlgTextNew( m_srcText );
    for( TpbPpRulePtr_t rule:  rules )
    {
        LogTrace( TRACE3 ) << "apply rule[" << rule->id() << "]: " << rule->description( ENGLISH );
        tlgTextNew = rule->apply( tlgTextNew );
    }
    return tlgTextNew;
}

string TpbPp::preprocess( const string& tlgText,
                          TpbPpBaseRulesLoader const* rulesLoader)
{
    return TpbPp( tlgText, rulesLoader ).preprocess();
}

//-----------------------------------------------------------------------------

TpbPpTemplate* TpbPpTemplateFactory::createTemplate( TpbPpTemplate::Kind kind,
                                                     const string& body )
{
    switch( kind )
    {
        case TpbPpTemplate::Simple:
            return new TpbPpTemplate( body );
        case TpbPpTemplate::Regex:
            return new TpbPpRegexTemplate( body );
        default:
            break;
    };

    throw TpbPpException( STDLOG, "TpbPpTemplateFactory::createTemplate() failed" );
}

TpbPpTemplate::Kind TpbPpTemplateFactory::short2kind( short shortKind )
{
    // TODO throw if shortKind not in enum TpbPpTemplate::Kind
    return (TpbPpTemplate::Kind)shortKind;
}

//-----------------------------------------------------------------------------

TpbPpAction* TpbPpActionFactory::createAction( TpbPpAction::Kind kind,
                                               const string& actParam )
{
    switch( kind )
    {
        case TpbPpAction::Replace:
            return new TpbPpReplaceAction( actParam );
        case TpbPpAction::RemoveBefore:
            return new TpbPpRemoveBeforeAction( actParam );
        case TpbPpAction::RemoveAfter:
            return new TpbPpRemoveAfterAction( actParam );
        case TpbPpAction::RemoveWholeLine:
            return new TpbPpRemoveWholeLineAction( actParam );
        default:
            break;
    };

    throw TpbPpException( STDLOG, "TpbPpActionFactory::createAction() failed" );
}

TpbPpAction::Kind TpbPpActionFactory::short2kind( short shortKind )
{
    // TODO throw if shortKind not in enum TpbPpAction::Kind
    return (TpbPpAction::Kind)shortKind;
}

//-----------------------------------------------------------------------------

string outerEndls2Inner( const string& outer )
{
    return StrUtils::ReplaceInStr( outer, "\\\\n", "\n" );
}

string innerEndls2Outer( const string& inner )
{
    return StrUtils::ReplaceInStr( inner, "\n", "\\\\n" );
}

string outerRets2Inner( const string& outer )
{
    return StrUtils::ReplaceInStr( outer, "\\\\r", "\r" );
}

string innerRets2Outer( const string& inner )
{
    return StrUtils::ReplaceInStr( inner, "\r", "\\\\r" );
}

string outerTempl2Inner( const string& outer )
{
    LogTrace( TRACE5 ) << "outer template string:\n" << outer;
    string res;
    res = outerEndls2Inner( outer );
    res = outerRets2Inner( res );
    LogTrace( TRACE5 ) << "inner template string:\n" << res;
    return res;
}

string innerTempl2Outer( const string& inner )
{
    LogTrace( TRACE5 ) << "inner template string:\n" << inner;
    string res;
    res = innerEndls2Outer( inner );
    res = innerRets2Outer( res );
    LogTrace( TRACE5 ) << "outer template string:\n" << res;
    return res;
}


}//namespace typeb_pp
