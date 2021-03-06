#include "cache.h"
#include "date_time.h"
#include "misc.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_locale.h"
#include "tlg/tlg.h"
#include "flt_binding.h"
#include "astra_service.h"
#include "term_version.h"
#include "comp_layers.h"
#include "astra_misc.h"
#include "timer.h"
#include "qrys.h"
#include "trip_tasks.h"
#include "jxtlib/xml_stuff.h"
#include <serverlib/algo.h>
#include <serverlib/cursctl.h>

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

const char * CacheFieldTypeS[NumFieldType] = {"NS","NU","D","T","S","B","SL",""};

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace ASTRA;

/* ??? ???????? ⥣?? params ??ॢ?????? ? ????孨? ॣ???? - ??६????? ? sql ? ???孥? ॣ?????*/
void TCacheTable::getParams(xmlNodePtr paramNode, TParams &vparams)
{
  vparams.clear();
  if ( paramNode == NULL ) /* ??????????? ⥣ ??ࠬ??஢ */
    return;
  xmlNodePtr curNode = paramNode->children;
  while( curNode ) {
    string name = upperc( (const char*)curNode->name );
    string value = NodeAsString(curNode);
    vparams[ name ].Value = value;
    ProgTrace( TRACE5, "param name=%s, value=%s", name.c_str(), value.c_str() );
    xmlNodePtr propNode  = GetNode( "@type", curNode ); /* ?ᯮ???????? ??? sqlparams */
    if ( propNode )
      vparams[ name ].DataType = (TCacheConvertType)NodeAsInteger( propNode );
    else
      vparams[ name ].DataType = ctString;
    curNode = curNode->next;
  }
}

/* ??????????? ?????? - ?롨ࠥ? ??騥 ??ࠬ???? + ??騥 ??६????? ??? sql ???????
   + ?롮ઠ ?????? ?? ⠡???? cache_tables, cache_fields */
void TCacheTable::Init(xmlNodePtr cacheNode)
{
  if ( cacheNode == NULL )
    throw Exception("wrong message format");
  getParams(GetNode("params", cacheNode), Params); /* ??騥 ??ࠬ???? */
  getParams(GetNode("sqlparams", cacheNode), SQLParams); /* ??ࠬ???? ??????? sql */
  if ( Params.find( TAG_CODE ) == Params.end() )
    throw Exception("wrong message format");
  string code = Params[TAG_CODE].Value;
  if (code.find("USE_ACCESS")==0)
    throw UserException("MSG.WORKING_WITH_TABLE_PROHIBITED.USE_ACCESS_SCREEN");
  Qry = OraSession.CreateQuery();
  Forbidden = true;
  ReadOnly = true;
  clientVerData = -1;
  clientVerIface = -1;

  Qry->Clear();
  Qry->SQLText = "SELECT title, select_sql, refresh_sql, insert_sql, update_sql, delete_sql, "
                 "       logging, keep_locally, keep_deleted_rows, event_type, tid, need_refresh, "
                 "       select_right, insert_right, update_right, delete_right "
                 " FROM cache_tables WHERE code = :code";
  Qry->DeclareVariable("code", otString);
  Qry->SetVariable("code", code);
  Qry->Execute();
  if ( Qry->Eof )
    throw Exception( "table " + string( code ) + " not found" );
  Title = Qry->FieldAsString( "title" );
  SelectSQL = Qry->FieldAsString("select_sql");
  RefreshSQL = Qry->FieldAsString("refresh_sql");
  InsertSQL = Qry->FieldAsString("insert_sql");
  UpdateSQL = Qry->FieldAsString("update_sql");
  DeleteSQL = Qry->FieldAsString("delete_sql");
  Logging = Qry->FieldAsInteger("logging") != 0;
  KeepLocally = Qry->FieldAsInteger("keep_locally") != 0;
  KeepDeletedRows = Qry->FieldAsInteger("keep_deleted_rows") != 0;

  EventType = DecodeEventType( Qry->FieldAsString( "event_type" ) );
  curVerIface = Qry->FieldAsInteger( "tid" ); /* ⥪???? ?????? ?????䥩?? */
  pr_dconst = !Qry->FieldAsInteger( "need_refresh" );
  //????稬 ?ࠢ? ????㯠 ?? ?????権
  if (!Qry->FieldIsNULL("select_right"))
    SelectRight=Qry->FieldAsInteger("select_right");
  else
    SelectRight=-1;
  if (!Qry->FieldIsNULL("insert_right"))
    InsertRight=Qry->FieldAsInteger("insert_right");
  else
    InsertRight=-1;
  if (!Qry->FieldIsNULL("update_right"))
    UpdateRight=Qry->FieldAsInteger("update_right");
  else
    UpdateRight=-1;
  if (!Qry->FieldIsNULL("delete_right"))
    DeleteRight=Qry->FieldAsInteger("delete_right");
  else
    DeleteRight=-1;
  getPerms( );
  initChildTables();
  initFields(); /* ???樠??????? FFields */
}

TCacheTable::~TCacheTable()
{
  OraSession.DeleteQuery(*Qry);
}

void TCacheTable::Clear()
{
  table.clear();
}

bool TCacheTable::refreshInterface()
{
  string code = Params[TAG_CODE].Value;
  string stid = Params[ TAG_REFRESH_INTERFACE ].Value;
  TrimString( stid );
  if ( stid.empty() || StrToInt( stid.c_str(), clientVerIface ) == EOF )
    clientVerIface = -1;
  ProgTrace(TRACE5, "Client version interface: %d", clientVerIface );
  if ( clientVerIface == curVerIface )
    return false;
  clientVerIface = curVerIface;
  ProgTrace( TRACE5, "must refresh interface" );
  return true;
}

void TCacheTable::initChildTables()
{
    string code = Params[TAG_CODE].Value;
    Qry->Clear();
    Qry->SQLText =
        "SELECT "
        "  cache_child_tables.cache_child, "
        "  NVL(cache_child_tables.title, cache_tables.title) AS title, "
        "  cache_child_fields.field_parent, "
        "  cache_child_fields.field_child, "
        "  cache_child_fields.select_var, "
        "  cache_child_fields.modify_var AS insert_var, "
        "  cache_child_fields.modify_var AS update_var, "
        "  cache_child_fields.modify_var AS delete_var, "
        "  cache_child_fields.auto_insert, "
        "  cache_child_fields.check_equal, "
        "  cache_child_fields.read_only "
        "FROM "
        "  cache_child_tables, "
        "  cache_child_fields, "
        "  cache_tables "
        "WHERE "
        "  cache_child_tables.cache_parent = :code AND "
        "  cache_child_tables.cache_parent = cache_child_fields.cache_parent AND "
        "  cache_child_tables.cache_child = cache_child_fields.cache_child AND "
        "  cache_child_tables.cache_child = cache_tables.code "
        "ORDER BY num ";
    Qry->CreateVariable("code", otString, code);
    Qry->Execute();
    string prev_child;
    for(; not Qry->Eof; Qry->Next()) {
        string child = Qry->FieldAsString("cache_child");
        if(child != prev_child) {
            prev_child = child;
            TCacheChildTable new_child;
            new_child.code = child;
            new_child.title = Qry->FieldAsString("title");
            FChildTables.push_back(new_child);
        }
        TCacheChildField field;
        field.field_parent = Qry->FieldAsString("field_parent");
        field.field_child = Qry->FieldAsString("field_child");
        field.select_var = Qry->FieldAsString("select_var");
        field.insert_var = Qry->FieldAsString("insert_var");
        field.update_var = Qry->FieldAsString("update_var");
        field.delete_var = Qry->FieldAsString("delete_var");
        field.auto_insert = Qry->FieldAsInteger("auto_insert")!=0;
        field.check_equal = Qry->FieldAsInteger("check_equal")!=0;
        field.read_only = Qry->FieldAsInteger("read_only")!=0;
        FChildTables.back().fields.push_back(field);
    }
}

void TCacheTable::initFields()
{
    string code = Params[TAG_CODE].Value;
    // ???⠥? ???? ? ????? ????
    Qry->Clear();
    Qry->SQLText =
        "SELECT name,title,width,char_case,align,data_type, "
        "       data_size,scale,nullable,pr_ident,read_only, "
        "       refer_code,refer_name,refer_level,refer_ident,lang,num "
        "FROM cache_fields "
        "WHERE code=:code AND (lang IS NULL OR lang=:lang)"
        "ORDER BY name, lang NULLS LAST ";
    Qry->CreateVariable("code",otString,code);
    Qry->CreateVariable("lang",otString,TReqInfo::Instance()->desk.lang);
    Qry->Execute();

    if(Qry->Eof)
        throw Exception((string)"Fields of table '"+code+"' not found");
    string prior_name;
    while(!Qry->Eof) {

          if ( !prior_name.empty() && prior_name == Qry->FieldAsString("name") ) { // ?????७?? ???? ? ????? ?????? ?ਮ????⮬
            Qry->Next();
            continue;
          }
          prior_name = Qry->FieldAsString("name");

        TCacheField2 FField;

        FField.Name = Qry->FieldAsString("name");
        FField.Name = upperc( FField.Name );
        if(FField.Name.find(';') != string::npos)
            throw Exception((string)"Wrong field name '"+code+"."+FField.Name+"'");
        if ((FField.Name == "TID") || (FField.Name == "PR_DEL"))
            throw Exception((string)"Field name '"+code+"."+FField.Name+"' reserved");
        FField.Title = Qry->FieldAsString("title");

        // ????稬 ⨯ ????

        FField.DataType = ftUnknown;
        for(int ft = 0; ft < NumFieldType; ft++) {
            if(strcmp(Qry->FieldAsString("data_type"), CacheFieldTypeS[ft]) == 0) {
                FField.DataType = (TCacheFieldType)ft;
                break;
            }
        }
        if(FField.DataType == ftUnknown)
            throw Exception((string)"Unknown type of field '"+code+"."+FField.Name+"'");

        FField.CharCase = ecNormal;

        if((string)Qry->FieldAsString("char_case") == "L") FField.CharCase = ecLowerCase;
        if((string)Qry->FieldAsString("char_case") == "U") FField.CharCase = ecUpperCase;

        switch(FField.DataType) {
            case ftSignedNumber:
            case ftUnsignedNumber:
                FField.Align = TAlignment::RightJustify;
                break;
            default: FField.Align = TAlignment::LeftJustify;
        }

        if((string)Qry->FieldAsString("align") == "L") FField.Align = TAlignment::LeftJustify;
        if((string)Qry->FieldAsString("align") == "R") FField.Align = TAlignment::RightJustify;
        if((string)Qry->FieldAsString("align") == "C") FField.Align = TAlignment::Center;

        FField.DataSize = Qry->FieldAsInteger("data_size");
        if(FField.DataSize<=0)
            throw Exception((string)"Wrong size of field '"+code+"."+FField.Name+"'");
        FField.Scale = Qry->FieldAsInteger("scale");
        if((FField.Scale<0) || (FField.Scale>FField.DataSize))
            throw Exception((string)"Wrong scale of field '"+code+"."+FField.Name+"'");
        if((FField.DataType == ftSignedNumber || FField.DataType == ftUnsignedNumber) &&
                FField.DataSize>15)
            throw Exception((string)"Wrong size of field '"+code+"."+FField.Name+"'");
        /* ??ਭ? ???? */

        if(Qry->FieldIsNULL("width")) {
            switch(FField.DataType) {
                case ftSignedNumber:
                case ftUnsignedNumber:
                    FField.Width = FField.DataSize;
                    if (FField.DataType == ftSignedNumber) FField.Width++;
                    if(FField.Scale>0) {
                        FField.Width++;
                        if (FField.DataSize == FField.Scale) FField.Width++;
                    }
                    break;
                case ftDate:
                    if (FField.DataSize>=1 && FField.DataSize<=7)
                        FField.Width = 5; /* dd.mm */
                    else
                        if (FField.DataSize>=8 && FField.DataSize<=9)
                            FField.Width = 8; /* dd.mm.yy */
                        else FField.Width = 10; /* dd.mm.yyyy */
                    break;
                case ftTime:
                        FField.Width = 5;
                        break;
                default:
                        FField.Width = FField.DataSize;
            }
        }
        else
            FField.Width = Qry->FieldAsInteger("width");

        FField.Nullable = Qry->FieldAsInteger("nullable") != 0;
        FField.Ident = Qry->FieldAsInteger("pr_ident") != 0;

        FField.ReadOnly = Qry->FieldAsInteger("read_only") != 0;
        FField.ReferCode = Qry->FieldAsString("refer_code");
        FField.ReferCode = upperc( FField.ReferCode );
        FField.ReferName = Qry->FieldAsString("refer_name");
        FField.ReferName = upperc( FField.ReferName );
        FField.num = Qry->FieldAsInteger("num");

        if (FField.ReferCode.empty() ^ FField.ReferName.empty())
            throw Exception((string)"Wrong reference of field '"+code+"."+FField.Name+"'");
        if(Qry->FieldIsNULL("refer_level"))
            FField.ReferLevel = -1;
        else {
            FField.ReferLevel = Qry->FieldAsInteger("refer_level");
            if(FField.ReferLevel<0)
                throw Exception((string)"Wrong reference of field '"+code+"."+FField.Name+"'");
        }
        if(Qry->FieldIsNULL("refer_ident"))
            FField.ReferIdent = 0;
        else {
            FField.ReferIdent = Qry->FieldAsInteger("refer_ident");
            if(FField.ReferIdent<0)
                throw Exception((string)"Wrong reference of field '"+code+"."+FField.Name+"'");
        }

        /* ?஢?ਬ, ?⮡? ????? ????? ?? ?㡫?஢????? */
        for(vector<TCacheField2>::iterator i = FFields.begin(); i != FFields.end(); i++)
            if(FField.Name == i->Name)
                throw Exception((string)"Duplicate field name '"+code+"."+FField.Name+"'");

        /* ?஢?ਬ, ???? ?? ??ॢ????? ᮤ?ন??? ???? ? ᮮ⢥??⢨? ? LANG */
        if (FField.ReferName == "CODE/CODE_LAT" ||
            FField.ReferName == "CODE_LAT/CODE" )
        {
          FField.ElemCategory=cecCode;
        };

        if (FField.ReferName == "NAME/NAME_LAT" ||
            FField.ReferName == "NAME_LAT/NAME" )
        {
          FField.ElemCategory=cecName;
        };

        if (FField.ReferName == "SHORT_NAME/SHORT_NAME_LAT" ||
            FField.ReferName == "SHORT_NAME_LAT/SHORT_NAME" )
        {
          FField.ElemCategory=cecNameShort;
        };

        if (FField.ReferName == "DESCR/DESCR_LAT" ||
            FField.ReferName == "DESCR_LAT/DESCR" )
        {
          FField.ElemCategory=cecNone;
        };

        if ((code == "ROLES" && FField.Name == "ROLE_NAME") ||
            (FField.ReferCode == "ROLES" && FField.ReferName == "ROLE_NAME"))
        {
          FField.ElemCategory=cecRoleName;
        };

        if ((code == "AIRLINE_PROFILES" && FField.Name == "NAME") ||
            (FField.ReferCode == "AIRLINE_PROFILES" && FField.ReferName == "NAME"))
        {
          FField.ElemCategory=cecProfileName;
        };

        if ((code == "USERS" && FField.Name == "USER_NAME") ||
            (FField.ReferCode == "USERS" && FField.ReferName == "USER_NAME"))
        {
          FField.ElemCategory=cecUserName;
        };

        if (code == "USERS" && FField.Name == "USER_PERMS")
        {
          FField.ElemCategory=cecUserPerms;
        };

        if (FField.ElemCategory==cecCode ||
            FField.ElemCategory==cecName ||
            FField.ElemCategory==cecNameShort)
        {
          int i=sizeof(ReferCacheTable)/sizeof(ReferCacheTable[0])-1;
          for(;i>=0;i--)
            if (ReferCacheTable[i].CacheCode==FField.ReferCode) break;
          if (i>=0)
          {
            FField.ElemType=ReferCacheTable[i].ElemType;
            ProgTrace(TRACE5,"initFields: name=%s, elem_type=%d", FField.Name.c_str(), (int)FField.ElemType);
          }
          else
          {
            FField.ElemCategory=cecNone;
            ProgTrace(TRACE5,"initFields: name=%s, elem_type unknown", FField.Name.c_str());
          };
        };
        if ((FField.ReferCode == "STATIONS" ||
             FField.ReferCode == "BI_HALLS_AND_TERMINALS") && FField.ReferName == "AIRP_VIEW")
        {
          FField.ElemCategory=cecCode;
          FField.ElemType=etAirp;
        };
        if ((FField.ReferCode == "BRANDS" ||
             FField.ReferCode == "FQT_TIER_LEVELS" ||
             FField.ReferCode == "FQT_TIER_LEVELS_EXTENDED" ||
             FField.ReferCode == "RFISC_COMP_PROPS") && FField.ReferName == "AIRLINE_VIEW")
        {
          FField.ElemCategory=cecCode;
          FField.ElemType=etAirline;
        };
        if (FField.ReferCode == "SALE_POINTS" && FField.ReferName == "VALIDATOR_VIEW" )
        {
          FField.ElemCategory=cecCode;
          FField.ElemType=etValidatorType;
        };
        if (FField.ReferCode == "AIRP_TERMINALS" && FField.ReferName == "NAME" )
        {
          FField.ElemCategory=cecName;
          FField.ElemType=etAirpTerminal;
        };
        if (FField.ReferCode == "BI_HALLS_AND_TERMINALS" && FField.ReferName == "TERMINAL_VIEW" )
        {
          FField.ElemCategory=cecName;
          FField.ElemType=etAirpTerminal;
        };
        if (FField.ReferCode == "BI_HALLS_AND_TERMINALS" && FField.ReferName == "HALL_VIEW" )
        {
          FField.ElemCategory=cecName;
          FField.ElemType=etBIHall;
        };

        FFields.push_back(FField);
        Qry->Next();
    }
    sort(FFields.begin(),FFields.end(),lf);
    for (vector<TCacheField2>::iterator i=FFields.begin(); i!=FFields.end(); i++ ) {
        ProgTrace( TRACE5, "cache field: name=%s, num=%d, read_only=%d", i->Name.c_str(), i->num, i->ReadOnly );
    }
}

void TCacheTable::DeclareSysVariables(std::vector<string> &vars, TQuery *Qry)
{
    vector<string>::iterator f;
    // ??????? ??६????? SYS_user_id
    f = find( vars.begin(), vars.end(), "SYS_USER_ID" );
    if ( f != vars.end() ) {
      Qry->DeclareVariable("SYS_user_id", otInteger);
      Qry->SetVariable( "SYS_user_id", TReqInfo::Instance()->user.user_id );
      vars.erase( f );
    }

    // ??????? ??६????? SYS_user_descr
    f = find( vars.begin(), vars.end(), "SYS_USER_DESCR" );
    if ( f != vars.end() ) {
      Qry->DeclareVariable("SYS_user_descr", otString);
      Qry->SetVariable( "SYS_user_descr", TReqInfo::Instance()->user.descr );
      vars.erase( f );
    }

    // ??????? ??६????? SYS_canon_name
    f = find( vars.begin(), vars.end(), "SYS_CANON_NAME" );
    if ( f != vars.end() ) {
      Qry->DeclareVariable("SYS_canon_name", otString);
      Qry->SetVariable( "SYS_canon_name", OWN_CANON_NAME() );
      vars.erase( f );
    }

    // ??????? ??६????? SYS_point_addr
    f = find( vars.begin(), vars.end(), "SYS_POINT_ADDR" );
    if ( f != vars.end() ) {
      Qry->DeclareVariable("SYS_point_addr", otString);
      Qry->SetVariable( "SYS_point_addr", OWN_POINT_ADDR() );
      vars.erase( f );
    }

    f = find( vars.begin(), vars.end(), "SYS_DESK_LANG" );
    if ( f != vars.end() ) {
      Qry->DeclareVariable("SYS_desk_lang", otString);
      Qry->SetVariable( "SYS_desk_lang", TReqInfo::Instance()->desk.lang );
      vars.erase( f );
    }

    f = find( vars.begin(), vars.end(), "SYS_DESK_CODE" );
    if ( f != vars.end() ) {
      Qry->DeclareVariable("SYS_desk_code", otString);
      Qry->SetVariable( "SYS_desk_code", TReqInfo::Instance()->desk.code );
      vars.erase( f );
    }

    f = find( vars.begin(), vars.end(), "SYS_DESK_VERSION" );
    if ( f != vars.end() ) {
      Qry->DeclareVariable("SYS_desk_version", otString);
      Qry->SetVariable( "SYS_desk_version", TReqInfo::Instance()->desk.version );
      vars.erase( f );
    }
};

string get_profile_name(int profile_id, TQuery &Qry)
{
  ostringstream res;
  const char* sql="SELECT name,airline,airp FROM airline_profiles WHERE profile_id=:profile_id";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("profile_id", otInteger);
  };
  Qry.SetVariable("profile_id", profile_id);
  Qry.Execute();
  if (!Qry.Eof)
  {
    res << Qry.FieldAsString("name");
    if (!Qry.FieldIsNULL("airline") || !Qry.FieldIsNULL("airp"))
    {
      res << " (" << ElemIdToCodeNative(etAirline, Qry.FieldAsString("airline"));
      if (!Qry.FieldIsNULL("airline") && !Qry.FieldIsNULL("airp")) res << "+";
      res << ElemIdToCodeNative(etAirp, Qry.FieldAsString("airp")) << ")";
    };
  };
  return res.str();
};

string get_role_name(int role_id, TQuery &Qry)
{
  ostringstream res;
  const char* sql="SELECT name,airline,airp FROM roles WHERE role_id=:role_id";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("role_id", otInteger);
  };
  Qry.SetVariable("role_id", role_id);
  Qry.Execute();
  if (!Qry.Eof)
  {
    res << Qry.FieldAsString("name");
    if (!Qry.FieldIsNULL("airline") || !Qry.FieldIsNULL("airp"))
    {
      res << " (" << ElemIdToCodeNative(etAirline, Qry.FieldAsString("airline"));
      if (!Qry.FieldIsNULL("airline") && !Qry.FieldIsNULL("airp")) res << "+";
      res << ElemIdToCodeNative(etAirp, Qry.FieldAsString("airp")) << ")";
    };
  };
  return res.str();
};

string get_user_airlines_airps(int user_id, TUserType user_type,
                               TQuery &Qry1, TQuery &Qry2)
{
  ostringstream res;

  const char* sql1="SELECT airline FROM aro_airlines WHERE aro_id=:user_id";
  if (strcmp(Qry1.SQLText.SQLText(),sql1)!=0)
  {
    Qry1.Clear();
    Qry1.SQLText=sql1;
    Qry1.DeclareVariable("user_id", otInteger);
  };
  Qry1.SetVariable("user_id", user_id);
  Qry1.Execute();
  vector<string> airlines;
  for(;!Qry1.Eof;Qry1.Next())
    airlines.push_back(ElemIdToCodeNative(etAirline, Qry1.FieldAsString("airline")));
  sort(airlines.begin(), airlines.end());

  vector<string>::iterator a;
  string airlines_str;
  a=airlines.begin();
  for(int i=0;a!=airlines.end();a++,i++)
  {
    if (i!=0) airlines_str+=' ';
    airlines_str+=*a;
    if (airlines_str.size()>11) break;
  };
  if (a!=airlines.end()) airlines_str+="...";
  if (airlines_str.empty() && user_type==utAirline) airlines_str='?';

  const char* sql2="SELECT airp FROM aro_airps WHERE aro_id=:user_id";
  if (strcmp(Qry2.SQLText.SQLText(),sql2)!=0)
  {
    Qry2.Clear();
    Qry2.SQLText=sql2;
    Qry2.DeclareVariable("user_id", otInteger);
  };
  Qry2.SetVariable("user_id", user_id);
  Qry2.Execute();
  vector<string> airps;
  for(;!Qry2.Eof;Qry2.Next())
    airps.push_back(ElemIdToCodeNative(etAirp, Qry2.FieldAsString("airp")));
  sort(airps.begin(), airps.end());

  string airps_str;
  a=airps.begin();
  for(int i=0;a!=airps.end();a++,i++)
  {
    if (i!=0) airps_str+=' ';
    airps_str+=*a;
    if (airps_str.size()>11) break;
  };
  if (a!=airps.end()) airps_str+="...";
  if (airps_str.empty() && user_type==utAirport) airps_str='?';

  if (user_type==utSupport || user_type==utAirline)
  {
    res << airlines_str;
    if (!airlines_str.empty() && !airps_str.empty()) res << " + ";
    res << airps_str;
  }
  else
  {
    res << airps_str;
    if (!airlines_str.empty() && !airps_str.empty()) res << " + ";
    res << airlines_str;
  };
  return res.str();
};

string get_user_descr(int user_id,
                      TQuery &Qry, TQuery &Qry1, TQuery &Qry2,
                      bool only_airlines_airps)
{
  const char* sql="SELECT type, descr FROM users2 WHERE user_id=:user_id";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("user_id", otInteger);
  };
  Qry.SetVariable("user_id", user_id);
  Qry.Execute();
  if (Qry.Eof) return "";

  string user_access=get_user_airlines_airps(user_id, (TUserType)Qry.FieldAsInteger("type"),
                                             Qry1, Qry2);

  ostringstream res;
  if (!only_airlines_airps)
  {
    res << Qry.FieldAsString("descr");
    if (!user_access.empty())
      res << " (" << user_access << ")";
  }
  else
    res << user_access;

  return res.str();
};

TUpdateDataType TCacheTable::refreshData()
{
    Clear();
    string code = Params[TAG_CODE].Value;

    string stid = Params[ TAG_REFRESH_DATA ].Value;

    TrimString( stid );
    if ( stid.empty() || StrToInt( stid.c_str(), clientVerData ) == EOF )
      clientVerData = -1; /* ?? ?????? ??????, ?????? ????????? ?ᥣ?? ??? */
    ProgTrace(TRACE5, "Client version data: %d", clientVerData );

    /*???஡㥬 ????? ? ?????? ??६????? :user_id
      ??⮬? ??? ???????? ?⥭?? ?????? ?? ??।. ???????????? */
    TCacheQueryType query_type;
    vars.clear();
    Qry->Clear();
    vector<string>::iterator f;
    if ( RefreshSQL.empty() || clientVerData < 0 ) { /* ????뢠?? ??? ?????? */
      Qry->SQLText = SelectSQL;
      query_type = cqtSelect;
      FindVariables(Qry->SQLText.SQLText(), false, vars);
      clientVerData = -1;
    }
    else { /* ??????塞 ? ?ᯮ?짮?????? RefreshSQL ? clientVerData */
      Qry->SQLText = RefreshSQL;
      query_type = cqtRefresh;
      /* ?뤥????? ???? ??६????? ??? ??????? */
      FindVariables( Qry->SQLText.SQLText(), false, vars );
      /* ??????? ??६????? TID */
      f = find( vars.begin(), vars.end(), "TID" );
      if ( f != vars.end() ) {
        Qry->DeclareVariable( "tid", otInteger );
        Qry->SetVariable( "tid", clientVerData );
        ProgTrace( TRACE5, "set clientVerData: tid variable %d", clientVerData );
        vars.erase( f );
      }
    }

    DeclareSysVariables(vars,Qry);

    /* ?஡?? ?? ??६????? ? ???????, ??譨? ??६?????, ??????? ???諨 ?? ????뢠?? */
    for(vector<string>::iterator v = vars.begin(); v != vars.end(); v++ )
    {
        otFieldType vtype;
        switch( SQLParams[ *v ].DataType ) {
          case ctInteger: vtype = otInteger;
                          break;
          case ctDouble: vtype = otFloat;
                 break;
          case ctDateTime: vtype = otDate;
                           break;
          default: vtype = otString;
        }
        Qry->DeclareVariable( *v, vtype );
        if ( !SQLParams[ *v ].Value.empty() )
          Qry->SetVariable( *v, SQLParams[ *v ].Value );
        else
          Qry->SetVariable( *v, FNull );
        ProgTrace( TRACE5, "variable %s = %s, type=%i", v->c_str(),
                   SQLParams[ *v ].Value.c_str(), vtype );
    }

    if(OnBeforeRefresh)
      try {
          (*OnBeforeRefresh)(*this, *Qry, query_type);
      } catch(const UserException &E) {
          throw;
      } catch(const Exception &E) {
          ProgError(STDLOG, "OnBeforeRefresh failed: %s", E.what());
          throw;
      } catch(...) {
          ProgError(STDLOG, "OnBeforeRefresh failed: something unexpected");
          throw;
      }

    ProgTrace(TRACE5, "SQLText=%s", Qry->SQLText.SQLText());
    Qry->Execute();
    // ?饬, ?⮡? ??? ????, ??????? ???ᠭ? ? ???? ?뫨 ? ???????
    vector<int> vecFieldIdx;
    for(vector<TCacheField2>::iterator i = FFields.begin(); i != FFields.end(); i++)
    {
        int FieldIdx = Qry->GetFieldIndex(i->Name);
        if( FieldIdx < 0)
        {
          if (i->ElemCategory==cecCode ||
              i->ElemCategory==cecName ||
              i->ElemCategory==cecNameShort)
          {
            //?஢?ਬ - ???? ????? ???? _VIEW
            if (i->Name.size()>5 && i->Name.substr(i->Name.size()-5)=="_VIEW")
              FieldIdx = Qry->GetFieldIndex(i->Name.substr(0,i->Name.size()-5));
          };

          if( FieldIdx < 0)
            throw Exception("Field '" + code + "." + i->Name + "' not found in select_sql");
        };
        vecFieldIdx.push_back( FieldIdx );
    }

    int tidIdx = Qry->GetFieldIndex("TID");
    int delIdx = Qry->GetFieldIndex("PR_DEL");
    if ( clientVerData >= 0 && delIdx < 0 )
        throw Exception( "Field '" + code +".PR_DEL' not found");

    TQuery TempQry1(&OraSession);
    TQuery TempQry2(&OraSession);
    TQuery TempQry3(&OraSession);

    bool trip_bag_norms=(TCacheTable::code()=="TRIP_BAG_NORMS");
    TRow tmp_row;

    //??⠥? ???
    for(; !Qry->Eof; Qry->Next())
    {
      if( tidIdx >= 0 && Qry->FieldAsInteger( tidIdx ) > clientVerData )
        clientVerData = Qry->FieldAsInteger( tidIdx );
      TRow local_row;
      TRow &row=(trip_bag_norms && Qry->FieldAsInteger("id")==1000000000)?tmp_row:local_row;
      int j=0;
      for(vector<TCacheField2>::iterator i = FFields.begin(); i != FFields.end(); i++,j++)
      {
        if(Qry->FieldIsNULL(vecFieldIdx[ j ] ))
          row.cols.push_back( "" );
        else {
            switch( i->DataType ) {
              case ftSignedNumber:
              case ftUnsignedNumber:
                if ( i->Scale > 0 || i->DataSize > 9 )
                  row.cols.push_back( Qry->FieldAsString( vecFieldIdx[ j ] ) );
                else
                  row.cols.push_back( IntToString( Qry->FieldAsInteger( vecFieldIdx[ j ] ) ) );
                break;
              case ftBoolean:
                  row.cols.push_back( IntToString( (int)(Qry->FieldAsInteger( vecFieldIdx[ j ] ) !=0 ) ) );
                break;
              default:
                if (i->ElemCategory!=cecNone)
                {
                  int int_id=ASTRA::NoExists;
                  if (Qry->FieldType( vecFieldIdx[ j ] ) == otInteger)
                    int_id=Qry->FieldAsInteger( vecFieldIdx[ j ] );
                  else
                  {
                    if (Qry->FieldType( vecFieldIdx[ j ] ) == otFloat && i->Scale ==0) //?????? ?? ?祭? ?ࠢ??쭮 ??।??????? i->DataSize
                    {
                      double f=Qry->FieldAsFloat( vecFieldIdx[ j ] );
                      if ((f>-1E9) && (f<1E9)) int_id=(int)f;
                    };
                  };

                  switch (i->ElemCategory)
                  {
                    case cecCode: if (int_id!=ASTRA::NoExists)
                                    row.cols.push_back( ElemIdToCodeNative( i->ElemType, int_id ) );
                                  else
                                    row.cols.push_back( ElemIdToCodeNative( i->ElemType, Qry->FieldAsString(vecFieldIdx[ j ]) ) );
                                  break;
                    case cecName: if (int_id!=ASTRA::NoExists)
                                    row.cols.push_back( ElemIdToNameLong( i->ElemType, int_id ) );
                                  else
                                    row.cols.push_back( ElemIdToNameLong( i->ElemType, Qry->FieldAsString(vecFieldIdx[ j ]) ) );
                                  break;
               case cecNameShort: if (int_id!=ASTRA::NoExists)
                                    row.cols.push_back( ElemIdToNameShort( i->ElemType, int_id ) );
                                  else
                                    row.cols.push_back( ElemIdToNameShort( i->ElemType, Qry->FieldAsString(vecFieldIdx[ j ]) ) );
                                  break;
                case cecProfileName: if (int_id!=ASTRA::NoExists)
                                    row.cols.push_back( get_profile_name(int_id, TempQry1));
                                  else
                                    row.cols.push_back( Qry->FieldAsString(vecFieldIdx[ j ]) );
                                  break;
                case cecRoleName: if (int_id!=ASTRA::NoExists)
                                    row.cols.push_back( get_role_name(int_id, TempQry1));
                                  else
                                    row.cols.push_back( Qry->FieldAsString(vecFieldIdx[ j ]) );
                                  break;
                case cecUserName:
               case cecUserPerms: if (int_id!=ASTRA::NoExists)
                                    row.cols.push_back( get_user_descr(int_id, TempQry1, TempQry2, TempQry3, i->ElemCategory==cecUserPerms));
                                  else
                                    row.cols.push_back( Qry->FieldAsString(vecFieldIdx[ j ]) );
                                  break;
                         default: row.cols.push_back( Qry->FieldAsString(vecFieldIdx[ j ]) );
                                  break;
                  };
                }
                else row.cols.push_back( Qry->FieldAsString(vecFieldIdx[ j ]) );
                break;
            }
        }
      }
      if(delIdx >= 0 &&  Qry->FieldAsInteger(delIdx) != 0)
        row.status = usDeleted;
      else
        row.status = usUnmodified;
      if (!(trip_bag_norms && Qry->FieldAsInteger("id")==1000000000))
        table.push_back(row);
    }

    if (trip_bag_norms && !table.empty() && !tmp_row.cols.empty())
    {
      vector<TCacheField2>::const_iterator i = FFields.begin();
      std::vector<std::string>::iterator j = tmp_row.cols.begin();
      std::vector<std::string>::const_iterator k = table.begin()->cols.begin();

      for(; i != FFields.end() && j != tmp_row.cols.end() && k != table.begin()->cols.end(); ++i, ++j, ++k)
        if (i->Name=="AIRLINE" || i->Name=="AIRLINE_VIEW")
          *j=*k;
      table.push_back(tmp_row);
    }

    ProgTrace( TRACE5, "Server version data: %d", clientVerData );
    if ( !table.empty() ) // ????⠫? ?????????
        return upExists;
    else
        if ( clientVerData >= 0 ) // ??? ?????????
            return upNone;
        else
            return upClearAll; // ??? 㤠????
}

void TCacheTable::refresh()
{
    if(Params.find(TAG_REFRESH_INTERFACE) != Params.end()) {
        pr_irefresh = refreshInterface();
        if ( pr_irefresh )
          Params[ TAG_REFRESH_DATA ].Value.clear();
    }
    else
        pr_irefresh = false;
    if ( (Params.find(TAG_REFRESH_DATA) != Params.end() &&
            !pr_dconst) ||
           pr_irefresh ) {
        if ( pr_irefresh )
          clientVerData = -1;
        refresh_data_type = refreshData();
    }
    else
        refresh_data_type = upNone;
}

void TCacheTable::buildAnswer(xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    NewTextChild( dataNode, "code", code() );
    NewTextChild(dataNode, "Forbidden", Forbidden);
    NewTextChild(dataNode, "ReadOnly", ReadOnly);
    NewTextChild(dataNode, "keep_locally", KeepLocally );
    NewTextChild(dataNode, "keep_deleted_rows", KeepDeletedRows );
    vector<string> sql_vars;
    bool user_depend = false;
    if (!user_depend)
    {
      FindVariables(SelectSQL, false, sql_vars);
      if ( find( sql_vars.begin(), sql_vars.end(), "SYS_USER_ID" ) != sql_vars.end() )
        user_depend = true;
    };
    if (!user_depend)
    {
      FindVariables(RefreshSQL, false, sql_vars);
      if ( find( sql_vars.begin(), sql_vars.end(), "SYS_USER_ID" ) != sql_vars.end() )
        user_depend = true;
    };
    NewTextChild( dataNode, "user_depend", (int)user_depend );

    if(pr_irefresh)
        XMLInterface(dataNode);

    if ( refresh_data_type != upNone || pr_irefresh )
        XMLData(dataNode);
}

void TCacheTable::XMLInterface(const xmlNodePtr dataNode)
{
    xmlNodePtr ifaceNode = NewTextChild(dataNode, "iface");

    NewTextChild(ifaceNode, "title", AstraLocale::getLocaleText(  Title ) );
    NewTextChild(ifaceNode, "CanRefresh", !RefreshSQL.empty());
    NewTextChild(ifaceNode, "CanInsert", !(InsertSQL.empty()||InsertRight<0) );
    NewTextChild(ifaceNode, "CanUpdate", !(UpdateSQL.empty()||UpdateRight<0) );
    NewTextChild(ifaceNode, "CanDelete", !(DeleteSQL.empty()||DeleteRight<0) );

    if(not FChildTables.empty()) {
        xmlNodePtr tablesNode = NewTextChild(ifaceNode, "child_tables");
        for(vector<TCacheChildTable>::const_iterator t = FChildTables.begin(); t != FChildTables.end(); ++t) {
            xmlNodePtr tableNode = NewTextChild(tablesNode, "child_table");
            NewTextChild(tableNode, "code", t->code);
            NewTextChild(tableNode, "title", AstraLocale::getLocaleText(t->title));
            xmlNodePtr fieldsNode = NewTextChild(tableNode, "fields");
            for(vector<TCacheChildField>::const_iterator f = t->fields.begin(); f != t->fields.end(); ++f) {
                xmlNodePtr fieldNode = NewTextChild(fieldsNode, "field");
                NewTextChild(fieldNode, "field_parent", f->field_parent);
                NewTextChild(fieldNode, "field_child", f->field_child);
                NewTextChild(fieldNode, "select_var", f->select_var);
                NewTextChild(fieldNode, "insert_var", f->insert_var);
                NewTextChild(fieldNode, "update_var", f->update_var);
                NewTextChild(fieldNode, "delete_var", f->delete_var);
                NewTextChild(fieldNode, "auto_insert", (int)f->auto_insert);
                NewTextChild(fieldNode, "check_equal", (int)f->check_equal);
                NewTextChild(fieldNode, "read_only", (int)f->read_only);
            }
        }
    }

    xmlNodePtr ffieldsNode = NewTextChild(ifaceNode, "fields");
    SetProp( ffieldsNode, "tid", curVerIface );
    int i = 0;
    for(vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++) {
        xmlNodePtr ffieldNode = NewTextChild(ffieldsNode, "field");

        SetProp(ffieldNode, "index", i++);

        NewTextChild(ffieldNode, "Name", iv->Name);

        NewTextChild( ffieldNode, "Title", AstraLocale::getLocaleText( iv->Title ) );
        NewTextChild( ffieldNode, "Width", iv->Width );
        const char *charCase;
        switch(iv->CharCase) {
            case ecLowerCase:
                charCase = "L";
                break;
            case ecUpperCase:
                charCase = "U";
                break;
            default:
                charCase = "";
        }
        NewTextChild(ffieldNode, "CharCase",     charCase);
        NewTextChild(ffieldNode, "Align",        iv->Align);
        NewTextChild(ffieldNode, "DataType",     iv->DataType);
        NewTextChild(ffieldNode, "DataSize",     iv->DataSize);
        NewTextChild(ffieldNode, "Scale",        iv->Scale);
        NewTextChild(ffieldNode, "Nullable",     iv->Nullable);
        NewTextChild(ffieldNode, "Ident",        iv->Ident);
        NewTextChild(ffieldNode, "ReadOnly",     iv->ReadOnly);
        NewTextChild(ffieldNode, "ReferCode",    iv->ReferCode);
        NewTextChild(ffieldNode, "ReferName",    iv->ReferName);
        NewTextChild(ffieldNode, "ReferLevel",   iv->ReferLevel);
        NewTextChild(ffieldNode, "ReferIdent",   iv->ReferIdent);
    }
}

void TCacheTable::XMLData(const xmlNodePtr dataNode)
{
    xmlNodePtr tabNode = NewTextChild(dataNode, "rows");
    SetProp( tabNode, "tid", clientVerData );
    for(TTable::iterator it = table.begin(); it != table.end(); it++) {
        xmlNodePtr rowNode = NewTextChild(tabNode, "row");
        SetProp(rowNode, "pr_del", (it->status == usDeleted));
        int colidx = 0;
        for(vector<string>::iterator ir = it->cols.begin(); ir != it->cols.end(); ir++,colidx++) {
            NewTextChild(rowNode, "col", ir->c_str());
        }
    }
}

void TCacheTable::parse_updates(xmlNodePtr rowsNode)
{
    table.clear();
    if(rowsNode == NULL)
        throw Exception("wrong message format");
    xmlNodePtr rowNode = rowsNode->children;
    while(rowNode) {
        TRow row;
        getParams( GetNode( "sqlparams", rowNode ), row.params ); /* ??६????? sql ??????? */
        xmlNodePtr statusNode = GetNode("@status", rowNode);
        string status;
        if(statusNode != NULL)
            status = NodeAsString(statusNode);
        if(status == "inserted")
            row.status = usInserted;
        else if(status == "deleted")
            row.status = usDeleted;
        else if(status == "modified")
            row.status = usModified;
        else
            row.status = usUnmodified;
        for (int i=0; i<(int)FFields.size(); i++) { /* ?஡?? ?? ????? */
          string strnode = "col[@index ='"+IntToString(i)+"']";
          switch(row.status) {
            case usModified:
              row.old_cols.push_back( NodeAsString(string(strnode + "/old").c_str(), rowNode) );
              row.cols.push_back( NodeAsString(string(strnode + "/new").c_str(), rowNode) );
              break;
            case usDeleted:
              row.old_cols.push_back( NodeAsString(strnode.c_str(), rowNode) );
              break;
            case usInserted:
              row.cols.push_back( NodeAsString(strnode.c_str(), rowNode) );
              break;
            default:;
          }
        }
        table.push_back(row);
        rowNode = rowNode->next;
    }
}

string TCacheTable::code()
{
  if ( Params.find( TAG_CODE ) == Params.end() )
    throw Exception("cache not inizialize");
  return Params[TAG_CODE].Value;
}

int TCacheTable::FieldIndex( const string name )
{
  string strn = name;
  strn = upperc( strn );
  int FieldId = 0;
  vector<TCacheField2>::iterator i;
  for(i=FFields.begin();i!=FFields.end();i++,FieldId++)
    if ( strn == i->Name ) break;
  if (i==FFields.end()) FieldId = -1;
  return FieldId;
}

string TCacheTable::FieldValue( const string name, const TRow &row )
{
  int idx = FieldIndex(name);
  if ( idx < 0 || idx >=(int)row.cols.size() )
    throw Exception( "TCacheTable::FieldValue: field '%s' not found", name.c_str() );
  return row.cols[idx];
};

string TCacheTable::FieldOldValue( const string name, const TRow &row )
{
  int idx = FieldIndex(name);
  if ( idx < 0 || idx >=(int)row.old_cols.size() )
    throw Exception( "TCacheTable::FieldOldValue: field '%s' not found", name.c_str() );
  return row.old_cols[idx];
};

void OnLoggingF( TCacheTable &cache, const TRow &row, TCacheUpdateStatus UpdateStatus,
                 TLogLocale &tlocale )
{
  string code = cache.code();
  ostringstream msg;
  tlocale.ev_type = evtFlt;
  int point_id;
  if ( UpdateStatus == usInserted ) {
    TParams::const_iterator ip = row.params.find( "POINT_ID" );
    if ( ip == row.params.end() )
      throw Exception( "Can't find variable point_id" );
    point_id = ToInt( ip->second.Value );
  }
  else {
    point_id = ToInt( cache.FieldOldValue("point_id", row) );
  }
  tlocale.id1 = point_id;
  if (code == "TRIP_BP"
      || code == "TRIP_BI"
      || code == "TRIP_VO"
      || code == "TRIP_EMDA"
      )
  {
      string del_cls_lexeme;
      string del_lexeme;
      string ins_cls_lexeme;
      string ins_lexeme;
      string modify_lexeme;
      if(code == "TRIP_BP") {
          del_cls_lexeme = "EVT.BP_FORM_DELETED_FOR_CLASS";
          del_lexeme =     "EVT.BP_FORM_DELETED";
          ins_cls_lexeme = "EVT.BP_FORM_INSERTED_FOR_CLASS";
          ins_lexeme =     "EVT.BP_FORM_INSERTED";
          modify_lexeme =  "EVT.BP_FORM_MODIFIED";
      } else if(code == "TRIP_BI") {
          del_cls_lexeme = "EVT.BI_FORM_DELETED_FOR_CLASS";
          del_lexeme =     "EVT.BI_FORM_DELETED";
          ins_cls_lexeme = "EVT.BI_FORM_INSERTED_FOR_CLASS";
          ins_lexeme =     "EVT.BI_FORM_INSERTED";
          modify_lexeme =  "EVT.BI_FORM_MODIFIED";
      } else if(code == "TRIP_VO") {
          del_cls_lexeme = "EVT.VO_FORM_DELETED_FOR_CLASS";
          del_lexeme =     "EVT.VO_FORM_DELETED";
          ins_cls_lexeme = "EVT.VO_FORM_INSERTED_FOR_CLASS";
          ins_lexeme =     "EVT.VO_FORM_INSERTED";
          modify_lexeme =  "EVT.VO_FORM_MODIFIED";
      } else if(code == "TRIP_EMDA") {
          del_cls_lexeme = "EVT.EMDA_FORM_DELETED_FOR_CLASS";
          del_lexeme =     "EVT.EMDA_FORM_DELETED";
          ins_cls_lexeme = "EVT.EMDA_FORM_INSERTED_FOR_CLASS";
          ins_lexeme =     "EVT.EMDA_FORM_INSERTED";
          modify_lexeme =  "EVT.EMDA_FORM_MODIFIED";
      }

    if (UpdateStatus == usDeleted)
    {
      if (!cache.FieldOldValue( "class", row ).empty())
      {
        tlocale.lexema_id = del_cls_lexeme;
        tlocale.prms << PrmSmpl<string>("old_name", cache.FieldOldValue("bp_name", row))
                     << PrmElem<string>("old_cls", etClass, cache.FieldOldValue("class", row));
      }
      else
      {
        tlocale.lexema_id = del_lexeme;
        tlocale.prms << PrmSmpl<string>("old_name", cache.FieldOldValue("bp_name", row));
      }
    }
    else if (UpdateStatus == usInserted)
    {
      if (!cache.FieldValue( "class", row ).empty())
      {
        tlocale.lexema_id = ins_cls_lexeme;
        tlocale.prms << PrmSmpl<string>("name", cache.FieldValue("bp_name", row))
                     << PrmElem<string>("cls", etClass, cache.FieldValue("class", row));
      }
      else
      {
        tlocale.lexema_id = ins_lexeme;
        tlocale.prms << PrmSmpl<string>("name", cache.FieldValue("bp_name", row));
      }
    }
    else if (UpdateStatus == usModified)
    {
      tlocale.lexema_id = modify_lexeme;
      if (!cache.FieldOldValue( "class", row ).empty())
      {
        PrmLexema old_form("old_form", del_cls_lexeme);
        old_form.prms << PrmSmpl<string>("old_name", cache.FieldOldValue("bp_name", row))
                     << PrmElem<string>("old_cls", etClass, cache.FieldOldValue("class", row));
        tlocale.prms << old_form;
      }
      else
      {
        PrmLexema old_form("old_form", del_lexeme);
        old_form.prms << PrmSmpl<string>("old_name", cache.FieldOldValue("bp_name", row));
        tlocale.prms << old_form;
      }
      if (!cache.FieldValue( "class", row ).empty())
      {
        PrmLexema new_form("new_form", ins_cls_lexeme);
        new_form.prms << PrmSmpl<string>("name", cache.FieldValue("bp_name", row))
                     << PrmElem<string>("cls", etClass, cache.FieldValue("class", row));
        tlocale.prms << new_form;
      }
      else
      {
        PrmLexema new_form("new_form", ins_lexeme);
        new_form.prms << PrmSmpl<string>("name", cache.FieldValue("bp_name", row));
        tlocale.prms << new_form;
      }
    }
  }
  if ( code == "TRIP_BT" )
  {
    if (UpdateStatus == usDeleted )
    {
      tlocale.lexema_id = "EVT.BT_FORM_DELETED";
      tlocale.prms << PrmSmpl<string>("name", cache.FieldOldValue("bt_name", row));
    }
    else if ( UpdateStatus == usInserted)
    {
      tlocale.lexema_id = "EVT.BT_FORM_INSERTED";
      tlocale.prms << PrmSmpl<string>("name", cache.FieldValue("bt_name", row));
    }
    else if ( UpdateStatus == usModified)
    {
        tlocale.lexema_id = "EVT.BT_FORM_MODIFIED";
        tlocale.prms << PrmSmpl<string>("old_name", cache.FieldOldValue("bt_name", row))
                     << PrmSmpl<string>("name", cache.FieldValue("bt_name", row));
    }
  }
  if ( code == "TRIP_BRD_WITH_REG" ||
       code == "TRIP_EXAM_WITH_BRD" )
  {
    if (UpdateStatus == usDeleted)
    {
      tlocale.prms << PrmLexema("action", "EVT.MODE_DELETED");
      if (code == "TRIP_BRD_WITH_REG")
      {
        if ( ToInt(cache.FieldOldValue( "pr_misc", row )) == 0 )
          tlocale.lexema_id = "EVT.TRIP_SEPARATE_BRD_AND_REG";
        else
          tlocale.lexema_id = "EVT.TRIP_BRD_AND_REG";
      }
      else
      {
        if ( ToInt(cache.FieldOldValue( "pr_misc", row )) == 0 )
          tlocale.lexema_id = "EVT.TRIP_SEPARATE_EXAM_AND_BRD";
        else
          tlocale.lexema_id = "EVT.TRIP_EXAM_AND_BRD";
      }
      if ( !cache.FieldOldValue( "hall", row ).empty() )
      {
          PrmLexema lexema("hall", "EVT.FOR_HALL");
          lexema.prms <<  PrmSmpl<string>("hall", cache.FieldOldValue( "hall_view", row ));
          tlocale.prms << lexema;
      }
      else
          tlocale.prms << PrmSmpl<string>("hall", "");
    }
    else if (UpdateStatus == usInserted)
    {
        tlocale.prms << PrmLexema("action", "EVT.MODE_INSERTED");
        if (code == "TRIP_BRD_WITH_REG")
        {
          if ( ToInt(cache.FieldValue( "pr_misc", row )) == 0 )
            tlocale.lexema_id = "EVT.TRIP_SEPARATE_BRD_AND_REG";
          else
            tlocale.lexema_id = "EVT.TRIP_BRD_AND_REG";
        }
        else
        {
          if ( ToInt(cache.FieldValue( "pr_misc", row )) == 0 )
            tlocale.lexema_id = "EVT.TRIP_SEPARATE_EXAM_AND_BRD";
          else
            tlocale.lexema_id = "EVT.TRIP_EXAM_AND_BRD";
        }
        if ( !cache.FieldValue( "hall", row ).empty() )
        {
            PrmLexema lexema("hall", "EVT.FOR_HALL");
            lexema.prms <<  PrmSmpl<string>("hall", cache.FieldValue( "hall_view", row ));
            tlocale.prms << lexema;
        }
        else
            tlocale.prms << PrmSmpl<string>("hall", "");
    }
    else if (UpdateStatus == usModified)
    {
      tlocale.lexema_id = "EVT.MODE_MODIFIED";
      PrmLexema old_mode("old_mode", "EVT.TRIP_BRD_AND_REG");
      old_mode.prms << PrmLexema("action", "EVT.MODE_DELETED");

      PrmLexema new_mode("new_mode", "EVT.TRIP_BRD_AND_REG");
      new_mode.prms << PrmLexema("action", "EVT.MODE_INSERTED");
      if (code == "TRIP_BRD_WITH_REG")
      {
        if ( ToInt(cache.FieldOldValue( "pr_misc", row )) == 0 )
          old_mode.ChangeLexemaId("EVT.TRIP_SEPARATE_BRD_AND_REG");
        if ( ToInt(cache.FieldValue( "pr_misc", row )) == 0 )
          new_mode.ChangeLexemaId("EVT.TRIP_SEPARATE_BRD_AND_REG");
      }
      else
      {
        if ( ToInt(cache.FieldOldValue( "pr_misc", row )) == 0 )
          old_mode.ChangeLexemaId("EVT.TRIP_SEPARATE_EXAM_AND_BRD");
        else
          old_mode.ChangeLexemaId("EVT.TRIP_EXAM_AND_BRD");
        if ( ToInt(cache.FieldValue( "pr_misc", row )) == 0 )
          new_mode.ChangeLexemaId("EVT.TRIP_SEPARATE_EXAM_AND_BRD");
        else
          new_mode.ChangeLexemaId("EVT.TRIP_EXAM_AND_BRD");
      };
      if ( !cache.FieldOldValue("hall", row ).empty())
      {
        PrmLexema lexema("hall", "EVT.FOR_HALL");
        lexema.prms << PrmSmpl<string>("hall", cache.FieldOldValue( "hall_view", row ));
        old_mode.prms << lexema;
      }
      else
          old_mode.prms << PrmSmpl<string>("hall", "");
      if ( !cache.FieldValue("hall", row ).empty())
      {
        PrmLexema lexema("hall", "EVT.FOR_HALL");
        lexema.prms << PrmSmpl<string>("hall", cache.FieldValue( "hall_view", row ));
        new_mode.prms << lexema;
      }
      else
        new_mode.prms << PrmSmpl<string>("hall", "");
      tlocale.prms << new_mode << old_mode;
    }
  }
  if ( code == "TRIP_WEB_CKIN" ||
       code == "TRIP_KIOSK_CKIN" ||
       code == "TRIP_PAID_CKIN" ) {
    if ( (UpdateStatus == usDeleted || ToInt(cache.FieldValue( "pr_permit", row )) == 0) &&
         (UpdateStatus == usInserted || ToInt(cache.FieldOldValue( "pr_permit", row )) == 0) )
    {
        tlocale.lexema_id.clear(); //?? ?????뢠?? ? ???
    }
    else
    {
      tlocale.lexema_id = "EVT.TRIP_CKIN";
      if ((UpdateStatus == usInserted || UpdateStatus == usModified) &&
          ToInt(cache.FieldValue( "pr_permit", row )) != 0)
      {
        if ( code == "TRIP_WEB_CKIN" ||
             code == "TRIP_KIOSK_CKIN" )
          tlocale.prms << PrmLexema("action", "EVT.CKIN_ALLOWED");
        else
          tlocale.prms << PrmLexema("action", "EVT.CKIN_PERFORMED");
      }
      else
      {
        if ( code == "TRIP_WEB_CKIN" ||
             code == "TRIP_KIOSK_CKIN" )
          tlocale.prms << PrmLexema("action", "EVT.CKIN_NOT_ALLOWED");
        else
          tlocale.prms << PrmLexema("action", "EVT.CKIN_NOT_PERFORMED");
      };

      if ( code == "TRIP_WEB_CKIN") {
        PrmLexema lexema("what", "EVT.TRIP_WEB_CKIN");
        lexema.prms << PrmSmpl<string>("desk_grp", "");
        tlocale.prms << lexema;
      }
      else if ( code == "TRIP_PAID_CKIN")
        tlocale.prms << PrmLexema("what", "EVT.TRIP_PAID_CKIN");
      else if ( code == "TRIP_KIOSK_CKIN")
      {
        PrmLexema lexema("what", "EVT.TRIP_KIOSK_CKIN");
        //????? ??? desk_grp_id ?? ????? ???????? ????? UpdateStatus == usModified
        //?᫨ ??? ?? ⠪, ???????? ???? ??।??뢠??
        if (((UpdateStatus == usInserted || UpdateStatus == usModified) &&
             !cache.FieldValue( "desk_grp_id", row ).empty()) ||
            ((UpdateStatus == usDeleted) &&
             !cache.FieldOldValue( "desk_grp_id", row ).empty()))
        {
          PrmLexema desk_grp("desk_grp", "EVT.FOR_DESK_GRP");
          if (UpdateStatus == usInserted || UpdateStatus == usModified)
            desk_grp.prms << PrmElem<int>("desk_grp", etDeskGrp, ToInt(cache.FieldValue("desk_grp_id", row)), efmtNameLong);
          else
            desk_grp.prms << PrmElem<int>("desk_grp", etDeskGrp, ToInt(cache.FieldOldValue("desk_grp_id", row)), efmtNameLong);
          lexema.prms << desk_grp;
        }
        else
          lexema.prms << PrmSmpl<string>("desk_grp", "");
        tlocale.prms << lexema;
      };
      PrmLexema params("params", "EVT.PARAMS");
      if ((UpdateStatus == usInserted || UpdateStatus == usModified) &&
          ToInt(cache.FieldValue( "pr_permit", row )) != 0)
      {
        if ( code == "TRIP_WEB_CKIN" ||
             code == "TRIP_KIOSK_CKIN" )
        {
          params.prms << PrmBool("tckin", ToInt(cache.FieldValue( "pr_tckin", row )));
          params.prms << PrmBool("upd_stage", ToInt(cache.FieldValue( "pr_upd_stage", row )));
        }
        else
        {
          params.ChangeLexemaId("EVT.PROT_TIMEOUT");
          if ( !cache.FieldValue( "prot_timeout", row ).empty() ) {
            PrmLexema timeout("timeout", "EVT.TIMEOUT_VALUE");
            timeout.prms << PrmSmpl<int>("timeout", ToInt(cache.FieldValue( "prot_timeout", row )));
            params.prms << timeout;
          }
          else
            params.prms << PrmLexema("timeout", "EVT.UNKNOWN");
        };
        tlocale.prms << params;
      }
      else
      tlocale.prms << PrmSmpl<string>("params", "");
    };
  };
  return;
}

void TCacheTable::OnLogging( const TRow &row, TCacheUpdateStatus UpdateStatus )
{
  string code = this->code();
  if ( code == "TRIP_BP" ||
       code == "TRIP_BI" ||
       code == "TRIP_BT" ||
       code == "TRIP_BRD_WITH_REG" ||
       code == "TRIP_EXAM_WITH_BRD" ||
       code == "TRIP_WEB_CKIN" ||
       code == "TRIP_KIOSK_CKIN" ||
       code == "TRIP_PAID_CKIN" ) {
    TLogLocale tlocale;
    OnLoggingF( *this, row, UpdateStatus, tlocale);
    if (!tlocale.lexema_id.empty())
      TReqInfo::Instance()->LocaleToLog(tlocale);
    return;
  }
  string str1, str2, lexema_id;
  PrmEnum enum1("str1", ",");
  PrmEnum enum2("str2", ",");
  if ( UpdateStatus == usModified || UpdateStatus == usInserted ) {
    int Idx=0;
    for(vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++, Idx++) {
      if ( iv->VarIdx[0] < 0 )
        continue;
      str1.clear();
      if ( iv->Title.empty() )
        str1 += iv->Name;
      else
        str1 += iv->Title;
      str1 += "='" + string( Qry->GetVariableAsString( vars[ iv->VarIdx[ 0 ] ] ) ) + "'";
      enum1.prms << PrmSmpl<string>("", str1);
    }
  }
  for (int l=0; l<2; l++ ) {
    int Idx=0;
    bool empty = true;
    for(vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++, Idx++) {
      ProgTrace( TRACE5, "l=%d, Name=%s, Ident=%d, Idx=%d, iv->VarIdx[0]=%d, iv->VarIdx[1]=%d",
                 l, iv->Name.c_str(), iv->Ident, Idx, iv->VarIdx[0], iv->VarIdx[1] );
      if ( (!l && !iv->Ident) ||
           (UpdateStatus == usInserted && iv->VarIdx[ 0 ] < 0) ||
           (UpdateStatus != usInserted && iv->VarIdx[ 1 ] < 0) )
        continue;
      str2.clear();
      if ( !iv->Title.empty() )
        str2 += iv->Title;
      else
        str2 += iv->Name;
      if ( UpdateStatus == usInserted )
        str2 += "='" + string( Qry->GetVariableAsString( vars[ iv->VarIdx[ 0 ] ] ) ) + "'";
      else
        str2 += "='" + string( Qry->GetVariableAsString( vars[ iv->VarIdx[ 1 ] ] ) ) + "'";
      empty = false;
      ProgTrace( TRACE5, "str2=|%s|", str2.c_str() );
      enum2.prms << PrmSmpl<string>("", str2);
    }
    if (!empty)
      break;
  }
  switch( UpdateStatus ) {
    case usInserted:
           lexema_id = "EVT.TABLE.INSERT_ROW";
           break;
    case usModified:
           lexema_id = "EVT.TABLE.UPDATE_ROW";
           break;
    case usDeleted:
           lexema_id = "EVT.TABLE.DELETE_ROW";
           break;
    default:;
  }
  if (!lexema_id.empty())
      TReqInfo::Instance()->LocaleToLog(lexema_id, LEvntPrms() << PrmSmpl<string>("title", Title)
                                        << enum1 << enum2, EventType);
}

void DeclareVariablesFromParams(const std::vector<string> &vars, const TParams &SQLParams, TQuery &Qry)
{
  for ( vector<string>::const_iterator r=vars.begin(); r!=vars.end(); r++ ) {
    if ( Qry.Variables->FindVariable( r->c_str() ) == -1 ) {
      map<std::string, TParam>::const_iterator ip = SQLParams.find( *r );
      if ( ip != SQLParams.end() ) {
        ProgTrace( TRACE5, "Declare variable from SQLParams r->c_str()=%s", r->c_str() );
        switch( ip->second.DataType ) {
          case ctInteger:
            Qry.DeclareVariable( *r, otInteger );
            break;
          case ctDouble:
            Qry.DeclareVariable( *r, otFloat );
            break;
          case ctDateTime:
            Qry.DeclareVariable( *r, otDate );
            break;
          case ctString:
            Qry.DeclareVariable( *r, otString );
            break;
        }
      }
    }
  }
};

void SetVariablesFromParams(const std::vector<string> &vars, const TParams &SQLParams, TQuery &Qry)
{
  for( map<std::string, TParam>::const_iterator iv=SQLParams.begin(); iv!=SQLParams.end(); iv++ ) {
    if ( Qry.Variables->FindVariable( iv->first.c_str() ) == -1 ) continue;
    Qry.SetVariable( iv->first, iv->second.Value );
    ProgTrace( TRACE5, "SetVariable name=%s, value=%s",
              iv->first.c_str(),iv->second.Value.c_str() );
  }
};

void TCacheTable::ApplyUpdates(xmlNodePtr reqNode)
{
  parse_updates(GetNode("rows", reqNode));

  int NewVerData = -1;

  if(OnBeforeApplyAll)
      try {
          (*OnBeforeApplyAll)(*this);
      } catch(const UserException &E) {
          throw;
      } catch(const Exception &E) {
          ProgError(STDLOG, "OnBeforeApplyAll failed: %s", E.what());
          throw;
      } catch(...) {
          ProgError(STDLOG, "OnBeforeApplyAll failed: something unexpected");
          throw;
      }

  for(int i = 0; i < 3; i++) {
    string sql;
    TCacheUpdateStatus status;
    TCacheQueryType query_type;
    switch(i) {
      case 0:
          sql = DeleteSQL;
          status = usDeleted;
          query_type = cqtDelete;
          break;
      case 1:
          sql = UpdateSQL;
          status = usModified;
          query_type = cqtUpdate;
          break;
      case 2:
          sql = InsertSQL;
          status = usInserted;
          query_type = cqtInsert;
          break;
    }
    if (!sql.empty()) {
      vars.clear();
      FindVariables(sql, false, vars);
      bool tidExists = find(vars.begin(), vars.end(), "TID") != vars.end();
      if ( tidExists && NewVerData < 0 ) {
        Qry->Clear();
        Qry->SQLText = "SELECT tid__seq.nextval AS tid FROM dual";
        Qry->Execute();
        NewVerData = Qry->FieldAsInteger( "tid" );
      }
      Qry->Clear();
      Qry->SQLText = sql;
      DeclareSysVariables(vars, Qry);
      DeclareVariables( vars ); //??࠭?? ᮧ???? ??? ??६?????
      if ( tidExists ) {
        Qry->DeclareVariable( "tid", otInteger );
        Qry->SetVariable( "tid", NewVerData );
        ProgTrace( TRACE5, "NewVerData=%d", NewVerData );
      }
      bool firstRow=true;
      for( TTable::iterator iv = table.begin(); iv != table.end(); iv++ )
      {
        //横? ?? ????窠?
        if ( iv->status != status ) continue;
        if (firstRow)
        {
          DeclareVariablesFromParams(vars, iv->params, *Qry);
          firstRow=false;
        };

        if(OnBeforeApply)
            try {
                (*OnBeforeApply)(*this, *iv, *Qry, query_type);
            } catch(const UserException &E) {
                throw;
            } catch(const Exception &E) {
                ProgError(STDLOG, "OnBeforeApply failed: %s", E.what());
                throw;
            } catch(...) {
                ProgError(STDLOG, "OnBeforeApply failed: something unexpected");
                throw;
            }

        SetVariables( *iv, vars );
        try {
          Qry->Execute();
          if ( Logging ) /* ????஢???? */
            OnLogging( *iv, status );
        }
        catch(const EOracleError &E) {
          if ( E.Code >= 20000 ) {
            string str = E.what();
            throw UserException(EOracleError2UserException(str));
          }
          else {
            switch( E.Code ) {
              case 1: throw AstraLocale::UserException("MSG.UNIQUE_CONSTRAINT_VIOLATED");
              case 1400:
              case 1407: throw AstraLocale::UserException("MSG.CANNOT_INSERT_NULL");
              case 2291: throw AstraLocale::UserException("MSG.INTEGRITY_VIOLATED_PARENT_KEY_NOT_FOUND");
              case 2292: throw AstraLocale::UserException("MSG.INTEGRITY_VIOLATED_CHILD_RECORD_FOUND");
              default: throw;
            }
          } /* end else */
        } /* end try */
        if(OnAfterApply)
            try {
                (*OnAfterApply)(*this, *iv, *Qry, query_type);
            } catch(const UserException &E) {
                throw;
            } catch(const Exception &E) {
                ProgError(STDLOG, "OnAfterApply failed: %s", E.what());
                throw;
            } catch(...) {
                ProgError(STDLOG, "OnAfterApply failed: something unexpected");
                throw;
            }
      } /* end for */
    } /* end if */
  } /* end for  0..2 */

  if(OnAfterApplyAll)
      try {
          (*OnAfterApplyAll)(*this);
      } catch(const UserException &E) {
          throw;
      } catch(const Exception &E) {
          ProgError(STDLOG, "OnAfterApplyAll failed: %s", E.what());
          throw;
      } catch(...) {
          ProgError(STDLOG, "OnAfterApplyAll failed: something unexpected");
          throw;
      }

  Qry->Clear();
  Qry->SQLText =
    "SELECT tid FROM cache_tables WHERE code=:code";
  Qry->CreateVariable( "code", otString, code() );
  Qry->Execute();
  if ( !Qry->Eof ) {
    curVerIface = Qry->FieldAsInteger( "tid" );
  }
  if ( pr_dconst ) {
    Params[ TAG_REFRESH_INTERFACE ].Value.clear();
    Params[ TAG_REFRESH_DATA ].Value.clear();
  }
}

void TCacheTable::DeclareVariables(const std::vector<string> &vars)
{
  vector<string>::const_iterator f;
  for( vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++ ) {
    string VarName;
    for( int i = 0; i < 2; i++ ) {
      if ( i == 0 )
        VarName = iv->Name;
      else
        VarName = "OLD_" + iv->Name;
      f = find( vars.begin(), vars.end(), VarName );
      if ( f != vars.end()) {
        switch(iv->DataType) {
          case ftSignedNumber:
          case ftUnsignedNumber:
                 if ((iv->Scale>0) || (iv->DataSize>9))
                   Qry->DeclareVariable(VarName,otFloat);
                 else
                   Qry->DeclareVariable(VarName,otInteger);
                 break;
          case ftDate:
          case ftTime:
                 Qry->DeclareVariable(VarName,otDate);
                 break;
          case ftString:
          case ftStringList:
                 Qry->DeclareVariable(VarName,otString);
                 break;
          case ftBoolean:
                 Qry->DeclareVariable(VarName,otInteger);
                 break;
          default:;
        }
                iv->VarIdx[i] = distance( vars.begin(), f );
                ProgTrace( TRACE5, "variable name=%s, iv->VarIdx[i]=%d, i=%d",
                           VarName.c_str(), iv->VarIdx[i], i );
      }
      else {
        iv->VarIdx[i] = -1;
      }
    }
  }
  DeclareVariablesFromParams(vars, SQLParams, *Qry);
}

void TCacheTable::SetVariables(TRow &row, const std::vector<std::string> &vars)
{
  string value;
  int Idx=0;
  for(vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++, Idx++) {
    for(int i = 0; i < 2; i++) {
      //ProgTrace(TRACE5, "TCacheTable::SetVariables: i = %d, Name = %s", i, iv->Name.c_str());
      if(iv->VarIdx[i] >= 0) { /* ???? ?????? ??६????? */
        if(i == 0)
          value = row.cols[ Idx ]; /* ??६ ?? ?㦭??? ?⮫??? ??????, ??????? ???諨 */
        else
          value = row.old_cols[ Idx ];
        if ( !value.empty() )
          Qry->SetVariable( vars[ iv->VarIdx[i] ],value.c_str());
        else
          Qry->SetVariable( vars[ iv->VarIdx[i] ],FNull);
        ProgTrace( TRACE5, "SetVariable name=%s, value=%s, ind=%d",
                   vars[ iv->VarIdx[i] ].c_str(), value.c_str(), Idx );
      }
    }
  }

  SetVariablesFromParams(vars, SQLParams, *Qry);
  SetVariablesFromParams(vars, row.params, *Qry);
}

int TCacheTable::getIfaceVer() {
  string stid = Params[ TAG_REFRESH_INTERFACE ].Value;
  int res;
  TrimString( stid );
  if ( stid.empty() || StrToInt( stid.c_str(), res ) == EOF )
    res = -1;
  return res;
}

bool TCacheTable::changeIfaceVer() {
  return ( getIfaceVer() != curVerIface );
}

void TCacheTable::getPerms( )
{
  if ( Params.find( TAG_CODE ) == Params.end() )
    throw Exception("wrong message format");
  string code = Params[TAG_CODE].Value;
  Qry->Clear();
  Qry->SQLText=
    "SELECT role_rights.right_id "
    "FROM user_roles,role_rights "
    "WHERE user_roles.role_id=role_rights.role_id AND "
    "      user_roles.user_id=:user_id AND role_rights.right_id=:right_id AND "
    "      rownum<2";
  Qry->DeclareVariable("user_id",otInteger);
  Qry->DeclareVariable("right_id",otString);
  Qry->SetVariable( "user_id", TReqInfo::Instance()->user.user_id );

  if (SelectRight>=0)
  {
    Qry->SetVariable( "right_id", SelectRight );
    Qry->Execute();
    if (!Qry->Eof) SelectRight=-1;
  };

  if (InsertRight>=0)
  {
    Qry->SetVariable( "right_id", InsertRight );
    Qry->Execute();
    if (Qry->Eof) InsertRight=-1;
  };
  if (UpdateRight>=0)
  {
    Qry->SetVariable( "right_id", UpdateRight );
    Qry->Execute();
    if (Qry->Eof) UpdateRight=-1;
  };
  if (DeleteRight>=0)
  {
    Qry->SetVariable( "right_id", DeleteRight );
    Qry->Execute();
    if (Qry->Eof) DeleteRight=-1;
  };

  Forbidden = SelectRight>=0;
  ReadOnly = SelectRight>=0 ||
             ((InsertSQL.empty() || InsertRight<0) &&
              (UpdateSQL.empty() || UpdateRight<0) &&
              (DeleteSQL.empty() || DeleteRight<0));
}

void BeforeRefresh(TCacheTable &cache, TQuery &refreshQry, const TCacheQueryType qryType)
{
  if (cache.code() == "TRIP_BAG_NORMS" ||
      cache.code() == "TRIP_BAG_RATES" ||
      cache.code() == "TRIP_VALUE_BAG_TAXES" ||
      cache.code() == "TRIP_EXCHANGE_RATES" )
  {
    if (qryType==cqtSelect)
    {
      if (refreshQry.Variables!=NULL &&
          refreshQry.Variables->FindVariable("use_mark_flt")!=-1 &&
          refreshQry.VariableIsNULL("use_mark_flt"))
      {
        refreshQry.DeleteVariable("use_mark_flt");
        refreshQry.DeleteVariable("airline_mark");
        if (cache.code() == "TRIP_BAG_NORMS" ||
            cache.code() == "TRIP_BAG_RATES")
          refreshQry.DeleteVariable("flt_no_mark");
      };

      if (refreshQry.Variables==NULL ||
          refreshQry.Variables->FindVariable("use_mark_flt")==-1)
      {
        refreshQry.CreateVariable("use_mark_flt",otInteger,(int)false);
        refreshQry.CreateVariable("airline_mark",otString,FNull);
        if (cache.code() == "TRIP_BAG_NORMS" ||
            cache.code() == "TRIP_BAG_RATES")
          refreshQry.CreateVariable("flt_no_mark",otInteger,FNull);
      };
    };
  };
};

static set<int> tlg_out_point_ids;
static int typeb_addrs_id;

void BeforeApplyAll(TCacheTable &cache)
{
  if (TSyncTlgOutMng::Instance()->IsCacheToSync(cache.code()))
    tlg_out_point_ids.clear();
  if (cache.code() == "TYPEB_CREATE_POINTS")
      typeb_addrs_id = NoExists;
};

void AfterApplyAll(TCacheTable &cache)
{
    for(set<int>::const_iterator i=tlg_out_point_ids.begin(); i!=tlg_out_point_ids.end(); ++i)
        TSyncTlgOutMng::Instance()->sync_by_cache(cache.code(), *i);
    tlg_out_point_ids.clear();
    if (cache.code() == "TYPEB_CREATE_POINTS")
    {
        if(typeb_addrs_id != NoExists) {
            set<int> point_ids;
            string tlg_type;
            calc_tlg_out_point_ids(typeb_addrs_id, point_ids, tlg_type);
            for(set<int>::const_iterator i = point_ids.begin(); i != point_ids.end(); i++)
                TSyncTlgOutMng::Instance()->sync_by_type(tlg_type, *i);
            typeb_addrs_id = NoExists;
        }
    };
};

#include <boost/regex.hpp>

void BeforeApply(TCacheTable &cache, const TRow &row, TQuery &applyQry, const TCacheQueryType qryType)
{
    if (cache.code() == "DOC_NUM_COPIES") {
        string num;
        if (
                row.status != usDeleted and
                row.status != usUnmodified
           )
            num=cache.FieldValue("num", row);
        if(not num.empty() and num[0] == '0')
                throw AstraLocale::UserException("MSG.INVALID_COPIES");
    }

    if (cache.code() == "CUSTOM_ALARM_SETS") {
        if (
                row.status != usDeleted and
                row.status != usUnmodified
           ) {
            ostringstream alarm_id;
            alarm_id << setw(9) << setfill('0') << cache.FieldValue("alarm", row);
            string airline = getBaseTable(etCustomAlarmType).get_row("code", alarm_id.str()).AsString("airline");
            if(not airline.empty() and airline != cache.FieldValue("airline", row))
                throw AstraLocale::UserException("MSG.ALARM_DOES_NOT_MEET_AIRLINE");
        }
    }

    if (cache.code() == "PAY_CLIENTS") {
        if (
                row.status != usDeleted and
                row.status != usUnmodified
           ) {
            int client_id = ToInt(cache.FieldValue("client_id", row));
            if(client_id < 1 or client_id > 65535)
                throw AstraLocale::UserException("MSG.WRONG_CLIENT_ID");
        }
    }

    if (cache.code() == "REM_TXT_SETS") {
        if (
                row.status != usDeleted and
                row.status != usUnmodified
           ) {
            int tag_index = ToInt(cache.FieldValue("tag_index", row));
            int text_length = ToInt(cache.FieldValue("text_length", row));
            if(tag_index > 9)
                throw AstraLocale::UserException("MSG.REM_TXT_SETS.WRONG_TAG_INDEX");
            if(tag_index < 5) {
                if(text_length > 26)
                    throw AstraLocale::UserException("MSG.REM_TXT_SETS.WRONG_TEXT_LENGTH_SMALL");
            } else if(text_length > 73)
                throw AstraLocale::UserException("MSG.REM_TXT_SETS.WRONG_TEXT_LENGTH_BIG");
            int assigned = 0;
            assigned += not cache.FieldValue("rfisc", row).empty();
            assigned += not cache.FieldValue("brand_code", row).empty();
            assigned += not cache.FieldValue("fqt_tier_level", row).empty();
            if(not assigned)
                throw AstraLocale::UserException("MSG.CANNOT_INSERT_NULL");
            if(assigned > 1)
                throw AstraLocale::UserException("MSG.MORE_THAN_ONE_CRITERION");
        }
    }

  if (cache.code() == "BI_PRINT_RULES" ||
      cache.code() == "REM_TXT_SETS" ||
      cache.code() == "CUSTOM_ALARM_SETS" ||
      cache.code() == "DCS_SERVICE_APPLYING" ||
      cache.code() == "CONFIRMATION_SETS") {
      list<string> l;
      if (
              row.status != usDeleted and
              row.status != usUnmodified
         ) {
          l.push_back(cache.FieldValue("rfisc", row));
          if(cache.code() == "CUSTOM_ALARM_SETS")
              l.push_back(cache.FieldValue("rfisc_tlg", row));
      }

      for(const auto &i: l)
          if(not i.empty()) {
              static const boost::regex e("^[?-??A-Z0-9]{3,15}$");
              boost::smatch results;
              if(not boost::regex_match(i, results, e))
                  throw AstraLocale::UserException("MSG.WRONG_RFISC");
          }
  }
  if (cache.code() == "CODESHARE_SETS")
  {
    string airp;
    if (row.status == usInserted)
      airp=cache.FieldValue("airp_dep", row);
    else
      airp=cache.FieldOldValue("airp_dep", row);
    TDateTime now_local= UTCToLocal( NowUTC(), AirpTZRegion(airp) );
    modf(now_local,&now_local);
    applyQry.CreateVariable("now_local", otDate, now_local);
  };
};

void AfterApply(TCacheTable &cache, const TRow &row, TQuery &applyQry, const TCacheQueryType qryType)
{
    if (TSyncTlgOutMng::Instance()->IsCacheToSync(cache.code()))
    {
        set<int> point_ids;
        TSimpleFltInfo flt;
        if(qryType == cqtDelete or qryType == cqtUpdate) {
            flt.airline = cache.FieldOldValue("airline", row);
            flt.airp_dep = cache.FieldOldValue("airp_dep", row);
            flt.airp_arv = cache.FieldOldValue("airp_arv", row);
            flt.flt_no = ToInt(cache.FieldOldValue("flt_no", row));
            if(flt.flt_no == 0) flt.flt_no = ASTRA::NoExists;
            calc_tlg_out_point_ids(flt, point_ids);
        }
        if(qryType == cqtInsert or qryType == cqtUpdate) {
            flt.airline = cache.FieldValue("airline", row);
            flt.airp_dep = cache.FieldValue("airp_dep", row);
            flt.airp_arv = cache.FieldValue("airp_arv", row);
            flt.flt_no = ToInt(cache.FieldValue("flt_no", row));
            if(flt.flt_no == 0) flt.flt_no = ASTRA::NoExists;
            calc_tlg_out_point_ids(flt, point_ids);
        }
        tlg_out_point_ids.insert(point_ids.begin(), point_ids.end());
    };


    if (cache.code() == "TYPEB_CREATE_POINTS") {
        int tmp_id;
        if (qryType == cqtInsert || qryType == cqtUpdate)
            tmp_id=ToInt(cache.FieldValue( "typeb_addrs_id", row ));
        else
            tmp_id=ToInt(cache.FieldOldValue( "typeb_addrs_id", row ));
        if(typeb_addrs_id != NoExists and typeb_addrs_id != tmp_id)
            throw Exception("typeb_create_points after apply: id must be same");
        typeb_addrs_id = tmp_id;
    }

    if (cache.code() == "TRIP_PAID_CKIN")
    {
        if (!( (qryType == cqtDelete || ToInt(cache.FieldValue( "pr_permit", row )) == 0) &&
                    (qryType == cqtInsert || ToInt(cache.FieldOldValue( "pr_permit", row )) == 0) ) )
        {
            int point_id;
            if (qryType == cqtInsert || qryType == cqtUpdate)
                point_id=ToInt(cache.FieldValue( "point_id", row ));
            else
                point_id=ToInt(cache.FieldOldValue( "point_id", row ));

            TPointIdsForCheck point_ids_spp;
            if ((qryType == cqtInsert || qryType == cqtUpdate) &&
                    ToInt(cache.FieldValue( "pr_permit", row )) != 0)
            {
                SyncTripCompLayers(NoExists, point_id, cltPNLBeforePay, point_ids_spp);
                SyncTripCompLayers(NoExists, point_id, cltPNLAfterPay, point_ids_spp);
                SyncTripCompLayers(NoExists, point_id, cltProtBeforePay, point_ids_spp);
                SyncTripCompLayers(NoExists, point_id, cltProtAfterPay, point_ids_spp);
            }
            else
            {
                DeleteTripCompLayers(NoExists, point_id, cltPNLBeforePay, point_ids_spp);
                DeleteTripCompLayers(NoExists, point_id, cltPNLAfterPay, point_ids_spp);
                DeleteTripCompLayers(NoExists, point_id, cltProtBeforePay, point_ids_spp);
                DeleteTripCompLayers(NoExists, point_id, cltProtAfterPay, point_ids_spp);
            };
            check_layer_change( point_ids_spp, __FUNCTION__ );
        };
    };

    if (cache.code() == "CODESHARE_SETS")
    {
        TDateTime now = Now();
        modf(now,&now);
        //??? ???????: ?? ? ??᫥ ?????????
        vector<TTripInfo> flts;
        TTripInfo markFlt;
        if (row.status != usDeleted)
        {
            markFlt.airline=cache.FieldValue( "airline_mark", row );
            markFlt.flt_no=ToInt(cache.FieldValue( "flt_no_mark", row ));
            markFlt.suffix=cache.FieldValue( "suffix_mark", row );
            markFlt.airp=cache.FieldValue( "airp_dep", row );
        }
        else
        {
            markFlt.airline=cache.FieldOldValue( "airline_mark", row );
            markFlt.flt_no=ToInt(cache.FieldOldValue( "flt_no_mark", row ));
            markFlt.suffix=cache.FieldOldValue( "suffix_mark", row );
            markFlt.airp=cache.FieldOldValue( "airp_dep", row );
        };
        for(markFlt.scd_out=now-5;markFlt.scd_out<=now+CREATE_SPP_DAYS()+1;markFlt.scd_out+=1.0) flts.push_back(markFlt);

        //???離?/?ਢ離? ३ᮢ
        TTlgBinding tlgBinding(true);
        TTrferBinding trferBinding;

        tlgBinding.trace_for_bind(flts, "codeshare_sets: flts");

        tlgBinding.unbind_flt(flts, false);
        trferBinding.unbind_flt(flts, false);
        if (row.status != usDeleted)
        {
            tlgBinding.bind_flt(flts, false);
            trferBinding.bind_flt(flts, false);
        };
    };
};

/*//////////////////////////////////////////////////////////////////////////////*/
void CacheInterface::LoadCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE2, "CacheInterface::LoadCache, reqNode->Name=%s, resNode->Name=%s",
           (const char*)reqNode->name,(const char*)resNode->name);
  TCacheTable cache;
  cache.Init(reqNode);
  cache.OnBeforeRefresh = BeforeRefresh;
  cache.refresh();
  SetProp(resNode, "handle", "1");
  xmlNodePtr ifaceNode = NewTextChild(resNode, "interface");
  SetProp(ifaceNode, "id", "cache");
  SetProp(ifaceNode, "ver", "1");
  cache.buildAnswer(resNode);
  //ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
};

void CacheInterface::SaveCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE2, "CacheInterface::SaveCache");
  TCacheTable cache;
  cache.Init(reqNode);
  if ( cache.changeIfaceVer() )
    throw AstraLocale::UserException( "MSG.CACHE.IFACE_VERSION_CHANGED.REFRESH" );
  cache.OnBeforeApplyAll = BeforeApplyAll;
  cache.OnAfterApplyAll = AfterApplyAll;
  cache.OnBeforeApply = BeforeApply;
  cache.OnAfterApply = AfterApply;
  cache.ApplyUpdates( reqNode );
  cache.refresh();
  SetProp(resNode, "handle", "1");
  xmlNodePtr ifaceNode = NewTextChild(resNode, "interface");
  SetProp(ifaceNode, "id", "cache");
  SetProp(ifaceNode, "ver", "1");
  cache.buildAnswer(resNode);
  AstraLocale::showMessage( "MSG.CHANGED_DATA_COMMIT" );
  //ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
};

void CacheInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};

// TParams1

void TParams1::getParams(xmlNodePtr paramNode)
{
    this->clear();
    if ( paramNode == NULL ) // ??????????? ⥣ ??ࠬ??஢
        return;
    xmlNodePtr curNode = paramNode->children;
    while(curNode) {
        string name = upperc( (const char*)curNode->name );
        string value = NodeAsString(curNode);
        (*this)[ name ].Value = value;
        ProgTrace( TRACE5, "param name=%s, value=%s", name.c_str(), value.c_str() );
        xmlNodePtr propNode  = GetNode( "@type", curNode );
        if ( propNode )
            (*this)[ name ].DataType = (TCacheConvertType)NodeAsInteger( propNode );
        else
            (*this)[ name ].DataType = ctString;
        curNode = curNode->next;
    }
}

void TParams1::setSQL(TQuery *Qry)
{
    vector<string> vars;
    FindVariables(Qry->SQLText.SQLText(), false, vars);
    for(vector<string>::iterator v = vars.begin(); v != vars.end(); v++ )
    {
        otFieldType vtype;
        switch( (*this)[ *v ].DataType ) {
            case ctInteger:
                vtype = otInteger;
                break;
            case ctDouble:
                vtype = otFloat;
                break;
            case ctDateTime:
                vtype = otDate;
                break;
            default:
                vtype = otString;
                break;
        }
        Qry->DeclareVariable( *v, vtype );
        if ( !(*this)[ *v ].Value.empty() ) {
            if(vtype == otDate) {
                TDateTime Value;
                if(StrToDateTime( (*this)[ *v ].Value.c_str(), ServerFormatDateTimeAsString, Value ) == EOF)
                    throw Exception("TParams1::setSQL: cannot convert " + *v + " value to otDate");
                Qry->SetVariable( *v, Value );
            } else
                Qry->SetVariable( *v, (*this)[ *v ].Value );
        } else
            Qry->SetVariable( *v, FNull );
        ProgTrace( TRACE5, "variable %s = %s, type=%i", v->c_str(),
                (*this)[ *v ].Value.c_str(), vtype );
    }
}

#ifdef XP_TESTING

boost::optional<TCacheQueryType> CacheTableTermRequest::tryGetQueryType(const std::string& par)
{
  boost::optional<TCacheQueryType> result;
  if      (par=="insert")  result=cqtInsert;
  else if (par=="update")  result=cqtUpdate;
  else if (par=="delete")  result=cqtDelete;

  return result;
};

std::string CacheTableTermRequest::getAppliedRowStatus(const TCacheQueryType queryType)
{
  if (queryType==cqtInsert) return "inserted";
  if (queryType==cqtUpdate) return "modified";
  if (queryType==cqtDelete) return "deleted";

  return "";
};

boost::optional<int> CacheTableTermRequest::getInterfaceVersion(const std::string& cacheCode)
{
  boost::optional<int> result;

  auto cur = make_curs("SELECT tid FROM cache_tables WHERE code=:code");
  int tid;
  cur.def(tid)
     .bind(":code", StrUtils::ToUpper(cacheCode))
     .EXfet();

  if(cur.err() != NO_DATA_FOUND) result=tid;

  return result;
}

std::string CacheTableTermRequest::getSQLParamXml(const std::vector<std::string> &par)
{
  ASSERT(par.size() == 3);

  XMLDoc sqlParamDoc("sqlparams");
  xmlNodePtr paramNode=NewTextChild(NodeAsNode("/sqlparams", sqlParamDoc.docPtr()), par[0].c_str(), par[2]);

  TCacheConvertType type=ctString;
  std::string typeStr=StrUtils::ToLower(par[1]);
  if      (typeStr=="integer") type=ctInteger;
  else if (typeStr=="double") type=ctDouble;
  else if (typeStr=="datetime") type=ctDateTime;

  SetProp(paramNode, "type", (int)type);

  return StrUtils::replaceSubstrCopy(sqlParamDoc.text(), "encoding=\"UTF-8\"", "encoding=\"CP866\"");
}

CacheTableTermRequest::CacheTableTermRequest(const std::vector<std::string> &par)
{
  ASSERT(par.size() >= 5);

  auto iPar=par.begin();

  queryLogin=*(iPar++);
  queryLang=*(iPar++);

  XMLDoc initDoc("cache");
  xmlNodePtr cacheNode=NodeAsNode("/cache", initDoc.docPtr());
  xmlNodePtr paramsNode=NewTextChild(cacheNode, "params");
  string cacheCode=StrUtils::ToUpper(*(iPar++));
  NewTextChild(paramsNode, "code", cacheCode);
  NewTextChild(paramsNode, "interface_ver", *(iPar++));
  NewTextChild(paramsNode, "data_ver", *(iPar++));

  //??ࠡ?⠥? ????????? SQLParams
  auto iParQueryType=std::find_if(iPar, par.end(), [](const std::string& p) { return tryGetQueryType(p); });
  addSQLParams(iPar, iParQueryType);
  if (sqlParamsDoc) CopyNode(cacheNode, NodeAsNode("/sqlparams", sqlParamsDoc.get().docPtr()));

  //??ࠡ?⠥? ????????? appliedRows
  for(; iParQueryType!=par.end();)
  {
    boost::optional<TCacheQueryType> queryType=tryGetQueryType(*(iParQueryType++));
    ASSERT(queryType);

    iPar=iParQueryType;
    iParQueryType=std::find_if(iPar, par.end(), [](const std::string& p) { return tryGetQueryType(p); });
    addAppliedRow(queryType.get(), iPar, iParQueryType);
  }


  Init(cacheNode);
}

void CacheTableTermRequest::addSQLParams(const std::vector<std::string>::const_iterator& b,
                                         const std::vector<std::string>::const_iterator& e)
{
  for(auto iPar=b; iPar!=e; ++iPar)
  {
    XMLDoc sqlParamDoc(*iPar);
    ASSERT(sqlParamDoc.docPtr()!=nullptr);
    xml_decode_nodelist(sqlParamDoc.docPtr()->children);

    if (!sqlParamsDoc) sqlParamsDoc=boost::in_place("sqlparams");
    CopyNodeList(NodeAsNode("/sqlparams", sqlParamsDoc.get().docPtr()),
                 NodeAsNode("/sqlparams", sqlParamDoc.docPtr()));
  }
}

void CacheTableTermRequest::addAppliedRow(const TCacheQueryType queryType,
                                          const std::vector<std::string>::const_iterator& b,
                                          const std::vector<std::string>::const_iterator& e)
{
  appliedRows.emplace_back(queryType, std::map<std::string, std::string>());
  for(auto iPar=b; iPar!=e; ++iPar)
  {
    string::size_type idx=iPar->find_first_of("=:");
    if (idx!=string::npos)
    {
      std::string name=StrUtils::trim(iPar->substr(0,idx));
      std::string value=StrUtils::trim(iPar->substr(idx+1));
      ASSERT(!name.empty());
      appliedRows.back().second.emplace(StrUtils::ToUpper(name), value);
    }
  }
}

void CacheTableTermRequest::appliedRowToXml(const int rowIndex,
                                            const TCacheQueryType queryType,
                                            const std::map<std::string, std::string>& fields,
                                            xmlNodePtr rowsNode) const
{
  if (rowsNode==nullptr) return;

  xmlNodePtr rowNode=NewTextChild(rowsNode, "row");
  SetProp(rowNode, "index", rowIndex);
  SetProp(rowNode, "status", getAppliedRowStatus(queryType));

  if (sqlParamsDoc) CopyNode(rowNode, NodeAsNode("/sqlparams", sqlParamsDoc.get().docPtr()));

  int colIndex=0;
  for(const auto& f : FFields)
  {
    boost::optional<string> oldValue, newValue;
    if (queryType==cqtUpdate || queryType==cqtDelete)
      oldValue=algo::find_opt<boost::optional>(fields, "OLD_"+f.Name);
    if (queryType==cqtUpdate || queryType==cqtInsert)
      newValue=algo::find_opt<boost::optional>(fields, f.Name);

    xmlNodePtr colNode=nullptr;
    if (queryType==cqtInsert)
      colNode=NewTextChild(rowNode, "col", newValue?newValue.get():"");
    if (queryType==cqtUpdate)
    {
      colNode=NewTextChild(rowNode, "col");
      NewTextChild(colNode, "old", oldValue?oldValue.get():"");
      NewTextChild(colNode, "new", newValue?newValue.get():"");
    }
    if (queryType==cqtDelete)
      colNode=NewTextChild(rowNode, "col", oldValue?oldValue.get():"");

    ASSERT(colNode!=nullptr);
    SetProp(colNode, "index" ,colIndex++);
  }
}

void CacheTableTermRequest::queryPropsToXml(xmlNodePtr queryNode) const
{
  if (queryNode==nullptr) return;

  auto cur = make_curs(
    "SELECT desks.term_mode, desks.term_id FROM users2, desks "
    "WHERE users2.desk=desks.code AND users2.login=:login");
  string mode;
  uint32_t termId;
  cur.def(mode)
     .def(termId)
     .bind(":login", queryLogin)
     .EXfet();

  ASSERT(cur.err() != NO_DATA_FOUND);

  SetProp(queryNode, "handle", 0);
  SetProp(queryNode, "id", "cache");
  SetProp(queryNode, "ver", 1);
  SetProp(queryNode, "opr", queryLogin);
  SetProp(queryNode, "screen", "MAINDCS.EXE");
  SetProp(queryNode, "mode", mode);
  SetProp(queryNode, "lang", queryLang);
  SetProp(queryNode, "term_id", std::to_string(termId));
}

std::string CacheTableTermRequest::getXml() const
{
  XMLDoc queryDoc("term");
  xmlNodePtr queryNode=NewTextChild(NodeAsNode("/term", queryDoc.docPtr()), "query");
  queryPropsToXml(queryNode);

  xmlNodePtr cacheNode=appliedRows.empty()?NewTextChild(queryNode, "cache"):
                                           NewTextChild(queryNode, "cache_apply");
  xmlNodePtr paramsNode=NewTextChild(cacheNode, "params");
  const auto cacheCode=algo::find_opt<boost::optional>(Params, std::string(TAG_CODE));
  const auto cacheInterfaceVersion=algo::find_opt<boost::optional>(Params, TAG_REFRESH_INTERFACE);
  const auto cacheDataVersion=algo::find_opt<boost::optional>(Params, TAG_REFRESH_DATA);
  NewTextChild(paramsNode, "code", cacheCode?cacheCode.get().Value:"");
  NewTextChild(paramsNode, "interface_ver", cacheInterfaceVersion?cacheInterfaceVersion.get().Value:"");
  NewTextChild(paramsNode, "data_ver", cacheDataVersion?cacheDataVersion.get().Value:"");

  if (sqlParamsDoc) CopyNode(cacheNode, NodeAsNode("/sqlparams", sqlParamsDoc.get().docPtr()));

  xmlNodePtr rowsNode=NewTextChild(cacheNode, "rows");
  int rowIndex=0;
  for(const auto& row : appliedRows)
    appliedRowToXml(rowIndex++, row.first, row.second, rowsNode);

  return StrUtils::replaceSubstrCopy(queryDoc.text(), "encoding=\"UTF-8\"", "encoding=\"CP866\"");
}

#endif/*XP_TESTING*/

/*
<term>
  <query handle="0" id="cache" ver="1" opr="VLAD"
    <cache>
      <params>
        <code>TRIP_BRD_WITH_REG</code>
        <data_ver/>
        <interface_ver>681841448</interface_ver>
      </params>
      <sqlparams>
        <point_id type="0">4864052</point_id>
      </sqlparams>
    </cache>
  </query>
</term>

<term>
  <query handle="0" id="cache" ver="1" opr="VLAD"
    <cache_apply>
      <params>
        <code>TRIP_BRD_WITH_REG</code>
        <interface_ver>681835639</interface_ver>
        <data_ver>-1</data_ver>
      </params>
      <sqlparams>
        <point_id type="0">4864052</point_id>
      </sqlparams>
      <rows>
        <row index="0" status="modified">
          <col index="0">
            <old>4864052</old>
            <new>4864052</new>
          </col>
          <col index="1">
            <old/>
            <new/>
          </col>
          <col index="2">
            <old/>
            <new/>
          </col>
          <col index="3">
            <old>0</old>
            <new>1</new>
          </col>
        </row>
      </rows>
    </cache_apply>
  </query>
</term>

<term>
  <query handle="0" id="cache" ver="1" opr="VLAD" s
    <cache_apply>
      <params>
        <code>TRIP_BRD_WITH_REG</code>
        <interface_ver>681841448</interface_ver>
        <data_ver>-1</data_ver>
      </params>
      <sqlparams>
        <point_id type="0">4864052</point_id>
      </sqlparams>
      <rows>
        <row index="0" status="modified">
          <col index="0">
            <old/>
            <new/>
          </col>
          <col index="1">
            <old/>
            <new/>
          </col>
          <col index="2">
            <old>0</old>
            <new>1</new>
          </col>
          <col index="3">
            <old>4864052</old>
            <new>4864052</new>
          </col>
        </row>
      </rows>
    </cache_apply>
  </query>
</term>


<term>
  <query handle="0" id="cache" ver="1" opr="VLAD" s
    <cache_apply>
      <params>
        <code>TRIP_BRD_WITH_REG</code>
        <interface_ver>681835639</interface_ver>
        <data_ver>-1</data_ver>
      </params>
      <sqlparams>
        <point_id type="0">4864052</point_id>
      </sqlparams>
      <rows>
        <row index="0" status="deleted">
          <col index="0">4864052</col>
          <col index="1"/>
          <col index="2"/>
          <col index="3">1</col>
        </row>
      </rows>
    </cache_apply>
  </query>
</term>

<term>
  <query handle="0" id="cache" ver="1" opr="VLAD" scre
    <cache_apply>
      <params>
        <code>TRIP_BRD_WITH_REG</code>
        <interface_ver>681835639</interface_ver>
        <data_ver>-1</data_ver>
      </params>
      <sqlparams>
        <point_id type="0">4864052</point_id>
      </sqlparams>
      <rows>
        <row index="0" status="inserted">
          <sqlparams>
            <point_id type="0">4864052</point_id>
          </sqlparams>
          <col index="0"/>
          <col index="1"/>
          <col index="2"/>
          <col index="3">0</col>
        </row>
      </rows>
    </cache_apply>
  </query>
</term>
*/
