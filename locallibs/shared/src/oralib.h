//---------------------------------------------------------------------------
#ifndef oralibH
#define oralibH
#include <vector>
#include <string>
#include <oci.h>
#include "setup.h"
#include "date_time.h"
#include "exceptions.h"
#include "memory_manager.h"
#define HDA_SIZE 256
#include <stdio.h>
#include <map>

//#define SQL_COUNTERS

#ifdef SQL_COUNTERS
extern std::map<std::string,int> sqlCounters; 
extern int queryCount;
#endif

using BASIC::date_time::TDateTime;

//////////////////////// special for OCI function //////////////////////////////
sb4 CallbackDefine( void *octxp, OCIDefine *defnp, ub4 iter, \
                    void **bufpp, ub4 **alenp, ub1 *piece, \
                    void **indp,  ub2 **rcodep ); // send params right to left

//--------------------------------------------------------------------
enum TOCI { OCI8 = 1, OCI7 = 0 };
enum tconnect { Sconnect = 1, disconnect = 0 };
enum otFieldType { otInteger = SQLT_INT,
                   otFloat = SQLT_FLT,
                   otString = SQLT_STR,
                   otChar = SQLT_AVC,
                   otDate = OCI_TYPECODE_DATE,
                   otLong = SQLT_LNG,
                   otLongRaw = SQLT_LBI };
enum tnull { FNull };
const int MinElementCount = 5;

typedef struct LongItems {
    int AllocatedSize;
    ub4 ActualSize;
    char *PieceBuf;
} TLongItems;

class TSQLText {
  private:
    bool change;
    std::string sqlText;
    void SetData( const std::string &Value );
  public:
    TSQLText( );
    void operator= ( const std::string &Value );
    void operator= ( char *Value );
    void operator= ( const char *Value );
    const char *SQLText( );
    bool IsChange( );
    void CommitChange();
    bool IsEmpty();
    unsigned long Size();
    void Clear();
};

class TVariableData {
 private:
    std::string name;
    bool bufown;
    TMemoryManager *MM;
 public:
   OCIBind *bindhp; //OCIBind
   otFieldType buftype;
   int buf_size;
   void *buf;
   sb2 *indp;
   ub2 len;
   char *cbuffer;
   int cbuf_len;
   const char *GetName( );
   void SetName( const char *Value );
   TVariableData( TMemoryManager *AMM );
   ~TVariableData();
   void SetOwnBuffer( bool Value );
   bool IsOwnBuffer( );
   void FreeBuffer( );
};

class TFieldData {
 private:
   std::string name;
 public:
   TMemoryManager *MM;
   sb4 dbsize;
   sb2 dbtype;
   char *buf;
   sb4 buf_size;
   sb2 prec;
   sb2 scale;
   sb2 nullok;
   otFieldType buftype;
   int cache;
   char *cbuffer;
   int cbuf_len;
   TFieldData( TMemoryManager *AMM );
   ~TFieldData();
   TLongItems *LongValue;
   sb2 *arind;
   ub2 *arlen;
   void ClearLongData( );
   int LongItemsCount;
   const char *GetName( );
   void SetName( const char *Value, unsigned int len );
};

class TFields {
 private:
   std::vector<TFieldData*> FFields;
   TMemoryManager *MM;
 public:
   TFields( TMemoryManager *AMM );
   ~TFields( );
   void PreSetFieldsCount( int AColCount );
   TFieldData *CreateFieldData( TMemoryManager *AMM );
   TFieldData *GetFieldData( int Index );
   int GetFieldsCount();
   void Clear( );
};

class TVariables {
 private:
   std::vector<TVariableData*> FVariables;
   TMemoryManager *MM;
 public:
   TVariables( TMemoryManager *AMM );
   ~TVariables( );
   TVariableData *CreateVariable( );
   int GetVariablesCount( );
   TVariableData *GetVariableData( int Index );
   int FindVariable( const char *name );
   void DeleteVariable( char *name );
   void DeleteVariable( int VariableIndex );
   void Clear( );
};

class TSession;

class TQuery {
 private:
   OCIError *errhp, *secerrhp;
   OCIStmt *stmthp;
   OCIDefine *defhp;
   Cda_Def cda;
   int FFunctionType; // опаеделпев зво делаев запаоб 1 select, 2 update, 3 delete, 4 insert,
   sb4 LastOCIError;  // 5 create, ... 8,9 plsql
   int FCache;
   int UsedCache;
   tconnect FOpened;
   int CacheIndex;
   int Cached; // кол-во бваок в кние
   void OCICall( int err );
   int ReturnCode( );
   void RaiseOracleError( /*int ErrorHandle*/ );
   void BindVariables( );
   void Open( );
   void Parser( );
   void Describe( );
   void Define( );
   void AllocStmthp( );
   void InitLongField( int FieldId );
   void InitPieces( );
   void CheckEOF( );
   void *FieldData( int FieldId );
   int ValueAsInteger( void *Data, int &Value, otFieldType atype );
   int ValueAsFloat( void *Data, double &Value, otFieldType atype );
   int ValueAsString( void *Data, char *Value, otFieldType atype, sb2 scale );
   int ValueAsDateTime( void *Data, TDateTime &Value, otFieldType atype );
   //int ErrorHandle( );
   TSession* Session;
   TMemoryManager MM;
 public:
   TSQLText SQLText;
   TQuery( TSession *ASession );
   ~TQuery( );
   TFields *Fields;
   TVariables *Variables;

   void SetSession( TSession *ASession );

   void Close( );
   void Execute( );
   void Next( );
   int Eof;
   int RowsProcessed( );
   int RowCount( );

   int FieldsCount( void );
   const char *FieldName( int FieldId );
   otFieldType FieldType( int FieldId );
   int FieldIndex( const char* name );
   int FieldIndex( const std::string &name );
   int GetFieldIndex( const std::string &name );
   int FieldIsNULL( int FieldId );
   int FieldIsNULL( const char *name );
   int FieldIsNULL( const std::string &name );
   int FieldAsInteger( int FieldId );
   int FieldAsInteger( const char *name );
   int FieldAsInteger( const std::string &name );
   double FieldAsFloat( int FieldId );
   double FieldAsFloat( const char *name );
   double FieldAsFloat( const std::string &name );
   char *FieldAsString( int FieldId );
   char *FieldAsString( const char *name );
   char *FieldAsString( const std::string &name );
   TDateTime FieldAsDateTime( int FieldId );
   TDateTime FieldAsDateTime( const char *name );
   TDateTime FieldAsDateTime( const std::string &name );

   int GetSizeLongField( int FieldId );
   int GetSizeLongField( const char *name );
   int GetSizeLongField( const std::string &name );

   int FieldAsLong( int FieldId, void *Value );
   int FieldAsLong( const char *name, void *Value );
   int FieldAsLong( const std::string &name, void *Value );

   int GetSizeField( int FieldId );
   int GetSizeField( const char *name );
   int GetSizeField( const std::string &name );

   int VariablesCount( void );
   const char *VariableName( int VariableId );
   int VariableIndex( const char* name );
   int VariableIndex( const std::string &name );
   int GetVariableIndex( const std::string &name );
   int VariableIsNULL( int VariableId );
   int VariableIsNULL( const char *name );
   int VariableIsNULL( const std::string &name );
   void DeclareVariable( const char *name, otFieldType atype );
   void DeclareVariable( const std::string &name, otFieldType atype );
   void DeleteVariable( const char *name );
   void DeleteVariable( const std::string &name );
   void SetVariable( const char *name, tnull Data );
   void SetVariable( const std::string &name, tnull Data );
   void SetVariable( const char *name, int Data );
   void SetVariable( const std::string &name, int Data );
   void SetVariable( const char *name, double Data );
   void SetVariable( const std::string &name, double Data );
   void SetVariable( const char *name, const char *Data );
   void SetVariable( const std::string &name, const char *Data );
   void SetVariable( const std::string &name, const std::string &Data );
   void SetVariable( const char *name, void *Data );
   void SetVariable( const std::string &name, void *Data );
   void SetLongVariable( const char *name, void *Data, int Length );
   void SetLongVariable( const std::string &name, void *Data, int Length );
   void CreateVariable( const char *name, otFieldType atype, tnull Data );
   void CreateVariable( const std::string &name, otFieldType atype, tnull Data );
   void CreateVariable( const char *name, otFieldType atype, int Data );
   void CreateVariable( const std::string &name, otFieldType atype, int Data );
   void CreateVariable( const char *name, otFieldType atype, double Data );
   void CreateVariable( const std::string &name, otFieldType atype, double Data );
   void CreateVariable( const char *name, otFieldType atype, const char *Data );
   void CreateVariable( const std::string &name, otFieldType atype, const char *Data );
   void CreateVariable( const std::string &name, otFieldType atype, const std::string &Data );
   void CreateVariable( const char *name, otFieldType atype, void *Data );
   void CreateVariable( const std::string &name, otFieldType atype, void *Data );
   void CreateLongVariable( const char *name, otFieldType atype, void *Data, int Length );
   void CreateLongVariable( const std::string &name, otFieldType atype, void *Data, int Length );
   int GetVariableAsInteger( int VariableId );
   int GetVariableAsInteger( const char *name );
   int GetVariableAsInteger( const std::string &name );
   double GetVariableAsFloat( const char *name );
   double GetVariableAsFloat( const std::string &name );
   double GetVariableAsFloat( int VariableId );
   char *GetVariableAsString( const char *name );
   char *GetVariableAsString( const std::string &name );
   char *GetVariableAsString( int VariableId );
   TDateTime GetVariableAsDateTime( const char *name );
   TDateTime GetVariableAsDateTime( const std::string &name );
   TDateTime GetVariableAsDateTime( int VariableId );
   int GetSizeLongVariable( const char *name );
   int GetSizeLongVariable( const std::string &name );
   int GetVariableAsLong( const char *name, void *Value );
   int GetVariableAsLong( const std::string &name, void *Value );
   void ClearVariables( );
   void Clear( );
};

class TSession {
 private:
   bool UseOCI8;
   void ServerAttach( );
   tconnect FConnect;
   int LastOCIError;
   void OCICall( int err );
   int ReturnCode( );
   char Version[500];
   std::string FUsername;
   std::string FPassword;
   std::string FDatabase;
   ub1 hda[ HDA_SIZE ];
   Lda_Def lda;
   Lda_Def *plda;
   bool lda_owner;
   OCIEnv *envhp;
   OCIError *errhp, *secerrhp;
   OCISvcCtx *svchp;
   OCISession *authp;
   OCIServer *srvhp;
   tconnect StatusConnect( );
   void CloseChildren( );
   int UsedCount;
   int Count;
   std::vector <TQuery*> QueryList;
   //int ErrorHandle( );
 public:
   TMemoryManager MM;
   TSession( TOCI AUseOCI8 = OCI8 );
   ~TSession( );
   void RaiseOracleError( );
   void LogOn( char *Connect_String, TOCI AUseOCI8 = OCI8 );
   void Initialize( Lda_Def *vlda );
   void LogOff( );
   bool isConnect();
   OCISvcCtx *Getsvchp();
   OCIEnv *Getenvhp();
   Lda_Def *Getlda();
   void Commit( );
   void Rollback( );
   TQuery *CreateQuery( );
   void DeleteQuery( TQuery &Query );
   void ClearQuerys( );
   bool isOCI8();
};

class EOracleError:public EXCEPTIONS::Exception
{
  private:
    std::string sqlText;
  public:
    int Code;
    EOracleError( const char *msg, long code, const char *sql ):EXCEPTIONS::Exception( msg )
    {
        Code = code;
        sqlText = sql;
    }
    EOracleError( const char *msg, long code ):EXCEPTIONS::Exception( msg )
    {
        Code = code;
        sqlText = "";
    }
    ~EOracleError() throw() {};
    const char* SQLText() const throw()
    {
      return sqlText.c_str();
    };
};

int ConvertORACLEDate_TO_DateTime( void *Value, TDateTime &VDateTime );
int ConvertORACLEDate_TO_Str( void *Data, char *Value );
int ConvertDateTime_TO_ORACLEDate( const TDateTime &VDateTime, void *Value );
void FindVariables( const std::string &SQL, bool IncludeDuplicates, std::vector<std::string> &vars );

extern TSession OraSession;

#endif
