#ifndef __MESPRO_CRYPT_H__
#define __MESPRO_CRYPT_H__

#ifdef __cplusplus
#include <string>
#include <vector>
#include <stdint.h>
 
struct MPCryptParams
{
  std::string CA; // Корневой сертификат
  std::string PKey; // Секретный ключ сервера
  std::string PKeyPass; // Пароль к секретному ключу сервера
  std::string server_cert; // Сертификат сервера

  std::string client_cert; // Сертификат клиента
};

void getMPCryptParams(const char *head, int hlen, int *err, MPCryptParams &params);

#ifdef USE_MESPRO
int mespro_decrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out);
int mespro_encrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out);
#endif // USE_MESPRO

#endif // __cplusplus

#endif // __MESPRO_CRYPT_H__

