#ifndef __CRYPT_H__
#define __CRYPT_H__

#include "tclmon/mespro_crypt.h"

void getMesProParams(const char *head, int hlen, int *error, MPCryptParams &params);
int form_crypt_error(char *res, char *head, int hlen, int error);

#endif // __CRYPT_H__
