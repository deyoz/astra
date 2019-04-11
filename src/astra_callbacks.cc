#include <string>
#include <vector>
#include "astra_callbacks.h"
#include "maindcs.h"
#include "adm.h"
#include "cache.h"
#include "brd.h"
#include "season.h"
#include "etick.h"
#include "iatci.h"
#include "images.h"
#include "tripinfo.h"
#include "cent.h"
#include "prepreg.h"
#include "salonform.h"
#include "salonform2.h"
#include "sopp.h"
#include "stat/stat_main.h"
#include "print.h"
#include "checkin.h"
#include "events.h"
#include "docs/docs_main.h"
#include "access.h"
#include "telegram.h"
#include "astra_service.h"
#include "payment.h"
#include "crypt.h"
#include "dev_tuning.h"
#include "astra_utils.h"
#include "exceptions.h"
#include "astra_locale.h"
#include "oralib.h"
#include "xml_unit.h"
#include "base_tables.h"
#include "web_main.h"
#include "http_main.h"
#include "astra_locale.h"
#include "request_dup.h"
#include "tlg/tlg.h"
#include "jxtlib/jxtlib.h"
#include "serverlib/msg_const.h"
#include "serverlib/query_runner.h"
#include "serverlib/ocilocal.h"
#include "serverlib/perfom.h"
#include "external_spp_synch.h"
#include "rfisc_sirena.h"
#include "html_pages.h"
#include "kiosk/kiosk_config.h"
#include "kiosk/kiosk_events.h"
#include "zamar_dsm.h"
#include "hotel_acmd.h"
#include "libra.h"
#include "self_ckin_log.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace jxtlib;

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
    new ETRequestControlInterface();
    new ETRequestACInterface();
    new ETStatusInterface();
    new EMDSearchInterface();
    new EMDDisplayInterface();
    new EMDSystemUpdateInterface();
    new EMDStatusInterface();
    new EMDAutoBoundInterface();
    new IatciInterface();
    new ChangeStatusInterface();
    new ImagesInterface();
    new EventsInterface();
    new SalonsInterface();
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
    new HtmlInterface();
    new KIOSKEVENTS::KioskRequestInterface();
    new KIOSKCONFIG::KioskRequestInterface();
    new HotelAcmdInterface();
    new SelfCkinLogInterface();

    new AstraWeb::WebRequestsIface();
    new LibraInterface();

    new HTTPRequestsIface();
    new PieceConceptInterface();
    new ServicePaymentInterface();
    new SirenaExchangeInterface();

    new ZamarDSMInterface();
    new ZamarSBDOInterface();

    CheckInInterface::instance();
    TripsInterface::instance();
    SalonFormInterface::instance();
}

void AstraJxtCallbacks::UserBefore(const std::string &head, const std::string &body)
{
    try
    {
      if (SEND_REQUEST_DUP() &&
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
    //०��
    xmlNodePtr propNode;
    if ((propNode = GetNode("@mode", node))!=NULL)
      reqInfoData.mode = NodeAsString(propNode);
    //��. �ନ����
    if ((propNode = GetNode("@term_id", node))!=NULL)
      reqInfoData.term_id = NodeAsFloat(propNode);
    //�� �ନ����
    if ((propNode = GetNode("@lang", node))!=NULL) {
        reqInfoData.lang = NodeAsString(propNode);
        ProgTrace( TRACE5, "reqInfoData.lang=%s", reqInfoData.lang.c_str() );
        if ( GetNode( "UserLogon", node ) != NULL && reqInfoData.lang != AstraLocale::LANG_DEFAULT ) {
            TLangTypes langs = (TLangTypes&)base_tables.get("lang_types");
            try {
            langs.get_row("code",reqInfoData.lang);
          if (AstraLocale::TLocaleMessages::Instance()->checksum( reqInfoData.lang ) == 0) // ��� ������ ��� ᫮����
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

    reqInfoData.pr_web = (head[0]==2); //�ਧ��� ⮣� �� ����� � SWC � �१ ���� ࠡ���� ���� ��� � ���� ॣ����樨
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
    { // ������ ��襫 ���⮩ - ��ࠢ�塞 ���� �� �������
        showBasicInfo();
    }
    PerfomInit();
    ServerFramework::getQueryRunner().setPult(xmlRC->pult);
}

void CheckTermResDoc()
{
  ProgTrace(TRACE5, "%s started", __FUNCTION__);

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

void AstraJxtCallbacks::UserAfter()
{
    base_tables.Invalidate();
    PerfomTest( 2007 );
    if (fp_post_process != NULL)
      (*fp_post_process)();

    //���⪮ �ॡ㥬 encoding=UTF-8
    //࠭��, �� ���������� � ��ॢ� ��� �� ����� property �� � UTF-8, encoding ᡨ������
    //�� � ᢮� ��।� �ਢ���� � �訡�� xmlDocDumpFormatMemory
    //����� libxml � ���ᨨ 2.7 �࠭�� � �ॡ�� ࠡ���� � ��ॢ�� � UTF-8, � �� � 866
    SetXMLDocEncoding(getXmlCtxt()->resDoc, "UTF-8");
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
              if (TReqInfo::Instance()->isSelfCkinClientType())
                AstraLocale::showProgError("WRAP.QRY_HANDLER_ERR",
                                           AstraLocale::LParams() << AstraLocale::LParam("text", orae->what()));
              else
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

      if (TReqInfo::Instance()->isSelfCkinClientType())
        AstraLocale::showProgError("WRAP.QRY_HANDLER_ERR",
                                   AstraLocale::LParams() << AstraLocale::LParam("text", e->what()));
      else
        AstraLocale::showProgError("MSG.QRY_HANDLER_ERR.CALL_ADMIN");
      try
      {
        traceXML(ctxt->reqDoc);
      }
      catch(...) {};
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
