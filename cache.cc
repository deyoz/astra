#include "cache.h"
#include "basic.h"
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#define TAG_REFRESH_DATA        "DATA_VER"
#define TAG_REFRESH_INTERFACE   "INTERFACE_VER"
#define TAG_CODE                "CODE"

const char * CacheFieldTypeS[NumFieldType] = {"NS","NU","D","T","S","B","SL",""};

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

/* все названия тегов params переводятся в вверхний регистр - переменные в sql в верхнем регистре*/
void TCacheTable::getParams(xmlNodePtr paramNode, TParams &vparams)
{
  tst();
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
TCacheTable::TCacheTable(xmlNodePtr cacheNode)
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
                 "       logging, event_type, tid "
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
  EventType = DecodeEventType( Qry->FieldAsString( "event_type" ) );
  curVerIface = Qry->FieldAsInteger( "tid" ); /* текущая версия интерфейса */
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

void TCacheTable::initFields()
{
    string code = Params[TAG_CODE].Value;
    // считаем инфу о полях кэша
    Qry->Clear();
    Qry->SQLText = 
        "SELECT name,title,width,char_case,align,data_type, "
        "       data_size,scale,nullable,pr_ident,read_only, "
        "       refer_code,refer_name,refer_level "
        "FROM cache_fields "
        "WHERE code=:code "
        "ORDER BY num ";
    Qry->DeclareVariable("code",otString);
    Qry->SetVariable("code",code);
    Qry->Execute();

    if(Qry->Eof)
        throw Exception((string)"Fields of table '"+code+"' not found");

    while(!Qry->Eof) {
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
        FField.ReferName = Qry->FieldAsString("refer_name");

        if (FField.ReferCode.empty() ^ FField.ReferName.empty())
            throw Exception((string)"Wrong reference of field '"+code+"."+FField.Name+"'");
        if(Qry->FieldIsNULL("refer_level"))
            FField.ReferLevel = -1;
        else {
            FField.ReferLevel = Qry->FieldAsInteger("refer_level");
            if(FField.ReferLevel<0)
                throw Exception((string)"Wrong reference of field '"+code+"."+FField.Name+"'");
        }
        /* проверим, чтобы имена полей не дублировались */
        for(vector<TCacheField2>::iterator i = FFields.begin(); i != FFields.end(); i++)
            if(FField.Name == i->Name) 
                throw Exception((string)"Duplicate field name '"+code+"."+FField.Name+"'");
        FFields.push_back(FField);
        Qry->Next();
    }
}

bool TCacheTable::refreshData()
{
    Clear();
    string code = Params[TAG_CODE].Value;

    string stid = Params[ TAG_REFRESH_DATA ].Value;
    tst();
    TrimString( stid );
    if ( stid.empty() || StrToInt( stid.c_str(), clientVerData ) == EOF )
      clientVerData = -1; /* не задана версия, значит обновлять всегда все */
    ProgTrace(TRACE5, "Client version data: %d", clientVerData );

    /*Попробуем найти в именах переменных :user_id
      потому что возможно чтение данных по опред. авиакомпании */
    vector<string> vars;
    Qry->Clear();
    vector<string>::iterator f;
    if ( RefreshSQL.empty() || clientVerData < 0 ) { /* считываем все заново */
      Qry->SQLText = SelectSQL;
      FindVariables(Qry->SQLText.SQLText(), false, vars);
      clientVerData = -1;
    } 
    else { /* обновляем с использованием RefreshSQL и clientVerData */ 
      Qry->SQLText = RefreshSQL;  
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
    /* задание переменной USER_ID */    
    f = find( vars.begin(), vars.end(), "USER_ID" );
    if ( f != vars.end() ) {
      Qry->DeclareVariable("user_id", otInteger);
      Qry->SetVariable( "user_id", TReqInfo::Instance()->user.user_id ); 
      vars.erase( f );
    }
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
   
    ProgTrace(TRACE5, "SQLText=%s", Qry->SQLText.SQLText());
    Qry->Execute();

    // ищем, чтобы все поля, которые описаны в кэше были в запросе
    vector<int> vecFieldIdx;
    for(vector<TCacheField2>::iterator i = FFields.begin(); i != FFields.end(); i++) {
        int FieldIdx = Qry->GetFieldIndex(i->Name);
        if( FieldIdx < 0)
            throw Exception("Field '" + code + "." + i->Name + "' not found in select_sql");
        vecFieldIdx.push_back( FieldIdx );
    }

    int tidIdx = Qry->GetFieldIndex("TID");
    int delIdx = Qry->GetFieldIndex("PR_DEL");
    if ( clientVerData >= 0 && delIdx < 0 )
        throw Exception( "Field '" + code +".PR_DEL' not found");
    //читаем кэш
    tst();
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
                  row.cols.push_back( Qry->FieldAsString(vecFieldIdx[ j ]) );              
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
    return !table.empty();
}

void TCacheTable::refresh()
{
    if(Params.find(TAG_REFRESH_INTERFACE) != Params.end())
        pr_irefresh = refreshInterface();
    else
        pr_irefresh = false;
    if(Params.find(TAG_REFRESH_DATA) != Params.end())
        pr_drefresh = refreshData();
    else
        pr_drefresh = false;
}

void TCacheTable::buildAnswer(xmlNodePtr resNode)
{
    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    NewTextChild(dataNode, "Forbidden", Forbidden);
    NewTextChild(dataNode, "ReadOnly", ReadOnly);
    if(pr_irefresh)
        XMLInterface(dataNode);
    if(pr_drefresh)
        XMLData(dataNode);
}

void TCacheTable::XMLInterface(const xmlNodePtr dataNode)
{
    tst();

    xmlNodePtr ifaceNode = NewTextChild(dataNode, "iface");

    NewTextChild(ifaceNode, "title", Title);
    NewTextChild(ifaceNode, "CanRefresh", !RefreshSQL.empty());
    NewTextChild(ifaceNode, "CanInsert", !InsertSQL.empty());
    NewTextChild(ifaceNode, "CanUpdate", !UpdateSQL.empty());
    NewTextChild(ifaceNode, "CanDelete", !DeleteSQL.empty());

    xmlNodePtr ffieldsNode = NewTextChild(ifaceNode, "fields");
    SetProp( ffieldsNode, "tid", curVerIface );
    int i = 0;
    for(vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++) {
        xmlNodePtr ffieldNode = NewTextChild(ffieldsNode, "field");

        SetProp(ffieldNode, "index", i++);

        NewTextChild(ffieldNode, "Name", iv->Name); 

        NewTextChild( ffieldNode, "Title", iv->Title ); 
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
    tst();
    table.clear();
    if(rowsNode == NULL)
        throw Exception("wrong message format");
    tst();
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

void TCacheTable::OnLogging( const TRow &row, TCacheUpdateStatus UpdateStatus, 
                             const std::vector<std::string> &vars )
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
      if ( !l && !iv->Ident ||  /* new variable value !!!iv->VarIdx[i] */
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
  TReqInfo::Instance()->MsgToLog( str1, EventType );
}

void TCacheTable::ApplyUpdates(xmlNodePtr reqNode)
{
  parse_updates(GetNode("rows", reqNode));

  int NewVerData = -1;
  for(int i = 0; i < 3; i++) {
    string sql;
    TCacheUpdateStatus status;
    switch(i) {
      case 0:
          sql = DeleteSQL;
          status = usDeleted;
          break;
      case 1:
          sql = UpdateSQL;
          status = usModified;
          break;
      case 2:
          sql = InsertSQL;
          status = usInserted;
          break;
    }
    if (!sql.empty()) {
      vector<string> vars;
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
      DeclareVariables( vars );
      if ( tidExists ) {
        Qry->DeclareVariable( "tid", otInteger );
        Qry->SetVariable( "tid", NewVerData );
        ProgTrace( TRACE5, "NewVerData=%d", NewVerData );
      }
      for( TTable::iterator iv = table.begin(); iv != table.end(); iv++ ) {
        if ( iv->status != status ) 
          continue;
        SetVariables( *iv, vars );
        try {
          Qry->Execute();
          if ( Logging ) /* логирование */
            OnLogging( *iv, status, vars );
        }
        catch( EOracleError E ) {
          if ( E.Code >= 20000 ) {
            string str = E.what();
            if (str.substr( 0, 3 ) == "ORA") {
              size_t s = str.find( ": " );
              if ( s != string::npos )
                str = str.substr( s + 2, str.size() - i );
            }
            throw UserException( str.c_str() );
          }
          else {
            switch( E.Code ) {
              case 1: throw UserException("Нарушена уникальность данных");
              case 1400:
              case 1407: throw UserException("Не указано значение в одном из обязательных для заполнения полей");
              case 2291: throw UserException("Значение одного из полей ссылается на несуществующие данные");
              case 2292: throw UserException("Невозможно изменить/удалить значение, на которое ссылаются другие данные");
              default: throw;
            }
          } /* end else */
        } /* end try */
      } /* end for */
        tst();
    } /* end if */
  } /* end for  0..2 */
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

void TCacheTable::DeclareVariables(const std::vector<string> &vars)
{
  for( vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++ ) {
    string VarName;
    for( int i = 0; i < 2; i++ ) {
      if ( i == 0 )
        VarName = iv->Name;
      else
        VarName = "OLD_" + iv->Name;
      vector<string>::const_iterator f = find( vars.begin(), vars.end(), VarName );
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
      else
        iv->VarIdx[i] = -1;
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
  string sql;
  sql =       /*???user_cache_perms */
    "SELECT MAX(access_code) AS access_code FROM"\
    "  (SELECT access_code FROM user_cache_perms"\
    "   WHERE user_id=:user_id AND cache=:cache"\
    "   UNION"\
    "   SELECT MAX(access_code) FROM ";
  sql += COMMON_ORAUSER();
  sql += ".user_roles,";
  sql += COMMON_ORAUSER();
  sql += ".role_cache_perms";
  sql += 
    "   WHERE user_roles.role_id=role_cache_perms.role_id AND"
    "         user_roles.user_id=:user_id AND role_cache_perms.cache=:cache)";
  
  Qry->SQLText = sql;
  Qry->DeclareVariable("user_id",otInteger);
  Qry->DeclareVariable("cache",otString);
  Qry->SetVariable( "user_id", TReqInfo::Instance()->user.user_id );
  Qry->SetVariable( "cache", code );
  tst();
  Qry->Execute();
  if(Qry->Eof || (Qry->FieldAsInteger("access_code")<=0)) {
    Forbidden = true;
    ReadOnly = true;
  } 
  else {
    Forbidden = false;
    ReadOnly = (Qry->FieldAsInteger("access_code")<5) ||
                InsertSQL.empty() && UpdateSQL.empty() && DeleteSQL.empty();
  }  	
}

/*//////////////////////////////////////////////////////////////////////////////*/
void CacheInterface::LoadCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{  	
  ProgTrace(TRACE2, "CacheInterface::LoadCache, reqNode->Name=%s, resNode->Name=%s",
           (char*)reqNode->name,(char*)resNode->name);
  TCacheTable cache( reqNode );
  tst();
  cache.refresh();
  tst();
  SetProp(resNode, "handle", "1");
  xmlNodePtr ifaceNode = NewTextChild(resNode, "interface");
  SetProp(ifaceNode, "id", "cache");
  SetProp(ifaceNode, "ver", "1");
  cache.buildAnswer(resNode);  	  
};

void CacheInterface::SaveCache(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{	
  ProgTrace(TRACE2, "CacheInterface::SaveCache");	  
  TCacheTable cache( reqNode );
  if ( cache.changeIfaceVer() )
    throw UserException( "Версия интерфейса изменилась. Обновите данные." );    
  cache.ApplyUpdates( reqNode );  
  cache.refresh();
  tst();
  SetProp(resNode, "handle", "1");
  xmlNodePtr ifaceNode = NewTextChild(resNode, "interface");
  SetProp(ifaceNode, "id", "cache");
  SetProp(ifaceNode, "ver", "1");
  cache.buildAnswer(resNode);  	 
  showMessage( "Изменения успешно сохранены" );
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
            case ctInteger: vtype = otInteger;
                            break;
            case ctDouble: vtype = otFloat;
                           break;
            case ctDateTime: vtype = otDate;
                             break;
            default: vtype = otString;
        }
        Qry->DeclareVariable( *v, vtype );
        if ( !(*this)[ *v ].Value.empty() )
          Qry->SetVariable( *v, (*this)[ *v ].Value );
        else
          Qry->SetVariable( *v, FNull );
        ProgTrace( TRACE5, "variable %s = %s, type=%i", v->c_str(), 
                (*this)[ *v ].Value.c_str(), vtype );
    }
}
