//---------------------------------------------------------------------
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include "oralib.h"
#include "setup.h"
#include "exceptions.h"
#include "stl_utils.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"


using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;

const char* emptyStr = "";

#ifdef SQL_COUNTERS
std::map<std::string,int> sqlCounters;
int queryCount = 0;
#endif

TSQLText::TSQLText( )
{
  change = true;
}

void TSQLText::SetData( const string &Value )
{
  if ( sqlText != Value )
   {
     sqlText = Value;
     change = true;
   }
}

void TSQLText::operator= ( const string &Value )
{
   SetData( Value.c_str() );
}

void TSQLText::operator= ( char *Value )
{
  if ( Value == NULL )
    SetData( "" );
  else
    SetData( Value );
}

void TSQLText::operator= ( const char *Value )
{
  if ( Value == NULL )
    SetData( "" );
  else
    SetData( (char*)Value );
}

const char *TSQLText::SQLText( )
{
  return sqlText.c_str();
}

bool TSQLText::IsChange( )
{
  return change;
}

void TSQLText::CommitChange()
{
  change = false;
}

bool TSQLText::IsEmpty()
{
  return sqlText.empty();
}

unsigned long TSQLText::Size()
{
  return sqlText.size();
}

void TSQLText::Clear()
{
  sqlText.clear();
}

/* --------------------------------- End Class TSQLTExt --------------------- */

TFieldData::TFieldData( TMemoryManager *AMM )
{
  MM = AMM;
  buf = NULL;
  arind = NULL;
  arlen = NULL;
  buf_size = 0;
  cache = 0;
  LongValue = NULL;
  LongItemsCount = 0;
  cbuffer = NULL;
  cbuf_len = 0;
}

void TFieldData::ClearLongData( )
{
 if ( LongValue )
  {
    for ( int i=0; i<LongItemsCount; i++ ) {
      MM->destroy( LongValue[ i ].PieceBuf, STDLOG );
      delete [ ] LongValue[ i ].PieceBuf;
    }
    MM->free( LongValue, STDLOG );
    LongValue = NULL;
    LongItemsCount = 0;
  };
}

const char *TFieldData::GetName( )
{
  return name.c_str();
}

void TFieldData::SetName( const char *Value, unsigned int len )
{
  name.clear();
  name.append( Value, len );
}

TFieldData::~TFieldData()
{
  if ( buf ) MM->free( buf, STDLOG );
  if ( arind ) MM->free ( arind, STDLOG );
  if ( arlen ) MM->free ( arlen, STDLOG );
  ClearLongData( );
  if ( cbuffer )
    MM->free( cbuffer, STDLOG );
}
/* ------------------------------- End Class TFieldData --------------------- */

TVariableData::TVariableData( TMemoryManager *AMM )
{
  MM = AMM;
  indp = NULL;
  bindhp = NULL;
  len = 0;
  bufown = true;
  cbuffer = NULL;
  cbuf_len = 0;
};

TVariableData::~TVariableData( )
{
  FreeBuffer( );
  if ( indp )
    MM->free( indp, STDLOG );
  if ( cbuffer )
    MM->free( cbuffer, STDLOG );
};

void TVariableData::FreeBuffer( )
{
  if ( bufown )
    MM->free( buf, STDLOG );
  buf = NULL;
  buf_size = 0;
};

const char *TVariableData::GetName( )
{
  return name.c_str();
}

void TVariableData::SetName( const char *Value )
{
  name = Value;
}

void TVariableData::SetOwnBuffer( bool Value )
{
  bufown = Value;
}

bool TVariableData::IsOwnBuffer( )
{
  return bufown;
}
/* ------------------------------- End Class TVariableData ------------------ */

TFields::TFields( TMemoryManager *AMM )
{
  MM = AMM;
}

TFields::~TFields( )
{
  Clear( );
}

void TFields::PreSetFieldsCount( int AColCount )
{
  Clear( );
  FFields.reserve( AColCount );
}

TFieldData *TFields::CreateFieldData( TMemoryManager *AMM )
{
  TFieldData *FieldData;
  try {
    FieldData = new TFieldData( AMM );
    AMM->create( FieldData, STDLOG );
  }
  catch( bad_alloc ) {
    throw EMemoryError( "Can not allocate memory" );
  };
  FFields.push_back( FieldData );
  return FieldData;
}

TFieldData* TFields::GetFieldData( int Index )
{
  if ( Index < 0 || Index >= (int)FFields.size() )
    throw EOracleError( "Field index out of range", 0 );
  return FFields[ Index ];
}

int TFields::GetFieldsCount()
{
  return FFields.size();
}

void TFields::Clear( )
{
  for ( unsigned int i=0; i<FFields.size(); i++ ) {
    MM->destroy( FFields[ i ], STDLOG );
    delete FFields[ i ];
  }
  FFields.clear();
}
/* --------------------------------- End Class TFields ---------------------- */

TVariables::TVariables( TMemoryManager *AMM )
{
  MM = AMM;
}

TVariables::~TVariables( )
{
  Clear( );
}

int TVariables::GetVariablesCount( )
{
  return FVariables.size();
}

void TVariables::Clear( )
{
  for ( unsigned int i=0; i<FVariables.size(); i++ ) {
    MM->destroy( FVariables[ i ], STDLOG );
    delete FVariables[ i ];
  }
  FVariables.clear();
}


int TVariables::FindVariable( const char *name )
{
  int Result = -1;
  string str_name = upperc( string(name) );
  for ( int unsigned i=0; i<FVariables.size(); i++ )
   {
     if ( str_name == upperc( string( FVariables[ i ]->GetName() ) ) )
      //strcmp( name, FVariables[ i ]->GetName() ) == 0 )
      {
        Result = i;
        break;
      };
   };
  return Result;
}

TVariableData *TVariables::CreateVariable( )
{
  TVariableData *VariableData;
  try {
    VariableData = new TVariableData( MM );
    MM->create( VariableData, STDLOG );
  }
  catch( bad_alloc ) {
    throw EMemoryError( "Can not allocate memory" );
  }
  FVariables.push_back( VariableData );
  return VariableData;
}

TVariableData *TVariables::GetVariableData( int Index )
{
  if ( Index < 0 || Index >= (int)FVariables.size() )
    throw EOracleError( "Field index out of range", 0 );
  return FVariables[ Index ];
}

void TVariables::DeleteVariable( int VariableId )
{
  if ( VariableId < 0 || VariableId >= (int)FVariables.size() )
   {
     char serror[ 100 ];
     sprintf( serror, "Variable %d does not exists", VariableId );
     throw EOracleError( serror, 0);
   }
  else
   {
     MM->destroy( FVariables[ VariableId ], STDLOG );
     delete FVariables[ VariableId ];
     FVariables.erase( FVariables.begin() + VariableId );
   };
};
/* --------------------------------- End Class TVariables ------------------- */

TSession::TSession( TOCI AUseOCI8 ):
  UseOCI8( AUseOCI8 ),MM(STDLOG)
{
  plda = &lda;
  FConnect = disconnect;
  envhp = NULL;
  errhp = NULL;
  secerrhp = NULL;
  svchp = NULL;
  authp = NULL;
  srvhp = NULL;
  UsedCount = 0;
  Count = MinElementCount;
  TQuery *Qry;
  for ( int i=0; i<Count; i++ ) {
    Qry = new TQuery(this);
    QueryList.push_back( Qry );
    MM.create( Qry, STDLOG );
  }
  lda_owner = true;
};

void TSession::Initialize( Lda_Def *vlda )
{
  UseOCI8 = false;
  plda = vlda;
  memset( &hda, 0, HDA_SIZE );
  lda_owner = false;
  FConnect = Sconnect;
}


TSession::~TSession( )
{
 LogOff( );
 for ( int i=0; i<Count; i++ ) {
   MM.destroy( QueryList[i], STDLOG );
   delete QueryList[i];
 }
 QueryList.clear();
};

void TSession::CloseChildren( )
{
 for ( int i=0; i<Count; i++ )
   QueryList[i]->Close( );
};

void TSession::RaiseOracleError(  )
{
  const int texterror_size = 2000;
  sb4 Result;
  char texterror[ texterror_size ];
  if ( UseOCI8 )
   {
     OCIErrorGet( errhp, 1, NULL, &Result, (text*)texterror, texterror_size, OCI_HTYPE_ERROR );
   }
  else
   {
     oerhms( Getlda(), Getlda()->rc, (text*)texterror, texterror_size );
     Result = Getlda()->rc;
   };
  throw EOracleError( texterror, Result );
};

void TSession::OCICall( int err )
{
  LastOCIError = err;
  return;
};

int TSession::ReturnCode( )
{ sb4 Result;
  if ( UseOCI8 )
   {
     if ( ( LastOCIError == OCI_ERROR )||( LastOCIError == OCI_SUCCESS_WITH_INFO ) )
       OCIErrorGet( errhp, 1, NULL, &Result, NULL, 0, OCI_HTYPE_ERROR );
     else Result = LastOCIError;
   }
  else Result = Getlda()->rc;
 return Result;
};

void TSession::ServerAttach( )
{
 ub4 EnvMode;
 if ( !envhp )
  {
   EnvMode = OCI_DEFAULT;//|OCI_ENV_NO_MUTEX;
   OCICall( OCIEnvInit( &envhp, EnvMode, 0, NULL ) );
   if ( ReturnCode( ) ) RaiseOracleError( );
   OCICall( OCIHandleAlloc( envhp, (void**)&svchp, OCI_HTYPE_SVCCTX, 0, NULL ) );
   OCICall( OCIHandleAlloc( envhp, (void**)&errhp, OCI_HTYPE_ERROR, 0, NULL ) );
   OCICall( OCIHandleAlloc( envhp, (void**)&secerrhp, OCI_HTYPE_ERROR, 0, NULL ) );
   OCICall( OCIHandleAlloc( envhp, (void**)&srvhp, OCI_HTYPE_SERVER, 0, NULL ) );
   OCICall( OCIHandleAlloc( envhp, (void**)&authp, OCI_HTYPE_SESSION, 0, NULL ) );
   OCICall( OCIServerAttach( srvhp, errhp, (text*)FDatabase.c_str(), FDatabase.size(), OCI_DEFAULT ) );
   if ( ReturnCode( ) ) RaiseOracleError( );
   OCIAttrSet( svchp, OCI_HTYPE_SVCCTX, srvhp, 0, OCI_ATTR_SERVER, secerrhp );
   OCIAttrSet( svchp, OCI_HTYPE_SVCCTX, authp, 0, OCI_ATTR_SESSION, secerrhp );
  };
};

void TSession::LogOn( char *Connect_String, TOCI AUseOCI8 )
{
  if ( FConnect ) return;
  char *cs = Connect_String;
  if ( cs == NULL )
    cs = (char*)emptyStr;
  UseOCI8 = AUseOCI8;
  FUsername.clear();
  FDatabase.clear();
  FPassword.clear();
  if ( UseOCI8 )
   {
     char *p;
     char *cs = Connect_String;
     if ( ( p = strchr( cs, '/' ) ) != NULL )
      {
        FUsername.append( cs, p - cs );
        cs = p + 1;
      };
     if ( ( p = strchr( cs, '@' ) ) != NULL )
      {
         FPassword.append( cs, p - cs );
         FDatabase = p + 1;
      }
     else
       FPassword = cs;
     if ( !FPassword.empty() && FUsername.empty() ) {
       FUsername = FPassword;
       FPassword.clear();
     }
     OCIInitialize( OCI_DEFAULT, NULL, NULL, NULL, NULL );
     ServerAttach( );
     OCICall( OCIAttrSet( authp, OCI_HTYPE_SESSION, (char*)FUsername.c_str(), FUsername.size(), OCI_ATTR_USERNAME, errhp ) );
     OCICall( OCIAttrSet( authp, OCI_HTYPE_SESSION, (char*)FPassword.c_str(), FPassword.size(), OCI_ATTR_PASSWORD, errhp ) );
     OCICall( OCISessionBegin( svchp, errhp, authp, OCI_CRED_RDBMS, OCI_DEFAULT ) );
   }
  else
   {
     memset( &hda, 0, HDA_SIZE );
     memset( Getlda(), 0, sizeof( lda ) );
     olog( Getlda(), hda, (text*)cs, -1, NULL, -1, NULL, -1, 0 );
   };
  if ( ReturnCode( ) )
   RaiseOracleError( );
  if ( UseOCI8 )
    OCICall( OCIServerVersion( srvhp, errhp, (text*)Version, sizeof( Version ), OCI_HTYPE_SERVER ) );
  FConnect = Sconnect;
};

void TSession::LogOff( )
{
 if ( !FConnect ) return;
 CloseChildren( );
 FConnect = disconnect;
 if ( UseOCI8 )
  OCICall( OCISessionEnd( svchp, errhp, authp, OCI_DEFAULT ) );
 else
   if ( lda_owner )
     ologof( Getlda() );
 if ( ReturnCode( ) )
   RaiseOracleError( );
 if ( UseOCI8 )
  {
    OCICall( OCIServerDetach( srvhp, errhp, OCI_DEFAULT ) );
    if ( ReturnCode( ) ) RaiseOracleError( );
    if ( envhp )
     {
       OCICall( OCIHandleFree( srvhp, OCI_HTYPE_SERVER ) );
       OCICall( OCIHandleFree( svchp, OCI_HTYPE_SVCCTX ) );
       OCICall( OCIHandleFree( errhp, OCI_HTYPE_ERROR ) );
       OCICall( OCIHandleFree( secerrhp, OCI_HTYPE_ERROR ) );
       OCICall( OCIHandleFree( authp, OCI_HTYPE_SESSION ) );
       OCICall( OCIHandleFree( envhp, OCI_HTYPE_ENV ) );
       envhp = NULL;
     }
  }
}

bool TSession::isConnect()
{
  return FConnect;
}

Lda_Def *TSession::Getlda()
{
  return plda;
}

OCIEnv *TSession::Getenvhp()
{
  return envhp;
}

OCISvcCtx *TSession::Getsvchp()
{
  return svchp;
}

void TSession::Commit( )
{
 if ( !FConnect ) return;
 if ( UseOCI8 )
  OCICall( OCITransCommit( svchp, errhp, OCI_DEFAULT ) );
 else ocom( Getlda() );
 if ( ReturnCode( ) ) RaiseOracleError( );
};

void TSession::Rollback( )
{
 if ( !FConnect ) return;
 if ( UseOCI8 )
  OCICall( OCITransRollback( svchp, errhp, OCI_DEFAULT ) );
 else orol( Getlda() );
 if ( ReturnCode( ) ) RaiseOracleError( );
};

tconnect TSession::StatusConnect( )
{
 return FConnect;
};

TQuery *TSession::CreateQuery( )
{
 UsedCount++;
 if ( UsedCount > Count )
  {
    Count++;
    TQuery *Qry = new TQuery(this);
    MM.create( Qry, STDLOG );
    QueryList.push_back( Qry );
  };
 QueryList[ UsedCount - 1 ]->Clear( );
 return QueryList[ UsedCount - 1 ];
};

void TSession::DeleteQuery( TQuery &Query )
{
  if ( &Query == NULL ) return;
  vector <TQuery*>::iterator pos;
  pos = find( QueryList.begin(), QueryList.end(), &Query );
  if ( pos == QueryList.end() )
    return;
  QueryList.erase( pos );
  if ( UsedCount > MinElementCount )
   {
     MM.destroy( &Query, STDLOG );
     delete &Query;
     Count--;
   }
   else
     QueryList.push_back( &Query );
  UsedCount--;
};

void TSession::ClearQuerys( )
{
  if ( Count > MinElementCount )
   {
     for (int i=MinElementCount;i<UsedCount;i++) {
       MM.destroy( QueryList[ i ], STDLOG );
       delete QueryList[ i ];
     };
     Count = MinElementCount;
     QueryList.resize( Count );
   };
  UsedCount = 0;
};

bool TSession::isOCI8()
{
  return UseOCI8;
};


////////////////////////////////////////////////////////////////////
TQuery::TQuery( TSession *ASession ):MM(STDLOG)
{
#ifdef SQL_COUNTERS
  queryCount++;
#endif
  MM.create( this, STDLOG );
  errhp = NULL;
  secerrhp = NULL;
  stmthp = NULL;
  defhp = NULL;
  FFunctionType = 0;
  Session = ASession;
  FOpened = disconnect;
  FCache = 25;
  LastOCIError = 0;
  Fields = new TFields( &MM );
  MM.create( Fields, STDLOG );
  Variables = new TVariables( &MM );
  MM.create( Variables, STDLOG );
  Eof = 0;
};

TQuery::~TQuery( )
{
#ifdef SQL_COUNTERS
//  queryCount--;
#endif
  Close( );
  MM.destroy( Fields, STDLOG );
  delete Fields;
  MM.destroy( Variables, STDLOG );
  delete Variables;
  MM.destroy( this, STDLOG );
};

void TQuery::SetSession( TSession *ASession )
{
   Close();
   Fields->Clear();
   Session = ASession;
}


void TQuery::Open( )
{
  if ( ( FOpened )||( !Session ) )
    return;
  if ( Session->isOCI8() )
   {
     OCIHandleAlloc( Session->Getenvhp(), (void**)&errhp, OCI_HTYPE_ERROR, 0, NULL );
     MM.create( errhp, STDLOG );
     OCIHandleAlloc( Session->Getenvhp(), (void**)&secerrhp, OCI_HTYPE_ERROR, 0, NULL );
     MM.create( secerrhp, STDLOG );
   }
  else oopen( &cda, Session->Getlda(), NULL, -1, -1, NULL, -1 );
  if ( ReturnCode( ) ) RaiseOracleError( );
  FOpened = Sconnect;
};

void TQuery::Close( )
{    // удаление дескрипторов
  if ( !Session )
    return;
  if ( ( Session->isConnect() )&&( FOpened ) )
   {
     FOpened = disconnect;
     Fields->Clear( );
     if ( Session->isOCI8() )
      {
        MM.destroy( errhp, STDLOG );
        OCIHandleFree( errhp, OCI_HTYPE_ERROR );
        MM.destroy( secerrhp, STDLOG );
        OCIHandleFree( secerrhp, OCI_HTYPE_ERROR );
        MM.destroy( stmthp, STDLOG );
        OCICall( OCIHandleFree( stmthp, OCI_HTYPE_STMT ) );
        stmthp = NULL;
      }
     else oclose( &cda );
   };
};

void TQuery::OCICall( int err )
{
  LastOCIError = err;
  return;
};

int TQuery::ReturnCode( )
{
  sb4 Result;
  if ( Session->isOCI8() )
   {
     switch ( LastOCIError ) {
       case OCI_ERROR:
       case OCI_INVALID_HANDLE:
              OCIErrorGet( errhp, 1, NULL, &Result, NULL, 0, OCI_HTYPE_ERROR );
              break;
       case OCI_SUCCESS_WITH_INFO:
       case OCI_NEED_DATA:
              Result = 0;
              break;
/*       case OCI_NO_DATA:
              Result = LastOCIError;14030;
              break;*/
/*       case 1403:
              Result = OCI_NO_DATA; //??
              break;*/
       default:
         Result = LastOCIError;
      };
   }
  else
    if ( cda.rc == 1403 )
      Result = OCI_NO_DATA;
    else
      Result = cda.rc;
 return Result;
};

void TQuery::RaiseOracleError( )
{
  const int texterror_size = 2000;
  sb4 Result;
  char texterror[ texterror_size ];
  if ( Session->isOCI8() )
   {
     OCIErrorGet( errhp, 1, NULL, &Result, (text*)texterror, texterror_size, OCI_HTYPE_ERROR );
   }
  else
   {
     Result = cda.rc;
     oerhms( &cda, cda.rc, (text*)texterror, texterror_size );
   };
  throw EOracleError( texterror, Result, SQLText.SQLText() );
};

void TQuery::AllocStmthp( )
{
  int PreFetch;
  TVariableData *AVariableData;
  if ( stmthp ) {
    MM.destroy( stmthp, STDLOG );
    OCIHandleFree( stmthp, OCI_HTYPE_STMT );
  }
  OCIHandleAlloc( Session->Getenvhp(), (void**)&stmthp, OCI_HTYPE_STMT, 0, NULL );
  MM.create( stmthp, STDLOG );
  PreFetch = 0;
  OCICall( OCIAttrSet( stmthp, OCI_HTYPE_STMT, &PreFetch, 4, OCI_ATTR_PREFETCH_ROWS, errhp ) );
  OCICall( OCIAttrSet( stmthp, OCI_HTYPE_STMT, &PreFetch, 4, OCI_ATTR_PREFETCH_MEMORY, errhp ) );
  int VariablesCount = Variables->GetVariablesCount();
  for( int i=0; i<VariablesCount; i++ )
   {
     AVariableData = Variables->GetVariableData( i );
     AVariableData->bindhp = NULL;
   };
};

void TQuery::BindVariables( )
{
  ub2 *lp;
  TVariableData *AVariableData;
  int VariablesCount = Variables->GetVariablesCount( );
  for ( int i=0; i<VariablesCount; i++ )
   {
     AVariableData = Variables->GetVariableData( i );
     lp = NULL;
     if ( ( ( AVariableData->buftype == otLong )||
            ( AVariableData->buftype == otLongRaw ) )&&
          ( AVariableData->buf_size <= 0xFFFF ) ) lp = &AVariableData->len;
      if ( Session->isOCI8() )
       {
         if ( AVariableData->bindhp ) OCICall( OCI_SUCCESS );
           else
            {
              OCICall( OCIBindByName( stmthp, &AVariableData->bindhp, errhp, (text*)AVariableData->GetName(), -1, \
                       AVariableData->buf, AVariableData->buf_size, AVariableData->buftype, \
                       AVariableData->indp, lp, NULL, 0, NULL, OCI_DEFAULT ) );
            };
       }
      else
       {
         obndra( &cda, (text*)AVariableData->GetName(), -1, (ub1*)AVariableData->buf, AVariableData->buf_size,
                 AVariableData->buftype, -1, AVariableData->indp, lp, NULL,
                 0, (ub4*)NULL, (text*)NULL, -1 , -1 );
       };
      if ( ReturnCode( ) ) RaiseOracleError( );
   };
};

void TQuery::Parser( )
{
  Open( );
  Fields->Clear( );
  if ( SQLText.IsEmpty() )
    throw EOracleError( "SQLText is empty", 0 );
  if ( stmthp && !SQLText.IsChange() )
    return;
#ifdef SQL_COUNTERS
  if( !Session->isOCI8() && !SQLText.IsChange() ) {
    return; //???
  }
  sqlCounters[ SQLText.SQLText() ]++;
#endif
  if ( Session->isOCI8() )
   {
     AllocStmthp( );
     OCICall( OCIStmtPrepare( stmthp, errhp, (text*)SQLText.SQLText(), SQLText.Size(), OCI_NTV_SYNTAX, OCI_DEFAULT ) );
     if ( !ReturnCode( ) )
      OCICall( OCIAttrGet( stmthp, OCI_HTYPE_STMT, &FFunctionType, NULL, OCI_ATTR_STMT_TYPE, errhp ) );
   }
  else
   {
     oparse( &cda, (text*)SQLText.SQLText(), -1, 1, 2 );
     FFunctionType = cda.ft;
   };
  if ( ReturnCode( ) )
   {
     FFunctionType = -1;
     RaiseOracleError( );
   };
  SQLText.CommitChange();
};

void TQuery::Describe( )
{
  TFields &AFields = *Fields;
  UsedCache = FCache;
  ub2 u2;
  ub1 u1;
  sb1 s1;
  sb2 s2;
  char *cbufp;
  OCIParam *paramhp;
  int Count;
  if ( Session->isOCI8() )
   {
     OCICall( OCIAttrGet( stmthp, OCI_HTYPE_STMT, &Count, NULL, OCI_ATTR_PARAM_COUNT, errhp ) );
     if ( ReturnCode( ) ) RaiseOracleError( );
     AFields.PreSetFieldsCount( Count );
   }

  bool EndFields = false;
  int Pos = 1;
  TFieldData *AFieldData;
  while ( 1 )
   {
     if ( EndFields || (Session->isOCI8() && Pos > Count) ) break;
     if ( Session->isOCI8() )
      {
        ub4 cbufl;
        AFieldData = AFields.CreateFieldData( &MM );
        OCICall( OCIParamGet( stmthp, OCI_HTYPE_STMT, errhp, (void**)&paramhp, Pos ) );
        if ( !ReturnCode( ) )
         {
            // определяем размер данных в БД
            OCIAttrGet( paramhp, OCI_DTYPE_PARAM, &u2, NULL, OCI_ATTR_DATA_SIZE, secerrhp );
            AFieldData->dbsize = u2;
            // определяем тип данных
            OCIAttrGet( paramhp, OCI_DTYPE_PARAM, &u2, NULL, OCI_ATTR_DATA_TYPE, secerrhp );
            AFieldData->dbtype = u2;
            // не умеем работать с типами
            switch ( AFieldData->dbtype ) {
             case SQLT_RID:
             case SQLT_RDD:
             case SQLT_FILE:
             case SQLT_BLOB:
             case SQLT_CLOB:
             case SQLT_RSET:
             case SQLT_OSL:
             case SQLT_NTY:
             case SQLT_REF:
               throw EOracleError( "Unsupported field type", 0, SQLText.SQLText());
            };
            // определяем название поля
            cbufl = 30;
            OCIAttrGet( paramhp, OCI_DTYPE_PARAM, &cbufp, &cbufl, OCI_ATTR_NAME, secerrhp );
            if ( cbufl > 30 ) cbufl = 30;
            AFieldData->SetName( cbufp, cbufl );
            // ТОЛЬКО ДЛЯ ТИПА NUMBER(N,B)
            // определяем точность поля - кол-во чисел для числовых данных:
            // 0 - задан, -127 - это тип FLOAT
            OCIAttrGet(paramhp, OCI_DTYPE_PARAM, &u1, NULL, OCI_ATTR_PRECISION, secerrhp );
            AFieldData->prec = u1; // ко-во знаков до запятой
            OCIAttrGet( paramhp, OCI_DTYPE_PARAM, &s1, NULL, OCI_ATTR_SCALE, secerrhp );
            AFieldData->scale = s1; // ко-во знаков после запятой
            OCIAttrGet( paramhp, OCI_DTYPE_PARAM, &u1, NULL, OCI_ATTR_IS_NULL, secerrhp );
            AFieldData->nullok = u1; // 0 - NOT NULL, ELSE NULL
            OCICall( OCIDescriptorFree( paramhp, OCI_DTYPE_PARAM ) );
         };
      }
     else
      {
        sb4 cbufl;
        sb4 dsize;
        sb4 dbsize;
        sb2 prec;
        sb2 scale;
        sb2 nullok;
        sb1 name[ 31 ];
        cbufl = 30;
        odescr( &cda, Pos, &dbsize, &s2,
                name, &cbufl, &dsize,
                &prec, &scale, &nullok );
        int err = ReturnCode( );
        EndFields = ( err == 1007 );
        if ( EndFields ) {
          cda.rc = 0;
          break;
        }
        else
         {
           AFieldData = AFields.CreateFieldData( &MM );
           AFieldData->dbsize = dbsize;
           AFieldData->prec = prec;
           AFieldData->scale = scale;
           AFieldData->nullok = nullok;
           AFieldData->SetName( (char*)name, (unsigned int)cbufl );
           AFieldData->dbtype = s2;
         };
        if ( !EndFields && err  )
          RaiseOracleError( );
      };

     if ( ReturnCode( ) )
      {
        AFields.Clear( ); // GetErrorLocation; ParsedSQL = '';
        RaiseOracleError( );
        break;
      };
    // определим размер буфера для каждого поля
     switch ( AFieldData->dbtype ) {
      case OCI_TYPECODE_DATE:
             AFieldData->buftype = otDate;
             AFieldData->buf_size = 7;
             break;
      case OCI_TYPECODE_NUMBER:
             if ( ( AFieldData->scale != -127 )&& //FLOAT;
                  !( AFieldData->prec == 0 && AFieldData->scale == 0 ) && // NUMBER;
                   ( AFieldData->scale <= 0 )&&( AFieldData->prec <= 9 ) ) // NUMBER(P,S);
               {
                 AFieldData->buftype = otInteger;
                 AFieldData->buf_size = sizeof( int );
               }
              else if ( AFieldData->prec <= 126 )
                    {
                      AFieldData->buftype = otFloat;
                      AFieldData->buf_size = sizeof( double );
                     }
                    else
                     {
                       AFieldData->buftype = otString;
                       if ( AFieldData->scale > 0 )
                        AFieldData->buf_size = AFieldData->prec + 3;
                       else AFieldData->buf_size = 2;
                       AFieldData->buf_size += ( 38 + 127 );
                     };
               break;
       case SQLT_BIN:
              AFieldData->buftype = otString;
              AFieldData->buf_size = ( AFieldData->dbsize * 2 ) + 1;
              break;
       case SQLT_LNG:
              AFieldData->buftype = otLong;
              AFieldData->buf_size = 0;
              UsedCache = 1;
              break;
       case SQLT_LBI:
              AFieldData->buftype = otLongRaw;
              AFieldData->buf_size = 0;
              UsedCache = 1;
              break;
       default:
              AFieldData->buftype = otString;
              AFieldData->buf_size = AFieldData->dbsize + 1;
     };
     Pos++;
   };

 // выделяем память для данных в полях
 int FieldCounts = AFields.GetFieldsCount();
 for( int i=0; i<FieldCounts; i++ )
  {
    TFieldData &AFieldData = *AFields.GetFieldData( i );
    AFieldData.cache = UsedCache;
    if ( AFieldData.buf_size > 0 )
     {
       AFieldData.buf = (char*)MM.malloc( AFieldData.buf_size*UsedCache, STDLOG );
       if ( !AFieldData.buf )
         throw EMemoryError( "Can not allocate memory" );
     }
    else AFieldData.buf = NULL;
    AFieldData.arind = (sb2*)MM.malloc( sizeof( sb2 ) * UsedCache, STDLOG );
    AFieldData.arlen = (ub2*)MM.malloc( sizeof( ub2 ) * UsedCache, STDLOG );
    if ( ( !AFieldData.arlen )||( !AFieldData.arind ) )
      throw EMemoryError( "Can not allocate memory" );
  };
};

sb4 CallbackDefine( void *octxp, OCIDefine *defnp, ub4 iter,
                    void **bufpp, ub4 **alenp, ub1 *piece,
                    void **indp,  ub2 **rcodep ) // send params right to left
{
   int Result = OCI_CONTINUE;
   TFieldData &AFieldData = *(TFieldData*)octxp;
   int PieceSize;
   if ( AFieldData.LongItemsCount )
    {
       PieceSize = AFieldData.LongValue[ AFieldData.LongItemsCount - 1 ].AllocatedSize;
       if ( PieceSize < 32000 ) PieceSize = 2 * PieceSize;
    }
   else PieceSize = 2000;
   AFieldData.LongItemsCount++;
   AFieldData.LongValue = (TLongItems*)AFieldData.MM->realloc( AFieldData.LongValue, sizeof( TLongItems )*AFieldData.LongItemsCount, STDLOG );
   if ( !AFieldData.LongValue )
     throw EMemoryError( "Can not allocate memory" );
   AFieldData.LongValue[ AFieldData.LongItemsCount - 1 ].PieceBuf = new char[ PieceSize ];
   AFieldData.MM->create( AFieldData.LongValue[ AFieldData.LongItemsCount - 1 ].PieceBuf, STDLOG );
   AFieldData.LongValue[ AFieldData.LongItemsCount - 1 ].ActualSize = PieceSize;
   AFieldData.LongValue[ AFieldData.LongItemsCount - 1 ].AllocatedSize = PieceSize;
   *bufpp = (void*)AFieldData.LongValue[ AFieldData.LongItemsCount - 1 ].PieceBuf;
   *alenp = &AFieldData.LongValue[ AFieldData.LongItemsCount - 1 ].ActualSize;
   *indp = (void*)AFieldData.arind;
   rcodep = NULL;
   return Result;
};

void TQuery::Define( )
{
   ub4 mode;
   sb4 lbufsize;
   defhp = NULL;
   int FieldsCount = Fields->GetFieldsCount();
   for( int i=0; i<FieldsCount; i++ )
    {
      TFieldData &AFieldData = *Fields->GetFieldData( i );
      if ( Session->isOCI8() )
       {
         if ( ( AFieldData.buftype == otLong )||
              ( AFieldData.buftype == otLongRaw ) )
          {
             mode = OCI_DYNAMIC_FETCH;
             lbufsize = 0x7FFFFFFF; //MaxInt
          }
         else
          {
             mode = OCI_DEFAULT;
             lbufsize = AFieldData.buf_size;
          };
   /*      if ( ( AFieldData.buftype == otInteger )||( AFieldData.buftype == otFloat ) )
           lbuftype = OCI_TYPECODE_NUMBER;
         else lbuftype = AFieldData.buftype;  */
         OCICall( OCIDefineByPos( stmthp, &defhp, errhp, i + 1, AFieldData.buf, lbufsize, AFieldData.buftype,
   //      OCICall( OCIDefineByPos( stmthp, &defhp, errhp, i + 1, AFieldData.buf, lbufsize, lbuftype,
                                  AFieldData.arind, AFieldData.arlen, NULL, mode ) );
         if ( ( !ReturnCode( ) )&&( mode == OCI_DYNAMIC_FETCH ) )
           OCICall( OCIDefineDynamic( defhp, errhp, &AFieldData, &CallbackDefine  ) );
       }
      else
       {
         odefin( &cda, i + 1, (ub1*)AFieldData.buf, AFieldData.buf_size, AFieldData.buftype, -1,
                 AFieldData.arind, NULL, -1, -1, AFieldData.arlen, NULL );
       };
      if ( ReturnCode( ) ) RaiseOracleError( );
    };
};

void TQuery::InitPieces( )
{
  int FieldsCount = Fields->GetFieldsCount();
  for ( int i=0; i<FieldsCount; i++ )
   {
     TFieldData &FieldData = *Fields->GetFieldData( i );
    if ( ( FieldData.dbtype == otLong )||
         ( FieldData.dbtype == otLongRaw ) )
      FieldData.ClearLongData( );
   }
}


void TQuery::Execute( )
{
  int iters; // для не OCI_STMT_SELECT задает время выполнения; 0 = OCI_STMT_SELECT
             // если не OCI_STMT_SELECT и =0 => ошибка
  int cerror;
  ub4 ExecMode = OCI_DEFAULT;
  if ( !Session )
    throw EOracleError( "Session undefined", 0 );
  if ( !Session->isConnect() )
   throw EOracleError( "Not logged on", 0 );
  Eof = 1;
  Parser( ); // разборка и привязка текста
  BindVariables( ); // создание и привязка переменных
  // если SELECT statement we need to bind, describe and fetch

  if (
       (Session->isOCI8() && FFunctionType == OCI_STMT_SELECT) ||
       (!Session->isOCI8() && FFunctionType == 4/*FT_SELECT*/)
     )
   {
     iters = 0;
     if ( Session->isOCI8() )
      {
        OCICall( OCIStmtExecute( Session->Getsvchp(), stmthp, errhp, iters, 0, NULL, NULL, ExecMode ) );
        cerror = ReturnCode( );

        if ( ( cerror )&&( /*ReturnCode( )*/cerror != OCI_NO_DATA/*1403*/ ) )
          RaiseOracleError( );
      };
     Eof = 1; // true
     Describe( ); // инициализируем поля
     Define( ); // задаем размеры возвращаемых данных
     InitPieces( );
     if ( Session->isOCI8() )
       OCICall( OCIStmtFetch( stmthp, errhp, UsedCache, OCI_FETCH_NEXT, OCI_DEFAULT ) );
     else
       oexfet( &cda, UsedCache, 0, 0 );
     cerror = ReturnCode( );
     if ( ( cerror )&&( cerror != OCI_NO_DATA/*1403*/ ) )
       RaiseOracleError( );
     CacheIndex = 0;
     Cached = RowsProcessed( );
     Eof = !Cached;
   }
  else // это не select
  {
    if ( Session->isOCI8() )
     {
       OCICall( OCIStmtExecute( Session->Getsvchp(), stmthp, errhp, 1, 0, NULL, NULL, ExecMode ) );
       OCIAttrGet( stmthp, OCI_HTYPE_STMT, &FFunctionType, NULL, OCI_ATTR_SQLFNCODE, secerrhp );
     }
    else
     {
       oexn( &cda, /*iters*/1, 0 );
       FFunctionType = cda.ft;
     };
    if ( ReturnCode( ) ) RaiseOracleError( );
  };
};

int TQuery::RowsProcessed( )
{
  int Result;
  if ( !FOpened )
    Result = 0;
  else if ( Session->isOCI8() )
        OCIAttrGet( stmthp, OCI_HTYPE_STMT, &Result, NULL, OCI_ATTR_ROW_COUNT, secerrhp );
       else Result = cda.rpc;
  return Result;
};

int TQuery::RowCount( )
{
  if ( Eof ) return RowsProcessed( );
  else return ( RowsProcessed( ) - Cached ) + ( CacheIndex + 1 );
};

void TQuery::Next( )
{
  if ( Eof ) return;
  if ( !Session )
   throw EOracleError( "Session undefined", 0 );
  if ( !Session->isConnect() )
   throw EOracleError( "Not logged on", 0 );
  CacheIndex++;
  if ( CacheIndex >= Cached )
  {
   if ( Cached < UsedCache ) Eof = 1;
   else
    {
      int oldrp = RowsProcessed( );
      // очистить поля, которые имеют тип Long, LongRow
      if ( UsedCache == 1 )
        InitPieces( );
      if ( Session->isOCI8() )
        OCICall( OCIStmtFetch( stmthp, errhp, UsedCache, OCI_FETCH_NEXT, OCI_DEFAULT ) );
      else
        ofen( &cda, UsedCache );
      int cerror = ReturnCode( );
      if ( ( cerror )&&( cerror != /*1403*/OCI_NO_DATA ) )
        RaiseOracleError( );
      CacheIndex = 0;
      Cached = RowsProcessed( ) - oldrp;
      Eof = !Cached;
    };
  };
};

int TQuery::FieldsCount( )
{
  return Fields->GetFieldsCount();
};

int TQuery::GetSizeField( int FieldId )
{
  if ( FieldIsNULL( FieldId ) )
    return 0;
  else
    return Fields->GetFieldData( FieldId )->buf_size;
}

int TQuery::GetSizeField( const char *name )
{
  return GetSizeField( FieldIndex( name ) );
}

int TQuery::GetSizeField( const string &name )
{
  return GetSizeField( name.c_str() );
}

const char *TQuery::FieldName( int FieldId )
{
  if ( ( FieldId < 0 )||( FieldId >= FieldsCount( ) ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Field %d does not exists", FieldId );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   }
  else return Fields->GetFieldData( FieldId )->GetName();
};

otFieldType TQuery::FieldType( int FieldId )
{
  if ( ( FieldId < 0 )||( FieldId >= FieldsCount( ) ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Field %d does not exists", FieldId );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   }
  else return Fields->GetFieldData( FieldId )->buftype;
};

int TQuery::GetFieldIndex( const string &name )
{
  string StrValue = upperc( name );
  int FieldId = -1;
  int FieldsCount = Fields->GetFieldsCount();
  for ( int i=0; i<FieldsCount; i++ )
   {
     if ( StrValue == Fields->GetFieldData( i )->GetName() )
      {
        FieldId = i;
        break;
      };
   };
  return FieldId;
}

int TQuery::FieldIndex( const char* name )
{
  int FieldId = GetFieldIndex( name );
  if ( FieldId == -1 )
   {
     char serror[ 100 ];
     sprintf( serror, "Field %s does not exists", name );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   };
  return FieldId;
}

int TQuery::FieldIndex( const string &name )
{
  return FieldIndex( name.c_str() );
}

int TQuery::FieldIsNULL( int FieldId )
{
  if ( ( FieldId < 0 )||( FieldId >= FieldsCount( ) ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Field %d does not exists", FieldId );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   }
  else
   {
      CheckEOF( );
      return ( Fields->GetFieldData( FieldId )->arind[ CacheIndex ] == -1 );
   };
}

int TQuery::FieldIsNULL( const char *name )
{
  return FieldIsNULL( FieldIndex( name ) );
}

int TQuery::FieldIsNULL( const string &name )
{
  return FieldIsNULL( name.c_str() );
}

void TQuery::CheckEOF( )
{
  if ( Eof )
    throw EOracleError( "You cannot access field data beyond Eof ", 0, SQLText.SQLText() );
}

void *TQuery::FieldData( int FieldId )
{
  if ( ( FieldId < 0 )||( FieldId >= FieldsCount( ) ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Field %d does not exists", FieldId );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   }
  else
   {
     CheckEOF( );
     TFieldData &AFieldData = *Fields->GetFieldData( FieldId );
     return &AFieldData.buf[ CacheIndex*AFieldData.buf_size ];
   };
};

int TQuery::ValueAsInteger( void *Data, int &Value, otFieldType atype )
{
  switch ( atype ) {
    case otInteger:
      Value = *(int*)Data;
      break;
    case otFloat:
      Value = (int)(*(double*)Data);
      break;
    case otDate:
      {
        TDateTime VDate;
        if ( ConvertORACLEDate_TO_DateTime( Data, VDate ) != EOF )
         {
           Value = (int)VDate;
           break;
         };
      };
    case otString:
      if ( StrToInt( (char*)Data, Value ) != EOF ) break;
    default:
      return EOF;
  };
 return 0;
}

int TQuery::FieldAsInteger( int FieldId )
{
  int Value;
  if ( FieldIsNULL( FieldId ) ) Value = 0;
  else
   {
     void *Data;
     TFieldData &AFieldData = *Fields->GetFieldData( FieldId );
     Data = FieldData( FieldId );
     if ( ValueAsInteger( Data, Value, AFieldData.buftype ) == EOF )
      {
        char serror[ 100 ];
        Value = 0;
        sprintf( serror, "Cannot convert field %s to an Integer", AFieldData.GetName() );
        throw EConvertError( serror );
      };
   };
 return Value;
}

int TQuery::FieldAsInteger( const char *name )
{
  return FieldAsInteger( FieldIndex( name ) );
}

int TQuery::FieldAsInteger( const string &name )
{
  return FieldAsInteger( name.c_str() );
}

int TQuery::ValueAsFloat( void *Data, double &Value, otFieldType atype )
{
  switch ( atype ) {
    case otInteger:
      Value = *(int*)Data;
      break;
    case otFloat:
      Value = *(double*)Data;
      break;
    case otDate:
      if ( ConvertORACLEDate_TO_DateTime( Data, Value ) != EOF ) break;
    case otString:
      if ( StrToFloat( (char*)Data, Value ) != EOF ) break;
    default:
      return EOF;
  };
 return 0;
}

double TQuery::FieldAsFloat( int FieldId )
{
  double Value;
  if ( FieldIsNULL( FieldId ) ) Value = 0.0;
  else
   {
     void *Data;
     TFieldData &AFieldData = *Fields->GetFieldData( FieldId );
     Data = FieldData( FieldId );
     if ( ValueAsFloat( Data, Value, AFieldData.buftype ) == EOF )
      {
        char serror[ 100 ];
        Value = 0.0;
        sprintf( serror, "Cannot convert field %s to an Float", AFieldData.GetName() );
        throw EConvertError( serror );
      };
   };
 return Value;
}

double TQuery::FieldAsFloat( const char *name )
{
  return FieldAsFloat( FieldIndex( name ) );
}

double TQuery::FieldAsFloat( const string &name )
{
  return FieldAsFloat( name.c_str() );
}

int TQuery::ValueAsString( void *Data, char *Value, otFieldType atype, sb2 scale )
{
  switch ( atype ) {
    case otInteger:
      if ( sprintf( Value, "%d", *(int*)Data ) >= 0 ) break;
    case otFloat:
      if ( sprintf( Value, "%.*f", scale, *(double*)Data ) >= 0 ) break;
    case otDate:
      if ( ConvertORACLEDate_TO_Str( Data, Value ) != EOF ) break;
    case otString:
      strcpy( Value, (char*)Data );
      break;
    default:
     return EOF;
  };
 return 0;
}

char *TQuery::FieldAsString( int FieldId )
{
  TFieldData &AFieldData = *Fields->GetFieldData( FieldId );
  int flen = 0;
  int scale;
  if ( FieldIsNULL( FieldId ) ) return (char*)emptyStr;
  else
   {
     scale = AFieldData.scale;
     void *Data;
     Data = FieldData( FieldId );
     switch ( AFieldData.buftype ) {
       case otString:
         flen = AFieldData.dbsize + 1;
           break;
       case otInteger:
         flen = 11;
         break;
       case otFloat:
          if ( scale < 0 ) //???<= <
            scale = 6; //???
         if ( AFieldData.prec > 0 && AFieldData.scale != -127 ) {
           flen = AFieldData.prec + 2 /* знак + \0 */;
           flen += scale;
           if ( AFieldData.scale > 0 )
             flen += 1; /* точка */
          }
         else
           flen = ( 38 + 130 ); //??? 38 целых и 127 дробных + точка + знак
         break;
       case otDate:
         flen = 20;
         break;
       default:
         flen = EOF;
     };
     if ( AFieldData.cbuf_len < flen )
      {
        AFieldData.cbuf_len = flen;
        AFieldData.cbuffer = (char*)MM.realloc( AFieldData.cbuffer, AFieldData.cbuf_len, STDLOG );
        if ( !AFieldData.cbuffer ) EMemoryError( "Can not allocate memory" );
      };
     if ( ( flen == EOF )||( ValueAsString( Data, AFieldData.cbuffer, AFieldData.buftype,
                                                  AFieldData.scale ) == EOF ) )
      {
        char serror[ 100 ];
        AFieldData.cbuffer = 0;
        sprintf( serror, "Cannot convert field %s to an String", AFieldData.GetName() );
        throw EConvertError( serror );
      };
   };
 return AFieldData.cbuffer;
}

char *TQuery::FieldAsString( const char *name )
{
  return FieldAsString( FieldIndex( name ) );
}

char *TQuery::FieldAsString( const string &name )
{
  return FieldAsString( name.c_str() );
}

int TQuery::ValueAsDateTime( void *Data, TDateTime &Value, otFieldType atype )
{
  switch ( atype ) {
    case otInteger:
      Value = *(int*)Data;
      break;
    case otFloat:
      Value = *(double*)Data;
      break;
    case otDate:
      if ( ConvertORACLEDate_TO_DateTime( Data, Value ) != EOF ) break;
    case otString:
      if ( StrToDateTime( (char*)Data, Value ) != EOF ) break;
    default:
      return EOF;
   };
  return 0;
}

TDateTime TQuery::FieldAsDateTime( int FieldId )
{
  TDateTime Value;
  if ( FieldIsNULL( FieldId ) ) Value = 0.0;
  else
   {
     void *Data;
     TFieldData &AFieldData = *Fields->GetFieldData( FieldId );
     Data = FieldData( FieldId );
     if ( ValueAsDateTime( Data, Value, AFieldData.buftype ) == EOF )
      {
        char serror[ 100 ];
        Value = 0.0;
        sprintf( serror, "Cannot convert field %s to an DateTime", AFieldData.GetName() );
        throw EConvertError( serror );
      };
   };
 return Value;
}

TDateTime TQuery::FieldAsDateTime( const char *name )
{
  return FieldAsDateTime( FieldIndex( name ) );
}

TDateTime TQuery::FieldAsDateTime( const string &name )
{
  return FieldAsDateTime( name.c_str() );
}

void TQuery::InitLongField( int FieldId )
{
  int PieceSize = 2000;
  TFieldData &AFieldData = *Fields->GetFieldData( FieldId );
  if ( AFieldData.LongItemsCount ) /* allready */
    return;
  ub4 ret = PieceSize;
  sb4 offset = 0;
  while ( ret )
   {
     if ( AFieldData.LongItemsCount )
       AFieldData.LongValue = (TLongItems*)MM.realloc( AFieldData.LongValue, sizeof( TLongItems )*( AFieldData.LongItemsCount + 1 ), STDLOG );
     else
       AFieldData.LongValue = (TLongItems*)MM.malloc( sizeof( TLongItems ), STDLOG );
     char *buflong = AFieldData.LongValue[ AFieldData.LongItemsCount ].PieceBuf = new char[ PieceSize ];
     MM.create( AFieldData.LongValue[ AFieldData.LongItemsCount ].PieceBuf, STDLOG );
     AFieldData.LongValue[ AFieldData.LongItemsCount ].AllocatedSize = PieceSize;
     ub4 oldrpc = cda.rpc;
     oflng( &cda, FieldId + 1, (ub1*)buflong, PieceSize, AFieldData.buftype, &ret, offset );
     cda.rpc = oldrpc;
     if ( ReturnCode( ) )
       RaiseOracleError( );
     AFieldData.LongValue[ AFieldData.LongItemsCount ].ActualSize = ret;
     AFieldData.LongItemsCount++;
     if ( ret < (ub4)PieceSize )
       break;
     offset += ret;
   };
}

int TQuery::GetSizeLongField( int FieldId )
{

  if ( ( FieldId < 0 )||( FieldId >= FieldsCount( ) ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Field %d does not exists", FieldId );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   };
  TFieldData &AFieldData = *Fields->GetFieldData( FieldId );
  if ( ( AFieldData.buftype != otLong )&&( AFieldData.buftype != otLongRaw ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Field %d does not type Long", FieldId );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   };
  int Result = 0;
  TLongItems ALongValue;
  if ( !Session->isOCI8() )
    InitLongField( FieldId );
  for ( int i=0; i<AFieldData.LongItemsCount; i++ )
   {
     ALongValue = AFieldData.LongValue[ i ];
     Result += ALongValue.ActualSize;
   };
  return Result;
}

int TQuery::GetSizeLongField( const char *name )
{
  return GetSizeLongField( FieldIndex( name ) );
}

int TQuery::GetSizeLongField( const string &name )
{
  return GetSizeLongField( name.c_str() );
}

int TQuery::FieldAsLong( int FieldId, void *Value )
{
  int Result = 0;
  void *Valuep;
  if ( GetSizeLongField( FieldId ) )
   {
     TFieldData &AFieldData = *Fields->GetFieldData( FieldId );
     TLongItems ALongValue;
     Valuep = Value;
     for ( int i=0; i<AFieldData.LongItemsCount; i++ )
      {
        ALongValue = AFieldData.LongValue[ i ];
        memcpy( Valuep, ALongValue.PieceBuf, ALongValue.ActualSize );
        Result += ALongValue.ActualSize;
        Valuep = (char*)Valuep + ALongValue.ActualSize;
      };
   };
 return Result;
}

int TQuery::FieldAsLong( const char *name, void *Value )
{
  return FieldAsLong( FieldIndex( name ), Value );
}

int TQuery::FieldAsLong( const string &name, void *Value )
{
  return FieldAsLong( name.c_str(), Value );
}

int TQuery::VariablesCount( void )
{
  return Variables->GetVariablesCount();
}

void TQuery::ClearVariables( )
{
  Variables->Clear( );
}

void TQuery::Clear( )
{
  Variables->Clear( );
  SQLText.Clear();
  Close( );
}

void TQuery::CreateVariable( const char *name, otFieldType atype, tnull Data )
{
   DeclareVariable( name, atype );
   SetVariable( name, Data );
}

void TQuery::CreateVariable( const string &name, otFieldType atype, tnull Data )
{
  CreateVariable( name.c_str(), atype, Data );
}

void TQuery::CreateVariable( const char *name, otFieldType atype, int Data )
{
  DeclareVariable( name, atype );
  SetVariable( name, Data );
}

void TQuery::CreateVariable( const string &name, otFieldType atype, int Data )
{
  CreateVariable( name.c_str(), atype, Data );
}

void TQuery::CreateVariable( const char *name, otFieldType atype, double Data )
{
  DeclareVariable( name, atype );
  SetVariable( name, Data );
}

void TQuery::CreateVariable( const string &name, otFieldType atype, double Data )
{
  CreateVariable( name.c_str(), atype, Data );
}

void TQuery::CreateVariable( const char *name, otFieldType atype, const char *Data )
{
  DeclareVariable( name, atype );
  SetVariable( name, Data );
}

void TQuery::CreateVariable( const string &name, otFieldType atype, const char *Data )
{
  CreateVariable( name.c_str(), atype, Data );
}

void TQuery::CreateVariable( const string &name, otFieldType atype, const string &Data )
{
   DeclareVariable( name.c_str(), atype );
   SetVariable( name.c_str(), Data.c_str() );
}

void TQuery::CreateVariable( const char *name, otFieldType atype, void *Data )
{
   DeclareVariable( name, atype );
   SetVariable( name, Data );
}

void TQuery::CreateVariable( const string &name, otFieldType atype, void *Data )
{
  CreateVariable( name.c_str(), atype, Data );
}

void TQuery::CreateLongVariable( const char *name, otFieldType atype, void *Data, int Length )
{
   DeclareVariable( name, atype );
   SetLongVariable( name, Data, Length );
}

void TQuery::CreateLongVariable( const string &name, otFieldType atype, void *Data, int Length )
{
  CreateLongVariable( name.c_str(), atype, Data, Length );
}

void TQuery::DeclareVariable( const char *name, otFieldType atype )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot declare a variable with an empty name", 1);
  else
  {
    int VariableId = Variables->FindVariable( name );
    if ( VariableId >= 0 )
      Variables->DeleteVariable( VariableId );
    if ( Session )
      Close( );
    TVariableData &AVariableData = *Variables->CreateVariable( );
    AVariableData.buftype = atype;
    AVariableData.SetName( name );
    AVariableData.indp = (sb2*)MM.malloc( sizeof( sb2 ), STDLOG );
    if ( !AVariableData.indp )
      throw EMemoryError( "Can not allocate memory" );
    *AVariableData.indp = -1;
    switch ( AVariableData.buftype ) {
      case otDate:
        AVariableData.buf_size = 7;
        break;
      case otInteger:
        AVariableData.buf_size = sizeof( int );
        break;
      case otFloat:
        AVariableData.buf_size = sizeof( double );
        break;
      case otString:
      case otChar:
        AVariableData.buf_size = 4000 + 1; // макс. число байт для данного типа Oracle
        break;
      case otLong:
      case otLongRaw:
       AVariableData.buf_size = 0;
    };
    if ( AVariableData.buf_size > 0 )
     {
       AVariableData.buf = MM.malloc( AVariableData.buf_size, STDLOG );
       if ( !AVariableData.buf )
         throw EMemoryError( "Can not allocate memory" );
     }
    else AVariableData.buf = NULL;
  };
}

void TQuery::DeclareVariable( const string &name, otFieldType atype )
{
  DeclareVariable( name.c_str(), atype );
}

void TQuery::DeleteVariable( const char *name )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot delete a variable with an empty name", 1);
  int VariableId = Variables->FindVariable( name );
  if ( VariableId < 0 ) {
    char serror[ 100 ];
    sprintf( serror, "Variable %s does not exists", name );
    throw EOracleError( serror, 0, SQLText.SQLText() );
  }
  Variables->DeleteVariable( VariableId );
  if ( Session )
    Close( );
}

void TQuery::DeleteVariable( const string &name )
{
  DeleteVariable( name.c_str() );
}

const char *TQuery::VariableName( int VariableId )
{
  if ( ( VariableId < 0 )||( VariableId >= VariablesCount( ) ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Variable %d does not exists", VariableId );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   }
  else
    return Variables->GetVariableData( VariableId )->GetName();
}

int TQuery::GetVariableIndex( const string &name )
{
  return Variables->FindVariable( name.c_str() );
}

int TQuery::VariableIndex( const char* name )
{
  int VariableId = Variables->FindVariable( name );
  if ( VariableId != -1 )
    return VariableId;
  else
   {
     char serror[ 100 ];
     sprintf( serror, "Variable %s does not exists", name );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   };
}

int TQuery::VariableIndex( const string &name )
{
  return VariableIndex( name.c_str() );
}

int TQuery::VariableIsNULL( int VariableId )
{
  if ( ( VariableId < 0 )||( VariableId >= VariablesCount( ) ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Variable %d does not exists", VariableId );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   }
  else
    return ( *Variables->GetVariableData( VariableId )->indp == -1 );
}

int TQuery::VariableIsNULL( const char *name )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot get value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  return VariableIsNULL( VariableId );
}

int TQuery::VariableIsNULL( const string &name )
{
  return VariableIsNULL( name.c_str() );
}

int TQuery::GetVariableAsInteger( int VariableId )
{
  int Variable;
  if ( VariableIsNULL( VariableId ) )
    Variable = 0;
  else
   {
     TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
     if ( ValueAsInteger( AVariableData.buf, Variable, AVariableData.buftype ) == EOF )
      {
        char serror[ 100 ];
        sprintf( serror, "Cannot convert variable %s to an Integer", (char*)AVariableData.GetName() );
        throw EConvertError( serror );
      };
   };
  return Variable;
}

int TQuery::GetVariableAsInteger( const char *name )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot get value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  return GetVariableAsInteger( VariableId );
}

int TQuery::GetVariableAsInteger( const string &name )
{
  return GetVariableAsInteger( name.c_str() );
}

double TQuery::GetVariableAsFloat( int VariableId )
{
  double Variable;
  if ( VariableIsNULL( VariableId ) )
    Variable = 0.0;
  else
   {
     TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
     if ( ValueAsFloat( AVariableData.buf, Variable, AVariableData.buftype ) == EOF )
      {
        char serror[ 100 ];
        sprintf( serror, "Cannot convert variable %s to an Float", (char*)AVariableData.GetName() );
        throw EConvertError( serror );
      };
   };
  return Variable;
}

double TQuery::GetVariableAsFloat( const char *name )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot get value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  return GetVariableAsFloat( VariableId );
}

double TQuery::GetVariableAsFloat( const string &name )
{
  return GetVariableAsFloat( name.c_str() );
}

char *TQuery::GetVariableAsString( int VariableId )
{
  if ( VariableIsNULL( VariableId ) ) return (char*)emptyStr;
  else
   {
     int flen = 0;
     TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
     switch ( AVariableData.buftype ) {
       case otString:
        flen = strlen( (char*)AVariableData.buf ) + 1;
        break;
       case otInteger:
         flen = 11;
         break;
       case otFloat:
         flen = ( 38 + 130 );
         break;
       case otDate:
         flen = 20;
         break;
       default:
         flen = EOF;
     };
     if ( AVariableData.cbuf_len < flen )
      {
        AVariableData.cbuf_len = flen;
        AVariableData.cbuffer = (char*)MM.realloc( AVariableData.cbuffer, AVariableData.cbuf_len, STDLOG );
        if ( !AVariableData.cbuffer )
          EMemoryError( "Can not allocate memory" );
      };
     if ( ( flen == EOF )||( ValueAsString( AVariableData.buf, AVariableData.cbuffer, AVariableData.buftype,
                                                               6 ) == EOF ) )
      {
        char serror[ 100 ];
        AVariableData.cbuffer = 0;
        sprintf( serror, "Cannot convert field %s to an String", (char*)AVariableData.GetName() );
        throw EConvertError( serror );
      };
     return AVariableData.cbuffer;
   };
}

char *TQuery::GetVariableAsString( const char *name )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot get value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  return GetVariableAsString( VariableId );
}

char *TQuery::GetVariableAsString( const string &name )
{
  return GetVariableAsString( name.c_str() );
}

TDateTime TQuery::GetVariableAsDateTime( int VariableId )
{
  TDateTime Variable;
  if ( VariableIsNULL( VariableId ) )
    Variable = 0.0;
  else
   {
     TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
     if ( ValueAsDateTime( AVariableData.buf, Variable, AVariableData.buftype ) == EOF )
      {
        char serror[ 100 ];
        sprintf( serror, "Cannot convert variable %s to an DateTime", (char*)AVariableData.GetName() );
        throw EConvertError( serror );
      };
   };
  return Variable;
}

TDateTime TQuery::GetVariableAsDateTime( const char *name )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot get value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  return GetVariableAsDateTime( VariableId );
}

TDateTime TQuery::GetVariableAsDateTime( const string &name )
{
  return GetVariableAsDateTime( name.c_str() );
}

int TQuery::GetSizeLongVariable( const char *name )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot get size of variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
  if ( ( AVariableData.buftype != otLong )&&( AVariableData.buftype != otLongRaw ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Variable %s does not type Long", name );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   };
  return AVariableData.len;
}

int TQuery::GetSizeLongVariable( const string &name )
{
  return GetSizeLongVariable( name.c_str() );
}

int TQuery::GetVariableAsLong( const char *name, void *Value )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot get value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
  if ( ( AVariableData.buftype != otLong )&&( AVariableData.buftype != otLongRaw ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Variable %s does not type Long",name );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   };
  memcpy( Value, AVariableData.buf, AVariableData.len );
  return AVariableData.len;
}

int TQuery::GetVariableAsLong( const string &name, void *Value )
{
   return GetVariableAsLong( name.c_str(), Value );
}

void TQuery::SetVariable( const char *name, tnull Data )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot set value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
  *AVariableData.indp = -1;
}

void TQuery::SetVariable( const string &name, tnull Data )
{
  SetVariable( name.c_str(), Data );
}

void TQuery::SetVariable( const char *name, int Data )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot set value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
  *AVariableData.indp = 0;
  switch ( AVariableData.buftype ) {
    case otInteger:
      *(int*)AVariableData.buf = Data;
      break;
    case otFloat:
      *(double*)AVariableData.buf = (double)Data;
      break;
    case otDate:
      if ( ConvertDateTime_TO_ORACLEDate( (double)Data, AVariableData.buf ) != EOF )
        break;
      else
       {
         char serror[ 100 ];
         sprintf( serror, "Cannot convert variable %s to an OCIDate", (char*)AVariableData.GetName() );
         throw EConvertError( serror );
       };
    case otString:
    case otChar:
      sprintf( (char*)AVariableData.buf, "%d", Data );
      AVariableData.buf_size = strlen( (char*)AVariableData.buf ) + 1;
      break;
    case otLong:
    case otLongRaw:
    default:
      {
        char serror[ 100 ];
        sprintf( serror, "Cannot convert variable %s to an Long", (char*)AVariableData.GetName() );
        throw EConvertError( serror );
      };
  };
}

void TQuery::SetVariable( const string &name, int Data )
{
  SetVariable( name.c_str(), Data );
}

void TQuery::SetVariable( const char *name, double Data )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot set value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
  *AVariableData.indp = 0;
  switch ( AVariableData.buftype ) {
    case otInteger:
      *(int*)AVariableData.buf = (int)Data;
      break;
    case otFloat:
      *(double*)AVariableData.buf = Data;
      break;
    case otDate:
      if ( ConvertDateTime_TO_ORACLEDate( Data, AVariableData.buf ) != EOF )
        break;
      else
       {
         char serror[ 100 ];
         sprintf( serror, "Cannot convert variable %s to an OCIDate", (char*)AVariableData.GetName() );
         throw EConvertError( serror );
       };
    case otString:
    case otChar:
      sprintf( (char*)AVariableData.buf, "%f", Data );
      AVariableData.buf_size = strlen( (char*)AVariableData.buf ) + 1;
      break;
    case otLong:
    case otLongRaw:
    default:
      {
        char serror[ 100 ];
        sprintf( serror, "Cannot convert variable %s to an Long", (char*)AVariableData.GetName() );
        throw EConvertError( serror );
      };
  };
}

void TQuery::SetVariable( const string &name, double Data )
{
  SetVariable( name.c_str(), Data );
}

void TQuery::SetVariable( const char *name, const char *Data )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot set value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
  if ( !Data )
    *AVariableData.indp = -1;
  else
   {
     int j;
     *AVariableData.indp = 0;
     switch ( AVariableData.buftype ) {
       case otInteger:
         int i;
         if (StrToInt( (char*)Data, i ) != EOF)
          {
            *(int*)AVariableData.buf =  i;
            break;
          }
         else
          {
            char serror[ 100 ];
            sprintf( serror, "Cannot convert variable %s to an Integer", (char*)AVariableData.GetName() );
            throw EConvertError( serror );
          };
       case otFloat:
         double f;
         if (StrToFloat( (char*)Data, f ) != EOF)
          {
            *(double*)AVariableData.buf = f;
            break;
          }
         else
          {
            char serror[ 100 ];
            sprintf( serror, "Cannot convert variable %s to an Float", (char*)AVariableData.GetName() );
            throw EConvertError( serror );
          };
       case otDate:
         TDateTime VDate;
         if ( ( StrToDateTime( Data, VDate ) != EOF )&&
              ( ConvertDateTime_TO_ORACLEDate( VDate, AVariableData.buf ) != EOF ) )
           break;
         else
          {
            char serror[ 100 ];
            sprintf( serror, "Cannot convert variable %s to an OCIDate", (char*)AVariableData.GetName() );
            throw EConvertError( serror );
          };
       case otString:
       case otChar:
         j = strlen( (char*)Data );
         if ( j > AVariableData.buf_size ) j = AVariableData.buf_size;
         if ( j ) memcpy( AVariableData.buf, Data, j );
         ( (char*)AVariableData.buf )[ j ] = '\0';
         break;
       case otLong:
       case otLongRaw:
         AVariableData.FreeBuffer( );
         j = strlen( (char*)Data );
         if ( j )
          {
            if ( ( AVariableData.buf = MM.malloc( j + 1, STDLOG ) ) == NULL )
              throw EMemoryError( "Can not allocate memory for Value of long variable" );
            else
             {
               AVariableData.SetOwnBuffer( true );
               memcpy( AVariableData.buf, Data, j );
               ( (char*)AVariableData.buf )[ j ] = '\0';
               AVariableData.buf_size = j + 1;
               AVariableData.len = AVariableData.buf_size;
             };
          };
     };
   };
}

void TQuery::SetVariable( const string &name, const char *Data )
{
  SetVariable( name.c_str(), Data );
}

void TQuery::SetVariable( const string &name, const string &Data )
{
  SetVariable( name.c_str(), Data.c_str() );
}

void TQuery::SetVariable( const char *name, void *Data )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot set value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
  if ( !Data )
    *AVariableData.indp = -1;
  else
   {
     int j;
     *AVariableData.indp = 0;
      switch ( AVariableData.buftype ) {
        case otInteger:
          *(int*)AVariableData.buf = *(int*)Data;
          break;
        case otFloat:
          *(double*)AVariableData.buf = *(double*)Data;
          break;
        case otString:
        case otChar:
          j = strlen( (char*)Data );
          if ( j > AVariableData.buf_size ) j = AVariableData.buf_size;
          if ( j ) memcpy( AVariableData.buf, Data, j );
          ( (char*)AVariableData.buf )[ j ] = '\0';
          break;
        case otDate:
          if ( ConvertDateTime_TO_ORACLEDate( *(TDateTime*)Data, (OCIDate*)AVariableData.buf ) != EOF )
            break;
          else
           {
             char serror[ 100 ];
             sprintf( serror, "Cannot convert variable %s to an OCIDate", (char*)AVariableData.GetName() );
             throw EConvertError( serror );
           };
        case otLong:
        case otLongRaw:
          AVariableData.FreeBuffer( );
          j = strlen( (char*)Data );
          if ( j )
           {
             if ( ( AVariableData.buf = MM.malloc( j + 1, STDLOG ) ) == NULL )
               throw EMemoryError( "Can not allocate memory" );
             else
              {
                AVariableData.SetOwnBuffer( true );
                memcpy( AVariableData.buf, Data, j );
                ( (char*)AVariableData.buf )[ j ] = '\0';
                AVariableData.buf_size = j + 1;
                AVariableData.len = AVariableData.buf_size;
              };
           };
      };
   };
}

void TQuery::SetVariable( const string &name, void *Data )
{
  SetVariable( name.c_str(), Data );
}

void TQuery::SetLongVariable( const char *name, void *Data, int Length )
{
  if ( !strlen( name ) )
    throw EOracleError( "You cannot set value to variable with an empty name", 1 );
  int VariableId = VariableIndex( name );
  TVariableData &AVariableData = *Variables->GetVariableData( VariableId );
  if ( ( AVariableData.buftype != otLong )&&( AVariableData.buftype != otLongRaw ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Variable %s does not type Long", name );
     throw EOracleError( serror, 0, SQLText.SQLText() );
   };
  AVariableData.FreeBuffer( );
  if ( !Data )
    *AVariableData.indp = -1;
  else
   {
     *AVariableData.indp = 0;
     AVariableData.buf = Data;
     AVariableData.len = Length;
     AVariableData.buf_size = Length;
     AVariableData.SetOwnBuffer( false );
     AVariableData.bindhp = NULL;
   };
}

void TQuery::SetLongVariable( const string &name, void *Data, int Length )
{
  SetLongVariable( name.c_str(), Data, Length );
}

////////////////////////////////////////////////////////////////////////////


int ConvertORACLEDate_TO_DateTime( void *Value, TDateTime &VDateTime )
{
  unsigned char *AOCIDate = (unsigned char*)Value;
  int Year;
  char Month, Day, Hour, Minute, Second;
  TDateTime Date, TT;
  Year  = ( AOCIDate[ 0 ] - 100 ) * 100 + ( AOCIDate[ 1 ] - 100 );
  Month = AOCIDate[ 2 ];
  Day = AOCIDate[ 3 ];
  Hour = AOCIDate[ 4 ] - 1;
  Minute = AOCIDate[ 5 ] - 1;
  Second = AOCIDate[ 6 ] - 1;
  try {
    EncodeDate( Year, Month, Day, Date );
    EncodeTime( Hour, Minute, Second, TT );
    if ( Date < 0 ) VDateTime = Date - TT;
    else VDateTime = Date + TT;
    return 0;
    }
  catch(...) {
    return EOF;
  };
};

int ConvertDateTime_TO_ORACLEDate( const TDateTime &VDateTime, void *Value )
{
  int Year, Month, Day, Hour, Min, Sec;
  unsigned char *AOCIDate = (unsigned char*)Value;
  try {
    DecodeDate( VDateTime, Year, Month, Day );
    DecodeTime( VDateTime, Hour, Min, Sec );
    AOCIDate[ 0 ] = 100 + Year/100;
    AOCIDate[ 1 ] = 100 + Year%100;
    AOCIDate[ 2 ] = Month;
    AOCIDate[ 3 ] = Day;
    AOCIDate[ 4 ] = Hour + 1;
    AOCIDate[ 5 ] = Min + 1;
    AOCIDate[ 6 ] = Sec + 1;
    return 0;
    }
   catch(...) {
    return EOF;
   };
};

int ConvertORACLEDate_TO_Str( void *Data, char *Value )
{
  unsigned char *AOCIDate = (unsigned char*)Data;
  const int *TableDays;
  int Year;
  char Month, Day, Hour, Minute, Second;
  Year  = ( AOCIDate[ 0 ] - 100 ) * 100 + ( AOCIDate[ 1 ] - 100 );
  Month = AOCIDate[ 2 ];
  Day = AOCIDate[ 3 ];
  Hour = AOCIDate[ 4 ] - 1;
  Minute = AOCIDate[ 5 ] - 1;
  Second = AOCIDate[ 6 ] - 1;
  TableDays = MonthDays[ IsLeapYear( Year ) ];
  if ( ( Year>=1 )&&( Year<=9999 )&&( Month>=1 )&&( Month<=12 )&& \
       ( Day>=1 )&&( Day<=TableDays[ (int)Month ] )&& \
       ( Hour<24 )&&( Minute<60 )&&( Second<60 )&& \
       ( sprintf( Value, "%02d.%02d.%04d %02d:%02d:%02d", Day, Month, Year, Hour,\
                                                          Minute, Second ) != EOF ) ) \
   return 0;
  else return EOF;
};

void FindVariables( const string &SQL, bool IncludeDuplicates, vector<string> &vars )
{
  vars.clear();
  string s = SQL + "\xD\xA";
  char Mode = 'S';
  string EndC;
  string VarName;
  for (int i=0; i<(int)s.length(); i++) {
    unsigned char c;
    switch( Mode ) {
      case 'S':
        if ( s[ i ] == ':' ) {
          Mode = 'V';
          VarName.clear();
        }
        if ( s[ i ] == '\'') {
          Mode = 'Q';
          EndC = '\'';
        }
        if ( s[ i ] == '/' && s[ i + 1 ] == '*' ) {
          Mode = 'C';
          EndC = "*/";
        }
        if ( s[ i ] == '-' && s[ i + 1 ] == '-' ) {
          Mode = 'C';
          EndC = "\xD\xA";
        }
        break;
      case 'V':
        c = (unsigned char)s[ i ];
        if (  !((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '_' ||
                c == '#' ||
                c == '$' ||
                c >= (unsigned char)'\x80' )) {
          VarName = upperc( VarName );
          if ( !VarName.empty() && ( IncludeDuplicates || find( vars.begin(), vars.end(), VarName ) == vars.end() ) )
            vars.push_back( VarName );
          Mode = 'S';
        }
        else {
          VarName += s[ i ];
        }
       break;
      case 'C':
        if ( s[ i ] == EndC[ 0 ] && s[ i + 1 ] == EndC[ 1 ] )
          Mode = 'S';
        break;
      case 'Q':
        if ( s[ i ] == EndC[ 0 ] )
          Mode = 'S';
    }
  }
}

TSession OraSession;
