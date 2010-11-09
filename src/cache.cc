#include "cache.h"
#include "basic.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_locale.h"
#include "tlg/tlg.h"
#include "astra_service.h"
#include "term_version.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

const char * CacheFieldTypeS[NumFieldType] = {"NS","NU","D","T","S","B","SL",""};

struct TReferCacheTable
{
  const char* CacheCode;
  TElemType ElemType;
};

const TReferCacheTable ReferCacheTable[13] = { {"COUNTRIES",       etCountry},
                                               {"CITIES",          etCity},
                                               {"AIRLINES",        etAirline},
                                               {"AIRPS",           etAirp},
                                               {"CRAFTS",          etCraft},
                                               {"CLASSES",         etClass},
                                               {"SUBCLS",          etSubcls},
                                               {"TRIP_TYPES",      etTripType},
                                               {"GRAPH_STAGES",    etGraphStage},
                                               {"BAG_NORM_TYPES",  etBagNormType},
                                               {"CURRENCY",        etCurrency},
                                               {"MISC_SET_TYPES",  etMiscSetType},
                                               {"SEAT_ALGO_TYPES", etSeatAlgoType}
                                             };

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC;

/* все названия тегов params переводятся в вверхний регистр - переменные в sql в верхнем регистре*/
void TCacheTable::getParams(xmlNodePtr paramNode, TParams &vparams)
{
  vparams.clear();
  if ( paramNode == NULL ) /* отсутствует тег параметров */
    return;
  xmlNodePtr curNode = paramNode->children;
  while( curNode ) {
    string name = upperc( (char*)curNode->name );
    string value = NodeAsString(curNode);
    vparams[ name ].Value = value;
    ProgTrace( TRACE5, "param name=%s, value=%s", name.c_str(), value.c_str() );
    xmlNodePtr propNode  = GetNode( "@type", curNode ); /* используется для sqlparams */
    if ( propNode )
      vparams[ name ].DataType = (TCacheConvertType)NodeAsInteger( propNode );
    else
      vparams[ name ].DataType = ctString;
    curNode = curNode->next;
  }
}

/* конструктор класса - выбираем общие параметры + общие переменные для sql запроса
   + выборка данных из таблицы cache_tables, cache_fields */
void TCacheTable::Init(xmlNodePtr cacheNode)
{
  if ( cacheNode == NULL )
    throw Exception("wrong message format");
  getParams(GetNode("params", cacheNode), Params); /* общие параметры */
  getParams(GetNode("sqlparams", cacheNode), SQLParams); /* параметры запроса sql */
  if ( Params.find( TAG_CODE ) == Params.end() )
    throw Exception("wrong message format");
  string code = Params[TAG_CODE].Value;
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
  curVerIface = Qry->FieldAsInteger( "tid" ); /* текущая версия интерфейса */
  pr_dconst = !Qry->FieldAsInteger( "need_refresh" );
  //получим права доступа до операций
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
  initFields(); /* инициализация FFields */
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


bool lf( const TCacheField2 &item1, const TCacheField2 &item2 )
{
	return item1.num<item2.num;
}


void TCacheTable::initFields()
{
    string code = Params[TAG_CODE].Value;
    // считаем инфу о полях кэша
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

    	  if ( !prior_name.empty() && prior_name == Qry->FieldAsString("name") ) { // повторение поля с более низким приоритетом
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

        if (FField.ReferName == "DESCR/DESCR_LAT" ||
            FField.ReferName == "DESCR_LAT/DESCR" )
        {
          FField.ElemCategory=cecNone;
          if (!TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
            FField.ReferName="DESCR";
        };

        if (FField.ElemCategory!=cecNone)
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

    // задание переменной SYS_user_id
    f = find( vars.begin(), vars.end(), "SYS_USER_ID" );
    if ( f != vars.end() ) {
      Qry->DeclareVariable("SYS_user_id", otInteger);
      Qry->SetVariable( "SYS_user_id", TReqInfo::Instance()->user.user_id );
      vars.erase( f );
    }

    // задание переменной SYS_canon_name
    f = find( vars.begin(), vars.end(), "SYS_CANON_NAME" );
    if ( f != vars.end() ) {
      Qry->DeclareVariable("SYS_canon_name", otString);
      Qry->SetVariable( "SYS_canon_name", OWN_CANON_NAME() );
      vars.erase( f );
    }

    // задание переменной SYS_point_addr
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

    f = find( vars.begin(), vars.end(), "SYS_DESK_VERSION" );
    if ( f != vars.end() ) {
      Qry->DeclareVariable("SYS_desk_version", otString);
      Qry->SetVariable( "SYS_desk_version", TReqInfo::Instance()->desk.version );
      vars.erase( f );
    }
};

TUpdateDataType TCacheTable::refreshData()
{
    Clear();
    string code = Params[TAG_CODE].Value;

    string stid = Params[ TAG_REFRESH_DATA ].Value;

    TrimString( stid );
    if ( stid.empty() || StrToInt( stid.c_str(), clientVerData ) == EOF )
      clientVerData = -1; /* не задана версия, значит обновлять всегда все */
    ProgTrace(TRACE5, "Client version data: %d", clientVerData );

    /*Попробуем найти в именах переменных :user_id
      потому что возможно чтение данных по опред. авиакомпании */
    TCacheQueryType query_type;
    vars.clear();
    Qry->Clear();
    vector<string>::iterator f;
    if ( RefreshSQL.empty() || clientVerData < 0 ) { /* считываем все заново */
      Qry->SQLText = SelectSQL;
      query_type = cqtSelect;
      FindVariables(Qry->SQLText.SQLText(), false, vars);
      clientVerData = -1;
    }
    else { /* обновляем с использованием RefreshSQL и clientVerData */
      Qry->SQLText = RefreshSQL;
      query_type = cqtRefresh;
      /* выделение всех переменных без повтора */
      FindVariables( Qry->SQLText.SQLText(), false, vars );
      /* задание переменной TID */
      f = find( vars.begin(), vars.end(), "TID" );
      if ( f != vars.end() ) {
        Qry->DeclareVariable( "tid", otInteger );
        Qry->SetVariable( "tid", clientVerData );
        ProgTrace( TRACE5, "set clientVerData: tid variable %d", clientVerData );
        vars.erase( f );
      }
    }

    DeclareSysVariables(vars,Qry);

    /* пробег по переменным в запросе, лишние переменные, которые пришли не учитываем */
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
      } catch(UserException E) {
          throw;
      } catch(Exception E) {
          ProgError(STDLOG, "OnBeforeRefresh failed: %s", E.what());
          throw;
      } catch(...) {
          ProgError(STDLOG, "OnBeforeRefresh failed: something unexpected");
          throw;
      }

    ProgTrace(TRACE5, "SQLText=%s", Qry->SQLText.SQLText());
    Qry->Execute();

    // ищем, чтобы все поля, которые описаны в кэше были в запросе
    vector<int> vecFieldIdx;
    for(vector<TCacheField2>::iterator i = FFields.begin(); i != FFields.end(); i++)
    {
        int FieldIdx = Qry->GetFieldIndex(i->Name);
        if( FieldIdx < 0)
        {
          if (i->ElemCategory!=cecNone)
          {
            //проверим - поле может быть _VIEW
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
    //читаем кэш
    while( !Qry->Eof ) {
      if( tidIdx >= 0 && Qry->FieldAsInteger( tidIdx ) > clientVerData )
        clientVerData = Qry->FieldAsInteger( tidIdx );
      TRow row;
      int j=0;
      for(vector<TCacheField2>::iterator i = FFields.begin(); i != FFields.end(); i++,j++) {
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
                switch (i->ElemCategory)
                {
                  case cecCode: if (Qry->FieldType( vecFieldIdx[ j ] ) == otInteger)
                                  row.cols.push_back( ElemIdToCodeNative( i->ElemType, Qry->FieldAsInteger( vecFieldIdx[ j ] ) ) );
                                else
                                  row.cols.push_back( ElemIdToCodeNative( i->ElemType, Qry->FieldAsString(vecFieldIdx[ j ]) ) );
                                break;
                  case cecName: if (Qry->FieldType( vecFieldIdx[ j ] ) == otInteger)
                                  row.cols.push_back( ElemIdToNameLong( i->ElemType, Qry->FieldAsInteger( vecFieldIdx[ j ] ) ) );
                                else
                                  row.cols.push_back( ElemIdToNameLong( i->ElemType, Qry->FieldAsString(vecFieldIdx[ j ] ) ) );
                                break;
                       default: row.cols.push_back( Qry->FieldAsString(vecFieldIdx[ j ]) );
                                break;
                };
                break;
            }
        }
      }
      if(delIdx >= 0 &&  Qry->FieldAsInteger(delIdx) != 0)
        row.status = usDeleted;
      else
        row.status = usUnmodified;
      table.push_back(row);

      Qry->Next();
    }
    ProgTrace( TRACE5, "Server version data: %d", clientVerData );
    if ( !table.empty() ) // начитали изменения
    	return upExists;
    else
    	if ( clientVerData >= 0 ) // нет изменений
    		return upNone;
    	else
    		return upClearAll; // все удалили
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
    if ( Params.find(TAG_REFRESH_DATA) != Params.end() &&
    	    (!TReqInfo::Instance()->desk.compatible(LATIN_VERSION) || !pr_dconst) ||
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
    if (TReqInfo::Instance()->desk.compatible(LATIN_VERSION))
      NewTextChild(dataNode, "keep_locally", KeepLocally );
    else
      NewTextChild(dataNode, "Keep_Locally", KeepLocally );
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

    if ( TReqInfo::Instance()->desk.compatible(LATIN_VERSION) ) {
    	if ( refresh_data_type != upNone || pr_irefresh )
    		XMLData(dataNode);
    }
    else {
    	if ( refresh_data_type == upExists )
    		XMLData(dataNode);
    }
}

void TCacheTable::XMLInterface(const xmlNodePtr dataNode)
{
    xmlNodePtr ifaceNode = NewTextChild(dataNode, "iface");

    NewTextChild(ifaceNode, "title", AstraLocale::getLocaleText(  Title ) );
    NewTextChild(ifaceNode, "CanRefresh", !RefreshSQL.empty());
    NewTextChild(ifaceNode, "CanInsert", !(InsertSQL.empty()||InsertRight<0) );
    NewTextChild(ifaceNode, "CanUpdate", !(UpdateSQL.empty()||UpdateRight<0) );
    NewTextChild(ifaceNode, "CanDelete", !(DeleteSQL.empty()||DeleteRight<0) );

    xmlNodePtr ffieldsNode = NewTextChild(ifaceNode, "fields");
    SetProp( ffieldsNode, "tid", curVerIface );
    int i = 0;
    for(vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++) {
        xmlNodePtr ffieldNode = NewTextChild(ffieldsNode, "field");

        SetProp(ffieldNode, "index", i++);

        NewTextChild(ffieldNode, "Name", iv->Name);

        NewTextChild( ffieldNode, "Title", AstraLocale::getLocaleText( iv->Title ) );
        NewTextChild( ffieldNode, "Width", iv->Width );
        char *charCase;
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
    	getParams( GetNode( "sqlparams", rowNode ), row.params ); /* переменные sql запроса */
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
        for (int i=0; i<(int)FFields.size(); i++) { /* пробег по полям */
          string strnode = "col[@index ='"+IntToString(i)+"']";
          switch(row.status) {
            case usModified:
              row.old_cols.push_back( NodeAsString((char*)string(strnode + "/old").c_str(), rowNode) );
              row.cols.push_back( NodeAsString((char*)string(strnode + "/new").c_str(), rowNode) );
              break;
            case usDeleted:
              row.old_cols.push_back( NodeAsString((char*)strnode.c_str(), rowNode) );
              break;
            case usInserted:
              row.cols.push_back( NodeAsString((char*)strnode.c_str(), rowNode) );
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
  if ( idx < 0 )
    throw Exception( "TCacheTable::FieldValue: field '%s' not found", name.c_str() );
  return row.cols[idx];
};

string TCacheTable::FieldOldValue( const string name, const TRow &row )
{
  int idx = FieldIndex(name);
  if ( idx < 0 )
    throw Exception( "TCacheTable::FieldOldValue: field '%s' not found", name.c_str() );
  return row.old_cols[idx];
};

void OnLoggingF( TCacheTable &cache, const TRow &row, TCacheUpdateStatus UpdateStatus,
                 TLogMsg &message )
{
  string code = cache.code();
  if ( code == "TRIP_BP" ||
       code == "TRIP_BT" ||
       code == "TRIP_BRD_WITH_REG" ||
       code == "TRIP_EXAM_WITH_BRD" ||
       code == "TRIP_WEB_CKIN") {
    ostringstream msg;
    message.ev_type = evtFlt;
    int point_id;
    if ( UpdateStatus == usInserted ) {
      TParams p = row.params;
      TParams::iterator ip = p.find( "POINT_ID" );
      if ( ip == p.end() )
        throw Exception( "Can't find variable point_id" );
      point_id = ToInt( ip->second.Value );
    }
    else {
      point_id = ToInt( cache.FieldOldValue("point_id", row) );
    }
    message.id1 = point_id;
    if ( code == "TRIP_BP" )
    {
      if ( UpdateStatus == usModified || UpdateStatus == usDeleted )
      {
        msg << "Отменен бланк пос. талона '"
            << cache.FieldOldValue( "bp_name", row ) << "'";
        if ( !cache.FieldOldValue( "class", row ).empty() )
          msg << " для класса " + cache.FieldOldValue( "class", row );
        msg << ". ";
      };
      if ( UpdateStatus == usInserted || UpdateStatus == usModified )
      {
        msg << "Установлен бланк пос. талона '"
            << cache.FieldValue( "bp_name", row ) << "'";
        if ( !cache.FieldValue( "class", row ).empty() )
          msg << " для класса " + cache.FieldValue( "class", row );
        msg << ". ";
      };
    };
    if ( code == "TRIP_BT" ) {
      if ( UpdateStatus == usModified || UpdateStatus == usDeleted )
      {
        msg << "Отменен бланк баг. бирки '"
            << cache.FieldOldValue( "bt_name", row ) << "'. ";
      };
      if ( UpdateStatus == usInserted || UpdateStatus == usModified )
      {
        msg << "Установлен бланк баг. бирки '"
            << cache.FieldValue( "bt_name", row ) << "'. ";
      };
    };
    if ( code == "TRIP_BRD_WITH_REG" ||
         code == "TRIP_EXAM_WITH_BRD" )
    {
      if ( UpdateStatus == usModified || UpdateStatus == usDeleted )
      {
        msg << "Отменен режим";
        if (code == "TRIP_BRD_WITH_REG")
        {
          if ( ToInt(cache.FieldOldValue( "pr_misc", row )) == 0 )
            msg << " раздельной регистрации и посадки";
          else
            msg << " посадки при регистрации";
        }
        else
        {
          if ( ToInt(cache.FieldOldValue( "pr_misc", row )) == 0 )
            msg << " раздельной посадки и досмотра";
          else
            msg << " досмотра при посадке";
        };
        if ( !cache.FieldOldValue( "hall_id", row ).empty() )
        {
          msg << " для зала '"
              << cache.FieldOldValue( "hall_name", row ) << "'";
        };
        msg << ". ";
      };
      if ( UpdateStatus == usInserted || UpdateStatus == usModified )
      {
        msg << "Установлен режим";
        if (code == "TRIP_BRD_WITH_REG")
        {
          if ( ToInt(cache.FieldValue( "pr_misc", row )) == 0 )
            msg << " раздельной регистрации и посадки";
          else
            msg << " посадки при регистрации";
        }
        else
        {
          if ( ToInt(cache.FieldValue( "pr_misc", row )) == 0 )
            msg << " раздельной посадки и досмотра";
          else
            msg << " досмотра при посадке";
        };
        if ( !cache.FieldValue( "hall_id", row ).empty() )
        {
          msg << " для зала '"
              << cache.FieldValue( "hall_name", row ) << "'";
        };
        msg << ". ";
      };
    };
    if ( code == "TRIP_WEB_CKIN" ) {
      msg << "На рейсе";
      if ( ToInt(cache.FieldValue( "pr_permit", row )) == 0 )
        msg << " запрещена";
      else
        msg << " разрешена";
      msg << " web-регистрация";
      if ( ToInt(cache.FieldValue( "pr_permit", row )) == 0 )
      {
        if ( ToInt(cache.FieldOldValue( "pr_permit", row )) == 0 )
          msg.str(""); //не записываем в лог
      }
      else
      {
        msg << " с параметрами: "
            << "лист ожидания: ";
        if ( ToInt(cache.FieldValue( "pr_waitlist", row )) == 0 )
          msg << "нет";
        else
          msg << "да";
        msg << ", сквоз. рег.: ";
        if ( ToInt(cache.FieldValue( "pr_tckin", row )) == 0 )
          msg << "нет";
        else
          msg << "да";
        msg << ", перерасч. времен: ";
        if ( ToInt(cache.FieldValue( "pr_upd_stage", row )) == 0 )
          msg << "нет";
        else
          msg << "да";
      };
    };
    message.msg=msg.str();
    return;
  }
}

void TCacheTable::OnLogging( const TRow &row, TCacheUpdateStatus UpdateStatus )
{
  string str1, str2;
  if ( UpdateStatus == usModified || UpdateStatus == usInserted ) {
    int Idx=0;
    for(vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++, Idx++) {
      if ( iv->VarIdx[0] < 0 )
       continue;
      if ( !str1.empty() )
        str1 += ", ";
        if ( iv->Title.empty() )
          str1 += iv->Name;
        else
          str1 += iv->Title;
        str1 += "='" + string( Qry->GetVariableAsString( vars[ iv->VarIdx[ 0 ] ] ) ) + "'";
    }
  }
  for (int l=0; l<2; l++ ) {
    int Idx=0;
    str2 = "";
    for(vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++, Idx++) {
      ProgTrace( TRACE5, "l=%d, Ident=%d, Idx=%d, iv->VarIdx[0]=%d, iv->VarIdx[1]=%d",
                 l, iv->Ident, Idx, iv->VarIdx[0], iv->VarIdx[1] );
      if ( !l && !iv->Ident ||
           UpdateStatus == usInserted && iv->VarIdx[ 0 ] < 0 ||
           UpdateStatus != usInserted && iv->VarIdx[ 1 ] < 0 )
        continue;
      if ( !str2.empty() )
        str2 += ", ";
      if ( !iv->Title.empty() )
        str2 += iv->Title;
      else
        str2 += iv->Name;
      if ( UpdateStatus == usInserted )
        str2 += "='" + string( Qry->GetVariableAsString( vars[ iv->VarIdx[ 0 ] ] ) ) + "'";
      else
        str2 += "='" + string( Qry->GetVariableAsString( vars[ iv->VarIdx[ 1 ] ] ) ) + "'";
      ProgTrace( TRACE5, "str2=|%s|", str2.c_str() );
    }
    if ( !str2.empty() )
      break;
  }
  switch( UpdateStatus ) {
    case usInserted:
           str1 = "Ввод строки в таблице '" + Title + "': " + str1 +
                  ". Идентификатор: " + str2;
           break;
    case usModified:
           str1 = "Изменение строки в таблице '" + Title + "': " + str1 +
                  ". Идентификатор: " + str2;
           break;
    case usDeleted:
           str1 = "Удаление строки в таблице '" + Title + "'"+
                  ". Идентификатор: " + str2;
           break;
    default:;
  }
  TLogMsg message;
  message.msg = str1;
  message.ev_type = EventType;
  OnLoggingF( *this, row, UpdateStatus, message );
  if (!message.msg.empty())
    TReqInfo::Instance()->MsgToLog( message );
}

void TCacheTable::ApplyUpdates(xmlNodePtr reqNode)
{
  parse_updates(GetNode("rows", reqNode));

  int NewVerData = -1;
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
      DeclareVariables( vars ); //заранее создаем все переменные
      if ( tidExists ) {
        Qry->DeclareVariable( "tid", otInteger );
        Qry->SetVariable( "tid", NewVerData );
        ProgTrace( TRACE5, "NewVerData=%d", NewVerData );
      }
      for( TTable::iterator iv = table.begin(); iv != table.end(); iv++ )
      {
        //цикл по строчкам
        if ( iv->status != status ) continue;

        if(OnBeforeApply)
            try {
                (*OnBeforeApply)(*this, *iv, *Qry, query_type);
            } catch(UserException E) {
                throw;
            } catch(Exception E) {
                ProgError(STDLOG, "OnBeforeApply failed: %s", E.what());
                throw;
            } catch(...) {
                ProgError(STDLOG, "OnBeforeApply failed: something unexpected");
                throw;
            }

        SetVariables( *iv, vars );
        try {
          Qry->Execute();
          if ( Logging ) /* логирование */
            OnLogging( *iv, status );
        }
        catch( EOracleError E ) {
          if ( E.Code >= 20000 ) {
            string str = E.what();
            EOracleError2UserException(str);
            throw UserException( str.c_str() );
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
      } /* end for */
    } /* end if */
  } /* end for  0..2 */
  Qry->Clear();
  Qry->SQLText =
    "SELECT tid FROM cache_tables WHERE code=:code";
  Qry->CreateVariable( "code", otString, code() );
  Qry->Execute();
  if ( !Qry->Eof ) {
    curVerIface = Qry->FieldAsInteger( "tid" );
  }
  if ( pr_dconst && TReqInfo::Instance()->desk.compatible(LATIN_VERSION) ) {
  	Params[ TAG_REFRESH_INTERFACE ].Value.clear();
    Params[ TAG_REFRESH_DATA ].Value.clear();
  }
}

void TCacheTable::SetVariables(TRow &row, const std::vector<std::string> &vars)
{
  string value;
  int Idx=0;
  for(vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++, Idx++) {
    for(int i = 0; i < 2; i++) {
      if(iv->VarIdx[i] >= 0) { /* есть индекс переменной */
        if(i == 0)
          value = row.cols[ Idx ]; /* берем из нужного столбца данных, которые пришли */
        else
          value = row.old_cols[ Idx ];
        if ( !value.empty() )
          Qry->SetVariable( vars[ iv->VarIdx[i] ],(char *)value.c_str());
        else
          Qry->SetVariable( vars[ iv->VarIdx[i] ],FNull);
        ProgTrace( TRACE5, "SetVariable name=%s, value=%s, ind=%d",
                  (char*)vars[ iv->VarIdx[i] ].c_str(),(char *)value.c_str(), Idx );
      }
    }
  }


  /* задаем переменные, которые дополнительно пришли
     после вызова на клиенте OnSetVariable */
  for( map<std::string, TParam>::iterator iv=row.params.begin(); iv!=row.params.end(); iv++ ) {
    Qry->SetVariable( iv->first, iv->second.Value );
    ProgTrace( TRACE5, "SetVariable name=%s, value=%s",
              (char*)iv->first.c_str(),(char *)iv->second.Value.c_str() );
  }
}

void TCacheTable::DeclareVariables(std::vector<string> &vars)
{

  DeclareSysVariables(vars,Qry);

  vector<string>::iterator f;
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
  for ( vector<string>::iterator r=vars.begin(); r!=vars.end(); r++ ) {
    if ( Qry->Variables->FindVariable( r->c_str() ) == -1 ) {
      map<std::string, TParam>::iterator ip = SQLParams.find( *r );
      if ( ip != SQLParams.end() ) {
        ProgTrace( TRACE5, "DEclare Variable from SQLParams r->c_str()=%s", r->c_str() );
        switch( ip->second.DataType ) {
          case ctInteger:
            Qry->DeclareVariable( *r, otInteger );
            break;
          case ctDouble:
            Qry->DeclareVariable( *r, otFloat );
            break;
          case ctDateTime:
            Qry->DeclareVariable( *r, otDate );
            break;
          case ctString:
            Qry->DeclareVariable( *r, otString );
            break;
        }
      }
    }
  }
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
             (InsertSQL.empty() || InsertRight<0) &&
             (UpdateSQL.empty() || UpdateRight<0) &&
             (DeleteSQL.empty() || DeleteRight<0);
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

void BeforeApply(TCacheTable &cache, const TRow &row, TQuery &applyQry, const TCacheQueryType qryType)
{
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

/*//////////////////////////////////////////////////////////////////////////////*/
void CacheInterface::LoadCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace(TRACE2, "CacheInterface::LoadCache, reqNode->Name=%s, resNode->Name=%s",
           (char*)reqNode->name,(char*)resNode->name);
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
  cache.OnBeforeApply = BeforeApply;
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
    if ( paramNode == NULL ) // отсутствует тег параметров
        return;
    xmlNodePtr curNode = paramNode->children;
    while(curNode) {
        string name = upperc( (char*)curNode->name );
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

