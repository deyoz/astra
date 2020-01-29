#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "edilib/edi_err.h"
#define NICKNAME "ROMAN"
#include "edi_test.h"

typedef struct msgtab{
    unsigned id;
    const char * eng,*rus;
}msgtab;

static unsigned int last_err_num;

static msgtab tab[]={
#include "edimsgout.dat"
};

const char * get_edi_msg_by_num(unsigned c, int lang)
{

    EdiTrace(8,EDILOG,"get_msg_by_num of %d lang %d ",c,lang);
    last_err_num=c;
    return lang ? tab[c].rus : tab[c].eng;

}

/* Описано в err_msg.h */
int get_edi_last_err_num(void)
{
    return last_err_num;
}

void set_edi_last_err_num(int e)
{
    last_err_num=e;
}
