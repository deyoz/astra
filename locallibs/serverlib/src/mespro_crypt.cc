#if HAVE_CONFIG_H
#endif

#ifdef USE_MESPRO

#include <string>
#include <vector>
#include "mespro.h"
#include "mespro_crypt.h"
#include "tcl_utils.h"
#include "tclmon.h"

#define NICKNAME "ILYA"
#include "test.h"

static bool CryptingInitialized=false;

const int INPUT_FMT=FORMAT_ASN1;
const int OUTPUT_FMT=FORMAT_ASN1;
const char CIPHER_ALGORITHM[]="RUS-GAMMAR";

static int init_rand_callback(int c, int step, int from, char *userdata)
{
	char cc[64];
	sprintf(cc,"%c",c);
	return *cc;
}

bool initMesProCrypting(MPCryptParams &params)
{
  if(!CryptingInitialized)
  {
    ProgTrace( TRACE5, "mespro init" );
    SetRandInitCallbackFun(init_rand_callback);
    int err=PKCS7Init(0,0);
    if(err)
    {
      ProgError(STDLOG, "PKCS7Init() failed with code %i\n",err);
      return false;
    }

    err=SetInputFormat(INPUT_FMT);
    if(err)
    {
      ProgError(STDLOG, "SetInputFormat(FORMAT_ASN1) failed with code %i\n",err);
      return false;
    }

    err=SetOutputFormat(OUTPUT_FMT);
    if(err)
    {
      ProgError(STDLOG, "SetOutputFormat(FORMAT_ASN1) failed with code %i\n",err);
      return false;
    }

    err=SetCipherAlgorithm((char *)CIPHER_ALGORITHM);
    if(err)
    {
      ProgError(STDLOG, "SetCipherAlgorithm(\"RUS-GAMMAR\") failed with code %i\n",err);
      return false;
    }

    err=AddCAFromBuffer((char *)params.CA.data(),params.CA.size());
    if(err)
    {
      ProgError(STDLOG, "AddCAFromBuffer() failed with code %i\n",err);
      return false;
    }

    // Проверим тип сертификата сервера:
    // Если RSA/DSA - для загрузки секретного ключа используем AddPrivateKeyFromBuffer()
    // Если ГОСТ - используем AddPSEPrivateKeyEx
    char *cert_algo=GetCertPublicKeyAlgorithmBuffer((char *)params.server_cert.data(),params.server_cert.size());
    if(!cert_algo)
    {
      ProgError(STDLOG, "GetCertPublicKeyAlgorithmBuffer() failed\n");
      return false;
    }
    if(strcmp(cert_algo,MP_KEY_ALG_NAME_RSA)!=0 && strcmp(cert_algo,MP_KEY_ALG_NAME_DSA)!=0) // ГОСТ
    {
      std::string PSEpath=readStringFromTcl("MESPRO_PSE_PATH","./crypt");
      err=AddPSEPrivateKeyFromBufferEx((char *)PSEpath.c_str(),0,(char *)params.PKey.data(),params.PKey.size(),params.PKeyPass.empty()?0:((char *)params.PKeyPass.c_str()));
      if(err)
      {
        ProgError(STDLOG, "AddPSEPrivateKeyFromBufferEx() failed with code %i, PSEpath='%s'\n",err,PSEpath.c_str());
        return false;
      }
    }
    else
    {
      err=AddPrivateKeyFromBuffer((char *)params.PKey.data(),params.PKey.size(),params.PKeyPass.empty()?0:((char *)params.PKeyPass.c_str()));
      if(err)
      {
        ProgError(STDLOG, "AddPrivateKeyFromBuffer() failed with code %i\n",err);
        return false;
      }
    }

    CryptingInitialized=true;
  }
  return true;
}

std::string prepareCrypting(const char *head, int hlen, int *error, int pr_encrypt)
{
  MPCryptParams params;
  getMPCryptParams(head,hlen,error,params);
  if(*error)
  {
    ProgError(STDLOG,"getMPCryptParams() failed with error=%d",*error);
    return std::string();
  }

  if(!initMesProCrypting(params))
  {
    *error=UNKNOWN_ERR;
    return std::string();
  }

  return pr_encrypt ? params.client_cert : params.server_cert;
}

//-----------------------------------------------------------------------
int mespro_decrypt_internal(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out, bool pr_init)
{
    void* out_buf = NULL;
    try
    {
        int error = 0;
        if ( pr_init ) { // reinit crypt library variables
          PKCS7Final();
          CryptingInitialized = false;
        }
        std::string cl_cert = prepareCrypting((const char*)head,hlen,&error,0);
        if(error!=0)
        {
            ProgError(STDLOG, "prepareCrypting() failed with %d\n",error);
            throw error;
        }

        error = AddCertificateFromBuffer((char*)cl_cert.data(), cl_cert.size());
        if(error!=0)
        {
            ProgError(STDLOG, "AddCertificateFromBuffer() returned error=%d\n",error);
            error=UNKNOWN_ERR;
            throw error;
        }

        int out_len = 0;

        error = DecryptBuffer((void*)in.data(), in.size(), &out_buf, &out_len);
        if(error!=0)
        {
            ProgError(STDLOG, "DecryptBuffer() returned error=%d\n",error);
            error=WRONG_KEY;
            throw error;
        }

        if ( !error && pr_init ) {
            ProgError(STDLOG, "DecryptBuffer() is good\n");
        }

        error = ClearCertificates();
        if(error!=0)
        {
            ProgError(STDLOG, "ClearCertificates() returned error=%d\n",error);
            error=UNKNOWN_ERR;
            throw error;
        }

        if (out_buf != NULL)
        {
            out.resize(out_len);
            memcpy(out.data(), out_buf, out_len);
        }
        else out.clear();
        if (out_buf != NULL) FreeBuffer(out_buf);
        return 0;
    }
    catch(int err)
    {
        out.clear();
        if (out_buf != NULL) FreeBuffer(out_buf);
        return err;
    }
    catch(...)
    {
        out.clear();
        if (out_buf != NULL) FreeBuffer(out_buf);
        return UNKNOWN_ERR;
    }
}

int mespro_decrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
  int res = mespro_decrypt_internal( head, hlen, in, out, false );
  if ( res &&
       readIntFromTcl("MESPRO_RELOAD_AFTER_ERROR",1) == 1 ) { // error && debug in localafter.tcl
    return mespro_decrypt_internal( head, hlen, in, out, true ); //reinit lib mespro & new decrypt data
  }
  return res;
}

int mespro_encrypt_internal(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out, bool pr_init)
{
    void* CTX = NULL;
    void* out_buf = NULL;
    try
    {
        int error = 0;
        if ( pr_init ) { // reinit crypt library variables
          PKCS7Final();
          CryptingInitialized = false;
        }
        std::string cl_cert = prepareCrypting((const char*)head,hlen,&error,1);
        if(error!=0)
        {
            ProgError(STDLOG, "prepareCrypting() failed with %d\n",error);
            throw error;
        }

        CTX = GetCipherCTX();

        if(CTX == NULL)
        {
            ProgError(STDLOG, "GetCipherCTX() failed to create context\n");
            error=UNKNOWN_ERR;
            throw error;
        }

        long cert_size=cl_cert.size();
        error=AddRecipient(CTX,BY_BUFFER,(void *)cl_cert.data(),&cert_size);
        if(error!=0)
        {
            ProgError(STDLOG, "AddRecipient() failed with %d\n",error);
            error=WRONG_KEY;
            throw error;
        }

        int out_len = 0;

        error=EncryptBuffer(CTX, (void*)in.data(), in.size(), &out_buf, &out_len);
        if(error!=0)
        {
            ProgError(STDLOG, "EncryptBuffer() returned error=%d\n",error);
            error=WRONG_KEY;
            throw error;
        }

        if ( !error && pr_init ) {
            ProgError(STDLOG, "EncryptBuffer() is good\n");
        }

        if (out_buf != NULL)
        {
            out.resize(out_len);
            memcpy(out.data(), out_buf, out_len);
        }
        else out.clear();
        if (CTX!=NULL) FreeCipherCTX(CTX);
        if (out_buf != NULL) FreeBuffer(out_buf);
        return 0;
    }
    catch(int err)
    {
        out.clear();
        if (CTX!=NULL) FreeCipherCTX(CTX);
        if (out_buf != NULL) FreeBuffer(out_buf);
        return err;
    }
    catch(...)
    {
        out.clear();
        if (CTX!=NULL) FreeCipherCTX(CTX);
        if (out_buf != NULL) FreeBuffer(out_buf);
        return UNKNOWN_ERR;
    }
}

int mespro_encrypt(const void* head, size_t hlen, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
  int res = mespro_encrypt_internal( head, hlen, in, out, false );
  if ( res &&
       readIntFromTcl("MESPRO_RELOAD_AFTER_ERROR",1) == 1 ) { // error && debug in localafter.tcl
    return mespro_encrypt_internal( head, hlen, in, out, true ); //reinit lib mespro & new encrypt data
  }
  return res;

}

#endif // USE_MESPRO
