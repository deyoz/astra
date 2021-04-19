#include <string>
#include <stdio.h>
#include <optional>
#include "tclmon/mespro_crypt.h"
#include "serverlib/mespro_api.h"
#include "tclmon/tclmon.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/msg_const.h"
#include "serverlib/levc_callbacks.h"
#include "serverlib/sirena_queue.h"
#include <serverlib/str_utils.h>
#include <serverlib/dbcpp_session.h>
#include <serverlib/pg_cursctl.h>
#include "crypt.h"
#include "oralib.h"
#include "date_time.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "stl_utils.h"
#include "tclmon/tcl_utils.h"
#include "cache.h"

#include "db_tquery.h"
#include "PgOraConfig.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"
#include <serverlib/slogger.h>

const int MIN_DAYS_CERT_WARRNING = 10;
const unsigned int PASSWORD_LENGTH = 16;

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

static time_t randt = time(NULL);

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

#ifdef USE_MESPRO

void GetCertRequestInfo( const string &desk, bool pr_grp, TCertRequest &req, bool ignoreException );
#endif //USE_MESPRO

size_t form_crypt_error(char* res, size_t res_len, const char* head, size_t hlen, int error)
{
  int newlen;
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
  memcpy(res, head, hlen);
  res[byte] |= MSG_SYS_ERROR;
  auto utf8txt = CP866toUTF8(msg_error);
  if(utf8txt.empty())
    utf8txt="System error!";
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
      newlen = utf8txt.size()+err_len;
      memcpy(res+hlen+err_len,utf8txt.data(),utf8txt.size());
  };
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
    case 5: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_5"; break;
    case 6: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_6"; break;
    case 7: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_7"; break;
    case 8: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_8"; break;
    case 9: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_9"; break;
    case 10: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_10"; break;
    case 11: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_11"; break;
    case 12: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_12"; break;
    case 13: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_13"; break;
    case 14: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_14"; break;
    case 15: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_15"; break;
    case 16: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_16"; break;
    case 17:
    case 103: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_103"; break;
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
    case 109: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_109"; break;
    case 112: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_112"; break;
    case 114: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_114"; break;
    case 115: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_115"; break;
    case 116: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_116"; break;
    case 117: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_117"; break;
    case 118: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_118"; break;
    case 119: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_119"; break;
    case 122: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_122"; break;
    case 123: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_123"; break;
    case 124: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_124"; break;
    case 125: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_125"; break;
    case 127: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_127"; break;
    case 128: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_128"; break;
    case 129: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_129"; break;
    case 132: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_132"; break;
    case 134: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_134"; break;
    case 135: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_135"; break;
    case 136: lexema.lexema_id = "MSG.MESSAGEPRO.ERROR_136"; break;
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
#endif //USE_MESPRO

struct RawBuffer final {

    void* buffer;
    size_t capacity;

    RawBuffer() noexcept : buffer{nullptr}, capacity{0} {}

    ~RawBuffer() noexcept {
        free(buffer);
    }

    void increaseCapacity(size_t newCapacity) {
        if (capacity < newCapacity) {
            free(buffer);
            buffer = malloc(newCapacity);
            if (nullptr == buffer) {
                throw Exception("Ошибка Программы");
            }
            capacity = newCapacity;
        }
    }
};

// определяет ошибку режима шифрования и возвращает признак шифрования для группы
// за основу определения шифрования по группе или пульту берем эту ф-цию
bool GetCryptGrp(const std::string& desk, int& grp_id, bool& pr_grp)
{
    std::string deskUpper = StrUtils::ToUpper(desk);

    std::optional<int> group = getDeskGroupByCode(deskUpper);

    if (!deskGroupExists(group)) {
        return false;
    }

    DB::TQuery Qry(PgOra::getROSession("CRYPT_SETS"));
    Qry.SQLText =
       "SELECT pr_crypt, desk_grp_id, desk "
       "FROM crypt_sets "
       "WHERE desk IS NULL OR desk = :deskUpper "
       "ORDER BY desk";

    Qry.CreateVariable( "deskUpper", otString, deskUpper );
    Qry.Execute();

    if (!Qry.Eof) {
        grp_id = Qry.FieldAsInteger( "desk_grp_id" );
        pr_grp = Qry.FieldIsNULL( "desk" );

        return Qry.FieldAsInteger( "pr_crypt" ) != 0;
    }
    return false; //пульт не может работать в режиме шифрования, а пришло зашифрованное сообщение
}

//сертификат выбираем по след. правилам:
//1. сортировка по пульту, доступу и времени начала действия сертификата
//2. pr_grp - определяет какой сертификат изпользовать (для пульта или групповой)
bool GetClientCertificate( int grp_id, bool pr_grp, const std::string &desk, std::string &certificate, int &pkcs_id )
{
  pkcs_id = -1;
  certificate.clear();
  TDateTime nowLocal = Now();
  DB::TQuery Qry(PgOra::getROSession("CRYPT_TERM_CERT"));
  Qry.SQLText =
    "SELECT pkcs_id, desk, certificate, pr_denial, first_date, last_date FROM crypt_term_cert "
    " WHERE desk_grp_id=:grp_id AND ( desk IS NULL OR desk=:desk ) "
    " ORDER BY desk ASC, pr_denial ASC, first_date ASC, id ASC";
  Qry.CreateVariable( "grp_id", otInteger, grp_id );
  Qry.CreateVariable( "desk", otString, desk );
  Qry.Execute();
  bool pr_exists=false;
  while ( !Qry.Eof ) {
    if ( (!Qry.FieldIsNULL( "desk" ) && pr_grp) || // если пультовой сертификат, а у нас описан групповой
           (Qry.FieldIsNULL( "desk" ) && !pr_grp) ) { // если групповой сертификат, а у нас описан пультовой
        Qry.Next();
          continue;
        }
    if ( Qry.FieldAsInteger( "pr_denial" ) == 0 && // разрешен
         nowLocal > Qry.FieldAsDateTime( "first_date" ) ) // начал выполняться
        pr_exists = true; // значит сертификат есть, но возможно он просрочен
    if ( nowLocal < Qry.FieldAsDateTime( "first_date" ) || // не начал выполняться
         nowLocal > Qry.FieldAsDateTime( "last_date" ) ) { // закончил выполняться
      Qry.Next();
      continue;
    }
    // сертификат актуален. Сортировка по пульту. а потом по группе + признак отмены
    if ( Qry.FieldAsInteger( "pr_denial" ) != 0 )
        break;
    certificate = Qry.FieldAsString( "certificate" );
    if ( !Qry.FieldIsNULL( "pkcs_id" ) )
        pkcs_id = Qry.FieldAsInteger( "pkcs_id" );

    break;
  }
  if ( pkcs_id >= 0 )
    ProgTrace( TRACE5, "pkcs_id=%d", pkcs_id );
  ProgTrace( TRACE5, "pr_exists=%d", pr_exists );
  return pr_exists;
}

#ifdef USE_MESPRO
bool isRightCert( const std::string& cert )
{
  std::string algo;
  char *buf = MESPRO_API::GetCertPublicKeyAlgorithmBuffer( (char*)cert.c_str(), cert.size() );
  if ( buf == nullptr ) {
    LogTrace(TRACE5) << cert;
    return false;
  }
  algo = buf;
  MESPRO_API::FreeBuffer( buf );
  return ( (!isMespro_V1() && (std::string(MP_KEY_ALG_NAME_GOST_12_256) == algo)) ||
           (isMespro_V1() && (std::string(MP_KEY_ALG_NAME_GOST_12_256) != algo)) );
}

void GetServerCertificate( std::string &ca, std::string &pk, std::string &server )
{
  tst();
  DB::TQuery Qry(PgOra::getROSession("CRYPT_SERVER"));
  Qry.SQLText =
    "SELECT id,certificate,private_key,first_date,last_date,pr_ca FROM crypt_server "
    " WHERE pr_denial=0 AND :nowutc BETWEEN first_date AND last_date "
    " ORDER BY id DESC";
  Qry.CreateVariable("nowutc", otDate, NowUTC());
  Qry.Execute();

  while ( !Qry.Eof ) {
    if ( Qry.FieldAsInteger("pr_ca") && ca.empty() ) {
      ca = Qry.FieldAsString( "certificate" );
      if ( !isRightCert( ca ) )
        ca.clear();
      else
        LogTrace(TRACE5) << "find CA " << Qry.FieldAsInteger("id");
    }
    if ( !Qry.FieldAsInteger("pr_ca") && server.empty() ) {
      pk = Qry.FieldAsString( "private_key" );
      server = Qry.FieldAsString( "certificate" );
      if ( !isRightCert( server ) )
        server.clear();
      else
        LogTrace(TRACE5) << "find Server cert " << Qry.FieldAsInteger("id");
    }
    if ( !ca.empty() && !server.empty() )
      break;
    Qry.Next();
  }
  return;
}

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
  int grp_id;
  bool pr_grp;
  if ( !GetCryptGrp( desk, grp_id, pr_grp ) ) { //пульт не может работать в режиме шифрования, а пришло зашифрованное сообщение
    *error=WRONG_TERM_CRYPT_MODE;
    return;
  }
  TCertRequest req;
  GetCertRequestInfo( desk, pr_grp, req, true );
  MESPRO_API::setLibNameFromAlgo( req.Algo );
  GetServerCertificate( params.CA, params.PKey, params.server_cert );
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
  bool pr_exists = GetClientCertificate( grp_id, pr_grp, desk, params.client_cert, pkcs_id );
  if ( params.client_cert.empty() ) {
    if ( pr_exists )
      *error = EXPIRED_KEY;
    else
      *error = UNKNOWN_CLIENT_CERTIFICATE;
    return;
  }
}
#endif // USE_MESPRO

void TCrypt::Init( const std::string &desk )
{
  Clear();
  if ( TReqInfo::Instance()->duplicate ) //!!! не работаем в режиме дуплицирования в режиме шифрования
    return;
  #ifdef USE_MESPRO
  TQuery Qry(&OraSession);
  int grp_id;
  bool pr_grp;
  if ( !GetCryptGrp( Qry, desk, grp_id, pr_grp ) )
    return;
  ProgTrace( TRACE5, "grp_id=%d,pr_grp=%d", grp_id, pr_grp );
  TCertRequest req;
  GetCertRequestInfo( desk, pr_grp, req, true );
  MESPRO_API::setLibNameFromAlgo( req.Algo );

  string pk;
  GetServerCertificate( Qry, ca_cert, pk, server_cert );
  if ( ca_cert.empty() || server_cert.empty() )
    throw Exception("ca or server certificate not found");

  bool pr_exists = GetClientCertificate( Qry, grp_id, pr_grp, desk, client_cert, pkcs_id );
  if ( client_cert.empty() ) {
    if ( !pr_exists )
      AstraLocale::showProgError("MSG.MESSAGEPRO.CRYPT_CONNECT_CERT_NOT_FOUND.CALL_ADMIN");
    else
      AstraLocale::showProgError("MSG.MESSAGEPRO.CRYPT_CONNECT_CERT_OUTDATED.CALL_ADMIN");
    throw UserException2();
  }
  #endif //USE_MESPRO
}

void convertDataToHex(char* dest, const char* src, size_t len)
{
    static const char hexTable[16] = {
       '0', '1', '2', '3', '4', '5', '6', '7',
       '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    const unsigned char* bound = reinterpret_cast<const unsigned char*>(src + len);
    const unsigned char*    it = reinterpret_cast<const unsigned char*>(src);
          unsigned char* dstIt = reinterpret_cast<      unsigned char*>(dest);

    while (it != bound) {
        *(dstIt++) = hexTable[*it >> 4];
        *(dstIt++) = hexTable[*it & 15];
        ++it;
    }

    *dstIt = 0;
}

void fillNodeWithFilesOra(xmlNodePtr node, int pkcs_id) {
    TQuery Qry(&OraSession);
    Qry.SQLText =
       "SELECT name, data from crypt_files WHERE pkcs_id=:pkcs_id";
    Qry.CreateVariable( "pkcs_id", otInteger, pkcs_id );
    Qry.Execute();

    if (Qry.Eof) {
        return;
    }

    node = NewTextChild( node, "files" );

    RawBuffer rawBuffer;

    while ( !Qry.Eof ) {
        size_t len = Qry.GetSizeLongField( "data" );

        // len - это куда запишут нам данные
        // 2 len - это займет hex представление данных
        // +1 - это null для того что бы hex был cstring.
        rawBuffer.increaseCapacity(len * 6 + 1);

        const char* data       = (const char*)rawBuffer.buffer;
        const char* hex_buffer = data + len;

        Qry.FieldAsLong( "data", rawBuffer.buffer );
        convertDataToHex(const_cast<char*>(hex_buffer), data, len);
        xmlNodePtr fnode = NewTextChild(node, "file", hex_buffer);
        SetProp(fnode, "filename", Qry.FieldAsString("name"));
        Qry.Next();
    }
}

void fillNodeWithFiles(xmlNodePtr node, int pkcs_id) {

    std::string data;
    std::string name;

    PgCpp::BinaryDefHelper<std::string> defdata{data};

    PgCpp::CursCtl cur = DbCpp::mainPgReadOnlySession(STDLOG)
       .createPgCursor(STDLOG,
       "SELECT name, data from crypt_files WHERE pkcs_id=:pkcs_id",
        true
    );

    cur.bind(":pkcs_id", pkcs_id)
       .def(name)
       .def(defdata)
       .exec();

    if (0 == cur.rowcount()) {
        return;
    }

    node = NewTextChild(node, "files");

    while (!cur.fen()) {
        std::string hex;
        StringToHex(data, hex);
        xmlNodePtr fnode = NewTextChild(node, "file", hex.c_str());
        SetProp(fnode, "filename", name);
    }
}

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
       (GetNode( "getkeys", reqNode ) == NULL && Qry.FieldAsInteger( "send_count" ) != 0) )
      return;

    node = NewTextChild( node, "pkcs", Crypt.pkcs_id );
    if ( Qry.FieldIsNULL( "desk" ) )
      SetProp( node, "common_dir" );
    SetProp( node, "key_filename", Qry.FieldAsString( "keyname" ) );
    SetProp( node, "cert_filename", Qry.FieldAsString( "certname" ) );

    if (PgOra::supportsPg("CRYPT_FILES")) {
        fillNodeWithFiles(node, Crypt.pkcs_id);
    } else {
        fillNodeWithFilesOra(node, Crypt.pkcs_id);
    }
  }
}

// это первый запрос с клиента или запрос после ошибки работы шифрования
void CryptInterface::GetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
   IntGetCertificates(ctxt, reqNode, resNode);
}

void GetCertRequestInfo( const string &desk, bool pr_grp, TCertRequest &req, bool ignoreException )
{
  ProgTrace( TRACE5, "GetCertRequestInfo, desk=%s, pr_grp=%d", desk.c_str(), pr_grp );
  TDateTime udate = NowUTC();
  DB::TQuery Qry(PgOra::getROSession("CRYPT_REQ_DATA"));
  if ( pr_grp ) {
    std::optional<int> group = getDeskGroupByCode(desk);
    if (!deskGroupExists(group)) {
      throw AstraLocale::UserException( "MSG.MESSAGEPRO.NO_DATA_FOR_CERT_QRY" );
    }
    Qry.SQLText =
      "SELECT   country, state, city, organization, organizational_unit, "
               "title, user_name, email, key_algo, keyslength "
      "FROM     crypt_req_data "
      "WHERE    desk_grp_id =: group "
           "AND desk IS NULL "
           "AND pr_denial = 0 "
      "ORDER BY desk";
    Qry.CreateVariable( "group", otString, *group );
  } else {
    Qry.SQLText =
      "SELECT   country, state, city, organization, organizational_unit, "
               "title, user_name, email, key_algo, keyslength "
      "FROM     crypt_req_data "
      "WHERE    desk = :desk "
           "AND pr_denial = 0";
    Qry.CreateVariable( "desk", otString, desk );
  }
  Qry.Execute();
  if ( Qry.Eof ) {
    if ( ignoreException ) return;
    throw AstraLocale::UserException( "MSG.MESSAGEPRO.NO_DATA_FOR_CERT_QRY" );
  }
  req.FileKey = "astra"+DateTimeToStr( udate, "ddmmyyhhnn" );
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
    req.CommonName += DateTimeToStr( udate, "ddmmyyhhnn" );
  }
  req.EmailAddress = Qry.FieldAsString( "email" );
}

void ValidateCertificateRequest( const string &desk, bool pr_grp )
{
    std::optional<int> group = getDeskGroupByCode(desk);

    if (!deskGroupExists(group)) {
        return;
    }
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT desk "
       "FROM crypt_term_req "
       "WHERE desk_grp_id = :grp_id "
       "AND (desk IS NULL OR desk = :desk) "
       "ORDER BY desk",
        PgOra::getROSession("CRYPT_TERM_REQ")
    );

    std::string filteredDesk;

    cur.stb()
       .defNull(filteredDesk, "")
       .bind(":grp_id", *group)
       .bind(":desk", desk)
       .exec();

    while (!cur.fen()) {
        if ((filteredDesk.empty() && pr_grp) || (!filteredDesk.empty() && !pr_grp)) {
            throw AstraLocale::UserException( "MSG.MESSAGEPRO.CERT_QRY_CREATED_EARLIER.NOT_PROCESSED_YET" );
        }
    }
}

// запрос может быть подписан и сертификат залит в БД, но пока это не отправлено на клиент - выполнения заново цикла откладывается!
void ValidatePKCSData( const string &desk, bool pr_grp )
{
    tst();

    std::optional<int> group = getDeskGroupByCode(desk);

    if (!deskGroupExists(group)) {
        return;
    }

    DbCpp::CursCtl cur = make_db_curs(
       "SELECT desk "
       "FROM crypt_file_params "
       "WHERE send_count = 0 "
       "AND desk_grp_id = :grp_id "
       "AND (desk IS NULL OR desk = :desk) "
       "ORDER BY desk",
        PgOra::getROSession("CRYPT_FILE_PARAMS")
    );

    std::string filteredDesk;

    cur.stb()
       .defNull(filteredDesk, "")
       .bind(":grp_id", *group)
       .bind(":desk", desk)
       .exec();

    while (!cur.fen()) {
        if ((filteredDesk.empty() && pr_grp) || (!filteredDesk.empty() && !pr_grp)) {
            throw AstraLocale::UserException( "MSG.MESSAGEPRO.CERT_CREATED_EARLIER.NOT_PROCESSED_YET" );
        }
    }
}

void IntRequestCertificateData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

  bool pr_grp = GetNode( "pr_grp", reqNode );
  ProgTrace( TRACE5, "IntRequestCertificateData: pr_grp=%d", pr_grp );
  ValidateCertificateRequest( TReqInfo::Instance()->desk.code, pr_grp );

  TCertRequest req;
  GetCertRequestInfo( TReqInfo::Instance()->desk.code, pr_grp, req, false );
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
    int id = PgOra::getSeqNextVal_int("ID__SEQ");
    std::optional<int> group = getDeskGroupByCode(desk);

    if (!deskGroupExists(group)) {
        // что в этом случае мы должны сделать?
        LogTrace(TRACE5) << "Desk group for desk: " << desk << " not found";
        return;
    }

    DB::TQuery Qry(PgOra::getRWSession("CRYPT_TERM_REQ"));
    Qry.SQLText =
       "INSERT INTO crypt_term_req( id,  desk_grp_id,  desk,  request,  pkcs_id) "
       "VALUES "                 "(:id, :desk_grp_id, :desk, :request, :pkcs_id)";

    Qry.CreateVariable("id", otInteger, id);
    Qry.CreateVariable("desk_grp_id", otInteger, *group);

    if (pr_grp) {
        Qry.CreateVariable("desk", otString, FNull);
    } else {
        Qry.CreateVariable("desk", otString, desk);
    }

    Qry.CreateVariable("request", otString, ConvertCodepage(request, "UTF-8", "CP866")); //request to db CP866

    if (NoExists == pkcs_id) {
        Qry.CreateVariable("pkcs_id", otInteger, FNull);
    } else {
        Qry.CreateVariable("pkcs_id", otInteger, pkcs_id);
    }

    Qry.Execute();
}

void CryptInterface::PutRequestCertificate(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  bool pr_grp = GetNode( "pr_grp", reqNode );
  string request = NodeAsString( "request_certificate", reqNode );
  IntPutRequestCertificate( request, TReqInfo::Instance()->desk.code, pr_grp, NoExists );
}

std::string composeGroupName(const int groupId)
{
    std::string descr;
    std::string city;
    std::string airline;
    std::string airp;

    DbCpp::CursCtl cur = make_db_curs(
       "SELECT descr, city, airline, airp "
       "FROM desk_grp "
       "WHERE grp_id=:grp_id",
        PgOra::getRWSession("DESK_GRP")
    );

    cur.stb()
       .bind(":grp_id", groupId)
       .defNull(descr, "")
       .defNull(city, "")
       .defNull(airline, "")
       .defNull(airp, "")
       .EXfet();

    std::string result = descr;
    result += ' ';
    result += city;

    if (!airline.empty() || !airp.empty()) {
        result += '(';
        result += airline;
        if (!airline.empty() && !airp.empty()) {
            result += ',';
        }
        result += airp;
        result += ')';
    }

    return result;
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

void CryptInterface::GetRequestsCertificate(XMLRequestCtxt*, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    bool pr_search = GetNode("search", reqNode);

    DB::TQuery Qry(PgOra::getROSession("CRYPT_TERM_REQ"));

    if (GetNode("id", reqNode)) {
        Qry.SQLText =
           "SELECT id, desk, desk_grp_id, request "
           "FROM crypt_term_req "
           "WHERE id=:id";
        Qry.CreateVariable("id", otInteger, NodeAsInteger("id", reqNode));
    } else {
        Qry.SQLText =
           "SELECT id, desk, desk_grp_id, request "
           "FROM crypt_term_req";
    }

    Qry.Execute();

    if (Qry.Eof) {
        throw AstraLocale::UserException("MSG.MESSAGEPRO.NO_CERT_QRYS");
    }

    vector<TSearchData> reqs;
    xmlNodePtr desksNode = NULL, grpsNode = NULL, reqsNode = NULL;

    while (!Qry.Eof) {
        TSearchData req_search;
        req_search.id = Qry.FieldAsInteger("id");
        if (!Qry.FieldIsNULL("desk")) {
            if (pr_search && !desksNode) {
                desksNode = NewTextChild(resNode, "desks");
            }
            req_search.desk = Qry.FieldAsString("desk");
        }
        if (Qry.FieldIsNULL("desk")) {
            if (pr_search && !grpsNode) {
                grpsNode = NewTextChild(resNode, "grps");
            }
            req_search.grp = composeGroupName(Qry.FieldAsInteger("desk_grp_id"));
        }
        req_search.data = Qry.FieldAsString("request");
        reqs.push_back(req_search);
        Qry.Next();
    }

    for (vector<TSearchData>::iterator i = reqs.begin(); i != reqs.end(); i++) {
        if (pr_search) {
            xmlNodePtr dNode = !i->desk.empty()
              ? NewTextChild(desksNode, "desk", i->desk)
              : NewTextChild(grpsNode, "grp", i->grp);
            SetProp(dNode, "id", i->id);
        } else {
            if (!reqsNode) {
                reqsNode = NewTextChild(resNode, "cert_requests");
            }
            xmlNodePtr n = NewTextChild(reqsNode, "request");
            NewTextChild(n, "id", i->id);
            NewTextChild(n, "desk", i->desk);
            NewTextChild(n, "grp", i->grp);
            NewTextChild(n, "data", i->data);
        }
    }
}

struct TRequest {
        int id;
  string cert;
};

TDateTime ConvertCertificateDate( char *certificate_date )
{
  TDateTime d;
  int res = StrToDateTime( upperc(certificate_date).c_str(), "dd.mm.yyyy hh:nn:ss", d, true );
  if ( res == EOF )
    throw Exception( "Invalid Certificate date=%s", certificate_date );
  return d;
}

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

#ifdef USE_MESPRO

void DeletePSE( const string &PSEpath, const string &file_key, const string &file_req )
{
  if ( !file_key.empty() )
    unlink( file_key.c_str() );
  if ( !file_req.empty() )
    unlink( file_req.c_str() );
  MESPRO_API::PSE_Erase( (char *)PSEpath.c_str() );
  remove( PSEpath.c_str() );
}

#endif //USE_MESPRO

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
  pkcs_id = PgOra::getSeqNextVal_int("ID__SEQ");
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
                if ( idx > strtable.size() ) {
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

static char* CP866toUTF8Char( const std::string& value ) {
  static std::string v = "";
  v = ConvertCodepage( value,  "CP866", "UTF-8" );
  return (char*)v.c_str();
}


void CreatePSE( const string &desk, bool pr_grp, int password_len, TPKCS &pkcs )
{
#ifdef USE_MESPRO
  MESPRO_API::setLibNameFromAlgo( MP_KEY_ALG_NAME_GOST_12_256 );
  MESPRO_API::SetInputCodePage( X509_NAME_UTF8 );
  pkcs.pse_files.clear();
  ValidateCertificateRequest( desk, pr_grp );
  ValidatePKCSData( desk, pr_grp ); //нельзя создавать несколько PKCS для одного пульта или группы пультов
  TCertRequest req;
  GetCertRequestInfo( desk, pr_grp, req, false );
  pkcs.key_filename = req.FileKey + ".key";
  pkcs.cert_filename = req.FileKey + ".pem";
  pkcs.password = getPassword( );
  string PSEpath = MESPRO_API::getPSEPath();
  PSEpath += "/pses";
  mkdir( PSEpath.c_str(), 0777 );
  int i = 1;
  while ( i < 100 && mkdir( string( PSEpath + "/" + IntToString(i) ).c_str(), 0777 )) i++;
  if ( i == 100 )
        throw Exception( "Can't create dir=" + string( PSEpath + "/" + IntToString(i) ) + ", error=" + IntToString( errno ) );
  PSEpath += "/" + IntToString(i);
  ProgTrace( TRACE5, "CreatePKCS: PSEpath=%s", PSEpath.c_str() );
  MESPRO_API::SetRandInitCallbackFun(init_rand_callback);
  GetError( "PKCS7Init", MESPRO_API::PKCS7Init( 0, 0 ) );
  MESPRO_API::setInitLib( true );

  try {
    string file_key = PSEpath + "/pkey.key";
    string file_req = PSEpath + "/request.req";
    GetError( "SetNewKeysAlgorithm", MESPRO_API::SetNewKeysAlgorithm( req.Algo.empty()?(char*)MP_KEY_ALG_NAME_ECR3410:(char*)MESPRO_API::GetAlgo( req.Algo ).c_str() ) );
    GetError( "SetCertificateRequestFlags", MESPRO_API::SetCertificateRequestFlags( CERT_REQ_DONT_PRINT_TEXT ) );
    GetError( "SetCountry", MESPRO_API::SetCountry( CP866toUTF8Char( req.Country ) ) );
    GetError( "SetStateOrProvince", MESPRO_API::SetStateOrProvince( CP866toUTF8Char(req.StateOrProvince ) ) );
    GetError( "SetLocality", MESPRO_API::SetLocality( CP866toUTF8Char( req.Localite ) ) );
    GetError( "SetOrganization", MESPRO_API::SetOrganization( CP866toUTF8Char( req.Organization ) ) );
    GetError( "SetOrganizationalUnit", MESPRO_API::SetOrganizationalUnit( CP866toUTF8Char( req.OrganizationalUnit ) ) );
    GetError( "SetTitle", MESPRO_API::SetTitle( CP866toUTF8Char( req.Title ) ) );
    GetError( "SetCommonName", MESPRO_API::SetCommonName( CP866toUTF8Char( req.CommonName ) ) );
    GetError( "SetEmailAddress", MESPRO_API::SetEmailAddress( CP866toUTF8Char( req.EmailAddress ) ) ); //!!!
    GetError( "PSE_Generation", MESPRO_API::PSE_Generation( (char*)PSEpath.c_str(), 0, NULL, 0 ) );
    try {
      ProgTrace( TRACE5, "req.Algo=%s, file_key=%s", req.Algo.c_str(), file_key.c_str() );
      if ( req.Algo == MP_KEY_ALG_NAME_RSA || req.Algo == MP_KEY_ALG_NAME_DSA ) {
        if ( (req.Algo == MP_KEY_ALG_NAME_RSA) && (req.KeyLength > 0) ) {
          GetError( "SetKeysLength", MESPRO_API::SetKeysLength( req.KeyLength ) );
        }
        GetError( "NewKeysGeneration", MESPRO_API::NewKeysGeneration( (char*)file_key.c_str(), (char*)pkcs.password.c_str(), (char*)file_req.c_str() ) );
      }
      else {
        GetError( "NewKeysGenerationEx", MESPRO_API::NewKeysGenerationEx( (char*)PSEpath.c_str(), NULL, (char*)file_key.c_str(), (char*)pkcs.password.c_str(), (char*)file_req.c_str() ) );
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
    MESPRO_API::PKCS7Final();
    MESPRO_API::setInitLib( false );
    throw;
  }
  MESPRO_API::PKCS7Final();
#endif //USE_MESPRO
}

#ifdef USE_MESPRO
std::string getCertificate(int pkcs_id)
{
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT certificate "
       "FROM crypt_term_cert "
       "WHERE pkcs_id=:pkcs_id",
        PgOra::getRWSession("CRYPT_TERM_CERT")
    );

    std::string certificate;

    cur.stb()
       .bind(":pkcs_id", pkcs_id)
       .def(certificate)
       .EXfet();

    if (DbCpp::ResultCode::NoDataFound == cur.err()) {
        throw Exception("CryptValidateServerKey: cert not found");
    }

    return certificate;
}

void fillTpkcsWithFiles(TPKCS& pkcs, int pkcs_id)
{
    DB::TQuery Qry(PgOra::getRWSession("CRYPT_FILES"));
    Qry.SQLText =
       "SELECT name, data "
       "FROM crypt_files "
       "WHERE pkcs_id=:pkcs_id";
    Qry.CreateVariable("pkcs_id", otInteger, pkcs_id);
    Qry.Execute();

    RawBuffer pkey;

    while (!Qry.Eof) {
        int len = Qry.GetSizeLongField("data");
        rawBuffer.increaseCapacity(len * 2);

        Qry.FieldAsLong("data", pkey.buffer);
        TPSEFile psefile;
        psefile.filename = Qry.FieldAsString("name");
        ProgTrace(TRACE5, "psefile.filename=%s", psefile.filename.c_str());
        psefile.data = string((char*)pkey.buffer, len);
        pkcs.pse_files.push_back(psefile);
        Qry.Next();
    }

    if (pkcs.pse_files.empty()) {
        throw Exception("CryptValidateServerKey: keys not found");
    }
}

void CryptInterface::CryptValidateServerKey(XMLRequestCtxt*, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    //пришло подтверждение авторизации - удаление из БД всей информации по PSE
    tst(); //!!!проверить
    MESPRO_API::setLibNameFromAlgo(MP_KEY_ALG_NAME_GOST_12_256);

    int pkcs_id = NodeAsInteger("pkcs_id", reqNode);
    ProgTrace(TRACE5, "CryptValidateServerKey, pkcs_id=%d", pkcs_id);
    //!!! изменение пароля на новый

    TPKCS pkcs;
    bool pr_GOST;

    fillTpkcsWithFiles(pkcs, pkcs_id);

    string cert = getCertificate(pkcs_id);
    char* buf = MESPRO_API::GetCertPublicKeyAlgorithmBuffer((char*)cert.c_str(), cert.size());
    if (buf == nullptr) {
        throw Exception("Ошибка программы");
    }
    string algo = buf;
    MESPRO_API::FreeBuffer(buf);
    pr_GOST = (algo != MP_KEY_ALG_NAME_RSA && algo != MP_KEY_ALG_NAME_DSA);
    ProgTrace(TRACE5, "pr_GOST=%d, algo=%s", pr_GOST, algo.c_str());

    Qry.Clear();
    Qry.SQLText = "SELECT keyname,certname,password FROM crypt_file_params "
                  " WHERE pkcs_id=:pkcs_id";
    Qry.CreateVariable("pkcs_id", otInteger, pkcs_id);
    Qry.Execute();
    if (Qry.Eof) {
        throw Exception("CryptValidateServerKey: keys not found");
    }
    pkcs.key_filename = Qry.FieldAsString("keyname");
    pkcs.cert_filename = Qry.FieldAsString("certname");
    pkcs.password = Qry.FieldAsString("password");
    string PSEpath = MESPRO_API::getPSEPath();
    PSEpath += "/pses";
    mkdir(PSEpath.c_str(), 0777);
    int i = 1;
    while (i < 100 && mkdir(string(PSEpath + "/" + IntToString(i)).c_str(), 0777))
        i++;
    if (i == 100)
        throw Exception("Can't create dir=" + string(PSEpath + "/" + IntToString(i)) + ", error=" + IntToString(errno));
    ProgTrace(TRACE5, "i=%d", i);
    PSEpath += "/" + IntToString(i);
    ProgTrace(TRACE5, "CryptValidateServerKey: PSEpath=%s", PSEpath.c_str());
    pkcs.key_filename = PSEpath + "/" + pkcs.key_filename;
    MESPRO_API::SetRandInitCallbackFun(init_rand_callback);
    GetError("PKCS7Init", MESPRO_API::PKCS7Init(0, 0));
    MESPRO_API::setInitLib(true);
    try {
        try {
            for (vector<TPSEFile>::iterator i = pkcs.pse_files.begin(); i != pkcs.pse_files.end(); i++) {
                ProgTrace(TRACE5, "filename=%s", i->filename.c_str());
                writePSEFile(PSEpath + "/", *i);
                tst();
            }
            string newpassword = getPassword();
            ProgTrace(TRACE5, "oldpassword=%s, newpassword=%s, keyfile=%s", pkcs.password.c_str(), newpassword.c_str(), pkcs.key_filename.c_str());
            if (pr_GOST)
                GetError("ChangePrivateKeyPasswordEx",
                    MESPRO_API::ChangePrivateKeyPasswordEx((char*)PSEpath.c_str(), NULL, (char*)pkcs.key_filename.c_str(),
                        (char*)pkcs.password.c_str(), (char*)newpassword.c_str()));
            else
                GetError("ChangePrivateKeyPassword",
                    MESPRO_API::ChangePrivateKeyPassword((char*)PSEpath.c_str(), (char*)pkcs.password.c_str(), (char*)newpassword.c_str()));
            tst();
            pkcs.password = newpassword;
            Qry.Clear();
            Qry.SQLText = "UPDATE crypt_files SET data=:data WHERE pkcs_id=:pkcs_id AND name=:name";
            Qry.CreateVariable("pkcs_id", otInteger, pkcs_id);
            Qry.DeclareVariable("name", otString);
            Qry.DeclareVariable("data", otLongRaw);
            for (vector<TPSEFile>::iterator i = pkcs.pse_files.begin(); i != pkcs.pse_files.end(); i++) {
                TPSEFile pse_file;
                readPSEFile(PSEpath + "/" + i->filename, i->filename, *i);
                Qry.SetVariable("name", i->filename);
                Qry.SetLongVariable("data", (void*)i->data.c_str(), i->data.size());
                ProgTrace(TRACE5, "filename=%s", i->filename.c_str());
                Qry.Execute();
            }
            tst();
            make_db_curs(
                "UPDATE crypt_file_params SET password=:password, send_count=send_count+1 WHERE pkcs_id=:pkcs_id",
                PgOra::getRWSession("CRYPT_FILES"))
                .stb()
                .bind(":password", pkcs.password)
                .bind(":pkcs_id", pkcs_id)
                .exec();
            tst();
        } catch (...) {
            DeletePSE(PSEpath, pkcs.key_filename, "");
            throw;
        }
        tst();
        DeletePSE(PSEpath, pkcs.key_filename, "");
    } catch (...) {
        MESPRO_API::PKCS7Final();
        MESPRO_API::setInitLib(false);
        throw;
    }
    MESPRO_API::PKCS7Final();
    tst();
}

#endif /*USE MESPRO*/
void CryptInterface::RequestPSE(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TPKCS pkcs;
  bool pr_grp = GetNode( "pr_grp", reqNode );
  ProgTrace( TRACE5, "IntRequestCertificateData: pr_grp=%d", pr_grp );
  CreatePSE( NodeAsString( "desk", reqNode ), pr_grp, PASSWORD_LENGTH, pkcs );
  AstraLocale::showMessage( "MSG.MESSAGE_PRO.KEY_AND_REQUEST_OK" );
}

void CryptInterface::SetCertificates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  tst();
  #ifdef USE_MESPRO
  xmlNodePtr node = GetNode( "certificates", reqNode );
  if ( !node )
    return;
  MESPRO_API::setLibNameFromAlgo( MP_KEY_ALG_NAME_GOST_12_256 );
  GetError( "PKCS7Init", MESPRO_API::PKCS7Init( 0, 0 ) );
  MESPRO_API::setInitLib( true );
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
  ProgTrace( TRACE5, "requests count=%zu", requests.size() );
  Qry.Clear();
  const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
  Qry.SQLText =
    "BEGIN "
    " INSERT INTO crypt_term_cert(id,desk_grp_id,desk,certificate,first_date,last_date,pr_denial,pkcs_id) "
    " SELECT :new_id,desk_grp_id,desk,:certificate,:first_date,:last_date,0,pkcs_id FROM crypt_term_req "
    "  WHERE id=:id; "
    " DELETE crypt_term_req WHERE id=:id; "
    "END;";
  Qry.DeclareVariable( "new_id", otInteger );
  Qry.DeclareVariable( "id", otInteger );
  Qry.DeclareVariable( "certificate", otString );
  Qry.DeclareVariable( "first_date", otDate );
  Qry.DeclareVariable( "last_date", otDate );
  Qry.SetVariable( "new_id", new_id );
  string cert;
  try {
    node = GetNode( "certificate", node );
    while ( node ) {
      cert = NodeAsString( node );
      ProgTrace( TRACE5, "certificate=%s, requests count=%zu", cert.c_str(), requests.size() );
      for ( vector<TRequest>::iterator i=requests.begin(); i!=requests.end(); i++ ) {
        int err = MESPRO_API::CertAndRequestMatchBuffer( (char*)cert.data(), cert.size(), (char*)i->cert.data(), i->cert.size() );
        if ( err ) {
          CERTIFICATE_INFO info;
          GetError( "GetCertificateInfoBufferEx", MESPRO_API::GetCertificateInfoBufferEx( (char*)cert.data(), cert.size(), &info ) );
          try {
            Qry.SetVariable( "id", i->id );
            Qry.SetVariable( "certificate", cert );
            Qry.SetVariable( "first_date", ConvertCertificateDate( info.NotBefore ) );
            Qry.SetVariable( "last_date", ConvertCertificateDate( info.NotAfter ) );
            Qry.Execute();
          }
          catch( ... ) {
            MESPRO_API::FreeCertificateInfo( &info );
            throw;
          }
          MESPRO_API::FreeCertificateInfo( &info );
          requests.erase( i );
          break;
        }
      }
      node = node->next;
    }
  }
  catch(...) {
        MESPRO_API::PKCS7Final();
        throw;
  }
  MESPRO_API::PKCS7Final();
  #endif /*USE MESPRO*/
}

void CryptInterface::CertificatesInfo(XMLRequestCtxt*, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    bool pr_search = GetNode("search", reqNode);

    DB::TQuery Qry(PgOra::getROSession("CRYPT_TERM_CERT"));

    if (GetNode("id", reqNode)) {
        Qry.SQLText =
           "SELECT id, desk, desk_grp_id, certificate, first_date, last_date, pr_denial "
           "FROM crypt_term_cert "
           "WHERE pr_denial = 0 AND id=:id";
        Qry.CreateVariable("id", otInteger, NodeAsInteger("id", reqNode));
    } else {
        TDateTime nowutc = NowUTC();
        Qry.SQLText =
           "SELECT id, desk, desk_grp_id, certificate, first_date, last_date, pr_denial "
           "FROM crypt_term_cert "
           "WHERE last_date >= :nowutc";
        Qry.CreateVariable("nowutc", otDate, nowutc);
    }

    Qry.Execute();

    if (Qry.Eof) {
        throw AstraLocale::UserException("MSG.MESSAGEPRO.NO_CERTS");
    }

    vector<TSearchData> certs;
    xmlNodePtr desksNode = NULL, grpsNode = NULL, reqsNode = NULL;

    while (!Qry.Eof) {
        TSearchData cert_search;
        cert_search.id = Qry.FieldAsInteger("id");
        if (!Qry.FieldIsNULL("desk")) {
            if (pr_search && !desksNode) {
                desksNode = NewTextChild(resNode, "desks");
            }
            cert_search.desk = Qry.FieldAsString("desk");
        }
        if (Qry.FieldIsNULL("desk")) {
            if (pr_search && !grpsNode) {
                grpsNode = NewTextChild(resNode, "grps");
            }
            cert_search.grp = composeGroupName(Qry.FieldAsInteger("desk_grp_id"));
        }
        cert_search.data = Qry.FieldAsString("certificate");
        cert_search.first_date = Qry.FieldAsDateTime("first_date");
        cert_search.last_date = Qry.FieldAsDateTime("last_date");
        cert_search.pr_denial = Qry.FieldAsInteger("pr_denial");
        certs.push_back(cert_search);
        Qry.Next();
    }

    for (vector<TSearchData>::iterator i = certs.begin(); i != certs.end(); i++) {
        if (pr_search) {
            xmlNodePtr dNode = !i->desk.empty()
                ? NewTextChild(desksNode, "desk", i->desk)
                : NewTextChild(grpsNode, "grp", i->grp);
            SetProp(dNode, "id", i->id);
        } else {
            if (!reqsNode) {
                reqsNode = NewTextChild(resNode, "certificates");
            }
            xmlNodePtr n = NewTextChild(reqsNode, "request");
            NewTextChild(n, "id", i->id);
            NewTextChild(n, "desk", i->desk);
            NewTextChild(n, "grp", i->grp);
            NewTextChild(n, "data", i->data);
            NewTextChild(n, "first_date", DateTimeToStr(i->first_date, ServerFormatDateTimeAsString));
            NewTextChild(n, "last_date", DateTimeToStr(i->last_date, ServerFormatDateTimeAsString));
            NewTextChild(n, "pr_denial", i->pr_denial);
        }
    }
}

#ifdef XP_TESTING

#include "xp_testing.h"
#include <utility>
#include <vector>
#include <string>

template <class Error, class Func, class ...Params>
bool expectToThrow(Func func, Params... params) {
  try {
    func(params...);
  } catch (Error& e) {
    return true;
  }
  return false;
}

void fill_desk_grp() {
    make_db_curs("INSERT INTO desk_grp(city, descr, grp_id) VALUES ('МОВ', 'DESCR', 123001)",  PgOra::getRWSession("DESK_GRP")).exec();
    make_db_curs("INSERT INTO desk_grp(city, descr, grp_id) VALUES ('МОВ', 'DESCR', 123002)",  PgOra::getRWSession("DESK_GRP")).exec();
    make_db_curs("INSERT INTO desk_grp(city, descr, grp_id) VALUES ('МОВ', 'DESCR', 123003)",  PgOra::getRWSession("DESK_GRP")).exec();
}

void fill_desks() {
    make_db_curs("INSERT INTO desks(code, currency, grp_id, id) VALUES ('DSK1G1', 'РУБ', 123001, 456001)", PgOra::getRWSession("DESKS")).exec();
    make_db_curs("INSERT INTO desks(code, currency, grp_id, id) VALUES ('DSK2G1', 'РУБ', 123001, 456002)", PgOra::getRWSession("DESKS")).exec();
    make_db_curs("INSERT INTO desks(code, currency, grp_id, id) VALUES ('DSK1G2', 'РУБ', 123002, 456003)", PgOra::getRWSession("DESKS")).exec();
    make_db_curs("INSERT INTO desks(code, currency, grp_id, id) VALUES ('DSK2G2', 'РУБ', 123002, 456004)", PgOra::getRWSession("DESKS")).exec();
    make_db_curs("INSERT INTO desks(code, currency, grp_id, id) VALUES ('DSK1G3', 'РУБ', 123003, 456005)", PgOra::getRWSession("DESKS")).exec();
    make_db_curs("INSERT INTO desks(code, currency, grp_id, id) VALUES ('DSK2G3', 'РУБ', 123003, 456006)", PgOra::getRWSession("DESKS")).exec();
}

void fill_crypt_file_params() {
    make_db_curs("INSERT INTO crypt_file_params(certname, desk, desk_grp_id, keyname, password, pkcs_id, send_count) "
       "VALUES ('CFP_01', 'DSK1G1', 123001, 'CFP_01_KEY', 'CFP_01_PWD', 789001, 0)", PgOra::getRWSession("CRYPT_FILE_PARAMS")).exec();
    make_db_curs("INSERT INTO crypt_file_params(certname, desk, desk_grp_id, keyname, password, pkcs_id, send_count) "
       "VALUES ('CFP_02', 'DSK2G1', 123001, 'CFP_02_KEY', 'CFP_02_PWD', 789002, 0)", PgOra::getRWSession("CRYPT_FILE_PARAMS")).exec();
    make_db_curs("INSERT INTO crypt_file_params(certname, desk, desk_grp_id, keyname, password, pkcs_id, send_count) "
       "VALUES ('CFP_03', 'DSK1G2', 123002, 'CFP_03_KEY', 'CFP_03_PWD', 789003, 0)", PgOra::getRWSession("CRYPT_FILE_PARAMS")).exec();
    make_db_curs("INSERT INTO crypt_file_params(certname, desk, desk_grp_id, keyname, password, pkcs_id, send_count) "
       "VALUES ('CFP_04',     NULL, 123002, 'CFP_04_KEY', 'CFP_04_PWD', 789004, 0)", PgOra::getRWSession("CRYPT_FILE_PARAMS")).exec();
    make_db_curs("INSERT INTO crypt_file_params(certname, desk, desk_grp_id, keyname, password, pkcs_id, send_count) "
       "VALUES ('CFP_05',     NULL, 123003, 'CFP_05_KEY', 'CFP_05_PWD', 789005, 0)", PgOra::getRWSession("CRYPT_FILE_PARAMS")).exec();
}

void fill_crypt_term_req() {
    make_db_curs("INSERT INTO crypt_term_req(desk, desk_grp_id, id, request) "
       "VALUES ('DSK1G1', 123001, 567001, 'REQUEST1G1')", PgOra::getRWSession("crypt_term_req")).exec();
    make_db_curs("INSERT INTO crypt_term_req(desk, desk_grp_id, id, request) "
       "VALUES ('DSK2G1', 123001, 567002, 'REQUEST2G1')", PgOra::getRWSession("crypt_term_req")).exec();
    make_db_curs("INSERT INTO crypt_term_req(desk, desk_grp_id, id, request) "
       "VALUES ('DSK1G2', 123002, 567003, 'REQUEST1G2')", PgOra::getRWSession("crypt_term_req")).exec();
    make_db_curs("INSERT INTO crypt_term_req(desk, desk_grp_id, id, request) "
       "VALUES (    NULL, 123002, 567004, 'REQUEST2G2')", PgOra::getRWSession("crypt_term_req")).exec();
    make_db_curs("INSERT INTO crypt_term_req(desk, desk_grp_id, id, request) "
       "VALUES (    NULL, 123003, 567005, 'REQUEST1G3')", PgOra::getRWSession("crypt_term_req")).exec();
}

void fill_crypt_term_cert() {
    const boost::posix_time::ptime     today = boost::posix_time::second_clock::local_time();
    const boost::posix_time::ptime yesterday = today - boost::gregorian::days(1);
    const boost::posix_time::ptime  tomorrow = today + boost::gregorian::days(1);

    make_db_curs("INSERT INTO crypt_term_cert(id, desk_grp_id, desk, pkcs_id, certificate, pr_denial, first_date, last_date) "
       "VALUES (777001, 123001, 'DSK1G1', 789001, 'Certificate 1', 0, :yesterday, :tomorrow)", PgOra::getRWSession("CRYPT_TERM_CERT")).bind(":yesterday", yesterday).bind(":tomorrow", tomorrow).exec();
    make_db_curs("INSERT INTO crypt_term_cert(id, desk_grp_id, desk, pkcs_id, certificate, pr_denial, first_date, last_date) "
       "VALUES (777002, 123003,     NULL, 789005, 'Certificate 2', 0, :yesterday, :tomorrow)", PgOra::getRWSession("CRYPT_TERM_CERT")).bind(":yesterday", yesterday).bind(":tomorrow", tomorrow).exec();
}

void fill_crypt_sets() {
    make_db_curs("INSERT INTO crypt_sets(id, pr_crypt, desk_grp_id, desk) "
       "VALUES (800001, 1, 123001, 'DSK1G1')", PgOra::getRWSession("CRYPT_SETS")).exec();
    make_db_curs("INSERT INTO crypt_sets(id, pr_crypt, desk_grp_id, desk) "
       "VALUES (800002, 1, 123001, 'DSK2G1')", PgOra::getRWSession("CRYPT_SETS")).exec();
    make_db_curs("INSERT INTO crypt_sets(id, pr_crypt, desk_grp_id, desk) "
       "VALUES (800003, 1, 123002, 'DSK1G2')", PgOra::getRWSession("CRYPT_SETS")).exec();
    make_db_curs("INSERT INTO crypt_sets(id, pr_crypt, desk_grp_id, desk) "
       "VALUES (800004, 1, 123002, 'DSK2G2')", PgOra::getRWSession("CRYPT_SETS")).exec();
    make_db_curs("INSERT INTO crypt_sets(id, pr_crypt, desk_grp_id, desk) "
       "VALUES (800005, 1, 123003, 'DSK1G3')", PgOra::getRWSession("CRYPT_SETS")).exec();
}


START_TEST(check_ValidatePKCSData)
{
    fill_desk_grp();
    fill_desks();
    fill_crypt_file_params();

    ValidatePKCSData("DSK1G1", true);
    ValidatePKCSData("DSK2G1", true);
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidatePKCSData), std::string, bool>(ValidatePKCSData, "DSK1G2", true)));
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidatePKCSData), std::string, bool>(ValidatePKCSData, "DSK2G2", true)));
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidatePKCSData), std::string, bool>(ValidatePKCSData, "DSK1G3", true)));
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidatePKCSData), std::string, bool>(ValidatePKCSData, "DSK2G3", true)));
    ValidatePKCSData("NODESK", true);
    ValidatePKCSData("", true);

    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidatePKCSData), std::string, bool>(ValidatePKCSData, "DSK1G1", false)));
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidatePKCSData), std::string, bool>(ValidatePKCSData, "DSK2G1", false)));
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidatePKCSData), std::string, bool>(ValidatePKCSData, "DSK1G2", false)));
    ValidatePKCSData("DSK2G2", false);
    ValidatePKCSData("DSK1G3", false);
    ValidatePKCSData("DSK2G3", false);
    ValidatePKCSData("NODESK", false);
    ValidatePKCSData("", false);
}
END_TEST

START_TEST(check_ValidateCertificateRequest)
{
    fill_desk_grp();
    fill_desks();
    fill_crypt_term_req();

    ValidateCertificateRequest("DSK1G1", true);
    ValidateCertificateRequest("DSK2G1", true);
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidateCertificateRequest), std::string, bool>(ValidateCertificateRequest, "DSK1G2", true)));
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidateCertificateRequest), std::string, bool>(ValidateCertificateRequest, "DSK2G2", true)));
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidateCertificateRequest), std::string, bool>(ValidateCertificateRequest, "DSK1G3", true)));
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidateCertificateRequest), std::string, bool>(ValidateCertificateRequest, "DSK2G3", true)));
    ValidateCertificateRequest("NODESK", true);
    ValidateCertificateRequest("", true);

    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidateCertificateRequest), std::string, bool>(ValidateCertificateRequest, "DSK1G1", false)));
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidateCertificateRequest), std::string, bool>(ValidateCertificateRequest, "DSK2G1", false)));
    fail_unless((expectToThrow<AstraLocale::UserException, decltype(ValidateCertificateRequest), std::string, bool>(ValidateCertificateRequest, "DSK1G2", false)));
    ValidateCertificateRequest("DSK2G2", false);
    ValidateCertificateRequest("DSK1G3", false);
    ValidateCertificateRequest("DSK2G3", false);
    ValidateCertificateRequest("NODESK", false);
    ValidateCertificateRequest("", false);
}
END_TEST

std::string getDeskByRequest(const std::string& request) {
    std::string desk;

    DbCpp::CursCtl cur = make_db_curs(
       "SELECT desk FROM crypt_term_req "
       "WHERE request = :request",
       PgOra::getROSession("CRYPT_TERM_REQ")
    );
    cur.bind(":request", request)
       .defNull(desk, "(null)")
       .exfet();

    return desk;
}

struct RequestCertificateData {
    int desk_grp_id;
    std::string desk;
    std::string request;
    int pkcs_id;
    bool pr_grp;
    std::string answerDesk;
};

bool checkIntPutRequestCertificate(const RequestCertificateData& requestData) {
    IntPutRequestCertificate(requestData.request, requestData.desk, requestData.pr_grp, requestData.pkcs_id);
    return requestData.answerDesk == getDeskByRequest(requestData.request);
}

START_TEST(check_IntPutRequestCertificate)
{
    fill_desk_grp();
    fill_desks();
    fill_crypt_file_params();

    RequestCertificateData firstTestData {
        123001,
       "DSK1G1",
       "FirstTestRequest",
        789001,
        false,
       "DSK1G1"
    };

    fail_unless(checkIntPutRequestCertificate(firstTestData));

    RequestCertificateData secondTestData {
        123003,
       "DSK1G3",
       "SecondTestRequest",
        789005,
        true,
       "(null)"
    };

    fail_unless(checkIntPutRequestCertificate(secondTestData));
}
END_TEST

/*
std::vector<std::pair<int, std::string>> cfprequest(const std::string& desk) {
    DbCpp::CursCtl cur = make_db_curs(
       "SELECT "
               "crypt_file_params.desk_grp_id   grp_id, "
               "crypt_file_params.desk          desk "
       "FROM "
               "crypt_file_params, "
               "desks, "
               "desk_grp "
       "WHERE "
               "send_count = 0 "
           "AND desks.code = :desk "
           "AND desks.grp_id = desk_grp.grp_id "
           "AND crypt_file_params.desk_grp_id = desk_grp.grp_id "
           "AND ( crypt_file_params.desk IS NULL "
              "OR crypt_file_params.desk = desks.code ) "
       "ORDER BY "
           "desk, "
           "grp_id",
        PgOra::getROSession("crypt_file_params")
    );
    std::pair<int, std::string> row;
    cur.bind(":desk", desk)
       .def(row.first)
       .defNull(row.second, "NULL")
       .exec();

    std::vector<std::pair<int, std::string>> result;

    while (!cur.fen()) {
        result.push_back(row);
    }

    return result;
}

START_TEST(check_ValidatePKCSData_additional)
{
    fill_desk_grp();
    fill_desks();
    fill_crypt_file_params();

    std::vector<std::pair<int, std::string>> bb = {{123001, "DSK1G1"}};
    fail_unless(cfprequest("DSK1G1") == bb);
}
END_TEST
*/

#ifdef USE_MESPRO
START_TEST(check_GetServerCertificate)
{
    const boost::posix_time::ptime     today = boost::posix_time::second_clock::local_time();
    const boost::posix_time::ptime yesterday = today - boost::gregorian::days(1);
    const boost::posix_time::ptime  tomorrow = today + boost::gregorian::days(1);

    std::string ca;
    std::string pk;
    std::string server;

    make_db_curs("INSERT INTO crypt_server(id, certificate, private_key, first_date, last_date, pr_ca, pr_denial) "
       "VALUES (777, 'Certificate', 'Private Key', :yesterday, :tomorrow, 0, 0)", PgOra::getRWSession("CRYPT_SERVER"))
       .bind(":yesterday", yesterday)
       .bind(":tomorrow", tomorrow)
       .exec();

    GetServerCertificate(ca, pk, server);
    fail_unless(ca.empty() && pk == "Private Key" && server == "Certificate");

    make_db_curs("UPDATE crypt_server SET pr_ca = 1 WHERE id = 777", PgOra::getRWSession("CRYPT_SERVER")).exec();
    ca = "";
    pk = "";
    server = "";

    GetServerCertificate(ca, pk, server);
    fail_unless(ca == "Certificate" && pk.empty() && server.empty());
}
END_TEST
#endif //USE_MESPRO

START_TEST(check_GetClientCertificate)
{
    fill_desk_grp();
    fill_desks();
    fill_crypt_file_params();
    fill_crypt_term_cert();

    std::string certificate;
    int pkcs_id = -1;

    fail_unless(GetClientCertificate(123001, 0, "DSK1G1", certificate, pkcs_id));
    fail_unless(certificate == "Certificate 1" && pkcs_id == 789001);

    fail_unless(GetClientCertificate(123003, 1, "DSK1G3", certificate, pkcs_id));
    fail_unless(certificate == "Certificate 2" && pkcs_id == 789005);
}
END_TEST

START_TEST(check_GetCryptGrp)
{
    fill_desk_grp();
    fill_desks();
    fill_crypt_file_params();
    fill_crypt_sets();

    int grp_id  = -1;
    bool pr_grp = -1;
    fail_unless(GetCryptGrp("DSK1G1", grp_id, pr_grp));
    fail_unless(grp_id == 123001 && pr_grp == 0);
    fail_unless(GetCryptGrp("DSK2G1", grp_id, pr_grp));
    fail_unless(grp_id == 123001 && pr_grp == 0);
    fail_unless(GetCryptGrp("DSK1G2", grp_id, pr_grp));
    fail_unless(grp_id == 123002 && pr_grp == 0);
    fail_unless(GetCryptGrp("DSK2G2", grp_id, pr_grp));
    fail_unless(grp_id == 123002 && pr_grp == 0);
    fail_unless(GetCryptGrp("DSK1G3", grp_id, pr_grp));
    fail_unless(grp_id == 123003 && pr_grp == 0);

    fail_if(GetCryptGrp("UNKNWN", grp_id, pr_grp));
}
END_TEST


void checkNode(xmlNodePtr node, size_t otstup = 0) {
    std::cout << std::string(otstup, ' ') << "Node: " << node->name << " contains: [" << NodeAsString(node) << "]" << std::endl;
    for (xmlNodePtr child = node->children; child; child = child->next) {
        checkNode(child, otstup + 4);
    }
}

void findNode(xmlNodePtr node, size_t otstup = 0) {
    std::cout << std::string(otstup, ' ') << "Node: " << NodeAsString(node) << std::endl;
    for (xmlNodePtr child = node->children; child; child = child->next) {
        checkNode(child, otstup + 4);
    }
}

void printIfExists(xmlNodePtr node) {
    if (nullptr != node) {
        std::cout << "Found node: " << node->name << " contains: [" << NodeAsString(node) << "]" << std::endl;
    }
}

START_TEST(check_fillNodeWithFiles)
{
    fill_desk_grp();
    fill_desks();
    fill_crypt_file_params();

    make_db_curs(
        PgOra::supportsPg("CRYPT_FILES")
         ? "INSERT INTO crypt_files(pkcs_id, name, data) VALUES (789001, 'readme.md', '\\x000102030405060708090A0B0C0D0E0FFFEFDFCFBFAF9F8F7F6F5F4F3F2F1F0F')"
         : "INSERT INTO crypt_files(pkcs_id, name, data) VALUES (789001, 'readme.md', '000102030405060708090A0B0C0D0E0FFFEFDFCFBFAF9F8F7F6F5F4F3F2F1F0F')",
        PgOra::getRWSession("CRYPT_FILES")).exec();

    int pkcs_id = 789001;

    xmlNodePtr root = xmlNewNode(nullptr, (const xmlChar*)"root");
    xmlNodePtr node = NewTextChild(root, "pkcs", pkcs_id);

    if (PgOra::supportsPg("CRYPT_FILES")) {
        fillNodeWithFiles(node, pkcs_id);
    } else {
        fillNodeWithFilesOra(node, pkcs_id);
    }

    fail_unless(789001 == NodeAsInteger(node->children));
    fail_unless(0 == strcmp("000102030405060708090A0B0C0D0E0FFFEFDFCFBFAF9F8F7F6F5F4F3F2F1F0F", NodeAsString(node->children->next->children)));
}
END_TEST

    // checkNode(root);

    // TQuery Qry(&OraSession);
    // Qry.SQLText = "SELECT SYSDATE FROM DUAL";
    // Qry.Execute();
    // if ( !Qry.Eof ) {
    //     TDateTime sysdate = Qry.FieldAsDateTime( "SYSDATE" );
    //     TDateTime now     = Now();
    //     std::cout << DateTimeToStr(sysdate) << " " << DateTimeToStr(now) << std::endl;
    // }


#define SUITENAME "crypt"
TCASEREGISTER(testInitDB, testShutDBConnection)
{
    ADD_TEST(check_ValidatePKCSData);
    ADD_TEST(check_ValidateCertificateRequest);
    ADD_TEST(check_IntPutRequestCertificate);
    #ifdef USE_MESPRO
    ADD_TEST(check_GetServerCertificate);
    #endif //USE_MESPRO
    ADD_TEST(check_GetClientCertificate);
    ADD_TEST(check_GetCryptGrp);
    ADD_TEST(check_fillNodeWithFiles);
}
TCASEFINISH;
#undef SUITENAME // "crypt"

#endif // XP_TESTING
