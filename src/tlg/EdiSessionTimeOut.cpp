/*
*  C++ Implementation: EdiSessionTimeOut
*
* Description:
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
*
*/

#include "EdiSessionTimeOut.h"
#include "EdiHandlersFactory.h"
#include "AgentWaitsForRemote.h"
#include "ResponseHandler.h"
#include "remote_results.h"
#include "edi_tlg.h"

#include <edilib/edi_func_cpp.h>
#include <edilib/edi_session.h>
#include <serverlib/ocilocal.h>

#include <boost/scoped_ptr.hpp>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace edifact
{
using namespace edilib;

namespace
{

inline static boost::shared_ptr<AstraEdiSessRD> getSess(EdiSessionId_t id)
{
    boost::shared_ptr<AstraEdiSessRD> psess (new AstraEdiSessRD());
    psess->loadEdiSession(id);
    return psess;
}

}//namespace


void HandleEdiSessTimeOut(const EdiSessionTimeOut & to)
{
    using namespace Ticketing;

    boost::shared_ptr<AstraEdiSessRD> psess = getSess(to.ediSessionId());

    boost::scoped_ptr<TlgHandling::AstraEdiResponseHandler> handler
            (Ticketing::EdiResHandlersFactory(to.answerMsgType(), to.funcCode(), psess));

    if(handler)
    {
        handler->onTimeOut();
        handler->readRemoteResults();
        if(handler->remoteResults())
            handler->remoteResults()->setStatus(RemoteStatus::Timeout);
    }
    else
    {
        LogTrace(TRACE1) << "Nothing to do";
    }
}

//void HandleEdiSessCONTRL(EdiSessionId_t Id)
//{
//    using namespace Ticketing;

//    EdiSessionTimeOut to = EdiSessionTimeOut::readById(Id);
//    boost::shared_ptr<edifact::AstraEdiSessRD> psess = getSess(Id);
//    boost::scoped_ptr<TlgHandling::EdifactResponse> handler
//            (Ticketing::EdiResHandlersFactory(to.answerMsgType(), to.funcCode(), psess));

//    if(!handler)
//    {
//        LogTrace(TRACE1) << "Nothing to do";
//    }
//    else
//    {
//        handler->onCONTRL();
//    }

//    if(handler->remoteResults())
//        handler->remoteResults()->setStatus(RemoteStatus::Contrl);
//}

}//namespace edifact

#if 0

#include "xp_testing.h"
#ifdef XP_TESTING
#include <edilib/edi_session.h>
#include <serverlib/int_parameters_oci.h>
#include <edilib/edi_astra_msg_types.h>
#include "local.h"

using namespace edifact;
namespace {
    void init()
    {
        testInitDB();
        init_locale(Environment::TlgHandler);
    }

    void tear_down()
    {
        testClearShutDB();
    }
}

using namespace edilib;
using namespace Ticketing;
START_TEST (edisess_timeout_1)
{
    try
    {
        EdiSession edis;
        edis.setOurRef("OURREF");
        edis.setOurCarf("CARF");
        edis.setStatus(edi_act_o);
        edis.writeDb();

        OciCpp::oracle_datetime tout;
        EdiSessionTimeOut::add(TKCREQ, "142", edis.ida(), 10);
        make_curs("select time_out from EDISESSION_TIMEOUTS where SESS_IDA = :id").
                bind(":id", edis.ida()).
                def(tout).
                EXfet();
        fail_unless(Dates::second_clock::local_time() + Dates::time_duration(0,0,10) ==
                    Dates::from_oracle_time(tout), "inv timeout");
        try
        {
            EdiSessionTimeOut::add(TKCUAC, "107", edis.ida(), 10);
            fail("check unique");
        }
        catch(std::exception &e)
        {
            LogTrace(TRACE3) << e.what();
        }

        // delete
        EdiSessionTimeOut::deleteDb(edis.ida());
        int one;
        OciCpp::CursCtl cur = make_curs("select 1 from EDISESSION_TIMEOUTS where sess_ida=:id");
        cur.
                bind(":id", edis.ida()).
                def(one).
                EXfet();
        fail_unless(cur.err() == NO_DATA_FOUND, "delete from EDISESSION_TIMEOUTS");
    }
    catch(std::exception &e)
    {
        fail(e.what());
    }
}
END_TEST;

START_TEST (edisess_timeout_2)
{
    try
    {
        EdiSession edis;
        edis.setOurRef("OURREF");
        edis.setOurCarf("CARF");
        edis.setStatus(edi_act_o);
        edis.writeDb();

        EdiSession edis2;
        edis2.setOurRef("OURREF2");
        edis2.setOurCarf("CARF2");
        edis2.setStatus(edi_act_o);
        edis2.writeDb();

        EdiSession edis3;
        edis3.setOurRef("OURREF3");
        edis3.setOurCarf("CARF3");
        edis3.setStatus(edi_act_o);
        edis3.writeDb();

        EdiSessionTimeOut::add(TKCREQ, "142", edis.ida(),10);
        EdiSessionTimeOut::add(TKCUAC, "107", edis2.ida(),10);
        EdiSessionTimeOut::add(TKCREQ, "131", edis3.ida(),10);

        make_curs("update EDISESSION_TIMEOUTS set time_out = SYSDATE-1/86400").exec();

        std::list<EdiSessionTimeOut> lExpired;
        EdiSessionTimeOut::readExpiredSessions(lExpired);
        fail_unless(lExpired.size() == 3, "inv expired sessions num");
        fail_unless(lExpired.back().timeout() == 1, "inv timeout" );
        fail_unless(lExpired.back().funcCode() == "131", "inv func code");
        fail_unless(lExpired.back().msgType() == TKCREQ, "inv msgType");
        fail_unless(lExpired.back().answerMsgType() == TKCRES, "inv answerMsgType");
    }
    catch(std::exception &e)
    {
        fail(e.what());
    }
}
END_TEST;

START_TEST (edisess_timeout_3)
{
    try
    {
        EdiSessionTimeOut edisto(TKCUAC, "107", EdiSessionId_t(10), 11);
        fail_unless(edisto.msgType() == TKCUAC, "inv msgType");
        fail_unless(edisto.funcCode() == "107", "inv func code");
        fail_unless(edisto.answerMsgType()==TKCRES, "inv answerMsgType");
        fail_unless(edisto.ediSessionId() == EdiSessionId_t(10), "inv edisession id");
        fail_unless(edisto.timeout() == 11, "inv timeout");
    }
    catch(std::exception &e)
    {
        fail(e.what());
    }
}
END_TEST;

START_TEST (edisess_timeout_4)
{
    try
    {
        EdiSession edis;
        edis.setOurRef("OURREF");
        edis.setOurCarf("CARF");
        edis.setStatus(edi_act_o);
        edis.writeDb();

        EdiSessionTimeOut edisto(TKCUAC, "107", edis.ida(), 11);
        edisto.writeDb();

        EdiSessionTimeOut edisto2 = EdiSessionTimeOut::readById(edis.ida());
        fail_unless(edisto2.msgType() == TKCUAC, "inv msgType");
        fail_unless(edisto2.funcCode() == "107", "inv func code");
        fail_unless(edisto2.answerMsgType()==TKCRES, "inv answerMsgType");
        fail_unless(edisto2.ediSessionId() == edis.ida(), "inv edisession id");
        fail_unless(edisto2.timeout() == -11, "inv timeout");
    }
    catch(std::exception &e)
    {
        fail(e.what());
    }
}
END_TEST;

#define SUITENAME "edisession_timeout"
TCASEREGISTER( init, tear_down)
{
    ADD_TEST( edisess_timeout_1 );
    ADD_TEST( edisess_timeout_2 );
    ADD_TEST( edisess_timeout_3 );
    ADD_TEST( edisess_timeout_4 );
}
TCASEFINISH;
#endif /*XP_TESTING*/

#endif /*0*/
