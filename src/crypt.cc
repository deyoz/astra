#include <string>
#include "tclmon/mespro_crypt.h"
#include "tclmon/tclmon.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/sirena_queue.h"
#include "serverlib/helpcpp.h"
#include "crypt.h"
#include "oralib.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

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
    	msg_error = "Шифрованное соединение: срок действия ключа истек. Обратитесь к администратору";
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
  try {
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
    if ( Qry->Eof || Qry->FieldAsInteger( "pr_crypt" ) == 0 ) { //пульт не может работать в режиме шифрования, а пришло зашифрованное сообщение
      *error=WRONG_TERM_CRYPT_MODE;
      tst();
    	return;
    }
    int grp_id = Qry->FieldAsInteger( "grp_id" );
    Qry->Clear();
    Qry->SQLText =
      "SELECT certificate,private_key,first_date,last_date,pr_ca FROM crypt_server "
      " WHERE pr_denial=0 AND SYSDATE BETWEEN first_date AND last_date "
      " ORDER BY id DESC";
    Qry->Execute();
    while ( !Qry->Eof ) {
  	  if ( Qry->FieldAsInteger("pr_ca") && params.CA.empty() )
  		  params.CA = Qry->FieldAsString( "certificate" );
      if ( !Qry->FieldAsInteger("pr_ca") && params.PKey.empty() ) {
  		  params.PKey = Qry->FieldAsString( "private_key" );
  		  params.server_cert = Qry->FieldAsString( "certificate" );
  	  }
  	  if ( !params.CA.empty() && !params.PKey.empty() )
  	  	break;
  	  Qry->Next();
    }
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
    Qry->Clear();
    Qry->SQLText =
      "SELECT certificate FROM crypt_term_cert "
      " WHERE desk_grp_id=:grp_id AND ( desk IS NULL OR desk=:desk ) AND "
      "       pr_denial=0 AND SYSDATE BETWEEN first_date AND last_date"
      " ORDER BY desk ASC, id DESC";
    Qry->CreateVariable( "grp_id", otInteger, grp_id );
    Qry->CreateVariable( "desk", otString, desk );
    Qry->Execute();
    if ( Qry->Eof || Qry->FieldIsNULL( "certificate" ) ) {
  	  *error = UNKNOWN_CLIENT_CERTIFICATE;
  	  return;
    }
    params.client_cert = Qry->FieldAsString( "certificate" );
  }
  catch(...) {
  	delete Qry;
  	throw;
  };
  delete Qry;
}

#endif // USE_MESPRO

