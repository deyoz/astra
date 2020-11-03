#ifndef __MESPRO_CRYPT_H__
#define __MESPRO_CRYPT_H__

#ifdef __cplusplus
#include <string>
#include <vector>
#include <stdint.h>
 
struct MPCryptParams
{
  std::string CA; // ��୥��� ���䨪��
  std::string PKey; // ������ ���� �ࢥ�
  std::string PKeyPass; // ��஫� � ᥪ�⭮�� ����� �ࢥ�
  std::string server_cert; // ����䨪�� �ࢥ�

  std::string client_cert; // ����䨪�� ������
};

void getMPCryptParams(const char *head, int hlen, int *err, MPCryptParams &params);

#ifdef USE_MESPRO
int mespro_decrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out);
int mespro_encrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out);
#endif // USE_MESPRO

#endif // __cplusplus

#endif // __MESPRO_CRYPT_H__

