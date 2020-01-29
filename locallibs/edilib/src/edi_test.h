#ifndef _EDILIB_TEST_H
#define _EDILIB_TEST_H
#ifndef TEST
    #define TEST 1
#endif
#ifndef NICKNAME

#if TEST == 1
    #define tst() printf("%d %s\n",__LINE__,__FILE__)
#else
    #error no test
#endif
#endif

#include "edilib/edi_logger.h"

#define EDILOG NICKNAME, "edilib/" __FILE__, __LINE__
#define TRACE0  0,EDILOG
#define TRACE1  1,EDILOG
#define TRACE2  3,EDILOG
#ifdef EDILIB_DEBUG
#define TRACE3  7,EDILOG
#define TRACE4  9,EDILOG
#define TRACE5  12,EDILOG
#define TRACE6  15,EDILOG
#else
#define TRACE3  90,EDILOG
#define TRACE4  91,EDILOG
#define TRACE5  92,EDILOG
#define TRACE6  93,EDILOG
#endif /*EDILIB_DEBUG*/

#define CHECK_LOG_LEVEL 3


#define NVL(x,y) ((x)==NULL?(y):(x))
#define null(x) NVL(x,"(null)")

#ifdef EDILIB_DEBUG
#define tst() EdiTrace(TRACE3," ")
#else
#define tst() ;
#endif /*EDILIB_DEBUG*/
#define TST() EdiError(EDILOG," ")

#define  MSec(a) (((a)*1000)/sysconf(_SC_CLK_TCK))

#ifndef ISDIGIT
#define ISDIGIT(x) isdigit((unsigned char)(x))
#endif
#ifndef ISALPHA
#define ISALPHA(x) isalpha((unsigned char)(x))
#endif
#ifndef ISSPACE
#define ISSPACE(x) isspace((unsigned char)(x))
#endif

#endif /*_EDILIB_TEST_H*/
