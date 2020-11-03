#include "parser.h"
#include <stdio.h>
#include "exceptions.h"
#include "misc.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

const unsigned char NEW_CONTEXT = 0x01;
const unsigned char BEG_FIELD = 0x02;
const unsigned char END_CONTEXT = 0x03;

TField::TField( unsigned int FieldId )
{
  buf = NULL;
  buf_size = 0;
  Line = 0;
  separator = 0;
  Index = FieldId;
};

TField::~TField( )
{
  if ( buf ) free( buf );
};

unsigned int TField::GetLine( void )
{
  return Line;
};

void TField::SetField( const char *Data, unsigned int Len )
{
  SetField( 0, 0, Data, Len );
};


void TField::SetField( unsigned int ALine, char separ,
                       const char *Data, unsigned int Len )
{
  Line = ALine;
  if ( Len )
   {
     if ( ( buf = (char*)realloc( buf, Len + 1 ) ) == NULL )
      throw EMemoryError( "Can not allocate memory" );
     buf_size = Len + 1;
     memcpy( buf, Data, Len );
     buf[ Len ] = 0;
     separator = separ;
   }
  else
   {
     free( buf );
     buf = NULL;
     buf_size = 0;
     separator = 0;
   };
};

bool TField::FieldIsNULL( )
{
  return !buf_size;
};

int TField::FieldAsInteger( )
{
  int Value;
  if ( StrToInt( buf, Value ) == EOF )
   {
     char serror[ 100 ];
     sprintf( serror, "Cannot convert field %d to an Integer", Index );
     throw EConvertError( serror );
   };
  return Value; 
};

double TField::FieldAsFloat( )
{
  double Value;
  if ( StrToFloat( buf, Value ) == EOF )
   {
     char serror[ 100 ];
     sprintf( serror, "Cannot convert field %d to an Float", Index );
     throw EConvertError( serror );
   };
  return Value;
};

const char *TField::FieldAsString( )
{
  if ( buf == NULL )
    return "";
  else
    return buf;
};

TDateTime TField::FieldAsDateTime( )
{
  TDateTime Value;
  if ( StrToDateTime( buf, Value ) == EOF )
   {
     char serror[ 100 ];
     sprintf( serror, "Cannot convert field %d to an DateTime", Index );
     throw EConvertError( serror );
   };
  return Value; 
};

///////////////////// TCOMMONTEXT //////////////////////////////////////////////
TCommonContext::TCommonContext( )
{
  name = NULL;
  Modified=false;
};

TCommonContext::~TCommonContext( )
{
  Clear( );
};

void TCommonContext::Clear( )
{
  std::vector<TField*>::iterator VField;
  for ( VField=FFields.begin( ); VField!=FFields.end( ); VField++ ) delete *VField;
  FFields.clear( );
  Modified=true;
};

TField *TCommonContext::CreateField( unsigned int ALine, char separ,
                                     char *Data, unsigned int Len )
{
  TField *Field;
  try {
    Field = new TField( FFields.size( ) );
    Field->SetField( ALine, separ, Data, Len );
    FFields.push_back( Field );
  }
  catch( std::bad_alloc )
  {
    throw EMemoryError( "Can not allocate memory" );
  };
  return Field;
};

void TCommonContext::NewField( const char *field, unsigned int len )
{
  TField *Field;
  try {
    Field = new TField( FFields.size( ) );
    Field->SetField( field, len );
    FFields.push_back( Field );
  }
  catch( std::bad_alloc )
  {
    EMemoryError( "Can not allocate memory" );
  };
  Modified=true;
};

void TCommonContext::NewField( const char *field )
{
  NewField( field, strlen( field ) );
};

void TCommonContext::SetField( unsigned int FieldId, char *Value )
{
  if ( FieldId >= FieldsCount( ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Field %d does not exists", FieldId );
     throw EParserError( serror );
   };
  TField &Field = *FFields[ FieldId ];
  Field.SetField( Value, strlen( Value ) );
  Modified=true;
};

TField *TCommonContext::GetField( unsigned int FieldId )
{
  if ( FieldId >= FieldsCount( ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Field %d does not exists", FieldId );
     throw EParserError( serror );
   }
  return FFields[ FieldId ];
};

void TCommonContext::DeleteField( unsigned int FieldId )
{
  TField *Field = GetField( FieldId );
  if ( Field == NULL ) return;
  //FFields.erase( &FFields[ FieldId ] );  //???!!!!!!!!!
  delete Field;
  Modified=true;
};

unsigned int TCommonContext::FieldsCount( void )
{
  return FFields.size( );
};

unsigned int TCommonContext::FieldsCountInLine( unsigned int ALine )
{
  unsigned int Result = 0;
  for ( std::vector<TField*>::iterator VField = FFields.begin( );
        VField!=FFields.end( ); VField++ )
   {
     if ( (*VField)->GetLine( ) == ALine ) Result++;
   };
  return Result;
};

void TCommonContext::CreateName( char *vname, unsigned int len )
{
  name = (char*)malloc( len + 1 );
  if ( name == NULL )
    throw EMemoryError( "Can not allocate memory" );
  memcpy( name, vname, len );
  name[ len ] = 0;
};

void TCommonContext::CreateName( char *vname )
{
  CreateName( vname, strlen( vname ) );
};

char *TCommonContext::GetName( )
{
  return name;
};

void TCommonContext::Assign( TCommonContext &CommonContext )
{
  Clear( );
  NewField( (char *)MsgParser.GetName( ) );
  for ( unsigned int i=0; i<CommonContext.FieldsCount( ); i++ )
   {
     NewField( CommonContext.FieldAsString( i ) );
   };
};

bool TCommonContext::FieldIsNULL( unsigned int FieldId )
{
  if ( FieldId >= FieldsCount( ) )
   {
     char serror[ 100 ];
     sprintf( serror, "Field %d does not exists", FieldId );
     throw EParserError( serror );
   }
  else return (*FFields[ FieldId ]).FieldIsNULL( );
};

int TCommonContext::FieldAsInteger( unsigned int FieldId )
{
  if ( FieldIsNULL( FieldId ) ) return 0;
  else
   {
     TField &Field = *FFields[ FieldId ];
     return Field.FieldAsInteger( );
   };
};

double TCommonContext::FieldAsFloat( unsigned int FieldId )
{
  if ( FieldIsNULL( FieldId ) ) return 0.0;
  else
   {
     TField &Field = *FFields[ FieldId ];
     return Field.FieldAsFloat( );
   };
};

const char *TCommonContext::FieldAsString( unsigned int FieldId )
{
  if ( FieldIsNULL( FieldId ) ) return "";
  else
   {
     TField &Field = *FFields[ FieldId ];
     return Field.FieldAsString( );
   };
};

TDateTime TCommonContext::FieldAsDateTime( unsigned int FieldId )
{
  if ( FieldIsNULL( FieldId ) ) return 0.0;
  else
   {
     TField &Field = *FFields[ FieldId ];
     return Field.FieldAsDateTime( );
   };
};

//////////////////// TMSGPARSER ////////////////////////////////////////////////
// если длина строки больше того, что может уместиться на одной строке
// терминала, все равно это одна строка
void TMsgParser::Parse( char *Data, unsigned int Len, char *vname )
{
  unsigned int Ind, Line = 1, Pos = 0;
  char separ = 0;
  bool ismask;
  Clear( );
  CreateName( vname );
  if ( Len == 0 ) return;
  ismask = ( strcmp( GetName( ), "\033?" ) == 0 );
  for( Ind = 0; Ind < Len; Ind++ )
   {
     switch( Data[ Ind ] ) {
       case ',' :
       case '/' : if( !ismask )
                   {
                     if ( Ind )
                      CreateField( Line, separ, Data + Pos, Ind - Pos );
                     Pos = Ind + 1;
                     separ = Data[ Ind ];
                   };
                  break;
       case '\n': if( !ismask )
                   {
                     if ( Ind )
                      CreateField( Line, separ, Data + Pos, Ind - Pos );
                    Line++;
                    Pos = Ind + 1;
                    separ = Data[ Ind ];
                   };
                  break;  
       case 0x1b: if ( Ind < Len - 1 )
                   switch( Data[ Ind + 1 ] ) {
                     case '=': // конец сообщения
                     case '?': // начало параметра
                       if ( Ind ) // был пред. параметр
                         CreateField( Line, separ, Data + Pos, Ind - Pos );
                       separ = Data[ Ind + 1 ];
                   };
                  Ind++;
                  Pos = Ind + 1;
     };
   };
  if ( Pos < Ind || Data[ Ind - 1 ] == '/' )
     CreateField( Line, separ, Data + Pos, Ind - Pos );
};

char *TMsgParser::getcmd( )
{
  return GetName( );
};

TMsgParser MsgParser;



