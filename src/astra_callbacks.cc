#include <string>
#include <vector>
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
#include "access.h"
#include "telegram.h"
#include "design_blank.h"
#include "astra_service.h"
#include "payment.h"
#include "payment2.h"
#include "crypt.h"
#include "dev_tuning.h"
#include "astra_utils.h"
#include "basic.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "oralib.h"
#include "xml_unit.h"
#include "base_tables.h"
#include "web_main.h"
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
    new AccessInterface();
    new CryptInterface();

    new AstraWeb::WebRequestsIface();
};

void AstraJxtCallbacks::UserBefore(/*const char *body, int blen, const char *head,
        int hlen, char **res, int len*/)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
	  reqInfo->setPerform();
	  base_tables.Invalidate();
    OraSession.ClearQuerys();
    XMLRequestCtxt *xmlRC = getXmlCtxt();
    xmlNodePtr node=NodeAsNode("/term/query",xmlRC->reqDoc);
    TReqInfoInitData reqInfoData;
    reqInfoData.screen = NodeAsString("@screen", node);
    reqInfoData.pult = xmlRC->pult;
    reqInfoData.opr = NodeAsString("@opr", node);
    xmlNodePtr modeNode = GetNode("@mode", node);
    std::string mode;
    if (modeNode!=NULL)
      reqInfoData.mode = NodeAsString(modeNode);
    if ( GetNode( "@lang", node ) )
    	reqInfoData.lang = NodeAsString("@lang",node);

    reqInfoData.checkUserLogon =
        GetNode( "CheckUserLogon", node ) == NULL &&
        GetNode( "UserLogon", node ) == NULL &&
        GetNode( "ClientError", node ) == NULL &&
        GetNode( "SaveDeskTraces", node ) == NULL &&
        GetNode( "GetCertificates", node ) == NULL &&
        GetNode( "RequestCertificateData", node ) == NULL &&
        GetNode( "PutRequestCertificate", node ) == NULL;

  /*  reqInfoData.checkCrypt =
        GetNode( "kick", node ) == NULL &&
        GetNode( "GetCertificates", node ) == NULL &&
        GetNode( "RequestCertificateData", node ) == NULL &&
        GetNode( "PutRequestCertificate", node ) == NULL &&
        !((head)[getGrp3ParamsByte()+1]&MSG_MESPRO_CRYPT);

    reqInfoData.pr_web = (head[0]==2);*/

    try
    {
      reqInfo->Initialize( reqInfoData );
    }
    catch(AstraLocale::UserException)
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
    if ( reqInfo->screen.pr_logon && reqInfoData.opr.empty() &&
    	   ( GetNode( "UserLogon", node ) == NULL &&
           GetNode( "GetCertificates", node ) == NULL &&
           GetNode( "RequestCertificateData", node ) == NULL &&
           GetNode( "PutRequestCertificate", node ) == NULL) )
    { // оператор пришел пустой - отправляем инфу по оператору
        showBasicInfo();
    }
    PerfomInit();
    ServerFramework::getQueryRunner().setPult(xmlRC->pult);
}

void RevertWebResDoc( const char* answer_tag, xmlNodePtr resNode )
{
	// если есть тег <error> || <user_error>, то все остальное удаляем их из xml дерева
	std::vector<xmlNodePtr> vnodes;
	xmlNodePtr ne;
	ne = GetNode( "command/error", resNode );
	if ( ne == NULL )
	  ne = GetNode( "command/user_error", resNode );
	if ( ne != NULL ) {
		std::string error_message = NodeAsString( ne );
		std::string error_code = NodeAsString( "@code", ne, "0" );
		std::string lexema_id = NodeAsString( "@lexema_id", ne, "" );
		xmlNodePtr n = resNode->children;
    while ( n != NULL ) {
   		vnodes.push_back( n );
     	n = n->next;
    }
    resNode = NewTextChild( resNode, answer_tag ); // command tag
    //NewTextChild( resNode, "error_code", error_code );
    NewTextChild( resNode, "error_code", lexema_id );
    NewTextChild( resNode, "error_message", error_message );
	}
	else { // если есть тег <message> || <user_error_message>, то удаляем их из xml дерева
		if ( GetNode( "command/user_error_message", resNode ) != NULL ||
			   GetNode( "command/message", resNode ) != NULL ) {
     	vnodes.push_back( GetNode( "command", resNode ) );
		}
	}
  for ( std::vector<xmlNodePtr>::iterator i=vnodes.begin(); i!=vnodes.end(); i++ ) {
   	xmlUnlinkNode( *i );
   	xmlFreeNode( *i );
  }
}


void AstraJxtCallbacks::UserAfter()
{
    base_tables.Invalidate();
    PerfomTest( 2007 );
	  XMLRequestCtxt *xmlRC = getXmlCtxt();
	  xmlNodePtr node=NodeAsNode("/term/answer",xmlRC->resDoc);
	  SetProp(node, "execute_time", TReqInfo::Instance()->getExecuteMSec() );
	  if ( TReqInfo::Instance()->client_type == ctWeb ||
	       TReqInfo::Instance()->client_type == ctKiosk )
	  	RevertWebResDoc( (const char*)xmlRC->reqDoc->children->children->children->name, node );
}


void AstraJxtCallbacks::HandleException(ServerFramework::Exception *e)
{
    ProgTrace(TRACE3, "AstraJxtCallbacks::HandleException");

    XMLRequestCtxt *ctxt = getXmlCtxt();
    xmlNodePtr resNode = ctxt->resDoc->children->children;
    xmlNodePtr node = resNode->children;

    try {

      UserException2 *ue2 = dynamic_cast<UserException2*>(e);
      if (ue2) {
      	throw 1;
      }

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
          		AstraLocale::showError("MSG.SYSTEM_VERS_UPDATED.REPEAT");
          		break;
          	default:
          	  ProgError(STDLOG,"EOracleError %d: %s",orae->Code,orae->what());
          	  ProgError(STDLOG,"SQL: %s",orae->SQLText());
              showProgError("MSG.QRY_HANDLER_ERR.CALL_ADMIN");
          }
          throw 1;
      };
      AstraLocale::UserException *lue = dynamic_cast<AstraLocale::UserException*>(e);
      if (lue)
      {
          AstraLocale::showError( lue->getLexemaData(), lue->Code() );
          throw 1;
      }
      /*EXCEPTIONS::UserException *ue = dynamic_cast<EXCEPTIONS::UserException*>(e);
      if (ue)
      {
          ProgTrace( TRACE5, "UserException: %s", ue->what() );
          showError(ue->what(), ue->Code());
          throw 1;
      }*/
      std::logic_error *exp = dynamic_cast<std::logic_error*>(e);
      if (exp)
          ProgError(STDLOG,"std::logic_error: %s",exp->what());
      else
          ProgError(STDLOG,"std::exception: %s",e->what());

      AstraLocale::showProgError("MSG.QRY_HANDLER_ERR.CALL_ADMIN");
      throw 1;
    }
    catch( int ) {
    	UserAfter();
    }
}
