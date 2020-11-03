/*	2006 by Roman Kovalev 	*/
/*	roman@pike.dev.sirena2000.ru		*/
//#include <new>
#include <stdlib.h>
#include <cstdio>
#include "etick/et_utils.h"

#define NICKNAME "ROMAN"
#include <serverlib/test.h>

namespace EtUtils
{
std::string PrintApFormat(const char *format, va_list ap, const int StepSize)
{
    int buflen=StepSize;
    char *tmp = static_cast <char *> (malloc(StepSize));
    if(!tmp){
        TST();
        throw std::bad_alloc();
    }

    va_list ap_save;
    va_copy(ap_save, ap);
    while(1){
        int l=std::vsnprintf(tmp,buflen,format,ap);
        if( l<0 ){
            buflen=buflen*2;
        } else if( l>=buflen ){
            buflen=l+1;
        } else {
            break;
        }

        tmp=static_cast<char *> (realloc(tmp, buflen));
        if(!tmp){
            TST();
            free(tmp);
            throw std::bad_alloc();
        }
        va_copy(ap, ap_save);
    }

    std::string tmp_string (tmp);
    free(tmp);
    va_end(ap_save);
    return tmp_string;
}
}
