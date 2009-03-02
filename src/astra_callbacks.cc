#include "astra_callbacks.h"
#include "maindcs.h"
#include "adm.h"
#include "cache.h"
#include "brd.h"
#include "season.h"
#include "etick.h"
#include "images.h"
#include "tripinfo.h"
#include "cent.h"
#include "prepreg.h"
#include "salonform.h"
#include "salonform2.h"
#include "sopp.h"
#include "stat.h"
#include "print.h"
#include "checkin.h"
#include "events.h"
#include "docs.h"
#include "telegram.h"
#include "design_blank.h"
#include "astra_service.h"
#include "payment.h"
#include "payment2.h"
#include "dev_tuning.h"
#include "astra_utils.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "xml_unit.h"
#include "base_tables.h"
#include "jxtlib/jxtlib.h"
#include "serverlib/query_runner.h"
#include "serverlib/ocilocal.h"
#include "serverlib/perfom.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace jxtlib;
using namespace BASIC;

void AstraJxtCallbacks::InitInterfaces()
{
    ProgTrace(TRACE3, "AstraJxtCallbacks::InitInterfaces");
    new SysReqInterface();
    new MainDCSInterface();
    new AdmInterface();
    new CacheInterface();
    new BrdInterface();
    new SeasonInterface();
    new ETSearchInterface();
    new ETStatusInterface();
    new ImagesInterface();
    new CheckInInterface();
    new EventsInterface();
    new TripsInterface();
    new SalonsInterface();
    new SalonFormInterface();
    new CentInterface();
    new PrepRegInterface();
    new SoppInterface();
    new StatInterface();
    new PrintInterface();
    new DocsInterface();
    new TelegramInterface();
    new DesignBlankInterface();
    new AstraServiceInterface();
    new PaymentInterface();
    new PaymentOldInterface();
    new DevTuningInterface();
};

void AstraJxtCallbacks::UserBefore(const char *body, int blen, const char *head,
        int hlen, char **res, int len)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
	  reqInfo->setPerform();
	  base_tables.Invalidate();
    OraSession.ClearQuerys();
    XMLRequestCtxt *xmlRC = getXmlCtxt();
    xmlNodePtr node=NodeAsNode("/term/query",xmlRC->reqDoc);
    std::string screen = NodeAsString("@screen", node);
    std::string opr = NodeAsString("@opr", node);
    xmlNodePtr modeNode = GetNode("@mode", node);
    std::string mode;
    if (modeNode!=NULL)
      mode = NodeAsString(modeNode);

    bool checkUserLogon =
        GetNode( "CheckUserLogon", node ) == NULL &&
        GetNode( "UserLogon", node ) == NULL &&
        GetNode( "ClientError", node ) == NULL &&
        GetNode( "SaveDeskTraces", node ) == NULL;

    try
    {
      reqInfo->Initialize( screen, xmlRC->pult, opr, mode, checkUserLogon );
    }
    catch(EXCEPTIONS::UserException)
    {
      if (GetNode( "UserLogoff", node ) != NULL)
      {
        reqInfo->user.clear();
        reqInfo->desk.clear();
        showBasicInfo();
        throw UserException2();
      }
      else
        throw;
    };
    if ( reqInfo->screen.pr_logon && opr.empty() && (GetNode( "UserLogon", node ) == NULL))
    { // ������ ��襫 ���⮩ - ��ࠢ�塞 ���� �� �������
        showBasicInfo();
    }
    PerfomInit();
    ServerFramework::getQueryRunner().setPult(xmlRC->pult);
}

void AstraJxtCallbacks::UserAfter()
{
    base_tables.Invalidate();
    PerfomTest( 2007 );
	  XMLRequestCtxt *xmlRC = getXmlCtxt();
	  xmlNodePtr node=NodeAsNode("/term/answer",xmlRC->resDoc);
	  SetProp(node, "execute_time", TReqInfo::Instance()->getExecuteMSec() );
}


void AstraJxtCallbacks::HandleException(std::exception *e)
{
    ProgTrace(TRACE3, "AstraJxtCallbacks::HandleException");

    XMLRequestCtxt *ctxt = getXmlCtxt();
    xmlNodePtr resNode = ctxt->resDoc->children->children;
    xmlNodePtr node = resNode->children;

    UserException2 *ue2 = dynamic_cast<UserException2*>(e);
    if (ue2) return;

    xmlNodePtr node2;
    while(node!=NULL)
    {
        if (strcmp((char*)node->name,"basic_info")!=0&&strcmp((char*)node->name,"command")!=0)
        {
            node2=node;
            node=node->next;
            xmlUnlinkNode(node2);
            xmlFreeNode(node2);
        }
        else node=node->next;
    };

    EOracleError *orae = dynamic_cast<EOracleError*>(e);
    if (orae)
    {
        switch( orae->Code ) {
        	case 4061:
        	case 4068:
        		showError("����� ��⥬� �뫠 ���������. ������ ����⢨�");
        		break;
        	default:
        	  ProgError(STDLOG,"EOracleError %d: %s",orae->Code,orae->what());
        	  ProgError(STDLOG,"SQL: %s",orae->SQLText());
            showProgError("�訡�� ��ࠡ�⪨ �����. ������� � ࠧࠡ��稪��");
        }
        return;
    };
    EXCEPTIONS::UserException *ue = dynamic_cast<EXCEPTIONS::UserException*>(e);
    if (ue)
    {
        ProgTrace( TRACE5, "UserException: %s", ue->what() );
        showError(ue->what(), ue->Code());
        return;
    }
    std::logic_error *exp = dynamic_cast<std::logic_error*>(e);
    if (exp)
        ProgError(STDLOG,"std::logic_error: %s",exp->what());
    else
        ProgError(STDLOG,"std::exception: %s",e->what());

    showProgError("�訡�� ��ࠡ�⪨ �����. ������� � ࠧࠡ��稪��");
    return;
}
