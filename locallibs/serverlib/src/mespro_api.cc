#include <string>
#include <map>
#include <set>
#include <stdio.h>
#include <dlfcn.h>
#include <iostream>
#include "tcl_utils.h"
#ifdef USE_MESPRO
#include "mespro.h"
#endif //USE_MESPRO

#define NICKNAME "DJEK"
#include "serverlib/test.h"
#include <serverlib/slogger.h>

#ifdef USE_MESPRO

std::string GetAlgo( const std::string& algo );

typedef std::map<std::string,void*> MapStrVoid;

                                     //lib_name, func_name, func addr
class WrapperMesProFuncs: public std::map<std::string,MapStrVoid> {
private:
  std::string _lib_name = "";
  const std::string NAME_LIB_FILE = "libmesprox.so";
  const std::string NAME_LIB_FILE_V1 = "libmesprox_1.so";
  std::string libsPath;
  std::string TCLLibPath;
  char *error;
  std::set<std::string> initLibs;
private:
  MapStrVoid handles;
public:
  virtual ~WrapperMesProFuncs() {
    for ( const auto& h : handles ) {
      dlclose(h.second);
    }
    LogError(STDLOG) << "WrapperMesProFuncs destroy";
  }
  void setLibNameFromAlgo( const std::string& algo ) {
    if ( algo.empty() /*||
         std::string(MP_KEY_ALG_NAME_ECR3410) == GetAlgo( algo )*/ )
      setLib( NAME_LIB_FILE_V1 );
    else
      setLib( NAME_LIB_FILE );
  }
  void setLib( const std::string& lib_name ) {
    if ( _lib_name == lib_name ) return;
    if ( TCLLibPath.empty() ) {
      TCLLibPath = readStringFromTcl( "MESPRO_LIBS_PATH", "./crypt/libs" ) + "/";
    }
    libsPath = TCLLibPath + lib_name;
    LogTrace(TRACE5) << "set mespro lib " << libsPath << lib_name;
    MapStrVoid::const_iterator ih = handles.find( lib_name );
    if ( handles.end() == ih ) {
      void *_handle = dlopen((libsPath + "/" + lib_name).c_str(),RTLD_LAZY|RTLD_LOCAL); //???
      if ( !_handle ) {
        LogError(STDLOG) << "Error load lib " << lib_name << " error " << dlerror();
        return;
      }
      dlerror(); /* Clear any existing error */
      ih = handles.emplace( lib_name, _handle ).first;
    }
    _lib_name = ih->first;
  }
  bool isMespro_V1() {
    return ( NAME_LIB_FILE_V1 == _lib_name );
  }
  bool isInit( ) {
    return ( initLibs.end() != initLibs.find( _lib_name ) );
  }
  void setInit( bool pr_init ) {
    if ( pr_init )
      initLibs.emplace( _lib_name );
    else
      initLibs.erase( _lib_name );
    LogTrace(TRACE5) << _lib_name << " set init " << pr_init;
  }
  const std::string& getLibPath() {
    return libsPath;
  }
  void *get( const std::string& func_name ) {
    if ( _lib_name.empty() ) {
      LogError(STDLOG) << "Error lib not defined, func" << func_name;
      return nullptr;
    }
    void *_handle =  handles[ _lib_name ];
    void *f;
    WrapperMesProFuncs::iterator libFuncs = emplace( _lib_name, MapStrVoid() ).first;
    MapStrVoid::const_iterator p = libFuncs->second.find( func_name );
    if ( libFuncs->second.end() == p ) {
      *(void **) (&f) = dlsym(_handle, func_name.c_str());
      if ((error = dlerror()) != nullptr)  {
        LogError(STDLOG) << "Error find lib func " << func_name << " error " << error;
      }
      p = libFuncs->second.emplace( func_name, f ).first;
    }
    return p->second;
  }
};

static WrapperMesProFuncs funcs;

bool isMespro_V1()
{
  return funcs.isMespro_V1();
}

namespace MESPRO_API {

std::string GetAlgo( const std::string& algo )
{
  return algo.empty()?
           funcs.isMespro_V1()?
              MP_KEY_ALG_NAME_ECR3410:
              MP_KEY_ALG_NAME_GOST_12_256:
           algo;
}

void setLibName( const std::string& lib_name )
{
  funcs.setLib( lib_name );
}

void setLibNameFromAlgo( const std::string& algo )
{
  funcs.setLibNameFromAlgo( algo );
}

bool isInitLib( ) {
  return funcs.isInit();
}

void setInitLib( bool pr_init ) {
  funcs.setInit( pr_init );
}

std::string getPSEPath( )
{
  return readStringFromTcl("MESPRO_PSE_PATH","./crypt");
}

const std::string& getLibPath( )
{
  return funcs.getLibPath();
}

#define __DECLARE__INT_FUNC_AGR_CHAR__(func_name_, arg_char) \
  int (*fptr)(char *);\
  *(void **)(&fptr)= funcs.get( func_name_ );\
  return (*fptr)(arg_char);

#define __DECLARE__INT_FUNC_AGR_INT__(func_name_, arg_int) \
  int (*fptr)(int);\
  *(void **)(&fptr)= funcs.get( func_name_ );\
  return (*fptr)(arg_int);

#define __DECLARE__INT_FUNC_AGR_VOID__(func_name_) \
  int (*fptr)();\
  *(void **)(&fptr)= funcs.get( func_name_ );\
  return (*fptr)();

#define __DECLARE__INT_FUNC_AGRS_CHAR_INT__(func_name_, arg_char, arg_int) \
  int (*fptr)(char *,int);\
  *(void **)(&fptr)= funcs.get( func_name_ );\
  return (*fptr)(arg_char, arg_int);

#define __DECLARE__VOID_FUNC_ARG_PVOID__(func_name_, arg_pvoid) \
  void (*fptr)(void*);\
  *(void **)(&fptr)= funcs.get( func_name_ );\
  return (*fptr)(arg_pvoid);

/* Initialization and Completion Functions */
int PKCS7Init(char *pse_path, int reserved)
{
  __DECLARE__INT_FUNC_AGRS_CHAR_INT__(__func__,pse_path,reserved);
}

int PKCS7Final()
{
  __DECLARE__INT_FUNC_AGR_VOID__(__func__);
}

int SetInputCodePage(int codepage)
{
  __DECLARE__INT_FUNC_AGR_INT__(__func__,codepage);
}

void SetRandInitCallbackFun(void (*Func)())
{
  //!!!prior version MPFUN void MPAPI SetRandInitCallbackFun(void *Func);
  void (*fptr)(void (*Func)());
   *(void **)(&fptr)= funcs.get( __func__ );
  (*fptr)( Func );

}

void SetRandInitCallbackFun(int (*Func)(int, int, int, char *))
{
  void (*fptr)(int (*Func)(int, int, int, char *));
  *(void **)(&fptr)= funcs.get( __func__ );
  (*fptr)( Func );
}

int SetNewKeysAlgorithm(char *algor)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,algor);
}

int SetCertificateRequestFlags(unsigned long flags)
{
  int (*fptr)(unsigned long);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(flags);
}

int SetCountry(char *Country)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,Country);
}

int SetStateOrProvince(char *StateOrProvince)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,StateOrProvince);
}

int SetLocality(char *Locality)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,Locality);
}

int SetOrganization(char *Organization)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,Organization);
}

int SetOrganizationalUnit(char *OrganizationalUnit)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,OrganizationalUnit);
}

int SetTitle(char *Title)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,Title);
}

int SetCommonName(char *CommonName)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,CommonName);
}

int SetEmailAddress(char *EmailAddress)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,EmailAddress);
}

int IntPSE_Generation(const std::string& func_name, char *pse_path, int reserv1, char *reserv2, unsigned long flags)
{
  int (*fptr)(char *,int,char *,unsigned long);
  *(void **)(&fptr)= funcs.get( func_name );
  return (*fptr)(pse_path,reserv1,reserv2,flags);
}

int PSE31_Generation(char *pse_path, int reserv1, char *reserv2, unsigned long flags)
{
  return IntPSE_Generation( __func__,pse_path,reserv1,reserv2,flags );
}

int PSE_Generation(char *pse_path, int reserv, char *passwd, unsigned long flags)
{
  return funcs.isMespro_V1()?
          IntPSE_Generation( "PSE31_Generation",pse_path,reserv,passwd,flags ):
          IntPSE_Generation( __func__,pse_path,reserv,passwd,flags );
}

int IntPSE_Copy(const std::string& func_name, char *pse_path, char *passwd, char *new_pse_path, unsigned long flags)
{
  int (*fptr)(char *,char *,char *,unsigned long);
  *(void **)(&fptr)= funcs.get( func_name );
  return (*fptr)(pse_path,passwd,new_pse_path,flags);
}

int Copy_PSE31(char *pse_path, char *reserv, char *new_pse_path, unsigned long flags)
{
  return IntPSE_Copy(__func__,pse_path,reserv,new_pse_path,flags);
}

int PSE_Copy(char *pse_path, char *passwd, char *new_pse_path, unsigned long flags)
{
  return funcs.isMespro_V1()?
          IntPSE_Copy( "Copy_PSE31",pse_path,passwd,new_pse_path,flags ):
          IntPSE_Copy( __func__,pse_path,passwd,new_pse_path,flags );
}

int IntPSE_Erase(const std::string& func_name,char *pse_path)
{
  int (*fptr)(char *);
  *(void **)(&fptr)= funcs.get( func_name );
  return (*fptr)(pse_path);
}

int Erase_PSE31(char *pse_path)
{
  return IntPSE_Erase(__func__,pse_path);
}

int PSE_Erase(char *pse_path)
{
  return funcs.isMespro_V1()?
          IntPSE_Erase( "Erase_PSE31",pse_path ):
          IntPSE_Erase( __func__,pse_path );
}

int NewKeysGenerationEx(char *pse_path, char *pse_pass, char *keyfile,char *key_pass, char *reqfile)
{
  int (*fptr)(char *,char *,char *,char *,char *);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(pse_path,pse_pass,keyfile,key_pass,reqfile);
}

int NewKeysGeneration(char *keyfile, char *password, char *reqfile)
{
  int (*fptr)(char *,char *,char *);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(keyfile,password,reqfile);
}

void ClearBuffer(unsigned char *buf)
{
  void (*fptr)(unsigned char *);
  *(void **)(&fptr)= funcs.get( __func__ );
  (*fptr)(buf);
}

int SetInputFormat(int form)
{
  __DECLARE__INT_FUNC_AGR_INT__(__func__,form);
}

int SetOutputFormat(int form)
{
  __DECLARE__INT_FUNC_AGR_INT__(__func__,form);
}

int SetCipherAlgorithm(char *algor)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,algor);
}

int AddCA(char *CAfile)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,CAfile);
}

int AddCAFromBuffer(char *buf, int ln)
{
  __DECLARE__INT_FUNC_AGRS_CHAR_INT__(__func__,buf,ln);
}

int AddCAs(char *CAdir)
{
  __DECLARE__INT_FUNC_AGR_CHAR__(__func__,CAdir);
}

int ClearCAs(void)
{
  __DECLARE__INT_FUNC_AGR_VOID__(__func__);
}

int AddPSEPrivateKeyFromBufferEx(char *pse_path, char *pse_pass, char *buf, int len, char *key_pass)
{
  int (*fptr)(char*, char*, char*, int, char*);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(pse_path,pse_pass,buf,len,key_pass);
}

int AddPrivateKeyFromBuffer(char *buf, int ln, char *password)
{
  int (*fptr)(char*, int, char*);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(buf,ln,password);
}

char* GetCertPublicKeyAlgorithmBuffer(char *buf, int len)
{
  char* (*fptr)(char*, int);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(buf,len);
}

int AddCertificateFromBuffer(char *buf, int ln)
{
  __DECLARE__INT_FUNC_AGRS_CHAR_INT__(__func__,buf,ln);
}

int CertAndRequestMatchBuffer(char *certbuf, int certln, char *reqbuf, int reqln)
{
  int (*fptr)(char*, int, char*, int);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(certbuf,certln,reqbuf,reqln);
}

int GetCertificateInfoBufferEx(char *buf, int ln, CERTIFICATE_INFO *info)
{
  int (*fptr)(char*, int, CERTIFICATE_INFO *);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(buf,ln,info);
}

void FreeCertificateInfo(CERTIFICATE_INFO *info)
{
  void (*fptr)(CERTIFICATE_INFO *);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(info);
}

int DecryptBuffer(void *in_buf, int in_len,void **out_buf, int *out_len)
{
  int (*fptr)(void*, int, void**, int*);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(in_buf,in_len,out_buf,out_len);
}

int ClearCertificates(void)
{
  __DECLARE__INT_FUNC_AGR_VOID__(__func__);
}

void FreeBuffer(void *ptr)
{
  __DECLARE__VOID_FUNC_ARG_PVOID__(__func__,ptr)
}

void* GetCipherCTX(void)
{
  void* (*fptr)();
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)();
}

int AddRecipient(void *ctx, int type, void *param1, void *param2)
{
  int (*fptr)(void *, int, void*, void*);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(ctx,type,param1,param2);
}

int SetCipherEncapsulatedContentType(void *ctx, int type)
{
  int (*fptr)(void *, int);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(ctx,type);
}

void FreeCipherCTX(void *ctx)
{
  __DECLARE__VOID_FUNC_ARG_PVOID__(__func__,ctx);
}

int EncryptBuffer(void *ctx, void *in_buf, int in_len, void **out_buf, int *out_len)
{
  int (*fptr)(void *, void *, int, void **, int *);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(ctx,in_buf,in_len,out_buf,out_len);
}

int SetKeysLength(int bits)
{
  __DECLARE__INT_FUNC_AGR_INT__(__func__,bits);
}

int ChangePrivateKeyPassword(char *keyfile, char *old_psw, char *new_psw)
{
  int (*fptr)(char *, char *, char *);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(keyfile,old_psw,new_psw);
}

int ChangePrivateKeyPasswordEx(char *pse_path, char *pse_pass, char *key_path, char *old_pass, char *new_pass)
{
  int (*fptr)(char *, char *, char *, char *, char *);
  *(void **)(&fptr)= funcs.get( __func__ );
  return (*fptr)(pse_path,pse_pass,key_path,old_pass,new_pass);
}

} //END namespace MESPRO_API

#endif //USE_MESPRO

