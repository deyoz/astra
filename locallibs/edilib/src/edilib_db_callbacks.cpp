#include "edilib_db_callbacks.h"
#include "edi_session.h"
#include "edi_except.h"
#include "serverlib/string_cast.h"
#include "edi_sess_except.h"

#ifdef ENABLE_ORACLE
#include "edilib_dbora_callbacks.h"
#elif ENABLE_PG
// Do nothing
#else 
#error "You have to have at least one impl of DB"
#endif // ENABLE_ORACLE

#define NICKNAME "ROMAN"

namespace edilib {

const EdilibDbCallbacks *EdilibDbCallbacks::_instance = 0;

const EdilibDbCallbacks *EdilibDbCallbacks::instance()
{
    if(not _instance) {
#ifdef ENABLE_ORACLE
        _instance = new EdilibOraCallbacks();
#elif ENABLE_PG
        throw std::runtime_error("EdilibDbCallbacks::instance: unimplemented initializer for PG");
#else 
#error "You have to have at least one impl of DB"
#endif // ENABLE_ORACLE
    }
    return _instance;
}

void EdilibDbCallbacks::setEdilibDbCallbacks(const EdilibDbCallbacks *new_instance)
{
    if(_instance)
        delete _instance;
    _instance = new_instance;
}

EdiSession EdilibDbCallbacks::ediSessionReadByIda(edilib::EdiSessionId_t sess_ida, bool update) const
{
    EdiSession::ReadResult res = ediSessionReadByIda(sess_ida, update, max_wait_session_times);
    switch(res.status)
    {
        case EdiSession::ReadOK:
            return res.ediSession.get();
        case EdiSession::NoDataFound:
            throw EdiSessNotFound(STDLOG, (HelpCpp::string_cast(sess_ida) +
                                  ": No EDIFACT session found by ida").c_str());
        case EdiSession::Locked:
            throw EdiSessLocked(STDLOG, (HelpCpp::string_cast(sess_ida)+
                                ": EDIFACT session locked").c_str());
    }
    throw "never happens";
}

EdiSession EdilibDbCallbacks::ediSessionReadByCarf(const std::string &ourcarf,
                                                   const std::string &othcarf, bool update) const
{
    EdiSession::ReadResult res = ediSessionReadByCarf(ourcarf, othcarf, update, max_wait_session_times);
    switch(res.status)
    {
        case EdiSession::ReadOK:
            return res.ediSession.get();
        case EdiSession::NoDataFound:
            throw EdiSessNotFound(STDLOG,
                                  (ourcarf + "/" + othcarf +
                                  ": No EDIFACT session found by ida").c_str());
        case EdiSession::Locked:
            throw EdiSessLocked(STDLOG,
                                (ourcarf + "/" + othcarf +
                                ": EDIFACT session locked").c_str());
    }
    throw "never happens";
}
}// namespace edilib
