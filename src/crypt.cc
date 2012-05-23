#include <string>
#include <stdio.h>
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
#include "tclmon/tcl_utils.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

const int MIN_DAYS_CERT_WARRNING = 10;
const unsigned int PASSWORD_LENGTH = 16;

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

static time_t randt = time(NULL);


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

  std::string msg_error, msg_id;

  switch(error)
  {
    case 0: /* no error code is set */
      error=UNKNOWN_ERR;
      ProgTrace(TRACE1,"form_crypt_error: UNKNOWN_ERR");
      msg_error = "no error -> UNKNOWN_ERR!";
      break;
    case UNKNOWN_KEY:
    	msg_error = "Шифрованное соединение: нужен ключ. Обратитесь к администратору";
    	msg_id = "MSG.CRYPT.KEY_REQUIRED_REFER_ADMIN";
      break;
    case EXPIRED_KEY:
    	msg_error = "Шифрованное соединение: сертификат клиента просрочен. Обратитесь к администратору";
    	msg_id = "MSG.MESSAGEPRO.CRYPT_CONNECT_CERT_OUTDATED.CALL_ADMIN";
      break;
    case WRONG_OUR_KEY:
    case WRONG_KEY:
    	msg_error = "Шифрованное соединение: ключ неверен. Обратитесь к администратору";
    	msg_id = "MSG.CRYPT.INVALID_KEY_REFER_ADMIN";
      break;
    case WRONG_TERM_CRYPT_MODE:
    	msg_error = "Шифрованное соединение: ошибка режима шифрования. Повторите запрос";
    	msg_id = "MSG.MESSAGEPRO.CRYPT_MODE_ERR.REPEAT";
    	break;
    case UNKNOWN_CLIENT_CERTIFICATE:
    	msg_error = "Шифрованное соединение: сертификат клиента не найден. Обратитесь к администратору";
    	msg_id = "MSG.MESSAGEPRO.CRYPT_CONNECT_CERT_NOT_FOUND.CALL_ADMIN";
    	break;
    case UNKNOWN_ERR:
    case CRYPT_ALLOC_ERR:
    default:
    	msg_error = "Ошибка программы. Обратитесь к администратору";
    	msg_id = "PROGRAMM_ERROR_REFER_ADMIN";
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
  			                       "<error id=\"" + msg_id + "\">")+utf8txt+"</error>"+
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

#ifdef USE_MESPRO
void GetError( const string &func_name, int err )
{
	if ( err == 0 ) return;
	ProgTrace( TRACE5, "GetError: func_name=%s, err=%d", func_name.c_str(), err );
	AstraLocale::LexemaData lexema;
	switch( err ) {
		case 2: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_2"; break;
		case 3: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_3"; break;
		case 4: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_4"; break;
    case 5: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_5";	break;
		case 6: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_6"; break;
		case 7: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_7"; break;
		case 8: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_8"; break;
    case 9: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_9";	break;
		case 10: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_10"; break;
		case 11: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_11"; break;
		case 12: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_12"; break;
    case 13: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_13";	break;
		case 14: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_14"; break;
		case 15: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_15"; break;
		case 16: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_16"; break;
		case 17:
		case 103: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_103";	break;
    case 18: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_18"; break;
		case 19: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_19"; break;
		case 20: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_20"; break;
		case 21: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_21"; break;
    case 22: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_22"; break;
		case 23: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_23"; break;
		case 24: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_24"; break;
		case 104: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_104"; break;
    case 105: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_105"; break;
		case 106: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_106"; break;
		case 107: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_107"; break;
		case 108: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_108"; break;
    case 109: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_109";	break;
		case 112: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_112"; break;
		case 114: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_114"; break;
		case 115: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_115"; break;
    case 116: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_116";	break;
		case 117: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_117"; break;
		case 118: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_118"; break;
		case 119: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_119"; break;
    case 122: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_122";	break;
		case 123: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_123"; break;
		case 124: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_124"; break;
    case 125: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_125";	break;
		case 127: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_127"; break;
		case 128: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_128"; break;
		case 129: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_129"; break;
    case 132: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_132";	break;
		case 134: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_134"; break;
		case 135: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_135"; break;
    case 136: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_136";	break;
		case 138: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_138"; break;
    case 143: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_143"; break;
    case 144: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_144"; break;
    case 145: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_145"; break;
    case 147: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_147"; break;
    case 148: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_148"; break;
    case 149: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_149"; break;
    case 150: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_150"; break;
    case 153: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_153"; break;
    case 154: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_154"; break;
    case 155: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_155"; break;
    case 158: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_158"; break;
    case 159: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_159"; break;
    case 160: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_160"; break;
    case 161: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_161"; break;
    case 162: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_162"; break;
    case 163: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_163"; break;
    case 164: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_164"; break;
    case 165: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_165"; break;
    case 166: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_166"; break;
    case 167: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_167"; break;
    case 168: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_168"; break;
    case 169: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_169"; break;
    case 170: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_170"; break;
    case 171: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_171"; break;
    case 172: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_172"; break;
    case 173: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_173"; break;
    case 174: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_174"; break;
    case 175: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_175"; break;
    case 176: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_176"; break;
    case 177: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_177"; break;
    case 178: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_178"; break;
    case 179: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_179"; break;
    case 180: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_180"; break;
    case 181: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_181"; break;
    case 182: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_182"; break;
    case 183: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_183"; break;
    case 184: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_184"; break;
    case 185: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_185"; break;
    case 186: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_186"; break;
    case 187: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_187"; break;
    case 188: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_188"; break;
    case 189: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_189"; break;
    case 190: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_190"; break;
    case 191: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_191"; break;
    case 192: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_192"; break;
    case 193: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_193"; break;
    case 194: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_194"; break;
    case 195: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_195"; break;
    case 196: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_196"; break;
    case 197: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_197"; break;
    case 198: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_198"; break;
    case 199: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_199"; break;
    case 200: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_200"; break;
    case 201: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_201"; break;
    default: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_OTHER";
	}
  throw AstraLocale::UserException( "WRAP.MESSAGEPRO", AstraLocale::LParams()<<AstraLocale::LParam("func",func_name)<<AstraLocale::LParam("text",lexema)<<AstraLocale::LParam("err",err) );
}

static int init_rand_callback1(int c, int step, int from, char *userdata)
{
	char cc[64];
	sprintf(cc,"%c",c);
	return *cc;
}
#endif //USE_MESPRO

// определяет ошибку режима шифрования и возвращает признак шифрования для группы
// за основу определения шифрования по группе или пульту берем эту ф-цию
bool GetCryptGrp( TQuery *Qry, const std::string &desk, int &grp_id, bool &pr_grp )
{
	Qry->Clear();
  Qry->SQLText =
    "SELECT pr_crypt,crypt_sets.desk_grp_id grp_id, crypt_sets.desk "
    "FROM desks,desk_grp,crypt_sets "
    "WHERE desks.code = UPPER(:desk) AND "
    "      desks.grp_id = desk_grp.grp_id AND "
    "      crypt_sets.desk_grp_id=desk_grp.grp_id AND "
    "      ( crypt_sets.desk IS NULL OR crypt_sets.desk=desks.code ) "
    "ORDER BY desk ASC ";
  Qry->CreateVariable( "desk", otString, desk );
  Qry->Execute();
  if ( !Qry->Eof ) {
  	 grp_id = Qry->FieldAsInteger( "grp_id" );
  	 pr_grp = Qry->FieldIsNULL( "desk" );
  }
  return ( !Qry->Eof && Qry->FieldAsInteger( "pr_crypt" ) != 0 ); //пульт не может работать в режиме шифрования, а пришло зашифрованное сообщение
}

//сертификат выбираем по след. правилам:
//1. сортировка по пульту, доступу и времени начала действия сертификата
//2. pr_grp - определяет какой сертификат изпользовать (для пульта или групповой)
bool GetClientCertificate( TQuery *Qry, int grp_id, bool pr_grp, const std::string &desk, std::string &certificate, int &pkcs_id )
{
	pkcs_id = -1;
	certificate.clear();
	Qry->Clear();
  Qry->SQLText =
    "SELECT pkcs_id, desk, certificate, pr_denial, first_date, last_date, SYSDATE now FROM crypt_term_cert "
    " WHERE desk_grp_id=:grp_id AND ( desk IS NULL OR desk=:desk ) "
    " ORDER BY desk ASC, pr_denial ASC, first_date ASC, id ASC";
  Qry->CreateVariable( "grp_id", otInteger, grp_id );
  Qry->CreateVariable( "desk", otString, desk );
  Qry->Execute();
  bool pr_exists=false;
  while ( !Qry->Eof ) {
  	if ( !Qry->FieldIsNULL( "desk" ) && pr_grp || // если пультовой сертификат, а у нас описан групповой
  		   Qry->FieldIsNULL( "desk" ) && !pr_grp ) { // если групповой сертификат, а у нас описан пультовой
  		Qry->Next();
  	  continue;
  	}
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
    if ( !Qry->FieldIsNULL( "pkcs_id" ) )
    	pkcs_id = Qry->FieldAsInteger( "pkcs_id" );

    break;
  }
  if ( pkcs_id >= 0 )
    ProgTrace( TRACE5, "pkcs_id=%d", pkcs_id );
  ProgTrace( TRACE5, "pr_exists=%d", pr_exists );
  return pr_exists;
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

  if ( !((head)[getGrp3ParamsByte()+1]&MSG_MESPRO_CRYPT) ) { // пришло сообщение которое не зашифровано
  	ProgError( STDLOG, "getMesProParams: message is not crypted" );
  	return;
  }

  using namespace std;
  string desk = string(head+45,6);
  TQuery *Qry = new TQuery(&OraSession);
  int grp_id;
  bool pr_grp;
  try {
  	if ( !GetCryptGrp( Qry, desk, grp_id, pr_grp ) ) { //пульт не может работать в режиме шифрования, а пришло зашифрованное сообщение
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

    int pkcs_id;
    bool pr_exists = GetClientCertificate( Qry, grp_id, pr_grp, desk, params.client_cert, pkcs_id );
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
	#ifdef USE_MESPRO
  TQuery Qry(&OraSession);
  int grp_id;
  bool pr_grp;
  if ( !GetCryptGrp( &Qry, desk, grp_id, pr_grp ) )
  	return;
  ProgTrace( TRACE5, "grp_id=%d,pr_grp=%d", grp_id, pr_grp );
  string pk;
  GetServerCertificate( &Qry, ca_cert, pk, server_cert );
  if ( ca_cert.empty() || server_cert.empty() )
  	throw Exception("ca or server certificate not found");

  bool pr_exists = GetClientCertificate( &Qry, grp_id, pr_grp, desk, client_cert, pkcs_id );
  if ( client_cert.empty() ) {
  	if ( !pr_exists )
  	  AstraLocale::showProgError("MSG.MESSAGEPRO.CRYPT_CONNECT_CERT_NOT_FOUND.CALL_ADMIN");
  	else
  		AstraLocale::showProgError("MSG.MESSAGEPRO.CRYPT_CONNECT_CERT_OUTDATED.CALL_ADMIN");
  	throw UserException2();
  }
  #endif //USE_MESPRO
};


void IntGetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  // проверка на то, что это сертификат (возможно это запрос на сертификат?)
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
  //проверка на режим инициализации шифрования с клиента
  if ( Crypt.pkcs_id >= 0 ) {
    TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT keyname, certname, password, desk, desk_grp_id, send_count "
      " FROM crypt_file_params "
      " WHERE pkcs_id=:pkcs_id";
    Qry.CreateVariable( "pkcs_id", otInteger, Crypt.pkcs_id );
    Qry.Execute();
    if ( Qry.Eof ||
         GetNode( "getkeys", reqNode ) == NULL && Qry.FieldAsInteger( "send_count" ) != 0 )
      return;

    node = NewTextChild( node, "pkcs", Crypt.pkcs_id );
    if ( Qry.FieldIsNULL( "desk" ) )
      SetProp( node, "common_dir" );
    SetProp( node, "key_filename", Qry.FieldAsString( "keyname" ) );
    SetProp( node, "cert_filename", Qry.FieldAsString( "certname" ) );
    Qry.Clear();
    Qry.SQLText =
      "SELECT name, data from crypt_files WHERE pkcs_id=:pkcs_id";
    Qry.CreateVariable( "pkcs_id", otInteger, Crypt.pkcs_id );
    Qry.Execute();
    if ( !Qry.Eof )
    	node = NewTextChild( node, "files" );
    xmlNodePtr fnode;
    void *data = NULL;
    int len = 0;
    string hexstr;
    try {
      while ( !Qry.Eof ) {
      	len = Qry.GetSizeLongField( "data" );
      	if ( data == NULL )
          data = malloc( len );
        else
          data = realloc( data, len );
        if ( data == NULL )
          throw Exception( "Ошибка программы" );
        Qry.FieldAsLong( "data", data );
        StringToHex( string((char*)data, len), hexstr );
    	  fnode = NewTextChild( node, "file", hexstr.c_str() );
    	  SetProp( fnode, "filename", Qry.FieldAsString( "name" ) );
  	    Qry.Next();
      }
    }
    catch( ... ) {
    	if ( data )
    	  free( data );
  	  throw;
    }
	  if ( data )
  	  free( data );
  }
}

// это первый запрос с клиента или запрос после ошибки работы шифрования
void CryptInterface::GetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	IntGetCertificates(ctxt, reqNode, resNode);
}

struct TCertRequest {
	string FileKey;
	string Country;
	string Algo;
	int KeyLength;
	string StateOrProvince;
	string Localite;
	string Organization;
	string OrganizationalUnit;
	string Title;
	string CommonName;
	string EmailAddress;
};

void GetCertRequestInfo( const string &desk, bool pr_grp, TCertRequest &req )
{
	ProgTrace( TRACE5, "GetCertRequestInfo, desk=%s, pr_grp=%d", desk.c_str(), pr_grp );
	TQuery Qry(&OraSession);
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
  Qry.CreateVariable( "desk", otString, desk );
  Qry.Execute();
	if ( Qry.Eof )
		throw AstraLocale::UserException( "MSG.MESSAGEPRO.NO_DATA_FOR_CERT_QRY" );
	req.FileKey = "astra"+BASIC::DateTimeToStr( udate, "ddmmyyhhnn" );
	req.Country =	Qry.FieldAsString( "country" );
	req.Algo = Qry.FieldAsString( "key_algo" );
	if ( !Qry.FieldIsNULL( "keyslength" ) )
	  req.KeyLength = Qry.FieldAsInteger( "keyslength" );
	else
		req.KeyLength = 0;
	req.StateOrProvince = Qry.FieldAsString( "state" );
	req.Localite = Qry.FieldAsString( "city" );
	req.Organization = Qry.FieldAsString( "organization" );
	req.OrganizationalUnit = Qry.FieldAsString( "organizational_unit" );
	req.Title = Qry.FieldAsString( "title" );
	req.CommonName = Qry.FieldAsString( "user_name" );
  if ( req.CommonName.empty() ) {
  	req.CommonName = "Astra";
  	if ( pr_grp )
  		req.CommonName += "(Group)";
   	req.CommonName += desk;
   	req.CommonName += BASIC::DateTimeToStr( udate, "ddmmyyhhnn" );
  }
  req.EmailAddress = Qry.FieldAsString( "email" );
}

void ValidateCertificateRequest( const string &desk, bool pr_grp )
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
  Qry.CreateVariable( "desk", otString, desk );
  Qry.Execute();

  while ( !Qry.Eof ) {
  	if ( Qry.FieldIsNULL( "desk" ) && pr_grp ||
  		  !Qry.FieldIsNULL( "desk" ) && !pr_grp )
  	  throw AstraLocale::UserException( "MSG.MESSAGEPRO.CERT_QRY_CREATED_EARLIER.NOT_PROCESSED_YET" );
  	Qry.Next();
  }
}

// запрос может быть подписан и сертификат залит в БД, но пока это не отправлено на клиент - выполнения заново цикла откладывается!
void ValidatePKCSData( const string &desk, bool pr_grp )
{
  tst();
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT crypt_file_params.desk_grp_id grp_id, crypt_file_params.desk desk "
    " FROM crypt_file_params, desks, desk_grp"
    " WHERE send_count = 0 AND "
    "       desks.code = :desk AND "
    "       desks.grp_id = desk_grp.grp_id AND "
    "       crypt_file_params.desk_grp_id=desk_grp.grp_id AND "
    "      ( crypt_file_params.desk IS NULL OR crypt_file_params.desk=desks.code )"
    " ORDER BY desk ASC, grp_id ";
  Qry.CreateVariable( "desk", otString, desk );
  Qry.Execute();
  while ( !Qry.Eof ) {
  	if ( Qry.FieldIsNULL( "desk" ) && pr_grp ||
  		  !Qry.FieldIsNULL( "desk" ) && !pr_grp )
  	  throw AstraLocale::UserException( "MSG.MESSAGEPRO.CERT_CREATED_EARLIER.NOT_PROCESSED_YET" );
  	Qry.Next();
  }
}

void IntRequestCertificateData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

  bool pr_grp = GetNode( "pr_grp", reqNode );
  ProgTrace( TRACE5, "IntRequestCertificateData: pr_grp=%d", pr_grp );
  ValidateCertificateRequest( TReqInfo::Instance()->desk.code, pr_grp );

  TCertRequest req;
  GetCertRequestInfo( TReqInfo::Instance()->desk.code, pr_grp, req );
	xmlNodePtr node = NewTextChild( resNode, "RequestCertificateData" );
	NewTextChild( node, "server_id", SERVER_ID() );
	NewTextChild( node, "FileKey", req.FileKey );
  NewTextChild( node, "Country", req.Country );
	if ( !req.Algo.empty() )
	  NewTextChild( node, "Algo", req.Algo );
	if ( req.KeyLength > 0 )
	  NewTextChild( node, "KeyLength", req.KeyLength );
  if ( !req.StateOrProvince.empty() )
    NewTextChild( node, "StateOrProvince", req.StateOrProvince );
  if ( !req.Localite.empty() )
   	NewTextChild( node, "Localite", req.Localite );
  if ( !req.Organization.empty() )
  	NewTextChild( node, "Organization", req.Organization );
  if ( !req.OrganizationalUnit.empty() )
   	NewTextChild( node, "OrganizationalUnit", req.OrganizationalUnit );
  if ( !req.Title.empty() )
   	NewTextChild( node, "Title", req.Title );
 	NewTextChild( node, "CommonName", req.CommonName );
  if ( !req.EmailAddress.empty() )
  	NewTextChild( node, "EmailAddress", req.EmailAddress );
}

void CryptInterface::RequestCertificateData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	IntRequestCertificateData(ctxt, reqNode, resNode);
}

void IntPutRequestCertificate( const string &request, const string &desk, bool pr_grp, int pkcs_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "INSERT INTO crypt_term_req(id,desk_grp_id,desk,request,pkcs_id) "
    " SELECT id__seq.nextval,grp_id,DECODE(:pr_grp,0,:desk,NULL),:request, :pkcs_id FROM desks "
    "  WHERE code=:desk";
  Qry.CreateVariable( "pr_grp", otInteger, pr_grp );
  Qry.CreateVariable( "desk", otString, desk );
  Qry.CreateVariable( "request", otString, request );
  if ( pkcs_id != NoExists )
    Qry.CreateVariable( "pkcs_id", otInteger, pkcs_id );
  else
  	Qry.CreateVariable( "pkcs_id", otInteger, FNull );
  Qry.Execute();
}

void CryptInterface::PutRequestCertificate(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	bool pr_grp = GetNode( "pr_grp", reqNode );
	string request = NodeAsString( "request_certificate", reqNode );
	IntPutRequestCertificate( request, TReqInfo::Instance()->desk.code, pr_grp, NoExists );
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

#ifdef USE_MESPRO

struct TPSEFile {
	string filename;
	string data;
};

struct TPKCS {
	string key_filename;
	string password;
	string cert_filename;
	vector<TPSEFile> pse_files;
};

void DeletePSE( const string &PSEpath, const string &file_key, const string &file_req )
{
  if ( !file_key.empty() )
    unlink( file_key.c_str() );
  if ( !file_req.empty() )
    unlink( file_req.c_str() );
	Erase_PSE31( (char*)PSEpath.c_str() );
	remove( PSEpath.c_str() );
}

void readPSEFile( const string &filename, const string &name, TPSEFile &pse_file )
{
	ifstream f;
	f.open( filename.c_str() );
	if ( !f.is_open() ) throw Exception( "Can't open file '%s'", filename.c_str() );
	ostringstream tmpstream;
	try {
		tmpstream << f.rdbuf();
	}
  catch( ... ) {
    try { f.close(); } catch( ... ) { };
    throw;
  }
  f.close();
  pse_file.filename = name;
  pse_file.data = tmpstream.str();
}

void writePSEFile( const string &dirname, TPSEFile &pse_file )
{
  ofstream f;
  f.open( string(dirname + pse_file.filename).c_str() );
  if (!f.is_open()) throw Exception( "Can't open file '%s'", string(dirname + pse_file.filename).c_str() );
  try {
    f << pse_file.data;
    f.close();
  }
  catch(...) {
    try { f.close(); } catch( ... ) { };
    throw;
  };
}

void WritePSEFiles( const TPKCS &pkcs, const string &desk, bool pr_grp )
{
  TQuery Qry(&OraSession);
  std::string certificate;
  int pkcs_id;
  Qry.SQLText =
   "SELECT grp_id FROM desks WHERE code=:code";
  Qry.CreateVariable( "code", otString, desk );
  Qry.Execute();
  if ( Qry.Eof )
    throw Exception( "invalid desk: %s", desk.c_str() );
  int grp_id = Qry.FieldAsInteger( "grp_id" );
  Qry.Clear();
  Qry.SQLText =
    "SELECT pkcs_id FROM crypt_file_params "
    " WHERE desk_grp_id=:grp_id AND ( :pr_grp!=0 AND desk IS NULL OR :pr_grp=0 AND desk=:desk )";
  Qry.CreateVariable( "grp_id", otInteger, grp_id );
  Qry.CreateVariable( "desk", otString, desk );
  Qry.CreateVariable( "pr_grp", otInteger, pr_grp );
  Qry.Execute();
  ProgTrace( TRACE5, "grp_id=%d, desk=%s, pr_grp=%d, Qry.Eof=%d", grp_id, desk.c_str(), pr_grp, Qry.Eof );
  if ( !Qry.Eof ) {
    pkcs_id = Qry.FieldAsInteger( "pkcs_id" );
    ProgTrace( TRACE5, "pkcs_id=%d", pkcs_id );
    Qry.Clear(); //!!! удаляем
    Qry.SQLText =
	    "BEGIN "
      " UPDATE crypt_term_cert SET pkcs_id=NULL WHERE pkcs_id=:pkcs_id;"
	    " DELETE crypt_files WHERE pkcs_id=:pkcs_id;"
	    " DELETE crypt_file_params WHERE pkcs_id=:pkcs_id;"
	    "END;";
	  Qry.CreateVariable( "pkcs_id", otInteger, pkcs_id );
	  Qry.Execute();
  }
	Qry.Clear();
  Qry.SQLText =
    "SELECT id__seq.nextval pkcs_id FROM dual";
  Qry.Execute();
  pkcs_id = Qry.FieldAsInteger( "pkcs_id" );
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO crypt_file_params(pkcs_id,keyname,certname,password,desk,desk_grp_id,send_count)"
    " SELECT :pkcs_id,:keyname,:certname,:password,DECODE(:pr_grp,0,:desk,NULL),grp_id,0 FROM desks"
    "  WHERE code=:desk";
  Qry.CreateVariable( "pkcs_id", otInteger, pkcs_id );
  Qry.CreateVariable( "keyname", otString, pkcs.key_filename );
  Qry.CreateVariable( "certname", otString, pkcs.cert_filename );
  Qry.CreateVariable( "password", otString, pkcs.password );
  Qry.CreateVariable( "pr_grp", otInteger, pr_grp );
  Qry.CreateVariable( "desk", otString, desk );
  Qry.Execute();
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO crypt_files(pkcs_id,name,data) VALUES(:pkcs_id,:name,:data)";
  Qry.CreateVariable( "pkcs_id", otInteger, pkcs_id );
  Qry.DeclareVariable( "name", otString );
  Qry.DeclareVariable( "data", otLongRaw );
  vector<TPSEFile>::const_iterator ireq;
  for ( vector<TPSEFile>::const_iterator i=pkcs.pse_files.begin(); i!=pkcs.pse_files.end(); i++ ) {
  	if ( i->filename == pkcs.cert_filename ) {
  		ireq = i;
  		continue;
    }
    Qry.SetVariable( "name", i->filename );
    Qry.SetLongVariable( "data", (void*)i->data.c_str(), i->data.size() );
    Qry.Execute();
  }
  // записываем запрос на сертификат
  IntPutRequestCertificate( ireq->data, desk, pr_grp, pkcs_id );
}

string getPassword( )
{
	string strtable( "A1B2C3D4E5F6G7H8I9J0KLMNOP1Q2R3S4T5U6V7W8X9Y0Z" );
	string pswd;
	srand( randt );
  while ( pswd.size() < PASSWORD_LENGTH ) {
		randt = rand();
		unsigned int idx = 1 + (int)( strtable.size() * ( randt / ( RAND_MAX + 1.0 ) ) );
		if ( idx < 0 || idx > strtable.size() ) {
			ProgError( STDLOG, "getPassword: invalid idx=%d", idx );
			continue;
		}
		if ( !pswd.empty() && pswd[ pswd.size() - 1 ] == strtable[ idx - 1 ] )
			continue;
		pswd += strtable[ idx - 1 ];
	}
	srand( randt );
	return  pswd;
}

void CreatePSE( const string &desk, bool pr_grp, int password_len, TPKCS &pkcs )
{
	pkcs.pse_files.clear();
	tst();
	ValidateCertificateRequest( desk, pr_grp );
	ValidatePKCSData( desk, pr_grp ); //нельзя создавать несколько PKCS для одного пульта или группы пультов
	TCertRequest req;
	GetCertRequestInfo( desk, pr_grp, req );
	tst();
	pkcs.key_filename = req.FileKey + ".key";
	pkcs.cert_filename = req.FileKey + ".pem";
	pkcs.password = getPassword( );
	string PSEpath = readStringFromTcl( "MESPRO_PSE_PATH", "./crypt" );
  PSEpath += "/pses";
  mkdir( PSEpath.c_str(), 0777 );
  int i = 1;
  while ( i < 100 && mkdir( string( PSEpath + "/" + IntToString(i) ).c_str(), 0777 )) i++;
  if ( i == 100 )
  	throw Exception( "Can't create dir=" + string( PSEpath + "/" + IntToString(i) ) + ", error=" + IntToString( errno ) );
  PSEpath += "/" + IntToString(i);
  ProgTrace( TRACE5, "CreatePKCS: PSEpath=%s", PSEpath.c_str() );
  SetRandInitCallbackFun((void *)init_rand_callback1);
  GetError( "PKCS7Init", PKCS7Init( 0, 0 ) );
  try {
    string file_key = PSEpath + "/pkey.key";
    string file_req = PSEpath + "/request.req";
	  if ( req.Algo.empty() )
	    GetError( "SetNewKeysAlgorithm", SetNewKeysAlgorithm( (char*)"ECR3410" ) );
	  else
	  	GetError( "SetNewKeysAlgorithm", SetNewKeysAlgorithm( (char*)req.Algo.c_str() ) );
	  GetError( "SetCertificateRequestFlags", SetCertificateRequestFlags( CERT_REQ_DONT_PRINT_TEXT ) );
	  GetError( "SetCountry", SetCountry( (char*)req.Country.c_str() ) );
	  GetError( "SetStateOrProvince", SetStateOrProvince( (char*)req.StateOrProvince.c_str() ) );
	  GetError( "SetLocality", SetLocality( (char*)req.Localite.c_str() ) );
	  GetError( "SetOrganization", SetOrganization( (char*)req.Organization.c_str() ) );
	  GetError( "SetOrganizationalUnit", SetOrganizationalUnit( (char*)req.OrganizationalUnit.c_str() ) );
	  GetError( "SetTitle", SetTitle( (char*)req.Title.c_str() ) );
	  GetError( "SetCommonName", SetCommonName( (char*)req.CommonName.c_str() ) );
	  GetError( "SetEmailAddress", SetEmailAddress( (char*)req.EmailAddress.c_str() ) ); //!!!
    GetError( "PSE31_Generation", PSE31_Generation( (char*)PSEpath.c_str(), 0, NULL, 0 ) );
    try {
      ProgTrace( TRACE5, "req.Algo=%s, file_key=%s", req.Algo.c_str(), file_key.c_str() );
      if ( req.Algo == "RSA" || req.Algo == "DSA" ) {
      	if ( req.Algo == "RSA" && req.KeyLength > 0 )
      		GetError( "SetKeysLength", SetKeysLength( req.KeyLength ) );
        	GetError( "NewKeysGeneration", NewKeysGeneration( (char*)file_key.c_str(), (char*)pkcs.password.c_str(), (char*)file_req.c_str() ) );
      }
      else {
      	GetError( "NewKeysGenerationEx", NewKeysGenerationEx( (char*)PSEpath.c_str(), NULL, (char*)file_key.c_str(), (char*)pkcs.password.c_str(), (char*)file_req.c_str() ) );
      }
      TPSEFile pse_file;
      readPSEFile( PSEpath + "/masks.db3", "masks.db3", pse_file );
      pkcs.pse_files.push_back( pse_file );
      readPSEFile( PSEpath + "/mk.db3", "mk.db3", pse_file );
      pkcs.pse_files.push_back( pse_file );
      readPSEFile( PSEpath + "/kek.opq", "kek.opq", pse_file );
      pkcs.pse_files.push_back( pse_file );
      readPSEFile( PSEpath + "/rand.opq", "rand.opq", pse_file );
      pkcs.pse_files.push_back( pse_file );
      readPSEFile( file_key, pkcs.key_filename, pse_file );
      pkcs.pse_files.push_back( pse_file );
      readPSEFile( file_req, pkcs.cert_filename, pse_file );
      pkcs.pse_files.push_back( pse_file );
      WritePSEFiles( pkcs, desk, pr_grp );
    }
    catch(...) {
    	DeletePSE( PSEpath, file_key, file_req );
    	throw;
    }
  	DeletePSE( PSEpath, file_key, file_req );
  }
  catch( ... ) {
  	PKCS7Final();
  	throw;
  }
  PKCS7Final();
}

void CryptInterface::CryptValidateServerKey(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	//пришло подтверждение авторизации - удаление из БД всей информации по PSE
	int pkcs_id = NodeAsInteger( "pkcs_id", reqNode );
	ProgTrace( TRACE5, "CryptValidateServerKey, pkcs_id=%d", pkcs_id );
	TQuery Qry(&OraSession);
	//!!! изменение пароля на новый
  Qry.Clear();
  Qry.SQLText =
    "SELECT name, data FROM crypt_files "
    " WHERE pkcs_id=:pkcs_id";
	Qry.CreateVariable( "pkcs_id", otInteger, pkcs_id );
	Qry.Execute();
	tst();
  void *pkeydata = NULL;
  int len = 0;
  TPKCS pkcs;
  bool pr_GOST;
  try {
    while ( !Qry.Eof ) {
      len = Qry.GetSizeLongField( "data" );
      if ( pkeydata == NULL )
        pkeydata = malloc( len );
      else
        pkeydata = realloc( pkeydata, len );
      if ( pkeydata == NULL )
        throw Exception( "Ошибка программы" );
      Qry.FieldAsLong( "data", pkeydata );
      TPSEFile psefile;
      psefile.filename = Qry.FieldAsString( "name" );
      ProgTrace( TRACE5, "psefile.filename=%s", psefile.filename.c_str() );
      psefile.data = string( (char*)pkeydata, len );
      pkcs.pse_files.push_back( psefile );
      Qry.Next();
	  }
	  tst();
    Qry.Clear();
    Qry.SQLText =
      "SELECT certificate FROM crypt_term_cert WHERE pkcs_id=:pkcs_id";
    Qry.CreateVariable( "pkcs_id", otInteger, pkcs_id );
    Qry.Execute();
    if ( Qry.Eof )
      throw Exception( "CryptValidateServerKey: cert not found" );
    string cert = Qry.FieldAsString( "certificate" );
    string algo = GetCertPublicKeyAlgorithmBuffer( (char*)cert.c_str(), cert.size() );
    ProgTrace( TRACE5, "algo=%s", algo.c_str() );
    if ( algo.empty() )
      throw Exception( "Ошибка программы" );
    bool pr_GOST = ( algo == string( "ECR3410" ) ||
                     algo == string( "R3410" ) );
    ProgTrace( TRACE5, "pr_GOST=%d, algo=%s", pr_GOST, algo.c_str() );
  }
  catch(...) {
  	if ( pkeydata )
  	  free( pkeydata );
    throw;
  }
	if ( pkeydata )
    free( pkeydata );
	if ( pkcs.pse_files.empty() )
    throw Exception( "CryptValidateServerKey: keys not found" );
	Qry.Clear();
	Qry.SQLText =
    "SELECT keyname,certname,password FROM crypt_file_params "
    " WHERE pkcs_id=:pkcs_id";
	Qry.CreateVariable( "pkcs_id", otInteger, pkcs_id );
	tst();
	Qry.Execute();
  if ( Qry.Eof )
    throw Exception( "CryptValidateServerKey: keys not found" );
  pkcs.key_filename = Qry.FieldAsString( "keyname" );
  pkcs.cert_filename = Qry.FieldAsString( "certname" );
  pkcs.password = Qry.FieldAsString( "password" );
	string PSEpath = readStringFromTcl( "MESPRO_PSE_PATH", "./crypt" );
  PSEpath += "/pses";
  mkdir( PSEpath.c_str(), 0777 );
  int i = 1;
  while ( i < 100 && mkdir( string( PSEpath + "/" + IntToString(i) ).c_str(), 0777 )) i++;
  if ( i == 100 )
  	throw Exception( "Can't create dir=" + string( PSEpath + "/" + IntToString(i) ) + ", error=" + IntToString( errno ) );
  ProgTrace( TRACE5, "i=%d", i );
  PSEpath += "/" + IntToString(i);
  ProgTrace( TRACE5, "CryptValidateServerKey: PSEpath=%s", PSEpath.c_str() );
  pkcs.key_filename = PSEpath + "/" + pkcs.key_filename;
  SetRandInitCallbackFun((void *)init_rand_callback1);
  GetError( "PKCS7Init", PKCS7Init( 0, 0 ) );
  try {
    try {
      for ( vector<TPSEFile>::iterator i=pkcs.pse_files.begin(); i!=pkcs.pse_files.end(); i++ ) {
        ProgTrace( TRACE5, "filename=%s", i->filename.c_str() );
        writePSEFile( PSEpath + "/", *i );
        tst();
      }
      string newpassword = getPassword( );
      ProgTrace( TRACE5, "oldpassword=%s, newpassword=%s, keyfile=%s", pkcs.password.c_str(), newpassword.c_str(), pkcs.key_filename.c_str() );
      if ( pr_GOST )
        GetError( "ChangePrivateKeyPasswordEx", ChangePrivateKeyPasswordEx( (char*)PSEpath.c_str(), NULL, (char*)pkcs.key_filename.c_str(),
                                                                            (char*)pkcs.password.c_str(), (char*)newpassword.c_str() ) );
      else
        GetError( "ChangePrivateKeyPassword", ChangePrivateKeyPassword( (char*)PSEpath.c_str(), (char*)pkcs.password.c_str(), (char*)newpassword.c_str() ) );
      tst();
      pkcs.password = newpassword;
      Qry.Clear();
      Qry.SQLText =
        "UPDATE crypt_files SET data=:data WHERE pkcs_id=:pkcs_id AND name=:name";
      Qry.CreateVariable( "pkcs_id", otInteger, pkcs_id );
      Qry.DeclareVariable( "name", otString );
      Qry.DeclareVariable( "data", otLongRaw );
      for ( vector<TPSEFile>::iterator i=pkcs.pse_files.begin(); i!=pkcs.pse_files.end(); i++ ) {
        TPSEFile pse_file;
        readPSEFile( PSEpath + "/" + i->filename, i->filename, *i );
        Qry.SetVariable( "name", i->filename );
        Qry.SetLongVariable( "data", (void*)i->data.c_str(), i->data.size() );
        ProgTrace( TRACE5, "filename=%s", i->filename.c_str() );
        Qry.Execute();
      }
      tst();
      Qry.Clear();
      Qry.SQLText =
        "UPDATE crypt_file_params SET password=:password, send_count=send_count+1 WHERE pkcs_id=:pkcs_id";
      Qry.CreateVariable( "pkcs_id", otInteger, pkcs_id );
      Qry.CreateVariable( "password", otString, pkcs.password );
      Qry.Execute();
      tst();
    }
    catch(...) {
    	DeletePSE( PSEpath, pkcs.key_filename, "" );
     	throw;
    }
    tst();
    DeletePSE( PSEpath, pkcs.key_filename, "" );
  }
  catch( ... ) {
  	PKCS7Final();
  	throw;
  }
  PKCS7Final();
  tst();
}

void CryptInterface::RequestPSE(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	TPKCS pkcs;
	bool pr_grp = GetNode( "pr_grp", reqNode );
  ProgTrace( TRACE5, "IntRequestCertificateData: pr_grp=%d", pr_grp );
  CreatePSE( NodeAsString( "desk", reqNode ), pr_grp, PASSWORD_LENGTH, pkcs );
  AstraLocale::showMessage( "MSG.MESSAGE_PRO.KEY_AND_REQUEST_OK" );
}

#endif /*USE MESPRO*/

void CryptInterface::SetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	tst();
#ifdef USE_MESPRO
	xmlNodePtr node = GetNode( "certificates", reqNode );
	if ( !node )
		return;
	GetError( "PKCS7Init", PKCS7Init( 0, 0 ) );
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
    " INSERT INTO crypt_term_cert(id,desk_grp_id,desk,certificate,first_date,last_date,pr_denial,pkcs_id) "
    " SELECT id__seq.nextval,desk_grp_id,desk,:certificate,:first_date,:last_date,0,pkcs_id FROM crypt_term_req "
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
  			int err = CertAndRequestMatchBuffer( (char*)cert.data(), cert.size(), (char*)i->cert.data(), i->cert.size() );
  			if ( err ) {
  				CERTIFICATE_INFO info;
  				GetError( "GetCertificateInfoBufferEx", GetCertificateInfoBufferEx( (char*)cert.data(), cert.size(), &info ) );
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
