#ifndef __SSL_EMPLOYMENT_H_
#define __SSL_EMPLOYMENT_H_

#if HAVE_CONFIG_H
#endif

#ifdef HAVE_SSL

#include <vector>
#include <stdint.h>

int pub_decrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out);
int pub_encrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out);

int sym_decrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out);
int sym_encrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out, int* sym_key_id);

#endif

#endif // __SSL_EMPLOYMENT_H_
