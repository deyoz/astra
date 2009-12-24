#ifndef __CRYPT_H__
#define __CRYPT_H__

#include "tclmon/mespro_crypt.h"

#include <libxml/tree.h>
#include <string>
#include "jxtlib/JxtInterface.h"



void getMesProParams(const char *head, int hlen, int *error, MPCryptParams &params);
int form_crypt_error(char *res, char *head, int hlen, int error);

class TCrypt {
	public:
		bool server_sign;
		bool client_sign;
		std::string algo_sign;
		std::string algo_cipher;
		int inputformat;
		int outputformat;
		std::string ca_cert;
		std::string server_cert;
		std::string client_cert;
		void Clear() {
			server_sign = false;
			client_sign = false;
			algo_sign.clear();
			algo_cipher.clear();
			inputformat = 1; //FORMAT_ASN1 = 1 - по умолчанию
			outputformat = 1;//FORMAT_ASN1 = 1 - по умолчанию
			ca_cert.clear();
			server_cert.clear();
			client_cert.clear();
		}
		TCrypt() {
			Clear();
	  }
	  void Init( const std::string &desk );
};

void IntGetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
void IntPutRequestCertificate(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
void IntRequestCertificateData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

class CryptInterface : public JxtInterface
{
public:
  CryptInterface() : JxtInterface("","Crypt")
  {
     Handler *evHandle;
     evHandle=JxtHandler<CryptInterface>::CreateHandler(&CryptInterface::GetCertificates);
     AddEvent("GetCertificates",evHandle);
     evHandle=JxtHandler<CryptInterface>::CreateHandler(&CryptInterface::RequestCertificateData);
     AddEvent("RequestCertificateData",evHandle);
     evHandle=JxtHandler<CryptInterface>::CreateHandler(&CryptInterface::PutRequestCertificate);
     AddEvent("PutRequestCertificate",evHandle);
     evHandle=JxtHandler<CryptInterface>::CreateHandler(&CryptInterface::GetRequestsCertificate);
     AddEvent("GetRequestsCertificate",evHandle);
     evHandle=JxtHandler<CryptInterface>::CreateHandler(&CryptInterface::SetCertificates);
     AddEvent("SetCertificates",evHandle);
     evHandle=JxtHandler<CryptInterface>::CreateHandler(&CryptInterface::CertificatesInfo);
     AddEvent("CertificatesInfo",evHandle);
  };

  void GetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void RequestCertificateData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void PutRequestCertificate(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetRequestsCertificate(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void SetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void CertificatesInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};



#endif // __CRYPT_H__
