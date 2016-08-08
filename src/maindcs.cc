#include "maindcs.h"
#include "date_time.h"
#include "astra_elems.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "oralib.h"
#include "exceptions.h"
#include "misc.h"
#include "xml_unit.h"
#include "print.h"
#include "crypt.h"
#include "astra_locale.h"
#include "stl_utils.h"
#include "term_version.h"
#include "dev_utils.h"
#include "qrys.h"

#include <jxtlib/jxt_cont.h>
#include <serverlib/testmode.h>

#include <fstream>
#include <string>
#include <map>
#include <fstream>


#define NICKNAME "VLAD"
#include <serverlib/slogger.h>

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace std;

enum TDevParamCategory{dpcSession,dpcFormat,dpcModel};

const int NEW_TERM_VER_NOTICE=1;

int SetTermVersionNotice(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  string version, term_mode;
  ostringstream prior_version;
  try
  {
    //�஢��塞 ��ࠬ����
    if (argc<3) throw EConvertError("wrong parameters");
    ProgTrace(TRACE5, "SetTermVersionNotice: <platform>=%s <version>=%s", argv[1], argv[2]);
    if (!TDesk::isValidVersion(argv[2])) throw EConvertError("wrong terminal version");
    version=argv[2];
    int i = ToInt(version.substr(7));
    if (i<=0) throw EConvertError("wrong terminal version");
    i--;
    prior_version << version.substr(0,7) << setw(7) << setfill('0') << i;
    ProgTrace(TRACE5, "SetTermVersionNotice: prior_version=%s", prior_version.str().c_str());
    Qry.Clear();
    Qry.SQLText="SELECT code FROM term_modes WHERE code=UPPER(:term_mode)";
    Qry.CreateVariable("term_mode", otString, argv[1]);
    Qry.Execute();
    if (Qry.Eof) throw EConvertError("wrong platform");
    term_mode=Qry.FieldAsString("code");
    ProgTrace(TRACE5, "SetTermVersionNotice: term_mode=%s", term_mode.c_str());
  }
  catch(EConvertError &E)
  {
    printf("Error: %s\n", E.what());
    if (argc>0)
    {
      puts("Usage:");
      SetTermVersionNoticeHelp(argv[0]);
      puts("Example:");
      printf("  %s CUTE 201101-0123456\n",argv[0]);
    };
    return 1;
  };

  //�஢�ਬ notice_id �� ᮢ�������
  Qry.Clear();
  Qry.SQLText=
    "SELECT notice_id FROM desk_notices "
    "WHERE notice_type=:notice_type AND term_mode=:term_mode AND "
    "      last_version=:last_version ";
  Qry.CreateVariable("notice_type", otString, NEW_TERM_VER_NOTICE);
  Qry.CreateVariable("term_mode", otString, term_mode);
  Qry.CreateVariable("last_version", otString, prior_version.str());
  Qry.Execute();
  if (!Qry.Eof)
  {
    printf("Error: version %s has already been introduced\n", version.c_str());
    return 0;
  };

  Qry.Clear();
  Qry.SQLText=
    "DECLARE "
    "  CURSOR cur IS "
    "    SELECT notice_id FROM desk_notices "
    "    WHERE notice_type=:notice_type AND term_mode=:term_mode FOR UPDATE; "
    "BEGIN "
    "  FOR curRow IN cur LOOP "
    "    DELETE FROM locale_notices WHERE notice_id=curRow.notice_id; "
    "    DELETE FROM desk_disable_notices WHERE notice_id=curRow.notice_id; "
    "    DELETE FROM desk_notices WHERE notice_id=curRow.notice_id; "
    "  END LOOP; "
    "  SELECT cycle_id__seq.nextval INTO :notice_id FROM dual; "
    "  INSERT INTO desk_notices(notice_id, notice_type, term_mode, last_version, default_disable, time_create, pr_del) "
    "  VALUES(:notice_id, :notice_type, :term_mode, :last_version, 0, system.UTCSYSDATE, 0); "
    "END; ";
  Qry.DeclareVariable("notice_id", otInteger);
  Qry.CreateVariable("notice_type", otString, NEW_TERM_VER_NOTICE);
  Qry.CreateVariable("term_mode", otString, term_mode);
  Qry.CreateVariable("last_version", otString, prior_version.str());
  Qry.Execute();
  int notice_id=Qry.GetVariableAsInteger("notice_id");

  map<string,string> text;
  Qry.Clear();
  Qry.SQLText="SELECT code AS lang FROM lang_types";
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    const char* lang=Qry.FieldAsString("lang");
    text[lang] = getLocaleText("MSG.NOTICE.NEW_TERM_VERSION",
                               LParams() << LParam("version", version) << LParam("term_mode", term_mode), lang);
  };

  Qry.Clear();
  Qry.SQLText="INSERT INTO locale_notices(notice_id, lang, text) VALUES(:notice_id, :lang, :text)";
  Qry.CreateVariable("notice_id", otInteger, notice_id);
  Qry.DeclareVariable("lang", otString);
  Qry.DeclareVariable("text", otString);
  for(map<string,string>::iterator i=text.begin();i!=text.end();i++)
  {
    Qry.SetVariable("lang", i->first);
    Qry.SetVariable("text", i->second);
    Qry.Execute();
  };

  puts("New version of the terminal has successfully introduced");
  return 0;
};

void SetTermVersionNoticeHelp(const char *name)
{
  printf("  %-15.15s ", name);
  puts("<platform (STAND, CUTE ...)> <version (XXXXXX-XXXXXXX)>  ");
};

void GetNotices(xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (!reqInfo->desk.compatible(DESK_NOTICE_VERSION)) return;

/*  ProgTrace(TRACE5,"GetNotices: desk=%s lang=%s desk_grp_id=%d term_mode=%s version=%s",
            reqInfo->desk.code.c_str(),
            reqInfo->desk.lang.c_str(),
            reqInfo->desk.grp_id,
            EncodeOperMode(reqInfo->desk.mode).c_str(),
            reqInfo->desk.version.c_str());*/

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT desk_notices.notice_id, locale_notices.text, default_disable "
    "FROM desk_notices, locale_notices, "
    "     (SELECT notice_id FROM desk_disable_notices WHERE desk=:desk) desk_disable_notices "
    "WHERE desk_notices.notice_id=locale_notices.notice_id AND locale_notices.lang=:lang AND "
    "      desk_notices.notice_id=desk_disable_notices.notice_id(+) AND "
    "      desk_disable_notices.notice_id IS NULL AND "
    "      (desk IS NULL OR desk=:desk) AND "
    "      (desk_grp_id IS NULL OR desk_grp_id=:desk_grp_id) AND "
    "      (term_mode IS NULL OR term_mode=:term_mode) AND "
    "      (first_version IS NULL OR first_version<=:version) AND "
    "      (last_version IS NULL OR last_version>=:version) AND "
    "      pr_del=0 "
    "ORDER BY time_create";
  Qry.CreateVariable("desk",otString,reqInfo->desk.code);
  Qry.CreateVariable("lang",otString,reqInfo->desk.lang);
  Qry.CreateVariable("desk_grp_id",otInteger,reqInfo->desk.grp_id);
  Qry.CreateVariable("term_mode",otString,EncodeOperMode(reqInfo->desk.mode));
  Qry.CreateVariable("version",otString,reqInfo->desk.version);
  Qry.Execute();
  if (!Qry.Eof)
  {
    xmlNodePtr noticesNode = NewTextChild(resNode, "notices");
    for(;!Qry.Eof;Qry.Next())
    {
      xmlNodePtr node = NewTextChild(noticesNode, "notice");
      NewTextChild(node, "id", Qry.FieldAsInteger("notice_id"));
      NewTextChild(node, "text", Qry.FieldAsString("text"));
      NewTextChild(node, "default_disable", (int)(Qry.FieldAsInteger("default_disable")!=0), (int)false);
      //NewTextChild(node, "dlg_type", EncodeMsgDlgType(mtInformation));
    };
  };
};

void CheckTermExpireDate(void)
{
  try
  {
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (reqInfo->client_type!=ctTerm) return;
    if (!reqInfo->desk.compatible(OLDEST_SUPPORTED_VERSION))
      throw AstraLocale::UserException("MSG.TERM_VERSION.NOT_SUPPORTED");
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT MIN(expire_date) AS expire_date "
      "FROM term_expire_dates "
      "WHERE (term_mode IS NULL OR term_mode=:term_mode) AND "
      "      (first_version IS NULL OR first_version<=:version) AND "
      "      (last_version IS NULL OR last_version>=:version) ";
    Qry.CreateVariable("term_mode",otString,EncodeOperMode(reqInfo->desk.mode));
    Qry.CreateVariable("version",otString,reqInfo->desk.version);
    Qry.Execute();
    if (Qry.Eof || Qry.FieldIsNULL("expire_date")) return;

    TDateTime expire_date=Qry.FieldAsDateTime("expire_date");
    if (expire_date<= NowUTC())
      throw AstraLocale::UserException("MSG.TERM_VERSION.NOT_SUPPORTED");
    double remainDays=expire_date-NowUTC();
    modf(floor(remainDays),&remainDays);
    int remainDaysInt=(int)remainDays;
    LexemaData lexeme;
    if (remainDaysInt>0)
    {
      if (reqInfo->desk.lang==AstraLocale::LANG_RU)
      {
        if ((remainDaysInt%10)==1 && remainDaysInt!=11)
          lexeme.lexema_id="MSG.TERM_VERSION.NEED_TO_UPDATE.WITHIN_DAY";
        else
          lexeme.lexema_id="MSG.TERM_VERSION.NEED_TO_UPDATE.WITHIN_DAYS";
      }
      else
      {
        if (remainDaysInt==1)
          lexeme.lexema_id="MSG.TERM_VERSION.NEED_TO_UPDATE.WITHIN_DAY";
        else
          lexeme.lexema_id="MSG.TERM_VERSION.NEED_TO_UPDATE.WITHIN_DAYS";
      };
      lexeme.lparams << LParam("days", remainDaysInt);
    }
    else
    {
      lexeme.lexema_id="MSG.TERM_VERSION.NEED_TO_UPDATE.URGENT";
    };
    AstraLocale::showErrorMessage("WRAP.CONTACT_SUPPORT", LParams()<<LParam("text", lexeme));
  }
  catch(UserException &E)
  {
    throw UserException("WRAP.CONTACT_SUPPORT", LParams()<<LParam("text", E.getLexemaData()));
  };

  return;
};

void MainDCSInterface::DisableNotices(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node=NodeAsNode("notices",reqNode);
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="INSERT INTO desk_disable_notices(desk, notice_id) VALUES(:desk, :notice_id)";
  Qry.CreateVariable("desk", otString, TReqInfo::Instance()->desk.code);
  Qry.DeclareVariable("notice_id", otInteger);
  for(node=node->children;node!=NULL;node=node->next)
  {
    Qry.SetVariable("notice_id", NodeAsInteger(node));
    try
    {
      Qry.Execute();
    }
    catch(EOracleError E)
    {
      //if (E.Code!=1) throw; ���� �� ⠪, �� � ���� �� 㤠���� id �� desk_notices?
    };
  };
};

void GetModuleList(xmlNodePtr resNode)
{
  TReqInfo *reqinfo = TReqInfo::Instance();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT DISTINCT screen.id,screen.name,screen.exe,screen.view_order "
    "FROM user_roles,role_rights,screen_rights,screen "
    "WHERE user_roles.role_id=role_rights.role_id AND "
    "      role_rights.right_id=screen_rights.right_id AND "
    "      screen_rights.screen_id=screen.id AND "
    "      user_roles.user_id=:user_id AND view_order IS NOT NULL "
    "ORDER BY view_order";
  Qry.DeclareVariable("user_id", otInteger);
  Qry.SetVariable("user_id", reqinfo->user.user_id);
  Qry.Execute();
  xmlNodePtr modulesNode = NewTextChild(resNode, "modules");
  if (!Qry.Eof)
  {
    for(;!Qry.Eof;Qry.Next())
    {
      string exe=Qry.FieldAsString("exe");
      if (exe=="ACCESS.EXE" && !reqinfo->desk.compatible(ACCESS_MODULE_VERSION)) continue;
      xmlNodePtr moduleNode = NewTextChild(modulesNode, "module");
      NewTextChild(moduleNode, "id", Qry.FieldAsInteger("id"));
      NewTextChild(moduleNode, "name", Qry.FieldAsString("name"));
      NewTextChild(moduleNode, "exe", exe);
    };
  }
  else {
      if(!inTestMode()) {
          // ����뢠�� ����� �� ��⥬� �ࠢ � ����
          AstraLocale::showErrorMessage("MSG.ALL_MODULES_DENIED_FOR_USER");
      }
  }
}

struct TDevParam {
  string param_name;
  string subparam_name;
  string param_value;
  int editable;
  TDevParam() {
    editable = 0;
  }
  TDevParam( string aparam_name,
             string asubparam_name, string aparam_value, int aeditable ) {
    param_name = lowerc(aparam_name);
    subparam_name = lowerc(asubparam_name);
    param_value = aparam_value;
    editable = aeditable;
  }
};

typedef vector<TDevParam> TCategoryDevParams;

void PutParams( TDevParam local_param, TCategoryDevParams &params )
{
    for ( TCategoryDevParams::iterator server_param=params.begin(); server_param!=params.end(); server_param++ ) {
        if ( server_param->editable &&
               server_param->param_name == local_param.param_name &&
               server_param->subparam_name == local_param.subparam_name ) {
            ProgTrace( TRACE5, "server->param_name=%s", server_param->param_name.c_str() );
            server_param->param_value = local_param.param_value;
            break;
        }
    }
}

void ParseParams( xmlNodePtr paramsNode, TCategoryDevParams &params )
{
    //int editable;
  if ( paramsNode == NULL ) return;

  for ( xmlNodePtr pNode=paramsNode->children; pNode!=NULL && pNode->type == XML_ELEMENT_NODE; pNode=pNode->next ) { // �஡�� �� ��ࠬ��ࠬ
    ProgTrace( TRACE5, "param name=%s", (const char*)pNode->name );
    if ( pNode->children == NULL || pNode->children->type != XML_ELEMENT_NODE ) { //��� subparams
        //editable = NodeAsInteger( "@editable", pNode, 0 );
      //if ( editable )
      ProgTrace( TRACE5, "param name=%s,subparam_name=%s,param_value=%s,editable=%d",
                 (const char*)pNode->name,"", NodeAsString( pNode ), /*editable*/true );
        PutParams( TDevParam((const char*)pNode->name,
                                "",
                                NodeAsString( pNode ),
                                /*editable*/true),
                     params );
        continue;
    }
    for (xmlNodePtr subparamNode=pNode->children; subparamNode!=NULL && subparamNode->type == XML_ELEMENT_NODE; subparamNode=subparamNode->next) { // �஡�� �� subparams
        ProgTrace( TRACE5, "subparam name=%s", (const char*)subparamNode->name );
    //	editable = NodeAsInteger( "@editable", subparamNode, 0 );
      ProgTrace( TRACE5, "param name=%s,subparam_name=%s,param_value=%s,editable=%d",
                 (const char*)pNode->name,(const char*)subparamNode->name,
                 NodeAsString( subparamNode ), /*editable*/true );
      PutParams( TDevParam((const char*)pNode->name,
                          (const char*)subparamNode->name,
                          NodeAsString( subparamNode ),
                          /*editable*/true),
                   params );
    }
  }
}

void GetEventCmd( const vector<string> &event_names,
                  const bool exclude,
                  const string &dev_model,
                  const string &sess_type,
                  const string &fmt_type,
                  TCategoryDevParams &serverParams )
{
  serverParams.clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT DISTINCT dev_model,sess_type,fmt_type,desk_grp_id,event_name "
    "FROM dev_event_cmd "
    "WHERE fmt_type=:fmt_type AND "
    "      (dev_model IS NULL OR dev_model=:dev_model) AND "
    "      (desk_grp_id IS NULL OR desk_grp_id=:desk_grp_id) AND "
    "      (sess_type IS NULL OR sess_type=:sess_type) "
    "ORDER BY dev_model NULLS LAST, desk_grp_id NULLS LAST, sess_type NULLS LAST, event_name";
  Qry.CreateVariable("dev_model",otString,dev_model);
  Qry.CreateVariable("sess_type",otString,sess_type);
  Qry.CreateVariable("fmt_type",otString,fmt_type);
  Qry.CreateVariable("desk_grp_id",otInteger,TReqInfo::Instance()->desk.grp_id);
  Qry.Execute();
  if (Qry.Eof) return;
  string first_dev_model=Qry.FieldAsString("dev_model");
  string first_sess_type=Qry.FieldAsString("sess_type");
  string first_fmt_type=Qry.FieldAsString("fmt_type");
  int first_desk_grp_id=ASTRA::NoExists;
  if (!Qry.FieldIsNULL("desk_grp_id"))
    first_desk_grp_id=Qry.FieldAsInteger("desk_grp_id");


  TQuery CmdQry(&OraSession);
  CmdQry.Clear();
  CmdQry.SQLText=
    "SELECT cmd_data,cmd_order,wait_prior_cmd,cmd_fmt_hex,cmd_fmt_file, "
    "       binary,posted,error_show,error_log,error_abort "
    "FROM dev_event_cmd "
    "WHERE fmt_type=:fmt_type AND event_name=:event_name AND "
    "      (dev_model IS NULL AND :dev_model IS NULL OR dev_model=:dev_model) AND "
    "      (desk_grp_id IS NULL AND :desk_grp_id IS NULL OR desk_grp_id=:desk_grp_id) AND "
    "      (sess_type IS NULL AND :sess_type IS NULL OR sess_type=:sess_type) "
    "ORDER BY cmd_order";
  CmdQry.CreateVariable("dev_model",otString,first_dev_model);
  CmdQry.CreateVariable("sess_type",otString,first_sess_type);
  CmdQry.CreateVariable("fmt_type",otString,first_fmt_type);
  if (first_desk_grp_id!=ASTRA::NoExists)
    CmdQry.CreateVariable("desk_grp_id",otInteger,first_desk_grp_id);
  else
    CmdQry.CreateVariable("desk_grp_id",otInteger,FNull);
  CmdQry.DeclareVariable("event_name",otString);

  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    if (first_dev_model!=Qry.FieldAsString("dev_model") ||
        first_sess_type!=Qry.FieldAsString("sess_type") ||
        first_fmt_type!=Qry.FieldAsString("fmt_type") ||
        !((first_desk_grp_id==ASTRA::NoExists && Qry.FieldIsNULL("desk_grp_id")) ||
          first_desk_grp_id==Qry.FieldAsInteger("desk_grp_id"))) break;

    string event_name=Qry.FieldAsString("event_name");
    bool event_found=find(event_names.begin(),event_names.end(),event_name)!=event_names.end();

    ProgTrace(TRACE5,"event_name=%s event_found=%d",event_name.c_str(),(int)event_found);

    //�஢�ਬ exclude
    if (( exclude &&  event_found) ||
        (!exclude && !event_found) ) continue;

    CmdQry.SetVariable("event_name",event_name);

    XMLDoc cmdDoc("commands");
    xmlNodePtr cmdsNode=NodeAsNode("/commands",cmdDoc.docPtr());
    CmdQry.Execute();
    for(;!CmdQry.Eof;CmdQry.Next())
    {
      xmlNodePtr cmdNode;
      string cmd_data,cmd_data_hex;

      cmd_data=CmdQry.FieldAsString("cmd_data");
      ProgTrace(TRACE5,"cmd_data=%s",cmd_data.c_str());

      if (CmdQry.FieldAsInteger("cmd_fmt_file")!=0)
      {
        ifstream f;
        f.open(cmd_data.c_str());
        if (!f.is_open()) throw Exception("Can't open file '%s'",cmd_data.c_str());
        try
        {
          ostringstream cmd_data_stream;
          cmd_data_stream << f.rdbuf();
          cmd_data=cmd_data_stream.str();
          f.close();
        }
        catch(...)
        {
          try { f.close(); } catch( ... ) { };
          throw;
        };
      };

      if (CmdQry.FieldAsInteger("cmd_fmt_hex")!=0)
      {
        //������� � ��⭠����筮� �ଠ�
        cmd_data_hex=cmd_data;
        if (!HexToString(cmd_data_hex,cmd_data))
          throw Exception("GetEventCmd: not hexadecimal format (cmd_data=%s)",cmd_data_hex.c_str());

      };

      if (CmdQry.FieldAsInteger("binary")==0 &&
          ValidXMLString(cmd_data))
      {
        cmdNode=NewTextChild(cmdsNode,"command",cmd_data);
        SetProp(cmdNode,"hex",(int)false);
      }
      else
      {
        StringToHex(cmd_data,cmd_data_hex);
        cmdNode=NewTextChild(cmdsNode,"command",cmd_data_hex);
        SetProp(cmdNode,"hex",(int)true);
      };
      SetProp(cmdNode,"wait_prior_cmd", (int)(CmdQry.FieldAsInteger("wait_prior_cmd")!=0));
      SetProp(cmdNode,"posted",         (int)(CmdQry.FieldAsInteger("posted")!=0));
      SetProp(cmdNode,"error_show",     (int)(CmdQry.FieldAsInteger("error_show")!=0));
      SetProp(cmdNode,"error_log",      (int)(CmdQry.FieldAsInteger("error_log")!=0));
      SetProp(cmdNode,"error_abort",    (int)(CmdQry.FieldAsInteger("error_abort")!=0));
    };
    serverParams.push_back( TDevParam("cmd_"+event_name,"",
                                      XMLTreeToText(cmdDoc.docPtr()),
                                      (int)false) );
  };
}

void GetParams( TQuery &Qry, TCategoryDevParams &serverParams )
{
    serverParams.clear();
  string param_name,subparam_name;
  for(;!Qry.Eof;Qry.Next())
  {
    string qry_param_name = lowerc(Qry.FieldAsString("param_name"));
    string qry_subparam_name = lowerc(Qry.FieldAsString("subparam_name"));
    if (param_name==qry_param_name &&
        subparam_name==qry_subparam_name) continue;
    param_name = qry_param_name;
    subparam_name = qry_subparam_name;
    serverParams.push_back( TDevParam(param_name,subparam_name,
                                      Qry.FieldAsString("param_value"),
                                      Qry.FieldAsInteger("editable")) );
  }
}

void BuildParams( xmlNodePtr paramsNode, TCategoryDevParams &params, bool pr_editable )
{
    if ( paramsNode == NULL || params.empty() ) return;
  string paramType;
  xmlNodePtr paramNode=NULL,subparamNode;
  for (TCategoryDevParams::iterator iparam=params.begin(); iparam!=params.end(); iparam++) {
//  	ProgTrace( TRACE5, "param_name=%s, subparam_name=%s, param_value=%s, editable=%d, pr_editable=%d",
//  	           iparam->param_name.c_str(), iparam->subparam_name.c_str(), iparam->param_value.c_str(), iparam->editable, pr_editable );
    if ( paramNode==NULL || (const char*)paramNode->name!=iparam->param_name ) {
      if ( iparam->subparam_name.empty() ) {
        paramNode = NewTextChild( paramsNode, iparam->param_name.c_str(), iparam->param_value );
        SetProp( paramNode,"editable",(int)(iparam->editable && pr_editable) );
      }
      else
        paramNode=NewTextChild(paramsNode, iparam->param_name.c_str());
    }
    if ( !iparam->subparam_name.empty() ) {
      subparamNode=NewTextChild( paramNode, iparam->subparam_name.c_str(), iparam->param_value );
      SetProp( subparamNode,"editable", (int)(iparam->editable && pr_editable) );
    }
  }
}

void GetTerminalParams(xmlNodePtr reqNode, std::vector<std::string> &paramsList )
{
  paramsList.clear();
  if ( reqNode == NULL ) return;
  xmlNodePtr node = GetNode("command_line_params",reqNode);
  if ( node == NULL || node->children == NULL ) return;
  string params;
  node = node->children;
  while ( node != NULL && string((char*)node->name) == "param" ) {
    if ( !params.empty() )
      params += " ";
    params += string("'") + NodeAsString( node ) + "'";
    paramsList.push_back( upperc( NodeAsString( node ) ) );
    node = node->next;
  }
  ProgTrace( TRACE5, "Terminal command line params=%s", params.c_str() );
}

struct TDevModelDefaults
{
  string op_type;
  string dev_model;
  string sess_type;
  string fmt_type;
  TDevModelDefaults( const char* vop_type,
                     const char* vdev_model,
                     const char* vsess_type,
                     const char* vfmt_type) {
    op_type = vop_type;
    dev_model = vdev_model;
    sess_type = vsess_type;
    fmt_type = vfmt_type;
  };
};

struct TPlatformParams
{
  string dev_model, addr, pool_key;
  TPlatformParams() {};
  TPlatformParams(const string& p1,
                  const string& p2,
                  const string& p3) : dev_model(p1), addr(p2), pool_key(p3) {};
};

void AddPlatformParams(const ASTRA::TDevOperType &oper,
                       const string &dev_model,
                       const vector<string> &addrs,
                       const string &pool_key,
                       const set<string> &addrs_in_use,
                       map<TDevOperType, TPlatformParams > &opers)
{
  if ( opers.find( oper ) == opers.end() )
  {
    for(vector<string>::const_iterator a=addrs.begin(); a!=addrs.end(); ++a)
    {
      string addr(*a);
      TrimString(addr);
      if (addr.empty()) continue;
      if (addrs_in_use.find(addr)!=addrs_in_use.end()) continue;
      opers.insert( make_pair( oper, TPlatformParams(dev_model, addr, pool_key) ) );
      break;
    }
  }
}

void GetPlatformAddrs( const ASTRA::TOperMode desk_mode,
                       xmlNodePtr reqNode,
                       map<TDevOperType, TPlatformParams > &opers,
                       map<string/*dev_model*/, set<string> > &addrs )
{
  opers.clear();
  addrs.clear();
  xmlNodePtr n = NULL;
  string contextName;
  switch(desk_mode)
  {
    case omCUSE:
      n=GetNode("cuse_variables",reqNode);
      contextName="cuse_device_variables";
      break;
    case omMUSE:
      n=GetNode("muse_variables",reqNode);
      contextName="muse_device_variables";
      break;
    case omRESA:
      n=GetNode("resa_variables",reqNode);
      contextName="resa_device_variables";
      break;
    default: return;
  };

  if ( n == NULL || n->children == NULL ) {
    // GetDeviceInfo - ����� �� �ନ����
    vector<string> oper_vars;
    vector<string> dev_vars;
    SeparateString( JxtContext::getJxtContHandler()->sysContext()->read(contextName), 5, oper_vars );
    for ( vector<string>::iterator istr=oper_vars.begin(); istr!=oper_vars.end(); istr++ ) {
      ProgTrace( TRACE5, "vars=%s", istr->c_str() );
      SeparateString( *istr, '=', dev_vars );
      vector<string>::iterator idev_var = dev_vars.begin();
      if ( idev_var != dev_vars.end() && ++idev_var != dev_vars.end() ) {
        vector<string> devs;
        SeparateString( *idev_var, ',', devs );
        addrs[ *(dev_vars.begin()) ] = set<string>(devs.begin(), devs.end());
      }
    }
    return;
  }

  for ( int pass=0; pass<3; pass++ )
  {
    for(xmlNodePtr node = n->children; node!=NULL; node = node->next)
    {
      string env_name=(char*)node->name;
      ASTRA::TDevClassType dev_class=getDevClass(desk_mode, env_name);
      string dev_model=getDefaultDevModel(desk_mode, dev_class);

      if ( (dev_class == dctATB && pass==0) ||
           (dev_class == dctBTP && pass==0) ||
           (dev_class == dctBGR && pass==0) ||
           (dev_class == dctDCP && pass==0) ||
           (dev_class == dctSCN && pass==1) ||
           (dev_class == dctOCR && pass==1) ||
           (dev_class == dctMSR && pass==1) ||
           (dev_class == dctWGE && pass==2) )
      {
        vector<string> dev_addrs;
        SeparateString((string)NodeAsString( node ), ',', dev_addrs);
        if ( dev_addrs.empty() ) continue;

        set<string> &dev_addrs_set=addrs[ dev_model ];
        dev_addrs_set.insert(dev_addrs.begin(), dev_addrs.end());

        if ( dev_class == dctATB )
          AddPlatformParams(dotPrnBP, dev_model, dev_addrs, env_name, set<string>(), opers);

        if ( dev_class == dctBTP )
          AddPlatformParams(dotPrnBT, dev_model, dev_addrs, env_name, set<string>(), opers);

        if ( dev_class == dctDCP )
        {
          AddPlatformParams(dotPrnFlt, dev_model, dev_addrs, env_name, set<string>(), opers);
          AddPlatformParams(dotPrnArch, dev_model, dev_addrs, env_name, set<string>(), opers);
          AddPlatformParams(dotPrnDisp, dev_model, dev_addrs, env_name, set<string>(), opers);
          AddPlatformParams(dotPrnTlg, dev_model, dev_addrs, env_name, set<string>(), opers);
        };

        if ( dev_class == dctBGR ||
             dev_class == dctSCN ||
             dev_class == dctWGE)
        {
          set<string> addrs_in_use;
          AddPlatformParams(dotScnBP1, dev_model, dev_addrs, env_name, addrs_in_use, opers);
          if ( opers.find( dotScnBP1 ) != opers.end() )
          {
            addrs_in_use.insert(opers[dotScnBP1].addr);
            AddPlatformParams(dotScnBP2, dev_model, dev_addrs, env_name, addrs_in_use, opers);
          };
        }

        if ( dev_class == dctOCR ||
             dev_class == dctWGE)
          AddPlatformParams(dotScnDoc, dev_model, dev_addrs, env_name, set<string>(), opers);

        if ( dev_class == dctMSR ||
             dev_class == dctWGE)
          AddPlatformParams(dotScnCard, dev_model, dev_addrs, env_name, set<string>(), opers);
      };
    };
  };

  if ( opers.find( dotScnBP1 ) != opers.end() &&
       opers.find( dotScnBP2 ) != opers.end() &&
       opers[dotScnBP1].pool_key == opers[dotScnBP2].pool_key)
  {
    opers[dotScnBP1].pool_key+="1";
    opers[dotScnBP2].pool_key+="2";
  };

  string str_addrs;
  for ( map<string, set<string> >::iterator ivar=addrs.begin(); ivar!=addrs.end(); ivar++ ) {
    if ( !str_addrs.empty() )
      str_addrs.append( string( 1, 5 ) );
    str_addrs += ivar->first + "=";
    for ( set<string>::iterator iaddr=ivar->second.begin(); iaddr!=ivar->second.end(); iaddr++ ) {
      if ( iaddr != ivar->second.begin() )
        str_addrs += ",";
      str_addrs += *iaddr;
    }
  }
  ProgTrace( TRACE5, "write to context: %s=%s", contextName.c_str(), str_addrs.c_str() );
  JxtContext::getJxtContHandler()->sysContext()->write(contextName,str_addrs);
}

void MainDCSInterface::GetEventCmd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr devsNode=NodeAsNode("devices",reqNode);
  xmlNodePtr devNode,node,node2;
  vector<string> event_names;
  TCategoryDevParams params;
  for(devNode=devsNode->children;devNode!=NULL;devNode=devNode->next)
  {
    node2=devNode->children;
    event_names.clear();
    node=GetNodeFast("events",node2);
    if (node!=NULL)
    {
      for(node=node->children;node!=NULL;node=node->next)
        event_names.push_back(NodeAsString(node));
      ::GetEventCmd( event_names,
                     false,
                     NodeAsStringFast("dev_model",node2),
                     NodeAsStringFast("sess_type",node2),
                     NodeAsStringFast("fmt_type",node2),
                     params );
      node=NodeAsNodeFast("events",node2);
      xmlUnlinkNode(node);
      xmlFreeNode(node);
    }
    else
    {
      event_names.push_back("magic_btn_click");
      event_names.push_back("first_fmt_magic_btn_click");
      ::GetEventCmd( event_names,
                     true,
                     NodeAsStringFast("dev_model",node2),
                     NodeAsStringFast("sess_type",node2),
                     NodeAsStringFast("fmt_type",node2),
                     params );
    };
    BuildParams( NewTextChild(devNode,"events"), params, false );
  };
  CopyNodeList(resNode,reqNode);
};

void WriteFilterParamsContext(const set<string> &airlines,
                              const set<string> &airps)
{
  ostringstream ctxt1, ctxt2;
  for(set<string>::const_iterator i=airlines.begin(); i!=airlines.end(); ++i)
    ctxt1 << (i==airlines.begin()?"":"/") << *i;
  for(set<string>::const_iterator i=airps.begin(); i!=airps.end(); ++i)
    ctxt2 << (i==airps.begin()?"":"/") << *i;
  if (!ctxt1.str().empty())
    JxtContext::getJxtContHandler()->sysContext()->write("filter_airlines", ctxt1.str());
  if (!ctxt2.str().empty())
    JxtContext::getJxtContHandler()->sysContext()->write("filter_airps", ctxt2.str());
};

void RemoveFilterParamsContext()
{
  JxtContext::getJxtContHandler()->sysContext()->remove("filter_airlines");
  JxtContext::getJxtContHandler()->sysContext()->remove("filter_airps");
};

void GetFilterParams(const string &airlines_str,
                     const string &airps_str,
                     set<string> &airlines,
                     set<string> &airps)
{
  airlines.clear();
  airps.clear();
  TElemFmt fmt;
  string str;

  str=airlines_str;
  TrimString(str);
  if (!str.empty())
  {
    string airline = ElemToElemId( etAirline, upperc(str), fmt );
    if (fmt==efmtUnknown)
      throw UserException( "MSG.AIRLINE.INVALID_INPUT_VALUE",
                           LParams()<<LParam("airline", str ) );

    airlines.insert(airline);
  };

  str=airps_str;
  TrimString(str);
  if (!str.empty())
  {
    string airp = ElemToElemId( etAirp, upperc(str), fmt );
    if (fmt==efmtUnknown)
      throw UserException( "MSG.AIRP.INVALID_INPUT_VALUE",
                           LParams()<<LParam("airp", str ) );

    airps.insert(airp);
  };
};


const int run_params_separator=5;

class TSessionAirline
{
  public:
    string code;
    string code_lat;
    string aircode;
    string run_params;
};

typedef map<string, TSessionAirline> TSessionAirlines;

void WriteSessionParamsContext(const TSessionAirlines &airlines)
{
  ostringstream ctxt1, ctxt2;
  for(TSessionAirlines::const_iterator i=airlines.begin(); i!=airlines.end(); ++i)
  {
    ctxt1 << (i==airlines.begin()?"":"/") << i->first;
    ctxt2 << (i==airlines.begin()?"":string(1,run_params_separator)) << i->second.run_params;
  };
  JxtContext::getJxtContHandler()->sysContext()->write("session_airlines", ctxt1.str());
  JxtContext::getJxtContHandler()->sysContext()->write("session_airlines_params", ctxt2.str());

};

void RemoveSessionParamsContext()
{
  JxtContext::getJxtContHandler()->sysContext()->remove("session_airlines");
  JxtContext::getJxtContHandler()->sysContext()->remove("session_airlines_params");
};

void SessionParamsFromContext(vector<string> &run_params)
{
  run_params.clear();
  string ctxt=JxtContext::getJxtContHandler()->sysContext()->read("session_airlines_params");
  SeparateString(ctxt, run_params_separator, run_params);
};

void SessionParamsFromXML(xmlNodePtr reqNode, vector<string> &run_params)
{
  run_params.clear();
  xmlNodePtr node=GetNode("airlines", reqNode);
  if (node==NULL) return;

  for(node=node->children;node!=NULL;node=node->next)
    run_params.push_back(NodeAsString("@run_params", node));
};

void GetSessionAirlines(const vector<string> &run_params, TSessionAirlines &airlines, string &error_param)
{
  airlines.clear();
  error_param.clear();

  for(vector<string>::const_iterator p=run_params.begin(); p!=run_params.end(); ++p)
  {
    TSessionAirline sess;
    sess.run_params=*p;
    string code=sess.run_params;
    TrimString(code);
    size_t p1 = code.find( " " );
    if ( p1 != string::npos ) code.erase( p1 );
    string airline;
    for(int pass=0; pass<3; pass++)
    {
      if (pass==0)
      {
        try
        {
          TAirlinesRow &row=(TAirlinesRow&)(base_tables.get("airlines").get_row("code/code_lat",code));
          airline=row.code;
          sess.code=row.code;
          sess.code_lat=row.code_lat;
          sess.aircode=row.aircode;
          break;
        }
        catch(EBaseTableError) {};
      };
      if (pass==1)
      {
        try
        {
          TAirlinesRow &row=(TAirlinesRow&)(base_tables.get("airlines").get_row("aircode",code));
          airline=row.code;
          sess.code=row.code;
          sess.code_lat=row.code_lat;
          sess.aircode=row.aircode;
          break;
        }
        catch(EBaseTableError) {};
      };
      if (pass==2)
      {
        QParams QryParams;
        QryParams << QParam("code", otString, code);
        TCachedQuery Qry(
          "SELECT DISTINCT term_param_airlines.airline, "
          "       NVL(term_param_airlines.code, airlines.code) AS code, "
          "       NVL(term_param_airlines.code_lat, airlines.code_lat) AS code_lat, "
          "       NVL(term_param_airlines.aircode, airlines.aircode) AS aircode "
          "FROM airlines, term_param_airlines "
          "WHERE term_param_airlines.airline=airlines.code AND airlines.pr_del=0 AND "
          "      :code IN (term_param_airlines.code, "
          "                term_param_airlines.code_lat, "
          "                term_param_airlines.aircode) ", QryParams);
        Qry.get().Execute();
        if (!Qry.get().Eof)
        {
          airline=Qry.get().FieldAsString("airline");
          sess.code=Qry.get().FieldAsString("code");
          sess.code_lat=Qry.get().FieldAsString("code_lat");
          sess.aircode=Qry.get().FieldAsString("aircode");
          Qry.get().Next();
          if (!Qry.get().Eof)
          {
            //�㡫�஢���� � term_param_airlines
            ProgError(STDLOG, "GetSessionAirlines: same code '%s' for different rows in term_param_airlines", code.c_str());
            airline.clear();
            sess.code.clear();
            sess.code_lat.clear();
            sess.aircode.clear();
          };
        };
      };
    };
    if (!airline.empty())
      airlines.insert(make_pair(airline, sess));
    else
      if (error_param.empty()) error_param=sess.run_params;
  };
};

void PutSessionAirlines(const TSessionAirlines &airlines, xmlNodePtr resNode)
{
  if (resNode==NULL) return;
  TReqInfo *reqInfo = TReqInfo::Instance();

  xmlNodePtr airlinesNode=NewTextChild(resNode,"airlines");
  ProgTrace(TRACE5, "PutSessionAirlines: ");
  ProgTrace(TRACE5, "%-10s|%-10s|%-10s|%s", "code", "code_lat", "aircode", "run_params");
  for(TSessionAirlines::const_iterator i=airlines.begin(); i!=airlines.end(); ++i)
  {
    if (!reqInfo->user.access.airlines().permitted(i->first)) continue;
    TSessionAirline sess=i->second;
    int aircode;
    if (StrToInt(sess.aircode.c_str(),aircode)==EOF || sess.aircode.size()!=3)
      sess.aircode="954";

    xmlNodePtr node=NewTextChild(airlinesNode,"airline");
    NewTextChild(node, "code", sess.code);
    NewTextChild(node, "code_lat", sess.code_lat, "");
    NewTextChild(node, "aircode", sess.aircode, "");
    NewTextChild(node, "run_params", sess.run_params, "");

    ProgTrace(TRACE5, "%-10s|%-10s|%-10s|%s",
              sess.code.c_str(), sess.code_lat.c_str(), sess.aircode.c_str(), sess.run_params.c_str());
  };
  //���, ����ᠭ�� ���� �祭� ���� �孮�����᪨,
  //⠪ ��� �� ��뢠�� ����� ���짮��⥫� � ���樠����樥� ����㤮����� �������
  //�������筮 ������� �஡���� � �� �����, ��� � ��몥 ����᪠ �� �ய�ᠭ� ��������
  //� �� �⮬ ��-� ����� ᤥ���� ����� ���짮��⥫� �� �ᥬ �/�
  if (reqInfo->user.access.airlines().elems_permit())
  {
    ostringstream str;
    for(set<string>::const_iterator i=reqInfo->user.access.airlines().elems().begin();
                                    i!=reqInfo->user.access.airlines().elems().end(); ++i)
    {
      if (airlines.find(*i)!=airlines.end()) continue;

      TAirlinesRow &row=(TAirlinesRow&)(base_tables.get("airlines").get_row("code",*i));
      TSessionAirline sess;
      sess.code=row.code;
      sess.code_lat=row.code_lat;
      sess.aircode=row.aircode;

      int aircode;
      if (StrToInt(sess.aircode.c_str(),aircode)==EOF || sess.aircode.size()!=3)
        sess.aircode="954";

      xmlNodePtr node=NewTextChild(airlinesNode,"airline");
      NewTextChild(node, "code", sess.code);
      NewTextChild(node, "code_lat", sess.code_lat, "");
      NewTextChild(node, "aircode", sess.aircode, "");

      ProgTrace(TRACE5, "%-10s|%-10s|%-10s|%s",
                sess.code.c_str(), sess.code_lat.c_str(), sess.aircode.c_str(), sess.run_params.c_str());
      str << " " << sess.code;
    };
    if (!str.str().empty() && reqInfo->desk.mode!=omSTAND)
    {
      ProgTrace(TRACE5, "PutSessionAirlines WARNING! Some airlines from user access added into session parameters");
      ProgTrace(TRACE5, "PutSessionAirlines WARNING! Platform: %s; Desk group: %d; Airlines:%s",
                        EncodeOperMode(reqInfo->desk.mode).c_str(),
                        reqInfo->desk.grp_id,
                        str.str().c_str());
    };
  };
};

void ResaEquipmentToXML(const vector<string> &paramsList, xmlNodePtr node)
{
  const char* equip_param_name="EQUIPMENT";
  string param_name, param_value;

  //�饬 ��६����� EQUIPMENT
  for ( vector<string>::const_iterator istr=paramsList.begin(); istr!=paramsList.end(); istr++ )
  {
    size_t pos=istr->find("=");
    if (pos!=string::npos)
    {
      param_name=istr->substr(0,pos);
      param_value=istr->substr(pos+1);
    }
    else
    {
      param_name=*istr;
      param_value.clear();
    };
    TrimString(param_name);
    TrimString(param_value);
    if (param_name==equip_param_name) break;
  };
  if (param_name!=equip_param_name) param_value="ATB0,BTP0,BGR0,BGR1,RTE0,DCP0";

  ProgTrace( TRACE5, "%s=%s", equip_param_name, param_value.c_str()  );

  xmlNodePtr varNode=NewTextChild(node, "resa_variables");
  vector<string> addrs;
  SeparateString( param_value, ',', addrs );
  for ( vector<string>::iterator addr=addrs.begin(); addr!=addrs.end(); addr++ )
  {
    TrimString( *addr );
    //��।���� � ����� ����樨 �⭮���� ����
    if (addr->size()!=4) continue;
    if (!IsDigit(*addr->rbegin())) continue;

    NewTextChild(varNode, addr->substr(0,3).c_str(), *addr);
  };
}

void GetDevices( xmlNodePtr reqNode, xmlNodePtr resNode )
{
    /*��࠭�祭�� �� ��।���/�ਥ� ��ࠬ��஢:
      ����������� ��ࠬ��஢ (subparam_name) �������� ⮫쪮 � ��砥 ᯥ�. ��ࠬ��஢, �����
      ࠧ������� �� �ࢥ� ᯥ�. ����ᠬ�. ���ਬ�� param_name="timeouts" subrapam_name="print"
      �������� ��ࠬ��஢ � �����ࠬ��஢ ��।����� � �࠭���� � ������ ॣ����
    */
       /* ��ଠ� ����� ��ਠ�⮢ ⨯�� ��ࠬ��஢ �� ����樨+���ன���
       <devices variants_operation="BP_PRINT" variant_dev_model="BTP CUTE" >*/
/* �� ��ࠬ����, ��室�騥 � ������ - ।����㥬� */

    /* ��ଠ� ��।�� (�����) ������ �� ���ன�⢠�
    <devices>
        <operation type="BP_PRINT">
          <dev_model_code name="">
          </dev_mode_code>
          <sess_params type="">

            <paramName1 editable=1> </paramName1>

            <paramName2>
              <subparamName3 editable=0> </subparamName3>
            </paramName2>

            ..params...




          </sess_params>
          <fmt_params type="">
            ..params...
                <timeouts>
                ...params...
                </timeouts>
          </fmt_params>
          <model_params type="">
            ..params...
          </model_params>
        </operation>
        ..operation...
    </devices>
1.����㦠�� ��ࠬ���� � ������. ��।��塞 ���� dev_model+sess_type+fmt_type
2.�஢�ઠ �� �, �� ⠪�� ���� ������� � ⠡��� dev_model_sess_fmt
3.�᫨ ���� �� ������ - ����㧪� ��ࠬ��஢ �� 㬮�砭��
4.�᫨ ���� ������ - ����㦠�� ��ࠬ���� �� 㬮�砭�� � �ࢥ� � �� ��� ������뢠�� ��ࠬ���� � ������
  ��ࠬ��� � ������ ����� ���� ������� �᫨:
  -editable=1
  -�������� ��ࠬ���/�����ࠬ��� ������ ���� ���ᠭ � ⠡��� dev_model_params � ������묨 dev_model+sess_type+fmt_type+(grp_id,NULL)
  */
  vector<string> paramsList;
  GetTerminalParams(reqNode,paramsList);
  if (reqNode==NULL || resNode==NULL) return;

  resNode=NewTextChild(resNode,"devices");

  xmlNodePtr devNode = GetNode("devices",reqNode);
  string variant_model = NodeAsString( "@variant_model", devNode, "" );
  bool pr_default_sets = GetNode( "@operation_default_sets", devNode );

  ProgTrace( TRACE5, "variants mode=%s, pr_default_sets=%d", variant_model.c_str(), pr_default_sets );
  if ( variant_model.empty() && !pr_default_sets )
  {
    vector<string> run_params;
    SessionParamsFromContext(run_params);
    TSessionAirlines sess_airlines;
    string error_param;
    GetSessionAirlines(run_params, sess_airlines, error_param);
    PutSessionAirlines(sess_airlines, resNode);
  };

   if (devNode==NULL) return; // �᫨ � ����� ��� ��o�� ��ࠬ���, � �� �㦭� ᮡ���� ����� �� ���ன�⢠�
  //string VariantsOperation = NodeAsString( "@variants_operation", devNode, "" );

  TReqInfo *reqInfo = TReqInfo::Instance();

  bool pr_editable = reqInfo->user.access.rights().permitted(840);

  TQuery SessParamsQry( &OraSession );
  SessParamsQry.SQLText=
    "SELECT dev_model_params.sess_type AS param_type, "
    "       param_name,subparam_name,param_value,editable "
    "FROM dev_model_params,dev_sess_modes "
    "WHERE (dev_model_params.dev_model=:dev_model OR dev_model_params.dev_model IS NULL) AND "
    "      dev_model_params.sess_type=dev_sess_modes.sess_type AND "
    "      dev_sess_modes.term_mode=:term_mode AND dev_sess_modes.sess_type=:sess_type AND "
    "      (dev_model_params.fmt_type=:fmt_type OR dev_model_params.fmt_type IS NULL) AND "
    "      (desk_grp_id=:desk_grp_id OR desk_grp_id IS NULL) AND "
    "      (pr_sess_param<>0 OR pr_sess_param IS NULL) "
    "ORDER BY param_type, param_name, subparam_name NULLS FIRST, "
    "         dev_model_params.dev_model NULLS LAST, desk_grp_id NULLS LAST, "
    "         dev_model_params.fmt_type NULLS LAST";


  SessParamsQry.CreateVariable("term_mode",otString,EncodeOperMode(reqInfo->desk.mode));
  SessParamsQry.CreateVariable("desk_grp_id",otInteger,reqInfo->desk.grp_id);
  SessParamsQry.DeclareVariable("dev_model",otString);
  SessParamsQry.DeclareVariable("sess_type",otString);
  SessParamsQry.DeclareVariable("fmt_type",otString);

    TQuery FmtParamsQry( &OraSession );
  FmtParamsQry.SQLText=
    "SELECT dev_model_params.fmt_type AS param_type, "
    "       param_name,subparam_name,param_value,editable "
    "FROM dev_model_params,dev_fmt_opers "
    "WHERE (dev_model_params.dev_model=:dev_model OR dev_model_params.dev_model IS NULL) AND "
    "      dev_model_params.fmt_type=dev_fmt_opers.fmt_type AND "
    "      dev_fmt_opers.op_type=:op_type AND dev_fmt_opers.fmt_type=:fmt_type AND "
    "      (dev_model_params.sess_type=:sess_type OR dev_model_params.sess_type IS NULL) AND "
    "      (desk_grp_id=:desk_grp_id OR desk_grp_id IS NULL) AND "
    "      (pr_sess_param=0 OR pr_sess_param IS NULL) "
    "ORDER BY param_type, param_name, subparam_name NULLS FIRST, "
    "         dev_model_params.dev_model NULLS LAST, desk_grp_id NULLS LAST, "
    "         dev_model_params.sess_type NULLS LAST";
  FmtParamsQry.CreateVariable("desk_grp_id",otInteger,reqInfo->desk.grp_id);
  FmtParamsQry.DeclareVariable("op_type",otString);
  FmtParamsQry.DeclareVariable("dev_model",otString);
  FmtParamsQry.DeclareVariable("sess_type",otString);
  FmtParamsQry.DeclareVariable("fmt_type",otString);

  TQuery ModelParamsQry( &OraSession );
  ModelParamsQry.SQLText=
    "SELECT NULL AS param_type, "
    "       param_name,subparam_name,param_value,editable "
    "FROM dev_model_params "
    "WHERE dev_model_params.dev_model=:dev_model AND sess_type IS NULL AND fmt_type IS NULL AND "
    "      (desk_grp_id=:desk_grp_id OR desk_grp_id IS NULL) "
    "ORDER BY param_type, param_name, subparam_name NULLS FIRST, desk_grp_id NULLS LAST";
  ModelParamsQry.CreateVariable("desk_grp_id",otInteger,reqInfo->desk.grp_id);
  ModelParamsQry.DeclareVariable("dev_model",otString);
  // ࠧ��ࠥ� ��ࠬ���� ��襤訥 � ������
  string dev_model, sess_type, fmt_type, client_dev_model, client_sess_type, client_fmt_type;
  TCategoryDevParams params;
  const char* dev_model_sql=
      "SELECT DISTINCT dev_fmt_opers.op_type AS op_type, "
      "                dev_model_sess_fmt.dev_model,"
      "                dev_model_sess_fmt.sess_type,"
      "                dev_model_sess_fmt.fmt_type"
      " FROM dev_model_sess_fmt, dev_sess_modes, dev_fmt_opers "
      "WHERE dev_sess_modes.term_mode=:term_mode AND "
      "      dev_sess_modes.sess_type=dev_model_sess_fmt.sess_type AND "
      "      dev_fmt_opers.op_type=:op_type AND "
      "      dev_fmt_opers.fmt_type=dev_model_sess_fmt.fmt_type AND "
      "      dev_model_sess_fmt.dev_model=:dev_model ";
  TQuery DefQry(&OraSession);
  if ( variant_model.empty() ) {
    string sql =
      "SELECT dev_oper_types.code AS op_type, "
      "       dev_model_defaults.dev_model, "
      "       dev_model_defaults.sess_type, "
      "       dev_model_defaults.fmt_type "
      "FROM dev_oper_types,dev_model_defaults "
      "WHERE dev_oper_types.code=dev_model_defaults.op_type(+) AND "
      "      dev_model_defaults.term_mode(+)=:term_mode ";
    if ( pr_default_sets ) {
        sql += " AND dev_oper_types.code=:op_type";
    }
    DefQry.SQLText=sql;
  }
  else {
    DefQry.SQLText=dev_model_sql;
    DefQry.CreateVariable( "dev_model", otString, variant_model );
  }
  DefQry.CreateVariable("term_mode",otString,EncodeOperMode(reqInfo->desk.mode));
  if ( !variant_model.empty() || pr_default_sets )
    DefQry.CreateVariable( "op_type", otString, NodeAsString( "operation/@type", devNode ) );
  DefQry.Execute();

  vector<TDevModelDefaults> DevModelDefaults;
  for( ;!DefQry.Eof;DefQry.Next() ) { // 横� �� ⨯�� ����権 ��� 横� �� �������� ��ਠ�⠬ ����஥� �������� ����樨
    DevModelDefaults.push_back( TDevModelDefaults( DefQry.FieldAsString( "op_type" ),
                                                   DefQry.FieldAsString( "dev_model" ),
                                                   DefQry.FieldAsString( "sess_type" ),
                                                   DefQry.FieldAsString( "fmt_type" ) ) );
  }

  map<TDevOperType, TPlatformParams > opers;
  map<string/*dev_model*/, set<string> > valid_addrs;
  if ( reqInfo->desk.mode==omRESA ||
       reqInfo->desk.mode==omCUSE ||
       (reqInfo->desk.mode==omMUSE && reqInfo->desk.compatible(MUSE_DEV_VARIABLES)) )
  {
    if ( reqInfo->desk.mode==omRESA )
      ResaEquipmentToXML(paramsList, reqNode);

    if ( reqInfo->desk.mode==omRESA ||
         reqInfo->desk.mode==omCUSE ||
         (reqInfo->desk.mode==omMUSE && reqInfo->desk.compatible(MUSE_DEV_VARIABLES)) )
    {
      GetPlatformAddrs(reqInfo->desk.mode, reqNode, opers, valid_addrs);
    };

    if (variant_model.empty())
    {
      //�� ���� ��ॢ롮� dev_model �१ "����ன�� ����㤮�����" � �ନ����
      DefQry.Clear();
      DefQry.SQLText=dev_model_sql;
      DefQry.CreateVariable( "term_mode", otString, EncodeOperMode(reqInfo->desk.mode));
      DefQry.DeclareVariable( "op_type", otString );
      DefQry.DeclareVariable( "dev_model", otString );

      for(vector<TDevModelDefaults>::iterator def=DevModelDefaults.begin();
                                              def!=DevModelDefaults.end(); def++)
      {
        def->dev_model.clear();
        def->sess_type.clear();
        def->fmt_type.clear();
        TDevOperType oper=DecodeDevOperType(def->op_type);
        if (!opers[oper].addr.empty())
        {
          DefQry.SetVariable( "op_type", def->op_type );
          DefQry.SetVariable( "dev_model", opers[oper].dev_model );
          DefQry.Execute();
          if (!DefQry.Eof)
          {
            def->dev_model=DefQry.FieldAsString( "dev_model" );
            def->sess_type=DefQry.FieldAsString( "sess_type" );
            def->fmt_type=DefQry.FieldAsString( "fmt_type" );
          };
        };
      };
    };
  };

  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT dev_model_sess_fmt.dev_model,dev_model_sess_fmt.sess_type,dev_model_sess_fmt.fmt_type "
    " FROM dev_model_sess_fmt,dev_sess_modes,dev_fmt_opers "
    "WHERE dev_model_sess_fmt.dev_model=:dev_model AND "
    "      dev_model_sess_fmt.sess_type=:sess_type AND "
    "      dev_model_sess_fmt.fmt_type=:fmt_type AND "
    "      dev_sess_modes.term_mode=:term_mode AND "
    "      dev_sess_modes.sess_type=dev_model_sess_fmt.sess_type AND "
    "      dev_fmt_opers.op_type=:op_type AND "
    "      dev_fmt_opers.fmt_type=dev_model_sess_fmt.fmt_type";

  Qry.DeclareVariable( "dev_model", otString );
  Qry.DeclareVariable( "sess_type", otString );
  Qry.DeclareVariable( "fmt_type", otString );
  Qry.DeclareVariable( "op_type", otString );
  Qry.CreateVariable( "term_mode", otString, EncodeOperMode(reqInfo->desk.mode) );
  string operation;
  xmlNodePtr operNode;

  for(vector<TDevModelDefaults>::const_iterator def=DevModelDefaults.begin();
                                                def!=DevModelDefaults.end(); def++)
  {
    // 横� �� ⨯�� ����権 ��� 横� �� �������� ��ਠ�⠬ ����஥� �������� ����樨
    if ( variant_model.empty() && operation == def->op_type ) continue;
    operation = def->op_type;

    for ( operNode=GetNode( "operation", devNode ); operNode!=NULL; operNode=operNode->next ) // �஡�� �� ������ ������
        if ( operation == NodeAsString( "@type", operNode ) )
            break;
    ProgTrace( TRACE5, "operation=%s, is client=%d", operation.c_str(), (int)(operNode!=NULL) );

    dev_model.clear();
    sess_type.clear();
    fmt_type.clear();

    if ( operNode != NULL ) { // ����� � ������
        // ����� ���� dev_model+sess_type+fmt_type. �������� 2 ��ਠ��:
        // 1. ��砫쭠� ���樠������
        // 2. ������� ��ਠ��� ࠡ��� ���ன�⢠, ������᪨� ��ࠬ���� ���� ࠧ����� ����� ���� ᮢ���
      client_dev_model = NodeAsString( "dev_model_code", operNode, "" );
      dev_model = client_dev_model;
      client_sess_type = NodeAsString( "sess_params/@type", operNode, "" );
      sess_type = client_sess_type;
      client_fmt_type = NodeAsString( "fmt_params/@type", operNode, "" );
      fmt_type = client_fmt_type;
    }
    //ProgTrace( TRACE5, "variant_model=%s", variant_model.c_str() );
    for ( int k=!variant_model.empty(); k<=1; k++ ) { // ��� ��室�: 0-��ࠬ���� � ������, 1 - c �ࢥ�
        //ProgTrace( TRACE5, "k=%d", k );

        if ( k == 1 ) {
        dev_model = def->dev_model;
        sess_type = def->sess_type;
        fmt_type =  def->fmt_type;
        }
        if ( dev_model.empty() && sess_type.empty() && fmt_type.empty() ) continue;
        bool pr_parse_client_params = ( client_dev_model == dev_model && client_sess_type == sess_type && client_fmt_type == fmt_type );

        if ( pr_parse_client_params ) {
          if (reqInfo->desk.mode==omRESA ||
              reqInfo->desk.mode==omCUSE ||
              (reqInfo->desk.mode==omMUSE && reqInfo->desk.compatible(MUSE_DEV_VARIABLES)))
          {
            //�஢���� �� ���� ������� ��襤訩 � ������
            pr_parse_client_params = false;
            if (operNode!=NULL)
            {
              string addr = NodeAsString( "sess_params/addr", operNode, "" );
              if ( !addr.empty() )  {
                set<string> &dev_model_addrs=valid_addrs[client_dev_model];
                pr_parse_client_params = dev_model_addrs.find(addr) != dev_model_addrs.end();
              }
            }
            if ( k == 0 && !pr_parse_client_params )
              continue;
          };
        }

      Qry.SetVariable( "dev_model", dev_model );
      Qry.SetVariable( "sess_type", sess_type );
      Qry.SetVariable( "fmt_type", fmt_type );
      Qry.SetVariable( "op_type", operation );
      Qry.Execute();
      ProgTrace( TRACE5, "dev_model=%s, sess_type=%s, fmt_type=%s", dev_model.c_str(), sess_type.c_str(), fmt_type.c_str() );
      if ( Qry.Eof ) continue; // ����� ���� dev_model+sess_type+fmt_type �� ࠧ�襭

      try {
        base_tables.get("DEV_MODELS").get_row( "code", dev_model );
      }
      catch(EBaseTableError){
        continue;
      };

      xmlNodePtr newoperNode=NewTextChild( resNode, "operation" );
      xmlNodePtr pNode;
      SetProp( newoperNode, "type", operation );
      string sess_name, fmt_name;
      if ( !variant_model.empty() ) {
        sess_name = ElemIdToNameLong( etDevSessType, def->sess_type );
        fmt_name = ElemIdToNameLong( etDevFmtType, def->fmt_type );
      }
      if ( !sess_name.empty() && !fmt_name.empty() ) {
        SetProp( newoperNode, "variant_name", sess_name + "/" + fmt_name );
      }
      pNode = NewTextChild( newoperNode, "dev_model_code", dev_model );
      SetProp( pNode, "dev_model_name", ElemIdToNameLong(etDevModel,dev_model) );
        sess_name = ElemIdToNameLong( etDevSessType, Qry.FieldAsString("sess_type") );
        ProgTrace( TRACE5, "sess_type=%s, sess_name=%s", Qry.FieldAsString("sess_type"), sess_name.c_str() );
        fmt_name = ElemIdToNameLong( etDevFmtType, Qry.FieldAsString("fmt_type") );

      if ( !sess_name.empty() && !fmt_name.empty() ) {
        SetProp( pNode, "sess_fmt_name", sess_name + "/" + "fmt_name" );
      }

      if (!reqInfo->desk.compatible(OLDEST_SUPPORTED_VERSION))
        NewTextChild( newoperNode, "dev_model_name", ElemIdToNameLong(etDevModel,dev_model));

      SessParamsQry.SetVariable("dev_model",dev_model);
      SessParamsQry.SetVariable("sess_type",sess_type);
      SessParamsQry.SetVariable("fmt_type",fmt_type);
      SessParamsQry.Execute();
      GetParams( SessParamsQry, params );
      if ( reqInfo->desk.mode==omRESA ||
           reqInfo->desk.mode==omCUSE ||
           (reqInfo->desk.mode==omMUSE && reqInfo->desk.compatible(MUSE_DEV_VARIABLES)) )
      {
        TDevOperType oper=DecodeDevOperType(operation);
        if (!opers[oper].dev_model.empty() && opers[oper].dev_model==dev_model)
        {
          for(TCategoryDevParams::iterator p=params.begin();p!=params.end();++p)
          {
            if (p->param_name=="addr") p->param_value=opers[oper].addr;
            if (p->param_name=="pool_key") p->param_value=opers[oper].pool_key;
          };
        };
      };
      //ProgTrace( TRACE5, "pr_parse_client_params=%d, k=%d", pr_parse_client_params, k );
      if ( pr_parse_client_params )
        ParseParams( GetNode( "sess_params", operNode ), params );
      //for(TCategoryDevParams::iterator p=params.begin();p!=params.end();++p)
      //  ProgTrace( TRACE5, "p->param_name=%s p->param_value=%s", p->param_name.c_str(),  p->param_value.c_str() );

      pNode = NewTextChild( newoperNode, "sess_params" );
      SetProp( pNode, "type", sess_type );
      BuildParams( pNode, params, pr_editable );

        FmtParamsQry.SetVariable("op_type",operation);
      FmtParamsQry.SetVariable("dev_model",dev_model);
      FmtParamsQry.SetVariable("sess_type",sess_type);
      FmtParamsQry.SetVariable("fmt_type",fmt_type);
      FmtParamsQry.Execute();
      GetParams( FmtParamsQry, params );
      if ( pr_parse_client_params )
        ParseParams( GetNode( "fmt_params", operNode ), params );
      pNode = NewTextChild( newoperNode, "fmt_params" );
      SetProp( pNode, "type", fmt_type );
      BuildParams( pNode, params, pr_editable );

      //�饬 ��ࠬ��� posted_events
      vector<TDevParam>::iterator iParams=params.begin();
      for(;iParams!=params.end();iParams++)
        if (iParams->param_name=="posted_events") break;
      bool posted_events=iParams!=params.end() && ToInt(iParams->param_value)!=0;

      if (!posted_events)
      {
        vector<string> event_names;
        event_names.push_back("magic_btn_click");
        event_names.push_back("first_fmt_magic_btn_click");
        ::GetEventCmd( event_names, true, dev_model, sess_type, fmt_type, params );
        pNode = NewTextChild( newoperNode, "events" );
        BuildParams( pNode, params, false );
      };

      ModelParamsQry.SetVariable("dev_model",dev_model);
      ModelParamsQry.Execute();
      GetParams( ModelParamsQry, params );
      if ( pr_parse_client_params )
        ParseParams( GetNode( "model_params", operNode ), params );
      pNode = NewTextChild( newoperNode, "model_params" );
      SetProp( pNode, "type", dev_model );
      BuildParams( pNode, params, pr_editable );
      break;
    }
  }
}

void MainDCSInterface::UserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText =
      "SELECT user_id, login, passwd, descr, pr_denial, desk "
      "FROM users2 "
      "WHERE login= UPPER(:userr) AND passwd= UPPER(:passwd) FOR UPDATE ";
    Qry.CreateVariable("userr", otString, NodeAsString("userr", reqNode));
    Qry.CreateVariable("passwd", otString, NodeAsString("passwd", reqNode));
    Qry.Execute();
    if ( Qry.RowCount() == 0 )
      throw AstraLocale::UserException("MSG.WRONG_LOGIN_OR_PASSWD");
    if ( Qry.FieldAsInteger( "pr_denial" ) == -1 )
        throw AstraLocale::UserException( "MSG.USER_DELETED" );
    if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
      throw AstraLocale::UserException( "MSG.USER_DENIED" );
    reqInfo->user.user_id = Qry.FieldAsInteger("user_id");
    reqInfo->user.login = Qry.FieldAsString("login");
    reqInfo->user.descr = Qry.FieldAsString("descr");
    if(Qry.FieldIsNULL("desk"))
    {
      AstraLocale::showMessage( "MSG.USER_WELCOME", LParams() << LParam("user", reqInfo->user.descr));
    }
    else
     if (reqInfo->desk.code != Qry.FieldAsString("desk"))
       AstraLocale::showMessage("MSG.PULT_SWAP");
    if (Qry.FieldAsString("passwd")==(string)Qry.FieldAsString("login") ) {
       if(!inTestMode()) {
         // �� ��ᨬ �������� ��஫� � ����
         AstraLocale::showErrorMessage("MSG.USER_NEED_TO_CHANGE_PASSWD");
       }
    }


    Qry.Clear();
    Qry.SQLText =
      "BEGIN "
      "  UPDATE users2 SET desk = NULL WHERE desk = :desk; "
      "  UPDATE users2 SET desk = :desk WHERE user_id = :user_id; "
      "  UPDATE desks "
      "  SET version = :version, last_logon = system.UTCSYSDATE, term_id=:term_id, term_mode=:term_mode "
      "  WHERE code = :desk; "
      "END;";
    Qry.CreateVariable("user_id", otInteger, reqInfo->user.user_id);
    Qry.CreateVariable("desk", otString, reqInfo->desk.code);
    if (GetNode("term_version", reqNode)==NULL)
      throw AstraLocale::UserException("MSG.TERM_VERSION.NOT_SUPPORTED");
    Qry.CreateVariable("version", otString, NodeAsString("term_version", reqNode));
    xmlNodePtr propNode;
    if ((propNode = GetNode("/term/query/@term_id",ctxt->reqDoc))!=NULL)
      Qry.CreateVariable("term_id", otFloat, NodeAsFloat(propNode));
    else
      Qry.CreateVariable("term_id", otFloat, FNull);
    Qry.CreateVariable("term_mode", otString, EncodeOperMode(reqInfo->desk.mode));
    Qry.Execute();

    if(inTestMode())
    {
        // ������塞 ������� �ਢ������ ��
        Qry.Clear();
        Qry.SQLText =
          "INSERT INTO user_roles VALUES(1, 1, :user_id)";
        Qry.CreateVariable("user_id", otInteger, reqInfo->user.user_id);
        Qry.Execute();
    }

    vector<string> run_params;
    SessionParamsFromXML(reqNode, run_params);
    TSessionAirlines sess_airlines;
    string error_param;
    GetSessionAirlines(run_params, sess_airlines, error_param);
    if (!error_param.empty())
      throw AstraLocale::UserException("MSG.AIRLINE_CODE_NOT_FOUND", LParams() << LParam("airline", error_param));
    WriteSessionParamsContext(sess_airlines);

    set<string> filter_airlines;
    set<string> filter_airps;
    xmlNodePtr node2=reqNode->children;
    GetFilterParams(NodeAsStringFast("filter_airlines", node2, ""),
                    NodeAsStringFast("filter_airps", node2, ""),
                    filter_airlines,
                    filter_airps);
    WriteFilterParamsContext(filter_airlines, filter_airps);

    xmlNodePtr node=NodeAsNode("/term/query",ctxt->reqDoc);

    TReqInfoInitData reqInfoData;
    reqInfoData.screen = NodeAsString("@screen", node);
    reqInfoData.pult = ctxt->pult;
    reqInfoData.opr = NodeAsString("@opr", node);
    reqInfoData.lang = reqInfo->desk.lang; //!��।������ �몠 �뭥ᥭ� � astracallbacks.cc::UserBefore �.�. ⠬ �������� ���⥪�� xmlRC->setLang(RUSSIAN) � �� �ॡ���� ������ ����୮ ��� ������ ������
    if ((propNode = GetNode("@mode", node))!=NULL)
      reqInfoData.mode = NodeAsString(propNode);
    if ((propNode = GetNode("@term_id", node))!=NULL)
      reqInfoData.term_id = NodeAsFloat(propNode);
    reqInfoData.checkUserLogon = true; // ����� ��� �.�. ���� ������ ���� �� ���짮��⥫�
    reqInfoData.checkCrypt = false; // �� ����� ��᫠ ������ ������� �஢���
    reqInfoData.duplicate = reqInfo->duplicate; //!��।������ �㡫���� ����� �뭥ᥭ� � astracallbacks.cc::UserBefore �.�. ⠬ ࠧ��ࠥ��� head
    reqInfo->Initialize( reqInfoData );

    //����� reqInfo ��ଠ�쭮 ���樠����஢��
    CheckTermExpireDate();

    GetModuleList(resNode);
    GetDevices(reqNode,resNode);
    GetNotices(resNode);

    if ( GetNode("lang",reqNode) ) { //!!!����室���, �⮡� �� ᫮���� ��� �몠 �� 㬮�砭�� - ����� ��� �⮩ �஢�ન!!!
        ProgTrace( TRACE5, "desk.lang=%s, dict.lang=%s", reqInfo->desk.lang.c_str(), NodeAsString( "lang/@dictionary_lang",reqNode) );
      int client_checksum = NodeAsInteger("lang/@dictionary_checksum",reqNode);
      int server_checksum = AstraLocale::TLocaleMessages::Instance()->checksum( reqInfo->desk.lang );
      if ( NodeAsString( "lang/@dictionary_lang",reqNode) != reqInfo->desk.lang ||
           client_checksum == 0 ||
           server_checksum != client_checksum ) {
        ProgTrace( TRACE5, "Send dictionary: lang=%s, client_checksum=%d, server_checksum=%d",
                   reqInfo->desk.lang.c_str(), client_checksum,
                   AstraLocale::TLocaleMessages::Instance()->checksum( reqInfo->desk.lang ) );
        SetProp(NewTextChild( resNode, "lang", reqInfo->desk.lang ), "dictionary", AstraLocale::TLocaleMessages::Instance()->getDictionary(reqInfo->desk.lang));
      }
      else {
        NewTextChild( resNode, "lang", reqInfo->desk.lang );
      }
    }
    showBasicInfo();
}

void MainDCSInterface::UserLogoff(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    Qry.SQLText = "UPDATE users2 SET desk = NULL WHERE user_id = :user_id";
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.Execute();
    RemoveSessionParamsContext();
    RemoveFilterParamsContext();
    AstraLocale::showMessage("MSG.WORK_SEANCE_FINISHED");
    reqInfo->user.clear();
    reqInfo->desk.clear();
    showBasicInfo();
}

void MainDCSInterface::ChangePasswd(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    if (GetNode("old_passwd", reqNode)!=NULL)
    {
      Qry.Clear();
      Qry.SQLText =
        "SELECT passwd FROM users2 WHERE user_id = :user_id FOR UPDATE";
      Qry.CreateVariable("user_id", otInteger, reqInfo->user.user_id);
      Qry.Execute();
      if (Qry.Eof) throw Exception("user not found (user_id=%d)",reqInfo->user.user_id);
      if (strcmp(Qry.FieldAsString("passwd"),NodeAsString("old_passwd", reqNode))!=0)
        throw AstraLocale::UserException("MSG.PASSWORD.CURRENT_WRONG_SET");
    };
    Qry.Clear();
    Qry.SQLText =
      "BEGIN "
      "  UPDATE users2 SET passwd = :passwd WHERE user_id = :user_id; "
      "  hist.synchronize_history('users2',:user_id,:SYS_user_descr,:SYS_desk_code); "
      "END; ";
    Qry.CreateVariable("user_id", otInteger, reqInfo->user.user_id);
    Qry.CreateVariable("passwd", otString, NodeAsString("passwd", reqNode));
    Qry.CreateVariable( "SYS_user_descr", otString, reqInfo->user.descr );
    Qry.CreateVariable( "SYS_desk_code", otString, reqInfo->desk.code );
    Qry.Execute();
    if(Qry.RowsProcessed() == 0)
        throw Exception("user not found (user_id=%d)",reqInfo->user.user_id);
    TReqInfo::Instance()->LocaleToLog("EVT.PASSWORD.MODIFIED", evtAccess);
    AstraLocale::showMessage("MSG.PASSWORD.MODIFIED");
}

void MainDCSInterface::GetDeviceList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();

  string op_type;
  if (GetNode("operation",reqNode)!=NULL) op_type=NodeAsString("operation",reqNode);

  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;

  sql <<
    "SELECT dev_oper_types.code AS op_type, "
    "       dev_model_code "
    " FROM dev_oper_types, "
    "  (SELECT DISTINCT "
    "          dev_models.code AS dev_model_code, "
    "          dev_fmt_opers.op_type "
    "   FROM dev_models, "
    "        dev_model_sess_fmt,dev_sess_modes,dev_fmt_opers "
    "   WHERE dev_model_sess_fmt.dev_model=dev_models.code AND "
    "         dev_model_sess_fmt.sess_type=dev_sess_modes.sess_type AND "
    "         dev_model_sess_fmt.fmt_type=dev_fmt_opers.fmt_type AND "
    "         dev_sess_modes.term_mode=:term_mode ";

  if (!op_type.empty())
    sql << " AND dev_fmt_opers.op_type=:op_type ";

  sql <<
    "  ) dev_models "
    "WHERE dev_oper_types.code=dev_models.op_type(+) ";

  if (!op_type.empty())
    sql << " AND dev_oper_types.code=:op_type ";

  sql <<
    "ORDER BY op_type,dev_model_code";

  if (!op_type.empty())
    Qry.CreateVariable("op_type",otString,op_type);


  Qry.SQLText=sql.str().c_str();
  Qry.CreateVariable("term_mode",otString,EncodeOperMode(reqInfo->desk.mode));
  Qry.Execute();
  op_type.clear();
  xmlNodePtr opersNode=NewTextChild(resNode,"operations");
  xmlNodePtr operNode=NULL,devsNode=NULL,devNode=NULL;
  for(;!Qry.Eof;Qry.Next())
  {
    if (op_type!=Qry.FieldAsString("op_type")) operNode=NULL;
    if (operNode==NULL)
    {
      op_type=Qry.FieldAsString("op_type");
      operNode=NewTextChild(opersNode,"operation");
      NewTextChild(operNode,"type",op_type);
      NewTextChild(operNode,"name",ElemIdToNameLong(etDevOperType,Qry.FieldAsString("op_type")));
      devsNode=NewTextChild(operNode,"devices");
    };
    if (!Qry.FieldIsNULL("dev_model_code"))
    {
      devNode=NewTextChild(devsNode,"device");
      NewTextChild(devNode,"code",Qry.FieldAsString("dev_model_code"));
      NewTextChild(devNode,"name",ElemIdToNameLong(etDevModel,Qry.FieldAsString("dev_model_code")));
    };
  };
  opersNode=NewTextChild(resNode,"encodings");
  NewTextChild(opersNode,"encoding","CP855");
  NewTextChild(opersNode,"encoding","CP866");
  NewTextChild(opersNode,"encoding","CP1251");
};

void MainDCSInterface::GetDeviceInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetDevices( reqNode, resNode );
};

class TScanParams
{
  public:
    string prefix,postfix;
    unsigned int code_id_len;
};


bool ParseScanBPData(const string& data, TScanParams& params)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT pax_id FROM pax WHERE pax_id=:pax_id "
              "UNION "
              "SELECT TO_NUMBER(value) FROM prn_test_tags "
              "WHERE name=:tag_name AND TO_NUMBER(value)=:pax_id";
  Qry.DeclareVariable("pax_id",otInteger);
  Qry.CreateVariable("tag_name",otString,"pax_id");


  string str=data,c;
  string::size_type p,ph=0,str_size=str.size(),i;
  int pax_id;
  bool init=false;
  do
  {
    p=str.find_first_of("0123456789M",ph);
    if (p==string::npos) break;
    if (str_size<p+10) break;
    /*
    if (p<str_size)
      ProgTrace(TRACE5,"ParseScanBPData: p=%d str=%s", p, str.substr(p).c_str());
    else
      ProgTrace(TRACE5,"ParseScanBPData: p=%d",p);
    */
    ph=p;
    try
    {
      if (str[p]=='M')
      {
        //�������� �� 2D
        checkBCBP_M(str,p,p,i);
        if (i>p)
          ProgTrace(TRACE5,"ParseScanBPData: airline use=%s",str.substr(p,i-p).c_str());
        if (i-p!=10) throw EConvertError("09");
      };

      //�������� �� ����� 2 �� 5

      if (str_size<p+10) throw EConvertError("10");

      for(int j=0;j<10;j++)
        if (!IsDigit(str[p+j])) throw EConvertError("11");

      pax_id=ToInt(str.substr(p,10));
      Qry.SetVariable("pax_id",pax_id);
      Qry.Execute();
      if (!Qry.Eof)
      {
        if (init) return false;
        params.prefix=str.substr(0,ph);
        params.postfix=str.substr(p+10);
        params.code_id_len=0;
        init=true;
        ph=p+10;
        continue;
      };
    }
    catch(EConvertError &e)
    {
      /*
      if (p<str_size)
        ProgTrace(TRACE5,"ParseScanBPData: EConvertError %s: p=%d str=%s", e.what(), p, str.substr(p).c_str());
      else
        ProgTrace(TRACE5,"ParseScanBPData: EConvertError %s: p=%d", e.what(), p);
      */
    };
    ph++;
  }
  while (p!=string::npos);

  return init;
};

bool ParseScanDocData(const string& data, TScanParams& params)
{
  return false;
};

bool ParseScanCardData(const string& data, TScanParams& params)
{
  return false;
};


void MainDCSInterface::DetermineScanParams(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
 /*
   <DetermineScanParams>
     <operation type="SCAN_DOC">
       <fmt_params type="SCAN2">
         <encoding editable="0">CP1251</encoding>
         <postfix editable="1">031D</postfix>
         <prefix editable="1">1C02</prefix>
         <server_check editable="0">1</server_check>
       </fmt_params>
     </operation>
     <scan_data>
       <data>1C025643443C3C564...</data>
     </scan_data>
   </DetermineScanParams>
 */

  try
  {
    vector<TScanParams> ScanParams;
    TScanParams params;
    TDevOperType op_type=DecodeDevOperType(NodeAsString("operation/@type",reqNode));
    if (op_type!=dotScnBP1 &&
        op_type!=dotScnBP2 &&
        op_type!=dotScnDoc &&
        op_type!=dotScnCard) throw EConvertError("op_type=%s not supported",EncodeDevOperType(op_type).c_str());

    TDevFmtType fmt_type;
    if (TReqInfo::Instance()->desk.compatible(SCAN_DOC_VERSION))
      fmt_type=DecodeDevFmtType(NodeAsString("operation/fmt_params/@type",reqNode));
    else
      fmt_type=dftSCAN1;

    if (fmt_type!=dftSCAN1 &&
        fmt_type!=dftBCR &&
        fmt_type!=dftSCAN2 &&
        fmt_type!=dftSCAN3) throw EConvertError("fmt_type=%s not supported",EncodeDevFmtType(fmt_type).c_str());
    xmlNodePtr node;
    node=GetNode("operation/fmt_params/encoding",reqNode);
    if (node==NULL) throw EConvertError("Node 'encoding' not found");
    string encoding = NodeAsString(node);
    node=NodeAsNode("scan_data",reqNode);
    for(node=node->children;node!=NULL;node=node->next)
    {
      string data;
      if (!HexToString(NodeAsString(node),data)) throw EConvertError("HexToString error");
      ProgTrace(TRACE5,"DetermineScanParams: data.size()=%zu", data.size());
      data=ConvertCodepage(data,encoding,"CP866"); //������ �����頥� EConvertError
      ProgTrace(TRACE5,"DetermineScanParams: data=%s", data.c_str());

      if (op_type==dotScnBP1 || op_type==dotScnBP2)
      {
        if (ParseScanBPData(data,params))
          ScanParams.push_back(params);
      };
      if (op_type==dotScnDoc)
      {
        if (ParseScanDocData(data,params))
          ScanParams.push_back(params);
      };
      if (op_type==dotScnCard)
      {
        if (ParseScanCardData(data,params))
          ScanParams.push_back(params);
      };
    };
    if (ScanParams.empty()) throw EConvertError("ScanParams empty");
    for(vector<TScanParams>::iterator i=ScanParams.begin();i!=ScanParams.end();i++)
    {
      if (i==ScanParams.begin())
        params=*i;
      else
      {
        if (fmt_type==dftSCAN1 || fmt_type==dftBCR)
        {
          if (params.prefix.size()!=i->prefix.size() ||
              params.postfix!=i->postfix) throw EConvertError("Different prefix size or postfix");
          //�஡㥬 �뤥���� �����塞�� ���� ��䨪� - �� codeID
          string::iterator j1=params.prefix.begin();
          string::iterator j2=i->prefix.begin();
          int j=0;
          for(;j1!=params.prefix.end() && j2!=i->prefix.end();j1++,j2++,j++)
            if (*j1!=*j2) break;
          if (params.prefix.size()-j>params.code_id_len) params.code_id_len=params.prefix.size()-j;
        };
        if (fmt_type==dftSCAN2 || fmt_type==dftSCAN3)
        {
          if (params.prefix !=i->prefix ||
              params.postfix!=i->postfix) throw EConvertError("Different prefix or postfix");
        };
      };
    };
    if (fmt_type==dftSCAN1 || fmt_type==dftBCR)
    {
      //����塞 prefix � ��⮬ code_id_len
      if (params.code_id_len>9 ||
          params.code_id_len>params.prefix.size())
        throw EConvertError("Wrong params.code_id_len=%lu",params.code_id_len);
      params.prefix.erase(params.prefix.size()-params.code_id_len);
    };
    node=NewTextChild(resNode,"fmt_params");
    string data;
    StringToHex(ConvertCodepage(params.prefix,"CP866",encoding),data);
    NewTextChild(node,"prefix",data);
    StringToHex(ConvertCodepage(params.postfix,"CP866",encoding),data);
    NewTextChild(node,"postfix",data);
    if (fmt_type==dftSCAN1 || fmt_type==dftBCR)
    {
      NewTextChild(node,"code_id_len",(int)params.code_id_len);
    };
  }
  catch(EConvertError &e)
  {
    ProgTrace(TRACE0,"DetermineScanParams: %s", e.what());
    throw AstraLocale::UserException("MSG.DEVICE.UNABLE_AUTO_DETECT_PARAMS");
  };
};

void MainDCSInterface::SaveDeskTraces(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  const char* file_name="astradesk.log";
  ofstream f;
  f.open(file_name, ios_base::out | ios_base::app );
  if (!f.is_open()) throw Exception("Can't open file '%s'",file_name);
  try
  {
    string tracing_data;
    HexToString(NodeAsString("tracing_data",reqNode),tracing_data);
    f << tracing_data;
    f.close();
  }
  catch(...)
  {
    try { f.close(); } catch( ... ) { };
    throw;
  };
};

void MainDCSInterface::GetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  IntGetCertificates(ctxt, reqNode, resNode);
}

void MainDCSInterface::RequestCertificateData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    IntRequestCertificateData(ctxt, reqNode, resNode);
}

void MainDCSInterface::PutRequestCertificate(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  bool pr_grp = GetNode( "pr_grp", reqNode );
    string request = NodeAsString( "request_certificate", reqNode );
    IntPutRequestCertificate( request, TReqInfo::Instance()->desk.code, pr_grp, NoExists );
}

