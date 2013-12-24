#include <string>
#include "apis.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "apis_edi_file.h"
#include "misc.h"
#include "passenger.h"
#include "tlg/tlg.h"
#include "trip_tasks.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

#define ENDL "\r\n"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

namespace APIS
{

const set<string> &customsUS()
{
  static bool init=false;
  static set<string> depend;
  if (!init)
  {
    TQuery Qry(&OraSession);
    GetCustomsDependCountries("ЮС", depend, Qry);
    init=true;
  };
  return depend;
};

void GetCustomsDependCountries(const string &regul,
                               set<string> &depend,
                               TQuery &Qry)
{
  depend.clear();
  depend.insert(regul);
  const char *sql =
    "SELECT country_depend FROM apis_customs WHERE country_regul=:country_regul";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("country_regul",otString);
  };
  Qry.SetVariable("country_regul", regul);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    depend.insert(Qry.FieldAsString("country_depend"));
};

string GetCustomsRegulCountry(const string &depend,
                              TQuery &Qry)
{
  const char *sql =
    "SELECT country_regul FROM apis_customs WHERE country_depend=:country_depend";
  if (strcmp(Qry.SQLText.SQLText(),sql)!=0)
  {
    Qry.Clear();
    Qry.SQLText=sql;
    Qry.DeclareVariable("country_depend",otString);
  };
  Qry.SetVariable("country_depend", depend);
  Qry.Execute();
  if (!Qry.Eof)
    return Qry.FieldAsString("country_regul");
  else
    return depend;
};

bool isValidGender(const string &fmt, const string &pax_doc_gender, const string &pax_name)
{
  if (fmt=="CSV_CZ" || fmt=="EDI_CZ" || fmt=="EDI_US")
  {
    int is_female=CheckIn::is_female(pax_doc_gender, pax_name);
    if (is_female==NoExists) return false;
  };
  return true;
};

bool isValidDocType(const string &fmt, const TPaxStatus &status, const string &doc_type)
{
  if (fmt=="EDI_CZ")
  {
    if (!(doc_type=="P" ||
          doc_type=="A" ||
          doc_type=="C" ||
          (status!=psCrew &&
           (doc_type=="B" ||
            doc_type=="T" ||
            doc_type=="N" ||
            doc_type=="M")) ||
          (status==psCrew &&
           (doc_type=="L")))) return false;
  };
  if (fmt=="EDI_CN")
  {
    /*
    1. Official travel documents:
      P for Passport
      PT for P.R. China Travel Permit
      PL for P.R. China Exit and Entry Permit
      W for Travel Permit To and From HK and Macao
      A for Travel Permit To and From HK and Macao for Public Affairs
      Q for Travel Permit To HK and Macao
      C for Travel Permit of HK and Macao Residents To and From Mainland
      D for Travel Permit of Mainland Residents To and From Taiwan
      T for Travel Permit of Taiwan Residents To and From Mainland
      S for Seafarer's Passport
      F for approved non-standard identity documents used for travel
    2. Other documents:
      V for Visa
      AC for Crew Member Certificate

    Visa and Crew Member Certificate are optional
    */
    if (!(doc_type=="P" ||
          doc_type=="PT" ||
          doc_type=="PL" ||
          doc_type=="W" ||
          doc_type=="A" ||
          doc_type=="Q" ||
          doc_type=="C" ||
          doc_type=="D" ||
          doc_type=="T" ||
          doc_type=="S" ||
          doc_type=="F" ||
          (status==psCrew &&
           (doc_type=="AC")))) return false;
  };
  if (fmt=="EDI_IN")
  {
    /*
    ICAO 9303 Document Types
      P Passport
      V Visa
      A Identity Card (exact use defined by the Issuing State)
      C Identity Card (exact use defined by the Issuing State)
      I Identity Card (exact use defined by the Issuing State)
      AC Crew Member Certificate
      IP Passport Card
    Other Document Types
      F Approved non-standard identity documents used for travel
      (exact use defined by the Issuing State).
    */
    if (!(doc_type=="P" ||
          doc_type=="A" ||
          doc_type=="C" ||
          doc_type=="I" ||
          doc_type=="IP" ||
          doc_type=="F" ||
          (status==psCrew &&
           (doc_type=="AC")))) return false;
  };
  if (fmt=="EDI_US")
  {
    /*
    P - Passport
    C - Permanent resident card
    A - Resident alien card
    M - US military ID.
    T - Re-entry permit or refugee permit
    IN - NEXUS card
    IS - SENTRI card
    F - Facilitation card
    V - U.S. Non-Immigrant Visa (Secondary Document Only)
    L - Pilots license (crew members only)
    */
    if (!(doc_type=="P" ||
          doc_type=="C" ||
          doc_type=="A" ||
          (status!=psCrew &&
           (doc_type=="M" ||
            doc_type=="T" ||
            doc_type=="IN" ||
            doc_type=="IS" ||
            doc_type=="F")) ||
          (status==psCrew &&
           (doc_type=="L")))) return false;
  };

  if (fmt=="EDI_UK")
  {
    /*
    P - Passport
    G - Group Passport
    A - National Identity Card or Resident Card. Exact use defined by issuing state
    C - National Identity Card or Resident Card. Exact use defined by issuing state
    I - National Identity Card or Resident Card. Exact use defined by issuing state
    M - Military Identification
    D - Diplomatic Identification
    AC - Crew Member Certificate
    IP - Passport Card
    F -  Other approved non-standard identity documents used for travel (as per Authority regulations)
    */
    if (!(doc_type=="P" ||
          doc_type=="G" ||
          doc_type=="A" ||
          doc_type=="C" ||
          doc_type=="I" ||
          doc_type=="M" ||
          doc_type=="D" ||
          doc_type=="IP" ||
          doc_type=="F" ||
          (status==psCrew &&
           (doc_type=="AC")))) return false;
  };
  return true;
};

};

const char* APIS_PARTY_INFO()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("APIS_PARTY_INFO","");
  return VAR.c_str();
};

#define MAX_PAX_PER_EDI_PART 15
#define MAX_LEN_OF_EDI_PART 3000

class TAirlineOfficeInfo
{
  public:
    string contact_name;
    string phone;
    string fax;
};

void GetAirlineOfficeInfo(const string &airline,
                          const string &country,
                          const string &airp,
                          list<TAirlineOfficeInfo> &offices)
{
  offices.clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT airp, contact_name, phone, fax "
    "FROM airline_offices "
    "WHERE airline=:airline AND country=:country AND "
    "      (airp IS NULL OR airp=:airp) AND to_apis<>0 "
    "ORDER BY airp NULLS LAST";
  Qry.CreateVariable("airline", otString, airline);
  Qry.CreateVariable("country", otString, country);
  Qry.CreateVariable("airp", otString, airp);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TAirlineOfficeInfo info;
    info.contact_name=Qry.FieldAsString("contact_name");
    info.phone=Qry.FieldAsString("phone");
    info.fax=Qry.FieldAsString("fax");
    offices.push_back(info);
  };

  vector<string> strs;
  SeparateString(string(APIS_PARTY_INFO()), ':', strs);
  if (!strs.empty())
  {
    TAirlineOfficeInfo info;
    vector<string>::const_iterator i;
    i=strs.begin();
    if (i!=strs.end()) info.contact_name=(*i++);
    if (i!=strs.end()) info.phone=(*i++);
    if (i!=strs.end()) info.fax=(*i++);
    offices.push_back(info);
  };
};


string NormalizeDocNo(const string& str, bool try_keep_only_digits)
{
  string result;
  string max_num, curr_num;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i)
    if (IsDigitIsLetter(*i)) result+=*i;
  if (try_keep_only_digits)
  {
    for(string::const_iterator i=result.begin(); i!=result.end(); ++i)
    {
      if (IsDigit(*i)) curr_num+=*i;
      if (IsLetter(*i) && !curr_num.empty())
      {
        if (curr_num.size()>max_num.size()) max_num=curr_num;
        curr_num.clear();
      };
    };
    if (curr_num.size()>max_num.size()) max_num=curr_num;
  };

  return (max_num.size()<6)?result:max_num;
};

string HyphenToSpace(const string& str)
{
  string result;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i)
    result+=(*i=='-')?' ':*i;
  return result;
};

void create_apis_nosir_help(const char *name)
{
  printf("  %-15.15s ", name);
  puts("<points.point_id>  ");
};

int create_apis_nosir(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  int point_id=ASTRA::NoExists;
  try
  {
    //проверяем параметры
    if (argc<2) throw EConvertError("wrong parameters");
    point_id = ToInt(argv[1]);
    Qry.Clear();
    Qry.SQLText="SELECT point_id FROM points WHERE point_id=:point_id";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if (Qry.Eof) throw EConvertError("point_id not found");
  }
  catch(EConvertError &E)
  {
    printf("Error: %s\n", E.what());
    if (argc>0)
    {
      puts("Usage:");
      create_apis_nosir_help(argv[0]);
      puts("Example:");
      printf("  %s 1234567\n",argv[0]);
    };
    return 1;
  };

  if (init_edifact()<0) throw Exception("'init_edifact' error");
  create_apis_file(point_id, "");

  puts("create_apis successfully completed");
  return 0;
};

bool create_apis_file(int point_id, const string& task_name)
{
  bool result=false;
  try
  {
  	TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT airline, flt_no, suffix, airp, scd_out, act_out, "
      "       point_num, first_point, pr_tranzit, "
      "       country "
      "FROM points,airps,cities "
      "WHERE points.airp=airps.code AND airps.city=cities.code AND "
      "      point_id=:point_id AND points.pr_del=0 AND points.pr_reg<>0 ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) return result;

    TAirlinesRow &airline = (TAirlinesRow&)base_tables.get("airlines").get_row("code",Qry.FieldAsString("airline"));
    string country_dep = Qry.FieldAsString("country");

    TTripRoute route;
    route.GetRouteAfter(NoExists,
                        point_id,
                        Qry.FieldAsInteger("point_num"),
                        Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
                        Qry.FieldAsInteger("pr_tranzit")!=0,
                        trtNotCurrent, trtNotCancelled);

    TQuery RouteQry(&OraSession);
    RouteQry.SQLText=
      "SELECT airp,scd_in,country "
      "FROM points,airps,cities "
      "WHERE points.airp=airps.code AND airps.city=cities.code AND point_id=:point_id";
    RouteQry.DeclareVariable("point_id",otInteger);

    TQuery ApisSetsQry(&OraSession);
    ApisSetsQry.Clear();
    ApisSetsQry.SQLText=
      "SELECT dir,edi_addr,edi_own_addr,format "
      "FROM apis_sets "
      "WHERE airline=:airline AND country_dep=:country_dep AND country_arv=:country_arv AND pr_denial=0";
    ApisSetsQry.CreateVariable("airline", otString, airline.code);
    ApisSetsQry.CreateVariable("country_dep", otString, country_dep);
    ApisSetsQry.DeclareVariable("country_arv", otString);

    TQuery PaxQry(&OraSession);
    PaxQry.SQLText=
      "SELECT pax.pax_id, pax.surname, pax.name, pax.pr_brd, "
      "       tckin_segments.airp_arv AS airp_final, pax_grp.status "
      "FROM pax_grp,pax,tckin_segments "
      "WHERE pax_grp.grp_id=pax.grp_id AND "
      "      pax_grp.grp_id=tckin_segments.grp_id(+) AND tckin_segments.pr_final(+)<>0 AND "
      "      pax_grp.point_dep=:point_dep AND pax_grp.point_arv=:point_arv AND pr_brd IS NOT NULL AND "
      "      (pax.name IS NULL OR pax.name<>'CBBG')";
    PaxQry.CreateVariable("point_dep",otInteger,point_id);
    PaxQry.DeclareVariable("point_arv",otInteger);

    TQuery CustomsQry(&OraSession);

    map<string /*country_regul_arv*/, string /*first airp_arv*/> CBPAirps;

    for(TTripRoute::const_iterator r=route.begin(); r!=route.end(); r++)
    {
      //получим информацию по пункту маршрута
      RouteQry.SetVariable("point_id",r->point_id);
      RouteQry.Execute();
      if (RouteQry.Eof) continue;

      TCountriesRow &country_arv = (TCountriesRow&)base_tables.get("countries").get_row("code",RouteQry.FieldAsString("country"));

      string country_regul_dep=APIS::GetCustomsRegulCountry(country_dep, CustomsQry);
      string country_regul_arv=APIS::GetCustomsRegulCountry(country_arv.code, CustomsQry);
      bool use_us_customs_tasks=country_regul_dep==US_CUSTOMS_CODE || country_regul_arv==US_CUSTOMS_CODE;
      map<string, string>::iterator iCBPAirp=CBPAirps.find(country_regul_arv);
      if (iCBPAirp==CBPAirps.end())
        iCBPAirp=CBPAirps.insert(make_pair(country_regul_arv, RouteQry.FieldAsString("airp"))).first;
      if (iCBPAirp==CBPAirps.end()) throw Exception("iCBPAirp==CBPAirps.end()");

      if (!(task_name.empty() ||
            (use_us_customs_tasks &&
             (task_name==BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL ||
              task_name==BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL)) ||
            (!use_us_customs_tasks &&
             (task_name==ON_TAKEOFF ||
              task_name==ON_CLOSE_CHECKIN)))) continue;
      //получим информацию по настройке APIS
      ApisSetsQry.SetVariable("country_arv",country_arv.code);
      ApisSetsQry.Execute();
      if (!ApisSetsQry.Eof)
      {
        if (airline.code_lat.empty()) throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
        int flt_no=Qry.FieldAsInteger("flt_no");
        string suffix;
        if (!Qry.FieldIsNULL("suffix"))
        {
          TTripSuffixesRow &suffixRow = (TTripSuffixesRow&)base_tables.get("trip_suffixes").get_row("code",Qry.FieldAsString("suffix"));
          if (suffixRow.code_lat.empty()) throw Exception("suffixRow.code_lat empty (code=%s)",suffixRow.code.c_str());
          suffix=suffixRow.code_lat;
        };
        TAirpsRow &airp_dep = (TAirpsRow&)base_tables.get("airps").get_row("code",Qry.FieldAsString("airp"));
        if (airp_dep.code_lat.empty()) throw Exception("airp_dep.code_lat empty (code=%s)",airp_dep.code.c_str());
        string tz_region=AirpTZRegion(airp_dep.code);
        if (Qry.FieldIsNULL("scd_out")) throw Exception("scd_out empty (airp_dep=%s)",airp_dep.code.c_str());
        TDateTime scd_out_local	= UTCToLocal(Qry.FieldAsDateTime("scd_out"),tz_region);

        bool final_apis=(task_name==ON_TAKEOFF || (task_name.empty() && !Qry.FieldIsNULL("act_out")));

        TAirpsRow &airp_arv = (TAirpsRow&)base_tables.get("airps").get_row("code",RouteQry.FieldAsString("airp"));
      	if (airp_arv.code_lat.empty()) throw Exception("airp_arv.code_lat empty (code=%s)",airp_arv.code.c_str());
      	tz_region=AirpTZRegion(airp_arv.code);
        if (RouteQry.FieldIsNULL("scd_in")) throw Exception("scd_in empty (airp_arv=%s)",airp_arv.code.c_str());
        TDateTime scd_in_local = UTCToLocal(RouteQry.FieldAsDateTime("scd_in"),tz_region);

        TAirpsRow &airp_cbp = (TAirpsRow&)base_tables.get("airps").get_row("code",iCBPAirp->second);
      	if (airp_cbp.code_lat.empty()) throw Exception("airp_cbp.code_lat empty (code=%s)",airp_cbp.code.c_str());

        for(;!ApisSetsQry.Eof;ApisSetsQry.Next())
        {
          string fmt=ApisSetsQry.FieldAsString("format");

          if (task_name==ON_CLOSE_CHECKIN && fmt!="EDI_UK") continue;

          string airline_country;
          if (fmt=="TXT_EE")
          {
            if (airline.city.empty()) throw Exception("airline.city empty (code=%s)",airline.code.c_str());
            TCitiesRow &airlineCityRow = (TCitiesRow&)base_tables.get("cities").get_row("code",airline.city);
            TCountriesRow &airlineCountryRow = (TCountriesRow&)base_tables.get("countries").get_row("code",airlineCityRow.country);
   	    	  if (airlineCountryRow.code_iso.empty()) throw Exception("airlineCountryRow.code_iso empty (code=%s)",airlineCityRow.country.c_str());
   	    	  airline_country = airlineCountryRow.code_iso;
          };

          string lst_type_extra;
          if (fmt=="EDI_UK")
          {
            lst_type_extra=(final_apis?"DC:1.0":"CI:1.0");
          };

          Paxlst::PaxlstInfo FPM(Paxlst::PaxlstInfo::FlightPassengerManifest, lst_type_extra);
        	Paxlst::PaxlstInfo FCM(Paxlst::PaxlstInfo::FlightCrewManifest, lst_type_extra);

          if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US" || fmt=="EDI_UK")
          {
            for(int pass=0; pass<2; pass++)
            {
              Paxlst::PaxlstInfo& paxlstInfo=(pass==0?FPM:FCM);

              if (fmt=="EDI_CN")
                paxlstInfo.settings().setRespAgnCode("ZZZ");
              if (fmt=="EDI_UK")
                paxlstInfo.settings().setRespAgnCode("109");

              if (fmt=="EDI_IN")
                paxlstInfo.settings().setAppRef("");
              if (fmt=="EDI_US")
                paxlstInfo.settings().setAppRef("USADHS");
              if (fmt=="EDI_UK")
                paxlstInfo.settings().setAppRef("UKBAOP");

              if (fmt=="EDI_IN" || fmt=="EDI_US" || fmt=="EDI_UK")
                paxlstInfo.settings().setMesRelNum("05B");

              paxlstInfo.settings().setViewUNGandUNE(true/*!(fmt=="EDI_IN" || fmt=="EDI_US")*/);

              //информация о преставительства a/к
              list<TAirlineOfficeInfo> offices;
              GetAirlineOfficeInfo(airline.code, country_arv.code, airp_arv.code, offices);
              if (!offices.empty())
              {
                paxlstInfo.setPartyName(offices.begin()->contact_name);
                if (fmt=="EDI_CN")
                {
                  paxlstInfo.setPhone(HyphenToSpace(offices.begin()->phone));
                  paxlstInfo.setFax(HyphenToSpace(offices.begin()->fax));
                }
                else
                {
                  paxlstInfo.setPhone(offices.begin()->phone);
                  paxlstInfo.setFax(offices.begin()->fax);
                };
              };

              vector<string> strs;
              vector<string>::const_iterator i;

              SeparateString(string(ApisSetsQry.FieldAsString("edi_own_addr")), ':', strs);
              i=strs.begin();
              if (i!=strs.end()) paxlstInfo.setSenderName(*i++);
              if (i!=strs.end()) paxlstInfo.setSenderCarrierCode(*i++);

              SeparateString(string(ApisSetsQry.FieldAsString("edi_addr")), ':', strs);
              i=strs.begin();
              if (i!=strs.end()) paxlstInfo.setRecipientName(*i++);
              if (i!=strs.end()) paxlstInfo.setRecipientCarrierCode(*i++);

              ostringstream flight;
              flight << airline.code_lat << flt_no << suffix;

              string iataCode;
              if (fmt=="EDI_UK")
                iataCode=Paxlst::createIataCode(flight.str(),scd_in_local,"yyyymmddhhnn");
              else
                iataCode=Paxlst::createIataCode(flight.str(),scd_in_local,"/yymmdd/hhnn");
              paxlstInfo.setIataCode( iataCode );
              if (fmt=="EDI_IN" || fmt=="EDI_US" || fmt=="EDI_UK")
                paxlstInfo.setCarrier(airline.code_lat);
              paxlstInfo.setFlight(flight.str());
              paxlstInfo.setDepPort(airp_dep.code_lat);
              paxlstInfo.setDepDateTime(scd_out_local);
              paxlstInfo.setArrPort(airp_arv.code_lat);
              paxlstInfo.setArrDateTime(scd_in_local);
            };
          };

        	int count=0;
      	  ostringstream body;
      	  PaxQry.SetVariable("point_arv",r->point_id);
      	  PaxQry.Execute();
      	  for(;!PaxQry.Eof;PaxQry.Next(),count++)
      	  {
            int pax_id=PaxQry.FieldAsInteger("pax_id");
            bool boarded=PaxQry.FieldAsInteger("pr_brd")!=0;
            TPaxStatus status=DecodePaxStatus(PaxQry.FieldAsString("status"));
            if (status==psCrew && !(fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US" || fmt=="EDI_UK")) continue;
            if (status!=psCrew && !boarded && final_apis) continue;

      	    Paxlst::PassengerInfo paxInfo;
      	    string airp_final_lat;
      	    if (fmt=="CSV_DE")
        	  {
        	    if (!PaxQry.FieldIsNULL("airp_final"))
        	    {
        	      TAirpsRow &airp_final = (TAirpsRow&)base_tables.get("airps").get_row("code",PaxQry.FieldAsString("airp_final"));
                if (airp_final.code_lat.empty()) throw Exception("airp_final.code_lat empty (code=%s)",airp_final.code.c_str());
                airp_final_lat=airp_final.code_lat;
              }
              else
                airp_final_lat=airp_arv.code_lat;
            };

            const CheckIn::TPaxDocItem doc;
            const CheckIn::TPaxDocoItem doco;
            const CheckIn::TPaxDocaItem docaD, docaR, docaB;
            LoadPaxDoc(pax_id, (CheckIn::TPaxDocItem&)doc);
            bool doco_exists=LoadPaxDoco(pax_id, (CheckIn::TPaxDocoItem&)doco);
            bool docaD_exists=false;
            bool docaR_exists=false;
            bool docaB_exists=false;
            if (fmt=="EDI_US")
            {
              docaD_exists=LoadPaxDoca(pax_id, CheckIn::docaDestination, (CheckIn::TPaxDocaItem&)docaD);
              docaR_exists=LoadPaxDoca(pax_id, CheckIn::docaResidence, (CheckIn::TPaxDocaItem&)docaR);
              docaB_exists=LoadPaxDoca(pax_id, CheckIn::docaBirth, (CheckIn::TPaxDocaItem&)docaB);
            };

    	      string doc_surname, doc_first_name, doc_second_name;
            if (!doc.surname.empty())
            {
              doc_surname=transliter(doc.surname,1,1);
      	  		doc_first_name=transliter(doc.first_name,1,1);
      	  		doc_second_name=transliter(doc.second_name,1,1);
            }
            else
            {
              //в терминалах до версии 201107-0126021 невозможен контроль и ввод фамилии из документа
              doc_surname=transliter(PaxQry.FieldAsString("surname"),1,1);
              doc_first_name=transliter(PaxQry.FieldAsString("name"),1,1);
            };
            if (fmt=="CSV_DE" || fmt=="TXT_EE")
            {
              doc_first_name=TruncNameTitles(doc_first_name.c_str());
              doc_second_name=TruncNameTitles(doc_second_name.c_str());
            };
            if (fmt=="EDI_CN" || fmt=="CSV_DE" || fmt=="TXT_EE")
            {
              if (!doc_second_name.empty())
              {
                doc_first_name+=" "+doc_second_name;
                doc_second_name.clear();
              };
            };

            int is_female=CheckIn::is_female(doc.gender, PaxQry.FieldAsString("name"));
            string gender;
            if (is_female!=NoExists)
            {
              gender=(is_female==0?"M":"F");
            }
            else
            {
              if (fmt=="CSV_CZ" || fmt=="EDI_CZ" || fmt=="EDI_US") gender = "M";//gender.clear();
              if (fmt=="CSV_DE" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_UK") gender = "U";
              if (fmt=="TXT_EE") gender = "N";
            };

    	    	string doc_type;
    	    	if (!doc.type.empty())
    	    	{
    	    	  TPaxDocTypesRow &doc_type_row = (TPaxDocTypesRow&)base_tables.get("pax_doc_types").get_row("code",doc.type);
    	    	  if (doc_type_row.code_lat.empty()) throw Exception("doc_type.code_lat empty (code=%s)",doc.type.c_str());
    	    	  doc_type=doc_type_row.code_lat;

              if (!APIS::isValidDocType(fmt, status, doc_type)) doc_type.clear();

    	    	  if (fmt=="CSV_DE")
    	    	  {
    	    	    if (doc_type!="P" && doc_type!="I") doc_type="P";
              };
              if (fmt=="TXT_EE")
    	    	  {
    	    	    if (doc_type!="P") doc_type.clear(); else doc_type="2";
    	    	  };
    	    	};
    	    	string nationality=doc.nationality;
    	    	string issue_country=doc.issue_country;
    	    	string birth_date;
    	    	if (doc.birth_date!=NoExists)
    	    	{
              if (fmt=="CSV_CZ")
    	    	    birth_date=DateTimeToStr(doc.birth_date,"ddmmmyy",true);
    	    	  if (fmt=="CSV_DE")
    	    	    birth_date=DateTimeToStr(doc.birth_date,"yymmdd",true);
    	    	  if (fmt=="TXT_EE")
    	    	    birth_date=DateTimeToStr(doc.birth_date,"dd.mm.yyyy",true);
    	    	};

    	    	string expiry_date;
    	    	if (doc.expiry_date!=NoExists)
    	    	{
    	    	  if (fmt=="CSV_CZ")
    	    	    expiry_date=DateTimeToStr(doc.expiry_date,"ddmmmyy",true);
    	    	};

            string doc_no=doc.no;
            if (fmt=="EDI_IN")
              doc_no=NormalizeDocNo(doc.no, true);
            if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_US" || fmt=="EDI_UK")
              doc_no=NormalizeDocNo(doc.no, false);

            if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US" || fmt=="EDI_UK")
            {
              paxInfo.setSurname(doc_surname);
              paxInfo.setFirstName(doc_first_name);
              paxInfo.setSecondName(doc_second_name);
              paxInfo.setSex(gender);

      	      if (doc.birth_date!=NoExists)
      	        paxInfo.setBirthDate(doc.birth_date);

              if (fmt=="EDI_US")
              {
                if (country_regul_dep!=US_CUSTOMS_CODE)
                  paxInfo.setCBPPort(airp_cbp.code_lat);
              };
      	      paxInfo.setDepPort(airp_dep.code_lat);
              paxInfo.setArrPort(airp_arv.code_lat);
              paxInfo.setNationality(nationality);

              if (status!=psCrew)
              {
                //PNR
                vector<TPnrAddrItem> pnrs;
                GetPaxPnrAddr(pax_id,pnrs);
                if (!pnrs.empty())
                  paxInfo.setReservNum(convert_pnr_addr(pnrs.begin()->addr, 1));
              };


              if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US" || fmt=="EDI_UK")
              {
                if (!doc_type.empty() && !doc_no.empty())
                {
                  //обязательно наличие типа и номера документа
                  paxInfo.setDocType(doc_type);
                  paxInfo.setDocNumber(doc_no);
                }
              }
              else
              {
                if (!doc_no.empty())
                {
                  paxInfo.setDocType(doc_type);
                  paxInfo.setDocNumber(doc_no);
                };
              };
              if (doc.expiry_date!=NoExists)
                paxInfo.setDocExpirateDate(doc.expiry_date);
              paxInfo.setDocCountry(issue_country);
            };

            if (fmt=="CSV_CZ")
      	    {
      	    	body << doc_surname << ";"
      	  		     << doc_first_name << ";"
      	  		     << doc_second_name << ";"
      	  		     << birth_date << ";"
      	  		     << gender << ";"
      	  		     << nationality << ";"
      	  		     << doc_type << ";"
      	  		     << doc_no << ";"
      	  		     << expiry_date << ";"
      	  		     << issue_country << ";";
      	  	};
      	  	if (fmt=="CSV_DE")
      	    {
      	      body << doc_surname << ";"
      	           << doc_first_name << ";"
      	           << gender << ";"
      	           << birth_date << ";"
      	           << nationality << ";"
                   << airp_arv.code_lat << ";"
    	  		       << airp_dep.code_lat << ";"
    	  		       << airp_final_lat << ";"
    	  		       << doc_type << ";"
    	  		       << convert_char_view(doc_no,true) << ";"
    	  		       << issue_country;
      	    };
      	    if (fmt=="TXT_EE")
      	    {
              body << "1# " << count+1 << ENDL
    	  		       << "2# " << doc_surname << ENDL
    	  		       << "3# " << doc_first_name << ENDL
    	  		       << "4# " << birth_date << ENDL
    	  		       << "5# " << nationality << ENDL
    	  		       << "6# " << doc_type << ENDL
    	  		       << "7# " << convert_char_view(doc_no,true) << ENDL
    	  		       << "8# " << issue_country << ENDL
    	  		       << "9# " << gender << ENDL;
      	    };
      	    if (doco_exists && (fmt=="CSV_DE" || fmt=="TXT_EE"))
      	  	{
      	  	  //виза пассажира найдена
        	    string doco_type;
      	    	if (!doco.type.empty())
      	    	{
      	    	  TPaxDocTypesRow &doco_type_row = (TPaxDocTypesRow&)base_tables.get("pax_doc_types").get_row("code",doco.type);
      	    	  if (doco_type_row.code_lat.empty()) throw Exception("doco_type.code_lat empty (code=%s)",doco.type.c_str());
      	    	  doco_type=doco_type_row.code_lat;
      	    	};
      	    	string applic_country=doco.applic_country;

      	      if (fmt=="CSV_DE")
        	    {
        	      body << ";"
                     << doco_type << ";"
                     << convert_char_view(doco.no,true) << ";"
                     << applic_country;
              };

              if (fmt=="TXT_EE")
              {
                body << "10# " << ENDL
                     << "11# " << (doco_type=="V"?convert_char_view(doco.no,true):"") << ENDL;
              };
            }
            else
            {
              if (fmt=="TXT_EE")
              {
                body << "10# " << ENDL
                     << "11# " << ENDL;
              };
            };

            if (docaD_exists && (fmt=="EDI_US"))
            {
              if (status!=psCrew)
              {
                paxInfo.setStreet(docaD.address);
                paxInfo.setCity(docaD.city);
                if (country_arv.code_lat!="US" || docaD.region.size()==2) //код штата для US
                  paxInfo.setCountrySubEntityCode(docaD.region);
                paxInfo.setPostalCode(docaD.postal_code);
                paxInfo.setDestCountry(docaD.country);
              };
            };

            if (docaR_exists && (fmt=="EDI_US"))
            {
              paxInfo.setResidCountry(docaR.country);
              if (status==psCrew)
              {
                paxInfo.setStreet(docaR.address);
                paxInfo.setCity(docaR.city);
                paxInfo.setCountrySubEntityCode(docaR.region);
                paxInfo.setPostalCode(docaR.postal_code);
                paxInfo.setDestCountry(docaR.country);
              };
            };

            if (docaB_exists && (fmt=="EDI_US"))
            {
              if (status==psCrew)
              {
                paxInfo.setBirthCity(docaB.city);
                paxInfo.setBirthRegion(docaB.region);
                paxInfo.setBirthCountry(docaB.country);
              };
            };

            if (fmt=="CSV_CZ" || fmt=="CSV_DE")
              body << ENDL;

            if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US" || fmt=="EDI_UK")
            {
              if (status!=psCrew)
      	        FPM.addPassenger( paxInfo );
              else
                FCM.addPassenger( paxInfo );
            };
      	  }; //цикл по пассажирам

          vector< pair<string, string> > files;

          if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US" || fmt=="EDI_UK")
          {
            for(int pass=0; pass<2; pass++)
            {
              Paxlst::PaxlstInfo& paxlstInfo=(pass==0?FPM:FCM);

              if (!(task_name.empty() ||
                    !use_us_customs_tasks ||
                    (task_name==BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL && pass==0) ||
                    (task_name==BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL && pass!=0))) continue;

              if (!paxlstInfo.passengersList().empty())
              {
                vector<string> parts;
                string file_extension;
                if (fmt=="EDI_CZ")
                {
                  file_extension="TXT";
                  parts.push_back(paxlstInfo.toEdiString());
                };
                if (fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US" || fmt=="EDI_UK")
                {
                  file_extension=(pass==0?"FPM":"FCM");
                  for(unsigned maxPaxPerString=MAX_PAX_PER_EDI_PART;maxPaxPerString>0;maxPaxPerString--)
                  {
                    parts=paxlstInfo.toEdiStrings(maxPaxPerString);
                    vector<string>::const_iterator p=parts.begin();
                    for(; p!=parts.end(); ++p)
                      if (p->size()>MAX_LEN_OF_EDI_PART) break;
                    if (p==parts.end()) break;
                  };
                };

                int part_num=parts.size()>1?1:0;
                for(vector<string>::const_iterator p=parts.begin(); p!=parts.end(); ++p, part_num++)
                {
                  ostringstream file_name;
                  file_name << ApisSetsQry.FieldAsString("dir")
                            << "/"
                            << Paxlst::createEdiPaxlstFileName(airline.code_lat,
                                                               flt_no,
                                                               suffix,
                                                               airp_dep.code_lat,
                                                               airp_arv.code_lat,
                                                               scd_out_local,
                                                               file_extension,
                                                               part_num);
                  files.push_back( make_pair(file_name.str(), *p) );
                };
              };
            };
      	  }
      	  else
          {
            if (task_name.empty() ||
                !use_us_customs_tasks ||
                task_name==BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL)
            {
        	    ostringstream file_name;
        	    if (fmt=="CSV_CZ" || fmt=="CSV_DE")
              {
                ostringstream f;
                f << airline.code_lat << flt_no << suffix;
              	file_name << ApisSetsQry.FieldAsString("dir")
                          << "/"
                          << airline.code_lat
                          << (f.str().size()<6?string(6-f.str().size(),'0'):"") << flt_no << suffix  //по стандарту поле не должно превышать 6 символов
              	          << airp_dep.code_lat
              	          << airp_arv.code_lat
              	          << DateTimeToStr(scd_out_local,"yyyymmdd") << ".CSV";
              };

              if (fmt=="TXT_EE")
                file_name << ApisSetsQry.FieldAsString("dir")
                          << "/"
                          << "LL-" << airline.code_lat << setw(3) << setfill('0') << flt_no << suffix
                          << "-" << DateTimeToStr(scd_in_local,"ddmmyyyy-hhnn") << "-S.TXT";

              //доклеиваем заголовочную часть
              ostringstream header;
              if (fmt=="CSV_CZ")
              	header << "csv;ROSSIYA;"
              	    	 << airline.code_lat << setw(3) << setfill('0') << flt_no << suffix << ";"
            	      	 << airp_dep.code_lat << ";" << DateTimeToStr(scd_out_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
            	      	 << airp_arv.code_lat << ";" << DateTimeToStr(scd_in_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
            	      	 << count << ";" << ENDL;
            	if (fmt=="CSV_DE")
                header << airline.code_lat << ";"
                    	 << airline.code_lat << setw(3) << setfill('0') << flt_no << ";"
                    	 << airp_dep.code_lat << ";" << DateTimeToStr(scd_out_local,"yymmddhhnn") << ";"
                    	 << airp_arv.code_lat << ";" << DateTimeToStr(scd_in_local,"yymmddhhnn") << ";"
                    	 << count << ENDL;
              if (fmt=="TXT_EE")
              {
                string airline_name=airline.short_name_lat;
                if (airline_name.empty())
                  airline_name=airline.name_lat;
                if (airline_name.empty())
                  airline_name=airline.code_lat;

                header << "1$ " << airline_name << ENDL
                    	 << "2$ " << ENDL
                    	 << "3$ " << airline_country << ENDL
                    	 << "4$ " << ENDL
                    	 << "5$ " << ENDL
                    	 << "6$ " << ENDL
                    	 << "7$ " << ENDL
                    	 << "8$ " << ENDL
                    	 << "9$ " << DateTimeToStr(scd_in_local,"dd.mm.yy hh:nn") << ENDL
                    	 << "10$ " << (airp_arv.code=="TLL"?"Tallinna Lennujaama piiripunkt":
                         	          airp_arv.code=="TAY"?"Tartu piiripunkt":
                             	      airp_arv.code=="URE"?"Kuressaare-2 piiripunkt":
                                 	  airp_arv.code=="KDL"?"Kardla Lennujaama piiripunkt":"") << ENDL
                    	 << "11$ " << ENDL
                    	 << "1$ " << airline.code_lat << setw(3) << setfill('0') << flt_no << suffix << ENDL
                    	 << "2$ " << ENDL
                    	 << "3$ " << count << ENDL;
              };
              files.push_back( make_pair(file_name.str(), string(header.str()).append(body.str())) );
            };
      	  };

          if (!files.empty())
          {
            for(vector< pair<string, string> >::const_iterator iFile=files.begin();iFile!=files.end();++iFile)
            {
            	ofstream f;
              f.open(iFile->first.c_str());
              if (!f.is_open()) throw Exception("Can't open file '%s'",iFile->first.c_str());
              try
              {
              	f << iFile->second;
              	f.close();
              }
              catch(...)
              {
                try { f.close(); } catch( ... ) { };
                try
                {
                  //в случае ошибки запишем пустой файл
                  f.open(iFile->first.c_str());
                  if (f.is_open()) f.close();
                }
                catch( ... ) { };
                throw;
              };
            };

            ostringstream msg;
            msg << "Сформирован APIS формата " << fmt
                << ": " << country_dep << "(" << airp_dep.code << ")"
                << "->" << country_arv.code << "(" << airp_arv.code << ")";
            TReqInfo::Instance()->MsgToLog(msg.str(),evtFlt,point_id);

            result=true;
          };
        };
      };

    };
  }
  catch(Exception &E)
  {
    throw Exception("create_apis_file: %s",E.what());
  };
  return result;
};

void create_apis_task(int point_id, const std::string& task_name)
{
  create_apis_file(point_id, task_name);
};

