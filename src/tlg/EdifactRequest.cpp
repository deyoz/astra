/*
*  C++ Implementation: EdifactRequest
*
* Description: Makes an EDIFACT request structure
*
*
* Author: Komtech-N <rom@sirena2000.ru>, (C) 2007
*
*/
#include <memory.h>
#include <boost/scoped_ptr.hpp>

#include "EdifactRequest.h"
#include "edilib/edi_func_cpp.h"
#include "exceptions.h"
#include "EdiSessionAstra.h"
#define ETICK_CHECK_PRIVATE
// #include "remote_system_context.h"
// #include "tlg_source_edifact.h"
// #include "obr_tlg_queue.h"
// #include "EdiSessionTimeOut.h"
// #include "AgentWaitsForRemote.h"

#define NICKNAME "ROMAN"
#define NICK_TRACE ROMAN_TRACE
#include "serverlib/slogger.h"

namespace edifact
{
using namespace EXCEPTIONS;

EdifactRequest::EdifactRequest(const std::string &pult,
                               const RemoteSystemContext::SystemContext *SCont,
                               edi_msg_types_t msg_type)
    :edilib::EdifactRequest(msg_type), TlgOut(0)
{
    setEdiSessionController(new AstraEdiSessWR(pult));
}

EdifactRequest::~EdifactRequest()
{
    delete TlgOut;
}

const RemoteSystemContext::SystemContext * EdifactRequest::sysCont()
{
    return 0;//dynamic_cast<AstraEdiSessWR *>(ediSess())->sysCont();
}

void EdifactRequest::sendTlg()
{
    if(TlgOut)
        delete TlgOut;

//     LogTrace(TRACE2) <<
//             "RouterId: " << sysCont()->router() <<
//             "; Router H2h: " << (ediSess()->hth()?"yes":"NO");

//     TlgOut = new TlgSourceEdifact(makeEdifactText(), ediSess()->hth());

//     TlgOut->setToRot( sysCont()->router() );

//     LogTrace(TRACE1) << *TlgOut;

//     TlgHandling::TlgQueue::putTlg2OutQueue(*TlgOut);
    ediSess()->ediSession()->CommitEdiSession();
    // Записать информацию о timeout отправленной телеграммы
//     EdiSessionTimeOut::add(ediSess()->edih()->msg_type,
//                            MsgFuncCode,
//                            ediSessId(),
//                            sysCont()->edifactResponseTimeOut());

//     Ticketing::ConfigAgentToWait(sysCont()->ida(),
//                                  Ticketing::EdiSessionId_t(ediSess()->ediSession()->ida()));
}

const TlgHandling::TlgSourceEdifact * EdifactRequest::tlgOut() const
{
    return TlgOut;
}

int EdifactRequest::ediSessId() const
{
    return i_ediSessId();
}

} // namespace edifact

#include "xp_testing.h"
#ifdef XP_TESTING
#include "basetables.h"
#include "edilib/edi_tick_msg_types.h"
#include "serverlib/ocilocal.h"
#include "TicketBaseTypesOci.h"
#include "dates_oci.h"
// #include "edi_tlg.h"
#include "local.h"
using namespace edifact;
using namespace OciCpp;
namespace {
    void init()
    {
        testInitDB();
//         edifact::init_sirena_edifact();
        init_locale(Environment::TlgHandler);
    }

    void tear_down()
    {
        testClearShutDB();
    }
}

START_TEST (edifact_request_case1)
{
    using namespace Ticketing::RemoteSystemContext;
    using namespace OciCpp;
    make_curs ("update rot set H2H_ADDR=:h2haddr, OUR_H2H_ADDR=:h2hour, H2H=:h2h,"
            "RESP_TIMEOUT=4 where ida = :id").
            bind(":h2haddr", "TSTDEST").
            bind(":h2hour", "TSTSRC").
            bind(":h2h",1).
            bind(":id",1).
            exec();

    EtsSystemContext *ets = EtsSystemContext::initEdifactAndCreateDefault("ETY1", "ET1Y",
            BaseTables::Company("Y1")->ida(),
                                BaseTables::Company("1Y")->ida());

    BaseTables::Router_impl::setDbH2Haddr(ets->router());
    BaseTables::Router_impl::setDbTimeout(ets->router(),5);
    tst();
    EdifactRequest edireq(ets, TKCREQ);
    tst();

    edireq.setMsgCode("888");
    edireq.sendTlg();

    BaseTables::Router rot (ets->router());
    LogTrace(TRACE1) << "h2h src:" << rot->h2hSrcAddr() <<
            " h2h rcv:" << rot->h2hDestAddr();
    LogTrace(TRACE1) << "h2h:" << *edireq.tlgOut()->h2h();
    fail_unless(edireq.tlgOut()->h2h() != 0, "tlg h2h is null");
    fail_unless(!strcmp(edireq.tlgOut()->h2h()->sender, rot->h2hSrcAddr().c_str()), "inv h2h src");
    fail_unless(!strcmp(edireq.tlgOut()->h2h()->receiver, rot->h2hDestAddr().c_str()), "inv h2h dest");
    fail_unless(!strcmp(edireq.tlgOut()->h2h()->tpr,
                 std::string(edireq.ediSess()->edih()->our_ref).substr(6,4).c_str()), "inv tpr");
    fail_unless(!strlen(edireq.tlgOut()->h2h()->why), "inv h2h err");
    fail_unless(edireq.tlgOut()->h2h()->qri5 ==
            edilib::EdiSess::Qri5Flg::toQri5(edireq.ediSess()->ediSession()->status()), "inv h2h qri5");
    fail_unless(edireq.tlgOut()->h2h()->qri5 == 'W', "inv h2h qri5");
    fail_unless(edireq.tlgOut()->h2h()->qri6 == 'A', "inv h2h qri6");

    fail_unless(edireq.tlgOut()->text2view().find("MSG+:888") != std::string::npos, "inv tlg");

    TlgHandling::TlgSourceEdifact tlgdb =
            TlgHandling::TlgSourceEdifact::readFromDb(edireq.tlgOut()->tlgNum());
    fail_unless(edireq.tlgOut()->text() == tlgdb.text(), "inv tlg in db");
    fail_unless(*edireq.tlgOut() == tlgdb, "inv tlg in db");


        //== timeout

    OciCpp::oracle_datetime tout;
    make_curs("select time_out from EDISESSION_TIMEOUTS where SESS_IDA = :id").
            bind(":id", edireq.ediSessId()).
            def(tout).
            EXfet();
    fail_unless(Dates::second_clock::local_time() + Dates::time_duration(0,0,5) ==
            Dates::from_oracle_datetime(tout), "inv timeout");

    make_curs("update EDISESSION_TIMEOUTS set time_out=SYSDATE where sess_ida=:id")
            .bind(":id",edireq.ediSessId()).
            exec();

    std::list<EdiSessionTimeOut> lExpired;
    EdiSessionTimeOut::readExpiredSessions(lExpired);
    fail_unless(lExpired.size() == 1, "inv touts size");
}
END_TEST;

#define SUITENAME "edifact_request"
TCASEREGISTER( init, tear_down)
{
    ADD_TEST( edifact_request_case1 );
}
TCASEFINISH;
#endif /*XP_TESTING*/
