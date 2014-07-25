#include "develop_dbf.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>

#include "exceptions.h"
#include "basic.h"
#include "stl_utils.h"

#define NICKNAME "DJEK"
#define NICKTRACE DJEK_TRACE
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;


Develop_dbf::Develop_dbf( )
{
  /*
   FoxBASE+/dBASE III +, без memo - 0x03
   FoxBASE+/dBASE III +, с memo   - 0x83
   FoxPro/dBASE IV,      без memo - 0x03
   FoxPro                с memo   - 0xF5
   dBASE IV              c memo   - 0x8B
  */
  version = 0x03;
  rowCount = 0;
  headerLen = 32;
  endDBF = char(26);
}

void Develop_dbf::setVersion( unsigned char v )
{
  if ( v != 0x03 && v != 0x83 && v != 0xF5 && v != 0x8B )
    throw Exception( "Invalid version number, value=%d", (int)v );
  version = v;
}

void putbinary_tostream( ostringstream &s, int value, int vsize )
{
  for (int i=0; i<vsize; i++ ) {
    s.write( ((const char*)&value + i), 1 ); //???
  }
}

void getbinary_fromstream( string &indata, int &value, int vsize )
{
  value = 0;
  for (int i=0; i<vsize; i++ ) {
    if ( indata.empty() )
      throw Exception( "getbinary_fromstream: Invalid format data" );
    ProgTrace( TRACE5, "vsize=%d, i=%d, indata=%d", vsize, i, *indata.substr(0,1).c_str() );
    *((char*)&value + i) = *indata.substr(0,1).c_str();
    ProgTrace( TRACE5, "vsize=%d, i=%d, value=%d", vsize, i, value );
    indata.erase( 0, 1 );
  }
}

void getbinary_fromstream( string &indata, long unsigned int &value, int vsize )
{
  int v;
  getbinary_fromstream( indata, v, vsize );
  value = (unsigned long int)v;
}


void Develop_dbf::BuildHeader()
{
	header.clear();
  header.str("");
	ProgTrace( TRACE5, "header=|%s|", header.str().c_str() );
	// нулевой байт заголовка содержит:
	// 7 - подключение файла DBT
	// 6-5 - флаг SQL (только для dBase IV)
	// 4 - зарезервировано
	// 3 - Файл DBT (в dBase IV)
	//2-0 - номер версии
	unsigned char b = version;
	header << b;
	// следующие 3 байта содержат шестнадцатиричную дату последнего обновления в формате ГГММДД
	int Year, Month, Day;
	DecodeDate( NowUTC(), Year, Month, Day );
	Year = Year % 100;

	putbinary_tostream( header, Year, 1 );
	putbinary_tostream( header, Month, 1 );
	putbinary_tostream( header, Day, 1 );
	ProgTrace( TRACE5, "rowCount=%d, headerLen=%d, descriptorFieldsLen=%d, recLen=%d",
             (int)rowCount, headerLen, descriptorFieldsLen, recLen );
	// далее 4 байта - кол-во строк
	putbinary_tostream( header, rowCount, 4 );
	// далее 2 байта - совокупный размер заголовка и дескрипторов полей - указатель на данные
	putbinary_tostream( header, headerLen + descriptorFieldsLen, 2 );
	// далее 2 байта - длина записи
	putbinary_tostream( header, recLen, 2 );
	int tmp_zero = 0;
	// два зарезервированных байта
	putbinary_tostream( header, tmp_zero, 2 );
	// байт определяющий задержку по транзакции (только для dBase IV)
	putbinary_tostream( header, tmp_zero, 1 );
	// байты с 15-27 - резерв
	putbinary_tostream( header, tmp_zero, 4 );
	putbinary_tostream( header, tmp_zero, 4 );
	putbinary_tostream( header, tmp_zero, 4 );
	putbinary_tostream( header, tmp_zero, 1 );
	// Байт 28 - (только для dBase IV) - указывает подключен ли множественный индекс MDX
	putbinary_tostream( header, tmp_zero, 1 );
	// с 29-31 - резерв
	putbinary_tostream( header, tmp_zero, 3 );
	string hs;
	StringToHex( header.str(), hs );
	ProgTrace( TRACE5, "header=|%s|", hs.c_str() );
};

void Develop_dbf::ParseHeader( std::string &indata )
{
	header.clear();
	header.str("");
	if ( indata.size() < (unsigned int)headerLen )
	  throw Exception( "Invalid header size()=%zu", indata.size() );
  header << indata.substr( 0, headerLen );
  setVersion( *indata.substr( 0, 1 ).c_str( ) );
  ProgTrace( TRACE5, "indata.size=%zu", indata.size() );
  indata.erase( 0, 1 );
  ProgTrace( TRACE5, "indata.size=%zu", indata.size() );
	// следующие 3 байта содержат шестнадцатиричную дату последнего обновления в формате ГГММДД
	int VYear, Year, Month, Day;
	DecodeDate( NowUTC(), VYear, Month, Day );
  ProgTrace( TRACE5, "indata.size=%zu", indata.size() );
	getbinary_fromstream( indata, Year, 1 );
	ProgTrace( TRACE5, "indata.size=%zu", indata.size() );
	getbinary_fromstream( indata, Month, 1 );
	ProgTrace( TRACE5, "indata.size=%zu", indata.size() );
	getbinary_fromstream( indata, Day, 1 );
	ProgTrace( TRACE5, "indata.size=%zu", indata.size() );
	VYear -= VYear % 100;
	Year += VYear;
	ProgTrace( TRACE5, "VYear=%d, Year=%d, Month=%d, Day=%d", VYear, Year, Month, Day );
	getbinary_fromstream( indata, rowCount, 4 );
	ProgTrace( TRACE5, "rowcount=%d", (int)rowCount );
	// далее 2 байта - совокупный размер заголовка и дескрипторов полей - указатель на данные
	getbinary_fromstream( indata, descriptorFieldsLen, 2 );
	descriptorFieldsLen -= headerLen;
	// далее 2 байта - длина записи
	getbinary_fromstream( indata, recLen, 2 );
	ProgTrace( TRACE5, "rowCount=%d, headerLen=%d, descriptorFieldsLen=%d, recLen=%d",
             (int)rowCount, headerLen, descriptorFieldsLen, recLen );
  int tmp_zero;
	getbinary_fromstream( indata, tmp_zero, 2 );
	// байт определяющий задержку по транзакции (только для dBase IV)
	getbinary_fromstream( indata, tmp_zero, 1 );
	// байты с 15-27 - резерв
	getbinary_fromstream( indata, tmp_zero, 4 );
	getbinary_fromstream( indata, tmp_zero, 4 );
	getbinary_fromstream( indata, tmp_zero, 4 );
	getbinary_fromstream( indata, tmp_zero, 1 );
	// Байт 28 - (только для dBase IV) - указывает подключен ли множественный индекс MDX
	getbinary_fromstream( indata, tmp_zero, 1 );
	// с 29-31 - резерв
	getbinary_fromstream( indata, tmp_zero, 3 );

	ProgTrace( TRACE5, "header=|%s|", header.str().c_str() );
}

void Develop_dbf::BuildFields()
{
	descrField.clear();
  descrField.str("");
  int tmp_zero = 0;
  char c = 0;
  descrField << std::setfill(c);
  for ( vector<TField>::iterator i=fields.begin(); i!=fields.end(); i++ ) {
   descrField << std::left << std::setw(11) << i->name;
   descrField << i->type;
   // далее 4 байта зарезервированы и должны быть заполнены нулями
   putbinary_tostream( descrField, tmp_zero, 4 );
   // далее 16 байт - описывает длину поля: до запятой + запятая + после
   putbinary_tostream( descrField, i->len, 1 );
   // 17 байт - кол-во десятичных разрядов
   putbinary_tostream( descrField, i->precision, 1 );
   // далее 13 зарезервированных байт
   putbinary_tostream( descrField, tmp_zero, 4 );
   putbinary_tostream( descrField, tmp_zero, 4 );
   putbinary_tostream( descrField, tmp_zero, 4 );
   putbinary_tostream( descrField, tmp_zero, 1 );
   // подключение тега MDX в dBase4
   putbinary_tostream( descrField, tmp_zero, 1 );
  }
  c = 13;
  descrField << c;
  descriptorFieldsLen = (int)descrField.str().size();
  ProgTrace( TRACE5, "descripterFields.size()=%zu", descrField.str().size() );
};

void Develop_dbf::ParseFields( std::string &indata )
{
  descrField.clear();
  descrField.str("");
  fields.clear();
  if ( indata.size() < (unsigned int)descriptorFieldsLen )
    throw Exception( "Invalid descrField size()=%zu", indata.size() );
  if ( descriptorFieldsLen%32 - 1 != 0 )
    throw Exception( "Invalid descrField len=%d", descriptorFieldsLen );
  descrField << indata.substr( 0, descriptorFieldsLen );
  TField field;
  for ( int i=0; i<descriptorFieldsLen/32; i++ ) {
    ProgTrace( TRACE5, "field.name=%s",indata.c_str() );
    field.name = indata.substr( 0, 11 ).c_str();
    field.idx = fields.size();
    indata.erase( 0, 11 );
    field.type = *indata.substr( 0, 1 ).c_str();
    indata.erase( 0, 1 );
    indata.erase( 0, 4 );
    getbinary_fromstream( indata, field.len, 1 );
    getbinary_fromstream( indata, field.precision, 1 );
    indata.erase( 0, 4 );
    indata.erase( 0, 4 );
    indata.erase( 0, 4 );
    indata.erase( 0, 1 );
    indata.erase( 0, 1 );
    AddField( field.name, field.type, field.len, field.precision );
  }
  if ( !indata.empty() )
    indata.erase( 0, 1 ); //13
  ProgTrace( TRACE5, "fields.size()=%zu", fields.size() );
}

void Develop_dbf::BuildData( const std::string &encoding )
{
  ProgTrace( TRACE5, "encoding=%s", encoding.c_str() );
	recLen = endDBF.size();
	data.clear();
	data.str("");
	for ( vector<DBFRow>::iterator i=rows.begin(); i!=rows.end(); i++ ) {
		if ( i->pr_del )
			data << '*';
		else
			data << ' ';
    ProgTrace( TRACE5, "data.size()=%zu, rows.size()=%zu, fields.size()=%zu, rowdata.size()=%zu",
               data.str().size(), rows.size(), fields.size(), i->newdata.size() );
	  vector<TField>::iterator f=fields.begin();
		for ( vector<string>::iterator j=i->newdata.begin(); j!=i->newdata.end(); j++ ) {
      if ( f->type != 'N' && f->type != 'F' )
 		    data << std::left;
      else
        data << std::right;
      if ( encoding.empty() || encoding == "CP866" )
        data << setw( f->len ) <<  j->substr( 0, f->len );
      else {
        data << setw( f->len ) << string(ConvertCodepage( *j, "CP866", encoding )).substr( 0, f->len );
      }
      ProgTrace( TRACE5, "Develop_dbf::BuildData: f->name=%s, f->len=%d", f->name.c_str(), f->len );
		  f++;
		}
		if ( i == rows.begin() )
			recLen = (int)data.str().size();
	}
	data << endDBF;
  ProgTrace( TRACE5, "endDBF.size()=%zu, data.size()=%zu, Data=|%s|", endDBF.size(), data.str().size(), data.str().c_str() );
};

void Develop_dbf::ParseData( std::string &indata, const std::string &encoding )
{
  data.clear();
  data.str("");
  rows.clear();
  int len = 0;
  for ( std::vector<TField>::iterator i=fields.begin(); i!=fields.end(); i++ ) {
    ProgTrace( TRACE5, "field.name=%s, field.type=%c, field.len=%d",
               i->name.c_str(), i->type, i->len );
    len += i->len;
  }
  if ( indata.size() < len*rowCount )
    throw Exception( "Invalid RowsData size()=%zu", indata.size() );
  data << indata;
  string del_str = "*";
  string value;
  int rCount = rowCount;
  rowCount = 0;
  for ( int r=0; r<rCount; r++ ) {
    DBFRow row;
    if ( indata.size() < 1 )
      throw Exception( "Invalid RowsData size()=%zu", indata.size() );
    row.pr_del = ( indata.substr( 0, 1 ) == del_str );
    indata.erase( 0, 1 );
    for ( std::vector<TField>::iterator i=fields.begin(); i!=fields.end(); i++ ) {
      if ( indata.size() < (unsigned int)i->len )
        throw Exception( "Invalid RowsData size()=%zu, i->len=%d, i->name=%s", indata.size(), i->len, i->name.c_str() );
      value = indata.substr( 0, i->len );
      value = TrimString( value );
      value = ConvertCodepage( value, encoding, "CP866" );
      row.olddata.push_back( value );
      indata.erase( 0, i->len );
    }
    row.newdata = row.olddata;
    AddRow( row );
  }
  //!!!endDBF = indata; // для совместимости включить строку кода
  ProgTrace( TRACE5, "endDBF=|%s|, endDBF.size=%zu", endDBF.c_str(), endDBF.size() );
}

void Develop_dbf::Build( const std::string &encoding )
{
	tst();
	BuildData( encoding );
	tst();
	BuildFields();
	BuildHeader();
}

string Develop_dbf::Result()
{
	string res;
	res = header.str() + descrField.str() + data.str();
	return res;
}


void Develop_dbf::AddField( std::string name, char type, int len, int precision )
{
	TField field;
  if ( (int)fields.size() >= 255 )
    throw Exception( "Invalid Fields count>255" );
  if ( name.size() > 11 )
    throw Exception( "Invalid Field name size>11" );
	field.name = upperc( name );
	string strtype = &type;
	strtype = upperc( strtype );
	type = *strtype.c_str();
   switch ( type ) {
     case 'C': /* символьное */
     case 'L': /* логическое */
     case 'N': /* числовое */
     case 'D': /* дата */
     //case 'M': /* типа memo */
     case 'F': /* с плавающей точкой */
     //case 'P': /* шаблон */
               field.type = type;
	       break;
     default:
	       throw Exception( "Invalid Field type, field name= " + name );
   }
  if ( len > 255 )
    throw Exception( "Invalid Field size>255" );
  field.len = len;
  if ( precision > len )
  	throw Exception( "Invalid Field precision" );
  field.precision = precision;
  field.idx = fields.size();
  fields.push_back( field );
}

void Develop_dbf::AddField( std::string name, char type, int len )
{
	AddField( name, type, len, 0 );
}

void Develop_dbf::GetRow( int idx, DBFRow &row )
{
  row.newdata.clear();
  if ( idx < 0 || (unsigned int)idx >= rowCount )
    throw Exception( "Invalid index in GetRow" );
  row.pr_del = rows[ idx ].pr_del;
  row.newdata = rows[ idx ].newdata;
  row.modify = rows[ idx ].modify;
}

void Develop_dbf::AddRow( DBFRow &row )
{
	if ( row.newdata.size() != fields.size() ) {
		ProgTrace( TRACE5, "row.data.size()=%zu, fields.size()=%zu", row.newdata.size(), fields.size() );
		throw Exception( "Invalid format data " );
	}
	string::size_type t;
	vector<string>::iterator r=row.newdata.begin();
	for ( vector<TField>::iterator f=fields.begin(); f!=fields.end() && r!=row.newdata.end(); f++, r++ ) {
		if ( (int)r->size() > f->len ) {
			ProgTrace( TRACE5, "data size=%zu, data value=%s, field size=%d, field name=%s",
			           r->size(), r->c_str(), f->len, f->name.c_str() );
			throw Exception( "Invalid format data (data size > field size)" );
	  }
		switch ( f->type  ) {
			case 'C': break;
			case 'L': if ( r->empty() ||
				             *r == "Y" || *r == "y" ||
	                   *r == "T" || *r == "t" ||
	                   *r == "N" || *r == "n" ||
	                   *r == "F" || *r == "f" )
	                break;
	              throw Exception( "Invalid format data" );
      case 'N': t = r->find( "." );
      	        if ( t == string::npos || f->precision == f->len ||
      	        	   (( (int)t <= f->precision ) && f->len - f->precision >= (int)( r->size() - t ) - 1) )
      	        	break;
      	        throw Exception( "Invalid format data" );
      case 'D': TDateTime v;
      	        if ( r->empty() || StrToDateTime( r->c_str(), "yyyymmdd", v ) != EOF )
      	        	break;
      	        ProgTrace( TRACE5, "date value=|%s|", r->c_str() );
      	        throw Exception( "Invalid format data as DateTime " );
      case 'F': double d;
      	        if ( r->empty() || StrToFloat( r->c_str(), d ) != EOF )
      	        	break;
      	        ProgTrace( TRACE5, "float value=%s", r->c_str() );
      	        throw Exception( "Invalid format data" );
		}
	}
	tst();
	rows.push_back( row );
	rowCount++;
	tst();
}

void Develop_dbf::DeleteRow( int idx, bool pr_del )
{
  DBFRow row;
  GetRow( idx, row );
  if ( pr_del ) {
    rows[ idx ].pr_del = pr_del;
    rows[ idx ].modify = true;
  }
  else {
    rowCount--;
    rows.erase( rows.begin() + idx );
  }
}

void Develop_dbf::NewRow( )
{
  DBFRow row;
  row.pr_del = 0;
  row.modify = true;
  for ( vector<TField>::iterator r=fields.begin(); r!=fields.end(); r++ ) {
    row.newdata.push_back( "" );
  }
  AddRow( row );
}

void Develop_dbf::Parse( const std::string &indata, const std::string &encoding )
{
  string vdata = indata;
  ParseHeader( vdata );
  ProgTrace( TRACE5, "indata.size=%zu", vdata.size() );
  ParseFields( vdata );
  ProgTrace( TRACE5, "indata.size=%zu", vdata.size() );
  ParseData( vdata, encoding );
  ProgTrace( TRACE5, "indata.size=%zu", vdata.size() );
}

int Develop_dbf::GetFieldIndex( const string &FieldName )
{
  for ( vector<TField>::iterator i=fields.begin(); i!=fields.end(); i++ ) {
    if ( i->name == FieldName ) {
      return i->idx;
    }
  }
  return -1;
}

std::string Develop_dbf::GetFieldValue( int idx, const string &FieldName )
{
  DBFRow row;
  GetRow( idx, row );
  int fieldIdx = GetFieldIndex( FieldName );
  if ( fieldIdx == -1 )
     throw Exception( "Field name %s not found", FieldName.c_str() );
  if ( fieldIdx >= (int)row.newdata.size() )
    throw Exception( "GetFieldValue: invalid format data, fieldName=%s", FieldName.c_str() );
  return row.newdata[ fieldIdx ];
}

void Develop_dbf::SetFieldValue( int idx, const std::string &FieldName, const string &Value )
{
  DBFRow row;
  GetRow( idx, row );
  int fieldIdx = GetFieldIndex( FieldName );
  if ( fieldIdx == -1 )
     throw Exception( "Field name %s not found", FieldName.c_str() );
  if ( fieldIdx >= (int)row.newdata.size() )
    throw Exception( "GetFieldValue: invalid format data, fieldName=%s", FieldName.c_str() );
  string oldvalue = TrimString( rows[ idx ].newdata[ fieldIdx ] );
  string newvalue = Value;
  newvalue = TrimString(  newvalue );
  newvalue = newvalue.substr( 0, fields[ fieldIdx ].len );
  if ( oldvalue != newvalue ) {
    ProgTrace( TRACE5, "SetFieldValue: FieldName=%s, oldvalue=%s, newvalue=%s",
               FieldName.c_str(), rows[ idx ].newdata[ fieldIdx ].c_str(), Value.c_str() );
    rows[ idx ].newdata[ fieldIdx ] = newvalue;
    rows[ idx ].modify = true;
  }
}

bool Develop_dbf::isModifyRow( int idx )
{
  DBFRow row;
  GetRow( idx, row );
  return row.modify;
}

void Develop_dbf::RollBackRow( int idx )
{
  DBFRow row;
  GetRow( idx, row );
  if ( rows[ idx ].olddata.empty() )
    DeleteRow( idx, false );
  else {
    if ( rows[ idx ].olddata.size() != rows[ idx ].newdata.size() )
      throw Exception( "RollBackRow: invalid fields count, old count=%zu, new count=%zu", rows[ idx ].olddata.size(), rows[ idx ].newdata.size() );
    rows[ idx ].newdata = rows[ idx ].olddata;
    rows[ idx ].pr_del = false;
    rows[ idx ].modify = false;
  }
}
