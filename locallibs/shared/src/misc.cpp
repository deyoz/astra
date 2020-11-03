//---------------------------------------------------------------------------
#include <stdio.h>
#include <cstdlib>
#include <cmath>
#include <string.h>
#include <sstream>
#include "misc.h"
#include "memory_manager.h"
#include "exceptions.h"
#include <unistd.h>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE

using namespace std;
using namespace EXCEPTIONS;
//using namespace BASIC;

const char rus_big[ ] =   "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";
const char rus_small[ ] = "абвгдеёжзийклмнопрстуфхцчшщъыьэюя";

const char a_ascii[ ] = "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЫЬЭЮЯЁЪ";
const char a_koi7[ ] =  "abwgdevzijklmnoprstufhc~{}yx|`q";

void* Koi7ToAscii( void *buffer, unsigned int Len )
{
 buffer = ByteReplace( (char*)buffer, Len, a_koi7, a_ascii );
 return buffer;
};

char* AsciiToKoi7( char *buffer )
{
 CharReplace( buffer, "Ъ", "'" );
 CharReplace( buffer, a_ascii, a_koi7 );
 return buffer;
}

char AsciiToKoi7( char c)
{
 if (c=='Ъ') return '\'';
 ByteReplace(&c,1,a_ascii,a_koi7);
 return c;
};

char Koi7ToAscii( char c)
{
 ByteReplace(&c,1,a_koi7,a_ascii);
 return c;
};

char ToUpper(char c)
{
  if ((unsigned char)c<0x80)
  {
    if (c>='a'&&c<='z') c=c-'a'+'A';
  }
  else ByteReplace(&c,1,rus_small,rus_big);
  return c;
};

char ToLower(char c)
{
  if ((unsigned char)c<0x80)
  {
    if (c>='A'&&c<='Z') c=c-'A'+'a';
  }
  else ByteReplace(&c,1,rus_big,rus_small);
  return c;
};

bool IsAscii7(char c)
{
  return (unsigned char)c<0x80;
};

bool IsUpperLetter(char c)
{
  if ((unsigned char)c<0x80)
    return c>='A'&&c<='Z';
  else    	
    return strchr(rus_big,c)!=NULL;
};

bool IsLowerLetter(char c)
{
  if ((unsigned char)c<0x80)
    return c>='a'&&c<='z';
  else    	
    return strchr(rus_small,c)!=NULL;
};

bool IsLetter(char c)
{
  return IsUpperLetter(c)||IsLowerLetter(c);
};

bool IsDigit(char c)
{
  return c>='0'&&c<='9';	
};

bool IsDigitIsLetter(char c)
{
  return IsDigit(c)||IsLetter(c);	
};

char* BreakStr(char *s, int len)
{
  char *p=s,*ph,*pend;
  if (s==NULL) return NULL;
  pend=p+strlen(p);
  while (pend-p>len) //пока все не дорезали - резать
  {
    ph=strchr(p,'\n');
    if (ph!=NULL&&ph-p<=len);  //и резать не надо - перевод строки там где надо
    else
    {
      for(ph=p+len;ph>=p;ph--)  // отмерили len и ищем первый пробел с конца
        if ((unsigned char)(*ph)<=0x20) break;
      if (ph>=p) *ph='\n';
      else                      // не нашли
      {
        for(ph=p+len;ph<pend;ph++)  //ищем первый пробел после конца
          if ((unsigned char)(*ph)<=0x20) break;
        if (ph<pend) *ph='\n';
      };
    };
    p=ph+1;  //следующий кусок
  };
  return s;
};

#ifndef __WIN32__
TPerfTimer::TPerfTimer(std::string msg) {
  text = msg;
  Init();
}

void TPerfTimer::Init() {
  tm1 = times(&stm1);
}

long TPerfTimer::Print() {
  return ((times(&stm2) - tm1) * 1000) / sysconf(_SC_CLK_TCK);
}

string TPerfTimer::PrintWithMessage() {
    ostringstream buf;
    buf << "REQUEST EXECUTION TIME - " << (text.empty() ? text : text + " - ") << ((times(&stm2)-tm1)*1000)/sysconf(_SC_CLK_TCK) << " ms";
    return buf.str();
}
#endif

/*** From basic.cpp ***/

char *RTrim( char *value )
{
    if ( value == NULL ) return NULL;
    for ( unsigned char *p= (unsigned char*)value + strlen( value ) - 1;
        p>=(unsigned char*)value;
        p-- )
    {
      if ( *p <= 32 )
          *p = 0;
      else
          break;
    }
    return value;
}

char *LTrim( char *value )
{
    unsigned char *p;
    unsigned len = strlen( value );
    unsigned slen = len;

    if ( value == NULL || len == 0 )
        return NULL;

    for ( p = (unsigned char*)value; p < (unsigned char*)value + slen; ++p )
    {
        if ( *p <= 32 )
          len--;
        else
          break;
    }

    if (p != (unsigned char*)value )
    {
        memmove( value, (char*)p, len );
        value[ len ] = 0;
    }

    return value;
}

char *Trim( char *value )
{
    RTrim( value );
    LTrim( value );
    return value;
}

int StrToFloat( const char *buf, double &Value )
{
    int Res;
    char *charstop;
    char *strvalue;

    if ( buf == NULL ) return EOF;

    TMemoryManager mem(STDLOG);
    strvalue = (char*)mem.malloc( strlen( buf ) + 1, STDLOG );

    if ( strvalue == NULL ) EMemoryError( "Can't allocate memory" );

    try {
        strcpy( strvalue, buf );
        Trim( strvalue );
        errno=0;
        Value = strtod( strvalue, &charstop );

        if ( errno == ERANGE || *charstop || Value == fabs( HUGE_VAL ) )
        {
            if (strchr(strvalue,'.')!=NULL)
                CharReplace(strvalue,".",",");
            else
                CharReplace(strvalue,",",".");

            errno=0;
            Value = strtod( strvalue, &charstop );

            if ( errno == ERANGE || *charstop || Value == fabs( HUGE_VAL ) )
                Res = EOF;
            else
                Res = 0;
        }
        else
          Res = 0;
    }
    catch( ... ) {
        mem.free( strvalue, STDLOG );
        throw;
    }

    mem.free( strvalue, STDLOG );
    return Res;
}

int StrToInt( const char *buf, int &Value )
{
    int Res;
    char *charstop;
    char *strvalue;

    if ( buf == NULL ) return EOF;

    TMemoryManager mem(STDLOG);
    strvalue = (char*)mem.malloc( strlen( buf ) + 1, STDLOG );
    
    if ( strvalue == NULL ) EMemoryError( "Can't allocate memory" );
    
    try {
        strcpy( strvalue, buf );
        Trim( strvalue );
        errno=0;
        Value = strtol( strvalue, &charstop, 10 );

        if ( errno == ERANGE || *charstop )
            Res = EOF;
        else
            Res = 0;
    }
    catch( ... ) {
        mem.free( strvalue, STDLOG );
        throw;
    }

    mem.free( strvalue, STDLOG );
    return Res;
}

int StrToBool( const char *buf, bool &Value )
{
    int Res;
    char *strvalue;

    if ( buf == NULL ) return EOF;

    TMemoryManager mem(STDLOG);
    strvalue = (char*)mem.malloc( strlen( buf ) + 1, STDLOG );
    
    if ( strvalue == NULL ) EMemoryError( "Can't allocate memory" );
    try {
        strcpy( strvalue, buf );
        Trim( strvalue );
        if ( strcmp(strvalue, "true")==0 ||
            strcmp(strvalue, "True")==0 ||
            strcmp(strvalue, "TRUE")==0 ||
            strcmp(strvalue, "1")==0 )
        {
            Value=true;
            Res = 0;
        }
        else if
          ( strcmp(strvalue, "false")==0 ||
            strcmp(strvalue, "False")==0 ||
            strcmp(strvalue, "FALSE")==0 ||
            strcmp(strvalue, "0")==0 )
        {
            Value=false;
            Res = 0;
        }
        else
          Res = EOF;
    }
    catch( ... ) {
        mem.free( strvalue, STDLOG );
        throw;
    }

    mem.free( strvalue, STDLOG );
    return Res;
}

std::string &CharReplace( string &Value, const string &OldPattern,
                    const string &NewPattern )
{
    string::size_type idx;
    for ( string::iterator i = Value.begin(); i != Value.end(); ++i ) {
        idx = OldPattern.find( *i );
        if ( idx != string::npos && idx < NewPattern.size() ) {
            *i = NewPattern[ idx ];
        }
    }

    return Value;
}

char *CharReplace( char *Value, const char *OldPattern, const char *NewPattern )
{
    ByteReplace(Value, strlen(Value), OldPattern, NewPattern);
    return Value;
}

void *ByteReplace( char *Value, unsigned int Len,
                  const char *OldPattern, const char *NewPattern )
{
    if ( Value == NULL ) return NULL;
    
    unsigned int OldPattern_len =  strlen( OldPattern );
    unsigned int NewPattern_len = strlen( NewPattern );
    unsigned int index;
    
    for( unsigned int i = 0; i < Len; ++i )
    {
        const void *ptr;
        if ( ( ptr = memchr( OldPattern, ((char*)Value)[ i ], OldPattern_len ) ) != NULL )
        {
            index = (uintptr_t)ptr - (uintptr_t)OldPattern;
            if ( index < NewPattern_len  )
              Value[ i ] = NewPattern[ index ];
        }
    }
    return Value;
}