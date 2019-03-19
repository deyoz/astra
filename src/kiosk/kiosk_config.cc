#include "kiosk_config.h"
#define NICKNAME "DJEK"
#include "serverlib/slogger.h"
#include "serverlib/str_utils.h"
#include "oralib.h"
#include "stl_utils.h"
#include "date_time.h"
#include "xml_unit.h"
#include "date_time.h"
#include "astra_utils.h"
#include <boost/crc.hpp>
#include <iostream>
#include <fstream>
#include <boost/shared_array.hpp>
#include "boost/filesystem/operations.hpp"
#include "exceptions.h"
#include "jxtlib/xml_stuff.h"


using namespace std;
using namespace boost;
namespace fs = boost::filesystem;
using namespace EXCEPTIONS;
/*
 *
 * Общие правила.
 * параметры бывают
 * - общие (не задано Рриложение и Пульт)
 * - приложения (задана приложение не задан ПУЛЬТ)
 * - киоска (задано Приложение и задан Пульт
 * значение, когда не задано приложение и задан пульт ЗАПРЕЩЕНЫ
 */
/*
 CREATE TABLE kiosk_grp_names(
 id NUMBER(9) NOT NULL,
 name VARCHAR2(100) NOT NULL,
 descr VARCHAR(1000)
);

*/

/* киоск может входить в несколько групп
CREATE TABLE kiosk_grp(
 id NUMBER(9) NOT NULL,
 kiosk_id VARCHAR2(100) NOT NULL,
 grp_id NUMBER(9) NOT NULL
);

ALTER TABLE kiosk_grp
       ADD CONSTRAINT kiosk_grp__PK PRIMARY KEY (grp_id);

CREATE TABLE kiosk_config(
 id NUMBER(9) NOT NULL,
 name VARCHAR2(500) NOT NULL,
 value VARCHAR2(2000) NOT NULL,
 descr VARCHAR(1000),
 application VARCHAR2(100) NULL,
 kiosk_id VARCHAR2(100) NULL,
 grp_id NUMBER(9) NULL,
 pr_del NUMBER(1) NOT NULL,
 tid NUMBER(9) NOT NULL
);


ALTER TABLE kiosk_config
       ADD CONSTRAINT kiosk_config__kiosk_grp__FK
              FOREIGN KEY (grp_id)
                             REFERENCES kiosk_grp  (grp_id);

=======================================================
CREATE TABLE kiosk_aliases(
 name VARCHAR2(500) NOT NULL,
 lang VARCHAR2(100) NOT NULL,
 value VARCHAR2(2000) NOT NULL,
 descr VARCHAR(1000),
 application VARCHAR2(100) NULL,
 kiosk_id VARCHAR2(100) NULL,
 grp_id NUMBER(9) NULL,
 pr_del NUMBER(1) NOT NULL,
 tid NUMBER(9) NOT NULL
);

ALTER TABLE kiosk_aliases
       ADD CONSTRAINT kiosk_aliases__kiosk_grp__FK
              FOREIGN KEY (grp_id)
                             REFERENCES kiosk_grp  (grp_id);


*/

//!!!! не может быть так, что ид. киоска указан, а приложение не указано

enum FieldsContentEnum { elang, etids, ecrs32 };
enum DataType { eCommon, eApp, eKiosk };

struct KioskParam
{
   std::string value;
   std::string lang;
   int pr_del;
   int tid;
   int id;
};


class KioskParams
{
  private:
    std::string application;
    std::map<std::string,vector<KioskParam>> kioskParams;
    std::map<std::string,vector<KioskParam>> appParams;
    std::map<std::string,vector<KioskParam>> commonParams;
    std::string kioskId;
    int version, client_version;
    std::string table_name;
    BitSet<FieldsContentEnum> fieldsContent;
  public:
    KioskParams( const std::string &application, const std::string kioskId,
                  int client_version, const std::string &table_name,
                  const BitSet<FieldsContentEnum> &fieldsContent ) {
      this->application = application;
      this->kioskId = kioskId;
      this->client_version = client_version;
      this->version = -1;
      this->table_name = table_name;
      this->fieldsContent = fieldsContent;
    }

    int getVersion() {
      return version;
    }

    std::string getSQL() {
      string sql =
        "SELECT " + table_name + ".id, " +
        "       " + table_name + ".name, " +
        "       " + table_name + ".value, " +
        "       kiosk_app_list.name AS application, kiosk_id, grp_id, ";
      if ( fieldsContent.isFlag( elang ) ) {
        sql += " kiosk_lang.code AS lang, ";
      }
      else {
        sql += " NULL AS lang, ";
      }
      if ( fieldsContent.isFlag( etids ) ) {
        sql += " pr_del, tid ";
      }
      else {
        sql += " 0 AS pr_del, 0 AS tid ";
      }
      sql +=
        " FROM " + table_name +
        " , kiosk_app_list ";
      if ( fieldsContent.isFlag( elang ) ) {
        sql += ", kiosk_lang ";
      }
      sql +=
        " WHERE " +  table_name + ".app_id=kiosk_app_list.id(+) AND "
        "   ( kiosk_id=:kiosk_id OR "
        "         kiosk_id IS NULL OR "
        "         grp_id IN ( SELECT grp_id FROM kiosk_grp WHERE kiosk_id=:kiosk_id ) ) AND "
        "       ( kiosk_app_list.name=:application OR "
        "         kiosk_app_list.name IS NULL ) ";
      if ( fieldsContent.isFlag( elang ) ) {
        sql += " AND kiosk_lang.id= " + table_name + ".lang_id ";
      }
      if ( fieldsContent.isFlag( etids ) ) {
        if ( client_version >= 0 ) {
           sql += " AND tid > :tid ";
        }
        else {
          sql += " AND pr_del=0 ";
        }
      }
      if ( fieldsContent.isFlag( etids ) ) {
         sql += " ORDER BY tid";
      }
      else {
        sql += " ORDER BY id";
      }
      LogTrace(TRACE5)<< "sql=" << sql;
      return sql;
    }

    bool checkUpdate( int version ) {
      return this->version == version;
    }
    void fromDB() {
      TQuery Qry(&OraSession);
      Qry.SQLText = getSQL();
      Qry.CreateVariable("application", otString, application );
      Qry.CreateVariable("kiosk_id", otString, kioskId );
      if ( fieldsContent.isFlag( etids ) &&
           client_version >= 0 ) {
        Qry.CreateVariable("tid", otInteger, client_version );
        this->version = client_version;
        LogTrace(TRACE5)<< "client_version=" << client_version;
      }
      Qry.Execute();
      int field_id_idx = Qry.GetFieldIndex("id");
      int field_name_idx = Qry.GetFieldIndex("name");
      int field_value_idx = Qry.GetFieldIndex("value");
      int field_lang_idx = Qry.GetFieldIndex("lang");
      int field_application_idx = Qry.GetFieldIndex("application");
      int field_grp_id_idx = Qry.GetFieldIndex("grp_id");
      int field_kiosk_id_idx = Qry.GetFieldIndex("kiosk_id");
      int field_pr_del_idx = Qry.GetFieldIndex("pr_del");
      int field_tid_idx = Qry.GetFieldIndex("tid");
      std::ostringstream buf;
      kioskParams.clear();
      appParams.clear();
      commonParams.clear();
      for ( ;!Qry.Eof; Qry.Next() ) {
        KioskParam param;
        std::string name = Qry.FieldAsString( field_name_idx );
        param.value = Qry.FieldAsString( field_value_idx );
        if ( field_lang_idx >= 0 ) {
          param.lang = Qry.FieldAsString( field_lang_idx );
        }
        param.pr_del = Qry.FieldAsInteger( field_pr_del_idx );
        if ( field_tid_idx >= 0 ) {
          param.tid = Qry.FieldAsInteger( field_tid_idx );
        }
        param.id = Qry.FieldAsInteger( field_id_idx );
        if ( !Qry.FieldIsNULL( field_kiosk_id_idx ) ||
             !Qry.FieldIsNULL( field_grp_id_idx ) ) {
          kioskParams[ name ].push_back( param );
        }
        else
          if ( !Qry.FieldIsNULL( field_application_idx )  ) {
            appParams[ name ].push_back( param );
          }
          else {
            commonParams[ name ].push_back( param );
          }
        if ( version < param.tid ) {
          version = param.tid;
        }
        if ( fieldsContent.isFlag( ecrs32 ) ) {
          buf << name << "|" << param.lang << "|" << param.value;
        }
      }
      if ( fieldsContent.isFlag( ecrs32 ) ) {
        boost::crc_32_type result;
        result.reset();
        result.process_bytes( buf.str().data(), buf.str().size() );
        version = result.checksum();
      }
    }
    void toDB();

    void addPropType( xmlNodePtr propNode, std::string typeName, bool add_xs, bool add_xsi ) {
       if ( add_xsi ) {
         SetProp( propNode, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" );
       }
       SetProp( propNode, "xsi:type", typeName );
       if ( add_xs ) {
         SetProp( propNode,  "xmlns:xs", "http://www.w3.org/2001/XMLSchema" );
       }
    }

    void toXMLMap( const std::map<std::string,vector<KioskParam>> &params, std::string name, xmlNodePtr node ) {
      if ( params.empty() ) {
        return;
      }
      xmlNodePtr paramsNode = NewTextChild( node, name.c_str() );
      for ( auto p : params ) {
        xmlNodePtr entryNode = NewTextChild( paramsNode, "entry" );
        addPropType( NewTextChild( entryNode, "key", p.first ), "xs:string", true, true );
        xmlNodePtr valueNode;
        addPropType( valueNode=NewTextChild( entryNode, "value" ), "paramValues", false, true );
        for ( auto s : p.second ) {
          xmlNodePtr valueContentNode;
          addPropType( valueContentNode=NewTextChild( valueNode, "paramValues" ),  fieldsContent.isFlag( elang )?"paramLangValue":"paramValue", false, false );
          NewTextChild( valueContentNode, "value", s.value );
          if ( fieldsContent.isFlag( elang ) ) {
            NewTextChild( valueContentNode, "lang", s.lang );
          }
          if ( fieldsContent.isFlag( etids ) ) {
            NewTextChild( valueContentNode, "pr_del", s.pr_del );
            //NewTextChild( valNode, "tid", param.tid );
          }
          NewTextChild( valueContentNode, "id", s.id );
        }
      }
    }

    void toXML( xmlNodePtr node ) {
      NewTextChild( node, "version", getVersion() );
      if ( checkUpdate( client_version ) ) {
        return;
      }
      toXMLMap( commonParams, "commonParams", node );
      toXMLMap( appParams, "appsParams", node );
      toXMLMap( kioskParams, "kioskParams", node );
    }

    void fromXML( xmlNodePtr resNode );
};
//================================================================

namespace KIOSKCONFIG
{

void KioskRequestInterface::AppParamsKiosk(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
   BitSet<FieldsContentEnum> fieldsContent;
   fieldsContent.setFlag( etids );
   KioskParams configParams( NodeAsString( "application", reqNode ),
                             NodeAsString( "kioskId", reqNode ),
                             NodeAsInteger( "version", reqNode, 0 ),
                             "kiosk_config",
                             fieldsContent );
   configParams.fromDB();
   xmlNodePtr node = NewTextChild( resNode, "AppParamsKiosk" );
   configParams.toXML( node );
}

void KioskRequestInterface::AppAliasesKiosk(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
   BitSet<FieldsContentEnum> fieldsContent;
   fieldsContent.setFlag( elang );
   fieldsContent.setFlag( etids );
   KioskParams aliasesParams( NodeAsString( "application", reqNode ),
                              NodeAsString( "kioskId", reqNode ),
                              NodeAsInteger( "version", reqNode, 0 ),
                              "kiosk_aliases",
                              fieldsContent );
   aliasesParams.fromDB();
   xmlNodePtr node = NewTextChild( resNode, "AppAliasesKiosk" );
   aliasesParams.toXML( node );
}


    struct TAlias {
        string name;
        string descr;
        std::map<string, string> locale;
    };

   void alias_to_db_help(const char *name)
    {
        printf("  %-15.15s ", name);
        puts("<path to alias xml file> <application name>");
    }


    void usage(string name, string what)
    {
        cout
            << "Error: " << what << endl
            << "Usage:" << endl;
        alias_to_db_help(name.c_str());
        cout
            << "Example:" << endl
            << "  " << name << " aliases_kiosk" << endl;

    }


    int alias_to_db(int argc, char **argv)
    {
        try {
            fs::path full_path;
            string application;
            if(argc > 1)
                full_path = fs::system_complete(fs::path(argv[1]));
            else
                throw Exception("file not specified");

            if (argc > 2)
                application = argv[2];
            else
                throw Exception("application name not specified");

            if(not fs::is_regular_file(full_path))
                throw Exception("%s not a regular file", full_path.string().c_str());

            ostringstream fname;
            fname << full_path.filename().c_str();

            ifstream in(full_path.string().c_str());
            if(!in.good())
                throw Exception("Cannot open file %s", fname.str().c_str());
            cout << "run..";
            string content = string( std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() );
            boost::replace_all(content, "&#xd;", "&lt;br&gt;");
            LogTrace(TRACE5) << content;
            XMLDoc doc(content);
            if(not doc.docPtr())
                throw Exception("cannot parse XML content");
            xmlNodePtr returnNode = doc.docPtr()->children;
            for(; returnNode; returnNode = returnNode->children)
                if(not strcmp((const char*)returnNode->name, "return")) break;
            if(not returnNode)
                throw Exception("return node not found");
            xmlNodePtr curNode = returnNode->children;
            int count = 0;
            std::vector<TAlias> aliases;
            for(; curNode; curNode = curNode->next, count++) {
                std::string appCodeStr = NodeAsString("@appCode", curNode);
                if ( appCodeStr.find( "kiosk") == std::string::npos ||
                     appCodeStr.find( "CUSS")  == std::string::npos ) {
                  continue;
                }
                LogTrace(TRACE5) << NodeAsString("@appCode", curNode) << "  " << NodeAsString("@name", curNode);
                TAlias alias;
                alias.descr = NodeAsString("@description", curNode);
                alias.name = NodeAsString("@name", curNode);

                xmlNodePtr valueNode = curNode->children;
                for(; valueNode; valueNode = valueNode->next) {
                    alias.locale.insert(make_pair(
                                NodeAsString("@lang", valueNode),
                                NodeAsString(valueNode)));
                }
                aliases.push_back( alias );
            }

            LogTrace(TRACE5) << count << " aliases processed";
            TQuery Qry(&OraSession);
            Qry.SQLText = "CREATE TABLE tmp_aliases( APP VARCHAR2(200), ID VARCHAR2(2000) NOT NULL, LANG VARCHAR2(2) NOT NULL, TEXT VARCHAR2(2000) )";
            try {
             Qry.Execute();
            }
            catch(...) {};
            try {
              Qry.SQLText = "INSERT INTO tmp_aliases( APP, ID, LANG, TEXT) VALUES( :APP, :ID, :LANG, :TEXT)";
              Qry.CreateVariable( "APP", otString, application );
              Qry.DeclareVariable( "ID", otString );
              Qry.DeclareVariable( "LANG", otString );
              Qry.DeclareVariable( "TEXT", otString );
              for ( auto p : aliases ) {
              LogTrace(TRACE5) << "lexema=" <<  UTF8toCP866( p.name );
                Qry.SetVariable( "ID", UTF8toCP866( p.name ) );
                for ( auto l : p.locale ) {
                  if ( l.first == "ru" ||
                       l.first == "en" ) {
                    Qry.SetVariable( "LANG", l.first );
                    LogTrace(TRACE5) << "value=" << l.second;
                     LogTrace(TRACE5) << "value=" << UTF8toCP866( l.second );
                    Qry.SetVariable( "TEXT", UTF8toCP866( l.second ) );
                    Qry.Execute();
                  }
                }
              }
            }
            catch(...) {
              Qry.Clear();
              Qry.SQLText = "DROP TABLE tmp_aliases";
              Qry.Execute();
              throw;
            }

        } catch(Exception &E) {
            usage(argv[0], E.what());
            return 1;
        }
        return 0;
    }

/*
kiosk.CUSS.PrintSelectionScreen.title
kiosk.CUSS.RegistrationScreen.title
kiosk.CUSS.SeatSelectionScreen.title
kiosk.CUSS.SelectLanguageScreen.ruWelcome
kiosk.CUSS.StatusArrivalScreen.withVisa
kiosk.CUSS.InfoTypeSelectionScreen.rePrintingNumber
kiosk.CUSS.InfoTypeSelectionScreen.boardingPassNumberDetails
kiosk.CUSS.InfoTypeSelectionScreen.docNumber
kiosk.CUSS.InfoTypeSelectionScreen.documentNumberDetails
kiosk.CUSS.InfoTypeSelectionScreen.rePrintingNumber
kiosk.CUSS.InfoTypeSelectionScreen.ticketNumberDetails
kiosk.CUSS.LastScreen.thank
kiosk.CUSS.general.endRead
kiosk.CUSS.ParamInputScreen.del
kiosk.CUSS.ParamInputScreen.language
kiosk.CUSS.ParamsInputScreen.contextHelp.document
kiosk.CUSS.RegistrationScreen.title
kiosk.CUSS.SelectLanguageScreen.ruWelcome
kiosk.CUSS.SeatSelectionScreen.occupiedPlace
kiosk.CUSS.general.read
kiosk.CUSS.ParamsInputScreen.contextHelp.lastName
kiosk.CUSS.ParamInputScreen.del
kiosk.CUSS.ParamInputScreen.language
kiosk.CUSS.ParamsInputScreen.contextHelp.date
kiosk.CUSS.ParamsInputScreen.contextHelp.date
kiosk.CUSS.ParamsInputScreen.contextHelp.flight
kiosk.CUSS.InfoTypeSelectionScreen.receiptRouteNumberDetails
kiosk.CUSS.LastScreen.thank
kiosk.CUSS.general.read
kioskCUSS.FlightSegmentScreen.checkinClosing
kiosk.CUSS.InfoTypeSelectionScreen.documentNumberDetails
kiosk.CUSS.ParamInputScreen.del
kiosk.CUSS.ParamInputScreen.language
kiosk.CUSS.PrintSelectionScreen.title
kiosk.CUSS.RegistrationScreen.statusNumber
kiosk.CUSS.SeatSelectionScreen.title
kiosk.CUSS.SelectLanguageScreen.ruWelcome
kiosk.CUSS.errorDipCardreaderError
kiosk.CUSS.general.read
kiosk.CUSS.InfoTypeSelectionScreen.ticketNumber
kiosk.CUSS.ParamsInputScreen.contextHelp.flight
kiosk.CUSS.ParamsInputScreen.contextHelp.document
kiosk.CUSS.ParamsInputScreen.contextHelp.document
kiosk.CUSS.RegistrationScreen.statusNumber
kiosk.CUSS.RegistrationScreen.statusNumber
kiosk.CUSS.SeatSelectionScreen.occupiedPlace
kiosk.CUSS.StatusArrivalScreen.withVisa
kiosk.CUSS.InfoTypeSelectionScreen.docNumber
kiosk.CUSS.InfoTypeSelectionScreen.documentNumberDetails
kiosk.CUSS.InfoTypeSelectionScreen.orderNumber
kiosk.CUSS.InfoTypeSelectionScreen.rePrintingNumberDetails
kiosk.CUSS.LastScreen.problems
kiosk.CUSS.RegistrationScreen.register
kiosk.CUSS.ParamsInputScreen.contextHelp.lastName
kiosk.CUSS.PrintingScreen.niceFlight
kiosk.CUSS.SeatSelectionScreen.selectingPlace
kiosk.CUSS.SelectLanguageScreen.welcomeRus
kiosk.CUSS.errorDipCardreaderError
kiosk.CUSS.errorDipCardreaderError
kiosk.CUSS.InfoTypeSelectionScreen.receiptRouteNumberDetails
kiosk.CUSS.InfoTypeSelectionScreen.documentNumberDetails
kiosk.CUSS.errorDipCardreaderError
kioskCUSS.FlightSegmentScreen.checkinClosing
kioskCUSS.PrintingScreen.boardingClosing
kiosk.CUSS.SeatSelectionScreen.selectingPlace
kiosk.CUSS.SeatSelectionScreen.seleсtedPlace
kiosk.CUSS.general.nextButton
kiosk.CUSS.LuggageRulesCaseScreen.title
kiosk.CUSS.ParamsInputScreen.contextHelp.date
kiosk.CUSS.ParamsInputScreen.contextHelp.ticket
kiosk.CUSS.ParamsInputScreen.contextHelp.ticket
kiosk.CUSS.ParamsInputScreen.contextHelp.ticket
kiosk.CUSS.RegistrationScreen.register
kiosk.CUSS.InfoTypeSelectionScreen.boardingPassNumberDetails
kiosk.CUSS.LuggageRulesCaseScreen.title
kiosk.CUSS.LuggageRulesCaseScreen.title
kiosk.CUSS.InfoTypeSelectionScreen.documentNumber
kiosk.CUSS.InfoTypeSelectionScreen.orderNumber
kiosk.CUSS.LuggageRulesCaseScreen.title
kiosk.CUSS.PrintingScreen.niceFlight
kiosk.CUSS.PrintingScreen.niceFlight
kiosk.CUSS.SeatSelectionScreen.occupiedPlace
kiosk.CUSS.general.endRead
kiosk.CUSS.LuggageRulesCaseScreen.title
kiosk.CUSS.ParamsInputScreen.contextHelp.flight
kiosk.CUSS.ParamsInputScreen.contextHelp.ticket
kiosk.CUSS.ParamsInputScreen.contextHelp.order
kiosk.CUSS.PrintingScreen.printigFault
kiosk.CUSS.RegistrationScreen.title
kiosk.CUSS.SeatSelectionScreen.seleсtedPlace
kiosk.CUSS.StatusArrivalScreen.withoutVisa
kiosk.CUSS.StatusArrivalScreen.withoutVisa
kiosk.CUSS.InfoTypeSelectionScreen.documentNumberDetails
kiosk.CUSS.InfoTypeSelectionScreen.rePrintingNumber
kiosk.CUSS.InfoTypeSelectionScreen.ticketNumber
kiosk.CUSS.LastScreen.problems
kiosk.CUSS.InfoTypeSelectionScreen.orderNumberDetails
kiosk.CUSS.general.nextPage
kioskCUSS.PrintingScreen.boardingClosing
kiosk.CUSS.ParamsInputScreen.contextHelp.lastName
kiosk.CUSS.RegistrationScreen.title
kiosk.CUSS.SelectLanguageScreen.welcomeRus
kiosk.CUSS.general.nextButton
kiosk.CUSS.StatusArrivalScreen.withVisa
kiosk.CUSS.StatusArrivalScreen.withoutVisa
kiosk.CUSS.ParamsInputScreen.contextHelp.date
kiosk.CUSS.ParamInputScreen.language
kiosk.CUSS.ParamsInputScreen.contextHelp.flight
kiosk.CUSS.ParamsInputScreen.contextHelp.order
kiosk.CUSS.InfoTypeSelectionScreen.documentNumber
kiosk.CUSS.InfoTypeSelectionScreen.documentNumber
kiosk.CUSS.InfoTypeSelectionScreen.orderNumberDetails
kiosk.CUSS.LastScreen.thank
kiosk.CUSS.LastScreen.thank
kiosk.CUSS.InfoTypeSelectionScreen.rePrintingNumber
kiosk.CUSS.general.endRead
kiosk.CUSS.general.read
kiosk.CUSS.PrintingScreen.printigFault
kiosk.CUSS.SelectLanguageScreen.welcomeRus
kiosk.CUSS.StatusArrivalScreen.withVisa
kiosk.CUSS.StatusArrivalScreen.withoutVisa
kiosk.CUSS.general.read
kiosk.CUSS.ParamsInputScreen.contextHelp.order
kiosk.CUSS.ParamInputScreen.del
kiosk.CUSS.PrintingScreen.niceFlight
kiosk.CUSS.SelectLanguageScreen.ruWelcome
kiosk.CUSS.SelectLanguageScreen.welcomeRus
kiosk.CUSS.InfoTypeSelectionScreen.ticketNumberDetails
kiosk.CUSS.InfoTypeSelectionScreen.rePrintingNumberDetails
kiosk.CUSS.general.nextButton
kiosk.CUSS.general.nextPage
kioskCUSS.PrintingScreen.boardingClosing
kiosk.CUSS.ParamsInputScreen.contextHelp.lastName
kiosk.CUSS.RegistrationScreen.statusNumber
kiosk.CUSS.InfoTypeSelectionScreen.documentNumber
kiosk.CUSS.ParamsInputScreen.contextHelp.document
kiosk.CUSS.ParamsInputScreen.contextHelp.document
kiosk.CUSS.ParamsInputScreen.contextHelp.order


select id, c from
( select id, lang, text, count(*) AS c, complete AS d from tmp_aliases
group by id, lang, text, complete
)
where c != 6 and d =0

AND
id not in
(select distinct name from kiosk_aliases )

alter table tmp_aliases add complete number(1);


select distinct  id, lang, text, app from tmp_aliases
where id='kiosk.CUSS.SeatSelectionScreen.title'


update tmp_aliases set complete=1 where id='kiosk.CUSS.RegistrationScreen.title';
commit;

*/
}
