#include <string>
#include <map>
#include <fstream>
#include "maindcs.h"
#include "basic.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "oralib.h"
#include "exceptions.h"
#include "misc.h"
#include <fstream>
#include "xml_unit.h"
#include "print.h"
#include "jxtlib/jxt_cont.h"
#include "crypt.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace ASTRA;
using namespace EXCEPTIONS;
using namespace std;
using namespace JxtContext;

enum TDevParamCategory{dpcSession,dpcFormat,dpcModel};

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
    else showErrorMessage("Пользователю закрыт доступ ко всем модулям");
};

void GetDeviceAirlines(xmlNodePtr node)
{
  if (node==NULL) return;
  TReqInfo *reqInfo = TReqInfo::Instance();
  if (reqInfo->desk.mode!=omCUTE && reqInfo->desk.mode!=omMUSE) return;
  xmlNodePtr accessNode=NewTextChild(node,"airlines");
  TAirlines &airlines=(TAirlines&)(base_tables.get("airlines"));
  if (reqInfo->user.access.airlines_permit)
    for(vector<string>::const_iterator i=reqInfo->user.access.airlines.begin();
                                       i!=reqInfo->user.access.airlines.end();i++)
    {
      try
      {
        TAirlinesRow &row=(TAirlinesRow&)(airlines.get_row("code",*i));
        xmlNodePtr airlineNode=NewTextChild(accessNode,"airline");
        NewTextChild(airlineNode,"code",row.code);
        NewTextChild(airlineNode,"code_lat",row.code_lat,"");
        int aircode;
        if (BASIC::StrToInt(row.aircode.c_str(),aircode)!=EOF && row.aircode.size()==3)
          NewTextChild(airlineNode,"aircode",row.aircode,"");
      }
      catch(EBaseTableError) {}
    };
};

struct TDevParam {
  string param_name;
  string subparam_name;
  string param_value;
  int editable;
  TDevParam::TDevParam() {
  	editable = 0;
  }
  TDevParam::TDevParam( string aparam_name,
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
  	ProgTrace( TRACE5, "param_name=%s, subparam_name=%s, param_value=%s, editable=%d, pr_editable=%d",
  	           iparam->param_name.c_str(), iparam->subparam_name.c_str(), iparam->param_value.c_str(), iparam->editable, pr_editable );
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
  if (reqNode==NULL || resNode==NULL) return;
  resNode=NewTextChild(resNode,"devices");

  reqNode=GetNode("devices",reqNode);
  string variant_model = NodeAsString( "@variant_model", reqNode, "" );
  bool pr_default_sets = GetNode( "@operation_default_sets", reqNode );

  ProgTrace( TRACE5, "variants mode=%s, pr_default_sets=%d", variant_model.c_str(), pr_default_sets );
  if ( variant_model.empty() && !pr_default_sets )
    GetDeviceAirlines(resNode);

   if (reqNode==NULL) return; // если в запросе нет этoго параметра, то не нужно собирать данные по устройствам
  //string VariantsOperation = NodeAsString( "@variants_operation", reqNode, "" );

  TReqInfo *reqInfo = TReqInfo::Instance();

  bool pr_editable = ( find( reqInfo->user.access.rights.begin(),
                             reqInfo->user.access.rights.end(), 840 ) != reqInfo->user.access.rights.end() );

  TQuery ModelQry(&OraSession);
  ModelQry.Clear();
  ModelQry.SQLText="SELECT name FROM dev_models WHERE code=:dev_model";
  ModelQry.DeclareVariable("dev_model",otString);

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
  TQuery DefQry(&OraSession);
  if ( variant_model.empty() ) {
    string sql =
      "SELECT dev_oper_types.code AS op_type, "
      "       dev_model_defaults.dev_model, "
      "       dev_model_defaults.sess_type, "
      "       dev_model_defaults.fmt_type, "
      "       '' sess_name, '' fmt_name "
      "FROM dev_oper_types,dev_model_defaults "
      "WHERE dev_oper_types.code=dev_model_defaults.op_type(+) AND "
      "      dev_model_defaults.term_mode(+)=:term_mode ";
    if ( pr_default_sets ) {
    	sql += " AND dev_oper_types.code=:op_type";
    }
    DefQry.SQLText=sql;
  }
  else {
  	DefQry.SQLText=
      "SELECT DISTINCT dev_fmt_opers.op_type AS op_type, "
      "                dev_model_sess_fmt.dev_model,"
      "                dev_model_sess_fmt.sess_type,"
      "                dev_model_sess_fmt.fmt_type,"
      "                dev_sess_types.name sess_name,"
      "                dev_fmt_types.name fmt_name,"
      "                DECODE(dev_model_defaults.op_type,NULL,0,1) pr_default "
      " FROM dev_model_sess_fmt, dev_sess_modes, dev_fmt_opers, dev_sess_types, dev_fmt_types, dev_model_defaults "
      "WHERE dev_sess_modes.term_mode=:term_mode AND "
      "      dev_sess_modes.sess_type=dev_model_sess_fmt.sess_type AND "
      "      dev_sess_types.code=dev_model_sess_fmt.sess_type AND "
      "      dev_fmt_opers.op_type=:op_type AND "
      "      dev_fmt_opers.fmt_type=dev_model_sess_fmt.fmt_type AND "
      "      dev_fmt_types.code=dev_model_sess_fmt.fmt_type AND "
      "      dev_model_sess_fmt.dev_model=:dev_model AND "
      "      dev_model_defaults.op_type(+)=:op_type AND "
      "      dev_model_defaults.term_mode(+)=:term_mode AND "
      "      dev_model_sess_fmt.dev_model=dev_model_defaults.dev_model(+) AND "
      "      dev_model_sess_fmt.sess_type=dev_model_defaults.sess_type(+) AND "
      "      dev_model_sess_fmt.fmt_type=dev_model_defaults.fmt_type(+) ";
    DefQry.CreateVariable( "dev_model", otString, variant_model );
    tst();
  }
  DefQry.CreateVariable("term_mode",otString,EncodeOperMode(reqInfo->desk.mode));
  if ( !variant_model.empty() || pr_default_sets )
    DefQry.CreateVariable( "op_type", otString, NodeAsString( "operation/@type", reqNode ) );
  DefQry.Execute();

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
    "      dev_fmt_opers.fmt_type=dev_model_sess_fmt.fmt_type ";
  Qry.DeclareVariable( "dev_model", otString );
  Qry.DeclareVariable( "sess_type", otString );
  Qry.DeclareVariable( "fmt_type", otString );
  Qry.DeclareVariable( "op_type", otString );
  Qry.CreateVariable( "term_mode", otString, EncodeOperMode(reqInfo->desk.mode) );
  string operation;
  xmlNodePtr operNode;


  for( ;!DefQry.Eof;DefQry.Next() ) { // цикл по типам операций или цикл по возможным вариантам настроек заданной операции
    if ( variant_model.empty() && operation == DefQry.FieldAsString( "op_type" ) ) continue;
    operation = DefQry.FieldAsString( "op_type" );
    ProgTrace( TRACE5, "operation=%s", operation.c_str() );

    for ( operNode=GetNode( "operation", reqNode ); operNode!=NULL; operNode=operNode->next ) // пробег по операциям клиента
    	if ( operation == NodeAsString( "@type", operNode ) )
    		break;
    ProgTrace( TRACE5, "operation type=%s, is client=%d", operation.c_str(), (int)operNode );

    dev_model.clear();
    sess_type.clear();
    fmt_type.clear();

    if ( operNode != NULL ) { // данные с клиента
    	// имеем ключ dev_model+sess_type+fmt_type. Возможно 2 варианта:
    	// 1. Начальная инициализация
    	// 2. Различные варианты работы устройства, слиентские параметры надо разбирать когда ключ совпал
      client_dev_model = NodeAsString( "dev_model_code", operNode, "" );
      dev_model = client_dev_model;
      client_sess_type = NodeAsString( "sess_params/@type", operNode, "" );
      sess_type = client_sess_type;
      client_fmt_type = NodeAsString( "fmt_params/@type", operNode, "" );
      fmt_type = client_fmt_type;
    }
    //ProgTrace( TRACE5, "VariantsOperation=%s, !VariantsOperation.empty()=%d", VariantsOperation.c_str(), (int)!VariantsOperation.empty() );
    for ( int k=!variant_model.empty(); k<=1; k++ ) { // два прохода: 0-параметры с клиента, 1 - c сервера
    	ProgTrace( TRACE5, "k=%d", k );
    	if ( k == 1 ) {
        dev_model = DefQry.FieldAsString( "dev_model" );
        sess_type = DefQry.FieldAsString( "sess_type" );
        fmt_type = DefQry.FieldAsString( "fmt_type" );
    	}
    	if ( dev_model.empty() && sess_type.empty() && fmt_type.empty() ) continue;
    	bool pr_parse_client_params = ( client_dev_model == dev_model && client_sess_type == sess_type && client_fmt_type == fmt_type );

      Qry.SetVariable( "dev_model", dev_model );
      Qry.SetVariable( "sess_type", sess_type );
      Qry.SetVariable( "fmt_type", fmt_type );
      Qry.SetVariable( "op_type", operation );
      Qry.Execute();
      ProgTrace( TRACE5, "dev_model=%s, sess_type=%s, fmt_type=%s", dev_model.c_str(), sess_type.c_str(), fmt_type.c_str() );
      if ( Qry.Eof ) continue; // данный ключ dev_model+sess_type+fmt_type не разрешен
      tst();
      ModelQry.SetVariable( "dev_model", dev_model );
      ModelQry.Execute();
      if ( ModelQry.Eof ) continue; //       	модель не найдена
      	tst();
      xmlNodePtr newoperNode=NewTextChild( resNode, "operation" );
      xmlNodePtr pNode;
      SetProp( newoperNode, "type", operation );
      if ( !DefQry.FieldIsNULL( "sess_name" ) && !DefQry.FieldIsNULL( "fmt_name" ) ) {
      	SetProp( newoperNode, "variant_name", string( string(DefQry.FieldAsString( "sess_name" )) + "/" + DefQry.FieldAsString( "fmt_name" ) ).c_str() );
      }
      pNode = NewTextChild( newoperNode, "dev_model_code", dev_model );
      SetProp( pNode, "dev_model_name", ModelQry.FieldAsString( "name" ) );
      if (reqInfo->desk.version.empty() ||
          reqInfo->desk.version==UNKNOWN_VERSION)
        NewTextChild( newoperNode, "dev_model_name", ModelQry.FieldAsString( "name" ) );

      SessParamsQry.SetVariable("dev_model",dev_model);
      SessParamsQry.SetVariable("sess_type",sess_type);
      SessParamsQry.SetVariable("fmt_type",fmt_type);
      SessParamsQry.Execute();
      GetParams( SessParamsQry, params );
      if ( pr_parse_client_params )
        ParseParams( GetNode( "sess_params", operNode ), params );
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
      ProgTrace( TRACE5, "fmt_params count=%d", params.size() );
      pNode = NewTextChild( newoperNode, "fmt_params" );
      SetProp( pNode, "type", fmt_type );
      BuildParams( pNode, params, pr_editable );

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

bool MainDCSInterface::GetSessionAirlines(xmlNodePtr node, string &str)
{
  str.clear();
  if (node==NULL) return true;
  vector<string> airlines;
  for(node=node->children;node!=NULL;node=node->next)
  {
    try
    {
      airlines.push_back(base_tables.get("airlines").get_row("code/code_lat",NodeAsString(node)).AsString("code"));
    }
    catch(EBaseTableError)
    {
    	try {
    		airlines.push_back(base_tables.get("airlines").get_row("aircode",NodeAsString(node)).AsString("code"));
    	}
    	catch(EBaseTableError) {
        str=NodeAsString(node);
        return false;
      }
    };
  };
  if (airlines.empty()) return true;
  sort(airlines.begin(),airlines.end());
  string prior_airline;
  //удалим одинаковые компании
  for(vector<string>::iterator i=airlines.begin();i!=airlines.end();i++)
  {
    if (*i!=prior_airline)
    {
      if (!str.empty()) str.append("/");
      str.append(*i);
      prior_airline=*i;
    };
  };
  return true;
};

void MainDCSInterface::CheckUserLogon(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqinfo = TReqInfo::Instance();
    try
    {
      throw 0; //никаких автологонов!
      if(reqinfo->user.login.empty()) throw 0;

      string airlines;
      if (!GetSessionAirlines(GetNode("airlines", reqNode),airlines)) throw 0;

      // проверим session_airlines
      if (getJxtContHandler()->sysContext()->read("session_airlines")!=airlines)
        throw 0;

      GetModuleList(resNode);
    }
    catch(int)
    {
      reqinfo->user.clear();
      reqinfo->desk.clear();
      showBasicInfo();
    };
}

struct TOldPrnParams
{
  int code;
  string name;
  string iface;
//    format_id: TPectabFmt;
  string format;
  bool pr_stock;
};
struct TNewPrnParams
{
  string code;
  string name;
};
struct TDocTypeConvert
{
  TDocType oldDoc;
  string newDoc;
};

TDocTypeConvert docTypes[7] = { {dtBP,     "PRINT_BP"   },
                                {dtBT,     "PRINT_BT"   },
                                {dtReceipt,"PRINT_BR"   },
                                {dtFltDoc, "PRINT_FLT"  },
                                {dtArchive,"PRINT_ARCH" },
                                {dtDisp,   "PRINT_DISP" },
                                {dtTlg,    "PRINT_TLG"  } };


void ConvertDevOldFormat(xmlNodePtr reqNode, xmlNodePtr resNode)
{
  XMLRequestCtxt *ctxt = getXmlCtxt();
  TReqInfo *reqInfo = TReqInfo::Instance();

  xmlNodePtr node,node2;
  xmlNodePtr oldParamsNode=GetNode("old_profile",reqNode);
  if (oldParamsNode==NULL) return;

  try
  {
    if (reqInfo->desk.mode==omSTAND)
    {
      xmlNodePtr prnNode=GetNode("Printers/Printers",oldParamsNode);
      if (prnNode!=NULL) prnNode=prnNode->children; //спустились на уровень <printer>
      while (prnNode!=NULL)
      {
        node2=prnNode->children;
        if ((GetNodeFast("type",node2)==NULL) ||
            (GetNodeFast("name",node2)==NULL) ||
            (GetNodeFast("port",node2)==NULL)) throw EConvertError("type, name, port not found");

        string devOper=NodeAsStringFast("type",node2);
        string devName=NodeAsStringFast("name",node2);
        string devPort=NodeAsStringFast("port",node2);

        TOldPrnParams oldPrn;
        TNewPrnParams newPrn;
        string dev_model, dev_sess_type, dev_fmt_type;

        //находим операцию
        int docIdx=0;
        for(;docIdx<=6;docIdx++)
          if (devOper==EncodeDocType(docTypes[docIdx].oldDoc)) break;
        if (docIdx>6)
          throw EConvertError("operation %s not found",devOper.c_str());

        {
          XMLDoc reqDoc,resDoc;
          reqDoc.docPtr=CreateXMLDoc("UTF-8","request");
          node=NodeAsNode("/request",reqDoc.docPtr);
          NewTextChild(node, "doc_type", devOper);


          resDoc.docPtr=CreateXMLDoc("UTF-8","response");
          if (reqDoc.docPtr==NULL || resDoc.docPtr==NULL)
            throw EConvertError("can't create reqDoc or resDoc");
          JxtInterfaceMng::Instance()->
            GetInterface("print")->
              OnEvent("GetPrinterList",  ctxt,
                                         NodeAsNode("/request",reqDoc.docPtr),
                                         NodeAsNode("/response",resDoc.docPtr));

          node = NodeAsNode("/response/printers/*[1]", resDoc.docPtr);




          if (strcmp((char*)node->name,"drv")==0)
          {
            if (devPort.empty())
            {
              //WINDOWS-принтер
              dev_model="DRV PRINT";
              dev_sess_type="WDP";
              dev_fmt_type="FRX";
            }
            else
            {
              //прямой вывод в локальный порт
              switch (docTypes[docIdx].oldDoc)
              {
                case dtReceipt:
                  dev_model="DIR PRINT";
                  dev_sess_type="LPT";
                  dev_fmt_type="EPSON";
                  break;
                case dtFltDoc:
                case dtArchive:
                case dtDisp:
                case dtTlg:
                  dev_model="DIR PRINT";
                  dev_sess_type="LPT";
                  dev_fmt_type="TEXT";
                  break;
                default:
                  throw EConvertError("wrong printer %s",devName.c_str());
              };
            };
          }
          else
          {
            while (node != NULL)
            {
              oldPrn.code = NodeAsInteger("code", node);
              oldPrn.name = NodeAsString("name", node);
              oldPrn.iface = NodeAsString("iface", node);
             // oldPrn.format_id = TPectabFmt(NodeAsInteger("format_id", node));
              oldPrn.format = NodeAsString("format", node);
              oldPrn.pr_stock = NodeAsInteger("pr_stock", node) != 0;

              ProgTrace(TRACE5,"ConvertDevOldFormat: %s %s %s",
                                          oldPrn.name.c_str(),
                                          oldPrn.format.c_str(),
                                          oldPrn.iface.c_str());

              if ((oldPrn.name  + " (" + oldPrn.format + " " + oldPrn.iface + ")") == devName) break;

              node = node->next;
            };
            if (node==NULL) //не нашли соответствие name
              throw EConvertError("printer %s not found in GetPrinterList",devName.c_str());

            //находим модель
            TQuery Qry(&OraSession);
            Qry.Clear();
            Qry.SQLText=
              "SELECT new_code FROM convert_old_prn WHERE old_code=:code";
            Qry.CreateVariable("code",otInteger,oldPrn.code);
            Qry.Execute();
            if (Qry.Eof || Qry.FieldIsNULL("new_code"))
              throw EConvertError("printer code %d not found",oldPrn.code);

            dev_model=Qry.FieldAsString("new_code");

            Qry.Clear();
            Qry.SQLText=
              "SELECT new_iface FROM convert_old_iface WHERE old_iface=:iface";
            Qry.CreateVariable("iface",otString,oldPrn.iface);
            Qry.Execute();
            if (Qry.Eof || Qry.FieldIsNULL("new_iface"))
              throw EConvertError("iface code %d not found",oldPrn.iface.c_str());

            dev_sess_type=Qry.FieldAsString("new_iface");

            Qry.Clear();
            Qry.SQLText=
              "SELECT new_fmt FROM convert_old_fmt WHERE old_fmt=:fmt";
            Qry.CreateVariable("fmt",otString,oldPrn.format);
            Qry.Execute();
            if (Qry.Eof || Qry.FieldIsNULL("new_fmt"))
              throw EConvertError("format code %d not found",oldPrn.format.c_str());

            dev_fmt_type=Qry.FieldAsString("new_fmt");

          };
        };


        {
          XMLDoc reqDoc,resDoc;
          reqDoc.docPtr=CreateXMLDoc("UTF-8","request");
          node=NodeAsNode("/request",reqDoc.docPtr);
          NewTextChild(node,"operation",docTypes[docIdx].newDoc);


          resDoc.docPtr=CreateXMLDoc("UTF-8","response");
          if (reqDoc.docPtr==NULL || resDoc.docPtr==NULL)
            throw EConvertError("can't create reqDoc or resDoc");
          JxtInterfaceMng::Instance()->
            GetInterface("MainDCS")->
              OnEvent("GetDeviceList",   ctxt,
                                         NodeAsNode("/request",reqDoc.docPtr),
                                         NodeAsNode("/response",resDoc.docPtr));

          node=NodeAsNode("/response/operations",resDoc.docPtr)->children;
          xmlNodePtr devNode=NULL;
          while (node!=NULL)
          {
            if (NodeAsString("type",node)==docTypes[docIdx].newDoc)
            {
              devNode=NodeAsNode("devices",node)->children;
              while (devNode!=NULL)
              {
                newPrn.code=NodeAsString("code",devNode);
                newPrn.name=NodeAsString("name",devNode);
                if (newPrn.code==dev_model) break;
                devNode=devNode->next;
              };
              if (devNode!=NULL) break;
            };
            node=node->next;
          };
          if (node==NULL || devNode==NULL)
            throw EConvertError("device %s not found in GetDeviceList",dev_model.c_str());
        };


        int i;
        string str;

        xmlNodePtr devicesNode=NodeAsNode("devices",reqNode);
        if (devicesNode==NULL)
          throw EConvertError("devices node not found in request");
      /*  if (devicesNode->children!=NULL)
          throw EConvertError("devices node not empty in request");*/

        xmlNodePtr operNode=NewTextChild(devicesNode,"operation");
        SetProp(operNode,"type",docTypes[docIdx].newDoc);

        node=NewTextChild(operNode,"dev_model_code",newPrn.code);
        //SetProp(node,"dev_model_name",newPrn.name);

        node=NewTextChild(operNode,"sess_params");
        SetProp(node,"type",dev_sess_type);

        if (dev_model=="DRV PRINT")
          NewTextChild(node,"addr",devName);
        else
          NewTextChild(node,"addr",devPort);
        if (dev_sess_type=="COM")
        {
          str.clear();
          if (GetNodeFast("Check",node2)!=NULL)
            str=NodeAsStringFast("Check",node2);
          if (!str.empty() && str!="false")
          {
            str.clear();
            if (GetNodeFast("Parity",node2)!=NULL)
              str=NodeAsStringFast("Parity",node2);
            if (!str.empty() && str!="None")
              throw EConvertError("wrong Parity=%s Check=true",str.c_str());
          };

          if (GetNodeFast("BaudRate",node2)!=NULL)
            NewTextChild(node,"baud_rate",NodeAsStringFast("BaudRate",node2));
          if (GetNodeFast("DataBits",node2)!=NULL)
            NewTextChild(node,"data_bits",NodeAsStringFast("DataBits",node2));
          if (GetNodeFast("Parity",node2)!=NULL)
            NewTextChild(node,"parity_bits",NodeAsStringFast("Parity",node2));
          if (GetNodeFast("StopBits",node2)!=NULL)
            NewTextChild(node,"stop_bits",NodeAsStringFast("StopBits",node2));
          if (GetNodeFast("FrameBegin",node2)!=NULL &&
             !NodeIsNULLFast("FrameBegin",node2))
          {
            i=NodeAsIntegerFast("FrameBegin",node2);
            if (i<0 || i>255) throw EConvertError("wrong FrameBegin=%d",i);
            char b[2] = {0,0};
            b[0]=i;
            StringToHex(b,str);
            NewTextChild(node,"prefix",str);
          };
          if (GetNodeFast("FrameEnd",node2)!=NULL &&
             !NodeIsNULLFast("FrameEnd",node2))
          {
            i=NodeAsIntegerFast("FrameEnd",node2);
            if (i<0 || i>255) throw EConvertError("wrong FrameEnd=%d",i);
            char b[2] = {0,0};
            b[0]=i;
            StringToHex(b,str);
            NewTextChild(node,"suffix",str);
          };
          if (GetNodeFast("RTS",node2)!=NULL &&
              NodeAsStringFast("RTS",node2)!=(string)"Disable")
            NewTextChild(node,"control_rts",NodeAsStringFast("RTS",node2));
        };

        node=NewTextChild(operNode,"fmt_params");
        SetProp(node,"type",dev_fmt_type);
        if (GetNodeFast("encoding",node2)!=NULL)
          NewTextChild(node,"encoding",NodeAsStringFast("encoding",node2));
        if (dev_fmt_type=="EPSON")
        {
          if (GetNodeFast("offset",node2)!=NULL)
            NewTextChild(node,"left",NodeAsStringFast("offset",node2));
          if (GetNodeFast("top",node2)!=NULL)
            NewTextChild(node,"top",NodeAsStringFast("top",node2));
        };
        if (dev_fmt_type=="FRX")
        {
          if (GetNodeFast("graphic",node2)!=NULL)
            NewTextChild(node,"export_bmp",NodeAsStringFast("graphic",node2));
        };
        if (dev_fmt_type=="ATB" ||
            dev_fmt_type=="BTP")
        {
          if (GetNodeFast("timeout",node2)!=NULL)
            NewTextChild(NewTextChild(node,"timeouts"),
                         "print",NodeAsStringFast("timeout",node2));
        };
        prnNode=prnNode->next;
      };
      //конвертация параметров сканера
      xmlNodePtr scnNode=GetNode("Peripherals",oldParamsNode);
      if (scnNode!=NULL)
      {
        scnNode=GetNode("Scanner",scnNode);
        if (scnNode!=NULL)
        {
          xmlNodePtr devicesNode=NodeAsNode("devices",reqNode);
          if (devicesNode==NULL)
            throw EConvertError("devices node not found in request");
        /*  if (devicesNode->children!=NULL)
            throw EConvertError("devices node not empty in request");*/

          xmlNodePtr operNode=NewTextChild(devicesNode,"operation");
          SetProp(operNode,"type","SCAN_BP");

          node=NewTextChild(operNode,"dev_model_code","SCANNER");
          //SetProp(node,"dev_model_name","");

          node=NewTextChild(operNode,"sess_params");
          if (NodeIsNULL(scnNode))
            //разрыв клавиатуры
            SetProp(node,"type","KBW");
          else
          {
            //COM-сессия
            SetProp(node,"type","COM");
            NewTextChild(node,"addr",NodeAsString(scnNode));
          };

          node=NewTextChild(operNode,"fmt_params");
          SetProp(node,"type","SCAN1");

          xmlNodePtr scnParamNode=GetNode("Peripherals/ScanParams",oldParamsNode);
          if (scnParamNode!=NULL)
          {
            node2=scnParamNode->children;
            if (GetNodeFast("Prefix",node2)!=NULL)
            {
              if (NodeIsNULL(scnNode))
              {
                //KBW
                string str1,str2;
                str1=NodeAsStringFast("Prefix",node2);
                if (!HexToString(str1,str2))
                  throw EConvertError("wrong Prefix=%s",str1.c_str());
                str1=ConvertCodepage(str2,"CP1251","UTF-16LE");
                StringToHex(str1,str2);
                NewTextChild(node,"prefix",str2);
              }
              else
                NewTextChild(node,"prefix",NodeAsStringFast("Prefix",node2));
            };
            if (GetNodeFast("Postfix",node2)!=NULL)
            {
              if (NodeIsNULL(scnNode))
              {
                //KBW
                string str1,str2;
                str1=NodeAsStringFast("Postfix",node2);
                if (!HexToString(str1,str2))
                  throw EConvertError("wrong Postfix=%s",str1.c_str());
                str1=ConvertCodepage(str2,"CP1251","UTF-16LE");
                StringToHex(str1,str2);
                NewTextChild(node,"postfix",str2);
              }
              else
                NewTextChild(node,"postfix",NodeAsStringFast("Postfix",node2));
            }
            if (GetNodeFast("Interval",node2)!=NULL)
              NewTextChild(node,"interval",NodeAsStringFast("Interval",node2));
          };
        };
      };
    };
    xmlUnlinkNode(oldParamsNode);
    xmlFreeNode(oldParamsNode);
    ProgTrace(TRACE5,"ConvertDevOldFormat: %s",XMLTreeToText(reqNode->doc).c_str());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"ConvertDevOldFormat: %s",E.what());
    NewTextChild(resNode,"convert_profile_error");
  };
  /*
             <printer>
               <type>
               <name>
               <port>
               <encoding>     format +
               <timeout>      format/timeouts +
               <BaudRate>     sess +
               <DataBits>     sess +
               <StopBits>     sess +
               <ReplaceChar>  sess -
               <FrameBegin>   sess +
               <FrameEnd>     sess +
               <ParityReplace>sess -
               <Check>        sess -
               <Parity>       sess +
               <offset>       format +
               <top>          format +
               <graphic>      format +
               <RTS>          sess
             </printer>
  */
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
      throw UserException("Неверно указан пользователь или пароль");
    if ( Qry.FieldAsInteger( "pr_denial" ) == -1 )
    	throw UserException( "Пользователь удален из системы" );
    if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
      throw UserException( "Пользователю отказано в доступе" );
    reqInfo->user.user_id = Qry.FieldAsInteger("user_id");
    reqInfo->user.login = Qry.FieldAsString("login");
    reqInfo->user.descr = Qry.FieldAsString("descr");
    if(Qry.FieldIsNULL("desk"))
    {
      showMessage( reqInfo->user.descr + ", добро пожаловать в систему");
    }
    else
     if (reqInfo->desk.code != Qry.FieldAsString("desk"))
       showMessage("Замена терминала");
    if (Qry.FieldAsString("passwd")==(string)"ПАРОЛЬ" )
      showErrorMessage("Пользователю необходимо изменить пароль");
    Qry.Clear();
    Qry.SQLText =
      "BEGIN "
      "  UPDATE users2 SET desk = NULL WHERE desk = :desk; "
      "  UPDATE users2 SET desk = :desk WHERE user_id = :user_id; "
      "  UPDATE desks "
      "  SET version = :version, last_logon = system.UTCSYSDATE "
      "  WHERE code = :desk; "
      "END;";
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.CreateVariable("desk",otString,reqInfo->desk.code);
    string version;
    if (GetNode("term_version", reqNode)!=NULL)
      version=NodeAsString("term_version", reqNode);
    if (!version.empty())
      Qry.CreateVariable("version",otString,version);
    else
      Qry.CreateVariable("version",otString,UNKNOWN_VERSION);
    Qry.Execute();

    TReqInfoInitData reqInfoData;

    string airlines;
    if (!GetSessionAirlines(GetNode("airlines", reqNode),airlines))
      throw UserException("Не найден код авиакомпании %s",airlines.c_str());
    getJxtContHandler()->sysContext()->write("session_airlines",airlines);
    xmlNodePtr node=NodeAsNode("/term/query",ctxt->reqDoc);
    reqInfoData.screen = NodeAsString("@screen", node);
    reqInfoData.pult = ctxt->pult;
    reqInfoData.opr = NodeAsString("@opr", node);
    xmlNodePtr modeNode = GetNode("@mode", node);
    if (modeNode!=NULL)
      reqInfoData.mode = NodeAsString(modeNode);
    reqInfoData.checkUserLogon = false;
    reqInfoData.checkCrypt = false;
    reqInfo->Initialize( reqInfoData );

    //здесь reqInfo нормально инициализирован
    GetModuleList(resNode);
    ConvertDevOldFormat(reqNode,resNode);
    GetDevices(reqNode,resNode);
    showBasicInfo();
}

void MainDCSInterface::UserLogoff(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    Qry.SQLText = "UPDATE users2 SET desk = NULL WHERE user_id = :user_id";
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.Execute();
    getJxtContHandler()->sysContext()->remove("session_airlines");
    showMessage("Сеанс работы в системе завершен");
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
        throw UserException("Неверно указан текущий пароль");
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
    showMessage("Пароль изменен");
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
    "SELECT dev_oper_types.code AS op_type, dev_oper_types.name AS op_name, "
    "       dev_model_code,dev_model_name "
    "FROM dev_oper_types, "
    "  (SELECT DISTINCT "
    "          dev_models.code AS dev_model_code, dev_models.name AS dev_model_name, "
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
    "ORDER BY op_type,dev_model_name";

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
      NewTextChild(operNode,"name",Qry.FieldAsString("op_name"));
      devsNode=NewTextChild(operNode,"devices");
    };
    if (!Qry.FieldIsNULL("dev_model_code"))
    {
      devNode=NewTextChild(devsNode,"device");
      NewTextChild(devNode,"code",Qry.FieldAsString("dev_model_code"));
      NewTextChild(devNode,"name",Qry.FieldAsString("dev_model_name"));
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


bool ParseScanData(const string& data, TScanParams& params)
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
      ProgTrace(TRACE5,"ParseScanData: p=%d str=%s", p, str.substr(p).c_str());
    else
      ProgTrace(TRACE5,"ParseScanData: p=%d",p);*/

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
        //ProgTrace(TRACE5,"ParseScanData: len_u=%d",len_u);
        p+=4;
        if (str_size<p+len_u+2) throw EConvertError("06");
        if (!HexToString(str.substr(p+len_u,2),c) || c.size()<1) throw EConvertError("07");
        len_r=(int)c[0]; //item number=17
        //ProgTrace(TRACE5,"ParseScanData: len_r=%d",len_r);
        p+=len_u+2;
        if (str_size<p+len_r) throw EConvertError("08");
        p+=len_r;
        if (i>p)
          ProgTrace(TRACE5,"ParseScanData: airline use=%s",str.substr(p,i-p).c_str());
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
        ProgTrace(TRACE5,"ParseScanData: EConvertError %s: p=%d str=%s", e.what(), p, str.substr(p).c_str());
      else
        ProgTrace(TRACE5,"ParseScanData: EConvertError %s: p=%d", e.what(), p);*/
    };
    ph++;
  }
  while (p!=string::npos);

/*str:=parsedData.proc_value;
  try
    if Length(str)<60 then raise EConvertError.Create('');
    i:=StrToInt('$'+Copy(str,59,2));
    Delete(str,1,60);
    if i>Length(str) then raise EConvertError.Create('');
    str:=Copy(str,1,i);
    if Length(str)<4 then raise EConvertError.Create('');
    len_u:=StrToInt('$'+Copy(str,3,2));
    Delete(str,1,4);
    if len_u+2>Length(str) then raise EConvertError.Create('');
    len_r:=StrToInt('$'+Copy(str,len_u+1,2));
    Delete(str,1,len_u+2);
    if len_r>Length(str) then raise EConvertError.Create('');
    Delete(str,1,len_r);
  except
    on EConvertError do
      str:=parsedData.proc_value;  //тючьюцэю ¤Єю 2/5
  end;

  if Length(str)=10 then
  begin
    i:=1;
    for i:=i to Length(str) do
      if not(IsDigit(str[i])) then break;
    if i>Length(str) then
    try
      parsedData.pax_id:=StrToInt(str);
      Result:=true;
      Exit;
    except
      on EConvertError do ;
    end;
  end;*/
  return init;
};


void MainDCSInterface::DetermineScanParams(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  try
  {
  	vector<TScanParams> ScanParams;
  	TScanParams params;
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

      if (ParseScanData(data,params))
        ScanParams.push_back(params);
    };
    if (ScanParams.empty()) throw EConvertError("ScanParams empty");
    for(vector<TScanParams>::iterator i=ScanParams.begin();i!=ScanParams.end();i++)
    {
      if (i==ScanParams.begin())
        params=*i;
      else
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
    };
    if (params.code_id_len<0 ||
        params.code_id_len>9 ||
        params.code_id_len>params.prefix.size())
      throw EConvertError("Wrong params.code_id_len=%lu",params.code_id_len);
    params.prefix.erase(params.prefix.size()-params.code_id_len);
    node=NewTextChild(resNode,"fmt_params");
    string data;
    StringToHex(ConvertCodepage(params.prefix,"CP866",encoding),data);
    NewTextChild(node,"prefix",data);
    StringToHex(ConvertCodepage(params.postfix,"CP866",encoding),data);
    NewTextChild(node,"postfix",data);
    NewTextChild(node,"code_id_len",(int)params.code_id_len);
  }
  catch(EConvertError &e)
  {
    ProgTrace(TRACE0,"DetermineScanParams: %s", e.what());
    throw UserException("Невозможно автоматически определить параметры устройства");
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
	IntPutRequestCertificate(ctxt, reqNode, resNode);
}
