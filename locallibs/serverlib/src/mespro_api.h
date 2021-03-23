#ifndef __CRYPT_API_H__
#define __CRYPT_API_H__
#ifdef USE_MESPRO
#include "mespro.h"

static int init_rand_callback(int c, int step, int from, char *userdata)
{
        char cc[64];
        sprintf(cc,"%c",c);
        return *cc;
}
bool isMespro_V1();

namespace MESPRO_API {
std::string GetAlgo( const std::string& algo );
/* Initialization and Completion Functions */
int PKCS7Init(char *pse_path, int reserved);
int PKCS7Final(void);

void setLibName( const std::string& lib_name );
void setLibNameFromAlgo( const std::string& algo );
std::string getPSEPath( );
const std::string& getLibPath( );
bool isInitLib( );
void setInitLib( bool pr_init );

int SetInputCodePage(int codepage);

/* Random Number Generator Functions */
void SetRandInitCallbackFun(int (*Func)(int, int, int, char *));
void SetRandInitCallbackFun(void (*Func)());

int SetNewKeysAlgorithm(char *algor);
int SetCertificateRequestFlags(unsigned long flags);
int SetCountry(char *Country);
int SetStateOrProvince(char *StateOrProvince);
int SetLocality(char *Locality);
int SetOrganization(char *Organization);
int SetOrganizationalUnit(char *OrganizationalUnit);
int SetTitle(char *Title);
int SetCommonName(char *CommonName);
int SetEmailAddress(char *EmailAddress);

//!!!prior version
int PSE31_Generation(char *pse_path, int reserv1, char *reserv2, unsigned long flags);
int Copy_PSE31(char *pse_path, char *reserv, char *new_pse_path, unsigned long flags);
int Erase_PSE31(char *pse_path);

/* Key and Certificate Request Generation Functions depends of lib version!!!*/
int PSE_Generation(char *pse_path, int reserv, char *passwd, unsigned long flags);
int PSE_Copy(char *pse_path, char *passwd, char *new_pse_path, unsigned long flags);
int PSE_Erase(char *pse_path);

int NewKeysGenerationEx(char *pse_path, char *pse_pass, char *keyfile, char *key_pass, char *reqfile);
int NewKeysGeneration(char *keyfile, char *password, char *reqfile);	/* obsolete, only used for RSA and DSA algorithms */

void ClearBuffer(unsigned char *buf);

int SetInputFormat(int form);
int SetOutputFormat(int form);

int SetCipherAlgorithm(char *algor);

/* CA Certificate Loading Functions */
int AddCA(char *CAfile);
int AddCAFromBuffer(char *buf, int ln);
int AddCAs(char *CAdir);
int ClearCAs(void);

int AddPSEPrivateKeyFromBufferEx(char *pse_path, char *pse_pass, char *buf, int len, char *key_pass);
int AddPrivateKeyFromBuffer(char *buf, int ln, char *password);			/* obsolete, only used for RSA and DSA algorithms */

char* GetCertPublicKeyAlgorithmBuffer(char *buf, int len);
int AddCertificateFromBuffer(char *buf, int ln);
int CertAndRequestMatchBuffer(char *certbuf, int certln, char *reqbuf, int reqln);
int GetCertificateInfoBufferEx(char *buf, int ln, CERTIFICATE_INFO *info);
void FreeCertificateInfo(CERTIFICATE_INFO *info);

int DecryptBuffer(void *in_buf, int in_len,void **out_buf, int *out_len);

int ClearCertificates(void);
void FreeBuffer(void *ptr);

void* GetCipherCTX(void);
int AddRecipient(void *ctx, int type, void *param1, void *param2);
int SetCipherEncapsulatedContentType(void *ctx, int type);
void FreeCipherCTX(void *ctx);

int EncryptBuffer(void *ctx, void *in_buf, int in_len, void **out_buf, int *out_len);

int SetKeysLength(int bits);

/* Private Key Password Changing Functions */
int ChangePrivateKeyPassword(char *keyfile, char *old_psw, char *new_psw); /* obsolete, only used for RSA and DSA algorithms */
int ChangePrivateKeyPasswordEx(char *pse_path, char *pse_pass, char *key_path, char *old_pass, char *new_pass);


} //END namespace MESPRO_API

#endif //USE_MESPRO

#endif // __CRYPT_API_H__
