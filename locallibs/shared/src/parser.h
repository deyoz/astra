#ifndef parserH
#define parserH

#include <vector>
#include "date_time.h"

using BASIC::date_time::TDateTime;

class TField
{
  private:
    char *buf; // указатель на данные
    unsigned int buf_size; // размер данных
    unsigned int Line; // координаты места значения переменной
    char separator;
    unsigned int Index;
  public:
    TField( unsigned int FieldId );
    ~TField( );
    void SetField( unsigned int ALine, char separ,
                   const char *Data, unsigned int Len );
    void SetField( const char *Data, unsigned int Len );
    unsigned int GetLine( void );
    char GetSeparator( );
    bool FieldIsNULL( );
    int FieldAsInteger( );
    double FieldAsFloat( );
    const char *FieldAsString( );
    TDateTime FieldAsDateTime( );
};

class TCommonContext
{
  private:
    std::vector<TField*> FFields;
    char *name;
  protected:
    void CreateName( char *vname, unsigned int len );
    void CreateName( char *vname );
    TField *CreateField( unsigned int ALine, char separ,
                         char *Data, unsigned int Len );
  public:
    bool Modified;
    TCommonContext( );
    ~TCommonContext( );
    void Clear( );
    char *GetName( );
    void NewField( const char *field, unsigned int len );
    void NewField( const char *field );
    void SetField( unsigned int FieldId, char *Value );
    TField *GetField( unsigned int FieldId );
    void DeleteField( unsigned int FieldId );
    void Assign( TCommonContext &Context );
    unsigned int FieldsCount( void );
    unsigned int FieldsCountInLine( unsigned int ALine );
    bool FieldIsNULL( unsigned int FieldId );
    int FieldAsInteger( unsigned int FieldId );
    double FieldAsFloat( unsigned int FieldId );
    const char *FieldAsString( unsigned int FieldId );
    TDateTime FieldAsDateTime( unsigned int FieldId );
};

class TMsgParser: public TCommonContext
{
  public:
    void Parse( char *Data, unsigned int Len, char *vcmd );
    char *getcmd( );
};

extern TMsgParser MsgParser;

#endif


