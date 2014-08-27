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
#include "astra_service.h"
#include "payment.h"
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
#include "http_main.h"
#include "astra_locale.h"
#include "empty_proc.h"
#include "tlg/tlg.h"
#include "jxtlib/jxtlib.h"
#include "serverlib/msg_const.h"
#include "serverlib/query_runner.h"
#include "serverlib/ocilocal.h"
#include "serverlib/perfom.h"
#include "external_spp_synch.h"

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
    new EMDSearchInterface();
    new EMDDisassociateInterface();
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
    new AstraServiceInterface();
    new PaymentInterface();
    new DevTuningInterface();
    new AccessInterface();
    new CryptInterface();
    new TestInterface();

    new AstraWeb::WebRequestsIface();

    new HTTPRequestsIface();

};


bool ENABLE_REQUEST_DUP()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("ENABLE_REQUEST_DUP",0,1,0);
  return VAR!=0;
}

bool BuildMsgForTermRequestDup(const std::string &pult,
                               const std::string &opr,
                               const std::string &body,
                               std::string &msg)
{
  msg.clear();
  char head[100];
  memset( &head, 0, sizeof(head) );
  char *p=head+4*3+4*8;
  strncpy(p,pult.c_str(),6);
  strcpy(p+6,"  ");
  strcpy(p+6+2,opr.c_str());
  head[84]=char(0x10);
  head[99]='D'; //признах что REQUEST_DUP
  *(long int*)head=htonl(body.size());
  msg+=char(3);
  msg+=std::string(head,sizeof(head));
  msg+=body;
  return true;
};

bool BuildMsgForWebRequestDup(short int client_id,
                              const std::string &body,
                              std::string &msg)
{
  msg.clear();
  char head[100];
  memset( &head, 0, sizeof(head) );
  char *p=head+4*3+4*8;
  *(short int*)p=htons(client_id);
  *(long int*)head=htonl(body.size());
  head[99]='D'; //признах что REQUEST_DUP
  msg+=char(2);
  msg+=std::string(head,sizeof(head));
  msg+=body;
  return true;
};

void AstraJxtCallbacks::UserBefore(const std::string &head, const std::string &body)
{
    try
    {
      if (ENABLE_REQUEST_DUP() &&
          !head.empty() && *(head.begin())==char(3))
      {
        if ( body.find("<kick") == std::string::npos )
        {
          std::string msg;
          if (BuildMsgForTermRequestDup(getXmlCtxt()->GetPult(), getXmlCtxt()->GetOpr(), body, msg))
          {
            /*std::string msg_hex;
            StringToHex(msg, msg_hex);
            ProgTrace(TRACE5, "UserBefore: msg_hex=%s", msg_hex.c_str());*/
            sendCmd("REQUEST_DUP", msg.c_str(), msg.size());
          };
        };
      };
    }
    catch(...) {};

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
    //режим
    xmlNodePtr propNode;
    if ((propNode = GetNode("@mode", node))!=NULL)
      reqInfoData.mode = NodeAsString(propNode);
    //ид. терминала
    if ((propNode = GetNode("@term_id", node))!=NULL)
      reqInfoData.term_id = NodeAsFloat(propNode);
    //язык терминала
    if ((propNode = GetNode("@lang", node))!=NULL) {
    	reqInfoData.lang = NodeAsString(propNode);
    	ProgTrace( TRACE5, "reqInfoData.lang=%s", reqInfoData.lang.c_str() );
    	if ( GetNode( "UserLogon", node ) != NULL && reqInfoData.lang != AstraLocale::LANG_DEFAULT ) {
    		TLangTypes langs = (TLangTypes&)base_tables.get("lang_types");
    		try {
      		langs.get_row("code",reqInfoData.lang);
          if (AstraLocale::TLocaleMessages::Instance()->checksum( reqInfoData.lang ) == 0) // нет данных для словаря
          	throw EBaseTableError("");
      	}
        catch( EBaseTableError ) {
        	ProgTrace( TRACE5, "Unknown client lang=%s", reqInfoData.lang.c_str() );
        	reqInfoData.lang = AstraLocale::LANG_DEFAULT;
        }
      }
    }

   if (!reqInfoData.lang.empty())
   {
    if (reqInfoData.lang==AstraLocale::LANG_RU)
      xmlRC->setLang(RUSSIAN);
    else
      xmlRC->setLang(ENGLISH);
   }
   else xmlRC->setLang(RUSSIAN);

    reqInfoData.checkUserLogon =
        GetNode( "UserLogon", node ) == NULL &&
        GetNode( "ClientError", node ) == NULL &&
        GetNode( "SaveDeskTraces", node ) == NULL &&
        GetNode( "GetCertificates", node ) == NULL &&
        GetNode( "RequestCertificateData", node ) == NULL &&
        GetNode( "PutRequestCertificate", node ) == NULL &&
        GetNode( "CryptValidateServerKey", node ) == NULL;

    reqInfoData.checkCrypt =
        GetNode( "kick", node ) == NULL &&
        GetNode( "GetCertificates", node ) == NULL &&
        GetNode( "RequestCertificateData", node ) == NULL &&
        GetNode( "PutRequestCertificate", node ) == NULL &&
        !((head)[getGrp3ParamsByte()+1]&MSG_MESPRO_CRYPT);

    reqInfoData.pr_web = (head[0]==2); //признак того что обмен с SWC А через него работают пока веб и киоск регистрации
    reqInfoData.duplicate = (head[100]=='D');

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

void CheckTermResDoc()
{
    XMLRequestCtxt *xmlRC = getXmlCtxt();
    xmlNodePtr resNode=NodeAsNode("/term/answer",xmlRC->resDoc);
    SetProp(resNode, "execute_time", TReqInfo::Instance()->getExecuteMSec() );

    xmlNodePtr errNode=NULL;
	int errPriority=ASTRA::NoExists;
	for(xmlNodePtr node=resNode->children; node!=NULL; node=node->next)
	{
      if (strcmp((const char*)node->name,"command")==0)
	  {
	    for(xmlNodePtr cmdNode=node->children; cmdNode!=NULL; cmdNode=cmdNode->next)
	    {
	      int priority=ASTRA::NoExists;
	      if (strcmp((const char*)cmdNode->name,"error")==0) priority=1;
	      if (strcmp((const char*)cmdNode->name,"user_error")==0) priority=2;
	      if (strcmp((const char*)cmdNode->name,"user_error_message")==0) priority=3;
	      if (strcmp((const char*)cmdNode->name,"message")==0) priority=4;
	      if (priority!=ASTRA::NoExists &&
            (errPriority==ASTRA::NoExists || priority<errPriority))
        {
          errNode=cmdNode;
          errPriority = priority;
        };
      };
	  };
  };

  if (errNode!=NULL)
  {
    for(xmlNodePtr node=resNode->children; node!=NULL; node=node->next)
  	{
  	  if (strcmp((const char*)node->name,"command")==0)
  	  {
        for(xmlNodePtr cmdNode=node->children; cmdNode!=NULL; )
        {
          if (cmdNode!=errNode)
          {
            xmlNodePtr node2=cmdNode->next;
	          xmlUnlinkNode(cmdNode);
	          xmlFreeNode(cmdNode);
	          cmdNode=node2;
	        }
	        else
	          cmdNode=cmdNode->next;
        };
  	  };
  	};
  };
};

void RevertWebResDoc()
{
  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer",xmlRC->resDoc);
  const char* answer_tag = (const char*)xmlRC->reqDoc->children->children->children->name;
  SetProp(resNode, "execute_time", TReqInfo::Instance()->getExecuteMSec() );
  std::string error_code, error_message;
  xmlNodePtr errNode = ResDocSelectPrior(resNode, error_code, error_message);

  if (errNode!=NULL)
  {
    resNode=NewTextChild( resNode, answer_tag );

    if (strcmp((const char*)errNode->name,"error")==0 ||
        strcmp((const char*)errNode->name,"checkin_user_error")==0 ||
        strcmp((const char*)errNode->name,"user_error")==0)
    {
      NewTextChild( resNode, "error_code", error_code );
      NewTextChild( resNode, "error_message", error_message );
    };

    if (strcmp((const char*)errNode->name,"checkin_user_error")==0)
    {
      xmlNodePtr segsNode=NodeAsNode("segments",errNode);
      if (segsNode!=NULL)
      {
        xmlUnlinkNode(segsNode);
        xmlAddChild( resNode, segsNode);
      };
    };
    xmlFreeNode(errNode);
  };
}

xmlNodePtr ResDocSelectPrior(xmlNodePtr resNode, std::string& error_code, std::string& error_message)
{
    // если есть тег <error> || <checkin_user_error> || <user_error>, то все остальное удаляем их из xml дерева
    xmlNodePtr errNode=NULL;
    int errPriority=ASTRA::NoExists;
    for(xmlNodePtr node=resNode->children; node!=NULL; node=node->next)
    {
      if (strcmp((const char*)node->name,"command")==0)
      {
        for(xmlNodePtr cmdNode=node->children; cmdNode!=NULL; cmdNode=cmdNode->next)
        {
          int priority=ASTRA::NoExists;
          if (strcmp((const char*)cmdNode->name,"error")==0) priority=1;
          if (strcmp((const char*)cmdNode->name,"checkin_user_error")==0) priority=2;
          if (strcmp((const char*)cmdNode->name,"user_error")==0) priority=3;
          if (priority!=ASTRA::NoExists &&
            (errPriority==ASTRA::NoExists || priority<errPriority))
        {
          errNode=cmdNode;
          errPriority = priority;
        };
      };
      };
  };

  if (errNode!=NULL)
  {
    if (strcmp((const char*)errNode->name,"error")==0 ||
        strcmp((const char*)errNode->name,"user_error")==0)
    {
      error_message = NodeAsString(errNode);
      error_code = NodeAsString( "@lexema_id", errNode, "" );
    };
    if (strcmp((const char*)errNode->name,"checkin_user_error")==0)
    {
      xmlNodePtr segNode=NodeAsNode("segments",errNode)->children;
      for(;segNode!=NULL; segNode=segNode->next)
      {
        if (GetNode("error_code",segNode)!=NULL)
        {
          error_message = NodeAsString("error_message", segNode, "");
          error_code = NodeAsString("error_code", segNode);
          break;
        };
        xmlNodePtr paxNode=NodeAsNode("passengers",segNode)->children;
        for(;paxNode!=NULL; paxNode=paxNode->next)
        {
          if (GetNode("error_code",paxNode)!=NULL)
          {
            error_message = NodeAsString("error_message", paxNode, "");
            error_code = NodeAsString("error_code", paxNode);
            break;
          };
        };
        if (paxNode!=NULL) break;
      };
    };
    //отцепляем
    xmlUnlinkNode(errNode);
  };

  for(xmlNodePtr node=resNode->children; node!=NULL;)
  {
      //отцепляем и удаляем либо все, либо <command> внутри <answer>
    xmlNodePtr node2=node->next;
    if (errNode!=NULL || strcmp((const char*)node->name,"command")==0)
    {
      xmlUnlinkNode(node);
      xmlFreeNode(node);
    };
    node=node2;
  };
  return errNode;
}

void AstraJxtCallbacks::UserAfter()
{
    base_tables.Invalidate();
    PerfomTest( 2007 );
    if (fp_post_process != NULL)
      (*fp_post_process)();
    XMLRequestCtxt *xmlRC = getXmlCtxt();
    //жестко требуем encoding=UTF-8
    //ранее, при добавлении в дерево хотя бы одной property не в UTF-8, encoding сбивается
    //это в свою очередь приводит к ошибке xmlDocDumpFormatMemory
    //вообще libxml с версии 2.7 хранит и требует работать с деревом в UTF-8, а не в 866
    SetXMLDocEncoding(xmlRC->resDoc, "UTF-8");

    if ( TReqInfo::Instance()->client_type == ctTerm )
      CheckTermResDoc(); //!!!anna
}


void AstraJxtCallbacks::HandleException(ServerFramework::Exception *e)
{
    ProgTrace(TRACE3, "AstraJxtCallbacks::HandleException: %s", e->what());

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
          if (strcmp((const char*)node->name,"basic_info")!=0&&strcmp((const char*)node->name,"command")!=0)
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
              AstraLocale::showProgError("MSG.QRY_HANDLER_ERR.CALL_ADMIN");
          }
          throw 1;
      };


      AstraLocale::UserException *lue = dynamic_cast<AstraLocale::UserException*>(e);
      if (lue)
      {
          AstraLocale::showError( lue->getLexemaData(), lue->Code() );

          CheckIn::UserException *cue = dynamic_cast<CheckIn::UserException*>(e);
          if (cue)
          {
            CheckIn::showError( cue->segs );
          };

          throw 1;
      }

      std::logic_error *exp = dynamic_cast<std::logic_error*>(e);
      if (exp)
          ProgError(STDLOG,"std::logic_error: %s",exp->what());
      else
          ProgError(STDLOG,"ServerFramework::Exception: %s",e->what());

      AstraLocale::showProgError("MSG.QRY_HANDLER_ERR.CALL_ADMIN");
      throw 1;
    }
    catch( int ) {
    	UserAfter();
    }
    catch(ServerFramework::Exception &localException)
    {
      ProgError(STDLOG,"AstraJxtCallbacks::HandleException: %s", localException.what());
      throw;
    };
}
