#include "basic.h"
#include "hist.h"
#include "hist_interface.h"
#include "term_version.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>

#define NICKNAME "ANNA"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

static const std::string Id = "ID";
static const std::string StartTime = "START_TIME";
static const std::string EndTime = "END_TIME";
static const std::string DefaultCloseTime = "01.01.2099 00:00:00";
static const unsigned int OptNumLDM = 3;
static const unsigned int OptNumLCI = 9;
static const unsigned int OptNumPRL = 3;
static const unsigned int OptNumPRLMARK = 3;
static const unsigned int OptNumBSM = 1;
static const unsigned int OptNumPNLMARK = 1;
static const unsigned int OptNumRemEvt = 10;

void HistoryInterface::LoadHistory(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE2, "HistoryInterface::LoadHistory, reqNode->Name=%s, resNode->Name=%s",
           (char*)reqNode->name,(char*)resNode->name);
  THistCacheTable cache;
  cache.Init(reqNode);
  cache.refresh();
  cache.DoAfter();
  SetProp(resNode, "handle", "1");
  xmlNodePtr ifaceNode = NewTextChild(resNode, "interface");
  SetProp(ifaceNode, "id", "cache");
  SetProp(ifaceNode, "ver", "1");
  cache.buildAnswer(resNode);
  //ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void THistCacheTable::Init(xmlNodePtr cacheNode)
{
  if ( cacheNode == NULL )
    throw Exception("Wrong message format");

  getParams(GetNode("params", cacheNode), Params); /* общие параметры */
  getParams(GetNode("sqlparams", cacheNode), SQLParams); /* параметры запроса sql */

  if ( Params.find( TAG_CODE ) == Params.end() )
    throw Exception("Wrong message format");

  string code = Params[TAG_CODE].Value;

  search_by_id = SQLParams.find( Id ) != SQLParams.end();
  search_by_dates = SQLParams.find( StartTime ) != SQLParams.end()
                          && SQLParams.find( EndTime ) != SQLParams.end();

  if ( !search_by_id && !search_by_dates)
    throw Exception("Not enough parameters for search history");

  if (search_by_dates) {
    string start_time = SQLParams[StartTime].Value;
    string end_time = SQLParams[StartTime].Value;

    TDateTime UTCNow = NowUTC();
    TDateTime search_beg, search_end;

    if ( StrToDateTime( start_time.c_str(), "dd.mm.yyyy hh:nn:ss", search_beg ) == EOF )
      throw Exception( "Incorrect format of start time, value=%s", start_time.c_str() );
    if ( StrToDateTime( end_time.c_str(), "dd.mm.yyyy hh:nn:ss", search_end ) == EOF )
      throw Exception( "Incorrect format of end time, value=%s", end_time.c_str() );
    if (search_beg > UTCNow || search_end > UTCNow)
      throw Exception( "Time must be less than current time" );
    if (search_beg > search_end)
      throw Exception( "Start time must be less than end time" );
  }

  Forbidden = true;
  ReadOnly = true;
  clientVerData = -1;
  clientVerIface = -1;

  Qry = OraSession.CreateQuery();
  Qry->Clear();
  Qry->SQLText = "SELECT title, select_sql, event_type, tid, select_right"
                 " FROM cache_tables WHERE code = :code";
  Qry->CreateVariable("code", otString, code);
  Qry->Execute();
  if ( Qry->Eof )
    throw Exception( "Table " + string( code ) + " not found" );

  Title = string("История изменения настроек: ") + Qry->FieldAsString( "title" );
  std::string select_sql = Qry->FieldAsString("select_sql");
  EventType = DecodeEventType( Qry->FieldAsString( "event_type" ) );
  curVerIface = Qry->FieldAsInteger( "tid" ); /* текущая версия интерфейса */
  pr_dconst = true;

  //права доступа до операций
  if (!Qry->FieldIsNULL("select_right"))
    SelectRight=Qry->FieldAsInteger("select_right");
  else
    SelectRight=-1;
  InsertRight=-1;
  UpdateRight=-1;
  DeleteRight=-1;
  getPerms( );

  // составим selectSQL
  makeSelectQuery(select_sql);

  initFields(); /* инициализация FFields */
}

void THistCacheTable::initFields()
{
  string code = Params[TAG_CODE].Value;
    // считаем инфу о полях кэша
    Qry->Clear();
    Qry->SQLText =
        "SELECT name,title,width,char_case,align,data_type, "
        "       data_size,scale,nullable,pr_ident,read_only, "
        "       refer_code,refer_name,refer_level,refer_ident,lang,num "
        "FROM cache_fields "
        "WHERE code=:code AND (lang IS NULL OR lang=:lang) "
        "UNION "
        "SELECT name,title,width,char_case,align,data_type, "
        "       data_size,scale,nullable,pr_ident,read_only, "
        "       NULL,NULL,NULL,NULL,NULL,num "
        "FROM hist_info "
        "ORDER BY name, lang NULLS LAST ";
    Qry->CreateVariable("code",otString,code);
    Qry->CreateVariable("lang",otString,TReqInfo::Instance()->desk.lang);
    Qry->Execute();

    if(Qry->Eof)
        throw Exception((string)"Fields of table '"+code+"' not found");

    string prior_name;
    while(!Qry->Eof) {
        string name = Qry->FieldAsString("name");
        if ( !prior_name.empty() && prior_name == name ) { // повторение поля с более низким приоритетом
          Qry->Next();
          continue;
        }

        prior_name = name;
        name = upperc(name);
        if (!params.flds_no_show.empty()) {
            vector<string>::iterator i;
            for (i = params.flds_no_show.begin(); i != params.flds_no_show.end(); i++) {
                if(name == (*i))
                    break;
            }
            if(i != params.flds_no_show.end()) {
                Qry->Next();
                continue;
            }
        }

        TCacheField2 FField;
        FField.Name = name;
        if(FField.Name.find(';') != string::npos)
            throw Exception((string)"Wrong field name '"+code+"."+FField.Name+"'");
        if ((FField.Name == "TID") || (FField.Name == "PR_DEL"))
            throw Exception((string)"Field name '"+code+"."+FField.Name+"' reserved");
        FField.Title = Qry->FieldAsString("title");

        // получим тип поля
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
                FField.Align = taRightJustify;
                break;
            default: FField.Align = taLeftJustify;
        }

        if((string)Qry->FieldAsString("align") == "L") FField.Align = taLeftJustify;
        if((string)Qry->FieldAsString("align") == "R") FField.Align = taRightJustify;
        if((string)Qry->FieldAsString("align") == "C") FField.Align = taCenter;

        FField.DataSize = Qry->FieldAsInteger("data_size");
        if(FField.DataSize<=0)
            throw Exception((string)"Wrong size of field '"+code+"."+FField.Name+"'");
        FField.Scale = Qry->FieldAsInteger("scale");
        if((FField.Scale<0) || (FField.Scale>FField.DataSize))
            throw Exception((string)"Wrong scale of field '"+code+"."+FField.Name+"'");
        if((FField.DataType == ftSignedNumber || FField.DataType == ftUnsignedNumber) &&
                FField.DataSize>15)
            throw Exception((string)"Wrong size of field '"+code+"."+FField.Name+"'");
        /* ширина поля */

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

        FField.ReadOnly = true;
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

        /* проверим, чтобы имена полей не дублировались */
        for(vector<TCacheField2>::iterator i = FFields.begin(); i != FFields.end(); i++)
            if(FField.Name == i->Name)
                throw Exception((string)"Duplicate field name '"+code+"."+FField.Name+"'");

        /* проверим, надо ли переводить содержимое поля в соответствии с LANG */
        if (FField.ReferName == "CODE/CODE_LAT" ||
            FField.ReferName == "CODE_LAT/CODE" )
        {
          FField.ElemCategory=cecCode;
          if (!TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
            FField.ReferName="CODE";
        };

        if (FField.ReferName == "NAME/NAME_LAT" ||
            FField.ReferName == "NAME_LAT/NAME" )
        {
          FField.ElemCategory=cecName;
          if (!TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
            FField.ReferName="NAME";
        };

        if (FField.ReferName == "SHORT_NAME/SHORT_NAME_LAT" ||
            FField.ReferName == "SHORT_NAME_LAT/SHORT_NAME" )
        {
          FField.ElemCategory=cecNameShort;
          if (!TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
            FField.ReferName="SHORT_NAME";
        };

        if (FField.ReferName == "DESCR/DESCR_LAT" ||
            FField.ReferName == "DESCR_LAT/DESCR" )
        {
          FField.ElemCategory=cecNone;
          if (!TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
            FField.ReferName="DESCR";
        };

        if ((code == "ROLES" && FField.Name == "ROLE_NAME") ||
            (FField.ReferCode == "ROLES" && FField.ReferName == "ROLE_NAME"))
        {
          FField.ElemCategory=cecRoleName;
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
        if (FField.ReferCode == "STATIONS" && FField.ReferName == "AIRP_VIEW" )
        {
          FField.ElemCategory=cecCode;
          FField.ElemType=etAirp;
        };

        if (FField.ReferCode == "SALE_POINTS" && FField.ReferName == "VALIDATOR_VIEW" )
        {
          FField.ElemCategory=cecCode;
          FField.ElemType=etValidatorType;
        };

        FFields.push_back(FField);
        Qry->Next();
    }
    sort(FFields.begin(),FFields.end(),lf);
    for (vector<TCacheField2>::iterator i=FFields.begin(); i!=FFields.end(); i++ ) {
        ProgTrace( TRACE5, "cache field: name=%s, num=%d, read_only=%d", i->Name.c_str(), i->num, i->ReadOnly );
    }
}

void THistCacheTable::makeSelectQuery(std::string& select_sql)
{
  if (select_sql.empty())
    throw Exception("Sql select statement is empty");

  if (!getHistInfo())
    throw Exception("History params not found");

  prepareQry(select_sql);

  if (!parseSelectStmt(select_sql))
    throw Exception("Can't parse sql select statement");

  eraseFlds();

  string code = this->code();
  if (code == "TYPEB_ADDRS_LDM" || code == "TYPEB_ADDRS_LCI" || code == "TYPEB_ADDRS_PRL" ||
      code == "TYPEB_ADDRS_PRL_MARK" || code == "TYPEB_ADDRS_BSM" || code == "TYPEB_ADDRS_PNL_MARK" )
    SelectSQL = multipleQry();
  else if (code == "USERS" || code == "SALE_DESKS")
    SelectSQL = exclusionQry();
  else
    SelectSQL = simpleQry();
}

bool THistCacheTable::parseSelectStmt(const std::string& select_sql)
{
  boost::regex pattern1("(?<=SELECT ).*?(?= FROM )");
  boost::regex pattern2("(?<= FROM ).*?((?= WHERE )|(?= ORDER BY)|(?= GROUP BY )|(?=$))");
  boost::regex pattern3("(?<= WHERE ).*?((?= ORDER BY )|(?= GROUP BY )|(?=$))");
  boost::smatch result1, result2, result3;

  if (!boost::regex_search(select_sql, result1, pattern1)
      || !boost::regex_search(select_sql, result2, pattern2))
    return false;

  info.fields = result1[0];
  info.source = result2[0];

  if (boost::regex_search(select_sql, result3, pattern3))
      info.conditions = result3[0];
  return true;
}

string THistCacheTable::simpleQry()
{
  int num = 1;
  string table_name, id_fld;

  for (vector<HistTableInfo>::iterator iv = params.tables_info.begin(); iv != params.tables_info.end(); iv++) {
    string hist_evt = string("HE") + IntToString(num);
    string table = iv->pseudo_name.empty()?(string("HIST_") + iv->table_name):iv->pseudo_name;
    if (num == 1) {
      table_name = table;
      id_fld = iv->ident_fld;
    }
    if (!info.conditions.empty())
      info.conditions = info.conditions + string(" AND ");
    info.conditions = info.conditions + table + string(".HIST_ORDER = ") + hist_evt + string(".HIST_ORDER AND ")
                                      + table + string(".HIST_TIME = ") + hist_evt + string(".OPEN_TIME AND ")
                                      + hist_evt + string(".TABLE_ID = ") + iv->table_id;
    if (num != 1)
      info.conditions = info.conditions + string(" AND ") + hist_evt + string(".CLOSE_TIME > HE1.OPEN_TIME") +
                                          string(" AND ") + hist_evt + string(".OPEN_TIME < HE1.CLOSE_TIME");
    info.source = info.source + string(", HISTORY_EVENTS ") + hist_evt;
    boost::replace_all(info.source, iv->table_name, string("HIST_") + iv->table_name);
    num++;
  }

  if (params.tables_info.size() > 1)
    info.fields = string("DISTINCT ") + info.fields;

  std::ostringstream sql;
  sql << "SELECT " << info.fields << ", HE1.OPEN_TIME, HE1.OPEN_USER, HE1.OPEN_DESK, "
                                       "HE1.CLOSE_TIME, HE1.CLOSE_USER, HE1.CLOSE_DESK"
         " FROM " << info.source <<
         " WHERE " << info.conditions;
  if (search_by_id)
    sql << " AND " + table_name + "." + id_fld + " = :ID ";
  if (search_by_dates)
    sql << " AND HE1.OPEN_TIME <= :END_TIME AND HE1.CLOSE_TIME >= :START_TIME ";
  sql << " ORDER BY " << table_name + "." + id_fld + ", HE1.OPEN_TIME";

  return sql.str();
}

string THistCacheTable::multipleQry()
{
  int num = 1;
  string table_name, id_fld;

  for (vector<HistTableInfo>::iterator iv = params.tables_info.begin(); iv != params.tables_info.end(); iv++) {
    string hist_tab = string("HE") + IntToString(num);
    string table = iv->pseudo_name.empty()?(string("HIST_") + iv->table_name):iv->pseudo_name;
    if (num == 1) {
      table_name = table;
      id_fld = iv->ident_fld;
    }
    if (!info.conditions.empty())
      info.conditions = info.conditions + string(" AND ");
    info.conditions = info.conditions + table + string(".HIST_ORDER = ") + hist_tab + string(".HIST_ORDER AND ")
                                      + table + string(".HIST_TIME = ") + hist_tab + string(".OPEN_TIME AND ")
                                      + hist_tab + string(".TABLE_ID = ") + iv->table_id;
    info.source = info.source + string(", HISTORY_EVENTS ") + hist_tab;
    boost::replace_all(info.source, iv->table_name, string("HIST_") + iv->table_name);
    num++;
  }

  std::ostringstream sql;
  sql << "SELECT " << info.fields << " , "
         "       CASE WHEN HE2.OPEN_TIME <= HE1.OPEN_TIME THEN HE1.OPEN_TIME ELSE HE2.OPEN_TIME END AS OPEN_TIME, "
         "       CASE WHEN HE2.OPEN_TIME <= HE1.OPEN_TIME THEN HE1.OPEN_USER ELSE HE2.OPEN_USER END AS OPEN_USER, "
         "       CASE WHEN HE2.OPEN_TIME <= HE1.OPEN_TIME THEN HE1.OPEN_DESK ELSE HE2.OPEN_DESK END AS OPEN_DESK, "
         "       CASE WHEN HE2.CLOSE_TIME >= HE1.CLOSE_TIME THEN HE1.CLOSE_TIME ELSE HE2.CLOSE_TIME END AS CLOSE_TIME, "
         "       CASE WHEN HE2.CLOSE_TIME >= HE1.CLOSE_TIME THEN HE1.CLOSE_USER ELSE HE2.CLOSE_USER END AS CLOSE_USER, "
         "       CASE WHEN HE2.CLOSE_TIME >= HE1.CLOSE_TIME THEN HE1.CLOSE_DESK ELSE HE2.CLOSE_DESK END AS CLOSE_DESK "
         " FROM " << info.source <<
         " WHERE " << info.conditions;
  if (search_by_id)
    sql << " AND " + table_name + "." + id_fld + " = :ID ";
  if (search_by_dates) {
    sql << " AND HE1.OPEN_TIME <= :END_TIME AND HE1.CLOSE_TIME >= :START_TIME "
           " AND HE2.CLOSE_TIME > GREATEST(HE1.OPEN_TIME, :START_TIME) "
           " AND HE2.OPEN_TIME < LEAST(HE1.CLOSE_TIME, :END_TIME)";
  } else
    sql << " AND HE2.CLOSE_TIME > HE1.OPEN_TIME "
           " AND HE2.OPEN_TIME < HE1.CLOSE_TIME ";
  sql << " ORDER BY " << table_name + "." + id_fld + ", OPEN_TIME, HE1.HIST_ORDER, HE2.HIST_ORDER";

  return sql.str();
}

string THistCacheTable::exclusionQry()
{
  std::ostringstream sql;
  string table1_id, table2_id;
  string code = this->code();
  if (code == "USERS") {
    table1_id = params.tables_info[0].table_id;
    table2_id = params.tables_info[1].table_id;

    sql << "SELECT hist_users2.user_id,login,descr,type AS type_code,type AS type_name, "
           "       pr_denial, hist_users2.user_id AS user_perms, hist_users2.user_id AS user_name, "
           "       DECODE(hist_user_sets.time,        00,00,01,01,02,02,01) AS time_fmt, "
           "       DECODE(hist_user_sets.disp_airline,05,05,06,06,07,07,08,08,09,09,09) AS disp_airline_fmt, "
           "       DECODE(hist_user_sets.disp_airp,   05,05,06,06,07,07,08,08,09,09,09) AS disp_airp_fmt, "
           "       DECODE(hist_user_sets.disp_craft,  05,05,06,06,07,07,08,08,09,09,09) AS disp_craft_fmt, "
           "       DECODE(hist_user_sets.disp_suffix, 15,15,16,16,17,17,17) AS disp_suffix_fmt, "
           "       CASE WHEN he2.open_time <= he1.open_time THEN he1.open_time ELSE he2.open_time END AS open_time, "
           "       CASE WHEN he2.open_time <= he1.open_time THEN he1.open_user ELSE he2.open_user END AS open_user, "
           "       CASE WHEN he2.open_time <= he1.open_time THEN he1.open_desk ELSE he2.open_desk END AS open_desk, "
           "       CASE WHEN he2.close_time >= he1.close_time THEN he1.close_time ELSE he2.close_time END AS close_time, "
           "       CASE WHEN he2.close_time >= he1.close_time THEN he1.close_user ELSE he2.close_user END AS close_user, "
           "       CASE WHEN he2.close_time >= he1.close_time THEN he1.close_desk ELSE he2.close_desk END AS close_desk "
           " FROM hist_users2, hist_user_sets, history_events he1, history_events he2 "
           "WHERE hist_users2.user_id=hist_user_sets.user_id "
           "      AND adm.check_user_view_access(hist_users2.user_id,:SYS_user_id)<>0 "
           "      AND pr_denial != -1 "
           "      AND hist_users2.hist_order = he1.hist_order "
           "      AND hist_users2.hist_time = he1.open_time "
           "      AND he1.table_id = " << table1_id <<
           "      AND hist_user_sets.hist_order = he2.hist_order "
           "      AND hist_user_sets.hist_time = he2.open_time "
           "      AND he2.table_id = " << table2_id;
    if (search_by_id)
      sql << " AND hist_users2.user_id = :id ";
    if (search_by_dates) {
      sql << " AND he1.open_time <= :end_time AND he1.close_time >= :start_time "
             " AND he2.close_time > GREATEST(he1.open_time, :start_time) "
             " AND he2.open_time < LEAST(he1.close_time, :end_time)";
    } else
      sql << " AND he2.close_time > he1.open_time "
             " AND he2.open_time < he1.close_time ";
    sql << " UNION "
           "SELECT hist_users2.user_id,login,descr,type AS type_code,type AS type_name, "
           "       pr_denial, hist_users2.user_id AS user_perms, hist_users2.user_id AS user_name, "
           "       01 AS time_fmt, 09 AS disp_airline_fmt, 09 AS disp_airp_fmt, "
           "       09 AS disp_craft_fmt, 17 AS disp_suffix_fmt, "
           "       open_time, open_user, open_desk, close_time, close_user, close_desk "
           "FROM hist_users2, history_events "
           "WHERE NOT EXISTS (SELECT user_id FROM hist_user_sets "
           "                  WHERE hist_users2.user_id=hist_user_sets.user_id) "
           "      AND adm.check_user_view_access(hist_users2.user_id,:SYS_user_id)<>0 "
           "      AND pr_denial != -1 "
           "      AND hist_users2.hist_order = history_events.hist_order "
           "      AND hist_users2.hist_time = open_time "
           "      AND table_id = " << table1_id;
    if (search_by_id)
      sql << " AND hist_users2.user_id = :id ";
    if (search_by_dates)
      sql << " AND open_time <= :end_time AND close_time >= :start_time ";
    sql << "ORDER BY open_time, descr";
  }
  else if (code == "SALE_DESKS") {
    table1_id = params.tables_info[0].table_id;
    table2_id = params.tables_info[1].table_id;

    sql << "SELECT hist_sale_desks.code, sale_point, validator, currency, hist_sale_desks.pr_denial, "
             "       hist_desks.id AS desks_id, hist_sale_desks.id AS sale_desks_id, "
             "       CASE WHEN he2.open_time <= he1.open_time THEN he1.open_time ELSE he2.open_time END AS open_time, "
             "       CASE WHEN he2.open_time <= he1.open_time THEN he1.open_user ELSE he2.open_user END AS open_user, "
             "       CASE WHEN he2.open_time <= he1.open_time THEN he1.open_desk ELSE he2.open_desk END AS open_desk, "
             "       CASE WHEN he2.close_time >= he1.close_time THEN he1.close_time ELSE he2.close_time END AS close_time, "
             "       CASE WHEN he2.close_time >= he1.close_time THEN he1.close_user ELSE he2.close_user END AS close_user, "
             "       CASE WHEN he2.close_time >= he1.close_time THEN he1.close_desk ELSE he2.close_desk END AS close_desk "
             "FROM hist_sale_desks, hist_desks, history_events he1, history_events he2 "
             "WHERE hist_desks.code = hist_sale_desks.code "
             "      AND hist_sale_desks.hist_order = he1.hist_order "
             "      AND hist_sale_desks.hist_time = he1.open_time "
             "      AND he1.table_id = " << table1_id <<
             "      AND hist_desks.hist_order = he2.hist_order "
             "      AND hist_desks.hist_time = he2.open_time "
             "      AND he2.table_id = " << table2_id;
      if (search_by_id)
        sql << " AND hist_sale_desks.id = :id ";
      if (search_by_dates) {
        sql << " AND he1.open_time <= :end_time AND he1.close_time >= :start_time "
               " AND he2.close_time > GREATEST(he1.open_time, :start_time) "
               " AND he2.open_time < LEAST(he1.close_time, :end_time)";
      } else
        sql << " AND he2.close_time > he1.open_time "
               " AND he2.open_time < he1.close_time ";
      sql << " UNION "
             "SELECT hist_sale_desks.code, sale_point, validator, null, hist_sale_desks.pr_denial, "
             "       null AS desks_id, hist_sale_desks.id AS sale_desks_id, "
             "       open_time, open_user, open_desk, close_time, close_user, close_desk "
             "FROM hist_sale_desks, history_events "
             "WHERE NOT EXISTS (SELECT id FROM hist_desks "
             "                  WHERE hist_desks.code = hist_sale_desks.code) "
             "      AND hist_sale_desks.hist_order = history_events.hist_order "
             "      AND hist_sale_desks.hist_time = open_time "
             "      AND table_id = " << table1_id;
      if (search_by_id)
        sql << " AND hist_sale_desks.id = :id ";
      if (search_by_dates)
        sql << " AND open_time <= :end_time AND close_time >= :start_time ";
      sql << "ORDER BY open_time, code";
  }
  return sql.str();
}

bool THistCacheTable::getHistInfo()
{
  Qry->Clear();
  Qry->SQLText="SELECT first_value AS table_name, second_value AS pseudo_name, ident_field, id "
               "FROM history_params, history_tables "
               "WHERE history_params.first_value = history_tables.code "
               "      AND history_params.code = :code "
               "      AND param_name between 'table_name1' AND 'table_name3' "
               "ORDER BY param_name";
  Qry->CreateVariable("code", otString, this->code());
  Qry->Execute();

  if (Qry->Eof)
    return false;

  while(!Qry->Eof) {

    HistTableInfo tab_info;
    tab_info.table_name = upperc(Qry->FieldAsString("table_name"));
    tab_info.table_id = upperc(IntToString(Qry->FieldAsInteger("id")));

    if(this->code() == "REM_EVENT_SETS")
      tab_info.ident_fld = "SET_ID";
    else
      tab_info.ident_fld = upperc(Qry->FieldAsString("ident_field"));

    if (!Qry->FieldIsNULL("pseudo_name"))
      tab_info.pseudo_name = upperc(Qry->FieldAsString("pseudo_name"));

    params.tables_info.push_back(tab_info);
    Qry->Next();
  }
  return true;
}

void THistCacheTable::prepareQry(std::string& select_sql)
{
  //удалим лишние пробелы
  std::istringstream ist(select_sql);
  std::ostringstream ost;
  std::copy(std::istream_iterator<std::string>(ist),
            std::istream_iterator<std::string>(),
            std::ostream_iterator<std::string>(ost, " ") );
  select_sql = ost.str();
  select_sql = upperc(select_sql);

  Qry->Clear();
  Qry->SQLText="SELECT param_name, first_value, second_value "
               "FROM history_params "
               "WHERE code = :code AND "
               "      param_name IN ('fields_param', 'replace_param')";
  Qry->CreateVariable("code", otString, this->code());
  Qry->Execute();

  while(!Qry->Eof) {
    string param_name = Qry->FieldAsString("param_name");
    if(param_name == "fields_param") {
      string source = upperc(Qry->FieldAsString("first_value"));
      boost::split(params.flds_no_show, source, boost::is_any_of(","));
      std::for_each(params.flds_no_show.begin(), params.flds_no_show.end(),
                    boost::bind(&boost::trim<std::string>, _1, std::locale() ));
    }
    else if (param_name == "replace_param") {
      string pattern, substitution;
      pattern = upperc(Qry->FieldAsString("first_value"));
      if (!Qry->FieldIsNULL("second_value"))
        substitution = upperc(Qry->FieldAsString("second_value"));
      select_sql = boost::regex_replace(select_sql, boost::regex(pattern), substitution);
    }
    Qry->Next();
  }

  for(vector<HistTableInfo>::iterator iv = params.tables_info.begin(); iv != params.tables_info.end(); iv++)
    boost::replace_all(select_sql, iv->table_name + string("."), string("HIST_") + iv->table_name + string("."));
}

void THistCacheTable::eraseFlds()
{
  // разобьем строку info.fields на отдельные поля
  int brecket_count = 0;
  std::string tmp;
  size_t token_count = 0;
  std::list<std::string> fld;
  size_t pos1 = 0;
  for (size_t pos2 = 0; pos2 < info.fields.size(); pos2++) {
    if (info.fields[pos2] == ',' && !brecket_count) {
      tmp = info.fields.substr(pos1,token_count);
      fld.push_back(TrimString(tmp));
      token_count = 0;
      pos1 = pos2 + 1;
      continue;
    }
    else if (info.fields[pos2] == '(') {
      brecket_count++;
    }
    else if (info.fields[pos2] == ')') {
      brecket_count--;
    }
    token_count++;
  }
  fld.push_back(info.fields.substr(pos1,token_count));

  // удалим ненужные поля
  for (std::list<string>::iterator i = fld.begin(); i != fld.end();) {
    if((*i).find(string("TID"))!=string::npos) {
      i = fld.erase(i);
      continue;
    }
    if (!params.flds_no_show.empty()) {
      vector<string>::iterator j;
      for (j=params.flds_no_show.begin(); j!=params.flds_no_show.end(); j++) {
        if((*i).find(*j) != string::npos) {
          i = fld.erase(i);
          break;
        }
      }
      if(j==params.flds_no_show.end())
        ++i;
    }
    else
      ++i;
  }

  // объединим снова в одну строку
  info.fields.clear();
  for (std::list<string>::iterator i = fld.begin(); i != fld.end(); i++) {
    if (!info.fields.empty())
      info.fields = info.fields + string(", ") + *i;
    else
      info.fields = info.fields + *i;
  }
}

void THistCacheTable::DoAfter()
{
  for(TTable::iterator it = table.begin(); it != table.end(); it++) {
      for(vector<string>::iterator ir = it->cols.begin(); ir != it->cols.end(); ir++)
          if(*ir == DefaultCloseTime) *ir = string("");
  }

  unsigned int opt_num = 0;
  string code = this->code();
  if (code == "TYPEB_ADDRS_LDM") opt_num = OptNumLDM;
  else if (code == "TYPEB_ADDRS_LCI") opt_num = OptNumLCI;
  else if (code == "TYPEB_ADDRS_PRL") opt_num = OptNumPRL;
  else if (code == "TYPEB_ADDRS_PRL_MARK") opt_num = OptNumPRLMARK;
  else if (code == "TYPEB_ADDRS_BSM") opt_num = OptNumBSM;
  else if (code == "TYPEB_ADDRS_PNL_MARK") opt_num = OptNumPNLMARK;
  else if (code == "REM_EVENT_SETS") opt_num = OptNumRemEvt;
  else return;

  if (opt_num == 1 || table.empty()) return;

  TTable target;
  TRow row;
  TTable::iterator it = table.begin();
  int info = it->cols.size() - InfoLength;
  int options = 0;
  if (code == "REM_EVENT_SETS") options = info - opt_num - 1;
  else {
    for (vector<TCacheField2>::iterator i=FFields.begin(); i!=FFields.end(); i++) {
      if(i->Name == "BASIC_TYPE") break;
      options ++;
    }
    options++;
  }
  if(table.size() < opt_num)
    throw Exception("Incorrect history table content");

  bool need_all = true;
  while (it!=table.end()) {
    for(int rep_count = opt_num; rep_count != 0; rep_count--) {
      if (row.cols.empty()) {
        row.cols.assign(it->cols.begin(), it->cols.end());
        row.status = it->status;
      }
      else {
        for(int index = options; index < info; index++) {
          if(!it->cols[index].empty() &&
             !boost::regex_match(it->cols[index], boost::regex(".*[+]$")))
            row.cols[index] = it->cols[index];
        }
        row.cols[info + OpenTimeShift] = it->cols[info + OpenTimeShift];
        row.cols[info + OpenUserShift] = it->cols[info + OpenUserShift];
        row.cols[info + OpenDeskShift] = it->cols[info + OpenDeskShift];
      }
      it++;
      if(!sameBase(it-1, it, options)) {
        if(need_all && (rep_count-1))
          throw Exception("Incorrect history table content");
        else {
          row.cols[info + CloseTimeShift] = (it-1)->cols[info + CloseTimeShift];
          row.cols[info + CloseUserShift] = (it-1)->cols[info + CloseUserShift];
          row.cols[info + CloseDeskShift] = (it-1)->cols[info + CloseDeskShift];
          target.push_back(row);
        }
        need_all = true;
        break;
      }
      else if ((it-1)->cols[info + OpenTimeShift] != it->cols[info + OpenTimeShift]) {
        row.cols[info + CloseTimeShift] = it->cols[info + OpenTimeShift];
        row.cols[info + CloseUserShift] = it->cols[info + OpenUserShift];
        row.cols[info + CloseDeskShift] = it->cols[info + OpenDeskShift];
        if (!need_all) {
          target.push_back(row);
          break;
        }
      }
      if(need_all && !(rep_count-1)) {
        need_all =false;
        target.push_back(row);
      }
    }
    if (need_all) {
      row.cols.clear();
      row.status = usUnmodified;
    }
  }

  //заменим teble на target
  table.clear();
  table.assign(target.begin(), target.end());
}

bool THistCacheTable::sameBase(TTable::iterator previos, TTable::iterator next, const int num_cols) const
{
  if (next == table.end()) return false;
  vector<string>::iterator ip = previos->cols.begin();
  vector<string>::iterator in = next->cols.begin();
  while((ip != previos->cols.begin() + num_cols) && (in != next->cols.begin() + num_cols)) {
    if((*ip) != (*in)) return false;
    ip++;
    in++;
  }
  return true;
}
