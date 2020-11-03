#ifndef _EDI_ERR_MSG_H_
#define _EDI_ERR_MSG_H_

#include <stdarg.h>

const char * get_edi_msg_by_num(unsigned code,int lang );
int edi_set_msg_lang(int l);
int edi_get_msg_lang(void);

int edi_get_last_err_num(void);


void edi_set_last_err_num(int e);

#endif /* _EDI_ERR_MSG_H_ */

