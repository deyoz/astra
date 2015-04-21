#include "edi_msg.h"
#include "astra_msg.h"
#include "config.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>

namespace edifact {

using namespace Ticketing;

// default edifact error code represented by de_9321 (ERC)
const std::string EdiErrMsgERC::DefaultEdiErr = "118";

// default edifact error code represented by de_9845 (ERD)
const std::string EdiErrMsgERD::DefaultEdiErr = "102";

// default inner error
const Ticketing::ErrMsg_t EdiErrMsg::DefaultInnerErr = AstraErr::EDI_PROC_ERR;


EdiErrMsg::EdiErrMsgMap_t* EdiErrMsg::m_ediErrMsgMap = 0;


EdiErrMsgDataElem::EdiErrMsgDataElem(int de, const std::string& defCode)
    : m_dataElem(de), m_defCode(defCode)
{
    m_errMsgMap = new ErrMsgMap_t();
}


void EdiErrMsgDataElem::addElement(const Ticketing::ErrMsg_t& innerErr,
                                   const std::string& ediErr)
{
    (*m_errMsgMap)[innerErr] = ediErr;
}

const std::string& EdiErrMsgDataElem::getEdiErrByInner(const Ticketing::ErrMsg_t& innerErr)
{
    ErrMsgMap_t::const_iterator i = m_errMsgMap->find(innerErr);
    if(i == m_errMsgMap->end())
    {
        LogTrace(TRACE3) << "No such edifact error code for inner " << innerErr;
        return m_defCode;
    }
    return i->second;
}

Ticketing::ErrMsg_t EdiErrMsgDataElem::getInnerErrByEdi(const std::string& ediErr)
{
    for(ErrMsgMap_t::const_iterator iter = m_errMsgMap->begin();
          iter != m_errMsgMap->end(); iter++)
    {
        if(iter->second == ediErr)
        {
            return iter->first;
        }
    }

    return EdiErrMsg::DefaultInnerErr;
}

//---------------------------------------------------------------------------------------

EdiErrMsg::EdiErrMsgMap_t* EdiErrMsg::ediErrMsgMap()
{
    if(!m_ediErrMsgMap)
    {
        m_ediErrMsgMap = new EdiErrMsgMap_t();
        init();
    }
    return m_ediErrMsgMap;
}

EdiErrMsgDataElem* EdiErrMsg::getEdiErrMapByDataElem(int de)
{
    EdiErrMsgMap_t::const_iterator iter = ediErrMsgMap()->find(de);
    if(iter == ediErrMsgMap()->end())
    {
        LogError(STDLOG) << "EdiErrMsgDataElem not found for data element: " << de;
        return 0;
    }
    return iter->second;
}

#define ADD_MSG(n, s, e) \
    el_##n->addElement(s,e);

void EdiErrMsg::init()
{
    LogTrace(TRACE5) << "Initializing edi_msg...";
    // edifact error codes represented by data element 9845 (ERD)
    EdiErrMsgDataElem* el_9845 = new EdiErrMsgDataElem(9845, EdiErrMsgERD::DefaultEdiErr);
#define ADD_MSG2(s, e) ADD_MSG(9845, AstraErr::s, e)

    ADD_MSG2(EDI_PROC_ERR,                  "102");
    ADD_MSG2(TIMEOUT_ON_HOST_3,             "196");

#undef ADD_MSG2
    (*m_ediErrMsgMap)[9845] = el_9845;
}

//---------------------------------------------------------------------------------------

std::string EdiErrMsgERC::getEdiErrByInner(const Ticketing::ErrMsg_t& innerErr)
{
    // TODO
    return DefaultEdiErr;
}

Ticketing::ErrMsg_t EdiErrMsgERC::getInnerErrByEdi(const std::string& ediErr)
{
    // TODO
    return DefaultInnerErr;
}

//---------------------------------------------------------------------------------------

std::string EdiErrMsgERD::getEdiErrByInner(const Ticketing::ErrMsg_t& innerErr)
{
    EdiErrMsgDataElem* errMap = getEdiErrMapByDataElem(9845);
    if(errMap)
        return errMap->getEdiErrByInner(innerErr);
    else
        return DefaultEdiErr;
}

Ticketing::ErrMsg_t EdiErrMsgERD::getInnerErrByEdi(const std::string& ediErr)
{
    EdiErrMsgDataElem* errMap = getEdiErrMapByDataElem(9845);
    if(errMap)
        return errMap->getInnerErrByEdi(ediErr);
    else
        return DefaultInnerErr;
}

//---------------------------------------------------------------------------------------

std::string getErdErrByInner(const Ticketing::ErrMsg_t& innerErr)
{
    return EdiErrMsgERD::getEdiErrByInner(innerErr);
}

Ticketing::ErrMsg_t getInnerErrByErd(const std::string& erdErr)
{
    return EdiErrMsgERD::getInnerErrByEdi(erdErr);
}

}//namespace edifact

/////////////////////////////////////////////////////////////////////////////////////////

#ifdef XP_TESTING
#include "xp_testing.h"

#include <serverlib/checkunit.h>


using namespace edifact;


namespace
{
    void init() {}
    void tear_down() {}
}//namespace

START_TEST(erd_edi_err_by_inner)
{
    try
    {
        fail_unless(EdiErrMsgERD::getEdiErrByInner(AstraErr::TIMEOUT_ON_HOST_3) == "196",
                    "inv edi error TIMEOUT_ON_HOST_3");
        fail_unless(EdiErrMsgERD::getEdiErrByInner(AstraErr::EDI_PROC_ERR) == "102",
                    "inv edi error EDI_PROC_ERR");
        fail_unless(EdiErrMsgERD::getEdiErrByInner("467812") == EdiErrMsgERD::DefaultEdiErr,
                    "inv default edi error");
        fail_unless(EdiErrMsgERD::DefaultEdiErr == "102",
                    "inv edi default error");
    }
    catch(std::exception& e)
    {
        fail_unless(0, e.what());
    }
}
END_TEST

START_TEST(inner_err_by_edi_erd)
{
    try
    {
        fail_unless(EdiErrMsgERD::getInnerErrByEdi("196") == AstraErr::TIMEOUT_ON_HOST_3,
                    "inv edi error with code 196" );
        fail_unless(EdiErrMsgERD::getInnerErrByEdi("102") == AstraErr::EDI_PROC_ERR,
                    "inv edi error EDI_PROC_ERR");
        fail_unless(EdiErrMsgERD::getInnerErrByEdi("") == EdiErrMsgERD::DefaultInnerErr,
                    "inv default inner error");
    }
    catch(std::exception& e)
    {
        fail_unless(0, e.what());
    }
}
END_TEST


#define SUITENAME "edi_msg"
TCASEREGISTER(init, tear_down)
{
    // 9845 (ERD)
    ADD_TEST(erd_edi_err_by_inner);
    ADD_TEST(inner_err_by_edi_erd);
}
TCASEFINISH;
#undef SUITENAME


// dummy function to enable xp-testing
void init_edi_msg_tests() {}

#endif //XP_TESTING
