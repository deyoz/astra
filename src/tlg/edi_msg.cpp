#include "edi_msg.h"
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

Ticketing::ErrMsg_t EdiErrMsgDataElem::getInnerErrByEdi(const std::string& ediErr,
                                                        const Ticketing::ErrMsg_t& defaultErr)
{
    for(ErrMsgMap_t::const_iterator iter = m_errMsgMap->begin();
          iter != m_errMsgMap->end(); iter++)
    {
        if(iter->second == ediErr)
        {
            return iter->first;
        }
    }

    return defaultErr;
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

    // edifact error codes represented by data element 9321 (ERC)
    EdiErrMsgDataElem* el_9321 = new EdiErrMsgDataElem(9321, EdiErrMsgERC::DefaultEdiErr);
 #define ADD_MSG2(s, e) ADD_MSG(9321, AstraErr::s, e)
     ADD_MSG2(INV_COUPON_STATUS,              "396");
     ADD_MSG2(TICK_NO_MATCH,                  "401");
#undef ADD_MSG2
    (*m_ediErrMsgMap)[9321] = el_9321;

    // edifact error codes represented by data element 9845 (ERD)
    EdiErrMsgDataElem* el_9845 = new EdiErrMsgDataElem(9845, EdiErrMsgERD::DefaultEdiErr);
#define ADD_MSG2(s, e) ADD_MSG(9845, AstraErr::s, e)
#define ADD_MSG3(s, e) ADD_MSG(9845, s, e)

    ADD_MSG2(PAX_SURNAME_NF,                     "1");
    ADD_MSG3("MSG.SEAT_NOT_AVLBL_IN_REQ_ZONE",   "2");
    ADD_MSG3("MSG.INVALID_SEAT_REQUEST",         "3");
    ADD_MSG3("MSG.BAG_TAG_NUM_REQUIRED",         "4");
    ADD_MSG2(INV_FLIGHT_DATE,                    "5");
    ADD_MSG2(TOO_MANY_PAX_WITH_SAME_SURNAME,     "6");
    ADD_MSG3("MSG.PAX_TYPE_GENDER_CONFLICT",     "7");
    ADD_MSG3("MSG.MORE_PRECISE_GENDER_REQUIRED", "8");
    ADD_MSG2(FLIGHT_NOT_FOR_THROUGH_CHECK_IN,    "9");
    ADD_MSG3("MSG.ARRIVAL_INVALID",              "10");
    ADD_MSG3("MSG.INVALID_DEP_ARR_CITY",         "11");
    ADD_MSG3("MSG.UNIQ_NAME_NOT_FOUND",          "12");
    ADD_MSG3("MSG.INVALID_SEAT_NUMBER",          "13");
    ADD_MSG3("MSG.INVALID_AIRCODE_FLNUM",        "14");
    ADD_MSG3("MSG.FLIGHT_CANCELLED",             "15");
    ADD_MSG3("MSG.CHECKIN_HELD_OR_SUSPENDED",    "16");
    ADD_MSG2(PAX_ALREADY_CHECKED_IN,             "17");
    ADD_MSG3("MSG.SEATING_CONFLICT",             "18");
    ADD_MSG2(BAGGAGE_WEIGHT_REQUIRED,            "19");
    ADD_MSG3("MSG.BAGGAGE_CONFLICT",             "20");
    ADD_MSG3("MSG.SEAT_NOT_AVLBL_FOR_PAX_TYPE",  "21");
    ADD_MSG3("MSG.TOO_MANY_CONNECTIONS",         "22");
    ADD_MSG3("MSG.INVALID_BAGGAGE_DEST",         "23");
    ADD_MSG3("MSG.PAX_WEIGHT_REQUIRED",          "24");
    ADD_MSG3("MSG.HAND_BAGGAGE_DETAILS_REQUIRED","25");
    ADD_MSG2(NO_SEAT_SELCTN_ON_FLIGHT,           "26");
    ADD_MSG3("MSG.DEPARTURE_INVALID",            "27");
    ADD_MSG3("MSG.FLIGHT_RESCHEDULED",           "28");
    ADD_MSG3("MSG.INVALID_BAG_TAG_NUMBER",       "32");
    ADD_MSG2(FLIGHT_CLOSED,                      "35");
    ADD_MSG2(TOO_MANY_PAXES,                     "44");
    ADD_MSG3("MSG.INVALID_TICKNUM",              "49");
    ADD_MSG2(TOO_MANY_INFANTS,                   "61");
    ADD_MSG2(SMOKING_ZONE_UNAVAILABLE,           "62");
    ADD_MSG2(NON_SMOKING_ZONE_UNAVAILABLE,       "63");
    ADD_MSG3("MSG.MODIFICATION_NOT_POSSIBLE",    "70");
    ADD_MSG2(UNABLE_TO_GIVE_SEAT,                "72");
    ADD_MSG2(TOO_MANY_BAGS,                      "82");
    ADD_MSG2(FUNC_NOT_SUPPORTED,                 "84");
    ADD_MSG2(EDI_PROC_ERR,                       "102");
    ADD_MSG3("MSG.API_PAX_DATA_REQUIRED",        "193");
    ADD_MSG2(PAX_SURNAME_NOT_CHECKED_IN,         "194");
    ADD_MSG2(TIMEOUT_ON_HOST_3,                  "196");
    ADD_MSG3("MSG.FFP_NUM_ERROR",                "197");
    ADD_MSG3("MSG.CLASS_CODE_REQUIRED",          "198");
    ADD_MSG2(CHECK_IN_SEPARATELY,                "199");
    ADD_MSG3("MSG.FQTV_NUM_NOT_ACCEPTED",        "200");
    ADD_MSG3("MSG.FQTV_NUM_ALREADY_PRESENT",     "201");
    ADD_MSG3("MSG.BAGGAGE_NOT_UPDATED",          "202");
    ADD_MSG3("MSG.SSR_NOT_UPDATED",              "203");
    ADD_MSG2(UPDATE_SEPARATELY,                  "208");
    ADD_MSG3("MSG.API_PAX_DATA_NOT_SUPPORTED",   "212");
    ADD_MSG3("MSG.API_BIRTHDATE_REQUIRED",       "214");
    ADD_MSG3("MSG.API_PASSPORT_NUM_REQUIRED",    "215");
    ADD_MSG3("MSG.API_FIRST_NAME_REQUIRED",      "217");
    ADD_MSG3("MSG.API_GENDER_REQUIRED",          "218");
    ADD_MSG2(CASCADED_QUERY_TIMEOUT,             "254");
    ADD_MSG3("MSG.API_DATA_PRESENT",             "257");
    ADD_MSG2(ID_CARD_REQUIRED,                   "259");
    ADD_MSG3("MSG.VISA_REQUIRED",                "260");
    ADD_MSG3("MSG.HOME_ADDRESS_REQUIRED",        "261");
    ADD_MSG3("MSG.DEST_ADDRESS_REQUIRED",        "262");
    ADD_MSG3("MSG.DEP_ADDRESS_REQUIRED",         "263");
    ADD_MSG3("MSG.INFANT_NAME_REQUIRED",         "267");
    ADD_MSG3("MSG.API_INCOMPLETE",               "276");
    ADD_MSG3("MSG.API_DOCTYPE_REQUIRED",         "277");
    ADD_MSG3("MSG.API_DOCISSUE_COUNTRY_REQUIRED","278");
    ADD_MSG3("MSG.API_FULLNAME_REQUIRED",        "279");
    ADD_MSG2(DOC_EXPIRE_DATE_REQUIRED,           "280");
    ADD_MSG3("MSG.INFANT_CONFLICT",              "282");

#undef ADD_MSG2
#undef ADD_MSG3
    (*m_ediErrMsgMap)[9845] = el_9845;
}

//---------------------------------------------------------------------------------------

std::string EdiErrMsgERC::getEdiErrByInner(const Ticketing::ErrMsg_t& innerErr)
{
    EdiErrMsgDataElem* errMap = getEdiErrMapByDataElem(9321);
    if(errMap) {
        return errMap->getEdiErrByInner(innerErr);
    } else {
        return DefaultEdiErr;
    }
}

Ticketing::ErrMsg_t EdiErrMsgERC::getInnerErrByEdi(const std::string& ediErr,
                                                   const Ticketing::ErrMsg_t& defaultErr)
{
    EdiErrMsgDataElem* errMap = getEdiErrMapByDataElem(9321);
    if(errMap) {
        return errMap->getInnerErrByEdi(ediErr, defaultErr);
    } else {
        return defaultErr;
    }
}

//---------------------------------------------------------------------------------------

std::string EdiErrMsgERD::getEdiErrByInner(const Ticketing::ErrMsg_t& innerErr)
{
    EdiErrMsgDataElem* errMap = getEdiErrMapByDataElem(9845);
    if(errMap) {
        return errMap->getEdiErrByInner(innerErr);
    } else {
        return DefaultEdiErr;
    }
}

Ticketing::ErrMsg_t EdiErrMsgERD::getInnerErrByEdi(const std::string& ediErr,
                                                   const Ticketing::ErrMsg_t& defaultErr)
{
    EdiErrMsgDataElem* errMap = getEdiErrMapByDataElem(9845);
    if(errMap) {
        return errMap->getInnerErrByEdi(ediErr, defaultErr);
    } else {
        return defaultErr;
    }
}

//---------------------------------------------------------------------------------------

std::string getErdErrByInner(const Ticketing::ErrMsg_t& innerErr)
{
    return EdiErrMsgERD::getEdiErrByInner(innerErr);
}

Ticketing::ErrMsg_t getInnerErrByErd(const std::string& erdErr,
                                     const Ticketing::ErrMsg_t& defaultErr)
{
    return EdiErrMsgERD::getInnerErrByEdi(erdErr, defaultErr);
}

std::string getErcErrByInner(const Ticketing::ErrMsg_t& innerErr)
{
    return EdiErrMsgERC::getEdiErrByInner(innerErr);
}

Ticketing::ErrMsg_t getInnerErrByErc(const std::string& ercErr,
                                     const Ticketing::ErrMsg_t& defaultErr)
{
    return EdiErrMsgERC::getInnerErrByEdi(ercErr, defaultErr);
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
        fail_unless(EdiErrMsgERD::getInnerErrByEdi("") == AstraErr::EDI_PROC_ERR,
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
