#include "maindcs.h"
#include "basic.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "oralib.h"
#include "exceptions.h"
#define NICKNAME "VLAD"
#include "test.h"
#include <string>
#include <fstream>
#include "xml_unit.h"
#include "jxt_cont.h"

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

void GetDeviceParams(TDevParamCategory category, TQuery &ParamsQry, xmlNodePtr devNode)
{
  if (ParamsQry.Eof || devNode==NULL) return;

  string paramType;
  xmlNodePtr paramTypeNode=NULL,paramNameNode=NULL,subparamNameNode;
  for(;!ParamsQry.Eof;ParamsQry.Next())
  {
    if (paramTypeNode==NULL ||
        paramType!=ParamsQry.FieldAsString("param_type"))
    {
      paramType=ParamsQry.FieldAsString("param_type");
      switch (category)
      {
        case dpcSession: paramTypeNode=NewTextChild(devNode,"sess_params");
                         break;
        case dpcFormat:  paramTypeNode=NewTextChild(devNode,"fmt_params");
                         break;
        case dpcModel:   paramTypeNode=NewTextChild(devNode,"model_params");
                         break;

      };
      SetProp(paramTypeNode,"type",paramType);
      paramNameNode=NULL;
    };
    if (paramNameNode==NULL ||
        strcmp((const char*)paramNameNode->name,ParamsQry.FieldAsString("param_name"))!=0)
    {
      if (ParamsQry.FieldIsNULL("subparam_name"))
      {
        paramNameNode=NewTextChild(paramTypeNode,ParamsQry.FieldAsString("param_name"),ParamsQry.FieldAsString("param_value"));
        SetProp(paramNameNode,"editable",(int)(ParamsQry.FieldAsInteger("editable")!=0));
      }
      else
        paramNameNode=NewTextChild(paramTypeNode,ParamsQry.FieldAsString("param_name"));
    };
    if (!ParamsQry.FieldIsNULL("subparam_name"))
    {
      subparamNameNode=NewTextChild(paramNameNode,ParamsQry.FieldAsString("subparam_name"),ParamsQry.FieldAsString("param_value"));
      SetProp(subparamNameNode,"editable",(int)(ParamsQry.FieldAsInteger("editable")!=0));
    };
  };
};

void GetDevices(xmlNodePtr reqNode, xmlNodePtr resNode)
{
  if (reqNode==NULL || resNode==NULL) return;
  resNode=NewTextChild(resNode,"devices");
  GetDeviceAirlines(resNode);

  reqNode=GetNode("devices",reqNode);
  if (reqNode==NULL) return;

  TReqInfo *reqInfo = TReqInfo::Instance();

  TQuery ModelQry(&OraSession);
  ModelQry.Clear();
  ModelQry.SQLText="SELECT name FROM dev_models WHERE code=:dev_model";
  ModelQry.DeclareVariable("dev_model",otString);

  TQuery SessParamsQry(&OraSession);
  SessParamsQry.Clear();
  SessParamsQry.SQLText=
    "SELECT dev_model_params.sess_type AS param_type, "
    "       param_name,subparam_name,param_value,editable "
    "FROM dev_model_params,dev_sess_modes "
    "WHERE dev_model_params.dev_model=:dev_model AND "
    "      dev_model_params.sess_type=dev_sess_modes.sess_type AND "
    "      dev_sess_modes.term_mode=:term_mode AND dev_sess_modes.sess_type=:sess_type AND "
    "      (dev_model_params.fmt_type IS NULL OR dev_model_params.fmt_type=:fmt_type) "
    "ORDER BY param_type, param_name, subparam_name NULLS FIRST";
  SessParamsQry.CreateVariable("term_mode",otString,EncodeOperMode(reqInfo->desk.mode));
  SessParamsQry.DeclareVariable("dev_model",otString);
  SessParamsQry.DeclareVariable("sess_type",otString);
  SessParamsQry.DeclareVariable("fmt_type",otString);

  TQuery FmtParamsQry(&OraSession);
  FmtParamsQry.Clear();
  FmtParamsQry.SQLText=
    "SELECT dev_model_params.fmt_type AS param_type, "
    "       param_name,subparam_name,param_value,editable "
    "FROM dev_model_params,dev_fmt_opers "
    "WHERE dev_model_params.dev_model=:dev_model AND "
    "      dev_model_params.fmt_type=dev_fmt_opers.fmt_type AND "
    "      dev_fmt_opers.op_type=:op_type AND dev_fmt_opers.fmt_type=:fmt_type AND "
    "      (dev_model_params.sess_type IS NULL OR dev_model_params.sess_type=:sess_type) "
    "ORDER BY param_type, param_name, subparam_name NULLS FIRST";
  FmtParamsQry.DeclareVariable("op_type",otString);
  FmtParamsQry.DeclareVariable("dev_model",otString);
  FmtParamsQry.DeclareVariable("sess_type",otString);
  FmtParamsQry.DeclareVariable("fmt_type",otString);

  TQuery ModelParamsQry(&OraSession);
  ModelParamsQry.Clear();
  ModelParamsQry.SQLText=
    "SELECT NULL AS param_type, "
    "       param_name,subparam_name,param_value,editable "
    "FROM dev_model_params "
    "WHERE dev_model_params.dev_model=:dev_model AND sess_type IS NULL AND fmt_type IS NULL "
    "ORDER BY param_type, param_name, subparam_name NULLS FIRST";
  ModelParamsQry.DeclareVariable("dev_model",otString);
  //считаем все операции + устройства по умолчанию
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT dev_oper_types.code AS op_type, "
    "       dev_model_defaults.dev_model, "
    "       dev_model_defaults.sess_type, "
    "       dev_model_defaults.fmt_type "
    "FROM dev_oper_types,dev_model_defaults "
    "WHERE dev_oper_types.code=dev_model_defaults.op_type(+) AND "
    "      dev_model_defaults.term_mode(+)=:term_mode ";
  Qry.CreateVariable("term_mode",otString,EncodeOperMode(reqInfo->desk.mode));
  Qry.Execute();
  string op_type;
  for(;!Qry.Eof;Qry.Next())
  {
    if (op_type==Qry.FieldAsString("op_type")) continue;
    op_type=Qry.FieldAsString("op_type");

    FmtParamsQry.SetVariable("op_type",op_type);

    xmlNodePtr node;
    for(node=reqNode->children;node!=NULL;node=node->next)
      if (op_type==NodeAsString("@type",node)) break;

    string fmt_type,sess_type,dev_model;
    if (node!=NULL)
    {
      dev_model=NodeAsString("dev_model",node);
      sess_type=NodeAsString("sess_type",node);
      fmt_type=NodeAsString("fmt_type",node);
    };

    for(int k=0;k<=1;k++)
    {
      if (k==1)
      {
        dev_model=Qry.FieldAsString("dev_model");
        sess_type=Qry.FieldAsString("sess_type");
        fmt_type=Qry.FieldAsString("fmt_type");
      };

      if (dev_model.empty() || sess_type.empty() || fmt_type.empty()) continue;

      ModelQry.SetVariable("dev_model",dev_model);
      ModelQry.Execute();
      if (ModelQry.Eof) continue;

      SessParamsQry.SetVariable("dev_model",dev_model);
      SessParamsQry.SetVariable("sess_type",sess_type);
      SessParamsQry.SetVariable("fmt_type",fmt_type);
      SessParamsQry.Execute();
      if (SessParamsQry.Eof) continue;

      FmtParamsQry.SetVariable("dev_model",dev_model);
      FmtParamsQry.SetVariable("sess_type",sess_type);
      FmtParamsQry.SetVariable("fmt_type",fmt_type);
      FmtParamsQry.Execute();
      if (FmtParamsQry.Eof) continue;

      ModelParamsQry.SetVariable("dev_model",dev_model);
      ModelParamsQry.Execute();
      xmlNodePtr operTypeNode=NewTextChild(resNode,"operation");
      SetProp(operTypeNode,"type",op_type);
      NewTextChild(operTypeNode,"dev_model_code",dev_model);
      NewTextChild(operTypeNode,"dev_model_name",ModelQry.FieldAsString("name"));
      GetDeviceParams(dpcSession,SessParamsQry,operTypeNode);
      GetDeviceParams(dpcFormat,FmtParamsQry,operTypeNode);
      GetDeviceParams(dpcModel,ModelParamsQry,operTypeNode);
      break;

    };
  };
};

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
      "END;";
    Qry.CreateVariable("user_id",otInteger,reqInfo->user.user_id);
    Qry.CreateVariable("desk",otString,reqInfo->desk.code);
    Qry.Execute();

    string airlines;
    if (!GetSessionAirlines(GetNode("airlines", reqNode),airlines))
      throw UserException("Не найден код авиакомпании %s",airlines.c_str());
    getJxtContHandler()->sysContext()->write("session_airlines",airlines);
    xmlNodePtr node=NodeAsNode("/term/query",ctxt->reqDoc);
    std::string screen = NodeAsString("@screen", node);
    std::string opr = NodeAsString("@opr", node);
    xmlNodePtr modeNode = GetNode("@mode", node);
    std::string mode;
    if (modeNode!=NULL)
      mode = NodeAsString(modeNode);
    reqInfo->Initialize( screen, ctxt->pult, opr, mode, false );

    //здесь reqInfo нормально инициализирован
    GetModuleList(resNode);
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
  sql << "SELECT dev_oper_types.code AS op_type, dev_oper_types.name AS op_name, "
         "       dev_models.code AS dev_model_code, dev_models.name AS dev_model_name "
         "FROM dev_oper_types,dev_models, "
         "     (SELECT DISTINCT dev_model_params.dev_model,dev_fmt_opers.op_type "
         "      FROM dev_fmt_opers,dev_model_params, "
         "           (SELECT DISTINCT dev_model "
         "            FROM dev_sess_modes,dev_model_params "
         "            WHERE dev_model_params.sess_type=dev_sess_modes.sess_type AND "
         "                  dev_sess_modes.term_mode=:term_mode) dev_sess_models "
         "      WHERE dev_model_params.fmt_type=dev_fmt_opers.fmt_type AND ";
  if (!op_type.empty())
    sql << "          dev_fmt_opers.op_type=:op_type AND ";

  sql << "            dev_model_params.dev_model=dev_sess_models.dev_model) dev_fmt_models "
         "WHERE dev_oper_types.code=dev_fmt_models.op_type(+) AND ";
  if (!op_type.empty())
    sql << "    dev_oper_types.code=:op_type AND ";

  sql << "      dev_fmt_models.dev_model=dev_models.code(+) "
         "ORDER BY op_type,dev_model_name ";

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
  TReqInfo *reqInfo = TReqInfo::Instance();

  string dev_model=NodeAsString("dev_model_code",reqNode);
  string op_type=NodeAsString("operation",reqNode);

  xmlNodePtr devNode=NewTextChild(resNode,"device");

  TQuery ModelQry(&OraSession);
  ModelQry.Clear();
  ModelQry.SQLText="SELECT name FROM dev_models WHERE code=:dev_model";
  ModelQry.CreateVariable("dev_model",otString,dev_model);
  ModelQry.Execute();
  if (ModelQry.Eof) return;

  TQuery SessParamsQry(&OraSession);
  SessParamsQry.Clear();
  SessParamsQry.SQLText=
    "SELECT dev_model_params.sess_type AS param_type, "
    "       param_name,subparam_name,param_value,editable "
    "FROM dev_model_params,dev_sess_modes "
    "WHERE dev_model_params.dev_model=:dev_model AND "
    "      dev_model_params.sess_type=dev_sess_modes.sess_type AND "
    "      dev_sess_modes.term_mode=:term_mode "
    "ORDER BY param_type, param_name, subparam_name NULLS FIRST";
  SessParamsQry.CreateVariable("term_mode",otString,EncodeOperMode(reqInfo->desk.mode));
  SessParamsQry.CreateVariable("dev_model",otString,dev_model);
  SessParamsQry.Execute();
  if (SessParamsQry.Eof) return;

  TQuery FmtParamsQry(&OraSession);
  FmtParamsQry.Clear();
  FmtParamsQry.SQLText=
    "SELECT dev_model_params.fmt_type AS param_type, "
    "       param_name,subparam_name,param_value,editable "
    "FROM dev_model_params,dev_fmt_opers "
    "WHERE dev_model_params.dev_model=:dev_model AND "
    "      dev_model_params.fmt_type=dev_fmt_opers.fmt_type AND "
    "      dev_fmt_opers.op_type=:op_type "
    "ORDER BY param_type, param_name, subparam_name NULLS FIRST";
  FmtParamsQry.CreateVariable("op_type",otString,op_type);
  FmtParamsQry.CreateVariable("dev_model",otString,dev_model);
  FmtParamsQry.Execute();
  if (FmtParamsQry.Eof) return;

  TQuery ModelParamsQry(&OraSession);
  ModelParamsQry.Clear();
  ModelParamsQry.SQLText=
    "SELECT NULL AS param_type, "
    "       param_name,subparam_name,param_value,editable "
    "FROM dev_model_params "
    "WHERE dev_model_params.dev_model=:dev_model AND sess_type IS NULL AND fmt_type IS NULL "
    "ORDER BY param_type, param_name, subparam_name NULLS FIRST";
  ModelParamsQry.CreateVariable("dev_model",otString,dev_model);
  ModelParamsQry.Execute();

  NewTextChild(devNode,"dev_model_code",dev_model);
  NewTextChild(devNode,"dev_model_name",ModelQry.FieldAsString("name"));
  xmlNodePtr paramsNode;
  paramsNode=NewTextChild(devNode,"sessions");
  GetDeviceParams(dpcSession,SessParamsQry,paramsNode);
  paramsNode=NewTextChild(devNode,"formats");
  GetDeviceParams(dpcFormat,FmtParamsQry,paramsNode);
  GetDeviceParams(dpcModel,ModelParamsQry,devNode);
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


