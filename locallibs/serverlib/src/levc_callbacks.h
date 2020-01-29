#ifndef __LEVC_CALLBACKS_H_
#define __LEVC_CALLBACKS_H_

#include <vector>
#include <stdint.h>

void levC_compose (const char *head, size_t hlen, const std::vector<uint8_t>& body, std::vector<uint8_t>& a_head, std::vector<uint8_t>& a_body);

bool levC_is_mespro_crypted(const char * h);
bool levC_should_mespro_crypt(const char *h, int len);

bool levC_is_sym_crypted(const char * h);
bool levC_should_sym_crypt(const char *h, int len);

bool levC_should_pub_crypt(const char *h, int len);
bool levC_is_pub_crypted(const char * h);

void levC_adjust_crypt_header(char *h, int newlen, int flags, int sym_key_id);

size_t levC_form_crypt_error(char* res, size_t res_len, const char* head, size_t hlen, int error);

bool levC_is_compressed(const char *h);
bool levC_should_compress(const uint8_t* h, size_t blen);

void levC_adjust_header(uint8_t* h, uint32_t newlen);

bool levC_is_perespros(const std::vector<uint8_t>& head);

#endif // __LEVC_CALLBACKS_H_;
