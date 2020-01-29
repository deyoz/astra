#if HAVE_CONFIG_H

#endif

#include <memory>
#ifdef HAVE_SSL
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <openssl/opensslv.h>
#endif /* HAVE_SSL*/
#include "msg_queue.h"
#include "tclmon.h"
#include "msg_framework.h"
#include "sirena_queue.h"
#include "ssl_employment.h"
#include "crypting.h"

#define NICKNAME "SYSTEM"
#include "slogger.h"

#ifdef HAVE_SSL

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
// OPENSSL 1.1
#define HAVE_OPENSSL11 1
#endif // 1.1 

/*************************************************************************/
/* returns 0 on success, error code (tclmon.h) on failure */

/* #define _ENABLE_RANDOM_ */

int InitRandomFileFlag=0;

int sym_decrypt_internal(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out, int padding)
{
    const char* h = static_cast<const char*>(head);
    if(h[0] == 2)  ProgTrace(TRACE5,"%s :: from client %u (padding=%i)",__FUNCTION__,uint16_t(uint8_t(h[45]))*256+uint8_t(h[46]),padding);
    if(in.empty())
    {
        out.clear();
        return 0;
    }

    unsigned char sym_key_str[EVP_MAX_KEY_LENGTH];

    int sym_key_id;
    if(int error=get_sym_key(h, hlen, sym_key_str,sizeof(sym_key_str), &sym_key_id))
        return error;

#ifdef HAVE_OPENSSL11
    std::shared_ptr<EVP_CIPHER_CTX> ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
#else // openssl 1.0
    std::shared_ptr<EVP_CIPHER_CTX> ctx(new EVP_CIPHER_CTX, EVP_CIPHER_CTX_cleanup);
#endif // openssl 1.0
    EVP_CIPHER_CTX_init(ctx.get());
    EVP_CIPHER_CTX_set_padding(ctx.get(),padding);
    EVP_DecryptInit(ctx.get(), EVP_des_ecb(), sym_key_str, NULL);
    out.resize(in.size()+100);

    int out_len = out.size();
    if(EVP_DecryptUpdate(ctx.get(), out.data(), &out_len, in.data(), in.size()) != 1) {

        return WRONG_SYM_KEY;
    }

    int tlen;
    if(EVP_DecryptFinal(ctx.get(), out.data()+out_len, &tlen) != 1)
    {
        if(padding)
            return sym_decrypt_internal(head,hlen,in,out,0);
        return WRONG_SYM_KEY;
    }

    out.resize(out_len+tlen);
    return 0;
}

int sym_decrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    return sym_decrypt_internal(head, hlen, in, out, 1);
}

int sym_encrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out, int* sym_key_id)
{
    if(in.empty())
    {
        out.clear();
        return 0;
    }

    unsigned char sym_key_str[EVP_MAX_KEY_LENGTH];

    int sym_key_id_local;
    if(int error=get_sym_key(static_cast<const char*>(head), hlen, sym_key_str,sizeof(sym_key_str),&sym_key_id_local))
        return error;

#ifdef HAVE_OPENSSL11
    std::shared_ptr<EVP_CIPHER_CTX> ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
#else // openssl 1.0
    std::shared_ptr<EVP_CIPHER_CTX> ctx(new EVP_CIPHER_CTX, EVP_CIPHER_CTX_cleanup);
#endif // openssl 1.0
    EVP_CIPHER_CTX_init(ctx.get());
    EVP_EncryptInit(ctx.get(), EVP_des_ecb(), sym_key_str, NULL);
    out.resize(in.size()+100);

    int out_len = out.size();
    if(EVP_EncryptUpdate(ctx.get(), out.data(), &out_len, in.data(), in.size()) != 1)
        return WRONG_SYM_KEY;

    int tlen;
    if(EVP_EncryptFinal(ctx.get(), out.data()+out_len, &tlen) != 1)
        return WRONG_SYM_KEY;

    out.resize(out_len+tlen);

    if(sym_key_id)
        *sym_key_id = sym_key_id_local;
    return 0;
}

int init_random_file(void)
{
  if(InitRandomFileFlag!=0)
  {
     return 0;
  }
#ifdef _ENABLE_RANDOM_
  char rand_file_name[1000];
  if(RAND_file_name(rand_file_name, sizeof(rand_file_name)) == NULL)
  {
     ProgError(STDLOG, "RAND_file_name Error");
     return -1;
  }
  int Ret = RAND_load_file(rand_file_name, -1);
  if(Ret <= 0)
  {
     ProgError(STDLOG, "RAND_load_file() Error(%d)", Ret);
     return -1;
  }
#endif /* _ENABLE_RANDOM_*/
/*
SSL_library_init();
SSL_load_error_strings();
*/
 ERR_load_RSA_strings();
 InitRandomFileFlag = 1;
return 0;
}

std::shared_ptr<RSA> read_our_private_key(int* error)
/* returns 0 on success, error code (tclmon.h) on failure */
{
  char priv_key_str[1000];
  int  priv_key_len=sizeof(priv_key_str);

  if(int err=get_our_private_key(priv_key_str, &priv_key_len))
  {
      if(error)
          *error = err;
      return std::shared_ptr<RSA>();
  }

  BIO *bio_key = BIO_new( BIO_s_mem() );
  BIO_write(bio_key, priv_key_str, priv_key_len);

  std::shared_ptr<RSA> RsaOurStr(PEM_read_bio_RSAPrivateKey(bio_key, NULL, NULL, NULL), RSA_free);
  if(not RsaOurStr)
  {
      ProgError(STDLOG, "Error read private key from bio:\n%s\n",ERR_error_string(ERR_get_error(), NULL));
      if(error)
          *error = CRYPT_ERR_READ_RSA_KEY;
  }
  *error = 0;
  return RsaOurStr;
}

RSA *get_remote_public_key(const char *head, int hlen, int *error)
{
  char pub_key_str[1000];
  int pub_key_len = sizeof(pub_key_str);

  *error=get_public_key(head, hlen, pub_key_str, &pub_key_len);
  if(*error!=0)
    return NULL;

  BIO* bio_key = BIO_new( BIO_s_mem() );
  BIO_write(bio_key, pub_key_str, pub_key_len);
  RSA* rsa_tmp = PEM_read_bio_RSA_PUBKEY(bio_key, NULL, NULL, NULL);
  if(rsa_tmp == NULL)
  {
     ProgError(STDLOG, "Error read public key from bio:\n%s\n",ERR_error_string(ERR_get_error(), NULL));
     *error = CRYPT_ERR_READ_RSA_KEY;
     return NULL;
  }
  return rsa_tmp;
}

//-----------------------------------------------------------------------

int pub_decrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
  const char* h = static_cast<const char*>(head);
  if(h[0] == 2)  ProgTrace(TRACE5,"%s :: from client %u",__FUNCTION__,h[45]*256+h[46]);

  if(init_random_file())
      return UNKNOWN_ERR;

  int error;
  const auto RsaOurStr = read_our_private_key(&error);
  if(not RsaOurStr)
      return error;

  std::shared_ptr<RSA> RsaRemStr(get_remote_public_key(h, hlen, &error), RSA_free);
  if(not RsaRemStr)
      return error;

  out.resize(in.size() + 100);
  size_t bz = out.size();

  error = RSA_decrypt(in.data(), in.size(), out.data(), &bz, RsaOurStr.get(), RsaRemStr.get());

  if(error)  out.clear();
  else       out.resize(bz);

  return error;
}

int pub_encrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
  if(init_random_file())
      return UNKNOWN_ERR;

  int error;
  const auto RsaOurStr = read_our_private_key(&error);
  if(not RsaOurStr)
      return error;

  std::shared_ptr<RSA> RsaRemStr(get_remote_public_key(static_cast<const char*>(head), hlen, &error), RSA_free);
  if(not RsaRemStr)
      return error;

  out.resize(in.size() + RSA_size(RsaOurStr.get()) + RSA_size(RsaRemStr.get()) + sizeof(int));
  size_t bz = out.size();

  error = RSA_encrypt(in.data(), in.size(), out.data(), &bz, RsaOurStr.get(), RsaRemStr.get());

  if(error)  out.clear();
  else       out.resize(bz);

  return error;
}

/*************************************************************************/
int RSA_decrypt(const unsigned char *src_str, size_t src_len,
                unsigned char *dest_str, size_t* dest_len,
                RSA *rsa_our, RSA *rsa_remote)
/* returns 0 on success, error code (tclmon.h) on failure */
{
#if __cplusplus > 201100L
  constexpr
#endif
            size_t len_off = sizeof(int);

  if(src_len < len_off)
  {
    /* Не будем писать FL_LOG_ERR, так как проблема не у нас */
    LogTrace(TRACE1)<<"RSA_decrypr error: SrcLen ("<<src_len<<") is too small\n";
    return UNKNOWN_ERR;
  }

  int32_t len_tmp_crypt;
  memcpy(&len_tmp_crypt, src_str, sizeof(int32_t));
  len_tmp_crypt=ntohl(len_tmp_crypt);
  if(len_tmp_crypt < 0 || static_cast<size_t>(len_tmp_crypt) >= src_len-len_off)
  {
    /* Не будем писать FL_LOG_ERR, так как проблема не у нас */
    LogTrace(TRACE1)<<"RSA_decrypt error: wrong CryptLen ("<<len_tmp_crypt<<"), SrcLen="<<src_len<<"\n";
    return UNKNOWN_ERR;
  }
  const size_t len_tmp_sign = src_len-len_off-len_tmp_crypt;

  unsigned char digest_str[100];
  size_t digest_len = sizeof(digest_str);
  int Ret = calc_digest(src_str+len_off, len_tmp_crypt, digest_str, &digest_len);
  if(Ret != 0)
  {
    /* Не будем писать FL_LOG_ERR, так как проблема не у нас */
    LogTrace(TRACE1)<< "RSA_decrypt error: calc_digest(dest_len="<<sizeof(digest_str)<<")\n";
    return UNKNOWN_ERR;
  }

  Ret = RSA_verify(NID_sha1, digest_str, digest_len,
                   const_cast<unsigned char*>(src_str)+len_off+len_tmp_crypt, len_tmp_sign, rsa_remote);
  if(Ret != 1)
  {
    /* Не будем писать FL_LOG_ERR, так как проблема не у нас */
    LogTrace(TRACE1) << "RSA_decrypt error: RSA_verify <" << ERR_error_string(ERR_get_error(), NULL) << ">\n";
    return WRONG_KEY;
  }
  // they swear not to taint source buffer... So be it, then!
  int len_decrypt = RSA_private_decrypt(len_tmp_crypt, const_cast<unsigned char*>(src_str)+len_off, dest_str, rsa_our, RSA_PKCS1_PADDING);
  if(len_decrypt == -1)
  {
    /* Не будем писать FL_LOG_ERR, так как проблема не у нас */
    LogTrace(TRACE1) << "RSA_decrypt error: private_decrypt <" << ERR_error_string(ERR_get_error(), NULL) << ">\n";
    return WRONG_OUR_KEY;
  }
  *dest_len = len_decrypt;
  return 0;
}

/*************************************************************************/

// RSA_encrypt работает аналогично:
// openssl pkeyutl -encrypt -in deskey -out deskey.crypted -pubin -inkey our_public.key
// openssl dgst -sha1 -binary -out des.crypted.sha1.crypted -sign private.key deskey.crypted

int RSA_encrypt(const unsigned char *src_str, size_t src_len,
                unsigned char *dest_str, size_t *dest_len,
                RSA *rsa_our, RSA *rsa_remote)
/* returns 0 on success, error code (tclmon.h) on failure */
{
#if __cplusplus > 201100L
  constexpr
#endif
            size_t len_off = sizeof(int);

  int32_t len_tmp_crypt = RSA_public_encrypt(src_len, const_cast<unsigned char*>(src_str),
                                             dest_str+len_off,rsa_remote,
                                             RSA_PKCS1_PADDING);
  if(len_tmp_crypt == -1)
  {
     LogError(STDLOG)
         << "RSA_encrypt error: Public_encrypt <"
         << ERR_error_string(ERR_get_error(), NULL)
         << ">\n";
     return WRONG_KEY;
  }

  unsigned char digest_str[100];
  size_t digest_len = sizeof(digest_str);
  int Ret = calc_digest(dest_str+len_off, len_tmp_crypt, digest_str, &digest_len);
  if(Ret != 0)
  {
      LogError(STDLOG)
          << "RSA_encrypt error: calc_digest(dest_len="
          << sizeof(digest_str)
          << ")\n";
      return UNKNOWN_ERR;
  }

  unsigned int len_tmp_sign=0;
  Ret = RSA_sign( NID_sha1, digest_str, digest_len,
                  dest_str+len_off+len_tmp_crypt, &len_tmp_sign,
                  rsa_our);
  if(Ret == 0)
  {
     LogError(STDLOG)
         << "RSA_encrypt error: RSA_sign <"
         << ERR_error_string(ERR_get_error(), NULL)
         << ">\n";
     return WRONG_OUR_KEY;
  }
  *(int32_t *)(dest_str) = htonl(len_tmp_crypt);
  *dest_len = len_off+len_tmp_crypt+len_tmp_sign;
  return 0;
}

int calc_digest(const unsigned char *src_str, size_t src_len, unsigned char *dest_str, size_t *dest_len)
{
  if(*dest_len < SHA_DIGEST_LENGTH )
  {
     return 1;
  }
  SHA1( src_str, src_len, dest_str);
  *dest_len = SHA_DIGEST_LENGTH;
return 0;
}

#else /* ifndef HAVE_SSL */

int calc_digest(const unsigned char *src_str, int src_len, unsigned char *dest_str,
                int *dest_len)
{
  *dest_len=0;
  return 1;
}

#endif /* ! HAVE_SSL*/

