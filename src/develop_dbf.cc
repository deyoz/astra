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
#include "slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;


Develop_dbf::Develop_dbf( )
{
  /*
   FoxBASE+/dBASE III +, ��� memo - 0x03
   FoxBASE+/dBASE III +, � memo   - 0x83
   FoxPro/dBASE IV,      ��� memo - 0x03
   FoxPro                � memo   - 0xF5
   dBASE IV              c memo   - 0x8B
  */
  version = 0x03;
  rowCount = 0;
  headerLen = 32;
}

void Develop_dbf::setVersion( unsigned char v )
{
  if ( v != 0x03 && v != 0x83 && v != 0xF5 && v != 0x8B )
    throw Exception( "Invalid version number" );
  version = v;
}

void putbinary_tostream( ostringstream &s, int value, int vsize )
{
  for (int i=0; i<vsize; i++ ) {
//  	ProgTrace( TRACE5, "put value=%d", (int)*((const char*)&value + i) );
    s.write( ((const char*)&value + i), 1 ); //???
  }
}

void Develop_dbf::BuildHeader()
{
	header.clear();
	// �㫥��� ���� ��������� ᮤ�ন�:
	// 7 - ������祭�� 䠩�� DBT
	// 6-5 - 䫠� SQL (⮫쪮 ��� dBase IV)
	// 4 - ��१�ࢨ஢���
	// 3 - ���� DBT (� dBase IV)
	//2-0 - ����� ���ᨨ
	unsigned char b = version;
	header << b;
	// ᫥���騥 3 ���� ᮤ�ঠ� ��⭠������� ���� ��᫥����� ���������� � �ଠ� ������
	int Year, Month, Day;
	DecodeDate( NowUTC(), Year, Month, Day );
	Year = Year % 100;

	putbinary_tostream( header, Year, 1 );
	putbinary_tostream( header, Month, 1 );
	putbinary_tostream( header, Day, 1 );
	// ����� 4 ���� - ���-�� ��ப
	ProgTrace( TRACE5, "rowcount=%d", rowCount );
	putbinary_tostream( header, rowCount, 4 );
	// ����� 2 ���� - ᮢ��㯭� ࠧ��� ��������� � ���ਯ�஢ ����� - 㪠��⥫� �� �����
	putbinary_tostream( header, headerLen + descriptorFieldsLen, 2 );
	// ����� 2 ���� - ����� �����
	putbinary_tostream( header, recLen, 2 );
	int tmp_zero = 0;
	// ��� ��१�ࢨ஢����� ����
	putbinary_tostream( header, tmp_zero, 2 );
	// ���� ��।����騩 ����প� �� �࠭���樨 (⮫쪮 ��� dBase IV)
	putbinary_tostream( header, tmp_zero, 1 );
	// ����� � 15-27 - १��
	putbinary_tostream( header, tmp_zero, 4 );
	putbinary_tostream( header, tmp_zero, 4 );
	putbinary_tostream( header, tmp_zero, 4 );
	putbinary_tostream( header, tmp_zero, 1 );
	// ���� 28 - (⮫쪮 ��� dBase IV) - 㪠�뢠�� ������祭 �� ������⢥��� ������ MDX
	putbinary_tostream( header, tmp_zero, 1 );
	// � 29-31 - १��
	putbinary_tostream( header, tmp_zero, 3 );
	ProgTrace( TRACE5, "header=|%s|", header.str().c_str() );
};

void Develop_dbf::BuildFields()
{
	descrField.clear();
  int tmp_zero = 0;
  char c = 0;
  descrField << std::setfill(c);
  for ( vector<TField>::iterator i=fields.begin(); i!=fields.end(); i++ ) {
   descrField << std::left << std::setw(11) << i->name;
   descrField << i->type;
   // ����� 4 ���� ��१�ࢨ஢��� � ������ ���� ��������� ��ﬨ
   putbinary_tostream( descrField, tmp_zero, 4 );
   // ����� 16 ���� - ����뢠�� ����� ����: �� ����⮩ + ������ + ��᫥
   putbinary_tostream( descrField, i->len, 1 );
   // 17 ���� - ���-�� �������� ࠧ�冷�
   putbinary_tostream( descrField, i->precision, 1 );
   // ����� 13 ��१�ࢨ஢����� ����
   putbinary_tostream( descrField, tmp_zero, 4 );
   putbinary_tostream( descrField, tmp_zero, 4 );
   putbinary_tostream( descrField, tmp_zero, 4 );
   putbinary_tostream( descrField, tmp_zero, 1 );
   // ������祭�� ⥣� MDX � dBase4
   putbinary_tostream( descrField, tmp_zero, 1 );
  }
  c = 13;
  descrField << c;
  descriptorFieldsLen = (int)descrField.str().size();
  ProgTrace( TRACE5, "descripterFields=|%s|", descrField.str().c_str() );
};

void Develop_dbf::BuildData()
{
	recLen = 1;
	data.clear();
	for ( vector<DBFRow>::iterator i=rows.begin(); i!=rows.end(); i++ ) {
		if ( i->pr_del )
			data << '*';
		else
			data << ' ';
	  vector<TField>::iterator f=fields.begin();
		for ( vector<string>::iterator j=i->data.begin(); j!=i->data.end(); j++ ) {
      if ( f->type != 'N' && f->type != 'F' )
 		    data << std::left;
      else
        data << std::right;
      data << setw( f->len ) <<  *j;
		  f++;
		}
		if ( i == rows.begin() )
			recLen = (int)data.str().size();
	}
	char c = 26;
	data << c;
  ProgTrace( TRACE5, "Data=|%s|", data.str().c_str() );
};


void Develop_dbf::Build()
{
	tst();
	BuildData();
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
     case 'C': /* ᨬ���쭮� */
     case 'L': /* �����᪮� */
     case 'N': /* �᫮��� */
     case 'D': /* ��� */
     //case 'M': /* ⨯� memo */
     case 'F': /* � ������饩 �窮� */
     //case 'P': /* 蠡��� */
               field.type = type;
	       break;
     default:
	       throw Exception( "Invalid Field type" );
   }
  if ( len > 255 )
    throw Exception( "Invalid Field size>255" );
  field.len = len;
  if ( precision > len )
  	throw Exception( "Invalid Field precision" );
  field.precision = precision;
  fields.push_back( field );
}

void Develop_dbf::AddField( std::string name, char type, int len )
{
	AddField( name, type, len, 0 );
}

void Develop_dbf::AddRow( DBFRow &row )
{
	if ( row.data.size() != fields.size() ) {
		ProgTrace( TRACE5, "row.data.size()=%d, fields.size()=%d", (int)row.data.size(), (int)fields.size() );
		throw Exception( "Invalid format data " );
	}
	string::size_type t;
	vector<string>::iterator r=row.data.begin();
	for ( vector<TField>::iterator f=fields.begin(); f!=fields.end() && r!=row.data.end(); f++, r++ ) {
		if ( (int)r->size() > f->len ) {
			ProgTrace( TRACE5, "data size=%d, data value=%s, field size=%d, field name=%s",
			           (int)r->size(), r->c_str(), f->len, f->name.c_str() );
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
      	        	   ( (int)t <= f->precision ) && f->len - f->precision >= (int)( r->size() - t ) - 1 )
      	        	break;
      	        throw Exception( "Invalid format data" );
      case 'D': TDateTime v;
      	        if ( r->empty() || StrToDateTime( r->c_str(), "yyyymmdd", v ) != EOF )
      	        	break;
      	        ProgTrace( TRACE5, "date value=%s", r->c_str() );
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

