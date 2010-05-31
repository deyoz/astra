#include <string>
#include "tclmon/mespro_crypt.h"
#include "tclmon/tclmon.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/sirena_queue.h"
#include "serverlib/string_cast.h"
#include "crypt.h"
#include "oralib.h"
#include "basic.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "exceptions.h"
#include "xml_unit.h"
#ifdef USE_MESPRO
#include "mespro.h"
#endif
#include "stl_utils.h"



#define NICKNAME "DJEK"
#include "serverlib/test.h"

const int MIN_DAYS_CERT_WARRNING = 10;

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;


int form_crypt_error(char *res, char *head, int hlen, int error)
{
  int newlen;
  char *utf8txt=NULL;
  int utf8len=0;
  int byte=getGrp3ParamsByte();
  if(head[0]==2) {
  	ProgError( STDLOG, "form_crypt_error: head[0]==2!!!" );
    byte=getGrp2ParamsByte();
  }

  std::string msg_error;

  switch(error)
  {
    case 0: /* no error code is set */
      error=UNKNOWN_ERR;
      ProgTrace(TRACE1,"form_crypt_error: UNKNOWN_ERR");
      msg_error = "no error -> UNKNOWN_ERR!";
      break;
    case UNKNOWN_KEY:
    	msg_error = "Шифрованное соединение: нужен ключ. Обратитесь к администратору";
      break;
    case EXPIRED_KEY:
    	msg_error = "Шифрованное соединение: сертификат клиента просрочен. Обратитесь к администратору";
      break;
    case WRONG_OUR_KEY:
    case WRONG_KEY:
    	msg_error = "Шифрованное соединение: ключ неверен. Обратитесь к администратору";
      break;
/*    case WRONG_SYM_KEY:
      strcpy(err_text,"WRONG_SYM_KEY");
      break;
    case UNKNOWN_SYM_KEY:
      strcpy(err_text,"UNKNOWN_SYM_KEY");
      break;
    case CRYPT_ERR_READ_RSA_KEY:
      strcpy(err_text,"CRYPT_ERR_READ_RSA_KEY");
      break;*/
    case WRONG_TERM_CRYPT_MODE:
    	msg_error = "Шифрованное соединение: ошибка режима шифрования. Повторите запрос";
    	break;
    case UNKNOWN_CLIENT_CERTIFICATE:
    	msg_error = "Шифрованное соединение: сертификат клиента не найден. Обратитесь к администратору";
    	break;
    case UNKNOWN_ERR:
    case CRYPT_ALLOC_ERR:
    default:
    	msg_error = "Ошибка программы. Обратитесь к администратору";
  }

  ProgTrace(TRACE1,"Crypting error occured: (%i) '%s'",error,msg_error.c_str());
  (head[byte])|=MSG_SYS_ERROR;
  memcpy(res, head, hlen);
  utf8len=CP866toUTF8((unsigned char **)&utf8txt,(unsigned char *)msg_error.c_str());
  if(utf8len<=0)
    utf8txt=(char *)"System error!";
  switch(head[0]) {
  	case 2:
      msg_error=std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                         "<sirena>"
                           "<answer>"
                             "<error code='-1' crypt_error='")+
                             HelpCpp::string_cast(error)+"'>"+
                             HelpCpp::string_cast(error)+" "+utf8txt+"</error>"
                           "</answer>"
                          "</sirena>";
      newlen =msg_error.size();
      memcpy(res+hlen,msg_error.c_str(),newlen);
      (res[byte])&=~MSG_TEXT;
  		break;
  	case 3:
  		msg_error=std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  			                    "<term>"
  			                     "<answer>"
  			                     "<clear_certificates/>"
  			                      "<command>"
  			                       "<error>")+utf8txt+"</error>"+
  			                      "</command>"+
  			                     "</answer>"+
  			                    "</term>";
      newlen =msg_error.size();
      memcpy(res+hlen,msg_error.c_str(),newlen);
      (res[byte])&=~MSG_TEXT;
  		break;
  	default:
      (res[byte])|=MSG_TEXT;
      int err_len=sprintf(res+hlen,"%i ",error);
      newlen = utf8len+err_len;
      memcpy(res+hlen+err_len,utf8txt,utf8len);
  };
  if(utf8len>0) // memory was allocated
    free(utf8txt);
  levC_adjust_crypt_header(res,newlen,0,0);

  (res[byte])&=~MSG_BINARY;
  (res[byte])&=~MSG_CAN_COMPRESS;
  (res[byte])&=~MSG_COMPRESSED;

  ProgTrace(TRACE1,"MSG_TEXT=%i",(res[byte])&MSG_TEXT);
  ProgTrace(TRACE1,"MSG_BINARY=%i",(res[byte])&MSG_BINARY);
  ProgTrace(TRACE1,"MSG_COMPRESSED=%i",(res[byte])&MSG_COMPRESSED);
  ProgTrace(TRACE1,"MSG_ENCRYPTED=%i",(res[byte])&MSG_ENCRYPTED);
  ProgTrace(TRACE1,"MSG_CAN_COMPRESS=%i",(res[byte])&MSG_CAN_COMPRESS);
  ProgTrace(TRACE1,"MSG_SPECIAL_PROC=%i",(res[byte])&MSG_SPECIAL_PROC);
  ProgTrace(TRACE1,"MSG_PUB_CRYPT=%i",(res[byte])&MSG_PUB_CRYPT);
  ProgTrace(TRACE1,"MSG_MESPRO_CRYPT=%i",(res[byte+1])&MSG_MESPRO_CRYPT);
  ProgTrace(TRACE1,"MSG_SYS_ERROR=%i",(res[byte])&MSG_SYS_ERROR);

  ProgTrace(TRACE5,"res(utf8)='%.*s'",newlen,res+hlen);

  return newlen+hlen;
}

bool GetClientCertificate( TQuery *Qry, int grp_id, const std::string &desk, std::string &certificate )
{
	certificate.clear();
	Qry->Clear();
  Qry->SQLText =
    "SELECT desk, certificate, pr_denial, first_date, last_date, SYSDATE now FROM crypt_term_cert "
    " WHERE desk_grp_id=:grp_id AND ( desk IS NULL OR desk=:desk ) "
    " ORDER BY desk ASC, pr_denial ASC, first_date ASC, id ASC";
  Qry->CreateVariable( "grp_id", otInteger, grp_id );
  Qry->CreateVariable( "desk", otString, desk );
  Qry->Execute();
  bool pr_exists=false;
  while ( !Qry->Eof ) {
   if ( Qry->FieldAsInteger( "pr_denial" ) == 0 && // разрешен
  	    Qry->FieldAsDateTime( "now" ) > Qry->FieldAsDateTime( "first_date" ) ) // начал выполняться
  	  pr_exists = true; // значит сертификат есть, но возможно он просрочен
   if ( Qry->FieldAsDateTime( "now" ) < Qry->FieldAsDateTime( "first_date" ) || // не начал выполняться
  	    Qry->FieldAsDateTime( "now" ) > Qry->FieldAsDateTime( "last_date" ) ) { // закончил выполняться
  	 Qry->Next();
  	 continue;
  }
  // сертификат актуален. Сортировка по пульту. а потом по группе + признак отмены
  if ( Qry->FieldAsInteger( "pr_denial" ) != 0 )
   	break;
   certificate = Qry->FieldAsString( "certificate" );
   break;
  }
  return pr_exists;
}

bool GetCryptGrp( TQuery *Qry, const std::string &desk, int &grp_id )
{
	Qry->Clear();
  Qry->SQLText =
    "SELECT pr_crypt,crypt_sets.desk_grp_id grp_id "
    "FROM desks,desk_grp,crypt_sets "
    "WHERE desks.code = UPPER(:desk) AND "
    "      desks.grp_id = desk_grp.grp_id AND "
    "      crypt_sets.desk_grp_id=desk_grp.grp_id AND "
    "      ( crypt_sets.desk IS NULL OR crypt_sets.desk=desks.code ) "
    "ORDER BY desk ASC ";
  Qry->CreateVariable( "desk", otString, desk );
  Qry->Execute();
  if ( !Qry->Eof )
  	 grp_id = Qry->FieldAsInteger( "grp_id" );
  return ( !Qry->Eof && Qry->FieldAsInteger( "pr_crypt" ) != 0 ); //пульт не может работать в режиме шифрования, а пришло зашифрованное сообщение
}

void GetServerCertificate( TQuery *Qry, std::string &ca, std::string &pk, std::string &server )
{
	Qry->Clear();
  Qry->SQLText =
    "SELECT certificate,private_key,first_date,last_date,pr_ca FROM crypt_server "
    " WHERE pr_denial=0 AND system.UTCSYSDATE BETWEEN first_date AND last_date "
    " ORDER BY id DESC";
  Qry->Execute();
  while ( !Qry->Eof ) {
	  if ( Qry->FieldAsInteger("pr_ca") && ca.empty() )
  		ca = Qry->FieldAsString( "certificate" );
    if ( !Qry->FieldAsInteger("pr_ca") && pk.empty() ) {
  		pk = Qry->FieldAsString( "private_key" );
  		server = Qry->FieldAsString( "certificate" );
  	}
  	if ( !ca.empty() && !pk.empty() )
  	  break;
  	Qry->Next();
  }
  return;
}

#ifdef USE_MESPRO
void getMesProParams(const char *head, int hlen, int *error, MPCryptParams &params)
{
  params.CA.clear();
  params.PKey.clear();
  params.client_cert.clear();
  params.server_cert.clear();

  tst();


  if ( !((head)[getGrp3ParamsByte()+1]&MSG_MESPRO_CRYPT) ) { // пришло сообщение которое не зашифровано
  	ProgError( STDLOG, "getMesProParams: message is not crypted" );
  	return;
  }

  using namespace std;
  string desk = string(head+45,6);
  TQuery *Qry = new TQuery(&OraSession);
  int grp_id;
  try {
  	if ( !GetCryptGrp( Qry, desk, grp_id ) ) { //пульт не может работать в режиме шифрования, а пришло зашифрованное сообщение
      *error=WRONG_TERM_CRYPT_MODE;
      tst();
    	return;
    }
    GetServerCertificate( Qry, params.CA, params.PKey, params.server_cert );
    if ( params.PKey.empty() ) {
    	*error = UNKNOWN_KEY;
    	return;
    }
    if ( params.CA.empty() ) {
    	*error = UNKNOWN_CA_CERTIFICATE;
    	return;
    }
    if ( params.server_cert.empty() ) {
    	*error = UNKNOWN_SERVER_CERTIFICATE;
    	return;
    }

    bool pr_exists = GetClientCertificate( Qry, grp_id, desk, params.client_cert );
    if ( params.client_cert.empty() ) {
    	if ( pr_exists )
    		*error = EXPIRED_KEY;
    	else
 	  	  *error = UNKNOWN_CLIENT_CERTIFICATE;
  	  return;
    }
  }
  catch(...) {
  	delete Qry;
  	throw;
  };
  delete Qry;
}

#endif // USE_MESPRO


void TCrypt::Init( const std::string &desk )
{
	Clear();
  TQuery Qry(&OraSession);
  int grp_id;
  if ( !GetCryptGrp( &Qry, desk, grp_id ) )
  	return;
  ProgTrace( TRACE5, "grp_id=%d", grp_id );
  string pk;
  GetServerCertificate( &Qry, ca_cert, pk, server_cert );
  if ( ca_cert.empty() || server_cert.empty() )
  	throw Exception("ca or server certificate not found");

  bool pr_exists = GetClientCertificate( &Qry, grp_id, desk, client_cert );
  if ( client_cert.empty() ) {
  	if ( !pr_exists )
  	  AstraLocale::showProgError("MSG.MESSAGEPRO.CRYPT_CONNECT_CERT_NOT_FOUND.CALL_ADMIN");
  	else
  		AstraLocale::showProgError("MSG.MESSAGEPRO.CRYPT_CONNECT_CERT_OUTDATED.CALL_ADMIN");
  	throw UserException2();
  }
};


void IntGetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  // проверка на то, что это сертификат (возможно это запрос на сертификат!!!)
  TCrypt Crypt;
  Crypt.Init( TReqInfo::Instance()->desk.code );
  xmlNodePtr node = NewTextChild( resNode, "crypt" );
  if ( Crypt.ca_cert.empty() || Crypt.server_cert.empty() || Crypt.client_cert.empty() )
  	return;
  NewTextChild( node, "server_id", SERVER_ID() );
  NewTextChild( node, "server_sign", Crypt.server_sign );
 	NewTextChild( node, "client_sign", Crypt.client_sign );
  if ( !Crypt.algo_sign.empty() )
  	NewTextChild( node, "SignAlgo", Crypt.algo_sign );
  if ( !Crypt.algo_cipher.empty() )
  	NewTextChild( node, "CipherAlgo", Crypt.algo_cipher );
  if ( Crypt.inputformat != 1 )
  	NewTextChild( node, "InputFormat", Crypt.inputformat );
  if ( Crypt.outputformat != 1 )
  	NewTextChild( node, "OutputFormat", Crypt.outputformat );
  node = NewTextChild( node, "certificates" );
  NewTextChild( node, "ca", Crypt.ca_cert );
  NewTextChild( node, "server", Crypt.server_cert );
  NewTextChild( node, "client", Crypt.client_cert );
  NewTextChild( node, "MIN_DAYS_CERT_WARRNING", MIN_DAYS_CERT_WARRNING );
}

// это первый запрос с клиента или запрос после ошибки работы шифрования
void CryptInterface::GetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	IntGetCertificates(ctxt, reqNode, resNode);
}

void IntRequestCertificateData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT crypt_term_req.desk_grp_id grp_id, crypt_term_req.desk desk "
    " FROM crypt_term_req, desks, desk_grp"
    " WHERE desks.code = :desk AND "
    "       desks.grp_id = desk_grp.grp_id AND "
    "       crypt_term_req.desk_grp_id=desk_grp.grp_id AND "
    "      ( crypt_term_req.desk IS NULL OR crypt_term_req.desk=desks.code )"
    " ORDER BY desk ASC, grp_id ";
  Qry.CreateVariable( "desk", otString, TReqInfo::Instance()->desk.code );
  Qry.Execute();
  bool pr_grp = GetNode( "pr_grp", reqNode );

  while ( !Qry.Eof ) {
  	if ( Qry.FieldIsNULL( "desk" ) && pr_grp ||
  		  !Qry.FieldIsNULL( "desk" ) && !pr_grp )
  	  throw AstraLocale::UserException( "MSG.MESSAGEPRO.CERT_QRY_CREATED_EARLIER.NOT_PROCESSED_YET" );
  	Qry.Next();
  }
  Qry.Clear();
  Qry.SQLText = "SELECT system.UTCSYSDATE udate FROM dual";
  Qry.Execute();
  BASIC::TDateTime udate = Qry.FieldAsDateTime( "udate" );
  Qry.Clear();
  if ( pr_grp )
    Qry.SQLText =
      "SELECT country,state,crypt_req_data.city,organization,organizational_unit,title,"
      "       user_name,email,key_algo,keyslength "
      " FROM crypt_req_data, desks, desk_grp "
      " WHERE desks.code = :desk AND "
      "       desks.grp_id = desk_grp.grp_id AND "
      "       crypt_req_data.desk_grp_id=desk_grp.grp_id AND "
      "       crypt_req_data.desk IS NULL AND "
      "       crypt_req_data.pr_denial=0 "
      " ORDER BY desk ASC ";
  else
    Qry.SQLText =
      "SELECT country,state,crypt_req_data.city,organization,organizational_unit,title,"
      "       user_name,email,key_algo,keyslength "
      " FROM crypt_req_data "
      " WHERE desk = :desk AND "
      "       crypt_req_data.pr_denial=0 ";
  Qry.CreateVariable( "desk", otString, TReqInfo::Instance()->desk.code );
  Qry.Execute();
	xmlNodePtr node = NewTextChild( resNode, "RequestCertificateData" );
	NewTextChild( node, "server_id", SERVER_ID() );
	NewTextChild( node, "FileKey", "astra"+BASIC::DateTimeToStr( udate, "ddmmyyhhnn" ) );
	if ( Qry.Eof )
		throw UserException( "MSG.MESSAGEPRO.NO_DATA_FOR_CERT_QRY" );
  NewTextChild( node, "Country", Qry.FieldAsString( "country" ) );
	if ( !Qry.FieldIsNULL( "key_algo" ) )
	  NewTextChild( node, "Algo", Qry.FieldAsString( "key_algo" ) );
	if ( !Qry.FieldIsNULL( "keyslength" ) )
	  NewTextChild( node, "KeyLength", Qry.FieldAsString( "keyslength" ) );
  if ( !Qry.FieldIsNULL( "state" ) )
    NewTextChild( node, "StateOrProvince", Qry.FieldAsString( "state" ) );
  if ( !Qry.FieldIsNULL( "city" ) )
   	NewTextChild( node, "Localite", Qry.FieldAsString( "city" ) );
  if ( !Qry.FieldIsNULL( "organization" ) )
  	NewTextChild( node, "Organization", Qry.FieldAsString( "organization" ) );
  if ( !Qry.FieldIsNULL( "organizational_unit" ) )
   	NewTextChild( node, "OrganizationalUnit", Qry.FieldAsString( "organizational_unit" ) );
  if ( !Qry.FieldIsNULL( "title" ) )
   	NewTextChild( node, "Title", Qry.FieldAsString( "title" ) );
  if ( Qry.FieldIsNULL( "user_name" ) ) {
  	string str = "Астра";
  	if ( pr_grp )
  		str += "(Группа)";
   	NewTextChild( node, "CommonName", str + TReqInfo::Instance()->desk.code +
    		                                BASIC::DateTimeToStr( udate, "ddmmyyhhnn" )  );
  }
  if ( !Qry.FieldIsNULL( "email" ) )
  	NewTextChild( node, "EmailAddress", Qry.FieldAsString( "email" ) );
}

void CryptInterface::RequestCertificateData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	IntRequestCertificateData(ctxt, reqNode, resNode);
}

void IntPutRequestCertificate(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	bool pr_grp = GetNode( "pr_grp", reqNode );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "INSERT INTO crypt_term_req(id,desk_grp_id,desk,request) "
    " SELECT id__seq.nextval,grp_id,DECODE(:pr_grp,0,:desk,NULL),:request FROM desks "
    "  WHERE code=:desk";
  Qry.CreateVariable( "pr_grp", otInteger, pr_grp );
  Qry.CreateVariable( "desk", otString, TReqInfo::Instance()->desk.code );
  Qry.CreateVariable( "request", otString, NodeAsString( "request_certificate", reqNode ) );
  Qry.Execute();
}

void CryptInterface::PutRequestCertificate(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	IntPutRequestCertificate(ctxt, reqNode, resNode);
}

struct TSearchData {
	int id;
	string desk;
	string grp;
	string data;
	TDateTime first_date;
	TDateTime last_date;
	int pr_denial;
};

void CryptInterface::GetRequestsCertificate(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  bool pr_search = GetNode( "search", reqNode );
  TQuery Qry(&OraSession);
  if ( GetNode( "id", reqNode ) ) {
    Qry.SQLText =
      "SELECT id,desk,desk_grp_id,descr,city,airline,airp,request FROM crypt_term_req, desk_grp "
      " WHERE crypt_term_req.desk_grp_id = desk_grp.grp_id AND crypt_term_req.id=:id";
    Qry.CreateVariable( "id", otInteger, NodeAsInteger( "id", reqNode ) );
  }
  else {
    Qry.SQLText =
      "SELECT id,desk,desk_grp_id,descr,city,airline,airp,request FROM crypt_term_req, desk_grp "
      " WHERE crypt_term_req.desk_grp_id = desk_grp.grp_id";
  }
  Qry.Execute();
  if ( Qry.Eof )
  	throw AstraLocale::UserException( "MSG.MESSAGEPRO.NO_CERT_QRYS" );
  vector<TSearchData> reqs;
  string grp_name;
  xmlNodePtr desksNode = NULL, grpsNode = NULL, reqsNode = NULL;
  while ( !Qry.Eof ) {
  	TSearchData req_search;
    req_search.id = Qry.FieldAsInteger( "id" );
 	  if ( !Qry.FieldIsNULL( "desk" ) ) {
 	  	if ( pr_search && !desksNode )
 	  	  desksNode = NewTextChild( resNode, "desks" );
    	req_search.desk = Qry.FieldAsString( "desk" );
    }
    if ( Qry.FieldIsNULL( "desk" ) ) {
    	if ( pr_search && !grpsNode )
    	  grpsNode = NewTextChild( resNode, "grps" );
    	grp_name = Qry.FieldAsString( "descr" );
    	grp_name += string(" ") + Qry.FieldAsString( "city" );

    	if ( !Qry.FieldIsNULL( "airline" ) || !Qry.FieldIsNULL( "airp" ) ) {
    		grp_name += "(";
    		grp_name += Qry.FieldAsString( "airline" );
    		if ( !Qry.FieldIsNULL( "airline" ) && !Qry.FieldIsNULL( "airp" ) )
    			grp_name += ",";
    		grp_name += Qry.FieldAsString( "airp" );
    		grp_name += ")";
    	}
    	req_search.grp = grp_name;
    }
    req_search.data = Qry.FieldAsString( "request" );
   	reqs.push_back( req_search );
  	Qry.Next();
  }

  for( vector<TSearchData>::iterator i=reqs.begin(); i!=reqs.end(); i++ ) {
  	if ( pr_search ) {
  		xmlNodePtr dNode;
  		if ( !i->desk.empty() )
  			dNode = NewTextChild( desksNode, "desk", i->desk );
  		else
  			dNode = NewTextChild( grpsNode, "grp", i->grp );
  		SetProp( dNode, "id", i->id );
  	}
  	else {
  		if ( !reqsNode )
  			reqsNode = NewTextChild( resNode, "cert_requests" );
  		xmlNodePtr n = NewTextChild( reqsNode, "request" );
  		NewTextChild( n, "id", i->id );
  		NewTextChild( n, "desk", i->desk );
  		NewTextChild( n, "grp", i->grp );
  		NewTextChild( n, "data", i->data );
  	}
  }
}

struct TRequest {
 	int id;
  string cert;
};

BASIC::TDateTime ConvertCertificateDate( char *certificate_date )
{

	BASIC::TDateTime d;
	int res = BASIC::StrToDateTime( upperc(certificate_date).c_str(), "dd.mm.yyyy hh:nn:ss", d, true );
	if ( res == EOF )
		throw Exception( "Invalid Certificate date=%s", certificate_date );
	return d;
}

void CryptInterface::SetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	tst();
#ifdef USE_MESPRO
	xmlNodePtr node = GetNode( "certificates", reqNode );
	if ( !node )
		return;
	int err = PKCS7Init( 0, 0 );
	if ( err )
		throw Exception( "MessagePro: PKCS7Init error=%d", err );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT id, request FROM crypt_term_req";
  Qry.Execute();
  vector<TRequest> requests;
  while ( !Qry.Eof ) {
  	TRequest r;
  	r.id = Qry.FieldAsInteger( "id" );
  	r.cert = Qry.FieldAsString( "request" );
  	requests.push_back( r );
  	Qry.Next();
  }
  ProgTrace( TRACE5, "requests count=%d", requests.size() );
  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    " INSERT INTO crypt_term_cert(id,desk_grp_id,desk,certificate,first_date,last_date,pr_denial) "
    " SELECT id__seq.nextval,desk_grp_id,desk,:certificate,:first_date,:last_date,0 FROM crypt_term_req "
    "  WHERE id=:id; "
    " DELETE crypt_term_req WHERE id=:id; "
    "END;";
  Qry.DeclareVariable( "id", otInteger );
  Qry.DeclareVariable( "certificate", otString );
  Qry.DeclareVariable( "first_date", otDate );
  Qry.DeclareVariable( "last_date", otDate );
	string cert;
	try {
	  node = GetNode( "certificate", node );
  	while ( node ) {
  		cert = NodeAsString( node );
  		ProgTrace( TRACE5, "certificate=%s, requests count=%d", cert.c_str(), requests.size() );
  		for ( vector<TRequest>::iterator i=requests.begin(); i!=requests.end(); i++ ) {
  			err = CertAndRequestMatchBuffer( (char*)cert.data(), cert.size(), (char*)i->cert.data(), i->cert.size() );
  			if ( err ) {
  				CERTIFICATE_INFO info;
  				err = GetCertificateInfoBufferEx( (char*)cert.data(), cert.size(), &info );
  				if ( err )
  					throw Exception( "MessagePro: GetCertificateInfoBufferEx error=%d", err );
  				try {
            Qry.SetVariable( "id", i->id );
            Qry.SetVariable( "certificate", cert );
            Qry.SetVariable( "first_date", ConvertCertificateDate( info.NotBefore ) );
            Qry.SetVariable( "last_date", ConvertCertificateDate( info.NotAfter ) );
            Qry.Execute();
          }
          catch( ... ) {
          	FreeCertificateInfo( &info );
          	throw;
          }
          FreeCertificateInfo( &info );
          requests.erase( i );
          break;
  			}
  		}
  		node = node->next;
  	}
  }
  catch(...) {
  	PKCS7Final();
  	throw;
  }
  PKCS7Final();
#endif /*USE MESPRO*/
}


void CryptInterface::CertificatesInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
bool pr_search = GetNode( "search", reqNode );
  TQuery Qry(&OraSession);
  if ( GetNode( "id", reqNode ) ) {
    Qry.SQLText =
      "SELECT id,desk,desk_grp_id,descr,city,airline,airp,certificate, first_date, last_date, crypt_term_cert.pr_denial "
      " FROM crypt_term_cert, desk_grp "
      " WHERE crypt_term_cert.desk_grp_id = desk_grp.grp_id AND crypt_term_cert.pr_denial=0 AND crypt_term_cert.id=:id";
    Qry.CreateVariable( "id", otInteger, NodeAsInteger( "id", reqNode ) );
  }
  else {
    Qry.SQLText =
      "SELECT id,desk,desk_grp_id,descr,city,airline,airp,certificate, first_date, last_date, crypt_term_cert.pr_denial "
      "  FROM crypt_term_cert, desk_grp "
      " WHERE crypt_term_cert.desk_grp_id = desk_grp.grp_id AND crypt_term_cert.last_date>=system.UTCSYSDATE";
  }
  Qry.Execute();
  if ( Qry.Eof )
  	throw AstraLocale::UserException( "MSG.MESSAGEPRO.NO_CERTS" );
  vector<TSearchData> certs;
  string grp_name;
  xmlNodePtr desksNode = NULL, grpsNode = NULL, reqsNode = NULL;
  while ( !Qry.Eof ) {
  	TSearchData cert_search;
    cert_search.id = Qry.FieldAsInteger( "id" );
 	  if ( !Qry.FieldIsNULL( "desk" ) ) {
 	  	if ( pr_search && !desksNode )
 	  	  desksNode = NewTextChild( resNode, "desks" );
    	cert_search.desk = Qry.FieldAsString( "desk" );
    }
    if ( Qry.FieldIsNULL( "desk" ) ) {
    	if ( pr_search && !grpsNode )
    	  grpsNode = NewTextChild( resNode, "grps" );
    	grp_name = Qry.FieldAsString( "descr" );
    	grp_name += string(" ") + Qry.FieldAsString( "city" );

    	if ( !Qry.FieldIsNULL( "airline" ) || !Qry.FieldIsNULL( "airp" ) ) {
    		grp_name += "(";
    		grp_name += Qry.FieldAsString( "airline" );
    		if ( !Qry.FieldIsNULL( "airline" ) && !Qry.FieldIsNULL( "airp" ) )
    			grp_name += ",";
    		grp_name += Qry.FieldAsString( "airp" );
    		grp_name += ")";
    	}
    	cert_search.grp = grp_name;
    }
    cert_search.data = Qry.FieldAsString( "certificate" );
    cert_search.first_date = Qry.FieldAsDateTime( "first_date" );
    cert_search.last_date = Qry.FieldAsDateTime( "last_date" );
    cert_search.pr_denial = Qry.FieldAsInteger( "pr_denial" );
   	certs.push_back( cert_search );
  	Qry.Next();
  }

  for( vector<TSearchData>::iterator i=certs.begin(); i!=certs.end(); i++ ) {
  	if ( pr_search ) {
  		xmlNodePtr dNode;
  		if ( !i->desk.empty() )
  			dNode = NewTextChild( desksNode, "desk", i->desk );
  		else
  			dNode = NewTextChild( grpsNode, "grp", i->grp );
  		SetProp( dNode, "id", i->id );
  	}
  	else {
  		if ( !reqsNode )
  			reqsNode = NewTextChild( resNode, "certificates" );
  		xmlNodePtr n = NewTextChild( reqsNode, "request" );
  		NewTextChild( n, "id", i->id );
  		NewTextChild( n, "desk", i->desk );
  		NewTextChild( n, "grp", i->grp );
  		NewTextChild( n, "data", i->data );
  		NewTextChild( n, "first_date", DateTimeToStr( i->first_date, ServerFormatDateTimeAsString ) );
  		NewTextChild( n, "last_date", DateTimeToStr( i->last_date, ServerFormatDateTimeAsString ) );
  		NewTextChild( n, "pr_denial", i->pr_denial );
  	}
  }
}

