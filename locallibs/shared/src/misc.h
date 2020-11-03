//---------------------------------------------------------------------------

#ifndef miscH
#define miscH

#include <string>

#ifndef __WIN32__
#include <sys/times.h>
#endif

void* Koi7ToAscii( void *buffer, unsigned int Len );
char* AsciiToKoi7( char *buffer );
char AsciiToKoi7( char c);
char Koi7ToAscii( char c);
char ToUpper(char c);
char ToLower(char c);
bool IsAscii7(char c);
bool IsUpperLetter(char c);
bool IsLowerLetter(char c);
bool IsLetter(char c);
bool IsDigit(char c);
bool IsDigitIsLetter(char c);
char* BreakStr(char *s, int len);

#ifndef __WIN32__
class TPerfTimer {
  private:
    std::string text;
    struct tms stm1, stm2;
    clock_t tm1;
  public:
    // Параметры: msg - строка добавляемая при записи в лог
    TPerfTimer(std::string msg = "");
    // Сброс таймера
    void Init();
    // Возвращает текущее значение таймера в миллисекундах
    long Print();
    // Возвращает строку вида "REQUEST EXECUTION TIME - msg - 10 ms"
    std::string PrintWithMessage();
};
#endif

char *LTrim( char *value );
char *RTrim( char *value );
char *Trim( char *value );
int StrToFloat( const char *buf, double &Value );
int StrToInt( const char *buf, int &Value );
int StrToBool( const char *buf, bool &Value );
char *CharReplace( char *Value, const char *OldPattern, const char *NewPattern );
void *ByteReplace( char *Value, unsigned int Len,
                   const char *OldPattern, const char *NewPattern );
std::string &CharReplace( std::string &Value, const std::string &OldPattern,
                          const std::string &NewPattern );

#endif
