#include <string>
#include <map>
#include <fstream>
#include "maindcs.h"
#include "basic.h"
#include "astra_elems.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "oralib.h"
#include "exceptions.h"
#include "misc.h"
#include <fstream>
#include "xml_unit.h"
#include "print.h"
#include "crypt.h"
#include "astra_locale.h"
#include "stl_utils.h"
#include "term_version.h"
#include "jxtlib/jxt_cont.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace ASTRA;
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
    //проверяем параметры
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
  
  //проверим notice_id на совпадение
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

    BASIC::TDateTime expire_date=Qry.FieldAsDateTime("expire_date");
    if (expire_date<=BASIC::NowUTC())
      throw AstraLocale::UserException("MSG.TERM_VERSION.NOT_SUPPORTED");
    double remainDays=expire_date-BASIC::NowUTC();
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
      //if (E.Code!=1) throw; надо бы так, но а вдруг мы удалили id из desk_notices?
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
      xmlNodePtr moduleNode = NewTextChild(modulesNode, "module");
      NewTextChild(moduleNode, "id", Qry.FieldAsInteger("id"));
      NewTextChild(moduleNode, "name", Qry.FieldAsString("name"));
      NewTextChild(moduleNode, "exe", Qry.FieldAsString("exe"));
    };
  }
  else AstraLocale::showErrorMessage("MSG.ALL_MODULES_DENIED_FOR_USER");
};

void GetDeviceAirlines(xmlNodePtr node)
{
  if (node==NULL) return;
  TReqInfo *reqInfo = TReqInfo::Instance();
//  if (reqInfo->desk.mode!=omCUTE && reqInfo->desk.mode!=omMUSE) return;
  xmlNodePtr accessNode=NewTextChild(node,"airlines");
  TAirlines &airlines=(TAirlines&)(base_tables.get("airlines"));
  if (reqInfo->user.access.airlines_permit) {
  	vector<string> airlines_params;
    SeparateString(JxtContext::getJxtContHandler()->sysContext()->read("session_airlines_params"),5,airlines_params);

    for(vector<string>::const_iterator i=reqInfo->user.access.airlines.begin();
                                       i!=reqInfo->user.access.airlines.end();i++)
    {
      try
      {
        ProgTrace(TRACE5, "airline=%s", i->c_str() );
        TAirlinesRow &row=(TAirlinesRow&)(airlines.get_row("code",*i));
        xmlNodePtr airlineNode=NewTextChild(accessNode,"airline");
        NewTextChild(airlineNode,"code",row.code);
        NewTextChild(airlineNode,"code_lat",row.code_lat,"");
        int aircode;
        if (BASIC::StrToInt(row.aircode.c_str(),aircode)!=EOF && row.aircode.size()==3)
          NewTextChild(airlineNode,"aircode",row.aircode,"");
        else
          NewTextChild(airlineNode,"aircode",954);
        for(vector<string>::const_iterator p=airlines_params.begin(); p!=airlines_params.end(); p++) {
          if ( p->find(row.code) != std::string::npos ||
               p->find(row.code_lat) != std::string::npos ) {
            NewTextChild(airlineNode,"run_params",*p);
            break;
          }
        }

      }
      catch(EBaseTableError) {}
    };
  }
};

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

  for ( xmlNodePtr pNode=paramsNode->children; pNode!=NULL && pNode->type == XML_ELEMENT_NODE; pNode=pNode->next ) { // пробег по параметрам
  	ProgTrace( TRACE5, "param name=%s", (const char*)pNode->name );
  	if ( pNode->children == NULL || pNode->children->type != XML_ELEMENT_NODE ) { //нет subparams
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
    for (xmlNodePtr subparamNode=pNode->children; subparamNode!=NULL && subparamNode->type == XML_ELEMENT_NODE; subparamNode=subparamNode->next) { // пробег по subparams
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
        !(first_desk_grp_id==ASTRA::NoExists && Qry.FieldIsNULL("desk_grp_id") ||
          first_desk_grp_id==Qry.FieldAsInteger("desk_grp_id"))) break;

    string event_name=Qry.FieldAsString("event_name");
    bool event_found=find(event_names.begin(),event_names.end(),event_name)!=event_names.end();

    ProgTrace(TRACE5,"event_name=%s event_found=%d",event_name.c_str(),(int)event_found);

    //проверим exclude
    if ( exclude &&  event_found ||
        !exclude && !event_found ) continue;

    CmdQry.SetVariable("event_name",event_name);

    XMLDoc cmdDoc("UTF-8","commands");
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
        //команда в шестнадцатиричном формате
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

void GetCUSEAddrs( map<TDevOperType, pair<string, string> > &opers, map<string, vector<string> > &addrs, xmlNodePtr reqNode )
{
  opers.clear();
  addrs.clear();
  xmlNodePtr n = GetNode("cuse_variables",reqNode);
  if ( n == NULL || n->children == NULL ) {
    // GetDeviceInfo - запрос из терминала
    vector<string> oper_vars;
    vector<string> dev_vars;
    SeparateString( JxtContext::getJxtContHandler()->sysContext()->read("cuse_device_variables"), 5, oper_vars );
    for ( vector<string>::iterator istr=oper_vars.begin(); istr!=oper_vars.end(); istr++ ) {
      ProgTrace( TRACE5, "vars=%s", istr->c_str() );
      SeparateString( *istr, '=', dev_vars );
      vector<string>::iterator idev_var = dev_vars.begin();
      if ( idev_var != dev_vars.end() && ++idev_var != dev_vars.end() ) {
        vector<string> devs;
        SeparateString( *idev_var, ',', devs );
        addrs[ *(dev_vars.begin()) ] = devs;
      }
    }
    return;
  }
  for ( int pass=0; pass<3; pass++ ) {
   xmlNodePtr node = n->children;
    while ( node != NULL ) {
      string addr = (char*)node->name;
      if ( addr == "ATB" && pass==0 ||
           addr == "BTP" && pass==0 ||
           addr == "BGR" && pass==0 ||
           addr == "DCP" && pass==0 ||
           addr == "SCN" && pass==1 ||
           addr == "OCR" && pass==1 ||
           addr == "SCL" && pass==1 ||
           addr == "MSR" && pass==1 ||
           addr == "WGE" && pass==2 ) {
        vector<string> dev_addrs;
        SeparateString((string)NodeAsString( node ), ',', dev_addrs);

        if ( dev_addrs.empty() ) {
          node = node->next;
          continue;
        }
        if ( pass == 0 ) {
          if ( addr == "ATB" ) {
            opers.insert( make_pair( dotPrnBP, make_pair("ATB CUSE", *dev_addrs.begin() ) ) );
            addrs[ "ATB CUSE" ] = dev_addrs;
          }
          if ( addr == "BTP" ) {
            opers.insert( make_pair( dotPrnBT, make_pair("BTP CUSE", *dev_addrs.begin()) ) );
            addrs[ "BTP CUSE" ] = dev_addrs;
          }
          if ( addr == "DCP" ) {
            opers.insert( make_pair( dotPrnFlt, make_pair("DCP CUSE", *dev_addrs.begin()) ) );
            opers.insert( make_pair( dotPrnArch, make_pair("DCP CUSE", *dev_addrs.begin()) ) );
            opers.insert( make_pair( dotPrnDisp, make_pair("DCP CUSE", *dev_addrs.begin()) ) );
            opers.insert( make_pair( dotPrnTlg, make_pair("DCP CUSE", *dev_addrs.begin()) ) );
            addrs[ "DCP CUSE" ] = dev_addrs;
          }
          if ( addr == "BGR" ) {
            opers.insert( make_pair( dotScnBP1,  make_pair("BCR CUSE", *dev_addrs.begin()) ) );
            vector<string>::const_iterator idev_addr = dev_addrs.begin();
            idev_addr++;
            if ( idev_addr != dev_addrs.end() ) {
              opers.insert( make_pair( dotScnBP2,  make_pair("BCR CUSE", *idev_addr) ) );
            }
            addrs[ "BCR CUSE" ] = dev_addrs;
          }
        }
        if ( pass == 1 ) {
          if ( addr == "SCN" )
          {
            if ( opers.find( dotScnBP1 ) == opers.end() )
              opers.insert( make_pair( dotScnBP1, make_pair("SCN CUSE", *dev_addrs.begin()) ) );
            addrs[ "SCN CUSE" ] = dev_addrs;
          }
          if ( addr == "OCR" ) {
            opers.insert( make_pair( dotScnDoc, make_pair("OCR CUSE", *dev_addrs.begin()) ) );
            addrs[ "OCR CUSE" ] = dev_addrs;
          }
          if ( addr == "MSR" ) {
            opers.insert( make_pair( dotScnCard, make_pair("MSR CUSE", *dev_addrs.begin()) ) );
            addrs[ "MSR CUSE" ] = dev_addrs;
          }
        }
        if ( pass == 2 ) {
          if ( addr == "WGE" ) {
            if ( opers.find( dotScnBP1 ) == opers.end() )
              opers.insert( make_pair( dotScnBP1, make_pair("WGE CUSE", *dev_addrs.begin()) ) );
            if ( opers.find( dotScnDoc ) == opers.end() )
              opers.insert( make_pair( dotScnDoc, make_pair("WGE CUSE", *dev_addrs.begin()) ) );
            if ( opers.find( dotScnCard ) == opers.end() )
              opers.insert( make_pair( dotScnCard, make_pair("WGE CUSE", *dev_addrs.begin()) ) );
             addrs[ "WGE CUSE" ] = dev_addrs;
          }
        }
      }
      node = node->next;
    }
  }
  string str_addrs;
  for ( map<string, vector<string> >::iterator ivar=addrs.begin(); ivar!=addrs.end(); ivar++ ) {
    if ( !str_addrs.empty() )
      str_addrs.append( string( 1, 5 ) );
    str_addrs += ivar->first + "=";
    for ( vector<string>::iterator iaddr=ivar->second.begin(); iaddr!=ivar->second.end(); iaddr++ ) {
      if ( iaddr != ivar->second.begin() )
        str_addrs += ",";
      str_addrs += *iaddr;
    }
  }
  ProgTrace( TRACE5, "write to context variables=%s", str_addrs.c_str() );
  JxtContext::getJxtContHandler()->sysContext()->write("cuse_device_variables",str_addrs);
}

void GetDevices( xmlNodePtr reqNode, xmlNodePtr resNode )
{
	/*Ограничение на передачу/прием параметров:
	  вложенность параметров (subparam_name) возможна только в случае спец. параметров, которые
	  разбираются на сервере спец. классами. Например param_name="timeouts" subrapam_name="print"
	  название параметров и подпараметров передаются и хранятся в нижнем регистре
	*/
	   /* Формат запроса вариантов типов параметров по операции+утсройству
	   <devices variants_operation="BP_PRINT" variant_dev_model="BTP CUTE" >*/
/* все параметры, приходящие с клиента - редактируемые */

    /* Формат передачи (запроса) данных по устройствам
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
1.Загружаем параметры с клиента. Определяем ключ dev_model+sess_type+fmt_type
2.Проверка на то, что такой ключ существует в таблице dev_model_sess_fmt
3.Если ключ не найден - загрузка параметров по умолчанию
4.Если ключ найден - загружаем параметры по умолчанию с сервера и на них накладываем параметры с клиента
  параметр с клиента может быть наложен если:
  -editable=1
  -название параметра/подпараметра должен быть описан в таблице dev_model_params с заданными dev_model+sess_type+fmt_type+(grp_id,NULL)
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
    GetDeviceAirlines(resNode);

   if (devNode==NULL) return; // если в запросе нет этoго параметра, то не нужно собирать данные по устройствам
  //string VariantsOperation = NodeAsString( "@variants_operation", devNode, "" );

  TReqInfo *reqInfo = TReqInfo::Instance();

  bool pr_editable = ( find( reqInfo->user.access.rights.begin(),
                             reqInfo->user.access.rights.end(), 840 ) != reqInfo->user.access.rights.end() );

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
  // разбираем параметры пришедшие с клиента
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
  for( ;!DefQry.Eof;DefQry.Next() ) { // цикл по типам операций или цикл по возможным вариантам настроек заданной операции
    DevModelDefaults.push_back( TDevModelDefaults( DefQry.FieldAsString( "op_type" ),
                                                   DefQry.FieldAsString( "dev_model" ),
                                                   DefQry.FieldAsString( "sess_type" ),
                                                   DefQry.FieldAsString( "fmt_type" ) ) );
  }
  
  map<TDevOperType, pair<string, string> > opers; //операция, dev_model, addr
  map<string, vector<string> > valid_addrs;
  if ( reqInfo->desk.mode==omRESA ||
       reqInfo->desk.mode==omCUSE)
  {
    if ( reqInfo->desk.mode==omRESA )
    {
      const char* equip_param_name="EQUIPMENT";
      string param_name, param_value;

      //ищем переменную EQUIPMENT
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
      if (param_name!=equip_param_name) param_value="ATB0,BTP0,BGR0,BGR1,DCP0";

      ProgTrace( TRACE5, "%s=%s", equip_param_name, param_value.c_str()  );

      vector<string> addrs;
      SeparateString( param_value, ',', addrs );
      for ( vector<string>::iterator addr=addrs.begin(); addr!=addrs.end(); addr++ ) {
        TrimString( *addr );
        //определим к какой операции относится адрес
        if (addr->size()!=4) continue;
        if (!IsDigit((*addr)[3])) continue;
        string devName=addr->substr(0,3);
        ProgTrace( TRACE5, "addr=%s devName=%s", addr->c_str(), devName.c_str() );

        if (devName=="BPP" || devName=="ATB")
        {
           if (opers[dotPrnBP].second.empty()) opers[dotPrnBP]=make_pair("ATB RESA",*addr);
           continue;
        };
        if (devName=="BTP")
        {
           if (opers[dotPrnBT].second.empty()) opers[dotPrnBT]=make_pair("BTP RESA",*addr);
           continue;
        };
        if (devName=="BCD" || devName=="BGR" || devName=="RTE")
        {
          if (opers[dotScnBP1].second==*addr || opers[dotScnBP2].second==*addr) continue;

          if (opers[dotScnBP1].second.empty()) opers[dotScnBP1]=make_pair(devName=="RTE"?"SCN RESA":"BCR RESA",*addr);
          else
            if (opers[dotScnBP2].second.empty()) opers[dotScnBP2]=make_pair(devName=="RTE"?"SCN RESA":"BCR RESA",*addr);
          continue;
        };
        if (devName=="DCP" || devName=="MSG")
        {
          if (opers[dotPrnFlt].second.empty()) opers[dotPrnFlt]=make_pair("DCP RESA",*addr);
          if (opers[dotPrnArch].second.empty()) opers[dotPrnArch]=make_pair("DCP RESA",*addr);
          if (opers[dotPrnDisp].second.empty()) opers[dotPrnDisp]=make_pair("DCP RESA",*addr);
          if (opers[dotPrnTlg].second.empty()) opers[dotPrnTlg]=make_pair("DCP RESA",*addr);
          continue;
        };
      };
    };
    if ( reqInfo->desk.mode==omCUSE )
    {
      GetCUSEAddrs(opers, valid_addrs, reqNode);
    };
    
    if (variant_model.empty())
    {
      //не идет перевыбор dev_model через "Настройки оборудования" в терминале
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
        if (!opers[oper].second.empty())
        {
          DefQry.SetVariable( "op_type", def->op_type );
          DefQry.SetVariable( "dev_model", opers[oper].first );
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
    // цикл по типам операций или цикл по возможным вариантам настроек заданной операции
    if ( variant_model.empty() && operation == def->op_type ) continue;
    operation = def->op_type;

    for ( operNode=GetNode( "operation", devNode ); operNode!=NULL; operNode=operNode->next ) // пробег по операциям клиента
    	if ( operation == NodeAsString( "@type", operNode ) )
    		break;
    ProgTrace( TRACE5, "operation=%s, is client=%d", operation.c_str(), (int)operNode );

    dev_model.clear();
    sess_type.clear();
    fmt_type.clear();

    if ( operNode != NULL ) { // данные с клиента
    	// имеем ключ dev_model+sess_type+fmt_type. Возможно 2 варианта:
    	// 1. Начальная инициализация
    	// 2. Различные варианты работы устройства, клиентские параметры надо разбирать когда ключ совпал
      client_dev_model = NodeAsString( "dev_model_code", operNode, "" );
      dev_model = client_dev_model;
      client_sess_type = NodeAsString( "sess_params/@type", operNode, "" );
      sess_type = client_sess_type;
      client_fmt_type = NodeAsString( "fmt_params/@type", operNode, "" );
      fmt_type = client_fmt_type;
    }
    //ProgTrace( TRACE5, "variant_model=%s", variant_model.c_str() );
    for ( int k=!variant_model.empty(); k<=1; k++ ) { // два прохода: 0-параметры с клиента, 1 - c сервера
    	//ProgTrace( TRACE5, "k=%d", k );

    	if ( k == 1 ) {
        dev_model = def->dev_model;
        sess_type = def->sess_type;
        fmt_type =  def->fmt_type;
    	}
    	if ( dev_model.empty() && sess_type.empty() && fmt_type.empty() ) continue;
    	bool pr_parse_client_params = ( client_dev_model == dev_model && client_sess_type == sess_type && client_fmt_type == fmt_type );
    	
    	if ( pr_parse_client_params ) {
    	  if (reqInfo->desk.mode==omCUSE)
        {
          //проверить что адрес валидный пришедший с клиента
          pr_parse_client_params = false;
          if (operNode!=NULL)
          {
            string addr = NodeAsString( "sess_params/addr", operNode, "" );
            if ( !addr.empty() )  {
              vector<string> &dev_model_addrs=valid_addrs[client_dev_model];
              pr_parse_client_params = ( find(dev_model_addrs.begin(),dev_model_addrs.end(),addr) != dev_model_addrs.end() );
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
      if ( Qry.Eof ) continue; // данный ключ dev_model+sess_type+fmt_type не разрешен

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
           reqInfo->desk.mode==omCUSE)
      {
        TDevOperType oper=DecodeDevOperType(operation);
        if (!opers[oper].first.empty() && opers[oper].first==dev_model)
        {
          for(TCategoryDevParams::iterator p=params.begin();p!=params.end();++p)
            if (p->param_name=="addr") p->param_value=opers[oper].second;
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

      //ищем параметр posted_events
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

void NormalizeAirlines( vector<string> &airlines,
                        map<string,string> &air_params,
                        string &str, string &airline_params )
{
  if (airlines.empty())
    return;
  sort(airlines.begin(),airlines.end());
  string prior_airline;
  //удалим одинаковые компании
  for(vector<string>::iterator i=airlines.begin();i!=airlines.end();i++)
  {
    if (*i!=prior_airline)
    {
      if (!str.empty()) str.append("/");
      str.append(*i);
      if (!airline_params.empty()) airline_params.append( string(1,5) );
      airline_params.append(air_params[*i]);
      prior_airline=*i;
    };
  };
  ProgTrace( TRACE5, "airline_params=%s", airline_params.c_str() );
}

bool MainDCSInterface::GetSessionAirlines(xmlNodePtr reqNode, string &str, std::string &airline_params)
{
  str.clear();
  airline_params.clear();
  xmlNodePtr node;
  string value_airline;
  vector<string> airlines;
  map<string,string> air_params;
  
  node = GetNode("airlines", reqNode);
  if (node==NULL) return true;
  xmlNodePtr run_paramNode;
  string run_param_airline;
  for(node=node->children;node!=NULL;node=node->next)
  {
  	value_airline = NodeAsString(node);
  	run_paramNode = GetNode( "@run_params", node );
  	if ( run_paramNode )
  		run_param_airline = NodeAsString( run_paramNode );
  	else
  		run_param_airline.clear();
  	ProgTrace( TRACE5, "value_airline=%s, run_param_airline=%s", value_airline.c_str(), run_param_airline.c_str() );
  	size_t p = run_param_airline.find( " " );
  	if ( p != string::npos ) {
  		run_param_airline.erase( p );
  	  ProgTrace( TRACE5, "run_param_airline=|%s|", run_param_airline.c_str() );
  	}
  	if ( !run_param_airline.empty() )
  		value_airline = run_param_airline;
    try
    {
      airlines.push_back(base_tables.get("airlines").get_row("code/code_lat",value_airline).AsString("code"));
    }
    catch(EBaseTableError)
    {
    	try {
    		airlines.push_back(base_tables.get("airlines").get_row("aircode",value_airline).AsString("code"));
    	}
    	catch(EBaseTableError) {
        str=value_airline;
        return false;
      }
    };
       xmlNodePtr paramNode = GetNode( "@run_params", node );
       if (paramNode) {
         air_params[ airlines.back() ] = NodeAsString(paramNode);
       }

  };
  NormalizeAirlines( airlines, air_params, str, airline_params );
  return true;
};

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
    if (Qry.FieldAsString("passwd")==(string)Qry.FieldAsString("login") )
      AstraLocale::showErrorMessage("MSG.USER_NEED_TO_CHANGE_PASSWD");

    Qry.Clear();
    Qry.SQLText =
      "BEGIN "
      "  UPDATE users2 SET desk = NULL WHERE desk = :desk; "
      "  UPDATE users2 SET desk = :desk WHERE user_id = :user_id; "
      "  UPDATE desks "
      "  SET version = :version, last_logon = system.UTCSYSDATE, term_id=:term_id "
      "  WHERE code = :desk; "
      "END;";
    Qry.CreateVariable("user_id", otInteger, reqInfo->user.user_id);
    Qry.CreateVariable("desk", otString, reqInfo->desk.code);
    Qry.CreateVariable("version", otString, NodeAsString("term_version", reqNode));
    xmlNodePtr propNode;
    if ((propNode = GetNode("/term/query/@term_id",ctxt->reqDoc))!=NULL)
      Qry.CreateVariable("term_id", otFloat, NodeAsFloat(propNode));
    else
      Qry.CreateVariable("term_id", otFloat, FNull);
    Qry.Execute();

    string airlines;
    string airlines_params;
    if (!GetSessionAirlines(reqNode,airlines,airlines_params))
      throw AstraLocale::UserException("MSG.AIRLINE_CODE_NOT_FOUND", LParams() << LParam("airline", airlines));
    JxtContext::getJxtContHandler()->sysContext()->write("session_airlines",airlines);
    JxtContext::getJxtContHandler()->sysContext()->write("session_airlines_params",airlines_params);
    xmlNodePtr node=NodeAsNode("/term/query",ctxt->reqDoc);
    
    TReqInfoInitData reqInfoData;
    reqInfoData.screen = NodeAsString("@screen", node);
    reqInfoData.pult = ctxt->pult;
    reqInfoData.opr = NodeAsString("@opr", node);
  	reqInfoData.lang = reqInfo->desk.lang; //!определение языка вынесено в astracallbacks.cc::UserBefore т.к. там задается контекст xmlRC->setLang(RUSSIAN) и не требуется делать повторно эту долгую операцию
    if ((propNode = GetNode("@mode", node))!=NULL)
      reqInfoData.mode = NodeAsString(propNode);
    if ((propNode = GetNode("@term_id", node))!=NULL)
      reqInfoData.term_id = NodeAsFloat(propNode);
    reqInfoData.checkUserLogon = true; // имеет смысл т.к. надо начитать инфу по пользователю
    reqInfoData.checkCrypt = false; // не имеет смысла делать повторную проверку
    reqInfo->Initialize( reqInfoData );

    //здесь reqInfo нормально инициализирован
    CheckTermExpireDate();
    
    GetModuleList(resNode);
    GetDevices(reqNode,resNode);
    GetNotices(resNode);

    if ( GetNode("lang",reqNode) ) { //!!!необходимо, чтобы был словарь для языка по умолчанию - здесь нет этой проверки!!!
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
    JxtContext::getJxtContHandler()->sysContext()->remove("session_airlines");
    JxtContext::getJxtContHandler()->sysContext()->remove("session_airlines_params");
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
      "UPDATE users2 SET passwd = :passwd WHERE user_id = :user_id";
    Qry.CreateVariable("user_id", otInteger, reqInfo->user.user_id);
    Qry.CreateVariable("passwd", otString, NodeAsString("passwd", reqNode));
    Qry.Execute();
    if(Qry.RowsProcessed() == 0)
        throw Exception("user not found (user_id=%d)",reqInfo->user.user_id);
    TReqInfo::Instance()->MsgToLog("Изменен пароль пользователя", evtAccess);
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
	//
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
  int pax_id,len_u,len_r;
  bool init=false;
  do
  {
    p=str.find_first_of("0123456789M",ph);
    if (p==string::npos) break;
    if (str_size<p+10) break;

  /*  if (p<str_size)
      ProgTrace(TRACE5,"ParseScanBPData: p=%d str=%s", p, str.substr(p).c_str());
    else
      ProgTrace(TRACE5,"ParseScanBPData: p=%d",p);*/

    ph=p;
    try
    {
      if (str[p]=='M')
      {
        //возможно это 2D
        if (str_size<p+60) throw EConvertError("01");
        if (!HexToString(str.substr(p+58,2),c) || c.size()<1) throw EConvertError("02");
        i=(int)c[0]; //item number=6
        p+=60;
        if (str_size<p+i) throw EConvertError("03");
        i+=p; //это должен быть конец pax_id
        if (str_size<p+4) throw EConvertError("04");
        if (!HexToString(str.substr(p+2,2),c) || c.size()<1) throw EConvertError("05");
        len_u=(int)c[0]; //item number=10
        //ProgTrace(TRACE5,"ParseScanBPData: len_u=%d",len_u);
        p+=4;
        if (str_size<p+len_u+2) throw EConvertError("06");
        if (!HexToString(str.substr(p+len_u,2),c) || c.size()<1) throw EConvertError("07");
        len_r=(int)c[0]; //item number=17
        //ProgTrace(TRACE5,"ParseScanBPData: len_r=%d",len_r);
        p+=len_u+2;
        if (str_size<p+len_r) throw EConvertError("08");
        p+=len_r;
        if (i>p)
          ProgTrace(TRACE5,"ParseScanBPData: airline use=%s",str.substr(p,i-p).c_str());
        if (i-p!=10) throw EConvertError("09");
      };

      //возможно это обычный 2 из 5

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
      /*if (p<str_size)
        ProgTrace(TRACE5,"ParseScanBPData: EConvertError %s: p=%d str=%s", e.what(), p, str.substr(p).c_str());
      else
        ProgTrace(TRACE5,"ParseScanBPData: EConvertError %s: p=%d", e.what(), p);*/
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
      ProgTrace(TRACE5,"DetermineScanParams: data.size()=%d", data.size());
      data=ConvertCodepage(data,encoding,"CP866"); //иногда возвращает EConvertError
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
          //пробуем выделить изменяемую часть префикса - это codeID
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
      //вычисляем prefix с учетом code_id_len
      if (params.code_id_len<0 ||
          params.code_id_len>9 ||
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

