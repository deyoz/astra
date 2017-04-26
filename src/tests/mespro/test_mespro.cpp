
#ifdef USE_MESPRO

#include "mespro.h"
#include "crypt.h"
#include "tclmon/mespro_crypt.h"
#include "oralib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <random>
#include <chrono>

#define UNKNOWN_KEY               1 /* ?????? ?????????? */
#define EXPIRED_KEY               2 /* ???????? ?????????????????? */
#define WRONG_KEY                 3 /* ???????? ?????????????? */
#define WRONG_SYM_KEY             4 /* wrong symmetric key */
#define WRONG_OUR_KEY             9 /* client has wrong our public key */
#define UNKNOWN_SYM_KEY           5 /* unknown symmetric key */
#define UNKNOWN_ERR               6 /* ???????????? ???????????? */
#define CRYPT_ERR_READ_RSA_KEY    7 /* ???????????? ???????????? ????????.?????????? RSA */
#define CRYPT_ALLOC_ERR           8 /* memory allocation error */
#define UNKNOWN_CA_CERTIFICATE     10    /* CA ???????????????????? ???? ???????????? */
#define UNKNOWN_SERVER_CERTIFICATE 11    /* Server ???????????????????? ???? ???????????? */
#define UNKNOWN_CLIENT_CERTIFICATE 12 /* Client ???????????????????? ???? ???????????? */
#define WRONG_TERM_CRYPT_MODE      13 /* ???????????????????????? ?????????? ???????????? ?????????????? */

bool GetClientCertificate( TQuery &Qry, int grp_id, bool pr_grp, const std::string &desk, std::string &certificate, int &pkcs_id );
void GetServerCertificate( TQuery &Qry, std::string &ca, std::string &pk, std::string &server );

namespace mptest {

static bool CryptingInitialized=false;

const int INPUT_FMT=FORMAT_ASN1;
const int OUTPUT_FMT=FORMAT_ASN1;
const char CIPHER_ALGORITHM[]="RUS-GAMMAR";

void getClientPKey(std::string& key)
{
    struct stat st;
    const char* filename = "./client_pkey.key";
    if(!stat(filename, &st)) {
        int fd = open(filename, O_RDONLY);
	char* data = new char[st.st_size];
        read(fd, data, st.st_size);
        key.assign(data, st.st_size);
        delete [] data;
        close(fd);
    }
}

struct MPCryptParams
{
  std::string CA; // ???????? ??????????
  
  std::string SrvPKey; // ????????? ???? ???????
  std::string SrvPKeyPass; // ?????? ? ?????????? ????? ???????
  std::string ServerCert; // ?????????? ???????

  std::string ClPKey; // ????????? ???? ???????
  std::string ClPKeyPass; // ?????? ? ?????????? ????? ???????
  std::string ClientCert; // ?????????? ???????
};

void getMesProCryptParams(int *error, MPCryptParams &params)
{
    params.CA.clear();
    
    params.SrvPKey.clear();
    params.ServerCert.clear();
    
    params.ClPKey.clear();
    params.ClientCert.clear();

    using namespace std;
    string desk = "OSSR01";
    TQuery Qry(&OraSession);

    GetServerCertificate( Qry, params.CA, params.SrvPKey, params.ServerCert );
    if ( params.SrvPKey.empty() ) {
        *error = UNKNOWN_KEY;
        return;
    }
    if ( params.CA.empty() ) {
        *error = UNKNOWN_CA_CERTIFICATE;
        return;
    }
    if ( params.ServerCert.empty() ) {
        *error = UNKNOWN_SERVER_CERTIFICATE;
        return;
    }

    int pkcs_id = 0;
    bool pr_exists = GetClientCertificate( Qry, 8495324, true, desk, params.ClientCert, pkcs_id );
    if ( params.ClientCert.empty() ) {
        if ( pr_exists )
            *error = EXPIRED_KEY;
        else
            *error = UNKNOWN_CLIENT_CERTIFICATE;
        return;
    }

    getClientPKey(params.ClPKey);
}

static int init_rand_callback(int c, int step, int from, char *userdata)
{
    char cc[64];
    sprintf(cc,"%c",c);
    return *cc;
}

bool initMesProCrypting(const std::string& CA, const std::string& Cert, const std::string& Key, bool client)
{
    if(!CryptingInitialized)
    {
        SetRandInitCallbackFun((void*)init_rand_callback);
        int err=PKCS7Init(0,0);
        if(err)
            return false;

        err=SetInputFormat(INPUT_FMT);
        if(err)
            return false;

        err=SetOutputFormat(OUTPUT_FMT);
        if(err)
            return false;

        err=SetCipherAlgorithm((char *)CIPHER_ALGORITHM);
        if(err)
            return false;

        err=AddCAFromBuffer((char *)CA.data(), CA.size());
        if(err)
            return false;

        if(Cert.size() && Key.size()) {
            // ???????? ??? ??????????? ???????:
            // ???? RSA/DSA - ??? ???????? ?????????? ????? ?????????? AddPrivateKeyFromBuffer()
            // ???? ???? - ?????????? AddPSEPrivateKeyEx
            char *cert_algo = GetCertPublicKeyAlgorithmBuffer((char *)Cert.data(), Cert.size());

            if(!cert_algo)
                return false;

            if(strcmp(cert_algo, "ECR3410") == 0 || strcmp(cert_algo, "R3410") == 0) // ????
            {
            	std::string crypt_dir = client ? "./crypt_client" : "./crypt_srv";
                err = AddPSEPrivateKeyFromBufferEx((char*)crypt_dir.c_str(), 0, (char*) Key.data(), Key.size(), 0);
                if(err)
                    return false;
            }
            else
            {
                err = AddPrivateKeyFromBuffer((char *) Key.data(), Key.size(), 0);
                if(err)
                    return false;
            }
        }

        CryptingInitialized = true;
    }

    return true;
}

void finishMesProCrypting()
{
    if(CryptingInitialized) {
        ClearPrivateKeys();
        ClearCertificates();
        ClearCRLs();
        ClearCAs();
        PKCS7Final();
        CryptingInitialized = false; 
    }
}

int mespro_decrypt(int& error, std::string& cl_cert, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    void* out_buf = NULL;
    try
    {
        error = AddCertificateFromBuffer((char*)cl_cert.data(), cl_cert.size());
        if(error!=0)
        {
            error=UNKNOWN_ERR;
            throw error;
        }

        int out_len = in.size() + 100;
        out_buf = malloc(out_len);

        if(out_buf == NULL)
        {
            error=CRYPT_ALLOC_ERR;
            throw error;
        }

        error = DecryptBuffer((void*)in.data(), in.size(), &out_buf, &out_len);
        if(error!=0)
        {
            error=WRONG_KEY;
            throw error;
        }

        error = ClearCertificates();
        if(error!=0)
        {
            error=UNKNOWN_ERR;
            throw error;
        }

        if (out_buf != NULL)
        {
            out.resize(out_len);
            memcpy(out.data(), out_buf, out_len);
        }
        else out.clear();
        if (out_buf != NULL) free(out_buf);
        return 0;
    }
    catch(int err)
    {
        out.clear();
        if (out_buf != NULL) free(out_buf);
        return err;
    }
    catch(...)
    {
        out.clear();
        if (out_buf != NULL) free(out_buf);
        return UNKNOWN_ERR;
    }
}

int mespro_encrypt(int& error, std::string& cl_cert, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    void* CTX = NULL;
    void* out_buf = NULL;
    try
    {
        CTX = GetCipherCTX();

        if(CTX == NULL)
        {
            error=UNKNOWN_ERR;
            throw error;
        }

        long cert_size = cl_cert.size();
        error=AddRecipient(CTX,BY_BUFFER,(void *)cl_cert.data(),&cert_size);
        if(error!=0)
        {
            error=WRONG_KEY;
            throw error;
        }

        int out_len = in.size() + 2048;
        out_buf = malloc(out_len);

        if(out_buf == NULL)
        {
            error=CRYPT_ALLOC_ERR;
            throw error;
        }

        error=EncryptBuffer(CTX, (void*)in.data(), in.size(), &out_buf, &out_len);
        if(error!=0)
        {
            error=WRONG_KEY;
            throw error;
        }

        if (out_buf != NULL)
        {
            out.resize(out_len);
            memcpy(out.data(), out_buf, out_len);
        }
        else out.clear();
        if (CTX!=NULL) FreeCipherCTX(CTX);
        if (out_buf != NULL) free(out_buf);
        return 0;
    }
    catch(int err)
    {
        out.clear();
        if (CTX!=NULL) FreeCipherCTX(CTX);
        if (out_buf != NULL) free(out_buf);
        return err;
    }
    catch(...)
    {
        out.clear();
        if (CTX!=NULL) FreeCipherCTX(CTX);
        if (out_buf != NULL) free(out_buf);
        return UNKNOWN_ERR;
    }
}

void fill_orig(std::vector<uint8_t>& vec, size_t size)
{
    vec.clear();
    unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();

    typedef std::linear_congruential_engine<uint_fast32_t, 48271, 0, 2147483647> minstd_rand_1;
    minstd_rand_1 g1(seed1);

    size_t max = std::ceil((float)size / sizeof(minstd_rand_1::result_type));
    vec.resize(max * sizeof(minstd_rand_1::result_type));
    minstd_rand_1::result_type* ref = reinterpret_cast<minstd_rand_1::result_type*>(vec.data());
    
    for(size_t s = 0; s < max; ++s)
        ref[s] = g1();

    vec.resize(size);
}

int test_mespro(int, char**)
{
    int error = 0;

    std::vector<uint8_t> orig, enc, dec;

    fill_orig(orig, 966);

    MPCryptParams params;
    getMesProCryptParams(&error,params);
    if(error)
        return 1;

    if(!initMesProCrypting(params.CA, "", ""/*params.ClientCert, params.ClPKey*/, true))
	return 1;
    
    mespro_encrypt(error, params.ServerCert, orig, enc);
    finishMesProCrypting();

    if(!initMesProCrypting(params.CA, params.ServerCert, params.SrvPKey, false))
	return 1;

    mespro_decrypt(error, params.ServerCert, enc, dec);

    std::cout << (orig == dec ? "GOOD" : "BAD") << std::endl;

    return 0;
}

} // namespace mptest

#else// USE_MESPRO

namespace mptest {

int test_mespro(int, char**)
{
}

}

#endif // USE_MESPRO
