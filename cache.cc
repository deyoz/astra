#include "cache.h"
#include "basic.h"
#define NICKNAME "DJEK"
#include "setup.h"
#include "test.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#define TAG_REFRESH_DATA        "DATA_VER"
#define TAG_REFRESH_INTERFACE   "INTERFACE_VER"
#define TAG_CODE                "CODE"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

const char * CacheFieldTypeS[NumFieldType] = {"NS","NU","D","T","S","B","SL",""};

/* �� �������� ⥣�� params ��ॢ������ � ����孨� ॣ���� - ��६���� � sql � ���孥� ॣ����*/
void TCacheTable::getParams(xmlNodePtr paramNode, TParams &vparams)
{
  tst();
  vparams.clear();
  if ( paramNode == NULL ) // ��������� ⥣ ��ࠬ��஢
    return;
  xmlNodePtr curNode = paramNode->children;
  while(curNode) {
    string name = upperc( (char*)curNode->name );
    string value = NodeAsString(curNode);
    vparams[ name ].Value = value;
    ProgTrace( TRACE5, "param name=%s, value=%s", name.c_str(), value.c_str() );
    xmlNodePtr propNode  = GetNode( "@type", curNode );
    if ( propNode )
      vparams[ name ].DataType = (TCacheConvertType)NodeAsInteger( propNode );
    else  
      vparams[ name ].DataType = ctString;
    curNode = curNode->next;
  }
}

TCacheTable::TCacheTable(xmlNodePtr cacheNode)
{
    if(cacheNode == NULL) throw Exception("wrong message format");	
    getParams(GetNode("params", cacheNode), Params); /* ��騥 ��ࠬ���� */
    getParams(GetNode("sqlparams", cacheNode), SQLParams); /* ��ࠬ���� ����� sql */
    if(Params.find(TAG_CODE) == Params.end())
        throw Exception("wrong message format");    
    string code = Params[TAG_CODE].Value;
    Qry = OraSession.CreateQuery();
    Forbidden = true;
    ReadOnly = true;
    tid = -1;
    itid = -1;

    Qry->Clear();
    Qry->SQLText =
        "select "
        "   title, "
        "   select_sql, "
        "   refresh_sql, "
        "   insert_sql, "
        "   update_sql, "
        "   delete_sql, "
        "   logging, "
        "   event_type, "
        "   tid "
        "from "
        "   cache_tables "
        "where "
        "   code = :code";
    Qry->DeclareVariable("code", otString);
    Qry->SetVariable("code", code);
    Qry->Execute();
    if ( Qry->Eof ) throw Exception( "table " + string( code ) + " not found" );
    title = Qry->FieldAsString( "title" );
    SelectSQL = Qry->FieldAsString("select_sql");
    RefreshSQL = Qry->FieldAsString("refresh_sql");
    InsertSQL = Qry->FieldAsString("insert_sql");
    UpdateSQL = Qry->FieldAsString("update_sql");
    DeleteSQL = Qry->FieldAsString("delete_sql");
    Logging = Qry->FieldAsInteger("logging") != 0;
    newitid = Qry->FieldAsInteger("tid");
    initFields();
};

TCacheTable::~TCacheTable()
{
    OraSession.DeleteQuery(*Qry);
};

bool TCacheTable::refreshInterface()
{
    string code = Params[TAG_CODE].Value;

    string stid = Params[ TAG_REFRESH_INTERFACE ].Value;
    tst();
    TrimString( stid );
    if ( !stid.empty() )
        if ( StrToInt( stid.c_str(), itid ) == EOF )
            itid = -1;

    Qry->Clear();
    Qry->SQLText = 
        "SELECT MAX(access_code) AS access_code FROM"
        "  (SELECT access_code FROM user_cache_perms"
        "   WHERE user_id=:user_id AND cache=:cache"
        "   UNION"
        "   SELECT MAX(access_code) FROM user_roles,role_cache_perms"
        "   WHERE user_roles.role_id=role_cache_perms.role_id AND"
        "         user_roles.user_id=:user_id AND role_cache_perms.cache=:cache)";
    Qry->DeclareVariable("user_id",otInteger);
    Qry->DeclareVariable("cache",otString);
    Qry->SetVariable("user_id", 1); // !!!
    Qry->SetVariable("cache",code);
    tst();
    Qry->Execute();
    if(Qry->Eof || (Qry->FieldAsInteger("access_code")<=0)) {
        Forbidden = true;
        ReadOnly = true;
    } else {
        Forbidden = false;
        ReadOnly = (Qry->FieldAsInteger("access_code")<5) ||
            InsertSQL.empty() && UpdateSQL.empty() && DeleteSQL.empty();
    }
    tst();
    if ( itid == newitid) return false;
    itid = newitid;
    return true;
}

void TCacheTable::initFields()
{
    string code = Params[TAG_CODE].Value;
    // ��⠥� ���� � ����� ���
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

    vector<int> FFieldsId;
    while(!Qry->Eof) {
        TCacheField2 FField;

        FField.Name = Qry->FieldAsString("name");
        FField.Name = upperc( FField.Name );
        if(FField.Name.find(';') != string::npos)
            throw Exception((string)"Wrong field name '"+code+"."+FField.Name+"'");
        if ((FField.Name == "TID") || (FField.Name == "PR_DEL"))
            throw Exception((string)"Field name '"+code+"."+FField.Name+"' reserved");
        FField.Title = Qry->FieldAsString("title");

        // ����稬 ⨯ ����

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
        // �ਭ� ����

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
                        FField.Width = 5; //dd.mm 
                    else
                        if (FField.DataSize>=8 && FField.DataSize<=9)
                            FField.Width = 8; //dd.mm.yy
                        else FField.Width = 10; //dd.mm.yyyy
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

        if(FField.Ident) FFieldsId.push_back(FFields.size());

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
        //�஢�ਬ, �⮡� ����� ����� �� �㡫�஢�����
        for(vector<TCacheField2>::iterator i = FFields.begin(); i != FFields.end(); i++)
            if(FField.Name == i->Name) 
                throw Exception((string)"Duplicate field name '"+code+"."+FField.Name+"'");
        FFields.push_back(FField);
        Qry->Next();
    }
}

bool TCacheTable::refreshData()
{
    string code = Params[TAG_CODE].Value;

    string stid = Params[ TAG_REFRESH_DATA ].Value;
    tst();
    TrimString( stid );
    if ( !stid.empty() )
        if ( StrToInt( stid.c_str(), tid ) == EOF )
            tid = -1;

    ProgTrace(TRACE5, "TID: %d", tid);
    tst();

    //���஡㥬 ���� � ������ ��६����� :user_id
    //��⮬� �� �������� �⥭�� ������ �� ��।. ������������
    vector<string> vars;
    Qry->Clear();
    if (RefreshSQL.empty() || tid<0) {
      Qry->SQLText = SelectSQL;
      FindVariables(Qry->SQLText.SQLText(), false, vars);
    } else {
        Qry->SQLText = RefreshSQL;  
        FindVariables(Qry->SQLText.SQLText(), false, vars);
        if(find(vars.begin(), vars.end(), "TID") != vars.end()) {
            Qry->DeclareVariable("tid", otInteger);
            Qry->SetVariable("tid", tid); // !!!
        }
    }
        

    if(find(vars.begin(), vars.end(), "USER_ID") != vars.end()) {
        Qry->DeclareVariable("user_id", otInteger);
        Qry->SetVariable("user_id", 1); // !!!
    }
    /* �஡�� �� ��६���� � �����, ��譨� ��६����, ����� ��諨 �� ���뢠�� */
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
    	Qry->SetVariable( *v, SQLParams[ *v ].Value );
    	ProgTrace( TRACE5, "variable %s = %s, type=%i", v->c_str(), 
    	           SQLParams[ *v ].Value.c_str(), vtype );
    }
   

    ProgTrace(TRACE5, "SQLText=%s", Qry->SQLText.SQLText());
    Qry->Execute();

    // �饬, �⮡� �� ����, ����� ���ᠭ� � ��� �뫨 � �����
    vector<int> vecFieldIdx;
    for(vector<TCacheField2>::iterator i = FFields.begin(); i != FFields.end(); i++) {
        int FieldIdx = Qry->GetFieldIndex(i->Name);
        if( FieldIdx < 0)
            throw Exception("Field '" + code + "." + i->Name + "' not found in select_sql");
        vecFieldIdx.push_back( FieldIdx );
    }

    int tidIdx = Qry->GetFieldIndex("TID");
    int delIdx = Qry->GetFieldIndex("PR_DEL");
    if (tid >= 0 && delIdx<0)
        throw Exception( "Field '" + code +".PR_DEL' not found");
    //�⠥� ���
    tst();
    while(!Qry->Eof) {
        if(tidIdx >= 0 && Qry->FieldAsInteger(tidIdx) > tid) tid = Qry->FieldAsInteger(tidIdx);
        TRow row;
        int j=0;
        for(vector<TCacheField2>::iterator i = FFields.begin(); i != FFields.end(); i++,j++) {
            if(Qry->FieldIsNULL(vecFieldIdx[ j ] )) row.cols.push_back( "" );
            else row.cols.push_back( Qry->FieldAsString(vecFieldIdx[ j ]) );
        }
        if(delIdx >= 0 &&  Qry->FieldAsInteger(delIdx) != 0)
            row.status = usDeleted;
        else
            row.status = usUnmodified;
        table.push_back(row);

        Qry->Next();
    }
    tst();
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

    NewTextChild(ifaceNode, "title", title);
    NewTextChild(ifaceNode, "CanInsert", !InsertSQL.empty());
    NewTextChild(ifaceNode, "CanUpdate", !UpdateSQL.empty());
    NewTextChild(ifaceNode, "CanDelete", !DeleteSQL.empty());

    xmlNodePtr ffieldsNode = NewTextChild(ifaceNode, "fields");
    SetProp(ffieldsNode, "tid", itid);
    int i = 0;
    for(vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++) {
        xmlNodePtr ffieldNode = NewTextChild(ffieldsNode, "field");

        SetProp(ffieldNode, "index", i++);

        NewTextChild(ffieldNode, "Name", iv->Name); 

        NewTextChild(ffieldNode, "Title",        iv->Title); 
        NewTextChild(ffieldNode, "Width",        iv->Width); 
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
    SetProp(tabNode, "tid", tid);
    int index = 0;
    for(TTable::iterator it = table.begin(); it != table.end(); it++) {
        xmlNodePtr rowNode = NewTextChild(tabNode, "row");
        SetProp(rowNode, "pr_del", (it->status ==usDeleted));
        SetProp(rowNode, "index", index++);
        int colidx = 0;
        for(vector<string>::iterator ir = it->cols.begin(); ir != it->cols.end(); ir++,colidx++) {
            SetProp(NewTextChild(rowNode, "col", ir->c_str()), "index", colidx);
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
    	getParams( GetNode( "sqlparams", rowNode ), row.params ); /* ��६���� sql ����� */
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
        for (int i=0; i<(int)FFields.size(); i++) { /* �஡�� �� ���� */
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

void TCacheTable::ApplyUpdates(xmlNodePtr reqNode)
{
    parse_updates(GetNode("rows", reqNode));

    int NewTid = -1;
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
            if(tidExists && NewTid < 0) {
                Qry->Clear();
                Qry->SQLText = "SELECT tid__seq.nextval AS tid FROM dual";
                Qry->Execute();
                if(!Qry->Eof) // !!!
                    NewTid = Qry->FieldAsInteger("tid");
                else
                    NewTid = 0;
            }
            Qry->Clear();
            Qry->SQLText = sql;
            DeclareVariables(vars);
            if(tidExists) {
                Qry->DeclareVariable("tid", otInteger);
                Qry->SetVariable("tid", NewTid);
                ProgTrace( TRACE5, "newtid=%d", NewTid );
            }
            for(TTable::iterator iv = table.begin(); iv != table.end(); iv++) {
                if(iv->status != status) continue;
                SetVariables(*iv, vars);
                // ������ ��६����, ����� �������⥫쭮 ��諨 
                // ��᫥ �맮�� �� ������ OnSetVariable            
                try {
                  Qry->Execute();
                }
                catch( EOracleError E ) {
                  if (E.Code>=20000) {
                    string str = E.what();
                    if (str.substr( 0, 3 ) == "ORA") {
                      size_t s = str.find( ": " );
              	      if ( s != string::npos )
              	        str = str.substr( s + 2, str.size() - i );
              	    }
              	    throw UserException( str.c_str() );
                  }
                  else {
              	    switch(E.Code) {
              	      case 1: throw UserException("����襭� 㭨���쭮��� ������");
              	      case 1400:
                      case 1407: throw UserException("�� 㪠���� ���祭�� � ����� �� ��易⥫��� ��� ���������� �����");
                      case 2291: throw UserException("���祭�� ������ �� ����� ��뫠���� �� ���������騥 �����");
                      case 2292: throw UserException("���������� ��������/㤠���� ���祭��, �� ���஥ ��뫠���� ��㣨� �����");
                      default:
                          // !!! ������ � ���
                          throw;
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
            if(iv->VarIdx[i] >= 0) { // ���� ������ ��६�����
              if(i == 0)
                  value = row.cols[ Idx ]; // ��६ �� �㦭��� �⮫�� ������, ����� ��諨
              else 
                  value = row.old_cols[ Idx ];
              Qry->SetVariable( vars[ iv->VarIdx[i] ],(char *)value.c_str());
              ProgTrace( TRACE5, "SetVariable name=%s, value=%s, ind=%d", 
                         (char*)vars[ iv->VarIdx[i] ].c_str(),(char *)value.c_str(), Idx );
            }
        }
    }
}

void TCacheTable::DeclareVariables(const std::vector<string> &vars)
{
/*  for (int i=0; i<vars.size(); i++) {
    ProgTrace( TRACE5, "vars[%d]=%s", i, vars[i].c_str());
  }*/
    for(vector<TCacheField2>::iterator iv = FFields.begin(); iv != FFields.end(); iv++) {
        string VarName;
        for(int i = 0; i < 2; i++) {
            if(i == 0)
                VarName = iv->Name;
            else
                VarName = "OLD_" + iv->Name;
            vector<string>::const_iterator f = find(vars.begin(), vars.end(), VarName);
            if( f != vars.end()) {
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
            } else
                iv->VarIdx[i] = -1;
        }
    }
    // ����� ������������ ��� ����஢���� !!!
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
  return ( getIfaceVer() != newitid);
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
    throw UserException( "����� ����䥩� ����������. ������� �����." );  
  tst();
  cache.ApplyUpdates( reqNode );
  NewTextChild( resNode, "message", "��������� �ᯥ譮 ��࠭���" );
};

void CacheInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};

